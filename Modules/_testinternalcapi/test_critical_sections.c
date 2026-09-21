/*
 * C Extension module to test pycore_critical_section.h API.
 */

#include "parts.h"
#include "pycore_critical_section.h"
#include "pycore_pystate.h"
#include "pycore_pythread.h"

#ifdef MS_WINDOWS
#  include <windows.h>            // Sleep()
#endif

#ifdef Py_GIL_DISABLED
#define assert_nogil assert
#define assert_gil(x)
#else
#define assert_gil assert
#define assert_nogil(x)
#endif


static PyObject *
test_critical_sections(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyObject *d1 = PyDict_New();
    assert(d1 != NULL);

    PyObject *d2 = PyDict_New();
    assert(d2 != NULL);

    // Beginning a critical section should lock the associated object and
    // push the critical section onto the thread's stack (in Py_GIL_DISABLED builds).
    Py_BEGIN_CRITICAL_SECTION(d1);
    assert_nogil(PyMutex_IsLocked(&d1->ob_mutex));
    assert_nogil(_PyCriticalSection_IsActive(PyThreadState_GET()->critical_section));
    assert_gil(PyThreadState_GET()->critical_section == 0);
    Py_END_CRITICAL_SECTION();
    assert_nogil(!PyMutex_IsLocked(&d1->ob_mutex));

    assert_nogil(!PyMutex_IsLocked(&d1->ob_mutex));
    assert_nogil(!PyMutex_IsLocked(&d2->ob_mutex));
    Py_BEGIN_CRITICAL_SECTION2(d1, d2);
    assert_nogil(PyMutex_IsLocked(&d1->ob_mutex));
    assert_nogil(PyMutex_IsLocked(&d2->ob_mutex));
    Py_END_CRITICAL_SECTION2();
    assert_nogil(!PyMutex_IsLocked(&d1->ob_mutex));
    assert_nogil(!PyMutex_IsLocked(&d2->ob_mutex));

    // Passing the same object twice should work (and not deadlock).
    assert_nogil(!PyMutex_IsLocked(&d2->ob_mutex));
    Py_BEGIN_CRITICAL_SECTION2(d2, d2);
    assert_nogil(PyMutex_IsLocked(&d2->ob_mutex));
    Py_END_CRITICAL_SECTION2();
    assert_nogil(!PyMutex_IsLocked(&d2->ob_mutex));

    Py_DECREF(d2);
    Py_DECREF(d1);
    Py_RETURN_NONE;
}

static void
lock_unlock_object(PyObject *obj, int recurse_depth)
{
    Py_BEGIN_CRITICAL_SECTION(obj);
    if (recurse_depth > 0) {
        lock_unlock_object(obj, recurse_depth - 1);
    }
    Py_END_CRITICAL_SECTION();
}

static void
lock_unlock_two_objects(PyObject *a, PyObject *b, int recurse_depth)
{
    Py_BEGIN_CRITICAL_SECTION2(a, b);
    if (recurse_depth > 0) {
        lock_unlock_two_objects(a, b, recurse_depth - 1);
    }
    Py_END_CRITICAL_SECTION2();
}


// Test that nested critical sections do not deadlock if they attempt to lock
// the same object.
static PyObject *
test_critical_sections_nest(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyObject *a = PyDict_New();
    assert(a != NULL);
    PyObject *b = PyDict_New();
    assert(b != NULL);

    // Locking an object recursively with this API should not deadlock.
    assert_nogil(!PyMutex_IsLocked(&a->ob_mutex));
    Py_BEGIN_CRITICAL_SECTION(a);
    assert_nogil(PyMutex_IsLocked(&a->ob_mutex));
    lock_unlock_object(a, 10);
    assert_nogil(PyMutex_IsLocked(&a->ob_mutex));
    Py_END_CRITICAL_SECTION();
    assert_nogil(!PyMutex_IsLocked(&a->ob_mutex));

    // Same test but with two objects.
    Py_BEGIN_CRITICAL_SECTION2(b, a);
    lock_unlock_two_objects(a, b, 10);
    assert_nogil(PyMutex_IsLocked(&a->ob_mutex));
    assert_nogil(PyMutex_IsLocked(&b->ob_mutex));
    Py_END_CRITICAL_SECTION2();

    Py_DECREF(b);
    Py_DECREF(a);
    Py_RETURN_NONE;
}

// Test that a critical section is suspended by a Py_BEGIN_ALLOW_THREADS and
// resumed by a Py_END_ALLOW_THREADS.
static PyObject *
test_critical_sections_suspend(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyObject *a = PyDict_New();
    assert(a != NULL);

    Py_BEGIN_CRITICAL_SECTION(a);
    assert_nogil(PyMutex_IsLocked(&a->ob_mutex));

    // Py_BEGIN_ALLOW_THREADS should suspend the active critical section
    Py_BEGIN_ALLOW_THREADS
    assert_nogil(!PyMutex_IsLocked(&a->ob_mutex));
    Py_END_ALLOW_THREADS;

    // After Py_END_ALLOW_THREADS the critical section should be resumed.
    assert_nogil(PyMutex_IsLocked(&a->ob_mutex));
    Py_END_CRITICAL_SECTION();

    Py_DECREF(a);
    Py_RETURN_NONE;
}

#ifdef Py_CAN_START_THREADS
struct test_data {
    PyObject *obj1;
    PyObject *obj2;
    PyObject *obj3;
    Py_ssize_t countdown;
    PyEvent done_event;
};

static void
thread_critical_sections(void *arg)
{
    const Py_ssize_t NUM_ITERS = 200;
    struct test_data *test_data = arg;
    PyGILState_STATE gil = PyGILState_Ensure();

    for (Py_ssize_t i = 0; i < NUM_ITERS; i++) {
        Py_BEGIN_CRITICAL_SECTION(test_data->obj1);
        Py_END_CRITICAL_SECTION();

        Py_BEGIN_CRITICAL_SECTION(test_data->obj2);
        lock_unlock_object(test_data->obj1, 1);
        Py_END_CRITICAL_SECTION();

        Py_BEGIN_CRITICAL_SECTION2(test_data->obj3, test_data->obj1);
        lock_unlock_object(test_data->obj2, 2);
        Py_END_CRITICAL_SECTION2();

        Py_BEGIN_CRITICAL_SECTION(test_data->obj3);
        Py_BEGIN_ALLOW_THREADS
        Py_END_ALLOW_THREADS
        Py_END_CRITICAL_SECTION();
    }

    PyGILState_Release(gil);
    if (_Py_atomic_add_ssize(&test_data->countdown, -1) == 1) {
        // last thread to finish sets done_event
        _PyEvent_Notify(&test_data->done_event);
    }
}

static PyObject *
test_critical_sections_threads(PyObject *self, PyObject *Py_UNUSED(args))
{
    const Py_ssize_t NUM_THREADS = 4;
    struct test_data test_data = {
        .obj1 = PyDict_New(),
        .obj2 = PyDict_New(),
        .obj3 = PyDict_New(),
        .countdown = NUM_THREADS,
    };
    assert(test_data.obj1 != NULL);
    assert(test_data.obj2 != NULL);
    assert(test_data.obj3 != NULL);

    for (Py_ssize_t i = 0; i < NUM_THREADS; i++) {
        PyThread_start_new_thread(&thread_critical_sections, &test_data);
    }
    PyEvent_Wait(&test_data.done_event);

    Py_DECREF(test_data.obj3);
    Py_DECREF(test_data.obj2);
    Py_DECREF(test_data.obj1);
    Py_RETURN_NONE;
}

static void
pysleep(int ms)
{
#ifdef MS_WINDOWS
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

struct test_data_gc {
    PyObject *obj;
    Py_ssize_t num_threads;
    Py_ssize_t id;
    Py_ssize_t countdown;
    PyEvent done_event;
    PyEvent ready;
};

static void
thread_gc(void *arg)
{
    struct test_data_gc *test_data = arg;
    PyGILState_STATE gil = PyGILState_Ensure();

    Py_ssize_t id = _Py_atomic_add_ssize(&test_data->id, 1);
    if (id == test_data->num_threads - 1) {
        _PyEvent_Notify(&test_data->ready);
    }
    else {
        // wait for all test threads to more reliably reproduce the issue.
        PyEvent_Wait(&test_data->ready);
    }

    if (id == 0) {
        Py_BEGIN_CRITICAL_SECTION(test_data->obj);
        // pause long enough that the lock would be handed off directly to
        // a waiting thread.
        pysleep(5);
        PyGC_Collect();
        Py_END_CRITICAL_SECTION();
    }
    else if (id == 1) {
        pysleep(1);
        Py_BEGIN_CRITICAL_SECTION(test_data->obj);
        pysleep(1);
        Py_END_CRITICAL_SECTION();
    }
    else if (id == 2) {
        // sleep long enough so that thread 0 is waiting to stop the world
        pysleep(6);
        Py_BEGIN_CRITICAL_SECTION(test_data->obj);
        pysleep(1);
        Py_END_CRITICAL_SECTION();
    }

    PyGILState_Release(gil);
    if (_Py_atomic_add_ssize(&test_data->countdown, -1) == 1) {
        // last thread to finish sets done_event
        _PyEvent_Notify(&test_data->done_event);
    }
}

static PyObject *
test_critical_sections_gc(PyObject *self, PyObject *Py_UNUSED(args))
{
    // gh-118332: Contended critical sections should not deadlock with GC
    const Py_ssize_t NUM_THREADS = 3;
    struct test_data_gc test_data = {
        .obj = PyDict_New(),
        .countdown = NUM_THREADS,
        .num_threads = NUM_THREADS,
    };
    assert(test_data.obj != NULL);

    for (Py_ssize_t i = 0; i < NUM_THREADS; i++) {
        PyThread_start_new_thread(&thread_gc, &test_data);
    }
    PyEvent_Wait(&test_data.done_event);
    Py_DECREF(test_data.obj);
    Py_RETURN_NONE;
}

#endif

#ifdef Py_GIL_DISABLED

static PyObject *
test_critical_section1_reacquisition(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyObject *a = PyDict_New();
    assert(a != NULL);

    PyCriticalSection cs1, cs2;
    // First acquisition of critical section on object locks it
    PyCriticalSection_Begin(&cs1, a);
    assert(PyMutex_IsLocked(&a->ob_mutex));
    assert(_PyCriticalSection_IsActive(PyThreadState_GET()->critical_section));
    assert(_PyThreadState_GET()->critical_section == (uintptr_t)&cs1);
    // Attempting to re-acquire critical section on same object which
    // is already locked by top-most critical section is a no-op.
    PyCriticalSection_Begin(&cs2, a);
    assert(PyMutex_IsLocked(&a->ob_mutex));
    assert(_PyCriticalSection_IsActive(PyThreadState_GET()->critical_section));
    assert(_PyThreadState_GET()->critical_section == (uintptr_t)&cs1);
    // Releasing second critical section is a no-op.
    PyCriticalSection_End(&cs2);
    assert(PyMutex_IsLocked(&a->ob_mutex));
    assert(_PyCriticalSection_IsActive(PyThreadState_GET()->critical_section));
    assert(_PyThreadState_GET()->critical_section == (uintptr_t)&cs1);
    // Releasing first critical section unlocks the object
    PyCriticalSection_End(&cs1);
    assert(!PyMutex_IsLocked(&a->ob_mutex));

    Py_DECREF(a);
    Py_RETURN_NONE;
}

static PyObject *
test_critical_section2_reacquisition(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyObject *a = PyDict_New();
    assert(a != NULL);
    PyObject *b = PyDict_New();
    assert(b != NULL);

    PyCriticalSection2 cs;
    // First acquisition of critical section on objects locks them
    PyCriticalSection2_Begin(&cs, a, b);
    assert(PyMutex_IsLocked(&a->ob_mutex));
    assert(PyMutex_IsLocked(&b->ob_mutex));
    assert(_PyCriticalSection_IsActive(PyThreadState_GET()->critical_section));
    assert((_PyThreadState_GET()->critical_section &
                  ~_Py_CRITICAL_SECTION_MASK) == (uintptr_t)&cs);

    // Attempting to re-acquire critical section on either of two
    // objects already locked by top-most critical section is a no-op.

    // Check re-acquiring on first object
    PyCriticalSection a_cs;
    PyCriticalSection_Begin(&a_cs, a);
    assert(PyMutex_IsLocked(&a->ob_mutex));
    assert(PyMutex_IsLocked(&b->ob_mutex));
    assert(_PyCriticalSection_IsActive(PyThreadState_GET()->critical_section));
    assert((_PyThreadState_GET()->critical_section &
                  ~_Py_CRITICAL_SECTION_MASK) == (uintptr_t)&cs);
    // Releasing critical section on either object is a no-op.
    PyCriticalSection_End(&a_cs);
    assert(PyMutex_IsLocked(&a->ob_mutex));
    assert(PyMutex_IsLocked(&b->ob_mutex));
    assert(_PyCriticalSection_IsActive(PyThreadState_GET()->critical_section));
    assert((_PyThreadState_GET()->critical_section &
                  ~_Py_CRITICAL_SECTION_MASK) == (uintptr_t)&cs);

    // Check re-acquiring on second object
    PyCriticalSection b_cs;
    PyCriticalSection_Begin(&b_cs, b);
    assert(PyMutex_IsLocked(&a->ob_mutex));
    assert(PyMutex_IsLocked(&b->ob_mutex));
    assert(_PyCriticalSection_IsActive(PyThreadState_GET()->critical_section));
    assert((_PyThreadState_GET()->critical_section &
                  ~_Py_CRITICAL_SECTION_MASK) == (uintptr_t)&cs);
    // Releasing critical section on either object is a no-op.
    PyCriticalSection_End(&b_cs);
    assert(PyMutex_IsLocked(&a->ob_mutex));
    assert(PyMutex_IsLocked(&b->ob_mutex));
    assert(_PyCriticalSection_IsActive(PyThreadState_GET()->critical_section));
    assert((_PyThreadState_GET()->critical_section &
                  ~_Py_CRITICAL_SECTION_MASK) == (uintptr_t)&cs);

    // Releasing critical section on both objects unlocks them
    PyCriticalSection2_End(&cs);
    assert(!PyMutex_IsLocked(&a->ob_mutex));
    assert(!PyMutex_IsLocked(&b->ob_mutex));

    Py_DECREF(a);
    Py_DECREF(b);
    Py_RETURN_NONE;
}

#endif // Py_GIL_DISABLED

#ifdef Py_CAN_START_THREADS

// gh-144513: Test that critical sections don't deadlock with stop-the-world.
// This test is designed to deadlock (timeout) on builds without the fix.
struct test_data_stw {
    PyObject *obj;
    Py_ssize_t num_threads;
    Py_ssize_t started;
    PyEvent ready;
};

static void
thread_stw(void *arg)
{
    struct test_data_stw *test_data = arg;
    PyGILState_STATE gil = PyGILState_Ensure();

    if (_Py_atomic_add_ssize(&test_data->started, 1) == test_data->num_threads - 1) {
        _PyEvent_Notify(&test_data->ready);
    }

    // All threads: acquire critical section and hold it long enough to
    // trigger TIME_TO_BE_FAIR_NS (1 ms), which causes direct handoff on unlock.
    Py_BEGIN_CRITICAL_SECTION(test_data->obj);
    pysleep(10);  // 10 ms = 10 x TIME_TO_BE_FAIR_NS
    Py_END_CRITICAL_SECTION();

    PyGILState_Release(gil);
}

static PyObject *
test_critical_sections_stw(PyObject *self, PyObject *Py_UNUSED(args))
{
    // gh-144513: Test that critical sections don't deadlock during STW.
    //
    // The deadlock occurs when lock ownership is handed off (due to fairness
    // after TIME_TO_BE_FAIR_NS) to a thread that has already suspended for
    // stop-the-world. The STW requester then cannot acquire the lock.
    //
    // With the fix, the STW requester detects world_stopped and skips locking.

    #define STW_NUM_THREADS 2
    struct test_data_stw test_data = {
        .obj = PyDict_New(),
        .num_threads = STW_NUM_THREADS,
    };
    if (test_data.obj == NULL) {
        return NULL;
    }

    PyThread_handle_t handles[STW_NUM_THREADS];
    PyThread_ident_t idents[STW_NUM_THREADS];
    for (Py_ssize_t i = 0; i < STW_NUM_THREADS; i++) {
        PyThread_start_joinable_thread(&thread_stw, &test_data,
                                       &idents[i], &handles[i]);
    }

    // Wait for threads to start, then let them compete for the lock
    PyEvent_Wait(&test_data.ready);
    pysleep(5);

    // Request stop-the-world and try to acquire the critical section.
    // Without the fix, this may deadlock.
    PyInterpreterState *interp = PyInterpreterState_Get();
    _PyEval_StopTheWorld(interp);

    Py_BEGIN_CRITICAL_SECTION(test_data.obj);
    Py_END_CRITICAL_SECTION();

    _PyEval_StartTheWorld(interp);

    for (Py_ssize_t i = 0; i < STW_NUM_THREADS; i++) {
        PyThread_join_thread(handles[i]);
    }
    #undef STW_NUM_THREADS
    Py_DECREF(test_data.obj);
    Py_RETURN_NONE;
}

#endif // Py_CAN_START_THREADS

static PyMethodDef test_methods[] = {
    {"test_critical_sections", test_critical_sections, METH_NOARGS},
    {"test_critical_sections_nest", test_critical_sections_nest, METH_NOARGS},
    {"test_critical_sections_suspend", test_critical_sections_suspend, METH_NOARGS},
#ifdef Py_GIL_DISABLED
    {"test_critical_section1_reacquisition", test_critical_section1_reacquisition, METH_NOARGS},
    {"test_critical_section2_reacquisition", test_critical_section2_reacquisition, METH_NOARGS},
#endif
#ifdef Py_CAN_START_THREADS
    {"test_critical_sections_threads", test_critical_sections_threads, METH_NOARGS},
    {"test_critical_sections_gc", test_critical_sections_gc, METH_NOARGS},
    {"test_critical_sections_stw", test_critical_sections_stw, METH_NOARGS},
#endif
    {NULL, NULL} /* sentinel */
};

int
_PyTestInternalCapi_Init_CriticalSection(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }
    return 0;
}
