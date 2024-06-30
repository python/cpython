#include "parts.h"


PyDoc_STRVAR(docstring_empty,
""
);

PyDoc_STRVAR(docstring_no_signature,
"This docstring has no signature."
);

PyDoc_STRVAR(docstring_with_invalid_signature,
"docstring_with_invalid_signature($module, /, boo)\n"
"\n"
"This docstring has an invalid signature."
);

PyDoc_STRVAR(docstring_with_invalid_signature2,
"docstring_with_invalid_signature2($module, /, boo)\n"
"\n"
"--\n"
"\n"
"This docstring also has an invalid signature."
);

PyDoc_STRVAR(docstring_with_signature,
"docstring_with_signature($module, /, sig)\n"
"--\n"
"\n"
"This docstring has a valid signature."
);

PyDoc_STRVAR(docstring_with_signature_but_no_doc,
"docstring_with_signature_but_no_doc($module, /, sig)\n"
"--\n"
"\n"
);

PyDoc_STRVAR(docstring_with_signature_and_extra_newlines,
"docstring_with_signature_and_extra_newlines($module, /, parameter)\n"
"--\n"
"\n"
"\n"
"This docstring has a valid signature and some extra newlines."
);

PyDoc_STRVAR(docstring_with_signature_with_defaults,
"docstring_with_signature_with_defaults(module, s='avocado',\n"
"        b=b'bytes', d=3.14, i=35, n=None, t=True, f=False,\n"
"        local=the_number_three, sys=sys.maxsize,\n"
"        exp=sys.maxsize - 1)\n"
"--\n"
"\n"
"\n"
"\n"
"This docstring has a valid signature with parameters,\n"
"and the parameters take defaults of varying types."
);

/* This is here to provide a docstring for test_descr. */
static PyObject *
test_with_docstring(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    Py_RETURN_NONE;
}

static PyMethodDef test_methods[] = {
    {"docstring_empty",
        (PyCFunction)test_with_docstring, METH_VARARGS,
        docstring_empty},
    {"docstring_no_signature",
        (PyCFunction)test_with_docstring, METH_VARARGS,
        docstring_no_signature},
    {"docstring_no_signature_noargs",
        (PyCFunction)test_with_docstring, METH_NOARGS,
        docstring_no_signature},
    {"docstring_no_signature_o",
        (PyCFunction)test_with_docstring, METH_O,
        docstring_no_signature},
    {"docstring_with_invalid_signature",
        (PyCFunction)test_with_docstring, METH_VARARGS,
        docstring_with_invalid_signature},
    {"docstring_with_invalid_signature2",
        (PyCFunction)test_with_docstring, METH_VARARGS,
        docstring_with_invalid_signature2},
    {"docstring_with_signature",
        (PyCFunction)test_with_docstring, METH_VARARGS,
        docstring_with_signature},
    {"docstring_with_signature_and_extra_newlines",
        (PyCFunction)test_with_docstring, METH_VARARGS,
        docstring_with_signature_and_extra_newlines},
    {"docstring_with_signature_but_no_doc",
        (PyCFunction)test_with_docstring, METH_VARARGS,
        docstring_with_signature_but_no_doc},
    {"docstring_with_signature_with_defaults",
        (PyCFunction)test_with_docstring, METH_VARARGS,
        docstring_with_signature_with_defaults},
    {"no_docstring",
        (PyCFunction)test_with_docstring, METH_VARARGS},
    {"test_with_docstring",
        test_with_docstring,              METH_VARARGS,
        PyDoc_STR("This is a pretty normal docstring.")},
    {"func_with_unrepresentable_signature",
        (PyCFunction)test_with_docstring, METH_VARARGS,
        PyDoc_STR(
            "func_with_unrepresentable_signature($module, /, a, b=<x>)\n"
            "--\n\n"
            "This docstring has a signature with unrepresentable default."
        )},
    {NULL},
};

static PyMethodDef DocStringNoSignatureTest_methods[] = {
    {"meth_noargs",
        (PyCFunction)test_with_docstring, METH_NOARGS,
        docstring_no_signature},
    {"meth_o",
        (PyCFunction)test_with_docstring, METH_O,
        docstring_no_signature},
    {"meth_noargs_class",
        (PyCFunction)test_with_docstring, METH_NOARGS|METH_CLASS,
        docstring_no_signature},
    {"meth_o_class",
        (PyCFunction)test_with_docstring, METH_O|METH_CLASS,
        docstring_no_signature},
    {"meth_noargs_static",
        (PyCFunction)test_with_docstring, METH_NOARGS|METH_STATIC,
        docstring_no_signature},
    {"meth_o_static",
        (PyCFunction)test_with_docstring, METH_O|METH_STATIC,
        docstring_no_signature},
    {"meth_noargs_coexist",
        (PyCFunction)test_with_docstring, METH_NOARGS|METH_COEXIST,
        docstring_no_signature},
    {"meth_o_coexist",
        (PyCFunction)test_with_docstring, METH_O|METH_COEXIST,
        docstring_no_signature},
    {NULL},
};

static PyTypeObject DocStringNoSignatureTest = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testcapi.DocStringNoSignatureTest",
    .tp_basicsize = sizeof(PyObject),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_methods = DocStringNoSignatureTest_methods,
    .tp_new = PyType_GenericNew,
};

static PyMethodDef DocStringUnrepresentableSignatureTest_methods[] = {
    {"meth",
        (PyCFunction)test_with_docstring, METH_VARARGS,
        PyDoc_STR(
            "meth($self, /, a, b=<x>)\n"
            "--\n\n"
            "This docstring has a signature with unrepresentable default."
        )},
    {"classmeth",
        (PyCFunction)test_with_docstring, METH_VARARGS|METH_CLASS,
        PyDoc_STR(
            "classmeth($type, /, a, b=<x>)\n"
            "--\n\n"
            "This docstring has a signature with unrepresentable default."
        )},
    {"staticmeth",
        (PyCFunction)test_with_docstring, METH_VARARGS|METH_STATIC,
        PyDoc_STR(
            "staticmeth(a, b=<x>)\n"
            "--\n\n"
            "This docstring has a signature with unrepresentable default."
        )},
    {"with_default",
        (PyCFunction)test_with_docstring, METH_VARARGS,
        PyDoc_STR(
            "with_default($self, /, x=ONE)\n"
            "--\n\n"
            "This instance method has a default parameter value from the module scope."
        )},
    {NULL},
};

static PyTypeObject DocStringUnrepresentableSignatureTest = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testcapi.DocStringUnrepresentableSignatureTest",
    .tp_basicsize = sizeof(PyObject),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_methods = DocStringUnrepresentableSignatureTest_methods,
    .tp_new = PyType_GenericNew,
};

int
_PyTestCapi_Init_Docstring(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }
    if (PyModule_AddType(mod, &DocStringNoSignatureTest) < 0) {
        return -1;
    }
    if (PyModule_AddType(mod, &DocStringUnrepresentableSignatureTest) < 0) {
        return -1;
    }
    if (PyModule_AddObject(mod, "ONE", PyLong_FromLong(1)) < 0) {
        return -1;
    }
    return 0;
}
