
/* =========================== Module Res =========================== */

#include "Python.h"



#define SystemSevenOrLater 1

#include "macglue.h"
#include <Memory.h>
#include <Dialogs.h>
#include <Menus.h>
#include <Controls.h>

extern PyObject *ResObj_New(Handle);
extern int ResObj_Convert(PyObject *, Handle *);
extern PyObject *OptResObj_New(Handle);
extern int OptResObj_Convert(PyObject *, Handle *);

extern PyObject *WinObj_New(WindowPtr);
extern int WinObj_Convert(PyObject *, WindowPtr *);
extern PyTypeObject Window_Type;
#define WinObj_Check(x) ((x)->ob_type == &Window_Type)

extern PyObject *DlgObj_New(DialogPtr);
extern int DlgObj_Convert(PyObject *, DialogPtr *);
extern PyTypeObject Dialog_Type;
#define DlgObj_Check(x) ((x)->ob_type == &Dialog_Type)

extern PyObject *MenuObj_New(MenuHandle);
extern int MenuObj_Convert(PyObject *, MenuHandle *);

extern PyObject *CtlObj_New(ControlHandle);
extern int CtlObj_Convert(PyObject *, ControlHandle *);

extern PyObject *GrafObj_New(GrafPtr);
extern int GrafObj_Convert(PyObject *, GrafPtr *);

extern PyObject *BMObj_New(BitMapPtr);
extern int BMObj_Convert(PyObject *, BitMapPtr *);

extern PyObject *WinObj_WhichWindow(WindowPtr);

#include <Resources.h>
#include <string.h>

#define resNotFound -192 /* Can't include <Errors.h> because of Python's "errors.h" */

static PyObject *Res_Error;

/* ---------------------- Object type Resource ---------------------- */

PyTypeObject Resource_Type;

#define ResObj_Check(x) ((x)->ob_type == &Resource_Type)

typedef struct ResourceObject {
	PyObject_HEAD
	Handle ob_itself;
} ResourceObject;

PyObject *ResObj_New(itself)
	Handle itself;
{
	ResourceObject *it;
	if (itself == NULL) return PyMac_Error(resNotFound);
	it = PyObject_NEW(ResourceObject, &Resource_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}
ResObj_Convert(v, p_itself)
	PyObject *v;
	Handle *p_itself;
{
	if (!ResObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "Resource required");
		return 0;
	}
	*p_itself = ((ResourceObject *)v)->ob_itself;
	return 1;
}

static void ResObj_dealloc(self)
	ResourceObject *self;
{
	/* Cleanup of self->ob_itself goes here */
	PyMem_DEL(self);
}

static PyObject *ResObj_HomeResFile(_self, _args)
	ResourceObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = HomeResFile(_self->ob_itself);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *ResObj_MacLoadResource(_self, _args)
	ResourceObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	MacLoadResource(_self->ob_itself);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *ResObj_ReleaseResource(_self, _args)
	ResourceObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	ReleaseResource(_self->ob_itself);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *ResObj_DetachResource(_self, _args)
	ResourceObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	DetachResource(_self->ob_itself);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *ResObj_GetResAttrs(_self, _args)
	ResourceObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetResAttrs(_self->ob_itself);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *ResObj_GetResInfo(_self, _args)
	ResourceObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short theID;
	ResType theType;
	Str255 name;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetResInfo(_self->ob_itself,
	           &theID,
	           &theType,
	           name);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("hO&O&",
	                     theID,
	                     PyMac_BuildOSType, theType,
	                     PyMac_BuildStr255, name);
	return _res;
}

static PyObject *ResObj_SetResInfo(_self, _args)
	ResourceObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short theID;
	Str255 name;
	if (!PyArg_ParseTuple(_args, "hO&",
	                      &theID,
	                      PyMac_GetStr255, name))
		return NULL;
	SetResInfo(_self->ob_itself,
	           theID,
	           name);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *ResObj_AddResource(_self, _args)
	ResourceObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	ResType theType;
	short theID;
	Str255 name;
	if (!PyArg_ParseTuple(_args, "O&hO&",
	                      PyMac_GetOSType, &theType,
	                      &theID,
	                      PyMac_GetStr255, name))
		return NULL;
	AddResource(_self->ob_itself,
	            theType,
	            theID,
	            name);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *ResObj_GetResourceSizeOnDisk(_self, _args)
	ResourceObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetResourceSizeOnDisk(_self->ob_itself);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *ResObj_GetMaxResourceSize(_self, _args)
	ResourceObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMaxResourceSize(_self->ob_itself);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *ResObj_RsrcMapEntry(_self, _args)
	ResourceObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = RsrcMapEntry(_self->ob_itself);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *ResObj_SetResAttrs(_self, _args)
	ResourceObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short attrs;
	if (!PyArg_ParseTuple(_args, "h",
	                      &attrs))
		return NULL;
	SetResAttrs(_self->ob_itself,
	            attrs);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *ResObj_ChangedResource(_self, _args)
	ResourceObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	ChangedResource(_self->ob_itself);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *ResObj_RemoveResource(_self, _args)
	ResourceObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	RemoveResource(_self->ob_itself);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *ResObj_WriteResource(_self, _args)
	ResourceObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	WriteResource(_self->ob_itself);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *ResObj_SetResourceSize(_self, _args)
	ResourceObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long newSize;
	if (!PyArg_ParseTuple(_args, "l",
	                      &newSize))
		return NULL;
	SetResourceSize(_self->ob_itself,
	                newSize);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *ResObj_GetNextFOND(_self, _args)
	ResourceObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Handle _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetNextFOND(_self->ob_itself);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *ResObj_as_Control(_self, _args)
	ResourceObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;

	return CtlObj_New((ControlHandle)_self->ob_itself);

}

static PyObject *ResObj_as_Menu(_self, _args)
	ResourceObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;

	return MenuObj_New((MenuHandle)_self->ob_itself);

}

static PyMethodDef ResObj_methods[] = {
	{"HomeResFile", (PyCFunction)ResObj_HomeResFile, 1,
	 "() -> (short _rv)"},
	{"MacLoadResource", (PyCFunction)ResObj_MacLoadResource, 1,
	 "() -> None"},
	{"ReleaseResource", (PyCFunction)ResObj_ReleaseResource, 1,
	 "() -> None"},
	{"DetachResource", (PyCFunction)ResObj_DetachResource, 1,
	 "() -> None"},
	{"GetResAttrs", (PyCFunction)ResObj_GetResAttrs, 1,
	 "() -> (short _rv)"},
	{"GetResInfo", (PyCFunction)ResObj_GetResInfo, 1,
	 "() -> (short theID, ResType theType, Str255 name)"},
	{"SetResInfo", (PyCFunction)ResObj_SetResInfo, 1,
	 "(short theID, Str255 name) -> None"},
	{"AddResource", (PyCFunction)ResObj_AddResource, 1,
	 "(ResType theType, short theID, Str255 name) -> None"},
	{"GetResourceSizeOnDisk", (PyCFunction)ResObj_GetResourceSizeOnDisk, 1,
	 "() -> (long _rv)"},
	{"GetMaxResourceSize", (PyCFunction)ResObj_GetMaxResourceSize, 1,
	 "() -> (long _rv)"},
	{"RsrcMapEntry", (PyCFunction)ResObj_RsrcMapEntry, 1,
	 "() -> (long _rv)"},
	{"SetResAttrs", (PyCFunction)ResObj_SetResAttrs, 1,
	 "(short attrs) -> None"},
	{"ChangedResource", (PyCFunction)ResObj_ChangedResource, 1,
	 "() -> None"},
	{"RemoveResource", (PyCFunction)ResObj_RemoveResource, 1,
	 "() -> None"},
	{"WriteResource", (PyCFunction)ResObj_WriteResource, 1,
	 "() -> None"},
	{"SetResourceSize", (PyCFunction)ResObj_SetResourceSize, 1,
	 "(long newSize) -> None"},
	{"GetNextFOND", (PyCFunction)ResObj_GetNextFOND, 1,
	 "() -> (Handle _rv)"},
	{"as_Control", (PyCFunction)ResObj_as_Control, 1,
	 "Return this resource/handle as a Control"},
	{"as_Menu", (PyCFunction)ResObj_as_Menu, 1,
	 "Return this resource/handle as a Menu"},
	{NULL, NULL, 0}
};

PyMethodChain ResObj_chain = { ResObj_methods, NULL };

static PyObject *ResObj_getattr(self, name)
	ResourceObject *self;
	char *name;
{

	if (strcmp(name, "size") == 0)
		return PyInt_FromLong(GetHandleSize(self->ob_itself));
	if (strcmp(name, "data") == 0) {
		PyObject *res;
		char state;
		state = HGetState(self->ob_itself);
		HLock(self->ob_itself);
		res = PyString_FromStringAndSize(
			*self->ob_itself,
			GetHandleSize(self->ob_itself));
		HUnlock(self->ob_itself);
		HSetState(self->ob_itself, state);
		return res;
	}
	if (strcmp(name, "__members__") == 0)
		return Py_BuildValue("[ss]", "data", "size");

	return Py_FindMethodInChain(&ResObj_chain, (PyObject *)self, name);
}

static int
ResObj_setattr(self, name, value)
	ResourceObject *self;
	char *name;
	PyObject *value;
{
	char *data;
	long size;
	
	if (strcmp(name, "data") != 0 || value == NULL )
		return -1;
	if ( !PyString_Check(value) )
		return -1;
	size = PyString_Size(value);
	data = PyString_AsString(value);
	/* XXXX Do I need the GetState/SetState calls? */
	SetHandleSize(self->ob_itself, size);
	if ( MemError())
		return -1;
	HLock(self->ob_itself);
	memcpy((char *)*self->ob_itself, data, size);
	HUnlock(self->ob_itself);
	/* XXXX Should I do the Changed call immedeately? */
	return 0;
}


PyTypeObject Resource_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"Resource", /*tp_name*/
	sizeof(ResourceObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) ResObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) ResObj_getattr, /*tp_getattr*/
	(setattrfunc) ResObj_setattr, /*tp_setattr*/
};

/* -------------------- End object type Resource -------------------- */


static PyObject *Res_InitResources(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = InitResources();
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Res_RsrcZoneInit(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	RsrcZoneInit();
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Res_CloseResFile(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short refNum;
	if (!PyArg_ParseTuple(_args, "h",
	                      &refNum))
		return NULL;
	CloseResFile(refNum);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Res_ResError(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = ResError();
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Res_CurResFile(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = CurResFile();
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Res_CreateResFile(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Str255 fileName;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetStr255, fileName))
		return NULL;
	CreateResFile(fileName);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Res_OpenResFile(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	Str255 fileName;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetStr255, fileName))
		return NULL;
	_rv = OpenResFile(fileName);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Res_UseResFile(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short refNum;
	if (!PyArg_ParseTuple(_args, "h",
	                      &refNum))
		return NULL;
	UseResFile(refNum);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Res_CountTypes(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = CountTypes();
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Res_Count1Types(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = Count1Types();
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Res_GetIndType(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	ResType theType;
	short index;
	if (!PyArg_ParseTuple(_args, "h",
	                      &index))
		return NULL;
	GetIndType(&theType,
	           index);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("O&",
	                     PyMac_BuildOSType, theType);
	return _res;
}

static PyObject *Res_Get1IndType(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	ResType theType;
	short index;
	if (!PyArg_ParseTuple(_args, "h",
	                      &index))
		return NULL;
	Get1IndType(&theType,
	            index);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("O&",
	                     PyMac_BuildOSType, theType);
	return _res;
}

static PyObject *Res_SetResLoad(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean load;
	if (!PyArg_ParseTuple(_args, "b",
	                      &load))
		return NULL;
	SetResLoad(load);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Res_CountResources(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	ResType theType;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &theType))
		return NULL;
	_rv = CountResources(theType);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Res_Count1Resources(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	ResType theType;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &theType))
		return NULL;
	_rv = Count1Resources(theType);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Res_GetIndResource(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Handle _rv;
	ResType theType;
	short index;
	if (!PyArg_ParseTuple(_args, "O&h",
	                      PyMac_GetOSType, &theType,
	                      &index))
		return NULL;
	_rv = GetIndResource(theType,
	                     index);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *Res_Get1IndResource(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Handle _rv;
	ResType theType;
	short index;
	if (!PyArg_ParseTuple(_args, "O&h",
	                      PyMac_GetOSType, &theType,
	                      &index))
		return NULL;
	_rv = Get1IndResource(theType,
	                      index);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *Res_GetResource(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Handle _rv;
	ResType theType;
	short theID;
	if (!PyArg_ParseTuple(_args, "O&h",
	                      PyMac_GetOSType, &theType,
	                      &theID))
		return NULL;
	_rv = GetResource(theType,
	                  theID);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *Res_Get1Resource(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Handle _rv;
	ResType theType;
	short theID;
	if (!PyArg_ParseTuple(_args, "O&h",
	                      PyMac_GetOSType, &theType,
	                      &theID))
		return NULL;
	_rv = Get1Resource(theType,
	                   theID);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *Res_GetNamedResource(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Handle _rv;
	ResType theType;
	Str255 name;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetOSType, &theType,
	                      PyMac_GetStr255, name))
		return NULL;
	_rv = GetNamedResource(theType,
	                       name);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *Res_Get1NamedResource(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Handle _rv;
	ResType theType;
	Str255 name;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetOSType, &theType,
	                      PyMac_GetStr255, name))
		return NULL;
	_rv = Get1NamedResource(theType,
	                        name);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *Res_UniqueID(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	ResType theType;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &theType))
		return NULL;
	_rv = UniqueID(theType);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Res_Unique1ID(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	ResType theType;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &theType))
		return NULL;
	_rv = Unique1ID(theType);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Res_UpdateResFile(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short refNum;
	if (!PyArg_ParseTuple(_args, "h",
	                      &refNum))
		return NULL;
	UpdateResFile(refNum);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Res_SetResPurge(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean install;
	if (!PyArg_ParseTuple(_args, "b",
	                      &install))
		return NULL;
	SetResPurge(install);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Res_GetResFileAttrs(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	short refNum;
	if (!PyArg_ParseTuple(_args, "h",
	                      &refNum))
		return NULL;
	_rv = GetResFileAttrs(refNum);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Res_SetResFileAttrs(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short refNum;
	short attrs;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &refNum,
	                      &attrs))
		return NULL;
	SetResFileAttrs(refNum,
	                attrs);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Res_OpenRFPerm(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	Str255 fileName;
	short vRefNum;
	SignedByte permission;
	if (!PyArg_ParseTuple(_args, "O&hb",
	                      PyMac_GetStr255, fileName,
	                      &vRefNum,
	                      &permission))
		return NULL;
	_rv = OpenRFPerm(fileName,
	                 vRefNum,
	                 permission);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Res_RGetResource(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Handle _rv;
	ResType theType;
	short theID;
	if (!PyArg_ParseTuple(_args, "O&h",
	                      PyMac_GetOSType, &theType,
	                      &theID))
		return NULL;
	_rv = RGetResource(theType,
	                   theID);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *Res_HOpenResFile(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	short vRefNum;
	long dirID;
	Str255 fileName;
	SignedByte permission;
	if (!PyArg_ParseTuple(_args, "hlO&b",
	                      &vRefNum,
	                      &dirID,
	                      PyMac_GetStr255, fileName,
	                      &permission))
		return NULL;
	_rv = HOpenResFile(vRefNum,
	                   dirID,
	                   fileName,
	                   permission);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Res_HCreateResFile(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short vRefNum;
	long dirID;
	Str255 fileName;
	if (!PyArg_ParseTuple(_args, "hlO&",
	                      &vRefNum,
	                      &dirID,
	                      PyMac_GetStr255, fileName))
		return NULL;
	HCreateResFile(vRefNum,
	               dirID,
	               fileName);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Res_FSpOpenResFile(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	FSSpec spec;
	SignedByte permission;
	if (!PyArg_ParseTuple(_args, "O&b",
	                      PyMac_GetFSSpec, &spec,
	                      &permission))
		return NULL;
	_rv = FSpOpenResFile(&spec,
	                     permission);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Res_FSpCreateResFile(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	FSSpec spec;
	OSType creator;
	OSType fileType;
	ScriptCode scriptTag;
	if (!PyArg_ParseTuple(_args, "O&O&O&h",
	                      PyMac_GetFSSpec, &spec,
	                      PyMac_GetOSType, &creator,
	                      PyMac_GetOSType, &fileType,
	                      &scriptTag))
		return NULL;
	FSpCreateResFile(&spec,
	                 creator,
	                 fileType,
	                 scriptTag);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Res_Resource(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;

	char *buf;
	int len;
	Handle h;

	if (!PyArg_ParseTuple(_args, "s#", &buf, &len))
		return NULL;
	h = NewHandle(len);
	if ( h == NULL ) {
		PyErr_NoMemory();
		return NULL;
	}
	HLock(h);
	memcpy(*h, buf, len);
	HUnlock(h);
	return (PyObject *)ResObj_New(h);

}

static PyMethodDef Res_methods[] = {
	{"InitResources", (PyCFunction)Res_InitResources, 1,
	 "() -> (short _rv)"},
	{"RsrcZoneInit", (PyCFunction)Res_RsrcZoneInit, 1,
	 "() -> None"},
	{"CloseResFile", (PyCFunction)Res_CloseResFile, 1,
	 "(short refNum) -> None"},
	{"ResError", (PyCFunction)Res_ResError, 1,
	 "() -> (OSErr _rv)"},
	{"CurResFile", (PyCFunction)Res_CurResFile, 1,
	 "() -> (short _rv)"},
	{"CreateResFile", (PyCFunction)Res_CreateResFile, 1,
	 "(Str255 fileName) -> None"},
	{"OpenResFile", (PyCFunction)Res_OpenResFile, 1,
	 "(Str255 fileName) -> (short _rv)"},
	{"UseResFile", (PyCFunction)Res_UseResFile, 1,
	 "(short refNum) -> None"},
	{"CountTypes", (PyCFunction)Res_CountTypes, 1,
	 "() -> (short _rv)"},
	{"Count1Types", (PyCFunction)Res_Count1Types, 1,
	 "() -> (short _rv)"},
	{"GetIndType", (PyCFunction)Res_GetIndType, 1,
	 "(short index) -> (ResType theType)"},
	{"Get1IndType", (PyCFunction)Res_Get1IndType, 1,
	 "(short index) -> (ResType theType)"},
	{"SetResLoad", (PyCFunction)Res_SetResLoad, 1,
	 "(Boolean load) -> None"},
	{"CountResources", (PyCFunction)Res_CountResources, 1,
	 "(ResType theType) -> (short _rv)"},
	{"Count1Resources", (PyCFunction)Res_Count1Resources, 1,
	 "(ResType theType) -> (short _rv)"},
	{"GetIndResource", (PyCFunction)Res_GetIndResource, 1,
	 "(ResType theType, short index) -> (Handle _rv)"},
	{"Get1IndResource", (PyCFunction)Res_Get1IndResource, 1,
	 "(ResType theType, short index) -> (Handle _rv)"},
	{"GetResource", (PyCFunction)Res_GetResource, 1,
	 "(ResType theType, short theID) -> (Handle _rv)"},
	{"Get1Resource", (PyCFunction)Res_Get1Resource, 1,
	 "(ResType theType, short theID) -> (Handle _rv)"},
	{"GetNamedResource", (PyCFunction)Res_GetNamedResource, 1,
	 "(ResType theType, Str255 name) -> (Handle _rv)"},
	{"Get1NamedResource", (PyCFunction)Res_Get1NamedResource, 1,
	 "(ResType theType, Str255 name) -> (Handle _rv)"},
	{"UniqueID", (PyCFunction)Res_UniqueID, 1,
	 "(ResType theType) -> (short _rv)"},
	{"Unique1ID", (PyCFunction)Res_Unique1ID, 1,
	 "(ResType theType) -> (short _rv)"},
	{"UpdateResFile", (PyCFunction)Res_UpdateResFile, 1,
	 "(short refNum) -> None"},
	{"SetResPurge", (PyCFunction)Res_SetResPurge, 1,
	 "(Boolean install) -> None"},
	{"GetResFileAttrs", (PyCFunction)Res_GetResFileAttrs, 1,
	 "(short refNum) -> (short _rv)"},
	{"SetResFileAttrs", (PyCFunction)Res_SetResFileAttrs, 1,
	 "(short refNum, short attrs) -> None"},
	{"OpenRFPerm", (PyCFunction)Res_OpenRFPerm, 1,
	 "(Str255 fileName, short vRefNum, SignedByte permission) -> (short _rv)"},
	{"RGetResource", (PyCFunction)Res_RGetResource, 1,
	 "(ResType theType, short theID) -> (Handle _rv)"},
	{"HOpenResFile", (PyCFunction)Res_HOpenResFile, 1,
	 "(short vRefNum, long dirID, Str255 fileName, SignedByte permission) -> (short _rv)"},
	{"HCreateResFile", (PyCFunction)Res_HCreateResFile, 1,
	 "(short vRefNum, long dirID, Str255 fileName) -> None"},
	{"FSpOpenResFile", (PyCFunction)Res_FSpOpenResFile, 1,
	 "(FSSpec spec, SignedByte permission) -> (short _rv)"},
	{"FSpCreateResFile", (PyCFunction)Res_FSpCreateResFile, 1,
	 "(FSSpec spec, OSType creator, OSType fileType, ScriptCode scriptTag) -> None"},
	{"Resource", (PyCFunction)Res_Resource, 1,
	 "Convert a string to a resource object.\n\nThe created resource object is actually just a handle.\nApply AddResource() to write it to a resource file.\n"},
	{NULL, NULL, 0}
};




/* Alternative version of ResObj_New, which returns None for null argument */
PyObject *OptResObj_New(itself)
	Handle itself;
{
	if (itself == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	return ResObj_New(itself);
}

OptResObj_Convert(v, p_itself)
	PyObject *v;
	Handle *p_itself;
{
	if ( v == Py_None ) {
		*p_itself = NULL;
		return 1;
	}
	if (!ResObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "Resource required");
		return 0;
	}
	*p_itself = ((ResourceObject *)v)->ob_itself;
	return 1;
}



void initRes()
{
	PyObject *m;
	PyObject *d;





	m = Py_InitModule("Res", Res_methods);
	d = PyModule_GetDict(m);
	Res_Error = PyMac_GetOSErrException();
	if (Res_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Res_Error) != 0)
		Py_FatalError("can't initialize Res.Error");
	Resource_Type.ob_type = &PyType_Type;
	Py_INCREF(&Resource_Type);
	if (PyDict_SetItemString(d, "ResourceType", (PyObject *)&Resource_Type) != 0)
		Py_FatalError("can't initialize ResourceType");
}

/* ========================= End module Res ========================= */

