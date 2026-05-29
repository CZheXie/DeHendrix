#pragma once
#include "deobf/ir.h"
#include "deobf/lifter.h"
#include "deobf/memory.h"
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace deobf {

struct HandlerRecord {
    std::optional<uint64_t> vpc_before;
    std::optional<uint64_t> vpc_after;
    uint64_t handler_va;
    size_t ir_start_idx;
    size_t ir_end_idx;
};

struct VJCCBranch {
    uint64_t vjcc_va;
    std::optional<uint64_t> vpc_taken;
    std::optional<uint64_t> vpc_not_taken;
    bool explored_taken = false;
    bool explored_not_taken = false;
};

enum class EvalResult {
    IN_PROGRESS = 0,
    RET,
    VMEXIT_RET,
    VMEXIT_CALL,
    UNRESOLVED_TARGET,
    TARGET_OUTSIDE_IMAGE,
    VPC_CONVERGED,
    VPC_UNRESOLVED,
    INNER_LOOP,
    MAX_STEPS,
    MAX_VM_INSNS,
    OUT_OF_BOUNDS,
};

const char* eval_result_name(EvalResult r);

struct EvalReport {
    EvalResult result;
    uint64_t   stop_va;
    int        steps;
    int        vm_insn_count;
    size_t     ir_count;
    std::optional<uint64_t> dispatcher_va;
    std::optional<uint64_t> final_vpc;
};

class GuidedEvaluator {
public:
    GuidedEvaluator(const uint8_t* image, size_t image_len,
                    uint64_t image_base = 0x140000000,
                    const std::string& vpc_reg = "r11");

    void add_safe_range(uint64_t start, uint64_t end);

    EvalReport run(uint64_t start_va, int max_steps = 50000,
                   int max_vm_insns = 2000);

    const std::vector<HandlerRecord>& handlers() const { return handlers_; }
    const std::vector<std::optional<uint64_t>>& vpc_trace() const { return vpc_trace_; }
    const std::unordered_map<uint64_t, int>& handler_freq() const { return handler_freq_; }
    const std::vector<uint64_t>& jmp_trace() const { return jmp_trace_; }
    const Function& func() const { return func_; }
    const ByteMemory& memory() const { return memory_; }

    const std::vector<VJCCBranch>& vjcc_branches() const { return vjcc_branches_; }

    std::string dump_handler_catalog() const;
    std::string dump_vpc_trace() const;
    std::string dump_vjcc_branches() const;

private:
    const uint8_t* image_;
    size_t image_len_;
    uint64_t base_;
    std::string vpc_reg_;

    Function func_;
    LiftState state_;
    ByteMemory memory_;
    std::unordered_set<uint64_t> visited_;
    std::vector<uint64_t> jmp_trace_;

    std::optional<uint64_t> dispatcher_va_;
    std::vector<std::optional<uint64_t>> vpc_trace_;
    std::vector<HandlerRecord> handlers_;
    std::unordered_map<uint64_t, int> handler_freq_;

    std::optional<uint64_t> last_vpc_;
    size_t handler_ir_start_ = 0;
    int vm_insn_count_ = 0;
    std::vector<VJCCBranch> vjcc_branches_;

    const uint8_t* read_bytes(uint64_t va, size_t size) const;
    bool is_in_image(uint64_t va) const;
    std::optional<uint64_t> resolve_vpc() const;
    std::optional<uint64_t> resolve_jmp_target() const;
    std::optional<EvalResult> check_vmexit() const;
    void try_promote_load(Instruction& instr) const;
    bool detect_vjcc(uint64_t handler_va) const;
};

} // namespace deobf
