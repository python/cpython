/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_suggestions__generate_suggestions__doc__,
"_generate_suggestions($module, candidates, item, /)\n"
"--\n"
"\n"
"Returns the candidate in candidates that\'s closest to item");

#define _SUGGESTIONS__GENERATE_SUGGESTIONS_METHODDEF    \
    {"_generate_suggestions", _PyCFunction_CAST(_suggestions__generate_suggestions), METH_FASTCALL, _suggestions__generate_suggestions__doc__},

static PyObject *
_suggestions__generate_suggestions_impl(PyObject *module,
                                        PyObject *candidates, PyObject *item);

static PyObject *
_suggestions__generate_suggestions(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *candidates;
    PyObject *item;

    if (!_PyArg_CheckPositional("_generate_suggestions", nargs, 2, 2)) {
        goto exit;
    }
    candidates = args[0];
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("_generate_suggestions", "argument 2", "str", args[1]);
        goto exit;
    }
    item = args[1];
    Py_BEGIN_CRITICAL_SECTION(candidates);
    return_value = _suggestions__generate_suggestions_impl(module, candidates, item);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}
/*[clinic end generated code: output=1690dd15a464d19c input=a9049054013a1b77]*/
