#!/usr/bin/env python3
"""
DeHendrix Python usage example.

Build the Python module first:
    cd bindings/python
    pip install pybind11
    python setup.py build_ext --inplace
    # or via CMake: cmake .. -DDEOBF_BUILD_PYTHON=ON

Then run:
    python examples/devirt_example.py
"""

import sys

try:
    import deobf
except ImportError:
    print("Error: deobf module not built. See build instructions above.")
    sys.exit(1)


def demo_ir_passes():
    """Demonstrate IR creation and optimization passes."""
    print("=== IR + Passes Demo ===\n")

    f = deobf.Function("demo")

    # Build: 3 + 4 = ?
    # In a real scenario, these come from lifting x86 instructions
    a = f.emit(deobf.Op.CONST, [deobf.Const(3)])
    b = f.emit(deobf.Op.CONST, [deobf.Const(4)])
    f.emit(deobf.Op.ADD, [a.ref(), b.ref()])

    print("Before optimization:")
    print(f.dump())
    print()

    report = deobf.run_to_convergence(f)
    print(f"After optimization ({report.iterations} iterations):")
    print(f.dump())
    print()


def demo_evaluator():
    """Demonstrate guided symbolic evaluation on a raw memory dump."""
    print("=== Guided Evaluator Demo ===\n")

    # In real use, load a PE or memory dump:
    #   with open("dump.bin", "rb") as f:
    #       image = f.read()
    #   ev = deobf.GuidedEvaluator(image, base=0x140000000, vpc_reg="r11")
    #   ev.add_safe_range(0x140B45000, 0x14196B000)  # bytecode section
    #   report = ev.run(0x14132C758, max_steps=50000, max_vm_insns=500)
    #   print(report.result)
    #   print(ev.dump_handler_catalog())
    #   print(ev.dump_vpc_trace())

    print("(Skipped: no dump file available. See code for usage pattern.)")
    print()


def demo_memory():
    """Demonstrate byte-level memory tracking."""
    print("=== ByteMemory Demo ===\n")

    mem = deobf.ByteMemory()
    mem.add_safe_range(0x1000, 0x2000)
    print(f"0x1500 is safe: {mem.is_safe(0x1500)}")
    print(f"0x3000 is safe: {mem.is_safe(0x3000)}")

    mem.store_u64(0x1000, 0xDEADBEEFCAFEBABE)
    val = mem.load_u64(0x1000)
    print(f"Stored 0xDEADBEEFCAFEBABE, loaded back: 0x{val:016X}")
    print()


if __name__ == "__main__":
    demo_ir_passes()
    demo_memory()
    demo_evaluator()
    print("All demos completed.")
