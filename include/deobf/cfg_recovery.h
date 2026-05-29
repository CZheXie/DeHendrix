#pragma once
#include "deobf/ir.h"
#include "deobf/cfg.h"
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace deobf {

/// Register state at a VM basic-block boundary.
/// Maps a register name (e.g. "rax", "r11") to its symbolic/concrete Value.
struct VMState {
    std::unordered_map<std::string, Value> regs;

    const Value* find(const std::string& name) const {
        auto it = regs.find(name);
        return it == regs.end() ? nullptr : &it->second;
    }
};

/// How a straight-line VM segment ends.
enum class SegmentKind : uint8_t {
    Ret,         ///< VM returns / VMEXIT — terminal block.
    Goto,        ///< Unconditional continuation to a single next VPC.
    Branch,      ///< VJCC — two possible next VPCs gated by `condition`.
    Unresolved,  ///< Dispatch target could not be resolved.
};

/// Result of evaluating one straight-line VM segment (the work done by a
/// SegmentOracle). `instrs` are the lifted+optimized IR effects of the segment;
/// their result_ids MUST be globally unique across the whole recovered function
/// (the oracle owns id allocation).
struct SegmentResult {
    SegmentKind kind = SegmentKind::Unresolved;
    std::vector<Instruction> instrs;

    // Ret
    std::optional<Value> ret_value;

    // Goto
    std::optional<uint64_t> next_vpc;
    VMState out_state;

    // Branch (VJCC)
    std::optional<Value> condition;
    std::optional<uint64_t> vpc_taken;
    std::optional<uint64_t> vpc_not_taken;
    VMState out_state_taken;
    VMState out_state_not_taken;
};

/// Evaluates one VM basic block: given the entry VPC and the register state on
/// entry, run handlers until the next VJCC / VMEXIT / unconditional dispatch.
using SegmentOracle =
    std::function<SegmentResult(uint64_t entry_vpc, const VMState& in_state)>;

struct CFGRecoveryConfig {
    uint64_t entry_vpc = 0;
    int max_blocks = 8192;
    /// Registers to reconcile with phi nodes at merge points. If empty, the set
    /// is derived from the union of registers seen across all incoming states.
    std::vector<std::string> tracked_regs;

    /// Full-SSA mode. When true (and `tracked_regs` is non-empty), each block is
    /// evaluated with a *unique symbolic entry value* per tracked register
    /// (`reg@bbN`), and after phi construction those entry symbols are rewritten
    /// to the phi result (or the single incoming value / function input). The
    /// result is real SSA: block IR references phi results and loop def-use
    /// chains are closed, instead of being pinned to one predecessor's values.
    bool full_ssa = false;
};

/// Worklist-driven multi-path CFG recovery.
///
/// Drives a SegmentOracle over the VM bytecode, identifying VM basic blocks by
/// their VPC value, following both arms of every VJCC, detecting back-edges, and
/// inserting phi nodes where predecessors disagree on a register's value.
class CFGRecovery {
public:
    static CFGFunction recover(const SegmentOracle& oracle,
                               const CFGRecoveryConfig& config,
                               const VMState& initial_state = {});
};

} // namespace deobf
