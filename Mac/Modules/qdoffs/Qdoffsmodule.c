
/* ========================= Module Qdoffs ========================== */

#include "Python.h"



#include "macglue.h"
#include "pymactoolbox.h"

#ifdef WITHOUT_FRAMEWORKS
#include <QDOffscreen.h>
#else
#include <Carbon/Carbon.h>
#endif

#ifdef USE_TOOLBOX_OBJECT_GLUE
extern PyObject *_GWorldObj_New(GWorldPtr);
extern int _GWorldObj_Convert(PyObject *, GWorldPtr *);

#define GWorldObj_New _GWorldObj_New
#define GWorldObj_Convert _GWorldObj_Convert
#endif

#define as_GrafPtr(gworld) ((GrafPtr)(gworld))


static PyObject *Qdoffs_Error;

/* ----------------------- Object type GWorld ----------------------- */

PyTypeObject GWorld_Type;

#define GWorldObj_Check(x) ((x)->ob_type == &GWorld_Type)

typedef struct GWorldObject {
	PyObject_HEAD
	GWorldPtr ob_itself;
} GWorldObject;

PyObject *GWorldObj_New(GWorldPtr itself)
{
	GWorldObject *it;
	if (itself == NULL) return PyMac_Error(resNotFound);
	it = PyObject_NEW(GWorldObject, &GWorld_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}
GWorldObj_Convert(PyObject *v, GWorldPtr *p_itself)
{
	if (!GWorldObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "GWorld required");
		return 0;
	}
	*p_itself = ((GWorldObject *)v)->ob_itself;
	return 1;
}

static void GWorldObj_dealloc(GWorldObject *self)
{
	DisposeGWorld(self->ob_itself);
	PyMem_DEL(self);
}

static PyObject *GWorldObj_GetGWorldDevice(GWorldObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	GDHandle _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetGWorldDevice(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *GWorldObj_GetGWorldPixMap(GWorldObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PixMapHandle _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetGWorldPixMap(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *GWorldObj_as_GrafPtr(GWorldObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	GrafPtr _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = as_GrafPtr(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     GrafObj_New, _rv);
	return _res;
}

static PyMethodDef GWorldObj_methods[] = {
	{"GetGWorldDevice", (PyCFunction)GWorldObj_GetGWorldDevice, 1,
	 "() -> (GDHandle _rv)"},
	{"GetGWorldPixMap", (PyCFunction)GWorldObj_GetGWorldPixMap, 1,
	 "() -> (PixMapHandle _rv)"},
	{"as_GrafPtr", (PyCFunction)GWorldObj_as_GrafPtr, 1,
	 "() -> (GrafPtr _rv)"},
	{NULL, NULL, 0}
};

PyMethodChain GWorldObj_chain = { GWorldObj_methods, NULL };

static PyObject *GWorldObj_getattr(GWorldObject *self, char *name)
{
	return Py_FindMethodInChain(&GWorldObj_chain, (PyObject *)self, name);
}

#define GWorldObj_setattr NULL

#define GWorldObj_compare NULL

#define GWorldObj_repr NULL

#define GWorldObj_hash NULL

PyTypeObject GWorld_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"GWorld", /*tp_name*/
	sizeof(GWorldObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) GWorldObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) GWorldObj_getattr, /*tp_getattr*/
	(setattrfunc) GWorldObj_setattr, /*tp_setattr*/
	(cmpfunc) GWorldObj_compare, /*tp_compare*/
	(reprfunc) GWorldObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) GWorldObj_hash, /*tp_hash*/
};

/* --------------------- End object type GWorld --------------------- */


static PyObject *Qdoffs_NewGWorld(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	QDErr _err;
	GWorldPtr offscreenGWorld;
	short PixelDepth;
	Rect boundsRect;
	CTabHandle cTable;
	GDHandle aGDevice;
	GWorldFlags flags;
	if (!PyArg_ParseTuple(_args, "hO&O&O&l",
	                      &PixelDepth,
	                      PyMac_GetRect, &boundsRect,
	                      OptResObj_Convert, &cTable,
	                      OptResObj_Convert, &aGDevice,
	                      &flags))
		return NULL;
	_err = NewGWorld(&offscreenGWorld,
	                 PixelDepth,
	                 &boundsRect,
	                 cTable,
	                 aGDevice,
	                 flags);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     GWorldObj_New, offscreenGWorld);
	return _res;
}

static PyObject *Qdoffs_LockPixels(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	PixMapHandle pm;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &pm))
		return NULL;
	_rv = LockPixels(pm);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Qdoffs_UnlockPixels(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PixMapHandle pm;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &pm))
		return NULL;
	UnlockPixels(pm);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qdoffs_UpdateGWorld(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	GWorldFlags _rv;
	GWorldPtr offscreenGWorld;
	short pixelDepth;
	Rect boundsRect;
	CTabHandle cTable;
	GDHandle aGDevice;
	GWorldFlags flags;
	if (!PyArg_ParseTuple(_args, "hO&O&O&l",
	                      &pixelDepth,
	                      PyMac_GetRect, &boundsRect,
	                      OptResObj_Convert, &cTable,
	                      OptResObj_Convert, &aGDevice,
	                      &flags))
		return NULL;
	_rv = UpdateGWorld(&offscreenGWorld,
	                   pixelDepth,
	                   &boundsRect,
	                   cTable,
	                   aGDevice,
	                   flags);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     GWorldObj_New, offscreenGWorld);
	return _res;
}

static PyObject *Qdoffs_GetGWorld(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CGrafPtr port;
	GDHandle gdh;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetGWorld(&port,
	          &gdh);
	_res = Py_BuildValue("O&O&",
	                     GrafObj_New, port,
	                     ResObj_New, gdh);
	return _res;
}

static PyObject *Qdoffs_SetGWorld(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CGrafPtr port;
	GDHandle gdh;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      GrafObj_Convert, &port,
	                      OptResObj_Convert, &gdh))
		return NULL;
	SetGWorld(port,
	          gdh);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qdoffs_CTabChanged(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CTabHandle ctab;
	if (!PyArg_ParseTuple(_args, "O&",
	                      OptResObj_Convert, &ctab))
		return NULL;
	CTabChanged(ctab);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qdoffs_PixPatChanged(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PixPatHandle ppat;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &ppat))
		return NULL;
	PixPatChanged(ppat);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qdoffs_PortChanged(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	GrafPtr port;
	if (!PyArg_ParseTuple(_args, "O&",
	                      GrafObj_Convert, &port))
		return NULL;
	PortChanged(port);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qdoffs_GDeviceChanged(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	GDHandle gdh;
	if (!PyArg_ParseTuple(_args, "O&",
	                      OptResObj_Convert, &gdh))
		return NULL;
	GDeviceChanged(gdh);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qdoffs_AllowPurgePixels(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PixMapHandle pm;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &pm))
		return NULL;
	AllowPurgePixels(pm);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qdoffs_NoPurgePixels(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PixMapHandle pm;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &pm))
		return NULL;
	NoPurgePixels(pm);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qdoffs_GetPixelsState(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	GWorldFlags _rv;
	PixMapHandle pm;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &pm))
		return NULL;
	_rv = GetPixelsState(pm);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qdoffs_SetPixelsState(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PixMapHandle pm;
	GWorldFlags state;
	if (!PyArg_ParseTuple(_args, "O&l",
	                      ResObj_Convert, &pm,
	                      &state))
		return NULL;
	SetPixelsState(pm,
	               state);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qdoffs_GetPixRowBytes(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
	PixMapHandle pm;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &pm))
		return NULL;
	_rv = GetPixRowBytes(pm);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qdoffs_NewScreenBuffer(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	QDErr _err;
	Rect globalRect;
	Boolean purgeable;
	GDHandle gdh;
	PixMapHandle offscreenPixMap;
	if (!PyArg_ParseTuple(_args, "O&b",
	                      PyMac_GetRect, &globalRect,
	                      &purgeable))
		return NULL;
	_err = NewScreenBuffer(&globalRect,
	                       purgeable,
	                       &gdh,
	                       &offscreenPixMap);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&O&",
	                     ResObj_New, gdh,
	                     ResObj_New, offscreenPixMap);
	return _res;
}

static PyObject *Qdoffs_DisposeScreenBuffer(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PixMapHandle offscreenPixMap;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &offscreenPixMap))
		return NULL;
	DisposeScreenBuffer(offscreenPixMap);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Qdoffs_QDDone(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	GrafPtr port;
	if (!PyArg_ParseTuple(_args, "O&",
	                      GrafObj_Convert, &port))
		return NULL;
	_rv = QDDone(port);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Qdoffs_OffscreenVersion(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = OffscreenVersion();
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Qdoffs_NewTempScreenBuffer(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	QDErr _err;
	Rect globalRect;
	Boolean purgeable;
	GDHandle gdh;
	PixMapHandle offscreenPixMap;
	if (!PyArg_ParseTuple(_args, "O&b",
	                      PyMac_GetRect, &globalRect,
	                      &purgeable))
		return NULL;
	_err = NewTempScreenBuffer(&globalRect,
	                           purgeable,
	                           &gdh,
	                           &offscreenPixMap);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&O&",
	                     ResObj_New, gdh,
	                     ResObj_New, offscreenPixMap);
	return _res;
}

static PyObject *Qdoffs_PixMap32Bit(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	PixMapHandle pmHandle;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &pmHandle))
		return NULL;
	_rv = PixMap32Bit(pmHandle);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Qdoffs_GetPixMapBytes(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;

	PixMapHandle pm;
	int from, length;
	char *cp;

	if ( !PyArg_ParseTuple(_args, "O&ii", ResObj_Convert, &pm, &from, &length) )
		return NULL;
	cp = GetPixBaseAddr(pm)+from;
	return PyString_FromStringAndSize(cp, length);

}

static PyObject *Qdoffs_PutPixMapBytes(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;

	PixMapHandle pm;
	int from, length;
	char *cp, *icp;

	if ( !PyArg_ParseTuple(_args, "O&is#", ResObj_Convert, &pm, &from, &icp, &length) )
		return NULL;
	cp = GetPixBaseAddr(pm)+from;
	memcpy(cp, icp, length);
	Py_INCREF(Py_None);
	return Py_None;

}

static PyMethodDef Qdoffs_methods[] = {
	{"NewGWorld", (PyCFunction)Qdoffs_NewGWorld, 1,
	 "(short PixelDepth, Rect boundsRect, CTabHandle cTable, GDHandle aGDevice, GWorldFlags flags) -> (GWorldPtr offscreenGWorld)"},
	{"LockPixels", (PyCFunction)Qdoffs_LockPixels, 1,
	 "(PixMapHandle pm) -> (Boolean _rv)"},
	{"UnlockPixels", (PyCFunction)Qdoffs_UnlockPixels, 1,
	 "(PixMapHandle pm) -> None"},
	{"UpdateGWorld", (PyCFunction)Qdoffs_UpdateGWorld, 1,
	 "(short pixelDepth, Rect boundsRect, CTabHandle cTable, GDHandle aGDevice, GWorldFlags flags) -> (GWorldFlags _rv, GWorldPtr offscreenGWorld)"},
	{"GetGWorld", (PyCFunction)Qdoffs_GetGWorld, 1,
	 "() -> (CGrafPtr port, GDHandle gdh)"},
	{"SetGWorld", (PyCFunction)Qdoffs_SetGWorld, 1,
	 "(CGrafPtr port, GDHandle gdh) -> None"},
	{"CTabChanged", (PyCFunction)Qdoffs_CTabChanged, 1,
	 "(CTabHandle ctab) -> None"},
	{"PixPatChanged", (PyCFunction)Qdoffs_PixPatChanged, 1,
	 "(PixPatHandle ppat) -> None"},
	{"PortChanged", (PyCFunction)Qdoffs_PortChanged, 1,
	 "(GrafPtr port) -> None"},
	{"GDeviceChanged", (PyCFunction)Qdoffs_GDeviceChanged, 1,
	 "(GDHandle gdh) -> None"},
	{"AllowPurgePixels", (PyCFunction)Qdoffs_AllowPurgePixels, 1,
	 "(PixMapHandle pm) -> None"},
	{"NoPurgePixels", (PyCFunction)Qdoffs_NoPurgePixels, 1,
	 "(PixMapHandle pm) -> None"},
	{"GetPixelsState", (PyCFunction)Qdoffs_GetPixelsState, 1,
	 "(PixMapHandle pm) -> (GWorldFlags _rv)"},
	{"SetPixelsState", (PyCFunction)Qdoffs_SetPixelsState, 1,
	 "(PixMapHandle pm, GWorldFlags state) -> None"},
	{"GetPixRowBytes", (PyCFunction)Qdoffs_GetPixRowBytes, 1,
	 "(PixMapHandle pm) -> (long _rv)"},
	{"NewScreenBuffer", (PyCFunction)Qdoffs_NewScreenBuffer, 1,
	 "(Rect globalRect, Boolean purgeable) -> (GDHandle gdh, PixMapHandle offscreenPixMap)"},
	{"DisposeScreenBuffer", (PyCFunction)Qdoffs_DisposeScreenBuffer, 1,
	 "(PixMapHandle offscreenPixMap) -> None"},
	{"QDDone", (PyCFunction)Qdoffs_QDDone, 1,
	 "(GrafPtr port) -> (Boolean _rv)"},
	{"OffscreenVersion", (PyCFunction)Qdoffs_OffscreenVersion, 1,
	 "() -> (long _rv)"},
	{"NewTempScreenBuffer", (PyCFunction)Qdoffs_NewTempScreenBuffer, 1,
	 "(Rect globalRect, Boolean purgeable) -> (GDHandle gdh, PixMapHandle offscreenPixMap)"},
	{"PixMap32Bit", (PyCFunction)Qdoffs_PixMap32Bit, 1,
	 "(PixMapHandle pmHandle) -> (Boolean _rv)"},
	{"GetPixMapBytes", (PyCFunction)Qdoffs_GetPixMapBytes, 1,
	 "(pixmap, int start, int size) -> string. Return bytes from the pixmap"},
	{"PutPixMapBytes", (PyCFunction)Qdoffs_PutPixMapBytes, 1,
	 "(pixmap, int start, string data). Store bytes into the pixmap"},
	{NULL, NULL, 0}
};




void initQdoffs(void)
{
	PyObject *m;
	PyObject *d;



		PyMac_INIT_TOOLBOX_OBJECT_NEW(GWorldPtr, GWorldObj_New);
		PyMac_INIT_TOOLBOX_OBJECT_CONVERT(GWorldPtr, GWorldObj_Convert);


	m = Py_InitModule("Qdoffs", Qdoffs_methods);
	d = PyModule_GetDict(m);
	Qdoffs_Error = PyMac_GetOSErrException();
	if (Qdoffs_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Qdoffs_Error) != 0)
		return;
	GWorld_Type.ob_type = &PyType_Type;
	Py_INCREF(&GWorld_Type);
	if (PyDict_SetItemString(d, "GWorldType", (PyObject *)&GWorld_Type) != 0)
		Py_FatalError("can't initialize GWorldType");
}

/* ======================= End module Qdoffs ======================== */

