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

   This module uses Limited API 3.15.
   See ``xxlimited_3_13.c`` if you want to support older CPython versions.

   This module roughly corresponds to the following.
   (All underscore-prefixed attributes are not accessible from Python.)

   ::

      class Xxo:
         """A class that explicitly stores attributes in an internal dict
         (to simulate custom attribute handling).
         """

          def __init__(self):
              # In the C class, "_x_attr" is not accessible from Python code
              self._x_attr = {}
              self._x_exports = 0

          def __getattr__(self, name):
              return self._x_attr[name]

          def __setattr__(self, name, value):
              if name == "reserved":
                  raise AttributeError("cannot set 'reserved'")
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

// Target both flavors of the Stable ABI.
// Both are set to version 3.15, which adds PyModExport
// (When using a build tool, check if it has an option to set these
//  so they do not need to be defined in the source.)
#define Py_LIMITED_API 0x030f0000   // abi3 (GIL-enabled builds)
#define Py_TARGET_ABI3T 0x030f0000  // abi3t (free-threaded builds)


#include "Python.h"
#include <string.h>

#define BUFSIZE 10

// Module state
typedef struct {
    PyTypeObject *Xxo_Type;    // Xxo class
    PyObject *Error_Type;       // Error class
} xx_state;


/* Xxo objects.
 *
 * A non-trivial extension type, intentionally showing a number of features
 * that aren't easy to implement in the Limited API.
 */

// Forward declaration
static PyType_Spec Xxo_Type_spec;

// Get the module state (xx_state*) from a given type object 'type', which
// must be a subclass of Xxo (the type we're defining).
// This is complicated by the fact that the Xxo type is dynamically allocated,
// and there may be several such types in a given Python process -- for
// example, in different subinterpreters, or through loading this
// extension module several times.
// So, we don't have a "global" pointer to the type, or to the module, etc.;
// instead we search based on `Xxo_Type_spec` (which is static, immutable,
// and process-global).
//
// When possible, it's better to avoid `PyType_GetBaseByToken` -- for an
// example, see the `demo` method (Xxo_demo C function), which uses a
// "defining class". But, in many cases it's the best solution.
static xx_state *
Xxo_state_from_type(PyTypeObject *type)
{
    PyTypeObject *base;
    // Search all superclasses of 'type' for one that was defined using
    // "Xxo_Type_spec". That must be our 'Xxo' class.
    if (PyType_GetBaseByToken(type, &Xxo_Type_spec, &base) < 0) {
        return NULL;
    }
    if (base == NULL) {
        PyErr_SetString(PyExc_TypeError, "need Xxo subclass");
        return NULL;
    }
    // From this type, get the associated module. That must be the
    // relevant `xxlimited` module.
    xx_state *state = PyType_GetModuleState(base);
    Py_DECREF(base);
    return state;
}

// Structure for data needed by the XxoObject type.
// Since the object may be shared across threads, access to the fields
// usually needs to be synchronized (using Py_BEGIN_CRITICAL_SECTION).
typedef struct {
    PyObject   *x_attr;           /* Attributes dictionary.
                                   * May be NULL, which acts as an
                                   * empty dict.
                                   */
    Py_ssize_t x_exports;         /* how many buffers are exported */
    char       x_buffer[BUFSIZE]; /* buffer for Py_buffer (for simplicity,
                                   * this is constant, so does not need
                                   * synchronization)
                                   */
} XxoObject_Data;

// Get the `XxoObject_Data` structure for a given instance of our type.
static XxoObject_Data *
Xxo_get_data(PyObject *self)
{
    xx_state *state = Xxo_state_from_type(Py_TYPE(self));
    if (!state) {
        return NULL;
    }
    XxoObject_Data *data = PyObject_GetTypeData(self, state->Xxo_Type);
    return data;
}

// A variant of Xxo_get_data to be used in the tp_traverse handler.
// This function cannot have side effects (including reference count
// manipulation, creating objects, and raising exceptions), and must not
// call API functions that might have side effects.
// See: https://docs.python.org/3.15/c-api/gcsupport.html#traversal
static XxoObject_Data *
Xxo_get_data_DuringGC(PyObject *self)
{
    PyTypeObject *base;
    PyType_GetBaseByToken_DuringGC(Py_TYPE(self), &Xxo_Type_spec, &base);
    if (base == NULL) {
        return NULL;
    }
    xx_state *state = PyType_GetModuleState_DuringGC(base);
    if (state == NULL) {
        return NULL;
    }
    XxoObject_Data *data = PyObject_GetTypeData_DuringGC(self, state->Xxo_Type);
    return data;
}

// Xxo initialization
// This is the implementation of Xxo.__new__
static PyObject *
Xxo_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    // Validate that we did not get any arguments.
    if ((args != NULL && PyObject_Length(args))
        || (kwargs != NULL && PyObject_Length(kwargs)))
    {
        PyErr_SetString(PyExc_TypeError, "Xxo.__new__() takes no arguments");
        return NULL;
    }
    // Create an instance of *type* (which may be a subclass)
    allocfunc alloc = PyType_GetSlot(type, Py_tp_alloc);
    PyObject *self = alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }

    // Initialize the C members on the instance.
    // This is only included for the sake of example. The default alloc
    // function zeroes instance memory; we don't need to do it again.
    // Note that we during initialization (and finalization), we hold the only
    // reference to the object, so we don't need to synchronize with
    // other threads.
    XxoObject_Data *xxo_data = Xxo_get_data(self);
    if (xxo_data == NULL) {
        Py_DECREF(self);
        return NULL;
    }

    xxo_data->x_attr = NULL;
    memset(xxo_data->x_buffer, 0, BUFSIZE);
    xxo_data->x_exports = 0;
    return self;
}

/* Xxo finalization.
 *
 * Types that store references to other PyObjects generally need to implement
 * the GC slots: traverse, clear, dealloc, and (optionally) finalize.
 */

// traverse: Visit all references from an object, including its type
static int
Xxo_traverse(PyObject *self, visitproc visit, void *arg)
{
    // Visit the type
    Py_VISIT(Py_TYPE(self));

    // Visit the attribute dict
    XxoObject_Data *data = Xxo_get_data_DuringGC(self);
    if (data == NULL) {
        return 0;
    }
    Py_VISIT(data->x_attr);
    return 0;
}

// clear: drop references in order to break all reference cycles
static int
Xxo_clear(PyObject *self)
{
    XxoObject_Data *data = Xxo_get_data(self);
    if (data == NULL) {
        return 0;
    }
    Py_CLEAR(data->x_attr);
    return 0;
}

// finalize: like clear, but should leave the object in a consistent state.
// Equivalent to `__del__` in Python.
static void
Xxo_finalize(PyObject *self)
{
    XxoObject_Data *data = Xxo_get_data(self);
    if (data == NULL) {
        return;
    }
    Py_CLEAR(data->x_attr);
}

// dealloc: drop all remaining references and free memory
static void
Xxo_dealloc(PyObject *self)
{
    // This function must preserve currently raised exception, if any.
    PyObject *exc = PyErr_GetRaisedException();

    PyObject_GC_UnTrack(self);
    Xxo_finalize(self);

    PyTypeObject *tp = Py_TYPE(self);
    freefunc free = PyType_GetSlot(tp, Py_tp_free);
    free(self);
    Py_DECREF(tp);

    if (PyErr_Occurred()) {
        PyErr_WriteUnraisable(NULL);
    }
    PyErr_SetRaisedException(exc);
}


/* Xxo attribute handling */

// Get an attribute.
static PyObject *
Xxo_getattro(PyObject *self, PyObject *name)
{
    XxoObject_Data *data = Xxo_get_data(self);
    if (data == NULL) {
        return 0;
    }

    PyObject *x_attr;
    Py_BEGIN_CRITICAL_SECTION(self);
    x_attr = data->x_attr;
    Py_END_CRITICAL_SECTION();

    if (x_attr != NULL) {
        PyObject *v = PyDict_GetItemWithError(x_attr, name);
        if (v != NULL) {
            return Py_NewRef(v);
        }
        else if (PyErr_Occurred()) {
            return NULL;
        }
    }
    // Fall back to generic implementation (this handles special attributes,
    // raising AttributeError, etc.)
    return PyObject_GenericGetAttr(self, name);
}

// Set or delete an attribute.
static int
Xxo_setattro(PyObject *self, PyObject *name, PyObject *v)
{
    // filter a specific attribute name
    if (PyUnicode_Check(name) && PyUnicode_EqualToUTF8(name, "reserved")) {
        PyErr_Format(PyExc_AttributeError, "cannot set %R", name);
        return -1;
    }

    XxoObject_Data *data = Xxo_get_data(self);
    if (data == NULL) {
        return -1;
    }

    // If the attribute dict is not created yet, make one.
    // This needs to be protected by a critical section to avoid another thread
    // creating a duplicate dict.
    PyObject *x_attr;
    Py_BEGIN_CRITICAL_SECTION(self);
    x_attr = data->x_attr;
    if (x_attr == NULL) {
        // prepare the attribute dict
        data->x_attr = x_attr = PyDict_New();
    }
    Py_END_CRITICAL_SECTION();
    if (x_attr == NULL) {
        return -1;
    }

    if (v == NULL) {
        // delete an attribute
        int rv = PyDict_DelItem(x_attr, name);
        if (rv < 0 && PyErr_ExceptionMatches(PyExc_KeyError)) {
            PyErr_SetString(PyExc_AttributeError,
                "delete non-existing Xxo attribute");
            return -1;
        }
        return rv;
    }
    else {
        // set an attribute
        return PyDict_SetItem(x_attr, name, v);
    }
}

/* Xxo methods: C functions plus a PyMethodDef array that lists them and
 * specifies metadata.
 */

static PyObject *
Xxo_demo(PyObject *self, PyTypeObject *defining_class,
         PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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

/* Xxo buffer interface: C functions later referenced from PyType_Slot array.
 * Other interfaces (e.g. for sequence-like or number-like types) are defined
 * similarly.
 */

static int
Xxo_getbuffer(PyObject *self, Py_buffer *view, int flags)
{
    XxoObject_Data *data = Xxo_get_data(self);
    if (data == NULL) {
        return -1;
    }
    int res = PyBuffer_FillInfo(view, self,
                               (void *)data->x_buffer, BUFSIZE,
                               0, flags);
    if (res == 0) {
        Py_BEGIN_CRITICAL_SECTION(self);
        data->x_exports++;
        Py_END_CRITICAL_SECTION();
    }
    return res;
}

static void
Xxo_releasebuffer(PyObject *self, Py_buffer *Py_UNUSED(view))
{
    XxoObject_Data *data = Xxo_get_data(self);
    if (data == NULL) {
        return;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    data->x_exports--;
    Py_END_CRITICAL_SECTION();
}

static PyObject *
Xxo_get_x_exports(PyObject *self, void *Py_UNUSED(closure))
{
    XxoObject_Data *data = Xxo_get_data(self);
    if (data == NULL) {
        return NULL;
    }
    Py_ssize_t result;

    Py_BEGIN_CRITICAL_SECTION(self);
    result = data->x_exports;
    Py_END_CRITICAL_SECTION();

    return PyLong_FromSsize_t(result);
}

/* Xxo type definition */

PyDoc_STRVAR(Xxo_doc,
             "A class that explicitly stores attributes in an internal dict");

static PyGetSetDef Xxo_getsetlist[] = {
    {"x_exports", Xxo_get_x_exports, NULL, NULL},
    {NULL},
};


static PyType_Slot Xxo_Type_slots[] = {
    {Py_tp_doc, (char *)Xxo_doc},
    {Py_tp_new, Xxo_new},
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
    {Py_tp_token, Py_TP_USE_SPEC},
    {0, 0},  /* sentinel */
};

static PyType_Spec Xxo_Type_spec = {
    .name = "xxlimited.Xxo",
    .basicsize = -(Py_ssize_t)sizeof(XxoObject_Data),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE,
    .slots = Xxo_Type_slots,
};


/* Str type definition*/

static PyType_Slot Str_Type_slots[] = {
    // slots array intentionally kept empty
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


/* Function of no arguments returning new Xxo object.
 * Note that a function exposed to Python with METH_NOARGS requires an unused
 * second argument, so we cannot use newXxoObject directly.
 */

static PyObject *
xx_new(PyObject *module, PyObject *Py_UNUSED(unused))
{
    xx_state *state = PyModule_GetState(module);

    return Xxo_new(state->Xxo_Type, NULL, NULL);
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

    state->Xxo_Type = (PyTypeObject*)PyType_FromModuleAndSpec(
        m, &Xxo_Type_spec, NULL);
    if (state->Xxo_Type == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, state->Xxo_Type) < 0) {
        return -1;
    }

    // Add the Str type. It is not needed from C code, so it is only
    // added to the module dict.
    // It does not inherit from "object" (PyObject_Type), but from "str"
    // (PyUnincode_Type).
    PyTypeObject *Str_Type = (PyTypeObject*)PyType_FromModuleAndSpec(
        m, &Str_Type_spec, (PyObject *)&PyUnicode_Type);
    if (Str_Type == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, Str_Type) < 0) {
        Py_DECREF(Str_Type);
        return -1;
    }
    Py_DECREF(Str_Type);

    return 0;
}

// Module finalization: modules that hold references in their module state
// need to implement the fullowing GC hooks. They're similar to the ones for
// types (see "Xxo finalization").

static int
xx_traverse(PyObject *module, visitproc visit, void *arg)
{
    xx_state *state = PyModule_GetState_DuringGC(module);
    if (state == NULL) {
        return 0;
    }
    Py_VISIT(state->Xxo_Type);
    Py_VISIT(state->Error_Type);
    return 0;
}

static int
xx_clear(PyObject *module)
{
    xx_state *state = PyModule_GetState(module);
    if (state == NULL) {
        return 0;
    }
    Py_CLEAR(state->Xxo_Type);
    Py_CLEAR(state->Error_Type);
    return 0;
}

static void
xx_free(void *module)
{
    // allow xx_modexec to omit calling xx_clear on error
    (void)xx_clear((PyObject *)module);

    xx_state *state = PyModule_GetState(module);
    if (state == NULL) {
        return;
    }
}

// Information that CPython uses to prevent loading incompatible extenstions
PyABIInfo_VAR(abi_info);

static PySlot xx_slots[] = {
    /* Basic metadata */
    PySlot_STATIC_DATA(Py_mod_name, "xxlimited"),
    PySlot_STATIC_DATA(Py_mod_doc, (void*)module_doc),
    PySlot_DATA(Py_mod_abi, &abi_info),

    /* The method table */
    PySlot_STATIC_DATA(Py_mod_methods, xx_methods),

    /* exec function to initialize the module (called as part of import
     * after the object was added to sys.modules)
     */
    PySlot_FUNC(Py_mod_exec, xx_modexec),

    /* Module state and associated functions */
    PySlot_SIZE(Py_mod_state_size, sizeof(xx_state)),
    PySlot_FUNC(Py_mod_state_traverse, xx_traverse),
    PySlot_FUNC(Py_mod_state_clear, xx_clear),
    PySlot_FUNC(Py_mod_state_free, xx_free),

    /* Signal that this module supports being loaded in multiple interpreters
     * with separate GILs (global interpreter locks).
     * See "Isolating Extension Modules" on how to prepare a module for this:
     *   https://docs.python.org/3/howto/isolating-extensions.html
     */
    PySlot_DATA(Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED),

    /* Signal that this module does not rely on the GIL for its own needs.
     * Without this slot, free-threaded builds of CPython will enable
     * the GIL when this module is loaded.
     */
    PySlot_DATA(Py_mod_gil, Py_MOD_GIL_NOT_USED),

    PySlot_END
};


/* Export function for the module. *Must* be called PyModExport_xx; usually
 * it is the only non-`static` object in a module definition.
 */

PyMODEXPORT_FUNC
PyModExport_xxlimited(void)
{
    return xx_slots;
}
