/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_html_escape__doc__,
"escape($module, /, s, quote=True)\n"
"--\n"
"\n"
"Replace special characters \"&\", \"<\" and \">\" to HTML-safe sequences.\n"
"\n"
"If the optional flag quote is true (the default), the quotation mark\n"
"characters, both double quote (\") and single quote (\'), are also\n"
"translated.");

#define _HTML_ESCAPE_METHODDEF    \
    {"escape", _PyCFunction_CAST(_html_escape), METH_FASTCALL|METH_KEYWORDS, _html_escape__doc__},

static PyObject *
_html_escape_impl(PyObject *module, PyObject *s, int quote);

static PyObject *
_html_escape(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _Py_LATIN1_CHR('s'), &_Py_ID(quote), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"s", "quote", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "escape",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *s;
    int quote = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("escape", "argument 's'", "str", args[0]);
        goto exit;
    }
    s = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    quote = PyObject_IsTrue(args[1]);
    if (quote < 0) {
        goto exit;
    }
skip_optional_pos:
    return_value = _html_escape_impl(module, s, quote);

exit:
    return return_value;
}

PyDoc_STRVAR(_html_unescape__doc__,
"unescape($module, s, /)\n"
"--\n"
"\n"
"Convert named and numeric character references to Unicode characters.\n"
"\n"
"This function uses the rules defined by the HTML 5 standard for both\n"
"valid and invalid character references, and the list of HTML 5 named\n"
"character references defined in html.entities.html5.");

#define _HTML_UNESCAPE_METHODDEF    \
    {"unescape", (PyCFunction)_html_unescape, METH_O, _html_unescape__doc__},

static PyObject *
_html_unescape_impl(PyObject *module, PyObject *s);

static PyObject *
_html_unescape(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *s;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("unescape", "argument", "str", arg);
        goto exit;
    }
    s = arg;
    return_value = _html_unescape_impl(module, s);

exit:
    return return_value;
}
/*[clinic end generated code: output=3173663201cb635a input=a9049054013a1b77]*/
