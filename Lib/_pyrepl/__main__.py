import os
import sys

def interactive_console(mainmodule=None, quiet=False, pythonstartup=False):
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
    #
    run_interactive = None
    try:
        if not os.isatty(sys.stdin.fileno()):
            raise ImportError
        from .simple_interact import check
        if not check():
            raise ImportError
        from .simple_interact import run_multiline_interactive_console
        run_interactive = run_multiline_interactive_console
    except Exception as e:
        print(f"Warning: 'import _pyrepl' failed with '{e}'", sys.stderr)
    if run_interactive is None:
        return sys._baserepl()
    return run_interactive(mainmodule)

if __name__ == "__main__":
    interactive_console()
