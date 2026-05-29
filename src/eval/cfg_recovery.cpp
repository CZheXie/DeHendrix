#include "deobf/cfg_recovery.h"

#include <algorithm>
#include <deque>
#include <map>
#include <sstream>
#include <unordered_set>

namespace deobf {

namespace {

std::string vpc_label(uint64_t vpc) {
    std::ostringstream os;
    os << "vpc_0x" << std::hex << vpc;
    return os.str();
}

// One recorded predecessor edge into a block, carrying the register state that
// flowed across that edge (used later to build phi nodes).
struct Incoming {
    uint32_t pred_block;
    VMState state;
};

uint32_t max_result_id(const CFGFunction& cfg) {
    uint32_t m = 0;
    for (const auto& bb : cfg.blocks) {
        for (const auto& in : bb.instrs) m = std::max(m, in.result_id);
        for (const auto& p : bb.phis) m = std::max(m, p.result_id);
    }
    return m;
}

// Rewrite a Value: a SymReg whose name maps in `subst` is replaced.
Value apply_subst(const Value& v, const std::unordered_map<std::string, Value>& subst) {
    if (const auto* sr = std::get_if<SymReg>(&v)) {
        auto it = subst.find(sr->name);
        if (it != subst.end()) return it->second;
    }
    return v;
}

// Follow chains of symbolic-entry rewrites to their terminal value (phi result,
// function input, or constant), with a cycle guard.
void resolve_subst(std::unordered_map<std::string, Value>& subst) {
    for (auto& kv : subst) {
        Value cur = kv.second;
        std::unordered_set<std::string> seen;
        while (const auto* sr = std::get_if<SymReg>(&cur)) {
            auto it = subst.find(sr->name);
            if (it == subst.end() || seen.count(sr->name)) break;
            seen.insert(sr->name);
            cur = it->second;
        }
        kv.second = cur;
    }
}

} // namespace

CFGFunction CFGRecovery::recover(const SegmentOracle& oracle,
                                 const CFGRecoveryConfig& config,
                                 const VMState& initial_state) {
    CFGFunction cfg("devirt");

    std::map<uint64_t, uint32_t> vpc_to_block;
    std::unordered_map<uint32_t, std::vector<Incoming>> incoming;

    auto get_block = [&](uint64_t vpc) -> uint32_t {
        auto it = vpc_to_block.find(vpc);
        if (it != vpc_to_block.end()) return it->second;
        auto& bb = cfg.add_block(vpc_label(vpc));
        bb.term.kind = TermKind::UNREACHABLE;
        vpc_to_block[vpc] = bb.id;
        return bb.id;
    };

    const uint32_t entry_id = get_block(config.entry_vpc);
    cfg.entry_block = entry_id;

    std::deque<uint64_t> worklist;
    std::unordered_set<uint64_t> enqueued;
    std::unordered_set<uint64_t> processed;
    worklist.push_back(config.entry_vpc);
    enqueued.insert(config.entry_vpc);

    auto enqueue = [&](uint64_t vpc) {
        if (!enqueued.count(vpc)) {
            enqueued.insert(vpc);
            worklist.push_back(vpc);
        }
    };

    while (!worklist.empty() &&
           static_cast<int>(cfg.blocks.size()) <= config.max_blocks) {
        uint64_t vpc = worklist.front();
        worklist.pop_front();
        if (processed.count(vpc)) continue;
        processed.insert(vpc);

        const uint32_t bid = vpc_to_block[vpc];

        // State that drives evaluation of this block. In full-SSA mode each
        // tracked register gets a unique symbolic entry value (reg@bbN), later
        // rewritten to the phi result. Otherwise we use the first incoming edge
        // (forward edge), or the seed state for the entry block.
        VMState drive_state;
        if (config.full_ssa && !config.tracked_regs.empty()) {
            for (const auto& r : config.tracked_regs)
                drive_state.regs[r] = SymReg(r + "@bb" + std::to_string(bid));
        } else {
            auto inc_it = incoming.find(bid);
            if (inc_it != incoming.end() && !inc_it->second.empty())
                drive_state = inc_it->second.front().state;
            else if (bid == entry_id)
                drive_state = initial_state;
        }

        SegmentResult res = oracle(vpc, drive_state);

        // Build the terminator first. This may create successor blocks, which can
        // grow `cfg.blocks` — so we must not hold a pointer into it across these
        // calls. We assemble the Terminator + edges using only block ids, then
        // fetch the current block pointer afterwards.
        Terminator term;
        switch (res.kind) {
            case SegmentKind::Ret:
                term.kind = TermKind::RET;
                if (res.ret_value) term.condition = res.ret_value;
                break;

            case SegmentKind::Goto:
                if (res.next_vpc) {
                    bool back = processed.count(*res.next_vpc) > 0;
                    uint32_t s = get_block(*res.next_vpc);
                    cfg.add_edge(bid, s, back);
                    incoming[s].push_back({bid, res.out_state});
                    term.kind = TermKind::JMP;
                    term.target = s;
                    enqueue(*res.next_vpc);
                } else {
                    term.kind = TermKind::UNREACHABLE;
                }
                break;

            case SegmentKind::Branch:
                if (res.vpc_taken && res.vpc_not_taken) {
                    bool tback = processed.count(*res.vpc_taken) > 0;
                    bool fback = processed.count(*res.vpc_not_taken) > 0;
                    uint32_t t = get_block(*res.vpc_taken);
                    uint32_t f = get_block(*res.vpc_not_taken);
                    cfg.add_edge(bid, t, tback);
                    cfg.add_edge(bid, f, fback);
                    incoming[t].push_back({bid, res.out_state_taken});
                    incoming[f].push_back({bid, res.out_state_not_taken});
                    term.kind = TermKind::CJMP;
                    term.condition = res.condition;
                    term.target = t;
                    term.fallthrough = f;
                    enqueue(*res.vpc_taken);
                    enqueue(*res.vpc_not_taken);
                } else {
                    term.kind = TermKind::UNREACHABLE;
                }
                break;

            case SegmentKind::Unresolved:
                term.kind = TermKind::UNREACHABLE;
                break;
        }

        BasicBlock* bb = cfg.block_by_id(bid);
        bb->instrs = std::move(res.instrs);
        bb->term = std::move(term);
    }

    // --- Phi construction ---
    // Determine which registers to reconcile.
    std::vector<std::string> tracked = config.tracked_regs;
    if (tracked.empty()) {
        std::unordered_set<std::string> seen;
        for (auto& [bid, incs] : incoming)
            for (auto& inc : incs)
                for (auto& [name, val] : inc.state.regs) seen.insert(name);
        tracked.assign(seen.begin(), seen.end());
        std::sort(tracked.begin(), tracked.end());
    }

    uint32_t next_id = max_result_id(cfg) + 1;

    for (auto& bb : cfg.blocks) {
        auto it = incoming.find(bb.id);
        if (it == incoming.end() || it->second.size() < 2) continue;
        const auto& incs = it->second;

        for (const auto& reg : tracked) {
            std::vector<std::pair<uint32_t, Value>> pairs;
            pairs.reserve(incs.size());
            bool complete = true;
            for (const auto& inc : incs) {
                const Value* v = inc.state.find(reg);
                if (!v) { complete = false; break; }
                pairs.push_back({inc.pred_block, *v});
            }
            if (!complete || pairs.size() < 2) continue;

            bool all_same = true;
            for (size_t i = 1; i < pairs.size(); ++i) {
                if (!(pairs[i].second == pairs[0].second)) { all_same = false; break; }
            }
            if (all_same) continue;

            PhiNode phi;
            phi.result_id = next_id++;
            phi.reg = reg;
            phi.incoming = std::move(pairs);
            bb.phis.push_back(std::move(phi));
        }
    }

    // --- Full-SSA rewrite ---
    // Replace each block's symbolic entry value (reg@bbN) with its definition:
    // the phi result if one exists, else the single incoming value, else the
    // function input. This wires block IR to phi results and closes loop
    // def-use chains, turning the structural phi graph into real SSA.
    if (config.full_ssa && !config.tracked_regs.empty()) {
        std::unordered_map<std::string, Value> subst;
        for (auto& bb : cfg.blocks) {
            for (const auto& r : config.tracked_regs) {
                std::string sym = r + "@bb" + std::to_string(bb.id);
                Value target = SymReg(r); // default: clean function input
                const PhiNode* phi = nullptr;
                for (auto& p : bb.phis)
                    if (p.reg == r) { phi = &p; break; }
                if (phi) {
                    target = InstrRef(phi->result_id);
                } else if (bb.id == entry_id) {
                    if (const Value* iv = initial_state.find(r)) target = *iv;
                } else {
                    auto it = incoming.find(bb.id);
                    if (it != incoming.end() && !it->second.empty())
                        if (const Value* iv = it->second.front().state.find(r))
                            target = *iv;
                }
                subst[sym] = target;
            }
        }
        resolve_subst(subst);

        for (auto& bb : cfg.blocks) {
            for (auto& in : bb.instrs)
                for (auto& op : in.operands) op = apply_subst(op, subst);
            for (auto& phi : bb.phis)
                for (auto& inc : phi.incoming) inc.second = apply_subst(inc.second, subst);
            if (bb.term.condition)
                bb.term.condition = apply_subst(*bb.term.condition, subst);
        }
    }

    return cfg;
}

} // namespace deobf
