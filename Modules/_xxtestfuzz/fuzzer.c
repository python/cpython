/* A fuzz test for CPython.

  The only exposed function is LLVMFuzzerTestOneInput, which is called by
  fuzzers and by the _fuzz module for smoke tests.

  To build exactly one fuzz test, as when running in oss-fuzz etc.,
  build with -D _Py_FUZZ_ONE and -D _Py_FUZZ_<test_name>. e.g. to build
  LLVMFuzzerTestOneInput to only run "fuzz_builtin_float", build this file with
      -D _Py_FUZZ_ONE -D _Py_FUZZ_fuzz_builtin_float.

  See the source code for LLVMFuzzerTestOneInput for details. */

#ifndef Py_BUILD_CORE
#  define Py_BUILD_CORE 1
#endif

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
    int base = Py_HashBuffer(data, size) % 37;
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


PyObject* struct_unpack_method = NULL;
PyObject* struct_error = NULL;
/* Called by LLVMFuzzerTestOneInput for initialization */
static int init_struct_unpack(void) {
    /* Import struct.unpack */
    PyObject* struct_module = PyImport_ImportModule("struct");
    if (struct_module == NULL) {
        return 0;
    }
    struct_error = PyObject_GetAttrString(struct_module, "error");
    if (struct_error == NULL) {
        return 0;
    }
    struct_unpack_method = PyObject_GetAttrString(struct_module, "unpack");
    return struct_unpack_method != NULL;
}
/* Fuzz struct.unpack(x, y) */
static int fuzz_struct_unpack(const char* data, size_t size) {
    /* Everything up to the first null byte is considered the
       format. Everything after is the buffer */
    const char* first_null = memchr(data, '\0', size);
    if (first_null == NULL) {
        return 0;
    }

    size_t format_length = first_null - data;
    size_t buffer_length = size - format_length - 1;

    PyObject* pattern = PyBytes_FromStringAndSize(data, format_length);
    if (pattern == NULL) {
        return 0;
    }
    PyObject* buffer = PyBytes_FromStringAndSize(first_null + 1, buffer_length);
    if (buffer == NULL) {
        Py_DECREF(pattern);
        return 0;
    }

    PyObject* unpacked = PyObject_CallFunctionObjArgs(
        struct_unpack_method, pattern, buffer, NULL);
    /* Ignore any overflow errors, these are easily triggered accidentally */
    if (unpacked == NULL && PyErr_ExceptionMatches(PyExc_OverflowError)) {
        PyErr_Clear();
    }
    /* The pascal format string will throw a negative size when passing 0
       like: struct.unpack('0p', b'') */
    if (unpacked == NULL && PyErr_ExceptionMatches(PyExc_SystemError)) {
        PyErr_Clear();
    }
    /* Ignore any struct.error exceptions, these can be caused by invalid
       formats or incomplete buffers both of which are common. */
    if (unpacked == NULL && PyErr_ExceptionMatches(struct_error)) {
        PyErr_Clear();
    }

    Py_XDECREF(unpacked);
    Py_DECREF(pattern);
    Py_DECREF(buffer);
    return 0;
}


#define MAX_JSON_TEST_SIZE 0x100000

PyObject* json_loads_method = NULL;
/* Called by LLVMFuzzerTestOneInput for initialization */
static int init_json_loads(void) {
    /* Import json.loads */
    PyObject* json_module = PyImport_ImportModule("json");
    if (json_module == NULL) {
        return 0;
    }
    json_loads_method = PyObject_GetAttrString(json_module, "loads");
    return json_loads_method != NULL;
}
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
    PyObject* parsed = PyObject_CallOneArg(json_loads_method, input_bytes);
    if (parsed == NULL) {
        /* Ignore ValueError as the fuzzer will more than likely
           generate some invalid json and values */
        if (PyErr_ExceptionMatches(PyExc_ValueError) ||
        /* Ignore RecursionError as the fuzzer generates long sequences of
           arrays such as `[[[...` */
            PyErr_ExceptionMatches(PyExc_RecursionError) ||
        /* Ignore unicode errors, invalid byte sequences are common */
            PyErr_ExceptionMatches(PyExc_UnicodeDecodeError)
        ) {
            PyErr_Clear();
        }
    }
    Py_DECREF(input_bytes);
    Py_XDECREF(parsed);
    return 0;
}

#define MAX_RE_TEST_SIZE 0x10000

PyObject* re_compile_method = NULL;
PyObject* re_error_exception = NULL;
int RE_FLAG_DEBUG = 0;
/* Called by LLVMFuzzerTestOneInput for initialization */
static int init_sre_compile(void) {
    /* Import sre_compile.compile and sre.error */
    PyObject* re_module = PyImport_ImportModule("re");
    if (re_module == NULL) {
        return 0;
    }
    re_compile_method = PyObject_GetAttrString(re_module, "compile");
    if (re_compile_method == NULL) {
        return 0;
    }

    re_error_exception = PyObject_GetAttrString(re_module, "error");
    if (re_error_exception == NULL) {
        return 0;
    }
    PyObject* debug_flag = PyObject_GetAttrString(re_module, "DEBUG");
    if (debug_flag == NULL) {
        return 0;
    }
    RE_FLAG_DEBUG = PyLong_AsLong(debug_flag);
    return 1;
}
/* Fuzz re.compile(x) */
static int fuzz_sre_compile(const char* data, size_t size) {
    /* Ignore really long regex patterns that will timeout the fuzzer */
    if (size > MAX_RE_TEST_SIZE) {
        return 0;
    }
    /* We treat the first 2 bytes of the input as a number for the flags */
    if (size < 2) {
        return 0;
    }
    uint16_t flags = ((uint16_t*) data)[0];
    /* We remove the SRE_FLAG_DEBUG if present. This is because it
       prints to stdout which greatly decreases fuzzing speed */
    flags &= ~RE_FLAG_DEBUG;

    /* Pull the pattern from the remaining bytes */
    PyObject* pattern_bytes = PyBytes_FromStringAndSize(data + 2, size - 2);
    if (pattern_bytes == NULL) {
        return 0;
    }
    PyObject* flags_obj = PyLong_FromUnsignedLong(flags);
    if (flags_obj == NULL) {
        Py_DECREF(pattern_bytes);
        return 0;
    }

    /* compiled = re.compile(data[2:], data[0:2] */
    PyObject* compiled = PyObject_CallFunctionObjArgs(
        re_compile_method, pattern_bytes, flags_obj, NULL);
    /* Ignore ValueError as the fuzzer will more than likely
       generate some invalid combination of flags */
    if (compiled == NULL && PyErr_ExceptionMatches(PyExc_ValueError)) {
        PyErr_Clear();
    }
    /* Ignore some common errors thrown by sre_parse:
       Overflow, Assertion, Recursion and Index */
    if (compiled == NULL && (PyErr_ExceptionMatches(PyExc_OverflowError) ||
                             PyErr_ExceptionMatches(PyExc_AssertionError) ||
                             PyErr_ExceptionMatches(PyExc_RecursionError) ||
                             PyErr_ExceptionMatches(PyExc_IndexError))
    ) {
        PyErr_Clear();
    }
    /* Ignore re.error */
    if (compiled == NULL && PyErr_ExceptionMatches(re_error_exception)) {
        PyErr_Clear();
    }

    Py_DECREF(pattern_bytes);
    Py_DECREF(flags_obj);
    Py_XDECREF(compiled);
    return 0;
}

/* Some random patterns used to test re.match.
   Be careful not to add catostraphically slow regexes here, we want to
   exercise the matching code without causing timeouts.*/
static const char* regex_patterns[] = {
    ".", "^", "abc", "abc|def", "^xxx$", "\\b", "()", "[a-zA-Z0-9]",
    "abc+", "[^A-Z]", "[x]", "(?=)", "a{z}", "a+b", "a*?", "a??", "a+?",
    "{}", "a{,}", "{", "}", "^\\(*\\d{3}\\)*( |-)*\\d{3}( |-)*\\d{4}$",
    "(?:a*)*", "a{1,2}?"
};
const size_t NUM_PATTERNS = sizeof(regex_patterns) / sizeof(regex_patterns[0]);
PyObject** compiled_patterns = NULL;
/* Called by LLVMFuzzerTestOneInput for initialization */
static int init_sre_match(void) {
    PyObject* re_module = PyImport_ImportModule("re");
    if (re_module == NULL) {
        return 0;
    }
    compiled_patterns = (PyObject**) PyMem_RawMalloc(
        sizeof(PyObject*) * NUM_PATTERNS);
    if (compiled_patterns == NULL) {
        PyErr_NoMemory();
        return 0;
    }

    /* Precompile all the regex patterns on the first run for faster fuzzing */
    for (size_t i = 0; i < NUM_PATTERNS; i++) {
        PyObject* compiled = PyObject_CallMethod(
            re_module, "compile", "y", regex_patterns[i]);
        /* Bail if any of the patterns fail to compile */
        if (compiled == NULL) {
            return 0;
        }
        compiled_patterns[i] = compiled;
    }
    return 1;
}
/* Fuzz re.match(x) */
static int fuzz_sre_match(const char* data, size_t size) {
    if (size < 1 || size > MAX_RE_TEST_SIZE) {
        return 0;
    }
    /* Use the first byte as a uint8_t specifying the index of the
       regex to use */
    unsigned char idx = (unsigned char) data[0];
    idx = idx % NUM_PATTERNS;

    /* Pull the string to match from the remaining bytes */
    PyObject* to_match = PyBytes_FromStringAndSize(data + 1, size - 1);
    if (to_match == NULL) {
        return 0;
    }

    PyObject* pattern = compiled_patterns[idx];
    PyObject* match_callable = PyObject_GetAttrString(pattern, "match");

    PyObject* matches = PyObject_CallOneArg(match_callable, to_match);

    Py_XDECREF(matches);
    Py_DECREF(match_callable);
    Py_DECREF(to_match);
    return 0;
}

#define MAX_CSV_TEST_SIZE 0x100000
PyObject* csv_module = NULL;
PyObject* csv_error = NULL;
/* Called by LLVMFuzzerTestOneInput for initialization */
static int init_csv_reader(void) {
    /* Import csv and csv.Error */
    csv_module = PyImport_ImportModule("csv");
    if (csv_module == NULL) {
        return 0;
    }
    csv_error = PyObject_GetAttrString(csv_module, "Error");
    return csv_error != NULL;
}
/* Fuzz csv.reader([x]) */
static int fuzz_csv_reader(const char* data, size_t size) {
    if (size < 1 || size > MAX_CSV_TEST_SIZE) {
        return 0;
    }
    /* Ignore non null-terminated strings since _csv can't handle
       embedded nulls */
    if (memchr(data, '\0', size) == NULL) {
        return 0;
    }

    PyObject* s = PyUnicode_FromString(data);
    /* Ignore exceptions until we have a valid string */
    if (s == NULL) {
        PyErr_Clear();
        return 0;
    }

    /* Split on \n so we can test multiple lines */
    PyObject* lines = PyObject_CallMethod(s, "split", "s", "\n");
    if (lines == NULL) {
        Py_DECREF(s);
        return 0;
    }

    PyObject* reader = PyObject_CallMethod(csv_module, "reader", "N", lines);
    if (reader) {
        /* Consume all of the reader as an iterator */
        PyObject* parsed_line;
        while ((parsed_line = PyIter_Next(reader))) {
            Py_DECREF(parsed_line);
        }
    }

    /* Ignore csv.Error because we're probably going to generate
       some bad files (embedded new-lines, unterminated quotes etc) */
    if (PyErr_ExceptionMatches(csv_error)) {
        PyErr_Clear();
    }

    Py_XDECREF(reader);
    Py_DECREF(s);
    return 0;
}

#define MAX_AST_LITERAL_EVAL_TEST_SIZE 0x100000
PyObject* ast_literal_eval_method = NULL;
/* Called by LLVMFuzzerTestOneInput for initialization */
static int init_ast_literal_eval(void) {
    PyObject* ast_module = PyImport_ImportModule("ast");
    if (ast_module == NULL) {
        return 0;
    }
    ast_literal_eval_method = PyObject_GetAttrString(ast_module, "literal_eval");
    return ast_literal_eval_method != NULL;
}
/* Fuzz ast.literal_eval(x) */
static int fuzz_ast_literal_eval(const char* data, size_t size) {
    if (size > MAX_AST_LITERAL_EVAL_TEST_SIZE) {
        return 0;
    }
    /* Ignore non null-terminated strings since ast can't handle
       embedded nulls */
    if (memchr(data, '\0', size) == NULL) {
        return 0;
    }

    PyObject* s = PyUnicode_FromString(data);
    /* Ignore exceptions until we have a valid string */
    if (s == NULL) {
        PyErr_Clear();
        return 0;
    }

    PyObject* literal = PyObject_CallOneArg(ast_literal_eval_method, s);
    /* Ignore some common errors thrown by ast.literal_eval */
    if (literal == NULL && (PyErr_ExceptionMatches(PyExc_ValueError) ||
                            PyErr_ExceptionMatches(PyExc_TypeError) ||
                            PyErr_ExceptionMatches(PyExc_SyntaxError) ||
                            PyErr_ExceptionMatches(PyExc_MemoryError) ||
                            PyErr_ExceptionMatches(PyExc_RecursionError))
    ) {
        PyErr_Clear();
    }

    Py_XDECREF(literal);
    Py_DECREF(s);
    return 0;
}

#define MAX_ELEMENTTREE_PARSEWHOLE_TEST_SIZE 0x100000
PyObject* xmlparser_type = NULL;
PyObject* bytesio_type = NULL;
/* Called by LLVMFuzzerTestOneInput for initialization */
static int init_elementtree_parsewhole(void) {
    PyObject* elementtree_module = PyImport_ImportModule("_elementtree");
    if (elementtree_module == NULL) {
        return 0;
    }
    xmlparser_type = PyObject_GetAttrString(elementtree_module, "XMLParser");
    Py_DECREF(elementtree_module);
    if (xmlparser_type == NULL) {
        return 0;
    }


    PyObject* io_module = PyImport_ImportModule("io");
    if (io_module == NULL) {
        return 0;
    }
    bytesio_type = PyObject_GetAttrString(io_module, "BytesIO");
    Py_DECREF(io_module);
    if (bytesio_type == NULL) {
        return 0;
    }

    return 1;
}
/* Fuzz _elementtree.XMLParser._parse_whole(x) */
static int fuzz_elementtree_parsewhole(const char* data, size_t size) {
    if (size > MAX_ELEMENTTREE_PARSEWHOLE_TEST_SIZE) {
        return 0;
    }

    PyObject *input = PyObject_CallFunction(bytesio_type, "y#", data, (Py_ssize_t)size);
    if (input == NULL) {
        assert(PyErr_Occurred());
        PyErr_Print();
        abort();
    }

    PyObject *xmlparser_instance = PyObject_CallObject(xmlparser_type, NULL);
    if (xmlparser_instance == NULL) {
        assert(PyErr_Occurred());
        PyErr_Print();
        abort();
    }

    PyObject *result = PyObject_CallMethod(xmlparser_instance, "_parse_whole", "O", input);
    if (result == NULL) {
        /* Ignore exception here, which can be caused by invalid XML input */
        PyErr_Clear();
    } else {
        Py_DECREF(result);
    }

    Py_DECREF(xmlparser_instance);
    Py_DECREF(input);

    return 0;
}

#define MAX_PYCOMPILE_TEST_SIZE 16384

static const int start_vals[] = {Py_eval_input, Py_single_input, Py_file_input};
const size_t NUM_START_VALS = sizeof(start_vals) / sizeof(start_vals[0]);

static const int optimize_vals[] = {-1, 0, 1, 2};
const size_t NUM_OPTIMIZE_VALS = sizeof(optimize_vals) / sizeof(optimize_vals[0]);

/* Fuzz `PyCompileStringExFlags` using a variety of input parameters.
 * That function is essentially behind the `compile` builtin */
static int fuzz_pycompile(const char* data, size_t size) {
    // Ignore overly-large inputs, and account for a NUL terminator
    if (size > MAX_PYCOMPILE_TEST_SIZE - 1) {
        return 0;
    }

    // Need 2 bytes for parameter selection
    if (size < 2) {
        return 0;
    }

    // Use first byte to determine element of `start_vals` to use
    unsigned char start_idx = (unsigned char) data[0];
    int start = start_vals[start_idx % NUM_START_VALS];

    // Use second byte to determine element of `optimize_vals` to use
    unsigned char optimize_idx = (unsigned char) data[1];
    int optimize = optimize_vals[optimize_idx % NUM_OPTIMIZE_VALS];

    char pycompile_scratch[MAX_PYCOMPILE_TEST_SIZE];

    // Create a NUL-terminated C string from the remaining input
    memcpy(pycompile_scratch, data + 2, size - 2);
    // Put a NUL terminator just after the copied data. (Space was reserved already.)
    pycompile_scratch[size - 2] = '\0';

    // XXX: instead of always using NULL for the `flags` value to
    // `Py_CompileStringExFlags`, there are many flags that conditionally
    // change parser behavior:
    //
    //     #define PyCF_TYPE_COMMENTS 0x1000
    //     #define PyCF_ALLOW_TOP_LEVEL_AWAIT 0x2000
    //     #define PyCF_ONLY_AST 0x0400
    //
    // It would be good to test various combinations of these, too.
    PyCompilerFlags *flags = NULL;

    PyObject *result = Py_CompileStringExFlags(pycompile_scratch, "<fuzz input>", start, flags, optimize);
    if (result == NULL) {
        /* Compilation failed, most likely from a syntax error. If it was a
           SystemError we abort. There's no non-bug reason to raise a
           SystemError. */
        if (PyErr_Occurred() && PyErr_ExceptionMatches(PyExc_SystemError)) {
            PyErr_Print();
            abort();
        }
        PyErr_Clear();
    } else {
        Py_DECREF(result);
    }

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
    PyConfig config;
    PyConfig_InitPythonConfig(&config);
    config.install_signal_handlers = 0;
    /* Raise the limit above the default allows exercising larger things
     * now that we fall back to the _pylong module for large values. */
    config.int_max_str_digits = 8086;
    PyStatus status;
    status = PyConfig_SetBytesString(&config, &config.program_name, *argv[0]);
    if (PyStatus_Exception(status)) {
        goto fail;
    }

    status = Py_InitializeFromConfig(&config);
    if (PyStatus_Exception(status)) {
        goto fail;
    }
    PyConfig_Clear(&config);

    return 0;

fail:
    PyConfig_Clear(&config);
    Py_ExitStatusException(status);
}

/* Fuzz test interface.
   This returns the bitwise or of all fuzz test's return values.

   All fuzz tests must return 0, as all nonzero return codes are reserved for
   future use -- we propagate the return values for that future case.
   (And we bitwise or when running multiple tests to verify that normally we
   only return 0.) */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    assert(Py_IsInitialized());

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
#if !defined(_Py_FUZZ_ONE) || defined(_Py_FUZZ_fuzz_struct_unpack)
    static int STRUCT_UNPACK_INITIALIZED = 0;
    if (!STRUCT_UNPACK_INITIALIZED && !init_struct_unpack()) {
        PyErr_Print();
        abort();
    } else {
        STRUCT_UNPACK_INITIALIZED = 1;
    }
    rv |= _run_fuzz(data, size, fuzz_struct_unpack);
#endif
#if !defined(_Py_FUZZ_ONE) || defined(_Py_FUZZ_fuzz_json_loads)
    static int JSON_LOADS_INITIALIZED = 0;
    if (!JSON_LOADS_INITIALIZED && !init_json_loads()) {
        PyErr_Print();
        abort();
    } else {
        JSON_LOADS_INITIALIZED = 1;
    }

    rv |= _run_fuzz(data, size, fuzz_json_loads);
#endif
#if !defined(_Py_FUZZ_ONE) || defined(_Py_FUZZ_fuzz_sre_compile)
    static int SRE_COMPILE_INITIALIZED = 0;
    if (!SRE_COMPILE_INITIALIZED && !init_sre_compile()) {
        PyErr_Print();
        abort();
    } else {
        SRE_COMPILE_INITIALIZED = 1;
    }

    if (SRE_COMPILE_INITIALIZED) {
        rv |= _run_fuzz(data, size, fuzz_sre_compile);
    }
#endif
#if !defined(_Py_FUZZ_ONE) || defined(_Py_FUZZ_fuzz_sre_match)
    static int SRE_MATCH_INITIALIZED = 0;
    if (!SRE_MATCH_INITIALIZED && !init_sre_match()) {
        PyErr_Print();
        abort();
    } else {
        SRE_MATCH_INITIALIZED = 1;
    }

    rv |= _run_fuzz(data, size, fuzz_sre_match);
#endif
#if !defined(_Py_FUZZ_ONE) || defined(_Py_FUZZ_fuzz_csv_reader)
    static int CSV_READER_INITIALIZED = 0;
    if (!CSV_READER_INITIALIZED && !init_csv_reader()) {
        PyErr_Print();
        abort();
    } else {
        CSV_READER_INITIALIZED = 1;
    }

    rv |= _run_fuzz(data, size, fuzz_csv_reader);
#endif
#if !defined(_Py_FUZZ_ONE) || defined(_Py_FUZZ_fuzz_ast_literal_eval)
    static int AST_LITERAL_EVAL_INITIALIZED = 0;
    if (!AST_LITERAL_EVAL_INITIALIZED && !init_ast_literal_eval()) {
        PyErr_Print();
        abort();
    } else {
        AST_LITERAL_EVAL_INITIALIZED = 1;
    }

    rv |= _run_fuzz(data, size, fuzz_ast_literal_eval);
#endif
#if !defined(_Py_FUZZ_ONE) || defined(_Py_FUZZ_fuzz_elementtree_parsewhole)
    static int ELEMENTTREE_PARSEWHOLE_INITIALIZED = 0;
    if (!ELEMENTTREE_PARSEWHOLE_INITIALIZED && !init_elementtree_parsewhole()) {
        PyErr_Print();
        abort();
    } else {
        ELEMENTTREE_PARSEWHOLE_INITIALIZED = 1;
    }

    rv |= _run_fuzz(data, size, fuzz_elementtree_parsewhole);
#endif
#if !defined(_Py_FUZZ_ONE) || defined(_Py_FUZZ_fuzz_pycompile)
    rv |= _run_fuzz(data, size, fuzz_pycompile);
#endif
  return rv;
}
