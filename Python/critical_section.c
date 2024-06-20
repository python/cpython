#include "Python.h"

#include "pycore_lock.h"
#include "pycore_critical_section.h"

#ifdef Py_GIL_DISABLED
static_assert(_Alignof(PyCriticalSection) >= 4,
              "critical section must be aligned to at least 4 bytes");
#endif

void
PyCriticalSection_Begin(PyCriticalSection *c, PyObject *op)
{
#ifdef Py_GIL_DISABLED
    _PyCriticalSection_BeginInline(c, &op->ob_mutex);
#endif
}

void
PyCriticalSection_End(PyCriticalSection *c)
{
#ifdef Py_GIL_DISABLED
    _PyCriticalSection_EndInline(c);
#endif
}

void
PyCriticalSection2_Begin(PyCriticalSection2 *c, PyObject *a, PyObject *b)
{
#ifdef Py_GIL_DISABLED
    _PyCriticalSection2_BeginInline(c, &a->ob_mutex, &b->ob_mutex);
#endif
}

void
PyCriticalSection2_End(PyCriticalSection2 *c)
{
#ifdef Py_GIL_DISABLED
    _PyCriticalSection2_EndInline(c);
#endif
}

void
_PyCriticalSection_BeginSlow(PyCriticalSection *c, PyMutex *m)
{
#ifdef Py_GIL_DISABLED
    PyThreadState *tstate = _PyThreadState_GET();
    c->mutex = NULL;
    c->prev = (uintptr_t)tstate->critical_section;
    tstate->critical_section = (uintptr_t)c;

    PyMutex_Lock(m);
    c->mutex = m;
#endif
}

void
_PyCriticalSection2_BeginSlow(PyCriticalSection2 *c, PyMutex *m1, PyMutex *m2,
                              int is_m1_locked)
{
#ifdef Py_GIL_DISABLED
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
#endif
}

#ifdef Py_GIL_DISABLED
static PyCriticalSection *
untag_critical_section(uintptr_t tag)
{
    return (PyCriticalSection *)(tag & ~_Py_CRITICAL_SECTION_MASK);
}
#endif

// Release all locks held by critical sections. This is called by
// _PyThreadState_Detach.
void
_PyCriticalSection_SuspendAll(PyThreadState *tstate)
{
#ifdef Py_GIL_DISABLED
    uintptr_t *tagptr = &tstate->critical_section;
    while (_PyCriticalSection_IsActive(*tagptr)) {
        PyCriticalSection *c = untag_critical_section(*tagptr);

        if (c->mutex) {
            PyMutex_Unlock(c->mutex);
            if ((*tagptr & _Py_CRITICAL_SECTION_TWO_MUTEXES)) {
                PyCriticalSection2 *c2 = (PyCriticalSection2 *)c;
                if (c2->mutex2) {
                    PyMutex_Unlock(c2->mutex2);
                }
            }
        }

        *tagptr |= _Py_CRITICAL_SECTION_INACTIVE;
        tagptr = &c->prev;
    }
#endif
}

void
_PyCriticalSection_Resume(PyThreadState *tstate)
{
#ifdef Py_GIL_DISABLED
    uintptr_t p = tstate->critical_section;
    PyCriticalSection *c = untag_critical_section(p);
    assert(!_PyCriticalSection_IsActive(p));

    PyMutex *m1 = c->mutex;
    c->mutex = NULL;

    PyMutex *m2 = NULL;
    PyCriticalSection2 *c2 = NULL;
    if ((p & _Py_CRITICAL_SECTION_TWO_MUTEXES)) {
        c2 = (PyCriticalSection2 *)c;
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
#endif
}
