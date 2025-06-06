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
        (PyCFunction)test_with_docstring, METH_NOARGS,
        docstring_empty},
    {"docstring_no_signature",
        (PyCFunction)test_with_docstring, METH_NOARGS,
        docstring_no_signature},
    {"docstring_with_invalid_signature",
        (PyCFunction)test_with_docstring, METH_NOARGS,
        docstring_with_invalid_signature},
    {"docstring_with_invalid_signature2",
        (PyCFunction)test_with_docstring, METH_NOARGS,
        docstring_with_invalid_signature2},
    {"docstring_with_signature",
        (PyCFunction)test_with_docstring, METH_NOARGS,
        docstring_with_signature},
    {"docstring_with_signature_and_extra_newlines",
        (PyCFunction)test_with_docstring, METH_NOARGS,
        docstring_with_signature_and_extra_newlines},
    {"docstring_with_signature_but_no_doc",
        (PyCFunction)test_with_docstring, METH_NOARGS,
        docstring_with_signature_but_no_doc},
    {"docstring_with_signature_with_defaults",
        (PyCFunction)test_with_docstring, METH_NOARGS,
        docstring_with_signature_with_defaults},
    {"no_docstring",
        (PyCFunction)test_with_docstring, METH_NOARGS},
    {"test_with_docstring",
        test_with_docstring,              METH_NOARGS,
        PyDoc_STR("This is a pretty normal docstring.")},
    {NULL},
};

int
_PyTestCapi_Init_Docstring(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }
    return 0;
}
