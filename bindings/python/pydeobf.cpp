/// pybind11 bindings for libdeobf
///
/// Usage from Python:
///   import deobf
///   f = deobf.Function("test")
///   f.emit(deobf.Op.CONST, [deobf.Const(42)])
///   deobf.run_to_convergence(f)
///   print(f.dump())
///
///   ev = deobf.GuidedEvaluator(image_bytes, 0x140000000, "r11")
///   ev.add_safe_range(0x140B45000, 0x14196B000)
///   report = ev.run(0x14132C758)

#ifdef DEOBF_HAS_PYBIND11

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
namespace py = pybind11;

#include "deobf/ir.h"
#include "deobf/cfg.h"
#include "deobf/passes.h"
#include "deobf/memory.h"
#include "deobf/lifter.h"
#include "deobf/evaluator.h"

using namespace deobf;

PYBIND11_MODULE(deobf, m) {
    m.doc() = "libdeobf — C++17 binary deobfuscation framework";

    // --- Value types ---
    py::class_<Const>(m, "Const")
        .def(py::init<uint64_t, uint8_t>(), py::arg("value"), py::arg("bits") = 64)
        .def_readwrite("value", &Const::value)
        .def_readwrite("bits", &Const::bits)
        .def("__repr__", [](const Const& c) {
            return "#0x" + std::to_string(c.value);
        });

    py::class_<SymReg>(m, "SymReg")
        .def(py::init<std::string>())
        .def_readwrite("name", &SymReg::name);

    py::class_<InstrRef>(m, "InstrRef")
        .def(py::init<uint32_t>())
        .def_readwrite("id", &InstrRef::id);

    // --- Op enum ---
    py::enum_<Op>(m, "Op")
        .value("CONST", Op::CONST)
        .value("CONST_PTR", Op::CONST_PTR)
        .value("ADD", Op::ADD).value("SUB", Op::SUB).value("MUL", Op::MUL)
        .value("XOR", Op::XOR).value("AND", Op::AND).value("OR", Op::OR)
        .value("SHL", Op::SHL).value("SHR", Op::SHR).value("SAR", Op::SAR)
        .value("ROL", Op::ROL).value("ROR", Op::ROR)
        .value("NOT", Op::NOT).value("NEG", Op::NEG).value("BSWAP", Op::BSWAP)
        .value("ZEXT", Op::ZEXT).value("SEXT", Op::SEXT).value("TRUNC", Op::TRUNC)
        .value("LOAD", Op::LOAD).value("STORE", Op::STORE)
        .value("CALL", Op::CALL).value("RET", Op::RET)
        .value("JMP", Op::JMP).value("CJMP", Op::CJMP)
        .value("PASSTHROUGH", Op::PASSTHROUGH).value("NOP", Op::NOP);

    // --- Instruction ---
    py::class_<Instruction>(m, "Instruction")
        .def_readwrite("op", &Instruction::op)
        .def_readwrite("result_id", &Instruction::result_id)
        .def("to_string", &Instruction::to_string)
        .def("ref", &Instruction::ref);

    // --- Function ---
    py::class_<Function>(m, "Function")
        .def(py::init<std::string>(), py::arg("name") = "unnamed")
        .def_readwrite("name", &Function::name)
        .def("dump", &Function::dump)
        .def("__len__", [](const Function& f) { return f.instrs.size(); });

    // --- Passes ---
    m.def("const_promote_pass", &const_promote_pass);
    m.def("const_fold_pass", &const_fold_pass);
    m.def("mem_forward_pass", &mem_forward_pass);
    m.def("dead_store_elim_pass", &dead_store_elim_pass);
    m.def("dead_dep_elim_pass", &dead_dep_elim_pass);
    m.def("insn_combine_pass", &insn_combine_pass);
    m.def("branch_fold_pass", &branch_fold_pass);

    py::class_<ConvergenceReport>(m, "ConvergenceReport")
        .def_readwrite("iterations", &ConvergenceReport::iterations)
        .def_readwrite("pass_hits", &ConvergenceReport::pass_hits);

    m.def("run_to_convergence", &run_to_convergence,
          py::arg("f"), py::arg("max_iters") = 32);

    // --- ByteMemory ---
    py::class_<ByteMemory>(m, "ByteMemory")
        .def(py::init<>())
        .def("add_safe_range", &ByteMemory::add_safe_range)
        .def("is_safe", &ByteMemory::is_safe)
        .def("store_u64", &ByteMemory::store_u64)
        .def("load_u64", &ByteMemory::load_u64);

    // --- EvalResult ---
    py::enum_<EvalResult>(m, "EvalResult")
        .value("RET", EvalResult::RET)
        .value("VMEXIT_RET", EvalResult::VMEXIT_RET)
        .value("VMEXIT_CALL", EvalResult::VMEXIT_CALL)
        .value("UNRESOLVED_TARGET", EvalResult::UNRESOLVED_TARGET)
        .value("TARGET_OUTSIDE_IMAGE", EvalResult::TARGET_OUTSIDE_IMAGE)
        .value("VPC_CONVERGED", EvalResult::VPC_CONVERGED)
        .value("VPC_UNRESOLVED", EvalResult::VPC_UNRESOLVED)
        .value("INNER_LOOP", EvalResult::INNER_LOOP)
        .value("MAX_STEPS", EvalResult::MAX_STEPS)
        .value("MAX_VM_INSNS", EvalResult::MAX_VM_INSNS)
        .value("OUT_OF_BOUNDS", EvalResult::OUT_OF_BOUNDS);

    // --- EvalReport ---
    py::class_<EvalReport>(m, "EvalReport")
        .def_readwrite("result", &EvalReport::result)
        .def_readwrite("stop_va", &EvalReport::stop_va)
        .def_readwrite("steps", &EvalReport::steps)
        .def_readwrite("vm_insn_count", &EvalReport::vm_insn_count)
        .def_readwrite("ir_count", &EvalReport::ir_count);

    // --- GuidedEvaluator ---
    py::class_<GuidedEvaluator>(m, "GuidedEvaluator")
        .def(py::init([](py::bytes image, uint64_t base, const std::string& vpc_reg) {
            std::string data = image;
            auto* ev = new GuidedEvaluator(
                reinterpret_cast<const uint8_t*>(data.data()),
                data.size(), base, vpc_reg);
            return ev;
        }), py::arg("image"), py::arg("base") = 0x140000000,
            py::arg("vpc_reg") = "r11")
        .def("add_safe_range", &GuidedEvaluator::add_safe_range)
        .def("run", &GuidedEvaluator::run,
             py::arg("start_va"), py::arg("max_steps") = 50000,
             py::arg("max_vm_insns") = 2000)
        .def("dump_handler_catalog", &GuidedEvaluator::dump_handler_catalog)
        .def("dump_vpc_trace", &GuidedEvaluator::dump_vpc_trace);
}

#endif // DEOBF_HAS_PYBIND11
