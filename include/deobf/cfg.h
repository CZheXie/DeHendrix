#pragma once
#include "deobf/ir.h"
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace deobf {

enum class TermKind : uint8_t {
    FALLTHROUGH,
    JMP,
    CJMP,
    RET,
    UNREACHABLE,
};

struct Terminator {
    TermKind kind = TermKind::UNREACHABLE;
    std::optional<uint32_t> target;
    std::optional<uint32_t> fallthrough;
    std::optional<Value> condition;
};

struct PhiNode {
    uint32_t result_id;
    std::vector<std::pair<uint32_t, Value>> incoming; // (block_id, value)
};

struct BasicBlock {
    uint32_t id;
    std::string label;
    std::vector<PhiNode> phis;
    std::vector<Instruction> instrs;
    Terminator term;

    Instruction& emit(Op op, std::vector<Value> operands,
                      std::unordered_map<std::string, std::string> ann = {});
};

struct Edge {
    uint32_t from;
    uint32_t to;
    bool is_backedge = false;
};

class CFGFunction {
public:
    std::string name;
    std::vector<BasicBlock> blocks;
    std::vector<Edge> edges;
    uint32_t entry_block = 0;

    explicit CFGFunction(std::string n = "unnamed") : name(std::move(n)) {}

    BasicBlock& add_block(const std::string& label = "");
    void add_edge(uint32_t from, uint32_t to, bool backedge = false);

    BasicBlock* block_by_id(uint32_t id);
    const BasicBlock* block_by_id(uint32_t id) const;

    std::vector<uint32_t> successors(uint32_t block_id) const;
    std::vector<uint32_t> predecessors(uint32_t block_id) const;

    size_t total_instrs() const;
    std::string dump() const;

private:
    uint32_t next_block_id_ = 0;
    uint32_t next_instr_id_ = 0;
    friend struct BasicBlock;
};

} // namespace deobf
