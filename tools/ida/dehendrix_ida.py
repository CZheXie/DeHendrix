"""DeHendrix IDA integration (IDAPython).

Right-click / hotkey on the function under the cursor to run the DeHendrix
deobfuscation engine on it and view the recovered CFG. The result is shown in a
viewer, echoed to the Output window, and attached as a repeatable function
comment.

Install: copy this file into IDA's `plugins/` directory, or run it via
File -> Script file. It registers the action + hotkey on load.

  Hotkey:  Ctrl-Shift-D   (Devirtualize function under cursor)

Configuration (environment):
  DEHEX_CLI   path to dehex_cli(.exe). If unset, common build paths are tried,
              otherwise you are prompted to locate it (the choice is remembered).
"""
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
    cli = _find_cli()
    if not cli:
        ida_kernwin.warning("dehex_cli not found (set DEHEX_CLI).")
        return

    mode = ida_kernwin.ask_buttons(
        "Native CFG", "VM devirt", "Cancel", idaapi.ASKBTN_YES,
        "DeHendrix: recover function at 0x%X as?" % start)
    if mode == idaapi.ASKBTN_CANCEL:
        return

    if mode == idaapi.ASKBTN_YES:
        args = [cli, "cfg", "--image", path, "--base", hex(base),
                "--entry", hex(start), "--emit-llvm"]
    else:
        vpc = ida_kernwin.ask_str("r11", 0, "VPC register") or "r11"
        safe = ida_kernwin.ask_str("", 0,
                "Safe ranges 0xSTART:0xEND (space-separated, optional)") or ""
        args = [cli, "vm-cfg", "--image", path, "--base", hex(base),
                "--entry", hex(start), "--vpc-reg", vpc]
        for r in safe.split():
            args += ["--safe", r]

    ida_kernwin.msg("[DeHendrix] %s\n" % " ".join(args))
    out = _run_cli(args)
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
