
/* =========================== Module Evt =========================== */

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

#include <Events.h>

#define resNotFound -192 /* Can't include <Errors.h> because of Python's "errors.h" */

static PyObject *Evt_Error;

static PyObject *Evt_GetCaretTime(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	UInt32 _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetCaretTime();
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Evt_SetEventMask(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	EventMask value;
	if (!PyArg_ParseTuple(_args, "h",
	                      &value))
		return NULL;
	SetEventMask(value);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Evt_GetDblTime(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	UInt32 _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetDblTime();
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Evt_GetNextEvent(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	EventMask eventMask;
	EventRecord theEvent;
	if (!PyArg_ParseTuple(_args, "h",
	                      &eventMask))
		return NULL;
	_rv = GetNextEvent(eventMask,
	                   &theEvent);
	_res = Py_BuildValue("bO&",
	                     _rv,
	                     PyMac_BuildEventRecord, &theEvent);
	return _res;
}

static PyObject *Evt_WaitNextEvent(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	EventMask eventMask;
	EventRecord theEvent;
	UInt32 sleep;
	if (!PyArg_ParseTuple(_args, "hl",
	                      &eventMask,
	                      &sleep))
		return NULL;
	_rv = WaitNextEvent(eventMask,
	                    &theEvent,
	                    sleep,
	                    (RgnHandle)0);
	_res = Py_BuildValue("bO&",
	                     _rv,
	                     PyMac_BuildEventRecord, &theEvent);
	return _res;
}

static PyObject *Evt_EventAvail(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	EventMask eventMask;
	EventRecord theEvent;
	if (!PyArg_ParseTuple(_args, "h",
	                      &eventMask))
		return NULL;
	_rv = EventAvail(eventMask,
	                 &theEvent);
	_res = Py_BuildValue("bO&",
	                     _rv,
	                     PyMac_BuildEventRecord, &theEvent);
	return _res;
}

static PyObject *Evt_GetMouse(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Point mouseLoc;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetMouse(&mouseLoc);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildPoint, mouseLoc);
	return _res;
}

static PyObject *Evt_Button(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = Button();
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Evt_StillDown(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = StillDown();
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Evt_WaitMouseUp(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WaitMouseUp();
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Evt_GetKeys(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	KeyMap theKeys__out__;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetKeys(theKeys__out__);
	_res = Py_BuildValue("s#",
	                     (char *)&theKeys__out__, (int)sizeof(KeyMap));
 theKeys__error__: ;
	return _res;
}

static PyObject *Evt_TickCount(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	UInt32 _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = TickCount();
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Evt_PostEvent(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	EventKind eventNum;
	UInt32 eventMsg;
	if (!PyArg_ParseTuple(_args, "hl",
	                      &eventNum,
	                      &eventMsg))
		return NULL;
	_err = PostEvent(eventNum,
	                 eventMsg);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Evt_OSEventAvail(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	EventMask mask;
	EventRecord theEvent;
	if (!PyArg_ParseTuple(_args, "h",
	                      &mask))
		return NULL;
	_rv = OSEventAvail(mask,
	                   &theEvent);
	_res = Py_BuildValue("bO&",
	                     _rv,
	                     PyMac_BuildEventRecord, &theEvent);
	return _res;
}

static PyObject *Evt_GetOSEvent(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	EventMask mask;
	EventRecord theEvent;
	if (!PyArg_ParseTuple(_args, "h",
	                      &mask))
		return NULL;
	_rv = GetOSEvent(mask,
	                 &theEvent);
	_res = Py_BuildValue("bO&",
	                     _rv,
	                     PyMac_BuildEventRecord, &theEvent);
	return _res;
}

static PyObject *Evt_FlushEvents(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	EventMask whichMask;
	EventMask stopMask;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &whichMask,
	                      &stopMask))
		return NULL;
	FlushEvents(whichMask,
	            stopMask);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Evt_SystemClick(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	EventRecord theEvent;
	WindowPtr theWindow;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetEventRecord, &theEvent,
	                      WinObj_Convert, &theWindow))
		return NULL;
	SystemClick(&theEvent,
	            theWindow);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Evt_SystemTask(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	SystemTask();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Evt_SystemEvent(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	EventRecord theEvent;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetEventRecord, &theEvent))
		return NULL;
	_rv = SystemEvent(&theEvent);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyMethodDef Evt_methods[] = {
	{"GetCaretTime", (PyCFunction)Evt_GetCaretTime, 1,
	 "() -> (UInt32 _rv)"},
	{"SetEventMask", (PyCFunction)Evt_SetEventMask, 1,
	 "(EventMask value) -> None"},
	{"GetDblTime", (PyCFunction)Evt_GetDblTime, 1,
	 "() -> (UInt32 _rv)"},
	{"GetNextEvent", (PyCFunction)Evt_GetNextEvent, 1,
	 "(EventMask eventMask) -> (Boolean _rv, EventRecord theEvent)"},
	{"WaitNextEvent", (PyCFunction)Evt_WaitNextEvent, 1,
	 "(EventMask eventMask, UInt32 sleep) -> (Boolean _rv, EventRecord theEvent)"},
	{"EventAvail", (PyCFunction)Evt_EventAvail, 1,
	 "(EventMask eventMask) -> (Boolean _rv, EventRecord theEvent)"},
	{"GetMouse", (PyCFunction)Evt_GetMouse, 1,
	 "() -> (Point mouseLoc)"},
	{"Button", (PyCFunction)Evt_Button, 1,
	 "() -> (Boolean _rv)"},
	{"StillDown", (PyCFunction)Evt_StillDown, 1,
	 "() -> (Boolean _rv)"},
	{"WaitMouseUp", (PyCFunction)Evt_WaitMouseUp, 1,
	 "() -> (Boolean _rv)"},
	{"GetKeys", (PyCFunction)Evt_GetKeys, 1,
	 "() -> (KeyMap theKeys)"},
	{"TickCount", (PyCFunction)Evt_TickCount, 1,
	 "() -> (UInt32 _rv)"},
	{"PostEvent", (PyCFunction)Evt_PostEvent, 1,
	 "(EventKind eventNum, UInt32 eventMsg) -> None"},
	{"OSEventAvail", (PyCFunction)Evt_OSEventAvail, 1,
	 "(EventMask mask) -> (Boolean _rv, EventRecord theEvent)"},
	{"GetOSEvent", (PyCFunction)Evt_GetOSEvent, 1,
	 "(EventMask mask) -> (Boolean _rv, EventRecord theEvent)"},
	{"FlushEvents", (PyCFunction)Evt_FlushEvents, 1,
	 "(EventMask whichMask, EventMask stopMask) -> None"},
	{"SystemClick", (PyCFunction)Evt_SystemClick, 1,
	 "(EventRecord theEvent, WindowPtr theWindow) -> None"},
	{"SystemTask", (PyCFunction)Evt_SystemTask, 1,
	 "() -> None"},
	{"SystemEvent", (PyCFunction)Evt_SystemEvent, 1,
	 "(EventRecord theEvent) -> (Boolean _rv)"},
	{NULL, NULL, 0}
};




void initEvt()
{
	PyObject *m;
	PyObject *d;




	m = Py_InitModule("Evt", Evt_methods);
	d = PyModule_GetDict(m);
	Evt_Error = PyMac_GetOSErrException();
	if (Evt_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Evt_Error) != 0)
		Py_FatalError("can't initialize Evt.Error");
}

/* ========================= End module Evt ========================= */

