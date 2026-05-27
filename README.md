# DeHendrix — Automated Binary Deobfuscation Framework

> C++17 反混淆引擎 | x86-64 Guided Symbolic Evaluation | 方法论: back.engineering + SATURN

给一个 VMP/Themida/OLLVM/ACE 保护的函数地址，还你原始语义。

## 设计理念

**混淆是编译器变换 → 去混淆是编译器优化**

## 特性

- **SSA IR** — 28 种操作码，Const/SymReg/SymMem/InstrRef 四型值
- **CFG** — BasicBlock + PhiNode + Terminator，支持多块 IR
- **9 优化 Passes** — const_promote / const_fold / mem_forward / dead_store_elim / dead_dep_elim / insn_combine / branch_fold / dead_dep_analysis / stack_rewrite
- **x64 Lifter** — capstone x64 → IR，30+ 指令支持
- **Guided Symbolic Evaluator** — VPC-aware dispatcher 循环穿越 + VJCC 检测
- **Handler Catalog** — 自动记录每次 dispatch 的 handler + VPC trace
- **ByteMemory** — 字节级 VM-private 内存跟踪 + 常量提升
- **PE Loader** — section/import/export 解析 + 平坦化加载
- **Target Plugin** — VMP3 / Themida / ACE / Generic 可插拔
- **LLVM IR Export** — 导出 `.ll` 文件，`clang -O3` 一步编译
- **pybind11** — Python 绑定（可选）

## 构建

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

需要 Capstone 反汇编库。可选 pybind11 (`-DDEOBF_BUILD_PYTHON=ON`)。

## 使用

```bash
# 去虚拟化一个 VMP3 保护的函数
./dehex_cli devirt \
    --image dump.bin \
    --base 0x140000000 \
    --entry 0x14132C758 \
    --vpc-reg r11 \
    --max-vm-insns 500 \
    --verbose
```

## 方法论参考

- [back.engineering: Static Devirtualization of Themida](https://back.engineering/blog/09/05/2026/)
- [SATURN: LLVM-based deobfuscation (arXiv:1909.01752)](https://arxiv.org/pdf/1909.01752)
- [JonathanSalwan: VMProtect-devirtualization](https://github.com/JonathanSalwan/VMProtect-devirtualization)
- [eversinc33: Naive LLVM-based Devirtualizer](https://eversinc33.com/2026/05/07/llvm-devirtualizer)

## 许可

MIT License
