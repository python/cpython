# Contains code from https://github.com/tornadoweb/tornado/tree/v6.5.2
# SPDX-License-Identifier: PSF-2.0 AND Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2025 The Tornado Authors

"""
Compatibility for [add|remove]_[reader|writer] where unavailable (Proactor).

Runs select in a background thread.
_Only_ `select.select` is called in the background thread.

Callbacks are all handled back in the event loop's thread,
as scheduled by `loop.call_soon_threadsafe`.

Adapted from Tornado 6.5.2
"""

import asyncio
import atexit
import contextvars
import errno
import functools
import select
import socket
import threading
import typing

from typing import (
    Any,
    Callable,
    Protocol,
)

from . import events


class _HasFileno(Protocol):
    def fileno(self) -> int:
        pass


_FileDescriptorLike = int | _HasFileno

# Collection of selector thread event loops to shut down on exit.
_selector_loops: set["SelectorThread"] = set()


def _atexit_callback() -> None:
    for loop in _selector_loops:
        with loop._select_cond:
            loop._closing_selector = True
            loop._select_cond.notify()
        try:
            loop._waker_w.send(b"a")
        except BlockingIOError:
            pass
    _selector_loops.clear()


# use internal _register_atexit to avoid need for daemon threads
# I can't find a public API for equivalent functionality
# to run something prior to thread join during process teardown
threading._register_atexit(_atexit_callback)


class SelectorThread:
    """Define ``add_reader`` methods to be called in a background select thread.

    Instances of this class start a second thread to run a selector.
    This thread is completely hidden from the user;
    all callbacks are run on the wrapped event loop's thread
    via :meth:`loop.call_soon_threadsafe`.
    """

    _closed = False

    def __init__(self, real_loop: asyncio.AbstractEventLoop) -> None:
        self._main_thread_ctx = contextvars.copy_context()

        self._real_loop = real_loop

        self._select_cond = threading.Condition()
        self._select_args: tuple[list[_FileDescriptorLike], list[_FileDescriptorLike]] | None = None
        self._closing_selector = False
        self._thread: threading.Thread | None = None
        self._thread_manager_handle = self._thread_manager()

        # When the loop starts, start the thread. Not too soon because we can't
        # clean up if we get to this point but the event loop is closed without
        # starting.
        self._real_loop.call_soon(
            lambda: self._real_loop.create_task(self._thread_manager_handle.__anext__()),
            context=self._main_thread_ctx,
        )

        self._readers: dict[int, tuple[_FileDescriptorLike, Callable]] = {}
        self._writers: dict[int, tuple[_FileDescriptorLike, Callable]] = {}

        # Writing to _waker_w will wake up the selector thread, which
        # watches for _waker_r to be readable.
        self._waker_r, self._waker_w = socket.socketpair()
        self._waker_r.setblocking(False)
        self._waker_w.setblocking(False)
        _selector_loops.add(self)
        self.add_reader(self._waker_r, self._consume_waker)

    def close(self) -> None:
        if self._closed:
            return
        with self._select_cond:
            self._closing_selector = True
            self._select_cond.notify()
        self._wake_selector()
        if self._thread is not None:
            self._thread.join()
        _selector_loops.discard(self)
        self.remove_reader(self._waker_r)
        self._waker_r.close()
        self._waker_w.close()
        self._closed = True

    async def _thread_manager(self) -> typing.AsyncGenerator[None, None]:
        # Create a thread to run the select system call. We manage this thread
        # manually so we can trigger a clean shutdown from an atexit hook. Note
        # that due to the order of operations at shutdown,
        # we rely on private `threading._register_atexit`
        # to wake the thread before joining to avoid hangs.
        # See https://github.com/python/cpython/issues/86128 for more info
        self._thread = threading.Thread(
            name="asyncio selector",
            target=self._run_select,
        )
        self._thread.start()
        self._start_select()
        try:
            # The presence of this yield statement means that this coroutine
            # is actually an asynchronous generator, which has a special
            # shutdown protocol. We wait at this yield point until the
            # event loop's shutdown_asyncgens method is called, at which point
            # we will get a GeneratorExit exception and can shut down the
            # selector thread.
            yield
        except GeneratorExit:
            self.close()
            raise

    def _wake_selector(self) -> None:
        """Wake the selector thread from another thread."""
        if self._closed:
            return
        try:
            self._waker_w.send(b"a")
        except BlockingIOError:
            pass

    def _consume_waker(self) -> None:
        """Consume messages sent via _wake_selector."""
        try:
            self._waker_r.recv(1024)
        except BlockingIOError:
            pass

    def _start_select(self) -> None:
        """Start select waiting for events.

        Called from the event loop thread,
        schedules select to be called in the background thread.
        """
        # Capture reader and writer sets here in the event loop
        # thread to avoid any problems with concurrent
        # modification while the select loop uses them.
        with self._select_cond:
            assert self._select_args is None
            self._select_args = (list(self._readers.keys()), list(self._writers.keys()))
            self._select_cond.notify()

    def _run_select(self) -> None:
        """The main function of the select thread.

        Runs `select.select()` until `_closing_selector` attribute is set (typically by `close()`).
        Schedules handling of `select.select` output on the main thread
        via `loop.call_soon_threadsafe()`.
        """
        while not self._closing_selector:
            with self._select_cond:
                while self._select_args is None and not self._closing_selector:
                    self._select_cond.wait()
                if self._closing_selector:
                    return
                assert self._select_args is not None
                to_read, to_write = self._select_args
                self._select_args = None

            # We use the simpler interface of the select module instead of
            # the more stateful interface in the selectors module because
            # this class is only intended for use on windows, where
            # select.select is the only option. The selector interface
            # does not have well-documented thread-safety semantics that
            # we can rely on so ensuring proper synchronization would be
            # tricky.
            try:
                # On windows, selecting on a socket for write will not
                # return the socket when there is an error (but selecting
                # for reads works). Also select for errors when selecting
                # for writes, and merge the results.
                #
                # This pattern is also used in
                # https://github.com/python/cpython/blob/v3.8.0/Lib/selectors.py#L312-L317
                rs, ws, xs = select.select(to_read, to_write, to_write)
                ws = ws + xs
            except OSError as e:
                # After remove_reader or remove_writer is called, the file
                # descriptor may subsequently be closed on the event loop
                # thread. It's possible that this select thread hasn't
                # gotten into the select system call by the time that
                # happens in which case (at least on macOS), select may
                # raise a "bad file descriptor" error. If we get that
                # error, check and see if we're also being woken up by
                # polling the waker alone. If we are, just return to the
                # event loop and we'll get the updated set of file
                # descriptors on the next iteration. Otherwise, raise the
                # original error.
                if e.errno == getattr(errno, "WSAENOTSOCK", errno.EBADF):
                    rs, _, _ = select.select([self._waker_r.fileno()], [], [], 0)
                    if rs:
                        ws = []
                    else:
                        raise
                else:
                    raise

            # if close has already started, don't schedule callbacks,
            # which could cause a race
            with self._select_cond:
                if self._closing_selector:
                    return
            self._real_loop.call_soon_threadsafe(
                    self._handle_select, rs, ws, context=self._main_thread_ctx
                )

    def _handle_select(
        self, rs: list[_FileDescriptorLike], ws: list[_FileDescriptorLike]
    ) -> None:
        """Handle the result of select.select.

        This method is called on the event loop thread via `call_soon_threadsafe`.
        """
        for r in rs:
            self._handle_event(r, self._readers)
        for w in ws:
            self._handle_event(w, self._writers)
        self._start_select()

    def _handle_event(
        self,
        fd: _FileDescriptorLike,
        cb_map: dict[int, tuple[_FileDescriptorLike, Callable]],
    ) -> None:
        """Handle one callback event.

        This method is called on the event loop thread via `call_soon_threadsafe` (from `_handle_select`),
        so exception handler wrappers, etc. are applied.
        """
        try:
            fileobj, handle = cb_map[fd]
        except KeyError:
            return
        if not handle.cancelled():
            handle._run()

    def _split_fd(self, fd: _FileDescriptorLike) -> tuple[int, _FileDescriptorLike]:
        """Return fd, file object

        Keeps a handle on the fileobject given,
        but always registers integer FD.
        """
        fileno = fd
        if not isinstance(fileno, int):
            try:
                fileno = int(fileno.fileno())
            except (AttributeError, TypeError, ValueError):
                # This code matches selectors._fileobj_to_fd function.
                raise ValueError(f"Invalid file object: {fd!r}") from None
        return fileno, fd

    def add_reader(
        self, fd: _FileDescriptorLike, callback: Callable[..., None], *args: Any
    ) -> None:
        fd, fileobj = self._split_fd(fd)
        if fd in self._readers:
            _, handle = self._readers[fd]
            handle.cancel()
        self._readers[fd] = (fileobj, events.Handle(callback, args, self._real_loop))
        self._wake_selector()

    def add_writer(
        self, fd: _FileDescriptorLike, callback: Callable[..., None], *args: Any
    ) -> None:
        fd, fileobj = self._split_fd(fd)
        if fd in self._writers:
            _, handle = self._writers[fd]
            handle.cancel()
        self._writers[fd] = (fileobj, events.Handle(callback, args, self._real_loop))
        self._wake_selector()

    def remove_reader(self, fd: _FileDescriptorLike) -> bool:
        fd, _ = self._split_fd(fd)
        try:
            _, handle = self._readers.pop(fd)
        except KeyError:
            return False
        handle.cancel()
        self._wake_selector()
        return True

    def remove_writer(self, fd: _FileDescriptorLike) -> bool:
        fd, _ = self._split_fd(fd)
        try:
            _, handle = self._writers.pop(fd)
        except KeyError:
            return False
        handle.cancel()
        self._wake_selector()
        return True
