
/* =========================== Module CF ============================ */

#include "Python.h"



#include "macglue.h"
#include "pymactoolbox.h"

/* Macro to test whether a weak-loaded CFM function exists */
#define PyMac_PRECHECK(rtn) do { if ( &rtn == NULL )  {\
    	PyErr_SetString(PyExc_NotImplementedError, \
    	"Not available in this shared library/OS version"); \
    	return NULL; \
    }} while(0)


#ifdef WITHOUT_FRAMEWORKS
#include <CFBase.h>
#include <CFArray.h>
#include <CFData.h>
#include <CFDictionary.h>
#include <CFString.h>
#include <CFURL.h>
#else
#include <CoreFoundation.h>
#endif

/* For now we declare them forward here. They'll go to mactoolbox later */
staticforward PyObject *CFTypeRefObj_New(CFTypeRef);
staticforward int CFTypeRefObj_Convert(PyObject *, CFTypeRef *);
staticforward PyObject *CFStringRefObj_New(CFStringRef);
staticforward int CFStringRefObj_Convert(PyObject *, CFStringRef *);
staticforward PyObject *CFURLRefObj_New(CFURLRef);
staticforward int CFURLRefObj_Convert(PyObject *, CFURLRef *);

staticforward int CFURLRefObj_Convert(PyObject *, CFURLRef *);

// ADD declarations
#ifdef NOTYET_USE_TOOLBOX_OBJECT_GLUE
//extern PyObject *_CFTypeRefObj_New(CFTypeRef);
//extern int _CFTypeRefObj_Convert(PyObject *, CFTypeRef *);

//#define CFTypeRefObj_New _CFTypeRefObj_New
//#define CFTypeRefObj_Convert _CFTypeRefObj_Convert
#endif

/*
** Parse/generate CFRange records
*/
PyObject *CFRange_New(CFRange *itself)
{

	return Py_BuildValue("ll", (long)itself->location, (long)itself->length);
}

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

#define CFTypeRefObj_Check(x) ((x)->ob_type == &CFTypeRef_Type)

typedef struct CFTypeRefObject {
	PyObject_HEAD
	CFTypeRef ob_itself;
	void (*ob_freeit)(CFTypeRef ptr);
} CFTypeRefObject;

PyObject *CFTypeRefObj_New(CFTypeRef itself)
{
	CFTypeRefObject *it;
	if (itself == NULL) return PyMac_Error(resNotFound);
	it = PyObject_NEW(CFTypeRefObject, &CFTypeRef_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	it->ob_freeit = CFRelease;
	return (PyObject *)it;
}
CFTypeRefObj_Convert(PyObject *v, CFTypeRef *p_itself)
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
	}
	PyMem_DEL(self);
}

static PyObject *CFTypeRefObj_CFGetTypeID(CFTypeRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CFTypeID _rv;
	PyMac_PRECHECK(CFGetTypeID);
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
	PyMac_PRECHECK(CFRetain);
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
	PyMac_PRECHECK(CFRelease);
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
	PyMac_PRECHECK(CFGetRetainCount);
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
	PyMac_PRECHECK(CFEqual);
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
	PyMac_PRECHECK(CFHash);
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
	PyMac_PRECHECK(CFCopyDescription);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = CFCopyDescription(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     CFStringRefObj_New, _rv);
	return _res;
}

static PyObject *CFTypeRefObj_CFShow(CFTypeRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PyMac_PRECHECK(CFShow);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	CFShow(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef CFTypeRefObj_methods[] = {
	{"CFGetTypeID", (PyCFunction)CFTypeRefObj_CFGetTypeID, 1,
	 "() -> (CFTypeID _rv)"},
	{"CFRetain", (PyCFunction)CFTypeRefObj_CFRetain, 1,
	 "() -> (CFTypeRef _rv)"},
	{"CFRelease", (PyCFunction)CFTypeRefObj_CFRelease, 1,
	 "() -> None"},
	{"CFGetRetainCount", (PyCFunction)CFTypeRefObj_CFGetRetainCount, 1,
	 "() -> (CFIndex _rv)"},
	{"CFEqual", (PyCFunction)CFTypeRefObj_CFEqual, 1,
	 "(CFTypeRef cf2) -> (Boolean _rv)"},
	{"CFHash", (PyCFunction)CFTypeRefObj_CFHash, 1,
	 "() -> (CFHashCode _rv)"},
	{"CFCopyDescription", (PyCFunction)CFTypeRefObj_CFCopyDescription, 1,
	 "() -> (CFStringRef _rv)"},
	{"CFShow", (PyCFunction)CFTypeRefObj_CFShow, 1,
	 "() -> None"},
	{NULL, NULL, 0}
};

PyMethodChain CFTypeRefObj_chain = { CFTypeRefObj_methods, NULL };

static PyObject *CFTypeRefObj_getattr(CFTypeRefObject *self, char *name)
{
	return Py_FindMethodInChain(&CFTypeRefObj_chain, (PyObject *)self, name);
}

#define CFTypeRefObj_setattr NULL

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
	sprintf(buf, "<CFTypeRef type-%d object at 0x%08.8x for 0x%08.8x>", CFGetTypeID(self->ob_itself), self, self->ob_itself);
	return PyString_FromString(buf);
}

static int CFTypeRefObj_hash(CFTypeRefObject *self)
{
	/* XXXX Or should we use CFHash?? */
	return (int)self->ob_itself;
}

PyTypeObject CFTypeRef_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"CFTypeRef", /*tp_name*/
	sizeof(CFTypeRefObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) CFTypeRefObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) CFTypeRefObj_getattr, /*tp_getattr*/
	(setattrfunc) CFTypeRefObj_setattr, /*tp_setattr*/
	(cmpfunc) CFTypeRefObj_compare, /*tp_compare*/
	(reprfunc) CFTypeRefObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) CFTypeRefObj_hash, /*tp_hash*/
};

/* ------------------- End object type CFTypeRef -------------------- */


/* --------------------- Object type CFArrayRef --------------------- */

PyTypeObject CFArrayRef_Type;

#define CFArrayRefObj_Check(x) ((x)->ob_type == &CFArrayRef_Type)

typedef struct CFArrayRefObject {
	PyObject_HEAD
	CFArrayRef ob_itself;
	void (*ob_freeit)(CFTypeRef ptr);
} CFArrayRefObject;

PyObject *CFArrayRefObj_New(CFArrayRef itself)
{
	CFArrayRefObject *it;
	if (itself == NULL) return PyMac_Error(resNotFound);
	it = PyObject_NEW(CFArrayRefObject, &CFArrayRef_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	it->ob_freeit = CFRelease;
	return (PyObject *)it;
}
CFArrayRefObj_Convert(PyObject *v, CFArrayRef *p_itself)
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
	}
	PyMem_DEL(self);
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
	PyMac_PRECHECK(CFArrayGetCount);
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
	 "() -> (CFArrayRef _rv)"},
	{"CFArrayGetCount", (PyCFunction)CFArrayRefObj_CFArrayGetCount, 1,
	 "() -> (CFIndex _rv)"},
	{"CFStringCreateByCombiningStrings", (PyCFunction)CFArrayRefObj_CFStringCreateByCombiningStrings, 1,
	 "(CFStringRef separatorString) -> (CFStringRef _rv)"},
	{NULL, NULL, 0}
};

PyMethodChain CFArrayRefObj_chain = { CFArrayRefObj_methods, &CFTypeRefObj_chain };

static PyObject *CFArrayRefObj_getattr(CFArrayRefObject *self, char *name)
{
	return Py_FindMethodInChain(&CFArrayRefObj_chain, (PyObject *)self, name);
}

#define CFArrayRefObj_setattr NULL

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
	sprintf(buf, "<CFArrayRef object at 0x%08.8x for 0x%08.8x>", self, self->ob_itself);
	return PyString_FromString(buf);
}

static int CFArrayRefObj_hash(CFArrayRefObject *self)
{
	/* XXXX Or should we use CFHash?? */
	return (int)self->ob_itself;
}

PyTypeObject CFArrayRef_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"CFArrayRef", /*tp_name*/
	sizeof(CFArrayRefObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) CFArrayRefObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) CFArrayRefObj_getattr, /*tp_getattr*/
	(setattrfunc) CFArrayRefObj_setattr, /*tp_setattr*/
	(cmpfunc) CFArrayRefObj_compare, /*tp_compare*/
	(reprfunc) CFArrayRefObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) CFArrayRefObj_hash, /*tp_hash*/
};

/* ------------------- End object type CFArrayRef ------------------- */


/* ----------------- Object type CFMutableArrayRef ------------------ */

PyTypeObject CFMutableArrayRef_Type;

#define CFMutableArrayRefObj_Check(x) ((x)->ob_type == &CFMutableArrayRef_Type)

typedef struct CFMutableArrayRefObject {
	PyObject_HEAD
	CFMutableArrayRef ob_itself;
	void (*ob_freeit)(CFTypeRef ptr);
} CFMutableArrayRefObject;

PyObject *CFMutableArrayRefObj_New(CFMutableArrayRef itself)
{
	CFMutableArrayRefObject *it;
	if (itself == NULL) return PyMac_Error(resNotFound);
	it = PyObject_NEW(CFMutableArrayRefObject, &CFMutableArrayRef_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	it->ob_freeit = CFRelease;
	return (PyObject *)it;
}
CFMutableArrayRefObj_Convert(PyObject *v, CFMutableArrayRef *p_itself)
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
	}
	PyMem_DEL(self);
}

static PyObject *CFMutableArrayRefObj_CFArrayRemoveValueAtIndex(CFMutableArrayRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CFIndex idx;
	PyMac_PRECHECK(CFArrayRemoveValueAtIndex);
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
	PyMac_PRECHECK(CFArrayRemoveAllValues);
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
	PyMac_PRECHECK(CFArrayExchangeValuesAtIndices);
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

static PyMethodDef CFMutableArrayRefObj_methods[] = {
	{"CFArrayRemoveValueAtIndex", (PyCFunction)CFMutableArrayRefObj_CFArrayRemoveValueAtIndex, 1,
	 "(CFIndex idx) -> None"},
	{"CFArrayRemoveAllValues", (PyCFunction)CFMutableArrayRefObj_CFArrayRemoveAllValues, 1,
	 "() -> None"},
	{"CFArrayExchangeValuesAtIndices", (PyCFunction)CFMutableArrayRefObj_CFArrayExchangeValuesAtIndices, 1,
	 "(CFIndex idx1, CFIndex idx2) -> None"},
	{NULL, NULL, 0}
};

PyMethodChain CFMutableArrayRefObj_chain = { CFMutableArrayRefObj_methods, &CFArrayRefObj_chain };

static PyObject *CFMutableArrayRefObj_getattr(CFMutableArrayRefObject *self, char *name)
{
	return Py_FindMethodInChain(&CFMutableArrayRefObj_chain, (PyObject *)self, name);
}

#define CFMutableArrayRefObj_setattr NULL

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
	sprintf(buf, "<CFMutableArrayRef object at 0x%08.8x for 0x%08.8x>", self, self->ob_itself);
	return PyString_FromString(buf);
}

static int CFMutableArrayRefObj_hash(CFMutableArrayRefObject *self)
{
	/* XXXX Or should we use CFHash?? */
	return (int)self->ob_itself;
}

PyTypeObject CFMutableArrayRef_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"CFMutableArrayRef", /*tp_name*/
	sizeof(CFMutableArrayRefObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) CFMutableArrayRefObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) CFMutableArrayRefObj_getattr, /*tp_getattr*/
	(setattrfunc) CFMutableArrayRefObj_setattr, /*tp_setattr*/
	(cmpfunc) CFMutableArrayRefObj_compare, /*tp_compare*/
	(reprfunc) CFMutableArrayRefObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) CFMutableArrayRefObj_hash, /*tp_hash*/
};

/* --------------- End object type CFMutableArrayRef ---------------- */


/* ------------------ Object type CFDictionaryRef ------------------- */

PyTypeObject CFDictionaryRef_Type;

#define CFDictionaryRefObj_Check(x) ((x)->ob_type == &CFDictionaryRef_Type)

typedef struct CFDictionaryRefObject {
	PyObject_HEAD
	CFDictionaryRef ob_itself;
	void (*ob_freeit)(CFTypeRef ptr);
} CFDictionaryRefObject;

PyObject *CFDictionaryRefObj_New(CFDictionaryRef itself)
{
	CFDictionaryRefObject *it;
	if (itself == NULL) return PyMac_Error(resNotFound);
	it = PyObject_NEW(CFDictionaryRefObject, &CFDictionaryRef_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	it->ob_freeit = CFRelease;
	return (PyObject *)it;
}
CFDictionaryRefObj_Convert(PyObject *v, CFDictionaryRef *p_itself)
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
	}
	PyMem_DEL(self);
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
	PyMac_PRECHECK(CFDictionaryGetCount);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = CFDictionaryGetCount(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyMethodDef CFDictionaryRefObj_methods[] = {
	{"CFDictionaryCreateCopy", (PyCFunction)CFDictionaryRefObj_CFDictionaryCreateCopy, 1,
	 "() -> (CFDictionaryRef _rv)"},
	{"CFDictionaryGetCount", (PyCFunction)CFDictionaryRefObj_CFDictionaryGetCount, 1,
	 "() -> (CFIndex _rv)"},
	{NULL, NULL, 0}
};

PyMethodChain CFDictionaryRefObj_chain = { CFDictionaryRefObj_methods, &CFTypeRefObj_chain };

static PyObject *CFDictionaryRefObj_getattr(CFDictionaryRefObject *self, char *name)
{
	return Py_FindMethodInChain(&CFDictionaryRefObj_chain, (PyObject *)self, name);
}

#define CFDictionaryRefObj_setattr NULL

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
	sprintf(buf, "<CFDictionaryRef object at 0x%08.8x for 0x%08.8x>", self, self->ob_itself);
	return PyString_FromString(buf);
}

static int CFDictionaryRefObj_hash(CFDictionaryRefObject *self)
{
	/* XXXX Or should we use CFHash?? */
	return (int)self->ob_itself;
}

PyTypeObject CFDictionaryRef_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"CFDictionaryRef", /*tp_name*/
	sizeof(CFDictionaryRefObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) CFDictionaryRefObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) CFDictionaryRefObj_getattr, /*tp_getattr*/
	(setattrfunc) CFDictionaryRefObj_setattr, /*tp_setattr*/
	(cmpfunc) CFDictionaryRefObj_compare, /*tp_compare*/
	(reprfunc) CFDictionaryRefObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) CFDictionaryRefObj_hash, /*tp_hash*/
};

/* ---------------- End object type CFDictionaryRef ----------------- */


/* --------------- Object type CFMutableDictionaryRef --------------- */

PyTypeObject CFMutableDictionaryRef_Type;

#define CFMutableDictionaryRefObj_Check(x) ((x)->ob_type == &CFMutableDictionaryRef_Type)

typedef struct CFMutableDictionaryRefObject {
	PyObject_HEAD
	CFMutableDictionaryRef ob_itself;
	void (*ob_freeit)(CFTypeRef ptr);
} CFMutableDictionaryRefObject;

PyObject *CFMutableDictionaryRefObj_New(CFMutableDictionaryRef itself)
{
	CFMutableDictionaryRefObject *it;
	if (itself == NULL) return PyMac_Error(resNotFound);
	it = PyObject_NEW(CFMutableDictionaryRefObject, &CFMutableDictionaryRef_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	it->ob_freeit = CFRelease;
	return (PyObject *)it;
}
CFMutableDictionaryRefObj_Convert(PyObject *v, CFMutableDictionaryRef *p_itself)
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
	}
	PyMem_DEL(self);
}

static PyObject *CFMutableDictionaryRefObj_CFDictionaryRemoveAllValues(CFMutableDictionaryRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PyMac_PRECHECK(CFDictionaryRemoveAllValues);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	CFDictionaryRemoveAllValues(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef CFMutableDictionaryRefObj_methods[] = {
	{"CFDictionaryRemoveAllValues", (PyCFunction)CFMutableDictionaryRefObj_CFDictionaryRemoveAllValues, 1,
	 "() -> None"},
	{NULL, NULL, 0}
};

PyMethodChain CFMutableDictionaryRefObj_chain = { CFMutableDictionaryRefObj_methods, &CFDictionaryRefObj_chain };

static PyObject *CFMutableDictionaryRefObj_getattr(CFMutableDictionaryRefObject *self, char *name)
{
	return Py_FindMethodInChain(&CFMutableDictionaryRefObj_chain, (PyObject *)self, name);
}

#define CFMutableDictionaryRefObj_setattr NULL

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
	sprintf(buf, "<CFMutableDictionaryRef object at 0x%08.8x for 0x%08.8x>", self, self->ob_itself);
	return PyString_FromString(buf);
}

static int CFMutableDictionaryRefObj_hash(CFMutableDictionaryRefObject *self)
{
	/* XXXX Or should we use CFHash?? */
	return (int)self->ob_itself;
}

PyTypeObject CFMutableDictionaryRef_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"CFMutableDictionaryRef", /*tp_name*/
	sizeof(CFMutableDictionaryRefObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) CFMutableDictionaryRefObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) CFMutableDictionaryRefObj_getattr, /*tp_getattr*/
	(setattrfunc) CFMutableDictionaryRefObj_setattr, /*tp_setattr*/
	(cmpfunc) CFMutableDictionaryRefObj_compare, /*tp_compare*/
	(reprfunc) CFMutableDictionaryRefObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) CFMutableDictionaryRefObj_hash, /*tp_hash*/
};

/* ------------- End object type CFMutableDictionaryRef ------------- */


/* --------------------- Object type CFDataRef ---------------------- */

PyTypeObject CFDataRef_Type;

#define CFDataRefObj_Check(x) ((x)->ob_type == &CFDataRef_Type)

typedef struct CFDataRefObject {
	PyObject_HEAD
	CFDataRef ob_itself;
	void (*ob_freeit)(CFTypeRef ptr);
} CFDataRefObject;

PyObject *CFDataRefObj_New(CFDataRef itself)
{
	CFDataRefObject *it;
	if (itself == NULL) return PyMac_Error(resNotFound);
	it = PyObject_NEW(CFDataRefObject, &CFDataRef_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	it->ob_freeit = CFRelease;
	return (PyObject *)it;
}
CFDataRefObj_Convert(PyObject *v, CFDataRef *p_itself)
{

	if (v == Py_None) { *p_itself = NULL; return 1; }
	/* Check for other CF objects here */

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
	}
	PyMem_DEL(self);
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
	PyMac_PRECHECK(CFDataGetLength);
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

static PyMethodDef CFDataRefObj_methods[] = {
	{"CFDataCreateCopy", (PyCFunction)CFDataRefObj_CFDataCreateCopy, 1,
	 "() -> (CFDataRef _rv)"},
	{"CFDataGetLength", (PyCFunction)CFDataRefObj_CFDataGetLength, 1,
	 "() -> (CFIndex _rv)"},
	{"CFStringCreateFromExternalRepresentation", (PyCFunction)CFDataRefObj_CFStringCreateFromExternalRepresentation, 1,
	 "(CFStringEncoding encoding) -> (CFStringRef _rv)"},
	{NULL, NULL, 0}
};

PyMethodChain CFDataRefObj_chain = { CFDataRefObj_methods, &CFTypeRefObj_chain };

static PyObject *CFDataRefObj_getattr(CFDataRefObject *self, char *name)
{
	return Py_FindMethodInChain(&CFDataRefObj_chain, (PyObject *)self, name);
}

#define CFDataRefObj_setattr NULL

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
	sprintf(buf, "<CFDataRef object at 0x%08.8x for 0x%08.8x>", self, self->ob_itself);
	return PyString_FromString(buf);
}

static int CFDataRefObj_hash(CFDataRefObject *self)
{
	/* XXXX Or should we use CFHash?? */
	return (int)self->ob_itself;
}

PyTypeObject CFDataRef_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"CFDataRef", /*tp_name*/
	sizeof(CFDataRefObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) CFDataRefObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) CFDataRefObj_getattr, /*tp_getattr*/
	(setattrfunc) CFDataRefObj_setattr, /*tp_setattr*/
	(cmpfunc) CFDataRefObj_compare, /*tp_compare*/
	(reprfunc) CFDataRefObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) CFDataRefObj_hash, /*tp_hash*/
};

/* ------------------- End object type CFDataRef -------------------- */


/* ------------------ Object type CFMutableDataRef ------------------ */

PyTypeObject CFMutableDataRef_Type;

#define CFMutableDataRefObj_Check(x) ((x)->ob_type == &CFMutableDataRef_Type)

typedef struct CFMutableDataRefObject {
	PyObject_HEAD
	CFMutableDataRef ob_itself;
	void (*ob_freeit)(CFTypeRef ptr);
} CFMutableDataRefObject;

PyObject *CFMutableDataRefObj_New(CFMutableDataRef itself)
{
	CFMutableDataRefObject *it;
	if (itself == NULL) return PyMac_Error(resNotFound);
	it = PyObject_NEW(CFMutableDataRefObject, &CFMutableDataRef_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	it->ob_freeit = CFRelease;
	return (PyObject *)it;
}
CFMutableDataRefObj_Convert(PyObject *v, CFMutableDataRef *p_itself)
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
	}
	PyMem_DEL(self);
}

static PyObject *CFMutableDataRefObj_CFDataSetLength(CFMutableDataRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CFIndex length;
	PyMac_PRECHECK(CFDataSetLength);
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
	PyMac_PRECHECK(CFDataIncreaseLength);
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
	PyMac_PRECHECK(CFDataAppendBytes);
	if (!PyArg_ParseTuple(_args, "s#",
	                      &bytes__in__, &bytes__in_len__))
		return NULL;
	bytes__len__ = bytes__in_len__;
	CFDataAppendBytes(_self->ob_itself,
	                  bytes__in__, bytes__len__);
	Py_INCREF(Py_None);
	_res = Py_None;
 bytes__error__: ;
	return _res;
}

static PyObject *CFMutableDataRefObj_CFDataReplaceBytes(CFMutableDataRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CFRange range;
	unsigned char *newBytes__in__;
	long newBytes__len__;
	int newBytes__in_len__;
	PyMac_PRECHECK(CFDataReplaceBytes);
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
 newBytes__error__: ;
	return _res;
}

static PyObject *CFMutableDataRefObj_CFDataDeleteBytes(CFMutableDataRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CFRange range;
	PyMac_PRECHECK(CFDataDeleteBytes);
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
	 "(CFIndex length) -> None"},
	{"CFDataIncreaseLength", (PyCFunction)CFMutableDataRefObj_CFDataIncreaseLength, 1,
	 "(CFIndex extraLength) -> None"},
	{"CFDataAppendBytes", (PyCFunction)CFMutableDataRefObj_CFDataAppendBytes, 1,
	 "(Buffer bytes) -> None"},
	{"CFDataReplaceBytes", (PyCFunction)CFMutableDataRefObj_CFDataReplaceBytes, 1,
	 "(CFRange range, Buffer newBytes) -> None"},
	{"CFDataDeleteBytes", (PyCFunction)CFMutableDataRefObj_CFDataDeleteBytes, 1,
	 "(CFRange range) -> None"},
	{NULL, NULL, 0}
};

PyMethodChain CFMutableDataRefObj_chain = { CFMutableDataRefObj_methods, &CFDataRefObj_chain };

static PyObject *CFMutableDataRefObj_getattr(CFMutableDataRefObject *self, char *name)
{
	return Py_FindMethodInChain(&CFMutableDataRefObj_chain, (PyObject *)self, name);
}

#define CFMutableDataRefObj_setattr NULL

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
	sprintf(buf, "<CFMutableDataRef object at 0x%08.8x for 0x%08.8x>", self, self->ob_itself);
	return PyString_FromString(buf);
}

static int CFMutableDataRefObj_hash(CFMutableDataRefObject *self)
{
	/* XXXX Or should we use CFHash?? */
	return (int)self->ob_itself;
}

PyTypeObject CFMutableDataRef_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"CFMutableDataRef", /*tp_name*/
	sizeof(CFMutableDataRefObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) CFMutableDataRefObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) CFMutableDataRefObj_getattr, /*tp_getattr*/
	(setattrfunc) CFMutableDataRefObj_setattr, /*tp_setattr*/
	(cmpfunc) CFMutableDataRefObj_compare, /*tp_compare*/
	(reprfunc) CFMutableDataRefObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) CFMutableDataRefObj_hash, /*tp_hash*/
};

/* ---------------- End object type CFMutableDataRef ---------------- */


/* -------------------- Object type CFStringRef --------------------- */

PyTypeObject CFStringRef_Type;

#define CFStringRefObj_Check(x) ((x)->ob_type == &CFStringRef_Type)

typedef struct CFStringRefObject {
	PyObject_HEAD
	CFStringRef ob_itself;
	void (*ob_freeit)(CFTypeRef ptr);
} CFStringRefObject;

PyObject *CFStringRefObj_New(CFStringRef itself)
{
	CFStringRefObject *it;
	if (itself == NULL) return PyMac_Error(resNotFound);
	it = PyObject_NEW(CFStringRefObject, &CFStringRef_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	it->ob_freeit = CFRelease;
	return (PyObject *)it;
}
CFStringRefObj_Convert(PyObject *v, CFStringRef *p_itself)
{

	if (v == Py_None) { *p_itself = NULL; return 1; }
	if (PyString_Check(v)) {
	    char *cStr = PyString_AsString(v);
		*p_itself = CFStringCreateWithCString((CFAllocatorRef)NULL, cStr, 0);
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
	}
	PyMem_DEL(self);
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
	PyMac_PRECHECK(CFStringGetLength);
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
	PyMac_PRECHECK(CFStringGetBytes);
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
	PyMac_PRECHECK(CFStringGetSmallestEncoding);
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
	PyMac_PRECHECK(CFStringGetFastestEncoding);
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
	CFStringRef string2;
	CFRange rangeToCompare;
	CFOptionFlags compareOptions;
	PyMac_PRECHECK(CFStringCompareWithOptions);
	if (!PyArg_ParseTuple(_args, "O&O&l",
	                      CFStringRefObj_Convert, &string2,
	                      CFRange_Convert, &rangeToCompare,
	                      &compareOptions))
		return NULL;
	_rv = CFStringCompareWithOptions(_self->ob_itself,
	                                 string2,
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
	CFStringRef string2;
	CFOptionFlags compareOptions;
	PyMac_PRECHECK(CFStringCompare);
	if (!PyArg_ParseTuple(_args, "O&l",
	                      CFStringRefObj_Convert, &string2,
	                      &compareOptions))
		return NULL;
	_rv = CFStringCompare(_self->ob_itself,
	                      string2,
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
	PyMac_PRECHECK(CFStringFindWithOptions);
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
	PyMac_PRECHECK(CFStringFind);
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
	PyMac_PRECHECK(CFStringHasPrefix);
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
	PyMac_PRECHECK(CFStringHasSuffix);
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
	PyMac_PRECHECK(CFStringGetLineBounds);
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
	PyMac_PRECHECK(CFStringGetIntValue);
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
	PyMac_PRECHECK(CFStringGetDoubleValue);
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
	PyMac_PRECHECK(CFStringConvertIANACharSetNameToEncoding);
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
	PyMac_PRECHECK(CFShowStr);
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
	_res = (PyObject *)PyUnicode_FromUnicode(data, size);
	free(data);
	return _res;

}

static PyMethodDef CFStringRefObj_methods[] = {
	{"CFStringCreateWithSubstring", (PyCFunction)CFStringRefObj_CFStringCreateWithSubstring, 1,
	 "(CFRange range) -> (CFStringRef _rv)"},
	{"CFStringCreateCopy", (PyCFunction)CFStringRefObj_CFStringCreateCopy, 1,
	 "() -> (CFStringRef _rv)"},
	{"CFStringGetLength", (PyCFunction)CFStringRefObj_CFStringGetLength, 1,
	 "() -> (CFIndex _rv)"},
	{"CFStringGetBytes", (PyCFunction)CFStringRefObj_CFStringGetBytes, 1,
	 "(CFRange range, CFStringEncoding encoding, UInt8 lossByte, Boolean isExternalRepresentation, CFIndex maxBufLen) -> (CFIndex _rv, UInt8 buffer, CFIndex usedBufLen)"},
	{"CFStringCreateExternalRepresentation", (PyCFunction)CFStringRefObj_CFStringCreateExternalRepresentation, 1,
	 "(CFStringEncoding encoding, UInt8 lossByte) -> (CFDataRef _rv)"},
	{"CFStringGetSmallestEncoding", (PyCFunction)CFStringRefObj_CFStringGetSmallestEncoding, 1,
	 "() -> (CFStringEncoding _rv)"},
	{"CFStringGetFastestEncoding", (PyCFunction)CFStringRefObj_CFStringGetFastestEncoding, 1,
	 "() -> (CFStringEncoding _rv)"},
	{"CFStringCompareWithOptions", (PyCFunction)CFStringRefObj_CFStringCompareWithOptions, 1,
	 "(CFStringRef string2, CFRange rangeToCompare, CFOptionFlags compareOptions) -> (CFComparisonResult _rv)"},
	{"CFStringCompare", (PyCFunction)CFStringRefObj_CFStringCompare, 1,
	 "(CFStringRef string2, CFOptionFlags compareOptions) -> (CFComparisonResult _rv)"},
	{"CFStringFindWithOptions", (PyCFunction)CFStringRefObj_CFStringFindWithOptions, 1,
	 "(CFStringRef stringToFind, CFRange rangeToSearch, CFOptionFlags searchOptions) -> (Boolean _rv, CFRange result)"},
	{"CFStringCreateArrayWithFindResults", (PyCFunction)CFStringRefObj_CFStringCreateArrayWithFindResults, 1,
	 "(CFStringRef stringToFind, CFRange rangeToSearch, CFOptionFlags compareOptions) -> (CFArrayRef _rv)"},
	{"CFStringFind", (PyCFunction)CFStringRefObj_CFStringFind, 1,
	 "(CFStringRef stringToFind, CFOptionFlags compareOptions) -> (CFRange _rv)"},
	{"CFStringHasPrefix", (PyCFunction)CFStringRefObj_CFStringHasPrefix, 1,
	 "(CFStringRef prefix) -> (Boolean _rv)"},
	{"CFStringHasSuffix", (PyCFunction)CFStringRefObj_CFStringHasSuffix, 1,
	 "(CFStringRef suffix) -> (Boolean _rv)"},
	{"CFStringGetLineBounds", (PyCFunction)CFStringRefObj_CFStringGetLineBounds, 1,
	 "(CFRange range) -> (CFIndex lineBeginIndex, CFIndex lineEndIndex, CFIndex contentsEndIndex)"},
	{"CFStringCreateArrayBySeparatingStrings", (PyCFunction)CFStringRefObj_CFStringCreateArrayBySeparatingStrings, 1,
	 "(CFStringRef separatorString) -> (CFArrayRef _rv)"},
	{"CFStringGetIntValue", (PyCFunction)CFStringRefObj_CFStringGetIntValue, 1,
	 "() -> (SInt32 _rv)"},
	{"CFStringGetDoubleValue", (PyCFunction)CFStringRefObj_CFStringGetDoubleValue, 1,
	 "() -> (double _rv)"},
	{"CFStringConvertIANACharSetNameToEncoding", (PyCFunction)CFStringRefObj_CFStringConvertIANACharSetNameToEncoding, 1,
	 "() -> (CFStringEncoding _rv)"},
	{"CFShowStr", (PyCFunction)CFStringRefObj_CFShowStr, 1,
	 "() -> None"},
	{"CFURLCreateWithString", (PyCFunction)CFStringRefObj_CFURLCreateWithString, 1,
	 "(CFURLRef baseURL) -> (CFURLRef _rv)"},
	{"CFURLCreateWithFileSystemPath", (PyCFunction)CFStringRefObj_CFURLCreateWithFileSystemPath, 1,
	 "(CFURLPathStyle pathStyle, Boolean isDirectory) -> (CFURLRef _rv)"},
	{"CFURLCreateStringByReplacingPercentEscapes", (PyCFunction)CFStringRefObj_CFURLCreateStringByReplacingPercentEscapes, 1,
	 "(CFStringRef charactersToLeaveEscaped) -> (CFStringRef _rv)"},
	{"CFStringGetString", (PyCFunction)CFStringRefObj_CFStringGetString, 1,
	 "() -> (string _rv)"},
	{"CFStringGetUnicode", (PyCFunction)CFStringRefObj_CFStringGetUnicode, 1,
	 "() -> (unicode _rv)"},
	{NULL, NULL, 0}
};

PyMethodChain CFStringRefObj_chain = { CFStringRefObj_methods, &CFTypeRefObj_chain };

static PyObject *CFStringRefObj_getattr(CFStringRefObject *self, char *name)
{
	return Py_FindMethodInChain(&CFStringRefObj_chain, (PyObject *)self, name);
}

#define CFStringRefObj_setattr NULL

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
	sprintf(buf, "<CFStringRef object at 0x%08.8x for 0x%08.8x>", self, self->ob_itself);
	return PyString_FromString(buf);
}

static int CFStringRefObj_hash(CFStringRefObject *self)
{
	/* XXXX Or should we use CFHash?? */
	return (int)self->ob_itself;
}

PyTypeObject CFStringRef_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"CFStringRef", /*tp_name*/
	sizeof(CFStringRefObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) CFStringRefObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) CFStringRefObj_getattr, /*tp_getattr*/
	(setattrfunc) CFStringRefObj_setattr, /*tp_setattr*/
	(cmpfunc) CFStringRefObj_compare, /*tp_compare*/
	(reprfunc) CFStringRefObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) CFStringRefObj_hash, /*tp_hash*/
};

/* ------------------ End object type CFStringRef ------------------- */


/* ----------------- Object type CFMutableStringRef ----------------- */

PyTypeObject CFMutableStringRef_Type;

#define CFMutableStringRefObj_Check(x) ((x)->ob_type == &CFMutableStringRef_Type)

typedef struct CFMutableStringRefObject {
	PyObject_HEAD
	CFMutableStringRef ob_itself;
	void (*ob_freeit)(CFTypeRef ptr);
} CFMutableStringRefObject;

PyObject *CFMutableStringRefObj_New(CFMutableStringRef itself)
{
	CFMutableStringRefObject *it;
	if (itself == NULL) return PyMac_Error(resNotFound);
	it = PyObject_NEW(CFMutableStringRefObject, &CFMutableStringRef_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	it->ob_freeit = CFRelease;
	return (PyObject *)it;
}
CFMutableStringRefObj_Convert(PyObject *v, CFMutableStringRef *p_itself)
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
	}
	PyMem_DEL(self);
}

static PyObject *CFMutableStringRefObj_CFStringAppend(CFMutableStringRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CFStringRef appendedString;
	PyMac_PRECHECK(CFStringAppend);
	if (!PyArg_ParseTuple(_args, "O&",
	                      CFStringRefObj_Convert, &appendedString))
		return NULL;
	CFStringAppend(_self->ob_itself,
	               appendedString);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CFMutableStringRefObj_CFStringAppendPascalString(CFMutableStringRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	StringPtr pStr;
	CFStringEncoding encoding;
	PyMac_PRECHECK(CFStringAppendPascalString);
	if (!PyArg_ParseTuple(_args, "O&l",
	                      PyMac_GetStr255, &pStr,
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
	PyMac_PRECHECK(CFStringAppendCString);
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
	PyMac_PRECHECK(CFStringInsert);
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
	PyMac_PRECHECK(CFStringDelete);
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
	PyMac_PRECHECK(CFStringReplace);
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
	PyMac_PRECHECK(CFStringReplaceAll);
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
	PyMac_PRECHECK(CFStringPad);
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
	PyMac_PRECHECK(CFStringTrim);
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
	PyMac_PRECHECK(CFStringTrimWhitespace);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	CFStringTrimWhitespace(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef CFMutableStringRefObj_methods[] = {
	{"CFStringAppend", (PyCFunction)CFMutableStringRefObj_CFStringAppend, 1,
	 "(CFStringRef appendedString) -> None"},
	{"CFStringAppendPascalString", (PyCFunction)CFMutableStringRefObj_CFStringAppendPascalString, 1,
	 "(StringPtr pStr, CFStringEncoding encoding) -> None"},
	{"CFStringAppendCString", (PyCFunction)CFMutableStringRefObj_CFStringAppendCString, 1,
	 "(char* cStr, CFStringEncoding encoding) -> None"},
	{"CFStringInsert", (PyCFunction)CFMutableStringRefObj_CFStringInsert, 1,
	 "(CFIndex idx, CFStringRef insertedStr) -> None"},
	{"CFStringDelete", (PyCFunction)CFMutableStringRefObj_CFStringDelete, 1,
	 "(CFRange range) -> None"},
	{"CFStringReplace", (PyCFunction)CFMutableStringRefObj_CFStringReplace, 1,
	 "(CFRange range, CFStringRef replacement) -> None"},
	{"CFStringReplaceAll", (PyCFunction)CFMutableStringRefObj_CFStringReplaceAll, 1,
	 "(CFStringRef replacement) -> None"},
	{"CFStringPad", (PyCFunction)CFMutableStringRefObj_CFStringPad, 1,
	 "(CFStringRef padString, CFIndex length, CFIndex indexIntoPad) -> None"},
	{"CFStringTrim", (PyCFunction)CFMutableStringRefObj_CFStringTrim, 1,
	 "(CFStringRef trimString) -> None"},
	{"CFStringTrimWhitespace", (PyCFunction)CFMutableStringRefObj_CFStringTrimWhitespace, 1,
	 "() -> None"},
	{NULL, NULL, 0}
};

PyMethodChain CFMutableStringRefObj_chain = { CFMutableStringRefObj_methods, &CFStringRefObj_chain };

static PyObject *CFMutableStringRefObj_getattr(CFMutableStringRefObject *self, char *name)
{
	return Py_FindMethodInChain(&CFMutableStringRefObj_chain, (PyObject *)self, name);
}

#define CFMutableStringRefObj_setattr NULL

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
	sprintf(buf, "<CFMutableStringRef object at 0x%08.8x for 0x%08.8x>", self, self->ob_itself);
	return PyString_FromString(buf);
}

static int CFMutableStringRefObj_hash(CFMutableStringRefObject *self)
{
	/* XXXX Or should we use CFHash?? */
	return (int)self->ob_itself;
}

PyTypeObject CFMutableStringRef_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"CFMutableStringRef", /*tp_name*/
	sizeof(CFMutableStringRefObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) CFMutableStringRefObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) CFMutableStringRefObj_getattr, /*tp_getattr*/
	(setattrfunc) CFMutableStringRefObj_setattr, /*tp_setattr*/
	(cmpfunc) CFMutableStringRefObj_compare, /*tp_compare*/
	(reprfunc) CFMutableStringRefObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) CFMutableStringRefObj_hash, /*tp_hash*/
};

/* --------------- End object type CFMutableStringRef --------------- */


/* ---------------------- Object type CFURLRef ---------------------- */

PyTypeObject CFURLRef_Type;

#define CFURLRefObj_Check(x) ((x)->ob_type == &CFURLRef_Type)

typedef struct CFURLRefObject {
	PyObject_HEAD
	CFURLRef ob_itself;
	void (*ob_freeit)(CFTypeRef ptr);
} CFURLRefObject;

PyObject *CFURLRefObj_New(CFURLRef itself)
{
	CFURLRefObject *it;
	if (itself == NULL) return PyMac_Error(resNotFound);
	it = PyObject_NEW(CFURLRefObject, &CFURLRef_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	it->ob_freeit = CFRelease;
	return (PyObject *)it;
}
CFURLRefObj_Convert(PyObject *v, CFURLRef *p_itself)
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
	}
	PyMem_DEL(self);
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

static PyObject *CFURLRefObj_CFURLCopyAbsoluteURL(CFURLRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CFURLRef _rv;
	PyMac_PRECHECK(CFURLCopyAbsoluteURL);
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
	PyMac_PRECHECK(CFURLGetString);
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
	PyMac_PRECHECK(CFURLGetBaseURL);
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
	PyMac_PRECHECK(CFURLCanBeDecomposed);
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
	PyMac_PRECHECK(CFURLCopyScheme);
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
	PyMac_PRECHECK(CFURLCopyNetLocation);
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
	PyMac_PRECHECK(CFURLCopyPath);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = CFURLCopyPath(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     CFStringRefObj_New, _rv);
	return _res;
}

static PyObject *CFURLRefObj_CFURLHasDirectoryPath(CFURLRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	PyMac_PRECHECK(CFURLHasDirectoryPath);
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
	PyMac_PRECHECK(CFURLCopyResourceSpecifier);
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
	PyMac_PRECHECK(CFURLCopyHostName);
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
	PyMac_PRECHECK(CFURLGetPortNumber);
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
	PyMac_PRECHECK(CFURLCopyUserName);
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
	PyMac_PRECHECK(CFURLCopyPassword);
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
	PyMac_PRECHECK(CFURLCopyParameterString);
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
	PyMac_PRECHECK(CFURLCopyQueryString);
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
	PyMac_PRECHECK(CFURLCopyFragment);
	if (!PyArg_ParseTuple(_args, "O&",
	                      CFStringRefObj_Convert, &charactersToLeaveEscaped))
		return NULL;
	_rv = CFURLCopyFragment(_self->ob_itself,
	                        charactersToLeaveEscaped);
	_res = Py_BuildValue("O&",
	                     CFStringRefObj_New, _rv);
	return _res;
}

static PyMethodDef CFURLRefObj_methods[] = {
	{"CFURLCreateData", (PyCFunction)CFURLRefObj_CFURLCreateData, 1,
	 "(CFStringEncoding encoding, Boolean escapeWhitespace) -> (CFDataRef _rv)"},
	{"CFURLCopyAbsoluteURL", (PyCFunction)CFURLRefObj_CFURLCopyAbsoluteURL, 1,
	 "() -> (CFURLRef _rv)"},
	{"CFURLGetString", (PyCFunction)CFURLRefObj_CFURLGetString, 1,
	 "() -> (CFStringRef _rv)"},
	{"CFURLGetBaseURL", (PyCFunction)CFURLRefObj_CFURLGetBaseURL, 1,
	 "() -> (CFURLRef _rv)"},
	{"CFURLCanBeDecomposed", (PyCFunction)CFURLRefObj_CFURLCanBeDecomposed, 1,
	 "() -> (Boolean _rv)"},
	{"CFURLCopyScheme", (PyCFunction)CFURLRefObj_CFURLCopyScheme, 1,
	 "() -> (CFStringRef _rv)"},
	{"CFURLCopyNetLocation", (PyCFunction)CFURLRefObj_CFURLCopyNetLocation, 1,
	 "() -> (CFStringRef _rv)"},
	{"CFURLCopyPath", (PyCFunction)CFURLRefObj_CFURLCopyPath, 1,
	 "() -> (CFStringRef _rv)"},
	{"CFURLHasDirectoryPath", (PyCFunction)CFURLRefObj_CFURLHasDirectoryPath, 1,
	 "() -> (Boolean _rv)"},
	{"CFURLCopyResourceSpecifier", (PyCFunction)CFURLRefObj_CFURLCopyResourceSpecifier, 1,
	 "() -> (CFStringRef _rv)"},
	{"CFURLCopyHostName", (PyCFunction)CFURLRefObj_CFURLCopyHostName, 1,
	 "() -> (CFStringRef _rv)"},
	{"CFURLGetPortNumber", (PyCFunction)CFURLRefObj_CFURLGetPortNumber, 1,
	 "() -> (SInt32 _rv)"},
	{"CFURLCopyUserName", (PyCFunction)CFURLRefObj_CFURLCopyUserName, 1,
	 "() -> (CFStringRef _rv)"},
	{"CFURLCopyPassword", (PyCFunction)CFURLRefObj_CFURLCopyPassword, 1,
	 "() -> (CFStringRef _rv)"},
	{"CFURLCopyParameterString", (PyCFunction)CFURLRefObj_CFURLCopyParameterString, 1,
	 "(CFStringRef charactersToLeaveEscaped) -> (CFStringRef _rv)"},
	{"CFURLCopyQueryString", (PyCFunction)CFURLRefObj_CFURLCopyQueryString, 1,
	 "(CFStringRef charactersToLeaveEscaped) -> (CFStringRef _rv)"},
	{"CFURLCopyFragment", (PyCFunction)CFURLRefObj_CFURLCopyFragment, 1,
	 "(CFStringRef charactersToLeaveEscaped) -> (CFStringRef _rv)"},
	{NULL, NULL, 0}
};

PyMethodChain CFURLRefObj_chain = { CFURLRefObj_methods, &CFTypeRefObj_chain };

static PyObject *CFURLRefObj_getattr(CFURLRefObject *self, char *name)
{
	return Py_FindMethodInChain(&CFURLRefObj_chain, (PyObject *)self, name);
}

#define CFURLRefObj_setattr NULL

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
	sprintf(buf, "<CFURL object at 0x%08.8x for 0x%08.8x>", self, self->ob_itself);
	return PyString_FromString(buf);
}

static int CFURLRefObj_hash(CFURLRefObject *self)
{
	/* XXXX Or should we use CFHash?? */
	return (int)self->ob_itself;
}

PyTypeObject CFURLRef_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"CFURLRef", /*tp_name*/
	sizeof(CFURLRefObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) CFURLRefObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) CFURLRefObj_getattr, /*tp_getattr*/
	(setattrfunc) CFURLRefObj_setattr, /*tp_setattr*/
	(cmpfunc) CFURLRefObj_compare, /*tp_compare*/
	(reprfunc) CFURLRefObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) CFURLRefObj_hash, /*tp_hash*/
};

/* -------------------- End object type CFURLRef -------------------- */


static PyObject *CF_CFAllocatorGetTypeID(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CFTypeID _rv;
	PyMac_PRECHECK(CFAllocatorGetTypeID);
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
	PyMac_PRECHECK(CFAllocatorGetPreferredSizeForSize);
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
	CFTypeID theType;
	PyMac_PRECHECK(CFCopyTypeIDDescription);
	if (!PyArg_ParseTuple(_args, "l",
	                      &theType))
		return NULL;
	_rv = CFCopyTypeIDDescription(theType);
	_res = Py_BuildValue("O&",
	                     CFStringRefObj_New, _rv);
	return _res;
}

static PyObject *CF_CFArrayGetTypeID(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CFTypeID _rv;
	PyMac_PRECHECK(CFArrayGetTypeID);
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
	PyMac_PRECHECK(CFArrayCreateMutable);
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
	CFArrayRef srcArray;
	PyMac_PRECHECK(CFArrayCreateMutableCopy);
	if (!PyArg_ParseTuple(_args, "lO&",
	                      &capacity,
	                      CFArrayRefObj_Convert, &srcArray))
		return NULL;
	_rv = CFArrayCreateMutableCopy((CFAllocatorRef)NULL,
	                               capacity,
	                               srcArray);
	_res = Py_BuildValue("O&",
	                     CFMutableArrayRefObj_New, _rv);
	return _res;
}

static PyObject *CF_CFDataGetTypeID(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CFTypeID _rv;
	PyMac_PRECHECK(CFDataGetTypeID);
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
	PyMac_PRECHECK(CFDataCreate);
	if (!PyArg_ParseTuple(_args, "s#",
	                      &bytes__in__, &bytes__in_len__))
		return NULL;
	bytes__len__ = bytes__in_len__;
	_rv = CFDataCreate((CFAllocatorRef)NULL,
	                   bytes__in__, bytes__len__);
	_res = Py_BuildValue("O&",
	                     CFDataRefObj_New, _rv);
 bytes__error__: ;
	return _res;
}

static PyObject *CF_CFDataCreateWithBytesNoCopy(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CFDataRef _rv;
	unsigned char *bytes__in__;
	long bytes__len__;
	int bytes__in_len__;
	PyMac_PRECHECK(CFDataCreateWithBytesNoCopy);
	if (!PyArg_ParseTuple(_args, "s#",
	                      &bytes__in__, &bytes__in_len__))
		return NULL;
	bytes__len__ = bytes__in_len__;
	_rv = CFDataCreateWithBytesNoCopy((CFAllocatorRef)NULL,
	                                  bytes__in__, bytes__len__,
	                                  (CFAllocatorRef)NULL);
	_res = Py_BuildValue("O&",
	                     CFDataRefObj_New, _rv);
 bytes__error__: ;
	return _res;
}

static PyObject *CF_CFDataCreateMutable(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CFMutableDataRef _rv;
	CFIndex capacity;
	PyMac_PRECHECK(CFDataCreateMutable);
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
	CFDataRef data;
	PyMac_PRECHECK(CFDataCreateMutableCopy);
	if (!PyArg_ParseTuple(_args, "lO&",
	                      &capacity,
	                      CFDataRefObj_Convert, &data))
		return NULL;
	_rv = CFDataCreateMutableCopy((CFAllocatorRef)NULL,
	                              capacity,
	                              data);
	_res = Py_BuildValue("O&",
	                     CFMutableDataRefObj_New, _rv);
	return _res;
}

static PyObject *CF_CFDictionaryGetTypeID(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CFTypeID _rv;
	PyMac_PRECHECK(CFDictionaryGetTypeID);
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
	PyMac_PRECHECK(CFDictionaryCreateMutable);
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
	CFDictionaryRef dict;
	PyMac_PRECHECK(CFDictionaryCreateMutableCopy);
	if (!PyArg_ParseTuple(_args, "lO&",
	                      &capacity,
	                      CFDictionaryRefObj_Convert, &dict))
		return NULL;
	_rv = CFDictionaryCreateMutableCopy((CFAllocatorRef)NULL,
	                                    capacity,
	                                    dict);
	_res = Py_BuildValue("O&",
	                     CFMutableDictionaryRefObj_New, _rv);
	return _res;
}

static PyObject *CF_CFStringGetTypeID(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CFTypeID _rv;
	PyMac_PRECHECK(CFStringGetTypeID);
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
	StringPtr pStr;
	CFStringEncoding encoding;
	PyMac_PRECHECK(CFStringCreateWithPascalString);
	if (!PyArg_ParseTuple(_args, "O&l",
	                      PyMac_GetStr255, &pStr,
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
	PyMac_PRECHECK(CFStringCreateWithCString);
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

static PyObject *CF_CFStringCreateWithPascalStringNoCopy(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CFStringRef _rv;
	StringPtr pStr;
	CFStringEncoding encoding;
	PyMac_PRECHECK(CFStringCreateWithPascalStringNoCopy);
	if (!PyArg_ParseTuple(_args, "O&l",
	                      PyMac_GetStr255, &pStr,
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
	PyMac_PRECHECK(CFStringCreateWithCStringNoCopy);
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

static PyObject *CF_CFStringCreateMutable(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CFMutableStringRef _rv;
	CFIndex maxLength;
	PyMac_PRECHECK(CFStringCreateMutable);
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
	PyMac_PRECHECK(CFStringCreateMutableCopy);
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
	PyMac_PRECHECK(CFStringCreateWithBytes);
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
 bytes__error__: ;
	return _res;
}

static PyObject *CF_CFStringGetSystemEncoding(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CFStringEncoding _rv;
	PyMac_PRECHECK(CFStringGetSystemEncoding);
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
	PyMac_PRECHECK(CFStringGetMaximumSizeForEncoding);
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
	PyMac_PRECHECK(CFStringIsEncodingAvailable);
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
	PyMac_PRECHECK(CFStringGetNameOfEncoding);
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
	PyMac_PRECHECK(CFStringConvertEncodingToNSStringEncoding);
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
	PyMac_PRECHECK(CFStringConvertNSStringEncodingToEncoding);
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
	PyMac_PRECHECK(CFStringConvertEncodingToWindowsCodepage);
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
	PyMac_PRECHECK(CFStringConvertWindowsCodepageToEncoding);
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
	PyMac_PRECHECK(CFStringConvertEncodingToIANACharSetName);
	if (!PyArg_ParseTuple(_args, "l",
	                      &encoding))
		return NULL;
	_rv = CFStringConvertEncodingToIANACharSetName(encoding);
	_res = Py_BuildValue("O&",
	                     CFStringRefObj_New, _rv);
	return _res;
}

static PyObject *CF___CFStringMakeConstantString(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CFStringRef _rv;
	char* cStr;
	PyMac_PRECHECK(__CFStringMakeConstantString);
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
	PyMac_PRECHECK(CFURLGetTypeID);
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
	PyMac_PRECHECK(CFURLCreateWithBytes);
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
 URLBytes__error__: ;
	return _res;
}

static PyMethodDef CF_methods[] = {
	{"CFAllocatorGetTypeID", (PyCFunction)CF_CFAllocatorGetTypeID, 1,
	 "() -> (CFTypeID _rv)"},
	{"CFAllocatorGetPreferredSizeForSize", (PyCFunction)CF_CFAllocatorGetPreferredSizeForSize, 1,
	 "(CFIndex size, CFOptionFlags hint) -> (CFIndex _rv)"},
	{"CFCopyTypeIDDescription", (PyCFunction)CF_CFCopyTypeIDDescription, 1,
	 "(CFTypeID theType) -> (CFStringRef _rv)"},
	{"CFArrayGetTypeID", (PyCFunction)CF_CFArrayGetTypeID, 1,
	 "() -> (CFTypeID _rv)"},
	{"CFArrayCreateMutable", (PyCFunction)CF_CFArrayCreateMutable, 1,
	 "(CFIndex capacity) -> (CFMutableArrayRef _rv)"},
	{"CFArrayCreateMutableCopy", (PyCFunction)CF_CFArrayCreateMutableCopy, 1,
	 "(CFIndex capacity, CFArrayRef srcArray) -> (CFMutableArrayRef _rv)"},
	{"CFDataGetTypeID", (PyCFunction)CF_CFDataGetTypeID, 1,
	 "() -> (CFTypeID _rv)"},
	{"CFDataCreate", (PyCFunction)CF_CFDataCreate, 1,
	 "(Buffer bytes) -> (CFDataRef _rv)"},
	{"CFDataCreateWithBytesNoCopy", (PyCFunction)CF_CFDataCreateWithBytesNoCopy, 1,
	 "(Buffer bytes) -> (CFDataRef _rv)"},
	{"CFDataCreateMutable", (PyCFunction)CF_CFDataCreateMutable, 1,
	 "(CFIndex capacity) -> (CFMutableDataRef _rv)"},
	{"CFDataCreateMutableCopy", (PyCFunction)CF_CFDataCreateMutableCopy, 1,
	 "(CFIndex capacity, CFDataRef data) -> (CFMutableDataRef _rv)"},
	{"CFDictionaryGetTypeID", (PyCFunction)CF_CFDictionaryGetTypeID, 1,
	 "() -> (CFTypeID _rv)"},
	{"CFDictionaryCreateMutable", (PyCFunction)CF_CFDictionaryCreateMutable, 1,
	 "(CFIndex capacity) -> (CFMutableDictionaryRef _rv)"},
	{"CFDictionaryCreateMutableCopy", (PyCFunction)CF_CFDictionaryCreateMutableCopy, 1,
	 "(CFIndex capacity, CFDictionaryRef dict) -> (CFMutableDictionaryRef _rv)"},
	{"CFStringGetTypeID", (PyCFunction)CF_CFStringGetTypeID, 1,
	 "() -> (CFTypeID _rv)"},
	{"CFStringCreateWithPascalString", (PyCFunction)CF_CFStringCreateWithPascalString, 1,
	 "(StringPtr pStr, CFStringEncoding encoding) -> (CFStringRef _rv)"},
	{"CFStringCreateWithCString", (PyCFunction)CF_CFStringCreateWithCString, 1,
	 "(char* cStr, CFStringEncoding encoding) -> (CFStringRef _rv)"},
	{"CFStringCreateWithPascalStringNoCopy", (PyCFunction)CF_CFStringCreateWithPascalStringNoCopy, 1,
	 "(StringPtr pStr, CFStringEncoding encoding) -> (CFStringRef _rv)"},
	{"CFStringCreateWithCStringNoCopy", (PyCFunction)CF_CFStringCreateWithCStringNoCopy, 1,
	 "(char* cStr, CFStringEncoding encoding) -> (CFStringRef _rv)"},
	{"CFStringCreateMutable", (PyCFunction)CF_CFStringCreateMutable, 1,
	 "(CFIndex maxLength) -> (CFMutableStringRef _rv)"},
	{"CFStringCreateMutableCopy", (PyCFunction)CF_CFStringCreateMutableCopy, 1,
	 "(CFIndex maxLength, CFStringRef theString) -> (CFMutableStringRef _rv)"},
	{"CFStringCreateWithBytes", (PyCFunction)CF_CFStringCreateWithBytes, 1,
	 "(Buffer bytes, CFStringEncoding encoding, Boolean isExternalRepresentation) -> (CFStringRef _rv)"},
	{"CFStringGetSystemEncoding", (PyCFunction)CF_CFStringGetSystemEncoding, 1,
	 "() -> (CFStringEncoding _rv)"},
	{"CFStringGetMaximumSizeForEncoding", (PyCFunction)CF_CFStringGetMaximumSizeForEncoding, 1,
	 "(CFIndex length, CFStringEncoding encoding) -> (CFIndex _rv)"},
	{"CFStringIsEncodingAvailable", (PyCFunction)CF_CFStringIsEncodingAvailable, 1,
	 "(CFStringEncoding encoding) -> (Boolean _rv)"},
	{"CFStringGetNameOfEncoding", (PyCFunction)CF_CFStringGetNameOfEncoding, 1,
	 "(CFStringEncoding encoding) -> (CFStringRef _rv)"},
	{"CFStringConvertEncodingToNSStringEncoding", (PyCFunction)CF_CFStringConvertEncodingToNSStringEncoding, 1,
	 "(CFStringEncoding encoding) -> (UInt32 _rv)"},
	{"CFStringConvertNSStringEncodingToEncoding", (PyCFunction)CF_CFStringConvertNSStringEncodingToEncoding, 1,
	 "(UInt32 encoding) -> (CFStringEncoding _rv)"},
	{"CFStringConvertEncodingToWindowsCodepage", (PyCFunction)CF_CFStringConvertEncodingToWindowsCodepage, 1,
	 "(CFStringEncoding encoding) -> (UInt32 _rv)"},
	{"CFStringConvertWindowsCodepageToEncoding", (PyCFunction)CF_CFStringConvertWindowsCodepageToEncoding, 1,
	 "(UInt32 codepage) -> (CFStringEncoding _rv)"},
	{"CFStringConvertEncodingToIANACharSetName", (PyCFunction)CF_CFStringConvertEncodingToIANACharSetName, 1,
	 "(CFStringEncoding encoding) -> (CFStringRef _rv)"},
	{"__CFStringMakeConstantString", (PyCFunction)CF___CFStringMakeConstantString, 1,
	 "(char* cStr) -> (CFStringRef _rv)"},
	{"CFURLGetTypeID", (PyCFunction)CF_CFURLGetTypeID, 1,
	 "() -> (CFTypeID _rv)"},
	{"CFURLCreateWithBytes", (PyCFunction)CF_CFURLCreateWithBytes, 1,
	 "(Buffer URLBytes, CFStringEncoding encoding, CFURLRef baseURL) -> (CFURLRef _rv)"},
	{NULL, NULL, 0}
};




void initCF(void)
{
	PyObject *m;
	PyObject *d;



	//	PyMac_INIT_TOOLBOX_OBJECT_NEW(Track, TrackObj_New);
	//	PyMac_INIT_TOOLBOX_OBJECT_CONVERT(Track, TrackObj_Convert);


	m = Py_InitModule("CF", CF_methods);
	d = PyModule_GetDict(m);
	CF_Error = PyMac_GetOSErrException();
	if (CF_Error == NULL ||
	    PyDict_SetItemString(d, "Error", CF_Error) != 0)
		return;
	CFTypeRef_Type.ob_type = &PyType_Type;
	Py_INCREF(&CFTypeRef_Type);
	if (PyDict_SetItemString(d, "CFTypeRefType", (PyObject *)&CFTypeRef_Type) != 0)
		Py_FatalError("can't initialize CFTypeRefType");
	CFArrayRef_Type.ob_type = &PyType_Type;
	Py_INCREF(&CFArrayRef_Type);
	if (PyDict_SetItemString(d, "CFArrayRefType", (PyObject *)&CFArrayRef_Type) != 0)
		Py_FatalError("can't initialize CFArrayRefType");
	CFMutableArrayRef_Type.ob_type = &PyType_Type;
	Py_INCREF(&CFMutableArrayRef_Type);
	if (PyDict_SetItemString(d, "CFMutableArrayRefType", (PyObject *)&CFMutableArrayRef_Type) != 0)
		Py_FatalError("can't initialize CFMutableArrayRefType");
	CFDictionaryRef_Type.ob_type = &PyType_Type;
	Py_INCREF(&CFDictionaryRef_Type);
	if (PyDict_SetItemString(d, "CFDictionaryRefType", (PyObject *)&CFDictionaryRef_Type) != 0)
		Py_FatalError("can't initialize CFDictionaryRefType");
	CFMutableDictionaryRef_Type.ob_type = &PyType_Type;
	Py_INCREF(&CFMutableDictionaryRef_Type);
	if (PyDict_SetItemString(d, "CFMutableDictionaryRefType", (PyObject *)&CFMutableDictionaryRef_Type) != 0)
		Py_FatalError("can't initialize CFMutableDictionaryRefType");
	CFDataRef_Type.ob_type = &PyType_Type;
	Py_INCREF(&CFDataRef_Type);
	if (PyDict_SetItemString(d, "CFDataRefType", (PyObject *)&CFDataRef_Type) != 0)
		Py_FatalError("can't initialize CFDataRefType");
	CFMutableDataRef_Type.ob_type = &PyType_Type;
	Py_INCREF(&CFMutableDataRef_Type);
	if (PyDict_SetItemString(d, "CFMutableDataRefType", (PyObject *)&CFMutableDataRef_Type) != 0)
		Py_FatalError("can't initialize CFMutableDataRefType");
	CFStringRef_Type.ob_type = &PyType_Type;
	Py_INCREF(&CFStringRef_Type);
	if (PyDict_SetItemString(d, "CFStringRefType", (PyObject *)&CFStringRef_Type) != 0)
		Py_FatalError("can't initialize CFStringRefType");
	CFMutableStringRef_Type.ob_type = &PyType_Type;
	Py_INCREF(&CFMutableStringRef_Type);
	if (PyDict_SetItemString(d, "CFMutableStringRefType", (PyObject *)&CFMutableStringRef_Type) != 0)
		Py_FatalError("can't initialize CFMutableStringRefType");
	CFURLRef_Type.ob_type = &PyType_Type;
	Py_INCREF(&CFURLRef_Type);
	if (PyDict_SetItemString(d, "CFURLRefType", (PyObject *)&CFURLRef_Type) != 0)
		Py_FatalError("can't initialize CFURLRefType");
}

/* ========================= End module CF ========================== */

