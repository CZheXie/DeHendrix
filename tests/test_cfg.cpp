#include "deobf/cfg.h"
#ifdef NDEBUG
#undef NDEBUG   // keep assert() active in Release — these are correctness tests
#endif
#include <cassert>
#include <iostream>

using namespace deobf;

static void test_basic_cfg() {
    CFGFunction f("test_cfg");

    auto& entry = f.add_block("entry");
    auto& then_bb = f.add_block("then");
    auto& else_bb = f.add_block("else");
    auto& merge = f.add_block("merge");

    entry.term.kind = TermKind::CJMP;
    entry.term.condition = SymReg("cond");
    entry.term.target = then_bb.id;
    entry.term.fallthrough = else_bb.id;
    f.add_edge(entry.id, then_bb.id);
    f.add_edge(entry.id, else_bb.id);

    then_bb.term.kind = TermKind::JMP;
    then_bb.term.target = merge.id;
    f.add_edge(then_bb.id, merge.id);

    else_bb.term.kind = TermKind::JMP;
    else_bb.term.target = merge.id;
    f.add_edge(else_bb.id, merge.id);

    merge.term.kind = TermKind::RET;

    assert(f.blocks.size() == 4);
    assert(f.edges.size() == 4);

    auto succs = f.successors(entry.id);
    assert(succs.size() == 2);

    auto preds = f.predecessors(merge.id);
    assert(preds.size() == 2);

    std::cout << "  PASS: basic CFG with diamond shape\n";
    std::cout << f.dump() << "\n";
}

static void test_loop_cfg() {
    CFGFunction f("test_loop");

    auto& header = f.add_block("header");
    auto& body = f.add_block("body");
    auto& exit_bb = f.add_block("exit");

    header.term.kind = TermKind::CJMP;
    header.term.condition = SymReg("loop_cond");
    header.term.target = body.id;
    header.term.fallthrough = exit_bb.id;
    f.add_edge(header.id, body.id);
    f.add_edge(header.id, exit_bb.id);

    body.term.kind = TermKind::JMP;
    body.term.target = header.id;
    f.add_edge(body.id, header.id, true);

    exit_bb.term.kind = TermKind::RET;

    PhiNode phi;
    phi.result_id = 100;
    phi.incoming = {{0, Const(0)}, {body.id, InstrRef(101)}};
    header.phis.push_back(phi);

    assert(f.blocks.size() == 3);

    auto preds = f.predecessors(header.id);
    assert(preds.size() == 1); // backedge from body

    std::cout << "  PASS: loop CFG with phi node\n";
    std::cout << f.dump() << "\n";
}

int main() {
    std::cout << "=== libdeobf CFG tests ===\n";
    test_basic_cfg();
    test_loop_cfg();
    std::cout << "\nAll CFG tests passed!\n";
    return 0;
}
