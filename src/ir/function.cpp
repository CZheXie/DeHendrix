#include "deobf/ir.h"
#include <sstream>

namespace deobf {

Instruction& Function::emit(Op op, std::vector<Value> operands,
                            std::unordered_map<std::string, std::string> ann) {
    Instruction instr;
    instr.op = op;
    instr.operands = std::move(operands);
    instr.result_id = next_id_++;
    instr.annotations = std::move(ann);
    instrs.push_back(std::move(instr));
    return instrs.back();
}

std::string Function::dump() const {
    std::ostringstream os;
    os << "func " << name << " {\n";
    for (auto& i : instrs) {
        os << "  " << i.to_string() << "\n";
    }
    os << "}";
    return os.str();
}

} // namespace deobf
