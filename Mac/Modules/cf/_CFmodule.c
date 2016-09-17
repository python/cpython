
/* =========================== Module _CF =========================== */

#include "Python.h"



#include "pymactoolbox.h"

/* Macro to test whether a weak-loaded CFM function exists */
#define PyMac_PRECHECK(rtn) do { if ( &rtn == NULL )  {\
    PyErr_SetString(PyExc_NotImplementedError, \
    "Not available in this shared library/OS version"); \
    return NULL; \
    }} while(0)


#include <CoreServices/CoreServices.h>

#include "pycfbridge.h"

#ifdef USE_TOOLBOX_OBJECT_GLUE
extern PyObject *_CFObj_New(CFTypeRef);
extern int _CFObj_Convert(PyObject *, CFTypeRef *);
#define CFObj_New _CFObj_New
#define CFObj_Convert _CFObj_Convert

extern PyObject *_CFTypeRefObj_New(CFTypeRef);
extern int _CFTypeRefObj_Convert(PyObject *, CFTypeRef *);
#define CFTypeRefObj_New _CFTypeRefObj_New
#define CFTypeRefObj_Convert _CFTypeRefObj_Convert

extern PyObject *_CFStringRefObj_New(CFStringRef);
extern int _CFStringRefObj_Convert(PyObject *, CFStringRef *);
#define CFStringRefObj_New _CFStringRefObj_New
#define CFStringRefObj_Convert _CFStringRefObj_Convert

extern PyObject *_CFMutableStringRefObj_New(CFMutableStringRef);
extern int _CFMutableStringRefObj_Convert(PyObject *, CFMutableStringRef *);
#define CFMutableStringRefObj_New _CFMutableStringRefObj_New
#define CFMutableStringRefObj_Convert _CFMutableStringRefObj_Convert

extern PyObject *_CFArrayRefObj_New(CFArrayRef);
extern int _CFArrayRefObj_Convert(PyObject *, CFArrayRef *);
#define CFArrayRefObj_New _CFArrayRefObj_New
#define CFArrayRefObj_Convert _CFArrayRefObj_Convert

extern PyObject *_CFMutableArrayRefObj_New(CFMutableArrayRef);
extern int _CFMutableArrayRefObj_Convert(PyObject *, CFMutableArrayRef *);
#define CFMutableArrayRefObj_New _CFMutableArrayRefObj_New
#define CFMutableArrayRefObj_Convert _CFMutableArrayRefObj_Convert

extern PyObject *_CFDataRefObj_New(CFDataRef);
extern int _CFDataRefObj_Convert(PyObject *, CFDataRef *);
#define CFDataRefObj_New _CFDataRefObj_New
#define CFDataRefObj_Convert _CFDataRefObj_Convert

extern PyObject *_CFMutableDataRefObj_New(CFMutableDataRef);
extern int _CFMutableDataRefObj_Convert(PyObject *, CFMutableDataRef *);
#define CFMutableDataRefObj_New _CFMutableDataRefObj_New
#define CFMutableDataRefObj_Convert _CFMutableDataRefObj_Convert

extern PyObject *_CFDictionaryRefObj_New(CFDictionaryRef);
extern int _CFDictionaryRefObj_Convert(PyObject *, CFDictionaryRef *);
#define CFDictionaryRefObj_New _CFDictionaryRefObj_New
#define CFDictionaryRefObj_Convert _CFDictionaryRefObj_Convert

extern PyObject *_CFMutableDictionaryRefObj_New(CFMutableDictionaryRef);
extern int _CFMutableDictionaryRefObj_Convert(PyObject *, CFMutableDictionaryRef *);
#define CFMutableDictionaryRefObj_New _CFMutableDictionaryRefObj_New
#define CFMutableDictionaryRefObj_Convert _CFMutableDictionaryRefObj_Convert

extern PyObject *_CFURLRefObj_New(CFURLRef);
extern int _CFURLRefObj_Convert(PyObject *, CFURLRef *);
extern int _OptionalCFURLRefObj_Convert(PyObject *, CFURLRef *);
#define CFURLRefObj_New _CFURLRefObj_New
#define CFURLRefObj_Convert _CFURLRefObj_Convert
#define OptionalCFURLRefObj_Convert _OptionalCFURLRefObj_Convert
#endif

/*
** Parse/generate CFRange records
*/
PyObject *CFRange_New(CFRange *itself)
{
    return Py_BuildValue("ll", (long)itself->location, (long)itself->length);
}

int
CFRange_Convert(PyObject *v, CFRange *p_itself)
{
    long location, length;

    if( !PyArg_ParseTuple(v, "ll", &location, &length) )
        return 0;
    p_itself->location = (CFIndex)location;
    p_itself->length = (CFIndex)length;
    return 1;
}

/* Optional CFURL argument or None (passed as NULL) */
int
OptionalCFURLRefObj_Convert(PyObject *v, CFURLRef *p_itself)
{
    if ( v == Py_None ) {
        p_itself = NULL;
        return 1;
    }
    return CFURLRefObj_Convert(v, p_itself);
}

static PyObject *CF_Error;

/* --------------------- Object type CFTypeRef ---------------------- */

PyTypeObject CFTypeRef_Type;

#define CFTypeRefObj_Check(x) ((x)->ob_type == &CFTypeRef_Type || PyObject_TypeCheck((x), &CFTypeRef_Type))

typedef struct CFTypeRefObject {
    PyObject_HEAD
    CFTypeRef ob_itself;
    void (*ob_freeit)(CFTypeRef ptr);
} CFTypeRefObject;

PyObject *CFTypeRefObj_New(CFTypeRef itself)
{
    CFTypeRefObject *it;
    if (itself == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError, "cannot wrap NULL");
        return NULL;
    }
    it = PyObject_NEW(CFTypeRefObject, &CFTypeRef_Type);
    if (it == NULL) return NULL;
    it->ob_itself = itself;
    it->ob_freeit = CFRelease;
    return (PyObject *)it;
}

int CFTypeRefObj_Convert(PyObject *v, CFTypeRef *p_itself)
{

    if (v == Py_None) { *p_itself = NULL; return 1; }
    /* Check for other CF objects here */

    if (!CFTypeRefObj_Check(v))
    {
        PyErr_SetString(PyExc_TypeError, "CFTypeRef required");
        return 0;
    }
    *p_itself = ((CFTypeRefObject *)v)->ob_itself;
    return 1;
}

static void CFTypeRefObj_dealloc(CFTypeRefObject *self)
{
    if (self->ob_freeit && self->ob_itself)
    {
        self->ob_freeit((CFTypeRef)self->ob_itself);
        self->ob_itself = NULL;
    }
    self->ob_type->tp_free((PyObject *)self);
}

static PyObject *CFTypeRefObj_CFGetTypeID(CFTypeRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFTypeID _rv;
#ifndef CFGetTypeID
    PyMac_PRECHECK(CFGetTypeID);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFGetTypeID(_self->ob_itself);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CFTypeRefObj_CFRetain(CFTypeRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFTypeRef _rv;
#ifndef CFRetain
    PyMac_PRECHECK(CFRetain);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFRetain(_self->ob_itself);
    _res = Py_BuildValue("O&",
                         CFTypeRefObj_New, _rv);
    return _res;
}

static PyObject *CFTypeRefObj_CFRelease(CFTypeRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
#ifndef CFRelease
    PyMac_PRECHECK(CFRelease);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    CFRelease(_self->ob_itself);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CFTypeRefObj_CFGetRetainCount(CFTypeRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFIndex _rv;
#ifndef CFGetRetainCount
    PyMac_PRECHECK(CFGetRetainCount);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFGetRetainCount(_self->ob_itself);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CFTypeRefObj_CFEqual(CFTypeRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
    CFTypeRef cf2;
#ifndef CFEqual
    PyMac_PRECHECK(CFEqual);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          CFTypeRefObj_Convert, &cf2))
        return NULL;
    _rv = CFEqual(_self->ob_itself,
                  cf2);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CFTypeRefObj_CFHash(CFTypeRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFHashCode _rv;
#ifndef CFHash
    PyMac_PRECHECK(CFHash);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFHash(_self->ob_itself);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CFTypeRefObj_CFCopyDescription(CFTypeRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
#ifndef CFCopyDescription
    PyMac_PRECHECK(CFCopyDescription);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFCopyDescription(_self->ob_itself);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CFTypeRefObj_CFPropertyListCreateXMLData(CFTypeRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFDataRef _rv;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFPropertyListCreateXMLData((CFAllocatorRef)NULL,
                                      _self->ob_itself);
    _res = Py_BuildValue("O&",
                         CFDataRefObj_New, _rv);
    return _res;
}

static PyObject *CFTypeRefObj_CFPropertyListCreateDeepCopy(CFTypeRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFTypeRef _rv;
    CFOptionFlags mutabilityOption;
    if (!PyArg_ParseTuple(_args, "l",
                          &mutabilityOption))
        return NULL;
    _rv = CFPropertyListCreateDeepCopy((CFAllocatorRef)NULL,
                                       _self->ob_itself,
                                       mutabilityOption);
    _res = Py_BuildValue("O&",
                         CFTypeRefObj_New, _rv);
    return _res;
}

static PyObject *CFTypeRefObj_CFShow(CFTypeRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
#ifndef CFShow
    PyMac_PRECHECK(CFShow);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    CFShow(_self->ob_itself);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CFTypeRefObj_CFPropertyListCreateFromXMLData(CFTypeRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;

    CFTypeRef _rv;
    CFOptionFlags mutabilityOption;
    CFStringRef errorString;
    if (!PyArg_ParseTuple(_args, "l",
                          &mutabilityOption))
        return NULL;
    _rv = CFPropertyListCreateFromXMLData((CFAllocatorRef)NULL,
                                          _self->ob_itself,
                                          mutabilityOption,
                                          &errorString);
    if (errorString)
        CFRelease(errorString);
    if (_rv == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Parse error in XML data");
        return NULL;
    }
    _res = Py_BuildValue("O&",
                         CFTypeRefObj_New, _rv);
    return _res;

}

static PyObject *CFTypeRefObj_toPython(CFTypeRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;

    _res = PyCF_CF2Python(_self->ob_itself);
    return _res;

}

static PyMethodDef CFTypeRefObj_methods[] = {
    {"CFGetTypeID", (PyCFunction)CFTypeRefObj_CFGetTypeID, 1,
     PyDoc_STR("() -> (CFTypeID _rv)")},
    {"CFRetain", (PyCFunction)CFTypeRefObj_CFRetain, 1,
     PyDoc_STR("() -> (CFTypeRef _rv)")},
    {"CFRelease", (PyCFunction)CFTypeRefObj_CFRelease, 1,
     PyDoc_STR("() -> None")},
    {"CFGetRetainCount", (PyCFunction)CFTypeRefObj_CFGetRetainCount, 1,
     PyDoc_STR("() -> (CFIndex _rv)")},
    {"CFEqual", (PyCFunction)CFTypeRefObj_CFEqual, 1,
     PyDoc_STR("(CFTypeRef cf2) -> (Boolean _rv)")},
    {"CFHash", (PyCFunction)CFTypeRefObj_CFHash, 1,
     PyDoc_STR("() -> (CFHashCode _rv)")},
    {"CFCopyDescription", (PyCFunction)CFTypeRefObj_CFCopyDescription, 1,
     PyDoc_STR("() -> (CFStringRef _rv)")},
    {"CFPropertyListCreateXMLData", (PyCFunction)CFTypeRefObj_CFPropertyListCreateXMLData, 1,
     PyDoc_STR("() -> (CFDataRef _rv)")},
    {"CFPropertyListCreateDeepCopy", (PyCFunction)CFTypeRefObj_CFPropertyListCreateDeepCopy, 1,
     PyDoc_STR("(CFOptionFlags mutabilityOption) -> (CFTypeRef _rv)")},
    {"CFShow", (PyCFunction)CFTypeRefObj_CFShow, 1,
     PyDoc_STR("() -> None")},
    {"CFPropertyListCreateFromXMLData", (PyCFunction)CFTypeRefObj_CFPropertyListCreateFromXMLData, 1,
     PyDoc_STR("(CFOptionFlags mutabilityOption) -> (CFTypeRefObj)")},
    {"toPython", (PyCFunction)CFTypeRefObj_toPython, 1,
     PyDoc_STR("() -> (python_object)")},
    {NULL, NULL, 0}
};

#define CFTypeRefObj_getsetlist NULL


static int CFTypeRefObj_compare(CFTypeRefObject *self, CFTypeRefObject *other)
{
    /* XXXX Or should we use CFEqual?? */
    if ( self->ob_itself > other->ob_itself ) return 1;
    if ( self->ob_itself < other->ob_itself ) return -1;
    return 0;
}

static PyObject * CFTypeRefObj_repr(CFTypeRefObject *self)
{
    char buf[100];
    sprintf(buf, "<CFTypeRef type-%d object at 0x%8.8x for 0x%8.8x>", (int)CFGetTypeID(self->ob_itself), (unsigned)self, (unsigned)self->ob_itself);
    return PyString_FromString(buf);
}

static int CFTypeRefObj_hash(CFTypeRefObject *self)
{
    /* XXXX Or should we use CFHash?? */
    return (int)self->ob_itself;
}
static int CFTypeRefObj_tp_init(PyObject *_self, PyObject *_args, PyObject *_kwds)
{
    CFTypeRef itself;
    char *kw[] = {"itself", 0};

    if (PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, CFTypeRefObj_Convert, &itself))
    {
        ((CFTypeRefObject *)_self)->ob_itself = itself;
        return 0;
    }
    return -1;
}

#define CFTypeRefObj_tp_alloc PyType_GenericAlloc

static PyObject *CFTypeRefObj_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
    PyObject *self;
    if ((self = type->tp_alloc(type, 0)) == NULL) return NULL;
    ((CFTypeRefObject *)self)->ob_itself = NULL;
    ((CFTypeRefObject *)self)->ob_freeit = CFRelease;
    return self;
}

#define CFTypeRefObj_tp_free PyObject_Del


PyTypeObject CFTypeRef_Type = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "_CF.CFTypeRef", /*tp_name*/
    sizeof(CFTypeRefObject), /*tp_basicsize*/
    0, /*tp_itemsize*/
    /* methods */
    (destructor) CFTypeRefObj_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)0, /*tp_getattr*/
    (setattrfunc)0, /*tp_setattr*/
    (cmpfunc) CFTypeRefObj_compare, /*tp_compare*/
    (reprfunc) CFTypeRefObj_repr, /*tp_repr*/
    (PyNumberMethods *)0, /* tp_as_number */
    (PySequenceMethods *)0, /* tp_as_sequence */
    (PyMappingMethods *)0, /* tp_as_mapping */
    (hashfunc) CFTypeRefObj_hash, /*tp_hash*/
    0, /*tp_call*/
    0, /*tp_str*/
    PyObject_GenericGetAttr, /*tp_getattro*/
    PyObject_GenericSetAttr, /*tp_setattro */
    0, /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, /* tp_flags */
    0, /*tp_doc*/
    0, /*tp_traverse*/
    0, /*tp_clear*/
    0, /*tp_richcompare*/
    0, /*tp_weaklistoffset*/
    0, /*tp_iter*/
    0, /*tp_iternext*/
    CFTypeRefObj_methods, /* tp_methods */
    0, /*tp_members*/
    CFTypeRefObj_getsetlist, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    CFTypeRefObj_tp_init, /* tp_init */
    CFTypeRefObj_tp_alloc, /* tp_alloc */
    CFTypeRefObj_tp_new, /* tp_new */
    CFTypeRefObj_tp_free, /* tp_free */
};

/* ------------------- End object type CFTypeRef -------------------- */


/* --------------------- Object type CFArrayRef --------------------- */

PyTypeObject CFArrayRef_Type;

#define CFArrayRefObj_Check(x) ((x)->ob_type == &CFArrayRef_Type || PyObject_TypeCheck((x), &CFArrayRef_Type))

typedef struct CFArrayRefObject {
    PyObject_HEAD
    CFArrayRef ob_itself;
    void (*ob_freeit)(CFTypeRef ptr);
} CFArrayRefObject;

PyObject *CFArrayRefObj_New(CFArrayRef itself)
{
    CFArrayRefObject *it;
    if (itself == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError, "cannot wrap NULL");
        return NULL;
    }
    it = PyObject_NEW(CFArrayRefObject, &CFArrayRef_Type);
    if (it == NULL) return NULL;
    /* XXXX Should we tp_init or tp_new our basetype? */
    it->ob_itself = itself;
    it->ob_freeit = CFRelease;
    return (PyObject *)it;
}

int CFArrayRefObj_Convert(PyObject *v, CFArrayRef *p_itself)
{

    if (v == Py_None) { *p_itself = NULL; return 1; }
    /* Check for other CF objects here */

    if (!CFArrayRefObj_Check(v))
    {
        PyErr_SetString(PyExc_TypeError, "CFArrayRef required");
        return 0;
    }
    *p_itself = ((CFArrayRefObject *)v)->ob_itself;
    return 1;
}

static void CFArrayRefObj_dealloc(CFArrayRefObject *self)
{
    if (self->ob_freeit && self->ob_itself)
    {
        self->ob_freeit((CFTypeRef)self->ob_itself);
        self->ob_itself = NULL;
    }
    CFTypeRef_Type.tp_dealloc((PyObject *)self);
}

static PyObject *CFArrayRefObj_CFArrayCreateCopy(CFArrayRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFArrayRef _rv;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFArrayCreateCopy((CFAllocatorRef)NULL,
                            _self->ob_itself);
    _res = Py_BuildValue("O&",
                         CFArrayRefObj_New, _rv);
    return _res;
}

static PyObject *CFArrayRefObj_CFArrayGetCount(CFArrayRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFIndex _rv;
#ifndef CFArrayGetCount
    PyMac_PRECHECK(CFArrayGetCount);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFArrayGetCount(_self->ob_itself);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CFArrayRefObj_CFStringCreateByCombiningStrings(CFArrayRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
    CFStringRef separatorString;
    if (!PyArg_ParseTuple(_args, "O&",
                          CFStringRefObj_Convert, &separatorString))
        return NULL;
    _rv = CFStringCreateByCombiningStrings((CFAllocatorRef)NULL,
                                           _self->ob_itself,
                                           separatorString);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyMethodDef CFArrayRefObj_methods[] = {
    {"CFArrayCreateCopy", (PyCFunction)CFArrayRefObj_CFArrayCreateCopy, 1,
     PyDoc_STR("() -> (CFArrayRef _rv)")},
    {"CFArrayGetCount", (PyCFunction)CFArrayRefObj_CFArrayGetCount, 1,
     PyDoc_STR("() -> (CFIndex _rv)")},
    {"CFStringCreateByCombiningStrings", (PyCFunction)CFArrayRefObj_CFStringCreateByCombiningStrings, 1,
     PyDoc_STR("(CFStringRef separatorString) -> (CFStringRef _rv)")},
    {NULL, NULL, 0}
};

#define CFArrayRefObj_getsetlist NULL


static int CFArrayRefObj_compare(CFArrayRefObject *self, CFArrayRefObject *other)
{
    /* XXXX Or should we use CFEqual?? */
    if ( self->ob_itself > other->ob_itself ) return 1;
    if ( self->ob_itself < other->ob_itself ) return -1;
    return 0;
}

static PyObject * CFArrayRefObj_repr(CFArrayRefObject *self)
{
    char buf[100];
    sprintf(buf, "<CFArrayRef object at 0x%8.8x for 0x%8.8x>", (unsigned)self, (unsigned)self->ob_itself);
    return PyString_FromString(buf);
}

static int CFArrayRefObj_hash(CFArrayRefObject *self)
{
    /* XXXX Or should we use CFHash?? */
    return (int)self->ob_itself;
}
static int CFArrayRefObj_tp_init(PyObject *_self, PyObject *_args, PyObject *_kwds)
{
    CFArrayRef itself;
    char *kw[] = {"itself", 0};

    if (PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, CFArrayRefObj_Convert, &itself))
    {
        ((CFArrayRefObject *)_self)->ob_itself = itself;
        return 0;
    }

    /* Any CFTypeRef descendent is allowed as initializer too */
    if (PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, CFTypeRefObj_Convert, &itself))
    {
        ((CFArrayRefObject *)_self)->ob_itself = itself;
        return 0;
    }
    return -1;
}

#define CFArrayRefObj_tp_alloc PyType_GenericAlloc

static PyObject *CFArrayRefObj_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
    PyObject *self;
    if ((self = type->tp_alloc(type, 0)) == NULL) return NULL;
    ((CFArrayRefObject *)self)->ob_itself = NULL;
    ((CFArrayRefObject *)self)->ob_freeit = CFRelease;
    return self;
}

#define CFArrayRefObj_tp_free PyObject_Del


PyTypeObject CFArrayRef_Type = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "_CF.CFArrayRef", /*tp_name*/
    sizeof(CFArrayRefObject), /*tp_basicsize*/
    0, /*tp_itemsize*/
    /* methods */
    (destructor) CFArrayRefObj_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)0, /*tp_getattr*/
    (setattrfunc)0, /*tp_setattr*/
    (cmpfunc) CFArrayRefObj_compare, /*tp_compare*/
    (reprfunc) CFArrayRefObj_repr, /*tp_repr*/
    (PyNumberMethods *)0, /* tp_as_number */
    (PySequenceMethods *)0, /* tp_as_sequence */
    (PyMappingMethods *)0, /* tp_as_mapping */
    (hashfunc) CFArrayRefObj_hash, /*tp_hash*/
    0, /*tp_call*/
    0, /*tp_str*/
    PyObject_GenericGetAttr, /*tp_getattro*/
    PyObject_GenericSetAttr, /*tp_setattro */
    0, /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, /* tp_flags */
    0, /*tp_doc*/
    0, /*tp_traverse*/
    0, /*tp_clear*/
    0, /*tp_richcompare*/
    0, /*tp_weaklistoffset*/
    0, /*tp_iter*/
    0, /*tp_iternext*/
    CFArrayRefObj_methods, /* tp_methods */
    0, /*tp_members*/
    CFArrayRefObj_getsetlist, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    CFArrayRefObj_tp_init, /* tp_init */
    CFArrayRefObj_tp_alloc, /* tp_alloc */
    CFArrayRefObj_tp_new, /* tp_new */
    CFArrayRefObj_tp_free, /* tp_free */
};

/* ------------------- End object type CFArrayRef ------------------- */


/* ----------------- Object type CFMutableArrayRef ------------------ */

PyTypeObject CFMutableArrayRef_Type;

#define CFMutableArrayRefObj_Check(x) ((x)->ob_type == &CFMutableArrayRef_Type || PyObject_TypeCheck((x), &CFMutableArrayRef_Type))

typedef struct CFMutableArrayRefObject {
    PyObject_HEAD
    CFMutableArrayRef ob_itself;
    void (*ob_freeit)(CFTypeRef ptr);
} CFMutableArrayRefObject;

PyObject *CFMutableArrayRefObj_New(CFMutableArrayRef itself)
{
    CFMutableArrayRefObject *it;
    if (itself == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError, "cannot wrap NULL");
        return NULL;
    }
    it = PyObject_NEW(CFMutableArrayRefObject, &CFMutableArrayRef_Type);
    if (it == NULL) return NULL;
    /* XXXX Should we tp_init or tp_new our basetype? */
    it->ob_itself = itself;
    it->ob_freeit = CFRelease;
    return (PyObject *)it;
}

int CFMutableArrayRefObj_Convert(PyObject *v, CFMutableArrayRef *p_itself)
{

    if (v == Py_None) { *p_itself = NULL; return 1; }
    /* Check for other CF objects here */

    if (!CFMutableArrayRefObj_Check(v))
    {
        PyErr_SetString(PyExc_TypeError, "CFMutableArrayRef required");
        return 0;
    }
    *p_itself = ((CFMutableArrayRefObject *)v)->ob_itself;
    return 1;
}

static void CFMutableArrayRefObj_dealloc(CFMutableArrayRefObject *self)
{
    if (self->ob_freeit && self->ob_itself)
    {
        self->ob_freeit((CFTypeRef)self->ob_itself);
        self->ob_itself = NULL;
    }
    CFArrayRef_Type.tp_dealloc((PyObject *)self);
}

static PyObject *CFMutableArrayRefObj_CFArrayRemoveValueAtIndex(CFMutableArrayRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFIndex idx;
#ifndef CFArrayRemoveValueAtIndex
    PyMac_PRECHECK(CFArrayRemoveValueAtIndex);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &idx))
        return NULL;
    CFArrayRemoveValueAtIndex(_self->ob_itself,
                              idx);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CFMutableArrayRefObj_CFArrayRemoveAllValues(CFMutableArrayRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
#ifndef CFArrayRemoveAllValues
    PyMac_PRECHECK(CFArrayRemoveAllValues);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    CFArrayRemoveAllValues(_self->ob_itself);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CFMutableArrayRefObj_CFArrayExchangeValuesAtIndices(CFMutableArrayRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFIndex idx1;
    CFIndex idx2;
#ifndef CFArrayExchangeValuesAtIndices
    PyMac_PRECHECK(CFArrayExchangeValuesAtIndices);
#endif
    if (!PyArg_ParseTuple(_args, "ll",
                          &idx1,
                          &idx2))
        return NULL;
    CFArrayExchangeValuesAtIndices(_self->ob_itself,
                                   idx1,
                                   idx2);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CFMutableArrayRefObj_CFArrayAppendArray(CFMutableArrayRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFArrayRef otherArray;
    CFRange otherRange;
#ifndef CFArrayAppendArray
    PyMac_PRECHECK(CFArrayAppendArray);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&",
                          CFArrayRefObj_Convert, &otherArray,
                          CFRange_Convert, &otherRange))
        return NULL;
    CFArrayAppendArray(_self->ob_itself,
                       otherArray,
                       otherRange);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyMethodDef CFMutableArrayRefObj_methods[] = {
    {"CFArrayRemoveValueAtIndex", (PyCFunction)CFMutableArrayRefObj_CFArrayRemoveValueAtIndex, 1,
     PyDoc_STR("(CFIndex idx) -> None")},
    {"CFArrayRemoveAllValues", (PyCFunction)CFMutableArrayRefObj_CFArrayRemoveAllValues, 1,
     PyDoc_STR("() -> None")},
    {"CFArrayExchangeValuesAtIndices", (PyCFunction)CFMutableArrayRefObj_CFArrayExchangeValuesAtIndices, 1,
     PyDoc_STR("(CFIndex idx1, CFIndex idx2) -> None")},
    {"CFArrayAppendArray", (PyCFunction)CFMutableArrayRefObj_CFArrayAppendArray, 1,
     PyDoc_STR("(CFArrayRef otherArray, CFRange otherRange) -> None")},
    {NULL, NULL, 0}
};

#define CFMutableArrayRefObj_getsetlist NULL


static int CFMutableArrayRefObj_compare(CFMutableArrayRefObject *self, CFMutableArrayRefObject *other)
{
    /* XXXX Or should we use CFEqual?? */
    if ( self->ob_itself > other->ob_itself ) return 1;
    if ( self->ob_itself < other->ob_itself ) return -1;
    return 0;
}

static PyObject * CFMutableArrayRefObj_repr(CFMutableArrayRefObject *self)
{
    char buf[100];
    sprintf(buf, "<CFMutableArrayRef object at 0x%8.8x for 0x%8.8x>", (unsigned)self, (unsigned)self->ob_itself);
    return PyString_FromString(buf);
}

static int CFMutableArrayRefObj_hash(CFMutableArrayRefObject *self)
{
    /* XXXX Or should we use CFHash?? */
    return (int)self->ob_itself;
}
static int CFMutableArrayRefObj_tp_init(PyObject *_self, PyObject *_args, PyObject *_kwds)
{
    CFMutableArrayRef itself;
    char *kw[] = {"itself", 0};

    if (PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, CFMutableArrayRefObj_Convert, &itself))
    {
        ((CFMutableArrayRefObject *)_self)->ob_itself = itself;
        return 0;
    }

    /* Any CFTypeRef descendent is allowed as initializer too */
    if (PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, CFTypeRefObj_Convert, &itself))
    {
        ((CFMutableArrayRefObject *)_self)->ob_itself = itself;
        return 0;
    }
    return -1;
}

#define CFMutableArrayRefObj_tp_alloc PyType_GenericAlloc

static PyObject *CFMutableArrayRefObj_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
    PyObject *self;
    if ((self = type->tp_alloc(type, 0)) == NULL) return NULL;
    ((CFMutableArrayRefObject *)self)->ob_itself = NULL;
    ((CFMutableArrayRefObject *)self)->ob_freeit = CFRelease;
    return self;
}

#define CFMutableArrayRefObj_tp_free PyObject_Del


PyTypeObject CFMutableArrayRef_Type = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "_CF.CFMutableArrayRef", /*tp_name*/
    sizeof(CFMutableArrayRefObject), /*tp_basicsize*/
    0, /*tp_itemsize*/
    /* methods */
    (destructor) CFMutableArrayRefObj_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)0, /*tp_getattr*/
    (setattrfunc)0, /*tp_setattr*/
    (cmpfunc) CFMutableArrayRefObj_compare, /*tp_compare*/
    (reprfunc) CFMutableArrayRefObj_repr, /*tp_repr*/
    (PyNumberMethods *)0, /* tp_as_number */
    (PySequenceMethods *)0, /* tp_as_sequence */
    (PyMappingMethods *)0, /* tp_as_mapping */
    (hashfunc) CFMutableArrayRefObj_hash, /*tp_hash*/
    0, /*tp_call*/
    0, /*tp_str*/
    PyObject_GenericGetAttr, /*tp_getattro*/
    PyObject_GenericSetAttr, /*tp_setattro */
    0, /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, /* tp_flags */
    0, /*tp_doc*/
    0, /*tp_traverse*/
    0, /*tp_clear*/
    0, /*tp_richcompare*/
    0, /*tp_weaklistoffset*/
    0, /*tp_iter*/
    0, /*tp_iternext*/
    CFMutableArrayRefObj_methods, /* tp_methods */
    0, /*tp_members*/
    CFMutableArrayRefObj_getsetlist, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    CFMutableArrayRefObj_tp_init, /* tp_init */
    CFMutableArrayRefObj_tp_alloc, /* tp_alloc */
    CFMutableArrayRefObj_tp_new, /* tp_new */
    CFMutableArrayRefObj_tp_free, /* tp_free */
};

/* --------------- End object type CFMutableArrayRef ---------------- */


/* ------------------ Object type CFDictionaryRef ------------------- */

PyTypeObject CFDictionaryRef_Type;

#define CFDictionaryRefObj_Check(x) ((x)->ob_type == &CFDictionaryRef_Type || PyObject_TypeCheck((x), &CFDictionaryRef_Type))

typedef struct CFDictionaryRefObject {
    PyObject_HEAD
    CFDictionaryRef ob_itself;
    void (*ob_freeit)(CFTypeRef ptr);
} CFDictionaryRefObject;

PyObject *CFDictionaryRefObj_New(CFDictionaryRef itself)
{
    CFDictionaryRefObject *it;
    if (itself == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError, "cannot wrap NULL");
        return NULL;
    }
    it = PyObject_NEW(CFDictionaryRefObject, &CFDictionaryRef_Type);
    if (it == NULL) return NULL;
    /* XXXX Should we tp_init or tp_new our basetype? */
    it->ob_itself = itself;
    it->ob_freeit = CFRelease;
    return (PyObject *)it;
}

int CFDictionaryRefObj_Convert(PyObject *v, CFDictionaryRef *p_itself)
{

    if (v == Py_None) { *p_itself = NULL; return 1; }
    /* Check for other CF objects here */

    if (!CFDictionaryRefObj_Check(v))
    {
        PyErr_SetString(PyExc_TypeError, "CFDictionaryRef required");
        return 0;
    }
    *p_itself = ((CFDictionaryRefObject *)v)->ob_itself;
    return 1;
}

static void CFDictionaryRefObj_dealloc(CFDictionaryRefObject *self)
{
    if (self->ob_freeit && self->ob_itself)
    {
        self->ob_freeit((CFTypeRef)self->ob_itself);
        self->ob_itself = NULL;
    }
    CFTypeRef_Type.tp_dealloc((PyObject *)self);
}

static PyObject *CFDictionaryRefObj_CFDictionaryCreateCopy(CFDictionaryRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFDictionaryRef _rv;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFDictionaryCreateCopy((CFAllocatorRef)NULL,
                                 _self->ob_itself);
    _res = Py_BuildValue("O&",
                         CFDictionaryRefObj_New, _rv);
    return _res;
}

static PyObject *CFDictionaryRefObj_CFDictionaryGetCount(CFDictionaryRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFIndex _rv;
#ifndef CFDictionaryGetCount
    PyMac_PRECHECK(CFDictionaryGetCount);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFDictionaryGetCount(_self->ob_itself);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyMethodDef CFDictionaryRefObj_methods[] = {
    {"CFDictionaryCreateCopy", (PyCFunction)CFDictionaryRefObj_CFDictionaryCreateCopy, 1,
     PyDoc_STR("() -> (CFDictionaryRef _rv)")},
    {"CFDictionaryGetCount", (PyCFunction)CFDictionaryRefObj_CFDictionaryGetCount, 1,
     PyDoc_STR("() -> (CFIndex _rv)")},
    {NULL, NULL, 0}
};

#define CFDictionaryRefObj_getsetlist NULL


static int CFDictionaryRefObj_compare(CFDictionaryRefObject *self, CFDictionaryRefObject *other)
{
    /* XXXX Or should we use CFEqual?? */
    if ( self->ob_itself > other->ob_itself ) return 1;
    if ( self->ob_itself < other->ob_itself ) return -1;
    return 0;
}

static PyObject * CFDictionaryRefObj_repr(CFDictionaryRefObject *self)
{
    char buf[100];
    sprintf(buf, "<CFDictionaryRef object at 0x%8.8x for 0x%8.8x>", (unsigned)self, (unsigned)self->ob_itself);
    return PyString_FromString(buf);
}

static int CFDictionaryRefObj_hash(CFDictionaryRefObject *self)
{
    /* XXXX Or should we use CFHash?? */
    return (int)self->ob_itself;
}
static int CFDictionaryRefObj_tp_init(PyObject *_self, PyObject *_args, PyObject *_kwds)
{
    CFDictionaryRef itself;
    char *kw[] = {"itself", 0};

    if (PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, CFDictionaryRefObj_Convert, &itself))
    {
        ((CFDictionaryRefObject *)_self)->ob_itself = itself;
        return 0;
    }

    /* Any CFTypeRef descendent is allowed as initializer too */
    if (PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, CFTypeRefObj_Convert, &itself))
    {
        ((CFDictionaryRefObject *)_self)->ob_itself = itself;
        return 0;
    }
    return -1;
}

#define CFDictionaryRefObj_tp_alloc PyType_GenericAlloc

static PyObject *CFDictionaryRefObj_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
    PyObject *self;
    if ((self = type->tp_alloc(type, 0)) == NULL) return NULL;
    ((CFDictionaryRefObject *)self)->ob_itself = NULL;
    ((CFDictionaryRefObject *)self)->ob_freeit = CFRelease;
    return self;
}

#define CFDictionaryRefObj_tp_free PyObject_Del


PyTypeObject CFDictionaryRef_Type = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "_CF.CFDictionaryRef", /*tp_name*/
    sizeof(CFDictionaryRefObject), /*tp_basicsize*/
    0, /*tp_itemsize*/
    /* methods */
    (destructor) CFDictionaryRefObj_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)0, /*tp_getattr*/
    (setattrfunc)0, /*tp_setattr*/
    (cmpfunc) CFDictionaryRefObj_compare, /*tp_compare*/
    (reprfunc) CFDictionaryRefObj_repr, /*tp_repr*/
    (PyNumberMethods *)0, /* tp_as_number */
    (PySequenceMethods *)0, /* tp_as_sequence */
    (PyMappingMethods *)0, /* tp_as_mapping */
    (hashfunc) CFDictionaryRefObj_hash, /*tp_hash*/
    0, /*tp_call*/
    0, /*tp_str*/
    PyObject_GenericGetAttr, /*tp_getattro*/
    PyObject_GenericSetAttr, /*tp_setattro */
    0, /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, /* tp_flags */
    0, /*tp_doc*/
    0, /*tp_traverse*/
    0, /*tp_clear*/
    0, /*tp_richcompare*/
    0, /*tp_weaklistoffset*/
    0, /*tp_iter*/
    0, /*tp_iternext*/
    CFDictionaryRefObj_methods, /* tp_methods */
    0, /*tp_members*/
    CFDictionaryRefObj_getsetlist, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    CFDictionaryRefObj_tp_init, /* tp_init */
    CFDictionaryRefObj_tp_alloc, /* tp_alloc */
    CFDictionaryRefObj_tp_new, /* tp_new */
    CFDictionaryRefObj_tp_free, /* tp_free */
};

/* ---------------- End object type CFDictionaryRef ----------------- */


/* --------------- Object type CFMutableDictionaryRef --------------- */

PyTypeObject CFMutableDictionaryRef_Type;

#define CFMutableDictionaryRefObj_Check(x) ((x)->ob_type == &CFMutableDictionaryRef_Type || PyObject_TypeCheck((x), &CFMutableDictionaryRef_Type))

typedef struct CFMutableDictionaryRefObject {
    PyObject_HEAD
    CFMutableDictionaryRef ob_itself;
    void (*ob_freeit)(CFTypeRef ptr);
} CFMutableDictionaryRefObject;

PyObject *CFMutableDictionaryRefObj_New(CFMutableDictionaryRef itself)
{
    CFMutableDictionaryRefObject *it;
    if (itself == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError, "cannot wrap NULL");
        return NULL;
    }
    it = PyObject_NEW(CFMutableDictionaryRefObject, &CFMutableDictionaryRef_Type);
    if (it == NULL) return NULL;
    /* XXXX Should we tp_init or tp_new our basetype? */
    it->ob_itself = itself;
    it->ob_freeit = CFRelease;
    return (PyObject *)it;
}

int CFMutableDictionaryRefObj_Convert(PyObject *v, CFMutableDictionaryRef *p_itself)
{

    if (v == Py_None) { *p_itself = NULL; return 1; }
    /* Check for other CF objects here */

    if (!CFMutableDictionaryRefObj_Check(v))
    {
        PyErr_SetString(PyExc_TypeError, "CFMutableDictionaryRef required");
        return 0;
    }
    *p_itself = ((CFMutableDictionaryRefObject *)v)->ob_itself;
    return 1;
}

static void CFMutableDictionaryRefObj_dealloc(CFMutableDictionaryRefObject *self)
{
    if (self->ob_freeit && self->ob_itself)
    {
        self->ob_freeit((CFTypeRef)self->ob_itself);
        self->ob_itself = NULL;
    }
    CFDictionaryRef_Type.tp_dealloc((PyObject *)self);
}

static PyObject *CFMutableDictionaryRefObj_CFDictionaryRemoveAllValues(CFMutableDictionaryRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
#ifndef CFDictionaryRemoveAllValues
    PyMac_PRECHECK(CFDictionaryRemoveAllValues);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    CFDictionaryRemoveAllValues(_self->ob_itself);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyMethodDef CFMutableDictionaryRefObj_methods[] = {
    {"CFDictionaryRemoveAllValues", (PyCFunction)CFMutableDictionaryRefObj_CFDictionaryRemoveAllValues, 1,
     PyDoc_STR("() -> None")},
    {NULL, NULL, 0}
};

#define CFMutableDictionaryRefObj_getsetlist NULL


static int CFMutableDictionaryRefObj_compare(CFMutableDictionaryRefObject *self, CFMutableDictionaryRefObject *other)
{
    /* XXXX Or should we use CFEqual?? */
    if ( self->ob_itself > other->ob_itself ) return 1;
    if ( self->ob_itself < other->ob_itself ) return -1;
    return 0;
}

static PyObject * CFMutableDictionaryRefObj_repr(CFMutableDictionaryRefObject *self)
{
    char buf[100];
    sprintf(buf, "<CFMutableDictionaryRef object at 0x%8.8x for 0x%8.8x>", (unsigned)self, (unsigned)self->ob_itself);
    return PyString_FromString(buf);
}

static int CFMutableDictionaryRefObj_hash(CFMutableDictionaryRefObject *self)
{
    /* XXXX Or should we use CFHash?? */
    return (int)self->ob_itself;
}
static int CFMutableDictionaryRefObj_tp_init(PyObject *_self, PyObject *_args, PyObject *_kwds)
{
    CFMutableDictionaryRef itself;
    char *kw[] = {"itself", 0};

    if (PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, CFMutableDictionaryRefObj_Convert, &itself))
    {
        ((CFMutableDictionaryRefObject *)_self)->ob_itself = itself;
        return 0;
    }

    /* Any CFTypeRef descendent is allowed as initializer too */
    if (PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, CFTypeRefObj_Convert, &itself))
    {
        ((CFMutableDictionaryRefObject *)_self)->ob_itself = itself;
        return 0;
    }
    return -1;
}

#define CFMutableDictionaryRefObj_tp_alloc PyType_GenericAlloc

static PyObject *CFMutableDictionaryRefObj_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
    PyObject *self;
    if ((self = type->tp_alloc(type, 0)) == NULL) return NULL;
    ((CFMutableDictionaryRefObject *)self)->ob_itself = NULL;
    ((CFMutableDictionaryRefObject *)self)->ob_freeit = CFRelease;
    return self;
}

#define CFMutableDictionaryRefObj_tp_free PyObject_Del


PyTypeObject CFMutableDictionaryRef_Type = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "_CF.CFMutableDictionaryRef", /*tp_name*/
    sizeof(CFMutableDictionaryRefObject), /*tp_basicsize*/
    0, /*tp_itemsize*/
    /* methods */
    (destructor) CFMutableDictionaryRefObj_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)0, /*tp_getattr*/
    (setattrfunc)0, /*tp_setattr*/
    (cmpfunc) CFMutableDictionaryRefObj_compare, /*tp_compare*/
    (reprfunc) CFMutableDictionaryRefObj_repr, /*tp_repr*/
    (PyNumberMethods *)0, /* tp_as_number */
    (PySequenceMethods *)0, /* tp_as_sequence */
    (PyMappingMethods *)0, /* tp_as_mapping */
    (hashfunc) CFMutableDictionaryRefObj_hash, /*tp_hash*/
    0, /*tp_call*/
    0, /*tp_str*/
    PyObject_GenericGetAttr, /*tp_getattro*/
    PyObject_GenericSetAttr, /*tp_setattro */
    0, /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, /* tp_flags */
    0, /*tp_doc*/
    0, /*tp_traverse*/
    0, /*tp_clear*/
    0, /*tp_richcompare*/
    0, /*tp_weaklistoffset*/
    0, /*tp_iter*/
    0, /*tp_iternext*/
    CFMutableDictionaryRefObj_methods, /* tp_methods */
    0, /*tp_members*/
    CFMutableDictionaryRefObj_getsetlist, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    CFMutableDictionaryRefObj_tp_init, /* tp_init */
    CFMutableDictionaryRefObj_tp_alloc, /* tp_alloc */
    CFMutableDictionaryRefObj_tp_new, /* tp_new */
    CFMutableDictionaryRefObj_tp_free, /* tp_free */
};

/* ------------- End object type CFMutableDictionaryRef ------------- */


/* --------------------- Object type CFDataRef ---------------------- */

PyTypeObject CFDataRef_Type;

#define CFDataRefObj_Check(x) ((x)->ob_type == &CFDataRef_Type || PyObject_TypeCheck((x), &CFDataRef_Type))

typedef struct CFDataRefObject {
    PyObject_HEAD
    CFDataRef ob_itself;
    void (*ob_freeit)(CFTypeRef ptr);
} CFDataRefObject;

PyObject *CFDataRefObj_New(CFDataRef itself)
{
    CFDataRefObject *it;
    if (itself == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError, "cannot wrap NULL");
        return NULL;
    }
    it = PyObject_NEW(CFDataRefObject, &CFDataRef_Type);
    if (it == NULL) return NULL;
    /* XXXX Should we tp_init or tp_new our basetype? */
    it->ob_itself = itself;
    it->ob_freeit = CFRelease;
    return (PyObject *)it;
}

int CFDataRefObj_Convert(PyObject *v, CFDataRef *p_itself)
{

    if (v == Py_None) { *p_itself = NULL; return 1; }
    if (PyString_Check(v)) {
        char *cStr;
        Py_ssize_t cLen;
        if( PyString_AsStringAndSize(v, &cStr, &cLen) < 0 ) return 0;
        *p_itself = CFDataCreate((CFAllocatorRef)NULL, (unsigned char *)cStr, cLen);
        return 1;
    }

    if (!CFDataRefObj_Check(v))
    {
        PyErr_SetString(PyExc_TypeError, "CFDataRef required");
        return 0;
    }
    *p_itself = ((CFDataRefObject *)v)->ob_itself;
    return 1;
}

static void CFDataRefObj_dealloc(CFDataRefObject *self)
{
    if (self->ob_freeit && self->ob_itself)
    {
        self->ob_freeit((CFTypeRef)self->ob_itself);
        self->ob_itself = NULL;
    }
    CFTypeRef_Type.tp_dealloc((PyObject *)self);
}

static PyObject *CFDataRefObj_CFDataCreateCopy(CFDataRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFDataRef _rv;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFDataCreateCopy((CFAllocatorRef)NULL,
                           _self->ob_itself);
    _res = Py_BuildValue("O&",
                         CFDataRefObj_New, _rv);
    return _res;
}

static PyObject *CFDataRefObj_CFDataGetLength(CFDataRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFIndex _rv;
#ifndef CFDataGetLength
    PyMac_PRECHECK(CFDataGetLength);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFDataGetLength(_self->ob_itself);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CFDataRefObj_CFStringCreateFromExternalRepresentation(CFDataRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
    CFStringEncoding encoding;
    if (!PyArg_ParseTuple(_args, "l",
                          &encoding))
        return NULL;
    _rv = CFStringCreateFromExternalRepresentation((CFAllocatorRef)NULL,
                                                   _self->ob_itself,
                                                   encoding);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CFDataRefObj_CFDataGetData(CFDataRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;

    int size = CFDataGetLength(_self->ob_itself);
    char *data = (char *)CFDataGetBytePtr(_self->ob_itself);

    _res = (PyObject *)PyString_FromStringAndSize(data, size);
    return _res;

}

static PyMethodDef CFDataRefObj_methods[] = {
    {"CFDataCreateCopy", (PyCFunction)CFDataRefObj_CFDataCreateCopy, 1,
     PyDoc_STR("() -> (CFDataRef _rv)")},
    {"CFDataGetLength", (PyCFunction)CFDataRefObj_CFDataGetLength, 1,
     PyDoc_STR("() -> (CFIndex _rv)")},
    {"CFStringCreateFromExternalRepresentation", (PyCFunction)CFDataRefObj_CFStringCreateFromExternalRepresentation, 1,
     PyDoc_STR("(CFStringEncoding encoding) -> (CFStringRef _rv)")},
    {"CFDataGetData", (PyCFunction)CFDataRefObj_CFDataGetData, 1,
     PyDoc_STR("() -> (string _rv)")},
    {NULL, NULL, 0}
};

#define CFDataRefObj_getsetlist NULL


static int CFDataRefObj_compare(CFDataRefObject *self, CFDataRefObject *other)
{
    /* XXXX Or should we use CFEqual?? */
    if ( self->ob_itself > other->ob_itself ) return 1;
    if ( self->ob_itself < other->ob_itself ) return -1;
    return 0;
}

static PyObject * CFDataRefObj_repr(CFDataRefObject *self)
{
    char buf[100];
    sprintf(buf, "<CFDataRef object at 0x%8.8x for 0x%8.8x>", (unsigned)self, (unsigned)self->ob_itself);
    return PyString_FromString(buf);
}

static int CFDataRefObj_hash(CFDataRefObject *self)
{
    /* XXXX Or should we use CFHash?? */
    return (int)self->ob_itself;
}
static int CFDataRefObj_tp_init(PyObject *_self, PyObject *_args, PyObject *_kwds)
{
    CFDataRef itself;
    char *kw[] = {"itself", 0};

    if (PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, CFDataRefObj_Convert, &itself))
    {
        ((CFDataRefObject *)_self)->ob_itself = itself;
        return 0;
    }

    /* Any CFTypeRef descendent is allowed as initializer too */
    if (PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, CFTypeRefObj_Convert, &itself))
    {
        ((CFDataRefObject *)_self)->ob_itself = itself;
        return 0;
    }
    return -1;
}

#define CFDataRefObj_tp_alloc PyType_GenericAlloc

static PyObject *CFDataRefObj_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
    PyObject *self;
    if ((self = type->tp_alloc(type, 0)) == NULL) return NULL;
    ((CFDataRefObject *)self)->ob_itself = NULL;
    ((CFDataRefObject *)self)->ob_freeit = CFRelease;
    return self;
}

#define CFDataRefObj_tp_free PyObject_Del


PyTypeObject CFDataRef_Type = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "_CF.CFDataRef", /*tp_name*/
    sizeof(CFDataRefObject), /*tp_basicsize*/
    0, /*tp_itemsize*/
    /* methods */
    (destructor) CFDataRefObj_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)0, /*tp_getattr*/
    (setattrfunc)0, /*tp_setattr*/
    (cmpfunc) CFDataRefObj_compare, /*tp_compare*/
    (reprfunc) CFDataRefObj_repr, /*tp_repr*/
    (PyNumberMethods *)0, /* tp_as_number */
    (PySequenceMethods *)0, /* tp_as_sequence */
    (PyMappingMethods *)0, /* tp_as_mapping */
    (hashfunc) CFDataRefObj_hash, /*tp_hash*/
    0, /*tp_call*/
    0, /*tp_str*/
    PyObject_GenericGetAttr, /*tp_getattro*/
    PyObject_GenericSetAttr, /*tp_setattro */
    0, /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, /* tp_flags */
    0, /*tp_doc*/
    0, /*tp_traverse*/
    0, /*tp_clear*/
    0, /*tp_richcompare*/
    0, /*tp_weaklistoffset*/
    0, /*tp_iter*/
    0, /*tp_iternext*/
    CFDataRefObj_methods, /* tp_methods */
    0, /*tp_members*/
    CFDataRefObj_getsetlist, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    CFDataRefObj_tp_init, /* tp_init */
    CFDataRefObj_tp_alloc, /* tp_alloc */
    CFDataRefObj_tp_new, /* tp_new */
    CFDataRefObj_tp_free, /* tp_free */
};

/* ------------------- End object type CFDataRef -------------------- */


/* ------------------ Object type CFMutableDataRef ------------------ */

PyTypeObject CFMutableDataRef_Type;

#define CFMutableDataRefObj_Check(x) ((x)->ob_type == &CFMutableDataRef_Type || PyObject_TypeCheck((x), &CFMutableDataRef_Type))

typedef struct CFMutableDataRefObject {
    PyObject_HEAD
    CFMutableDataRef ob_itself;
    void (*ob_freeit)(CFTypeRef ptr);
} CFMutableDataRefObject;

PyObject *CFMutableDataRefObj_New(CFMutableDataRef itself)
{
    CFMutableDataRefObject *it;
    if (itself == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError, "cannot wrap NULL");
        return NULL;
    }
    it = PyObject_NEW(CFMutableDataRefObject, &CFMutableDataRef_Type);
    if (it == NULL) return NULL;
    /* XXXX Should we tp_init or tp_new our basetype? */
    it->ob_itself = itself;
    it->ob_freeit = CFRelease;
    return (PyObject *)it;
}

int CFMutableDataRefObj_Convert(PyObject *v, CFMutableDataRef *p_itself)
{

    if (v == Py_None) { *p_itself = NULL; return 1; }
    /* Check for other CF objects here */

    if (!CFMutableDataRefObj_Check(v))
    {
        PyErr_SetString(PyExc_TypeError, "CFMutableDataRef required");
        return 0;
    }
    *p_itself = ((CFMutableDataRefObject *)v)->ob_itself;
    return 1;
}

static void CFMutableDataRefObj_dealloc(CFMutableDataRefObject *self)
{
    if (self->ob_freeit && self->ob_itself)
    {
        self->ob_freeit((CFTypeRef)self->ob_itself);
        self->ob_itself = NULL;
    }
    CFDataRef_Type.tp_dealloc((PyObject *)self);
}

static PyObject *CFMutableDataRefObj_CFDataSetLength(CFMutableDataRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFIndex length;
#ifndef CFDataSetLength
    PyMac_PRECHECK(CFDataSetLength);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &length))
        return NULL;
    CFDataSetLength(_self->ob_itself,
                    length);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CFMutableDataRefObj_CFDataIncreaseLength(CFMutableDataRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFIndex extraLength;
#ifndef CFDataIncreaseLength
    PyMac_PRECHECK(CFDataIncreaseLength);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &extraLength))
        return NULL;
    CFDataIncreaseLength(_self->ob_itself,
                         extraLength);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CFMutableDataRefObj_CFDataAppendBytes(CFMutableDataRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    unsigned char *bytes__in__;
    long bytes__len__;
    int bytes__in_len__;
#ifndef CFDataAppendBytes
    PyMac_PRECHECK(CFDataAppendBytes);
#endif
    if (!PyArg_ParseTuple(_args, "s#",
                          &bytes__in__, &bytes__in_len__))
        return NULL;
    bytes__len__ = bytes__in_len__;
    CFDataAppendBytes(_self->ob_itself,
                      bytes__in__, bytes__len__);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CFMutableDataRefObj_CFDataReplaceBytes(CFMutableDataRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFRange range;
    unsigned char *newBytes__in__;
    long newBytes__len__;
    int newBytes__in_len__;
#ifndef CFDataReplaceBytes
    PyMac_PRECHECK(CFDataReplaceBytes);
#endif
    if (!PyArg_ParseTuple(_args, "O&s#",
                          CFRange_Convert, &range,
                          &newBytes__in__, &newBytes__in_len__))
        return NULL;
    newBytes__len__ = newBytes__in_len__;
    CFDataReplaceBytes(_self->ob_itself,
                       range,
                       newBytes__in__, newBytes__len__);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CFMutableDataRefObj_CFDataDeleteBytes(CFMutableDataRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFRange range;
#ifndef CFDataDeleteBytes
    PyMac_PRECHECK(CFDataDeleteBytes);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          CFRange_Convert, &range))
        return NULL;
    CFDataDeleteBytes(_self->ob_itself,
                      range);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyMethodDef CFMutableDataRefObj_methods[] = {
    {"CFDataSetLength", (PyCFunction)CFMutableDataRefObj_CFDataSetLength, 1,
     PyDoc_STR("(CFIndex length) -> None")},
    {"CFDataIncreaseLength", (PyCFunction)CFMutableDataRefObj_CFDataIncreaseLength, 1,
     PyDoc_STR("(CFIndex extraLength) -> None")},
    {"CFDataAppendBytes", (PyCFunction)CFMutableDataRefObj_CFDataAppendBytes, 1,
     PyDoc_STR("(Buffer bytes) -> None")},
    {"CFDataReplaceBytes", (PyCFunction)CFMutableDataRefObj_CFDataReplaceBytes, 1,
     PyDoc_STR("(CFRange range, Buffer newBytes) -> None")},
    {"CFDataDeleteBytes", (PyCFunction)CFMutableDataRefObj_CFDataDeleteBytes, 1,
     PyDoc_STR("(CFRange range) -> None")},
    {NULL, NULL, 0}
};

#define CFMutableDataRefObj_getsetlist NULL


static int CFMutableDataRefObj_compare(CFMutableDataRefObject *self, CFMutableDataRefObject *other)
{
    /* XXXX Or should we use CFEqual?? */
    if ( self->ob_itself > other->ob_itself ) return 1;
    if ( self->ob_itself < other->ob_itself ) return -1;
    return 0;
}

static PyObject * CFMutableDataRefObj_repr(CFMutableDataRefObject *self)
{
    char buf[100];
    sprintf(buf, "<CFMutableDataRef object at 0x%8.8x for 0x%8.8x>", (unsigned)self, (unsigned)self->ob_itself);
    return PyString_FromString(buf);
}

static int CFMutableDataRefObj_hash(CFMutableDataRefObject *self)
{
    /* XXXX Or should we use CFHash?? */
    return (int)self->ob_itself;
}
static int CFMutableDataRefObj_tp_init(PyObject *_self, PyObject *_args, PyObject *_kwds)
{
    CFMutableDataRef itself;
    char *kw[] = {"itself", 0};

    if (PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, CFMutableDataRefObj_Convert, &itself))
    {
        ((CFMutableDataRefObject *)_self)->ob_itself = itself;
        return 0;
    }

    /* Any CFTypeRef descendent is allowed as initializer too */
    if (PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, CFTypeRefObj_Convert, &itself))
    {
        ((CFMutableDataRefObject *)_self)->ob_itself = itself;
        return 0;
    }
    return -1;
}

#define CFMutableDataRefObj_tp_alloc PyType_GenericAlloc

static PyObject *CFMutableDataRefObj_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
    PyObject *self;
    if ((self = type->tp_alloc(type, 0)) == NULL) return NULL;
    ((CFMutableDataRefObject *)self)->ob_itself = NULL;
    ((CFMutableDataRefObject *)self)->ob_freeit = CFRelease;
    return self;
}

#define CFMutableDataRefObj_tp_free PyObject_Del


PyTypeObject CFMutableDataRef_Type = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "_CF.CFMutableDataRef", /*tp_name*/
    sizeof(CFMutableDataRefObject), /*tp_basicsize*/
    0, /*tp_itemsize*/
    /* methods */
    (destructor) CFMutableDataRefObj_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)0, /*tp_getattr*/
    (setattrfunc)0, /*tp_setattr*/
    (cmpfunc) CFMutableDataRefObj_compare, /*tp_compare*/
    (reprfunc) CFMutableDataRefObj_repr, /*tp_repr*/
    (PyNumberMethods *)0, /* tp_as_number */
    (PySequenceMethods *)0, /* tp_as_sequence */
    (PyMappingMethods *)0, /* tp_as_mapping */
    (hashfunc) CFMutableDataRefObj_hash, /*tp_hash*/
    0, /*tp_call*/
    0, /*tp_str*/
    PyObject_GenericGetAttr, /*tp_getattro*/
    PyObject_GenericSetAttr, /*tp_setattro */
    0, /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, /* tp_flags */
    0, /*tp_doc*/
    0, /*tp_traverse*/
    0, /*tp_clear*/
    0, /*tp_richcompare*/
    0, /*tp_weaklistoffset*/
    0, /*tp_iter*/
    0, /*tp_iternext*/
    CFMutableDataRefObj_methods, /* tp_methods */
    0, /*tp_members*/
    CFMutableDataRefObj_getsetlist, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    CFMutableDataRefObj_tp_init, /* tp_init */
    CFMutableDataRefObj_tp_alloc, /* tp_alloc */
    CFMutableDataRefObj_tp_new, /* tp_new */
    CFMutableDataRefObj_tp_free, /* tp_free */
};

/* ---------------- End object type CFMutableDataRef ---------------- */


/* -------------------- Object type CFStringRef --------------------- */

PyTypeObject CFStringRef_Type;

#define CFStringRefObj_Check(x) ((x)->ob_type == &CFStringRef_Type || PyObject_TypeCheck((x), &CFStringRef_Type))

typedef struct CFStringRefObject {
    PyObject_HEAD
    CFStringRef ob_itself;
    void (*ob_freeit)(CFTypeRef ptr);
} CFStringRefObject;

PyObject *CFStringRefObj_New(CFStringRef itself)
{
    CFStringRefObject *it;
    if (itself == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError, "cannot wrap NULL");
        return NULL;
    }
    it = PyObject_NEW(CFStringRefObject, &CFStringRef_Type);
    if (it == NULL) return NULL;
    /* XXXX Should we tp_init or tp_new our basetype? */
    it->ob_itself = itself;
    it->ob_freeit = CFRelease;
    return (PyObject *)it;
}

int CFStringRefObj_Convert(PyObject *v, CFStringRef *p_itself)
{

    if (v == Py_None) { *p_itself = NULL; return 1; }
    if (PyString_Check(v)) {
        char *cStr;
        if (!PyArg_Parse(v, "es", "ascii", &cStr))
        return 0;
        *p_itself = CFStringCreateWithCString((CFAllocatorRef)NULL, cStr, kCFStringEncodingASCII);
        PyMem_Free(cStr);
        return 1;
    }
    if (PyUnicode_Check(v)) {
        /* We use the CF types here, if Python was configured differently that will give an error */
        CFIndex size = PyUnicode_GetSize(v);
        UniChar *unichars = PyUnicode_AsUnicode(v);
        if (!unichars) return 0;
        *p_itself = CFStringCreateWithCharacters((CFAllocatorRef)NULL, unichars, size);
        return 1;
    }


    if (!CFStringRefObj_Check(v))
    {
        PyErr_SetString(PyExc_TypeError, "CFStringRef required");
        return 0;
    }
    *p_itself = ((CFStringRefObject *)v)->ob_itself;
    return 1;
}

static void CFStringRefObj_dealloc(CFStringRefObject *self)
{
    if (self->ob_freeit && self->ob_itself)
    {
        self->ob_freeit((CFTypeRef)self->ob_itself);
        self->ob_itself = NULL;
    }
    CFTypeRef_Type.tp_dealloc((PyObject *)self);
}

static PyObject *CFStringRefObj_CFStringCreateWithSubstring(CFStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
    CFRange range;
    if (!PyArg_ParseTuple(_args, "O&",
                          CFRange_Convert, &range))
        return NULL;
    _rv = CFStringCreateWithSubstring((CFAllocatorRef)NULL,
                                      _self->ob_itself,
                                      range);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CFStringRefObj_CFStringCreateCopy(CFStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFStringCreateCopy((CFAllocatorRef)NULL,
                             _self->ob_itself);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CFStringRefObj_CFStringGetLength(CFStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFIndex _rv;
#ifndef CFStringGetLength
    PyMac_PRECHECK(CFStringGetLength);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFStringGetLength(_self->ob_itself);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CFStringRefObj_CFStringGetBytes(CFStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFIndex _rv;
    CFRange range;
    CFStringEncoding encoding;
    UInt8 lossByte;
    Boolean isExternalRepresentation;
    UInt8 buffer;
    CFIndex maxBufLen;
    CFIndex usedBufLen;
#ifndef CFStringGetBytes
    PyMac_PRECHECK(CFStringGetBytes);
#endif
    if (!PyArg_ParseTuple(_args, "O&lbll",
                          CFRange_Convert, &range,
                          &encoding,
                          &lossByte,
                          &isExternalRepresentation,
                          &maxBufLen))
        return NULL;
    _rv = CFStringGetBytes(_self->ob_itself,
                           range,
                           encoding,
                           lossByte,
                           isExternalRepresentation,
                           &buffer,
                           maxBufLen,
                           &usedBufLen);
    _res = Py_BuildValue("lbl",
                         _rv,
                         buffer,
                         usedBufLen);
    return _res;
}

static PyObject *CFStringRefObj_CFStringCreateExternalRepresentation(CFStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFDataRef _rv;
    CFStringEncoding encoding;
    UInt8 lossByte;
    if (!PyArg_ParseTuple(_args, "lb",
                          &encoding,
                          &lossByte))
        return NULL;
    _rv = CFStringCreateExternalRepresentation((CFAllocatorRef)NULL,
                                               _self->ob_itself,
                                               encoding,
                                               lossByte);
    _res = Py_BuildValue("O&",
                         CFDataRefObj_New, _rv);
    return _res;
}

static PyObject *CFStringRefObj_CFStringGetSmallestEncoding(CFStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringEncoding _rv;
#ifndef CFStringGetSmallestEncoding
    PyMac_PRECHECK(CFStringGetSmallestEncoding);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFStringGetSmallestEncoding(_self->ob_itself);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CFStringRefObj_CFStringGetFastestEncoding(CFStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringEncoding _rv;
#ifndef CFStringGetFastestEncoding
    PyMac_PRECHECK(CFStringGetFastestEncoding);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFStringGetFastestEncoding(_self->ob_itself);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CFStringRefObj_CFStringCompareWithOptions(CFStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFComparisonResult _rv;
    CFStringRef theString2;
    CFRange rangeToCompare;
    CFOptionFlags compareOptions;
#ifndef CFStringCompareWithOptions
    PyMac_PRECHECK(CFStringCompareWithOptions);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&l",
                          CFStringRefObj_Convert, &theString2,
                          CFRange_Convert, &rangeToCompare,
                          &compareOptions))
        return NULL;
    _rv = CFStringCompareWithOptions(_self->ob_itself,
                                     theString2,
                                     rangeToCompare,
                                     compareOptions);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CFStringRefObj_CFStringCompare(CFStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFComparisonResult _rv;
    CFStringRef theString2;
    CFOptionFlags compareOptions;
#ifndef CFStringCompare
    PyMac_PRECHECK(CFStringCompare);
#endif
    if (!PyArg_ParseTuple(_args, "O&l",
                          CFStringRefObj_Convert, &theString2,
                          &compareOptions))
        return NULL;
    _rv = CFStringCompare(_self->ob_itself,
                          theString2,
                          compareOptions);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CFStringRefObj_CFStringFindWithOptions(CFStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
    CFStringRef stringToFind;
    CFRange rangeToSearch;
    CFOptionFlags searchOptions;
    CFRange result;
#ifndef CFStringFindWithOptions
    PyMac_PRECHECK(CFStringFindWithOptions);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&l",
                          CFStringRefObj_Convert, &stringToFind,
                          CFRange_Convert, &rangeToSearch,
                          &searchOptions))
        return NULL;
    _rv = CFStringFindWithOptions(_self->ob_itself,
                                  stringToFind,
                                  rangeToSearch,
                                  searchOptions,
                                  &result);
    _res = Py_BuildValue("lO&",
                         _rv,
                         CFRange_New, result);
    return _res;
}

static PyObject *CFStringRefObj_CFStringCreateArrayWithFindResults(CFStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFArrayRef _rv;
    CFStringRef stringToFind;
    CFRange rangeToSearch;
    CFOptionFlags compareOptions;
    if (!PyArg_ParseTuple(_args, "O&O&l",
                          CFStringRefObj_Convert, &stringToFind,
                          CFRange_Convert, &rangeToSearch,
                          &compareOptions))
        return NULL;
    _rv = CFStringCreateArrayWithFindResults((CFAllocatorRef)NULL,
                                             _self->ob_itself,
                                             stringToFind,
                                             rangeToSearch,
                                             compareOptions);
    _res = Py_BuildValue("O&",
                         CFArrayRefObj_New, _rv);
    return _res;
}

static PyObject *CFStringRefObj_CFStringFind(CFStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFRange _rv;
    CFStringRef stringToFind;
    CFOptionFlags compareOptions;
#ifndef CFStringFind
    PyMac_PRECHECK(CFStringFind);
#endif
    if (!PyArg_ParseTuple(_args, "O&l",
                          CFStringRefObj_Convert, &stringToFind,
                          &compareOptions))
        return NULL;
    _rv = CFStringFind(_self->ob_itself,
                       stringToFind,
                       compareOptions);
    _res = Py_BuildValue("O&",
                         CFRange_New, _rv);
    return _res;
}

static PyObject *CFStringRefObj_CFStringHasPrefix(CFStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
    CFStringRef prefix;
#ifndef CFStringHasPrefix
    PyMac_PRECHECK(CFStringHasPrefix);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          CFStringRefObj_Convert, &prefix))
        return NULL;
    _rv = CFStringHasPrefix(_self->ob_itself,
                            prefix);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CFStringRefObj_CFStringHasSuffix(CFStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
    CFStringRef suffix;
#ifndef CFStringHasSuffix
    PyMac_PRECHECK(CFStringHasSuffix);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          CFStringRefObj_Convert, &suffix))
        return NULL;
    _rv = CFStringHasSuffix(_self->ob_itself,
                            suffix);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CFStringRefObj_CFStringGetLineBounds(CFStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFRange range;
    CFIndex lineBeginIndex;
    CFIndex lineEndIndex;
    CFIndex contentsEndIndex;
#ifndef CFStringGetLineBounds
    PyMac_PRECHECK(CFStringGetLineBounds);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          CFRange_Convert, &range))
        return NULL;
    CFStringGetLineBounds(_self->ob_itself,
                          range,
                          &lineBeginIndex,
                          &lineEndIndex,
                          &contentsEndIndex);
    _res = Py_BuildValue("lll",
                         lineBeginIndex,
                         lineEndIndex,
                         contentsEndIndex);
    return _res;
}

static PyObject *CFStringRefObj_CFStringCreateArrayBySeparatingStrings(CFStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFArrayRef _rv;
    CFStringRef separatorString;
    if (!PyArg_ParseTuple(_args, "O&",
                          CFStringRefObj_Convert, &separatorString))
        return NULL;
    _rv = CFStringCreateArrayBySeparatingStrings((CFAllocatorRef)NULL,
                                                 _self->ob_itself,
                                                 separatorString);
    _res = Py_BuildValue("O&",
                         CFArrayRefObj_New, _rv);
    return _res;
}

static PyObject *CFStringRefObj_CFStringGetIntValue(CFStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    SInt32 _rv;
#ifndef CFStringGetIntValue
    PyMac_PRECHECK(CFStringGetIntValue);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFStringGetIntValue(_self->ob_itself);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CFStringRefObj_CFStringGetDoubleValue(CFStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    double _rv;
#ifndef CFStringGetDoubleValue
    PyMac_PRECHECK(CFStringGetDoubleValue);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFStringGetDoubleValue(_self->ob_itself);
    _res = Py_BuildValue("d",
                         _rv);
    return _res;
}

static PyObject *CFStringRefObj_CFStringConvertIANACharSetNameToEncoding(CFStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringEncoding _rv;
#ifndef CFStringConvertIANACharSetNameToEncoding
    PyMac_PRECHECK(CFStringConvertIANACharSetNameToEncoding);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFStringConvertIANACharSetNameToEncoding(_self->ob_itself);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CFStringRefObj_CFShowStr(CFStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
#ifndef CFShowStr
    PyMac_PRECHECK(CFShowStr);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    CFShowStr(_self->ob_itself);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CFStringRefObj_CFURLCreateWithString(CFStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFURLRef _rv;
    CFURLRef baseURL;
    if (!PyArg_ParseTuple(_args, "O&",
                          OptionalCFURLRefObj_Convert, &baseURL))
        return NULL;
    _rv = CFURLCreateWithString((CFAllocatorRef)NULL,
                                _self->ob_itself,
                                baseURL);
    _res = Py_BuildValue("O&",
                         CFURLRefObj_New, _rv);
    return _res;
}

static PyObject *CFStringRefObj_CFURLCreateWithFileSystemPath(CFStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFURLRef _rv;
    CFURLPathStyle pathStyle;
    Boolean isDirectory;
    if (!PyArg_ParseTuple(_args, "ll",
                          &pathStyle,
                          &isDirectory))
        return NULL;
    _rv = CFURLCreateWithFileSystemPath((CFAllocatorRef)NULL,
                                        _self->ob_itself,
                                        pathStyle,
                                        isDirectory);
    _res = Py_BuildValue("O&",
                         CFURLRefObj_New, _rv);
    return _res;
}

static PyObject *CFStringRefObj_CFURLCreateWithFileSystemPathRelativeToBase(CFStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFURLRef _rv;
    CFURLPathStyle pathStyle;
    Boolean isDirectory;
    CFURLRef baseURL;
    if (!PyArg_ParseTuple(_args, "llO&",
                          &pathStyle,
                          &isDirectory,
                          OptionalCFURLRefObj_Convert, &baseURL))
        return NULL;
    _rv = CFURLCreateWithFileSystemPathRelativeToBase((CFAllocatorRef)NULL,
                                                      _self->ob_itself,
                                                      pathStyle,
                                                      isDirectory,
                                                      baseURL);
    _res = Py_BuildValue("O&",
                         CFURLRefObj_New, _rv);
    return _res;
}

static PyObject *CFStringRefObj_CFURLCreateStringByReplacingPercentEscapes(CFStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
    CFStringRef charactersToLeaveEscaped;
    if (!PyArg_ParseTuple(_args, "O&",
                          CFStringRefObj_Convert, &charactersToLeaveEscaped))
        return NULL;
    _rv = CFURLCreateStringByReplacingPercentEscapes((CFAllocatorRef)NULL,
                                                     _self->ob_itself,
                                                     charactersToLeaveEscaped);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CFStringRefObj_CFURLCreateStringByAddingPercentEscapes(CFStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
    CFStringRef charactersToLeaveUnescaped;
    CFStringRef legalURLCharactersToBeEscaped;
    CFStringEncoding encoding;
    if (!PyArg_ParseTuple(_args, "O&O&l",
                          CFStringRefObj_Convert, &charactersToLeaveUnescaped,
                          CFStringRefObj_Convert, &legalURLCharactersToBeEscaped,
                          &encoding))
        return NULL;
    _rv = CFURLCreateStringByAddingPercentEscapes((CFAllocatorRef)NULL,
                                                  _self->ob_itself,
                                                  charactersToLeaveUnescaped,
                                                  legalURLCharactersToBeEscaped,
                                                  encoding);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CFStringRefObj_CFStringGetString(CFStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;

    int size = CFStringGetLength(_self->ob_itself)+1;
    char *data = malloc(size);

    if( data == NULL ) return PyErr_NoMemory();
    if ( CFStringGetCString(_self->ob_itself, data, size, 0) ) {
        _res = (PyObject *)PyString_FromString(data);
    } else {
        PyErr_SetString(PyExc_RuntimeError, "CFStringGetCString could not fit the string");
        _res = NULL;
    }
    free(data);
    return _res;

}

static PyObject *CFStringRefObj_CFStringGetUnicode(CFStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;

    int size = CFStringGetLength(_self->ob_itself)+1;
    Py_UNICODE *data = malloc(size*sizeof(Py_UNICODE));
    CFRange range;

    range.location = 0;
    range.length = size;
    if( data == NULL ) return PyErr_NoMemory();
    CFStringGetCharacters(_self->ob_itself, range, data);
    _res = (PyObject *)PyUnicode_FromUnicode(data, size-1);
    free(data);
    return _res;

}

static PyMethodDef CFStringRefObj_methods[] = {
    {"CFStringCreateWithSubstring", (PyCFunction)CFStringRefObj_CFStringCreateWithSubstring, 1,
     PyDoc_STR("(CFRange range) -> (CFStringRef _rv)")},
    {"CFStringCreateCopy", (PyCFunction)CFStringRefObj_CFStringCreateCopy, 1,
     PyDoc_STR("() -> (CFStringRef _rv)")},
    {"CFStringGetLength", (PyCFunction)CFStringRefObj_CFStringGetLength, 1,
     PyDoc_STR("() -> (CFIndex _rv)")},
    {"CFStringGetBytes", (PyCFunction)CFStringRefObj_CFStringGetBytes, 1,
     PyDoc_STR("(CFRange range, CFStringEncoding encoding, UInt8 lossByte, Boolean isExternalRepresentation, CFIndex maxBufLen) -> (CFIndex _rv, UInt8 buffer, CFIndex usedBufLen)")},
    {"CFStringCreateExternalRepresentation", (PyCFunction)CFStringRefObj_CFStringCreateExternalRepresentation, 1,
     PyDoc_STR("(CFStringEncoding encoding, UInt8 lossByte) -> (CFDataRef _rv)")},
    {"CFStringGetSmallestEncoding", (PyCFunction)CFStringRefObj_CFStringGetSmallestEncoding, 1,
     PyDoc_STR("() -> (CFStringEncoding _rv)")},
    {"CFStringGetFastestEncoding", (PyCFunction)CFStringRefObj_CFStringGetFastestEncoding, 1,
     PyDoc_STR("() -> (CFStringEncoding _rv)")},
    {"CFStringCompareWithOptions", (PyCFunction)CFStringRefObj_CFStringCompareWithOptions, 1,
     PyDoc_STR("(CFStringRef theString2, CFRange rangeToCompare, CFOptionFlags compareOptions) -> (CFComparisonResult _rv)")},
    {"CFStringCompare", (PyCFunction)CFStringRefObj_CFStringCompare, 1,
     PyDoc_STR("(CFStringRef theString2, CFOptionFlags compareOptions) -> (CFComparisonResult _rv)")},
    {"CFStringFindWithOptions", (PyCFunction)CFStringRefObj_CFStringFindWithOptions, 1,
     PyDoc_STR("(CFStringRef stringToFind, CFRange rangeToSearch, CFOptionFlags searchOptions) -> (Boolean _rv, CFRange result)")},
    {"CFStringCreateArrayWithFindResults", (PyCFunction)CFStringRefObj_CFStringCreateArrayWithFindResults, 1,
     PyDoc_STR("(CFStringRef stringToFind, CFRange rangeToSearch, CFOptionFlags compareOptions) -> (CFArrayRef _rv)")},
    {"CFStringFind", (PyCFunction)CFStringRefObj_CFStringFind, 1,
     PyDoc_STR("(CFStringRef stringToFind, CFOptionFlags compareOptions) -> (CFRange _rv)")},
    {"CFStringHasPrefix", (PyCFunction)CFStringRefObj_CFStringHasPrefix, 1,
     PyDoc_STR("(CFStringRef prefix) -> (Boolean _rv)")},
    {"CFStringHasSuffix", (PyCFunction)CFStringRefObj_CFStringHasSuffix, 1,
     PyDoc_STR("(CFStringRef suffix) -> (Boolean _rv)")},
    {"CFStringGetLineBounds", (PyCFunction)CFStringRefObj_CFStringGetLineBounds, 1,
     PyDoc_STR("(CFRange range) -> (CFIndex lineBeginIndex, CFIndex lineEndIndex, CFIndex contentsEndIndex)")},
    {"CFStringCreateArrayBySeparatingStrings", (PyCFunction)CFStringRefObj_CFStringCreateArrayBySeparatingStrings, 1,
     PyDoc_STR("(CFStringRef separatorString) -> (CFArrayRef _rv)")},
    {"CFStringGetIntValue", (PyCFunction)CFStringRefObj_CFStringGetIntValue, 1,
     PyDoc_STR("() -> (SInt32 _rv)")},
    {"CFStringGetDoubleValue", (PyCFunction)CFStringRefObj_CFStringGetDoubleValue, 1,
     PyDoc_STR("() -> (double _rv)")},
    {"CFStringConvertIANACharSetNameToEncoding", (PyCFunction)CFStringRefObj_CFStringConvertIANACharSetNameToEncoding, 1,
     PyDoc_STR("() -> (CFStringEncoding _rv)")},
    {"CFShowStr", (PyCFunction)CFStringRefObj_CFShowStr, 1,
     PyDoc_STR("() -> None")},
    {"CFURLCreateWithString", (PyCFunction)CFStringRefObj_CFURLCreateWithString, 1,
     PyDoc_STR("(CFURLRef baseURL) -> (CFURLRef _rv)")},
    {"CFURLCreateWithFileSystemPath", (PyCFunction)CFStringRefObj_CFURLCreateWithFileSystemPath, 1,
     PyDoc_STR("(CFURLPathStyle pathStyle, Boolean isDirectory) -> (CFURLRef _rv)")},
    {"CFURLCreateWithFileSystemPathRelativeToBase", (PyCFunction)CFStringRefObj_CFURLCreateWithFileSystemPathRelativeToBase, 1,
     PyDoc_STR("(CFURLPathStyle pathStyle, Boolean isDirectory, CFURLRef baseURL) -> (CFURLRef _rv)")},
    {"CFURLCreateStringByReplacingPercentEscapes", (PyCFunction)CFStringRefObj_CFURLCreateStringByReplacingPercentEscapes, 1,
     PyDoc_STR("(CFStringRef charactersToLeaveEscaped) -> (CFStringRef _rv)")},
    {"CFURLCreateStringByAddingPercentEscapes", (PyCFunction)CFStringRefObj_CFURLCreateStringByAddingPercentEscapes, 1,
     PyDoc_STR("(CFStringRef charactersToLeaveUnescaped, CFStringRef legalURLCharactersToBeEscaped, CFStringEncoding encoding) -> (CFStringRef _rv)")},
    {"CFStringGetString", (PyCFunction)CFStringRefObj_CFStringGetString, 1,
     PyDoc_STR("() -> (string _rv)")},
    {"CFStringGetUnicode", (PyCFunction)CFStringRefObj_CFStringGetUnicode, 1,
     PyDoc_STR("() -> (unicode _rv)")},
    {NULL, NULL, 0}
};

#define CFStringRefObj_getsetlist NULL


static int CFStringRefObj_compare(CFStringRefObject *self, CFStringRefObject *other)
{
    /* XXXX Or should we use CFEqual?? */
    if ( self->ob_itself > other->ob_itself ) return 1;
    if ( self->ob_itself < other->ob_itself ) return -1;
    return 0;
}

static PyObject * CFStringRefObj_repr(CFStringRefObject *self)
{
    char buf[100];
    sprintf(buf, "<CFStringRef object at 0x%8.8x for 0x%8.8x>", (unsigned)self, (unsigned)self->ob_itself);
    return PyString_FromString(buf);
}

static int CFStringRefObj_hash(CFStringRefObject *self)
{
    /* XXXX Or should we use CFHash?? */
    return (int)self->ob_itself;
}
static int CFStringRefObj_tp_init(PyObject *_self, PyObject *_args, PyObject *_kwds)
{
    CFStringRef itself;
    char *kw[] = {"itself", 0};

    if (PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, CFStringRefObj_Convert, &itself))
    {
        ((CFStringRefObject *)_self)->ob_itself = itself;
        return 0;
    }

    /* Any CFTypeRef descendent is allowed as initializer too */
    if (PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, CFTypeRefObj_Convert, &itself))
    {
        ((CFStringRefObject *)_self)->ob_itself = itself;
        return 0;
    }
    return -1;
}

#define CFStringRefObj_tp_alloc PyType_GenericAlloc

static PyObject *CFStringRefObj_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
    PyObject *self;
    if ((self = type->tp_alloc(type, 0)) == NULL) return NULL;
    ((CFStringRefObject *)self)->ob_itself = NULL;
    ((CFStringRefObject *)self)->ob_freeit = CFRelease;
    return self;
}

#define CFStringRefObj_tp_free PyObject_Del


PyTypeObject CFStringRef_Type = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "_CF.CFStringRef", /*tp_name*/
    sizeof(CFStringRefObject), /*tp_basicsize*/
    0, /*tp_itemsize*/
    /* methods */
    (destructor) CFStringRefObj_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)0, /*tp_getattr*/
    (setattrfunc)0, /*tp_setattr*/
    (cmpfunc) CFStringRefObj_compare, /*tp_compare*/
    (reprfunc) CFStringRefObj_repr, /*tp_repr*/
    (PyNumberMethods *)0, /* tp_as_number */
    (PySequenceMethods *)0, /* tp_as_sequence */
    (PyMappingMethods *)0, /* tp_as_mapping */
    (hashfunc) CFStringRefObj_hash, /*tp_hash*/
    0, /*tp_call*/
    0, /*tp_str*/
    PyObject_GenericGetAttr, /*tp_getattro*/
    PyObject_GenericSetAttr, /*tp_setattro */
    0, /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, /* tp_flags */
    0, /*tp_doc*/
    0, /*tp_traverse*/
    0, /*tp_clear*/
    0, /*tp_richcompare*/
    0, /*tp_weaklistoffset*/
    0, /*tp_iter*/
    0, /*tp_iternext*/
    CFStringRefObj_methods, /* tp_methods */
    0, /*tp_members*/
    CFStringRefObj_getsetlist, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    CFStringRefObj_tp_init, /* tp_init */
    CFStringRefObj_tp_alloc, /* tp_alloc */
    CFStringRefObj_tp_new, /* tp_new */
    CFStringRefObj_tp_free, /* tp_free */
};

/* ------------------ End object type CFStringRef ------------------- */


/* ----------------- Object type CFMutableStringRef ----------------- */

PyTypeObject CFMutableStringRef_Type;

#define CFMutableStringRefObj_Check(x) ((x)->ob_type == &CFMutableStringRef_Type || PyObject_TypeCheck((x), &CFMutableStringRef_Type))

typedef struct CFMutableStringRefObject {
    PyObject_HEAD
    CFMutableStringRef ob_itself;
    void (*ob_freeit)(CFTypeRef ptr);
} CFMutableStringRefObject;

PyObject *CFMutableStringRefObj_New(CFMutableStringRef itself)
{
    CFMutableStringRefObject *it;
    if (itself == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError, "cannot wrap NULL");
        return NULL;
    }
    it = PyObject_NEW(CFMutableStringRefObject, &CFMutableStringRef_Type);
    if (it == NULL) return NULL;
    /* XXXX Should we tp_init or tp_new our basetype? */
    it->ob_itself = itself;
    it->ob_freeit = CFRelease;
    return (PyObject *)it;
}

int CFMutableStringRefObj_Convert(PyObject *v, CFMutableStringRef *p_itself)
{

    if (v == Py_None) { *p_itself = NULL; return 1; }
    /* Check for other CF objects here */

    if (!CFMutableStringRefObj_Check(v))
    {
        PyErr_SetString(PyExc_TypeError, "CFMutableStringRef required");
        return 0;
    }
    *p_itself = ((CFMutableStringRefObject *)v)->ob_itself;
    return 1;
}

static void CFMutableStringRefObj_dealloc(CFMutableStringRefObject *self)
{
    if (self->ob_freeit && self->ob_itself)
    {
        self->ob_freeit((CFTypeRef)self->ob_itself);
        self->ob_itself = NULL;
    }
    CFStringRef_Type.tp_dealloc((PyObject *)self);
}

static PyObject *CFMutableStringRefObj_CFStringAppend(CFMutableStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef appendedString;
#ifndef CFStringAppend
    PyMac_PRECHECK(CFStringAppend);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          CFStringRefObj_Convert, &appendedString))
        return NULL;
    CFStringAppend(_self->ob_itself,
                   appendedString);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CFMutableStringRefObj_CFStringAppendCharacters(CFMutableStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    UniChar *chars__in__;
    UniCharCount chars__len__;
    int chars__in_len__;
#ifndef CFStringAppendCharacters
    PyMac_PRECHECK(CFStringAppendCharacters);
#endif
    if (!PyArg_ParseTuple(_args, "u#",
                          &chars__in__, &chars__in_len__))
        return NULL;
    chars__len__ = chars__in_len__;
    CFStringAppendCharacters(_self->ob_itself,
                             chars__in__, chars__len__);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CFMutableStringRefObj_CFStringAppendPascalString(CFMutableStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Str255 pStr;
    CFStringEncoding encoding;
#ifndef CFStringAppendPascalString
    PyMac_PRECHECK(CFStringAppendPascalString);
#endif
    if (!PyArg_ParseTuple(_args, "O&l",
                          PyMac_GetStr255, pStr,
                          &encoding))
        return NULL;
    CFStringAppendPascalString(_self->ob_itself,
                               pStr,
                               encoding);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CFMutableStringRefObj_CFStringAppendCString(CFMutableStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    char* cStr;
    CFStringEncoding encoding;
#ifndef CFStringAppendCString
    PyMac_PRECHECK(CFStringAppendCString);
#endif
    if (!PyArg_ParseTuple(_args, "sl",
                          &cStr,
                          &encoding))
        return NULL;
    CFStringAppendCString(_self->ob_itself,
                          cStr,
                          encoding);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CFMutableStringRefObj_CFStringInsert(CFMutableStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFIndex idx;
    CFStringRef insertedStr;
#ifndef CFStringInsert
    PyMac_PRECHECK(CFStringInsert);
#endif
    if (!PyArg_ParseTuple(_args, "lO&",
                          &idx,
                          CFStringRefObj_Convert, &insertedStr))
        return NULL;
    CFStringInsert(_self->ob_itself,
                   idx,
                   insertedStr);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CFMutableStringRefObj_CFStringDelete(CFMutableStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFRange range;
#ifndef CFStringDelete
    PyMac_PRECHECK(CFStringDelete);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          CFRange_Convert, &range))
        return NULL;
    CFStringDelete(_self->ob_itself,
                   range);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CFMutableStringRefObj_CFStringReplace(CFMutableStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFRange range;
    CFStringRef replacement;
#ifndef CFStringReplace
    PyMac_PRECHECK(CFStringReplace);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&",
                          CFRange_Convert, &range,
                          CFStringRefObj_Convert, &replacement))
        return NULL;
    CFStringReplace(_self->ob_itself,
                    range,
                    replacement);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CFMutableStringRefObj_CFStringReplaceAll(CFMutableStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef replacement;
#ifndef CFStringReplaceAll
    PyMac_PRECHECK(CFStringReplaceAll);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          CFStringRefObj_Convert, &replacement))
        return NULL;
    CFStringReplaceAll(_self->ob_itself,
                       replacement);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CFMutableStringRefObj_CFStringPad(CFMutableStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef padString;
    CFIndex length;
    CFIndex indexIntoPad;
#ifndef CFStringPad
    PyMac_PRECHECK(CFStringPad);
#endif
    if (!PyArg_ParseTuple(_args, "O&ll",
                          CFStringRefObj_Convert, &padString,
                          &length,
                          &indexIntoPad))
        return NULL;
    CFStringPad(_self->ob_itself,
                padString,
                length,
                indexIntoPad);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CFMutableStringRefObj_CFStringTrim(CFMutableStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef trimString;
#ifndef CFStringTrim
    PyMac_PRECHECK(CFStringTrim);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          CFStringRefObj_Convert, &trimString))
        return NULL;
    CFStringTrim(_self->ob_itself,
                 trimString);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CFMutableStringRefObj_CFStringTrimWhitespace(CFMutableStringRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
#ifndef CFStringTrimWhitespace
    PyMac_PRECHECK(CFStringTrimWhitespace);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    CFStringTrimWhitespace(_self->ob_itself);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyMethodDef CFMutableStringRefObj_methods[] = {
    {"CFStringAppend", (PyCFunction)CFMutableStringRefObj_CFStringAppend, 1,
     PyDoc_STR("(CFStringRef appendedString) -> None")},
    {"CFStringAppendCharacters", (PyCFunction)CFMutableStringRefObj_CFStringAppendCharacters, 1,
     PyDoc_STR("(Buffer chars) -> None")},
    {"CFStringAppendPascalString", (PyCFunction)CFMutableStringRefObj_CFStringAppendPascalString, 1,
     PyDoc_STR("(Str255 pStr, CFStringEncoding encoding) -> None")},
    {"CFStringAppendCString", (PyCFunction)CFMutableStringRefObj_CFStringAppendCString, 1,
     PyDoc_STR("(char* cStr, CFStringEncoding encoding) -> None")},
    {"CFStringInsert", (PyCFunction)CFMutableStringRefObj_CFStringInsert, 1,
     PyDoc_STR("(CFIndex idx, CFStringRef insertedStr) -> None")},
    {"CFStringDelete", (PyCFunction)CFMutableStringRefObj_CFStringDelete, 1,
     PyDoc_STR("(CFRange range) -> None")},
    {"CFStringReplace", (PyCFunction)CFMutableStringRefObj_CFStringReplace, 1,
     PyDoc_STR("(CFRange range, CFStringRef replacement) -> None")},
    {"CFStringReplaceAll", (PyCFunction)CFMutableStringRefObj_CFStringReplaceAll, 1,
     PyDoc_STR("(CFStringRef replacement) -> None")},
    {"CFStringPad", (PyCFunction)CFMutableStringRefObj_CFStringPad, 1,
     PyDoc_STR("(CFStringRef padString, CFIndex length, CFIndex indexIntoPad) -> None")},
    {"CFStringTrim", (PyCFunction)CFMutableStringRefObj_CFStringTrim, 1,
     PyDoc_STR("(CFStringRef trimString) -> None")},
    {"CFStringTrimWhitespace", (PyCFunction)CFMutableStringRefObj_CFStringTrimWhitespace, 1,
     PyDoc_STR("() -> None")},
    {NULL, NULL, 0}
};

#define CFMutableStringRefObj_getsetlist NULL


static int CFMutableStringRefObj_compare(CFMutableStringRefObject *self, CFMutableStringRefObject *other)
{
    /* XXXX Or should we use CFEqual?? */
    if ( self->ob_itself > other->ob_itself ) return 1;
    if ( self->ob_itself < other->ob_itself ) return -1;
    return 0;
}

static PyObject * CFMutableStringRefObj_repr(CFMutableStringRefObject *self)
{
    char buf[100];
    sprintf(buf, "<CFMutableStringRef object at 0x%8.8x for 0x%8.8x>", (unsigned)self, (unsigned)self->ob_itself);
    return PyString_FromString(buf);
}

static int CFMutableStringRefObj_hash(CFMutableStringRefObject *self)
{
    /* XXXX Or should we use CFHash?? */
    return (int)self->ob_itself;
}
static int CFMutableStringRefObj_tp_init(PyObject *_self, PyObject *_args, PyObject *_kwds)
{
    CFMutableStringRef itself;
    char *kw[] = {"itself", 0};

    if (PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, CFMutableStringRefObj_Convert, &itself))
    {
        ((CFMutableStringRefObject *)_self)->ob_itself = itself;
        return 0;
    }

    /* Any CFTypeRef descendent is allowed as initializer too */
    if (PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, CFTypeRefObj_Convert, &itself))
    {
        ((CFMutableStringRefObject *)_self)->ob_itself = itself;
        return 0;
    }
    return -1;
}

#define CFMutableStringRefObj_tp_alloc PyType_GenericAlloc

static PyObject *CFMutableStringRefObj_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
    PyObject *self;
    if ((self = type->tp_alloc(type, 0)) == NULL) return NULL;
    ((CFMutableStringRefObject *)self)->ob_itself = NULL;
    ((CFMutableStringRefObject *)self)->ob_freeit = CFRelease;
    return self;
}

#define CFMutableStringRefObj_tp_free PyObject_Del


PyTypeObject CFMutableStringRef_Type = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "_CF.CFMutableStringRef", /*tp_name*/
    sizeof(CFMutableStringRefObject), /*tp_basicsize*/
    0, /*tp_itemsize*/
    /* methods */
    (destructor) CFMutableStringRefObj_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)0, /*tp_getattr*/
    (setattrfunc)0, /*tp_setattr*/
    (cmpfunc) CFMutableStringRefObj_compare, /*tp_compare*/
    (reprfunc) CFMutableStringRefObj_repr, /*tp_repr*/
    (PyNumberMethods *)0, /* tp_as_number */
    (PySequenceMethods *)0, /* tp_as_sequence */
    (PyMappingMethods *)0, /* tp_as_mapping */
    (hashfunc) CFMutableStringRefObj_hash, /*tp_hash*/
    0, /*tp_call*/
    0, /*tp_str*/
    PyObject_GenericGetAttr, /*tp_getattro*/
    PyObject_GenericSetAttr, /*tp_setattro */
    0, /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, /* tp_flags */
    0, /*tp_doc*/
    0, /*tp_traverse*/
    0, /*tp_clear*/
    0, /*tp_richcompare*/
    0, /*tp_weaklistoffset*/
    0, /*tp_iter*/
    0, /*tp_iternext*/
    CFMutableStringRefObj_methods, /* tp_methods */
    0, /*tp_members*/
    CFMutableStringRefObj_getsetlist, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    CFMutableStringRefObj_tp_init, /* tp_init */
    CFMutableStringRefObj_tp_alloc, /* tp_alloc */
    CFMutableStringRefObj_tp_new, /* tp_new */
    CFMutableStringRefObj_tp_free, /* tp_free */
};

/* --------------- End object type CFMutableStringRef --------------- */


/* ---------------------- Object type CFURLRef ---------------------- */

PyTypeObject CFURLRef_Type;

#define CFURLRefObj_Check(x) ((x)->ob_type == &CFURLRef_Type || PyObject_TypeCheck((x), &CFURLRef_Type))

typedef struct CFURLRefObject {
    PyObject_HEAD
    CFURLRef ob_itself;
    void (*ob_freeit)(CFTypeRef ptr);
} CFURLRefObject;

PyObject *CFURLRefObj_New(CFURLRef itself)
{
    CFURLRefObject *it;
    if (itself == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError, "cannot wrap NULL");
        return NULL;
    }
    it = PyObject_NEW(CFURLRefObject, &CFURLRef_Type);
    if (it == NULL) return NULL;
    /* XXXX Should we tp_init or tp_new our basetype? */
    it->ob_itself = itself;
    it->ob_freeit = CFRelease;
    return (PyObject *)it;
}

int CFURLRefObj_Convert(PyObject *v, CFURLRef *p_itself)
{

    if (v == Py_None) { *p_itself = NULL; return 1; }
    /* Check for other CF objects here */

    if (!CFURLRefObj_Check(v))
    {
        PyErr_SetString(PyExc_TypeError, "CFURLRef required");
        return 0;
    }
    *p_itself = ((CFURLRefObject *)v)->ob_itself;
    return 1;
}

static void CFURLRefObj_dealloc(CFURLRefObject *self)
{
    if (self->ob_freeit && self->ob_itself)
    {
        self->ob_freeit((CFTypeRef)self->ob_itself);
        self->ob_itself = NULL;
    }
    CFTypeRef_Type.tp_dealloc((PyObject *)self);
}

static PyObject *CFURLRefObj_CFURLCreateData(CFURLRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFDataRef _rv;
    CFStringEncoding encoding;
    Boolean escapeWhitespace;
    if (!PyArg_ParseTuple(_args, "ll",
                          &encoding,
                          &escapeWhitespace))
        return NULL;
    _rv = CFURLCreateData((CFAllocatorRef)NULL,
                          _self->ob_itself,
                          encoding,
                          escapeWhitespace);
    _res = Py_BuildValue("O&",
                         CFDataRefObj_New, _rv);
    return _res;
}

static PyObject *CFURLRefObj_CFURLGetFileSystemRepresentation(CFURLRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
    Boolean resolveAgainstBase;
    UInt8 buffer;
    CFIndex maxBufLen;
#ifndef CFURLGetFileSystemRepresentation
    PyMac_PRECHECK(CFURLGetFileSystemRepresentation);
#endif
    if (!PyArg_ParseTuple(_args, "ll",
                          &resolveAgainstBase,
                          &maxBufLen))
        return NULL;
    _rv = CFURLGetFileSystemRepresentation(_self->ob_itself,
                                           resolveAgainstBase,
                                           &buffer,
                                           maxBufLen);
    _res = Py_BuildValue("lb",
                         _rv,
                         buffer);
    return _res;
}

static PyObject *CFURLRefObj_CFURLCopyAbsoluteURL(CFURLRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFURLRef _rv;
#ifndef CFURLCopyAbsoluteURL
    PyMac_PRECHECK(CFURLCopyAbsoluteURL);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFURLCopyAbsoluteURL(_self->ob_itself);
    _res = Py_BuildValue("O&",
                         CFURLRefObj_New, _rv);
    return _res;
}

static PyObject *CFURLRefObj_CFURLGetString(CFURLRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
#ifndef CFURLGetString
    PyMac_PRECHECK(CFURLGetString);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFURLGetString(_self->ob_itself);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CFURLRefObj_CFURLGetBaseURL(CFURLRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFURLRef _rv;
#ifndef CFURLGetBaseURL
    PyMac_PRECHECK(CFURLGetBaseURL);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFURLGetBaseURL(_self->ob_itself);
    _res = Py_BuildValue("O&",
                         CFURLRefObj_New, _rv);
    return _res;
}

static PyObject *CFURLRefObj_CFURLCanBeDecomposed(CFURLRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
#ifndef CFURLCanBeDecomposed
    PyMac_PRECHECK(CFURLCanBeDecomposed);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFURLCanBeDecomposed(_self->ob_itself);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CFURLRefObj_CFURLCopyScheme(CFURLRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
#ifndef CFURLCopyScheme
    PyMac_PRECHECK(CFURLCopyScheme);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFURLCopyScheme(_self->ob_itself);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CFURLRefObj_CFURLCopyNetLocation(CFURLRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
#ifndef CFURLCopyNetLocation
    PyMac_PRECHECK(CFURLCopyNetLocation);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFURLCopyNetLocation(_self->ob_itself);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CFURLRefObj_CFURLCopyPath(CFURLRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
#ifndef CFURLCopyPath
    PyMac_PRECHECK(CFURLCopyPath);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFURLCopyPath(_self->ob_itself);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CFURLRefObj_CFURLCopyStrictPath(CFURLRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
    Boolean isAbsolute;
#ifndef CFURLCopyStrictPath
    PyMac_PRECHECK(CFURLCopyStrictPath);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFURLCopyStrictPath(_self->ob_itself,
                              &isAbsolute);
    _res = Py_BuildValue("O&l",
                         CFStringRefObj_New, _rv,
                         isAbsolute);
    return _res;
}

static PyObject *CFURLRefObj_CFURLCopyFileSystemPath(CFURLRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
    CFURLPathStyle pathStyle;
#ifndef CFURLCopyFileSystemPath
    PyMac_PRECHECK(CFURLCopyFileSystemPath);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &pathStyle))
        return NULL;
    _rv = CFURLCopyFileSystemPath(_self->ob_itself,
                                  pathStyle);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CFURLRefObj_CFURLHasDirectoryPath(CFURLRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
#ifndef CFURLHasDirectoryPath
    PyMac_PRECHECK(CFURLHasDirectoryPath);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFURLHasDirectoryPath(_self->ob_itself);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CFURLRefObj_CFURLCopyResourceSpecifier(CFURLRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
#ifndef CFURLCopyResourceSpecifier
    PyMac_PRECHECK(CFURLCopyResourceSpecifier);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFURLCopyResourceSpecifier(_self->ob_itself);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CFURLRefObj_CFURLCopyHostName(CFURLRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
#ifndef CFURLCopyHostName
    PyMac_PRECHECK(CFURLCopyHostName);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFURLCopyHostName(_self->ob_itself);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CFURLRefObj_CFURLGetPortNumber(CFURLRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    SInt32 _rv;
#ifndef CFURLGetPortNumber
    PyMac_PRECHECK(CFURLGetPortNumber);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFURLGetPortNumber(_self->ob_itself);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CFURLRefObj_CFURLCopyUserName(CFURLRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
#ifndef CFURLCopyUserName
    PyMac_PRECHECK(CFURLCopyUserName);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFURLCopyUserName(_self->ob_itself);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CFURLRefObj_CFURLCopyPassword(CFURLRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
#ifndef CFURLCopyPassword
    PyMac_PRECHECK(CFURLCopyPassword);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFURLCopyPassword(_self->ob_itself);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CFURLRefObj_CFURLCopyParameterString(CFURLRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
    CFStringRef charactersToLeaveEscaped;
#ifndef CFURLCopyParameterString
    PyMac_PRECHECK(CFURLCopyParameterString);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          CFStringRefObj_Convert, &charactersToLeaveEscaped))
        return NULL;
    _rv = CFURLCopyParameterString(_self->ob_itself,
                                   charactersToLeaveEscaped);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CFURLRefObj_CFURLCopyQueryString(CFURLRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
    CFStringRef charactersToLeaveEscaped;
#ifndef CFURLCopyQueryString
    PyMac_PRECHECK(CFURLCopyQueryString);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          CFStringRefObj_Convert, &charactersToLeaveEscaped))
        return NULL;
    _rv = CFURLCopyQueryString(_self->ob_itself,
                               charactersToLeaveEscaped);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CFURLRefObj_CFURLCopyFragment(CFURLRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
    CFStringRef charactersToLeaveEscaped;
#ifndef CFURLCopyFragment
    PyMac_PRECHECK(CFURLCopyFragment);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          CFStringRefObj_Convert, &charactersToLeaveEscaped))
        return NULL;
    _rv = CFURLCopyFragment(_self->ob_itself,
                            charactersToLeaveEscaped);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CFURLRefObj_CFURLCopyLastPathComponent(CFURLRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
#ifndef CFURLCopyLastPathComponent
    PyMac_PRECHECK(CFURLCopyLastPathComponent);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFURLCopyLastPathComponent(_self->ob_itself);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CFURLRefObj_CFURLCopyPathExtension(CFURLRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
#ifndef CFURLCopyPathExtension
    PyMac_PRECHECK(CFURLCopyPathExtension);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFURLCopyPathExtension(_self->ob_itself);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CFURLRefObj_CFURLCreateCopyAppendingPathComponent(CFURLRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFURLRef _rv;
    CFStringRef pathComponent;
    Boolean isDirectory;
    if (!PyArg_ParseTuple(_args, "O&l",
                          CFStringRefObj_Convert, &pathComponent,
                          &isDirectory))
        return NULL;
    _rv = CFURLCreateCopyAppendingPathComponent((CFAllocatorRef)NULL,
                                                _self->ob_itself,
                                                pathComponent,
                                                isDirectory);
    _res = Py_BuildValue("O&",
                         CFURLRefObj_New, _rv);
    return _res;
}

static PyObject *CFURLRefObj_CFURLCreateCopyDeletingLastPathComponent(CFURLRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFURLRef _rv;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFURLCreateCopyDeletingLastPathComponent((CFAllocatorRef)NULL,
                                                   _self->ob_itself);
    _res = Py_BuildValue("O&",
                         CFURLRefObj_New, _rv);
    return _res;
}

static PyObject *CFURLRefObj_CFURLCreateCopyAppendingPathExtension(CFURLRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFURLRef _rv;
    CFStringRef extension;
    if (!PyArg_ParseTuple(_args, "O&",
                          CFStringRefObj_Convert, &extension))
        return NULL;
    _rv = CFURLCreateCopyAppendingPathExtension((CFAllocatorRef)NULL,
                                                _self->ob_itself,
                                                extension);
    _res = Py_BuildValue("O&",
                         CFURLRefObj_New, _rv);
    return _res;
}

static PyObject *CFURLRefObj_CFURLCreateCopyDeletingPathExtension(CFURLRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFURLRef _rv;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFURLCreateCopyDeletingPathExtension((CFAllocatorRef)NULL,
                                               _self->ob_itself);
    _res = Py_BuildValue("O&",
                         CFURLRefObj_New, _rv);
    return _res;
}

static PyObject *CFURLRefObj_CFURLGetFSRef(CFURLRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
    FSRef fsRef;
#ifndef CFURLGetFSRef
    PyMac_PRECHECK(CFURLGetFSRef);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFURLGetFSRef(_self->ob_itself,
                        &fsRef);
    _res = Py_BuildValue("lO&",
                         _rv,
                         PyMac_BuildFSRef, &fsRef);
    return _res;
}

static PyMethodDef CFURLRefObj_methods[] = {
    {"CFURLCreateData", (PyCFunction)CFURLRefObj_CFURLCreateData, 1,
     PyDoc_STR("(CFStringEncoding encoding, Boolean escapeWhitespace) -> (CFDataRef _rv)")},
    {"CFURLGetFileSystemRepresentation", (PyCFunction)CFURLRefObj_CFURLGetFileSystemRepresentation, 1,
     PyDoc_STR("(Boolean resolveAgainstBase, CFIndex maxBufLen) -> (Boolean _rv, UInt8 buffer)")},
    {"CFURLCopyAbsoluteURL", (PyCFunction)CFURLRefObj_CFURLCopyAbsoluteURL, 1,
     PyDoc_STR("() -> (CFURLRef _rv)")},
    {"CFURLGetString", (PyCFunction)CFURLRefObj_CFURLGetString, 1,
     PyDoc_STR("() -> (CFStringRef _rv)")},
    {"CFURLGetBaseURL", (PyCFunction)CFURLRefObj_CFURLGetBaseURL, 1,
     PyDoc_STR("() -> (CFURLRef _rv)")},
    {"CFURLCanBeDecomposed", (PyCFunction)CFURLRefObj_CFURLCanBeDecomposed, 1,
     PyDoc_STR("() -> (Boolean _rv)")},
    {"CFURLCopyScheme", (PyCFunction)CFURLRefObj_CFURLCopyScheme, 1,
     PyDoc_STR("() -> (CFStringRef _rv)")},
    {"CFURLCopyNetLocation", (PyCFunction)CFURLRefObj_CFURLCopyNetLocation, 1,
     PyDoc_STR("() -> (CFStringRef _rv)")},
    {"CFURLCopyPath", (PyCFunction)CFURLRefObj_CFURLCopyPath, 1,
     PyDoc_STR("() -> (CFStringRef _rv)")},
    {"CFURLCopyStrictPath", (PyCFunction)CFURLRefObj_CFURLCopyStrictPath, 1,
     PyDoc_STR("() -> (CFStringRef _rv, Boolean isAbsolute)")},
    {"CFURLCopyFileSystemPath", (PyCFunction)CFURLRefObj_CFURLCopyFileSystemPath, 1,
     PyDoc_STR("(CFURLPathStyle pathStyle) -> (CFStringRef _rv)")},
    {"CFURLHasDirectoryPath", (PyCFunction)CFURLRefObj_CFURLHasDirectoryPath, 1,
     PyDoc_STR("() -> (Boolean _rv)")},
    {"CFURLCopyResourceSpecifier", (PyCFunction)CFURLRefObj_CFURLCopyResourceSpecifier, 1,
     PyDoc_STR("() -> (CFStringRef _rv)")},
    {"CFURLCopyHostName", (PyCFunction)CFURLRefObj_CFURLCopyHostName, 1,
     PyDoc_STR("() -> (CFStringRef _rv)")},
    {"CFURLGetPortNumber", (PyCFunction)CFURLRefObj_CFURLGetPortNumber, 1,
     PyDoc_STR("() -> (SInt32 _rv)")},
    {"CFURLCopyUserName", (PyCFunction)CFURLRefObj_CFURLCopyUserName, 1,
     PyDoc_STR("() -> (CFStringRef _rv)")},
    {"CFURLCopyPassword", (PyCFunction)CFURLRefObj_CFURLCopyPassword, 1,
     PyDoc_STR("() -> (CFStringRef _rv)")},
    {"CFURLCopyParameterString", (PyCFunction)CFURLRefObj_CFURLCopyParameterString, 1,
     PyDoc_STR("(CFStringRef charactersToLeaveEscaped) -> (CFStringRef _rv)")},
    {"CFURLCopyQueryString", (PyCFunction)CFURLRefObj_CFURLCopyQueryString, 1,
     PyDoc_STR("(CFStringRef charactersToLeaveEscaped) -> (CFStringRef _rv)")},
    {"CFURLCopyFragment", (PyCFunction)CFURLRefObj_CFURLCopyFragment, 1,
     PyDoc_STR("(CFStringRef charactersToLeaveEscaped) -> (CFStringRef _rv)")},
    {"CFURLCopyLastPathComponent", (PyCFunction)CFURLRefObj_CFURLCopyLastPathComponent, 1,
     PyDoc_STR("() -> (CFStringRef _rv)")},
    {"CFURLCopyPathExtension", (PyCFunction)CFURLRefObj_CFURLCopyPathExtension, 1,
     PyDoc_STR("() -> (CFStringRef _rv)")},
    {"CFURLCreateCopyAppendingPathComponent", (PyCFunction)CFURLRefObj_CFURLCreateCopyAppendingPathComponent, 1,
     PyDoc_STR("(CFStringRef pathComponent, Boolean isDirectory) -> (CFURLRef _rv)")},
    {"CFURLCreateCopyDeletingLastPathComponent", (PyCFunction)CFURLRefObj_CFURLCreateCopyDeletingLastPathComponent, 1,
     PyDoc_STR("() -> (CFURLRef _rv)")},
    {"CFURLCreateCopyAppendingPathExtension", (PyCFunction)CFURLRefObj_CFURLCreateCopyAppendingPathExtension, 1,
     PyDoc_STR("(CFStringRef extension) -> (CFURLRef _rv)")},
    {"CFURLCreateCopyDeletingPathExtension", (PyCFunction)CFURLRefObj_CFURLCreateCopyDeletingPathExtension, 1,
     PyDoc_STR("() -> (CFURLRef _rv)")},
    {"CFURLGetFSRef", (PyCFunction)CFURLRefObj_CFURLGetFSRef, 1,
     PyDoc_STR("() -> (Boolean _rv, FSRef fsRef)")},
    {NULL, NULL, 0}
};

#define CFURLRefObj_getsetlist NULL


static int CFURLRefObj_compare(CFURLRefObject *self, CFURLRefObject *other)
{
    /* XXXX Or should we use CFEqual?? */
    if ( self->ob_itself > other->ob_itself ) return 1;
    if ( self->ob_itself < other->ob_itself ) return -1;
    return 0;
}

static PyObject * CFURLRefObj_repr(CFURLRefObject *self)
{
    char buf[100];
    sprintf(buf, "<CFURL object at 0x%8.8x for 0x%8.8x>", (unsigned)self, (unsigned)self->ob_itself);
    return PyString_FromString(buf);
}

static int CFURLRefObj_hash(CFURLRefObject *self)
{
    /* XXXX Or should we use CFHash?? */
    return (int)self->ob_itself;
}
static int CFURLRefObj_tp_init(PyObject *_self, PyObject *_args, PyObject *_kwds)
{
    CFURLRef itself;
    char *kw[] = {"itself", 0};

    if (PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, CFURLRefObj_Convert, &itself))
    {
        ((CFURLRefObject *)_self)->ob_itself = itself;
        return 0;
    }

    /* Any CFTypeRef descendent is allowed as initializer too */
    if (PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, CFTypeRefObj_Convert, &itself))
    {
        ((CFURLRefObject *)_self)->ob_itself = itself;
        return 0;
    }
    return -1;
}

#define CFURLRefObj_tp_alloc PyType_GenericAlloc

static PyObject *CFURLRefObj_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
    PyObject *self;
    if ((self = type->tp_alloc(type, 0)) == NULL) return NULL;
    ((CFURLRefObject *)self)->ob_itself = NULL;
    ((CFURLRefObject *)self)->ob_freeit = CFRelease;
    return self;
}

#define CFURLRefObj_tp_free PyObject_Del


PyTypeObject CFURLRef_Type = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "_CF.CFURLRef", /*tp_name*/
    sizeof(CFURLRefObject), /*tp_basicsize*/
    0, /*tp_itemsize*/
    /* methods */
    (destructor) CFURLRefObj_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)0, /*tp_getattr*/
    (setattrfunc)0, /*tp_setattr*/
    (cmpfunc) CFURLRefObj_compare, /*tp_compare*/
    (reprfunc) CFURLRefObj_repr, /*tp_repr*/
    (PyNumberMethods *)0, /* tp_as_number */
    (PySequenceMethods *)0, /* tp_as_sequence */
    (PyMappingMethods *)0, /* tp_as_mapping */
    (hashfunc) CFURLRefObj_hash, /*tp_hash*/
    0, /*tp_call*/
    0, /*tp_str*/
    PyObject_GenericGetAttr, /*tp_getattro*/
    PyObject_GenericSetAttr, /*tp_setattro */
    0, /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, /* tp_flags */
    0, /*tp_doc*/
    0, /*tp_traverse*/
    0, /*tp_clear*/
    0, /*tp_richcompare*/
    0, /*tp_weaklistoffset*/
    0, /*tp_iter*/
    0, /*tp_iternext*/
    CFURLRefObj_methods, /* tp_methods */
    0, /*tp_members*/
    CFURLRefObj_getsetlist, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    CFURLRefObj_tp_init, /* tp_init */
    CFURLRefObj_tp_alloc, /* tp_alloc */
    CFURLRefObj_tp_new, /* tp_new */
    CFURLRefObj_tp_free, /* tp_free */
};

/* -------------------- End object type CFURLRef -------------------- */


static PyObject *CF___CFRangeMake(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFRange _rv;
    CFIndex loc;
    CFIndex len;
#ifndef __CFRangeMake
    PyMac_PRECHECK(__CFRangeMake);
#endif
    if (!PyArg_ParseTuple(_args, "ll",
                          &loc,
                          &len))
        return NULL;
    _rv = __CFRangeMake(loc,
                        len);
    _res = Py_BuildValue("O&",
                         CFRange_New, _rv);
    return _res;
}

static PyObject *CF_CFAllocatorGetTypeID(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFTypeID _rv;
#ifndef CFAllocatorGetTypeID
    PyMac_PRECHECK(CFAllocatorGetTypeID);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFAllocatorGetTypeID();
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CF_CFAllocatorGetPreferredSizeForSize(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFIndex _rv;
    CFIndex size;
    CFOptionFlags hint;
#ifndef CFAllocatorGetPreferredSizeForSize
    PyMac_PRECHECK(CFAllocatorGetPreferredSizeForSize);
#endif
    if (!PyArg_ParseTuple(_args, "ll",
                          &size,
                          &hint))
        return NULL;
    _rv = CFAllocatorGetPreferredSizeForSize((CFAllocatorRef)NULL,
                                             size,
                                             hint);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CF_CFCopyTypeIDDescription(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
    CFTypeID type_id;
#ifndef CFCopyTypeIDDescription
    PyMac_PRECHECK(CFCopyTypeIDDescription);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &type_id))
        return NULL;
    _rv = CFCopyTypeIDDescription(type_id);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFArrayGetTypeID(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFTypeID _rv;
#ifndef CFArrayGetTypeID
    PyMac_PRECHECK(CFArrayGetTypeID);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFArrayGetTypeID();
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CF_CFArrayCreateMutable(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFMutableArrayRef _rv;
    CFIndex capacity;
#ifndef CFArrayCreateMutable
    PyMac_PRECHECK(CFArrayCreateMutable);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &capacity))
        return NULL;
    _rv = CFArrayCreateMutable((CFAllocatorRef)NULL,
                               capacity,
                               &kCFTypeArrayCallBacks);
    _res = Py_BuildValue("O&",
                         CFMutableArrayRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFArrayCreateMutableCopy(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFMutableArrayRef _rv;
    CFIndex capacity;
    CFArrayRef theArray;
#ifndef CFArrayCreateMutableCopy
    PyMac_PRECHECK(CFArrayCreateMutableCopy);
#endif
    if (!PyArg_ParseTuple(_args, "lO&",
                          &capacity,
                          CFArrayRefObj_Convert, &theArray))
        return NULL;
    _rv = CFArrayCreateMutableCopy((CFAllocatorRef)NULL,
                                   capacity,
                                   theArray);
    _res = Py_BuildValue("O&",
                         CFMutableArrayRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFDataGetTypeID(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFTypeID _rv;
#ifndef CFDataGetTypeID
    PyMac_PRECHECK(CFDataGetTypeID);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFDataGetTypeID();
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CF_CFDataCreate(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFDataRef _rv;
    unsigned char *bytes__in__;
    long bytes__len__;
    int bytes__in_len__;
#ifndef CFDataCreate
    PyMac_PRECHECK(CFDataCreate);
#endif
    if (!PyArg_ParseTuple(_args, "s#",
                          &bytes__in__, &bytes__in_len__))
        return NULL;
    bytes__len__ = bytes__in_len__;
    _rv = CFDataCreate((CFAllocatorRef)NULL,
                       bytes__in__, bytes__len__);
    _res = Py_BuildValue("O&",
                         CFDataRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFDataCreateWithBytesNoCopy(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFDataRef _rv;
    unsigned char *bytes__in__;
    long bytes__len__;
    int bytes__in_len__;
#ifndef CFDataCreateWithBytesNoCopy
    PyMac_PRECHECK(CFDataCreateWithBytesNoCopy);
#endif
    if (!PyArg_ParseTuple(_args, "s#",
                          &bytes__in__, &bytes__in_len__))
        return NULL;
    bytes__len__ = bytes__in_len__;
    _rv = CFDataCreateWithBytesNoCopy((CFAllocatorRef)NULL,
                                      bytes__in__, bytes__len__,
                                      (CFAllocatorRef)NULL);
    _res = Py_BuildValue("O&",
                         CFDataRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFDataCreateMutable(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFMutableDataRef _rv;
    CFIndex capacity;
#ifndef CFDataCreateMutable
    PyMac_PRECHECK(CFDataCreateMutable);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &capacity))
        return NULL;
    _rv = CFDataCreateMutable((CFAllocatorRef)NULL,
                              capacity);
    _res = Py_BuildValue("O&",
                         CFMutableDataRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFDataCreateMutableCopy(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFMutableDataRef _rv;
    CFIndex capacity;
    CFDataRef theData;
#ifndef CFDataCreateMutableCopy
    PyMac_PRECHECK(CFDataCreateMutableCopy);
#endif
    if (!PyArg_ParseTuple(_args, "lO&",
                          &capacity,
                          CFDataRefObj_Convert, &theData))
        return NULL;
    _rv = CFDataCreateMutableCopy((CFAllocatorRef)NULL,
                                  capacity,
                                  theData);
    _res = Py_BuildValue("O&",
                         CFMutableDataRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFDictionaryGetTypeID(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFTypeID _rv;
#ifndef CFDictionaryGetTypeID
    PyMac_PRECHECK(CFDictionaryGetTypeID);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFDictionaryGetTypeID();
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CF_CFDictionaryCreateMutable(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFMutableDictionaryRef _rv;
    CFIndex capacity;
#ifndef CFDictionaryCreateMutable
    PyMac_PRECHECK(CFDictionaryCreateMutable);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &capacity))
        return NULL;
    _rv = CFDictionaryCreateMutable((CFAllocatorRef)NULL,
                                    capacity,
                                    &kCFTypeDictionaryKeyCallBacks,
                                    &kCFTypeDictionaryValueCallBacks);
    _res = Py_BuildValue("O&",
                         CFMutableDictionaryRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFDictionaryCreateMutableCopy(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFMutableDictionaryRef _rv;
    CFIndex capacity;
    CFDictionaryRef theDict;
#ifndef CFDictionaryCreateMutableCopy
    PyMac_PRECHECK(CFDictionaryCreateMutableCopy);
#endif
    if (!PyArg_ParseTuple(_args, "lO&",
                          &capacity,
                          CFDictionaryRefObj_Convert, &theDict))
        return NULL;
    _rv = CFDictionaryCreateMutableCopy((CFAllocatorRef)NULL,
                                        capacity,
                                        theDict);
    _res = Py_BuildValue("O&",
                         CFMutableDictionaryRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFPreferencesCopyAppValue(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFTypeRef _rv;
    CFStringRef key;
    CFStringRef applicationID;
#ifndef CFPreferencesCopyAppValue
    PyMac_PRECHECK(CFPreferencesCopyAppValue);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&",
                          CFStringRefObj_Convert, &key,
                          CFStringRefObj_Convert, &applicationID))
        return NULL;
    _rv = CFPreferencesCopyAppValue(key,
                                    applicationID);
    _res = Py_BuildValue("O&",
                         CFTypeRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFPreferencesGetAppBooleanValue(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
    CFStringRef key;
    CFStringRef applicationID;
    Boolean keyExistsAndHasValidFormat;
#ifndef CFPreferencesGetAppBooleanValue
    PyMac_PRECHECK(CFPreferencesGetAppBooleanValue);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&",
                          CFStringRefObj_Convert, &key,
                          CFStringRefObj_Convert, &applicationID))
        return NULL;
    _rv = CFPreferencesGetAppBooleanValue(key,
                                          applicationID,
                                          &keyExistsAndHasValidFormat);
    _res = Py_BuildValue("ll",
                         _rv,
                         keyExistsAndHasValidFormat);
    return _res;
}

static PyObject *CF_CFPreferencesGetAppIntegerValue(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFIndex _rv;
    CFStringRef key;
    CFStringRef applicationID;
    Boolean keyExistsAndHasValidFormat;
#ifndef CFPreferencesGetAppIntegerValue
    PyMac_PRECHECK(CFPreferencesGetAppIntegerValue);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&",
                          CFStringRefObj_Convert, &key,
                          CFStringRefObj_Convert, &applicationID))
        return NULL;
    _rv = CFPreferencesGetAppIntegerValue(key,
                                          applicationID,
                                          &keyExistsAndHasValidFormat);
    _res = Py_BuildValue("ll",
                         _rv,
                         keyExistsAndHasValidFormat);
    return _res;
}

static PyObject *CF_CFPreferencesSetAppValue(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef key;
    CFTypeRef value;
    CFStringRef applicationID;
#ifndef CFPreferencesSetAppValue
    PyMac_PRECHECK(CFPreferencesSetAppValue);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&O&",
                          CFStringRefObj_Convert, &key,
                          CFTypeRefObj_Convert, &value,
                          CFStringRefObj_Convert, &applicationID))
        return NULL;
    CFPreferencesSetAppValue(key,
                             value,
                             applicationID);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CF_CFPreferencesAddSuitePreferencesToApp(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef applicationID;
    CFStringRef suiteID;
#ifndef CFPreferencesAddSuitePreferencesToApp
    PyMac_PRECHECK(CFPreferencesAddSuitePreferencesToApp);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&",
                          CFStringRefObj_Convert, &applicationID,
                          CFStringRefObj_Convert, &suiteID))
        return NULL;
    CFPreferencesAddSuitePreferencesToApp(applicationID,
                                          suiteID);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CF_CFPreferencesRemoveSuitePreferencesFromApp(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef applicationID;
    CFStringRef suiteID;
#ifndef CFPreferencesRemoveSuitePreferencesFromApp
    PyMac_PRECHECK(CFPreferencesRemoveSuitePreferencesFromApp);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&",
                          CFStringRefObj_Convert, &applicationID,
                          CFStringRefObj_Convert, &suiteID))
        return NULL;
    CFPreferencesRemoveSuitePreferencesFromApp(applicationID,
                                               suiteID);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CF_CFPreferencesAppSynchronize(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
    CFStringRef applicationID;
#ifndef CFPreferencesAppSynchronize
    PyMac_PRECHECK(CFPreferencesAppSynchronize);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          CFStringRefObj_Convert, &applicationID))
        return NULL;
    _rv = CFPreferencesAppSynchronize(applicationID);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CF_CFPreferencesCopyValue(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFTypeRef _rv;
    CFStringRef key;
    CFStringRef applicationID;
    CFStringRef userName;
    CFStringRef hostName;
#ifndef CFPreferencesCopyValue
    PyMac_PRECHECK(CFPreferencesCopyValue);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&O&O&",
                          CFStringRefObj_Convert, &key,
                          CFStringRefObj_Convert, &applicationID,
                          CFStringRefObj_Convert, &userName,
                          CFStringRefObj_Convert, &hostName))
        return NULL;
    _rv = CFPreferencesCopyValue(key,
                                 applicationID,
                                 userName,
                                 hostName);
    _res = Py_BuildValue("O&",
                         CFTypeRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFPreferencesCopyMultiple(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFDictionaryRef _rv;
    CFArrayRef keysToFetch;
    CFStringRef applicationID;
    CFStringRef userName;
    CFStringRef hostName;
#ifndef CFPreferencesCopyMultiple
    PyMac_PRECHECK(CFPreferencesCopyMultiple);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&O&O&",
                          CFArrayRefObj_Convert, &keysToFetch,
                          CFStringRefObj_Convert, &applicationID,
                          CFStringRefObj_Convert, &userName,
                          CFStringRefObj_Convert, &hostName))
        return NULL;
    _rv = CFPreferencesCopyMultiple(keysToFetch,
                                    applicationID,
                                    userName,
                                    hostName);
    _res = Py_BuildValue("O&",
                         CFDictionaryRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFPreferencesSetValue(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef key;
    CFTypeRef value;
    CFStringRef applicationID;
    CFStringRef userName;
    CFStringRef hostName;
#ifndef CFPreferencesSetValue
    PyMac_PRECHECK(CFPreferencesSetValue);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&O&O&O&",
                          CFStringRefObj_Convert, &key,
                          CFTypeRefObj_Convert, &value,
                          CFStringRefObj_Convert, &applicationID,
                          CFStringRefObj_Convert, &userName,
                          CFStringRefObj_Convert, &hostName))
        return NULL;
    CFPreferencesSetValue(key,
                          value,
                          applicationID,
                          userName,
                          hostName);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CF_CFPreferencesSetMultiple(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFDictionaryRef keysToSet;
    CFArrayRef keysToRemove;
    CFStringRef applicationID;
    CFStringRef userName;
    CFStringRef hostName;
#ifndef CFPreferencesSetMultiple
    PyMac_PRECHECK(CFPreferencesSetMultiple);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&O&O&O&",
                          CFDictionaryRefObj_Convert, &keysToSet,
                          CFArrayRefObj_Convert, &keysToRemove,
                          CFStringRefObj_Convert, &applicationID,
                          CFStringRefObj_Convert, &userName,
                          CFStringRefObj_Convert, &hostName))
        return NULL;
    CFPreferencesSetMultiple(keysToSet,
                             keysToRemove,
                             applicationID,
                             userName,
                             hostName);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CF_CFPreferencesSynchronize(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
    CFStringRef applicationID;
    CFStringRef userName;
    CFStringRef hostName;
#ifndef CFPreferencesSynchronize
    PyMac_PRECHECK(CFPreferencesSynchronize);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&O&",
                          CFStringRefObj_Convert, &applicationID,
                          CFStringRefObj_Convert, &userName,
                          CFStringRefObj_Convert, &hostName))
        return NULL;
    _rv = CFPreferencesSynchronize(applicationID,
                                   userName,
                                   hostName);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CF_CFPreferencesCopyApplicationList(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFArrayRef _rv;
    CFStringRef userName;
    CFStringRef hostName;
#ifndef CFPreferencesCopyApplicationList
    PyMac_PRECHECK(CFPreferencesCopyApplicationList);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&",
                          CFStringRefObj_Convert, &userName,
                          CFStringRefObj_Convert, &hostName))
        return NULL;
    _rv = CFPreferencesCopyApplicationList(userName,
                                           hostName);
    _res = Py_BuildValue("O&",
                         CFArrayRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFPreferencesCopyKeyList(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFArrayRef _rv;
    CFStringRef applicationID;
    CFStringRef userName;
    CFStringRef hostName;
#ifndef CFPreferencesCopyKeyList
    PyMac_PRECHECK(CFPreferencesCopyKeyList);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&O&",
                          CFStringRefObj_Convert, &applicationID,
                          CFStringRefObj_Convert, &userName,
                          CFStringRefObj_Convert, &hostName))
        return NULL;
    _rv = CFPreferencesCopyKeyList(applicationID,
                                   userName,
                                   hostName);
    _res = Py_BuildValue("O&",
                         CFArrayRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFStringGetTypeID(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFTypeID _rv;
#ifndef CFStringGetTypeID
    PyMac_PRECHECK(CFStringGetTypeID);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFStringGetTypeID();
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CF_CFStringCreateWithPascalString(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
    Str255 pStr;
    CFStringEncoding encoding;
#ifndef CFStringCreateWithPascalString
    PyMac_PRECHECK(CFStringCreateWithPascalString);
#endif
    if (!PyArg_ParseTuple(_args, "O&l",
                          PyMac_GetStr255, pStr,
                          &encoding))
        return NULL;
    _rv = CFStringCreateWithPascalString((CFAllocatorRef)NULL,
                                         pStr,
                                         encoding);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFStringCreateWithCString(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
    char* cStr;
    CFStringEncoding encoding;
#ifndef CFStringCreateWithCString
    PyMac_PRECHECK(CFStringCreateWithCString);
#endif
    if (!PyArg_ParseTuple(_args, "sl",
                          &cStr,
                          &encoding))
        return NULL;
    _rv = CFStringCreateWithCString((CFAllocatorRef)NULL,
                                    cStr,
                                    encoding);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFStringCreateWithCharacters(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
    UniChar *chars__in__;
    UniCharCount chars__len__;
    int chars__in_len__;
#ifndef CFStringCreateWithCharacters
    PyMac_PRECHECK(CFStringCreateWithCharacters);
#endif
    if (!PyArg_ParseTuple(_args, "u#",
                          &chars__in__, &chars__in_len__))
        return NULL;
    chars__len__ = chars__in_len__;
    _rv = CFStringCreateWithCharacters((CFAllocatorRef)NULL,
                                       chars__in__, chars__len__);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFStringCreateWithPascalStringNoCopy(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
    Str255 pStr;
    CFStringEncoding encoding;
#ifndef CFStringCreateWithPascalStringNoCopy
    PyMac_PRECHECK(CFStringCreateWithPascalStringNoCopy);
#endif
    if (!PyArg_ParseTuple(_args, "O&l",
                          PyMac_GetStr255, pStr,
                          &encoding))
        return NULL;
    _rv = CFStringCreateWithPascalStringNoCopy((CFAllocatorRef)NULL,
                                               pStr,
                                               encoding,
                                               (CFAllocatorRef)NULL);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFStringCreateWithCStringNoCopy(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
    char* cStr;
    CFStringEncoding encoding;
#ifndef CFStringCreateWithCStringNoCopy
    PyMac_PRECHECK(CFStringCreateWithCStringNoCopy);
#endif
    if (!PyArg_ParseTuple(_args, "sl",
                          &cStr,
                          &encoding))
        return NULL;
    _rv = CFStringCreateWithCStringNoCopy((CFAllocatorRef)NULL,
                                          cStr,
                                          encoding,
                                          (CFAllocatorRef)NULL);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFStringCreateWithCharactersNoCopy(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
    UniChar *chars__in__;
    UniCharCount chars__len__;
    int chars__in_len__;
#ifndef CFStringCreateWithCharactersNoCopy
    PyMac_PRECHECK(CFStringCreateWithCharactersNoCopy);
#endif
    if (!PyArg_ParseTuple(_args, "u#",
                          &chars__in__, &chars__in_len__))
        return NULL;
    chars__len__ = chars__in_len__;
    _rv = CFStringCreateWithCharactersNoCopy((CFAllocatorRef)NULL,
                                             chars__in__, chars__len__,
                                             (CFAllocatorRef)NULL);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFStringCreateMutable(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFMutableStringRef _rv;
    CFIndex maxLength;
#ifndef CFStringCreateMutable
    PyMac_PRECHECK(CFStringCreateMutable);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &maxLength))
        return NULL;
    _rv = CFStringCreateMutable((CFAllocatorRef)NULL,
                                maxLength);
    _res = Py_BuildValue("O&",
                         CFMutableStringRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFStringCreateMutableCopy(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFMutableStringRef _rv;
    CFIndex maxLength;
    CFStringRef theString;
#ifndef CFStringCreateMutableCopy
    PyMac_PRECHECK(CFStringCreateMutableCopy);
#endif
    if (!PyArg_ParseTuple(_args, "lO&",
                          &maxLength,
                          CFStringRefObj_Convert, &theString))
        return NULL;
    _rv = CFStringCreateMutableCopy((CFAllocatorRef)NULL,
                                    maxLength,
                                    theString);
    _res = Py_BuildValue("O&",
                         CFMutableStringRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFStringCreateWithBytes(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
    unsigned char *bytes__in__;
    long bytes__len__;
    int bytes__in_len__;
    CFStringEncoding encoding;
    Boolean isExternalRepresentation;
#ifndef CFStringCreateWithBytes
    PyMac_PRECHECK(CFStringCreateWithBytes);
#endif
    if (!PyArg_ParseTuple(_args, "s#ll",
                          &bytes__in__, &bytes__in_len__,
                          &encoding,
                          &isExternalRepresentation))
        return NULL;
    bytes__len__ = bytes__in_len__;
    _rv = CFStringCreateWithBytes((CFAllocatorRef)NULL,
                                  bytes__in__, bytes__len__,
                                  encoding,
                                  isExternalRepresentation);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFStringGetSystemEncoding(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringEncoding _rv;
#ifndef CFStringGetSystemEncoding
    PyMac_PRECHECK(CFStringGetSystemEncoding);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFStringGetSystemEncoding();
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CF_CFStringGetMaximumSizeForEncoding(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFIndex _rv;
    CFIndex length;
    CFStringEncoding encoding;
#ifndef CFStringGetMaximumSizeForEncoding
    PyMac_PRECHECK(CFStringGetMaximumSizeForEncoding);
#endif
    if (!PyArg_ParseTuple(_args, "ll",
                          &length,
                          &encoding))
        return NULL;
    _rv = CFStringGetMaximumSizeForEncoding(length,
                                            encoding);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CF_CFStringIsEncodingAvailable(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
    CFStringEncoding encoding;
#ifndef CFStringIsEncodingAvailable
    PyMac_PRECHECK(CFStringIsEncodingAvailable);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &encoding))
        return NULL;
    _rv = CFStringIsEncodingAvailable(encoding);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CF_CFStringGetNameOfEncoding(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
    CFStringEncoding encoding;
#ifndef CFStringGetNameOfEncoding
    PyMac_PRECHECK(CFStringGetNameOfEncoding);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &encoding))
        return NULL;
    _rv = CFStringGetNameOfEncoding(encoding);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFStringConvertEncodingToNSStringEncoding(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    UInt32 _rv;
    CFStringEncoding encoding;
#ifndef CFStringConvertEncodingToNSStringEncoding
    PyMac_PRECHECK(CFStringConvertEncodingToNSStringEncoding);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &encoding))
        return NULL;
    _rv = CFStringConvertEncodingToNSStringEncoding(encoding);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CF_CFStringConvertNSStringEncodingToEncoding(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringEncoding _rv;
    UInt32 encoding;
#ifndef CFStringConvertNSStringEncodingToEncoding
    PyMac_PRECHECK(CFStringConvertNSStringEncodingToEncoding);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &encoding))
        return NULL;
    _rv = CFStringConvertNSStringEncodingToEncoding(encoding);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CF_CFStringConvertEncodingToWindowsCodepage(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    UInt32 _rv;
    CFStringEncoding encoding;
#ifndef CFStringConvertEncodingToWindowsCodepage
    PyMac_PRECHECK(CFStringConvertEncodingToWindowsCodepage);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &encoding))
        return NULL;
    _rv = CFStringConvertEncodingToWindowsCodepage(encoding);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CF_CFStringConvertWindowsCodepageToEncoding(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringEncoding _rv;
    UInt32 codepage;
#ifndef CFStringConvertWindowsCodepageToEncoding
    PyMac_PRECHECK(CFStringConvertWindowsCodepageToEncoding);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &codepage))
        return NULL;
    _rv = CFStringConvertWindowsCodepageToEncoding(codepage);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CF_CFStringConvertEncodingToIANACharSetName(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
    CFStringEncoding encoding;
#ifndef CFStringConvertEncodingToIANACharSetName
    PyMac_PRECHECK(CFStringConvertEncodingToIANACharSetName);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &encoding))
        return NULL;
    _rv = CFStringConvertEncodingToIANACharSetName(encoding);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFStringGetMostCompatibleMacStringEncoding(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringEncoding _rv;
    CFStringEncoding encoding;
#ifndef CFStringGetMostCompatibleMacStringEncoding
    PyMac_PRECHECK(CFStringGetMostCompatibleMacStringEncoding);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &encoding))
        return NULL;
    _rv = CFStringGetMostCompatibleMacStringEncoding(encoding);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CF___CFStringMakeConstantString(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFStringRef _rv;
    char* cStr;
#ifndef __CFStringMakeConstantString
    PyMac_PRECHECK(__CFStringMakeConstantString);
#endif
    if (!PyArg_ParseTuple(_args, "s",
                          &cStr))
        return NULL;
    _rv = __CFStringMakeConstantString(cStr);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFURLGetTypeID(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFTypeID _rv;
#ifndef CFURLGetTypeID
    PyMac_PRECHECK(CFURLGetTypeID);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CFURLGetTypeID();
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CF_CFURLCreateWithBytes(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFURLRef _rv;
    unsigned char *URLBytes__in__;
    long URLBytes__len__;
    int URLBytes__in_len__;
    CFStringEncoding encoding;
    CFURLRef baseURL;
#ifndef CFURLCreateWithBytes
    PyMac_PRECHECK(CFURLCreateWithBytes);
#endif
    if (!PyArg_ParseTuple(_args, "s#lO&",
                          &URLBytes__in__, &URLBytes__in_len__,
                          &encoding,
                          OptionalCFURLRefObj_Convert, &baseURL))
        return NULL;
    URLBytes__len__ = URLBytes__in_len__;
    _rv = CFURLCreateWithBytes((CFAllocatorRef)NULL,
                               URLBytes__in__, URLBytes__len__,
                               encoding,
                               baseURL);
    _res = Py_BuildValue("O&",
                         CFURLRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFURLCreateFromFileSystemRepresentation(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFURLRef _rv;
    unsigned char *buffer__in__;
    long buffer__len__;
    int buffer__in_len__;
    Boolean isDirectory;
#ifndef CFURLCreateFromFileSystemRepresentation
    PyMac_PRECHECK(CFURLCreateFromFileSystemRepresentation);
#endif
    if (!PyArg_ParseTuple(_args, "s#l",
                          &buffer__in__, &buffer__in_len__,
                          &isDirectory))
        return NULL;
    buffer__len__ = buffer__in_len__;
    _rv = CFURLCreateFromFileSystemRepresentation((CFAllocatorRef)NULL,
                                                  buffer__in__, buffer__len__,
                                                  isDirectory);
    _res = Py_BuildValue("O&",
                         CFURLRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFURLCreateFromFileSystemRepresentationRelativeToBase(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFURLRef _rv;
    unsigned char *buffer__in__;
    long buffer__len__;
    int buffer__in_len__;
    Boolean isDirectory;
    CFURLRef baseURL;
#ifndef CFURLCreateFromFileSystemRepresentationRelativeToBase
    PyMac_PRECHECK(CFURLCreateFromFileSystemRepresentationRelativeToBase);
#endif
    if (!PyArg_ParseTuple(_args, "s#lO&",
                          &buffer__in__, &buffer__in_len__,
                          &isDirectory,
                          OptionalCFURLRefObj_Convert, &baseURL))
        return NULL;
    buffer__len__ = buffer__in_len__;
    _rv = CFURLCreateFromFileSystemRepresentationRelativeToBase((CFAllocatorRef)NULL,
                                                                buffer__in__, buffer__len__,
                                                                isDirectory,
                                                                baseURL);
    _res = Py_BuildValue("O&",
                         CFURLRefObj_New, _rv);
    return _res;
}

static PyObject *CF_CFURLCreateFromFSRef(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    CFURLRef _rv;
    FSRef fsRef;
#ifndef CFURLCreateFromFSRef
    PyMac_PRECHECK(CFURLCreateFromFSRef);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          PyMac_GetFSRef, &fsRef))
        return NULL;
    _rv = CFURLCreateFromFSRef((CFAllocatorRef)NULL,
                               &fsRef);
    _res = Py_BuildValue("O&",
                         CFURLRefObj_New, _rv);
    return _res;
}

static PyObject *CF_toCF(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;

    CFTypeRef rv;
    CFTypeID typeid;

    if (!PyArg_ParseTuple(_args, "O&", PyCF_Python2CF, &rv))
        return NULL;
    typeid = CFGetTypeID(rv);

    if (typeid == CFStringGetTypeID())
        return Py_BuildValue("O&", CFStringRefObj_New, rv);
    if (typeid == CFArrayGetTypeID())
        return Py_BuildValue("O&", CFArrayRefObj_New, rv);
    if (typeid == CFDictionaryGetTypeID())
        return Py_BuildValue("O&", CFDictionaryRefObj_New, rv);
    if (typeid == CFURLGetTypeID())
        return Py_BuildValue("O&", CFURLRefObj_New, rv);

    _res = Py_BuildValue("O&", CFTypeRefObj_New, rv);
    return _res;

}

static PyMethodDef CF_methods[] = {
    {"__CFRangeMake", (PyCFunction)CF___CFRangeMake, 1,
     PyDoc_STR("(CFIndex loc, CFIndex len) -> (CFRange _rv)")},
    {"CFAllocatorGetTypeID", (PyCFunction)CF_CFAllocatorGetTypeID, 1,
     PyDoc_STR("() -> (CFTypeID _rv)")},
    {"CFAllocatorGetPreferredSizeForSize", (PyCFunction)CF_CFAllocatorGetPreferredSizeForSize, 1,
     PyDoc_STR("(CFIndex size, CFOptionFlags hint) -> (CFIndex _rv)")},
    {"CFCopyTypeIDDescription", (PyCFunction)CF_CFCopyTypeIDDescription, 1,
     PyDoc_STR("(CFTypeID type_id) -> (CFStringRef _rv)")},
    {"CFArrayGetTypeID", (PyCFunction)CF_CFArrayGetTypeID, 1,
     PyDoc_STR("() -> (CFTypeID _rv)")},
    {"CFArrayCreateMutable", (PyCFunction)CF_CFArrayCreateMutable, 1,
     PyDoc_STR("(CFIndex capacity) -> (CFMutableArrayRef _rv)")},
    {"CFArrayCreateMutableCopy", (PyCFunction)CF_CFArrayCreateMutableCopy, 1,
     PyDoc_STR("(CFIndex capacity, CFArrayRef theArray) -> (CFMutableArrayRef _rv)")},
    {"CFDataGetTypeID", (PyCFunction)CF_CFDataGetTypeID, 1,
     PyDoc_STR("() -> (CFTypeID _rv)")},
    {"CFDataCreate", (PyCFunction)CF_CFDataCreate, 1,
     PyDoc_STR("(Buffer bytes) -> (CFDataRef _rv)")},
    {"CFDataCreateWithBytesNoCopy", (PyCFunction)CF_CFDataCreateWithBytesNoCopy, 1,
     PyDoc_STR("(Buffer bytes) -> (CFDataRef _rv)")},
    {"CFDataCreateMutable", (PyCFunction)CF_CFDataCreateMutable, 1,
     PyDoc_STR("(CFIndex capacity) -> (CFMutableDataRef _rv)")},
    {"CFDataCreateMutableCopy", (PyCFunction)CF_CFDataCreateMutableCopy, 1,
     PyDoc_STR("(CFIndex capacity, CFDataRef theData) -> (CFMutableDataRef _rv)")},
    {"CFDictionaryGetTypeID", (PyCFunction)CF_CFDictionaryGetTypeID, 1,
     PyDoc_STR("() -> (CFTypeID _rv)")},
    {"CFDictionaryCreateMutable", (PyCFunction)CF_CFDictionaryCreateMutable, 1,
     PyDoc_STR("(CFIndex capacity) -> (CFMutableDictionaryRef _rv)")},
    {"CFDictionaryCreateMutableCopy", (PyCFunction)CF_CFDictionaryCreateMutableCopy, 1,
     PyDoc_STR("(CFIndex capacity, CFDictionaryRef theDict) -> (CFMutableDictionaryRef _rv)")},
    {"CFPreferencesCopyAppValue", (PyCFunction)CF_CFPreferencesCopyAppValue, 1,
     PyDoc_STR("(CFStringRef key, CFStringRef applicationID) -> (CFTypeRef _rv)")},
    {"CFPreferencesGetAppBooleanValue", (PyCFunction)CF_CFPreferencesGetAppBooleanValue, 1,
     PyDoc_STR("(CFStringRef key, CFStringRef applicationID) -> (Boolean _rv, Boolean keyExistsAndHasValidFormat)")},
    {"CFPreferencesGetAppIntegerValue", (PyCFunction)CF_CFPreferencesGetAppIntegerValue, 1,
     PyDoc_STR("(CFStringRef key, CFStringRef applicationID) -> (CFIndex _rv, Boolean keyExistsAndHasValidFormat)")},
    {"CFPreferencesSetAppValue", (PyCFunction)CF_CFPreferencesSetAppValue, 1,
     PyDoc_STR("(CFStringRef key, CFTypeRef value, CFStringRef applicationID) -> None")},
    {"CFPreferencesAddSuitePreferencesToApp", (PyCFunction)CF_CFPreferencesAddSuitePreferencesToApp, 1,
     PyDoc_STR("(CFStringRef applicationID, CFStringRef suiteID) -> None")},
    {"CFPreferencesRemoveSuitePreferencesFromApp", (PyCFunction)CF_CFPreferencesRemoveSuitePreferencesFromApp, 1,
     PyDoc_STR("(CFStringRef applicationID, CFStringRef suiteID) -> None")},
    {"CFPreferencesAppSynchronize", (PyCFunction)CF_CFPreferencesAppSynchronize, 1,
     PyDoc_STR("(CFStringRef applicationID) -> (Boolean _rv)")},
    {"CFPreferencesCopyValue", (PyCFunction)CF_CFPreferencesCopyValue, 1,
     PyDoc_STR("(CFStringRef key, CFStringRef applicationID, CFStringRef userName, CFStringRef hostName) -> (CFTypeRef _rv)")},
    {"CFPreferencesCopyMultiple", (PyCFunction)CF_CFPreferencesCopyMultiple, 1,
     PyDoc_STR("(CFArrayRef keysToFetch, CFStringRef applicationID, CFStringRef userName, CFStringRef hostName) -> (CFDictionaryRef _rv)")},
    {"CFPreferencesSetValue", (PyCFunction)CF_CFPreferencesSetValue, 1,
     PyDoc_STR("(CFStringRef key, CFTypeRef value, CFStringRef applicationID, CFStringRef userName, CFStringRef hostName) -> None")},
    {"CFPreferencesSetMultiple", (PyCFunction)CF_CFPreferencesSetMultiple, 1,
     PyDoc_STR("(CFDictionaryRef keysToSet, CFArrayRef keysToRemove, CFStringRef applicationID, CFStringRef userName, CFStringRef hostName) -> None")},
    {"CFPreferencesSynchronize", (PyCFunction)CF_CFPreferencesSynchronize, 1,
     PyDoc_STR("(CFStringRef applicationID, CFStringRef userName, CFStringRef hostName) -> (Boolean _rv)")},
    {"CFPreferencesCopyApplicationList", (PyCFunction)CF_CFPreferencesCopyApplicationList, 1,
     PyDoc_STR("(CFStringRef userName, CFStringRef hostName) -> (CFArrayRef _rv)")},
    {"CFPreferencesCopyKeyList", (PyCFunction)CF_CFPreferencesCopyKeyList, 1,
     PyDoc_STR("(CFStringRef applicationID, CFStringRef userName, CFStringRef hostName) -> (CFArrayRef _rv)")},
    {"CFStringGetTypeID", (PyCFunction)CF_CFStringGetTypeID, 1,
     PyDoc_STR("() -> (CFTypeID _rv)")},
    {"CFStringCreateWithPascalString", (PyCFunction)CF_CFStringCreateWithPascalString, 1,
     PyDoc_STR("(Str255 pStr, CFStringEncoding encoding) -> (CFStringRef _rv)")},
    {"CFStringCreateWithCString", (PyCFunction)CF_CFStringCreateWithCString, 1,
     PyDoc_STR("(char* cStr, CFStringEncoding encoding) -> (CFStringRef _rv)")},
    {"CFStringCreateWithCharacters", (PyCFunction)CF_CFStringCreateWithCharacters, 1,
     PyDoc_STR("(Buffer chars) -> (CFStringRef _rv)")},
    {"CFStringCreateWithPascalStringNoCopy", (PyCFunction)CF_CFStringCreateWithPascalStringNoCopy, 1,
     PyDoc_STR("(Str255 pStr, CFStringEncoding encoding) -> (CFStringRef _rv)")},
    {"CFStringCreateWithCStringNoCopy", (PyCFunction)CF_CFStringCreateWithCStringNoCopy, 1,
     PyDoc_STR("(char* cStr, CFStringEncoding encoding) -> (CFStringRef _rv)")},
    {"CFStringCreateWithCharactersNoCopy", (PyCFunction)CF_CFStringCreateWithCharactersNoCopy, 1,
     PyDoc_STR("(Buffer chars) -> (CFStringRef _rv)")},
    {"CFStringCreateMutable", (PyCFunction)CF_CFStringCreateMutable, 1,
     PyDoc_STR("(CFIndex maxLength) -> (CFMutableStringRef _rv)")},
    {"CFStringCreateMutableCopy", (PyCFunction)CF_CFStringCreateMutableCopy, 1,
     PyDoc_STR("(CFIndex maxLength, CFStringRef theString) -> (CFMutableStringRef _rv)")},
    {"CFStringCreateWithBytes", (PyCFunction)CF_CFStringCreateWithBytes, 1,
     PyDoc_STR("(Buffer bytes, CFStringEncoding encoding, Boolean isExternalRepresentation) -> (CFStringRef _rv)")},
    {"CFStringGetSystemEncoding", (PyCFunction)CF_CFStringGetSystemEncoding, 1,
     PyDoc_STR("() -> (CFStringEncoding _rv)")},
    {"CFStringGetMaximumSizeForEncoding", (PyCFunction)CF_CFStringGetMaximumSizeForEncoding, 1,
     PyDoc_STR("(CFIndex length, CFStringEncoding encoding) -> (CFIndex _rv)")},
    {"CFStringIsEncodingAvailable", (PyCFunction)CF_CFStringIsEncodingAvailable, 1,
     PyDoc_STR("(CFStringEncoding encoding) -> (Boolean _rv)")},
    {"CFStringGetNameOfEncoding", (PyCFunction)CF_CFStringGetNameOfEncoding, 1,
     PyDoc_STR("(CFStringEncoding encoding) -> (CFStringRef _rv)")},
    {"CFStringConvertEncodingToNSStringEncoding", (PyCFunction)CF_CFStringConvertEncodingToNSStringEncoding, 1,
     PyDoc_STR("(CFStringEncoding encoding) -> (UInt32 _rv)")},
    {"CFStringConvertNSStringEncodingToEncoding", (PyCFunction)CF_CFStringConvertNSStringEncodingToEncoding, 1,
     PyDoc_STR("(UInt32 encoding) -> (CFStringEncoding _rv)")},
    {"CFStringConvertEncodingToWindowsCodepage", (PyCFunction)CF_CFStringConvertEncodingToWindowsCodepage, 1,
     PyDoc_STR("(CFStringEncoding encoding) -> (UInt32 _rv)")},
    {"CFStringConvertWindowsCodepageToEncoding", (PyCFunction)CF_CFStringConvertWindowsCodepageToEncoding, 1,
     PyDoc_STR("(UInt32 codepage) -> (CFStringEncoding _rv)")},
    {"CFStringConvertEncodingToIANACharSetName", (PyCFunction)CF_CFStringConvertEncodingToIANACharSetName, 1,
     PyDoc_STR("(CFStringEncoding encoding) -> (CFStringRef _rv)")},
    {"CFStringGetMostCompatibleMacStringEncoding", (PyCFunction)CF_CFStringGetMostCompatibleMacStringEncoding, 1,
     PyDoc_STR("(CFStringEncoding encoding) -> (CFStringEncoding _rv)")},
    {"__CFStringMakeConstantString", (PyCFunction)CF___CFStringMakeConstantString, 1,
     PyDoc_STR("(char* cStr) -> (CFStringRef _rv)")},
    {"CFURLGetTypeID", (PyCFunction)CF_CFURLGetTypeID, 1,
     PyDoc_STR("() -> (CFTypeID _rv)")},
    {"CFURLCreateWithBytes", (PyCFunction)CF_CFURLCreateWithBytes, 1,
     PyDoc_STR("(Buffer URLBytes, CFStringEncoding encoding, CFURLRef baseURL) -> (CFURLRef _rv)")},
    {"CFURLCreateFromFileSystemRepresentation", (PyCFunction)CF_CFURLCreateFromFileSystemRepresentation, 1,
     PyDoc_STR("(Buffer buffer, Boolean isDirectory) -> (CFURLRef _rv)")},
    {"CFURLCreateFromFileSystemRepresentationRelativeToBase", (PyCFunction)CF_CFURLCreateFromFileSystemRepresentationRelativeToBase, 1,
     PyDoc_STR("(Buffer buffer, Boolean isDirectory, CFURLRef baseURL) -> (CFURLRef _rv)")},
    {"CFURLCreateFromFSRef", (PyCFunction)CF_CFURLCreateFromFSRef, 1,
     PyDoc_STR("(FSRef fsRef) -> (CFURLRef _rv)")},
    {"toCF", (PyCFunction)CF_toCF, 1,
     PyDoc_STR("(python_object) -> (CF_object)")},
    {NULL, NULL, 0}
};




/* Routines to convert any CF type to/from the corresponding CFxxxObj */
PyObject *CFObj_New(CFTypeRef itself)
{
    if (itself == NULL)
    {
        PyErr_SetString(PyExc_RuntimeError, "cannot wrap NULL");
        return NULL;
    }
    if (CFGetTypeID(itself) == CFArrayGetTypeID()) return CFArrayRefObj_New((CFArrayRef)itself);
    if (CFGetTypeID(itself) == CFDictionaryGetTypeID()) return CFDictionaryRefObj_New((CFDictionaryRef)itself);
    if (CFGetTypeID(itself) == CFDataGetTypeID()) return CFDataRefObj_New((CFDataRef)itself);
    if (CFGetTypeID(itself) == CFStringGetTypeID()) return CFStringRefObj_New((CFStringRef)itself);
    if (CFGetTypeID(itself) == CFURLGetTypeID()) return CFURLRefObj_New((CFURLRef)itself);
    /* XXXX Or should we use PyCF_CF2Python here?? */
    return CFTypeRefObj_New(itself);
}
int CFObj_Convert(PyObject *v, CFTypeRef *p_itself)
{

    if (v == Py_None) { *p_itself = NULL; return 1; }
    /* Check for other CF objects here */

    if (!CFTypeRefObj_Check(v) &&
        !CFArrayRefObj_Check(v) &&
        !CFMutableArrayRefObj_Check(v) &&
        !CFDictionaryRefObj_Check(v) &&
        !CFMutableDictionaryRefObj_Check(v) &&
        !CFDataRefObj_Check(v) &&
        !CFMutableDataRefObj_Check(v) &&
        !CFStringRefObj_Check(v) &&
        !CFMutableStringRefObj_Check(v) &&
        !CFURLRefObj_Check(v) )
    {
        /* XXXX Or should we use PyCF_Python2CF here?? */
        PyErr_SetString(PyExc_TypeError, "CF object required");
        return 0;
    }
    *p_itself = ((CFTypeRefObject *)v)->ob_itself;
    return 1;
}


void init_CF(void)
{
    PyObject *m;
    PyObject *d;



    PyMac_INIT_TOOLBOX_OBJECT_NEW(CFTypeRef, CFObj_New);
    PyMac_INIT_TOOLBOX_OBJECT_CONVERT(CFTypeRef, CFObj_Convert);
    PyMac_INIT_TOOLBOX_OBJECT_NEW(CFTypeRef, CFTypeRefObj_New);
    PyMac_INIT_TOOLBOX_OBJECT_CONVERT(CFTypeRef, CFTypeRefObj_Convert);
    PyMac_INIT_TOOLBOX_OBJECT_NEW(CFStringRef, CFStringRefObj_New);
    PyMac_INIT_TOOLBOX_OBJECT_CONVERT(CFStringRef, CFStringRefObj_Convert);
    PyMac_INIT_TOOLBOX_OBJECT_NEW(CFMutableStringRef, CFMutableStringRefObj_New);
    PyMac_INIT_TOOLBOX_OBJECT_CONVERT(CFMutableStringRef, CFMutableStringRefObj_Convert);
    PyMac_INIT_TOOLBOX_OBJECT_NEW(CFArrayRef, CFArrayRefObj_New);
    PyMac_INIT_TOOLBOX_OBJECT_CONVERT(CFArrayRef, CFArrayRefObj_Convert);
    PyMac_INIT_TOOLBOX_OBJECT_NEW(CFMutableArrayRef, CFMutableArrayRefObj_New);
    PyMac_INIT_TOOLBOX_OBJECT_CONVERT(CFMutableArrayRef, CFMutableArrayRefObj_Convert);
    PyMac_INIT_TOOLBOX_OBJECT_NEW(CFDictionaryRef, CFDictionaryRefObj_New);
    PyMac_INIT_TOOLBOX_OBJECT_CONVERT(CFDictionaryRef, CFDictionaryRefObj_Convert);
    PyMac_INIT_TOOLBOX_OBJECT_NEW(CFMutableDictionaryRef, CFMutableDictionaryRefObj_New);
    PyMac_INIT_TOOLBOX_OBJECT_CONVERT(CFMutableDictionaryRef, CFMutableDictionaryRefObj_Convert);
    PyMac_INIT_TOOLBOX_OBJECT_NEW(CFURLRef, CFURLRefObj_New);
    PyMac_INIT_TOOLBOX_OBJECT_CONVERT(CFURLRef, CFURLRefObj_Convert);


    m = Py_InitModule("_CF", CF_methods);
    d = PyModule_GetDict(m);
    CF_Error = PyMac_GetOSErrException();
    if (CF_Error == NULL ||
        PyDict_SetItemString(d, "Error", CF_Error) != 0)
        return;
    CFTypeRef_Type.ob_type = &PyType_Type;
    if (PyType_Ready(&CFTypeRef_Type) < 0) return;
    Py_INCREF(&CFTypeRef_Type);
    PyModule_AddObject(m, "CFTypeRef", (PyObject *)&CFTypeRef_Type);
    /* Backward-compatible name */
    Py_INCREF(&CFTypeRef_Type);
    PyModule_AddObject(m, "CFTypeRefType", (PyObject *)&CFTypeRef_Type);
    CFArrayRef_Type.ob_type = &PyType_Type;
    CFArrayRef_Type.tp_base = &CFTypeRef_Type;
    if (PyType_Ready(&CFArrayRef_Type) < 0) return;
    Py_INCREF(&CFArrayRef_Type);
    PyModule_AddObject(m, "CFArrayRef", (PyObject *)&CFArrayRef_Type);
    /* Backward-compatible name */
    Py_INCREF(&CFArrayRef_Type);
    PyModule_AddObject(m, "CFArrayRefType", (PyObject *)&CFArrayRef_Type);
    CFMutableArrayRef_Type.ob_type = &PyType_Type;
    CFMutableArrayRef_Type.tp_base = &CFArrayRef_Type;
    if (PyType_Ready(&CFMutableArrayRef_Type) < 0) return;
    Py_INCREF(&CFMutableArrayRef_Type);
    PyModule_AddObject(m, "CFMutableArrayRef", (PyObject *)&CFMutableArrayRef_Type);
    /* Backward-compatible name */
    Py_INCREF(&CFMutableArrayRef_Type);
    PyModule_AddObject(m, "CFMutableArrayRefType", (PyObject *)&CFMutableArrayRef_Type);
    CFDictionaryRef_Type.ob_type = &PyType_Type;
    CFDictionaryRef_Type.tp_base = &CFTypeRef_Type;
    if (PyType_Ready(&CFDictionaryRef_Type) < 0) return;
    Py_INCREF(&CFDictionaryRef_Type);
    PyModule_AddObject(m, "CFDictionaryRef", (PyObject *)&CFDictionaryRef_Type);
    /* Backward-compatible name */
    Py_INCREF(&CFDictionaryRef_Type);
    PyModule_AddObject(m, "CFDictionaryRefType", (PyObject *)&CFDictionaryRef_Type);
    CFMutableDictionaryRef_Type.ob_type = &PyType_Type;
    CFMutableDictionaryRef_Type.tp_base = &CFDictionaryRef_Type;
    if (PyType_Ready(&CFMutableDictionaryRef_Type) < 0) return;
    Py_INCREF(&CFMutableDictionaryRef_Type);
    PyModule_AddObject(m, "CFMutableDictionaryRef", (PyObject *)&CFMutableDictionaryRef_Type);
    /* Backward-compatible name */
    Py_INCREF(&CFMutableDictionaryRef_Type);
    PyModule_AddObject(m, "CFMutableDictionaryRefType", (PyObject *)&CFMutableDictionaryRef_Type);
    CFDataRef_Type.ob_type = &PyType_Type;
    CFDataRef_Type.tp_base = &CFTypeRef_Type;
    if (PyType_Ready(&CFDataRef_Type) < 0) return;
    Py_INCREF(&CFDataRef_Type);
    PyModule_AddObject(m, "CFDataRef", (PyObject *)&CFDataRef_Type);
    /* Backward-compatible name */
    Py_INCREF(&CFDataRef_Type);
    PyModule_AddObject(m, "CFDataRefType", (PyObject *)&CFDataRef_Type);
    CFMutableDataRef_Type.ob_type = &PyType_Type;
    CFMutableDataRef_Type.tp_base = &CFDataRef_Type;
    if (PyType_Ready(&CFMutableDataRef_Type) < 0) return;
    Py_INCREF(&CFMutableDataRef_Type);
    PyModule_AddObject(m, "CFMutableDataRef", (PyObject *)&CFMutableDataRef_Type);
    /* Backward-compatible name */
    Py_INCREF(&CFMutableDataRef_Type);
    PyModule_AddObject(m, "CFMutableDataRefType", (PyObject *)&CFMutableDataRef_Type);
    CFStringRef_Type.ob_type = &PyType_Type;
    CFStringRef_Type.tp_base = &CFTypeRef_Type;
    if (PyType_Ready(&CFStringRef_Type) < 0) return;
    Py_INCREF(&CFStringRef_Type);
    PyModule_AddObject(m, "CFStringRef", (PyObject *)&CFStringRef_Type);
    /* Backward-compatible name */
    Py_INCREF(&CFStringRef_Type);
    PyModule_AddObject(m, "CFStringRefType", (PyObject *)&CFStringRef_Type);
    CFMutableStringRef_Type.ob_type = &PyType_Type;
    CFMutableStringRef_Type.tp_base = &CFStringRef_Type;
    if (PyType_Ready(&CFMutableStringRef_Type) < 0) return;
    Py_INCREF(&CFMutableStringRef_Type);
    PyModule_AddObject(m, "CFMutableStringRef", (PyObject *)&CFMutableStringRef_Type);
    /* Backward-compatible name */
    Py_INCREF(&CFMutableStringRef_Type);
    PyModule_AddObject(m, "CFMutableStringRefType", (PyObject *)&CFMutableStringRef_Type);
    CFURLRef_Type.ob_type = &PyType_Type;
    CFURLRef_Type.tp_base = &CFTypeRef_Type;
    if (PyType_Ready(&CFURLRef_Type) < 0) return;
    Py_INCREF(&CFURLRef_Type);
    PyModule_AddObject(m, "CFURLRef", (PyObject *)&CFURLRef_Type);
    /* Backward-compatible name */
    Py_INCREF(&CFURLRef_Type);
    PyModule_AddObject(m, "CFURLRefType", (PyObject *)&CFURLRef_Type);

#define _STRINGCONST(name) PyModule_AddObject(m, #name, CFStringRefObj_New(name))
    _STRINGCONST(kCFPreferencesAnyApplication);
    _STRINGCONST(kCFPreferencesCurrentApplication);
    _STRINGCONST(kCFPreferencesAnyHost);
    _STRINGCONST(kCFPreferencesCurrentHost);
    _STRINGCONST(kCFPreferencesAnyUser);
    _STRINGCONST(kCFPreferencesCurrentUser);



}

/* ========================= End module _CF ========================= */

