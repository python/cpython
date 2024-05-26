import os
import sys

IS_SUPPORTED_PYREPL_PLATFORM = sys.platform != "win32"
IS_USING_PYREPL = False


def interactive_console():
    global IS_USING_PYREPL
    if not IS_SUPPORTED_PYREPL_PLATFORM:
        return sys._baserepl()

    startup_path = os.getenv("PYTHONSTARTUP")
    if startup_path:
        import tokenize
        with tokenize.open(startup_path) as f:
            startup_code = compile(f.read(), startup_path, "exec")
            exec(startup_code)

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
    if run_interactive is None:
        return sys._baserepl()
    IS_USING_PYREPL = True
    return run_interactive()
