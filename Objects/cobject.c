
/* Wrap void* pointers to be passed between C modules */

#include "Python.h"


/* Declarations for objects of type PyCObject */

typedef void (*destructor1)(void *);
typedef void (*destructor2)(void *, void*);

typedef struct {
    PyObject_HEAD
    void *cobject;
    void *desc;
    void (*destructor)(void *);
} PyCObject;

PyObject *
PyCObject_FromVoidPtr(void *cobj, void (*destr)(void *))
{
    PyCObject *self;

    self = PyObject_NEW(PyCObject, &PyCObject_Type);
    if (self == NULL)
        return NULL;
    self->cobject=cobj;
    self->destructor=destr;
    self->desc=NULL;

    return (PyObject *)self;
}

PyObject *
PyCObject_FromVoidPtrAndDesc(void *cobj, void *desc,
                             void (*destr)(void *, void *))
{
    PyCObject *self;

    if (!desc) {
        PyErr_SetString(PyExc_TypeError,
                        "PyCObject_FromVoidPtrAndDesc called with null"
                        " description");
        return NULL;
    }
    self = PyObject_NEW(PyCObject, &PyCObject_Type);
    if (self == NULL)
        return NULL;
    self->cobject=cobj;
    self->destructor=(destructor1)destr;
    self->desc=desc;

    return (PyObject *)self;
}

void *
PyCObject_AsVoidPtr(PyObject *self)
{
    if (self) {
        if (self->ob_type == &PyCObject_Type)
            return ((PyCObject *)self)->cobject;
        PyErr_SetString(PyExc_TypeError,
                        "PyCObject_AsVoidPtr with non-C-object");
    }
    if (!PyErr_Occurred())
        PyErr_SetString(PyExc_TypeError,
                        "PyCObject_AsVoidPtr called with null pointer");
    return NULL;
}

void *
PyCObject_GetDesc(PyObject *self)
{
    if (self) {
        if (self->ob_type == &PyCObject_Type)
            return ((PyCObject *)self)->desc;
        PyErr_SetString(PyExc_TypeError,
                        "PyCObject_GetDesc with non-C-object");
    }
    if (!PyErr_Occurred())
        PyErr_SetString(PyExc_TypeError,
                        "PyCObject_GetDesc called with null pointer");
    return NULL;
}

void *
PyCObject_Import(char *module_name, char *name)
{
    PyObject *m, *c;
    void *r = NULL;

    if ((m = PyImport_ImportModule(module_name))) {
        if ((c = PyObject_GetAttrString(m,name))) {
            r = PyCObject_AsVoidPtr(c);
            Py_DECREF(c);
	}
        Py_DECREF(m);
    }
    return r;
}

static void
PyCObject_dealloc(PyCObject *self)
{
    if (self->destructor) {
        if(self->desc)
            ((destructor2)(self->destructor))(self->cobject, self->desc);
        else
            (self->destructor)(self->cobject);
    }
    PyObject_DEL(self);
}


static char PyCObject_Type__doc__[] = 
"C objects to be exported from one extension module to another\n\
\n\
C objects are used for communication between extension modules.  They\n\
provide a way for an extension module to export a C interface to other\n\
extension modules, so that extension modules can use the Python import\n\
mechanism to link to one another.";

PyTypeObject PyCObject_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,					/*ob_size*/
    "PyCObject",			/*tp_name*/
    sizeof(PyCObject),			/*tp_basicsize*/
    0,					/*tp_itemsize*/
    /* methods */
    (destructor)PyCObject_dealloc,	/*tp_dealloc*/
    (printfunc)0,			/*tp_print*/
    (getattrfunc)0,			/*tp_getattr*/
    (setattrfunc)0,			/*tp_setattr*/
    (cmpfunc)0,				/*tp_compare*/
    (reprfunc)0,			/*tp_repr*/
    0,					/*tp_as_number*/
    0,					/*tp_as_sequence*/
    0,					/*tp_as_mapping*/
    (hashfunc)0,			/*tp_hash*/
    (ternaryfunc)0,			/*tp_call*/
    (reprfunc)0,			/*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    PyCObject_Type__doc__ 		/* Documentation string */
};
