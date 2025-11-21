"""
Compatibility for [add|remove]_[reader|writer] where unavailable (Proactor).

Runs select in a background thread.

Adapted from Tornado 6.5.1

:copyright: 2025, The Tornado Authors
:license: Apache-2.0
"""

from __future__ import annotations

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
    Dict,
    List,
    Optional,
    Protocol,
    Set,
    Tuple,
    TypeVar,
    Union,
)


class _HasFileno(Protocol):
    def fileno(self) -> int:
        pass


_FileDescriptorLike = Union[int, _HasFileno]

_T = TypeVar("_T")

# Collection of selector thread event loops to shut down on exit.
_selector_loops: Set["SelectorThread"] = set()


def _atexit_callback() -> None:
    for loop in _selector_loops:
        with loop._select_cond:
            loop._closing_selector = True
            loop._select_cond.notify()
        try:
            loop._waker_w.send(b"a")
        except BlockingIOError:
            pass
        if loop._thread is not None:
            # If we don't join our (daemon) thread here, we may get a deadlock
            # during interpreter shutdown. I don't really understand why. This
            # deadlock happens every time in CI (both travis and appveyor) but
            # I've never been able to reproduce locally.
            loop._thread.join()
    _selector_loops.clear()


atexit.register(_atexit_callback)


class SelectorThread:
    """Define ``add_reader`` methods to be called in a background select thread.

    Instances of this class start a second thread to run a selector.
    This thread is completely hidden from the user;
    all callbacks are run on the wrapped event loop's thread.

    Typically used via ``AddThreadSelectorEventLoop``,
    but can be attached to a running asyncio loop.
    """

    _closed = False

    def __init__(self, real_loop: asyncio.AbstractEventLoop) -> None:
        self._main_thread_ctx = contextvars.copy_context()

        self._real_loop = real_loop

        self._select_cond = threading.Condition()
        self._select_args: Optional[
            Tuple[List[_FileDescriptorLike], List[_FileDescriptorLike]]
        ] = None
        self._closing_selector = False
        self._thread: Optional[threading.Thread] = None
        self._thread_manager_handle = self._thread_manager()

        async def thread_manager_anext() -> None:
            # the anext builtin wasn't added until 3.10. We just need to iterate
            # this generator one step.
            await self._thread_manager_handle.__anext__()

        # When the loop starts, start the thread. Not too soon because we can't
        # clean up if we get to this point but the event loop is closed without
        # starting.
        self._real_loop.call_soon(
            lambda: self._real_loop.create_task(thread_manager_anext()),
            context=self._main_thread_ctx,
        )

        self._readers: Dict[_FileDescriptorLike, Callable] = {}
        self._writers: Dict[_FileDescriptorLike, Callable] = {}

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
        # that due to the order of operations at shutdown, only daemon threads
        # can be shut down in this way (non-daemon threads would require the
        # introduction of a new hook: https://bugs.python.org/issue41962)
        self._thread = threading.Thread(
            name="Tornado selector",
            daemon=True,
            target=self._run_select,
        )
        self._thread.start()
        self._start_select()
        try:
            # The presense of this yield statement means that this coroutine
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
        if self._closed:
            return
        try:
            self._waker_w.send(b"a")
        except BlockingIOError:
            pass

    def _consume_waker(self) -> None:
        try:
            self._waker_r.recv(1024)
        except BlockingIOError:
            pass

    def _start_select(self) -> None:
        # Capture reader and writer sets here in the event loop
        # thread to avoid any problems with concurrent
        # modification while the select loop uses them.
        with self._select_cond:
            assert self._select_args is None
            self._select_args = (list(self._readers.keys()), list(self._writers.keys()))
            self._select_cond.notify()

    def _run_select(self) -> None:
        while True:
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

            try:
                self._real_loop.call_soon_threadsafe(
                    self._handle_select, rs, ws, context=self._main_thread_ctx
                )
            except RuntimeError:
                # "Event loop is closed". Swallow the exception for
                # consistency with PollIOLoop (and logical consistency
                # with the fact that we can't guarantee that an
                # add_callback that completes without error will
                # eventually execute).
                pass
            except AttributeError:
                # ProactorEventLoop may raise this instead of RuntimeError
                # if call_soon_threadsafe races with a call to close().
                # Swallow it too for consistency.
                pass

    def _handle_select(
        self, rs: List[_FileDescriptorLike], ws: List[_FileDescriptorLike]
    ) -> None:
        for r in rs:
            self._handle_event(r, self._readers)
        for w in ws:
            self._handle_event(w, self._writers)
        self._start_select()

    def _handle_event(
        self,
        fd: _FileDescriptorLike,
        cb_map: Dict[_FileDescriptorLike, Callable],
    ) -> None:
        try:
            callback = cb_map[fd]
        except KeyError:
            return
        callback()

    def add_reader(
        self, fd: _FileDescriptorLike, callback: Callable[..., None], *args: Any
    ) -> None:
        self._readers[fd] = functools.partial(callback, *args)
        self._wake_selector()

    def add_writer(
        self, fd: _FileDescriptorLike, callback: Callable[..., None], *args: Any
    ) -> None:
        self._writers[fd] = functools.partial(callback, *args)
        self._wake_selector()

    def remove_reader(self, fd: _FileDescriptorLike) -> bool:
        try:
            del self._readers[fd]
        except KeyError:
            return False
        self._wake_selector()
        return True

    def remove_writer(self, fd: _FileDescriptorLike) -> bool:
        try:
            del self._writers[fd]
        except KeyError:
            return False
        self._wake_selector()
        return True
