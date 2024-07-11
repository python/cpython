#include "Python.h"
#include "pycore_call.h"

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
    PyObject *compiled = PyObject_CallMethodOneArg(st->re_module, &_Py_ID(compile), expr);
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
    PyObject *decorator = PyObject_CallFunctionObjArgs(st->lru_cache, maxsize, Py_True, NULL);
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
    fnmatchmodule_state *st = get_fnmatchmodulestate_state(module);
    assert(st->translator != NULL);
    return PyObject_CallOneArg(st->translator, pattern);
}

// ==== Module state functions ================================================

static int
fnmatchmodule_exec(PyObject *module)
{
#define IMPORT_MODULE(attribute, name) \
    do { \
        st->attribute = NULL; \
        st->attribute = PyImport_ImportModule((name)); \
        if (st->attribute == NULL) { \
            return -1; \
        } \
    } while (0)

    fnmatchmodule_state *st = get_fnmatchmodulestate_state(module);
    IMPORT_MODULE(os_module, "os");
    IMPORT_MODULE(posixpath_module, "posixpath");
    IMPORT_MODULE(re_module, "re");
#undef IMPORT_MODULE
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

static int
fnmatchmodule_traverse(PyObject *m, visitproc visit, void *arg)
{
    fnmatchmodule_state *st = get_fnmatchmodulestate_state(m);
    Py_VISIT(st->os_module);
    Py_VISIT(st->posixpath_module);
    Py_VISIT(st->re_module);
    Py_VISIT(st->lru_cache);
    Py_VISIT(st->translator);
    return 0;
}

static int
fnmatchmodule_clear(PyObject *m)
{
    fnmatchmodule_state *st = get_fnmatchmodulestate_state(m);
    Py_CLEAR(st->os_module);
    Py_CLEAR(st->posixpath_module);
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
    fnmatchmodule_state *st = get_fnmatchmodulestate_state(module);
    PyObject *os_path = PyObject_GetAttr(st->os_module, &_Py_ID(path));
    if (os_path == NULL) {
        return NULL;
    }
    // filter() always calls os.path.normcase() on the pattern,
    // but not on the names being mathed if os.path is posixmodule
    // XXX: maybe this should be changed in Python as well?
    // Note: the Python implementation uses the *runtime* os.path.normcase.
    PyObject *normcase = PyObject_GetAttr(os_path, &_Py_ID(normcase));
    if (normcase == NULL) {
        Py_DECREF(os_path);
        return NULL;
    }
    PyObject *patobj = PyObject_CallOneArg(normcase, pat);
    if (patobj == NULL) {
        Py_DECREF(normcase);
        Py_DECREF(os_path);
        return NULL;
    }
    int isposix = Py_Is(os_path, st->posixpath_module);
    Py_DECREF(os_path);
    // the matcher is cached with respect to the *normalized* pattern
    PyObject *matcher = get_matcher_function(module, patobj);
    Py_DECREF(patobj);
    if (matcher == NULL) {
        Py_DECREF(normcase);
        return NULL;
    }
    PyObject *result = isposix
        ? _Py_fnmatch_filter(matcher, names)
        : _Py_fnmatch_filter_normalized(matcher, names, normcase);
    Py_DECREF(matcher);
    Py_DECREF(normcase);
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
    // use the runtime 'os.path' value and not a cached one
    PyObject *os_path = PyObject_GetAttr(st->os_module, &_Py_ID(path));
    if (os_path == NULL) {
        return -1;
    }
    PyObject *normcase = PyObject_GetAttr(os_path, &_Py_ID(normcase));
    Py_DECREF(os_path);
    if (normcase == NULL) {
        return -1;
    }
    // apply case normalization on both arguments
    PyObject *nameobj = PyObject_CallOneArg(normcase, name);
    if (nameobj == NULL) {
        Py_DECREF(normcase);
        return -1;
    }
    PyObject *patobj = PyObject_CallOneArg(normcase, pat);
    Py_DECREF(normcase);
    if (patobj == NULL) {
        Py_DECREF(nameobj);
        return -1;
    }
    int matching = _fnmatch_fnmatchcase_impl(module, nameobj, patobj);
    Py_DECREF(patobj);
    Py_DECREF(nameobj);
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
    // fnmatchcase() does not apply any case normalization on the inputs
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
