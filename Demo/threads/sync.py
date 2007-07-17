# Defines classes that provide synchronization objects.  Note that use of
# this module requires that your Python support threads.
#
#    condition(lock=None)       # a POSIX-like condition-variable object
#    barrier(n)                 # an n-thread barrier
#    event()                    # an event object
#    semaphore(n=1)             # a semaphore object, with initial count n
#    mrsw()                     # a multiple-reader single-writer lock
#
# CONDITIONS
#
# A condition object is created via
#   import this_module
#   your_condition_object = this_module.condition(lock=None)
#
# As explained below, a condition object has a lock associated with it,
# used in the protocol to protect condition data.  You can specify a
# lock to use in the constructor, else the constructor will allocate
# an anonymous lock for you.  Specifying a lock explicitly can be useful
# when more than one condition keys off the same set of shared data.
#
# Methods:
#   .acquire()
#      acquire the lock associated with the condition
#   .release()
#      release the lock associated with the condition
#   .wait()
#      block the thread until such time as some other thread does a
#      .signal or .broadcast on the same condition, and release the
#      lock associated with the condition.  The lock associated with
#      the condition MUST be in the acquired state at the time
#      .wait is invoked.
#   .signal()
#      wake up exactly one thread (if any) that previously did a .wait
#      on the condition; that thread will awaken with the lock associated
#      with the condition in the acquired state.  If no threads are
#      .wait'ing, this is a nop.  If more than one thread is .wait'ing on
#      the condition, any of them may be awakened.
#   .broadcast()
#      wake up all threads (if any) that are .wait'ing on the condition;
#      the threads are woken up serially, each with the lock in the
#      acquired state, so should .release() as soon as possible.  If no
#      threads are .wait'ing, this is a nop.
#
#      Note that if a thread does a .wait *while* a signal/broadcast is
#      in progress, it's guaranteeed to block until a subsequent
#      signal/broadcast.
#
#      Secret feature:  `broadcast' actually takes an integer argument,
#      and will wake up exactly that many waiting threads (or the total
#      number waiting, if that's less).  Use of this is dubious, though,
#      and probably won't be supported if this form of condition is
#      reimplemented in C.
#
# DIFFERENCES FROM POSIX
#
# + A separate mutex is not needed to guard condition data.  Instead, a
#   condition object can (must) be .acquire'ed and .release'ed directly.
#   This eliminates a common error in using POSIX conditions.
#
# + Because of implementation difficulties, a POSIX `signal' wakes up
#   _at least_ one .wait'ing thread.  Race conditions make it difficult
#   to stop that.  This implementation guarantees to wake up only one,
#   but you probably shouldn't rely on that.
#
# PROTOCOL
#
# Condition objects are used to block threads until "some condition" is
# true.  E.g., a thread may wish to wait until a producer pumps out data
# for it to consume, or a server may wish to wait until someone requests
# its services, or perhaps a whole bunch of threads want to wait until a
# preceding pass over the data is complete.  Early models for conditions
# relied on some other thread figuring out when a blocked thread's
# condition was true, and made the other thread responsible both for
# waking up the blocked thread and guaranteeing that it woke up with all
# data in a correct state.  This proved to be very delicate in practice,
# and gave conditions a bad name in some circles.
#
# The POSIX model addresses these problems by making a thread responsible
# for ensuring that its own state is correct when it wakes, and relies
# on a rigid protocol to make this easy; so long as you stick to the
# protocol, POSIX conditions are easy to "get right":
#
#  A) The thread that's waiting for some arbitrarily-complex condition
#     (ACC) to become true does:
#
#     condition.acquire()
#     while not (code to evaluate the ACC):
#           condition.wait()
#           # That blocks the thread, *and* releases the lock.  When a
#           # condition.signal() happens, it will wake up some thread that
#           # did a .wait, *and* acquire the lock again before .wait
#           # returns.
#           #
#           # Because the lock is acquired at this point, the state used
#           # in evaluating the ACC is frozen, so it's safe to go back &
#           # reevaluate the ACC.
#
#     # At this point, ACC is true, and the thread has the condition
#     # locked.
#     # So code here can safely muck with the shared state that
#     # went into evaluating the ACC -- if it wants to.
#     # When done mucking with the shared state, do
#     condition.release()
#
#  B) Threads that are mucking with shared state that may affect the
#     ACC do:
#
#     condition.acquire()
#     # muck with shared state
#     condition.release()
#     if it's possible that ACC is true now:
#         condition.signal() # or .broadcast()
#
#     Note:  You may prefer to put the "if" clause before the release().
#     That's fine, but do note that anyone waiting on the signal will
#     stay blocked until the release() is done (since acquiring the
#     condition is part of what .wait() does before it returns).
#
# TRICK OF THE TRADE
#
# With simpler forms of conditions, it can be impossible to know when
# a thread that's supposed to do a .wait has actually done it.  But
# because this form of condition releases a lock as _part_ of doing a
# wait, the state of that lock can be used to guarantee it.
#
# E.g., suppose thread A spawns thread B and later wants to wait for B to
# complete:
#
# In A:                             In B:
#
# B_done = condition()              ... do work ...
# B_done.acquire()                  B_done.acquire(); B_done.release()
# spawn B                           B_done.signal()
# ... some time later ...           ... and B exits ...
# B_done.wait()
#
# Because B_done was in the acquire'd state at the time B was spawned,
# B's attempt to acquire B_done can't succeed until A has done its
# B_done.wait() (which releases B_done).  So B's B_done.signal() is
# guaranteed to be seen by the .wait().  Without the lock trick, B
# may signal before A .waits, and then A would wait forever.
#
# BARRIERS
#
# A barrier object is created via
#   import this_module
#   your_barrier = this_module.barrier(num_threads)
#
# Methods:
#   .enter()
#      the thread blocks until num_threads threads in all have done
#      .enter().  Then the num_threads threads that .enter'ed resume,
#      and the barrier resets to capture the next num_threads threads
#      that .enter it.
#
# EVENTS
#
# An event object is created via
#   import this_module
#   your_event = this_module.event()
#
# An event has two states, `posted' and `cleared'.  An event is
# created in the cleared state.
#
# Methods:
#
#   .post()
#      Put the event in the posted state, and resume all threads
#      .wait'ing on the event (if any).
#
#   .clear()
#      Put the event in the cleared state.
#
#   .is_posted()
#      Returns 0 if the event is in the cleared state, or 1 if the event
#      is in the posted state.
#
#   .wait()
#      If the event is in the posted state, returns immediately.
#      If the event is in the cleared state, blocks the calling thread
#      until the event is .post'ed by another thread.
#
# Note that an event, once posted, remains posted until explicitly
# cleared.  Relative to conditions, this is both the strength & weakness
# of events.  It's a strength because the .post'ing thread doesn't have to
# worry about whether the threads it's trying to communicate with have
# already done a .wait (a condition .signal is seen only by threads that
# do a .wait _prior_ to the .signal; a .signal does not persist).  But
# it's a weakness because .clear'ing an event is error-prone:  it's easy
# to mistakenly .clear an event before all the threads you intended to
# see the event get around to .wait'ing on it.  But so long as you don't
# need to .clear an event, events are easy to use safely.
#
# SEMAPHORES
#
# A semaphore object is created via
#   import this_module
#   your_semaphore = this_module.semaphore(count=1)
#
# A semaphore has an integer count associated with it.  The initial value
# of the count is specified by the optional argument (which defaults to
# 1) passed to the semaphore constructor.
#
# Methods:
#
#   .p()
#      If the semaphore's count is greater than 0, decrements the count
#      by 1 and returns.
#      Else if the semaphore's count is 0, blocks the calling thread
#      until a subsequent .v() increases the count.  When that happens,
#      the count will be decremented by 1 and the calling thread resumed.
#
#   .v()
#      Increments the semaphore's count by 1, and wakes up a thread (if
#      any) blocked by a .p().  It's an (detected) error for a .v() to
#      increase the semaphore's count to a value larger than the initial
#      count.
#
# MULTIPLE-READER SINGLE-WRITER LOCKS
#
# A mrsw lock is created via
#   import this_module
#   your_mrsw_lock = this_module.mrsw()
#
# This kind of lock is often useful with complex shared data structures.
# The object lets any number of "readers" proceed, so long as no thread
# wishes to "write".  When a (one or more) thread declares its intention
# to "write" (e.g., to update a shared structure), all current readers
# are allowed to finish, and then a writer gets exclusive access; all
# other readers & writers are blocked until the current writer completes.
# Finally, if some thread is waiting to write and another is waiting to
# read, the writer takes precedence.
#
# Methods:
#
#   .read_in()
#      If no thread is writing or waiting to write, returns immediately.
#      Else blocks until no thread is writing or waiting to write.  So
#      long as some thread has completed a .read_in but not a .read_out,
#      writers are blocked.
#
#   .read_out()
#      Use sometime after a .read_in to declare that the thread is done
#      reading.  When all threads complete reading, a writer can proceed.
#
#   .write_in()
#      If no thread is writing (has completed a .write_in, but hasn't yet
#      done a .write_out) or reading (similarly), returns immediately.
#      Else blocks the calling thread, and threads waiting to read, until
#      the current writer completes writing or all the current readers
#      complete reading; if then more than one thread is waiting to
#      write, one of them is allowed to proceed, but which one is not
#      specified.
#
#   .write_out()
#      Use sometime after a .write_in to declare that the thread is done
#      writing.  Then if some other thread is waiting to write, it's
#      allowed to proceed.  Else all threads (if any) waiting to read are
#      allowed to proceed.
#
#   .write_to_read()
#      Use instead of a .write_in to declare that the thread is done
#      writing but wants to continue reading without other writers
#      intervening.  If there are other threads waiting to write, they
#      are allowed to proceed only if the current thread calls
#      .read_out; threads waiting to read are only allowed to proceed
#      if there are are no threads waiting to write.  (This is a
#      weakness of the interface!)

import thread

class condition:
    def __init__(self, lock=None):
        # the lock actually used by .acquire() and .release()
        if lock is None:
            self.mutex = thread.allocate_lock()
        else:
            if hasattr(lock, 'acquire') and \
               hasattr(lock, 'release'):
                self.mutex = lock
            else:
                raise TypeError('condition constructor requires ' \
                                 'a lock argument')

        # lock used to block threads until a signal
        self.checkout = thread.allocate_lock()
        self.checkout.acquire()

        # internal critical-section lock, & the data it protects
        self.idlock = thread.allocate_lock()
        self.id = 0
        self.waiting = 0  # num waiters subject to current release
        self.pending = 0  # num waiters awaiting next signal
        self.torelease = 0      # num waiters to release
        self.releasing = 0      # 1 iff release is in progress

    def acquire(self):
        self.mutex.acquire()

    def release(self):
        self.mutex.release()

    def wait(self):
        mutex, checkout, idlock = self.mutex, self.checkout, self.idlock
        if not mutex.locked():
            raise ValueError("condition must be .acquire'd when .wait() invoked")

        idlock.acquire()
        myid = self.id
        self.pending = self.pending + 1
        idlock.release()

        mutex.release()

        while 1:
            checkout.acquire(); idlock.acquire()
            if myid < self.id:
                break
            checkout.release(); idlock.release()

        self.waiting = self.waiting - 1
        self.torelease = self.torelease - 1
        if self.torelease:
            checkout.release()
        else:
            self.releasing = 0
            if self.waiting == self.pending == 0:
                self.id = 0
        idlock.release()
        mutex.acquire()

    def signal(self):
        self.broadcast(1)

    def broadcast(self, num = -1):
        if num < -1:
            raise ValueError('.broadcast called with num %r' % (num,))
        if num == 0:
            return
        self.idlock.acquire()
        if self.pending:
            self.waiting = self.waiting + self.pending
            self.pending = 0
            self.id = self.id + 1
        if num == -1:
            self.torelease = self.waiting
        else:
            self.torelease = min( self.waiting,
                                  self.torelease + num )
        if self.torelease and not self.releasing:
            self.releasing = 1
            self.checkout.release()
        self.idlock.release()

class barrier:
    def __init__(self, n):
        self.n = n
        self.togo = n
        self.full = condition()

    def enter(self):
        full = self.full
        full.acquire()
        self.togo = self.togo - 1
        if self.togo:
            full.wait()
        else:
            self.togo = self.n
            full.broadcast()
        full.release()

class event:
    def __init__(self):
        self.state  = 0
        self.posted = condition()

    def post(self):
        self.posted.acquire()
        self.state = 1
        self.posted.broadcast()
        self.posted.release()

    def clear(self):
        self.posted.acquire()
        self.state = 0
        self.posted.release()

    def is_posted(self):
        self.posted.acquire()
        answer = self.state
        self.posted.release()
        return answer

    def wait(self):
        self.posted.acquire()
        if not self.state:
            self.posted.wait()
        self.posted.release()

class semaphore:
    def __init__(self, count=1):
        if count <= 0:
            raise ValueError('semaphore count %d; must be >= 1' % count)
        self.count = count
        self.maxcount = count
        self.nonzero = condition()

    def p(self):
        self.nonzero.acquire()
        while self.count == 0:
            self.nonzero.wait()
        self.count = self.count - 1
        self.nonzero.release()

    def v(self):
        self.nonzero.acquire()
        if self.count == self.maxcount:
            raise ValueError('.v() tried to raise semaphore count above ' \
                  'initial value %r' % self.maxcount)
        self.count = self.count + 1
        self.nonzero.signal()
        self.nonzero.release()

class mrsw:
    def __init__(self):
        # critical-section lock & the data it protects
        self.rwOK = thread.allocate_lock()
        self.nr = 0  # number readers actively reading (not just waiting)
        self.nw = 0  # number writers either waiting to write or writing
        self.writing = 0  # 1 iff some thread is writing

        # conditions
        self.readOK  = condition(self.rwOK)  # OK to unblock readers
        self.writeOK = condition(self.rwOK)  # OK to unblock writers

    def read_in(self):
        self.rwOK.acquire()
        while self.nw:
            self.readOK.wait()
        self.nr = self.nr + 1
        self.rwOK.release()

    def read_out(self):
        self.rwOK.acquire()
        if self.nr <= 0:
            raise ValueError('.read_out() invoked without an active reader')
        self.nr = self.nr - 1
        if self.nr == 0:
            self.writeOK.signal()
        self.rwOK.release()

    def write_in(self):
        self.rwOK.acquire()
        self.nw = self.nw + 1
        while self.writing or self.nr:
            self.writeOK.wait()
        self.writing = 1
        self.rwOK.release()

    def write_out(self):
        self.rwOK.acquire()
        if not self.writing:
            raise ValueError('.write_out() invoked without an active writer')
        self.writing = 0
        self.nw = self.nw - 1
        if self.nw:
            self.writeOK.signal()
        else:
            self.readOK.broadcast()
        self.rwOK.release()

    def write_to_read(self):
        self.rwOK.acquire()
        if not self.writing:
            raise ValueError('.write_to_read() invoked without an active writer')
        self.writing = 0
        self.nw = self.nw - 1
        self.nr = self.nr + 1
        if not self.nw:
            self.readOK.broadcast()
        self.rwOK.release()

# The rest of the file is a test case, that runs a number of parallelized
# quicksorts in parallel.  If it works, you'll get about 600 lines of
# tracing output, with a line like
#     test passed! 209 threads created in all
# as the last line.  The content and order of preceding lines will
# vary across runs.

def _new_thread(func, *args):
    global TID
    tid.acquire(); id = TID = TID+1; tid.release()
    io.acquire(); alive.append(id); \
                  print('starting thread', id, '--', len(alive), 'alive'); \
                  io.release()
    thread.start_new_thread( func, (id,) + args )

def _qsort(tid, a, l, r, finished):
    # sort a[l:r]; post finished when done
    io.acquire(); print('thread', tid, 'qsort', l, r); io.release()
    if r-l > 1:
        pivot = a[l]
        j = l+1   # make a[l:j] <= pivot, and a[j:r] > pivot
        for i in range(j, r):
            if a[i] <= pivot:
                a[j], a[i] = a[i], a[j]
                j = j + 1
        a[l], a[j-1] = a[j-1], pivot

        l_subarray_sorted = event()
        r_subarray_sorted = event()
        _new_thread(_qsort, a, l, j-1, l_subarray_sorted)
        _new_thread(_qsort, a, j, r,   r_subarray_sorted)
        l_subarray_sorted.wait()
        r_subarray_sorted.wait()

    io.acquire(); print('thread', tid, 'qsort done'); \
                  alive.remove(tid); io.release()
    finished.post()

def _randarray(tid, a, finished):
    io.acquire(); print('thread', tid, 'randomizing array'); \
                  io.release()
    for i in range(1, len(a)):
        wh.acquire(); j = randint(0,i); wh.release()
        a[i], a[j] = a[j], a[i]
    io.acquire(); print('thread', tid, 'randomizing done'); \
                  alive.remove(tid); io.release()
    finished.post()

def _check_sort(a):
    if a != range(len(a)):
        raise ValueError('a not sorted', a)

def _run_one_sort(tid, a, bar, done):
    # randomize a, and quicksort it
    # for variety, all the threads running this enter a barrier
    # at the end, and post `done' after the barrier exits
    io.acquire(); print('thread', tid, 'randomizing', a); \
                  io.release()
    finished = event()
    _new_thread(_randarray, a, finished)
    finished.wait()

    io.acquire(); print('thread', tid, 'sorting', a); io.release()
    finished.clear()
    _new_thread(_qsort, a, 0, len(a), finished)
    finished.wait()
    _check_sort(a)

    io.acquire(); print('thread', tid, 'entering barrier'); \
                  io.release()
    bar.enter()
    io.acquire(); print('thread', tid, 'leaving barrier'); \
                  io.release()
    io.acquire(); alive.remove(tid); io.release()
    bar.enter() # make sure they've all removed themselves from alive
                ##  before 'done' is posted
    bar.enter() # just to be cruel
    done.post()

def test():
    global TID, tid, io, wh, randint, alive
    import random
    randint = random.randint

    TID = 0                             # thread ID (1, 2, ...)
    tid = thread.allocate_lock()        # for changing TID
    io  = thread.allocate_lock()        # for printing, and 'alive'
    wh  = thread.allocate_lock()        # for calls to random
    alive = []                          # IDs of active threads

    NSORTS = 5
    arrays = []
    for i in range(NSORTS):
        arrays.append( range( (i+1)*10 ) )

    bar = barrier(NSORTS)
    finished = event()
    for i in range(NSORTS):
        _new_thread(_run_one_sort, arrays[i], bar, finished)
    finished.wait()

    print('all threads done, and checking results ...')
    if alive:
        raise ValueError('threads still alive at end', alive)
    for i in range(NSORTS):
        a = arrays[i]
        if len(a) != (i+1)*10:
            raise ValueError('length of array', i, 'screwed up')
        _check_sort(a)

    print('test passed!', TID, 'threads created in all')

if __name__ == '__main__':
    test()

# end of module
