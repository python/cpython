import os
import sys

CAN_USE_PYREPL: bool
if sys.platform != "win32":
    CAN_USE_PYREPL = True
else:
    CAN_USE_PYREPL = sys.getwindowsversion().build >= 10586  # Windows 10 TH2

FAIL_MSG = ""
try:
    import errno
    if not os.isatty(sys.stdin.fileno()):
        raise OSError(errno.ENOTTY, "tty required", "stdin")
    from .simple_interact import check
    if err := check():
        raise RuntimeError(err)
except Exception as e:
    FAIL_MSG = f"warning: can't use pyrepl: {e}"
    CAN_USE_PYREPL = False


def interactive_console(mainmodule=None, quiet=False, pythonstartup=False):
    if not CAN_USE_PYREPL:
        if not os.environ.get('PYTHON_BASIC_REPL', None):
            from .trace import trace
            trace(FAIL_MSG)
            print(FAIL_MSG, file=sys.stderr)
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

    from .simple_interact import run_multiline_interactive_console
    run_multiline_interactive_console(namespace)
