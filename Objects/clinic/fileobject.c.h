/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_BadArgument()

PyDoc_STRVAR(stdprinter_write__doc__,
"write($self, text, /)\n"
"--\n"
"\n"
"Write the text to the stream.");

#define STDPRINTER_WRITE_METHODDEF    \
    {"write", (PyCFunction)stdprinter_write, METH_O, stdprinter_write__doc__},

static PyObject *
stdprinter_write_impl(PyStdPrinter_Object *self, PyObject *unicode);

static PyObject *
stdprinter_write(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *unicode;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("write", "argument", "str", arg);
        goto exit;
    }
    unicode = arg;
    return_value = stdprinter_write_impl((PyStdPrinter_Object *)self, unicode);

exit:
    return return_value;
}
/*[clinic end generated code: output=7cf7d4c181518bbb input=a9049054013a1b77]*/
