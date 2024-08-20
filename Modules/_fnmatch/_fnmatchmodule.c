/*
 * C accelerator for the 'fnmatch' module.
 */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "macros.h"
#include "util.h"                       // prototypes

#include "pycore_runtime.h"             // for _Py_ID()

#include "clinic/_fnmatchmodule.c.h"

#define LRU_CACHE_SIZE          32768
#define INVALID_PATTERN_TYPE    "pattern must be a string or a bytes object"

// ==== Cached re.escape() unit ===============================================

/* Create an LRU-cached function for re.escape(). */
static int
fnmatchmodule_load_escapefunc(PyObject *Py_UNUSED(module),
                              fnmatchmodule_state *st)
{
    // make sure that this function is called once
    assert(st->re_escape == NULL);
    PyObject *maxsize = PyLong_FromLong(LRU_CACHE_SIZE);
    CHECK_NOT_NULL_OR_ABORT(maxsize);
    PyObject *cache = _PyImport_GetModuleAttrString("functools", "lru_cache");
    if (cache == NULL) {
        Py_DECREF(maxsize);
        return -1;
    }
    PyObject *wrapper = PyObject_CallOneArg(cache, maxsize);
    Py_DECREF(maxsize);
    Py_DECREF(cache);
    CHECK_NOT_NULL_OR_ABORT(wrapper);
    PyObject *wrapped = _PyImport_GetModuleAttrString("re", "escape");
    if (wrapped == NULL) {
        Py_DECREF(wrapper);
        return -1;
    }
    st->re_escape = PyObject_CallOneArg(wrapper, wrapped);
    Py_DECREF(wrapped);
    Py_DECREF(wrapper);
    CHECK_NOT_NULL_OR_ABORT(st->re_escape);
    return 0;
abort:
    return -1;
}

// ==== Cached re.sub() unit for set operation tokens =========================

/* Store a reference to re.compile('([&~|])').sub(). */
static int
fnmatchmodule_load_setops_re_sub(PyObject *Py_UNUSED(module),
                                 fnmatchmodule_state *st)
{
    // make sure that this function is called once
    assert(st->setops_re_subfn == NULL);
    PyObject *pattern = PyUnicode_FromStringAndSize("([&~|])", 7);
    CHECK_NOT_NULL_OR_ABORT(pattern);
    PyObject *re_compile = _PyImport_GetModuleAttrString("re", "compile");
    if (re_compile == NULL) {
        Py_DECREF(pattern);
        return -1;
    }
    PyObject *compiled = PyObject_CallOneArg(re_compile, pattern);
    Py_DECREF(re_compile);
    Py_DECREF(pattern);
    CHECK_NOT_NULL_OR_ABORT(compiled);
    st->setops_re_subfn = PyObject_GetAttr(compiled, &_Py_ID(sub));
    Py_DECREF(compiled);
    CHECK_NOT_NULL_OR_ABORT(st->setops_re_subfn);
    return 0;
abort:
    return -1;
}

// ==== Module state functions ================================================

static int
fnmatchmodule_exec(PyObject *module)
{
    // ---- def local macros --------------------------------------------------
    /* Intern a literal STRING and store it in 'STATE->ATTRIBUTE'. */
#define INTERN_STRING(STATE, ATTRIBUTE, STRING)                     \
    do {                                                            \
        STATE->ATTRIBUTE = PyUnicode_InternFromString((STRING));    \
        CHECK_NOT_NULL_OR_ABORT(STATE->ATTRIBUTE);                  \
    } while (0)
    // ------------------------------------------------------------------------
    fnmatchmodule_state *st = get_fnmatchmodule_state(module);
    CHECK_RET_CODE_OR_ABORT(fnmatchmodule_load_escapefunc(module, st));
    INTERN_STRING(st, hyphen_str, "-");
    INTERN_STRING(st, hyphen_esc_str, "\\-");
    INTERN_STRING(st, backslash_str, "\\");
    INTERN_STRING(st, backslash_esc_str, "\\\\");
    CHECK_RET_CODE_OR_ABORT(fnmatchmodule_load_setops_re_sub(module, st));
    INTERN_STRING(st, setops_repl_str, "\\\\\\1");
    return 0;
abort:
    return -1;
#undef INTERN_STRING
}

static int
fnmatchmodule_traverse(PyObject *m, visitproc visit, void *arg)
{
    fnmatchmodule_state *st = get_fnmatchmodule_state(m);
    Py_VISIT(st->setops_repl_str);
    Py_VISIT(st->setops_re_subfn);
    Py_VISIT(st->backslash_esc_str);
    Py_VISIT(st->backslash_str);
    Py_VISIT(st->hyphen_esc_str);
    Py_VISIT(st->hyphen_str);
    Py_VISIT(st->re_escape);
    return 0;
}

static int
fnmatchmodule_clear(PyObject *m)
{
    fnmatchmodule_state *st = get_fnmatchmodule_state(m);
    Py_CLEAR(st->setops_repl_str);
    Py_CLEAR(st->setops_re_subfn);
    Py_CLEAR(st->backslash_esc_str);
    Py_CLEAR(st->backslash_str);
    Py_CLEAR(st->hyphen_esc_str);
    Py_CLEAR(st->hyphen_str);
    Py_CLEAR(st->re_escape);
    return 0;
}

static inline void
fnmatchmodule_free(void *m)
{
    (void)fnmatchmodule_clear((PyObject *)m);
}

/*[clinic input]
module fnmatch
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=797aa965370a9ef2]*/

/*[clinic input]
fnmatch.translate -> object

    pat as pattern: object

Translate a shell pattern *pat* to a regular expression.

There is no way to quote meta-characters.
[clinic start generated code]*/

static PyObject *
fnmatch_translate_impl(PyObject *module, PyObject *pattern)
/*[clinic end generated code: output=77e0f5de9fbb59bd input=2cc1203a34c571fd]*/
{
    if (PyBytes_Check(pattern)) {
        PyObject *decoded = PyUnicode_DecodeLatin1(PyBytes_AS_STRING(pattern),
                                                   PyBytes_GET_SIZE(pattern),
                                                   "strict");
        CHECK_NOT_NULL_OR_ABORT(decoded);
        PyObject *translated = _Py_fnmatch_translate(module, decoded);
        Py_DECREF(decoded);
        CHECK_NOT_NULL_OR_ABORT(translated);
        PyObject *res = PyUnicode_AsLatin1String(translated);
        Py_DECREF(translated);
        return res;
    }
    else if (PyUnicode_Check(pattern)) {
        return _Py_fnmatch_translate(module, pattern);
    }
    else {
        PyErr_SetString(PyExc_TypeError, INVALID_PATTERN_TYPE);
        return NULL;
    }
abort:
    return NULL;
}

// ==== Module specs ==========================================================

static PyMethodDef fnmatchmodule_methods[] = {
    FNMATCH_TRANSLATE_METHODDEF
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

#undef INVALID_PATTERN_TYPE
#undef LRU_CACHE_SIZE
