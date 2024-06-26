import os
import sys

CAN_USE_PYREPL: bool
if sys.platform != "win32":
    CAN_USE_PYREPL = True
else:
    CAN_USE_PYREPL = sys.getwindowsversion().build >= 10586  # Windows 10 TH2


def interactive_console(mainmodule=None, quiet=False, pythonstartup=False):
    global CAN_USE_PYREPL
    if not CAN_USE_PYREPL:
        return sys._baserepl()

    if mainmodule:
        namespace = mainmodule.__dict__
    else:
        import __main__
        namespace = __main__.__dict__
        namespace.pop("__pyrepl_interactive_console", None)

    startup_path = os.getenv("PYTHONSTARTUP")
    if pythonstartup and startup_path:
        import tokenize
        with tokenize.open(startup_path) as f:
            startup_code = compile(f.read(), startup_path, "exec")
            exec(startup_code, namespace)

    # set sys.{ps1,ps2} just before invoking the interactive interpreter. This
    # mimics what CPython does in pythonrun.c
    if not hasattr(sys, "ps1"):
        sys.ps1 = ">>> "
    if not hasattr(sys, "ps2"):
        sys.ps2 = "... "

    run_interactive = None
    try:
        import errno
        if not os.isatty(sys.stdin.fileno()):
            raise OSError(errno.ENOTTY, "tty required", "stdin")
        from .simple_interact import check
        if err := check():
            raise RuntimeError(err)
        from .simple_interact import run_multiline_interactive_console
        run_interactive = run_multiline_interactive_console
    except Exception as e:
        from .trace import trace
        msg = f"warning: can't use pyrepl: {e}"
        trace(msg)
        print(msg, file=sys.stderr)
        CAN_USE_PYREPL = False
    if run_interactive is None:
        return sys._baserepl()
    run_interactive(namespace)
