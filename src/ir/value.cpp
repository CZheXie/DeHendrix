#include "deobf/ir.h"
#include <sstream>
#include <iomanip>

namespace deobf {

std::string value_to_string(const Value& v) {
    return std::visit([](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, Const>) {
            std::ostringstream os;
            os << "#0x" << std::hex << arg.value;
            return os.str();
        } else if constexpr (std::is_same_v<T, SymReg>) {
            return "%" + arg.name;
        } else if constexpr (std::is_same_v<T, SymMem>) {
            return "mem[%v" + std::to_string(arg.addr_ref) + "]:" + std::to_string(arg.bits);
        } else if constexpr (std::is_same_v<T, InstrRef>) {
            return "%v" + std::to_string(arg.id);
        }
    }, v);
}

const char* op_name(Op op) {
    switch (op) {
        case Op::CONST:       return "CONST";
        case Op::CONST_PTR:   return "CONST_PTR";
        case Op::ADD:         return "ADD";
        case Op::SUB:         return "SUB";
        case Op::MUL:        return "MUL";
        case Op::XOR:         return "XOR";
        case Op::AND:         return "AND";
        case Op::OR:          return "OR";
        case Op::SHL:         return "SHL";
        case Op::SHR:         return "SHR";
        case Op::SAR:         return "SAR";
        case Op::ROL:         return "ROL";
        case Op::ROR:         return "ROR";
        case Op::NOT:         return "NOT";
        case Op::NEG:         return "NEG";
        case Op::BSWAP:       return "BSWAP";
        case Op::ZEXT:        return "ZEXT";
        case Op::SEXT:        return "SEXT";
        case Op::TRUNC:       return "TRUNC";
        case Op::LOAD:        return "LOAD";
        case Op::STORE:       return "STORE";
        case Op::CALL:        return "CALL";
        case Op::RET:         return "RET";
        case Op::JMP:         return "JMP";
        case Op::CJMP:        return "CJMP";
        case Op::PASSTHROUGH: return "PASSTHROUGH";
        case Op::NOP:         return "NOP";
    }
    return "???";
}

bool op_is_side_effectful(Op op) {
    switch (op) {
        case Op::STORE: case Op::CALL: case Op::RET:
        case Op::JMP: case Op::CJMP:
            return true;
        default:
            return false;
    }
}

bool op_is_foldable_binary(Op op) {
    switch (op) {
        case Op::ADD: case Op::SUB: case Op::MUL:
        case Op::XOR: case Op::AND: case Op::OR:
        case Op::SHL: case Op::SHR: case Op::SAR:
        case Op::ROL: case Op::ROR:
            return true;
        default:
            return false;
    }
}

bool op_is_foldable_unary(Op op) {
    switch (op) {
        case Op::NOT: case Op::NEG: case Op::BSWAP:
            return true;
        default:
            return false;
    }
}

std::string Instruction::to_string() const {
    std::ostringstream os;
    os << "%v" << result_id << " = " << op_name(op) << " ";
    for (size_t i = 0; i < operands.size(); ++i) {
        if (i > 0) os << ", ";
        os << value_to_string(operands[i]);
    }
    if (!annotations.empty()) {
        os << "  ; ";
        bool first = true;
        for (auto& [k, v] : annotations) {
            if (!first) os << " ";
            os << k << "=" << v;
            first = false;
        }
    }
    return os.str();
}

} // namespace deobf
