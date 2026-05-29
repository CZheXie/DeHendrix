#include "deobf/segment_eval.h"
#include "deobf/cfg_recovery.h"
#include "deobf/passes.h"
#include "deobf/lifter.h"

#include <algorithm>
#include <unordered_set>

#ifdef DEOBF_HAS_CAPSTONE
#include <capstone/capstone.h>
#endif

namespace deobf {

const char* seg_stop_name(SegStop s) {
    switch (s) {
        case SegStop::Ret:           return "Ret";
        case SegStop::JmpResolved:   return "JmpResolved";
        case SegStop::JmpUnresolved: return "JmpUnresolved";
        case SegStop::Cjmp:          return "Cjmp";
        case SegStop::Limit:         return "Limit";
        case SegStop::OutOfBounds:   return "OutOfBounds";
    }
    return "?";
}

namespace {

// Value-propagating fixpoint: constant promotion / folding / algebraic
// combination / branch folding only. Crucially this does NOT run any dead-code
// elimination, so instructions that are referenced only externally (a segment's
// output registers, the VPC value) survive and stay resolvable. Using the full
// run_to_convergence() here would let dead_dep_elim delete those outputs.
void fold_converge(Function& f, int max_iters = 32) {
    for (int i = 0; i < max_iters; ++i) {
        bool changed = false;
        changed |= const_promote_pass(f);
        changed |= mem_forward_pass(f);
        changed |= const_promote_pass(f);
        changed |= const_fold_pass(f);
        changed |= insn_combine_pass(f);
        changed |= branch_fold_pass(f);
        if (!changed) break;
    }
}

} // namespace

// --- VJCC dual-target extraction (pure IR; no disassembler needed) ---

std::optional<std::pair<uint64_t, uint64_t>>
extract_vjcc_targets(const Function& f, const Value& vpc_value, const Value& condition) {
    auto compute = [&](uint64_t cond_val) -> std::optional<uint64_t> {
        Function g = f; // work on a copy; we mutate to fold under an assumption

        if (const auto* ref = get_instrref(condition)) {
            for (auto& in : g.instrs) {
                if (in.result_id == ref->id) {
                    in.op = Op::CONST;
                    in.operands = {Const(cond_val)};
                    in.annotations.clear();
                }
            }
        } else if (const auto* sr = std::get_if<SymReg>(&condition)) {
            for (auto& in : g.instrs)
                for (auto& op : in.operands)
                    if (const auto* osr = std::get_if<SymReg>(&op))
                        if (osr->name == sr->name) op = Value(Const(cond_val));
        } else {
            return std::nullopt; // condition is already constant — not a real branch
        }

        fold_converge(g, 32);
        auto im = build_instr_map(g);
        Value r = resolve(vpc_value, im);
        if (const auto* c = get_const(r)) return c->value;
        return std::nullopt;
    };

    auto taken = compute(1);
    auto not_taken = compute(0);
    if (taken && not_taken && *taken != *not_taken)
        return std::make_pair(*taken, *not_taken);
    return std::nullopt;
}

#ifdef DEOBF_HAS_CAPSTONE

namespace {

// Apply concrete stores into ByteMemory and promote safe-range loads to consts,
// then run the optimizer to convergence. Mirrors the promote/optimize cycle in
// GuidedEvaluator so segment IR concretizes VM-private memory the same way.
void promote_and_optimize(Function& func, ByteMemory& memory) {
    for (int round = 0; round < 3; ++round) {
        bool promoted = false;
        auto im = build_instr_map(func);
        for (auto& instr : func.instrs) {
            if (instr.op == Op::STORE && instr.operands.size() >= 2) {
                Value a = resolve(instr.operands[0], im);
                Value v = resolve(instr.operands[1], im);
                const auto* ac = get_const(a);
                const auto* vc = get_const(v);
                if (ac && vc && memory.is_safe(ac->value))
                    memory.store_u64(ac->value, vc->value);
            }
            if (instr.op == Op::LOAD && !instr.operands.empty()) {
                const auto* addr = get_const(instr.operands[0]);
                if (!addr) {
                    Value ra = resolve(instr.operands[0], im);
                    addr = get_const(ra);
                    if (addr) instr.operands[0] = ra;
                }
                if (addr && memory.is_safe(addr->value)) {
                    auto val = memory.load_u64(addr->value);
                    if (val) {
                        instr.op = Op::CONST;
                        instr.operands = {Const(*val)};
                        instr.annotations["promoted_from"] = "safe_mem";
                        instr.annotations["vm_private"] = "1";
                        promoted = true;
                    }
                }
            }
        }
        fold_converge(func, 8);
        if (!promoted) break;
    }
}

} // namespace

SegmentRun lift_and_optimize_segment(
    const uint8_t* image, size_t image_len, uint64_t base,
    uint64_t start_va,
    const std::unordered_map<std::string, Value>& in_regs,
    ByteMemory& memory,
    int max_steps,
    uint32_t id_base,
    bool follow_direct_jmps,
    const std::unordered_set<uint64_t>* leaders) {

    SegmentRun r;
    r.func = Function("segment");
    r.func.set_next_id(id_base);

    LiftState state(r.func);
    for (const auto& [k, v] : in_regs) state.set_reg(k, v);

    csh handle;
    if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK) {
        r.stop = SegStop::OutOfBounds;
        return r;
    }
    cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

    uint64_t va = start_va;
    int steps = 0;
    std::unordered_set<uint64_t> visited;
    bool done = false;

    while (steps < max_steps && !done) {
        if (visited.count(va)) { r.stop = SegStop::Limit; break; }
        visited.insert(va);

        uint64_t rva = va - base;
        if (rva >= image_len) { r.stop = SegStop::OutOfBounds; break; }
        size_t avail = static_cast<size_t>(image_len - rva);
        size_t window = std::min<size_t>(avail, 256);

        cs_insn* insn_arr = nullptr;
        size_t count = cs_disasm(handle, image + rva, window, va, 0, &insn_arr);
        if (count == 0) { r.stop = SegStop::OutOfBounds; break; }

        bool term = false;
        bool hit_leader = false;
        for (size_t i = 0; i < count; ++i) {
            uint64_t iaddr = insn_arr[i].address;
            if (leaders && iaddr != start_va && leaders->count(iaddr)) {
                r.stop = SegStop::HitLeader;
                r.stop_va = iaddr;
                r.next_va = iaddr;
                r.resolved_target = iaddr;
                hit_leader = true;
                break;
            }
            lift_insn(&insn_arr[i], state);
            steps++;
            std::string m(insn_arr[i].mnemonic);
            uint64_t after = insn_arr[i].address + insn_arr[i].size;
            if (m == "ret") {
                r.stop = SegStop::Ret;
                r.stop_va = insn_arr[i].address;
                r.next_va = after;
                done = true;
                term = true;
                break;
            }
            if (!m.empty() && (m[0] == 'j' || m == "call")) {
                r.stop_va = insn_arr[i].address;
                r.next_va = after;
                term = true;
                break;
            }
            if (steps >= max_steps) { term = true; break; }
        }
        cs_free(insn_arr, count);

        promote_and_optimize(r.func, memory);

        if (done) break;
        if (hit_leader) break;
        if (!term) { r.stop = SegStop::Limit; break; }

        // Resolve the branch target of the terminator we just lifted.
        auto im = build_instr_map(r.func);
        std::optional<uint64_t> target;
        Value cond;
        bool is_cjmp = false;
        for (auto it = r.func.instrs.rbegin(); it != r.func.instrs.rend(); ++it) {
            if (it->op == Op::JMP || it->op == Op::CALL) {
                if (!it->operands.empty()) {
                    Value rt = resolve(it->operands[0], im);
                    if (const auto* c = get_const(rt)) target = c->value;
                }
                break;
            }
            if (it->op == Op::CJMP && !it->operands.empty()) {
                if (!is_const(it->operands[0])) {
                    is_cjmp = true;
                    cond = it->operands[0];
                    if (it->operands.size() >= 2) {
                        if (const auto* tc = get_const(it->operands[1])) target = tc->value;
                    }
                }
                break;
            }
        }

        if (is_cjmp) {
            r.stop = SegStop::Cjmp;
            r.condition = cond;
            r.resolved_target = target;
            break;
        }
        if (!target) { r.stop = SegStop::JmpUnresolved; break; }
        if (*target - base >= image_len) {
            r.stop = SegStop::JmpResolved;
            r.resolved_target = target;
            break;
        }
        if (!follow_direct_jmps) {
            r.stop = SegStop::JmpResolved;
            r.resolved_target = target;
            break;
        }
        va = *target; // follow a direct jump that stays inside the image
    }

    if (steps >= max_steps && r.stop == SegStop::OutOfBounds)
        r.stop = SegStop::Limit;

    fold_converge(r.func, 32);

    auto im = build_instr_map(r.func);
    for (const auto& [k, v] : state.regs) r.exit_regs[k] = resolve(v, im);

    cs_close(&handle);
    return r;
}

// --- Native CFG recovery (leader fixpoint driving CFGRecovery) ---

CFGFunction recover_native_cfg(const uint8_t* image, size_t image_len, uint64_t base,
                               uint64_t entry_va, int max_blocks) {
    std::unordered_set<uint64_t> leaders;
    leaders.insert(entry_va);

    CFGFunction cfg("native");

    // Iterate to a fixpoint: each pass may discover new block leaders (branch
    // targets / fallthroughs), which split over-long blocks on the next pass.
    for (int pass = 0; pass < 64; ++pass) {
        std::unordered_set<uint64_t> discovered = leaders;
        uint32_t next_id = 1;

        SegmentOracle oracle = [&](uint64_t va, const VMState& in) -> SegmentResult {
            ByteMemory mem; // native code has no VM-private safe ranges
            SegmentRun sr = lift_and_optimize_segment(
                image, image_len, base, va, in.regs, mem,
                /*max_steps*/ 8000, /*id_base*/ next_id,
                /*follow_direct_jmps*/ false, &leaders);
            next_id = sr.func.next_id();

            SegmentResult res;
            res.instrs = std::move(sr.func.instrs);
            VMState out;
            out.regs = sr.exit_regs;

            switch (sr.stop) {
                case SegStop::Ret:
                    res.kind = SegmentKind::Ret;
                    res.out_state = out;
                    break;
                case SegStop::HitLeader:
                case SegStop::JmpResolved:
                    if (sr.resolved_target) {
                        res.kind = SegmentKind::Goto;
                        res.next_vpc = *sr.resolved_target;
                        res.out_state = out;
                        discovered.insert(*sr.resolved_target);
                    } else {
                        res.kind = SegmentKind::Unresolved;
                    }
                    break;
                case SegStop::Cjmp:
                    if (sr.resolved_target) {
                        res.kind = SegmentKind::Branch;
                        res.condition = sr.condition;
                        res.vpc_taken = *sr.resolved_target; // jcc target
                        res.vpc_not_taken = sr.next_va;      // fallthrough
                        res.out_state_taken = out;
                        res.out_state_not_taken = out;
                        discovered.insert(*sr.resolved_target);
                        discovered.insert(sr.next_va);
                    } else {
                        res.kind = SegmentKind::Unresolved;
                    }
                    break;
                default:
                    res.kind = SegmentKind::Unresolved;
                    break;
            }
            return res;
        };

        CFGRecoveryConfig rc;
        rc.entry_vpc = entry_va;
        rc.max_blocks = max_blocks;
        cfg = CFGRecovery::recover(oracle, rc, {});

        if (discovered.size() == leaders.size()) break; // fixpoint reached
        leaders.swap(discovered);
    }

    return cfg;
}

#endif // DEOBF_HAS_CAPSTONE

} // namespace deobf
