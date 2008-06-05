
/* =========================== Module _Cm =========================== */

#include "Python.h"



#include "pymactoolbox.h"

/* Macro to test whether a weak-loaded CFM function exists */
#define PyMac_PRECHECK(rtn) do { if ( &rtn == NULL )  {\
        PyErr_SetString(PyExc_NotImplementedError, \
        "Not available in this shared library/OS version"); \
        return NULL; \
    }} while(0)


#include <Carbon/Carbon.h>

#ifdef USE_TOOLBOX_OBJECT_GLUE
extern PyObject *_CmpObj_New(Component);
extern int _CmpObj_Convert(PyObject *, Component *);
extern PyObject *_CmpInstObj_New(ComponentInstance);
extern int _CmpInstObj_Convert(PyObject *, ComponentInstance *);

#define CmpObj_New _CmpObj_New
#define CmpObj_Convert _CmpObj_Convert
#define CmpInstObj_New _CmpInstObj_New
#define CmpInstObj_Convert _CmpInstObj_Convert
#endif

/*
** Parse/generate ComponentDescriptor records
*/
static PyObject *
CmpDesc_New(ComponentDescription *itself)
{

        return Py_BuildValue("O&O&O&ll",
                PyMac_BuildOSType, itself->componentType,
                PyMac_BuildOSType, itself->componentSubType,
                PyMac_BuildOSType, itself->componentManufacturer,
                itself->componentFlags, itself->componentFlagsMask);
}

static int
CmpDesc_Convert(PyObject *v, ComponentDescription *p_itself)
{
        return PyArg_ParseTuple(v, "O&O&O&ll",
                PyMac_GetOSType, &p_itself->componentType,
                PyMac_GetOSType, &p_itself->componentSubType,
                PyMac_GetOSType, &p_itself->componentManufacturer,
                &p_itself->componentFlags, &p_itself->componentFlagsMask);
}


static PyObject *Cm_Error;

/* ----------------- Object type ComponentInstance ------------------ */

PyTypeObject ComponentInstance_Type;

#define CmpInstObj_Check(x) ((x)->ob_type == &ComponentInstance_Type || PyObject_TypeCheck((x), &ComponentInstance_Type))

typedef struct ComponentInstanceObject {
	PyObject_HEAD
	ComponentInstance ob_itself;
} ComponentInstanceObject;

PyObject *CmpInstObj_New(ComponentInstance itself)
{
	ComponentInstanceObject *it;
	if (itself == NULL) {
	                                PyErr_SetString(Cm_Error,"NULL ComponentInstance");
	                                return NULL;
	                        }
	it = PyObject_NEW(ComponentInstanceObject, &ComponentInstance_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}

int CmpInstObj_Convert(PyObject *v, ComponentInstance *p_itself)
{
	if (!CmpInstObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "ComponentInstance required");
		return 0;
	}
	*p_itself = ((ComponentInstanceObject *)v)->ob_itself;
	return 1;
}

static void CmpInstObj_dealloc(ComponentInstanceObject *self)
{
	/* Cleanup of self->ob_itself goes here */
	self->ob_type->tp_free((PyObject *)self);
}

static PyObject *CmpInstObj_CloseComponent(ComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
#ifndef CloseComponent
	PyMac_PRECHECK(CloseComponent);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = CloseComponent(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CmpInstObj_GetComponentInstanceError(ComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
#ifndef GetComponentInstanceError
	PyMac_PRECHECK(GetComponentInstanceError);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetComponentInstanceError(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CmpInstObj_SetComponentInstanceError(ComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr theError;
#ifndef SetComponentInstanceError
	PyMac_PRECHECK(SetComponentInstanceError);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &theError))
		return NULL;
	SetComponentInstanceError(_self->ob_itself,
	                          theError);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CmpInstObj_GetComponentInstanceStorage(ComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Handle _rv;
#ifndef GetComponentInstanceStorage
	PyMac_PRECHECK(GetComponentInstanceStorage);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetComponentInstanceStorage(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *CmpInstObj_SetComponentInstanceStorage(ComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Handle theStorage;
#ifndef SetComponentInstanceStorage
	PyMac_PRECHECK(SetComponentInstanceStorage);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &theStorage))
		return NULL;
	SetComponentInstanceStorage(_self->ob_itself,
	                            theStorage);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

#ifndef __LP64__
static PyObject *CmpInstObj_ComponentFunctionImplemented(ComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
	short ftnNumber;
#ifndef ComponentFunctionImplemented
	PyMac_PRECHECK(ComponentFunctionImplemented);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &ftnNumber))
		return NULL;
	_rv = ComponentFunctionImplemented(_self->ob_itself,
	                                   ftnNumber);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *CmpInstObj_GetComponentVersion(ComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
#ifndef GetComponentVersion
	PyMac_PRECHECK(GetComponentVersion);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetComponentVersion(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *CmpInstObj_ComponentSetTarget(ComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
	ComponentInstance target;
#ifndef ComponentSetTarget
	PyMac_PRECHECK(ComponentSetTarget);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &target))
		return NULL;
	_rv = ComponentSetTarget(_self->ob_itself,
	                         target);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}
#endif /* !__LP64__*/

static PyMethodDef CmpInstObj_methods[] = {
	{"CloseComponent", (PyCFunction)CmpInstObj_CloseComponent, 1,
	 PyDoc_STR("() -> None")},
	{"GetComponentInstanceError", (PyCFunction)CmpInstObj_GetComponentInstanceError, 1,
	 PyDoc_STR("() -> None")},
	{"SetComponentInstanceError", (PyCFunction)CmpInstObj_SetComponentInstanceError, 1,
	 PyDoc_STR("(OSErr theError) -> None")},
	{"GetComponentInstanceStorage", (PyCFunction)CmpInstObj_GetComponentInstanceStorage, 1,
	 PyDoc_STR("() -> (Handle _rv)")},
	{"SetComponentInstanceStorage", (PyCFunction)CmpInstObj_SetComponentInstanceStorage, 1,
	 PyDoc_STR("(Handle theStorage) -> None")},
#ifndef __LP64__
	{"ComponentFunctionImplemented", (PyCFunction)CmpInstObj_ComponentFunctionImplemented, 1,
	 PyDoc_STR("(short ftnNumber) -> (long _rv)")},
	{"GetComponentVersion", (PyCFunction)CmpInstObj_GetComponentVersion, 1,
	 PyDoc_STR("() -> (long _rv)")},
	{"ComponentSetTarget", (PyCFunction)CmpInstObj_ComponentSetTarget, 1,
	 PyDoc_STR("(ComponentInstance target) -> (long _rv)")},
#endif /* !__LP64__ */
	{NULL, NULL, 0}
};

#define CmpInstObj_getsetlist NULL


#define CmpInstObj_compare NULL

#define CmpInstObj_repr NULL

#define CmpInstObj_hash NULL
#define CmpInstObj_tp_init 0

#define CmpInstObj_tp_alloc PyType_GenericAlloc

static PyObject *CmpInstObj_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
	PyObject *_self;
	ComponentInstance itself;
	char *kw[] = {"itself", 0};

	if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, CmpInstObj_Convert, &itself)) return NULL;
	if ((_self = type->tp_alloc(type, 0)) == NULL) return NULL;
	((ComponentInstanceObject *)_self)->ob_itself = itself;
	return _self;
}

#define CmpInstObj_tp_free PyObject_Del


PyTypeObject ComponentInstance_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"_Cm.ComponentInstance", /*tp_name*/
	sizeof(ComponentInstanceObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) CmpInstObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc)0, /*tp_getattr*/
	(setattrfunc)0, /*tp_setattr*/
	(cmpfunc) CmpInstObj_compare, /*tp_compare*/
	(reprfunc) CmpInstObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) CmpInstObj_hash, /*tp_hash*/
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
	CmpInstObj_methods, /* tp_methods */
	0, /*tp_members*/
	CmpInstObj_getsetlist, /*tp_getset*/
	0, /*tp_base*/
	0, /*tp_dict*/
	0, /*tp_descr_get*/
	0, /*tp_descr_set*/
	0, /*tp_dictoffset*/
	CmpInstObj_tp_init, /* tp_init */
	CmpInstObj_tp_alloc, /* tp_alloc */
	CmpInstObj_tp_new, /* tp_new */
	CmpInstObj_tp_free, /* tp_free */
};

/* --------------- End object type ComponentInstance ---------------- */


/* --------------------- Object type Component ---------------------- */

PyTypeObject Component_Type;

#define CmpObj_Check(x) ((x)->ob_type == &Component_Type || PyObject_TypeCheck((x), &Component_Type))

typedef struct ComponentObject {
	PyObject_HEAD
	Component ob_itself;
} ComponentObject;

PyObject *CmpObj_New(Component itself)
{
	ComponentObject *it;
	if (itself == NULL) {
	                                /* XXXX Or should we return None? */
	                                PyErr_SetString(Cm_Error,"No such component");
	                                return NULL;
	                        }
	it = PyObject_NEW(ComponentObject, &Component_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}

int CmpObj_Convert(PyObject *v, Component *p_itself)
{
	if ( v == Py_None ) {
	                                *p_itself = 0;
	                                return 1;
	        }
	if (!CmpObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "Component required");
		return 0;
	}
	*p_itself = ((ComponentObject *)v)->ob_itself;
	return 1;
}

static void CmpObj_dealloc(ComponentObject *self)
{
	/* Cleanup of self->ob_itself goes here */
	self->ob_type->tp_free((PyObject *)self);
}

static PyObject *CmpObj_UnregisterComponent(ComponentObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
#ifndef UnregisterComponent
	PyMac_PRECHECK(UnregisterComponent);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = UnregisterComponent(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CmpObj_GetComponentInfo(ComponentObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	ComponentDescription cd;
	Handle componentName;
	Handle componentInfo;
	Handle componentIcon;
#ifndef GetComponentInfo
	PyMac_PRECHECK(GetComponentInfo);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      ResObj_Convert, &componentName,
	                      ResObj_Convert, &componentInfo,
	                      ResObj_Convert, &componentIcon))
		return NULL;
	_err = GetComponentInfo(_self->ob_itself,
	                        &cd,
	                        componentName,
	                        componentInfo,
	                        componentIcon);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     CmpDesc_New, &cd);
	return _res;
}

static PyObject *CmpObj_OpenComponent(ComponentObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentInstance _rv;
#ifndef OpenComponent
	PyMac_PRECHECK(OpenComponent);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = OpenComponent(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     CmpInstObj_New, _rv);
	return _res;
}

static PyObject *CmpObj_ResolveComponentAlias(ComponentObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Component _rv;
#ifndef ResolveComponentAlias
	PyMac_PRECHECK(ResolveComponentAlias);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = ResolveComponentAlias(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     CmpObj_New, _rv);
	return _res;
}

static PyObject *CmpObj_GetComponentPublicIndString(ComponentObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Str255 theString;
	short strListID;
	short index;
#ifndef GetComponentPublicIndString
	PyMac_PRECHECK(GetComponentPublicIndString);
#endif
	if (!PyArg_ParseTuple(_args, "O&hh",
	                      PyMac_GetStr255, theString,
	                      &strListID,
	                      &index))
		return NULL;
	_err = GetComponentPublicIndString(_self->ob_itself,
	                                   theString,
	                                   strListID,
	                                   index);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CmpObj_GetComponentRefcon(ComponentObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
#ifndef GetComponentRefcon
	PyMac_PRECHECK(GetComponentRefcon);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetComponentRefcon(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *CmpObj_SetComponentRefcon(ComponentObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long theRefcon;
#ifndef SetComponentRefcon
	PyMac_PRECHECK(SetComponentRefcon);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &theRefcon))
		return NULL;
	SetComponentRefcon(_self->ob_itself,
	                   theRefcon);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CmpObj_OpenComponentResFile(ComponentObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short _rv;
#ifndef OpenComponentResFile
	PyMac_PRECHECK(OpenComponentResFile);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = OpenComponentResFile(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *CmpObj_GetComponentResource(ComponentObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	OSType resType;
	short resID;
	Handle theResource;
#ifndef GetComponentResource
	PyMac_PRECHECK(GetComponentResource);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      PyMac_GetOSType, &resType,
	                      &resID))
		return NULL;
	_err = GetComponentResource(_self->ob_itself,
	                            resType,
	                            resID,
	                            &theResource);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, theResource);
	return _res;
}

static PyObject *CmpObj_GetComponentIndString(ComponentObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Str255 theString;
	short strListID;
	short index;
#ifndef GetComponentIndString
	PyMac_PRECHECK(GetComponentIndString);
#endif
	if (!PyArg_ParseTuple(_args, "O&hh",
	                      PyMac_GetStr255, theString,
	                      &strListID,
	                      &index))
		return NULL;
	_err = GetComponentIndString(_self->ob_itself,
	                             theString,
	                             strListID,
	                             index);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CmpObj_CountComponentInstances(ComponentObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
#ifndef CountComponentInstances
	PyMac_PRECHECK(CountComponentInstances);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = CountComponentInstances(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *CmpObj_SetDefaultComponent(ComponentObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short flags;
#ifndef SetDefaultComponent
	PyMac_PRECHECK(SetDefaultComponent);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &flags))
		return NULL;
	_err = SetDefaultComponent(_self->ob_itself,
	                           flags);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CmpObj_CaptureComponent(ComponentObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Component _rv;
	Component capturingComponent;
#ifndef CaptureComponent
	PyMac_PRECHECK(CaptureComponent);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpObj_Convert, &capturingComponent))
		return NULL;
	_rv = CaptureComponent(_self->ob_itself,
	                       capturingComponent);
	_res = Py_BuildValue("O&",
	                     CmpObj_New, _rv);
	return _res;
}

static PyObject *CmpObj_UncaptureComponent(ComponentObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
#ifndef UncaptureComponent
	PyMac_PRECHECK(UncaptureComponent);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = UncaptureComponent(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

#ifndef __LP64__
static PyObject *CmpObj_GetComponentIconSuite(ComponentObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle iconSuite;
#ifndef GetComponentIconSuite
	PyMac_PRECHECK(GetComponentIconSuite);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetComponentIconSuite(_self->ob_itself,
	                             &iconSuite);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, iconSuite);
	return _res;
}
#endif /* !__LP64__ */

static PyMethodDef CmpObj_methods[] = {
	{"UnregisterComponent", (PyCFunction)CmpObj_UnregisterComponent, 1,
	 PyDoc_STR("() -> None")},
	{"GetComponentInfo", (PyCFunction)CmpObj_GetComponentInfo, 1,
	 PyDoc_STR("(Handle componentName, Handle componentInfo, Handle componentIcon) -> (ComponentDescription cd)")},
	{"OpenComponent", (PyCFunction)CmpObj_OpenComponent, 1,
	 PyDoc_STR("() -> (ComponentInstance _rv)")},
	{"ResolveComponentAlias", (PyCFunction)CmpObj_ResolveComponentAlias, 1,
	 PyDoc_STR("() -> (Component _rv)")},
	{"GetComponentPublicIndString", (PyCFunction)CmpObj_GetComponentPublicIndString, 1,
	 PyDoc_STR("(Str255 theString, short strListID, short index) -> None")},
	{"GetComponentRefcon", (PyCFunction)CmpObj_GetComponentRefcon, 1,
	 PyDoc_STR("() -> (long _rv)")},
	{"SetComponentRefcon", (PyCFunction)CmpObj_SetComponentRefcon, 1,
	 PyDoc_STR("(long theRefcon) -> None")},
	{"OpenComponentResFile", (PyCFunction)CmpObj_OpenComponentResFile, 1,
	 PyDoc_STR("() -> (short _rv)")},
	{"GetComponentResource", (PyCFunction)CmpObj_GetComponentResource, 1,
	 PyDoc_STR("(OSType resType, short resID) -> (Handle theResource)")},
	{"GetComponentIndString", (PyCFunction)CmpObj_GetComponentIndString, 1,
	 PyDoc_STR("(Str255 theString, short strListID, short index) -> None")},
	{"CountComponentInstances", (PyCFunction)CmpObj_CountComponentInstances, 1,
	 PyDoc_STR("() -> (long _rv)")},
	{"SetDefaultComponent", (PyCFunction)CmpObj_SetDefaultComponent, 1,
	 PyDoc_STR("(short flags) -> None")},
	{"CaptureComponent", (PyCFunction)CmpObj_CaptureComponent, 1,
	 PyDoc_STR("(Component capturingComponent) -> (Component _rv)")},
	{"UncaptureComponent", (PyCFunction)CmpObj_UncaptureComponent, 1,
	 PyDoc_STR("() -> None")},
#ifndef __LP64__
	{"GetComponentIconSuite", (PyCFunction)CmpObj_GetComponentIconSuite, 1,
	 PyDoc_STR("() -> (Handle iconSuite)")},
#endif /* !__LP64__ */
	{NULL, NULL, 0}
};

#define CmpObj_getsetlist NULL


#define CmpObj_compare NULL

#define CmpObj_repr NULL

#define CmpObj_hash NULL
#define CmpObj_tp_init 0

#define CmpObj_tp_alloc PyType_GenericAlloc

static PyObject *CmpObj_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
	PyObject *_self;
	Component itself;
	char *kw[] = {"itself", 0};

	if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, CmpObj_Convert, &itself)) return NULL;
	if ((_self = type->tp_alloc(type, 0)) == NULL) return NULL;
	((ComponentObject *)_self)->ob_itself = itself;
	return _self;
}

#define CmpObj_tp_free PyObject_Del


PyTypeObject Component_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"_Cm.Component", /*tp_name*/
	sizeof(ComponentObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) CmpObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc)0, /*tp_getattr*/
	(setattrfunc)0, /*tp_setattr*/
	(cmpfunc) CmpObj_compare, /*tp_compare*/
	(reprfunc) CmpObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) CmpObj_hash, /*tp_hash*/
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
	CmpObj_methods, /* tp_methods */
	0, /*tp_members*/
	CmpObj_getsetlist, /*tp_getset*/
	0, /*tp_base*/
	0, /*tp_dict*/
	0, /*tp_descr_get*/
	0, /*tp_descr_set*/
	0, /*tp_dictoffset*/
	CmpObj_tp_init, /* tp_init */
	CmpObj_tp_alloc, /* tp_alloc */
	CmpObj_tp_new, /* tp_new */
	CmpObj_tp_free, /* tp_free */
};

/* ------------------- End object type Component -------------------- */


static PyObject *Cm_RegisterComponentResource(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Component _rv;
	ComponentResourceHandle cr;
	short global;
#ifndef RegisterComponentResource
	PyMac_PRECHECK(RegisterComponentResource);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      ResObj_Convert, &cr,
	                      &global))
		return NULL;
	_rv = RegisterComponentResource(cr,
	                                global);
	_res = Py_BuildValue("O&",
	                     CmpObj_New, _rv);
	return _res;
}

static PyObject *Cm_FindNextComponent(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Component _rv;
	Component aComponent;
	ComponentDescription looking;
#ifndef FindNextComponent
	PyMac_PRECHECK(FindNextComponent);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      CmpObj_Convert, &aComponent,
	                      CmpDesc_Convert, &looking))
		return NULL;
	_rv = FindNextComponent(aComponent,
	                        &looking);
	_res = Py_BuildValue("O&",
	                     CmpObj_New, _rv);
	return _res;
}

static PyObject *Cm_CountComponents(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
	ComponentDescription looking;
#ifndef CountComponents
	PyMac_PRECHECK(CountComponents);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpDesc_Convert, &looking))
		return NULL;
	_rv = CountComponents(&looking);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Cm_GetComponentListModSeed(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
#ifndef GetComponentListModSeed
	PyMac_PRECHECK(GetComponentListModSeed);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetComponentListModSeed();
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Cm_CloseComponentResFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short refnum;
#ifndef CloseComponentResFile
	PyMac_PRECHECK(CloseComponentResFile);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &refnum))
		return NULL;
	_err = CloseComponentResFile(refnum);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Cm_OpenDefaultComponent(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ComponentInstance _rv;
	OSType componentType;
	OSType componentSubType;
#ifndef OpenDefaultComponent
	PyMac_PRECHECK(OpenDefaultComponent);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetOSType, &componentType,
	                      PyMac_GetOSType, &componentSubType))
		return NULL;
	_rv = OpenDefaultComponent(componentType,
	                           componentSubType);
	_res = Py_BuildValue("O&",
	                     CmpInstObj_New, _rv);
	return _res;
}

static PyObject *Cm_RegisterComponentResourceFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
	short resRefNum;
	short global;
#ifndef RegisterComponentResourceFile
	PyMac_PRECHECK(RegisterComponentResourceFile);
#endif
	if (!PyArg_ParseTuple(_args, "hh",
	                      &resRefNum,
	                      &global))
		return NULL;
	_rv = RegisterComponentResourceFile(resRefNum,
	                                    global);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyMethodDef Cm_methods[] = {
	{"RegisterComponentResource", (PyCFunction)Cm_RegisterComponentResource, 1,
	 PyDoc_STR("(ComponentResourceHandle cr, short global) -> (Component _rv)")},
	{"FindNextComponent", (PyCFunction)Cm_FindNextComponent, 1,
	 PyDoc_STR("(Component aComponent, ComponentDescription looking) -> (Component _rv)")},
	{"CountComponents", (PyCFunction)Cm_CountComponents, 1,
	 PyDoc_STR("(ComponentDescription looking) -> (long _rv)")},
	{"GetComponentListModSeed", (PyCFunction)Cm_GetComponentListModSeed, 1,
	 PyDoc_STR("() -> (long _rv)")},
	{"CloseComponentResFile", (PyCFunction)Cm_CloseComponentResFile, 1,
	 PyDoc_STR("(short refnum) -> None")},
	{"OpenDefaultComponent", (PyCFunction)Cm_OpenDefaultComponent, 1,
	 PyDoc_STR("(OSType componentType, OSType componentSubType) -> (ComponentInstance _rv)")},
	{"RegisterComponentResourceFile", (PyCFunction)Cm_RegisterComponentResourceFile, 1,
	 PyDoc_STR("(short resRefNum, short global) -> (long _rv)")},
	{NULL, NULL, 0}
};




void init_Cm(void)
{
	PyObject *m;
	PyObject *d;



	        PyMac_INIT_TOOLBOX_OBJECT_NEW(Component, CmpObj_New);
	        PyMac_INIT_TOOLBOX_OBJECT_CONVERT(Component, CmpObj_Convert);
	        PyMac_INIT_TOOLBOX_OBJECT_NEW(ComponentInstance, CmpInstObj_New);
	        PyMac_INIT_TOOLBOX_OBJECT_CONVERT(ComponentInstance, CmpInstObj_Convert);


	m = Py_InitModule("_Cm", Cm_methods);
	d = PyModule_GetDict(m);
	Cm_Error = PyMac_GetOSErrException();
	if (Cm_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Cm_Error) != 0)
		return;
	ComponentInstance_Type.ob_type = &PyType_Type;
	if (PyType_Ready(&ComponentInstance_Type) < 0) return;
	Py_INCREF(&ComponentInstance_Type);
	PyModule_AddObject(m, "ComponentInstance", (PyObject *)&ComponentInstance_Type);
	/* Backward-compatible name */
	Py_INCREF(&ComponentInstance_Type);
	PyModule_AddObject(m, "ComponentInstanceType", (PyObject *)&ComponentInstance_Type);
	Component_Type.ob_type = &PyType_Type;
	if (PyType_Ready(&Component_Type) < 0) return;
	Py_INCREF(&Component_Type);
	PyModule_AddObject(m, "Component", (PyObject *)&Component_Type);
	/* Backward-compatible name */
	Py_INCREF(&Component_Type);
	PyModule_AddObject(m, "ComponentType", (PyObject *)&Component_Type);
}

/* ========================= End module _Cm ========================= */

