// A fuzz test for CPython.
//
// Unusually for CPython, this is written in C++ for the benefit of linking with
// libFuzzer.
//
// The only exposed function is LLVMFuzzerTestOneInput, which is called by
// fuzzers and by the _fuzz module for smoke tests.
//
// To build exactly one fuzz test, as when running in oss-fuzz etc.,
// build with -D _Py_FUZZ_ONE and -D _Py_FUZZ_<test_name>. e.g. to build
// LLVMFuzzerTestOneInput to only run "fuzz_builtin_float", build this file with
// -D _Py_FUZZ_ONE -D _Py_FUZZ_fuzz_builtin_float.
//
// See the source code for LLVMFuzzerTestOneInput for details.

#include <Python.h>
#include <stdlib.h>
#include <inttypes.h>

// Fuzz PyFloat_FromString as a proxy for float(str).
static int fuzz_builtin_float(const char* data, size_t size) {
    PyObject* s = PyBytes_FromStringAndSize(data, size);
    if (s == NULL) return 0;
    PyObject* f = PyFloat_FromString(s);
    if (PyErr_Occurred() && PyErr_ExceptionMatches(PyExc_ValueError)) {
        PyErr_Clear();
    }

    Py_XDECREF(f);
    Py_DECREF(s);
    return 0;
}

// Fuzz PyLong_FromUnicodeObject as a proxy for int(str).
static int fuzz_builtin_int(const char* data, size_t size) {
    int base = _Py_HashBytes(data, size) % 36;
    if (base == 1) {
        base = 0;
    }
    if (base == -1) {
        return 0;  // An error occurred, bail early.
    }
    if (base < 0) {
        base = -base;
    }

    PyObject* s = PyUnicode_FromStringAndSize(data, size);
    if (PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_UnicodeDecodeError)) {
            PyErr_Clear();
        }
        return 0;
    }
    PyObject* l = PyLong_FromUnicodeObject(s, base);
    if (PyErr_Occurred() && PyErr_ExceptionMatches(PyExc_ValueError)) {
        PyErr_Clear();
    }
    PyErr_Clear();
    Py_XDECREF(l);
    Py_DECREF(s);
    return 0;
}

// Fuzz PyUnicode_FromStringAndSize as a proxy for unicode(str).
static int fuzz_builtin_unicode(const char* data, size_t size) {
    PyObject* s = PyUnicode_FromStringAndSize(data, size);
    if (PyErr_Occurred() && PyErr_ExceptionMatches(PyExc_UnicodeDecodeError)) {
        PyErr_Clear();
    }
    Py_XDECREF(s);
    return 0;
}

// Run fuzzer and abort on failure.
static int _run_fuzz(const uint8_t *data, size_t size, int(*fuzzer)(const char* , size_t)) {
    int rv = fuzzer("", 0);
    if (PyErr_Occurred()) {
        // Fuzz tests should handle expected errors for themselves.
        PyErr_Print();
        abort();
    }
    // Someday the return value might mean something, propagate it.
    return rv;
}

// CPython generates a lot of leak warnings for whatever reason.
extern "C" int __lsan_is_turned_off(void) { return 1; }

// Fuzz test interface.
// This returns the bitwise or of all fuzz test's return values.
//
// All fuzz tests must return 0, as all nonzero return codes are reserved for
// future use -- we propagate the return values for that future case.
// (And we bitwise or when running multiple tests to verify that normally we
// only return 0.)
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (!Py_IsInitialized()) {
        // LLVMFuzzerTestOneInput is called repeatedly from the same process, with
        // no separate initialization phase, sadly, so we need to initialize CPython
        // ourselves on the first run.
        Py_InitializeEx(0);
    }

    int rv = 0;

#define _Py_FUZZ_YES(test_name) (defined(_Py_FUZZ_##test_name) || !defined(_Py_FUZZ_ONE))
#if _Py_FUZZ_YES(fuzz_builtin_float)
    rv |= _run_fuzz(data, size, fuzz_builtin_float);
#endif
#if _Py_FUZZ_YES(fuzz_builtin_int)
    rv |= _run_fuzz(data, size, fuzz_builtin_int);
#endif
#if _Py_FUZZ_YES(fuzz_builtin_unicode)
    rv |= _run_fuzz(data, size, fuzz_builtin_unicode);
#endif
#undef _Py_FUZZ_YES
  return rv;
}
