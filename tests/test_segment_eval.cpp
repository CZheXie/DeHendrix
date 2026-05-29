#include "deobf/segment_eval.h"
#include "deobf/passes.h"

#include <cstdlib>
#include <iostream>
#include <vector>

using namespace deobf;

// assert() is compiled out under NDEBUG (Release), which would make these tests
// silently pass. Use an always-active check that exits non-zero on failure.
static int g_fail = 0;
#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::cerr << "  FAIL: " #cond " @ line " << __LINE__ << "\n";    \
            g_fail = 1;                                                      \
        }                                                                    \
    } while (0)

// ---------------------------------------------------------------------------
// VJCC dual-target extraction (pure IR, no disassembler).
// Models a branchless VJCC:  vpc = base + (cond * delta)
//   cond = 1 -> base + delta ;  cond = 0 -> base
// ---------------------------------------------------------------------------
static void test_vjcc_extract() {
    Function f("vjcc");
    f.emit(Op::CONST, {Const(0x1000)});            // %0 vpc base
    f.emit(Op::AND,   {SymReg("flag"), Const(1)}); // %1 condition
    f.emit(Op::MUL,   {InstrRef(1), Const(0x10)}); // %2 cond * delta
    f.emit(Op::ADD,   {InstrRef(0), InstrRef(2)}); // %3 vpc

    auto res = extract_vjcc_targets(f, InstrRef(3), InstrRef(1));
    CHECK(res.has_value());
    if (res) {
        CHECK(res->first == 0x1010);
        CHECK(res->second == 0x1000);
        std::cout << "  ok: VJCC extract -> taken=0x" << std::hex << res->first
                  << " not_taken=0x" << res->second << std::dec << "\n";
    }

    // Condition-independent VPC must NOT be treated as a branch.
    Function g("novjcc");
    g.emit(Op::CONST, {Const(0x2000)});            // %0
    g.emit(Op::AND,   {SymReg("flag"), Const(1)}); // %1 (unused by vpc)
    g.emit(Op::ADD,   {InstrRef(0), Const(8)});    // %2 vpc = base+8 (no cond)
    auto none = extract_vjcc_targets(g, InstrRef(2), InstrRef(1));
    CHECK(!none.has_value());
    std::cout << "  ok: cond-independent VPC -> no false branch\n";
}

#ifdef DEOBF_HAS_CAPSTONE
static uint64_t rax_val(const SegmentRun& r, bool& ok) {
    ok = false;
    auto it = r.exit_regs.find("rax");
    if (it == r.exit_regs.end()) return 0;
    const Const* c = get_const(it->second);
    if (!c) return 0;
    ok = true;
    return c->value;
}

static void test_straightline() {
    // mov rax,5 ; mov rcx,7 ; add rax,rcx ; ret
    std::vector<uint8_t> img = {
        0x48,0xC7,0xC0,0x05,0x00,0x00,0x00,
        0x48,0xC7,0xC1,0x07,0x00,0x00,0x00,
        0x48,0x01,0xC8,
        0xC3
    };
    ByteMemory mem;
    auto r = lift_and_optimize_segment(img.data(), img.size(), 0x140000000,
                                       0x140000000, {}, mem);
    CHECK(r.stop == SegStop::Ret);
    bool ok = false;
    uint64_t rax = rax_val(r, ok);
    CHECK(ok);
    CHECK(rax == 12);
    std::cout << "  ok: straight-line mov/add/ret -> stop=" << seg_stop_name(r.stop)
              << " rax=" << rax << "\n";
}

static void test_injected_state() {
    // add rax,5 ; ret      (rax injected = 100 -> 105)
    std::vector<uint8_t> img = { 0x48,0x83,0xC0,0x05, 0xC3 };
    ByteMemory mem;
    std::unordered_map<std::string, Value> in = {{"rax", Const(100)}};
    auto r = lift_and_optimize_segment(img.data(), img.size(), 0x140000000,
                                       0x140000000, in, mem);
    CHECK(r.stop == SegStop::Ret);
    bool ok = false;
    uint64_t rax = rax_val(r, ok);
    CHECK(ok);
    CHECK(rax == 105);
    std::cout << "  ok: injected rax=100 + add 5 -> rax=" << rax << "\n";
}
#endif

int main() {
    std::cout << "=== libdeobf segment-eval tests ===\n";
    test_vjcc_extract();
#ifdef DEOBF_HAS_CAPSTONE
    test_straightline();
    test_injected_state();
#endif
    if (g_fail) {
        std::cout << "\nSEGMENT-EVAL TESTS FAILED\n";
        return 1;
    }
    std::cout << "\nAll segment-eval tests passed!\n";
    return 0;
}
