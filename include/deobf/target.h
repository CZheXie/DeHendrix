#pragma once
#include "deobf/ir.h"
#include "deobf/cfg.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace deobf {

/// VM-specific knowledge plugin interface.
/// Subclass this to add support for a new VM type (VMP3, Themida, ACE, custom).
class VMTarget {
public:
    virtual ~VMTarget() = default;

    virtual std::string name() const = 0;

    /// Return the register name used as VPC (Virtual Program Counter).
    virtual std::string vpc_reg() const = 0;

    /// Return the initial VPC concrete value (if known from the entry point).
    virtual std::optional<uint64_t> initial_vpc(uint64_t entry_va) const {
        return std::nullopt;
    }

    /// Identify if a given VA is the dispatcher loop entry.
    virtual bool is_dispatcher(uint64_t va) const { return false; }

    /// Classify a handler block's semantic meaning after devirtualization.
    virtual std::string classify_handler(const Function& handler_ir) const {
        return "unknown";
    }

    /// Detect if the current state represents a VMEXIT.
    virtual bool is_vmexit(uint64_t rsp_value, uint64_t init_rsp) const {
        return rsp_value == init_rsp;
    }

    /// Return memory ranges that are safe to constant-promote (VM bytecode regions).
    virtual std::vector<std::pair<uint64_t, uint64_t>> safe_ranges() const {
        return {};
    }

    /// Detect VJCC (virtual conditional jump) in handler IR.
    virtual bool is_vjcc(const Function& handler_ir) const { return false; }
};

/// VMP 3.x target
class VMP3Target : public VMTarget {
public:
    std::string name() const override { return "vmp3"; }
    std::string vpc_reg() const override { return "r11"; }

    bool is_vmexit(uint64_t rsp_value, uint64_t init_rsp) const override {
        uint64_t delta = rsp_value - init_rsp;
        return delta == 0 || delta == static_cast<uint64_t>(-0x10);
    }
};

/// Themida / CodeVirtualizer target
class ThemidaTarget : public VMTarget {
public:
    std::string name() const override { return "themida"; }
    std::string vpc_reg() const override { return "rbp"; }

    bool is_vmexit(uint64_t rsp_value, uint64_t init_rsp) const override {
        uint64_t delta = rsp_value - init_rsp;
        return delta == 0 || delta == static_cast<uint64_t>(-0x10);
    }

    bool is_vjcc(const Function& handler_ir) const override {
        for (auto& instr : handler_ir.instrs) {
            if (instr.op == Op::CJMP && !instr.operands.empty() &&
                !is_const(instr.operands[0]))
                return true;
        }
        return false;
    }
};

/// ACE custom VM target
class ACETarget : public VMTarget {
public:
    std::string name() const override { return "ace"; }
    std::string vpc_reg() const override { return "r11"; }
};

/// Generic / unknown VM target (auto-detect)
class GenericTarget : public VMTarget {
public:
    std::string name() const override { return "generic"; }
    std::string vpc_reg() const override { return "r11"; }
};

/// Factory: create target by name
std::unique_ptr<VMTarget> create_target(const std::string& name);

} // namespace deobf
