/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_sre_getcodesize__doc__,
"getcodesize($module, /)\n"
"--\n"
"\n");

#define _SRE_GETCODESIZE_METHODDEF    \
    {"getcodesize", (PyCFunction)_sre_getcodesize, METH_NOARGS, _sre_getcodesize__doc__},

static int
_sre_getcodesize_impl(PyObject *module);

static PyObject *
_sre_getcodesize(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = _sre_getcodesize_impl(module);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_ascii_iscased__doc__,
"ascii_iscased($module, character, /)\n"
"--\n"
"\n");

#define _SRE_ASCII_ISCASED_METHODDEF    \
    {"ascii_iscased", (PyCFunction)_sre_ascii_iscased, METH_O, _sre_ascii_iscased__doc__},

static int
_sre_ascii_iscased_impl(PyObject *module, int character);

static PyObject *
_sre_ascii_iscased(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int character;
    int _return_value;

    character = PyLong_AsInt(arg);
    if (character == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _sre_ascii_iscased_impl(module, character);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_unicode_iscased__doc__,
"unicode_iscased($module, character, /)\n"
"--\n"
"\n");

#define _SRE_UNICODE_ISCASED_METHODDEF    \
    {"unicode_iscased", (PyCFunction)_sre_unicode_iscased, METH_O, _sre_unicode_iscased__doc__},

static int
_sre_unicode_iscased_impl(PyObject *module, int character);

static PyObject *
_sre_unicode_iscased(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int character;
    int _return_value;

    character = PyLong_AsInt(arg);
    if (character == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _sre_unicode_iscased_impl(module, character);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_ascii_tolower__doc__,
"ascii_tolower($module, character, /)\n"
"--\n"
"\n");

#define _SRE_ASCII_TOLOWER_METHODDEF    \
    {"ascii_tolower", (PyCFunction)_sre_ascii_tolower, METH_O, _sre_ascii_tolower__doc__},

static int
_sre_ascii_tolower_impl(PyObject *module, int character);

static PyObject *
_sre_ascii_tolower(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int character;
    int _return_value;

    character = PyLong_AsInt(arg);
    if (character == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _sre_ascii_tolower_impl(module, character);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_unicode_tolower__doc__,
"unicode_tolower($module, character, /)\n"
"--\n"
"\n");

#define _SRE_UNICODE_TOLOWER_METHODDEF    \
    {"unicode_tolower", (PyCFunction)_sre_unicode_tolower, METH_O, _sre_unicode_tolower__doc__},

static int
_sre_unicode_tolower_impl(PyObject *module, int character);

static PyObject *
_sre_unicode_tolower(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int character;
    int _return_value;

    character = PyLong_AsInt(arg);
    if (character == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _sre_unicode_tolower_impl(module, character);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern_match__doc__,
"match($self, /, string, pos=0, endpos=sys.maxsize)\n"
"--\n"
"\n"
"Matches zero or more characters at the beginning of the string.");

#define _SRE_SRE_PATTERN_MATCH_METHODDEF    \
    {"match", _PyCFunction_CAST(_sre_SRE_Pattern_match), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _sre_SRE_Pattern_match__doc__},

static PyObject *
_sre_SRE_Pattern_match_impl(PatternObject *self, PyTypeObject *cls,
                            PyObject *string, Py_ssize_t pos,
                            Py_ssize_t endpos);

static PyObject *
_sre_SRE_Pattern_match(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(string), &_Py_ID(pos), &_Py_ID(endpos), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"string", "pos", "endpos", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "match",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *string;
    Py_ssize_t pos = 0;
    Py_ssize_t endpos = PY_SSIZE_T_MAX;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    string = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        {
            Py_ssize_t ival = -1;
            PyObject *iobj = _PyNumber_Index(args[1]);
            if (iobj != NULL) {
                ival = PyLong_AsSsize_t(iobj);
                Py_DECREF(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            pos = ival;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[2]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        endpos = ival;
    }
skip_optional_pos:
    return_value = _sre_SRE_Pattern_match_impl((PatternObject *)self, cls, string, pos, endpos);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern_fullmatch__doc__,
"fullmatch($self, /, string, pos=0, endpos=sys.maxsize)\n"
"--\n"
"\n"
"Matches against all of the string.");

#define _SRE_SRE_PATTERN_FULLMATCH_METHODDEF    \
    {"fullmatch", _PyCFunction_CAST(_sre_SRE_Pattern_fullmatch), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _sre_SRE_Pattern_fullmatch__doc__},

static PyObject *
_sre_SRE_Pattern_fullmatch_impl(PatternObject *self, PyTypeObject *cls,
                                PyObject *string, Py_ssize_t pos,
                                Py_ssize_t endpos);

static PyObject *
_sre_SRE_Pattern_fullmatch(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(string), &_Py_ID(pos), &_Py_ID(endpos), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"string", "pos", "endpos", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "fullmatch",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *string;
    Py_ssize_t pos = 0;
    Py_ssize_t endpos = PY_SSIZE_T_MAX;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    string = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        {
            Py_ssize_t ival = -1;
            PyObject *iobj = _PyNumber_Index(args[1]);
            if (iobj != NULL) {
                ival = PyLong_AsSsize_t(iobj);
                Py_DECREF(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            pos = ival;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[2]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        endpos = ival;
    }
skip_optional_pos:
    return_value = _sre_SRE_Pattern_fullmatch_impl((PatternObject *)self, cls, string, pos, endpos);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern_search__doc__,
"search($self, /, string, pos=0, endpos=sys.maxsize)\n"
"--\n"
"\n"
"Scan through string looking for a match, and return a corresponding match object instance.\n"
"\n"
"Return None if no position in the string matches.");

#define _SRE_SRE_PATTERN_SEARCH_METHODDEF    \
    {"search", _PyCFunction_CAST(_sre_SRE_Pattern_search), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _sre_SRE_Pattern_search__doc__},

static PyObject *
_sre_SRE_Pattern_search_impl(PatternObject *self, PyTypeObject *cls,
                             PyObject *string, Py_ssize_t pos,
                             Py_ssize_t endpos);

static PyObject *
_sre_SRE_Pattern_search(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(string), &_Py_ID(pos), &_Py_ID(endpos), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"string", "pos", "endpos", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "search",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *string;
    Py_ssize_t pos = 0;
    Py_ssize_t endpos = PY_SSIZE_T_MAX;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    string = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        {
            Py_ssize_t ival = -1;
            PyObject *iobj = _PyNumber_Index(args[1]);
            if (iobj != NULL) {
                ival = PyLong_AsSsize_t(iobj);
                Py_DECREF(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            pos = ival;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[2]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        endpos = ival;
    }
skip_optional_pos:
    return_value = _sre_SRE_Pattern_search_impl((PatternObject *)self, cls, string, pos, endpos);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern_findall__doc__,
"findall($self, /, string, pos=0, endpos=sys.maxsize)\n"
"--\n"
"\n"
"Return a list of all non-overlapping matches of pattern in string.");

#define _SRE_SRE_PATTERN_FINDALL_METHODDEF    \
    {"findall", _PyCFunction_CAST(_sre_SRE_Pattern_findall), METH_FASTCALL|METH_KEYWORDS, _sre_SRE_Pattern_findall__doc__},

static PyObject *
_sre_SRE_Pattern_findall_impl(PatternObject *self, PyObject *string,
                              Py_ssize_t pos, Py_ssize_t endpos);

static PyObject *
_sre_SRE_Pattern_findall(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(string), &_Py_ID(pos), &_Py_ID(endpos), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"string", "pos", "endpos", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "findall",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *string;
    Py_ssize_t pos = 0;
    Py_ssize_t endpos = PY_SSIZE_T_MAX;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    string = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        {
            Py_ssize_t ival = -1;
            PyObject *iobj = _PyNumber_Index(args[1]);
            if (iobj != NULL) {
                ival = PyLong_AsSsize_t(iobj);
                Py_DECREF(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            pos = ival;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[2]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        endpos = ival;
    }
skip_optional_pos:
    return_value = _sre_SRE_Pattern_findall_impl((PatternObject *)self, string, pos, endpos);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern_finditer__doc__,
"finditer($self, /, string, pos=0, endpos=sys.maxsize)\n"
"--\n"
"\n"
"Return an iterator over all non-overlapping matches for the RE pattern in string.\n"
"\n"
"For each match, the iterator returns a match object.");

#define _SRE_SRE_PATTERN_FINDITER_METHODDEF    \
    {"finditer", _PyCFunction_CAST(_sre_SRE_Pattern_finditer), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _sre_SRE_Pattern_finditer__doc__},

static PyObject *
_sre_SRE_Pattern_finditer_impl(PatternObject *self, PyTypeObject *cls,
                               PyObject *string, Py_ssize_t pos,
                               Py_ssize_t endpos);

static PyObject *
_sre_SRE_Pattern_finditer(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(string), &_Py_ID(pos), &_Py_ID(endpos), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"string", "pos", "endpos", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "finditer",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *string;
    Py_ssize_t pos = 0;
    Py_ssize_t endpos = PY_SSIZE_T_MAX;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    string = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        {
            Py_ssize_t ival = -1;
            PyObject *iobj = _PyNumber_Index(args[1]);
            if (iobj != NULL) {
                ival = PyLong_AsSsize_t(iobj);
                Py_DECREF(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            pos = ival;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[2]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        endpos = ival;
    }
skip_optional_pos:
    return_value = _sre_SRE_Pattern_finditer_impl((PatternObject *)self, cls, string, pos, endpos);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern_scanner__doc__,
"scanner($self, /, string, pos=0, endpos=sys.maxsize)\n"
"--\n"
"\n");

#define _SRE_SRE_PATTERN_SCANNER_METHODDEF    \
    {"scanner", _PyCFunction_CAST(_sre_SRE_Pattern_scanner), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _sre_SRE_Pattern_scanner__doc__},

static PyObject *
_sre_SRE_Pattern_scanner_impl(PatternObject *self, PyTypeObject *cls,
                              PyObject *string, Py_ssize_t pos,
                              Py_ssize_t endpos);

static PyObject *
_sre_SRE_Pattern_scanner(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(string), &_Py_ID(pos), &_Py_ID(endpos), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"string", "pos", "endpos", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "scanner",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *string;
    Py_ssize_t pos = 0;
    Py_ssize_t endpos = PY_SSIZE_T_MAX;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    string = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        {
            Py_ssize_t ival = -1;
            PyObject *iobj = _PyNumber_Index(args[1]);
            if (iobj != NULL) {
                ival = PyLong_AsSsize_t(iobj);
                Py_DECREF(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            pos = ival;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[2]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        endpos = ival;
    }
skip_optional_pos:
    return_value = _sre_SRE_Pattern_scanner_impl((PatternObject *)self, cls, string, pos, endpos);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern_split__doc__,
"split($self, /, string, maxsplit=0)\n"
"--\n"
"\n"
"Split string by the occurrences of pattern.");

#define _SRE_SRE_PATTERN_SPLIT_METHODDEF    \
    {"split", _PyCFunction_CAST(_sre_SRE_Pattern_split), METH_FASTCALL|METH_KEYWORDS, _sre_SRE_Pattern_split__doc__},

static PyObject *
_sre_SRE_Pattern_split_impl(PatternObject *self, PyObject *string,
                            Py_ssize_t maxsplit);

static PyObject *
_sre_SRE_Pattern_split(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(string), &_Py_ID(maxsplit), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"string", "maxsplit", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "split",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *string;
    Py_ssize_t maxsplit = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    string = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        maxsplit = ival;
    }
skip_optional_pos:
    return_value = _sre_SRE_Pattern_split_impl((PatternObject *)self, string, maxsplit);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern_sub__doc__,
"sub($self, /, repl, string, count=0)\n"
"--\n"
"\n"
"Return the string obtained by replacing the leftmost non-overlapping occurrences of pattern in string by the replacement repl.");

#define _SRE_SRE_PATTERN_SUB_METHODDEF    \
    {"sub", _PyCFunction_CAST(_sre_SRE_Pattern_sub), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _sre_SRE_Pattern_sub__doc__},

static PyObject *
_sre_SRE_Pattern_sub_impl(PatternObject *self, PyTypeObject *cls,
                          PyObject *repl, PyObject *string, Py_ssize_t count);

static PyObject *
_sre_SRE_Pattern_sub(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(repl), &_Py_ID(string), &_Py_ID(count), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"repl", "string", "count", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "sub",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *repl;
    PyObject *string;
    Py_ssize_t count = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    repl = args[0];
    string = args[1];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[2]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        count = ival;
    }
skip_optional_pos:
    return_value = _sre_SRE_Pattern_sub_impl((PatternObject *)self, cls, repl, string, count);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern_subn__doc__,
"subn($self, /, repl, string, count=0)\n"
"--\n"
"\n"
"Return the tuple (new_string, number_of_subs_made) found by replacing the leftmost non-overlapping occurrences of pattern with the replacement repl.");

#define _SRE_SRE_PATTERN_SUBN_METHODDEF    \
    {"subn", _PyCFunction_CAST(_sre_SRE_Pattern_subn), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _sre_SRE_Pattern_subn__doc__},

static PyObject *
_sre_SRE_Pattern_subn_impl(PatternObject *self, PyTypeObject *cls,
                           PyObject *repl, PyObject *string,
                           Py_ssize_t count);

static PyObject *
_sre_SRE_Pattern_subn(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(repl), &_Py_ID(string), &_Py_ID(count), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"repl", "string", "count", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "subn",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *repl;
    PyObject *string;
    Py_ssize_t count = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    repl = args[0];
    string = args[1];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[2]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        count = ival;
    }
skip_optional_pos:
    return_value = _sre_SRE_Pattern_subn_impl((PatternObject *)self, cls, repl, string, count);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern___copy____doc__,
"__copy__($self, /)\n"
"--\n"
"\n");

#define _SRE_SRE_PATTERN___COPY___METHODDEF    \
    {"__copy__", (PyCFunction)_sre_SRE_Pattern___copy__, METH_NOARGS, _sre_SRE_Pattern___copy____doc__},

static PyObject *
_sre_SRE_Pattern___copy___impl(PatternObject *self);

static PyObject *
_sre_SRE_Pattern___copy__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _sre_SRE_Pattern___copy___impl((PatternObject *)self);
}

PyDoc_STRVAR(_sre_SRE_Pattern___deepcopy____doc__,
"__deepcopy__($self, memo, /)\n"
"--\n"
"\n");

#define _SRE_SRE_PATTERN___DEEPCOPY___METHODDEF    \
    {"__deepcopy__", (PyCFunction)_sre_SRE_Pattern___deepcopy__, METH_O, _sre_SRE_Pattern___deepcopy____doc__},

static PyObject *
_sre_SRE_Pattern___deepcopy___impl(PatternObject *self, PyObject *memo);

static PyObject *
_sre_SRE_Pattern___deepcopy__(PyObject *self, PyObject *memo)
{
    PyObject *return_value = NULL;

    return_value = _sre_SRE_Pattern___deepcopy___impl((PatternObject *)self, memo);

    return return_value;
}

#if defined(Py_DEBUG)

PyDoc_STRVAR(_sre_SRE_Pattern__fail_after__doc__,
"_fail_after($self, count, exception, /)\n"
"--\n"
"\n"
"For debugging.");

#define _SRE_SRE_PATTERN__FAIL_AFTER_METHODDEF    \
    {"_fail_after", _PyCFunction_CAST(_sre_SRE_Pattern__fail_after), METH_FASTCALL, _sre_SRE_Pattern__fail_after__doc__},

static PyObject *
_sre_SRE_Pattern__fail_after_impl(PatternObject *self, int count,
                                  PyObject *exception);

static PyObject *
_sre_SRE_Pattern__fail_after(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int count;
    PyObject *exception;

    if (!_PyArg_CheckPositional("_fail_after", nargs, 2, 2)) {
        goto exit;
    }
    count = PyLong_AsInt(args[0]);
    if (count == -1 && PyErr_Occurred()) {
        goto exit;
    }
    exception = args[1];
    return_value = _sre_SRE_Pattern__fail_after_impl((PatternObject *)self, count, exception);

exit:
    return return_value;
}

#endif /* defined(Py_DEBUG) */

PyDoc_STRVAR(_sre_compile__doc__,
"compile($module, /, pattern, flags, code, groups, groupindex,\n"
"        indexgroup)\n"
"--\n"
"\n");

#define _SRE_COMPILE_METHODDEF    \
    {"compile", _PyCFunction_CAST(_sre_compile), METH_FASTCALL|METH_KEYWORDS, _sre_compile__doc__},

static PyObject *
_sre_compile_impl(PyObject *module, PyObject *pattern, int flags,
                  PyObject *code, Py_ssize_t groups, PyObject *groupindex,
                  PyObject *indexgroup);

static PyObject *
_sre_compile(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 6
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(pattern), &_Py_ID(flags), &_Py_ID(code), &_Py_ID(groups), &_Py_ID(groupindex), &_Py_ID(indexgroup), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"pattern", "flags", "code", "groups", "groupindex", "indexgroup", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "compile",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[6];
    PyObject *pattern;
    int flags;
    PyObject *code;
    Py_ssize_t groups;
    PyObject *groupindex;
    PyObject *indexgroup;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 6, /*maxpos*/ 6, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    pattern = args[0];
    flags = PyLong_AsInt(args[1]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!PyList_Check(args[2])) {
        _PyArg_BadArgument("compile", "argument 'code'", "list", args[2]);
        goto exit;
    }
    code = args[2];
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[3]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        groups = ival;
    }
    if (!PyDict_Check(args[4])) {
        _PyArg_BadArgument("compile", "argument 'groupindex'", "dict", args[4]);
        goto exit;
    }
    groupindex = args[4];
    if (!PyTuple_Check(args[5])) {
        _PyArg_BadArgument("compile", "argument 'indexgroup'", "tuple", args[5]);
        goto exit;
    }
    indexgroup = args[5];
    return_value = _sre_compile_impl(module, pattern, flags, code, groups, groupindex, indexgroup);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_template__doc__,
"template($module, pattern, template, /)\n"
"--\n"
"\n"
"\n"
"\n"
"  template\n"
"    A list containing interleaved literal strings (str or bytes) and group\n"
"    indices (int), as returned by re._parser.parse_template():\n"
"        [literal1, group1, ..., literalN, groupN]");

#define _SRE_TEMPLATE_METHODDEF    \
    {"template", _PyCFunction_CAST(_sre_template), METH_FASTCALL, _sre_template__doc__},

static PyObject *
_sre_template_impl(PyObject *module, PyObject *pattern, PyObject *template);

static PyObject *
_sre_template(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *pattern;
    PyObject *template;

    if (!_PyArg_CheckPositional("template", nargs, 2, 2)) {
        goto exit;
    }
    pattern = args[0];
    if (!PyList_Check(args[1])) {
        _PyArg_BadArgument("template", "argument 2", "list", args[1]);
        goto exit;
    }
    template = args[1];
    return_value = _sre_template_impl(module, pattern, template);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Match_expand__doc__,
"expand($self, /, template)\n"
"--\n"
"\n"
"Return the string obtained by doing backslash substitution on the string template, as done by the sub() method.");

#define _SRE_SRE_MATCH_EXPAND_METHODDEF    \
    {"expand", _PyCFunction_CAST(_sre_SRE_Match_expand), METH_FASTCALL|METH_KEYWORDS, _sre_SRE_Match_expand__doc__},

static PyObject *
_sre_SRE_Match_expand_impl(MatchObject *self, PyObject *template);

static PyObject *
_sre_SRE_Match_expand(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(template), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"template", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "expand",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *template;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    template = args[0];
    return_value = _sre_SRE_Match_expand_impl((MatchObject *)self, template);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Match_groups__doc__,
"groups($self, /, default=None)\n"
"--\n"
"\n"
"Return a tuple containing all the subgroups of the match, from 1.\n"
"\n"
"  default\n"
"    Is used for groups that did not participate in the match.");

#define _SRE_SRE_MATCH_GROUPS_METHODDEF    \
    {"groups", _PyCFunction_CAST(_sre_SRE_Match_groups), METH_FASTCALL|METH_KEYWORDS, _sre_SRE_Match_groups__doc__},

static PyObject *
_sre_SRE_Match_groups_impl(MatchObject *self, PyObject *default_value);

static PyObject *
_sre_SRE_Match_groups(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(default), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"default", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "groups",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *default_value = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    default_value = args[0];
skip_optional_pos:
    return_value = _sre_SRE_Match_groups_impl((MatchObject *)self, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Match_groupdict__doc__,
"groupdict($self, /, default=None)\n"
"--\n"
"\n"
"Return a dictionary containing all the named subgroups of the match, keyed by the subgroup name.\n"
"\n"
"  default\n"
"    Is used for groups that did not participate in the match.");

#define _SRE_SRE_MATCH_GROUPDICT_METHODDEF    \
    {"groupdict", _PyCFunction_CAST(_sre_SRE_Match_groupdict), METH_FASTCALL|METH_KEYWORDS, _sre_SRE_Match_groupdict__doc__},

static PyObject *
_sre_SRE_Match_groupdict_impl(MatchObject *self, PyObject *default_value);

static PyObject *
_sre_SRE_Match_groupdict(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(default), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"default", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "groupdict",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *default_value = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    default_value = args[0];
skip_optional_pos:
    return_value = _sre_SRE_Match_groupdict_impl((MatchObject *)self, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Match_start__doc__,
"start($self, group=0, /)\n"
"--\n"
"\n"
"Return index of the start of the substring matched by group.");

#define _SRE_SRE_MATCH_START_METHODDEF    \
    {"start", _PyCFunction_CAST(_sre_SRE_Match_start), METH_FASTCALL, _sre_SRE_Match_start__doc__},

static Py_ssize_t
_sre_SRE_Match_start_impl(MatchObject *self, PyObject *group);

static PyObject *
_sre_SRE_Match_start(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *group = NULL;
    Py_ssize_t _return_value;

    if (!_PyArg_CheckPositional("start", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    group = args[0];
skip_optional:
    _return_value = _sre_SRE_Match_start_impl((MatchObject *)self, group);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Match_end__doc__,
"end($self, group=0, /)\n"
"--\n"
"\n"
"Return index of the end of the substring matched by group.");

#define _SRE_SRE_MATCH_END_METHODDEF    \
    {"end", _PyCFunction_CAST(_sre_SRE_Match_end), METH_FASTCALL, _sre_SRE_Match_end__doc__},

static Py_ssize_t
_sre_SRE_Match_end_impl(MatchObject *self, PyObject *group);

static PyObject *
_sre_SRE_Match_end(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *group = NULL;
    Py_ssize_t _return_value;

    if (!_PyArg_CheckPositional("end", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    group = args[0];
skip_optional:
    _return_value = _sre_SRE_Match_end_impl((MatchObject *)self, group);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Match_span__doc__,
"span($self, group=0, /)\n"
"--\n"
"\n"
"For match object m, return the 2-tuple (m.start(group), m.end(group)).");

#define _SRE_SRE_MATCH_SPAN_METHODDEF    \
    {"span", _PyCFunction_CAST(_sre_SRE_Match_span), METH_FASTCALL, _sre_SRE_Match_span__doc__},

static PyObject *
_sre_SRE_Match_span_impl(MatchObject *self, PyObject *group);

static PyObject *
_sre_SRE_Match_span(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *group = NULL;

    if (!_PyArg_CheckPositional("span", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    group = args[0];
skip_optional:
    return_value = _sre_SRE_Match_span_impl((MatchObject *)self, group);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Match___copy____doc__,
"__copy__($self, /)\n"
"--\n"
"\n");

#define _SRE_SRE_MATCH___COPY___METHODDEF    \
    {"__copy__", (PyCFunction)_sre_SRE_Match___copy__, METH_NOARGS, _sre_SRE_Match___copy____doc__},

static PyObject *
_sre_SRE_Match___copy___impl(MatchObject *self);

static PyObject *
_sre_SRE_Match___copy__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _sre_SRE_Match___copy___impl((MatchObject *)self);
}

PyDoc_STRVAR(_sre_SRE_Match___deepcopy____doc__,
"__deepcopy__($self, memo, /)\n"
"--\n"
"\n");

#define _SRE_SRE_MATCH___DEEPCOPY___METHODDEF    \
    {"__deepcopy__", (PyCFunction)_sre_SRE_Match___deepcopy__, METH_O, _sre_SRE_Match___deepcopy____doc__},

static PyObject *
_sre_SRE_Match___deepcopy___impl(MatchObject *self, PyObject *memo);

static PyObject *
_sre_SRE_Match___deepcopy__(PyObject *self, PyObject *memo)
{
    PyObject *return_value = NULL;

    return_value = _sre_SRE_Match___deepcopy___impl((MatchObject *)self, memo);

    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Scanner_match__doc__,
"match($self, /)\n"
"--\n"
"\n");

#define _SRE_SRE_SCANNER_MATCH_METHODDEF    \
    {"match", _PyCFunction_CAST(_sre_SRE_Scanner_match), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _sre_SRE_Scanner_match__doc__},

static PyObject *
_sre_SRE_Scanner_match_impl(ScannerObject *self, PyTypeObject *cls);

static PyObject *
_sre_SRE_Scanner_match(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "match() takes no arguments");
        return NULL;
    }
    return _sre_SRE_Scanner_match_impl((ScannerObject *)self, cls);
}

PyDoc_STRVAR(_sre_SRE_Scanner_search__doc__,
"search($self, /)\n"
"--\n"
"\n");

#define _SRE_SRE_SCANNER_SEARCH_METHODDEF    \
    {"search", _PyCFunction_CAST(_sre_SRE_Scanner_search), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _sre_SRE_Scanner_search__doc__},

static PyObject *
_sre_SRE_Scanner_search_impl(ScannerObject *self, PyTypeObject *cls);

static PyObject *
_sre_SRE_Scanner_search(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "search() takes no arguments");
        return NULL;
    }
    return _sre_SRE_Scanner_search_impl((ScannerObject *)self, cls);
}

#ifndef _SRE_SRE_PATTERN__FAIL_AFTER_METHODDEF
    #define _SRE_SRE_PATTERN__FAIL_AFTER_METHODDEF
#endif /* !defined(_SRE_SRE_PATTERN__FAIL_AFTER_METHODDEF) */
/*[clinic end generated code: output=bbf42e1de3bdd3ae input=a9049054013a1b77]*/
