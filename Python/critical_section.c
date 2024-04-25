#include "Python.h"

#include "pycore_lock.h"
#include "pycore_critical_section.h"

static_assert(_Alignof(_PyCriticalSection) >= 4,
              "critical section must be aligned to at least 4 bytes");

void
_PyCriticalSection_BeginSlow(_PyCriticalSection *c, PyMutex *m)
{
    PyThreadState *tstate = _PyThreadState_GET();
    c->mutex = NULL;
    c->prev = (uintptr_t)tstate->critical_section;
    tstate->critical_section = (uintptr_t)c;

    _PyMutex_LockSlow(m);
    c->mutex = m;
}

void
_PyCriticalSection2_BeginSlow(_PyCriticalSection2 *c, PyMutex *m1, PyMutex *m2,
                              int is_m1_locked)
{
    PyThreadState *tstate = _PyThreadState_GET();
    c->base.mutex = NULL;
    c->mutex2 = NULL;
    c->base.prev = tstate->critical_section;
    tstate->critical_section = (uintptr_t)c | _Py_CRITICAL_SECTION_TWO_MUTEXES;

    if (!is_m1_locked) {
        PyMutex_Lock(m1);
    }
    PyMutex_Lock(m2);
    c->base.mutex = m1;
    c->mutex2 = m2;
}

static _PyCriticalSection *
untag_critical_section(uintptr_t tag)
{
    return (_PyCriticalSection *)(tag & ~_Py_CRITICAL_SECTION_MASK);
}

// Release all locks held by critical sections. This is called by
// _PyThreadState_Detach.
void
_PyCriticalSection_SuspendAll(PyThreadState *tstate)
{
    uintptr_t *tagptr = &tstate->critical_section;
    while (_PyCriticalSection_IsActive(*tagptr)) {
        _PyCriticalSection *c = untag_critical_section(*tagptr);

        if (c->mutex) {
            PyMutex_Unlock(c->mutex);
            if ((*tagptr & _Py_CRITICAL_SECTION_TWO_MUTEXES)) {
                _PyCriticalSection2 *c2 = (_PyCriticalSection2 *)c;
                if (c2->mutex2) {
                    PyMutex_Unlock(c2->mutex2);
                }
            }
        }

        *tagptr |= _Py_CRITICAL_SECTION_INACTIVE;
        tagptr = &c->prev;
    }
}

void
_PyCriticalSection_Resume(PyThreadState *tstate)
{
    uintptr_t p = tstate->critical_section;
    _PyCriticalSection *c = untag_critical_section(p);
    assert(!_PyCriticalSection_IsActive(p));

    PyMutex *m1 = c->mutex;
    c->mutex = NULL;

    PyMutex *m2 = NULL;
    _PyCriticalSection2 *c2 = NULL;
    if ((p & _Py_CRITICAL_SECTION_TWO_MUTEXES)) {
        c2 = (_PyCriticalSection2 *)c;
        m2 = c2->mutex2;
        c2->mutex2 = NULL;
    }

    if (m1) {
        PyMutex_Lock(m1);
    }
    if (m2) {
        PyMutex_Lock(m2);
    }

    c->mutex = m1;
    if (m2) {
        c2->mutex2 = m2;
    }

    tstate->critical_section &= ~_Py_CRITICAL_SECTION_INACTIVE;
}
