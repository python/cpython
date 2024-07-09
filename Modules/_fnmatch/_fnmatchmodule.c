/*
 * C accelerator for the 'fnmatch' module (POSIX only).
 *
 * Most functions expect string or bytes instances, and thus the Python
 * implementation should first pre-process path-like objects, possibly
 * applying normalizations depending on the platform if needed.
 */

#include "Python.h"
#include "pycore_call.h" // for _PyObject_CallMethod

#include "_fnmatchmodule.h"
#include "clinic/_fnmatchmodule.c.h"

#define INVALID_PATTERN_TYPE "pattern must be a string or a bytes object"

// module state functions

static int
fnmatchmodule_clear(PyObject *m)
{
    fnmatchmodule_state *st = get_fnmatchmodulestate_state(m);
    Py_CLEAR(st->os_module);
    Py_CLEAR(st->re_module);
    Py_CLEAR(st->lru_cache);
    return 0;
}

static int
fnmatchmodule_traverse(PyObject *m, visitproc visit, void *arg)
{
    fnmatchmodule_state *st = get_fnmatchmodulestate_state(m);
    Py_VISIT(st->os_module);
    Py_VISIT(st->re_module);
    Py_VISIT(st->lru_cache);
    return 0;
}

static void
fnmatchmodule_free(void *m)
{
    fnmatchmodule_clear((PyObject *) m);
}

static int
fnmatchmodule_exec(PyObject *m)
{
#define IMPORT_MODULE(attr, name) \
    do { \
        state->attr = PyImport_ImportModule((name)); \
        if (state->attr == NULL) { \
            return -1; \
        } \
    } while (0)

#define INTERN_STRING(attr, str) \
    do { \
        state->attr = PyUnicode_InternFromString((str)); \
        if (state->attr == NULL) { \
            return -1; \
        } \
    } while (0)

    fnmatchmodule_state *state = get_fnmatchmodulestate_state(m);

    // imports
    IMPORT_MODULE(os_module, "os");
    IMPORT_MODULE(re_module, "re");

    // helpers
    state->lru_cache = _PyImport_GetModuleAttrString("functools", "lru_cache");
    if (state->lru_cache == NULL) {
        return -1;
    }
    // todo: handle LRU cache

#undef IMPORT_MODULE
#undef INTERN_STRING

    return 0;
}

/*[clinic input]
module _fnmatch
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=356e324d57d93f08]*/

static PyObject *
get_match_function(PyObject *module, PyObject *pattern)
{
    // TODO(picnixz): use LRU-cache
    PyObject *expr = _fnmatch_translate_impl(module, pattern);
    if (expr == NULL) {
        return NULL;
    }
    fnmatchmodule_state *st = get_fnmatchmodulestate_state(module);
    PyObject *compiled = _PyObject_CallMethod(st->re_module, &_Py_ID(compile), "O", expr);
    Py_DECREF(expr);
    if (compiled == NULL) {
        return NULL;
    }
    PyObject *matcher = PyObject_GetAttr(compiled, &_Py_ID(match));
    Py_DECREF(compiled);
    return matcher;
}

static PyMethodDef get_match_function_method_def = {
    "get_match_function",
    _PyCFunction_CAST(get_match_function),
    METH_O,
    NULL
};

/*[clinic input]
_fnmatch.filter -> object

    names: object
    pat: object

[clinic start generated code]*/

static PyObject *
_fnmatch_filter_impl(PyObject *module, PyObject *names, PyObject *pat)
/*[clinic end generated code: output=7f11aa68436d05fc input=1d233174e1c4157a]*/
{
#ifndef Py_HAVE_FNMATCH
    PyObject *matcher = get_match_function(module, pat);
    if (matcher == NULL) {
        return NULL;
    }
    PyObject *result = _regex_fnmatch_filter(matcher, names);
    Py_DECREF(matcher);
    return result;
#else
    // Note that the Python implementation of fnmatch.filter() does not
    // call os.fspath() on the names being matched, whereas it does on NT.
    if (PyBytes_Check(pat)) {
        const char *pattern = PyBytes_AS_STRING(pat);
        return _posix_fnmatch_filter(pattern, names, &_posix_fnmatch_encoded);
    }
    if (PyUnicode_Check(pat)) {
        const char *pattern = PyUnicode_AsUTF8(pat);
        return _posix_fnmatch_filter(pattern, names, &_posix_fnmatch_unicode);
    }
    PyErr_SetString(PyExc_TypeError, INVALID_PATTERN_TYPE);
    return NULL;
#endif
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
#ifndef Py_HAVE_FNMATCH
    PyObject *matcher = get_match_function(module, pat);
    if (matcher == NULL) {
        return -1;
    }
    int res = _regex_fnmatch_generic(matcher, name);
    Py_DECREF(matcher);
    return res;
#else
    // This function does not transform path-like objects, nor does it
    // case-normalize 'name' or 'pattern' (whether it is the Python or
    // the C implementation).
    if (PyBytes_Check(pat)) {
        const char *pattern = PyBytes_AS_STRING(pat);
        return _posix_fnmatch_encoded(pattern, name);
    }
    if (PyUnicode_Check(pat)) {
        const char *pattern = PyUnicode_AsUTF8(pat);
        return _posix_fnmatch_unicode(pattern, name);
    }
    PyErr_SetString(PyExc_TypeError, INVALID_PATTERN_TYPE);
    return -1;
#endif
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
        PyObject *str_expr = translate(module, unicode);
        Py_DECREF(unicode);
        if (str_expr == NULL) {
            return NULL;
        }
        PyObject *expr = PyUnicode_AsLatin1String(str_expr);
        Py_DECREF(str_expr);
        return expr;
    }
    else if (PyUnicode_Check(pattern)) {
        return translate(module, pattern);
    }
    else {
        PyErr_SetString(PyExc_TypeError, INVALID_PATTERN_TYPE);
        return NULL;
    }
}

static PyMethodDef fnmatchmodule_methods[] = {
    _FNMATCH_FILTER_METHODDEF
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
    "_fnmatch",
    NULL,
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
