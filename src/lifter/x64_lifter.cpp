#include "deobf/lifter.h"
#include <sstream>

#ifdef DEOBF_HAS_CAPSTONE
#include <capstone/capstone.h>
#endif

namespace deobf {

LiftState::LiftState(Function& f, uint64_t rsp_concrete) : func(f) {
    const char* gpr_names[] = {
        "rax", "rcx", "rdx", "rbx", "rbp", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
    };
    for (auto name : gpr_names) {
        regs[name] = SymReg(name);
    }
    regs["rsp"] = Const(rsp_concrete);
    regs["rflags"] = SymReg("rflags");
}

Value LiftState::get_reg(const std::string& name) const {
    auto it = regs.find(name);
    if (it != regs.end()) return it->second;
    return SymReg(name);
}

void LiftState::set_reg(const std::string& name, Value val) {
    regs[name] = std::move(val);
}

Instruction& LiftState::emit(Op op, std::vector<Value> operands,
                              std::unordered_map<std::string, std::string> ann) {
    return func.emit(op, std::move(operands), std::move(ann));
}

#ifdef DEOBF_HAS_CAPSTONE

static const std::unordered_map<x86_reg, std::string>& gpr64_map() {
    static const std::unordered_map<x86_reg, std::string> m = {
        // 64-bit
        {X86_REG_RAX, "rax"}, {X86_REG_RCX, "rcx"}, {X86_REG_RDX, "rdx"},
        {X86_REG_RBX, "rbx"}, {X86_REG_RSP, "rsp"}, {X86_REG_RBP, "rbp"},
        {X86_REG_RSI, "rsi"}, {X86_REG_RDI, "rdi"},
        {X86_REG_R8,  "r8"},  {X86_REG_R9,  "r9"},  {X86_REG_R10, "r10"},
        {X86_REG_R11, "r11"}, {X86_REG_R12, "r12"}, {X86_REG_R13, "r13"},
        {X86_REG_R14, "r14"}, {X86_REG_R15, "r15"},
        // 32-bit → treat as 64-bit parent (zero-extend semantics)
        {X86_REG_EAX, "rax"}, {X86_REG_ECX, "rcx"}, {X86_REG_EDX, "rdx"},
        {X86_REG_EBX, "rbx"}, {X86_REG_ESP, "rsp"}, {X86_REG_EBP, "rbp"},
        {X86_REG_ESI, "rsi"}, {X86_REG_EDI, "rdi"},
        {X86_REG_R8D,  "r8"},  {X86_REG_R9D,  "r9"},  {X86_REG_R10D, "r10"},
        {X86_REG_R11D, "r11"}, {X86_REG_R12D, "r12"}, {X86_REG_R13D, "r13"},
        {X86_REG_R14D, "r14"}, {X86_REG_R15D, "r15"},
        // 16-bit
        {X86_REG_AX, "rax"}, {X86_REG_CX, "rcx"}, {X86_REG_DX, "rdx"},
        {X86_REG_BX, "rbx"}, {X86_REG_SP, "rsp"}, {X86_REG_BP, "rbp"},
        {X86_REG_SI, "rsi"}, {X86_REG_DI, "rdi"},
        {X86_REG_R8W,  "r8"},  {X86_REG_R9W,  "r9"},  {X86_REG_R10W, "r10"},
        {X86_REG_R11W, "r11"}, {X86_REG_R12W, "r12"}, {X86_REG_R13W, "r13"},
        {X86_REG_R14W, "r14"}, {X86_REG_R15W, "r15"},
        // 8-bit low
        {X86_REG_AL, "rax"}, {X86_REG_CL, "rcx"}, {X86_REG_DL, "rdx"},
        {X86_REG_BL, "rbx"}, {X86_REG_SPL, "rsp"}, {X86_REG_BPL, "rbp"},
        {X86_REG_SIL, "rsi"}, {X86_REG_DIL, "rdi"},
        {X86_REG_R8B,  "r8"},  {X86_REG_R9B,  "r9"},  {X86_REG_R10B, "r10"},
        {X86_REG_R11B, "r11"}, {X86_REG_R12B, "r12"}, {X86_REG_R13B, "r13"},
        {X86_REG_R14B, "r14"}, {X86_REG_R15B, "r15"},
        // 8-bit high
        {X86_REG_AH, "rax"}, {X86_REG_CH, "rcx"}, {X86_REG_DH, "rdx"},
        {X86_REG_BH, "rbx"},
        // RIP
        {X86_REG_RIP, "rip"}, {X86_REG_EIP, "rip"},
        // EFLAGS
        {X86_REG_EFLAGS, "rflags"},
    };
    return m;
}

static std::string reg_name(x86_reg r) {
    auto& m = gpr64_map();
    auto it = m.find(r);
    if (it != m.end()) return it->second;
    return "unk_" + std::to_string(static_cast<int>(r));
}

static bool is_rip(x86_reg r) {
    return r == X86_REG_RIP || r == X86_REG_EIP;
}

static Value op_value(const cs_insn* insn, int idx, LiftState& state) {
    auto& op = insn->detail->x86.operands[idx];
    std::ostringstream va_str;
    va_str << "0x" << std::hex << insn->address;

    if (op.type == X86_OP_REG) {
        if (is_rip(static_cast<x86_reg>(op.reg)))
            return Const((insn->address + insn->size) & MASK64);
        return state.get_reg(reg_name(static_cast<x86_reg>(op.reg)));
    }
    if (op.type == X86_OP_IMM) {
        return Const(static_cast<uint64_t>(op.imm) & MASK64);
    }
    if (op.type == X86_OP_MEM) {
        Value base_val;
        if (op.mem.base && is_rip(static_cast<x86_reg>(op.mem.base)))
            base_val = Const((insn->address + insn->size) & MASK64);
        else if (op.mem.base)
            base_val = state.get_reg(reg_name(static_cast<x86_reg>(op.mem.base)));
        else
            base_val = Const(0);
        Value disp_val = Const(static_cast<uint64_t>(op.mem.disp) & MASK64);
        auto& addr_instr = state.emit(Op::ADD, {base_val, disp_val}, {{"addr_calc", "1"}});
        Value addr_ref = addr_instr.ref();

        if (op.mem.index) {
            Value idx_val = state.get_reg(reg_name(static_cast<x86_reg>(op.mem.index)));
            Value scale_val = Const(static_cast<uint64_t>(op.mem.scale) & MASK64);
            auto& scaled = state.emit(Op::MUL, {idx_val, scale_val});
            auto& final_addr = state.emit(Op::ADD, {addr_ref, scaled.ref()});
            addr_ref = final_addr.ref();
        }

        auto& load = state.emit(Op::LOAD, {addr_ref}, {{"va", va_str.str()}});
        return load.ref();
    }
    return SymReg("unknown");
}

void lift_insn(const cs_insn* insn, LiftState& state) {
    const char* m = insn->mnemonic;
    std::string mnem(m);
    int op_count = insn->detail->x86.op_count;

    std::ostringstream va_str;
    va_str << "0x" << std::hex << insn->address;

    if (mnem == "nop" || mnem.substr(0, 5) == "endbr") return;

    if (mnem == "push" && op_count >= 1) {
        Value val = op_value(insn, 0, state);
        Value rsp = state.get_reg("rsp");
        auto& new_rsp = state.emit(Op::SUB, {rsp, Const(8)});
        state.emit(Op::STORE, {new_rsp.ref(), val}, {{"va", va_str.str()}});
        state.set_reg("rsp", new_rsp.ref());
        return;
    }

    if (mnem == "pop" && op_count >= 1) {
        Value rsp = state.get_reg("rsp");
        auto& loaded = state.emit(Op::LOAD, {rsp}, {{"va", va_str.str()}});
        auto& new_rsp = state.emit(Op::ADD, {rsp, Const(8)});
        state.set_reg("rsp", new_rsp.ref());
        auto& dst = insn->detail->x86.operands[0];
        if (dst.type == X86_OP_REG)
            state.set_reg(reg_name(static_cast<x86_reg>(dst.reg)), loaded.ref());
        return;
    }

    if ((mnem == "mov" || mnem == "movabs") && op_count >= 2) {
        Value src = op_value(insn, 1, state);
        auto& dst = insn->detail->x86.operands[0];
        if (dst.type == X86_OP_REG) {
            state.set_reg(reg_name(static_cast<x86_reg>(dst.reg)), src);
        } else if (dst.type == X86_OP_MEM) {
            Value base_val = dst.mem.base
                ? state.get_reg(reg_name(static_cast<x86_reg>(dst.mem.base)))
                : Value(Const(0));
            Value disp_val = Const(static_cast<uint64_t>(dst.mem.disp) & MASK64);
            auto& addr = state.emit(Op::ADD, {base_val, disp_val});
            state.emit(Op::STORE, {addr.ref(), src}, {{"va", va_str.str()}});
        }
        return;
    }

    if (mnem == "lea" && op_count >= 2) {
        auto& dst_op = insn->detail->x86.operands[0];
        auto& mem_op = insn->detail->x86.operands[1];
        if (mem_op.type == X86_OP_MEM) {
            Value base_val;
            if (mem_op.mem.base && is_rip(static_cast<x86_reg>(mem_op.mem.base)))
                base_val = Const((insn->address + insn->size) & MASK64);
            else if (mem_op.mem.base)
                base_val = state.get_reg(reg_name(static_cast<x86_reg>(mem_op.mem.base)));
            else
                base_val = Const(0);
            Value disp_val = Const(static_cast<uint64_t>(mem_op.mem.disp) & MASK64);
            auto& result = state.emit(Op::ADD, {base_val, disp_val});
            Value result_ref = result.ref();
            if (mem_op.mem.index) {
                Value idx_val = state.get_reg(reg_name(static_cast<x86_reg>(mem_op.mem.index)));
                Value scale_val = Const(static_cast<uint64_t>(mem_op.mem.scale) & MASK64);
                auto& scaled = state.emit(Op::MUL, {idx_val, scale_val});
                auto& combined = state.emit(Op::ADD, {result_ref, scaled.ref()});
                result_ref = combined.ref();
            }
            if (dst_op.type == X86_OP_REG)
                state.set_reg(reg_name(static_cast<x86_reg>(dst_op.reg)), result_ref);
        }
        return;
    }

    // Binary ops
    static const std::unordered_map<std::string, Op> binop_map = {
        {"add", Op::ADD}, {"sub", Op::SUB}, {"xor", Op::XOR},
        {"and", Op::AND}, {"or", Op::OR}, {"shl", Op::SHL},
        {"shr", Op::SHR}, {"sar", Op::SAR}, {"rol", Op::ROL}, {"ror", Op::ROR},
    };
    auto binit = binop_map.find(mnem);
    if (binit != binop_map.end() && op_count == 2) {
        Value a = op_value(insn, 0, state);
        Value b = op_value(insn, 1, state);
        auto& result = state.emit(binit->second, {a, b}, {{"va", va_str.str()}});
        auto& dst = insn->detail->x86.operands[0];
        if (dst.type == X86_OP_REG)
            state.set_reg(reg_name(static_cast<x86_reg>(dst.reg)), result.ref());
        return;
    }

    // Unary ops
    static const std::unordered_map<std::string, Op> unop_map = {
        {"not", Op::NOT}, {"neg", Op::NEG}, {"bswap", Op::BSWAP},
    };
    auto unit = unop_map.find(mnem);
    if (unit != unop_map.end() && op_count == 1) {
        Value a = op_value(insn, 0, state);
        auto& result = state.emit(unit->second, {a}, {{"va", va_str.str()}});
        auto& dst = insn->detail->x86.operands[0];
        if (dst.type == X86_OP_REG)
            state.set_reg(reg_name(static_cast<x86_reg>(dst.reg)), result.ref());
        return;
    }

    // inc / dec : modeled as +/- 1 (CF is left untouched by these on x86)
    if ((mnem == "inc" || mnem == "dec") && op_count >= 1) {
        Value a = op_value(insn, 0, state);
        Op op = (mnem == "inc") ? Op::ADD : Op::SUB;
        auto& result = state.emit(op, {a, Const(1)}, {{"va", va_str.str()}});
        auto& dst = insn->detail->x86.operands[0];
        if (dst.type == X86_OP_REG) {
            state.set_reg(reg_name(static_cast<x86_reg>(dst.reg)), result.ref());
        } else if (dst.type == X86_OP_MEM) {
            Value base_val = dst.mem.base
                ? state.get_reg(reg_name(static_cast<x86_reg>(dst.mem.base)))
                : Value(Const(0));
            Value disp_val = Const(static_cast<uint64_t>(dst.mem.disp) & MASK64);
            auto& addr = state.emit(Op::ADD, {base_val, disp_val});
            state.emit(Op::STORE, {addr.ref(), result.ref()}, {{"va", va_str.str()}});
        }
        return;
    }

    if (mnem == "call" && op_count >= 1) {
        Value target = op_value(insn, 0, state);
        // x86-64 `call` pushes the return address (rsp -= 8; [rsp] = next_va).
        // Modeling this is required so a followed callee's stack-argument offsets
        // (e.g. ACE VM dispatch reading the bytecode/VPC via [rbp+0xA0]) resolve
        // to concrete values instead of staying symbolic.
        uint64_t ret_va = (insn->address + insn->size) & MASK64;
        Value rsp = state.get_reg("rsp");
        auto& new_rsp = state.emit(Op::SUB, {rsp, Const(8)});
        state.emit(Op::STORE, {new_rsp.ref(), Const(ret_va)}, {{"va", va_str.str()}});
        state.set_reg("rsp", new_rsp.ref());
        state.emit(Op::CALL, {target}, {{"va", va_str.str()}});
        return;
    }

    if (mnem == "ret") {
        Value rsp = state.get_reg("rsp");
        auto& ret_addr = state.emit(Op::LOAD, {rsp});
        state.emit(Op::RET, {ret_addr.ref()});
        return;
    }

    if (mnem == "jmp" && op_count >= 1) {
        Value target = op_value(insn, 0, state);
        state.emit(Op::JMP, {target}, {{"va", va_str.str()}});
        return;
    }

    if (mnem == "pushfq") {
        Value rsp = state.get_reg("rsp");
        auto& new_rsp = state.emit(Op::SUB, {rsp, Const(8)});
        state.emit(Op::STORE, {new_rsp.ref(), state.get_reg("rflags")});
        state.set_reg("rsp", new_rsp.ref());
        return;
    }

    if (mnem == "popfq") {
        Value rsp = state.get_reg("rsp");
        auto& flags = state.emit(Op::LOAD, {rsp});
        auto& new_rsp = state.emit(Op::ADD, {rsp, Const(8)});
        state.set_reg("rsp", new_rsp.ref());
        state.set_reg("rflags", flags.ref());
        return;
    }

    if (mnem == "test" && op_count == 2) {
        Value a = op_value(insn, 0, state);
        Value b = op_value(insn, 1, state);
        auto& result = state.emit(Op::AND, {a, b}, {{"va", va_str.str()}, {"flags_only", "1"}});
        state.set_reg("rflags", result.ref());
        return;
    }

    if (mnem == "cmp" && op_count == 2) {
        Value a = op_value(insn, 0, state);
        Value b = op_value(insn, 1, state);
        auto& result = state.emit(Op::SUB, {a, b}, {{"va", va_str.str()}, {"flags_only", "1"}});
        state.set_reg("rflags", result.ref());
        return;
    }

    // Conditional jumps (jcc): emit CJMP with rflags as condition
    static const std::vector<std::string> jcc_mnems = {
        "je", "jz", "jne", "jnz", "ja", "jb", "jae", "jbe",
        "jg", "jl", "jge", "jle", "js", "jns", "jo", "jno",
    };
    bool is_jcc = false;
    for (auto& j : jcc_mnems) {
        if (mnem == j) { is_jcc = true; break; }
    }
    if (is_jcc && op_count >= 1) {
        Value target = op_value(insn, 0, state);
        Value flags = state.get_reg("rflags");
        state.emit(Op::CJMP, {flags, target}, {{"va", va_str.str()}, {"jcc", mnem}});
        return;
    }

    if (mnem == "xchg" && op_count == 2) {
        auto& a_op = insn->detail->x86.operands[0];
        auto& b_op = insn->detail->x86.operands[1];
        if (a_op.type == X86_OP_REG && b_op.type == X86_OP_REG) {
            std::string a_name = reg_name(static_cast<x86_reg>(a_op.reg));
            std::string b_name = reg_name(static_cast<x86_reg>(b_op.reg));
            Value a_val = state.get_reg(a_name);
            Value b_val = state.get_reg(b_name);
            state.set_reg(a_name, b_val);
            state.set_reg(b_name, a_val);
        }
        return;
    }

    if (mnem == "cdqe") {
        // sign-extend eax to rax (32→64)
        Value rax = state.get_reg("rax");
        auto& result = state.emit(Op::SEXT, {rax}, {{"va", va_str.str()}});
        state.set_reg("rax", result.ref());
        return;
    }

    if (mnem == "movzx" && op_count >= 2) {
        Value src = op_value(insn, 1, state);
        auto& result = state.emit(Op::ZEXT, {src}, {{"va", va_str.str()}});
        auto& dst = insn->detail->x86.operands[0];
        if (dst.type == X86_OP_REG)
            state.set_reg(reg_name(static_cast<x86_reg>(dst.reg)), result.ref());
        return;
    }

    if (mnem == "movsx" || mnem == "movsxd") {
        if (op_count >= 2) {
            Value src = op_value(insn, 1, state);
            auto& result = state.emit(Op::SEXT, {src}, {{"va", va_str.str()}});
            auto& dst = insn->detail->x86.operands[0];
            if (dst.type == X86_OP_REG)
                state.set_reg(reg_name(static_cast<x86_reg>(dst.reg)), result.ref());
        }
        return;
    }

    // setcc dst : dst = (flag condition) ? 1 : 0, modeled as SELECT(rflags, 1, 0).
    // The precise cc only labels which arm is "taken"; for VJCC target recovery
    // both arms are extracted regardless.
    if (mnem.rfind("set", 0) == 0 && mnem.size() > 3 && op_count >= 1) {
        Value flags = state.get_reg("rflags");
        auto& result = state.emit(Op::SELECT, {flags, Const(1), Const(0)},
                                  {{"va", va_str.str()}, {"setcc", mnem}});
        auto& dst = insn->detail->x86.operands[0];
        if (dst.type == X86_OP_REG)
            state.set_reg(reg_name(static_cast<x86_reg>(dst.reg)), result.ref());
        return;
    }

    // cmovcc dst, src : dst = cond ? src : dst_old, modeled as SELECT(rflags, src, dst_old).
    if (mnem.rfind("cmov", 0) == 0 && op_count >= 2) {
        auto& dst = insn->detail->x86.operands[0];
        if (dst.type == X86_OP_REG) {
            Value src = op_value(insn, 1, state);
            std::string dn = reg_name(static_cast<x86_reg>(dst.reg));
            Value old = state.get_reg(dn);
            Value flags = state.get_reg("rflags");
            auto& result = state.emit(Op::SELECT, {flags, src, old},
                                      {{"va", va_str.str()}, {"cmov", mnem}});
            state.set_reg(dn, result.ref());
        }
        return;
    }

    state.emit(Op::NOP, {}, {{"va", va_str.str()}, {"unsupported", mnem}});
}

void lift_block(const uint8_t* code, size_t code_len, uint64_t base_va,
                Function& func, LiftState& state, int max_instrs) {
    csh handle;
    if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK) return;
    cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

    cs_insn* insn;
    size_t count = cs_disasm(handle, code, code_len, base_va, 0, &insn);
    int lifted = 0;
    for (size_t i = 0; i < count && lifted < max_instrs; ++i) {
        lift_insn(&insn[i], state);
        lifted++;
        const char* m = insn[i].mnemonic;
        if (std::string(m) == "ret" || std::string(m) == "jmp") break;
    }
    cs_free(insn, count);
    cs_close(&handle);
}

#endif // DEOBF_HAS_CAPSTONE

} // namespace deobf
