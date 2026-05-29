#include "deobf/evaluator.h"
#include "deobf/segment_eval.h"
#include "deobf/ir.h"

#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <vector>

using namespace deobf;

static int g_fail = 0;
#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::cerr << "  FAIL: " #cond " @ line " << __LINE__ << "\n";    \
            g_fail = 1;                                                      \
        }                                                                    \
    } while (0)

#ifdef DEOBF_HAS_CAPSTONE

static void put(std::vector<uint8_t>& img, uint64_t rva, std::initializer_list<uint8_t> bytes) {
    size_t i = rva;
    for (auto b : bytes) img[i++] = b;
}
static void put_qword(std::vector<uint8_t>& img, uint64_t rva, uint64_t v) {
    for (int i = 0; i < 8; ++i) img[rva + i] = static_cast<uint8_t>(v >> (8 * i));
}

// Direct-threaded micro-VM:
//   entry: r11 = &bytecode
//   dispatcher: rax = [r11]; r11 += 8; jmp rax
//   handler_add: rbx += 1; jmp dispatcher
//   handler_exit: ret
// bytecode = [add, add, exit]
static void test_threaded_vm() {
    std::vector<uint8_t> img(0x4000, 0xCC);

    put(img, 0x1000, {0x49,0xBB,0x00,0x30,0x00,0x40,0x01,0x00,0x00,0x00}); // movabs r11, 0x140003000
    put(img, 0x100A, {0x49,0x8B,0x03});                                    // mov rax, [r11]
    put(img, 0x100D, {0x49,0x83,0xC3,0x08});                               // add r11, 8
    put(img, 0x1011, {0xFF,0xE0});                                         // jmp rax

    put(img, 0x1100, {0x48,0x83,0xC3,0x01});                               // add rbx, 1
    put(img, 0x1104, {0xE9,0x01,0xFF,0xFF,0xFF});                          // jmp 0x14000100A

    put(img, 0x1200, {0xC3});                                              // ret

    put_qword(img, 0x3000, 0x140001100);
    put_qword(img, 0x3008, 0x140001100);
    put_qword(img, 0x3010, 0x140001200);

    GuidedEvaluator ev(img.data(), img.size(), 0x140000000, "r11");
    ev.add_safe_range(0x140003000, 0x140003018);
    auto rep = ev.run(0x140001000, 5000, 50);

    std::cout << "  result=" << eval_result_name(rep.result)
              << " vm_insns=" << rep.vm_insn_count
              << " dispatch_steps=" << ev.jmp_trace().size()
              << " ir=" << rep.ir_count << "\n";

    // The engine must traverse the dispatcher loop (not stall at the first
    // indirect jump) and reach the VM exit handler's ret.
    CHECK(rep.result == EvalResult::VMEXIT_RET || rep.result == EvalResult::RET);
    // entry -> add -> disp -> add -> disp -> exit  == 6 native blocks walked.
    CHECK(ev.jmp_trace().size() >= 5);
    // the exit handler must have been reached.
    bool reached_exit = false;
    for (auto v : ev.jmp_trace()) if (v == 0x140001200) reached_exit = true;
    CHECK(reached_exit);
    if (reached_exit)
        std::cout << "  ok: traversed [add,add,exit] -> VMEXIT at handler 0x140001200\n";
}

// Same VM but the first bytecode op is a cmov-based virtual conditional jump
// (VJCC) that selects the next VPC from a flag. The engine traverses to the
// VJCC; extract_vjcc_targets must recover BOTH next-VPC values from the real
// execution trace.
static void test_threaded_vm_vjcc() {
    std::vector<uint8_t> img(0x4000, 0xCC);

    // entry + dispatcher
    put(img, 0x1000, {0x49,0xBB,0x00,0x30,0x00,0x40,0x01,0x00,0x00,0x00}); // movabs r11, 0x140003000
    put(img, 0x100A, {0x49,0x8B,0x03});                                    // mov rax,[r11]
    put(img, 0x100D, {0x49,0x83,0xC3,0x08});                               // add r11, 8
    put(img, 0x1011, {0xFF,0xE0});                                         // jmp rax

    put(img, 0x1200, {0xC3});                                              // handler_exit: ret

    // handler_vjcc: r11 = (rbx != 0) ? 0x140003020 : 0x140003010 ; jmp dispatcher
    put(img, 0x1300, {0x48,0x85,0xDB});                                    // test rbx, rbx
    put(img, 0x1303, {0x48,0xB8,0x10,0x30,0x00,0x40,0x01,0x00,0x00,0x00}); // movabs rax, 0x140003010
    put(img, 0x130D, {0x48,0xB9,0x20,0x30,0x00,0x40,0x01,0x00,0x00,0x00}); // movabs rcx, 0x140003020
    put(img, 0x1317, {0x48,0x0F,0x45,0xC1});                               // cmovne rax, rcx
    put(img, 0x131B, {0x49,0x89,0xC3});                                    // mov r11, rax
    put(img, 0x131E, {0xE9,0xE7,0xFC,0xFF,0xFF});                          // jmp 0x14000100A

    put_qword(img, 0x3000, 0x140001300); // op0 = vjcc
    put_qword(img, 0x3010, 0x140001200); // taken arm bytecode -> exit
    put_qword(img, 0x3020, 0x140001200); // not-taken arm bytecode -> exit

    GuidedEvaluator ev(img.data(), img.size(), 0x140000000, "r11");
    ev.add_safe_range(0x140003000, 0x140003028);
    auto rep = ev.run(0x140001000, 5000, 50);
    std::cout << "  result=" << eval_result_name(rep.result)
              << " (expect UNRESOLVED_TARGET at the VJCC dispatch)\n";

    // Find the SELECT produced by the cmov in the recovered IR.
    const Function& f = ev.func();
    const Instruction* sel = nullptr;
    for (auto& in : f.instrs) if (in.op == Op::SELECT) sel = &in;
    CHECK(sel != nullptr);
    if (sel) {
        auto tg = extract_vjcc_targets(f, sel->ref(), sel->operands[0]);
        CHECK(tg.has_value());
        if (tg) {
            uint64_t a = tg->first, b = tg->second;
            bool ok = (a == 0x140003010 && b == 0x140003020) ||
                      (a == 0x140003020 && b == 0x140003010);
            CHECK(ok);
            std::cout << "  ok: VM cmov-VJCC -> next VPCs {0x" << std::hex
                      << a << ", 0x" << b << "}" << std::dec << "\n";

            // Step B core mechanic: resume each arm by injecting its VPC into
            // r11 and re-entering the dispatcher. Both arms must reach VMEXIT.
            // The dispatcher VA is where the unresolved dispatch stopped.
            uint64_t disp_va = rep.stop_va;
            for (uint64_t arm : {a, b}) {
                GuidedEvaluator ev2(img.data(), img.size(), 0x140000000, "r11");
                ev2.add_safe_range(0x140003000, 0x140003028);
                ev2.set_initial_regs({{"r11", Const(arm)}});
                auto r2 = ev2.run(disp_va, 5000, 50);
                bool exited = (r2.result == EvalResult::VMEXIT_RET ||
                               r2.result == EvalResult::RET);
                CHECK(exited);
                std::cout << "    resume arm 0x" << std::hex << arm << " from disp 0x"
                          << disp_va << " -> " << std::dec << eval_result_name(r2.result) << "\n";
            }
        }
    }
}

#endif // DEOBF_HAS_CAPSTONE

int main() {
    std::cout << "=== libdeobf synthetic threaded-code VM ===\n";
#ifdef DEOBF_HAS_CAPSTONE
    test_threaded_vm();
    test_threaded_vm_vjcc();
#else
    std::cout << "  (skipped: no capstone)\n";
#endif
    if (g_fail) {
        std::cout << "\nVM-SYNTH TESTS FAILED\n";
        return 1;
    }
    std::cout << "\nAll VM-synth tests passed!\n";
    return 0;
}
