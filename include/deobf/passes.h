#pragma once
#include "deobf/ir.h"
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace deobf {

Value resolve(const Value& operand,
              const std::unordered_map<uint32_t, const Instruction*>& by_id);

std::unordered_map<uint32_t, const Instruction*> build_instr_map(const Function& f);

bool const_promote_pass(Function& f);
bool const_fold_pass(Function& f);
bool mem_forward_pass(Function& f);
bool dead_store_elim_pass(Function& f);
bool dead_dep_elim_pass(Function& f);
bool insn_combine_pass(Function& f);
bool branch_fold_pass(Function& f);

/// Identifies which registers are live at VMEXIT by scanning post-exit code
/// for clobbers. Returns the set of register names that are NOT clobbered
/// (i.e., the registers whose values the VM must preserve).
std::unordered_set<std::string> dead_dep_analysis(
    const Function& f,
    const std::unordered_set<std::string>& all_regs);

/// Removes IR instructions that produce only dead register values.
/// `live_regs` is the set of registers that must be preserved at VMEXIT.
bool dead_dep_analysis_pass(Function& f,
                            const std::unordered_set<std::string>& live_regs);

/// Rewrites concrete stack addresses back to RSP-relative form.
/// `init_rsp` is the concrete RSP value used at the start of lifting.
bool stack_rewrite_pass(Function& f, uint64_t init_rsp = 0x7FFFFFFFD000ULL);

struct ConvergenceReport {
    int iterations;
    std::unordered_map<std::string, int> pass_hits;
};

ConvergenceReport run_to_convergence(Function& f, int max_iters = 32);

} // namespace deobf
