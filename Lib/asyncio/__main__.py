import ast
import asyncio
import code
import concurrent.futures
import inspect
import sys
import threading
import types

from . import futures


class AsyncIOInteractiveConsole(code.InteractiveConsole):

    def __init__(self, locals, loop):
        super().__init__(locals)
        self.compile.compiler.flags |= ast.PyCF_ALLOW_TOP_LEVEL_AWAIT

        self.loop = loop

    def runcode(self, code):
        future = concurrent.futures.Future()

        def callback():
            global repl_future

            func = types.FunctionType(code, self.locals)
            try:
                coro = func()
            except BaseException as ex:
                future.set_exception(ex)
                return

            if not inspect.iscoroutine(coro):
                future.set_result(coro)
                return

            try:
                repl_future = self.loop.create_task(coro)
                futures._chain_future(repl_future, future)
            except BaseException as exc:
                future.set_exception(exc)

        loop.call_soon_threadsafe(callback)

        try:
            return future.result()
        except SystemExit:
            raise
        except BaseException:
            self.showtraceback()


class REPLThread(threading.Thread):

    def run(self):
        try:
            banner = (
                f'asyncio REPL {sys.version} on {sys.platform}\n'
                f'Type "help", "copyright", "credits" or "license" '
                f'for more information.\n\n'
                f'{getattr(sys, "ps1", ">>> ")}import asyncio\n'
            )

            console.interact(
                banner=banner,
                exitmsg='exiting asyncio REPL...')
        finally:
            loop.call_soon_threadsafe(loop.stop)


if __name__ == '__main__':
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)

    repl_locals = {'asyncio': asyncio}
    for key in {'__name__', '__package__',
                '__loader__', '__spec__',
                '__builtins__', '__file__'}:
        repl_locals[key] = locals()[key]

    console = AsyncIOInteractiveConsole(repl_locals, loop)
    repl_future = None

    try:
        import readline  # NoQA
    except ImportError:
        pass

    REPLThread().start()

    while True:
        try:
            loop.run_forever()
        except KeyboardInterrupt:
            if repl_future and not repl_future.done():
                repl_future.cancel()
            continue
        else:
            break
