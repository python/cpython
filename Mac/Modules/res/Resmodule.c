
/* =========================== Module Res =========================== */

#include "Python.h"



#include "macglue.h"
#include "pymactoolbox.h"

#ifdef WITHOUT_FRAMEWORKS
#include <Resources.h>
#include <string.h>
#else
#include <Carbon/Carbon.h>
#endif

#ifdef USE_TOOLBOX_OBJECT_GLUE
extern PyObject *_ResObj_New(Handle);
extern int _ResObj_Convert(PyObject *, Handle *);
extern PyObject *_OptResObj_New(Handle);
extern int _OptResObj_Convert(PyObject *, Handle *);
#define ResObj_New _ResObj_New
#define ResObj_Convert _ResObj_Convert
#define OptResObj_New _OptResObj_New
#define OptResObj_Convert _OptResObj_Convert
#endif

/* Function to dispose a resource, with a "normal" calling sequence */
static void
PyMac_AutoDisposeHandle(Handle h)
{
	DisposeHandle(h);
}

static PyObject *Res_Error;

/* ---------------------- Object type Resource ---------------------- */

PyTypeObject Resource_Type;

#define ResObj_Check(x) ((x)->ob_type == &Resource_Type)

typedef struct ResourceObject {
	PyObject_HEAD
	Handle ob_itself;
	void (*ob_freeit)(Handle ptr);
} ResourceObject;

PyObject *ResObj_New(Handle itself)
{
	ResourceObject *it;
	if (itself == NULL) return PyMac_Error(resNotFound);
	it = PyObject_NEW(ResourceObject, &Resource_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	it->ob_freeit = NULL;
	return (PyObject *)it;
}
ResObj_Convert(PyObject *v, Handle *p_itself)
{
	if (!ResObj_Check(v))
	{
		PyObject *tmp;
		if ( (tmp=PyObject_CallMethod(v, "as_Resource", "")) )
		{
			*p_itself = ((ResourceObject *)tmp)->ob_itself;
			Py_DECREF(tmp);
			return 1;
		}
		PyErr_Clear();
	}
	if (!ResObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "Resource required");
		return 0;
	}
	*p_itself = ((ResourceObject *)v)->ob_itself;
	return 1;
}

static void ResObj_dealloc(ResourceObject *self)
{
	if (self->ob_freeit && self->ob_itself)
	{
		self->ob_freeit(self->ob_itself);
	}
	self->ob_itself = NULL;
	PyMem_DEL(self);
}

static PyObject *ResObj_HomeResFile(ResourceObject *_self, PyObject *_args)
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

static PyObject *ResObj_MacLoadResource(ResourceObject *_self, PyObject *_args)
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

static PyObject *ResObj_ReleaseResource(ResourceObject *_self, PyObject *_args)
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

static PyObject *ResObj_DetachResource(ResourceObject *_self, PyObject *_args)
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

static PyObject *ResObj_GetResAttrs(ResourceObject *_self, PyObject *_args)
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

static PyObject *ResObj_GetResInfo(ResourceObject *_self, PyObject *_args)
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

static PyObject *ResObj_SetResInfo(ResourceObject *_self, PyObject *_args)
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

static PyObject *ResObj_AddResource(ResourceObject *_self, PyObject *_args)
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

static PyObject *ResObj_GetResourceSizeOnDisk(ResourceObject *_self, PyObject *_args)
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

static PyObject *ResObj_GetMaxResourceSize(ResourceObject *_self, PyObject *_args)
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

#if TARGET_API_MAC_OS8

static PyObject *ResObj_RsrcMapEntry(ResourceObject *_self, PyObject *_args)
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
#endif

static PyObject *ResObj_SetResAttrs(ResourceObject *_self, PyObject *_args)
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

static PyObject *ResObj_ChangedResource(ResourceObject *_self, PyObject *_args)
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

static PyObject *ResObj_RemoveResource(ResourceObject *_self, PyObject *_args)
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

static PyObject *ResObj_WriteResource(ResourceObject *_self, PyObject *_args)
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

static PyObject *ResObj_SetResourceSize(ResourceObject *_self, PyObject *_args)
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

static PyObject *ResObj_GetNextFOND(ResourceObject *_self, PyObject *_args)
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

static PyObject *ResObj_as_Control(ResourceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;

	return CtlObj_New((ControlHandle)_self->ob_itself);

}

static PyObject *ResObj_as_Menu(ResourceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;

	return MenuObj_New((MenuHandle)_self->ob_itself);

}

static PyObject *ResObj_LoadResource(ResourceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	LoadResource(_self->ob_itself);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *ResObj_AutoDispose(ResourceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;

	int onoff, old = 0;
	if (!PyArg_ParseTuple(_args, "i", &onoff))
		return NULL;
	if ( _self->ob_freeit )
		old = 1;
	if ( onoff )
		_self->ob_freeit = PyMac_AutoDisposeHandle;
	else
		_self->ob_freeit = NULL;
	return Py_BuildValue("i", old);

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

#if TARGET_API_MAC_OS8
	{"RsrcMapEntry", (PyCFunction)ResObj_RsrcMapEntry, 1,
	 "() -> (long _rv)"},
#endif
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
	{"LoadResource", (PyCFunction)ResObj_LoadResource, 1,
	 "() -> None"},
	{"AutoDispose", (PyCFunction)ResObj_AutoDispose, 1,
	 "(int)->int. Automatically DisposeHandle the object on Python object cleanup"},
	{NULL, NULL, 0}
};

PyMethodChain ResObj_chain = { ResObj_methods, NULL };

static PyObject *ResObj_getattr(ResourceObject *self, char *name)
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
ResObj_setattr(ResourceObject *self, char *name, PyObject *value)
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


#define ResObj_compare NULL

#define ResObj_repr NULL

#define ResObj_hash NULL

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
	(cmpfunc) ResObj_compare, /*tp_compare*/
	(reprfunc) ResObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) ResObj_hash, /*tp_hash*/
};

/* -------------------- End object type Resource -------------------- */


#if TARGET_API_MAC_OS8

static PyObject *Res_InitResources(PyObject *_self, PyObject *_args)
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
#endif

#if TARGET_API_MAC_OS8

static PyObject *Res_RsrcZoneInit(PyObject *_self, PyObject *_args)
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
#endif

static PyObject *Res_CloseResFile(PyObject *_self, PyObject *_args)
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

static PyObject *Res_ResError(PyObject *_self, PyObject *_args)
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

static PyObject *Res_CurResFile(PyObject *_self, PyObject *_args)
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

#if TARGET_API_MAC_OS8

static PyObject *Res_CreateResFile(PyObject *_self, PyObject *_args)
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
#endif

#if TARGET_API_MAC_OS8

static PyObject *Res_OpenResFile(PyObject *_self, PyObject *_args)
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
#endif

static PyObject *Res_UseResFile(PyObject *_self, PyObject *_args)
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

static PyObject *Res_CountTypes(PyObject *_self, PyObject *_args)
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

static PyObject *Res_Count1Types(PyObject *_self, PyObject *_args)
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

static PyObject *Res_GetIndType(PyObject *_self, PyObject *_args)
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

static PyObject *Res_Get1IndType(PyObject *_self, PyObject *_args)
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

static PyObject *Res_SetResLoad(PyObject *_self, PyObject *_args)
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

static PyObject *Res_CountResources(PyObject *_self, PyObject *_args)
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

static PyObject *Res_Count1Resources(PyObject *_self, PyObject *_args)
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

static PyObject *Res_GetIndResource(PyObject *_self, PyObject *_args)
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

static PyObject *Res_Get1IndResource(PyObject *_self, PyObject *_args)
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

static PyObject *Res_GetResource(PyObject *_self, PyObject *_args)
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

static PyObject *Res_Get1Resource(PyObject *_self, PyObject *_args)
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

static PyObject *Res_GetNamedResource(PyObject *_self, PyObject *_args)
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

static PyObject *Res_Get1NamedResource(PyObject *_self, PyObject *_args)
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

static PyObject *Res_UniqueID(PyObject *_self, PyObject *_args)
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

static PyObject *Res_Unique1ID(PyObject *_self, PyObject *_args)
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

static PyObject *Res_UpdateResFile(PyObject *_self, PyObject *_args)
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

static PyObject *Res_SetResPurge(PyObject *_self, PyObject *_args)
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

static PyObject *Res_GetResFileAttrs(PyObject *_self, PyObject *_args)
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

static PyObject *Res_SetResFileAttrs(PyObject *_self, PyObject *_args)
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

static PyObject *Res_OpenRFPerm(PyObject *_self, PyObject *_args)
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

#if TARGET_API_MAC_OS8

static PyObject *Res_RGetResource(PyObject *_self, PyObject *_args)
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
#endif

static PyObject *Res_HOpenResFile(PyObject *_self, PyObject *_args)
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

static PyObject *Res_HCreateResFile(PyObject *_self, PyObject *_args)
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

static PyObject *Res_FSpOpenResFile(PyObject *_self, PyObject *_args)
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

static PyObject *Res_FSpCreateResFile(PyObject *_self, PyObject *_args)
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

#if TARGET_API_MAC_CARBON

static PyObject *Res_InsertResourceFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _rv;
	SInt16 refNum;
	RsrcChainLocation where;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &refNum,
	                      &where))
		return NULL;
	_rv = InsertResourceFile(refNum,
	                         where);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}
#endif

#if TARGET_API_MAC_CARBON

static PyObject *Res_DetachResourceFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _rv;
	SInt16 refNum;
	if (!PyArg_ParseTuple(_args, "h",
	                      &refNum))
		return NULL;
	_rv = DetachResourceFile(refNum);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}
#endif

#if TARGET_API_MAC_CARBON

static PyObject *Res_FSpResourceFileAlreadyOpen(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	FSSpec resourceFile;
	Boolean inChain;
	SInt16 refNum;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetFSSpec, &resourceFile))
		return NULL;
	_rv = FSpResourceFileAlreadyOpen(&resourceFile,
	                                 &inChain,
	                                 &refNum);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("bbh",
	                     _rv,
	                     inChain,
	                     refNum);
	return _res;
}
#endif

#if TARGET_API_MAC_CARBON

static PyObject *Res_FSpOpenOrphanResFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _rv;
	FSSpec spec;
	SignedByte permission;
	SInt16 refNum;
	if (!PyArg_ParseTuple(_args, "O&b",
	                      PyMac_GetFSSpec, &spec,
	                      &permission))
		return NULL;
	_rv = FSpOpenOrphanResFile(&spec,
	                           permission,
	                           &refNum);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("hh",
	                     _rv,
	                     refNum);
	return _res;
}
#endif

#if TARGET_API_MAC_CARBON

static PyObject *Res_GetTopResourceFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _rv;
	SInt16 refNum;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetTopResourceFile(&refNum);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("hh",
	                     _rv,
	                     refNum);
	return _res;
}
#endif

#if TARGET_API_MAC_CARBON

static PyObject *Res_GetNextResourceFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _rv;
	SInt16 curRefNum;
	SInt16 nextRefNum;
	if (!PyArg_ParseTuple(_args, "h",
	                      &curRefNum))
		return NULL;
	_rv = GetNextResourceFile(curRefNum,
	                          &nextRefNum);
	{
		OSErr _err = ResError();
		if (_err != noErr) return PyMac_Error(_err);
	}
	_res = Py_BuildValue("hh",
	                     _rv,
	                     nextRefNum);
	return _res;
}
#endif

static PyObject *Res_Resource(PyObject *_self, PyObject *_args)
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
	return ResObj_New(h);

}

static PyObject *Res_Handle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;

	char *buf;
	int len;
	Handle h;
	ResourceObject *rv;

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
	rv = (ResourceObject *)ResObj_New(h);
	rv->ob_freeit = PyMac_AutoDisposeHandle;
	return (PyObject *)rv;

}

static PyMethodDef Res_methods[] = {

#if TARGET_API_MAC_OS8
	{"InitResources", (PyCFunction)Res_InitResources, 1,
	 "() -> (short _rv)"},
#endif

#if TARGET_API_MAC_OS8
	{"RsrcZoneInit", (PyCFunction)Res_RsrcZoneInit, 1,
	 "() -> None"},
#endif
	{"CloseResFile", (PyCFunction)Res_CloseResFile, 1,
	 "(short refNum) -> None"},
	{"ResError", (PyCFunction)Res_ResError, 1,
	 "() -> (OSErr _rv)"},
	{"CurResFile", (PyCFunction)Res_CurResFile, 1,
	 "() -> (short _rv)"},

#if TARGET_API_MAC_OS8
	{"CreateResFile", (PyCFunction)Res_CreateResFile, 1,
	 "(Str255 fileName) -> None"},
#endif

#if TARGET_API_MAC_OS8
	{"OpenResFile", (PyCFunction)Res_OpenResFile, 1,
	 "(Str255 fileName) -> (short _rv)"},
#endif
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

#if TARGET_API_MAC_OS8
	{"RGetResource", (PyCFunction)Res_RGetResource, 1,
	 "(ResType theType, short theID) -> (Handle _rv)"},
#endif
	{"HOpenResFile", (PyCFunction)Res_HOpenResFile, 1,
	 "(short vRefNum, long dirID, Str255 fileName, SignedByte permission) -> (short _rv)"},
	{"HCreateResFile", (PyCFunction)Res_HCreateResFile, 1,
	 "(short vRefNum, long dirID, Str255 fileName) -> None"},
	{"FSpOpenResFile", (PyCFunction)Res_FSpOpenResFile, 1,
	 "(FSSpec spec, SignedByte permission) -> (short _rv)"},
	{"FSpCreateResFile", (PyCFunction)Res_FSpCreateResFile, 1,
	 "(FSSpec spec, OSType creator, OSType fileType, ScriptCode scriptTag) -> None"},

#if TARGET_API_MAC_CARBON
	{"InsertResourceFile", (PyCFunction)Res_InsertResourceFile, 1,
	 "(SInt16 refNum, RsrcChainLocation where) -> (OSErr _rv)"},
#endif

#if TARGET_API_MAC_CARBON
	{"DetachResourceFile", (PyCFunction)Res_DetachResourceFile, 1,
	 "(SInt16 refNum) -> (OSErr _rv)"},
#endif

#if TARGET_API_MAC_CARBON
	{"FSpResourceFileAlreadyOpen", (PyCFunction)Res_FSpResourceFileAlreadyOpen, 1,
	 "(FSSpec resourceFile) -> (Boolean _rv, Boolean inChain, SInt16 refNum)"},
#endif

#if TARGET_API_MAC_CARBON
	{"FSpOpenOrphanResFile", (PyCFunction)Res_FSpOpenOrphanResFile, 1,
	 "(FSSpec spec, SignedByte permission) -> (OSErr _rv, SInt16 refNum)"},
#endif

#if TARGET_API_MAC_CARBON
	{"GetTopResourceFile", (PyCFunction)Res_GetTopResourceFile, 1,
	 "() -> (OSErr _rv, SInt16 refNum)"},
#endif

#if TARGET_API_MAC_CARBON
	{"GetNextResourceFile", (PyCFunction)Res_GetNextResourceFile, 1,
	 "(SInt16 curRefNum) -> (OSErr _rv, SInt16 nextRefNum)"},
#endif
	{"Resource", (PyCFunction)Res_Resource, 1,
	 "Convert a string to a resource object.\n\nThe created resource object is actually just a handle,\napply AddResource() to write it to a resource file.\nSee also the Handle() docstring.\n"},
	{"Handle", (PyCFunction)Res_Handle, 1,
	 "Convert a string to a Handle object.\n\nResource() and Handle() are very similar, but objects created with Handle() are\nby default automatically DisposeHandle()d upon object cleanup. Use AutoDispose()\nto change this.\n"},
	{NULL, NULL, 0}
};




/* Alternative version of ResObj_New, which returns None for null argument */
PyObject *OptResObj_New(Handle itself)
{
	if (itself == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	return ResObj_New(itself);
}

OptResObj_Convert(PyObject *v, Handle *p_itself)
{
	PyObject *tmp;
	
	if ( v == Py_None ) {
		*p_itself = NULL;
		return 1;
	}
	if (ResObj_Check(v))
	{
		*p_itself = ((ResourceObject *)v)->ob_itself;
		return 1;
	}
	/* If it isn't a resource yet see whether it is convertible */
	if ( (tmp=PyObject_CallMethod(v, "as_Resource", "")) ) {
		*p_itself = ((ResourceObject *)tmp)->ob_itself;
		Py_DECREF(tmp);
		return 1;
	}
	PyErr_Clear();
	PyErr_SetString(PyExc_TypeError, "Resource required");
	return 0;
}


void initRes(void)
{
	PyObject *m;
	PyObject *d;



		PyMac_INIT_TOOLBOX_OBJECT_NEW(Handle, ResObj_New);
		PyMac_INIT_TOOLBOX_OBJECT_CONVERT(Handle, ResObj_Convert);
		PyMac_INIT_TOOLBOX_OBJECT_NEW(Handle, OptResObj_New);
		PyMac_INIT_TOOLBOX_OBJECT_CONVERT(Handle, OptResObj_Convert);


	m = Py_InitModule("Res", Res_methods);
	d = PyModule_GetDict(m);
	Res_Error = PyMac_GetOSErrException();
	if (Res_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Res_Error) != 0)
		return;
	Resource_Type.ob_type = &PyType_Type;
	Py_INCREF(&Resource_Type);
	if (PyDict_SetItemString(d, "ResourceType", (PyObject *)&Resource_Type) != 0)
		Py_FatalError("can't initialize ResourceType");
}

/* ========================= End module Res ========================= */

