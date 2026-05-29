#pragma once
#include <cstdint>
#include <string>
#include <variant>
#include <vector>
#include <unordered_map>

namespace deobf {

// --- Values ---

struct Const {
    uint64_t value;
    uint8_t  bits;
    Const() : value(0), bits(64) {}
    Const(uint64_t v, uint8_t b = 64) : value(v), bits(b) {}
    bool operator==(const Const& o) const { return value == o.value && bits == o.bits; }
};

struct SymReg {
    std::string name;
    SymReg() = default;
    explicit SymReg(std::string n) : name(std::move(n)) {}
    bool operator==(const SymReg& o) const { return name == o.name; }
};

struct SymMem {
    uint32_t addr_ref;
    uint8_t  bits;
    SymMem() : addr_ref(0), bits(64) {}
    SymMem(uint32_t r, uint8_t b = 64) : addr_ref(r), bits(b) {}
    bool operator==(const SymMem& o) const { return addr_ref == o.addr_ref && bits == o.bits; }
};

struct InstrRef {
    uint32_t id;
    InstrRef() : id(0) {}
    explicit InstrRef(uint32_t i) : id(i) {}
    bool operator==(const InstrRef& o) const { return id == o.id; }
};

using Value = std::variant<Const, SymReg, SymMem, InstrRef>;

inline bool is_const(const Value& v) { return std::holds_alternative<Const>(v); }
inline bool is_instrref(const Value& v) { return std::holds_alternative<InstrRef>(v); }

inline const Const* get_const(const Value& v) {
    return std::get_if<Const>(&v);
}
inline const InstrRef* get_instrref(const Value& v) {
    return std::get_if<InstrRef>(&v);
}

std::string value_to_string(const Value& v);

// --- Op ---

enum class Op : uint8_t {
    CONST, CONST_PTR,
    ADD, SUB, MUL, XOR, AND, OR, SHL, SHR, SAR, ROL, ROR,
    NOT, NEG, BSWAP, ZEXT, SEXT, TRUNC,
    LOAD, STORE,
    CALL, RET, JMP, CJMP,
    PASSTHROUGH, NOP,
    SELECT,   // SELECT(cond, a, b) => cond != 0 ? a : b  (models cmov / setcc-driven choices)
};

const char* op_name(Op op);
bool op_is_side_effectful(Op op);
bool op_is_foldable_binary(Op op);
bool op_is_foldable_unary(Op op);

// --- Instruction ---

struct Instruction {
    Op op;
    std::vector<Value> operands;
    uint32_t result_id;
    std::unordered_map<std::string, std::string> annotations;

    InstrRef ref() const { return InstrRef(result_id); }
    std::string to_string() const;
};

// --- Function (single linear block, SSA-lite) ---

class Function {
public:
    std::string name;
    std::vector<Instruction> instrs;

    explicit Function(std::string n = "unnamed") : name(std::move(n)) {}

    Instruction& emit(Op op, std::vector<Value> operands,
                      std::unordered_map<std::string, std::string> ann = {});

    uint32_t next_id() const { return next_id_; }
    /// Set the next result_id to be assigned. Used to give instructions emitted
    /// across multiple segment evaluations a globally-unique id space.
    void set_next_id(uint32_t id) { next_id_ = id; }
    std::string dump() const;

private:
    uint32_t next_id_ = 0;
};

} // namespace deobf
