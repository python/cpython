"""Support for running asyncio as a guest inside another event loop.

This module provides start_guest_run(), which allows asyncio to run
cooperatively inside a host event loop such as a GUI toolkit's main loop.
The host loop stays in control of the main thread while asyncio tasks
execute through a dual-thread architecture:

  Host thread:    process_events() + process_ready() -> sem.release()
  Backend thread: sem.acquire() -> poll_events() -> notify host

Inspired by Trio's guest mode (trio.lowlevel.start_guest_run).
"""

__all__ = ('start_guest_run',)

import threading
from functools import partial

from . import events


def start_guest_run(async_fn, *args,
                    run_sync_soon_threadsafe,
                    done_callback):
    """Run an async function as a guest inside another event loop.

    The host event loop (e.g. Tkinter mainloop) remains in control of the
    main thread.  asyncio I/O polling runs in a daemon background thread
    and dispatches work back to the host thread via *run_sync_soon_threadsafe*.

    Parameters
    ----------
    async_fn : coroutine function
        The async function to run.
    *args :
        Positional arguments passed to *async_fn*.
    run_sync_soon_threadsafe : callable
        A callback that schedules a zero-argument callable to run on the
        host thread.  Must be safe to call from any thread.
    done_callback : callable
        Called on the host thread when *async_fn* finishes.  Receives the
        completed ``asyncio.Task`` as its sole argument.  Callers can
        inspect the task with ``task.result()``, ``task.exception()``,
        or ``task.cancelled()``.

    Returns
    -------
    asyncio.Task
        The task wrapping *async_fn*.  To cancel from the host thread,
        use ``loop.call_soon_threadsafe(task.cancel)`` so that the I/O
        thread is woken from its selector wait.
    """
    loop = events.new_event_loop()
    events._set_running_loop(loop)

    _shutdown = threading.Event()
    _sem = threading.Semaphore(0)
    _done_called = False

    # -- helpers ------------------------------------------------------

    def _finish(task):
        """Clean up and forward completion to the host."""
        nonlocal _done_called
        if _done_called:
            return
        _done_called = True
        events._set_running_loop(None)
        try:
            done_callback(task)
        finally:
            if not loop.is_closed():
                loop.close()

    def _process_on_host(event_list):
        """Run on the host thread: process one batch of asyncio work."""
        if _shutdown.is_set():
            return
        loop.process_events(event_list)
        loop.process_ready()
        if not _shutdown.is_set():
            _sem.release()

    # -- threads -------------------------------------------------------

    def _backend():
        """Daemon thread: poll for I/O and wake the host."""
        try:
            while not _shutdown.is_set():
                _sem.acquire()
                if _shutdown.is_set():
                    break
                event_list = loop.poll_events()
                run_sync_soon_threadsafe(
                    partial(_process_on_host, event_list)
                )
        except Exception as exc:
            _shutdown.set()
            main_task.cancel(
                msg=f"asyncio guest I/O thread failed: {exc!r}"
            )
            run_sync_soon_threadsafe(lambda: _finish(main_task))

    # -- task setup ----------------------------------------------------

    main_task = loop.create_task(async_fn(*args))

    def _on_task_done(task):
        _shutdown.set()
        _sem.release()          # wake backend so it can exit
        run_sync_soon_threadsafe(lambda: _finish(task))

    main_task.add_done_callback(_on_task_done)

    # Kick off: process the initial callbacks enqueued by create_task.
    _process_on_host([])

    threading.Thread(
        target=_backend, daemon=True, name='asyncio-guest-io'
    ).start()

    return main_task
