/* typing accelerator C extension: _typing module. */

#include "Python.h"
#include "clinic/_typingmodule.c.h"

/*[clinic input]
module _typing

[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=1db35baf1c72942b]*/

/* helper function to make typing.NewType.__call__ method faster */

/*[clinic input]
_typing._idfunc -> object

    x: object
    /

[clinic start generated code]*/

static PyObject *
_typing__idfunc(PyObject *module, PyObject *x)
/*[clinic end generated code: output=63c38be4a6ec5f2c input=49f17284b43de451]*/
{
    Py_INCREF(x);
    return x;
}

/*[clinic input]
_typing.cast -> object

    typ: object
    val: object

Cast a value to a type.

This returns the value unchanged.  To the type checker this
signals that the return value has the designated type, but at
runtime we intentionally don't check anything (we want this
to be as fast as possible).
[clinic start generated code]*/

static PyObject *
_typing_cast_impl(PyObject *module, PyObject *typ, PyObject *val)
/*[clinic end generated code: output=11224a3fa037a9a1 input=b8f6ce3bf6198a5c]*/
{
    Py_INCREF(val);
    return val;
}


static PyMethodDef typing_methods[] = {
    _TYPING__IDFUNC_METHODDEF
    _TYPING_CAST_METHODDEF
    {NULL, NULL, 0, NULL}
};

PyDoc_STRVAR(typing_doc,
"Accelerators for the typing module.\n");

static struct PyModuleDef_Slot _typingmodule_slots[] = {
    {0, NULL}
};

static struct PyModuleDef typingmodule = {
        PyModuleDef_HEAD_INIT,
        "_typing",
        typing_doc,
        0,
        typing_methods,
        _typingmodule_slots,
        NULL,
        NULL,
        NULL
};

PyMODINIT_FUNC
PyInit__typing(void)
{
    return PyModuleDef_Init(&typingmodule);
}
