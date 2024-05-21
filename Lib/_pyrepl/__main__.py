import os
import sys


def interactive_console(mainmodule=None, quiet=False, pythonstartup=False):
    from _pyrepl import env
    if not env.IS_PYREPL_SUPPORTED_PLATFORM:
        return sys._baserepl()

    startup_path = os.getenv("PYTHONSTARTUP")
    if pythonstartup and startup_path:
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

    try:
        import errno
        if not os.isatty(sys.stdin.fileno()):
            raise OSError(errno.ENOTTY, "tty required", "stdin")
        from .simple_interact import check
        if err := check():
            raise RuntimeError(err)
        from .simple_interact import run_multiline_interactive_console
        return run_multiline_interactive_console(mainmodule)
    except Exception as e:
        from .trace import trace
        msg = f"warning: can't use pyrepl: {e}"
        trace(msg)
        print(msg, file=sys.stderr)
        env.CAN_USE_PYREPL = False
        return sys._baserepl()


if __name__ == "__main__":
    interactive_console()
