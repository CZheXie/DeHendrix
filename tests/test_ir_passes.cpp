#include "deobf/ir.h"
#include "deobf/passes.h"
#ifdef NDEBUG
#undef NDEBUG   // keep assert() active in Release — these are correctness tests
#endif
#include <cassert>
#include <iostream>

using namespace deobf;

static void test_const_promote_and_fold() {
    Function f("test_fold");
    auto& a = f.emit(Op::CONST, {Const(3)});
    auto& b = f.emit(Op::CONST, {Const(4)});
    f.emit(Op::ADD, {a.ref(), b.ref()});

    assert(const_promote_pass(f));
    assert(const_fold_pass(f));

    auto& last = f.instrs.back();
    assert(last.op == Op::CONST);
    auto* c = get_const(last.operands[0]);
    assert(c && c->value == 7);
    std::cout << "  PASS: const_promote + const_fold -> 3+4=7\n";
}

static void test_dead_dep_elim() {
    Function f("test_dde");
    f.emit(Op::CONST, {Const(1)});
    auto& used = f.emit(Op::CONST, {Const(2)});
    f.emit(Op::RET, {used.ref()});

    size_t before = f.instrs.size();
    assert(dead_dep_elim_pass(f));
    assert(f.instrs.size() == before - 1);
    std::cout << "  PASS: dead_dep_elim removes unused const\n";
}

static void test_branch_fold_true() {
    Function f("test_bf");
    f.emit(Op::CJMP, {Const(1), Const(0x1000)});
    assert(branch_fold_pass(f));
    assert(f.instrs[0].op == Op::JMP);
    assert(f.instrs[0].operands.size() == 1);
    std::cout << "  PASS: branch_fold const-true -> JMP\n";
}

static void test_branch_fold_false() {
    Function f("test_bf2");
    f.emit(Op::CJMP, {Const(0), Const(0x1000)});
    assert(branch_fold_pass(f));
    assert(f.instrs[0].op == Op::NOP);
    std::cout << "  PASS: branch_fold const-false -> NOP\n";
}

static void test_insn_combine_xor_self() {
    Function f("test_xor_self");
    auto& a = f.emit(Op::CONST, {Const(42)});
    f.emit(Op::XOR, {a.ref(), a.ref()});

    assert(insn_combine_pass(f));
    assert(f.instrs.back().op == Op::CONST);
    auto* c = get_const(f.instrs.back().operands[0]);
    assert(c && c->value == 0);
    std::cout << "  PASS: insn_combine x^x = 0\n";
}

static void test_convergence() {
    Function f("test_conv");
    auto& a = f.emit(Op::CONST, {Const(10)});
    auto& b = f.emit(Op::CONST, {Const(20)});
    auto& sum = f.emit(Op::ADD, {a.ref(), b.ref()});
    f.emit(Op::RET, {sum.ref()});

    auto report = run_to_convergence(f, 32);

    auto& last_ret = f.instrs.back();
    assert(last_ret.op == Op::RET);
    auto* c = get_const(last_ret.operands[0]);
    assert(c && c->value == 30);
    std::cout << "  PASS: convergence reduces 10+20 to RET #30 in "
              << report.iterations << " iterations\n";
}

static void test_mem_forward() {
    Function f("test_memfwd");
    auto& addr_val = f.emit(Op::CONST, {Const(0xDEAD)});
    auto& store_val = f.emit(Op::CONST, {Const(42)});
    f.emit(Op::STORE, {addr_val.ref(), store_val.ref()}, {{"vm_private", "1"}});
    f.emit(Op::LOAD, {addr_val.ref()}, {{"vm_private", "1"}});

    const_promote_pass(f);
    assert(mem_forward_pass(f));

    auto& load = f.instrs.back();
    assert(load.op == Op::CONST);
    auto* c = get_const(load.operands[0]);
    assert(c && c->value == 42);
    std::cout << "  PASS: mem_forward store 42 -> load = 42\n";
}

int main() {
    std::cout << "=== libdeobf IR + Passes unit tests ===\n";
    test_const_promote_and_fold();
    test_dead_dep_elim();
    test_branch_fold_true();
    test_branch_fold_false();
    test_insn_combine_xor_self();
    test_convergence();
    test_mem_forward();
    std::cout << "\nAll tests passed!\n";
    return 0;
}
