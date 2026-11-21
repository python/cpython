#
# Module implementing queues
#
# multiprocessing/queues.py
#
# Copyright (c) 2006-2008, R Oudkerk
# Licensed to PSF under a Contributor Agreement.
#

__all__ = ['Queue', 'SimpleQueue', 'JoinableQueue']

import sys
import os
import threading
import collections
import time
import types
import weakref
import errno

from queue import Empty, Full, ShutDown

from . import connection
from . import context
_ForkingPickler = context.reduction.ForkingPickler

from .util import debug, info, Finalize, register_after_fork, is_exiting

#
# Queue type using a pipe, buffer and thread
#

class Queue(object):

    def __init__(self, maxsize=0, *, ctx):
        if maxsize <= 0:
            # Can raise ImportError (see issues #3770 and #23400)
            from .synchronize import SEM_VALUE_MAX as maxsize
        self._maxsize = maxsize
        self._reader, self._writer = connection.Pipe(duplex=False)
        self._rlock = ctx.Lock()
        self._opid = os.getpid()
        if sys.platform == 'win32':
            self._wlock = None
        else:
            self._wlock = ctx.Lock()
        self._sem = ctx.BoundedSemaphore(maxsize)

        # State used by shutdown().  It is all kept in semaphores, which are
        # only ever tested for zero, because neither ctypes nor a working
        # sem_getvalue() is available on every platform.
        #
        # Where sem_getvalue() is broken (macOS), the zero test acquires
        # the semaphore and releases it again, so a second test made at the
        # same moment can wrongly find it zero.  Every test whose answer
        # matters is therefore made while holding _state_lock.
        self._state_lock = ctx.Lock()
        # Non-zero once shutdown() has been called.
        self._shutdown_flag = ctx.Semaphore(0)
        # Non-zero once shutdown(immediate=True) has been called.
        self._immediate_flag = ctx.Semaphore(0)
        # Number of items accepted by put() in any process and not yet
        # received: those in the pipe and in every process's buffer.
        self._outstanding = ctx.Semaphore(0)

        # For use by concurrent.futures
        self._ignore_epipe = False
        self._reset()

        if sys.platform != 'win32':
            register_after_fork(self, Queue._after_fork)

    def __getstate__(self):
        context.assert_spawning(self)
        return (self._ignore_epipe, self._maxsize, self._reader, self._writer,
                self._rlock, self._wlock, self._sem, self._opid,
                self._state_lock, self._shutdown_flag, self._immediate_flag,
                self._outstanding)

    def __setstate__(self, state):
        (self._ignore_epipe, self._maxsize, self._reader, self._writer,
         self._rlock, self._wlock, self._sem, self._opid,
         self._state_lock, self._shutdown_flag, self._immediate_flag,
         self._outstanding) = state
        self._reset()

    def _after_fork(self):
        debug('Queue._after_fork()')
        self._reset(after_fork=True)

    def _reset(self, after_fork=False):
        if after_fork:
            self._notempty._at_fork_reinit()
        else:
            self._notempty = threading.Condition(threading.Lock())
        self._buffer = collections.deque()
        self._thread = None
        self._jointhread = None
        self._joincancelled = False
        self._closed = False
        self._close = None
        # The shutdown flag is never cleared, so it can be cached once seen.
        self._shutdown_seen = False
        self._send_bytes = self._writer.send_bytes
        self._recv_bytes = self._reader.recv_bytes
        self._poll = self._reader.poll

    def _is_shutdown(self):
        if not self._shutdown_seen:
            with self._state_lock:
                self._shutdown_seen = (
                    not self._shutdown_flag._semlock._is_zero())
        return self._shutdown_seen

    def _release_slot(self):
        try:
            self._sem.release()
        except ValueError:
            # shutdown() may have released _sem once more than there were
            # items, to wake the blocked putters.
            if not self._is_shutdown():
                raise

    def _acquire_slot(self, block, timeout):
        # Reserve room for one item and count it as outstanding, or raise
        # Full or ShutDown.
        if self._shutdown_seen:
            raise ShutDown
        if not self._sem.acquire(block, timeout):
            raise ShutDown if self._is_shutdown() else Full
        if not self._shutdown_seen:
            # Testing the flag and counting the item must be atomic with
            # respect to shutdown(): once a getter has seen the queue shut
            # down with nothing outstanding, no put() may succeed.
            with self._state_lock:
                if self._shutdown_flag._semlock._is_zero():
                    self._outstanding.release()
                    return
                self._shutdown_seen = True
        # Hand the unit on, so that the next blocked putter wakes up too.
        self._release_slot()
        raise ShutDown

    def put(self, obj, block=True, timeout=None):
        if self._closed:
            raise ValueError(f"Queue {self!r} is closed")
        self._acquire_slot(block, timeout)

        with self._notempty:
            if self._thread is None:
                self._start_thread()
            self._buffer.append(obj)
            self._notempty.notify()

    def _received(self):
        # Called with _rlock held, for each item read from the pipe.
        if not self._outstanding.acquire(False):
            # Either a zero test was holding the semaphore (see __init__),
            # or the item was put by a subclass which overrides put()
            # without calling _acquire_slot() and was never counted.
            with self._state_lock:
                self._outstanding.acquire(False)

    def _after_marker(self, block, deadline):
        # Called with _rlock held, after reading the shutdown marker instead
        # of an item.  Return the next item, or raise ShutDown or Empty.
        #
        # shutdown() writes a single marker and whoever reads it must write
        # it back, so that every getter, blocked or yet to come, is woken in
        # its turn.  Extra markers are harmless: they are only wake-ups, and
        # a reader never writes back more than one.
        try:
            while True:
                with self._state_lock:
                    drained = self._outstanding._semlock._is_zero()
                    immediate = not self._immediate_flag._semlock._is_zero()
                if drained:
                    # No item is outstanding.  Still return anything found
                    # in the pipe, such as an item put by a subclass which
                    # does not count them, rather than strand it.
                    if not self._poll():
                        raise ShutDown
                elif not block:
                    if not self._poll():
                        raise Empty
                elif deadline is not None:
                    if not self._poll(max(0.0, deadline - time.monotonic())):
                        raise Empty
                # Otherwise an item is still in some process's buffer: wait
                # for it.  If it gets discarded instead, whoever discards it
                # writes another marker.
                res = self._recv_bytes()
                if not res:
                    continue
                if immediate:
                    self._make_discarder()(1)
                    continue
                return res
        finally:
            self._put_marker()

    def get(self, block=True, timeout=None):
        if self._closed:
            raise ValueError(f"Queue {self!r} is closed")
        if block and timeout is None:
            with self._rlock:
                res = self._recv_bytes()
                if not res:
                    # An empty message is the shutdown marker.
                    res = self._after_marker(True, None)
                self._received()
            self._release_slot()
        else:
            if block:
                deadline = time.monotonic() + timeout
            try:
                if not self._rlock.acquire(block, timeout):
                    raise Empty
                try:
                    if block:
                        timeout = deadline - time.monotonic()
                        if not self._poll(timeout):
                            raise Empty
                    elif not self._poll():
                        raise Empty
                    res = self._recv_bytes()
                    if not res:
                        res = self._after_marker(block,
                                                 deadline if block else None)
                    self._received()
                    self._release_slot()
                finally:
                    self._rlock.release()
            except Empty:
                # The pipe of a shut down queue is briefly empty while a
                # getter writes the marker back.
                if self._is_shutdown() and self._is_drained():
                    raise ShutDown from None
                raise
        # unserialize the data after having released the lock
        return _ForkingPickler.loads(res)

    def _is_drained(self):
        with self._state_lock:
            return self._outstanding._semlock._is_zero()

    def _put_marker(self):
        # The marker goes through the feeder thread so that it follows
        # everything this process has already put, and so that writing it
        # can never block the caller.
        with self._notempty:
            if self._thread is None:
                self._start_thread()
            self._buffer.append(_shutdown_marker)
            self._notempty.notify()

    def _make_discarder(self):
        # Return a function accounting for N items thrown away by
        # shutdown(immediate=True).  It must not refer to self: the feeder
        # thread keeps it, and is only told to exit when the queue is
        # garbage collected.
        sem, outstanding, state_lock = (
            self._sem, self._outstanding, self._state_lock)
        def discard(n):
            for _ in range(n):
                if not outstanding.acquire(False):
                    with state_lock:
                        outstanding.acquire(False)
                try:
                    sem.release()
                except ValueError:
                    pass
        return discard

    def shutdown(self, immediate=False):
        if self._closed:
            raise ValueError(f"Queue {self!r} is closed")
        with self._state_lock:
            first = self._shutdown_flag._semlock._is_zero()
            if first:
                self._shutdown_flag.release()
            if immediate and self._immediate_flag._semlock._is_zero():
                self._immediate_flag.release()
        self._shutdown_seen = True
        if first:
            # Wake a putter blocked on a full queue; it wakes the next one.
            # Putters which acquire a unit from now on give it back, so
            # there is nothing to do if the queue is not full.
            if self._sem._semlock._is_zero():
                self._release_slot()
            # Wake a blocked getter; it wakes the next one.
            self._put_marker()
        if immediate:
            self._discard_all()

    def _discard_all(self):
        discard = self._make_discarder()
        # Items this process has buffered.  Feeder threads, here and in
        # other processes, discard what they hold on seeing the flag.
        with self._notempty:
            keep = [obj for obj in self._buffer
                    if obj is _sentinel or obj is _shutdown_marker]
            n = len(self._buffer) - len(keep)
            self._buffer.clear()
            self._buffer.extend(keep)
        if n:
            discard(n)
            # A getter may be waiting for one of them.
            self._put_marker()
        # Items already in the pipe.
        wlock = self._wlock
        n = 0
        with self._rlock:
            while True:
                while self._poll():
                    if self._recv_bytes():
                        n += 1
                if wlock is None or wlock.acquire(timeout=0.01):
                    break
                # A feeder thread is in the middle of writing, and may be
                # waiting for room in the pipe: keep reading.
            if wlock is None:
                # Windows: there is no write lock to stop other processes
                # from writing, so an item may still get in ahead of the
                # marker and be returned by get().
                self._put_marker()
            else:
                # No one else can write now.  Empty the pipe and leave the
                # marker in it, ahead of any item a feeder thread had
                # already decided to send: the getter which reads the marker
                # throws those away before letting another getter in.
                try:
                    while self._poll():
                        if self._recv_bytes():
                            n += 1
                    self._send_bytes(b'')
                finally:
                    wlock.release()
            discard(n)

    def qsize(self):
        # Raises NotImplementedError on Mac OSX because of broken sem_getvalue()
        return max(0, self._maxsize - self._sem.get_value())

    def empty(self):
        if self._is_shutdown():
            # The pipe now holds the shutdown marker, so whether it can be
            # read says nothing about items.
            return self._is_drained()
        return not self._poll()

    def full(self):
        return self._sem._semlock._is_zero()

    def get_nowait(self):
        return self.get(False)

    def put_nowait(self, obj):
        return self.put(obj, False)

    def close(self):
        self._closed = True
        close = self._close
        if close:
            self._close = None
            close()

    def join_thread(self):
        debug('Queue.join_thread()')
        assert self._closed, "Queue {0!r} not closed".format(self)
        if self._jointhread:
            self._jointhread()

    def cancel_join_thread(self):
        debug('Queue.cancel_join_thread()')
        self._joincancelled = True
        try:
            self._jointhread.cancel()
        except AttributeError:
            pass

    def _terminate_broken(self):
        # Close a Queue on error.

        # gh-94777: Prevent queue writing to a pipe which is no longer read.
        self._reader.close()

        # gh-107219: Close the connection writer which can unblock
        # Queue._feed() if it was stuck in send_bytes().
        if sys.platform == 'win32':
            self._writer.close()

        self.close()
        self.join_thread()

    def _start_thread(self):
        debug('Queue._start_thread()')

        # Start thread which transfers data from buffer to pipe
        self._buffer.clear()
        self._thread = threading.Thread(
            target=Queue._feed,
            args=(self._buffer, self._notempty, self._send_bytes,
                  self._wlock, self._reader.close, self._writer.close,
                  self._ignore_epipe, self._on_queue_feeder_error,
                  self._sem, self._make_discarder(),
                  (self._state_lock, self._shutdown_flag,
                   self._immediate_flag, self._outstanding)),
            name='QueueFeederThread',
            daemon=True,
        )

        try:
            debug('doing self._thread.start()')
            self._thread.start()
            debug('... done self._thread.start()')
        except:
            # gh-109047: During Python finalization, creating a thread
            # can fail with RuntimeError.
            self._thread = None
            raise

        if not self._joincancelled:
            self._jointhread = Finalize(
                self._thread, Queue._finalize_join,
                [weakref.ref(self._thread)],
                exitpriority=-5
                )

        # Send sentinel to the thread queue object when garbage collected
        self._close = Finalize(
            self, Queue._finalize_close,
            [self._buffer, self._notempty],
            exitpriority=10
            )

    @staticmethod
    def _finalize_join(twr):
        debug('joining queue thread')
        thread = twr()
        if thread is not None:
            thread.join()
            debug('... queue thread joined')
        else:
            debug('... queue thread already dead')

    @staticmethod
    def _finalize_close(buffer, notempty):
        debug('telling queue thread to quit')
        with notempty:
            buffer.append(_sentinel)
            notempty.notify()

    @staticmethod
    def _feed(buffer, notempty, send_bytes, writelock, reader_close,
              writer_close, ignore_epipe, onerror, queue_sem,
              discard, shutdown_state):
        debug('starting thread to feed data to pipe')
        nacquire = notempty.acquire
        nrelease = notempty.release
        nwait = notempty.wait
        bpopleft = buffer.popleft
        sentinel = _sentinel
        marker = _shutdown_marker
        state_lock, shutdown_flag, immediate_flag, outstanding = shutdown_state
        not_immediate = immediate_flag._semlock._is_zero
        if sys.platform != 'win32':
            wacquire = writelock.acquire
            wrelease = writelock.release
        else:
            wacquire = None

        def send(data):
            if wacquire is None:
                send_bytes(data)
            else:
                wacquire()
                try:
                    send_bytes(data)
                finally:
                    wrelease()

        while 1:
            is_item = False
            try:
                nacquire()
                try:
                    if not buffer:
                        nwait()
                finally:
                    nrelease()
                ndiscarded = 0
                try:
                    while 1:
                        is_item = False
                        obj = bpopleft()
                        if obj is sentinel or obj is marker:
                            if ndiscarded:
                                # A getter may be waiting for one of the
                                # discarded items: wake it.
                                discard(ndiscarded)
                                ndiscarded = 0
                                send(b'')
                            if obj is sentinel:
                                debug('feeder thread got sentinel -- exiting')
                                reader_close()
                                writer_close()
                                return
                            # An empty message is never a valid pickle.
                            send(b'')
                            continue

                        # After shutdown(immediate=True), throw away what
                        # has not been sent yet.  This test is not made
                        # under state_lock, to keep the feeder cheap: at
                        # worst an item is sent which should not have been.
                        if not not_immediate():
                            ndiscarded += 1
                            continue

                        is_item = True
                        # serialize the data before acquiring the lock
                        obj = _ForkingPickler.dumps(obj)
                        send(obj)
                except IndexError:
                    if ndiscarded:
                        discard(ndiscarded)
                        send(b'')
            except Exception as e:
                if ignore_epipe and getattr(e, 'errno', 0) == errno.EPIPE:
                    return
                # Since this runs in a daemon thread the resources it uses
                # may be become unusable while the process is cleaning up.
                # We ignore errors which happen after the process has
                # started to cleanup.
                if is_exiting():
                    info('error in queue thread: %s', e)
                    return
                else:
                    # Since the object has not been sent in the queue, we need
                    # to decrease the size of the queue. The error acts as
                    # if the object had been silently removed from the queue
                    # and this step is necessary to have a properly working
                    # queue.
                    if is_item:
                        queue_sem.release()
                        with state_lock:
                            outstanding.acquire(False)
                            shutdown = not shutdown_flag._semlock._is_zero()
                        if shutdown:
                            # A getter may be waiting for this item.
                            buffer.appendleft(marker)
                    onerror(e, obj)

    @staticmethod
    def _on_queue_feeder_error(e, obj):
        """
        Private API hook called when feeding data in the background thread
        raises an exception.  For overriding by concurrent.futures.
        """
        import traceback
        traceback.print_exc()

    __class_getitem__ = classmethod(types.GenericAlias)


_sentinel = object()
# Put in a feeder thread's buffer to have the shutdown marker, an empty
# message, written to the pipe.
_shutdown_marker = object()

#
# A queue type which also supports join() and task_done() methods
#
# Note that if you do not call task_done() for each finished task then
# eventually the counter's semaphore may overflow causing Bad Things
# to happen.
#

class JoinableQueue(Queue):

    def __init__(self, maxsize=0, *, ctx):
        Queue.__init__(self, maxsize, ctx=ctx)
        self._unfinished_tasks = ctx.Semaphore(0)
        self._cond = ctx.Condition()

    def __getstate__(self):
        return Queue.__getstate__(self) + (self._cond, self._unfinished_tasks)

    def __setstate__(self, state):
        Queue.__setstate__(self, state[:-2])
        self._cond, self._unfinished_tasks = state[-2:]

    def put(self, obj, block=True, timeout=None):
        if self._closed:
            raise ValueError(f"Queue {self!r} is closed")
        self._acquire_slot(block, timeout)

        with self._notempty, self._cond:
            if self._thread is None:
                self._start_thread()
            self._buffer.append(obj)
            self._unfinished_tasks.release()
            self._notempty.notify()

    def task_done(self):
        with self._cond:
            if not self._unfinished_tasks.acquire(False):
                raise ValueError('task_done() called too many times')
            if self._unfinished_tasks._semlock._is_zero():
                self._cond.notify_all()

    def join(self):
        with self._cond:
            if not self._unfinished_tasks._semlock._is_zero():
                self._cond.wait()

    def _make_discarder(self):
        discard_items = super()._make_discarder()
        cond, unfinished_tasks = self._cond, self._unfinished_tasks
        def discard(n):
            discard_items(n)
            # Only the discarded items are done: a task which a worker has
            # already taken still needs its task_done().
            with cond:
                for _ in range(n):
                    if not unfinished_tasks.acquire(False):
                        break
                if unfinished_tasks._semlock._is_zero():
                    cond.notify_all()
        return discard

#
# Simplified Queue type -- really just a locked pipe
#

class SimpleQueue(object):

    def __init__(self, *, ctx):
        self._reader, self._writer = connection.Pipe(duplex=False)
        self._rlock = ctx.Lock()
        self._poll = self._reader.poll
        if sys.platform == 'win32':
            self._wlock = None
        else:
            self._wlock = ctx.Lock()

    def close(self):
        self._reader.close()
        self._writer.close()

    def empty(self):
        return not self._poll()

    def __getstate__(self):
        context.assert_spawning(self)
        return (self._reader, self._writer, self._rlock, self._wlock)

    def __setstate__(self, state):
        (self._reader, self._writer, self._rlock, self._wlock) = state
        self._poll = self._reader.poll

    def get(self):
        with self._rlock:
            res = self._reader.recv_bytes()
        # unserialize the data after having released the lock
        return _ForkingPickler.loads(res)

    def put(self, obj):
        # serialize the data before acquiring the lock
        obj = _ForkingPickler.dumps(obj)
        if self._wlock is None:
            # writes to a message oriented win32 pipe are atomic
            self._writer.send_bytes(obj)
        else:
            with self._wlock:
                self._writer.send_bytes(obj)

    __class_getitem__ = classmethod(types.GenericAlias)
