/* Use this file as a template to start implementing a module that
   also declares object types. All occurrences of 'Xxo' should be changed
   to something reasonable for your objects. After that, all other
   occurrences of 'xx' should be changed to something reasonable for your
   module. If your module is named foo your source file should be named
   foo.c or foomodule.c.

   You will probably want to delete all references to 'x_attr' and add
   your own types of attributes instead.  Maybe you want to name your
   local variables other than 'self'.  If your object type is needed in
   other files, you'll have to create a file "foobarobject.h"; see
   floatobject.h for an example.

   This module roughly corresponds to::

      class Xxo:
         """A class that explicitly stores attributes in an internal dict"""

          def __init__(self):
              # In the C class, "_x_attr" is not accessible from Python code
              self._x_attr = {}
              self._x_exports = 0

          def __getattr__(self, name):
              return self._x_attr[name]

          def __setattr__(self, name, value):
              self._x_attr[name] = value

          def __delattr__(self, name):
              del self._x_attr[name]

          @property
          def x_exports(self):
              """Return the number of times an internal buffer is exported."""
              # Each Xxo instance has a 10-byte buffer that can be
              # accessed via the buffer interface (e.g. `memoryview`).
              return self._x_exports

          def demo(o, /):
              if isinstance(o, str):
                  return o
              elif isinstance(o, Xxo):
                  return o
              else:
                  raise Error('argument must be str or Xxo')

      class Error(Exception):
          """Exception raised by the xxlimited module"""

      def foo(i: int, j: int, /):
          """Return the sum of i and j."""
          # Unlike this pseudocode, the C function will *only* work with
          # integers and perform C long int arithmetic
          return i + j

      def new():
          return Xxo()

      def Str(str):
          # A trivial subclass of a built-in type
          pass
   */

// Need limited C API version 3.13 for Py_mod_gil
#include "pyconfig.h"   // Py_GIL_DISABLED
#ifndef Py_GIL_DISABLED
#  define Py_LIMITED_API 0x030d0000
#endif

#include "Python.h"
#include <string.h>

#define BUFSIZE 10

// Module state
typedef struct {
    PyObject *Xxo_Type;    // Xxo class
    PyObject *Error_Type;       // Error class
} xx_state;


/* Xxo objects */

// Instance state
typedef struct {
    PyObject_HEAD
    PyObject            *x_attr;           /* Attributes dictionary */
    char                x_buffer[BUFSIZE]; /* buffer for Py_buffer */
    Py_ssize_t          x_exports;         /* how many buffer are exported */
} XxoObject;

// XXX: no good way to do this yet
// #define XxoObject_Check(v)      Py_IS_TYPE(v, Xxo_Type)

static XxoObject *
newXxoObject(PyObject *module)
{
    xx_state *state = PyModule_GetState(module);
    if (state == NULL) {
        return NULL;
    }
    XxoObject *self;
    self = PyObject_GC_New(XxoObject, (PyTypeObject*)state->Xxo_Type);
    if (self == NULL) {
        return NULL;
    }
    self->x_attr = NULL;
    memset(self->x_buffer, 0, BUFSIZE);
    self->x_exports = 0;
    return self;
}

/* Xxo finalization */

static int
Xxo_traverse(PyObject *self_obj, visitproc visit, void *arg)
{
    // Visit the type
    Py_VISIT(Py_TYPE(self_obj));

    // Visit the attribute dict
    XxoObject *self = (XxoObject *)self_obj;
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
Xxo_finalize(PyObject *self_obj)
{
    XxoObject *self = (XxoObject *)self_obj;
    Py_CLEAR(self->x_attr);
}

static void
Xxo_dealloc(PyObject *self)
{
    PyObject_GC_UnTrack(self);
    Xxo_finalize(self);
    PyTypeObject *tp = Py_TYPE(self);
    freefunc free = PyType_GetSlot(tp, Py_tp_free);
    free(self);
    Py_DECREF(tp);
}


/* Xxo attribute handling */

static PyObject *
Xxo_getattro(XxoObject *self, PyObject *name)
{
    if (self->x_attr != NULL) {
        PyObject *v = PyDict_GetItemWithError(self->x_attr, name);
        if (v != NULL) {
            return Py_NewRef(v);
        }
        else if (PyErr_Occurred()) {
            return NULL;
        }
    }
    return PyObject_GenericGetAttr((PyObject *)self, name);
}

static int
Xxo_setattro(XxoObject *self, PyObject *name, PyObject *v)
{
    if (self->x_attr == NULL) {
        // prepare the attribute dict
        self->x_attr = PyDict_New();
        if (self->x_attr == NULL) {
            return -1;
        }
    }
    if (v == NULL) {
        // delete an attribute
        int rv = PyDict_DelItem(self->x_attr, name);
        if (rv < 0 && PyErr_ExceptionMatches(PyExc_KeyError)) {
            PyErr_SetString(PyExc_AttributeError,
                "delete non-existing Xxo attribute");
            return -1;
        }
        return rv;
    }
    else {
        // set an attribute
        return PyDict_SetItem(self->x_attr, name, v);
    }
}

/* Xxo methods */

static PyObject *
Xxo_demo(XxoObject *self, PyTypeObject *defining_class,
         PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (kwnames != NULL && PyObject_Length(kwnames)) {
        PyErr_SetString(PyExc_TypeError, "demo() takes no keyword arguments");
        return NULL;
    }
    if (nargs != 1) {
        PyErr_SetString(PyExc_TypeError, "demo() takes exactly 1 argument");
        return NULL;
    }

    PyObject *o = args[0];

    /* Test if the argument is "str" */
    if (PyUnicode_Check(o)) {
        return Py_NewRef(o);
    }

    /* test if the argument is of the Xxo class */
    if (PyObject_TypeCheck(o, defining_class)) {
        return Py_NewRef(o);
    }

    return Py_NewRef(Py_None);
}

static PyMethodDef Xxo_methods[] = {
    {"demo",            _PyCFunction_CAST(Xxo_demo),
     METH_METHOD | METH_FASTCALL | METH_KEYWORDS, PyDoc_STR("demo(o) -> o")},
    {NULL,              NULL}           /* sentinel */
};

/* Xxo buffer interface */

static int
Xxo_getbuffer(XxoObject *self, Py_buffer *view, int flags)
{
    int res = PyBuffer_FillInfo(view, (PyObject*)self,
                               (void *)self->x_buffer, BUFSIZE,
                               0, flags);
    if (res == 0) {
        self->x_exports++;
    }
    return res;
}

static void
Xxo_releasebuffer(XxoObject *self, Py_buffer *view)
{
    self->x_exports--;
}

static PyObject *
Xxo_get_x_exports(XxoObject *self, void *c)
{
    return PyLong_FromSsize_t(self->x_exports);
}

/* Xxo type definition */

PyDoc_STRVAR(Xxo_doc,
             "A class that explicitly stores attributes in an internal dict");

static PyGetSetDef Xxo_getsetlist[] = {
    {"x_exports", (getter) Xxo_get_x_exports, NULL, NULL},
    {NULL},
};


static PyType_Slot Xxo_Type_slots[] = {
    {Py_tp_doc, (char *)Xxo_doc},
    {Py_tp_traverse, Xxo_traverse},
    {Py_tp_clear, Xxo_clear},
    {Py_tp_finalize, Xxo_finalize},
    {Py_tp_dealloc, Xxo_dealloc},
    {Py_tp_getattro, Xxo_getattro},
    {Py_tp_setattro, Xxo_setattro},
    {Py_tp_methods, Xxo_methods},
    {Py_bf_getbuffer, Xxo_getbuffer},
    {Py_bf_releasebuffer, Xxo_releasebuffer},
    {Py_tp_getset, Xxo_getsetlist},
    {0, 0},  /* sentinel */
};

static PyType_Spec Xxo_Type_spec = {
    .name = "xxlimited.Xxo",
    .basicsize = sizeof(XxoObject),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .slots = Xxo_Type_slots,
};


/* Str type definition*/

static PyType_Slot Str_Type_slots[] = {
    {0, 0},  /* sentinel */
};

static PyType_Spec Str_Type_spec = {
    .name = "xxlimited.Str",
    .basicsize = 0,
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .slots = Str_Type_slots,
};


/* Function of two integers returning integer (with C "long int" arithmetic) */

PyDoc_STRVAR(xx_foo_doc,
"foo(i,j)\n\
\n\
Return the sum of i and j.");

static PyObject *
xx_foo(PyObject *module, PyObject *args)
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
xx_new(PyObject *module, PyObject *Py_UNUSED(unused))
{
    XxoObject *rv;

    rv = newXxoObject(module);
    if (rv == NULL)
        return NULL;
    return (PyObject *)rv;
}



/* List of functions defined in the module */

static PyMethodDef xx_methods[] = {
    {"foo",             xx_foo,         METH_VARARGS,
        xx_foo_doc},
    {"new",             xx_new,         METH_NOARGS,
        PyDoc_STR("new() -> new Xx object")},
    {NULL,              NULL}           /* sentinel */
};


/* The module itself */

PyDoc_STRVAR(module_doc,
"This is a template module just for instruction.");

static int
xx_modexec(PyObject *m)
{
    xx_state *state = PyModule_GetState(m);

    state->Error_Type = PyErr_NewException("xxlimited.Error", NULL, NULL);
    if (state->Error_Type == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, (PyTypeObject*)state->Error_Type) < 0) {
        return -1;
    }

    state->Xxo_Type = PyType_FromModuleAndSpec(m, &Xxo_Type_spec, NULL);
    if (state->Xxo_Type == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, (PyTypeObject*)state->Xxo_Type) < 0) {
        return -1;
    }

    // Add the Str type. It is not needed from C code, so it is only
    // added to the module dict.
    // It does not inherit from "object" (PyObject_Type), but from "str"
    // (PyUnincode_Type).
    PyObject *Str_Type = PyType_FromModuleAndSpec(
        m, &Str_Type_spec, (PyObject *)&PyUnicode_Type);
    if (Str_Type == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, (PyTypeObject*)Str_Type) < 0) {
        return -1;
    }
    Py_DECREF(Str_Type);

    return 0;
}

static PyModuleDef_Slot xx_slots[] = {
    {Py_mod_exec, xx_modexec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static int
xx_traverse(PyObject *module, visitproc visit, void *arg)
{
    xx_state *state = PyModule_GetState(module);
    Py_VISIT(state->Xxo_Type);
    Py_VISIT(state->Error_Type);
    return 0;
}

static int
xx_clear(PyObject *module)
{
    xx_state *state = PyModule_GetState(module);
    Py_CLEAR(state->Xxo_Type);
    Py_CLEAR(state->Error_Type);
    return 0;
}

static struct PyModuleDef xxmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "xxlimited",
    .m_doc = module_doc,
    .m_size = sizeof(xx_state),
    .m_methods = xx_methods,
    .m_slots = xx_slots,
    .m_traverse = xx_traverse,
    .m_clear = xx_clear,
    /* m_free is not necessary here: xx_clear clears all references,
     * and the module state is deallocated along with the module.
     */
};


/* Export function for the module (*must* be called PyInit_xx) */

PyMODINIT_FUNC
PyInit_xxlimited(void)
{
    return PyModuleDef_Init(&xxmodule);
}
