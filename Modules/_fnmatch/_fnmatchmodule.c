/*
 * C accelerator for the 'fnmatch' module.
 *
 * Currently, the following inconsistencies in the Python implementation exist:
 *
 * - fnmatch.filter(NAMES, PATTERN) works with pathlib.Path() instances
 *   in NAMES on Windows but raises a TypeError on POSIX platforms.
 *
 * The reason is that os.path.normcase() is called on each NAME in NAMES
 * but not on POSIX platforms. In particular, os.fspath() is never called:
 *
 *      POSIX       fnmatch.filter([Path("a")], "*") -> TypeError
 *      Windows     fnmatch.filter([Path("a")], "*") -> [Path("a")]
 *
 * - Case normalization uses the runtime value of os.path.normcase(),
 *   forcing us to query the attribute each time.
 *
 * The C implementation of fnmatch.filter() uses the same os.path.normcase()
 * when iterating over NAMES, ignoring side-effects on os.path.normcase()
 * that may occur when processing a NAME in NAMES.
 *
 * More generally, os.path.normcase() is retrieved at most once per call
 * to fnmatch.filter() or fnmatch.fnmatch().
 */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "util.h"                       // prototypes

#include "pycore_runtime.h"             // for _Py_ID()

#include "clinic/_fnmatchmodule.c.h"

#define COMPILED_CACHE_SIZE     32768
#define INVALID_PATTERN_TYPE    "pattern must be a string or a bytes object"

// ==== Cached translation unit ===============================================

/*
 * Compile a UNIX shell pattern into a RE pattern
 * and returns the corresponding 'match()' method.
 *
 * This function is LRU-cached by the module itself.
 */
static PyObject *
get_matcher_function_impl(PyObject *module, PyObject *pattern)
{
    // translate the pattern into a RE pattern
    assert(module != NULL);
    PyObject *translated = fnmatch_translate_impl(module, pattern);
    if (translated == NULL) {
        return NULL;
    }
    fnmatchmodule_state *st = get_fnmatchmodule_state(module);
    // compile the pattern
    PyObject *compile_func = PyObject_GetAttr(st->re_module, &_Py_ID(compile));
    if (compile_func == NULL) {
        Py_DECREF(translated);
        return NULL;
    }
    PyObject *compiled = PyObject_CallOneArg(compile_func, translated);
    Py_DECREF(compile_func);
    Py_DECREF(translated);
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
    get_matcher_function_impl,
    METH_O,
    NULL
};

static int
fnmatchmodule_load_translator(PyObject *module, fnmatchmodule_state *st)
{
    // make sure that this function is called once
    assert(st->translator == NULL);
    PyObject *maxsize = PyLong_FromLong(COMPILED_CACHE_SIZE);
    if (maxsize == NULL) {
        return -1;
    }
    PyObject *cache = _PyImport_GetModuleAttrString("functools", "lru_cache");
    if (cache == NULL) {
        Py_DECREF(maxsize);
        return -1;
    }
    PyObject *args[3] = {NULL, maxsize, Py_True};
    size_t nargsf = 2 | PY_VECTORCALL_ARGUMENTS_OFFSET;
    PyObject *wrapper = PyObject_Vectorcall(cache, &args[1], nargsf, NULL);
    Py_DECREF(maxsize);
    Py_DECREF(cache);
    if (wrapper == NULL) {
        return -1;
    }
    assert(module != NULL);
    PyObject *wrapped = PyCFunction_New(&get_matcher_function_def, module);
    // reference on 'translator' will be removed upon module cleanup
    st->translator = PyObject_CallOneArg(wrapper, wrapped);
    Py_DECREF(wrapped);
    Py_DECREF(wrapper);
    if (st->translator == NULL) {
        return -1;
    }
    return 0;
}

// ==== Module data getters ===================================================

static inline PyObject * /* reference to re.compile(pattern).match() */
get_matcher_function(PyObject *module, PyObject *pattern)
{
    fnmatchmodule_state *st = get_fnmatchmodule_state(module);
    assert(st->translator != NULL);
    return PyObject_CallOneArg(st->translator, pattern);
}

static inline PyObject * /* reference to os.path.normcase() */
get_platform_normcase_function(PyObject *module, bool *isposix)
{
    fnmatchmodule_state *st = get_fnmatchmodule_state(module);
    PyObject *os_path = PyObject_GetAttr(st->os_module, &_Py_ID(path));
    if (os_path == NULL) {
        return NULL;
    }
    PyObject *normcase = PyObject_GetAttr(os_path, &_Py_ID(normcase));
    if (isposix != NULL) {
        *isposix = Py_Is(os_path, st->posixpath_module);
    }
    Py_DECREF(os_path);
    return normcase;
}

// ==== Module state functions ================================================

/* Import a named module and store it in 'STATE->ATTRIBUTE'. */
#define IMPORT_MODULE(STATE, ATTRIBUTE, MODULE_NAME)                \
    do {                                                            \
        /* make sure that the attribute is initialized once */      \
        assert(STATE->ATTRIBUTE == NULL);                           \
        STATE->ATTRIBUTE = PyImport_ImportModule((MODULE_NAME));    \
        if (STATE->ATTRIBUTE == NULL) {                             \
            return -1;                                              \
        }                                                           \
    } while (0)

/* Intern a literal STRING and store it in 'STATE->ATTRIBUTE'. */
#define INTERN_STRING(STATE, ATTRIBUTE, STRING)                     \
    do {                                                            \
        /* make sure that the attribute is initialized once */      \
        assert(STATE->ATTRIBUTE == NULL);                           \
        STATE->ATTRIBUTE = PyUnicode_InternFromString((STRING));    \
        if (STATE->ATTRIBUTE == NULL) {                             \
            return -1;                                              \
        }                                                           \
    } while (0)

static int
fnmatchmodule_exec(PyObject *module)
{
    fnmatchmodule_state *st = get_fnmatchmodule_state(module);
    IMPORT_MODULE(st, os_module, "os");
    IMPORT_MODULE(st, posixpath_module, "posixpath");
    IMPORT_MODULE(st, re_module, "re");
    if (fnmatchmodule_load_translator(module, st) < 0) {
        return -1;
    }
    INTERN_STRING(st, hyphen_str, "-");
    INTERN_STRING(st, hyphen_esc_str, "\\-");
    INTERN_STRING(st, backslash_str, "\\");
    INTERN_STRING(st, backslash_esc_str, "\\\\");
    INTERN_STRING(st, setops_str, "([&~|])");
    INTERN_STRING(st, setops_repl_str, "\\\\\\1");
    return 0;
}

#undef INTERN_STRING
#undef IMPORT_MODULE

static int
fnmatchmodule_traverse(PyObject *m, visitproc visit, void *arg)
{
    fnmatchmodule_state *st = get_fnmatchmodule_state(m);
    Py_VISIT(st->setops_repl_str);
    Py_VISIT(st->setops_str);
    Py_VISIT(st->backslash_esc_str);
    Py_VISIT(st->backslash_str);
    Py_VISIT(st->hyphen_esc_str);
    Py_VISIT(st->hyphen_str);
    Py_VISIT(st->translator);
    Py_VISIT(st->re_module);
    Py_VISIT(st->posixpath_module);
    Py_VISIT(st->os_module);
    return 0;
}

static int
fnmatchmodule_clear(PyObject *m)
{
    fnmatchmodule_state *st = get_fnmatchmodule_state(m);
    Py_CLEAR(st->setops_repl_str);
    Py_CLEAR(st->setops_str);
    Py_CLEAR(st->backslash_esc_str);
    Py_CLEAR(st->backslash_str);
    Py_CLEAR(st->hyphen_esc_str);
    Py_CLEAR(st->hyphen_str);
    Py_CLEAR(st->translator);
    Py_CLEAR(st->re_module);
    Py_CLEAR(st->posixpath_module);
    Py_CLEAR(st->os_module);
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
fnmatch.filter -> object

    names: object
    pat as pattern: object

Construct a list from the names in *names* matching *pat*.

[clinic start generated code]*/

static PyObject *
fnmatch_filter_impl(PyObject *module, PyObject *names, PyObject *pattern)
/*[clinic end generated code: output=1a68530a2e3cf7d0 input=7ac729daad3b1404]*/
{
    bool isposix = 0;
    PyObject *normcase = get_platform_normcase_function(module, &isposix);
    if (normcase == NULL) {
        return NULL;
    }
    PyObject *normalized_pattern = PyObject_CallOneArg(normcase, pattern);
    if (normalized_pattern == NULL) {
        Py_DECREF(normcase);
        return NULL;
    }
    // the matcher is cached with respect to the *normalized* pattern
    PyObject *matcher = get_matcher_function(module, normalized_pattern);
    Py_DECREF(normalized_pattern);
    if (matcher == NULL) {
        Py_DECREF(normcase);
        return NULL;
    }
    PyObject *normalizer = isposix ? NULL : normcase;
    PyObject *filtered = _Py_fnmatch_filter(matcher, names, normalizer);
    Py_DECREF(matcher);
    Py_DECREF(normcase);
    return filtered;
}

/*[clinic input]
fnmatch.fnmatch -> bool

    name: object
    pat as pattern: object

Test whether *name* matches *pat*.

Patterns are Unix shell style:

*       matches everything
?       matches any single character
[seq]   matches any character in seq
[!seq]  matches any char not in seq

An initial period in *name* is not special.
Both *name* and *pat* are first case-normalized
if the operating system requires it.

If you don't want this, use fnmatchcase(name, pat).

[clinic start generated code]*/

static int
fnmatch_fnmatch_impl(PyObject *module, PyObject *name, PyObject *pattern)
/*[clinic end generated code: output=c9dc542e8d6933b6 input=279a4a4f2ddea6a2]*/
{
    // use the runtime 'os.path' value and not a cached one
    PyObject *normcase = get_platform_normcase_function(module, NULL);
    if (normcase == NULL) {
        return -1;
    }
    // apply case normalization on both arguments
    PyObject *norm_name = PyObject_CallOneArg(normcase, name);
    if (norm_name == NULL) {
        Py_DECREF(normcase);
        return -1;
    }
    PyObject *norm_pattern = PyObject_CallOneArg(normcase, pattern);
    Py_DECREF(normcase);
    if (norm_pattern == NULL) {
        Py_DECREF(norm_name);
        return -1;
    }
    int matching = fnmatch_fnmatchcase_impl(module, norm_name, norm_pattern);
    Py_DECREF(norm_pattern);
    Py_DECREF(norm_name);
    return matching;
}

/*[clinic input]
fnmatch.fnmatchcase -> bool

    name: object
    pat as pattern: object

Test whether *name* matches *pat*, including case.

This is a version of fnmatch() which doesn't case-normalize
its arguments.
[clinic start generated code]*/

static int
fnmatch_fnmatchcase_impl(PyObject *module, PyObject *name, PyObject *pattern)
/*[clinic end generated code: output=4d6b268169001876 input=91d62999c08fd55e]*/
{
    // fnmatchcase() does not apply any case normalization on the inputs
    PyObject *matcher = get_matcher_function(module, pattern);
    if (matcher == NULL) {
        return -1;
    }
    // If 'name' is of incorrect type, it will be detected when calling
    // the matcher function (we check 're.compile(pattern).match(name)').
    PyObject *match = PyObject_CallOneArg(matcher, name);
    Py_DECREF(matcher);
    int matching = match == NULL ? -1 : !Py_IsNone(match);
    Py_XDECREF(match);
    return matching;
}

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
        if (decoded == NULL) {
            return NULL;
        }
        PyObject *translated = _Py_fnmatch_translate(module, decoded);
        Py_DECREF(decoded);
        if (translated == NULL) {
            return NULL;
        }
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
}

// ==== Module specs ==========================================================

// fmt: off
PyDoc_STRVAR(fnmatchmodule_doc,
"Filename matching with shell patterns.\n"
"fnmatch(FILENAME, PATTERN) matches according to the local convention.\n"
"fnmatchcase(FILENAME, PATTERN) always takes case in account.\n\n"
"The functions operate by translating the pattern into a regular\n"
"expression.  They cache the compiled regular expressions for speed.\n\n"
"The function translate(PATTERN) returns a regular expression\n"
"corresponding to PATTERN.  (It does not compile it.)");
// fmt: on

static PyMethodDef fnmatchmodule_methods[] = {
    FNMATCH_FILTER_METHODDEF
    FNMATCH_FNMATCH_METHODDEF
    FNMATCH_FNMATCHCASE_METHODDEF
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
    .m_doc = fnmatchmodule_doc,
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
#undef COMPILED_CACHE_SIZE
