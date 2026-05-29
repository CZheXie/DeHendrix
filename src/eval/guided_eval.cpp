#include "deobf/evaluator.h"
#include "deobf/passes.h"

#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>

#ifdef DEOBF_HAS_CAPSTONE
#include <capstone/capstone.h>
#endif

namespace deobf {

const char* eval_result_name(EvalResult r) {
    switch (r) {
        case EvalResult::IN_PROGRESS:          return "IN_PROGRESS";
        case EvalResult::RET:                  return "RET";
        case EvalResult::VMEXIT_RET:           return "VMEXIT_RET";
        case EvalResult::VMEXIT_CALL:          return "VMEXIT_CALL";
        case EvalResult::UNRESOLVED_TARGET:    return "UNRESOLVED_TARGET";
        case EvalResult::TARGET_OUTSIDE_IMAGE: return "TARGET_OUTSIDE_IMAGE";
        case EvalResult::VPC_CONVERGED:        return "VPC_CONVERGED";
        case EvalResult::VPC_UNRESOLVED:       return "VPC_UNRESOLVED";
        case EvalResult::INNER_LOOP:           return "INNER_LOOP";
        case EvalResult::MAX_STEPS:            return "MAX_STEPS";
        case EvalResult::MAX_VM_INSNS:         return "MAX_VM_INSNS";
        case EvalResult::OUT_OF_BOUNDS:        return "OUT_OF_BOUNDS";
    }
    return "UNKNOWN";
}

GuidedEvaluator::GuidedEvaluator(const uint8_t* image, size_t image_len,
                                 uint64_t image_base, const std::string& vpc_reg)
    : image_(image), image_len_(image_len), base_(image_base),
      vpc_reg_(vpc_reg), func_("guided"), state_(func_) {}

void GuidedEvaluator::add_safe_range(uint64_t start, uint64_t end) {
    memory_.add_safe_range(start, end);
    uint64_t rva_start = start - base_;
    uint64_t rva_end = end - base_;
    if (rva_start < image_len_ && rva_end <= image_len_) {
        for (uint64_t i = rva_start; i < rva_end; ++i)
            memory_.store(start + (i - rva_start), {image_[i]});
    }
}

void GuidedEvaluator::try_promote_load(Instruction& instr) const {
    if (instr.op != Op::LOAD || instr.operands.empty()) return;
    auto* addr = get_const(instr.operands[0]);
    if (!addr) {
        auto im = build_instr_map(func_);
        Value resolved_addr = resolve(instr.operands[0], im);
        addr = get_const(resolved_addr);
        if (addr) instr.operands[0] = resolved_addr;
    }
    if (!addr || !memory_.is_safe(addr->value)) return;
    auto val = memory_.load_u64(addr->value);
    if (val) {
        instr.op = Op::CONST;
        instr.operands = {Const(*val)};
        instr.annotations["promoted_from"] = "safe_mem";
        instr.annotations["vm_private"] = "1";
    }
}

const uint8_t* GuidedEvaluator::read_bytes(uint64_t va, size_t size) const {
    uint64_t rva = va - base_;
    if (rva + size <= image_len_) return image_ + rva;
    return nullptr;
}

bool GuidedEvaluator::is_in_image(uint64_t va) const {
    uint64_t rva = va - base_;
    return rva < image_len_;
}

std::optional<uint64_t> GuidedEvaluator::resolve_vpc() const {
    auto it = state_.regs.find(vpc_reg_);
    if (it == state_.regs.end()) return std::nullopt;
    auto* c = get_const(it->second);
    if (c) return c->value & MASK64;
    auto im = build_instr_map(func_);
    Value resolved = resolve(it->second, im);
    auto* rc = get_const(resolved);
    if (rc) return rc->value & MASK64;

    // Try to resolve through LOAD chains: if VPC points to a LOAD from safe mem
    auto* ref = get_instrref(resolved);
    if (ref) {
        auto iit = im.find(ref->id);
        if (iit != im.end() && iit->second->op == Op::LOAD &&
            !iit->second->operands.empty()) {
            Value load_addr = resolve(iit->second->operands[0], im);
            auto* addr_c = get_const(load_addr);
            if (addr_c && memory_.is_safe(addr_c->value)) {
                auto val = memory_.load_u64(addr_c->value);
                if (val) return *val & MASK64;
            }
        }
    }
    return std::nullopt;
}

std::optional<uint64_t> GuidedEvaluator::resolve_jmp_target() const {
    for (auto it = func_.instrs.rbegin(); it != func_.instrs.rend(); ++it) {
        if (it->op == Op::JMP || it->op == Op::CALL) {
            if (it->operands.empty()) return std::nullopt;
            auto* c = get_const(it->operands[0]);
            if (c) return c->value & MASK64;
            auto im = build_instr_map(func_);
            Value resolved = resolve(it->operands[0], im);
            auto* rc = get_const(resolved);
            if (rc) return rc->value & MASK64;
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<EvalResult> GuidedEvaluator::check_vmexit() const {
    auto it = state_.regs.find("rsp");
    if (it == state_.regs.end()) return std::nullopt;
    auto* c = get_const(it->second);
    if (!c) return std::nullopt;
    constexpr uint64_t INIT_RSP = 0x7FFFFFFFD000ULL;
    uint64_t delta = c->value - INIT_RSP;
    if (delta == 0) return EvalResult::VMEXIT_RET;
    if (delta == static_cast<uint64_t>(-0x10)) return EvalResult::VMEXIT_CALL;
    return std::nullopt;
}

#ifdef DEOBF_HAS_CAPSTONE

EvalReport GuidedEvaluator::run(uint64_t start_va, int max_steps, int max_vm_insns) {
    EvalReport report{};
    int steps = 0;
    uint64_t va = start_va;

    // Seed a concrete stack pointer and mark the stack window as a safe range.
    // Without this, rsp stays symbolic, every stack address is symbolic, and
    // outgoing stack arguments / spills (e.g. the VM bytecode pointer passed
    // into the dispatch helper and re-read via [rbp+0xA0]) can never be
    // concretized -> the VPC stays symbolic and dispatch is UNRESOLVED.
    constexpr uint64_t INIT_RSP = 0x7FFFFFFFD000ULL;
    state_.set_reg("rsp", Const(INIT_RSP));
    add_safe_range(INIT_RSP - 0x00200000ULL, INIT_RSP + 0x00010000ULL);

    csh handle;
    if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK) {
        report.result = EvalResult::OUT_OF_BOUNDS;
        return report;
    }
    cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

    while (steps < max_steps) {
        if (visited_.count(va)) {
            if (!dispatcher_va_) dispatcher_va_ = va;


            if (va == *dispatcher_va_) {
                auto cur_vpc = resolve_vpc();

                if (last_vpc_ && handler_ir_start_ < func_.instrs.size()) {
                    uint64_t h_va = jmp_trace_.empty() ? 0 : jmp_trace_.back();
                    handlers_.push_back({last_vpc_, cur_vpc, h_va,
                                        handler_ir_start_, func_.instrs.size()});
                    handler_freq_[h_va]++;
                }

                if (cur_vpc && last_vpc_ && *cur_vpc == *last_vpc_) {
                    report.result = EvalResult::VPC_CONVERGED;
                    report.final_vpc = cur_vpc;
                    break;
                }

                if (!cur_vpc && !last_vpc_ && vm_insn_count_ > 0) {
                    report.result = EvalResult::INNER_LOOP;
                    break;
                }

                last_vpc_ = cur_vpc;
                vpc_trace_.push_back(cur_vpc);
                handler_ir_start_ = func_.instrs.size();
                vm_insn_count_++;

                if (vm_insn_count_ >= max_vm_insns) {
                    report.result = EvalResult::MAX_VM_INSNS;
                    report.final_vpc = cur_vpc;
                    break;
                }

                visited_.clear();
                visited_.insert(va);
            } else {
                report.result = EvalResult::INNER_LOOP;
                break;
            }
        } else {
            visited_.insert(va);
        }

        jmp_trace_.push_back(va);

        auto* code = read_bytes(va, 256);
        if (!code) {
            report.result = EvalResult::OUT_OF_BOUNDS;
            break;
        }

        cs_insn* insn_arr;
        size_t count = cs_disasm(handle, code, 256, va, 0, &insn_arr);
        bool hit_term = false;

        for (size_t i = 0; i < count; ++i) {
            lift_insn(&insn_arr[i], state_);
            steps++;

                std::string m(insn_arr[i].mnemonic);
            if (m == "ret") {
                run_fold_convergence(func_, 8);
                auto ret_target = resolve_jmp_target();
                if (ret_target && memory_.is_safe(*ret_target) && !dispatcher_va_) {
                    va = *ret_target;
                    hit_term = true;
                    break;
                }
                auto vmexit = check_vmexit();
                report.result = vmexit.value_or(EvalResult::RET);
                report.stop_va = insn_arr[i].address;
                hit_term = true;
                break;
            }
            if (m == "jmp" || m == "call") {
                hit_term = true;
                break;
            }
        }
        cs_free(insn_arr, count);

        // Iterative optimize-promote cycle. Fold FIRST so that load/store address
        // expressions (e.g. ADD(vpc_reg, 0)) concretize to constants before we
        // test them against the safe ranges; otherwise a dispatch that reads its
        // target via [reg] in the very first block can never be promoted.
        for (int promote_round = 0; promote_round < 3; ++promote_round) {
            run_fold_convergence(func_, 8);
            bool promoted_any = false;
            auto store_im = build_instr_map(func_);
            for (auto& instr : func_.instrs) {
                // Apply concrete stores into ByteMemory (in program order) so that
                // later loads from the same address can be promoted. Gated by
                // is_safe so we only model stack / VM-private regions.
                if (instr.op == Op::STORE && instr.operands.size() >= 2) {
                    Value addr_v = resolve(instr.operands[0], store_im);
                    Value val_v  = resolve(instr.operands[1], store_im);
                    auto* ac = get_const(addr_v);
                    auto* vc = get_const(val_v);
                    if (ac && vc && memory_.is_safe(ac->value))
                        memory_.store_u64(ac->value, vc->value);
                }
                if (instr.op == Op::LOAD) {
                    Op old_op = instr.op;
                    try_promote_load(instr);
                    if (instr.op != old_op) promoted_any = true;
                }
            }
            if (!promoted_any) break;
        }

        if (report.result != EvalResult::IN_PROGRESS) {
            break;
        }

        auto target = resolve_jmp_target();
        if (!target) {
            // Check if this is a VJCC (virtual conditional jump).
            // If we find a CJMP with non-constant condition, record it.
            bool found_vjcc = false;
            for (auto it = func_.instrs.rbegin(); it != func_.instrs.rend(); ++it) {
                if (it->op == Op::CJMP && !it->operands.empty() &&
                    !is_const(it->operands[0])) {
                    VJCCBranch branch;
                    branch.vjcc_va = va;
                    // Try to extract taken/not-taken VPC values from the CJMP operands
                    if (it->operands.size() >= 2) {
                        auto* t = get_const(it->operands[1]);
                        if (t) branch.vpc_taken = t->value;
                    }
                    if (it->operands.size() >= 3) {
                        auto* f_val = get_const(it->operands[2]);
                        if (f_val) branch.vpc_not_taken = f_val->value;
                    }
                    vjcc_branches_.push_back(branch);
                    found_vjcc = true;
                    break;
                }
            }
            if (!found_vjcc) {
                report.result = EvalResult::UNRESOLVED_TARGET;
                break;
            }
            // For now, record the VJCC but stop (multi-path exploration is Phase 3+)
            report.result = EvalResult::UNRESOLVED_TARGET;
            break;
        }
        if (!is_in_image(*target)) {
            report.result = EvalResult::TARGET_OUTSIDE_IMAGE;
            break;
        }
        va = *target;
    }

    if (steps >= max_steps && report.result == EvalResult::IN_PROGRESS) {
        report.result = EvalResult::MAX_STEPS;
    }

    cs_close(&handle);

    // Final high-iteration optimization to maximize constant propagation
    run_to_convergence(func_, 32);

    report.stop_va = va;
    report.steps = steps;
    report.vm_insn_count = vm_insn_count_;
    report.ir_count = func_.instrs.size();
    report.dispatcher_va = dispatcher_va_;
    return report;
}

#else // !DEOBF_HAS_CAPSTONE

EvalReport GuidedEvaluator::run(uint64_t, int, int) {
    EvalReport report{};
    report.result = EvalResult::OUT_OF_BOUNDS;
    return report;
}

#endif

bool GuidedEvaluator::detect_vjcc(uint64_t handler_va) const {
    // VJCC detection heuristic: if the handler contains a CJMP with
    // a non-constant condition, it's likely a virtual conditional jump.
    // The condition typically comes from a branch_taken_flag in the VM context.
    for (auto& instr : func_.instrs) {
        if (instr.op == Op::CJMP && !instr.operands.empty()) {
            if (!is_const(instr.operands[0])) return true;
        }
    }
    return false;
}

std::string GuidedEvaluator::dump_vjcc_branches() const {
    std::ostringstream os;
    os << "=== VJCC Branches (" << vjcc_branches_.size() << ") ===\n";
    for (size_t i = 0; i < vjcc_branches_.size(); ++i) {
        auto& b = vjcc_branches_[i];
        os << "  [" << i << "] VA=0x" << std::hex << b.vjcc_va;
        if (b.vpc_taken) os << " taken=0x" << *b.vpc_taken;
        if (b.vpc_not_taken) os << " not_taken=0x" << *b.vpc_not_taken;
        os << " explored=" << (b.explored_taken ? "T" : "-")
           << (b.explored_not_taken ? "F" : "-") << "\n";
    }
    return os.str();
}

std::string GuidedEvaluator::dump_handler_catalog() const {
    std::ostringstream os;
    os << "=== Handler Catalog (" << handlers_.size() << " dispatches, "
       << handler_freq_.size() << " unique) ===\n";
    std::vector<std::pair<uint64_t, int>> sorted(handler_freq_.begin(), handler_freq_.end());
    std::sort(sorted.begin(), sorted.end(), [](auto& a, auto& b) { return a.second > b.second; });
    for (auto& [va, cnt] : sorted)
        os << "  0x" << std::hex << va << ": " << std::dec << cnt << " times\n";
    return os.str();
}

std::string GuidedEvaluator::dump_vpc_trace() const {
    std::ostringstream os;
    os << "=== VPC Trace (" << vpc_trace_.size() << " entries) ===\n";
    size_t limit = std::min(vpc_trace_.size(), size_t(100));
    for (size_t i = 0; i < limit; ++i) {
        os << "  [" << i << "] ";
        if (vpc_trace_[i]) os << "0x" << std::hex << *vpc_trace_[i];
        else os << "symbolic";
        os << "\n";
    }
    if (vpc_trace_.size() > 100)
        os << "  ... " << vpc_trace_.size() - 100 << " more\n";
    return os.str();
}

} // namespace deobf
