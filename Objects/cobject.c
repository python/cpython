
/* Wrap void* pointers to be passed between C modules */

#include "Python.h"


/* Declarations for objects of type PyCObject */

typedef void (*destructor1)(void *);
typedef void (*destructor2)(void *, void*);

static int cobject_deprecation_warning(void)
{
    return PyErr_WarnPy3k("CObject type is not supported in 3.x. "
        "Please use capsule objects instead.", 1);
}


PyObject *
PyCObject_FromVoidPtr(void *cobj, void (*destr)(void *))
{
    PyCObject *self;

    if (cobject_deprecation_warning()) {
        return NULL;
    }

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

    if (cobject_deprecation_warning()) {
        return NULL;
    }

    if (!desc) {
        PyErr_SetString(PyExc_TypeError,
                        "PyCObject_FromVoidPtrAndDesc called with null"
                        " description");
        return NULL;
    }
    self = PyObject_NEW(PyCObject, &PyCObject_Type);
    if (self == NULL)
        return NULL;
    self->cobject = cobj;
    self->destructor = (destructor1)destr;
    self->desc = desc;

    return (PyObject *)self;
}

void *
PyCObject_AsVoidPtr(PyObject *self)
{
    if (self) {
        if (PyCapsule_CheckExact(self)) {
            const char *name = PyCapsule_GetName(self);
            return (void *)PyCapsule_GetPointer(self, name);
        }
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

int
PyCObject_SetVoidPtr(PyObject *self, void *cobj)
{
    PyCObject* cself = (PyCObject*)self;
    if (cself == NULL || !PyCObject_Check(cself) ||
	cself->destructor != NULL) {
	PyErr_SetString(PyExc_TypeError, 
			"Invalid call to PyCObject_SetVoidPtr");
	return 0;
    }
    cself->cobject = cobj;
    return 1;
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


PyDoc_STRVAR(PyCObject_Type__doc__,
"C objects to be exported from one extension module to another\n\
\n\
C objects are used for communication between extension modules.  They\n\
provide a way for an extension module to export a C interface to other\n\
extension modules, so that extension modules can use the Python import\n\
mechanism to link to one another.");

PyTypeObject PyCObject_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "PyCObject",		/*tp_name*/
    sizeof(PyCObject),		/*tp_basicsize*/
    0,				/*tp_itemsize*/
    /* methods */
    (destructor)PyCObject_dealloc, /*tp_dealloc*/
    0,				/*tp_print*/
    0,				/*tp_getattr*/
    0,				/*tp_setattr*/
    0,				/*tp_compare*/
    0,				/*tp_repr*/
    0,				/*tp_as_number*/
    0,				/*tp_as_sequence*/
    0,				/*tp_as_mapping*/
    0,				/*tp_hash*/
    0,				/*tp_call*/
    0,				/*tp_str*/
    0,				/*tp_getattro*/
    0,				/*tp_setattro*/
    0,				/*tp_as_buffer*/
    0,				/*tp_flags*/
    PyCObject_Type__doc__	/*tp_doc*/
};
