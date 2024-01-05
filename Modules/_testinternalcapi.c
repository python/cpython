/*
 * C Extension module to test Python internal C APIs (Include/internal).
 */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

/* Always enable assertions */
#undef NDEBUG

#include "Python.h"
#include "pycore_bitutils.h"      // _Py_bswap32()
#include "pycore_bytesobject.h"   // _PyBytes_Find()
#include "pycore_ceval.h"         // _PyEval_AddPendingCall()
#include "pycore_compile.h"       // _PyCompile_CodeGen()
#include "pycore_context.h"       // _PyContext_NewHamtForTests()
#include "pycore_dict.h"          // _PyDictOrValues_GetValues()
#include "pycore_fileutils.h"     // _Py_normpath()
#include "pycore_frame.h"         // _PyInterpreterFrame
#include "pycore_gc.h"            // PyGC_Head
#include "pycore_hashtable.h"     // _Py_hashtable_new()
#include "pycore_initconfig.h"    // _Py_GetConfigsAsDict()
#include "pycore_interp.h"        // _PyInterpreterState_GetConfigCopy()
#include "pycore_long.h"          // _PyLong_Sign()
#include "pycore_object.h"        // _PyObject_IsFreed()
#include "pycore_pathconfig.h"    // _PyPathConfig_ClearGlobal()
#include "pycore_pyerrors.h"      // _PyErr_ChainExceptions1()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_typeobject.h"    // _PyType_GetModuleName()

#include "interpreteridobject.h"  // PyInterpreterID_LookUp()

#include "clinic/_testinternalcapi.c.h"

// Include test definitions from _testinternalcapi/
#include "_testinternalcapi/parts.h"


#define MODULE_NAME "_testinternalcapi"


static PyObject *
_get_current_module(void)
{
    // We ensured it was imported in _run_script().
    PyObject *name = PyUnicode_FromString(MODULE_NAME);
    if (name == NULL) {
        return NULL;
    }
    PyObject *mod = PyImport_GetModule(name);
    Py_DECREF(name);
    if (mod == NULL) {
        return NULL;
    }
    assert(mod != Py_None);
    return mod;
}


/* module state *************************************************************/

typedef struct {
    PyObject *record_list;
} module_state;

static inline module_state *
get_module_state(PyObject *mod)
{
    assert(mod != NULL);
    module_state *state = PyModule_GetState(mod);
    assert(state != NULL);
    return state;
}

static int
traverse_module_state(module_state *state, visitproc visit, void *arg)
{
    Py_VISIT(state->record_list);
    return 0;
}

static int
clear_module_state(module_state *state)
{
    Py_CLEAR(state->record_list);
    return 0;
}


/* module functions *********************************************************/

/*[clinic input]
module _testinternalcapi
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=7bb583d8c9eb9a78]*/
static PyObject *
get_configs(PyObject *self, PyObject *Py_UNUSED(args))
{
    return _Py_GetConfigsAsDict();
}


static PyObject*
get_recursion_depth(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyThreadState *tstate = _PyThreadState_GET();

    return PyLong_FromLong(tstate->py_recursion_limit - tstate->py_recursion_remaining);
}


static PyObject*
get_c_recursion_remaining(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyThreadState *tstate = _PyThreadState_GET();
    return PyLong_FromLong(tstate->c_recursion_remaining);
}


static PyObject*
test_bswap(PyObject *self, PyObject *Py_UNUSED(args))
{
    uint16_t u16 = _Py_bswap16(UINT16_C(0x3412));
    if (u16 != UINT16_C(0x1234)) {
        PyErr_Format(PyExc_AssertionError,
                     "_Py_bswap16(0x3412) returns %u", u16);
        return NULL;
    }

    uint32_t u32 = _Py_bswap32(UINT32_C(0x78563412));
    if (u32 != UINT32_C(0x12345678)) {
        PyErr_Format(PyExc_AssertionError,
                     "_Py_bswap32(0x78563412) returns %lu", u32);
        return NULL;
    }

    uint64_t u64 = _Py_bswap64(UINT64_C(0xEFCDAB9078563412));
    if (u64 != UINT64_C(0x1234567890ABCDEF)) {
        PyErr_Format(PyExc_AssertionError,
                     "_Py_bswap64(0xEFCDAB9078563412) returns %llu", u64);
        return NULL;
    }

    Py_RETURN_NONE;
}


static int
check_popcount(uint32_t x, int expected)
{
    // Use volatile to prevent the compiler to optimize out the whole test
    volatile uint32_t u = x;
    int bits = _Py_popcount32(u);
    if (bits != expected) {
        PyErr_Format(PyExc_AssertionError,
                     "_Py_popcount32(%lu) returns %i, expected %i",
                     (unsigned long)x, bits, expected);
        return -1;
    }
    return 0;
}


static PyObject*
test_popcount(PyObject *self, PyObject *Py_UNUSED(args))
{
#define CHECK(X, RESULT) \
    do { \
        if (check_popcount(X, RESULT) < 0) { \
            return NULL; \
        } \
    } while (0)

    CHECK(0, 0);
    CHECK(1, 1);
    CHECK(0x08080808, 4);
    CHECK(0x10000001, 2);
    CHECK(0x10101010, 4);
    CHECK(0x10204080, 4);
    CHECK(0xDEADCAFE, 22);
    CHECK(0xFFFFFFFF, 32);
    Py_RETURN_NONE;

#undef CHECK
}


static int
check_bit_length(unsigned long x, int expected)
{
    // Use volatile to prevent the compiler to optimize out the whole test
    volatile unsigned long u = x;
    int len = _Py_bit_length(u);
    if (len != expected) {
        PyErr_Format(PyExc_AssertionError,
                     "_Py_bit_length(%lu) returns %i, expected %i",
                     x, len, expected);
        return -1;
    }
    return 0;
}


static PyObject*
test_bit_length(PyObject *self, PyObject *Py_UNUSED(args))
{
#define CHECK(X, RESULT) \
    do { \
        if (check_bit_length(X, RESULT) < 0) { \
            return NULL; \
        } \
    } while (0)

    CHECK(0, 0);
    CHECK(1, 1);
    CHECK(0x1000, 13);
    CHECK(0x1234, 13);
    CHECK(0x54321, 19);
    CHECK(0x7FFFFFFF, 31);
    CHECK(0xFFFFFFFF, 32);
    Py_RETURN_NONE;

#undef CHECK
}


#define TO_PTR(ch) ((void*)(uintptr_t)ch)
#define FROM_PTR(ptr) ((uintptr_t)ptr)
#define VALUE(key) (1 + ((int)(key) - 'a'))

static Py_uhash_t
hash_char(const void *key)
{
    char ch = (char)FROM_PTR(key);
    return ch;
}


static int
hashtable_cb(_Py_hashtable_t *table,
             const void *key_ptr, const void *value_ptr,
             void *user_data)
{
    int *count = (int *)user_data;
    char key = (char)FROM_PTR(key_ptr);
    int value = (int)FROM_PTR(value_ptr);
    assert(value == VALUE(key));
    *count += 1;
    return 0;
}


static PyObject*
test_hashtable(PyObject *self, PyObject *Py_UNUSED(args))
{
    _Py_hashtable_t *table = _Py_hashtable_new(hash_char,
                                               _Py_hashtable_compare_direct);
    if (table == NULL) {
        return PyErr_NoMemory();
    }

    // Using an newly allocated table must not crash
    assert(table->nentries == 0);
    assert(table->nbuckets > 0);
    assert(_Py_hashtable_get(table, TO_PTR('x')) == NULL);

    // Test _Py_hashtable_set()
    char key;
    for (key='a'; key <= 'z'; key++) {
        int value = VALUE(key);
        if (_Py_hashtable_set(table, TO_PTR(key), TO_PTR(value)) < 0) {
            _Py_hashtable_destroy(table);
            return PyErr_NoMemory();
        }
    }
    assert(table->nentries == 26);
    assert(table->nbuckets > table->nentries);

    // Test _Py_hashtable_get_entry()
    for (key='a'; key <= 'z'; key++) {
        _Py_hashtable_entry_t *entry = _Py_hashtable_get_entry(table, TO_PTR(key));
        assert(entry != NULL);
        assert(entry->key == TO_PTR(key));
        assert(entry->value == TO_PTR(VALUE(key)));
    }

    // Test _Py_hashtable_get()
    for (key='a'; key <= 'z'; key++) {
        void *value_ptr = _Py_hashtable_get(table, TO_PTR(key));
        assert((int)FROM_PTR(value_ptr) == VALUE(key));
    }

    // Test _Py_hashtable_steal()
    key = 'p';
    void *value_ptr = _Py_hashtable_steal(table, TO_PTR(key));
    assert((int)FROM_PTR(value_ptr) == VALUE(key));
    assert(table->nentries == 25);
    assert(_Py_hashtable_get_entry(table, TO_PTR(key)) == NULL);

    // Test _Py_hashtable_foreach()
    int count = 0;
    int res = _Py_hashtable_foreach(table, hashtable_cb, &count);
    assert(res == 0);
    assert(count == 25);

    // Test _Py_hashtable_clear()
    _Py_hashtable_clear(table);
    assert(table->nentries == 0);
    assert(table->nbuckets > 0);
    assert(_Py_hashtable_get(table, TO_PTR('x')) == NULL);

    _Py_hashtable_destroy(table);
    Py_RETURN_NONE;
}


static PyObject *
test_get_config(PyObject *Py_UNUSED(self), PyObject *Py_UNUSED(args))
{
    PyConfig config;
    PyConfig_InitIsolatedConfig(&config);
    if (_PyInterpreterState_GetConfigCopy(&config) < 0) {
        PyConfig_Clear(&config);
        return NULL;
    }
    PyObject *dict = _PyConfig_AsDict(&config);
    PyConfig_Clear(&config);
    return dict;
}


static PyObject *
test_set_config(PyObject *Py_UNUSED(self), PyObject *dict)
{
    PyConfig config;
    PyConfig_InitIsolatedConfig(&config);
    if (_PyConfig_FromDict(&config, dict) < 0) {
        goto error;
    }
    if (_PyInterpreterState_SetConfig(&config) < 0) {
        goto error;
    }
    PyConfig_Clear(&config);
    Py_RETURN_NONE;

error:
    PyConfig_Clear(&config);
    return NULL;
}


static PyObject *
test_reset_path_config(PyObject *Py_UNUSED(self), PyObject *Py_UNUSED(arg))
{
    _PyPathConfig_ClearGlobal();
    Py_RETURN_NONE;
}


static int
check_edit_cost(const char *a, const char *b, Py_ssize_t expected)
{
    int ret = -1;
    PyObject *a_obj = NULL;
    PyObject *b_obj = NULL;

    a_obj = PyUnicode_FromString(a);
    if (a_obj == NULL) {
        goto exit;
    }
    b_obj = PyUnicode_FromString(b);
    if (b_obj == NULL) {
        goto exit;
    }
    Py_ssize_t result = _Py_UTF8_Edit_Cost(a_obj, b_obj, -1);
    if (result != expected) {
        PyErr_Format(PyExc_AssertionError,
                     "Edit cost from '%s' to '%s' returns %zd, expected %zd",
                     a, b, result, expected);
        goto exit;
    }
    // Check that smaller max_edits thresholds are exceeded.
    Py_ssize_t max_edits = result;
    while (max_edits > 0) {
        max_edits /= 2;
        Py_ssize_t result2 = _Py_UTF8_Edit_Cost(a_obj, b_obj, max_edits);
        if (result2 <= max_edits) {
            PyErr_Format(PyExc_AssertionError,
                         "Edit cost from '%s' to '%s' (threshold %zd) "
                         "returns %zd, expected greater than %zd",
                         a, b, max_edits, result2, max_edits);
            goto exit;
        }
    }
    // Check that bigger max_edits thresholds don't change anything
    Py_ssize_t result3 = _Py_UTF8_Edit_Cost(a_obj, b_obj, result * 2 + 1);
    if (result3 != result) {
        PyErr_Format(PyExc_AssertionError,
                     "Edit cost from '%s' to '%s' (threshold %zd) "
                     "returns %zd, expected %zd",
                     a, b, result * 2, result3, result);
        goto exit;
    }
    ret = 0;
exit:
    Py_XDECREF(a_obj);
    Py_XDECREF(b_obj);
    return ret;
}

static PyObject *
test_edit_cost(PyObject *self, PyObject *Py_UNUSED(args))
{
    #define CHECK(a, b, n) do {              \
        if (check_edit_cost(a, b, n) < 0) {  \
            return NULL;                     \
        }                                    \
    } while (0)                              \

    CHECK("", "", 0);
    CHECK("", "a", 2);
    CHECK("a", "A", 1);
    CHECK("Apple", "Aple", 2);
    CHECK("Banana", "B@n@n@", 6);
    CHECK("Cherry", "Cherry!", 2);
    CHECK("---0---", "------", 2);
    CHECK("abc", "y", 6);
    CHECK("aa", "bb", 4);
    CHECK("aaaaa", "AAAAA", 5);
    CHECK("wxyz", "wXyZ", 2);
    CHECK("wxyz", "wXyZ123", 8);
    CHECK("Python", "Java", 12);
    CHECK("Java", "C#", 8);
    CHECK("AbstractFoobarManager", "abstract_foobar_manager", 3+2*2);
    CHECK("CPython", "PyPy", 10);
    CHECK("CPython", "pypy", 11);
    CHECK("AttributeError", "AttributeErrop", 2);
    CHECK("AttributeError", "AttributeErrorTests", 10);

    #undef CHECK
    Py_RETURN_NONE;
}


static int
check_bytes_find(const char *haystack0, const char *needle0,
                 int offset, Py_ssize_t expected)
{
    Py_ssize_t len_haystack = strlen(haystack0);
    Py_ssize_t len_needle = strlen(needle0);
    Py_ssize_t result_1 = _PyBytes_Find(haystack0, len_haystack,
                                        needle0, len_needle, offset);
    if (result_1 != expected) {
        PyErr_Format(PyExc_AssertionError,
                    "Incorrect result_1: '%s' in '%s' (offset=%zd)",
                    needle0, haystack0, offset);
        return -1;
    }
    // Allocate new buffer with no NULL terminator.
    char *haystack = PyMem_Malloc(len_haystack);
    if (haystack == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    char *needle = PyMem_Malloc(len_needle);
    if (needle == NULL) {
        PyMem_Free(haystack);
        PyErr_NoMemory();
        return -1;
    }
    memcpy(haystack, haystack0, len_haystack);
    memcpy(needle, needle0, len_needle);
    Py_ssize_t result_2 = _PyBytes_Find(haystack, len_haystack,
                                        needle, len_needle, offset);
    PyMem_Free(haystack);
    PyMem_Free(needle);
    if (result_2 != expected) {
        PyErr_Format(PyExc_AssertionError,
                    "Incorrect result_2: '%s' in '%s' (offset=%zd)",
                    needle0, haystack0, offset);
        return -1;
    }
    return 0;
}

static int
check_bytes_find_large(Py_ssize_t len_haystack, Py_ssize_t len_needle,
                       const char *needle)
{
    char *zeros = PyMem_RawCalloc(len_haystack, 1);
    if (zeros == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    Py_ssize_t res = _PyBytes_Find(zeros, len_haystack, needle, len_needle, 0);
    PyMem_RawFree(zeros);
    if (res != -1) {
        PyErr_Format(PyExc_AssertionError,
                    "check_bytes_find_large(%zd, %zd) found %zd",
                    len_haystack, len_needle, res);
        return -1;
    }
    return 0;
}

static PyObject *
test_bytes_find(PyObject *self, PyObject *Py_UNUSED(args))
{
    #define CHECK(H, N, O, E) do {               \
        if (check_bytes_find(H, N, O, E) < 0) {  \
            return NULL;                         \
        }                                        \
    } while (0)

    CHECK("", "", 0, 0);
    CHECK("Python", "", 0, 0);
    CHECK("Python", "", 3, 3);
    CHECK("Python", "", 6, 6);
    CHECK("Python", "yth", 0, 1);
    CHECK("ython", "yth", 1, 1);
    CHECK("thon", "yth", 2, -1);
    CHECK("Python", "thon", 0, 2);
    CHECK("ython", "thon", 1, 2);
    CHECK("thon", "thon", 2, 2);
    CHECK("hon", "thon", 3, -1);
    CHECK("Pytho", "zz", 0, -1);
    CHECK("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "ab", 0, -1);
    CHECK("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "ba", 0, -1);
    CHECK("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "bb", 0, -1);
    CHECK("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaab", "ab", 0, 30);
    CHECK("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaba", "ba", 0, 30);
    CHECK("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaabb", "bb", 0, 30);
    #undef CHECK

    // Hunt for segfaults
    // n, m chosen here so that (n - m) % (m + 1) == 0
    // This would make default_find in fastsearch.h access haystack[n].
    if (check_bytes_find_large(2048, 2, "ab") < 0) {
        return NULL;
    }
    if (check_bytes_find_large(4096, 16, "0123456789abcdef") < 0) {
        return NULL;
    }
    if (check_bytes_find_large(8192, 2, "ab") < 0) {
        return NULL;
    }
    if (check_bytes_find_large(16384, 4, "abcd") < 0) {
        return NULL;
    }
    if (check_bytes_find_large(32768, 2, "ab") < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject *
normalize_path(PyObject *self, PyObject *filename)
{
    Py_ssize_t size = -1;
    wchar_t *encoded = PyUnicode_AsWideCharString(filename, &size);
    if (encoded == NULL) {
        return NULL;
    }

    PyObject *result = PyUnicode_FromWideChar(_Py_normpath(encoded, size), -1);
    PyMem_Free(encoded);

    return result;
}

static PyObject *
get_getpath_codeobject(PyObject *self, PyObject *Py_UNUSED(args)) {
    return _Py_Get_Getpath_CodeObject();
}


static PyObject *
encode_locale_ex(PyObject *self, PyObject *args)
{
    PyObject *unicode;
    int current_locale = 0;
    wchar_t *wstr;
    PyObject *res = NULL;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "U|is", &unicode, &current_locale, &errors)) {
        return NULL;
    }
    wstr = PyUnicode_AsWideCharString(unicode, NULL);
    if (wstr == NULL) {
        return NULL;
    }
    _Py_error_handler error_handler = _Py_GetErrorHandler(errors);

    char *str = NULL;
    size_t error_pos;
    const char *reason = NULL;
    int ret = _Py_EncodeLocaleEx(wstr,
                                 &str, &error_pos, &reason,
                                 current_locale, error_handler);
    PyMem_Free(wstr);

    switch(ret) {
    case 0:
        res = PyBytes_FromString(str);
        PyMem_RawFree(str);
        break;
    case -1:
        PyErr_NoMemory();
        break;
    case -2:
        PyErr_Format(PyExc_RuntimeError, "encode error: pos=%zu, reason=%s",
                     error_pos, reason);
        break;
    case -3:
        PyErr_SetString(PyExc_ValueError, "unsupported error handler");
        break;
    default:
        PyErr_SetString(PyExc_ValueError, "unknown error code");
        break;
    }
    return res;
}


static PyObject *
decode_locale_ex(PyObject *self, PyObject *args)
{
    char *str;
    int current_locale = 0;
    PyObject *res = NULL;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "y|is", &str, &current_locale, &errors)) {
        return NULL;
    }
    _Py_error_handler error_handler = _Py_GetErrorHandler(errors);

    wchar_t *wstr = NULL;
    size_t wlen = 0;
    const char *reason = NULL;
    int ret = _Py_DecodeLocaleEx(str,
                                 &wstr, &wlen, &reason,
                                 current_locale, error_handler);

    switch(ret) {
    case 0:
        res = PyUnicode_FromWideChar(wstr, wlen);
        PyMem_RawFree(wstr);
        break;
    case -1:
        PyErr_NoMemory();
        break;
    case -2:
        PyErr_Format(PyExc_RuntimeError, "decode error: pos=%zu, reason=%s",
                     wlen, reason);
        break;
    case -3:
        PyErr_SetString(PyExc_ValueError, "unsupported error handler");
        break;
    default:
        PyErr_SetString(PyExc_ValueError, "unknown error code");
        break;
    }
    return res;
}

static PyObject *
set_eval_frame_default(PyObject *self, PyObject *Py_UNUSED(args))
{
    module_state *state = get_module_state(self);
    _PyInterpreterState_SetEvalFrameFunc(_PyInterpreterState_GET(), _PyEval_EvalFrameDefault);
    Py_CLEAR(state->record_list);
    Py_RETURN_NONE;
}

static PyObject *
record_eval(PyThreadState *tstate, struct _PyInterpreterFrame *f, int exc)
{
    if (PyFunction_Check(f->f_funcobj)) {
        PyObject *module = _get_current_module();
        assert(module != NULL);
        module_state *state = get_module_state(module);
        Py_DECREF(module);
        int res = PyList_Append(state->record_list,
                                ((PyFunctionObject *)f->f_funcobj)->func_name);
        if (res < 0) {
            return NULL;
        }
    }
    return _PyEval_EvalFrameDefault(tstate, f, exc);
}


static PyObject *
set_eval_frame_record(PyObject *self, PyObject *list)
{
    module_state *state = get_module_state(self);
    if (!PyList_Check(list)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a list");
        return NULL;
    }
    Py_XSETREF(state->record_list, Py_NewRef(list));
    _PyInterpreterState_SetEvalFrameFunc(_PyInterpreterState_GET(), record_eval);
    Py_RETURN_NONE;
}

/*[clinic input]

_testinternalcapi.compiler_cleandoc -> object

    doc: unicode

C implementation of inspect.cleandoc().
[clinic start generated code]*/

static PyObject *
_testinternalcapi_compiler_cleandoc_impl(PyObject *module, PyObject *doc)
/*[clinic end generated code: output=2dd203a80feff5bc input=2de03fab931d9cdc]*/
{
    return _PyCompile_CleanDoc(doc);
}


/*[clinic input]

_testinternalcapi.compiler_codegen -> object

  ast: object
  filename: object
  optimize: int
  compile_mode: int = 0

Apply compiler code generation to an AST.
[clinic start generated code]*/

static PyObject *
_testinternalcapi_compiler_codegen_impl(PyObject *module, PyObject *ast,
                                        PyObject *filename, int optimize,
                                        int compile_mode)
/*[clinic end generated code: output=40a68f6e13951cc8 input=a0e00784f1517cd7]*/
{
    PyCompilerFlags *flags = NULL;
    return _PyCompile_CodeGen(ast, filename, flags, optimize, compile_mode);
}


/*[clinic input]

_testinternalcapi.optimize_cfg -> object

  instructions: object
  consts: object
  nlocals: int

Apply compiler optimizations to an instruction list.
[clinic start generated code]*/

static PyObject *
_testinternalcapi_optimize_cfg_impl(PyObject *module, PyObject *instructions,
                                    PyObject *consts, int nlocals)
/*[clinic end generated code: output=57c53c3a3dfd1df0 input=6a96d1926d58d7e5]*/
{
    return _PyCompile_OptimizeCfg(instructions, consts, nlocals);
}

static int
get_nonnegative_int_from_dict(PyObject *dict, const char *key) {
    PyObject *obj = PyDict_GetItemString(dict, key);
    if (obj == NULL) {
        return -1;
    }
    return PyLong_AsLong(obj);
}

/*[clinic input]

_testinternalcapi.assemble_code_object -> object

  filename: object
  instructions: object
  metadata: object

Create a code object for the given instructions.
[clinic start generated code]*/

static PyObject *
_testinternalcapi_assemble_code_object_impl(PyObject *module,
                                            PyObject *filename,
                                            PyObject *instructions,
                                            PyObject *metadata)
/*[clinic end generated code: output=38003dc16a930f48 input=e713ad77f08fb3a8]*/

{
    assert(PyDict_Check(metadata));
    _PyCompile_CodeUnitMetadata umd;

    umd.u_name = PyDict_GetItemString(metadata, "name");
    umd.u_qualname = PyDict_GetItemString(metadata, "qualname");

    assert(PyUnicode_Check(umd.u_name));
    assert(PyUnicode_Check(umd.u_qualname));

    umd.u_consts = PyDict_GetItemString(metadata, "consts");
    umd.u_names = PyDict_GetItemString(metadata, "names");
    umd.u_varnames = PyDict_GetItemString(metadata, "varnames");
    umd.u_cellvars = PyDict_GetItemString(metadata, "cellvars");
    umd.u_freevars = PyDict_GetItemString(metadata, "freevars");
    umd.u_fasthidden = PyDict_GetItemString(metadata, "fasthidden");

    assert(PyDict_Check(umd.u_consts));
    assert(PyDict_Check(umd.u_names));
    assert(PyDict_Check(umd.u_varnames));
    assert(PyDict_Check(umd.u_cellvars));
    assert(PyDict_Check(umd.u_freevars));
    assert(PyDict_Check(umd.u_fasthidden));

    umd.u_argcount = get_nonnegative_int_from_dict(metadata, "argcount");
    umd.u_posonlyargcount = get_nonnegative_int_from_dict(metadata, "posonlyargcount");
    umd.u_kwonlyargcount = get_nonnegative_int_from_dict(metadata, "kwonlyargcount");
    umd.u_firstlineno = get_nonnegative_int_from_dict(metadata, "firstlineno");

    assert(umd.u_argcount >= 0);
    assert(umd.u_posonlyargcount >= 0);
    assert(umd.u_kwonlyargcount >= 0);
    assert(umd.u_firstlineno >= 0);

    return (PyObject*)_PyCompile_Assemble(&umd, filename, instructions);
}


static PyObject *
get_interp_settings(PyObject *self, PyObject *args)
{
    int interpid = -1;
    if (!PyArg_ParseTuple(args, "|i:get_interp_settings", &interpid)) {
        return NULL;
    }

    PyInterpreterState *interp = NULL;
    if (interpid < 0) {
        PyThreadState *tstate = _PyThreadState_GET();
        interp = tstate ? tstate->interp : _PyInterpreterState_Main();
    }
    else if (interpid == 0) {
        interp = _PyInterpreterState_Main();
    }
    else {
        PyErr_Format(PyExc_NotImplementedError,
                     "%zd", interpid);
        return NULL;
    }
    assert(interp != NULL);

    PyObject *settings = PyDict_New();
    if (settings == NULL) {
        return NULL;
    }

    /* Add the feature flags. */
    PyObject *flags = PyLong_FromUnsignedLong(interp->feature_flags);
    if (flags == NULL) {
        Py_DECREF(settings);
        return NULL;
    }
    int res = PyDict_SetItemString(settings, "feature_flags", flags);
    Py_DECREF(flags);
    if (res != 0) {
        Py_DECREF(settings);
        return NULL;
    }

    /* "own GIL" */
    PyObject *own_gil = interp->ceval.own_gil ? Py_True : Py_False;
    if (PyDict_SetItemString(settings, "own_gil", own_gil) != 0) {
        Py_DECREF(settings);
        return NULL;
    }

    return settings;
}


static PyObject *
clear_extension(PyObject *self, PyObject *args)
{
    PyObject *name = NULL, *filename = NULL;
    if (!PyArg_ParseTuple(args, "OO:clear_extension", &name, &filename)) {
        return NULL;
    }
    if (_PyImport_ClearExtension(name, filename) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
write_perf_map_entry(PyObject *self, PyObject *args)
{
    PyObject *code_addr_v;
    const void *code_addr;
    unsigned int code_size;
    const char *entry_name;

    if (!PyArg_ParseTuple(args, "OIs", &code_addr_v, &code_size, &entry_name))
        return NULL;
    code_addr = PyLong_AsVoidPtr(code_addr_v);
    if (code_addr == NULL) {
        return NULL;
    }

    int ret = PyUnstable_WritePerfMapEntry(code_addr, code_size, entry_name);
    if (ret < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }
    return PyLong_FromLong(ret);
}

static PyObject *
perf_map_state_teardown(PyObject *Py_UNUSED(self), PyObject *Py_UNUSED(ignored))
{
    PyUnstable_PerfMapState_Fini();
    Py_RETURN_NONE;
}

static PyObject *
iframe_getcode(PyObject *self, PyObject *frame)
{
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return NULL;
    }
    struct _PyInterpreterFrame *f = ((PyFrameObject *)frame)->f_frame;
    return PyUnstable_InterpreterFrame_GetCode(f);
}

static PyObject *
iframe_getline(PyObject *self, PyObject *frame)
{
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return NULL;
    }
    struct _PyInterpreterFrame *f = ((PyFrameObject *)frame)->f_frame;
    return PyLong_FromLong(PyUnstable_InterpreterFrame_GetLine(f));
}

static PyObject *
iframe_getlasti(PyObject *self, PyObject *frame)
{
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return NULL;
    }
    struct _PyInterpreterFrame *f = ((PyFrameObject *)frame)->f_frame;
    return PyLong_FromLong(PyUnstable_InterpreterFrame_GetLasti(f));
}

static PyObject *
get_counter_optimizer(PyObject *self, PyObject *arg)
{
    return PyUnstable_Optimizer_NewCounter();
}

static PyObject *
get_uop_optimizer(PyObject *self, PyObject *arg)
{
    return PyUnstable_Optimizer_NewUOpOptimizer();
}

static PyObject *
set_optimizer(PyObject *self, PyObject *opt)
{
    if (opt == Py_None) {
        opt = NULL;
    }
    PyUnstable_SetOptimizer((_PyOptimizerObject*)opt);
    Py_RETURN_NONE;
}

static PyObject *
get_optimizer(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *opt = (PyObject *)PyUnstable_GetOptimizer();
    if (opt == NULL) {
        Py_RETURN_NONE;
    }
    return opt;
}

static PyObject *
get_executor(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{

    if (!_PyArg_CheckPositional("get_executor", nargs, 2, 2)) {
        return NULL;
    }
    PyObject *code = args[0];
    PyObject *offset = args[1];
    long ioffset = PyLong_AsLong(offset);
    if (ioffset == -1 && PyErr_Occurred()) {
        return NULL;
    }
    if (!PyCode_Check(code)) {
         PyErr_SetString(PyExc_TypeError, "first argument must be a code object");
        return NULL;
    }
    return (PyObject *)PyUnstable_GetExecutor((PyCodeObject *)code, ioffset);
}

static PyObject *
add_executor_dependency(PyObject *self, PyObject *args)
{
    PyObject *exec;
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "OO", &exec, &obj)) {
        return NULL;
    }
    /* No way to tell in general if exec is an executor, so we only accept
     * counting_executor */
    if (strcmp(Py_TYPE(exec)->tp_name, "counting_executor")) {
        PyErr_SetString(PyExc_TypeError, "argument must be a counting_executor");
        return NULL;
    }
    _Py_Executor_DependsOn((_PyExecutorObject *)exec, obj);
    Py_RETURN_NONE;
}

static PyObject *
invalidate_executors(PyObject *self, PyObject *obj)
{
    PyInterpreterState *interp = PyInterpreterState_Get();
    _Py_Executors_InvalidateDependency(interp, obj);
    Py_RETURN_NONE;
}

static int _pending_callback(void *arg)
{
    /* we assume the argument is callable object to which we own a reference */
    PyObject *callable = (PyObject *)arg;
    PyObject *r = PyObject_CallNoArgs(callable);
    Py_DECREF(callable);
    Py_XDECREF(r);
    return r != NULL ? 0 : -1;
}

/* The following requests n callbacks to _pending_callback.  It can be
 * run from any python thread.
 */
static PyObject *
pending_threadfunc(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *callable;
    int ensure_added = 0;
    static char *kwlist[] = {"", "ensure_added", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "O|$p:pending_threadfunc", kwlist,
                                     &callable, &ensure_added))
    {
        return NULL;
    }
    PyInterpreterState *interp = _PyInterpreterState_GET();

    /* create the reference for the callbackwhile we hold the lock */
    Py_INCREF(callable);

    int r;
    Py_BEGIN_ALLOW_THREADS
    r = _PyEval_AddPendingCall(interp, &_pending_callback, callable, 0);
    Py_END_ALLOW_THREADS
    if (r < 0) {
        /* unsuccessful add */
        if (!ensure_added) {
            Py_DECREF(callable);
            Py_RETURN_FALSE;
        }
        do {
            Py_BEGIN_ALLOW_THREADS
            r = _PyEval_AddPendingCall(interp, &_pending_callback, callable, 0);
            Py_END_ALLOW_THREADS
        } while (r < 0);
    }

    Py_RETURN_TRUE;
}


static struct {
    int64_t interpid;
} pending_identify_result;

static int
_pending_identify_callback(void *arg)
{
    PyThread_type_lock mutex = (PyThread_type_lock)arg;
    assert(pending_identify_result.interpid == -1);
    PyThreadState *tstate = PyThreadState_Get();
    pending_identify_result.interpid = PyInterpreterState_GetID(tstate->interp);
    PyThread_release_lock(mutex);
    return 0;
}

static PyObject *
pending_identify(PyObject *self, PyObject *args)
{
    PyObject *interpid;
    if (!PyArg_ParseTuple(args, "O:pending_identify", &interpid)) {
        return NULL;
    }
    PyInterpreterState *interp = PyInterpreterID_LookUp(interpid);
    if (interp == NULL) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_ValueError, "interpreter not found");
        }
        return NULL;
    }

    pending_identify_result.interpid = -1;

    PyThread_type_lock mutex = PyThread_allocate_lock();
    if (mutex == NULL) {
        return NULL;
    }
    PyThread_acquire_lock(mutex, WAIT_LOCK);
    /* It gets released in _pending_identify_callback(). */

    int r;
    do {
        Py_BEGIN_ALLOW_THREADS
        r = _PyEval_AddPendingCall(interp,
                                   &_pending_identify_callback, (void *)mutex,
                                   0);
        Py_END_ALLOW_THREADS
    } while (r < 0);

    /* Wait for the pending call to complete. */
    PyThread_acquire_lock(mutex, WAIT_LOCK);
    PyThread_release_lock(mutex);
    PyThread_free_lock(mutex);

    PyObject *res = PyLong_FromLongLong(pending_identify_result.interpid);
    pending_identify_result.interpid = -1;
    if (res == NULL) {
        return NULL;
    }
    return res;
}

static PyObject *
tracemalloc_get_traceback(PyObject *self, PyObject *args)
{
    unsigned int domain;
    PyObject *ptr_obj;

    if (!PyArg_ParseTuple(args, "IO", &domain, &ptr_obj)) {
        return NULL;
    }
    void *ptr = PyLong_AsVoidPtr(ptr_obj);
    if (PyErr_Occurred()) {
        return NULL;
    }

    return _PyTraceMalloc_GetTraceback(domain, (uintptr_t)ptr);
}


// Test PyThreadState C API
static PyObject *
test_tstate_capi(PyObject *self, PyObject *Py_UNUSED(args))
{
    // PyThreadState_Get()
    PyThreadState *tstate = PyThreadState_Get();
    assert(tstate != NULL);

    // test _PyThreadState_GetDict()
    PyObject *dict = PyThreadState_GetDict();
    assert(dict != NULL);
    // dict is a borrowed reference

    PyObject *dict2 = _PyThreadState_GetDict(tstate);
    assert(dict2 == dict);
    // dict2 is a borrowed reference

    Py_RETURN_NONE;
}


/* Test _PyUnicode_TransformDecimalAndSpaceToASCII() */
static PyObject *
unicode_transformdecimalandspacetoascii(PyObject *self, PyObject *arg)
{
    if (arg == Py_None) {
        arg = NULL;
    }
    return _PyUnicode_TransformDecimalAndSpaceToASCII(arg);
}


struct atexit_data {
    int called;
};

static void
callback(void *data)
{
    ((struct atexit_data *)data)->called += 1;
}

static PyObject *
test_atexit(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyThreadState *oldts = PyThreadState_Swap(NULL);
    PyThreadState *tstate = Py_NewInterpreter();

    struct atexit_data data = {0};
    int res = PyUnstable_AtExit(tstate->interp, callback, (void *)&data);
    Py_EndInterpreter(tstate);
    PyThreadState_Swap(oldts);
    if (res < 0) {
        return NULL;
    }

    if (data.called == 0) {
        PyErr_SetString(PyExc_RuntimeError, "atexit callback not called");
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject *
test_pyobject_is_freed(const char *test_name, PyObject *op)
{
    if (!_PyObject_IsFreed(op)) {
        PyErr_SetString(PyExc_AssertionError,
                        "object is not seen as freed");
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
check_pyobject_null_is_freed(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyObject *op = NULL;
    return test_pyobject_is_freed("check_pyobject_null_is_freed", op);
}


static PyObject *
check_pyobject_uninitialized_is_freed(PyObject *self,
                                      PyObject *Py_UNUSED(args))
{
    PyObject *op = (PyObject *)PyObject_Malloc(sizeof(PyObject));
    if (op == NULL) {
        return NULL;
    }
    /* Initialize reference count to avoid early crash in ceval or GC */
    Py_SET_REFCNT(op, 1);
    /* object fields like ob_type are uninitialized! */
    return test_pyobject_is_freed("check_pyobject_uninitialized_is_freed", op);
}


static PyObject *
check_pyobject_forbidden_bytes_is_freed(PyObject *self,
                                        PyObject *Py_UNUSED(args))
{
    /* Allocate an incomplete PyObject structure: truncate 'ob_type' field */
    PyObject *op = (PyObject *)PyObject_Malloc(offsetof(PyObject, ob_type));
    if (op == NULL) {
        return NULL;
    }
    /* Initialize reference count to avoid early crash in ceval or GC */
    Py_SET_REFCNT(op, 1);
    /* ob_type field is after the memory block: part of "forbidden bytes"
       when using debug hooks on memory allocators! */
    return test_pyobject_is_freed("check_pyobject_forbidden_bytes_is_freed", op);
}


static PyObject *
check_pyobject_freed_is_freed(PyObject *self, PyObject *Py_UNUSED(args))
{
    /* This test would fail if run with the address sanitizer */
#ifdef _Py_ADDRESS_SANITIZER
    Py_RETURN_NONE;
#else
    PyObject *op = PyObject_CallNoArgs((PyObject *)&PyBaseObject_Type);
    if (op == NULL) {
        return NULL;
    }
    Py_TYPE(op)->tp_dealloc(op);
    /* Reset reference count to avoid early crash in ceval or GC */
    Py_SET_REFCNT(op, 1);
    /* object memory is freed! */
    return test_pyobject_is_freed("check_pyobject_freed_is_freed", op);
#endif
}


static PyObject *
test_pymem_getallocatorsname(PyObject *self, PyObject *args)
{
    const char *name = _PyMem_GetCurrentAllocatorName();
    if (name == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "cannot get allocators name");
        return NULL;
    }
    return PyUnicode_FromString(name);
}

static PyObject *
get_object_dict_values(PyObject *self, PyObject *obj)
{
    PyTypeObject *type = Py_TYPE(obj);
    if (!_PyType_HasFeature(type, Py_TPFLAGS_MANAGED_DICT)) {
        Py_RETURN_NONE;
    }
    PyDictOrValues dorv = *_PyObject_DictOrValuesPointer(obj);
    if (!_PyDictOrValues_IsValues(dorv)) {
        Py_RETURN_NONE;
    }
    PyDictValues *values = _PyDictOrValues_GetValues(dorv);
    PyDictKeysObject *keys = ((PyHeapTypeObject *)type)->ht_cached_keys;
    assert(keys != NULL);
    int size = (int)keys->dk_nentries;
    assert(size >= 0);
    PyObject *res = PyTuple_New(size);
    if (res == NULL) {
        return NULL;
    }
    _Py_DECLARE_STR(anon_null, "<NULL>");
    for(int i = 0; i < size; i++) {
        PyObject *item = values->values[i];
        if (item == NULL) {
            item = &_Py_STR(anon_null);
        }
        else {
            Py_INCREF(item);
        }
        PyTuple_SET_ITEM(res, i, item);
    }
    return res;
}


static PyObject*
new_hamt(PyObject *self, PyObject *args)
{
    return _PyContext_NewHamtForTests();
}


static PyObject*
dict_getitem_knownhash(PyObject *self, PyObject *args)
{
    PyObject *mp, *key, *result;
    Py_ssize_t hash;

    if (!PyArg_ParseTuple(args, "OOn:dict_getitem_knownhash",
                          &mp, &key, &hash)) {
        return NULL;
    }

    result = _PyDict_GetItem_KnownHash(mp, key, (Py_hash_t)hash);
    if (result == NULL && !PyErr_Occurred()) {
        _PyErr_SetKeyError(key);
        return NULL;
    }

    return Py_XNewRef(result);
}


/* To run some code in a sub-interpreter. */
static PyObject *
run_in_subinterp_with_config(PyObject *self, PyObject *args, PyObject *kwargs)
{
    const char *code;
    int use_main_obmalloc = -1;
    int allow_fork = -1;
    int allow_exec = -1;
    int allow_threads = -1;
    int allow_daemon_threads = -1;
    int check_multi_interp_extensions = -1;
    int gil = -1;
    int r;
    PyThreadState *substate, *mainstate;
    /* only initialise 'cflags.cf_flags' to test backwards compatibility */
    PyCompilerFlags cflags = {0};

    static char *kwlist[] = {"code",
                             "use_main_obmalloc",
                             "allow_fork",
                             "allow_exec",
                             "allow_threads",
                             "allow_daemon_threads",
                             "check_multi_interp_extensions",
                             "gil",
                             NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                    "s$ppppppi:run_in_subinterp_with_config", kwlist,
                    &code, &use_main_obmalloc,
                    &allow_fork, &allow_exec,
                    &allow_threads, &allow_daemon_threads,
                    &check_multi_interp_extensions,
                    &gil)) {
        return NULL;
    }
    if (use_main_obmalloc < 0) {
        PyErr_SetString(PyExc_ValueError, "missing use_main_obmalloc");
        return NULL;
    }
    if (allow_fork < 0) {
        PyErr_SetString(PyExc_ValueError, "missing allow_fork");
        return NULL;
    }
    if (allow_exec < 0) {
        PyErr_SetString(PyExc_ValueError, "missing allow_exec");
        return NULL;
    }
    if (allow_threads < 0) {
        PyErr_SetString(PyExc_ValueError, "missing allow_threads");
        return NULL;
    }
    if (gil < 0) {
        PyErr_SetString(PyExc_ValueError, "missing gil");
        return NULL;
    }
    if (allow_daemon_threads < 0) {
        PyErr_SetString(PyExc_ValueError, "missing allow_daemon_threads");
        return NULL;
    }
    if (check_multi_interp_extensions < 0) {
        PyErr_SetString(PyExc_ValueError, "missing check_multi_interp_extensions");
        return NULL;
    }

    mainstate = PyThreadState_Get();

    PyThreadState_Swap(NULL);

    const PyInterpreterConfig config = {
        .use_main_obmalloc = use_main_obmalloc,
        .allow_fork = allow_fork,
        .allow_exec = allow_exec,
        .allow_threads = allow_threads,
        .allow_daemon_threads = allow_daemon_threads,
        .check_multi_interp_extensions = check_multi_interp_extensions,
        .gil = gil,
    };
    PyStatus status = Py_NewInterpreterFromConfig(&substate, &config);
    if (PyStatus_Exception(status)) {
        /* Since no new thread state was created, there is no exception to
           propagate; raise a fresh one after swapping in the old thread
           state. */
        PyThreadState_Swap(mainstate);
        _PyErr_SetFromPyStatus(status);
        PyObject *exc = PyErr_GetRaisedException();
        PyErr_SetString(PyExc_RuntimeError, "sub-interpreter creation failed");
        _PyErr_ChainExceptions1(exc);
        return NULL;
    }
    assert(substate != NULL);
    r = PyRun_SimpleStringFlags(code, &cflags);
    Py_EndInterpreter(substate);

    PyThreadState_Swap(mainstate);

    return PyLong_FromLong(r);
}


static PyObject *
get_interpreter_refcount(PyObject *self, PyObject *idobj)
{
    PyInterpreterState *interp = PyInterpreterID_LookUp(idobj);
    if (interp == NULL) {
        return NULL;
    }
    return PyLong_FromLongLong(interp->id_refcount);
}


static void
_xid_capsule_destructor(PyObject *capsule)
{
    _PyCrossInterpreterData *data = \
            (_PyCrossInterpreterData *)PyCapsule_GetPointer(capsule, NULL);
    if (data != NULL) {
        assert(_PyCrossInterpreterData_Release(data) == 0);
        _PyCrossInterpreterData_Free(data);
    }
}

static PyObject *
get_crossinterp_data(PyObject *self, PyObject *args)
{
    PyObject *obj = NULL;
    if (!PyArg_ParseTuple(args, "O:get_crossinterp_data", &obj)) {
        return NULL;
    }

    _PyCrossInterpreterData *data = _PyCrossInterpreterData_New();
    if (data == NULL) {
        return NULL;
    }
    if (_PyObject_GetCrossInterpreterData(obj, data) != 0) {
        _PyCrossInterpreterData_Free(data);
        return NULL;
    }
    PyObject *capsule = PyCapsule_New(data, NULL, _xid_capsule_destructor);
    if (capsule == NULL) {
        assert(_PyCrossInterpreterData_Release(data) == 0);
        _PyCrossInterpreterData_Free(data);
    }
    return capsule;
}

static PyObject *
restore_crossinterp_data(PyObject *self, PyObject *args)
{
    PyObject *capsule = NULL;
    if (!PyArg_ParseTuple(args, "O:restore_crossinterp_data", &capsule)) {
        return NULL;
    }

    _PyCrossInterpreterData *data = \
            (_PyCrossInterpreterData *)PyCapsule_GetPointer(capsule, NULL);
    if (data == NULL) {
        return NULL;
    }
    return _PyCrossInterpreterData_NewObject(data);
}


static PyObject *
raiseTestError(const char* test_name, const char* msg)
{
    PyErr_Format(PyExc_AssertionError, "%s: %s", test_name, msg);
    return NULL;
}


/*[clinic input]
_testinternalcapi.test_long_numbits
[clinic start generated code]*/

static PyObject *
_testinternalcapi_test_long_numbits_impl(PyObject *module)
/*[clinic end generated code: output=745d62d120359434 input=f14ca6f638e44dad]*/
{
    struct triple {
        long input;
        size_t nbits;
        int sign;
    } testcases[] = {{0, 0, 0},
                     {1L, 1, 1},
                     {-1L, 1, -1},
                     {2L, 2, 1},
                     {-2L, 2, -1},
                     {3L, 2, 1},
                     {-3L, 2, -1},
                     {4L, 3, 1},
                     {-4L, 3, -1},
                     {0x7fffL, 15, 1},          /* one Python int digit */
             {-0x7fffL, 15, -1},
             {0xffffL, 16, 1},
             {-0xffffL, 16, -1},
             {0xfffffffL, 28, 1},
             {-0xfffffffL, 28, -1}};
    size_t i;

    for (i = 0; i < Py_ARRAY_LENGTH(testcases); ++i) {
        size_t nbits;
        int sign;
        PyObject *plong;

        plong = PyLong_FromLong(testcases[i].input);
        if (plong == NULL)
            return NULL;
        nbits = _PyLong_NumBits(plong);
        sign = _PyLong_Sign(plong);

        Py_DECREF(plong);
        if (nbits != testcases[i].nbits)
            return raiseTestError("test_long_numbits",
                            "wrong result for _PyLong_NumBits");
        if (sign != testcases[i].sign)
            return raiseTestError("test_long_numbits",
                            "wrong result for _PyLong_Sign");
    }
    Py_RETURN_NONE;
}

static PyObject *
compile_perf_trampoline_entry(PyObject *self, PyObject *args)
{
    PyObject *co;
    if (!PyArg_ParseTuple(args, "O!", &PyCode_Type, &co)) {
        return NULL;
    }
    int ret = PyUnstable_PerfTrampoline_CompileCode((PyCodeObject *)co);
    if (ret != 0) {
        PyErr_SetString(PyExc_AssertionError, "Failed to compile trampoline");
        return NULL;
    }
    return PyLong_FromLong(ret);
}

static PyObject *
perf_trampoline_set_persist_after_fork(PyObject *self, PyObject *args)
{
    int enable;
    if (!PyArg_ParseTuple(args, "i", &enable)) {
        return NULL;
    }
    int ret = PyUnstable_PerfTrampoline_SetPersistAfterFork(enable);
    if (ret == 0) {
        PyErr_SetString(PyExc_AssertionError, "Failed to set persist_after_fork");
        return NULL;
    }
    return PyLong_FromLong(ret);
}


static PyObject *
get_type_module_name(PyObject *self, PyObject *type)
{
    assert(PyType_Check(type));
    return _PyType_GetModuleName((PyTypeObject *)type);
}


#ifdef Py_GIL_DISABLED
static PyObject *
get_py_thread_id(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    uintptr_t tid = _Py_ThreadId();
    Py_BUILD_ASSERT(sizeof(unsigned long long) >= sizeof(tid));
    return PyLong_FromUnsignedLongLong(tid);
}
#endif


static PyMethodDef module_functions[] = {
    {"get_configs", get_configs, METH_NOARGS},
    {"get_recursion_depth", get_recursion_depth, METH_NOARGS},
    {"get_c_recursion_remaining", get_c_recursion_remaining, METH_NOARGS},
    {"test_bswap", test_bswap, METH_NOARGS},
    {"test_popcount", test_popcount, METH_NOARGS},
    {"test_bit_length", test_bit_length, METH_NOARGS},
    {"test_hashtable", test_hashtable, METH_NOARGS},
    {"get_config", test_get_config, METH_NOARGS},
    {"set_config", test_set_config, METH_O},
    {"reset_path_config", test_reset_path_config, METH_NOARGS},
    {"test_edit_cost", test_edit_cost, METH_NOARGS},
    {"test_bytes_find", test_bytes_find, METH_NOARGS},
    {"normalize_path", normalize_path, METH_O, NULL},
    {"get_getpath_codeobject", get_getpath_codeobject, METH_NOARGS, NULL},
    {"EncodeLocaleEx", encode_locale_ex, METH_VARARGS},
    {"DecodeLocaleEx", decode_locale_ex, METH_VARARGS},
    {"set_eval_frame_default", set_eval_frame_default, METH_NOARGS, NULL},
    {"set_eval_frame_record", set_eval_frame_record, METH_O, NULL},
    _TESTINTERNALCAPI_COMPILER_CLEANDOC_METHODDEF
    _TESTINTERNALCAPI_COMPILER_CODEGEN_METHODDEF
    _TESTINTERNALCAPI_OPTIMIZE_CFG_METHODDEF
    _TESTINTERNALCAPI_ASSEMBLE_CODE_OBJECT_METHODDEF
    {"get_interp_settings", get_interp_settings, METH_VARARGS, NULL},
    {"clear_extension", clear_extension, METH_VARARGS, NULL},
    {"write_perf_map_entry", write_perf_map_entry, METH_VARARGS},
    {"perf_map_state_teardown", perf_map_state_teardown, METH_NOARGS},
    {"iframe_getcode", iframe_getcode, METH_O, NULL},
    {"iframe_getline", iframe_getline, METH_O, NULL},
    {"iframe_getlasti", iframe_getlasti, METH_O, NULL},
    {"get_optimizer", get_optimizer,  METH_NOARGS, NULL},
    {"set_optimizer", set_optimizer,  METH_O, NULL},
    {"get_executor", _PyCFunction_CAST(get_executor),  METH_FASTCALL, NULL},
    {"get_counter_optimizer", get_counter_optimizer, METH_NOARGS, NULL},
    {"get_uop_optimizer", get_uop_optimizer, METH_NOARGS, NULL},
    {"add_executor_dependency", add_executor_dependency, METH_VARARGS, NULL},
    {"invalidate_executors", invalidate_executors, METH_O, NULL},
    {"pending_threadfunc", _PyCFunction_CAST(pending_threadfunc),
     METH_VARARGS | METH_KEYWORDS},
    {"pending_identify", pending_identify, METH_VARARGS, NULL},
    {"_PyTraceMalloc_GetTraceback", tracemalloc_get_traceback, METH_VARARGS},
    {"test_tstate_capi", test_tstate_capi, METH_NOARGS, NULL},
    {"_PyUnicode_TransformDecimalAndSpaceToASCII", unicode_transformdecimalandspacetoascii, METH_O},
    {"test_atexit", test_atexit, METH_NOARGS},
    {"check_pyobject_forbidden_bytes_is_freed",
                            check_pyobject_forbidden_bytes_is_freed, METH_NOARGS},
    {"check_pyobject_freed_is_freed", check_pyobject_freed_is_freed, METH_NOARGS},
    {"check_pyobject_null_is_freed",  check_pyobject_null_is_freed,  METH_NOARGS},
    {"check_pyobject_uninitialized_is_freed",
                              check_pyobject_uninitialized_is_freed, METH_NOARGS},
    {"pymem_getallocatorsname", test_pymem_getallocatorsname, METH_NOARGS},
    {"get_object_dict_values", get_object_dict_values, METH_O},
    {"hamt", new_hamt, METH_NOARGS},
    {"dict_getitem_knownhash",  dict_getitem_knownhash,          METH_VARARGS},
    {"run_in_subinterp_with_config",
     _PyCFunction_CAST(run_in_subinterp_with_config),
     METH_VARARGS | METH_KEYWORDS},
    {"get_interpreter_refcount", get_interpreter_refcount, METH_O},
    {"compile_perf_trampoline_entry", compile_perf_trampoline_entry, METH_VARARGS},
    {"perf_trampoline_set_persist_after_fork", perf_trampoline_set_persist_after_fork, METH_VARARGS},
    {"get_crossinterp_data",    get_crossinterp_data,            METH_VARARGS},
    {"restore_crossinterp_data", restore_crossinterp_data,       METH_VARARGS},
    _TESTINTERNALCAPI_TEST_LONG_NUMBITS_METHODDEF
    {"get_type_module_name",    get_type_module_name,            METH_O},
#ifdef Py_GIL_DISABLED
    {"py_thread_id", get_py_thread_id, METH_NOARGS},
#endif
    {NULL, NULL} /* sentinel */
};


/* initialization function */

static int
module_exec(PyObject *module)
{
    if (_PyTestInternalCapi_Init_Lock(module) < 0) {
        return 1;
    }
    if (_PyTestInternalCapi_Init_PyTime(module) < 0) {
        return 1;
    }
    if (_PyTestInternalCapi_Init_Set(module) < 0) {
        return 1;
    }
    if (_PyTestInternalCapi_Init_CriticalSection(module) < 0) {
        return 1;
    }

    if (PyModule_Add(module, "SIZEOF_PYGC_HEAD",
                        PyLong_FromSsize_t(sizeof(PyGC_Head))) < 0) {
        return 1;
    }

    if (PyModule_Add(module, "SIZEOF_PYOBJECT",
                        PyLong_FromSsize_t(sizeof(PyObject))) < 0) {
        return 1;
    }

    if (PyModule_Add(module, "SIZEOF_TIME_T",
                        PyLong_FromSsize_t(sizeof(time_t))) < 0) {
        return 1;
    }

    return 0;
}

static struct PyModuleDef_Slot module_slots[] = {
    {Py_mod_exec, module_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {0, NULL},
};

static int
module_traverse(PyObject *module, visitproc visit, void *arg)
{
    module_state *state = get_module_state(module);
    assert(state != NULL);
    traverse_module_state(state, visit, arg);
    return 0;
}

static int
module_clear(PyObject *module)
{
    module_state *state = get_module_state(module);
    assert(state != NULL);
    (void)clear_module_state(state);
    return 0;
}

static void
module_free(void *module)
{
    module_state *state = get_module_state(module);
    assert(state != NULL);
    (void)clear_module_state(state);
}

static struct PyModuleDef _testcapimodule = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = MODULE_NAME,
    .m_doc = NULL,
    .m_size = sizeof(module_state),
    .m_methods = module_functions,
    .m_slots = module_slots,
    .m_traverse = module_traverse,
    .m_clear = module_clear,
    .m_free = (freefunc)module_free,
};


PyMODINIT_FUNC
PyInit__testinternalcapi(void)
{
    return PyModuleDef_Init(&_testcapimodule);
}
