import ast
import asyncio
import code
import concurrent.futures
import inspect
import sys
import threading
import types
import warnings

from . import futures


class AsyncIOInteractiveConsole(code.InteractiveConsole):

    def __init__(self, locals, loop=None):
        super().__init__(locals)
        self.compile.compiler.flags |= ast.PyCF_ALLOW_TOP_LEVEL_AWAIT
        if not loop:
            loop = asyncio.get_event_loop()
        self.loop = loop

    def runcode(self, code):
        future = concurrent.futures.Future()

        def callback():
            global repl_future
            global repl_future_interrupted

            repl_future = None
            repl_future_interrupted = False

            func = types.FunctionType(code, self.locals)
            try:
                coro = func()
            except SystemExit:
                raise
            except KeyboardInterrupt as ex:
                repl_future_interrupted = True
                future.set_exception(ex)
                return
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

        self.loop.call_soon_threadsafe(callback)

        try:
            return future.result()
        except SystemExit:
            raise
        except BaseException:
            if repl_future_interrupted:
                self.write("\nKeyboardInterrupt\n")
            else:
                self.showtraceback()


class REPLThread(threading.Thread):

    def __init__(self, console, preamble=(), loop=None):
        super().__init__()
        self.console = console
        self.preamble = list(preamble)
        if not loop:
            loop = asyncio.get_event_loop()
        self.loop = loop

    def run(self):
        try:
            prompt = getattr(sys, 'ps1', '>>> ')
            input_lines = ''.join([f'\n{prompt}{line}' for line in self.preamble])
            banner = (
                f'asyncio REPL {sys.version} on {sys.platform}\n'
                f'Use "await" directly instead of "asyncio.run()".\n'
                f'Type "help", "copyright", "credits" or "license" '
                f'for more information.\n'
                f'{prompt}import asyncio'
                f'{input_lines}')

            for line in self.preamble:
                self.console.push(line)  # NB: line will not appear in history
            self.console.interact(
                banner=banner,
                exitmsg='exiting asyncio REPL...')
        finally:
            warnings.filterwarnings(
                'ignore',
                message=r'^coroutine .* was never awaited$',
                category=RuntimeWarning)

            self.loop.call_soon_threadsafe(self.loop.stop)


def main(preamble=()):
    global repl_future_interrupted

    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)

    caller_locals = inspect.currentframe().f_back.f_locals
    repl_locals = {}
    for key in {'__name__', '__package__',
                '__loader__', '__spec__',
                '__builtins__', '__file__'}:
        if key in caller_locals:
            repl_locals[key] = caller_locals[key]

    repl_locals['asyncio'] = asyncio
    console = AsyncIOInteractiveConsole(repl_locals, loop)

    repl_future = None
    repl_future_interrupted = False

    try:
        import readline  # NoQA
    except ImportError:
        pass

    repl_thread = REPLThread(console, preamble, loop)
    repl_thread.daemon = True
    repl_thread.start()

    while True:
        try:
            loop.run_forever()
        except KeyboardInterrupt:
            if repl_future and not repl_future.done():
                repl_future.cancel()
                repl_future_interrupted = True
            continue
        else:
            break


if __name__ == '__main__':
    main()
