#include "Python.h"
#include "pycore_call.h" // for _PyObject_CallMethod

#include "_fnmatchmodule.h"
#include "clinic/_fnmatchmodule.c.h"

#define COMPILED_CACHE_SIZE     32768
#define INVALID_PATTERN_TYPE    "pattern must be a string or a bytes object"

// ==== Helper implementations ================================================

/*
 * Compile a UNIX shell pattern into a RE pattern
 * and returns the corresponding 'match()' method.
 *
 * This function is LRU-cached by the module itself.
 */
static PyObject *
fnmatchmodule_get_matcher_function(PyObject *module, PyObject *pattern)
{
    // translate the pattern into a RE pattern
    assert(module != NULL);
    PyObject *expr = _fnmatch_translate_impl(module, pattern);
    if (expr == NULL) {
        return NULL;
    }
    fnmatchmodule_state *st = get_fnmatchmodulestate_state(module);
    // compile the pattern
    PyObject *compiled = _PyObject_CallMethod(st->re_module, &_Py_ID(compile), "O", expr);
    Py_DECREF(expr);
    if (compiled == NULL) {
        return NULL;
    }
    // get the compiled pattern matcher function
    PyObject *matcher = PyObject_GetAttr(compiled, &_Py_ID(match));
    Py_DECREF(compiled);
    return matcher;
}

static PyMethodDef get_matcher_function_def = {
    "get_matcher_function",
    (PyCFunction)(fnmatchmodule_get_matcher_function),
    METH_O,
    NULL
};

static int
fnmatchmodule_load_lru_cache(PyObject *module, fnmatchmodule_state *st)
{
    st->lru_cache = _PyImport_GetModuleAttrString("functools", "lru_cache");
    if (st->lru_cache == NULL) {
        return -1;
    }
    return 0;
}

static int
fnmatchmodule_load_translator(PyObject *module, fnmatchmodule_state *st)
{
    assert(st->lru_cache != NULL);
    PyObject *maxsize = PyLong_FromLong(COMPILED_CACHE_SIZE);
    if (maxsize == NULL) {
        return -1;
    }
    PyObject *args[] = {NULL, maxsize, Py_True};
    size_t nargsf = 2 | PY_VECTORCALL_ARGUMENTS_OFFSET;
    PyObject *decorator = PyObject_Vectorcall(st->lru_cache, args + 1, nargsf, NULL);
    Py_DECREF(maxsize);
    if (decorator == NULL) {
        return -1;
    }
    // TODO(picnixz): should INCREF the refcount of 'module'?
    assert(module != NULL);
    PyObject *decorated = PyCFunction_New(&get_matcher_function_def, module);
    PyObject *translator = PyObject_CallOneArg(decorator, decorated);
    Py_DECREF(decorated);
    Py_DECREF(decorator);
    if (translator == NULL) {
        return -1;
    }
    // reference on 'translator' will be removed upon module cleanup
    st->translator = translator;
    return 0;
}

static inline PyObject *
get_matcher_function(PyObject *module, PyObject *pattern)
{
    assert(module != NULL);
    assert(pattern != NULL);
    fnmatchmodule_state *st = get_fnmatchmodulestate_state(module);
    assert(st->translator != NULL);
    size_t nargsf = 1 | PY_VECTORCALL_ARGUMENTS_OFFSET;
    return PyObject_Vectorcall(st->translator, &pattern, nargsf, NULL);
}

// ==== Module state functions ================================================

#define IMPORT_MODULE(state, attribute, name) \
    do { \
        state->attribute = NULL; \
        state->attribute = PyImport_ImportModule((name)); \
        if (state->attribute == NULL) { \
            return -1; \
        } \
    } while (0)

static int
fnmatchmodule_exec(PyObject *module)
{
    fnmatchmodule_state *st = get_fnmatchmodulestate_state(module);
    st->py_module = NULL;
    IMPORT_MODULE(st, py_module, "fnmatch");
    st->os_module = NULL;
    IMPORT_MODULE(st, os_module, "os");
    st->re_module = NULL;
    IMPORT_MODULE(st, re_module, "re");
    st->lru_cache = NULL;
    if (fnmatchmodule_load_lru_cache(module, st) < 0) {
        return -1;
    }
    st->translator = NULL;
    if (fnmatchmodule_load_translator(module, st) < 0) {
        return -1;
    }
    return 0;
}
#undef IMPORT_MODULE

static int
fnmatchmodule_traverse(PyObject *m, visitproc visit, void *arg)
{
    fnmatchmodule_state *st = get_fnmatchmodulestate_state(m);
    Py_VISIT(st->py_module);
    Py_VISIT(st->os_module);
    Py_VISIT(st->re_module);
    Py_VISIT(st->lru_cache);
    Py_VISIT(st->translator);
    return 0;
}

static int
fnmatchmodule_clear(PyObject *m)
{
    fnmatchmodule_state *st = get_fnmatchmodulestate_state(m);
    Py_CLEAR(st->py_module);
    Py_CLEAR(st->os_module);
    Py_CLEAR(st->re_module);
    Py_CLEAR(st->lru_cache);
    Py_CLEAR(st->translator);
    return 0;
}

static void
fnmatchmodule_free(void *m)
{
    (void)fnmatchmodule_clear((PyObject *)m);
}

/*[clinic input]
module _fnmatch
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=356e324d57d93f08]*/

/*[clinic input]
_fnmatch.filter -> object

    names: object
    pat: object

[clinic start generated code]*/

static PyObject *
_fnmatch_filter_impl(PyObject *module, PyObject *names, PyObject *pat)
/*[clinic end generated code: output=7f11aa68436d05fc input=1d233174e1c4157a]*/
{
    PyObject *matcher = get_matcher_function(module, pat);
    if (matcher == NULL) {
        return NULL;
    }
    PyObject *result = _Py_fnmatch_filter(matcher, names);
    Py_DECREF(matcher);
    return result;
}

/*[clinic input]
_fnmatch.fnmatch -> bool

    name: object
    pat: object

[clinic start generated code]*/

static int
_fnmatch_fnmatch_impl(PyObject *module, PyObject *name, PyObject *pat)
/*[clinic end generated code: output=b4cd0bd911e8bc93 input=c45e0366489540b8]*/
{
    fnmatchmodule_state *st = get_fnmatchmodulestate_state(module);
    PyObject *res = _PyObject_CallMethod(st->py_module, &_Py_ID(fnmatch), "OO", name, pat);
    if (res == NULL) {
        return -1;
    }
    int matching = PyLong_AsLong(res);
    if (matching < 0) {
        return -1;
    }
    Py_DECREF(res);
    return matching;
}

/*[clinic input]
_fnmatch.fnmatchcase -> bool

    name: object
    pat: object

Test whether `name` matches `pattern`, including case.

This is a version of fnmatch() which doesn't case-normalize
its arguments.

[clinic start generated code]*/

static int
_fnmatch_fnmatchcase_impl(PyObject *module, PyObject *name, PyObject *pat)
/*[clinic end generated code: output=4d1283b1b1fc7cb8 input=b02a6a5c8c5a46e2]*/
{
    PyObject *matcher = get_matcher_function(module, pat);
    if (matcher == NULL) {
        return -1;
    }
    int res = _Py_fnmatch_fnmatch(matcher, name);
    Py_DECREF(matcher);
    return res;
}

/*[clinic input]
_fnmatch.translate -> object

    pat as pattern: object

[clinic start generated code]*/

static PyObject *
_fnmatch_translate_impl(PyObject *module, PyObject *pattern)
/*[clinic end generated code: output=2d9e3bbcbcc6e90e input=56e39f7beea97810]*/
{
    if (PyBytes_Check(pattern)) {
        PyObject *unicode = PyUnicode_DecodeLatin1(PyBytes_AS_STRING(pattern),
                                                   PyBytes_GET_SIZE(pattern),
                                                   "strict");
        if (unicode == NULL) {
            return NULL;
        }
        // translated regular expression as a str object
        PyObject *str_expr = _Py_fnmatch_translate(module, unicode);
        Py_DECREF(unicode);
        if (str_expr == NULL) {
            return NULL;
        }
        PyObject *expr = PyUnicode_AsLatin1String(str_expr);
        Py_DECREF(str_expr);
        return expr;
    }
    else if (PyUnicode_Check(pattern)) {
        return _Py_fnmatch_translate(module, pattern);
    }
    else {
        PyErr_SetString(PyExc_TypeError, INVALID_PATTERN_TYPE);
        return NULL;
    }
}

static PyMethodDef fnmatchmodule_methods[] = {
    _FNMATCH_FILTER_METHODDEF
    _FNMATCH_FNMATCH_METHODDEF
    _FNMATCH_FNMATCHCASE_METHODDEF
    _FNMATCH_TRANSLATE_METHODDEF
    {NULL, NULL}
};

static struct PyModuleDef_Slot fnmatchmodule_slots[] = {
    {Py_mod_exec, fnmatchmodule_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL},
};

static struct PyModuleDef _fnmatchmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_fnmatch",
    .m_doc = NULL,
    .m_size = sizeof(fnmatchmodule_state),
    .m_methods = fnmatchmodule_methods,
    .m_slots = fnmatchmodule_slots,
    .m_traverse = fnmatchmodule_traverse,
    .m_clear = fnmatchmodule_clear,
    .m_free = fnmatchmodule_free,
};

PyMODINIT_FUNC
PyInit__fnmatch(void)
{
    return PyModuleDef_Init(&_fnmatchmodule);
}
