// Lock implementation

#include "Python.h"

#include "pycore_lock.h"
#include "pycore_parking_lot.h"
#include "pycore_semaphore.h"

#ifdef MS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>        // SwitchToThread()
#elif defined(HAVE_SCHED_H)
#include <sched.h>          // sched_yield()
#endif

// If a thread waits on a lock for longer than TIME_TO_BE_FAIR_NS (1 ms), then
// the unlocking thread directly hands off ownership of the lock. This avoids
// starvation.
static const _PyTime_t TIME_TO_BE_FAIR_NS = 1000*1000;

// Spin for a bit before parking the thread. This is only enabled for
// `--disable-gil` builds because it is unlikely to be helpful if the GIL is
// enabled.
#if Py_GIL_DISABLED
static const int MAX_SPIN_COUNT = 40;
#else
static const int MAX_SPIN_COUNT = 0;
#endif

struct mutex_entry {
    // The time after which the unlocking thread should hand off lock ownership
    // directly to the waiting thread. Written by the waiting thread.
    _PyTime_t time_to_be_fair;

    // Set to 1 if the lock was handed off. Written by the unlocking thread.
    int handed_off;
};

static void
_Py_yield(void)
{
#ifdef MS_WINDOWS
    SwitchToThread();
#elif defined(HAVE_SCHED_H)
    sched_yield();
#endif
}

void
_PyMutex_LockSlow(PyMutex *m)
{
    _PyMutex_LockTimed(m, -1, _PY_LOCK_DETACH);
}

PyLockStatus
_PyMutex_LockTimed(PyMutex *m, _PyTime_t timeout, _PyLockFlags flags)
{
    uint8_t v = _Py_atomic_load_uint8_relaxed(&m->v);
    if ((v & _Py_LOCKED) == 0) {
        if (_Py_atomic_compare_exchange_uint8(&m->v, &v, v|_Py_LOCKED)) {
            return PY_LOCK_ACQUIRED;
        }
    }
    else if (timeout == 0) {
        return PY_LOCK_FAILURE;
    }

    _PyTime_t now = _PyTime_GetMonotonicClock();
    _PyTime_t endtime = 0;
    if (timeout > 0) {
        endtime = _PyTime_Add(now, timeout);
    }

    struct mutex_entry entry = {
        .time_to_be_fair = now + TIME_TO_BE_FAIR_NS,
        .handed_off = 0,
    };

    Py_ssize_t spin_count = 0;
    for (;;) {
        if ((v & _Py_LOCKED) == 0) {
            // The lock is unlocked. Try to grab it.
            if (_Py_atomic_compare_exchange_uint8(&m->v, &v, v|_Py_LOCKED)) {
                return PY_LOCK_ACQUIRED;
            }
            continue;
        }

        if (!(v & _Py_HAS_PARKED) && spin_count < MAX_SPIN_COUNT) {
            // Spin for a bit.
            _Py_yield();
            spin_count++;
            continue;
        }

        if (timeout == 0) {
            return PY_LOCK_FAILURE;
        }

        uint8_t newv = v;
        if (!(v & _Py_HAS_PARKED)) {
            // We are the first waiter. Set the _Py_HAS_PARKED flag.
            newv = v | _Py_HAS_PARKED;
            if (!_Py_atomic_compare_exchange_uint8(&m->v, &v, newv)) {
                continue;
            }
        }

        int ret = _PyParkingLot_Park(&m->v, &newv, sizeof(newv), timeout,
                                     &entry, (flags & _PY_LOCK_DETACH) != 0);
        if (ret == Py_PARK_OK) {
            if (entry.handed_off) {
                // We own the lock now.
                assert(_Py_atomic_load_uint8_relaxed(&m->v) & _Py_LOCKED);
                return PY_LOCK_ACQUIRED;
            }
        }
        else if (ret == Py_PARK_INTR && (flags & _PY_LOCK_HANDLE_SIGNALS)) {
            if (Py_MakePendingCalls() < 0) {
                return PY_LOCK_INTR;
            }
        }
        else if (ret == Py_PARK_TIMEOUT) {
            assert(timeout >= 0);
            return PY_LOCK_FAILURE;
        }

        if (timeout > 0) {
            timeout = _PyDeadline_Get(endtime);
            if (timeout <= 0) {
                // Avoid negative values because those mean block forever.
                timeout = 0;
            }
        }

        v = _Py_atomic_load_uint8_relaxed(&m->v);
    }
}

static void
mutex_unpark(PyMutex *m, struct mutex_entry *entry, int has_more_waiters)
{
    uint8_t v = 0;
    if (entry) {
        _PyTime_t now = _PyTime_GetMonotonicClock();
        int should_be_fair = now > entry->time_to_be_fair;

        entry->handed_off = should_be_fair;
        if (should_be_fair) {
            v |= _Py_LOCKED;
        }
        if (has_more_waiters) {
            v |= _Py_HAS_PARKED;
        }
    }
    _Py_atomic_store_uint8(&m->v, v);
}

int
_PyMutex_TryUnlock(PyMutex *m)
{
    uint8_t v = _Py_atomic_load_uint8(&m->v);
    for (;;) {
        if ((v & _Py_LOCKED) == 0) {
            // error: the mutex is not locked
            return -1;
        }
        else if ((v & _Py_HAS_PARKED)) {
            // wake up a single thread
            _PyParkingLot_Unpark(&m->v, (_Py_unpark_fn_t *)mutex_unpark, m);
            return 0;
        }
        else if (_Py_atomic_compare_exchange_uint8(&m->v, &v, _Py_UNLOCKED)) {
            // fast-path: no waiters
            return 0;
        }
    }
}

void
_PyMutex_UnlockSlow(PyMutex *m)
{
    if (_PyMutex_TryUnlock(m) < 0) {
        Py_FatalError("unlocking mutex that is not locked");
    }
}

// _PyRawMutex stores a linked list of `struct raw_mutex_entry`, one for each
// thread waiting on the mutex, directly in the mutex itself.
struct raw_mutex_entry {
    struct raw_mutex_entry *next;
    _PySemaphore sema;
};

void
_PyRawMutex_LockSlow(_PyRawMutex *m)
{
    struct raw_mutex_entry waiter;
    _PySemaphore_Init(&waiter.sema);

    uintptr_t v = _Py_atomic_load_uintptr(&m->v);
    for (;;) {
        if ((v & _Py_LOCKED) == 0) {
            // Unlocked: try to grab it (even if it has a waiter).
            if (_Py_atomic_compare_exchange_uintptr(&m->v, &v, v|_Py_LOCKED)) {
                break;
            }
            continue;
        }

        // Locked: try to add ourselves as a waiter.
        waiter.next = (struct raw_mutex_entry *)(v & ~1);
        uintptr_t desired = ((uintptr_t)&waiter)|_Py_LOCKED;
        if (!_Py_atomic_compare_exchange_uintptr(&m->v, &v, desired)) {
            continue;
        }

        // Wait for us to be woken up. Note that we still have to lock the
        // mutex ourselves: it is NOT handed off to us.
        _PySemaphore_Wait(&waiter.sema, -1, /*detach=*/0);
    }

    _PySemaphore_Destroy(&waiter.sema);
}

void
_PyRawMutex_UnlockSlow(_PyRawMutex *m)
{
    uintptr_t v = _Py_atomic_load_uintptr(&m->v);
    for (;;) {
        if ((v & _Py_LOCKED) == 0) {
            Py_FatalError("unlocking mutex that is not locked");
        }

        struct raw_mutex_entry *waiter = (struct raw_mutex_entry *)(v & ~1);
        if (waiter) {
            uintptr_t next_waiter = (uintptr_t)waiter->next;
            if (_Py_atomic_compare_exchange_uintptr(&m->v, &v, next_waiter)) {
                _PySemaphore_Wakeup(&waiter->sema);
                return;
            }
        }
        else {
            if (_Py_atomic_compare_exchange_uintptr(&m->v, &v, _Py_UNLOCKED)) {
                return;
            }
        }
    }
}

void
_PyEvent_Notify(PyEvent *evt)
{
    uintptr_t v = _Py_atomic_exchange_uint8(&evt->v, _Py_LOCKED);
    if (v == _Py_UNLOCKED) {
        // no waiters
        return;
    }
    else if (v == _Py_LOCKED) {
        // event already set
        return;
    }
    else {
        assert(v == _Py_HAS_PARKED);
        _PyParkingLot_UnparkAll(&evt->v);
    }
}

void
PyEvent_Wait(PyEvent *evt)
{
    while (!PyEvent_WaitTimed(evt, -1))
        ;
}

int
PyEvent_WaitTimed(PyEvent *evt, _PyTime_t timeout_ns)
{
    for (;;) {
        uint8_t v = _Py_atomic_load_uint8(&evt->v);
        if (v == _Py_LOCKED) {
            // event already set
            return 1;
        }
        if (v == _Py_UNLOCKED) {
            if (!_Py_atomic_compare_exchange_uint8(&evt->v, &v, _Py_HAS_PARKED)) {
                continue;
            }
        }

        uint8_t expected = _Py_HAS_PARKED;
        (void) _PyParkingLot_Park(&evt->v, &expected, sizeof(evt->v),
                                  timeout_ns, NULL, 1);

        return _Py_atomic_load_uint8(&evt->v) == _Py_LOCKED;
    }
}

static int
unlock_once(_PyOnceFlag *o, int res)
{
    // On success (res=0), we set the state to _Py_ONCE_INITIALIZED.
    // On failure (res=-1), we reset the state to _Py_UNLOCKED.
    uint8_t new_value;
    switch (res) {
        case -1: new_value = _Py_UNLOCKED; break;
        case  0: new_value = _Py_ONCE_INITIALIZED; break;
        default: {
            Py_FatalError("invalid result from _PyOnceFlag_CallOnce");
            Py_UNREACHABLE();
            break;
        }
    }

    uint8_t old_value = _Py_atomic_exchange_uint8(&o->v, new_value);
    if ((old_value & _Py_HAS_PARKED) != 0) {
        // wake up anyone waiting on the once flag
        _PyParkingLot_UnparkAll(&o->v);
    }
    return res;
}

int
_PyOnceFlag_CallOnceSlow(_PyOnceFlag *flag, _Py_once_fn_t *fn, void *arg)
{
    uint8_t v = _Py_atomic_load_uint8(&flag->v);
    for (;;) {
        if (v == _Py_UNLOCKED) {
            if (!_Py_atomic_compare_exchange_uint8(&flag->v, &v, _Py_LOCKED)) {
                continue;
            }
            int res = fn(arg);
            return unlock_once(flag, res);
        }

        if (v == _Py_ONCE_INITIALIZED) {
            return 0;
        }

        // The once flag is initializing (locked).
        assert((v & _Py_LOCKED));
        if (!(v & _Py_HAS_PARKED)) {
            // We are the first waiter. Set the _Py_HAS_PARKED flag.
            uint8_t new_value = v | _Py_HAS_PARKED;
            if (!_Py_atomic_compare_exchange_uint8(&flag->v, &v, new_value)) {
                continue;
            }
            v = new_value;
        }

        // Wait for initialization to finish.
        _PyParkingLot_Park(&flag->v, &v, sizeof(v), -1, NULL, 1);
        v = _Py_atomic_load_uint8(&flag->v);
    }
}

#define _Py_WRITE_LOCKED 1
#define _PyRWMutex_READER_SHIFT 2
#define _Py_RWMUTEX_MAX_READERS (UINTPTR_MAX >> _PyRWMutex_READER_SHIFT)

static uintptr_t
rwmutex_set_parked_and_wait(_PyRWMutex *rwmutex, uintptr_t bits)
{
    // Set _Py_HAS_PARKED and wait until we are woken up.
    if ((bits & _Py_HAS_PARKED) == 0) {
        uintptr_t newval = bits | _Py_HAS_PARKED;
        if (!_Py_atomic_compare_exchange_uintptr(&rwmutex->bits,
                                                 &bits, newval)) {
            return bits;
        }
        bits = newval;
    }

    _PyParkingLot_Park(&rwmutex->bits, &bits, sizeof(bits), -1, NULL, 1);
    return _Py_atomic_load_uintptr_relaxed(&rwmutex->bits);
}

// The number of readers holding the lock
static uintptr_t
rwmutex_reader_count(uintptr_t bits)
{
    return bits >> _PyRWMutex_READER_SHIFT;
}

void
_PyRWMutex_RLock(_PyRWMutex *rwmutex)
{
    uintptr_t bits = _Py_atomic_load_uintptr_relaxed(&rwmutex->bits);
    for (;;) {
        if ((bits & _Py_WRITE_LOCKED)) {
            // A writer already holds the lock.
            bits = rwmutex_set_parked_and_wait(rwmutex, bits);
            continue;
        }
        else if ((bits & _Py_HAS_PARKED)) {
            // Reader(s) hold the lock (or just gave up the lock), but there is
            // at least one waiting writer. We can't grab the lock because we
            // don't want to starve the writer. Instead, we park ourselves and
            // wait for the writer to eventually wake us up.
            bits = rwmutex_set_parked_and_wait(rwmutex, bits);
            continue;
        }
        else {
            // The lock is unlocked or read-locked. Try to grab it.
            assert(rwmutex_reader_count(bits) < _Py_RWMUTEX_MAX_READERS);
            uintptr_t newval = bits + (1 << _PyRWMutex_READER_SHIFT);
            if (!_Py_atomic_compare_exchange_uintptr(&rwmutex->bits,
                                                     &bits, newval)) {
                continue;
            }
            return;
        }
    }
}

void
_PyRWMutex_RUnlock(_PyRWMutex *rwmutex)
{
    uintptr_t bits = _Py_atomic_add_uintptr(&rwmutex->bits, -(1 << _PyRWMutex_READER_SHIFT));
    assert(rwmutex_reader_count(bits) > 0 && "lock was not read-locked");
    bits -= (1 << _PyRWMutex_READER_SHIFT);

    if (rwmutex_reader_count(bits) == 0 && (bits & _Py_HAS_PARKED)) {
        _PyParkingLot_UnparkAll(&rwmutex->bits);
        return;
    }
}

void
_PyRWMutex_Lock(_PyRWMutex *rwmutex)
{
    uintptr_t bits = _Py_atomic_load_uintptr_relaxed(&rwmutex->bits);
    for (;;) {
        // If there are no active readers and it's not already write-locked,
        // then we can grab the lock.
        if ((bits & ~_Py_HAS_PARKED) == 0) {
            if (!_Py_atomic_compare_exchange_uintptr(&rwmutex->bits,
                                                     &bits,
                                                     bits | _Py_WRITE_LOCKED)) {
                continue;
            }
            return;
        }

        // Otherwise, we have to wait.
        bits = rwmutex_set_parked_and_wait(rwmutex, bits);
    }
}

void
_PyRWMutex_Unlock(_PyRWMutex *rwmutex)
{
    uintptr_t old_bits = _Py_atomic_exchange_uintptr(&rwmutex->bits, 0);

    assert((old_bits & _Py_WRITE_LOCKED) && "lock was not write-locked");
    assert(rwmutex_reader_count(old_bits) == 0 && "lock was read-locked");

    if ((old_bits & _Py_HAS_PARKED) != 0) {
        _PyParkingLot_UnparkAll(&rwmutex->bits);
    }
}
