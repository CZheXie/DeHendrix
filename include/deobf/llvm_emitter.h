#pragma once
#include "deobf/ir.h"
#include "deobf/cfg.h"
#include <string>
#include <sstream>
#include <unordered_map>

namespace deobf {

/// Emit our IR as LLVM IR text (.ll format) that can be compiled with clang.
/// This provides a "poor man's lowering" path: dump .ll → clang -O3 → native.
class LLVMEmitter {
public:
    /// Emit a single linear Function as an LLVM IR module string.
    static std::string emit_function(const Function& f,
                                     const std::string& func_name = "devirt");

    /// Emit a CFGFunction as an LLVM IR module string with basic blocks.
    static std::string emit_cfg_function(const CFGFunction& f,
                                         const std::string& func_name = "devirt");

private:
    struct EmitCtx {
        std::ostringstream os;
        std::unordered_map<uint32_t, std::string> val_names;
        int tmp_counter = 0;

        std::string val_ref(const Value& v);
        std::string new_tmp();
    };

    static void emit_instr(const Instruction& instr, EmitCtx& ctx);
    static std::string llvm_binop(Op op);
};

} // namespace deobf
