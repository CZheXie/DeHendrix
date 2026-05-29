#include "deobf/segment_eval.h"
#include "deobf/cfg.h"
#include "deobf/passes.h"

#include <cstdlib>
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

static const BasicBlock* block_with_n_preds(const CFGFunction& f, size_t n) {
    for (auto& bb : f.blocks)
        if (f.predecessors(bb.id).size() == n) return &bb;
    return nullptr;
}

static const PhiNode* find_phi(const BasicBlock& bb, const std::string& reg) {
    for (auto& p : bb.phis)
        if (p.reg == reg) return &p;
    return nullptr;
}

static bool has_self_backedge(const CFGFunction& f, uint32_t id) {
    for (auto& e : f.edges)
        if (e.from == id && e.to == id && e.is_backedge) return true;
    return false;
}

// ---------------------------------------------------------------------------
// Diamond:
//   mov rax,1 ; cmp rcx,0 ; je L_else
//   L_then: mov rax,10 ; jmp L_merge
//   L_else: mov rax,20
//   L_merge: ret
// rax differs across arms -> phi at merge; rcx is symbolic/invariant -> none.
// ---------------------------------------------------------------------------
static void test_diamond() {
    std::vector<uint8_t> img = {
        0x48,0xC7,0xC0,0x01,0x00,0x00,0x00, // 0x00 mov rax,1
        0x48,0x83,0xF9,0x00,                // 0x07 cmp rcx,0
        0x74,0x09,                          // 0x0B je  0x16
        0x48,0xC7,0xC0,0x0A,0x00,0x00,0x00, // 0x0D mov rax,10  (fallthrough/not-taken)
        0xEB,0x07,                          // 0x14 jmp 0x1D
        0x48,0xC7,0xC0,0x14,0x00,0x00,0x00, // 0x16 mov rax,20  (taken)
        0xC3                                // 0x1D ret
    };
    const uint64_t base = 0x400000;
    CFGFunction f = recover_native_cfg(img.data(), img.size(), base, base);

    CHECK(f.blocks.size() == 4);
    const BasicBlock* merge = block_with_n_preds(f, 2);
    CHECK(merge != nullptr);
    if (merge) {
        CHECK(merge->term.kind == TermKind::RET);
        const PhiNode* rax = find_phi(*merge, "rax");
        CHECK(rax != nullptr);
        if (rax) {
            CHECK(rax->incoming.size() == 2);
            bool s10 = false, s20 = false;
            for (auto& [bid, val] : rax->incoming) {
                const Const* c = get_const(val);
                CHECK(c != nullptr);
                if (c && c->value == 10) s10 = true;
                if (c && c->value == 20) s20 = true;
            }
            CHECK(s10 && s20);
        }
        CHECK(find_phi(*merge, "rcx") == nullptr);
    }
    std::cout << "  " << (g_fail ? "FAIL" : "ok")
              << ": native diamond -> merge phi rax{10,20}, blocks=" << f.blocks.size() << "\n";
    if (!g_fail) std::cout << f.dump() << "\n";
}

// ---------------------------------------------------------------------------
// Loop:
//   mov rax,0 ; mov rcx,3
//   L_head: add rax,rcx ; dec rcx ; jne L_head
//   ret
// Loop header has two preds (entry + self back-edge) and loop-carried phis.
// ---------------------------------------------------------------------------
static void test_loop() {
    std::vector<uint8_t> img = {
        0x48,0xC7,0xC0,0x00,0x00,0x00,0x00, // 0x00 mov rax,0
        0x48,0xC7,0xC1,0x03,0x00,0x00,0x00, // 0x07 mov rcx,3
        0x48,0x01,0xC8,                     // 0x0E add rax,rcx   [L_head]
        0x48,0xFF,0xC9,                     // 0x11 dec rcx
        0x75,0xF8,                          // 0x14 jne 0x0E
        0xC3                                // 0x16 ret
    };
    const uint64_t base = 0x400000;
    CFGFunction f = recover_native_cfg(img.data(), img.size(), base, base);

    CHECK(f.blocks.size() == 3);
    const BasicBlock* head = block_with_n_preds(f, 2);
    CHECK(head != nullptr);
    if (head) {
        CHECK(head->term.kind == TermKind::CJMP);
        CHECK(has_self_backedge(f, head->id));
        // loop-carried registers should get phi nodes
        CHECK(!head->phis.empty());
    }
    std::cout << "  " << (g_fail ? "FAIL" : "ok")
              << ": native loop -> self back-edge + loop-carried phi, blocks=" << f.blocks.size() << "\n";
    if (!g_fail) std::cout << f.dump() << "\n";
}

#endif // DEOBF_HAS_CAPSTONE

int main() {
    std::cout << "=== libdeobf native-CFG recovery tests ===\n";
#ifdef DEOBF_HAS_CAPSTONE
    test_diamond();
    test_loop();
#else
    std::cout << "  (skipped: built without capstone)\n";
#endif
    if (g_fail) {
        std::cout << "\nNATIVE-CFG TESTS FAILED\n";
        return 1;
    }
    std::cout << "\nAll native-CFG tests passed!\n";
    return 0;
}
