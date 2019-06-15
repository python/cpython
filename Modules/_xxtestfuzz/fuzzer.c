/* A fuzz test for CPython.

  The only exposed function is LLVMFuzzerTestOneInput, which is called by
  fuzzers and by the _fuzz module for smoke tests.

  To build exactly one fuzz test, as when running in oss-fuzz etc.,
  build with -D _Py_FUZZ_ONE and -D _Py_FUZZ_<test_name>. e.g. to build
  LLVMFuzzerTestOneInput to only run "fuzz_builtin_float", build this file with
      -D _Py_FUZZ_ONE -D _Py_FUZZ_fuzz_builtin_float.

  See the source code for LLVMFuzzerTestOneInput for details. */

#include <Python.h>
#include <stdlib.h>
#include <inttypes.h>

/*  Fuzz PyFloat_FromString as a proxy for float(str). */
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

#define MAX_INT_TEST_SIZE 0x10000

/* Fuzz PyLong_FromUnicodeObject as a proxy for int(str). */
static int fuzz_builtin_int(const char* data, size_t size) {
    /* Ignore test cases with very long ints to avoid timeouts
       int("9" * 1000000) is not a very interesting test caase */
    if (size > MAX_INT_TEST_SIZE) {
        return 0;
    }
    /* Pick a random valid base. (When the fuzzed function takes extra
       parameters, it's somewhat normal to hash the input to generate those
       parameters. We want to exercise all code paths, so we do so here.) */
    int base = _Py_HashBytes(data, size) % 37;
    if (base == 1) {
        // 1 is the only number between 0 and 36 that is not a valid base.
        base = 0;
    }
    if (base == -1) {
        return 0;  // An error occurred, bail early.
    }
    if (base < 0) {
        base = -base;
    }

    PyObject* s = PyUnicode_FromStringAndSize(data, size);
    if (s == NULL) {
        if (PyErr_ExceptionMatches(PyExc_UnicodeDecodeError)) {
            PyErr_Clear();
        }
        return 0;
    }
    PyObject* l = PyLong_FromUnicodeObject(s, base);
    if (l == NULL && PyErr_ExceptionMatches(PyExc_ValueError)) {
        PyErr_Clear();
    }
    PyErr_Clear();
    Py_XDECREF(l);
    Py_DECREF(s);
    return 0;
}

/* Fuzz PyUnicode_FromStringAndSize as a proxy for unicode(str). */
static int fuzz_builtin_unicode(const char* data, size_t size) {
    PyObject* s = PyUnicode_FromStringAndSize(data, size);
    if (s == NULL && PyErr_ExceptionMatches(PyExc_UnicodeDecodeError)) {
        PyErr_Clear();
    }
    Py_XDECREF(s);
    return 0;
}

#define MAX_JSON_TEST_SIZE 0x10000

/* Initialized in LLVMFuzzerTestOneInput */
PyObject* json_loads_method = NULL;
/* Fuzz json.loads(x) */
static int fuzz_json_loads(const char* data, size_t size) {
    /* Since python supports arbitrarily large ints in JSON,
       long inputs can lead to timeouts on boring inputs like
       `json.loads("9" * 100000)` */
    if (size > MAX_JSON_TEST_SIZE) {
        return 0;
    }
    PyObject* input_bytes = PyBytes_FromStringAndSize(data, size);
    if (input_bytes == NULL) {
        return 0;
    }
    PyObject* parsed = PyObject_CallFunctionObjArgs(json_loads_method, input_bytes, NULL);
    /* Ignore ValueError as the fuzzer will more than likely
       generate some invalid json and values */
    if (parsed == NULL && PyErr_ExceptionMatches(PyExc_ValueError)) {
        PyErr_Clear();
    }
    /* Ignore RecursionError as the fuzzer generates long sequences of
       arrays such as `[[[...` */
    if (parsed == NULL && PyErr_ExceptionMatches(PyExc_RecursionError)) {
        PyErr_Clear();
    }
    /* Ignore unicode errors, invalid byte sequences are common */
    if (parsed == NULL && PyErr_ExceptionMatches(PyExc_UnicodeDecodeError)) {
        PyErr_Clear();
    }
    Py_DECREF(input_bytes);
    Py_XDECREF(parsed);
    return 0;
}

/* Run fuzzer and abort on failure. */
static int _run_fuzz(const uint8_t *data, size_t size, int(*fuzzer)(const char* , size_t)) {
    int rv = fuzzer((const char*) data, size);
    if (PyErr_Occurred()) {
        /* Fuzz tests should handle expected errors for themselves.
           This is last-ditch check in case they didn't. */
        PyErr_Print();
        abort();
    }
    /* Someday the return value might mean something, propagate it. */
    return rv;
}

/* CPython generates a lot of leak warnings for whatever reason. */
int __lsan_is_turned_off(void) { return 1; }


int LLVMFuzzerInitialize(int *argc, char ***argv) {
    wchar_t* wide_program_name = Py_DecodeLocale(*argv[0], NULL);
    Py_SetProgramName(wide_program_name);
    return 0;
}

/* Fuzz test interface.
   This returns the bitwise or of all fuzz test's return values.

   All fuzz tests must return 0, as all nonzero return codes are reserved for
   future use -- we propagate the return values for that future case.
   (And we bitwise or when running multiple tests to verify that normally we
   only return 0.) */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (!Py_IsInitialized()) {
        /* LLVMFuzzerTestOneInput is called repeatedly from the same process,
           with no separate initialization phase, sadly, so we need to
           initialize CPython ourselves on the first run. */
        Py_InitializeEx(0);
    }
#if !defined(_Py_FUZZ_ONE) || defined(_Py_FUZZ_fuzz_json_loads)
    if (json_loads_method == NULL) {
        PyObject* json_module = PyImport_ImportModule("json");
        json_loads_method = PyObject_GetAttrString(json_module, "loads");
    }
#endif

    int rv = 0;

#if !defined(_Py_FUZZ_ONE) || defined(_Py_FUZZ_fuzz_builtin_float)
    rv |= _run_fuzz(data, size, fuzz_builtin_float);
#endif
#if !defined(_Py_FUZZ_ONE) || defined(_Py_FUZZ_fuzz_builtin_int)
    rv |= _run_fuzz(data, size, fuzz_builtin_int);
#endif
#if !defined(_Py_FUZZ_ONE) || defined(_Py_FUZZ_fuzz_builtin_unicode)
    rv |= _run_fuzz(data, size, fuzz_builtin_unicode);
#endif
#if !defined(_Py_FUZZ_ONE) || defined(_Py_FUZZ_fuzz_json_loads)
    rv |= _run_fuzz(data, size, fuzz_json_loads);
#endif
  return rv;
}
