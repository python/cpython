
/* =========================== Module Cm ============================ */

#include "Python.h"



#include "macglue.h"
#include "pymactoolbox.h"

#ifdef WITHOUT_FRAMEWORKS
#include <Components.h>
#else
#include <Carbon/Carbon.h>
#endif

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

#define CmpInstObj_Check(x) ((x)->ob_type == &ComponentInstance_Type)

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
CmpInstObj_Convert(PyObject *v, ComponentInstance *p_itself)
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
	PyMem_DEL(self);
}

static PyObject *CmpInstObj_CloseComponent(ComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
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
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &theStorage))
		return NULL;
	SetComponentInstanceStorage(_self->ob_itself,
	                            theStorage);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

#if !TARGET_API_MAC_CARBON

static PyObject *CmpInstObj_GetComponentInstanceA5(ComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetComponentInstanceA5(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *CmpInstObj_SetComponentInstanceA5(ComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long theA5;
	if (!PyArg_ParseTuple(_args, "l",
	                      &theA5))
		return NULL;
	SetComponentInstanceA5(_self->ob_itself,
	                       theA5);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

static PyObject *CmpInstObj_ComponentFunctionImplemented(ComponentInstanceObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
	short ftnNumber;
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
	if (!PyArg_ParseTuple(_args, "O&",
	                      CmpInstObj_Convert, &target))
		return NULL;
	_rv = ComponentSetTarget(_self->ob_itself,
	                         target);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyMethodDef CmpInstObj_methods[] = {
	{"CloseComponent", (PyCFunction)CmpInstObj_CloseComponent, 1,
	 "() -> None"},
	{"GetComponentInstanceError", (PyCFunction)CmpInstObj_GetComponentInstanceError, 1,
	 "() -> None"},
	{"SetComponentInstanceError", (PyCFunction)CmpInstObj_SetComponentInstanceError, 1,
	 "(OSErr theError) -> None"},
	{"GetComponentInstanceStorage", (PyCFunction)CmpInstObj_GetComponentInstanceStorage, 1,
	 "() -> (Handle _rv)"},
	{"SetComponentInstanceStorage", (PyCFunction)CmpInstObj_SetComponentInstanceStorage, 1,
	 "(Handle theStorage) -> None"},

#if !TARGET_API_MAC_CARBON
	{"GetComponentInstanceA5", (PyCFunction)CmpInstObj_GetComponentInstanceA5, 1,
	 "() -> (long _rv)"},
#endif

#if !TARGET_API_MAC_CARBON
	{"SetComponentInstanceA5", (PyCFunction)CmpInstObj_SetComponentInstanceA5, 1,
	 "(long theA5) -> None"},
#endif
	{"ComponentFunctionImplemented", (PyCFunction)CmpInstObj_ComponentFunctionImplemented, 1,
	 "(short ftnNumber) -> (long _rv)"},
	{"GetComponentVersion", (PyCFunction)CmpInstObj_GetComponentVersion, 1,
	 "() -> (long _rv)"},
	{"ComponentSetTarget", (PyCFunction)CmpInstObj_ComponentSetTarget, 1,
	 "(ComponentInstance target) -> (long _rv)"},
	{NULL, NULL, 0}
};

PyMethodChain CmpInstObj_chain = { CmpInstObj_methods, NULL };

static PyObject *CmpInstObj_getattr(ComponentInstanceObject *self, char *name)
{
	return Py_FindMethodInChain(&CmpInstObj_chain, (PyObject *)self, name);
}

#define CmpInstObj_setattr NULL

#define CmpInstObj_compare NULL

#define CmpInstObj_repr NULL

#define CmpInstObj_hash NULL

PyTypeObject ComponentInstance_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"ComponentInstance", /*tp_name*/
	sizeof(ComponentInstanceObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) CmpInstObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) CmpInstObj_getattr, /*tp_getattr*/
	(setattrfunc) CmpInstObj_setattr, /*tp_setattr*/
	(cmpfunc) CmpInstObj_compare, /*tp_compare*/
	(reprfunc) CmpInstObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) CmpInstObj_hash, /*tp_hash*/
};

/* --------------- End object type ComponentInstance ---------------- */


/* --------------------- Object type Component ---------------------- */

PyTypeObject Component_Type;

#define CmpObj_Check(x) ((x)->ob_type == &Component_Type)

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
CmpObj_Convert(PyObject *v, Component *p_itself)
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
	PyMem_DEL(self);
}

static PyObject *CmpObj_UnregisterComponent(ComponentObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
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
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = OpenComponent(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     CmpInstObj_New, _rv);
	return _res;
}

static PyObject *CmpObj_GetComponentRefcon(ComponentObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
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

static PyObject *CmpObj_ResolveComponentAlias(ComponentObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Component _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = ResolveComponentAlias(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     CmpObj_New, _rv);
	return _res;
}

static PyObject *CmpObj_CountComponentInstances(ComponentObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
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
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = UncaptureComponent(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CmpObj_GetComponentIconSuite(ComponentObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle iconSuite;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetComponentIconSuite(_self->ob_itself,
	                             &iconSuite);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, iconSuite);
	return _res;
}

static PyMethodDef CmpObj_methods[] = {
	{"UnregisterComponent", (PyCFunction)CmpObj_UnregisterComponent, 1,
	 "() -> None"},
	{"GetComponentInfo", (PyCFunction)CmpObj_GetComponentInfo, 1,
	 "(Handle componentName, Handle componentInfo, Handle componentIcon) -> (ComponentDescription cd)"},
	{"OpenComponent", (PyCFunction)CmpObj_OpenComponent, 1,
	 "() -> (ComponentInstance _rv)"},
	{"GetComponentRefcon", (PyCFunction)CmpObj_GetComponentRefcon, 1,
	 "() -> (long _rv)"},
	{"SetComponentRefcon", (PyCFunction)CmpObj_SetComponentRefcon, 1,
	 "(long theRefcon) -> None"},
	{"OpenComponentResFile", (PyCFunction)CmpObj_OpenComponentResFile, 1,
	 "() -> (short _rv)"},
	{"GetComponentResource", (PyCFunction)CmpObj_GetComponentResource, 1,
	 "(OSType resType, short resID) -> (Handle theResource)"},
	{"GetComponentIndString", (PyCFunction)CmpObj_GetComponentIndString, 1,
	 "(Str255 theString, short strListID, short index) -> None"},
	{"ResolveComponentAlias", (PyCFunction)CmpObj_ResolveComponentAlias, 1,
	 "() -> (Component _rv)"},
	{"CountComponentInstances", (PyCFunction)CmpObj_CountComponentInstances, 1,
	 "() -> (long _rv)"},
	{"SetDefaultComponent", (PyCFunction)CmpObj_SetDefaultComponent, 1,
	 "(short flags) -> None"},
	{"CaptureComponent", (PyCFunction)CmpObj_CaptureComponent, 1,
	 "(Component capturingComponent) -> (Component _rv)"},
	{"UncaptureComponent", (PyCFunction)CmpObj_UncaptureComponent, 1,
	 "() -> None"},
	{"GetComponentIconSuite", (PyCFunction)CmpObj_GetComponentIconSuite, 1,
	 "() -> (Handle iconSuite)"},
	{NULL, NULL, 0}
};

PyMethodChain CmpObj_chain = { CmpObj_methods, NULL };

static PyObject *CmpObj_getattr(ComponentObject *self, char *name)
{
	return Py_FindMethodInChain(&CmpObj_chain, (PyObject *)self, name);
}

#define CmpObj_setattr NULL

#define CmpObj_compare NULL

#define CmpObj_repr NULL

#define CmpObj_hash NULL

PyTypeObject Component_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"Component", /*tp_name*/
	sizeof(ComponentObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) CmpObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) CmpObj_getattr, /*tp_getattr*/
	(setattrfunc) CmpObj_setattr, /*tp_setattr*/
	(cmpfunc) CmpObj_compare, /*tp_compare*/
	(reprfunc) CmpObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) CmpObj_hash, /*tp_hash*/
};

/* ------------------- End object type Component -------------------- */


static PyObject *Cm_RegisterComponentResource(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Component _rv;
	ComponentResourceHandle cr;
	short global;
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
	 "(ComponentResourceHandle cr, short global) -> (Component _rv)"},
	{"FindNextComponent", (PyCFunction)Cm_FindNextComponent, 1,
	 "(Component aComponent, ComponentDescription looking) -> (Component _rv)"},
	{"CountComponents", (PyCFunction)Cm_CountComponents, 1,
	 "(ComponentDescription looking) -> (long _rv)"},
	{"GetComponentListModSeed", (PyCFunction)Cm_GetComponentListModSeed, 1,
	 "() -> (long _rv)"},
	{"CloseComponentResFile", (PyCFunction)Cm_CloseComponentResFile, 1,
	 "(short refnum) -> None"},
	{"OpenDefaultComponent", (PyCFunction)Cm_OpenDefaultComponent, 1,
	 "(OSType componentType, OSType componentSubType) -> (ComponentInstance _rv)"},
	{"RegisterComponentResourceFile", (PyCFunction)Cm_RegisterComponentResourceFile, 1,
	 "(short resRefNum, short global) -> (long _rv)"},
	{NULL, NULL, 0}
};




void initCm(void)
{
	PyObject *m;
	PyObject *d;



		PyMac_INIT_TOOLBOX_OBJECT_NEW(Component, CmpObj_New);
		PyMac_INIT_TOOLBOX_OBJECT_CONVERT(Component, CmpObj_Convert);
		PyMac_INIT_TOOLBOX_OBJECT_NEW(ComponentInstance, CmpInstObj_New);
		PyMac_INIT_TOOLBOX_OBJECT_CONVERT(ComponentInstance, CmpInstObj_Convert);


	m = Py_InitModule("Cm", Cm_methods);
	d = PyModule_GetDict(m);
	Cm_Error = PyMac_GetOSErrException();
	if (Cm_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Cm_Error) != 0)
		return;
	ComponentInstance_Type.ob_type = &PyType_Type;
	Py_INCREF(&ComponentInstance_Type);
	if (PyDict_SetItemString(d, "ComponentInstanceType", (PyObject *)&ComponentInstance_Type) != 0)
		Py_FatalError("can't initialize ComponentInstanceType");
	Component_Type.ob_type = &PyType_Type;
	Py_INCREF(&Component_Type);
	if (PyDict_SetItemString(d, "ComponentType", (PyObject *)&Component_Type) != 0)
		Py_FatalError("can't initialize ComponentType");
}

/* ========================= End module Cm ========================== */

