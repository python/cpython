__all__ = ('Runner', 'run')

import contextvars
import enum
from . import coroutines
from . import events
from . import tasks


class _State(enum.Enum):
    CREATED = "created"
    INITIALIZED = "initialized"
    CLOSED = "closed"


class Runner:
    """A context manager that controls event loop life cycle.

    The context manager always creates a new event loop,
    allows to run async functions inside it,
    and properly finalizes the loop at the context manager exit.

    If debug is True, the event loop will be run in debug mode.
    If loop_factory is passed, it is used for new event loop creation.

    asyncio.run(main(), debug=True)

    is a shortcut for

    with asyncio.Runner(debug=True) as runner:
        runner.run(main())

    The run() method can be called multiple times within the runner's context.

    This can be useful for interactive console (e.g. IPython),
    unittest runners, console tools, -- everywhere when async code
    is called from existing sync framework and where the preferred single
    asyncio.run() call doesn't work.

    """

    # Note: the class is final, it is not intended for inheritance.

    def __init__(self, *, debug=None, loop_factory=None):
        self._state = _State.CREATED
        self._debug = debug
        self._loop_factory = loop_factory
        self._loop = None
        self._context = None

    def __enter__(self):
        self._lazy_init()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def close(self):
        """Shutdown and close event loop."""
        if self._state is not _State.INITIALIZED:
            return
        try:
            loop = self._loop
            _cancel_all_tasks(loop)
            loop.run_until_complete(loop.shutdown_asyncgens())
            loop.run_until_complete(loop.shutdown_default_executor())
        finally:
            loop.close()
            self._loop = None
            self._state = _State.CLOSED

    def get_loop(self):
        """Return embedded event loop."""
        self._lazy_init()
        return self._loop

    def run(self, coro, *, context=None):
        """Run a coroutine inside the embedded event loop."""
        if not coroutines.iscoroutine(coro):
            raise ValueError("a coroutine was expected, got {!r}".format(coro))

        if events._get_running_loop() is not None:
            # fail fast with short traceback
            raise RuntimeError(
                "Runner.run() cannot be called from a running event loop")

        self._lazy_init()

        if context is None:
            context = self._context
        task = self._loop.create_task(coro, context=context)
        return self._loop.run_until_complete(task)

    def _lazy_init(self):
        if self._state is _State.CLOSED:
            raise RuntimeError("Runner is closed")
        if self._state is _State.INITIALIZED:
            return
        if self._loop_factory is None:
            self._loop = events.new_event_loop()
        else:
            self._loop = self._loop_factory()
        if self._debug is not None:
            self._loop.set_debug(self._debug)
        self._context = contextvars.copy_context()
        self._state = _State.INITIALIZED



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
    if events._get_running_loop() is not None:
        # fail fast with short traceback
        raise RuntimeError(
            "asyncio.run() cannot be called from a running event loop")

    with Runner(debug=debug) as runner:
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
