
/* This module is compiled using limited API from Python 3.5,
 * making sure that it works as expected.
 *
 * See the xxlimited module for an extension module template.
 */

#define Py_LIMITED_API 0x03050000

#include "Python.h"

/* Xxo objects */

static PyObject *ErrorObject;

typedef struct {
    PyObject_HEAD
    PyObject            *x_attr;        /* Attributes dictionary */
} XxoObject;

static PyObject *Xxo_Type;

#define XxoObject_Check(v)      Py_IS_TYPE(v, Xxo_Type)

static XxoObject *
newXxoObject(PyObject *arg)
{
    XxoObject *self;
    self = PyObject_GC_New(XxoObject, (PyTypeObject*)Xxo_Type);
    if (self == NULL)
        return NULL;
    self->x_attr = NULL;
    return self;
}

/* Xxo methods */

static int
Xxo_traverse(XxoObject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->x_attr);
    return 0;
}

static int
Xxo_clear(XxoObject *self)
{
    Py_CLEAR(self->x_attr);
    return 0;
}

static void
Xxo_finalize(XxoObject *self)
{
    Py_CLEAR(self->x_attr);
}

static PyObject *
Xxo_demo(XxoObject *self, PyObject *args)
{
    PyObject *o = NULL;
    if (!PyArg_ParseTuple(args, "|O:demo", &o))
        return NULL;
    /* Test availability of fast type checks */
    if (o != NULL && PyUnicode_Check(o)) {
        Py_INCREF(o);
        return o;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef Xxo_methods[] = {
    {"demo",            (PyCFunction)Xxo_demo,  METH_VARARGS,
        PyDoc_STR("demo() -> None")},
    {NULL,              NULL}           /* sentinel */
};

static PyObject *
Xxo_getattro(XxoObject *self, PyObject *name)
{
    if (self->x_attr != NULL) {
        PyObject *v = PyDict_GetItemWithError(self->x_attr, name);
        if (v != NULL) {
            Py_INCREF(v);
            return v;
        }
        else if (PyErr_Occurred()) {
            return NULL;
        }
    }
    return PyObject_GenericGetAttr((PyObject *)self, name);
}

static int
Xxo_setattr(XxoObject *self, const char *name, PyObject *v)
{
    if (self->x_attr == NULL) {
        self->x_attr = PyDict_New();
        if (self->x_attr == NULL)
            return -1;
    }
    if (v == NULL) {
        int rv = PyDict_DelItemString(self->x_attr, name);
        if (rv < 0 && PyErr_ExceptionMatches(PyExc_KeyError))
            PyErr_SetString(PyExc_AttributeError,
                "delete non-existing Xxo attribute");
        return rv;
    }
    else
        return PyDict_SetItemString(self->x_attr, name, v);
}

static PyType_Slot Xxo_Type_slots[] = {
    {Py_tp_doc, "The Xxo type"},
    {Py_tp_traverse, Xxo_traverse},
    {Py_tp_clear, Xxo_clear},
    {Py_tp_finalize, Xxo_finalize},
    {Py_tp_getattro, Xxo_getattro},
    {Py_tp_setattr, Xxo_setattr},
    {Py_tp_methods, Xxo_methods},
    {0, 0},
};

static PyType_Spec Xxo_Type_spec = {
    "xxlimited.Xxo",
    sizeof(XxoObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    Xxo_Type_slots
};

/* --------------------------------------------------------------------- */

/* Function of two integers returning integer */

PyDoc_STRVAR(xx_foo_doc,
"foo(i,j)\n\
\n\
Return the sum of i and j.");

static PyObject *
xx_foo(PyObject *self, PyObject *args)
{
    long i, j;
    long res;
    if (!PyArg_ParseTuple(args, "ll:foo", &i, &j))
        return NULL;
    res = i+j; /* XXX Do something here */
    return PyLong_FromLong(res);
}


/* Function of no arguments returning new Xxo object */

static PyObject *
xx_new(PyObject *self, PyObject *args)
{
    XxoObject *rv;

    if (!PyArg_ParseTuple(args, ":new"))
        return NULL;
    rv = newXxoObject(args);
    if (rv == NULL)
        return NULL;
    return (PyObject *)rv;
}

/* Test bad format character */

static PyObject *
xx_roj(PyObject *self, PyObject *args)
{
    PyObject *a;
    long b;
    if (!PyArg_ParseTuple(args, "O#:roj", &a, &b))
        return NULL;
    Py_INCREF(Py_None);
    return Py_None;
}


/* ---------- */

static PyType_Slot Str_Type_slots[] = {
    {Py_tp_base, NULL}, /* filled out in module init function */
    {0, 0},
};

static PyType_Spec Str_Type_spec = {
    "xxlimited.Str",
    0,
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    Str_Type_slots
};

/* ---------- */

static PyObject *
null_richcompare(PyObject *self, PyObject *other, int op)
{
    Py_RETURN_NOTIMPLEMENTED;
}

static PyType_Slot Null_Type_slots[] = {
    {Py_tp_base, NULL}, /* filled out in module init */
    {Py_tp_new, NULL},
    {Py_tp_richcompare, null_richcompare},
    {0, 0}
};

static PyType_Spec Null_Type_spec = {
    "xxlimited.Null",
    0,               /* basicsize */
    0,               /* itemsize */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    Null_Type_slots
};

/* ---------- */

/* List of functions defined in the module */

static PyMethodDef xx_methods[] = {
    {"roj",             xx_roj,         METH_VARARGS,
        PyDoc_STR("roj(a,b) -> None")},
    {"foo",             xx_foo,         METH_VARARGS,
        xx_foo_doc},
    {"new",             xx_new,         METH_VARARGS,
        PyDoc_STR("new() -> new Xx object")},
    {NULL,              NULL}           /* sentinel */
};

PyDoc_STRVAR(module_doc,
"This is a module for testing limited API from Python 3.5.");

static int
xx_modexec(PyObject *m)
{
    PyObject *o;

    /* Due to cross platform compiler issues the slots must be filled
     * here. It's required for portability to Windows without requiring
     * C++. */
    Null_Type_slots[0].pfunc = &PyBaseObject_Type;
    Null_Type_slots[1].pfunc = PyType_GenericNew;
    Str_Type_slots[0].pfunc = &PyUnicode_Type;

    Xxo_Type = PyType_FromSpec(&Xxo_Type_spec);
    if (Xxo_Type == NULL)
        goto fail;

    /* Add some symbolic constants to the module */
    if (ErrorObject == NULL) {
        ErrorObject = PyErr_NewException("xxlimited.error", NULL, NULL);
        if (ErrorObject == NULL)
            goto fail;
    }
    Py_INCREF(ErrorObject);
    PyModule_AddObject(m, "error", ErrorObject);

    /* Add Xxo */
    o = PyType_FromSpec(&Xxo_Type_spec);
    if (o == NULL)
        goto fail;
    PyModule_AddObject(m, "Xxo", o);

    /* Add Str */
    o = PyType_FromSpec(&Str_Type_spec);
    if (o == NULL)
        goto fail;
    PyModule_AddObject(m, "Str", o);

    /* Add Null */
    o = PyType_FromSpec(&Null_Type_spec);
    if (o == NULL)
        goto fail;
    PyModule_AddObject(m, "Null", o);
    return 0;
 fail:
    Py_XDECREF(m);
    return -1;
}


static PyModuleDef_Slot xx_slots[] = {
    {Py_mod_exec, xx_modexec},
    {0, NULL}
};

static struct PyModuleDef xxmodule = {
    PyModuleDef_HEAD_INIT,
    "xxlimited_35",
    module_doc,
    0,
    xx_methods,
    xx_slots,
    NULL,
    NULL,
    NULL
};

/* Export function for the module (*must* be called PyInit_xx) */

PyMODINIT_FUNC
PyInit_xxlimited_35(void)
{
    return PyModuleDef_Init(&xxmodule);
}
