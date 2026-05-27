"""
DeHendrix Python bindings setup.

Build:
    pip install pybind11
    python setup.py build_ext --inplace

Or with CMake:
    cmake .. -DDEOBF_BUILD_PYTHON=ON
"""

import os
import sys
from pathlib import Path

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext

try:
    import pybind11
    pybind11_include = pybind11.get_include()
except ImportError:
    pybind11_include = ""
    print("WARNING: pybind11 not installed. Run: pip install pybind11")

ROOT = Path(__file__).resolve().parent.parent.parent
INCLUDE_DIR = str(ROOT / "include")
SRC_DIR = str(ROOT / "src")

source_files = [
    str(ROOT / "bindings" / "python" / "pydeobf.cpp"),
    str(ROOT / "src" / "ir" / "value.cpp"),
    str(ROOT / "src" / "ir" / "function.cpp"),
    str(ROOT / "src" / "ir" / "cfg.cpp"),
    str(ROOT / "src" / "ir" / "pe_loader.cpp"),
    str(ROOT / "src" / "passes" / "transforms.cpp"),
    str(ROOT / "src" / "passes" / "iat_cracker.cpp"),
    str(ROOT / "src" / "memory" / "byte_memory.cpp"),
    str(ROOT / "src" / "eval" / "target.cpp"),
    str(ROOT / "src" / "lower" / "llvm_emitter.cpp"),
]

ext_modules = [
    Extension(
        "deobf",
        sources=source_files,
        include_dirs=[INCLUDE_DIR, pybind11_include],
        language="c++",
        define_macros=[("DEOBF_HAS_PYBIND11", "1")],
        extra_compile_args=["/std:c++17", "/EHsc"] if sys.platform == "win32"
                           else ["-std=c++17", "-fPIC"],
    ),
]

setup(
    name="dehendrix",
    version="0.1.0",
    author="NoobHunter42",
    description="DeHendrix - C++17 Binary Deobfuscation Framework Python Bindings",
    long_description=open(str(ROOT / "README.md")).read(),
    long_description_content_type="text/markdown",
    url="https://github.com/NoobHunter42/DeHendrix",
    ext_modules=ext_modules,
    python_requires=">=3.8",
    install_requires=["pybind11>=2.10"],
    classifiers=[
        "Programming Language :: Python :: 3",
        "Programming Language :: C++",
        "License :: OSI Approved :: MIT License",
        "Topic :: Security",
        "Topic :: Software Development :: Disassemblers",
    ],
)
