# Copyright 2009 Brian Quinlan. All Rights Reserved.
# Licensed to PSF under a Contributor Agreement.

"""Implements ThreadPoolExecutor."""

__author__ = 'Brian Quinlan (brian@sweetapp.com)'

import atexit
from concurrent.futures import _base
import itertools
import queue
import threading
import weakref
import os

# Workers are created as daemon threads. This is done to allow the interpreter
# to exit when there are still idle threads in a ThreadPoolExecutor's thread
# pool (i.e. shutdown() was not called). However, allowing workers to die with
# the interpreter has two undesirable properties:
#   - The workers would still be running during interpreter shutdown,
#     meaning that they would fail in unpredictable ways.
#   - The workers could be killed while evaluating a work item, which could
#     be bad if the callable being evaluated has external side-effects e.g.
#     writing to a file.
#
# To work around this problem, an exit handler is installed which tells the
# workers to exit when their work queues are empty and then waits until the
# threads finish.

_threads_queues = weakref.WeakKeyDictionary()
_shutdown = False

def _python_exit():
    global _shutdown
    _shutdown = True
    items = list(_threads_queues.items())
    for t, q in items:
        q.put(None)
    for t, q in items:
        t.join()

atexit.register(_python_exit)

class _WorkItem(_base._WorkItem):

    def run(self):
        if not self.future.set_running_or_notify_cancel():
            return

        try:
            result = self.fn(*self.args, **self.kwargs)
        except BaseException as exc:
            self.future.set_exception(exc)
            # Break a reference cycle with the exception 'exc'
            self = None
        else:
            self.future.set_result(result)


class _Worker(threading.Thread):
    """Worker Thread for ThreadPoolExecutor --used for introspection"""
    def __init__(self, executor_reference, work_queue, initializer, initargs,
                 name):
        super().__init__(name=name)
        self._executor_reference = executor_reference
        self._work_queue = work_queue
        self._initializer = initializer
        self._initargs = initargs
        self._work_item = None

    def run(self):
        if self._initializer is not None:
            try:
                self._initializer(*self._initargs)
            except BaseException:
                _base.LOGGER.critical('Exception in initializer:',
                                      exc_info=True)
                executor = self._executor_reference()
                if executor is not None:
                    executor._initializer_failed()
                return
        try:
            while True:
                self._work_item = self._work_queue.get(block=True)
                if self._work_item is not None:
                    self._work_item.run()
                    # Delete references to object. See issue16284
                    self._work_item = None
                    continue
                executor = self._executor_reference()
                # Exit if:
                #   - The interpreter is shutting down OR
                #   - The executor that owns the worker has been collected OR
                #   - The executor that owns the worker has been shutdown.
                if _shutdown or executor is None or executor._shutdown:
                    # Notice other workers
                    self._work_queue.put(None)
                    return
                del executor
        except BaseException:
            _base.LOGGER.critical('Exception in worker', exc_info=True)


class BrokenThreadPool(_base.BrokenExecutor):
    """
    Raised when a worker thread in a ThreadPoolExecutor failed initializing.
    """


class ThreadPoolExecutor(_base.Executor):

    # Used to assign unique thread names when thread_name_prefix is not supplied.
    _counter = itertools.count().__next__

    def __init__(self, max_workers=None, thread_name_prefix='',
                 initializer=None, initargs=()):
        """Initializes a new ThreadPoolExecutor instance.

        Args:
            max_workers: The maximum number of threads that can be used to
                execute the given calls.
            thread_name_prefix: An optional name prefix to give our threads.
            initializer: An callable used to initialize worker threads.
            initargs: A tuple of arguments to pass to the initializer.
        """
        if max_workers is None:
            # Use this number because ThreadPoolExecutor is often
            # used to overlap I/O instead of CPU work.
            max_workers = (os.cpu_count() or 1) * 5
        if max_workers <= 0:
            raise ValueError("max_workers must be greater than 0")

        if initializer is not None and not callable(initializer):
            raise TypeError("initializer must be a callable")

        self._max_workers = max_workers
        self._work_queue = queue.Queue()
        self._threads = set()
        self._broken = False
        self._shutdown = False
        self._shutdown_lock = threading.Lock()
        self._thread_name_prefix = (thread_name_prefix or
                                    ("ThreadPoolExecutor-%d" % self._counter()))
        self._initializer = initializer
        self._initargs = initargs

    def submit(self, fn, *args, **kwargs):
        with self._shutdown_lock:
            if self._broken:
                raise BrokenThreadPool(self._broken)

            if self._shutdown:
                raise RuntimeError('cannot schedule new futures after shutdown')

            f = _base.Future()
            w = _WorkItem(f, fn, args, kwargs)

            self._work_queue.put(w)
            self._adjust_thread_count()
            return f
    submit.__doc__ = _base.Executor.submit.__doc__

    def worker_count(self):
        return len(self._threads)

    def active_worker_count(self):
        return self.active_task_count()

    def idle_worker_count(self):
        return self.worker_count() - self.active_worker_count()

    def task_count(self):
        return self.active_task_count() + self.waiting_task_count()

    def active_task_count(self):
        return sum(1 for t in self._threads if t._work_item)

    def waiting_task_count(self):
        return self._work_queue.qsize()

    def active_tasks(self):
        return set(t._work_item for t in self._threads
                   if t._work_item)

    def waiting_tasks(self):
        active = self.active_tasks()
        with self._work_queue.mutex:
            return [task for task in self._work_queue.queue
                    if task not in active]

    def _adjust_thread_count(self):
        # When the executor gets lost, the weakref callback will wake up
        # the worker threads.
        def weakref_cb(_, q=self._work_queue):
            q.put(None)
        # Create a new thread if we're not at the max, and we
        # don't have enough idle threads to handle pending tasks.
        num_threads = len(self._threads)
        if (num_threads < self._max_workers and
                self.idle_worker_count() < self._work_queue.qsize()):
            thread_name = '%s_%d' % (self._thread_name_prefix or self,
                                     num_threads)
            t = _Worker(weakref.ref(self, weakref_cb), self._work_queue,
                        self._initializer, self._initargs, name=thread_name)
            t.daemon = True
            t.start()
            self._threads.add(t)
            _threads_queues[t] = self._work_queue

    def _initializer_failed(self):
        with self._shutdown_lock:
            self._broken = ('A thread initializer failed, the thread pool '
                            'is not usable anymore')
            # Drain work queue and mark pending futures failed
            while True:
                try:
                    work_item = self._work_queue.get_nowait()
                except queue.Empty:
                    break
                if work_item is not None:
                    work_item.future.set_exception(BrokenThreadPool(self._broken))

    def shutdown(self, wait=True):
        with self._shutdown_lock:
            self._shutdown = True
            self._work_queue.put(None)
        if wait:
            for t in self._threads:
                t.join()
    shutdown.__doc__ = _base.Executor.shutdown.__doc__
