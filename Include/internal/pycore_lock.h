// Lightweight locks and other synchronization mechanisms.
//
// These implementations are based on WebKit's WTF::Lock. See
// https://webkit.org/blog/6161/locking-in-webkit/ for a description of the
// design.
#ifndef Py_INTERNAL_LOCK_H
#define Py_INTERNAL_LOCK_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

//_Py_UNLOCKED is defined as 0 and _Py_LOCKED as 1 in Include/cpython/lock.h
#define _Py_HAS_PARKED  2
#define _Py_ONCE_INITIALIZED 4

static inline int
PyMutex_LockFast(uint8_t *lock_bits)
{
    uint8_t expected = _Py_UNLOCKED;
    return _Py_atomic_compare_exchange_uint8(lock_bits, &expected, _Py_LOCKED);
}

// Checks if the mutex is currently locked.
static inline int
PyMutex_IsLocked(PyMutex *m)
{
    return (_Py_atomic_load_uint8(&m->_bits) & _Py_LOCKED) != 0;
}

// Re-initializes the mutex after a fork to the unlocked state.
static inline void
_PyMutex_at_fork_reinit(PyMutex *m)
{
    memset(m, 0, sizeof(*m));
}

typedef enum _PyLockFlags {
    // Do not detach/release the GIL when waiting on the lock.
    _Py_LOCK_DONT_DETACH = 0,

    // Detach/release the GIL while waiting on the lock.
    _PY_LOCK_DETACH = 1,

    // Handle signals if interrupted while waiting on the lock.
    _PY_LOCK_HANDLE_SIGNALS = 2,
} _PyLockFlags;

// Lock a mutex with an optional timeout and additional options. See
// _PyLockFlags for details.
extern PyLockStatus
_PyMutex_LockTimed(PyMutex *m, PyTime_t timeout_ns, _PyLockFlags flags);

// Lock a mutex with aditional options. See _PyLockFlags for details.
static inline void
PyMutex_LockFlags(PyMutex *m, _PyLockFlags flags)
{
    uint8_t expected = _Py_UNLOCKED;
    if (!_Py_atomic_compare_exchange_uint8(&m->_bits, &expected, _Py_LOCKED)) {
        _PyMutex_LockTimed(m, -1, flags);
    }
}

// Unlock a mutex, returns 0 if the mutex is not locked (used for improved
// error messages).
extern int _PyMutex_TryUnlock(PyMutex *m);


// PyEvent is a one-time event notification
typedef struct {
    uint8_t v;
} PyEvent;

// Check if the event is set without blocking. Returns 1 if the event is set or
// 0 otherwise.
PyAPI_FUNC(int) _PyEvent_IsSet(PyEvent *evt);

// Set the event and notify any waiting threads.
// Export for '_testinternalcapi' shared extension
PyAPI_FUNC(void) _PyEvent_Notify(PyEvent *evt);

// Wait for the event to be set. If the event is already set, then this returns
// immediately.
PyAPI_FUNC(void) PyEvent_Wait(PyEvent *evt);

// Wait for the event to be set, or until the timeout expires. If the event is
// already set, then this returns immediately. Returns 1 if the event was set,
// and 0 if the timeout expired or thread was interrupted. If `detach` is
// true, then the thread will detach/release the GIL while waiting.
PyAPI_FUNC(int)
PyEvent_WaitTimed(PyEvent *evt, PyTime_t timeout_ns, int detach);

// _PyRawMutex implements a word-sized mutex that that does not depend on the
// parking lot API, and therefore can be used in the parking lot
// implementation.
//
// The mutex uses a packed representation: the least significant bit is used to
// indicate whether the mutex is locked or not. The remaining bits are either
// zero or a pointer to a `struct raw_mutex_entry` (see lock.c).
typedef struct {
    uintptr_t v;
} _PyRawMutex;

// Slow paths for lock/unlock
extern void _PyRawMutex_LockSlow(_PyRawMutex *m);
extern void _PyRawMutex_UnlockSlow(_PyRawMutex *m);

static inline void
_PyRawMutex_Lock(_PyRawMutex *m)
{
    uintptr_t unlocked = _Py_UNLOCKED;
    if (_Py_atomic_compare_exchange_uintptr(&m->v, &unlocked, _Py_LOCKED)) {
        return;
    }
    _PyRawMutex_LockSlow(m);
}

static inline void
_PyRawMutex_Unlock(_PyRawMutex *m)
{
    uintptr_t locked = _Py_LOCKED;
    if (_Py_atomic_compare_exchange_uintptr(&m->v, &locked, _Py_UNLOCKED)) {
        return;
    }
    _PyRawMutex_UnlockSlow(m);
}

// Type signature for one-time initialization functions. The function should
// return 0 on success and -1 on failure.
typedef int _Py_once_fn_t(void *arg);

// (private) slow path for one time initialization
PyAPI_FUNC(int)
_PyOnceFlag_CallOnceSlow(_PyOnceFlag *flag, _Py_once_fn_t *fn, void *arg);

// Calls `fn` once using `flag`. The `arg` is passed to the call to `fn`.
//
// Returns 0 on success and -1 on failure.
//
// If `fn` returns 0 (success), then subsequent calls immediately return 0.
// If `fn` returns -1 (failure), then subsequent calls will retry the call.
static inline int
_PyOnceFlag_CallOnce(_PyOnceFlag *flag, _Py_once_fn_t *fn, void *arg)
{
    if (_Py_atomic_load_uint8(&flag->v) == _Py_ONCE_INITIALIZED) {
        return 0;
    }
    return _PyOnceFlag_CallOnceSlow(flag, fn, arg);
}

// A recursive mutex. The mutex should zero-initialized.
typedef struct {
    PyMutex mutex;
    unsigned long long thread;  // i.e., PyThread_get_thread_ident_ex()
    size_t level;
} _PyRecursiveMutex;

PyAPI_FUNC(int) _PyRecursiveMutex_IsLockedByCurrentThread(_PyRecursiveMutex *m);
PyAPI_FUNC(void) _PyRecursiveMutex_Lock(_PyRecursiveMutex *m);
PyAPI_FUNC(void) _PyRecursiveMutex_Unlock(_PyRecursiveMutex *m);


// A readers-writer (RW) lock. The lock supports multiple concurrent readers or
// a single writer. The lock is write-preferring: if a writer is waiting while
// the lock is read-locked then, new readers will be blocked. This avoids
// starvation of writers.
//
// In C++, the equivalent synchronization primitive is std::shared_mutex
// with shared ("read") and exclusive ("write") locking.
//
// The two least significant bits are used to indicate if the lock is
// write-locked and if there are parked threads (either readers or writers)
// waiting to acquire the lock. The remaining bits are used to indicate the
// number of readers holding the lock.
//
// 0b000..00000: unlocked
// 0bnnn..nnn00: nnn..nnn readers holding the lock
// 0bnnn..nnn10: nnn..nnn readers holding the lock and a writer is waiting
// 0b00000..010: unlocked with awoken writer about to acquire lock
// 0b00000..001: write-locked
// 0b00000..011: write-locked and readers or other writers are waiting
//
// Note that reader_count must be zero if the lock is held by a writer, and
// vice versa. The lock can only be held by readers or a writer, but not both.
//
// The design is optimized for simplicity of the implementation. The lock is
// not fair: if fairness is desired, use an additional PyMutex to serialize
// writers. The lock is also not reentrant.
typedef struct {
    uintptr_t bits;
} _PyRWMutex;

// Read lock (i.e., shared lock)
PyAPI_FUNC(void) _PyRWMutex_RLock(_PyRWMutex *rwmutex);
PyAPI_FUNC(void) _PyRWMutex_RUnlock(_PyRWMutex *rwmutex);

// Write lock (i.e., exclusive lock)
PyAPI_FUNC(void) _PyRWMutex_Lock(_PyRWMutex *rwmutex);
PyAPI_FUNC(void) _PyRWMutex_Unlock(_PyRWMutex *rwmutex);

// Similar to linux seqlock: https://en.wikipedia.org/wiki/Seqlock
// We use a sequence number to lock the writer, an even sequence means we're unlocked, an odd
// sequence means we're locked.  Readers will read the sequence before attempting to read the
// underlying data and then read the sequence number again after reading the data.  If the
// sequence has not changed the data is valid.
//
// Differs a little bit in that we use CAS on sequence as the lock, instead of a separate spin lock.
// The writer can also detect that the undelering data has not changed and abandon the write
// and restore the previous sequence.
typedef struct {
    uint32_t sequence;
} _PySeqLock;

// Lock the sequence lock for the writer
PyAPI_FUNC(void) _PySeqLock_LockWrite(_PySeqLock *seqlock);

// Unlock the sequence lock and move to the next sequence number.
PyAPI_FUNC(void) _PySeqLock_UnlockWrite(_PySeqLock *seqlock);

// Abandon the current update indicating that no mutations have occurred
// and restore the previous sequence value.
PyAPI_FUNC(void) _PySeqLock_AbandonWrite(_PySeqLock *seqlock);

// Begin a read operation and return the current sequence number.
PyAPI_FUNC(uint32_t) _PySeqLock_BeginRead(_PySeqLock *seqlock);

// End the read operation and confirm that the sequence number has not changed.
// Returns 1 if the read was successful or 0 if the read should be retried.
PyAPI_FUNC(int) _PySeqLock_EndRead(_PySeqLock *seqlock, uint32_t previous);

// Check if the lock was held during a fork and clear the lock.  Returns 1
// if the lock was held and any associated data should be cleared.
PyAPI_FUNC(int) _PySeqLock_AfterFork(_PySeqLock *seqlock);

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_LOCK_H */
