__all__ = ('Runner', 'run')

from . import coroutines
from . import events
from . import tasks


class Runner:
    def __init__(self, debug=None):
        self._debug = debug
        self._loop = None

    def __enter__(self):
        if events._get_running_loop() is not None:
            raise RuntimeError(
                "asyncio.Runner cannot be called from a running event loop")

        self._loop = events.new_event_loop()
        if self._debug is not None:
            self._loop.set_debug(self._debug)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        try:
            _cancel_all_tasks(self._loop)
            self._loop.run_until_complete(self._loop.shutdown_asyncgens())
            self._loop.run_until_complete(self._loop.shutdown_default_executor())
        finally:
            self._loop.close()
            self._loop = None

    def run(self, coro):
        if self._loop is None:
            raise RuntimeError(
                "Runner.run() cannot be called outside of "
                "'with Runner(): ...' context manager"
            )
        if not coroutines.iscoroutine(coro):
            raise ValueError("a coroutine was expected, got {!r}".format(coro))

        return self._loop.run_until_complete(coro)



def run(main, *, debug=None):
    """Execute the coroutine and return the result.

    This function runs the passed coroutine, taking care of
    managing the asyncio event loop and finalizing asynchronous
    generators.

    This function cannot be called when another asyncio event loop is
    running in the same thread.

    If debug is True, the event loop will be run in debug mode.

    This function always creates a new event loop and closes it at the end.
    It should be used as a main entry point for asyncio programs, and should
    ideally only be called once.

    Example:

        async def main():
            await asyncio.sleep(1)
            print('hello')

        asyncio.run(main())
    """
    with Runner(debug) as runner:
        return runner.run(main)


def _cancel_all_tasks(loop):
    to_cancel = tasks.all_tasks(loop)
    if not to_cancel:
        return

    for task in to_cancel:
        task.cancel()

    loop.run_until_complete(tasks.gather(*to_cancel, return_exceptions=True))

    for task in to_cancel:
        if task.cancelled():
            continue
        if task.exception() is not None:
            loop.call_exception_handler({
                'message': 'unhandled exception during asyncio.run() shutdown',
                'exception': task.exception(),
                'task': task,
            })
