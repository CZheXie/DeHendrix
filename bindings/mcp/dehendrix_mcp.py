#!/usr/bin/env python3
"""DeHendrix MCP server.

Exposes the libdeobf / DeHendrix deobfuscation engine to MCP clients (AI agents,
Cursor, etc.) as structured tools. Each tool shells out to the `dehex_cli`
executable; `optimize_llvm` additionally runs `clang -O2` over emitted LLVM IR.

Run as an MCP (stdio) server:
    python dehendrix_mcp.py

Self-test the underlying tools without an MCP client:
    python dehendrix_mcp.py --selftest <image> [--entry 0x...] [--vpc-reg r11] \
        [--safe 0xSTART:0xEND ...]

Configuration (environment variables):
    DEHEX_CLI   path to dehex_cli(.exe)   (default: <repo>/build2/Release/dehex_cli.exe)
    CLANG       path to clang(.exe)       (default: PATH, then C:/Program Files/LLVM/bin/clang.exe)
"""
from __future__ import annotations

import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import List, Optional

_REPO = Path(__file__).resolve().parents[2]


def _default_cli() -> str:
    env = os.environ.get("DEHEX_CLI")
    if env:
        return env
    exe = "dehex_cli.exe" if os.name == "nt" else "dehex_cli"
    for sub in ("build2/Release", "build/Release", "build2", "build"):
        p = _REPO / sub / exe
        if p.exists():
            return str(p)
    found = shutil.which("dehex_cli")
    return found or str(_REPO / "build2" / "Release" / exe)


def _default_clang() -> Optional[str]:
    env = os.environ.get("CLANG")
    if env:
        return env
    found = shutil.which("clang")
    if found:
        return found
    win = r"C:\Program Files\LLVM\bin\clang.exe"
    return win if os.path.exists(win) else None


def _run(args: List[str], timeout: int = 300) -> str:
    """Run a subprocess and return combined stdout/stderr text."""
    try:
        proc = subprocess.run(
            args, capture_output=True, text=True, timeout=timeout,
            encoding="utf-8", errors="replace",
        )
    except FileNotFoundError as e:
        return f"ERROR: executable not found: {e}"
    except subprocess.TimeoutExpired:
        return f"ERROR: timed out after {timeout}s"
    out = proc.stdout or ""
    if proc.returncode != 0:
        out += f"\n[exit={proc.returncode}]\n" + (proc.stderr or "")
    return out


# --- Core operations (plain functions, importable + self-testable) ---

def native_cfg(image_path: str, entry: str = "", base: str = "0x140000000",
               emit_llvm: bool = False) -> str:
    """Recover a native multi-block CFG from a binary/dump at `entry`."""
    cli = _default_cli()
    args = [cli, "cfg", "--image", image_path, "--base", base]
    if entry:
        args += ["--entry", entry]
    if emit_llvm:
        args += ["--emit-llvm"]
    return _run(args)


def vm_devirt(image_path: str, entry: str, base: str = "0x140000000",
              vpc_reg: str = "r11", safe_ranges: Optional[List[str]] = None,
              emit_llvm: bool = False) -> str:
    """Multi-path VM devirtualization: recover a branching VM CFG at `entry`.

    `safe_ranges` are VM bytecode regions as "0xSTART:0xEND" strings (regions
    that may be constant-promoted).
    """
    cli = _default_cli()
    args = [cli, "vm-cfg", "--image", image_path, "--base", base,
            "--entry", entry, "--vpc-reg", vpc_reg]
    for r in (safe_ranges or []):
        args += ["--safe", r]
    if emit_llvm:
        args += ["--emit-llvm"]
    return _run(args)


def optimize_llvm(ll_text: str, opt_level: str = "O2") -> str:
    """Run clang's middle-end optimizer over LLVM IR text, returning the
    optimized .ll. This is the 'feed the recovered CFG to the optimizer' lever
    that collapses residual VM scaffolding."""
    clang = _default_clang()
    if not clang:
        return "ERROR: clang not found (set CLANG or install LLVM)"
    with tempfile.TemporaryDirectory() as td:
        inp = os.path.join(td, "in.ll")
        outp = os.path.join(td, "out.ll")
        with open(inp, "w", encoding="utf-8") as f:
            f.write(ll_text)
        res = _run([clang, f"-{opt_level}", "-emit-llvm", "-S",
                    "-Wno-override-module", inp, "-o", outp])
        if os.path.exists(outp):
            with open(outp, "r", encoding="utf-8") as f:
                return f.read()
        return "clang did not produce output:\n" + res


def _extract_llvm(cli_output: str) -> str:
    """Pull the LLVM IR section out of dehex_cli --emit-llvm output."""
    marker = "=== LLVM IR ==="
    i = cli_output.find(marker)
    return cli_output[i + len(marker):].strip() if i >= 0 else ""


def vm_devirt_optimized(image_path: str, entry: str, base: str = "0x140000000",
                        vpc_reg: str = "r11",
                        safe_ranges: Optional[List[str]] = None) -> str:
    """vm_devirt then clang -O2 over the emitted LLVM IR (end-to-end)."""
    raw = vm_devirt(image_path, entry, base, vpc_reg, safe_ranges, emit_llvm=True)
    ll = _extract_llvm(raw)
    if not ll:
        return "No LLVM IR emitted.\n" + raw
    return optimize_llvm(ll)


# --- MCP server wrapping ---

def _build_server():
    from mcp.server.fastmcp import FastMCP
    mcp = FastMCP("dehendrix")

    @mcp.tool()
    def tool_native_cfg(image_path: str, entry: str = "",
                        base: str = "0x140000000", emit_llvm: bool = False) -> str:
        """Recover a native multi-block CFG (with phi) from a PE/dump.
        entry defaults to the PE entry point when omitted."""
        return native_cfg(image_path, entry, base, emit_llvm)

    @mcp.tool()
    def tool_vm_devirt(image_path: str, entry: str, base: str = "0x140000000",
                       vpc_reg: str = "r11", safe_ranges: Optional[List[str]] = None,
                       emit_llvm: bool = False) -> str:
        """Multi-path VM devirtualization (VMProtect-style). safe_ranges are
        VM bytecode regions as '0xSTART:0xEND'."""
        return vm_devirt(image_path, entry, base, vpc_reg, safe_ranges, emit_llvm)

    @mcp.tool()
    def tool_vm_devirt_optimized(image_path: str, entry: str,
                                 base: str = "0x140000000", vpc_reg: str = "r11",
                                 safe_ranges: Optional[List[str]] = None) -> str:
        """vm_devirt followed by clang -O2 over the emitted LLVM IR."""
        return vm_devirt_optimized(image_path, entry, base, vpc_reg, safe_ranges)

    @mcp.tool()
    def tool_optimize_llvm(ll_text: str, opt_level: str = "O2") -> str:
        """Run clang's optimizer over LLVM IR text and return the optimized IR."""
        return optimize_llvm(ll_text, opt_level)

    return mcp


def _selftest(argv: List[str]) -> int:
    image = argv[0]
    entry = ""
    vpc = "r11"
    safe: List[str] = []
    i = 1
    while i < len(argv):
        if argv[i] == "--entry" and i + 1 < len(argv):
            entry = argv[i + 1]; i += 2
        elif argv[i] == "--vpc-reg" and i + 1 < len(argv):
            vpc = argv[i + 1]; i += 2
        elif argv[i] == "--safe" and i + 1 < len(argv):
            safe.append(argv[i + 1]); i += 2
        else:
            i += 1
    print(f"[selftest] dehex_cli = {_default_cli()}")
    print(f"[selftest] clang     = {_default_clang()}")
    if entry and safe:
        print("\n===== vm_devirt =====")
        print(vm_devirt(image, entry, vpc_reg=vpc, safe_ranges=safe))
        print("\n===== vm_devirt_optimized (clang -O2) =====")
        print(vm_devirt_optimized(image, entry, vpc_reg=vpc, safe_ranges=safe))
    else:
        print("\n===== native_cfg =====")
        print(native_cfg(image, entry, emit_llvm=True))
    return 0


if __name__ == "__main__":
    try:
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    except Exception:
        pass
    if len(sys.argv) > 1 and sys.argv[1] == "--selftest":
        sys.exit(_selftest(sys.argv[2:]))
    _build_server().run()
