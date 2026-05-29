#include "deobf/cfg.h"
#include <sstream>
#include <algorithm>

namespace deobf {

Instruction& BasicBlock::emit(Op op, std::vector<Value> operands,
                              std::unordered_map<std::string, std::string> ann) {
    Instruction instr;
    instr.op = op;
    instr.operands = std::move(operands);
    instr.result_id = 0; // set by CFGFunction
    instr.annotations = std::move(ann);
    instrs.push_back(std::move(instr));
    return instrs.back();
}

BasicBlock& CFGFunction::add_block(const std::string& label) {
    BasicBlock bb;
    bb.id = next_block_id_++;
    bb.label = label.empty() ? ("bb" + std::to_string(bb.id)) : label;
    blocks.push_back(std::move(bb));
    return blocks.back();
}

void CFGFunction::add_edge(uint32_t from, uint32_t to, bool backedge) {
    edges.push_back({from, to, backedge});
}

BasicBlock* CFGFunction::block_by_id(uint32_t id) {
    for (auto& b : blocks)
        if (b.id == id) return &b;
    return nullptr;
}

const BasicBlock* CFGFunction::block_by_id(uint32_t id) const {
    for (auto& b : blocks)
        if (b.id == id) return &b;
    return nullptr;
}

std::vector<uint32_t> CFGFunction::successors(uint32_t block_id) const {
    std::vector<uint32_t> result;
    for (auto& e : edges)
        if (e.from == block_id) result.push_back(e.to);
    return result;
}

std::vector<uint32_t> CFGFunction::predecessors(uint32_t block_id) const {
    std::vector<uint32_t> result;
    for (auto& e : edges)
        if (e.to == block_id) result.push_back(e.from);
    return result;
}

size_t CFGFunction::total_instrs() const {
    size_t n = 0;
    for (auto& b : blocks) {
        n += b.instrs.size();
        n += b.phis.size();
    }
    return n;
}

std::string CFGFunction::dump() const {
    std::ostringstream os;
    os << "cfg_func " << name << " {\n";
    os << "  entry: " << entry_block << "\n";
    os << "  edges:";
    for (auto& e : edges) {
        os << " " << e.from << "->" << e.to;
        if (e.is_backedge) os << "(back)";
    }
    os << "\n\n";

    for (auto& bb : blocks) {
        os << "  " << bb.label << " (id=" << bb.id << "):\n";
        for (auto& phi : bb.phis) {
            os << "    %v" << phi.result_id << " = PHI";
            if (!phi.reg.empty()) os << " <" << phi.reg << ">";
            for (auto& [bid, val] : phi.incoming)
                os << " [bb" << bid << ": " << value_to_string(val) << "]";
            os << "\n";
        }
        for (auto& instr : bb.instrs) {
            os << "    " << instr.to_string() << "\n";
        }
        os << "    term: ";
        switch (bb.term.kind) {
            case TermKind::FALLTHROUGH:
                os << "fallthrough -> bb" << (bb.term.fallthrough ? *bb.term.fallthrough : 0);
                break;
            case TermKind::JMP:
                os << "jmp -> bb" << (bb.term.target ? *bb.term.target : 0);
                break;
            case TermKind::CJMP:
                os << "cjmp " << (bb.term.condition ? value_to_string(*bb.term.condition) : "?")
                   << " -> bb" << (bb.term.target ? *bb.term.target : 0)
                   << " else bb" << (bb.term.fallthrough ? *bb.term.fallthrough : 0);
                break;
            case TermKind::RET:
                os << "ret";
                break;
            case TermKind::UNREACHABLE:
                os << "unreachable";
                break;
        }
        os << "\n\n";
    }
    os << "}";
    return os.str();
}

} // namespace deobf
