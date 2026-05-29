#include "deobf/cfg_recovery.h"
#include "deobf/cfg.h"

#ifdef NDEBUG
#undef NDEBUG   // keep assert() active in Release — these are correctness tests
#endif
#include <cassert>
#include <iostream>

using namespace deobf;

// Global id allocator so every emitted instruction has a unique result_id
// across all segments (the contract CFGRecovery relies on).
static uint32_t g_next_id = 1;

static Instruction mk_instr(Op op, std::vector<Value> operands) {
    Instruction in;
    in.op = op;
    in.operands = std::move(operands);
    in.result_id = g_next_id++;
    return in;
}

static const Const* as_const(const Value& v) { return get_const(v); }

static bool has_edge(const CFGFunction& f, uint32_t from, uint32_t to, bool* backedge = nullptr) {
    for (auto& e : f.edges) {
        if (e.from == from && e.to == to) {
            if (backedge) *backedge = e.is_backedge;
            return true;
        }
    }
    return false;
}

static const PhiNode* find_phi(const BasicBlock& bb, const std::string& reg) {
    for (auto& p : bb.phis)
        if (p.reg == reg) return &p;
    return nullptr;
}

// ---------------------------------------------------------------------------
// Scenario 1: diamond.  0x100 branches to 0x200 / 0x300, both merge at 0x400.
//   rax differs across the two arms  -> phi expected at the merge.
//   rbx is identical across both arms -> no phi.
// ---------------------------------------------------------------------------
static void test_diamond() {
    g_next_id = 1;

    SegmentOracle oracle = [](uint64_t vpc, const VMState&) -> SegmentResult {
        SegmentResult r;
        switch (vpc) {
            case 0x100:
                r.kind = SegmentKind::Branch;
                r.instrs.push_back(mk_instr(Op::AND, {SymReg("a"), SymReg("b")}));
                r.condition = SymReg("flag");
                r.vpc_taken = 0x200;
                r.vpc_not_taken = 0x300;
                r.out_state_taken.regs = {{"rax", Const(1)}, {"rbx", Const(7)}};
                r.out_state_not_taken.regs = {{"rax", Const(2)}, {"rbx", Const(7)}};
                return r;
            case 0x200:
                r.kind = SegmentKind::Goto;
                r.instrs.push_back(mk_instr(Op::ADD, {SymReg("rax"), Const(0)}));
                r.next_vpc = 0x400;
                r.out_state.regs = {{"rax", Const(1)}, {"rbx", Const(7)}};
                return r;
            case 0x300:
                r.kind = SegmentKind::Goto;
                r.instrs.push_back(mk_instr(Op::ADD, {SymReg("rax"), Const(0)}));
                r.next_vpc = 0x400;
                r.out_state.regs = {{"rax", Const(2)}, {"rbx", Const(7)}};
                return r;
            case 0x400:
                r.kind = SegmentKind::Ret;
                r.ret_value = SymReg("rax");
                return r;
        }
        return r; // Unresolved
    };

    CFGRecoveryConfig cfg;
    cfg.entry_vpc = 0x100;
    CFGFunction f = CFGRecovery::recover(oracle, cfg);

    assert(f.blocks.size() == 4);
    assert(f.edges.size() == 4);

    const BasicBlock* entry = f.block_by_id(f.entry_block);
    assert(entry && entry->term.kind == TermKind::CJMP);

    // Resolve block ids by walking the (only) successors.
    auto succs = f.successors(f.entry_block);
    assert(succs.size() == 2);

    // Merge block = the one with two predecessors.
    const BasicBlock* merge = nullptr;
    for (auto& bb : f.blocks)
        if (f.predecessors(bb.id).size() == 2) merge = &bb;
    assert(merge && "expected a merge block with 2 preds");
    assert(merge->term.kind == TermKind::RET);

    const PhiNode* rax_phi = find_phi(*merge, "rax");
    assert(rax_phi && "expected phi for rax at merge");
    assert(rax_phi->incoming.size() == 2);
    // Values should be Const(1) and Const(2) (order follows edge discovery).
    bool saw1 = false, saw2 = false;
    for (auto& [bid, val] : rax_phi->incoming) {
        auto* c = as_const(val);
        assert(c);
        if (c->value == 1) saw1 = true;
        if (c->value == 2) saw2 = true;
    }
    assert(saw1 && saw2);

    assert(find_phi(*merge, "rbx") == nullptr && "rbx is identical -> no phi");

    std::cout << "  PASS: diamond -> single rax phi at merge, no rbx phi\n";
    std::cout << f.dump() << "\n";
}

// ---------------------------------------------------------------------------
// Scenario 2: loop.  0x10 -> 0x20(header); header branches to 0x30(body)/
//   0x40(exit); body -> 0x20 (back-edge).  `i` is loop-carried (0 then 99) ->
//   phi at the header; `x` is invariant -> no phi.
// ---------------------------------------------------------------------------
static void test_loop() {
    g_next_id = 1;

    SegmentOracle oracle = [](uint64_t vpc, const VMState&) -> SegmentResult {
        SegmentResult r;
        switch (vpc) {
            case 0x10:
                r.kind = SegmentKind::Goto;
                r.instrs.push_back(mk_instr(Op::CONST, {Const(0)}));
                r.next_vpc = 0x20;
                r.out_state.regs = {{"i", Const(0)}, {"x", Const(5)}};
                return r;
            case 0x20:
                r.kind = SegmentKind::Branch;
                r.instrs.push_back(mk_instr(Op::SUB, {SymReg("i"), Const(10)}));
                r.condition = SymReg("c");
                r.vpc_taken = 0x30;
                r.vpc_not_taken = 0x40;
                r.out_state_taken.regs = {{"i", SymReg("i")}, {"x", Const(5)}};
                r.out_state_not_taken.regs = {{"i", SymReg("i")}, {"x", Const(5)}};
                return r;
            case 0x30:
                r.kind = SegmentKind::Goto;
                r.instrs.push_back(mk_instr(Op::ADD, {SymReg("i"), Const(1)}));
                r.next_vpc = 0x20; // back-edge
                r.out_state.regs = {{"i", Const(99)}, {"x", Const(5)}};
                return r;
            case 0x40:
                r.kind = SegmentKind::Ret;
                r.ret_value = SymReg("x");
                return r;
        }
        return r;
    };

    CFGRecoveryConfig cfg;
    cfg.entry_vpc = 0x10;
    CFGFunction f = CFGRecovery::recover(oracle, cfg);

    assert(f.blocks.size() == 4);

    // Identify header: the block with two predecessors.
    const BasicBlock* header = nullptr;
    for (auto& bb : f.blocks)
        if (f.predecessors(bb.id).size() == 2) header = &bb;
    assert(header && "expected loop header with 2 preds");
    assert(header->term.kind == TermKind::CJMP);

    // The body is the predecessor reached via a back-edge.
    bool found_backedge = false;
    for (auto& e : f.edges)
        if (e.to == header->id && e.is_backedge) found_backedge = true;
    assert(found_backedge && "expected a back-edge into the header");

    const PhiNode* i_phi = find_phi(*header, "i");
    assert(i_phi && "expected phi for loop-carried i");
    assert(i_phi->incoming.size() == 2);
    bool saw0 = false, saw99 = false;
    for (auto& [bid, val] : i_phi->incoming) {
        auto* c = as_const(val);
        assert(c);
        if (c->value == 0) saw0 = true;
        if (c->value == 99) saw99 = true;
    }
    assert(saw0 && saw99);

    assert(find_phi(*header, "x") == nullptr && "x is invariant -> no phi");

    std::cout << "  PASS: loop -> back-edge + phi for i, no phi for x\n";
    std::cout << f.dump() << "\n";
}

// ---------------------------------------------------------------------------
// Full-SSA: a counter loop. The body computes i = i + 1 using its symbolic
// entry value; with full_ssa the body's increment must reference the loop
// header's phi result (closed def-use), not a pinned predecessor constant.
// ---------------------------------------------------------------------------
static void test_full_ssa_loop() {
    g_next_id = 1;

    SegmentOracle oracle = [](uint64_t vpc, const VMState& in) -> SegmentResult {
        SegmentResult r;
        auto iv = [&]() -> Value {
            const Value* p = in.find("i");
            return p ? *p : Value(SymReg("i"));
        };
        switch (vpc) {
            case 0x10:  // entry: i = 0
                r.kind = SegmentKind::Goto;
                r.next_vpc = 0x20;
                r.out_state.regs = {{"i", Const(0)}};
                return r;
            case 0x20:  // header: branch, pass i through to both arms
                r.kind = SegmentKind::Branch;
                r.condition = SymReg("c");
                r.vpc_taken = 0x30;
                r.vpc_not_taken = 0x40;
                r.out_state_taken.regs = {{"i", iv()}};
                r.out_state_not_taken.regs = {{"i", iv()}};
                return r;
            case 0x30: {  // body: i = i + 1
                Instruction add = mk_instr(Op::ADD, {iv(), Const(1)});
                r.instrs.push_back(add);
                r.kind = SegmentKind::Goto;
                r.next_vpc = 0x20;  // back-edge
                r.out_state.regs = {{"i", InstrRef(add.result_id)}};
                return r;
            }
            case 0x40:
                r.kind = SegmentKind::Ret;
                return r;
        }
        return r;
    };

    CFGRecoveryConfig cfg;
    cfg.entry_vpc = 0x10;
    cfg.full_ssa = true;
    cfg.tracked_regs = {"i"};
    CFGFunction f = CFGRecovery::recover(oracle, cfg);

    const BasicBlock* header = nullptr;
    const PhiNode* iphi = nullptr;
    for (auto& bb : f.blocks) {
        const PhiNode* p = find_phi(bb, "i");
        if (p) { header = &bb; iphi = p; }
    }
    assert(header && iphi && "expected a loop-header phi for i");
    assert(iphi->incoming.size() == 2);

    const Instruction* add = nullptr;
    for (auto& bb : f.blocks)
        for (auto& in : bb.instrs)
            if (in.op == Op::ADD) add = &in;
    assert(add && !add->operands.empty() && "expected the body increment");

    const InstrRef* ref = get_instrref(add->operands[0]);
    assert(ref && "body increment operand should be an SSA value");
    assert(ref->id == iphi->result_id &&
           "full SSA: body i+1 must reference the header phi result");

    std::cout << "  PASS: full-SSA loop -> body i+1 references header phi %v"
              << iphi->result_id << "\n";
    std::cout << f.dump() << "\n";
}

int main() {
    std::cout << "=== libdeobf CFG recovery tests ===\n";
    test_diamond();
    test_loop();
    test_full_ssa_loop();
    std::cout << "\nAll CFG recovery tests passed!\n";
    return 0;
}
