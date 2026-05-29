# DeHendrix MCP server

Exposes the DeHendrix deobfuscation engine to MCP clients (AI agents, Cursor,
etc.). Each tool shells out to the `dehex_cli` executable; `optimize_llvm` /
`vm_devirt_optimized` additionally run `clang -O2` over the emitted LLVM IR.

## Tools

| Tool | Purpose |
|---|---|
| `tool_native_cfg` | Recover a native multi-block CFG (with phi) from a PE/dump. |
| `tool_vm_devirt` | Multi-path VM devirtualization (VMProtect-style); `safe_ranges` = `"0xSTART:0xEND"` VM bytecode regions. |
| `tool_vm_devirt_optimized` | `vm_devirt` + `clang -O2` over the emitted LLVM IR. |
| `tool_optimize_llvm` | Run clang's optimizer over arbitrary LLVM IR text. |

## Setup

```bash
pip install -r requirements.txt
# build dehex_cli first (see top-level README), then:
python dehendrix_mcp.py            # run as an MCP (stdio) server
```

## Configuration (environment)

- `DEHEX_CLI` — path to `dehex_cli(.exe)` (default: `<repo>/build2/Release/dehex_cli.exe`)
- `CLANG` — path to `clang(.exe)` (default: PATH, then `C:/Program Files/LLVM/bin/clang.exe`)

## Self-test (no MCP client needed)

```bash
# native CFG of a PE:
python dehendrix_mcp.py --selftest path/to/file.exe
# VM devirtualization:
python dehendrix_mcp.py --selftest dump.bin --entry 0x14132c758 --vpc-reg r11 \
    --safe 0x140b45000:0x14196b000
```
