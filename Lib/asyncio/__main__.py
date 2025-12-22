import argparse
import ast
import asyncio
import asyncio.tools
import contextvars
import inspect
import os
import site
import sys
import types
import warnings

from _colorize import get_theme
from _pyrepl.console import InteractiveColoredConsole


class AsyncIOInteractiveConsole(InteractiveColoredConsole):

    def __init__(self, locals, loop):
        super().__init__(locals, filename="<stdin>")
        self.compile.compiler.flags |= ast.PyCF_ALLOW_TOP_LEVEL_AWAIT

        self.loop = loop
        self.context = contextvars.copy_context()

    def runcode(self, code):
        global return_code

        async def callback():
            func = types.FunctionType(code, self.locals)
            coro = func()

            if not inspect.iscoroutine(coro):
                return coro
            return await coro


        task = self.loop.create_task(callback(), context=self.context)
        try:
            return self.loop.run_until_complete(task)
        except SystemExit:
            raise
        except BaseException:
            self.showtraceback()
            return self.STATEMENT_FAILED


def interact():
    global return_code

    try:
        banner = (
            f'asyncio REPL {sys.version} on {sys.platform}\n'
            f'Use "await" directly instead of "asyncio.run()".\n'
            f'Type "help", "copyright", "credits" or "license" '
            f'for more information.\n'
        )

        console.write(banner)

        if startup_path := os.getenv("PYTHONSTARTUP"):
            sys.audit("cpython.run_startup", startup_path)

            import tokenize
            with tokenize.open(startup_path) as f:
                startup_code = compile(f.read(), startup_path, "exec")
                exec(startup_code, console.locals)

        ps1 = getattr(sys, "ps1", ">>> ")
        if CAN_USE_PYREPL:
            theme = get_theme().syntax
            ps1 = f"{theme.prompt}{ps1}{theme.reset}"
            import_line = f'{theme.keyword}import{theme.reset} asyncio'
        else:
            import_line = "import asyncio"
        console.write(f"{ps1}{import_line}\n")

        if CAN_USE_PYREPL:
            from _pyrepl.simple_interact import (
                run_multiline_interactive_console,
            )
            try:
                sys.ps1 = ps1
                run_multiline_interactive_console(console)
            except SystemExit as se:
                # expected via the `exit` and `quit` commands
                return_code = se.code
            except BaseException:
                # unexpected issue
                console.showtraceback()
                console.write("Internal error, ")
                return_code = 1
        else:
            try:
                console.interact(banner="", exitmsg="")
            except SystemExit as se:
                return_code = se.code
    finally:
        warnings.filterwarnings(
            'ignore',
            message=r'^coroutine .* was never awaited$',
            category=RuntimeWarning)

        loop.call_soon_threadsafe(loop.stop)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        prog="python3 -m asyncio",
        description="Interactive asyncio shell and CLI tools",
        color=True,
    )
    subparsers = parser.add_subparsers(help="sub-commands", dest="command")
    ps = subparsers.add_parser(
        "ps", help="Display a table of all pending tasks in a process"
    )
    ps.add_argument("pid", type=int, help="Process ID to inspect")
    pstree = subparsers.add_parser(
        "pstree", help="Display a tree of all pending tasks in a process"
    )
    pstree.add_argument("pid", type=int, help="Process ID to inspect")
    args = parser.parse_args()
    match args.command:
        case "ps":
            asyncio.tools.display_awaited_by_tasks_table(args.pid)
            sys.exit(0)
        case "pstree":
            asyncio.tools.display_awaited_by_tasks_tree(args.pid)
            sys.exit(0)
        case None:
            pass  # continue to the interactive shell
        case _:
            # shouldn't happen as an invalid command-line wouldn't parse
            # but let's keep it for the next person adding a command
            print(f"error: unhandled command {args.command}", file=sys.stderr)
            parser.print_usage(file=sys.stderr)
            sys.exit(1)

    sys.audit("cpython.run_stdin")

    if os.getenv('PYTHON_BASIC_REPL'):
        CAN_USE_PYREPL = False
    else:
        from _pyrepl.main import CAN_USE_PYREPL

    return_code = 0
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)

    repl_locals = {'asyncio': asyncio}
    for key in {'__name__', '__package__',
                '__loader__', '__spec__',
                '__builtins__', '__file__'}:
        repl_locals[key] = locals()[key]

    console = AsyncIOInteractiveConsole(repl_locals, loop)

    try:
        import readline  # NoQA
    except ImportError:
        readline = None

    interactive_hook = getattr(sys, "__interactivehook__", None)

    if interactive_hook is not None:
        sys.audit("cpython.run_interactivehook", interactive_hook)
        interactive_hook()

    if interactive_hook is site.register_readline:
        # Fix the completer function to use the interactive console locals
        try:
            import rlcompleter
        except:
            pass
        else:
            if readline is not None:
                completer = rlcompleter.Completer(console.locals)
                readline.set_completer(completer.complete)

    interact()
    console.write('exiting asyncio REPL...\n')
    loop.close()
    sys.exit(return_code)
