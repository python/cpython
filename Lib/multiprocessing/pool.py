#
# Module providing the `Pool` class for managing a process pool
#
# multiprocessing/pool.py
#
# Copyright (c) 2006-2008, R Oudkerk
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of author nor the names of any contributors may be
#    used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

__all__ = ['Pool']

#
# Imports
#

import threading
import queue
import itertools
import collections
import time

from multiprocessing import Process, cpu_count, TimeoutError
from multiprocessing.util import Finalize, debug

#
# Constants representing the state of a pool
#

RUN = 0
CLOSE = 1
TERMINATE = 2

#
# Miscellaneous
#

job_counter = itertools.count()

def mapstar(args):
    return list(map(*args))

#
# Code run by worker processes
#

def worker(inqueue, outqueue, initializer=None, initargs=()):
    put = outqueue.put
    get = inqueue.get
    if hasattr(inqueue, '_writer'):
        inqueue._writer.close()
        outqueue._reader.close()

    if initializer is not None:
        initializer(*initargs)

    while 1:
        try:
            task = get()
        except (EOFError, IOError):
            debug('worker got EOFError or IOError -- exiting')
            break

        if task is None:
            debug('worker got sentinel -- exiting')
            break

        job, i, func, args, kwds = task
        try:
            result = (True, func(*args, **kwds))
        except Exception as e:
            result = (False, e)
        put((job, i, result))

#
# Class representing a process pool
#

class Pool(object):
    '''
    Class which supports an async version of applying functions to arguments.
    '''
    Process = Process

    def __init__(self, processes=None, initializer=None, initargs=()):
        self._setup_queues()
        self._taskqueue = queue.Queue()
        self._cache = {}
        self._state = RUN

        if processes is None:
            try:
                processes = cpu_count()
            except NotImplementedError:
                processes = 1

        if initializer is not None and not hasattr(initializer, '__call__'):
            raise TypeError('initializer must be a callable')

        self._pool = []
        for i in range(processes):
            w = self.Process(
                target=worker,
                args=(self._inqueue, self._outqueue, initializer, initargs)
                )
            self._pool.append(w)
            w.name = w.name.replace('Process', 'PoolWorker')
            w.daemon = True
            w.start()

        self._task_handler = threading.Thread(
            target=Pool._handle_tasks,
            args=(self._taskqueue, self._quick_put, self._outqueue, self._pool)
            )
        self._task_handler.daemon = True
        self._task_handler._state = RUN
        self._task_handler.start()

        self._result_handler = threading.Thread(
            target=Pool._handle_results,
            args=(self._outqueue, self._quick_get, self._cache)
            )
        self._result_handler.daemon = True
        self._result_handler._state = RUN
        self._result_handler.start()

        self._terminate = Finalize(
            self, self._terminate_pool,
            args=(self._taskqueue, self._inqueue, self._outqueue, self._pool,
                  self._task_handler, self._result_handler, self._cache),
            exitpriority=15
            )

    def _setup_queues(self):
        from .queues import SimpleQueue
        self._inqueue = SimpleQueue()
        self._outqueue = SimpleQueue()
        self._quick_put = self._inqueue._writer.send
        self._quick_get = self._outqueue._reader.recv

    def apply(self, func, args=(), kwds={}):
        '''
        Equivalent of `func(*args, **kwds)`.
        '''
        assert self._state == RUN
        return self.apply_async(func, args, kwds).get()

    def map(self, func, iterable, chunksize=None):
        '''
        Apply `func` to each element in `iterable`, collecting the results
        in a list that is returned.
        '''
        assert self._state == RUN
        return self.map_async(func, iterable, chunksize).get()

    def imap(self, func, iterable, chunksize=1):
        '''
        Equivalent of `map()` -- can be MUCH slower than `Pool.map()`.
        '''
        assert self._state == RUN
        if chunksize == 1:
            result = IMapIterator(self._cache)
            self._taskqueue.put((((result._job, i, func, (x,), {})
                         for i, x in enumerate(iterable)), result._set_length))
            return result
        else:
            assert chunksize > 1
            task_batches = Pool._get_tasks(func, iterable, chunksize)
            result = IMapIterator(self._cache)
            self._taskqueue.put((((result._job, i, mapstar, (x,), {})
                     for i, x in enumerate(task_batches)), result._set_length))
            return (item for chunk in result for item in chunk)

    def imap_unordered(self, func, iterable, chunksize=1):
        '''
        Like `imap()` method but ordering of results is arbitrary.
        '''
        assert self._state == RUN
        if chunksize == 1:
            result = IMapUnorderedIterator(self._cache)
            self._taskqueue.put((((result._job, i, func, (x,), {})
                         for i, x in enumerate(iterable)), result._set_length))
            return result
        else:
            assert chunksize > 1
            task_batches = Pool._get_tasks(func, iterable, chunksize)
            result = IMapUnorderedIterator(self._cache)
            self._taskqueue.put((((result._job, i, mapstar, (x,), {})
                     for i, x in enumerate(task_batches)), result._set_length))
            return (item for chunk in result for item in chunk)

    def apply_async(self, func, args=(), kwds={}, callback=None):
        '''
        Asynchronous version of `apply()` method.
        '''
        assert self._state == RUN
        result = ApplyResult(self._cache, callback)
        self._taskqueue.put(([(result._job, None, func, args, kwds)], None))
        return result

    def map_async(self, func, iterable, chunksize=None, callback=None):
        '''
        Asynchronous version of `map()` method.
        '''
        assert self._state == RUN
        if not hasattr(iterable, '__len__'):
            iterable = list(iterable)

        if chunksize is None:
            chunksize, extra = divmod(len(iterable), len(self._pool) * 4)
            if extra:
                chunksize += 1
        if len(iterable) == 0:
            chunksize = 0

        task_batches = Pool._get_tasks(func, iterable, chunksize)
        result = MapResult(self._cache, chunksize, len(iterable), callback)
        self._taskqueue.put((((result._job, i, mapstar, (x,), {})
                              for i, x in enumerate(task_batches)), None))
        return result

    @staticmethod
    def _handle_tasks(taskqueue, put, outqueue, pool):
        thread = threading.current_thread()

        for taskseq, set_length in iter(taskqueue.get, None):
            i = -1
            for i, task in enumerate(taskseq):
                if thread._state:
                    debug('task handler found thread._state != RUN')
                    break
                try:
                    put(task)
                except IOError:
                    debug('could not put task on queue')
                    break
            else:
                if set_length:
                    debug('doing set_length()')
                    set_length(i+1)
                continue
            break
        else:
            debug('task handler got sentinel')


        try:
            # tell result handler to finish when cache is empty
            debug('task handler sending sentinel to result handler')
            outqueue.put(None)

            # tell workers there is no more work
            debug('task handler sending sentinel to workers')
            for p in pool:
                put(None)
        except IOError:
            debug('task handler got IOError when sending sentinels')

        debug('task handler exiting')

    @staticmethod
    def _handle_results(outqueue, get, cache):
        thread = threading.current_thread()

        while 1:
            try:
                task = get()
            except (IOError, EOFError):
                debug('result handler got EOFError/IOError -- exiting')
                return

            if thread._state:
                assert thread._state == TERMINATE
                debug('result handler found thread._state=TERMINATE')
                break

            if task is None:
                debug('result handler got sentinel')
                break

            job, i, obj = task
            try:
                cache[job]._set(i, obj)
            except KeyError:
                pass

        while cache and thread._state != TERMINATE:
            try:
                task = get()
            except (IOError, EOFError):
                debug('result handler got EOFError/IOError -- exiting')
                return

            if task is None:
                debug('result handler ignoring extra sentinel')
                continue
            job, i, obj = task
            try:
                cache[job]._set(i, obj)
            except KeyError:
                pass

        if hasattr(outqueue, '_reader'):
            debug('ensuring that outqueue is not full')
            # If we don't make room available in outqueue then
            # attempts to add the sentinel (None) to outqueue may
            # block.  There is guaranteed to be no more than 2 sentinels.
            try:
                for i in range(10):
                    if not outqueue._reader.poll():
                        break
                    get()
            except (IOError, EOFError):
                pass

        debug('result handler exiting: len(cache)=%s, thread._state=%s',
              len(cache), thread._state)

    @staticmethod
    def _get_tasks(func, it, size):
        it = iter(it)
        while 1:
            x = tuple(itertools.islice(it, size))
            if not x:
                return
            yield (func, x)

    def __reduce__(self):
        raise NotImplementedError(
              'pool objects cannot be passed between processes or pickled'
              )

    def close(self):
        debug('closing pool')
        if self._state == RUN:
            self._state = CLOSE
            self._taskqueue.put(None)

    def terminate(self):
        debug('terminating pool')
        self._state = TERMINATE
        self._terminate()

    def join(self):
        debug('joining pool')
        assert self._state in (CLOSE, TERMINATE)
        self._task_handler.join()
        self._result_handler.join()
        for p in self._pool:
            p.join()

    @staticmethod
    def _help_stuff_finish(inqueue, task_handler, size):
        # task_handler may be blocked trying to put items on inqueue
        debug('removing tasks from inqueue until task handler finished')
        inqueue._rlock.acquire()
        while task_handler.is_alive() and inqueue._reader.poll():
            inqueue._reader.recv()
            time.sleep(0)

    @classmethod
    def _terminate_pool(cls, taskqueue, inqueue, outqueue, pool,
                        task_handler, result_handler, cache):
        # this is guaranteed to only be called once
        debug('finalizing pool')

        task_handler._state = TERMINATE
        taskqueue.put(None)                 # sentinel

        debug('helping task handler/workers to finish')
        cls._help_stuff_finish(inqueue, task_handler, len(pool))

        assert result_handler.is_alive() or len(cache) == 0

        result_handler._state = TERMINATE
        outqueue.put(None)                  # sentinel

        if pool and hasattr(pool[0], 'terminate'):
            debug('terminating workers')
            for p in pool:
                p.terminate()

        debug('joining task handler')
        task_handler.join(1e100)

        debug('joining result handler')
        result_handler.join(1e100)

        if pool and hasattr(pool[0], 'terminate'):
            debug('joining pool workers')
            for p in pool:
                if p.is_alive():
                    # worker has not yet exited
                    debug('cleaning up worker %d' % p.pid)
                    p.join()

#
# Class whose instances are returned by `Pool.apply_async()`
#

class ApplyResult(object):

    def __init__(self, cache, callback):
        self._cond = threading.Condition(threading.Lock())
        self._job = next(job_counter)
        self._cache = cache
        self._ready = False
        self._callback = callback
        cache[self._job] = self

    def ready(self):
        return self._ready

    def successful(self):
        assert self._ready
        return self._success

    def wait(self, timeout=None):
        self._cond.acquire()
        try:
            if not self._ready:
                self._cond.wait(timeout)
        finally:
            self._cond.release()

    def get(self, timeout=None):
        self.wait(timeout)
        if not self._ready:
            raise TimeoutError
        if self._success:
            return self._value
        else:
            raise self._value

    def _set(self, i, obj):
        self._success, self._value = obj
        if self._callback and self._success:
            self._callback(self._value)
        self._cond.acquire()
        try:
            self._ready = True
            self._cond.notify()
        finally:
            self._cond.release()
        del self._cache[self._job]

#
# Class whose instances are returned by `Pool.map_async()`
#

class MapResult(ApplyResult):

    def __init__(self, cache, chunksize, length, callback):
        ApplyResult.__init__(self, cache, callback)
        self._success = True
        self._value = [None] * length
        self._chunksize = chunksize
        if chunksize <= 0:
            self._number_left = 0
            self._ready = True
        else:
            self._number_left = length//chunksize + bool(length % chunksize)

    def _set(self, i, success_result):
        success, result = success_result
        if success:
            self._value[i*self._chunksize:(i+1)*self._chunksize] = result
            self._number_left -= 1
            if self._number_left == 0:
                if self._callback:
                    self._callback(self._value)
                del self._cache[self._job]
                self._cond.acquire()
                try:
                    self._ready = True
                    self._cond.notify()
                finally:
                    self._cond.release()

        else:
            self._success = False
            self._value = result
            del self._cache[self._job]
            self._cond.acquire()
            try:
                self._ready = True
                self._cond.notify()
            finally:
                self._cond.release()

#
# Class whose instances are returned by `Pool.imap()`
#

class IMapIterator(object):

    def __init__(self, cache):
        self._cond = threading.Condition(threading.Lock())
        self._job = next(job_counter)
        self._cache = cache
        self._items = collections.deque()
        self._index = 0
        self._length = None
        self._unsorted = {}
        cache[self._job] = self

    def __iter__(self):
        return self

    def next(self, timeout=None):
        self._cond.acquire()
        try:
            try:
                item = self._items.popleft()
            except IndexError:
                if self._index == self._length:
                    raise StopIteration
                self._cond.wait(timeout)
                try:
                    item = self._items.popleft()
                except IndexError:
                    if self._index == self._length:
                        raise StopIteration
                    raise TimeoutError
        finally:
            self._cond.release()

        success, value = item
        if success:
            return value
        raise value

    __next__ = next                    # XXX

    def _set(self, i, obj):
        self._cond.acquire()
        try:
            if self._index == i:
                self._items.append(obj)
                self._index += 1
                while self._index in self._unsorted:
                    obj = self._unsorted.pop(self._index)
                    self._items.append(obj)
                    self._index += 1
                self._cond.notify()
            else:
                self._unsorted[i] = obj

            if self._index == self._length:
                del self._cache[self._job]
        finally:
            self._cond.release()

    def _set_length(self, length):
        self._cond.acquire()
        try:
            self._length = length
            if self._index == self._length:
                self._cond.notify()
                del self._cache[self._job]
        finally:
            self._cond.release()

#
# Class whose instances are returned by `Pool.imap_unordered()`
#

class IMapUnorderedIterator(IMapIterator):

    def _set(self, i, obj):
        self._cond.acquire()
        try:
            self._items.append(obj)
            self._index += 1
            self._cond.notify()
            if self._index == self._length:
                del self._cache[self._job]
        finally:
            self._cond.release()

#
#
#

class ThreadPool(Pool):

    from .dummy import Process

    def __init__(self, processes=None, initializer=None, initargs=()):
        Pool.__init__(self, processes, initializer, initargs)

    def _setup_queues(self):
        self._inqueue = queue.Queue()
        self._outqueue = queue.Queue()
        self._quick_put = self._inqueue.put
        self._quick_get = self._outqueue.get

    @staticmethod
    def _help_stuff_finish(inqueue, task_handler, size):
        # put sentinels at head of inqueue to make workers finish
        inqueue.not_empty.acquire()
        try:
            inqueue.queue.clear()
            inqueue.queue.extend([None] * size)
            inqueue.not_empty.notify_all()
        finally:
            inqueue.not_empty.release()
