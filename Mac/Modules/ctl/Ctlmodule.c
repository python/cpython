
/* =========================== Module Ctl =========================== */

#include "Python.h"



#define SystemSevenOrLater 1

#include "macglue.h"
#include <Memory.h>
#include <Dialogs.h>
#include <Menus.h>
#include <Controls.h>

extern PyObject *ResObj_New(Handle);
extern int ResObj_Convert(PyObject *, Handle *);

extern PyObject *WinObj_New(WindowPtr);
extern int WinObj_Convert(PyObject *, WindowPtr *);

extern PyObject *DlgObj_New(DialogPtr);
extern int DlgObj_Convert(PyObject *, DialogPtr *);
extern PyTypeObject Dialog_Type;
#define DlgObj_Check(x) ((x)->ob_type == &Dialog_Type)

extern PyObject *MenuObj_New(MenuHandle);
extern int MenuObj_Convert(PyObject *, MenuHandle *);

extern PyObject *CtlObj_New(ControlHandle);
extern int CtlObj_Convert(PyObject *, ControlHandle *);

extern PyObject *WinObj_WhichWindow(WindowPtr);

#include <Controls.h>

#define resNotFound -192 /* Can't include <Errors.h> because of Python's "errors.h" */

extern PyObject *CtlObj_WhichControl(ControlHandle); /* Forward */

#ifdef THINK_C
#define  ControlActionUPP ProcPtr
#endif

static PyObject *Ctl_Error;

/* ---------------------- Object type Control ----------------------- */

PyTypeObject Control_Type;

#define CtlObj_Check(x) ((x)->ob_type == &Control_Type)

typedef struct ControlObject {
	PyObject_HEAD
	ControlHandle ob_itself;
} ControlObject;

PyObject *CtlObj_New(itself)
	const ControlHandle itself;
{
	ControlObject *it;
	if (itself == NULL) return PyMac_Error(resNotFound);
	it = PyObject_NEW(ControlObject, &Control_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	SetCRefCon(itself, (long)it);
	return (PyObject *)it;
}
CtlObj_Convert(v, p_itself)
	PyObject *v;
	ControlHandle *p_itself;
{
	if (!CtlObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "Control required");
		return 0;
	}
	*p_itself = ((ControlObject *)v)->ob_itself;
	return 1;
}

static void CtlObj_dealloc(self)
	ControlObject *self;
{
	/* Cleanup of self->ob_itself goes here */
	PyMem_DEL(self);
}

static PyObject *CtlObj_SetCTitle(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Str255 title;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetStr255, title))
		return NULL;
	SetCTitle(_self->ob_itself,
	          title);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_GetCTitle(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Str255 title;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetStr255, title))
		return NULL;
	GetCTitle(_self->ob_itself,
	          title);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_DisposeControl(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	DisposeControl(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_HideControl(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	HideControl(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_ShowControl(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	ShowControl(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_Draw1Control(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	Draw1Control(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_HiliteControl(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short hiliteState;
	if (!PyArg_ParseTuple(_args, "h",
	                      &hiliteState))
		return NULL;
	HiliteControl(_self->ob_itself,
	              hiliteState);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_MoveControl(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short h;
	short v;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &h,
	                      &v))
		return NULL;
	MoveControl(_self->ob_itself,
	            h,
	            v);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_SizeControl(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short w;
	short h;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &w,
	                      &h))
		return NULL;
	SizeControl(_self->ob_itself,
	            w,
	            h);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_SetCtlValue(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short theValue;
	if (!PyArg_ParseTuple(_args, "h",
	                      &theValue))
		return NULL;
	SetCtlValue(_self->ob_itself,
	            theValue);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_GetCtlValue(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetCtlValue(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *CtlObj_SetCtlMin(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short minValue;
	if (!PyArg_ParseTuple(_args, "h",
	                      &minValue))
		return NULL;
	SetCtlMin(_self->ob_itself,
	          minValue);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_GetCtlMin(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetCtlMin(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *CtlObj_SetCtlMax(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short maxValue;
	if (!PyArg_ParseTuple(_args, "h",
	                      &maxValue))
		return NULL;
	SetCtlMax(_self->ob_itself,
	          maxValue);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_GetCtlMax(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetCtlMax(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *CtlObj_SetCRefCon(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long data;
	if (!PyArg_ParseTuple(_args, "l",
	                      &data))
		return NULL;
	SetCRefCon(_self->ob_itself,
	           data);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_GetCRefCon(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetCRefCon(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *CtlObj_DragControl(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Point startPt;
	Rect limitRect;
	Rect slopRect;
	short axis;
	if (!PyArg_ParseTuple(_args, "O&O&O&h",
	                      PyMac_GetPoint, &startPt,
	                      PyMac_GetRect, &limitRect,
	                      PyMac_GetRect, &slopRect,
	                      &axis))
		return NULL;
	DragControl(_self->ob_itself,
	            startPt,
	            &limitRect,
	            &slopRect,
	            axis);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_TestControl(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	Point thePt;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &thePt))
		return NULL;
	_rv = TestControl(_self->ob_itself,
	                  thePt);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *CtlObj_TrackControl(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	Point thePoint;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &thePoint))
		return NULL;
	_rv = TrackControl(_self->ob_itself,
	                   thePoint,
	                   (ControlActionUPP)0);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *CtlObj_GetCVariant(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetCVariant(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyMethodDef CtlObj_methods[] = {
	{"SetCTitle", (PyCFunction)CtlObj_SetCTitle, 1,
	 "(Str255 title) -> None"},
	{"GetCTitle", (PyCFunction)CtlObj_GetCTitle, 1,
	 "(Str255 title) -> None"},
	{"DisposeControl", (PyCFunction)CtlObj_DisposeControl, 1,
	 "() -> None"},
	{"HideControl", (PyCFunction)CtlObj_HideControl, 1,
	 "() -> None"},
	{"ShowControl", (PyCFunction)CtlObj_ShowControl, 1,
	 "() -> None"},
	{"Draw1Control", (PyCFunction)CtlObj_Draw1Control, 1,
	 "() -> None"},
	{"HiliteControl", (PyCFunction)CtlObj_HiliteControl, 1,
	 "(short hiliteState) -> None"},
	{"MoveControl", (PyCFunction)CtlObj_MoveControl, 1,
	 "(short h, short v) -> None"},
	{"SizeControl", (PyCFunction)CtlObj_SizeControl, 1,
	 "(short w, short h) -> None"},
	{"SetCtlValue", (PyCFunction)CtlObj_SetCtlValue, 1,
	 "(short theValue) -> None"},
	{"GetCtlValue", (PyCFunction)CtlObj_GetCtlValue, 1,
	 "() -> (short _rv)"},
	{"SetCtlMin", (PyCFunction)CtlObj_SetCtlMin, 1,
	 "(short minValue) -> None"},
	{"GetCtlMin", (PyCFunction)CtlObj_GetCtlMin, 1,
	 "() -> (short _rv)"},
	{"SetCtlMax", (PyCFunction)CtlObj_SetCtlMax, 1,
	 "(short maxValue) -> None"},
	{"GetCtlMax", (PyCFunction)CtlObj_GetCtlMax, 1,
	 "() -> (short _rv)"},
	{"SetCRefCon", (PyCFunction)CtlObj_SetCRefCon, 1,
	 "(long data) -> None"},
	{"GetCRefCon", (PyCFunction)CtlObj_GetCRefCon, 1,
	 "() -> (long _rv)"},
	{"DragControl", (PyCFunction)CtlObj_DragControl, 1,
	 "(Point startPt, Rect limitRect, Rect slopRect, short axis) -> None"},
	{"TestControl", (PyCFunction)CtlObj_TestControl, 1,
	 "(Point thePt) -> (short _rv)"},
	{"TrackControl", (PyCFunction)CtlObj_TrackControl, 1,
	 "(Point thePoint) -> (short _rv)"},
	{"GetCVariant", (PyCFunction)CtlObj_GetCVariant, 1,
	 "() -> (short _rv)"},
	{NULL, NULL, 0}
};

PyMethodChain CtlObj_chain = { CtlObj_methods, NULL };

static PyObject *CtlObj_getattr(self, name)
	ControlObject *self;
	char *name;
{
	return Py_FindMethodInChain(&CtlObj_chain, (PyObject *)self, name);
}

#define CtlObj_setattr NULL

PyTypeObject Control_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"Control", /*tp_name*/
	sizeof(ControlObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) CtlObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) CtlObj_getattr, /*tp_getattr*/
	(setattrfunc) CtlObj_setattr, /*tp_setattr*/
};

/* -------------------- End object type Control --------------------- */


static PyObject *Ctl_NewControl(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	ControlHandle _rv;
	WindowPtr theWindow;
	Rect boundsRect;
	Str255 title;
	Boolean visible;
	short value;
	short min;
	short max;
	short procID;
	long refCon;
	if (!PyArg_ParseTuple(_args, "O&O&O&bhhhhl",
	                      WinObj_Convert, &theWindow,
	                      PyMac_GetRect, &boundsRect,
	                      PyMac_GetStr255, title,
	                      &visible,
	                      &value,
	                      &min,
	                      &max,
	                      &procID,
	                      &refCon))
		return NULL;
	_rv = NewControl(theWindow,
	                 &boundsRect,
	                 title,
	                 visible,
	                 value,
	                 min,
	                 max,
	                 procID,
	                 refCon);
	_res = Py_BuildValue("O&",
	                     CtlObj_New, _rv);
	return _res;
}

static PyObject *Ctl_GetNewControl(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	ControlHandle _rv;
	short controlID;
	WindowPtr owner;
	if (!PyArg_ParseTuple(_args, "hO&",
	                      &controlID,
	                      WinObj_Convert, &owner))
		return NULL;
	_rv = GetNewControl(controlID,
	                    owner);
	_res = Py_BuildValue("O&",
	                     CtlObj_New, _rv);
	return _res;
}

static PyObject *Ctl_KillControls(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	WindowPtr theWindow;
	if (!PyArg_ParseTuple(_args, "O&",
	                      WinObj_Convert, &theWindow))
		return NULL;
	KillControls(theWindow);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Ctl_DrawControls(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	WindowPtr theWindow;
	if (!PyArg_ParseTuple(_args, "O&",
	                      WinObj_Convert, &theWindow))
		return NULL;
	DrawControls(theWindow);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Ctl_UpdtControl(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	WindowPtr theWindow;
	if (!PyArg_ParseTuple(_args, "O&",
	                      WinObj_Convert, &theWindow))
		return NULL;
	UpdtControl(theWindow,
	            theWindow->visRgn);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Ctl_UpdateControls(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	WindowPtr theWindow;
	if (!PyArg_ParseTuple(_args, "O&",
	                      WinObj_Convert, &theWindow))
		return NULL;
	UpdateControls(theWindow,
	               theWindow->visRgn);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Ctl_FindControl(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	Point thePoint;
	WindowPtr theWindow;
	ControlHandle theControl;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetPoint, &thePoint,
	                      WinObj_Convert, &theWindow))
		return NULL;
	_rv = FindControl(thePoint,
	                  theWindow,
	                  &theControl);
	_res = Py_BuildValue("hO&",
	                     _rv,
	                     CtlObj_WhichControl, theControl);
	return _res;
}

static PyMethodDef Ctl_methods[] = {
	{"NewControl", (PyCFunction)Ctl_NewControl, 1,
	 "(WindowPtr theWindow, Rect boundsRect, Str255 title, Boolean visible, short value, short min, short max, short procID, long refCon) -> (ControlHandle _rv)"},
	{"GetNewControl", (PyCFunction)Ctl_GetNewControl, 1,
	 "(short controlID, WindowPtr owner) -> (ControlHandle _rv)"},
	{"KillControls", (PyCFunction)Ctl_KillControls, 1,
	 "(WindowPtr theWindow) -> None"},
	{"DrawControls", (PyCFunction)Ctl_DrawControls, 1,
	 "(WindowPtr theWindow) -> None"},
	{"UpdtControl", (PyCFunction)Ctl_UpdtControl, 1,
	 "(WindowPtr theWindow) -> None"},
	{"UpdateControls", (PyCFunction)Ctl_UpdateControls, 1,
	 "(WindowPtr theWindow) -> None"},
	{"FindControl", (PyCFunction)Ctl_FindControl, 1,
	 "(Point thePoint, WindowPtr theWindow) -> (short _rv, ControlHandle theControl)"},
	{NULL, NULL, 0}
};



PyObject *
CtlObj_WhichControl(ControlHandle c)
{
	PyObject *it;
	
	/* XXX What if we find a control belonging to some other package? */
	if (c == NULL)
		it = NULL;
	else
		it = (PyObject *) GetCRefCon(c);
	if (it == NULL || ((ControlObject *)it)->ob_itself != c)
		it = Py_None;
	Py_INCREF(it);
	return it;
}


void initCtl()
{
	PyObject *m;
	PyObject *d;




	m = Py_InitModule("Ctl", Ctl_methods);
	d = PyModule_GetDict(m);
	Ctl_Error = PyMac_GetOSErrException();
	if (Ctl_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Ctl_Error) != 0)
		Py_FatalError("can't initialize Ctl.Error");
}

/* ========================= End module Ctl ========================= */

