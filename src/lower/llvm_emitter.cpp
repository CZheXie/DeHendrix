#include "deobf/llvm_emitter.h"
#include "deobf/passes.h"
#include <iomanip>

namespace deobf {

std::string LLVMEmitter::EmitCtx::val_ref(const Value& v) {
    return std::visit([this](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, Const>) {
            return std::to_string(arg.value);
        } else if constexpr (std::is_same_v<T, SymReg>) {
            if (arg.name.empty()) return "0";
            return "%" + arg.name;
        } else if constexpr (std::is_same_v<T, SymMem>) {
            return "%mem_" + std::to_string(arg.addr_ref);
        } else if constexpr (std::is_same_v<T, InstrRef>) {
            auto it = val_names.find(arg.id);
            if (it != val_names.end()) {
                auto& name = it->second;
                if (!name.empty() && (std::isdigit(name[0]) || name[0] == '-'))
                    return name;
                if (!name.empty())
                    return "%" + name;
            }
            // Fallback: resolve through instr_map if available
            if (instr_map) {
                Value resolved = resolve(Value(InstrRef(arg.id)), *instr_map);
                auto* c = get_const(resolved);
                if (c) return std::to_string(c->value);
                auto* sr = std::get_if<SymReg>(&resolved);
                if (sr && !sr->name.empty()) return "%" + sr->name;
                // Instruction was eliminated - use 0 as fallback
                if (instr_map->find(arg.id) == instr_map->end())
                    return "0";
            }
            return "0";
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
            if (!instr.operands.empty()) {
                auto* c = get_const(instr.operands[0]);
                if (c) {
                    ctx.os << "  %" << name << " = add i64 "
                           << std::to_string(c->value) << ", 0\n";
                    return;
                }
            }
            ctx.os << "  %" << name << " = add i64 0, 0\n";
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
                ctx.os << "  store volatile i64 " << ctx.val_ref(instr.operands[1])
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
                auto* c = get_const(instr.operands[0]);
                if (c) {
                    ctx.val_names[instr.result_id] = std::to_string(c->value);
                    return;
                }
                auto* ref = get_instrref(instr.operands[0]);
                if (ref) {
                    auto it = ctx.val_names.find(ref->id);
                    if (it != ctx.val_names.end() && !it->second.empty()) {
                        ctx.val_names[instr.result_id] = it->second;
                    } else {
                        ctx.val_names[instr.result_id] = "v" + std::to_string(ref->id);
                    }
                    return;
                }
                auto* sr = std::get_if<SymReg>(&instr.operands[0]);
                if (sr) {
                    ctx.val_names[instr.result_id] = sr->name.empty() ? "unk" : sr->name;
                    return;
                }
            }
            ctx.val_names[instr.result_id] = name;
            break;
        }
        case Op::SELECT: {
            if (instr.operands.size() == 3) {
                std::string cmp = ctx.new_tmp();
                ctx.os << "  %" << cmp << " = icmp ne i64 "
                       << ctx.val_ref(instr.operands[0]) << ", 0\n";
                ctx.os << "  %" << name << " = select i1 %" << cmp
                       << ", i64 " << ctx.val_ref(instr.operands[1])
                       << ", i64 " << ctx.val_ref(instr.operands[2]) << "\n";
            }
            break;
        }
        case Op::NOP:
            ctx.os << "  %" << name << " = add i64 0, 0 ; nop\n";
            break;
        case Op::JMP:
        case Op::CJMP:
            ctx.os << "  %" << name << " = add i64 0, 0 ; " << op_name(instr.op) << "\n";
            break;
        default:
            ctx.os << "  %" << name << " = add i64 0, 0 ; unsupported: " << op_name(instr.op) << "\n";
            break;
    }
}

std::string LLVMEmitter::emit_function(const Function& f,
                                        const std::string& func_name) {
    EmitCtx ctx;
    ctx.os << "; ModuleID = 'dehex_devirt_v2'\n";
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

    // Build instruction map and resolve all InstrRef operands to their final values
    auto im = build_instr_map(f);
    ctx.instr_map = &im;
    // Pre-register all resolvable instruction results
    for (auto& instr : f.instrs) {
        if (instr.op == Op::NOP) continue;
        if (instr.op == Op::PASSTHROUGH && instr.operands.size() == 1) {
            Value resolved = resolve(instr.operands[0], im);
            auto* c = get_const(resolved);
            if (c) { ctx.val_names[instr.result_id] = std::to_string(c->value); continue; }
            auto* sr = std::get_if<SymReg>(&resolved);
            if (sr && !sr->name.empty()) { ctx.val_names[instr.result_id] = sr->name; continue; }
        }
        if ((instr.op == Op::CONST || instr.op == Op::CONST_PTR) && !instr.operands.empty()) {
            auto* c = get_const(instr.operands[0]);
            if (c) { ctx.val_names[instr.result_id] = std::to_string(c->value); continue; }
        }
        // For NOPped/const-folded instructions that are still referenced
        if (instr.op == Op::CONST && instr.operands.empty()) {
            ctx.val_names[instr.result_id] = "0";
        }
    }
    // Also resolve any NOP'd instructions that are referenced
    for (auto& instr : f.instrs) {
        if (instr.op != Op::NOP) continue;
        if (ctx.val_names.count(instr.result_id)) continue;
        ctx.val_names[instr.result_id] = "0";
    }

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
