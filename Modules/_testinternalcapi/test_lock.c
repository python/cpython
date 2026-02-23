// C Extension module to test pycore_lock.h API

#include "parts.h"
#include "pycore_lock.h"
#include "pycore_pythread.h"      // PyThread_get_thread_ident_ex()

#include "clinic/test_lock.c.h"

#ifdef MS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>         // usleep()
#endif

/*[clinic input]
module _testinternalcapi
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=7bb583d8c9eb9a78]*/


static void
pysleep(int ms)
{
#ifdef MS_WINDOWS
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

static PyObject *
test_lock_basic(PyObject *self, PyObject *obj)
{
    PyMutex m = (PyMutex){0};

    // uncontended lock and unlock
    PyMutex_Lock(&m);
    assert(m._bits == 1);
    PyMutex_Unlock(&m);
    assert(m._bits == 0);

    Py_RETURN_NONE;
}

struct test_lock2_data {
    PyMutex m;
    PyEvent done;
    int started;
};

static void
lock_thread(void *arg)
{
    struct test_lock2_data *test_data = arg;
    PyMutex *m = &test_data->m;
    _Py_atomic_store_int(&test_data->started, 1);

    PyMutex_Lock(m);
    // gh-135641: in rare cases the lock may still have `_Py_HAS_PARKED` set
    // (m->_bits == 3) due to bucket collisions in the parking lot hash table
    // between this mutex and the `test_data.done` event.
    assert(m->_bits == 1 || m->_bits == 3);

    PyMutex_Unlock(m);
    assert(m->_bits == 0);

    _PyEvent_Notify(&test_data->done);
}

static PyObject *
test_lock_two_threads(PyObject *self, PyObject *obj)
{
    // lock attempt by two threads
    struct test_lock2_data test_data;
    memset(&test_data, 0, sizeof(test_data));

    PyMutex_Lock(&test_data.m);
    assert(test_data.m._bits == 1);

    PyThread_start_new_thread(lock_thread, &test_data);

    // wait up to two seconds for the lock_thread to attempt to lock "m"
    int iters = 0;
    uint8_t v;
    do {
        pysleep(10);  // allow some time for the other thread to try to lock
        v = _Py_atomic_load_uint8_relaxed(&test_data.m._bits);
        assert(v == 1 || v == 3);
        iters++;
    } while (v != 3 && iters < 200);

    // both the "locked" and the "has parked" bits should be set
    v = _Py_atomic_load_uint8_relaxed(&test_data.m._bits);
    assert(v == 3);

    PyMutex_Unlock(&test_data.m);
    PyEvent_Wait(&test_data.done);
    assert(test_data.m._bits == 0);

    Py_RETURN_NONE;
}

#define COUNTER_THREADS 5
#define COUNTER_ITERS 10000

struct test_data_counter {
    PyMutex m;
    Py_ssize_t counter;
};

struct thread_data_counter {
    struct test_data_counter *test_data;
    PyEvent done_event;
};

static void
counter_thread(void *arg)
{
    struct thread_data_counter *thread_data = arg;
    struct test_data_counter *test_data = thread_data->test_data;

    for (Py_ssize_t i = 0; i < COUNTER_ITERS; i++) {
        PyMutex_Lock(&test_data->m);
        test_data->counter++;
        PyMutex_Unlock(&test_data->m);
    }
    _PyEvent_Notify(&thread_data->done_event);
}

static PyObject *
test_lock_counter(PyObject *self, PyObject *obj)
{
    // Test with rapidly locking and unlocking mutex
    struct test_data_counter test_data;
    memset(&test_data, 0, sizeof(test_data));

    struct thread_data_counter thread_data[COUNTER_THREADS];
    memset(&thread_data, 0, sizeof(thread_data));

    for (Py_ssize_t i = 0; i < COUNTER_THREADS; i++) {
        thread_data[i].test_data = &test_data;
        PyThread_start_new_thread(counter_thread, &thread_data[i]);
    }

    for (Py_ssize_t i = 0; i < COUNTER_THREADS; i++) {
        PyEvent_Wait(&thread_data[i].done_event);
    }

    assert(test_data.counter == COUNTER_THREADS * COUNTER_ITERS);
    Py_RETURN_NONE;
}

#define SLOW_COUNTER_ITERS 100

static void
slow_counter_thread(void *arg)
{
    struct thread_data_counter *thread_data = arg;
    struct test_data_counter *test_data = thread_data->test_data;

    for (Py_ssize_t i = 0; i < SLOW_COUNTER_ITERS; i++) {
        PyMutex_Lock(&test_data->m);
        if (i % 7 == 0) {
            pysleep(2);
        }
        test_data->counter++;
        PyMutex_Unlock(&test_data->m);
    }
    _PyEvent_Notify(&thread_data->done_event);
}

static PyObject *
test_lock_counter_slow(PyObject *self, PyObject *obj)
{
    // Test lock/unlock with occasional "long" critical section, which will
    // trigger handoff of the lock.
    struct test_data_counter test_data;
    memset(&test_data, 0, sizeof(test_data));

    struct thread_data_counter thread_data[COUNTER_THREADS];
    memset(&thread_data, 0, sizeof(thread_data));

    for (Py_ssize_t i = 0; i < COUNTER_THREADS; i++) {
        thread_data[i].test_data = &test_data;
        PyThread_start_new_thread(slow_counter_thread, &thread_data[i]);
    }

    for (Py_ssize_t i = 0; i < COUNTER_THREADS; i++) {
        PyEvent_Wait(&thread_data[i].done_event);
    }

    assert(test_data.counter == COUNTER_THREADS * SLOW_COUNTER_ITERS);
    Py_RETURN_NONE;
}

struct bench_data_locks {
    int stop;
    int work_inside;
    int work_outside;
    int num_acquisitions;
    Py_ssize_t target_iters;
    char padding[200];
    PyMutex m;
    double value;
};

struct bench_thread_data {
    struct bench_data_locks *bench_data;
    Py_ssize_t iters;
    PyEvent done;
};

static void
thread_benchmark_locks(void *arg)
{
    struct bench_thread_data *thread_data = arg;
    struct bench_data_locks *bench_data = thread_data->bench_data;
    int work_inside = bench_data->work_inside;
    int work_outside = bench_data->work_outside;
    int num_acquisitions = bench_data->num_acquisitions;
    Py_ssize_t target_iters = bench_data->target_iters;

    double local_value = 0.0;
    double my_value = 1.0;
    Py_ssize_t iters = 0;
    for (;;) {
        if (target_iters > 0) {
            // Fixed iteration mode: each thread runs for target_iters
            if (iters >= target_iters) {
                break;
            }
        }
        else {
            // Time-based mode: stop when signaled
            if (_Py_atomic_load_int_relaxed(&bench_data->stop)) {
                break;
            }
        }
        for (int acq = 0; acq < num_acquisitions; acq++) {
            PyMutex_Lock(&bench_data->m);
            for (int i = 0; i < work_inside; i++) {
                bench_data->value += my_value;
                my_value = bench_data->value;
            }
            PyMutex_Unlock(&bench_data->m);
        }
        for (int i = 0; i < work_outside; i++) {
            local_value += my_value;
            my_value = local_value;
        }
        iters += num_acquisitions;
    }

    thread_data->iters = iters;
    _PyEvent_Notify(&thread_data->done);
}

/*[clinic input]
_testinternalcapi.benchmark_locks

    num_threads: Py_ssize_t
    work_inside: int = 1
    work_outside: int = 0
    time_ms: int = 1000
    num_acquisitions: int = 1
    total_iters: Py_ssize_t = 0
    num_locks: Py_ssize_t = 1
    /

[clinic start generated code]*/

static PyObject *
_testinternalcapi_benchmark_locks_impl(PyObject *module,
                                       Py_ssize_t num_threads,
                                       int work_inside, int work_outside,
                                       int time_ms, int num_acquisitions,
                                       Py_ssize_t total_iters,
                                       Py_ssize_t num_locks)
/*[clinic end generated code: output=942723d0d7194f36 input=d21190b0d7cf00b9]*/
{
    // Run from Tools/lockbench/lockbench.py
    // Based on the WebKit lock benchmarks:
    // https://github.com/WebKit/WebKit/blob/main/Source/WTF/benchmarks/LockSpeedTest.cpp
    // See also https://webkit.org/blog/6161/locking-in-webkit/
    PyObject *thread_iters = NULL;
    PyObject *res = NULL;
    struct bench_data_locks *bench_data = NULL;
    struct bench_thread_data *thread_data = NULL;

    bench_data = PyMem_Calloc(num_locks, sizeof(*bench_data));
    if (bench_data == NULL) {
        PyErr_NoMemory();
        goto exit;
    }
    for (Py_ssize_t i = 0; i < num_locks; i++) {
        bench_data[i].work_inside = work_inside;
        bench_data[i].work_outside = work_outside;
        bench_data[i].num_acquisitions = num_acquisitions;
        bench_data[i].target_iters = total_iters;
    }

    thread_data = PyMem_Calloc(num_threads, sizeof(*thread_data));
    if (thread_data == NULL) {
        PyErr_NoMemory();
        goto exit;
    }
    thread_iters = PyList_New(num_threads);
    if (thread_iters == NULL) {
        goto exit;
    }

    PyTime_t start, end;
    if (PyTime_PerfCounter(&start) < 0) {
        goto exit;
    }

    for (Py_ssize_t i = 0; i < num_threads; i++) {
        thread_data[i].bench_data = &bench_data[i % num_locks];
        PyThread_start_new_thread(thread_benchmark_locks, &thread_data[i]);
    }

    if (total_iters == 0) {
        // Time-based mode: let the threads run for `time_ms` milliseconds
        pysleep(time_ms);
        for (Py_ssize_t i = 0; i < num_locks; i++) {
            _Py_atomic_store_int(&bench_data[i].stop, 1);
        }
    }

    // Wait for lock threads to finish
    for (Py_ssize_t i = 0; i < num_threads; i++) {
        PyEvent_Wait(&thread_data[i].done);
    }

    if (PyTime_PerfCounter(&end) < 0) {
        goto exit;
    }

    // Return the total number of acquisitions, the number of acquisitions
    // for each thread, and elapsed time.
    Py_ssize_t sum_iters = 0;
    for (Py_ssize_t i = 0; i < num_threads; i++) {
        PyObject *iter = PyLong_FromSsize_t(thread_data[i].iters);
        if (iter == NULL) {
            goto exit;
        }
        PyList_SET_ITEM(thread_iters, i, iter);
        sum_iters += thread_data[i].iters;
    }

    assert(end != start);
    PyTime_t elapsed_ns = end - start;
    double rate = sum_iters * 1e9 / elapsed_ns;
    res = Py_BuildValue("(dOL)", rate, thread_iters,
                        (long long)elapsed_ns);

exit:
    PyMem_Free(bench_data);
    PyMem_Free(thread_data);
    Py_XDECREF(thread_iters);
    return res;
}

static PyObject *
test_lock_benchmark(PyObject *module, PyObject *obj)
{
    // Just make sure the benchmark runs without crashing
    PyObject *res = _testinternalcapi_benchmark_locks_impl(
        module, 1, 1, 0, 100, 1, 0, 1);
    if (res == NULL) {
        return NULL;
    }
    Py_DECREF(res);
    Py_RETURN_NONE;
}

static int
init_maybe_fail(void *arg)
{
    int *counter = (int *)arg;
    (*counter)++;
    if (*counter < 5) {
        // failure
        return -1;
    }
    assert(*counter == 5);
    return 0;
}

static PyObject *
test_lock_once(PyObject *self, PyObject *obj)
{
    _PyOnceFlag once = {0};
    int counter = 0;
    for (int i = 0; i < 10; i++) {
        int res = _PyOnceFlag_CallOnce(&once, init_maybe_fail, &counter);
        if (i < 4) {
            assert(res == -1);
        }
        else {
            assert(res == 0);
            assert(counter == 5);
        }
    }
    Py_RETURN_NONE;
}

struct test_rwlock_data {
    Py_ssize_t nthreads;
    _PyRWMutex rw;
    PyEvent step1;
    PyEvent step2;
    PyEvent step3;
    PyEvent done;
};

static void
rdlock_thread(void *arg)
{
    struct test_rwlock_data *test_data = arg;

    // Acquire the lock in read mode
    _PyRWMutex_RLock(&test_data->rw);
    PyEvent_Wait(&test_data->step1);
    _PyRWMutex_RUnlock(&test_data->rw);

    _PyRWMutex_RLock(&test_data->rw);
    PyEvent_Wait(&test_data->step3);
    _PyRWMutex_RUnlock(&test_data->rw);

    if (_Py_atomic_add_ssize(&test_data->nthreads, -1) == 1) {
        _PyEvent_Notify(&test_data->done);
    }
}
static void
wrlock_thread(void *arg)
{
    struct test_rwlock_data *test_data = arg;

    // First acquire the lock in write mode
    _PyRWMutex_Lock(&test_data->rw);
    PyEvent_Wait(&test_data->step2);
    _PyRWMutex_Unlock(&test_data->rw);

    if (_Py_atomic_add_ssize(&test_data->nthreads, -1) == 1) {
        _PyEvent_Notify(&test_data->done);
    }
}

static void
wait_until(uintptr_t *ptr, uintptr_t value)
{
    // wait up to two seconds for *ptr == value
    int iters = 0;
    uintptr_t bits;
    do {
        pysleep(10);
        bits = _Py_atomic_load_uintptr(ptr);
        iters++;
    } while (bits != value && iters < 200);
}

static PyObject *
test_lock_rwlock(PyObject *self, PyObject *obj)
{
    struct test_rwlock_data test_data = {.nthreads = 3};

    _PyRWMutex_Lock(&test_data.rw);
    assert(test_data.rw.bits == 1);

    _PyRWMutex_Unlock(&test_data.rw);
    assert(test_data.rw.bits == 0);

    // Start two readers
    PyThread_start_new_thread(rdlock_thread, &test_data);
    PyThread_start_new_thread(rdlock_thread, &test_data);

    // wait up to two seconds for the threads to attempt to read-lock "rw"
    wait_until(&test_data.rw.bits, 8);
    assert(test_data.rw.bits == 8);

    // start writer (while readers hold lock)
    PyThread_start_new_thread(wrlock_thread, &test_data);
    wait_until(&test_data.rw.bits, 10);
    assert(test_data.rw.bits == 10);

    // readers release lock, writer should acquire it
    _PyEvent_Notify(&test_data.step1);
    wait_until(&test_data.rw.bits, 3);
    assert(test_data.rw.bits == 3);

    // writer releases lock, readers acquire it
    _PyEvent_Notify(&test_data.step2);
    wait_until(&test_data.rw.bits, 8);
    assert(test_data.rw.bits == 8);

    // readers release lock again
    _PyEvent_Notify(&test_data.step3);
    wait_until(&test_data.rw.bits, 0);
    assert(test_data.rw.bits == 0);

    PyEvent_Wait(&test_data.done);
    Py_RETURN_NONE;
}

static PyObject *
test_lock_recursive(PyObject *self, PyObject *obj)
{
    _PyRecursiveMutex m = (_PyRecursiveMutex){0};
    assert(!_PyRecursiveMutex_IsLockedByCurrentThread(&m));

    _PyRecursiveMutex_Lock(&m);
    assert(m.thread == PyThread_get_thread_ident_ex());
    assert(PyMutex_IsLocked(&m.mutex));
    assert(m.level == 0);

    _PyRecursiveMutex_Lock(&m);
    assert(m.level == 1);
    _PyRecursiveMutex_Unlock(&m);

    _PyRecursiveMutex_Unlock(&m);
    assert(m.thread == 0);
    assert(!PyMutex_IsLocked(&m.mutex));
    assert(m.level == 0);

    Py_RETURN_NONE;
}

static PyMethodDef test_methods[] = {
    {"test_lock_basic", test_lock_basic, METH_NOARGS},
    {"test_lock_two_threads", test_lock_two_threads, METH_NOARGS},
    {"test_lock_counter", test_lock_counter, METH_NOARGS},
    {"test_lock_counter_slow", test_lock_counter_slow, METH_NOARGS},
    _TESTINTERNALCAPI_BENCHMARK_LOCKS_METHODDEF
    {"test_lock_benchmark", test_lock_benchmark, METH_NOARGS},
    {"test_lock_once", test_lock_once, METH_NOARGS},
    {"test_lock_rwlock", test_lock_rwlock, METH_NOARGS},
    {"test_lock_recursive", test_lock_recursive, METH_NOARGS},
    {NULL, NULL} /* sentinel */
};

int
_PyTestInternalCapi_Init_Lock(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }
    return 0;
}
