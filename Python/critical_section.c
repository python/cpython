#include "Python.h"

#include "pycore_lock.h"
#include "pycore_critical_section.h"

#ifdef Py_GIL_DISABLED
static_assert(_Alignof(PyCriticalSection) >= 4,
              "critical section must be aligned to at least 4 bytes");
#endif

#ifdef Py_GIL_DISABLED
static PyCriticalSection *
untag_critical_section(uintptr_t tag)
{
    return (PyCriticalSection *)(tag & ~_Py_CRITICAL_SECTION_MASK);
}
#endif

void
_PyCriticalSection_BeginSlow(PyThreadState *tstate, PyCriticalSection *c, PyMutex *m)
{
#ifdef Py_GIL_DISABLED
    // As an optimisation for locking the same object recursively, skip
    // locking if the mutex is currently locked by the top-most critical
    // section.
    // If the top-most critical section is a two-mutex critical section,
    // then locking is skipped if either mutex is m.
    if (tstate->critical_section) {
        PyCriticalSection *prev = untag_critical_section(tstate->critical_section);
        if (prev->_cs_mutex == m) {
            c->_cs_mutex = NULL;
            c->_cs_prev = 0;
            return;
        }
        if (tstate->critical_section & _Py_CRITICAL_SECTION_TWO_MUTEXES) {
            PyCriticalSection2 *prev2 = (PyCriticalSection2 *)
                untag_critical_section(tstate->critical_section);
            if (prev2->_cs_mutex2 == m) {
                c->_cs_mutex = NULL;
                c->_cs_prev = 0;
                return;
            }
        }
    }
    c->_cs_mutex = NULL;
    c->_cs_prev = (uintptr_t)tstate->critical_section;
    tstate->critical_section = (uintptr_t)c;

    PyMutex_Lock(m);
    c->_cs_mutex = m;
#endif
}

void
_PyCriticalSection2_BeginSlow(PyThreadState *tstate, PyCriticalSection2 *c, PyMutex *m1, PyMutex *m2,
                              int is_m1_locked)
{
#ifdef Py_GIL_DISABLED
    c->_cs_base._cs_mutex = NULL;
    c->_cs_mutex2 = NULL;
    c->_cs_base._cs_prev = tstate->critical_section;
    tstate->critical_section = (uintptr_t)c | _Py_CRITICAL_SECTION_TWO_MUTEXES;

    if (!is_m1_locked) {
        PyMutex_Lock(m1);
    }
    PyMutex_Lock(m2);
    c->_cs_base._cs_mutex = m1;
    c->_cs_mutex2 = m2;
#endif
}


// Release all locks held by critical sections. This is called by
// _PyThreadState_Detach.
void
_PyCriticalSection_SuspendAll(PyThreadState *tstate)
{
#ifdef Py_GIL_DISABLED
    uintptr_t *tagptr = &tstate->critical_section;
    while (_PyCriticalSection_IsActive(*tagptr)) {
        PyCriticalSection *c = untag_critical_section(*tagptr);

        if (c->_cs_mutex) {
            PyMutex_Unlock(c->_cs_mutex);
            if ((*tagptr & _Py_CRITICAL_SECTION_TWO_MUTEXES)) {
                PyCriticalSection2 *c2 = (PyCriticalSection2 *)c;
                if (c2->_cs_mutex2) {
                    PyMutex_Unlock(c2->_cs_mutex2);
                }
            }
        }

        *tagptr |= _Py_CRITICAL_SECTION_INACTIVE;
        tagptr = &c->_cs_prev;
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

    PyMutex *m1 = c->_cs_mutex;
    c->_cs_mutex = NULL;

    PyMutex *m2 = NULL;
    PyCriticalSection2 *c2 = NULL;
    if ((p & _Py_CRITICAL_SECTION_TWO_MUTEXES)) {
        c2 = (PyCriticalSection2 *)c;
        m2 = c2->_cs_mutex2;
        c2->_cs_mutex2 = NULL;
    }

    if (m1) {
        PyMutex_Lock(m1);
    }
    if (m2) {
        PyMutex_Lock(m2);
    }

    c->_cs_mutex = m1;
    if (m2) {
        c2->_cs_mutex2 = m2;
    }

    tstate->critical_section &= ~_Py_CRITICAL_SECTION_INACTIVE;
#endif
}

#undef PyCriticalSection_Begin
void
PyCriticalSection_Begin(PyCriticalSection *c, PyObject *op)
{
#ifdef Py_GIL_DISABLED
    _PyCriticalSection_Begin(_PyThreadState_GET(), c, op);
#endif
}

#undef PyCriticalSection_BeginMutex
void
PyCriticalSection_BeginMutex(PyCriticalSection *c, PyMutex *m)
{
#ifdef Py_GIL_DISABLED
    _PyCriticalSection_BeginMutex(_PyThreadState_GET(), c, m);
#endif
}

#undef PyCriticalSection_End
void
PyCriticalSection_End(PyCriticalSection *c)
{
#ifdef Py_GIL_DISABLED
    _PyCriticalSection_End(_PyThreadState_GET(), c);
#endif
}

#undef PyCriticalSection2_Begin
void
PyCriticalSection2_Begin(PyCriticalSection2 *c, PyObject *a, PyObject *b)
{
#ifdef Py_GIL_DISABLED
    _PyCriticalSection2_Begin(_PyThreadState_GET(), c, a, b);
#endif
}

#undef PyCriticalSection2_BeginMutex
void
PyCriticalSection2_BeginMutex(PyCriticalSection2 *c, PyMutex *m1, PyMutex *m2)
{
#ifdef Py_GIL_DISABLED
    _PyCriticalSection2_BeginMutex(_PyThreadState_GET(), c, m1, m2);
#endif
}

#undef PyCriticalSection2_End
void
PyCriticalSection2_End(PyCriticalSection2 *c)
{
#ifdef Py_GIL_DISABLED
    _PyCriticalSection2_End(_PyThreadState_GET(), c);
#endif
}
