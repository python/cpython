/* Wrap void * pointers to be passed between C modules */

#include "Python.h"

/* Internal structure of PyCapsule */
typedef struct {
    PyObject_HEAD
    void *pointer;
    const char *name;
    void *context;
    PyCapsule_Destructor destructor;
} PyCapsule;



static int
_is_legal_capsule(PyCapsule *capsule, const char *invalid_capsule)
{
    if (!capsule || !PyCapsule_CheckExact(capsule) || capsule->pointer == NULL) {
        PyErr_SetString(PyExc_ValueError, invalid_capsule);
        return 0;
    }
    return 1;
}

#define is_legal_capsule(capsule, name) \
    (_is_legal_capsule(capsule, \
     name " called with invalid PyCapsule object"))


static int
name_matches(const char *name1, const char *name2) {
    /* if either is NULL, */
    if (!name1 || !name2) {
        /* they're only the same if they're both NULL. */
        return name1 == name2;
    }
    return !strcmp(name1, name2);
}



PyObject *
PyCapsule_New(void *pointer, const char *name, PyCapsule_Destructor destructor)
{
    PyCapsule *capsule;

    if (!pointer) {
        PyErr_SetString(PyExc_ValueError, "PyCapsule_New called with null pointer");
        return NULL;
    }

    capsule = PyObject_New(PyCapsule, &PyCapsule_Type);
    if (capsule == NULL) {
        return NULL;
    }

    capsule->pointer = pointer;
    capsule->name = name;
    capsule->context = NULL;
    capsule->destructor = destructor;

    return (PyObject *)capsule;
}


int
PyCapsule_IsValid(PyObject *o, const char *name)
{
    PyCapsule *capsule = (PyCapsule *)o;

    return (capsule != NULL &&
            PyCapsule_CheckExact(capsule) &&
            capsule->pointer != NULL &&
            name_matches(capsule->name, name));
}


void *
PyCapsule_GetPointer(PyObject *o, const char *name)
{
    PyCapsule *capsule = (PyCapsule *)o;

    if (!is_legal_capsule(capsule, "PyCapsule_GetPointer")) {
        return NULL;
    }

    if (!name_matches(name, capsule->name)) {
        PyErr_SetString(PyExc_ValueError, "PyCapsule_GetPointer called with incorrect name");
        return NULL;
    }

    return capsule->pointer;
}


const char *
PyCapsule_GetName(PyObject *o)
{
    PyCapsule *capsule = (PyCapsule *)o;

    if (!is_legal_capsule(capsule, "PyCapsule_GetName")) {
        return NULL;
    }
    return capsule->name;
}


PyCapsule_Destructor
PyCapsule_GetDestructor(PyObject *o)
{
    PyCapsule *capsule = (PyCapsule *)o;

    if (!is_legal_capsule(capsule, "PyCapsule_GetDestructor")) {
        return NULL;
    }
    return capsule->destructor;
}


void *
PyCapsule_GetContext(PyObject *o)
{
    PyCapsule *capsule = (PyCapsule *)o;

    if (!is_legal_capsule(capsule, "PyCapsule_GetContext")) {
        return NULL;
    }
    return capsule->context;
}


int
PyCapsule_SetPointer(PyObject *o, void *pointer)
{
    PyCapsule *capsule = (PyCapsule *)o;

    if (!pointer) {
        PyErr_SetString(PyExc_ValueError, "PyCapsule_SetPointer called with null pointer");
        return -1;
    }

    if (!is_legal_capsule(capsule, "PyCapsule_SetPointer")) {
        return -1;
    }

    capsule->pointer = pointer;
    return 0;
}


int
PyCapsule_SetName(PyObject *o, const char *name)
{
    PyCapsule *capsule = (PyCapsule *)o;

    if (!is_legal_capsule(capsule, "PyCapsule_SetName")) {
        return -1;
    }

    capsule->name = name;
    return 0;
}


int
PyCapsule_SetDestructor(PyObject *o, PyCapsule_Destructor destructor)
{
    PyCapsule *capsule = (PyCapsule *)o;

    if (!is_legal_capsule(capsule, "PyCapsule_SetDestructor")) {
        return -1;
    }

    capsule->destructor = destructor;
    return 0;
}


int
PyCapsule_SetContext(PyObject *o, void *context)
{
    PyCapsule *capsule = (PyCapsule *)o;

    if (!is_legal_capsule(capsule, "PyCapsule_SetContext")) {
        return -1;
    }

    capsule->context = context;
    return 0;
}


void *
PyCapsule_Import(const char *name, int no_block)
{
    PyObject *object = NULL;
    void *return_value = NULL;
    char *trace;
    size_t name_length = (strlen(name) + 1) * sizeof(char);
    char *name_dup = (char *)PyMem_Malloc(name_length);

    if (!name_dup) {
        return PyErr_NoMemory();
    }

    memcpy(name_dup, name, name_length);

    trace = name_dup;
    while (trace) {
        char *dot = strchr(trace, '.');
        if (dot) {
            *dot++ = '\0';
        }

        if (object == NULL) {
            object = PyImport_ImportModule(trace);
            if (!object) {
                PyErr_Format(PyExc_ImportError, "PyCapsule_Import could not import module \"%s\"", trace);
            }
        } else {
            PyObject *object2 = PyObject_GetAttrString(object, trace);
            Py_DECREF(object);
            object = object2;
        }
        if (!object) {
            goto EXIT;
        }

        trace = dot;
    }

    /* compare attribute name to module.name by hand */
    if (PyCapsule_IsValid(object, name)) {
        PyCapsule *capsule = (PyCapsule *)object;
        return_value = capsule->pointer;
    } else {
        PyErr_Format(PyExc_AttributeError,
            "PyCapsule_Import \"%s\" is not valid",
            name);
    }

EXIT:
    Py_XDECREF(object);
    if (name_dup) {
        PyMem_Free(name_dup);
    }
    return return_value;
}


static void
capsule_dealloc(PyObject *o)
{
    PyCapsule *capsule = (PyCapsule *)o;
    if (capsule->destructor) {
        capsule->destructor(o);
    }
    PyObject_Free(o);
}


static PyObject *
capsule_repr(PyObject *o)
{
    PyCapsule *capsule = (PyCapsule *)o;
    const char *name;
    const char *quote;

    if (capsule->name) {
        quote = "\"";
        name = capsule->name;
    } else {
        quote = "";
        name = "NULL";
    }

    return PyUnicode_FromFormat("<capsule object %s%s%s at %p>",
        quote, name, quote, capsule);
}



PyDoc_STRVAR(PyCapsule_Type__doc__,
"Capsule objects let you wrap a C \"void *\" pointer in a Python\n\
object.  They're a way of passing data through the Python interpreter\n\
without creating your own custom type.\n\
\n\
Capsules are used for communication between extension modules.\n\
They provide a way for an extension module to export a C interface\n\
to other extension modules, so that extension modules can use the\n\
Python import mechanism to link to one another.\n\
");

PyTypeObject PyCapsule_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "PyCapsule",                /*tp_name*/
    sizeof(PyCapsule),          /*tp_basicsize*/
    0,                          /*tp_itemsize*/
    /* methods */
    capsule_dealloc, /*tp_dealloc*/
    0,                          /*tp_vectorcall_offset*/
    0,                          /*tp_getattr*/
    0,                          /*tp_setattr*/
    0,                          /*tp_as_async*/
    capsule_repr, /*tp_repr*/
    0,                          /*tp_as_number*/
    0,                          /*tp_as_sequence*/
    0,                          /*tp_as_mapping*/
    0,                          /*tp_hash*/
    0,                          /*tp_call*/
    0,                          /*tp_str*/
    0,                          /*tp_getattro*/
    0,                          /*tp_setattro*/
    0,                          /*tp_as_buffer*/
    0,                          /*tp_flags*/
    PyCapsule_Type__doc__       /*tp_doc*/
};


