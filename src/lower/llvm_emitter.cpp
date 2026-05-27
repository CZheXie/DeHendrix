#include "deobf/llvm_emitter.h"
#include <iomanip>

namespace deobf {

std::string LLVMEmitter::EmitCtx::val_ref(const Value& v) {
    return std::visit([this](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, Const>) {
            return std::to_string(arg.value);
        } else if constexpr (std::is_same_v<T, SymReg>) {
            return "%" + arg.name;
        } else if constexpr (std::is_same_v<T, SymMem>) {
            return "%mem_" + std::to_string(arg.addr_ref);
        } else if constexpr (std::is_same_v<T, InstrRef>) {
            auto it = val_names.find(arg.id);
            if (it != val_names.end()) return "%" + it->second;
            return "%v" + std::to_string(arg.id);
        }
    }, v);
}

std::string LLVMEmitter::EmitCtx::new_tmp() {
    return "t" + std::to_string(tmp_counter++);
}

std::string LLVMEmitter::llvm_binop(Op op) {
    switch (op) {
        case Op::ADD: return "add";
        case Op::SUB: return "sub";
        case Op::MUL: return "mul";
        case Op::XOR: return "xor";
        case Op::AND: return "and";
        case Op::OR:  return "or";
        case Op::SHL: return "shl";
        case Op::SHR: return "lshr";
        case Op::SAR: return "ashr";
        default: return "add"; // fallback
    }
}

void LLVMEmitter::emit_instr(const Instruction& instr, EmitCtx& ctx) {
    std::string name = "v" + std::to_string(instr.result_id);
    ctx.val_names[instr.result_id] = name;

    switch (instr.op) {
        case Op::CONST:
        case Op::CONST_PTR: {
            // Constants are inlined in LLVM IR, no explicit instruction needed.
            // But we register the mapping so references work.
            if (!instr.operands.empty()) {
                auto* c = as_const(instr.operands[0]);
                if (c) {
                    // No instruction emitted; const is used inline
                    return;
                }
            }
            break;
        }
        case Op::ADD: case Op::SUB: case Op::MUL:
        case Op::XOR: case Op::AND: case Op::OR:
        case Op::SHL: case Op::SHR: case Op::SAR: {
            if (instr.operands.size() == 2) {
                ctx.os << "  %" << name << " = " << llvm_binop(instr.op)
                       << " i64 " << ctx.val_ref(instr.operands[0])
                       << ", " << ctx.val_ref(instr.operands[1]) << "\n";
            }
            break;
        }
        case Op::NOT: {
            if (instr.operands.size() == 1) {
                ctx.os << "  %" << name << " = xor i64 "
                       << ctx.val_ref(instr.operands[0]) << ", -1\n";
            }
            break;
        }
        case Op::NEG: {
            if (instr.operands.size() == 1) {
                ctx.os << "  %" << name << " = sub i64 0, "
                       << ctx.val_ref(instr.operands[0]) << "\n";
            }
            break;
        }
        case Op::LOAD: {
            if (instr.operands.size() == 1) {
                ctx.os << "  ; LOAD from " << ctx.val_ref(instr.operands[0]) << "\n";
                ctx.os << "  %" << name << " = inttoptr i64 "
                       << ctx.val_ref(instr.operands[0]) << " to ptr\n";
                std::string tmp = ctx.new_tmp();
                ctx.os << "  %" << tmp << " = load i64, ptr %" << name << "\n";
                ctx.val_names[instr.result_id] = tmp;
            }
            break;
        }
        case Op::STORE: {
            if (instr.operands.size() == 2) {
                std::string addr_tmp = ctx.new_tmp();
                ctx.os << "  %" << addr_tmp << " = inttoptr i64 "
                       << ctx.val_ref(instr.operands[0]) << " to ptr\n";
                ctx.os << "  store i64 " << ctx.val_ref(instr.operands[1])
                       << ", ptr %" << addr_tmp << "\n";
            }
            break;
        }
        case Op::RET: {
            if (instr.operands.size() == 1) {
                ctx.os << "  ret i64 " << ctx.val_ref(instr.operands[0]) << "\n";
            } else {
                ctx.os << "  ret i64 0\n";
            }
            break;
        }
        case Op::CALL: {
            if (instr.operands.size() >= 1) {
                ctx.os << "  ; CALL " << ctx.val_ref(instr.operands[0]) << "\n";
                ctx.os << "  %" << name << " = call i64 @external_call(i64 "
                       << ctx.val_ref(instr.operands[0]) << ")\n";
            }
            break;
        }
        case Op::PASSTHROUGH: {
            if (instr.operands.size() == 1) {
                // Identity: just alias the value
                auto* c = as_const(instr.operands[0]);
                if (c) {
                    // const passthrough: register as constant inline
                    return;
                }
                auto* ref = as_instrref(instr.operands[0]);
                if (ref) {
                    ctx.val_names[instr.result_id] = ctx.val_names[ref->id];
                    return;
                }
            }
            break;
        }
        case Op::NOP:
            break;
        default:
            ctx.os << "  ; unsupported op: " << op_name(instr.op) << "\n";
            break;
    }
}

std::string LLVMEmitter::emit_function(const Function& f,
                                        const std::string& func_name) {
    EmitCtx ctx;
    ctx.os << "; ModuleID = 'dehex_devirt'\n";
    ctx.os << "source_filename = \"dehex_devirt\"\n\n";
    ctx.os << "declare i64 @external_call(i64)\n\n";
    ctx.os << "define i64 @" << func_name << "(";

    // Collect symbolic registers as function parameters
    std::vector<std::string> sym_regs;
    for (auto& instr : f.instrs) {
        for (auto& op : instr.operands) {
            auto* sr = std::get_if<SymReg>(&op);
            if (sr) {
                if (std::find(sym_regs.begin(), sym_regs.end(), sr->name) == sym_regs.end())
                    sym_regs.push_back(sr->name);
            }
        }
    }

    for (size_t i = 0; i < sym_regs.size(); ++i) {
        if (i > 0) ctx.os << ", ";
        ctx.os << "i64 %" << sym_regs[i];
    }
    ctx.os << ") {\n";
    ctx.os << "entry:\n";

    for (auto& instr : f.instrs) {
        emit_instr(instr, ctx);
    }

    // If no RET was emitted, add a default one
    bool has_ret = false;
    for (auto& instr : f.instrs) {
        if (instr.op == Op::RET) { has_ret = true; break; }
    }
    if (!has_ret) ctx.os << "  ret i64 0\n";

    ctx.os << "}\n";
    return ctx.os.str();
}

std::string LLVMEmitter::emit_cfg_function(const CFGFunction& f,
                                            const std::string& func_name) {
    EmitCtx ctx;
    ctx.os << "; ModuleID = 'dehex_devirt'\n";
    ctx.os << "source_filename = \"dehex_devirt\"\n\n";
    ctx.os << "declare i64 @external_call(i64)\n\n";
    ctx.os << "define i64 @" << func_name << "() {\n";

    for (auto& bb : f.blocks) {
        ctx.os << bb.label << ":\n";

        for (auto& phi : bb.phis) {
            std::string name = "v" + std::to_string(phi.result_id);
            ctx.val_names[phi.result_id] = name;
            ctx.os << "  %" << name << " = phi i64 ";
            for (size_t i = 0; i < phi.incoming.size(); ++i) {
                if (i > 0) ctx.os << ", ";
                auto& [bid, val] = phi.incoming[i];
                auto* block = f.block_by_id(bid);
                std::string label = block ? block->label : ("bb" + std::to_string(bid));
                ctx.os << "[ " << ctx.val_ref(val) << ", %" << label << " ]";
            }
            ctx.os << "\n";
        }

        for (auto& instr : bb.instrs) {
            emit_instr(instr, ctx);
        }

        switch (bb.term.kind) {
            case TermKind::JMP:
                if (bb.term.target) {
                    auto* tgt = f.block_by_id(*bb.term.target);
                    ctx.os << "  br label %" << (tgt ? tgt->label : "???") << "\n";
                }
                break;
            case TermKind::CJMP:
                if (bb.term.target && bb.term.fallthrough && bb.term.condition) {
                    auto* tgt = f.block_by_id(*bb.term.target);
                    auto* ft = f.block_by_id(*bb.term.fallthrough);
                    std::string cond_tmp = ctx.new_tmp();
                    ctx.os << "  %" << cond_tmp << " = icmp ne i64 "
                           << ctx.val_ref(*bb.term.condition) << ", 0\n";
                    ctx.os << "  br i1 %" << cond_tmp
                           << ", label %" << (tgt ? tgt->label : "???")
                           << ", label %" << (ft ? ft->label : "???") << "\n";
                }
                break;
            case TermKind::RET:
                ctx.os << "  ret i64 0\n";
                break;
            case TermKind::FALLTHROUGH:
                if (bb.term.fallthrough) {
                    auto* ft = f.block_by_id(*bb.term.fallthrough);
                    ctx.os << "  br label %" << (ft ? ft->label : "???") << "\n";
                }
                break;
            case TermKind::UNREACHABLE:
                ctx.os << "  unreachable\n";
                break;
        }
    }

    ctx.os << "}\n";
    return ctx.os.str();
}

} // namespace deobf
