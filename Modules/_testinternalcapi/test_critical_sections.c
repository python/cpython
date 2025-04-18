/*
 * C Extension module to test pycore_critical_section.h API.
 */

#include "parts.h"
#include "pycore_critical_section.h"

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

static PyMethodDef test_methods[] = {
    {"test_critical_sections", test_critical_sections, METH_NOARGS},
    {"test_critical_sections_nest", test_critical_sections_nest, METH_NOARGS},
    {"test_critical_sections_suspend", test_critical_sections_suspend, METH_NOARGS},
#ifdef Py_CAN_START_THREADS
    {"test_critical_sections_threads", test_critical_sections_threads, METH_NOARGS},
    {"test_critical_sections_gc", test_critical_sections_gc, METH_NOARGS},
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
