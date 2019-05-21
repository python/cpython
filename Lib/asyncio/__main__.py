import ast
import asyncio
import code
import inspect
import sys
import types


class AsyncIOInteractiveConsole(code.InteractiveConsole):

    def __init__(self, locals=None):
        super().__init__(locals)
        self.compile.compiler.flags |= ast.PyCF_ALLOW_TOP_LEVEL_AWAIT

        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)

    def runcode(self, code):
        try:
            func = types.FunctionType(code, self.locals)
            coro = func()
            if inspect.isawaitable(coro):
                return self.loop.run_until_complete(coro)
        except SystemExit:
            raise
        except BaseException:
            self.showtraceback()


if __name__ == '__main__':
    console = AsyncIOInteractiveConsole(locals())

    try:
        import readline  # NoQA
    except ImportError:
        pass

    banner = (
        f'asyncio REPL\n\n'
        f'Python {sys.version} on {sys.platform}\n'
        f'Type "help", "copyright", "credits" or "license" '
        f'for more information.\n'
    )

    console.interact(
        banner=banner,
        exitmsg='exiting asyncio REPL...')
