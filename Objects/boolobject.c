/* Boolean type, a subtype of int */

#include "Python.h"

/* We need to define bool_print to override int_print */

static int
bool_print(PyBoolObject *self, FILE *fp, int flags)
{
    Py_BEGIN_ALLOW_THREADS
    fputs(self->ob_ival == 0 ? "False" : "True", fp);
    Py_END_ALLOW_THREADS
    return 0;
}

/* We define bool_repr to return "False" or "True" */

static PyObject *false_str = NULL;
static PyObject *true_str = NULL;

static PyObject *
bool_repr(PyBoolObject *self)
{
    PyObject *s;

    if (self->ob_ival)
        s = true_str ? true_str :
            (true_str = PyString_InternFromString("True"));
    else
        s = false_str ? false_str :
            (false_str = PyString_InternFromString("False"));
    Py_XINCREF(s);
    return s;
}

/* Function to return a bool from a C long */

PyObject *PyBool_FromLong(long ok)
{
    PyObject *result;

    if (ok)
        result = Py_True;
    else
        result = Py_False;
    Py_INCREF(result);
    return result;
}

/* We define bool_new to always return either Py_True or Py_False */

static PyObject *
bool_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"x", 0};
    PyObject *x = Py_False;
    long ok;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O:bool", kwlist, &x))
        return NULL;
    ok = PyObject_IsTrue(x);
    if (ok < 0)
        return NULL;
    return PyBool_FromLong(ok);
}

/* Arithmetic operations redefined to return bool if both args are bool. */

static PyObject *
bool_and(PyObject *a, PyObject *b)
{
    if (!PyBool_Check(a) || !PyBool_Check(b))
        return PyInt_Type.tp_as_number->nb_and(a, b);
    return PyBool_FromLong(
        ((PyBoolObject *)a)->ob_ival & ((PyBoolObject *)b)->ob_ival);
}

static PyObject *
bool_or(PyObject *a, PyObject *b)
{
    if (!PyBool_Check(a) || !PyBool_Check(b))
        return PyInt_Type.tp_as_number->nb_or(a, b);
    return PyBool_FromLong(
        ((PyBoolObject *)a)->ob_ival | ((PyBoolObject *)b)->ob_ival);
}

static PyObject *
bool_xor(PyObject *a, PyObject *b)
{
    if (!PyBool_Check(a) || !PyBool_Check(b))
        return PyInt_Type.tp_as_number->nb_xor(a, b);
    return PyBool_FromLong(
        ((PyBoolObject *)a)->ob_ival ^ ((PyBoolObject *)b)->ob_ival);
}

/* Doc string */

PyDoc_STRVAR(bool_doc,
"bool(x) -> bool\n\
\n\
Returns True when the argument x is true, False otherwise.\n\
The builtins True and False are the only two instances of the class bool.\n\
The class bool is a subclass of the class int, and cannot be subclassed.");

/* Arithmetic methods -- only so we can override &, |, ^. */

static PyNumberMethods bool_as_number = {
    0,                          /* nb_add */
    0,                          /* nb_subtract */
    0,                          /* nb_multiply */
    0,                          /* nb_divide */
    0,                          /* nb_remainder */
    0,                          /* nb_divmod */
    0,                          /* nb_power */
    0,                          /* nb_negative */
    0,                          /* nb_positive */
    0,                          /* nb_absolute */
    0,                          /* nb_nonzero */
    0,                          /* nb_invert */
    0,                          /* nb_lshift */
    0,                          /* nb_rshift */
    bool_and,                   /* nb_and */
    bool_xor,                   /* nb_xor */
    bool_or,                    /* nb_or */
    0,                          /* nb_coerce */
    0,                          /* nb_int */
    0,                          /* nb_long */
    0,                          /* nb_float */
    0,                          /* nb_oct */
    0,                          /* nb_hex */
    0,                          /* nb_inplace_add */
    0,                          /* nb_inplace_subtract */
    0,                          /* nb_inplace_multiply */
    0,                          /* nb_inplace_divide */
    0,                          /* nb_inplace_remainder */
    0,                          /* nb_inplace_power */
    0,                          /* nb_inplace_lshift */
    0,                          /* nb_inplace_rshift */
    0,                          /* nb_inplace_and */
    0,                          /* nb_inplace_xor */
    0,                          /* nb_inplace_or */
    0,                          /* nb_floor_divide */
    0,                          /* nb_true_divide */
    0,                          /* nb_inplace_floor_divide */
    0,                          /* nb_inplace_true_divide */
};

/* The type object for bool.  Note that this cannot be subclassed! */

PyTypeObject PyBool_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "bool",
    sizeof(PyIntObject),
    0,
    0,                                          /* tp_dealloc */
    (printfunc)bool_print,                      /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_compare */
    (reprfunc)bool_repr,                        /* tp_repr */
    &bool_as_number,                            /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    (reprfunc)bool_repr,                        /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES, /* tp_flags */
    bool_doc,                                   /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    &PyInt_Type,                                /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    bool_new,                                   /* tp_new */
};

/* The objects representing bool values False and True */

/* Named Zero for link-level compatibility */
PyIntObject _Py_ZeroStruct = {
    PyObject_HEAD_INIT(&PyBool_Type)
    0
};

PyIntObject _Py_TrueStruct = {
    PyObject_HEAD_INIT(&PyBool_Type)
    1
};
