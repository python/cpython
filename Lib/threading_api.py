"""Proposed new higher-level threading interfaces.

This module is safe for use with 'from threading import *'.  It
defines the following objects:

Lock()
    A factory function that returns a new primitive lock object.  Once
    a thread has acquired it, subsequent attempts to acquire it block,
    until it is released; any thread may release it.

RLock()
    A factory function that returns a new reentrant lock object.
    A reentrant lock must be released by the thread that acquired it.
    Once a thread has acquired a reentrant lock, the same thread may
    acquire it again without blocking; the thread must release it once
    for each time it has acquired it.

Condition()
    A factory function that returns a new condition variable object.
    A condition variable allows one or more threads to wait until they
    are notified by another thread.

Semaphore()
    A factory function that returns a new semaphore object.  A
    semaphore manages a counter representing the number of release()
    calls minus the number of acquire() calls, plus an initial value.
    The acquire() method blocks if necessary until it can return
    without making the counter negative.

Event()
    A factory function that returns a new event object.  An event
    manages a flag that can be set to true with the set() method and
    reset to false with the clear() method.  The wait() method blocks
    until the flag is true.

Thread
    A class that represents a thread of control -- subclassable.

currentThread()
    A function that returns the Thread object for the caller's thread.

activeCount()
    A function that returns the number of currently active threads.

enumerate()
    A function that returns a list of all currently active threads.

Detailed interfaces for each of these are documented below in the form
of pseudo class definitions.  Note that the classes marked as ``do not
subclass'' are actually implemented as factory functions; classes are
shown here as a way to structure the documentation only.

The design of this module is loosely based on Java's threading model.
However, where Java makes locks and condition variables basic behavior
of every object, they are separate objects in Python.  Python's Thread
class supports a subset of the behavior of Java's Thread class;
currently, there are no priorities, no thread groups, and threads
cannot be destroyed, stopped, suspended, resumed, or interrupted.  The
static methods of Java's Thread class, when implemented, are mapped to
module-level functions.

All methods described below are executed atomically.

"""


class Lock:
    """Primitive lock object.

    *** DO NOT SUBCLASS THIS CLASS ***

    A primitive lock is a synchronization primitive that is not owned
    by a particular thread when locked.  In Python, it is currently
    the lowest level synchronization primitive available, implemented
    directly by the thread extension module.

    A primitive lock is in one of two states, ``locked'' or
    ``unlocked''.  It is created in the unlocked state.  It has two
    basic methods, acquire() and release().  When the state is
    unlocked, acquire() changes the state to locked and returns
    immediately.  When the state is locked, acquire() blocks until a
    call to release() in another thread changes it to unlocked, then
    the acquire() call resets it to locked and returns.  The release()
    method should only be called in the locked state; it changes the
    state to unlocked and returns immediately.  When more than one
    thread is blocked in acquire() waiting for the state to turn to
    unlocked, only one thread proceeds when a release() call resets
    the state to unlocked; which one of the waiting threads proceeds
    is not defined, and may vary across implementations.

    All methods are executed atomically.

    """

    def acquire(self, blocking=1):
        """Acquire a lock, blocking or non-blocking.

        When invoked without arguments, block until the lock is
        unlocked, then set it to locked, and return.  There is no
        return value in this case.

        When invoked with the 'blocking' argument set to true, do the
        same thing as when called without arguments, and return true.

        When invoked with the 'blocking' argument set to false, do not
        block.  If a call without argument would block, return false
        immediately; otherwise, do the same thing as when called
        without arguments, and return true.

        """

    def release(self):
        """Release a lock.

        When the lock is locked, reset it to unlocked, and return.  If
        any other threads are blocked waiting for the lock to become
        unlocked, allow exactly one of them to proceed.

        Do not call this method when the lock is unlocked.

        There is no return value.

        """


class RLock:
    """Reentrant lock object.

    *** DO NOT SUBCLASS THIS CLASS ***

    A reentrant lock is a synchronization primitive that may be
    acquired multiple times by the same thread.  Internally, it uses
    the concepts of ``owning thread'' and ``recursion level'' in
    addition to the locked/unlocked state used by primitive locks.  In
    the locked state, some thread owns the lock; in the unlocked
    state, no thread owns it.

    To lock the lock, a thread calls its acquire() method; this
    returns once the thread owns the lock.  To unlock the lock, a
    thread calls its release() method.  acquire()/release() call pairs
    may be nested; only the final release() (i.e. the release() of the
    outermost pair) resets the lock to unlocked and allows another
    thread blocked in acquire() to proceed.

    """

    def acquire(self, blocking=1):
        """Acquire a lock, blocking or non-blocking.

        When invoked without arguments: if this thread already owns
        the lock, increment the recursion level by one, and return
        immediately.  Otherwise, if another thread owns the lock,
        block until the lock is unlocked.  Once the lock is unlocked
        (not owned by any thread), then grab ownership, set the
        recursion level to one, and return.  If more than one thread
        is blocked waiting until the lock is unlocked, only one at a
        time will be able to grab ownership of the lock.  There is no
        return value in this case.

        When invoked with the 'blocking' argument set to true, do the
        same thing as when called without arguments, and return true.

        When invoked with the 'blocking' argument set to false, do not
        block.  If a call without argument would block, return false
        immediately; otherwise, do the same thing as when called
        without arguments, and return true.

        """

    def release(self):
        """Release a lock.

        Only call this method when the calling thread owns the lock.
        Decrement the recursion level.  If after the decrement it is
        zero, reset the lock to unlocked (not owned by any thread),
        and if any other threads are blocked waiting for the lock to
        become unlocked, allow exactly one of them to proceed.  If
        after the decrement the recursion level is still nonzero, the
        lock remains locked and owned by the calling thread.

        Do not call this method when the lock is unlocked.

        There is no return value.

        """


class Condition:
    """Synchronized condition variable object.

    *** DO NOT SUBCLASS THIS CLASS ***

    A condition variable is always associated with some kind of lock;
    this can be passed in or one will be created by default.  (Passing
    one in is useful when several condition variables must share the
    same lock.)

    A condition variable has acquire() and release() methods that call
    the corresponding methods of the associated lock.

    It also has a wait() method, and notify() and notifyAll() methods.
    These three must only be called when the calling thread has
    acquired the lock.

    The wait() method releases the lock, and then blocks until it is
    awakened by a notifiy() or notifyAll() call for the same condition
    variable in another thread.  Once awakened, it re-acquires the
    lock and returns.  It is also possible to specify a timeout.

    The notify() method wakes up one of the threads waiting for the
    condition variable, if any are waiting.  The notifyAll() method
    wakes up all threads waiting for the condition variable.

    Note: the notify() and notifyAll() methods don't release the
    lock; this means that the thread or threads awakened will not
    return from their wait() call immediately, but only when the
    thread that called notify() or notifyAll() finally relinquishes
    ownership of the lock.

    Tip: the typical programming style using condition variables uses
    the lock to synchronize access to some shared state; threads that
    are interested in a particular change of state call wait()
    repeatedly until they see the desired state, while threads that
    modify the state call notify() or notifyAll() when they change the
    state in such a way that it could possibly be a desired state for
    one of the waiters.  For example, the following code is a generic
    producer-consumer situation with unlimited buffer capacity:

        # Consume one item
        cv.acquire()
        while not an_item_is_available():
            cv.wait()
        get_an_available_item()
        cv.release()

        # Produce one item
        cv.acquire()
        make_an_item_available()
        cv.notify()
        cv.release()

    To choose between notify() and notifyAll(), consider whether one
    state change can be interesting for only one or several waiting
    threads.  E.g. in a typical producer-consumer situation, adding
    one item to the buffer only needs to wake up one consumer thread.

    """

    def __init__(self, lock=None):
        """Constructor.

        If the lock argument is given and not None, it must be a Lock
        or RLock object, and it is used as the underlying lock.
        Otherwise, a new RLock object is created and used as the
        underlying lock.

        """

    def acquire(self, *args):
        """Acquire the underlying lock.

        This method calls the corresponding method on the underlying
        lock; the return value is whatever that method returns.

        """

    def release(self):
        """Release the underlying lock.

        This method calls the corresponding method on the underlying
        lock; there is no return value.

        """

    def wait(self, timeout=None):
        """Wait until notified or until a timeout occurs.

        This must only be called when the calling thread has acquired
        the lock.

        This method releases the underlying lock, and then blocks
        until it is awakened by a notify() or notifyAll() call for the
        same condition variable in another thread, or until the
        optional timeout occurs.  Once awakened or timed out, it
        re-acquires the lock and returns.

        When the timeout argument is present and not None, it should
        be a floating point number specifying a timeout for the
        operation in seconds (or fractions thereof).

        When the underlying lock is an RLock, it is not released using
        its release() method, since this may not actually unlock the
        lock when it was acquired() multiple times recursively.
        Instead, an internal interface of the RLock class is used,
        which really unlocks it even when it has been recursively
        acquired several times.  Another internal interface is then
        used to restore the recursion level when the lock is
        reacquired.

        """

    def notify(self):
        """Wake up a thread waiting on this condition, if any.

        This must only be called when the calling thread has acquired
        the lock.

        This method wakes up one of the threads waiting for the
        condition variable, if any are waiting; it is a no-op if no
        threads are waiting.

        The current implementation wakes up exactly one thread, if any
        are waiting.  However, it's not safe to rely on this behavior.
        A future, optimized implementation may occasionally wake up
        more than one thread.

        Note: the awakened thread does not actually return from its
        wait() call until it can reacquire the lock.  Since notify()
        does not release the lock, its caller should.

        """

    def notifyAll(self):
        """Wake up all threads waiting on this condition.

        This method acts like notify(), but wakes up all waiting
        threads instead of one.

        """


class Semaphore:
    """Semaphore object.

    This is one of the oldest synchronization primitives in the
    history of computer science, invented by the early Dutch computer
    scientist Edsger W. Dijkstra (he used P() and V() instead of
    acquire() and release()).

    A semaphore manages an internal counter which is decremented by
    each acquire() call and incremented by each release() call.  The
    counter can never go below zero; when acquire() finds that it is
    zero, it blocks, waiting until some other thread calls release().

    """

    def __init__(self, value=1):
        """Constructor.

        The optional argument gives the initial value for the internal
        counter; it defaults to 1.

        """

    def acquire(self, blocking=1):
        """Acquire a semaphore.

        When invoked without arguments: if the internal counter is
        larger than zero on entry, decrement it by one and return
        immediately.  If it is zero on entry, block, waiting until
        some other thread has called release() to make it larger than
        zero.  This is done with proper interlocking so that if
        multiple acquire() calls are blocked, release() will wake
        exactly one of them up.  The implementation may pick one at
        random, so the order in which blocked threads are awakened
        should not be relied on.  There is no return value in this
        case.

        When invoked with the 'blocking' argument set to true, do the
        same thing as when called without arguments, and return true.

        When invoked with the 'blocking' argument set to false, do not
        block.  If a call without argument would block, return false
        immediately; otherwise, do the same thing as when called
        without arguments, and return true.

        """

    def release(self):
        """Release a semaphore.

        Increment the internal counter by one.  When it was zero on
        entry and another thread is waiting for it to become larger
        than zero again, wake up that thread.

        """


class Event:
    """Event object.

    This is one of the simplest mechanisms for communication between
    threads: one thread signals an event and another thread, or
    threads, wait for it.

    An event object manages an internal flag that can be set to true
    with the set() method and reset to false with the clear() method.
    The wait() method blocks until the flag is true.

    """

    def __init__(self):
        """Constructor.

        The internal flag is initially false.

        """

    def isSet(self):
        """Return true iff the internal flag is true."""

    def set(self):
        """Set the internal flag to true.

        All threads waiting for it to become true are awakened.

        Threads that call wait() once the flag is true will not block
        at all.

        """

    def clear(self):
        """Reset the internal flag to false.

        Subsequently, threads calling wait() will block until set() is
        called to set the internal flag to true again.

        """

    def wait(self, timeout=None):
        """Block until the internal flag is true.

        If the internal flag is true on entry, return immediately.
        Otherwise, block until another thread calls set() to set the
        flag to true, or until the optional timeout occurs.

        When the timeout argument is present and not None, it should
        be a floating point number specifying a timeout for the
        operation in seconds (or fractions thereof).

        """


class Thread:
    """Thread class.

    *** ONLY OVERRIDE THE __init__() AND run() METHODS OF THIS CLASS ***

    This class represents an activity that is run in a separate thread
    of control.  There are two ways to specify the activity: by
    passing a callable object to the constructor, or by overriding the
    run() method in a subclass.  No other methods (except for the
    constructor) should be overridden in a subclass.

    Once a thread object is created, its activity must be started by
    calling the thread's start() method.  This invokes the run()
    method in a separate thread of control.

    Once the thread's activity is started, the thread is considered
    'alive' and 'active' (these concepts are almost, but not quite
    exactly, the same; their definition is intentionally somewhat
    vague).  It stops being alive and active when its run() method
    terminates -- either normally, or by raising an unhandled
    exception.  The isAlive() method tests whether the thread is
    alive.

    Other threads can call a thread's join() method.  This blocks the
    calling thread until the thread whose join() method is called
    is terminated.

    A thread has a name.  The name can be passed to the constructor,
    set with the setName() method, and retrieved with the getName()
    method.

    A thread can be flagged as a ``daemon thread''.  The significance
    of this flag is that the entire Python program exits when only
    daemon threads are left.  The initial value is inherited from the
    creating thread.  The flag can be set with the setDaemon() method
    and retrieved with the getDaemon() method.

    There is a ``main thread'' object; this corresponds to the
    initial thread of control in the Python program.  It is not a
    daemon thread.

    There is the possibility that ``dummy thread objects'' are
    created.  These are thread objects corresponding to ``alien
    threads''.  These are threads of control started outside the
    threading module, e.g. directly from C code.  Dummy thread objects
    have limited functionality; they are always considered alive,
    active, and daemonic, and cannot be join()ed.  They are never
    deleted, since it is impossible to detect the termination of alien
    threads.

    """

    def __init__(self, group=None, target=None, name=None,
                 args=(), kwargs={}):
        """Thread constructor.

        This constructor should always be called with keyword
        arguments.  Arguments are:

        group
            Should be None; reserved for future extension when a
            ThreadGroup class is implemented.

        target
            Callable object to be invoked by the run() method.
            Defaults to None, meaning nothing is called.

        name
            The thread name.  By default, a unique name is constructed
            of the form ``Thread-N'' where N is a small decimal
            number.

        args
            Argument tuple for the target invocation.  Defaults to ().

        kwargs
            Keyword argument dictionary for the target invocation.
            Defaults to {}.

        If the subclass overrides the constructor, it must make sure
        to invoke the base class constructor (Thread.__init__())
        before doing anything else to the thread.

        """

    def start(self):
        """Start the thread's activity.

        This must be called at most once per thread object.  It
        arranges for the object's run() method to be invoked in a
        separate thread of control.

        """

    def run(self):
        """Method representing the thread's activity.

        You may override this method in a subclass.  The standard
        run() method invokes the callable object passed as the
        'target' argument, if any, with sequential and keyword
        arguments taken from the 'args' and 'kwargs' arguments,
        respectively.

        """

    def join(self, timeout=None):
        """Wait until the thread terminates.

        This blocks the calling thread until the thread whose join()
        method is called terminates -- either normally or through an
        unhandled exception -- or until the optional timeout occurs.

        When the timeout argument is present and not None, it should
        be a floating point number specifying a timeout for the
        operation in seconds (or fractions thereof).

        A thread can be join()ed many times.

        A thread cannot join itself because this would cause a
        deadlock.

        It is an error to attempt to join() a thread before it has
        been started.

        """

    def getName(self):
        """Return the thread's name."""

    def setName(self, name):
        """Set the thread's name.

        The name is a string used for identification purposes only.
        It has no semantics.  Multiple threads may be given the same
        name.  The initial name is set by the constructor.

        """

    def isAlive(self):
        """Return whether the thread is alive.

        Roughly, a thread is alive from the moment the start() method
        returns until its run() method terminates.

        """

    def isDaemon(self):
        """Return the thread's daemon flag."""

    def setDaemon(self, daemonic):
        """Set the thread's daemon flag (a Boolean).

        This must be called before start() is called.

        The initial value is inherited from the creating thread.

        The entire Python program exits when no active non-daemon
        threads are left.

        """


# Module-level functions:


def currentThread():
    """Return the current Thread object.

    This function returns the Thread object corresponding to the
    caller's thread of control.

    If the caller's thread of control was not created through the
    threading module, a dummy thread object with limited functionality
    is returned.

    """


def activeCount():
    """Return the number of currently active Thread objects.

    The returned count is equal to the length of the list returned by
    enumerate().

    """


def enumerate():
    """Return a list of all currently active Thread objects.

    The list includes daemonic threads, dummy thread objects created
    by currentThread(), and the main thread.  It excludes terminated
    threads and threads that have not yet been started.

    """
