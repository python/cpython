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
    run_interactive = run_simple_interactive_console
    try:
        if not os.isatty(sys.stdin.fileno()):
            # Bail out if stdin is not tty-like, as pyrepl wouldn't be happy
            # For example, with:
            # subprocess.Popen(['pypy', '-i'], stdin=subprocess.PIPE)
            raise ImportError
        from .simple_interact import check

        if not check():
            raise ImportError
        from .simple_interact import run_multiline_interactive_console

        run_interactive = run_multiline_interactive_console
    except SyntaxError:
        print("Warning: 'import pyrepl' failed with SyntaxError")
    run_interactive(mainmodule)


def run_simple_interactive_console(mainmodule):
    import code

    if mainmodule is None:
        import __main__ as mainmodule
    console = code.InteractiveConsole(mainmodule.__dict__, filename="<stdin>")
    # some parts of code.py are copied here because it was impossible
    # to start an interactive console without printing at least one line
    # of banner.  This was fixed in 3.4; but then from 3.6 it prints a
    # line when exiting.  This can be disabled too---by passing an argument
    # that doesn't exist in <= 3.5.  So, too much mess: just copy the code.
    more = 0
    while 1:
        try:
            if more:
                prompt = getattr(sys, "ps2", "... ")
            else:
                prompt = getattr(sys, "ps1", ">>> ")
            try:
                line = input(prompt)
            except EOFError:
                console.write("\n")
                break
            else:
                more = console.push(line)
        except KeyboardInterrupt:
            console.write("\nKeyboardInterrupt\n")
            console.resetbuffer()
            more = 0


if __name__ == "__main__":
    interactive_console()
