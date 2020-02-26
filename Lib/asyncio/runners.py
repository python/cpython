__all__ = 'run',

import signal
import contextlib
import threading

from . import coroutines
from . import events
from . import tasks


def _custom_sigint_handler(signum, frame):
    """Interrupt the running loop when SIGINT is received"""
    events.get_running_loop().interrupt()


@contextlib.contextmanager
def _use_custom_signal_handler_if_needed():
    """If needed, register a custom SIGINT handler when an event loop is run.

    When running an event loop in the main thread, the default SIGINT
    handler can cause the loop to lose callbacks and make it get stuck
    if it is ever rerun (bpo-39622).
    """
    is_main_thread = threading.current_thread() is threading.main_thread()
    sigint_handler_is_default = signal.getsignal(signal.SIGINT) is \
                                signal.default_int_handler
    if is_main_thread and sigint_handler_is_default:
        previous_handler = signal.signal(signal.SIGINT, _custom_sigint_handler)
        try:
            yield
        finally:
            signal.signal(signal.SIGINT, previous_handler)
    else:
        yield


def run(main, *, debug=False):
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
        raise RuntimeError(
            "asyncio.run() cannot be called from a running event loop")

    if not coroutines.iscoroutine(main):
        raise ValueError("a coroutine was expected, got {!r}".format(main))

    loop = events.new_event_loop()
    # bpo-39622: allow event loop to stop gracefully when sigint is received
    with _use_custom_signal_handler_if_needed():
        try:
            events.set_event_loop(loop)
            loop.set_debug(debug)
            return loop.run_until_complete(main)
        finally:
            try:
                _cancel_all_tasks(loop)
                loop.run_until_complete(loop.shutdown_asyncgens())
                loop.run_until_complete(loop.shutdown_default_executor())
            finally:
                events.set_event_loop(None)
                loop.close()


def _cancel_all_tasks(loop):
    to_cancel = tasks.all_tasks(loop)
    if not to_cancel:
        return

    for task in to_cancel:
        task.cancel()

    loop.run_until_complete(
        tasks.gather(*to_cancel, loop=loop, return_exceptions=True))

    for task in to_cancel:
        if task.cancelled():
            continue
        if task.exception() is not None:
            loop.call_exception_handler({
                'message': 'unhandled exception during asyncio.run() shutdown',
                'exception': task.exception(),
                'task': task,
            })
