#pragma once
#include "deobf/ir.h"
#include <cstdint>
#include <string>
#include <unordered_map>

namespace deobf {

constexpr uint64_t MASK64 = 0xFFFFFFFFFFFFFFFF;

class LiftState {
public:
    Function& func;
    std::unordered_map<std::string, Value> regs;

    explicit LiftState(Function& f, uint64_t rsp_concrete = 0x7FFFFFFFD000ULL);

    Value get_reg(const std::string& name) const;
    void  set_reg(const std::string& name, Value val);

    Instruction& emit(Op op, std::vector<Value> operands,
                      std::unordered_map<std::string, std::string> ann = {});
};

#ifdef DEOBF_HAS_CAPSTONE
struct cs_insn;

void lift_insn(const cs_insn* insn, LiftState& state);

void lift_block(const uint8_t* code, size_t code_len, uint64_t base_va,
                Function& func, LiftState& state, int max_instrs = 500);
#endif

} // namespace deobf
