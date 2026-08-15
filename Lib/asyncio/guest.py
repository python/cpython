"""Support for running asyncio as a guest inside another event loop.

This module provides start_guest_run(), which allows asyncio to run
cooperatively inside a host event loop such as a GUI toolkit's main loop.
The host loop stays in control of its thread while asyncio tasks execute
through a dual-thread architecture:

  Host thread:  process_events() + process_ready() -> hand token to I/O thread
  I/O thread:   wait for token -> poll_events() -> hand events to host

A single "token" (semaphore permit) ping-pongs between the two threads,
so exactly one of them touches the event loop at any moment.

Inspired by Trio's guest mode (trio.lowlevel.start_guest_run).  The
asyncio-guest project, which this implementation grew out of, has
runnable examples for Tkinter, Qt, GTK, pygame, Win32 and Tornado
hosts:

    https://github.com/congzhangzh/asyncio-guest
"""

__all__ = ('start_guest_run',)

import signal
import threading
from functools import partial

from . import constants
from . import events
from . import runners
from . import tasks
from .log import logger

# Stop callbacks of unfinished guest runs, so interpreter shutdown can
# unblock and join their (non-daemon) I/O threads.
_lock = threading.Lock()
_stoppers = set()
_shutting_down = False
_atexit_registered = False


def _python_exit():
    """Wake and join the I/O threads of all unfinished guest runs.

    Registered via threading._register_atexit() so it runs before the
    interpreter joins non-daemon threads (the concurrent.futures
    pattern).  Each run is abandoned: pending tasks are not cancelled
    and done_callback is not called.
    """
    global _shutting_down
    with _lock:
        _shutting_down = True
        stoppers = list(_stoppers)
    for stop in stoppers:
        stop()


def _ensure_atexit():
    global _atexit_registered
    with _lock:
        if _shutting_down:
            raise RuntimeError(
                'cannot start an asyncio guest run at interpreter shutdown')
        if not _atexit_registered:
            threading._register_atexit(_python_exit)
            _atexit_registered = True


def _save_wakeup_fd():
    """Return the current signal wakeup fd, or None if it cannot be read.

    There is no getter for the wakeup fd, so briefly swapping it out is
    the only way to read it.  A signal arriving between the two calls
    loses its wakeup byte (its Python-level handler still runs); the
    window is a few instructions wide.
    """
    if (not hasattr(signal, 'set_wakeup_fd')
            or threading.current_thread() is not threading.main_thread()):
        return None
    fd = signal.set_wakeup_fd(-1)
    if fd != -1:
        signal.set_wakeup_fd(fd)
    return fd


def _restore_wakeup_fd(fd):
    if fd is not None:
        signal.set_wakeup_fd(fd)


def start_guest_run(async_fn, *args,
                    run_sync_soon_threadsafe,
                    done_callback):
    """Run async_fn(*args) as a guest inside another event loop.

    Must be called from the host event loop's thread.  The host loop
    (e.g. Tkinter's mainloop) remains in control of that thread; asyncio
    I/O polling runs in a background non-daemon thread that is joined
    when the run finishes.  Returns the Task wrapping *async_fn*; to
    cancel it from the host, use loop.call_soon_threadsafe(task.cancel)
    so that the I/O thread is woken from its selector wait.

    run_sync_soon_threadsafe is a callable that schedules a
    zero-argument callable to run on the host loop's thread.  It must be
    thread-safe, must not block, and must not raise; it need not
    preserve ordering.

    done_callback is called on the host thread after the run has fully
    finished: remaining tasks cancelled, asynchronous generators and the
    default executor shut down, and the loop closed (the same cleanup as
    asyncio.run()).  It receives the completed Task as its sole
    argument.

    For the whole run the guest loop is the thread's running loop:
    get_running_loop() works inside guest tasks, loop.is_running() is
    true, and starting another loop on the thread raises RuntimeError.
    Signal handling stays with the host: the guest loop never touches
    signal.set_wakeup_fd(), and loop.add_signal_handler() raises
    RuntimeError.  The host may forward signals into the loop with
    loop.call_soon_threadsafe().

    If the interpreter exits while the run is unfinished, the run is
    abandoned: the I/O thread is woken and joined, pending tasks are not
    cancelled, and done_callback is not called.
    """
    _ensure_atexit()

    # Create the loop without letting it capture the signal wakeup fd:
    # on Windows, BaseProactorEventLoop.__init__ installs its self-pipe
    # as the wakeup fd when on the main thread, which would silently
    # break the host's signal handling.
    host_wakeup_fd = _save_wakeup_fd()
    loop = events.new_event_loop()
    _restore_wakeup_fd(host_wakeup_fd)

    # The host owns signal handling (see the docstring).  Never reset:
    # 'finally' blocks of tasks cancelled during the final cleanup must
    # not be able to install signal handlers either.
    loop._guest_mode = True

    def _close_loop():
        # BaseProactorEventLoop.close() resets the wakeup fd to -1 on
        # the main thread; preserve the host's fd across it.
        fd = _save_wakeup_fd()
        try:
            loop.close()
        finally:
            _restore_wakeup_fd(fd)

    # Mark the loop as running for the whole guest run.  On Windows this
    # also starts the proactor's self-reading loop, so that
    # call_soon_threadsafe() can wake the I/O thread's poll.
    try:
        loop._run_forever_setup()
    except BaseException:
        _close_loop()
        raise

    shutdown = threading.Event()
    wakeup = threading.Semaphore(0)
    finished = False
    cleaned_up = False

    try:
        main_task = loop.create_task(async_fn(*args))
    except BaseException:
        loop._run_forever_cleanup()
        _close_loop()
        raise

    # -- helpers (host thread unless noted) ----------------------------

    def _cleanup_running_state():
        nonlocal cleaned_up
        if cleaned_up:
            return
        cleaned_up = True
        loop._run_forever_cleanup()

    def _join_backend():
        # By the token invariant the I/O thread is parked in
        # wakeup.acquire() here, never in the selector; the
        # _write_to_self() wake-up is defensive insurance.
        shutdown.set()
        wakeup.release()
        try:
            loop._write_to_self()
        except Exception:
            pass
        if backend_thread.ident is not None:
            backend_thread.join()

    def _deliver(task, *, graceful):
        with _lock:
            _stoppers.discard(_stop_at_exit)
        try:
            if graceful:
                # Same cleanup as asyncio.run().
                runners._cancel_all_tasks(loop)
                loop.run_until_complete(loop.shutdown_asyncgens())
                loop.run_until_complete(
                    loop.shutdown_default_executor(
                        constants.THREAD_JOIN_TIMEOUT))
        finally:
            _close_loop()
            done_callback(task)

    def _finish(task):
        # Always scheduled via run_sync_soon_threadsafe, never called
        # inline from a loop callback: the cleanup below drives the loop
        # with run_until_complete(), which would re-enter the
        # process_ready() iteration such a callback runs in.
        nonlocal finished
        if finished:
            return
        finished = True
        _join_backend()
        _cleanup_running_state()
        _deliver(task, graceful=True)

    def _abort(exc):
        # The I/O thread died; the loop can no longer be driven through
        # the guest handshake.  Unwind the main task directly.
        nonlocal finished
        if finished:
            return
        finished = True
        _join_backend()
        _cleanup_running_state()
        if not main_task.done():
            main_task.cancel(
                msg=f'asyncio guest I/O thread failed: {exc!r}')
            try:
                loop.run_until_complete(
                    tasks.gather(main_task, return_exceptions=True))
            except Exception:
                # The loop itself is broken (e.g. the selector raised);
                # abandon any pending work.
                pass
        loop.call_exception_handler({
            'message': 'asyncio guest I/O thread failed',
            'exception': exc,
            'task': main_task,
        })
        _deliver(main_task, graceful=False)

    def _process_on_host(event_list):
        """Process one batch of asyncio work on the host thread."""
        if shutdown.is_set() or loop.is_closed():
            return
        try:
            loop.process_events(event_list)
            loop.process_ready()
        except BaseException:
            # Internal loop failure (user callback exceptions are routed
            # to the exception handler by Handle._run).  Unblock the I/O
            # thread so it can exit and be joined.
            shutdown.set()
            wakeup.release()
            raise
        # Hand the polling token back to the I/O thread.  Exactly one
        # token circulates: while the host runs a batch, the I/O thread
        # is parked in wakeup.acquire(), so it can never poll (or touch
        # the loop at all) concurrently with this function.
        if not shutdown.is_set():
            wakeup.release()

    def _on_task_done(task):
        # Runs inside process_ready() while the host holds the token,
        # so the I/O thread is parked in wakeup.acquire(): the released
        # token wakes it, it observes 'shutdown', and exits without
        # re-entering the selector.
        shutdown.set()
        wakeup.release()
        try:
            run_sync_soon_threadsafe(partial(_finish, task))
        except Exception as exc:
            loop.call_exception_handler({
                'message': ('asyncio guest run could not schedule its '
                            'completion callback on the host'),
                'exception': exc,
                'task': task,
            })

    def _backend():
        """I/O thread: wait for the token, poll, hand events to the host."""
        try:
            while True:
                wakeup.acquire()
                if shutdown.is_set():
                    return
                event_list = loop.poll_events()
                if shutdown.is_set():
                    # Interpreter exit woke the selector; the host may
                    # be gone, so do not call into it.
                    return
                run_sync_soon_threadsafe(
                    partial(_process_on_host, event_list))
        except Exception as exc:
            if shutdown.is_set():
                return
            shutdown.set()
            try:
                run_sync_soon_threadsafe(partial(_abort, exc))
            except Exception:
                logger.error(
                    'asyncio guest run abandoned: host is unreachable '
                    'after an I/O thread failure', exc_info=True)

    def _stop_at_exit():
        # Interpreter-exit hook: unblock and join the I/O thread so
        # shutdown does not hang on a non-daemon thread.  The run is
        # abandoned -- no cancellation, no callbacks, no loop.close().
        shutdown.set()
        wakeup.release()
        try:
            loop._write_to_self()
        except Exception:
            pass
        if backend_thread.ident is not None:
            backend_thread.join()

    # -- start ---------------------------------------------------------

    main_task.add_done_callback(_on_task_done)

    backend_thread = threading.Thread(
        target=_backend, name='asyncio-guest-io')

    with _lock:
        shutting_down = _shutting_down
        if not shutting_down:
            _stoppers.add(_stop_at_exit)
    if shutting_down:
        loop._run_forever_cleanup()
        _close_loop()
        raise RuntimeError(
            'cannot start an asyncio guest run at interpreter shutdown')

    try:
        # Process the callbacks enqueued by create_task(), then let the
        # I/O thread take over polling.  The thread is started even if
        # the task already finished: it consumes the shutdown token and
        # exits, keeping the join logic uniform.
        _process_on_host([])
        backend_thread.start()
    except BaseException:
        with _lock:
            _stoppers.discard(_stop_at_exit)
        shutdown.set()
        _cleanup_running_state()
        _close_loop()
        raise

    return main_task
