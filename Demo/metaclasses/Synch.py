"""Synchronization metaclass.

This metaclass  makes it possible to declare synchronized methods.

"""

import thread

# First we need to define a reentrant lock.
# This is generally useful and should probably be in a standard Python
# library module.  For now, we in-line it.

class Lock:

    """Reentrant lock.

    This is a mutex-like object which can be acquired by the same
    thread more than once.  It keeps a reference count of the number
    of times it has been acquired by the same thread.  Each acquire()
    call must be matched by a release() call and only the last
    release() call actually releases the lock for acquisition by
    another thread.

    The implementation uses two locks internally:

    __mutex is a short term lock used to protect the instance variables
    __wait is the lock for which other threads wait

    A thread intending to acquire both locks should acquire __wait
    first.

   The implementation uses two other instance variables, protected by
   locking __mutex:

    __tid is the thread ID of the thread that currently has the lock
    __count is the number of times the current thread has acquired it

    When the lock is released, __tid is None and __count is zero.

    """

    def __init__(self):
        """Constructor.  Initialize all instance variables."""
        self.__mutex = thread.allocate_lock()
        self.__wait = thread.allocate_lock()
        self.__tid = None
        self.__count = 0

    def acquire(self, flag=1):
        """Acquire the lock.

        If the optional flag argument is false, returns immediately
        when it cannot acquire the __wait lock without blocking (it
        may still block for a little while in order to acquire the
        __mutex lock).

        The return value is only relevant when the flag argument is
        false; it is 1 if the lock is acquired, 0 if not.

        """
        self.__mutex.acquire()
        try:
            if self.__tid == thread.get_ident():
                self.__count = self.__count + 1
                return 1
        finally:
            self.__mutex.release()
        locked = self.__wait.acquire(flag)
        if not flag and not locked:
            return 0
        try:
            self.__mutex.acquire()
            assert self.__tid == None
            assert self.__count == 0
            self.__tid = thread.get_ident()
            self.__count = 1
            return 1
        finally:
            self.__mutex.release()

    def release(self):
        """Release the lock.

        If this thread doesn't currently have the lock, an assertion
        error is raised.

        Only allow another thread to acquire the lock when the count
        reaches zero after decrementing it.

        """
        self.__mutex.acquire()
        try:
            assert self.__tid == thread.get_ident()
            assert self.__count > 0
            self.__count = self.__count - 1
            if self.__count == 0:
                self.__tid = None
                self.__wait.release()
        finally:
            self.__mutex.release()


def _testLock():

    done = []

    def f2(lock, done=done):
        lock.acquire()
        print "f2 running in thread %d\n" % thread.get_ident(),
        lock.release()
        done.append(1)

    def f1(lock, f2=f2, done=done):
        lock.acquire()
        print "f1 running in thread %d\n" % thread.get_ident(),
        try:
            f2(lock)
        finally:
            lock.release()
        done.append(1)

    lock = Lock()
    lock.acquire()
    f1(lock)                            # Adds 2 to done
    lock.release()

    lock.acquire()
    
    thread.start_new_thread(f1, (lock,)) # Adds 2
    thread.start_new_thread(f1, (lock, f1)) # Adds 3
    thread.start_new_thread(f2, (lock,)) # Adds 1
    thread.start_new_thread(f2, (lock,)) # Adds 1

    lock.release()
    import time
    while len(done) < 9:
        print len(done)
        time.sleep(0.001)
    print len(done)


# Now, the Locking metaclass is a piece of cake.
# As an example feature, methods whose name begins with exactly one
# underscore are not synchronized.

from Meta import MetaClass, MetaHelper, MetaMethodWrapper

class LockingMethodWrapper(MetaMethodWrapper):
    def __call__(self, *args, **kw):
        if self.__name__[:1] == '_' and self.__name__[1:] != '_':
            return apply(self.func, (self.inst,) + args, kw)
        self.inst.__lock__.acquire()
        try:
            return apply(self.func, (self.inst,) + args, kw)
        finally:
            self.inst.__lock__.release()

class LockingHelper(MetaHelper):
    __methodwrapper__ = LockingMethodWrapper
    def __helperinit__(self, formalclass):
        MetaHelper.__helperinit__(self, formalclass)
        self.__lock__ = Lock()

class LockingMetaClass(MetaClass):
    __helper__ = LockingHelper

Locking = LockingMetaClass('Locking', (), {})

def _test():
    # For kicks, take away the Locking base class and see it die
    class Buffer(Locking):
        def __init__(self, initialsize):
            assert initialsize > 0
            self.size = initialsize
            self.buffer = [None]*self.size
            self.first = self.last = 0
        def put(self, item):
            # Do we need to grow the buffer?
            if (self.last+1) % self.size != self.first:
                # Insert the new item
                self.buffer[self.last] = item
                self.last = (self.last+1) % self.size
                return
            # Double the buffer size
            # First normalize it so that first==0 and last==size-1
            print "buffer =", self.buffer
            print "first = %d, last = %d, size = %d" % (
                self.first, self.last, self.size)
            if self.first <= self.last:
                temp = self.buffer[self.first:self.last]
            else:
                temp = self.buffer[self.first:] + self.buffer[:self.last]
            print "temp =", temp
            self.buffer = temp + [None]*(self.size+1)
            self.first = 0
            self.last = self.size-1
            self.size = self.size*2
            print "Buffer size doubled to", self.size
            print "new buffer =", self.buffer
            print "first = %d, last = %d, size = %d" % (
                self.first, self.last, self.size)
            self.put(item)              # Recursive call to test the locking
        def get(self):
            # Is the buffer empty?
            if self.first == self.last:
                raise EOFError          # Avoid defining a new exception
            item = self.buffer[self.first]
            self.first = (self.first+1) % self.size
            return item

    def producer(buffer, wait, n=1000):
        import time
        i = 0
        while i < n:
            print "put", i
            buffer.put(i)
            i = i+1
        print "Producer: done producing", n, "items"
        wait.release()

    def consumer(buffer, wait, n=1000):
        import time
        i = 0
        tout = 0.001
        while i < n:
            try:
                x = buffer.get()
                if x != i:
                    raise AssertionError, \
                          "get() returned %s, expected %s" % (x, i)
                print "got", i
                i = i+1
                tout = 0.001
            except EOFError:
                time.sleep(tout)
                tout = tout*2
        print "Consumer: done consuming", n, "items"
        wait.release()

    pwait = thread.allocate_lock()
    pwait.acquire()
    cwait = thread.allocate_lock()
    cwait.acquire()
    buffer = Buffer(1)
    n = 1000
    thread.start_new_thread(consumer, (buffer, cwait, n))
    thread.start_new_thread(producer, (buffer, pwait, n))
    pwait.acquire()
    print "Producer done"
    cwait.acquire()
    print "All done"
    print "buffer size ==", len(buffer.buffer)

if __name__ == '__main__':
    _testLock()
    _test()
