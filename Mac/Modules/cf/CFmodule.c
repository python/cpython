
/* =========================== Module CF ============================ */

#include "Python.h"



#include "macglue.h"
#include "pymactoolbox.h"

#ifdef WITHOUT_FRAMEWORKS
#include <CoreFoundation.h>
#else
#include <CoreFoundation.h>
#endif

/* For now we declare them forward here. They'll go to mactoolbox later */
extern PyObject *CFTypeRefObj_New(CFTypeRef);
extern int CFTypeRefObj_Convert(PyObject *, CFTypeRef *);
extern PyObject *CFStringRefObj_New(CFStringRef);
extern int CFStringRefObj_Convert(PyObject *, CFStringRef *);

#ifdef NOTYET_USE_TOOLBOX_OBJECT_GLUE
//extern PyObject *_CFTypeRefObj_New(CFTypeRef);
//extern int _CFTypeRefObj_Convert(PyObject *, CFTypeRef *);

//#define CFTypeRefObj_New _CFTypeRefObj_New
//#define CFTypeRefObj_Convert _CFTypeRefObj_Convert
#endif


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
	CFRetain(itself);
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
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = CFCopyDescription(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     CFStringRefObj_New, _rv);
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
	CFRetain(itself);
	it = PyObject_NEW(CFStringRefObject, &CFStringRef_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	it->ob_freeit = CFRelease;
	return (PyObject *)it;
}
CFStringRefObj_Convert(PyObject *v, CFStringRef *p_itself)
{

	if (v == Py_None) { *p_itself = NULL; return 1; }
	/* Check for other CF objects here */

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

static PyMethodDef CFStringRefObj_methods[] = {
	{NULL, NULL, 0}
};

PyMethodChain CFStringRefObj_chain = { CFStringRefObj_methods, NULL };

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
	sprintf(buf, "<CFTypeRef type-%d object at 0x%08.8x for 0x%08.8x>", CFGetTypeID(self->ob_itself), self, self->ob_itself);
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


static PyObject *CF_CFAllocatorGetTypeID(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CFTypeID _rv;
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
	if (!PyArg_ParseTuple(_args, "l",
	                      &theType))
		return NULL;
	_rv = CFCopyTypeIDDescription(theType);
	_res = Py_BuildValue("O&",
	                     CFStringRefObj_New, _rv);
	return _res;
}

static PyMethodDef CF_methods[] = {
	{"CFAllocatorGetTypeID", (PyCFunction)CF_CFAllocatorGetTypeID, 1,
	 "() -> (CFTypeID _rv)"},
	{"CFAllocatorGetPreferredSizeForSize", (PyCFunction)CF_CFAllocatorGetPreferredSizeForSize, 1,
	 "(CFIndex size, CFOptionFlags hint) -> (CFIndex _rv)"},
	{"CFCopyTypeIDDescription", (PyCFunction)CF_CFCopyTypeIDDescription, 1,
	 "(CFTypeID theType) -> (CFStringRef _rv)"},
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
	CFStringRef_Type.ob_type = &PyType_Type;
	Py_INCREF(&CFStringRef_Type);
	if (PyDict_SetItemString(d, "CFStringRefType", (PyObject *)&CFStringRef_Type) != 0)
		Py_FatalError("can't initialize CFStringRefType");
}

/* ========================= End module CF ========================== */

