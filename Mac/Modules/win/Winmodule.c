
/* =========================== Module Win =========================== */

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

#include <Windows.h>

#define resNotFound -192 /* Can't include <Errors.h> because of Python's "errors.h" */

#ifdef __MWERKS__
#define WindowPeek WindowPtr
#endif

extern PyObject *WinObj_WhichWindow(WindowPtr w); /* Forward */

static PyObject *Win_Error;

/* ----------------------- Object type Window ----------------------- */

PyTypeObject Window_Type;

#define WinObj_Check(x) ((x)->ob_type == &Window_Type)

typedef struct WindowObject {
	PyObject_HEAD
	WindowPtr ob_itself;
} WindowObject;

PyObject *WinObj_New(itself)
	const WindowPtr itself;
{
	WindowObject *it;
	if (itself == NULL) return PyMac_Error(resNotFound);
	it = PyObject_NEW(WindowObject, &Window_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	SetWRefCon(itself, (long)it);
	return (PyObject *)it;
}
WinObj_Convert(v, p_itself)
	PyObject *v;
	WindowPtr *p_itself;
{
	if (DlgObj_Check(v)) {
		*p_itself = ((WindowObject *)v)->ob_itself;
		return 1;
	}

	if (v == Py_None) { *p_itself = NULL; return 1; }
	if (PyInt_Check(v)) { *p_itself = (WindowPtr)PyInt_AsLong(v); return 1; }

	if (!WinObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "Window required");
		return 0;
	}
	*p_itself = ((WindowObject *)v)->ob_itself;
	return 1;
}

static void WinObj_dealloc(self)
	WindowObject *self;
{
	DisposeWindow(self->ob_itself);
	PyMem_DEL(self);
}

static PyObject *WinObj_GetWTitle(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Str255 title;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetWTitle(_self->ob_itself,
	          title);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildStr255, title);
	return _res;
}

static PyObject *WinObj_SelectWindow(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	SelectWindow(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_HideWindow(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	HideWindow(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_ShowWindow(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	ShowWindow(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_ShowHide(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean showFlag;
	if (!PyArg_ParseTuple(_args, "b",
	                      &showFlag))
		return NULL;
	ShowHide(_self->ob_itself,
	         showFlag);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_HiliteWindow(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean fHilite;
	if (!PyArg_ParseTuple(_args, "b",
	                      &fHilite))
		return NULL;
	HiliteWindow(_self->ob_itself,
	             fHilite);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_BringToFront(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	BringToFront(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_SendBehind(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	WindowPtr behindWindow;
	if (!PyArg_ParseTuple(_args, "O&",
	                      WinObj_Convert, &behindWindow))
		return NULL;
	SendBehind(_self->ob_itself,
	           behindWindow);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_DrawGrowIcon(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	DrawGrowIcon(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_MoveWindow(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short hGlobal;
	short vGlobal;
	Boolean front;
	if (!PyArg_ParseTuple(_args, "hhb",
	                      &hGlobal,
	                      &vGlobal,
	                      &front))
		return NULL;
	MoveWindow(_self->ob_itself,
	           hGlobal,
	           vGlobal,
	           front);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_SizeWindow(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short w;
	short h;
	Boolean fUpdate;
	if (!PyArg_ParseTuple(_args, "hhb",
	                      &w,
	                      &h,
	                      &fUpdate))
		return NULL;
	SizeWindow(_self->ob_itself,
	           w,
	           h,
	           fUpdate);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_ZoomWindow(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short partCode;
	Boolean front;
	if (!PyArg_ParseTuple(_args, "hb",
	                      &partCode,
	                      &front))
		return NULL;
	ZoomWindow(_self->ob_itself,
	           partCode,
	           front);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_BeginUpdate(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	BeginUpdate(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_EndUpdate(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	EndUpdate(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_SetWRefCon(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long data;
	if (!PyArg_ParseTuple(_args, "l",
	                      &data))
		return NULL;
	SetWRefCon(_self->ob_itself,
	           data);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_GetWRefCon(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetWRefCon(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *WinObj_ClipAbove(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	ClipAbove((WindowPeek)(_self->ob_itself));
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_SaveOld(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	SaveOld((WindowPeek)(_self->ob_itself));
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_DrawNew(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean update;
	if (!PyArg_ParseTuple(_args, "b",
	                      &update))
		return NULL;
	DrawNew((WindowPeek)(_self->ob_itself),
	        update);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_CalcVis(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	CalcVis((WindowPeek)(_self->ob_itself));
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_GrowWindow(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long _rv;
	Point startPt;
	Rect bBox;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetPoint, &startPt,
	                      PyMac_GetRect, &bBox))
		return NULL;
	_rv = GrowWindow(_self->ob_itself,
	                 startPt,
	                 &bBox);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *WinObj_TrackBox(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	Point thePt;
	short partCode;
	if (!PyArg_ParseTuple(_args, "O&h",
	                      PyMac_GetPoint, &thePt,
	                      &partCode))
		return NULL;
	_rv = TrackBox(_self->ob_itself,
	               thePt,
	               partCode);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *WinObj_GetWVariant(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetWVariant(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *WinObj_SetWTitle(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Str255 title;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetStr255, title))
		return NULL;
	SetWTitle(_self->ob_itself,
	          title);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_TrackGoAway(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	Point thePt;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &thePt))
		return NULL;
	_rv = TrackGoAway(_self->ob_itself,
	                  thePt);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *WinObj_DragWindow(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Point startPt;
	Rect boundsRect;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetPoint, &startPt,
	                      PyMac_GetRect, &boundsRect))
		return NULL;
	DragWindow(_self->ob_itself,
	           startPt,
	           &boundsRect);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef WinObj_methods[] = {
	{"GetWTitle", (PyCFunction)WinObj_GetWTitle, 1,
	 "() -> (Str255 title)"},
	{"SelectWindow", (PyCFunction)WinObj_SelectWindow, 1,
	 "() -> None"},
	{"HideWindow", (PyCFunction)WinObj_HideWindow, 1,
	 "() -> None"},
	{"ShowWindow", (PyCFunction)WinObj_ShowWindow, 1,
	 "() -> None"},
	{"ShowHide", (PyCFunction)WinObj_ShowHide, 1,
	 "(Boolean showFlag) -> None"},
	{"HiliteWindow", (PyCFunction)WinObj_HiliteWindow, 1,
	 "(Boolean fHilite) -> None"},
	{"BringToFront", (PyCFunction)WinObj_BringToFront, 1,
	 "() -> None"},
	{"SendBehind", (PyCFunction)WinObj_SendBehind, 1,
	 "(WindowPtr behindWindow) -> None"},
	{"DrawGrowIcon", (PyCFunction)WinObj_DrawGrowIcon, 1,
	 "() -> None"},
	{"MoveWindow", (PyCFunction)WinObj_MoveWindow, 1,
	 "(short hGlobal, short vGlobal, Boolean front) -> None"},
	{"SizeWindow", (PyCFunction)WinObj_SizeWindow, 1,
	 "(short w, short h, Boolean fUpdate) -> None"},
	{"ZoomWindow", (PyCFunction)WinObj_ZoomWindow, 1,
	 "(short partCode, Boolean front) -> None"},
	{"BeginUpdate", (PyCFunction)WinObj_BeginUpdate, 1,
	 "() -> None"},
	{"EndUpdate", (PyCFunction)WinObj_EndUpdate, 1,
	 "() -> None"},
	{"SetWRefCon", (PyCFunction)WinObj_SetWRefCon, 1,
	 "(long data) -> None"},
	{"GetWRefCon", (PyCFunction)WinObj_GetWRefCon, 1,
	 "() -> (long _rv)"},
	{"ClipAbove", (PyCFunction)WinObj_ClipAbove, 1,
	 "() -> None"},
	{"SaveOld", (PyCFunction)WinObj_SaveOld, 1,
	 "() -> None"},
	{"DrawNew", (PyCFunction)WinObj_DrawNew, 1,
	 "(Boolean update) -> None"},
	{"CalcVis", (PyCFunction)WinObj_CalcVis, 1,
	 "() -> None"},
	{"GrowWindow", (PyCFunction)WinObj_GrowWindow, 1,
	 "(Point startPt, Rect bBox) -> (long _rv)"},
	{"TrackBox", (PyCFunction)WinObj_TrackBox, 1,
	 "(Point thePt, short partCode) -> (Boolean _rv)"},
	{"GetWVariant", (PyCFunction)WinObj_GetWVariant, 1,
	 "() -> (short _rv)"},
	{"SetWTitle", (PyCFunction)WinObj_SetWTitle, 1,
	 "(Str255 title) -> None"},
	{"TrackGoAway", (PyCFunction)WinObj_TrackGoAway, 1,
	 "(Point thePt) -> (Boolean _rv)"},
	{"DragWindow", (PyCFunction)WinObj_DragWindow, 1,
	 "(Point startPt, Rect boundsRect) -> None"},
	{NULL, NULL, 0}
};

PyMethodChain WinObj_chain = { WinObj_methods, NULL };

static PyObject *WinObj_getattr(self, name)
	WindowObject *self;
	char *name;
{
	return Py_FindMethodInChain(&WinObj_chain, (PyObject *)self, name);
}

#define WinObj_setattr NULL

PyTypeObject Window_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"Window", /*tp_name*/
	sizeof(WindowObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) WinObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) WinObj_getattr, /*tp_getattr*/
	(setattrfunc) WinObj_setattr, /*tp_setattr*/
};

/* --------------------- End object type Window --------------------- */


static PyObject *Win_InitWindows(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	InitWindows();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Win_NewWindow(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	WindowPtr _rv;
	Rect boundsRect;
	Str255 title;
	Boolean visible;
	short theProc;
	WindowPtr behind;
	Boolean goAwayFlag;
	long refCon;
	if (!PyArg_ParseTuple(_args, "O&O&bhO&bl",
	                      PyMac_GetRect, &boundsRect,
	                      PyMac_GetStr255, title,
	                      &visible,
	                      &theProc,
	                      WinObj_Convert, &behind,
	                      &goAwayFlag,
	                      &refCon))
		return NULL;
	_rv = NewWindow((void *)0,
	                &boundsRect,
	                title,
	                visible,
	                theProc,
	                behind,
	                goAwayFlag,
	                refCon);
	_res = Py_BuildValue("O&",
	                     WinObj_New, _rv);
	return _res;
}

static PyObject *Win_GetNewWindow(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	WindowPtr _rv;
	short windowID;
	WindowPtr behind;
	if (!PyArg_ParseTuple(_args, "hO&",
	                      &windowID,
	                      WinObj_Convert, &behind))
		return NULL;
	_rv = GetNewWindow(windowID,
	                   (void *)0,
	                   behind);
	_res = Py_BuildValue("O&",
	                     WinObj_New, _rv);
	return _res;
}

static PyObject *Win_FrontWindow(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	WindowPtr _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = FrontWindow();
	_res = Py_BuildValue("O&",
	                     WinObj_New, _rv);
	return _res;
}

static PyObject *Win_InvalRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect badRect;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &badRect))
		return NULL;
	InvalRect(&badRect);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Win_ValidRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect goodRect;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &goodRect))
		return NULL;
	ValidRect(&goodRect);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Win_CheckUpdate(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	EventRecord theEvent;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = CheckUpdate(&theEvent);
	_res = Py_BuildValue("bO&",
	                     _rv,
	                     PyMac_BuildEventRecord, &theEvent);
	return _res;
}

static PyObject *Win_FindWindow(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	Point thePoint;
	WindowPtr theWindow;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &thePoint))
		return NULL;
	_rv = FindWindow(thePoint,
	                 &theWindow);
	_res = Py_BuildValue("hO&",
	                     _rv,
	                     WinObj_WhichWindow, theWindow);
	return _res;
}

static PyObject *Win_PinRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long _rv;
	Rect theRect;
	Point thePt;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetRect, &theRect,
	                      PyMac_GetPoint, &thePt))
		return NULL;
	_rv = PinRect(&theRect,
	              thePt);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Win_NewCWindow(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	WindowPtr _rv;
	Rect boundsRect;
	Str255 title;
	Boolean visible;
	short procID;
	WindowPtr behind;
	Boolean goAwayFlag;
	long refCon;
	if (!PyArg_ParseTuple(_args, "O&O&bhO&bl",
	                      PyMac_GetRect, &boundsRect,
	                      PyMac_GetStr255, title,
	                      &visible,
	                      &procID,
	                      WinObj_Convert, &behind,
	                      &goAwayFlag,
	                      &refCon))
		return NULL;
	_rv = NewCWindow((void *)0,
	                 &boundsRect,
	                 title,
	                 visible,
	                 procID,
	                 behind,
	                 goAwayFlag,
	                 refCon);
	_res = Py_BuildValue("O&",
	                     WinObj_New, _rv);
	return _res;
}

static PyObject *Win_GetNewCWindow(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	WindowPtr _rv;
	short windowID;
	WindowPtr behind;
	if (!PyArg_ParseTuple(_args, "hO&",
	                      &windowID,
	                      WinObj_Convert, &behind))
		return NULL;
	_rv = GetNewCWindow(windowID,
	                    (void *)0,
	                    behind);
	_res = Py_BuildValue("O&",
	                     WinObj_New, _rv);
	return _res;
}

static PyMethodDef Win_methods[] = {
	{"InitWindows", (PyCFunction)Win_InitWindows, 1,
	 "() -> None"},
	{"NewWindow", (PyCFunction)Win_NewWindow, 1,
	 "(Rect boundsRect, Str255 title, Boolean visible, short theProc, WindowPtr behind, Boolean goAwayFlag, long refCon) -> (WindowPtr _rv)"},
	{"GetNewWindow", (PyCFunction)Win_GetNewWindow, 1,
	 "(short windowID, WindowPtr behind) -> (WindowPtr _rv)"},
	{"FrontWindow", (PyCFunction)Win_FrontWindow, 1,
	 "() -> (WindowPtr _rv)"},
	{"InvalRect", (PyCFunction)Win_InvalRect, 1,
	 "(Rect badRect) -> None"},
	{"ValidRect", (PyCFunction)Win_ValidRect, 1,
	 "(Rect goodRect) -> None"},
	{"CheckUpdate", (PyCFunction)Win_CheckUpdate, 1,
	 "() -> (Boolean _rv, EventRecord theEvent)"},
	{"FindWindow", (PyCFunction)Win_FindWindow, 1,
	 "(Point thePoint) -> (short _rv, WindowPtr theWindow)"},
	{"PinRect", (PyCFunction)Win_PinRect, 1,
	 "(Rect theRect, Point thePt) -> (long _rv)"},
	{"NewCWindow", (PyCFunction)Win_NewCWindow, 1,
	 "(Rect boundsRect, Str255 title, Boolean visible, short procID, WindowPtr behind, Boolean goAwayFlag, long refCon) -> (WindowPtr _rv)"},
	{"GetNewCWindow", (PyCFunction)Win_GetNewCWindow, 1,
	 "(short windowID, WindowPtr behind) -> (WindowPtr _rv)"},
	{NULL, NULL, 0}
};



/* Return the object corresponding to the window, or NULL */

PyObject *
WinObj_WhichWindow(w)
	WindowPtr w;
{
	PyObject *it;
	
	/* XXX What if we find a stdwin window or a window belonging
	       to some other package? */
	it = (PyObject *) GetWRefCon(w);
	if (it == NULL || ((WindowObject *)it)->ob_itself != w)
		it = Py_None;
	Py_INCREF(it);
	return it;
}


void initWin()
{
	PyObject *m;
	PyObject *d;




	m = Py_InitModule("Win", Win_methods);
	d = PyModule_GetDict(m);
	Win_Error = PyMac_GetOSErrException();
	if (Win_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Win_Error) != 0)
		Py_FatalError("can't initialize Win.Error");
}

/* ========================= End module Win ========================= */

