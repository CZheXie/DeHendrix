#pragma once
#include "deobf/ir.h"
#include "deobf/cfg.h"
#include "deobf/memory.h"
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace deobf {

/// Why a straight-line segment lift stopped.
enum class SegStop : uint8_t {
    Ret,            ///< Hit a `ret` (VMEXIT candidate at the native level).
    JmpResolved,    ///< Unconditional branch to a resolved target (not followed).
    JmpUnresolved,  ///< Branch target could not be concretized (and no VJCC).
    Cjmp,           ///< Stopped on a conditional jump with a symbolic condition.
    HitLeader,      ///< Reached an instruction that is a known block leader.
    Limit,          ///< Hit the step budget or a native self-loop.
    OutOfBounds,    ///< Ran off the mapped image / disassembler failure.
};

const char* seg_stop_name(SegStop s);

/// Result of lifting+optimizing one straight-line native region.
struct SegmentRun {
    SegStop stop = SegStop::OutOfBounds;
    Function func{"segment"};
    std::unordered_map<std::string, Value> exit_regs; ///< Register values at exit (resolved).
    uint64_t stop_va = 0;                              ///< Address of the terminating instruction.
    uint64_t next_va = 0;                              ///< Address right after the terminator (fallthrough / leader).
    std::optional<uint64_t> resolved_target;          ///< Native target (JmpResolved / Cjmp taken arm / HitLeader).
    std::optional<Value> condition;                   ///< Branch condition (Cjmp).
};

/// Extract the two candidate next-VPC values of a virtual conditional jump.
///
/// VMProtect-style VJCCs compute the next VPC as a (typically branchless)
/// arithmetic function of the VM flag. We recover both targets generically by
/// substituting the branch condition with 1 (taken) and 0 (not-taken) and
/// constant-folding the VPC register's value expression. Returns
/// (vpc_taken, vpc_not_taken) only when both fold to *distinct* constants.
///
/// `vpc_value` is the VPC register's symbolic value at the branch; `condition`
/// is the value feeding the CJMP (an InstrRef into `f`, or a SymReg).
std::optional<std::pair<uint64_t, uint64_t>>
extract_vjcc_targets(const Function& f, const Value& vpc_value, const Value& condition);

#ifdef DEOBF_HAS_CAPSTONE
/// Lift + optimize a straight-line native region starting at `start_va`, seeded
/// with an injectable initial register state (`in_regs` overrides the default
/// symbolic register file). Applies the promote/optimize convergence cycle and
/// stops at ret / branch / a known leader / limits. `id_base` makes emitted
/// result_ids globally unique across segments.
///
/// `follow_direct_jmps`: when true (VM dispatch chasing), an unconditional jmp
/// whose target stays inside the image is followed transparently; when false
/// (native CFG recovery), the segment stops at the jmp and reports the target.
/// `leaders`: if non-null, the segment stops (HitLeader) just before an
/// instruction whose address is a known basic-block leader.
SegmentRun lift_and_optimize_segment(
    const uint8_t* image, size_t image_len, uint64_t base,
    uint64_t start_va,
    const std::unordered_map<std::string, Value>& in_regs,
    ByteMemory& memory,
    int max_steps = 4000,
    uint32_t id_base = 0,
    bool follow_direct_jmps = true,
    const std::unordered_set<uint64_t>* leaders = nullptr);

/// Recover a native control-flow graph (multi-block, with phi nodes at merges)
/// starting at `entry_va`, by driving CFGRecovery with a leader-fixpoint over
/// the segment evaluator. Conditional jumps become CJMP terminators with both
/// arms; registers that disagree across predecessors get phi nodes.
CFGFunction recover_native_cfg(
    const uint8_t* image, size_t image_len, uint64_t base,
    uint64_t entry_va, int max_blocks = 4096);

/// Recover a multi-path VM control-flow graph by devirtualization. Drives a
/// resumable GuidedEvaluator: the entry segment runs to the first VJCC/VMEXIT;
/// each virtual conditional jump's two next-VPCs are recovered via
/// extract_vjcc_targets, and each arm is explored by re-entering the dispatcher
/// with that VPC injected. Blocks are keyed by VPC value; conditional arms
/// become CJMP terminators. `safe_ranges` are the VM bytecode regions that may
/// be constant-promoted (VA start,end pairs).
CFGFunction recover_vm_cfg(
    const uint8_t* image, size_t image_len, uint64_t base,
    const std::string& vpc_reg, uint64_t entry_va,
    const std::vector<std::pair<uint64_t, uint64_t>>& safe_ranges,
    int max_blocks = 4096);
#endif

} // namespace deobf
