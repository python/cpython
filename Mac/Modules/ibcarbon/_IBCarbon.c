
/* ======================== Module _IBCarbon ======================== */

#include "Python.h"



#ifdef WITHOUT_FRAMEWORKS
#include <IBCarbonRuntime.h>
#else
#include <Carbon/Carbon.h>
#endif /* WITHOUT_FRAMEWORKS */
#include "macglue.h"

#ifdef USE_TOOLBOX_OBJECT_GLUE
extern int _CFStringRefObj_Convert(PyObject *, CFStringRef *);
//#define CFStringRefObj_Convert _CFStringRefObj_Convert
#endif

//extern int CFBundleRefObj_Convert(PyObject *, CFBundleRef *);  // need to wrap CFBundle


static PyObject *IBCarbon_Error;

/* ---------------------- Object type IBNibRef ---------------------- */

PyTypeObject IBNibRef_Type;

#define IBNibRefObj_Check(x) ((x)->ob_type == &IBNibRef_Type)

typedef struct IBNibRefObject {
	PyObject_HEAD
	IBNibRef ob_itself;
} IBNibRefObject;

PyObject *IBNibRefObj_New(IBNibRef itself)
{
	IBNibRefObject *it;
	it = PyObject_NEW(IBNibRefObject, &IBNibRef_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}
int IBNibRefObj_Convert(PyObject *v, IBNibRef *p_itself)
{
	if (!IBNibRefObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "IBNibRef required");
		return 0;
	}
	*p_itself = ((IBNibRefObject *)v)->ob_itself;
	return 1;
}

static void IBNibRefObj_dealloc(IBNibRefObject *self)
{
	DisposeNibReference(self->ob_itself);
	PyMem_DEL(self);
}

static PyObject *IBNibRefObj_CreateWindowFromNib(IBNibRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	CFStringRef inName;
	WindowPtr outWindow;
	if (!PyArg_ParseTuple(_args, "O&",
	                      CFStringRefObj_Convert, &inName))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	_err = CreateWindowFromNib(_self->ob_itself,
	                           inName,
	                           &outWindow);
	Py_END_ALLOW_THREADS
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     WinObj_New, outWindow);
	return _res;
}

static PyObject *IBNibRefObj_CreateMenuFromNib(IBNibRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	CFStringRef inName;
	MenuHandle outMenuRef;
	if (!PyArg_ParseTuple(_args, "O&",
	                      CFStringRefObj_Convert, &inName))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	_err = CreateMenuFromNib(_self->ob_itself,
	                         inName,
	                         &outMenuRef);
	Py_END_ALLOW_THREADS
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     MenuObj_New, outMenuRef);
	return _res;
}

static PyObject *IBNibRefObj_CreateMenuBarFromNib(IBNibRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	CFStringRef inName;
	Handle outMenuBar;
	if (!PyArg_ParseTuple(_args, "O&",
	                      CFStringRefObj_Convert, &inName))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	_err = CreateMenuBarFromNib(_self->ob_itself,
	                            inName,
	                            &outMenuBar);
	Py_END_ALLOW_THREADS
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, outMenuBar);
	return _res;
}

static PyObject *IBNibRefObj_SetMenuBarFromNib(IBNibRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	CFStringRef inName;
	if (!PyArg_ParseTuple(_args, "O&",
	                      CFStringRefObj_Convert, &inName))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	_err = SetMenuBarFromNib(_self->ob_itself,
	                         inName);
	Py_END_ALLOW_THREADS
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef IBNibRefObj_methods[] = {
	{"CreateWindowFromNib", (PyCFunction)IBNibRefObj_CreateWindowFromNib, 1,
	 "(CFStringRef inName) -> (WindowPtr outWindow)"},
	{"CreateMenuFromNib", (PyCFunction)IBNibRefObj_CreateMenuFromNib, 1,
	 "(CFStringRef inName) -> (MenuHandle outMenuRef)"},
	{"CreateMenuBarFromNib", (PyCFunction)IBNibRefObj_CreateMenuBarFromNib, 1,
	 "(CFStringRef inName) -> (Handle outMenuBar)"},
	{"SetMenuBarFromNib", (PyCFunction)IBNibRefObj_SetMenuBarFromNib, 1,
	 "(CFStringRef inName) -> None"},
	{NULL, NULL, 0}
};

PyMethodChain IBNibRefObj_chain = { IBNibRefObj_methods, NULL };

static PyObject *IBNibRefObj_getattr(IBNibRefObject *self, char *name)
{
	return Py_FindMethodInChain(&IBNibRefObj_chain, (PyObject *)self, name);
}

#define IBNibRefObj_setattr NULL

#define IBNibRefObj_compare NULL

#define IBNibRefObj_repr NULL

#define IBNibRefObj_hash NULL

PyTypeObject IBNibRef_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"_IBCarbon.IBNibRef", /*tp_name*/
	sizeof(IBNibRefObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) IBNibRefObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) IBNibRefObj_getattr, /*tp_getattr*/
	(setattrfunc) IBNibRefObj_setattr, /*tp_setattr*/
	(cmpfunc) IBNibRefObj_compare, /*tp_compare*/
	(reprfunc) IBNibRefObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) IBNibRefObj_hash, /*tp_hash*/
};

/* -------------------- End object type IBNibRef -------------------- */


static PyObject *IBCarbon_CreateNibReference(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	CFStringRef inNibName;
	IBNibRef outNibRef;
	if (!PyArg_ParseTuple(_args, "O&",
	                      CFStringRefObj_Convert, &inNibName))
		return NULL;
	Py_BEGIN_ALLOW_THREADS
	_err = CreateNibReference(inNibName,
	                          &outNibRef);
	Py_END_ALLOW_THREADS
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     IBNibRefObj_New, outNibRef);
	return _res;
}

static PyMethodDef IBCarbon_methods[] = {
	{"CreateNibReference", (PyCFunction)IBCarbon_CreateNibReference, 1,
	 "(CFStringRef inNibName) -> (IBNibRef outNibRef)"},
	{NULL, NULL, 0}
};




void init_IBCarbon(void)
{
	PyObject *m;
	PyObject *d;





	m = Py_InitModule("_IBCarbon", IBCarbon_methods);
	d = PyModule_GetDict(m);
	IBCarbon_Error = PyMac_GetOSErrException();
	if (IBCarbon_Error == NULL ||
	    PyDict_SetItemString(d, "Error", IBCarbon_Error) != 0)
		return;
	IBNibRef_Type.ob_type = &PyType_Type;
	Py_INCREF(&IBNibRef_Type);
	if (PyDict_SetItemString(d, "IBNibRefType", (PyObject *)&IBNibRef_Type) != 0)
		Py_FatalError("can't initialize IBNibRefType");
}

/* ====================== End module _IBCarbon ====================== */

