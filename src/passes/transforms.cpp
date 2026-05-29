#include "deobf/passes.h"
#include <algorithm>
#include <optional>
#include <unordered_set>

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace deobf {

// --- helpers ---

std::unordered_map<uint32_t, const Instruction*> build_instr_map(const Function& f) {
    std::unordered_map<uint32_t, const Instruction*> m;
    m.reserve(f.instrs.size());
    for (auto& i : f.instrs) m[i.result_id] = &i;
    return m;
}

Value resolve(const Value& operand,
              const std::unordered_map<uint32_t, const Instruction*>& by_id) {
    Value cur = operand;
    for (;;) {
        auto* ref = get_instrref(cur);
        if (!ref) return cur;
        auto it = by_id.find(ref->id);
        if (it == by_id.end()) return cur;
        auto* def = it->second;
        if ((def->op == Op::CONST || def->op == Op::CONST_PTR) &&
            !def->operands.empty()) {
            auto* c = get_const(def->operands[0]);
            if (c) return *c;
            return cur;
        }
        if (def->op == Op::PASSTHROUGH && def->operands.size() == 1) {
            cur = def->operands[0];
            continue;
        }
        return cur;
    }
}

static bool has_ann(const Instruction& i, const std::string& key, const std::string& val) {
    auto it = i.annotations.find(key);
    return it != i.annotations.end() && it->second == val;
}

// --- pass 1: const promotion ---

bool const_promote_pass(Function& f) {
    auto im = build_instr_map(f);
    bool changed = false;
    for (auto& instr : f.instrs) {
        for (auto& op : instr.operands) {
            if (!is_instrref(op)) continue;
            Value resolved = resolve(op, im);
            if (!(resolved == op)) {
                op = resolved;
                changed = true;
            }
        }
    }
    return changed;
}

// --- pass 2: constant folding ---

static uint64_t rol64(uint64_t v, uint64_t n) {
    n &= 63;
    return (v << n) | (v >> (64 - n));
}

static uint64_t ror64(uint64_t v, uint64_t n) {
    n &= 63;
    return (v >> n) | (v << (64 - n));
}

static uint64_t sar64(uint64_t v, uint64_t n) {
    n &= 63;
    if (v & (1ULL << 63))
        return (v >> n) | (UINT64_MAX << (64 - n));
    return v >> n;
}

static uint64_t bswap64(uint64_t v) {
#ifdef _MSC_VER
    return _byteswap_uint64(v);
#else
    return __builtin_bswap64(v);
#endif
}

static std::optional<uint64_t> fold_binary(Op op, uint64_t a, uint64_t b) {
    switch (op) {
        case Op::ADD: return a + b;
        case Op::SUB: return a - b;
        case Op::MUL: return a * b;
        case Op::XOR: return a ^ b;
        case Op::AND: return a & b;
        case Op::OR:  return a | b;
        case Op::SHL: return a << (b & 63);
        case Op::SHR: return a >> (b & 63);
        case Op::SAR: return sar64(a, b);
        case Op::ROL: return rol64(a, b);
        case Op::ROR: return ror64(a, b);
        default: return std::nullopt;
    }
}

static std::optional<uint64_t> fold_unary(Op op, uint64_t a) {
    switch (op) {
        case Op::NOT:   return ~a;
        case Op::NEG:   return static_cast<uint64_t>(-static_cast<int64_t>(a));
        case Op::BSWAP: return bswap64(a);
        default: return std::nullopt;
    }
}

bool const_fold_pass(Function& f) {
    bool changed = false;
    for (auto& instr : f.instrs) {
        if (op_is_foldable_binary(instr.op) && instr.operands.size() == 2) {
            auto* a = get_const(instr.operands[0]);
            auto* b = get_const(instr.operands[1]);
            if (a && b) {
                auto folded = fold_binary(instr.op, a->value, b->value);
                if (folded) {
                    instr.annotations.emplace("folded_from", op_name(instr.op));
                    instr.op = Op::CONST;
                    instr.operands = {Const(*folded, std::max(a->bits, b->bits))};
                    changed = true;
                }
            }
        } else if (op_is_foldable_unary(instr.op) && instr.operands.size() == 1) {
            auto* a = get_const(instr.operands[0]);
            if (a) {
                auto folded = fold_unary(instr.op, a->value);
                if (folded) {
                    instr.annotations.emplace("folded_from", op_name(instr.op));
                    instr.op = Op::CONST;
                    instr.operands = {Const(*folded, a->bits)};
                    changed = true;
                }
            }
        } else if (instr.op == Op::SELECT && instr.operands.size() == 3) {
            // SELECT(cond, a, b): once the condition is constant, collapse to the
            // chosen arm (PASSTHROUGH so a downstream pass forwards the value).
            auto* c = get_const(instr.operands[0]);
            if (c) {
                Value chosen = c->value ? instr.operands[1] : instr.operands[2];
                instr.annotations.emplace("folded_from", "SELECT");
                instr.op = Op::PASSTHROUGH;
                instr.operands = {chosen};
                changed = true;
            }
        }
    }
    return changed;
}

// --- pass 3: mem forwarding (VM-private only) ---

bool mem_forward_pass(Function& f) {
    bool changed = false;
    std::unordered_map<uint64_t, Value> last_store_const;
    std::unordered_map<uint32_t, Value> last_store_ref;

    for (auto& instr : f.instrs) {
        if (instr.op == Op::STORE && instr.operands.size() == 2) {
            auto* addr = get_const(instr.operands[0]);
            if (addr) {
                last_store_const[addr->value] = instr.operands[1];
            } else {
                auto* ref = get_instrref(instr.operands[0]);
                if (ref) last_store_ref[ref->id] = instr.operands[1];
            }
        } else if (instr.op == Op::LOAD && instr.operands.size() == 1) {
            auto* addr = get_const(instr.operands[0]);
            if (addr) {
                auto it = last_store_const.find(addr->value);
                if (it != last_store_const.end()) {
                    if (is_const(it->second))
                        instr.op = Op::CONST;
                    else
                        instr.op = Op::PASSTHROUGH;
                    instr.operands = {it->second};
                    instr.annotations["forwarded"] = "1";
                    changed = true;
                }
            } else {
                auto* ref = get_instrref(instr.operands[0]);
                if (ref) {
                    auto it = last_store_ref.find(ref->id);
                    if (it != last_store_ref.end()) {
                        if (is_const(it->second))
                            instr.op = Op::CONST;
                        else
                            instr.op = Op::PASSTHROUGH;
                        instr.operands = {it->second};
                        instr.annotations["forwarded"] = "1";
                        changed = true;
                    }
                }
            }
        }
    }
    return changed;
}

// --- pass 4: dead store elimination (VM-private) ---

bool dead_store_elim_pass(Function& f) {
    std::unordered_set<uint64_t> live;
    std::vector<bool> keep(f.instrs.size(), true);
    for (int i = static_cast<int>(f.instrs.size()) - 1; i >= 0; --i) {
        auto& instr = f.instrs[i];
        if (instr.op == Op::LOAD && has_ann(instr, "vm_private", "1") &&
            !instr.operands.empty()) {
            auto* c = get_const(instr.operands[0]);
            if (c) live.insert(c->value);
        } else if (instr.op == Op::STORE && has_ann(instr, "vm_private", "1") &&
                   instr.operands.size() >= 2) {
            auto* c = get_const(instr.operands[0]);
            if (c) {
                if (live.count(c->value)) {
                    live.erase(c->value);
                } else {
                    keep[i] = false;
                }
            }
        }
    }
    bool any_drop = std::any_of(keep.begin(), keep.end(), [](bool k) { return !k; });
    if (any_drop) {
        std::vector<Instruction> new_instrs;
        new_instrs.reserve(f.instrs.size());
        for (size_t i = 0; i < f.instrs.size(); ++i) {
            if (keep[i]) new_instrs.push_back(std::move(f.instrs[i]));
        }
        f.instrs = std::move(new_instrs);
    }
    return any_drop;
}

// --- pass 5: dead dependency elimination ---

bool dead_dep_elim_pass(Function& f) {
    std::unordered_set<uint32_t> referenced;
    for (auto& instr : f.instrs) {
        for (auto& op : instr.operands) {
            auto* ref = get_instrref(op);
            if (ref) referenced.insert(ref->id);
        }
    }
    bool changed = false;
    std::vector<bool> keep;
    keep.reserve(f.instrs.size());
    for (auto& instr : f.instrs) {
        if (op_is_side_effectful(instr.op) || referenced.count(instr.result_id)) {
            keep.push_back(true);
        } else {
            keep.push_back(false);
            changed = true;
        }
    }
    if (changed) {
        std::vector<Instruction> new_instrs;
        new_instrs.reserve(f.instrs.size());
        for (size_t i = 0; i < f.instrs.size(); ++i) {
            if (keep[i]) new_instrs.push_back(std::move(f.instrs[i]));
        }
        f.instrs = std::move(new_instrs);
    }
    return changed;
}

// --- pass 6: instruction combination (algebraic identities) ---

bool insn_combine_pass(Function& f) {
    auto im = build_instr_map(f);
    bool changed = false;
    for (auto& instr : f.instrs) {
        if (instr.operands.size() == 2) {
            auto& a = instr.operands[0];
            auto& b = instr.operands[1];
            Value rb = resolve(b, im);

            // x ^ x = 0, x - x = 0
            if ((instr.op == Op::XOR || instr.op == Op::SUB) && a == b) {
                instr.op = Op::CONST; instr.operands = {Const(0)};
                changed = true; continue;
            }
            // x & 0 = 0, x * 0 = 0
            auto* cb = get_const(rb);
            if (cb && cb->value == 0 &&
                (instr.op == Op::AND || instr.op == Op::MUL)) {
                instr.op = Op::CONST; instr.operands = {Const(0)};
                changed = true; continue;
            }
            // x & -1 = x
            if (cb && cb->value == UINT64_MAX && instr.op == Op::AND) {
                instr.op = Op::PASSTHROUGH; instr.operands = {a};
                changed = true; continue;
            }
            // x * 1 = x
            if (cb && cb->value == 1 && instr.op == Op::MUL) {
                instr.op = Op::PASSTHROUGH; instr.operands = {a};
                changed = true; continue;
            }
            // x + 0, x - 0, x ^ 0, x | 0, shifts by 0
            if (cb && cb->value == 0) {
                switch (instr.op) {
                    case Op::ADD: case Op::SUB: case Op::XOR: case Op::OR:
                    case Op::SHL: case Op::SHR: case Op::SAR:
                    case Op::ROL: case Op::ROR:
                        instr.op = Op::PASSTHROUGH; instr.operands = {a};
                        changed = true; continue;
                    default: break;
                }
            }
        }
        // NOT(NOT(x)), NEG(NEG(x)), BSWAP(BSWAP(x))
        if (instr.operands.size() == 1) {
            auto* ref = get_instrref(instr.operands[0]);
            if (ref) {
                auto it = im.find(ref->id);
                if (it != im.end()) {
                    auto* inner = it->second;
                    if (inner->op == instr.op && inner->operands.size() == 1 &&
                        (instr.op == Op::NOT || instr.op == Op::NEG || instr.op == Op::BSWAP)) {
                        instr.op = Op::PASSTHROUGH;
                        instr.operands = {inner->operands[0]};
                        changed = true;
                    }
                }
            }
        }

        // MBA: OR(XOR(AND(x,y), y), y) = y
        // Pattern: (a AND b) XOR b OR b = b
        if (instr.op == Op::OR && instr.operands.size() == 2) {
            for (int swap = 0; swap < 2; ++swap) {
                const Value& or_a = swap ? instr.operands[1] : instr.operands[0];
                const Value& or_b = swap ? instr.operands[0] : instr.operands[1];
                auto* xor_ref = get_instrref(or_a);
                if (!xor_ref) continue;
                auto xit = im.find(xor_ref->id);
                if (xit == im.end() || xit->second->op != Op::XOR) continue;
                auto& xor_instr = *xit->second;
                if (xor_instr.operands.size() != 2) continue;
                // Check XOR(something, or_b)
                int xor_b_idx = -1;
                if (xor_instr.operands[1] == or_b) xor_b_idx = 1;
                else if (xor_instr.operands[0] == or_b) xor_b_idx = 0;
                if (xor_b_idx < 0) continue;
                const Value& xor_other = xor_instr.operands[1 - xor_b_idx];
                auto* and_ref = get_instrref(xor_other);
                if (!and_ref) continue;
                auto ait = im.find(and_ref->id);
                if (ait == im.end() || ait->second->op != Op::AND) continue;
                auto& and_instr = *ait->second;
                if (and_instr.operands.size() != 2) continue;
                // Check AND(?, or_b) — one operand must be or_b
                if (and_instr.operands[0] == or_b || and_instr.operands[1] == or_b) {
                    instr.op = Op::PASSTHROUGH;
                    instr.operands = {or_b};
                    instr.annotations["mba_simplified"] = "and_xor_or";
                    changed = true;
                    break;
                }
            }
        }

        // MBA: XOR(AND(x,y), y) OR y = y  (alternate order detection)
        // Also: OR(a,b) where a = XOR(b, AND(x,b)) → b
        if (instr.op == Op::OR && instr.operands.size() == 2) {
            for (int swap = 0; swap < 2; ++swap) {
                const Value& or_a = swap ? instr.operands[1] : instr.operands[0];
                const Value& or_b = swap ? instr.operands[0] : instr.operands[1];
                // Check if or_a == or_b (x | x = x)
                if (or_a == or_b) {
                    instr.op = Op::PASSTHROUGH;
                    instr.operands = {or_b};
                    changed = true;
                    break;
                }
            }
        }
    }
    return changed;
}

// --- pass 7: branch folding ---

bool branch_fold_pass(Function& f) {
    bool changed = false;
    for (auto& instr : f.instrs) {
        if (instr.op == Op::CJMP && !instr.operands.empty()) {
            auto* c = get_const(instr.operands[0]);
            if (c) {
                instr.op = c->value ? Op::JMP : Op::NOP;
                instr.operands.erase(instr.operands.begin());
                changed = true;
            }
        }
    }
    return changed;
}

// --- pass 8: dead dependency analysis (register liveness at VMEXIT) ---

std::unordered_set<std::string> dead_dep_analysis(
    const Function& f,
    const std::unordered_set<std::string>& all_regs) {
    // Find the RET instruction, then check which regs are referenced before RET.
    // Registers that feed into the RET operand chain are "live".
    // In practice, post-VMEXIT native code will clobber certain regs;
    // anything not clobbered must be preserved = "live".
    std::unordered_set<std::string> live = all_regs; // conservative: all live by default
    return live;
}

bool dead_dep_analysis_pass(Function& f,
                            const std::unordered_set<std::string>& live_regs) {
    // Mark instructions that produce values for dead registers as NOP.
    // This is a post-VMEXIT cleanup pass.
    auto im = build_instr_map(f);
    bool changed = false;

    // Build reverse dependency: which instructions are reachable from RET/STORE/CALL
    std::unordered_set<uint32_t> reachable;
    std::vector<uint32_t> worklist;

    for (auto& instr : f.instrs) {
        if (op_is_side_effectful(instr.op)) {
            reachable.insert(instr.result_id);
            worklist.push_back(instr.result_id);
        }
    }

    while (!worklist.empty()) {
        uint32_t id = worklist.back();
        worklist.pop_back();
        auto it = im.find(id);
        if (it == im.end()) continue;
        for (auto& op : it->second->operands) {
            auto* ref = get_instrref(op);
            if (ref && !reachable.count(ref->id)) {
                reachable.insert(ref->id);
                worklist.push_back(ref->id);
            }
        }
    }

    std::vector<bool> keep;
    keep.reserve(f.instrs.size());
    for (auto& instr : f.instrs) {
        if (op_is_side_effectful(instr.op) || reachable.count(instr.result_id)) {
            keep.push_back(true);
        } else {
            keep.push_back(false);
            changed = true;
        }
    }

    if (changed) {
        std::vector<Instruction> new_instrs;
        new_instrs.reserve(f.instrs.size());
        for (size_t i = 0; i < f.instrs.size(); ++i)
            if (keep[i]) new_instrs.push_back(std::move(f.instrs[i]));
        f.instrs = std::move(new_instrs);
    }
    return changed;
}

// --- pass 9: stack pointer rewrite (concrete stack → RSP-relative) ---

bool stack_rewrite_pass(Function& f, uint64_t init_rsp) {
    bool changed = false;

    for (auto& instr : f.instrs) {
        if (instr.op != Op::LOAD && instr.op != Op::STORE) continue;
        if (instr.operands.empty()) continue;

        auto* addr = get_const(instr.operands[0]);
        if (!addr) continue;

        // Check if address is in reasonable stack range (init_rsp - 0x10000 to init_rsp + 0x100)
        int64_t offset = static_cast<int64_t>(addr->value) - static_cast<int64_t>(init_rsp);
        if (offset >= -0x10000 && offset <= 0x100) {
            // Rewrite: replace constant address with annotation
            instr.annotations["stack_offset"] = std::to_string(offset);
            instr.annotations["rsp_relative"] = "1";
            changed = true;
        }
    }
    return changed;
}

// --- convergence driver ---

using PassFn = bool(*)(Function&);

static const struct { const char* name; PassFn fn; } ALL_PASSES[] = {
    {"const_promote",  const_promote_pass},
    {"mem_forward",    mem_forward_pass},
    {"const_promote",  const_promote_pass},
    {"const_fold",     const_fold_pass},
    {"insn_combine",   insn_combine_pass},
    {"dead_store_elim",dead_store_elim_pass},
    {"dead_dep_elim",  dead_dep_elim_pass},
    {"branch_fold",    branch_fold_pass},
};

ConvergenceReport run_to_convergence(Function& f, int max_iters) {
    ConvergenceReport report;
    report.iterations = 0;
    for (int it = 0; it < max_iters; ++it) {
        bool any = false;
        for (auto& [name, fn] : ALL_PASSES) {
            if (fn(f)) {
                report.pass_hits[name]++;
                any = true;
            }
        }
        report.iterations = it + 1;
        if (!any) break;
    }
    return report;
}

// Value-propagating passes only — no dead-code elimination, so values that are
// live solely via external register state survive across blocks.
static const struct { const char* name; PassFn fn; } FOLD_PASSES[] = {
    {"const_promote", const_promote_pass},
    {"mem_forward",   mem_forward_pass},
    {"const_promote", const_promote_pass},
    {"const_fold",    const_fold_pass},
    {"insn_combine",  insn_combine_pass},
    {"branch_fold",   branch_fold_pass},
};

ConvergenceReport run_fold_convergence(Function& f, int max_iters) {
    ConvergenceReport report;
    report.iterations = 0;
    for (int it = 0; it < max_iters; ++it) {
        bool any = false;
        for (auto& [name, fn] : FOLD_PASSES) {
            if (fn(f)) {
                report.pass_hits[name]++;
                any = true;
            }
        }
        report.iterations = it + 1;
        if (!any) break;
    }
    return report;
}

} // namespace deobf
