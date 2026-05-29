"""DeHendrix IDA integration (IDAPython).

Right-click / hotkey on the function under the cursor to run the DeHendrix
deobfuscation engine on it and view the recovered CFG. The result is shown in a
viewer, echoed to the Output window, and attached as a repeatable function
comment.

Install: copy this file into IDA's `plugins/` directory, or run it via
File -> Script file. It registers the action + hotkey on load.

  Hotkey:  Ctrl-Shift-D   (Devirtualize function under cursor)

AI-callable interface (non-interactive, for agents / IDALib / IDA-MCP bridges):

  import dehendrix_ida as d
  d.devirt(ea=None, mode="auto"|"vm", vpc_reg="r11", safe_ranges=["0xA:0xB"])
  d.devirt_json(...)   # same, returns a JSON string

Both return {ok, entry, mode, cli, command, output[, error]} with no GUI; an
agent can drive them headlessly and parse the structured result.

Configuration (environment):
  DEHEX_CLI   path to dehex_cli(.exe). If unset, common build paths are tried,
              otherwise you are prompted to locate it (the choice is remembered).
"""
import json
import os
import subprocess

import idaapi
import idc
import ida_funcs
import ida_kernwin
import ida_nalt

ACTION_ID = "dehendrix:devirt_func"
_CLI_CACHE = {"path": None}


def _find_cli():
    if _CLI_CACHE["path"] and os.path.exists(_CLI_CACHE["path"]):
        return _CLI_CACHE["path"]
    env = os.environ.get("DEHEX_CLI")
    cands = []
    if env:
        cands.append(env)
    exe = "dehex_cli.exe" if os.name == "nt" else "dehex_cli"
    here = os.path.dirname(os.path.abspath(__file__))
    repo = os.path.abspath(os.path.join(here, "..", ".."))
    for sub in ("build2/Release", "build/Release", "build2", "build"):
        cands.append(os.path.join(repo, sub, exe))
    for c in cands:
        if c and os.path.exists(c):
            _CLI_CACHE["path"] = c
            return c
    picked = ida_kernwin.ask_file(False, exe, "Locate dehex_cli")
    if picked and os.path.exists(picked):
        _CLI_CACHE["path"] = picked
        return picked
    return None


def _run_cli(args):
    try:
        proc = subprocess.run(args, capture_output=True, text=True,
                              encoding="utf-8", errors="replace", timeout=300)
    except Exception as e:  # noqa: BLE001 - surface any launch failure to the analyst
        return "ERROR launching dehex_cli: %s" % e
    out = proc.stdout or ""
    if proc.returncode != 0:
        out += "\n[exit=%d]\n%s" % (proc.returncode, proc.stderr or "")
    return out


def _show_viewer(title, text):
    v = ida_kernwin.simplecustviewer_t()
    if not v.Create(title):
        ida_kernwin.msg(text + "\n")
        return
    for line in text.splitlines():
        v.AddLine(line)
    v.Show()


def _build_args(cli, path, base, start, mode, vpc_reg, safe_ranges, emit_llvm):
    if mode == "vm":
        args = [cli, "vm-cfg", "--image", path, "--base", hex(base),
                "--entry", hex(start), "--vpc-reg", vpc_reg or "r11"]
        for r in (safe_ranges or []):
            args += ["--safe", r]
    else:  # native
        args = [cli, "cfg", "--image", path, "--base", hex(base),
                "--entry", hex(start)]
    if emit_llvm:
        args += ["--emit-llvm"]
    return args


def _run_devirt(path, base, start, mode, vpc_reg, safe_ranges, emit_llvm):
    cli = _find_cli()
    if not cli:
        return {"ok": False, "error": "dehex_cli not found (set DEHEX_CLI)"}
    args = _build_args(cli, path, base, start, mode, vpc_reg, safe_ranges, emit_llvm)
    out = _run_cli(args)
    return {
        "ok": not out.startswith("ERROR"),
        "entry": hex(start),
        "mode": mode,
        "cli": cli,
        "command": " ".join(args),
        "output": out,
    }


# --- AI-callable API (non-interactive, structured result) ---

def devirt(ea=None, mode="auto", vpc_reg="r11", safe_ranges=None, emit_llvm=True):
    """Devirtualize the function containing `ea` (default: cursor) and return a
    JSON-serializable dict. Intended for AI agents / IDALib / IDA-MCP bridges:
    no GUI prompts.

    mode: "auto"/"native" -> native CFG recovery; "vm" -> VM devirtualization.
    safe_ranges: list of "0xSTART:0xEND" strings (VM bytecode regions).
    Returns: {ok, entry, mode, cli, command, output[, error]}.
    """
    if ea is None:
        ea = idc.here()
    func = ida_funcs.get_func(ea)
    if not func:
        return {"ok": False, "error": "no function at 0x%X" % ea}
    path = ida_nalt.get_input_file_path()
    if not path or not os.path.exists(path):
        return {"ok": False, "error": "input file not found: %r" % path}
    base = idaapi.get_imagebase()
    m = "vm" if mode == "vm" else "native"
    return _run_devirt(path, base, func.start_ea, m, vpc_reg, safe_ranges, emit_llvm)


def devirt_json(ea=None, mode="auto", vpc_reg="r11", safe_ranges=None, emit_llvm=True):
    """Same as devirt() but returns a JSON string (handy for string-based
    AI / MCP bridges)."""
    return json.dumps(devirt(ea, mode, vpc_reg, safe_ranges, emit_llvm), indent=2)


# --- Interactive entry (human; GUI prompts) ---

def devirt_current():
    ea = idc.here()
    func = ida_funcs.get_func(ea)
    if not func:
        ida_kernwin.warning("No function at 0x%X" % ea)
        return
    start = func.start_ea
    path = ida_nalt.get_input_file_path()
    if not path or not os.path.exists(path):
        picked = ida_kernwin.ask_file(False, "*.*", "Locate the input binary")
        if not picked:
            return
        path = picked
    base = idaapi.get_imagebase()
    if not _find_cli():
        ida_kernwin.warning("dehex_cli not found (set DEHEX_CLI).")
        return

    btn = ida_kernwin.ask_buttons(
        "Native CFG", "VM devirt", "Cancel", idaapi.ASKBTN_YES,
        "DeHendrix: recover function at 0x%X as?" % start)
    if btn == idaapi.ASKBTN_CANCEL:
        return

    if btn == idaapi.ASKBTN_YES:
        res = _run_devirt(path, base, start, "native", "r11", None, True)
    else:
        vpc = ida_kernwin.ask_str("r11", 0, "VPC register") or "r11"
        safe = ida_kernwin.ask_str("", 0,
                "Safe ranges 0xSTART:0xEND (space-separated, optional)") or ""
        res = _run_devirt(path, base, start, "vm", vpc, safe.split(), True)

    out = res.get("output", res.get("error", ""))
    ida_kernwin.msg("[DeHendrix] %s\n" % res.get("command", ""))
    _show_viewer("DeHendrix 0x%X" % start, out)
    ida_kernwin.msg(out + "\n")
    try:
        ida_funcs.set_func_cmt(func, "[DeHendrix devirt]\n" + out[:4000], True)
    except Exception:
        pass


class _Handler(idaapi.action_handler_t):
    def activate(self, ctx):
        devirt_current()
        return 1

    def update(self, ctx):
        return idaapi.AST_ENABLE_ALWAYS


def _register():
    idaapi.unregister_action(ACTION_ID)
    desc = idaapi.action_desc_t(
        ACTION_ID, "DeHendrix: devirtualize function", _Handler(),
        "Ctrl-Shift-D", "Run the DeHendrix deobfuscation engine on this function", -1)
    if idaapi.register_action(desc):
        idaapi.attach_action_to_menu("Edit/Plugins/", ACTION_ID, idaapi.SETMENU_APP)
        ida_kernwin.msg("[DeHendrix] action registered (Ctrl-Shift-D)\n")


# Minimal plugin wrapper so the file also works when dropped in plugins/.
class DeHendrixPlugin(idaapi.plugin_t):
    flags = idaapi.PLUGIN_KEEP
    comment = "DeHendrix deobfuscation"
    help = "Ctrl-Shift-D on a function"
    wanted_name = "DeHendrix"
    wanted_hotkey = ""

    def init(self):
        _register()
        return idaapi.PLUGIN_KEEP

    def run(self, arg):
        devirt_current()

    def term(self):
        idaapi.unregister_action(ACTION_ID)


def PLUGIN_ENTRY():
    return DeHendrixPlugin()


# When run as a script (File -> Script file), register the action immediately.
if __name__ == "__main__":
    _register()
