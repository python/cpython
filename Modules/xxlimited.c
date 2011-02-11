
/* Use this file as a template to start implementing a module that
   also declares object types. All occurrences of 'Xxo' should be changed
   to something reasonable for your objects. After that, all other
   occurrences of 'xx' should be changed to something reasonable for your
   module. If your module is named foo your sourcefile should be named
   foomodule.c.

   You will probably want to delete all references to 'x_attr' and add
   your own types of attributes instead.  Maybe you want to name your
   local variables other than 'self'.  If your object type is needed in
   other files, you'll have to create a file "foobarobject.h"; see
   floatobject.h for an example. */

/* Xxo objects */

#include "Python.h"

static PyObject *ErrorObject;

typedef struct {
    PyObject_HEAD
    PyObject            *x_attr;        /* Attributes dictionary */
} XxoObject;

static PyObject *Xxo_Type;

#define XxoObject_Check(v)      (Py_TYPE(v) == Xxo_Type)

static XxoObject *
newXxoObject(PyObject *arg)
{
    XxoObject *self;
    self = PyObject_New(XxoObject, (PyTypeObject*)Xxo_Type);
    if (self == NULL)
        return NULL;
    self->x_attr = NULL;
    return self;
}

/* Xxo methods */

static void
Xxo_dealloc(XxoObject *self)
{
    Py_XDECREF(self->x_attr);
    PyObject_Del(self);
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
        PyObject *v = PyDict_GetItem(self->x_attr, name);
        if (v != NULL) {
            Py_INCREF(v);
            return v;
        }
    }
    return PyObject_GenericGetAttr((PyObject *)self, name);
}

static int
Xxo_setattr(XxoObject *self, char *name, PyObject *v)
{
    if (self->x_attr == NULL) {
        self->x_attr = PyDict_New();
        if (self->x_attr == NULL)
            return -1;
    }
    if (v == NULL) {
        int rv = PyDict_DelItemString(self->x_attr, name);
        if (rv < 0)
            PyErr_SetString(PyExc_AttributeError,
                "delete non-existing Xxo attribute");
        return rv;
    }
    else
        return PyDict_SetItemString(self->x_attr, name, v);
}

static PyType_Slot Xxo_Type_slots[] = {
    {Py_tp_doc, "The Xxo type"},
    {Py_tp_dealloc, Xxo_dealloc},
    {Py_tp_getattro, Xxo_getattro},
    {Py_tp_setattr, Xxo_setattr},
    {Py_tp_methods, Xxo_methods},
    {0, 0},
};

static PyType_Spec Xxo_Type_spec = {
    "xxlimited.Xxo",
    sizeof(XxoObject),
    0,
    Py_TPFLAGS_DEFAULT,
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
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
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
"This is a template module just for instruction.");

/* Initialization function for the module (*must* be called PyInit_xx) */


static struct PyModuleDef xxmodule = {
    PyModuleDef_HEAD_INIT,
    "xxlimited",
    module_doc,
    -1,
    xx_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit_xxlimited(void)
{
    PyObject *m = NULL;
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

    /* Create the module and add the functions */
    m = PyModule_Create(&xxmodule);
    if (m == NULL)
        goto fail;

    /* Add some symbolic constants to the module */
    if (ErrorObject == NULL) {
        ErrorObject = PyErr_NewException("xxlimited.error", NULL, NULL);
        if (ErrorObject == NULL)
            goto fail;
    }
    Py_INCREF(ErrorObject);
    PyModule_AddObject(m, "error", ErrorObject);

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
    return m;
 fail:
    Py_XDECREF(m);
    return NULL;
}
