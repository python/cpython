
/* =========================== Module Dlg =========================== */

#include "Python.h"



#define SystemSevenOrLater 1

#include "macglue.h"
#include <Memory.h>
#include <Dialogs.h>
#include <Menus.h>
#include <Controls.h>

extern PyObject *ResObj_New(Handle);
extern PyObject *ResObj_OptNew(Handle);
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

#include <Dialogs.h>

#ifndef HAVE_UNIVERSAL_HEADERS
#define NewModalFilterProc(x) (x)
#endif

#define resNotFound -192 /* Can't include <Errors.h> because of Python's "errors.h" */

/* XXX Shouldn't this be a stack? */
static PyObject *Dlg_FilterProc_callback = NULL;

static PyObject *DlgObj_New(DialogPtr); /* Forward */

static pascal Boolean Dlg_UnivFilterProc(DialogPtr dialog,
                                         EventRecord *event,
                                         short *itemHit)
{
	Boolean rv;
	PyObject *args, *res;
	PyObject *callback = Dlg_FilterProc_callback;
	if (callback == NULL)
		return 0; /* Default behavior */
	Dlg_FilterProc_callback = NULL; /* We'll restore it when call successful */
	args = Py_BuildValue("O&O&", WinObj_WhichWindow, dialog, PyMac_BuildEventRecord, event);
	if (args == NULL)
		res = NULL;
	else {
		res = PyEval_CallObject(callback, args);
		Py_DECREF(args);
	}
	if (res == NULL) {
		fprintf(stderr, "Exception in Dialog Filter\n");
		PyErr_Print();
		*itemHit = -1; /* Fake return item */
		return 1; /* We handled it */
	}
	else {
		Dlg_FilterProc_callback = callback;
		if (PyInt_Check(res)) {
			*itemHit = PyInt_AsLong(res);
			rv = 1;
		}
		else
			rv = PyObject_IsTrue(res);
	}
	Py_DECREF(res);
	return rv;
}

static ModalFilterProcPtr
Dlg_PassFilterProc(PyObject *callback)
{
	PyObject *tmp = Dlg_FilterProc_callback;
	Dlg_FilterProc_callback = NULL;
	if (callback == Py_None) {
		Py_XDECREF(tmp);
		return NULL;
	}
	Py_INCREF(callback);
	Dlg_FilterProc_callback = callback;
	Py_XDECREF(tmp);
	return &Dlg_UnivFilterProc;
}

extern PyMethodChain WinObj_chain;

static PyObject *Dlg_Error;

/* ----------------------- Object type Dialog ----------------------- */

PyTypeObject Dialog_Type;

#define DlgObj_Check(x) ((x)->ob_type == &Dialog_Type)

typedef struct DialogObject {
	PyObject_HEAD
	DialogPtr ob_itself;
} DialogObject;

PyObject *DlgObj_New(itself)
	DialogPtr itself;
{
	DialogObject *it;
	if (itself == NULL) return Py_None;
	it = PyObject_NEW(DialogObject, &Dialog_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	SetWRefCon(itself, (long)it);
	return (PyObject *)it;
}
DlgObj_Convert(v, p_itself)
	PyObject *v;
	DialogPtr *p_itself;
{
	if (v == Py_None) { *p_itself = NULL; return 1; }
	if (PyInt_Check(v)) { *p_itself = (DialogPtr)PyInt_AsLong(v);
	                      return 1; }
	if (!DlgObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "Dialog required");
		return 0;
	}
	*p_itself = ((DialogObject *)v)->ob_itself;
	return 1;
}

static void DlgObj_dealloc(self)
	DialogObject *self;
{
	DisposeDialog(self->ob_itself);
	PyMem_DEL(self);
}

static PyObject *DlgObj_DrawDialog(_self, _args)
	DialogObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	DrawDialog(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_UpdateDialog(_self, _args)
	DialogObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	UpdateDialog(_self->ob_itself,
	             _self->ob_itself->visRgn);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_GetDialogItem(_self, _args)
	DialogObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short itemNo;
	short itemType;
	Handle item;
	Rect box;
	if (!PyArg_ParseTuple(_args, "h",
	                      &itemNo))
		return NULL;
	GetDialogItem(_self->ob_itself,
	              itemNo,
	              &itemType,
	              &item,
	              &box);
	_res = Py_BuildValue("hO&O&",
	                     itemType,
	                     ResObj_OptNew, item,
	                     PyMac_BuildRect, &box);
	return _res;
}

static PyObject *DlgObj_SetDialogItem(_self, _args)
	DialogObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short itemNo;
	short itemType;
	Handle item;
	Rect box;
	if (!PyArg_ParseTuple(_args, "hhO&O&",
	                      &itemNo,
	                      &itemType,
	                      ResObj_Convert, &item,
	                      PyMac_GetRect, &box))
		return NULL;
	SetDialogItem(_self->ob_itself,
	              itemNo,
	              itemType,
	              item,
	              &box);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_HideDialogItem(_self, _args)
	DialogObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short itemNo;
	if (!PyArg_ParseTuple(_args, "h",
	                      &itemNo))
		return NULL;
	HideDialogItem(_self->ob_itself,
	               itemNo);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_ShowDialogItem(_self, _args)
	DialogObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short itemNo;
	if (!PyArg_ParseTuple(_args, "h",
	                      &itemNo))
		return NULL;
	ShowDialogItem(_self->ob_itself,
	               itemNo);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_SelectDialogItemText(_self, _args)
	DialogObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short itemNo;
	short strtSel;
	short endSel;
	if (!PyArg_ParseTuple(_args, "hhh",
	                      &itemNo,
	                      &strtSel,
	                      &endSel))
		return NULL;
	SelectDialogItemText(_self->ob_itself,
	                     itemNo,
	                     strtSel,
	                     endSel);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_FindDialogItem(_self, _args)
	DialogObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	Point thePt;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &thePt))
		return NULL;
	_rv = FindDialogItem(_self->ob_itself,
	                     thePt);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *DlgObj_DialogCut(_self, _args)
	DialogObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	DialogCut(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_DialogPaste(_self, _args)
	DialogObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	DialogPaste(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_DialogCopy(_self, _args)
	DialogObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	DialogCopy(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_DialogDelete(_self, _args)
	DialogObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	DialogDelete(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_AppendDITL(_self, _args)
	DialogObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Handle theHandle;
	DITLMethod method;
	if (!PyArg_ParseTuple(_args, "O&h",
	                      ResObj_Convert, &theHandle,
	                      &method))
		return NULL;
	AppendDITL(_self->ob_itself,
	           theHandle,
	           method);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_CountDITL(_self, _args)
	DialogObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = CountDITL(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *DlgObj_ShortenDITL(_self, _args)
	DialogObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short numberItems;
	if (!PyArg_ParseTuple(_args, "h",
	                      &numberItems))
		return NULL;
	ShortenDITL(_self->ob_itself,
	            numberItems);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_StdFilterProc(_self, _args)
	DialogObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	EventRecord event;
	short itemHit;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = StdFilterProc(_self->ob_itself,
	                    &event,
	                    &itemHit);
	_res = Py_BuildValue("bO&h",
	                     _rv,
	                     PyMac_BuildEventRecord, &event,
	                     itemHit);
	return _res;
}

static PyObject *DlgObj_SetDialogDefaultItem(_self, _args)
	DialogObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	short newItem;
	if (!PyArg_ParseTuple(_args, "h",
	                      &newItem))
		return NULL;
	_err = SetDialogDefaultItem(_self->ob_itself,
	                            newItem);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_SetDialogCancelItem(_self, _args)
	DialogObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	short newItem;
	if (!PyArg_ParseTuple(_args, "h",
	                      &newItem))
		return NULL;
	_err = SetDialogCancelItem(_self->ob_itself,
	                           newItem);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_SetDialogTracksCursor(_self, _args)
	DialogObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	Boolean tracks;
	if (!PyArg_ParseTuple(_args, "b",
	                      &tracks))
		return NULL;
	_err = SetDialogTracksCursor(_self->ob_itself,
	                             tracks);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef DlgObj_methods[] = {
	{"DrawDialog", (PyCFunction)DlgObj_DrawDialog, 1,
	 "() -> None"},
	{"UpdateDialog", (PyCFunction)DlgObj_UpdateDialog, 1,
	 "() -> None"},
	{"GetDialogItem", (PyCFunction)DlgObj_GetDialogItem, 1,
	 "(short itemNo) -> (short itemType, Handle item, Rect box)"},
	{"SetDialogItem", (PyCFunction)DlgObj_SetDialogItem, 1,
	 "(short itemNo, short itemType, Handle item, Rect box) -> None"},
	{"HideDialogItem", (PyCFunction)DlgObj_HideDialogItem, 1,
	 "(short itemNo) -> None"},
	{"ShowDialogItem", (PyCFunction)DlgObj_ShowDialogItem, 1,
	 "(short itemNo) -> None"},
	{"SelectDialogItemText", (PyCFunction)DlgObj_SelectDialogItemText, 1,
	 "(short itemNo, short strtSel, short endSel) -> None"},
	{"FindDialogItem", (PyCFunction)DlgObj_FindDialogItem, 1,
	 "(Point thePt) -> (short _rv)"},
	{"DialogCut", (PyCFunction)DlgObj_DialogCut, 1,
	 "() -> None"},
	{"DialogPaste", (PyCFunction)DlgObj_DialogPaste, 1,
	 "() -> None"},
	{"DialogCopy", (PyCFunction)DlgObj_DialogCopy, 1,
	 "() -> None"},
	{"DialogDelete", (PyCFunction)DlgObj_DialogDelete, 1,
	 "() -> None"},
	{"AppendDITL", (PyCFunction)DlgObj_AppendDITL, 1,
	 "(Handle theHandle, DITLMethod method) -> None"},
	{"CountDITL", (PyCFunction)DlgObj_CountDITL, 1,
	 "() -> (short _rv)"},
	{"ShortenDITL", (PyCFunction)DlgObj_ShortenDITL, 1,
	 "(short numberItems) -> None"},
	{"StdFilterProc", (PyCFunction)DlgObj_StdFilterProc, 1,
	 "() -> (Boolean _rv, EventRecord event, short itemHit)"},
	{"SetDialogDefaultItem", (PyCFunction)DlgObj_SetDialogDefaultItem, 1,
	 "(short newItem) -> None"},
	{"SetDialogCancelItem", (PyCFunction)DlgObj_SetDialogCancelItem, 1,
	 "(short newItem) -> None"},
	{"SetDialogTracksCursor", (PyCFunction)DlgObj_SetDialogTracksCursor, 1,
	 "(Boolean tracks) -> None"},
	{NULL, NULL, 0}
};

PyMethodChain DlgObj_chain = { DlgObj_methods, &WinObj_chain };

static PyObject *DlgObj_getattr(self, name)
	DialogObject *self;
	char *name;
{
	return Py_FindMethodInChain(&DlgObj_chain, (PyObject *)self, name);
}

#define DlgObj_setattr NULL

PyTypeObject Dialog_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"Dialog", /*tp_name*/
	sizeof(DialogObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) DlgObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) DlgObj_getattr, /*tp_getattr*/
	(setattrfunc) DlgObj_setattr, /*tp_setattr*/
};

/* --------------------- End object type Dialog --------------------- */


static PyObject *Dlg_NewDialog(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	DialogPtr _rv;
	Rect boundsRect;
	Str255 title;
	Boolean visible;
	short procID;
	WindowPtr behind;
	Boolean goAwayFlag;
	long refCon;
	Handle itmLstHndl;
	if (!PyArg_ParseTuple(_args, "O&O&bhO&blO&",
	                      PyMac_GetRect, &boundsRect,
	                      PyMac_GetStr255, title,
	                      &visible,
	                      &procID,
	                      WinObj_Convert, &behind,
	                      &goAwayFlag,
	                      &refCon,
	                      ResObj_Convert, &itmLstHndl))
		return NULL;
	_rv = NewDialog((void *)0,
	                &boundsRect,
	                title,
	                visible,
	                procID,
	                behind,
	                goAwayFlag,
	                refCon,
	                itmLstHndl);
	_res = Py_BuildValue("O&",
	                     DlgObj_New, _rv);
	return _res;
}

static PyObject *Dlg_GetNewDialog(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	DialogPtr _rv;
	short dialogID;
	WindowPtr behind;
	if (!PyArg_ParseTuple(_args, "hO&",
	                      &dialogID,
	                      WinObj_Convert, &behind))
		return NULL;
	_rv = GetNewDialog(dialogID,
	                   (void *)0,
	                   behind);
	_res = Py_BuildValue("O&",
	                     DlgObj_New, _rv);
	return _res;
}

static PyObject *Dlg_ParamText(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Str255 param0;
	Str255 param1;
	Str255 param2;
	Str255 param3;
	if (!PyArg_ParseTuple(_args, "O&O&O&O&",
	                      PyMac_GetStr255, param0,
	                      PyMac_GetStr255, param1,
	                      PyMac_GetStr255, param2,
	                      PyMac_GetStr255, param3))
		return NULL;
	ParamText(param0,
	          param1,
	          param2,
	          param3);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Dlg_ModalDialog(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	PyObject* modalFilter;
	short itemHit;
	if (!PyArg_ParseTuple(_args, "O",
	                      &modalFilter))
		return NULL;
	ModalDialog(NewModalFilterProc(Dlg_PassFilterProc(modalFilter)),
	            &itemHit);
	_res = Py_BuildValue("h",
	                     itemHit);
	return _res;
}

static PyObject *Dlg_IsDialogEvent(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	EventRecord theEvent;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetEventRecord, &theEvent))
		return NULL;
	_rv = IsDialogEvent(&theEvent);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Dlg_DialogSelect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	EventRecord theEvent;
	DialogPtr theDialog;
	short itemHit;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetEventRecord, &theEvent))
		return NULL;
	_rv = DialogSelect(&theEvent,
	                   &theDialog,
	                   &itemHit);
	_res = Py_BuildValue("bO&h",
	                     _rv,
	                     WinObj_WhichWindow, theDialog,
	                     itemHit);
	return _res;
}

static PyObject *Dlg_Alert(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	short alertID;
	PyObject* modalFilter;
	if (!PyArg_ParseTuple(_args, "hO",
	                      &alertID,
	                      &modalFilter))
		return NULL;
	_rv = Alert(alertID,
	            NewModalFilterProc(Dlg_PassFilterProc(modalFilter)));
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Dlg_StopAlert(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	short alertID;
	PyObject* modalFilter;
	if (!PyArg_ParseTuple(_args, "hO",
	                      &alertID,
	                      &modalFilter))
		return NULL;
	_rv = StopAlert(alertID,
	                NewModalFilterProc(Dlg_PassFilterProc(modalFilter)));
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Dlg_NoteAlert(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	short alertID;
	PyObject* modalFilter;
	if (!PyArg_ParseTuple(_args, "hO",
	                      &alertID,
	                      &modalFilter))
		return NULL;
	_rv = NoteAlert(alertID,
	                NewModalFilterProc(Dlg_PassFilterProc(modalFilter)));
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Dlg_CautionAlert(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	short alertID;
	PyObject* modalFilter;
	if (!PyArg_ParseTuple(_args, "hO",
	                      &alertID,
	                      &modalFilter))
		return NULL;
	_rv = CautionAlert(alertID,
	                   NewModalFilterProc(Dlg_PassFilterProc(modalFilter)));
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Dlg_GetDialogItemText(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Handle item;
	Str255 text;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &item))
		return NULL;
	GetDialogItemText(item,
	                  text);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildStr255, text);
	return _res;
}

static PyObject *Dlg_SetDialogItemText(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Handle item;
	Str255 text;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &item,
	                      PyMac_GetStr255, text))
		return NULL;
	SetDialogItemText(item,
	                  text);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Dlg_NewColorDialog(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	DialogPtr _rv;
	Rect boundsRect;
	Str255 title;
	Boolean visible;
	short procID;
	WindowPtr behind;
	Boolean goAwayFlag;
	long refCon;
	Handle items;
	if (!PyArg_ParseTuple(_args, "O&O&bhO&blO&",
	                      PyMac_GetRect, &boundsRect,
	                      PyMac_GetStr255, title,
	                      &visible,
	                      &procID,
	                      WinObj_Convert, &behind,
	                      &goAwayFlag,
	                      &refCon,
	                      ResObj_Convert, &items))
		return NULL;
	_rv = NewColorDialog((void *)0,
	                     &boundsRect,
	                     title,
	                     visible,
	                     procID,
	                     behind,
	                     goAwayFlag,
	                     refCon,
	                     items);
	_res = Py_BuildValue("O&",
	                     DlgObj_New, _rv);
	return _res;
}

static PyObject *Dlg_GetAlertStage(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetAlertStage();
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Dlg_ResetAlertStage(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	ResetAlertStage();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Dlg_SetDialogFont(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short value;
	if (!PyArg_ParseTuple(_args, "h",
	                      &value))
		return NULL;
	SetDialogFont(value);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef Dlg_methods[] = {
	{"NewDialog", (PyCFunction)Dlg_NewDialog, 1,
	 "(Rect boundsRect, Str255 title, Boolean visible, short procID, WindowPtr behind, Boolean goAwayFlag, long refCon, Handle itmLstHndl) -> (DialogPtr _rv)"},
	{"GetNewDialog", (PyCFunction)Dlg_GetNewDialog, 1,
	 "(short dialogID, WindowPtr behind) -> (DialogPtr _rv)"},
	{"ParamText", (PyCFunction)Dlg_ParamText, 1,
	 "(Str255 param0, Str255 param1, Str255 param2, Str255 param3) -> None"},
	{"ModalDialog", (PyCFunction)Dlg_ModalDialog, 1,
	 "(PyObject* modalFilter) -> (short itemHit)"},
	{"IsDialogEvent", (PyCFunction)Dlg_IsDialogEvent, 1,
	 "(EventRecord theEvent) -> (Boolean _rv)"},
	{"DialogSelect", (PyCFunction)Dlg_DialogSelect, 1,
	 "(EventRecord theEvent) -> (Boolean _rv, DialogPtr theDialog, short itemHit)"},
	{"Alert", (PyCFunction)Dlg_Alert, 1,
	 "(short alertID, PyObject* modalFilter) -> (short _rv)"},
	{"StopAlert", (PyCFunction)Dlg_StopAlert, 1,
	 "(short alertID, PyObject* modalFilter) -> (short _rv)"},
	{"NoteAlert", (PyCFunction)Dlg_NoteAlert, 1,
	 "(short alertID, PyObject* modalFilter) -> (short _rv)"},
	{"CautionAlert", (PyCFunction)Dlg_CautionAlert, 1,
	 "(short alertID, PyObject* modalFilter) -> (short _rv)"},
	{"GetDialogItemText", (PyCFunction)Dlg_GetDialogItemText, 1,
	 "(Handle item) -> (Str255 text)"},
	{"SetDialogItemText", (PyCFunction)Dlg_SetDialogItemText, 1,
	 "(Handle item, Str255 text) -> None"},
	{"NewColorDialog", (PyCFunction)Dlg_NewColorDialog, 1,
	 "(Rect boundsRect, Str255 title, Boolean visible, short procID, WindowPtr behind, Boolean goAwayFlag, long refCon, Handle items) -> (DialogPtr _rv)"},
	{"GetAlertStage", (PyCFunction)Dlg_GetAlertStage, 1,
	 "() -> (short _rv)"},
	{"ResetAlertStage", (PyCFunction)Dlg_ResetAlertStage, 1,
	 "() -> None"},
	{"SetDialogFont", (PyCFunction)Dlg_SetDialogFont, 1,
	 "(short value) -> None"},
	{NULL, NULL, 0}
};




void initDlg()
{
	PyObject *m;
	PyObject *d;




	m = Py_InitModule("Dlg", Dlg_methods);
	d = PyModule_GetDict(m);
	Dlg_Error = PyMac_GetOSErrException();
	if (Dlg_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Dlg_Error) != 0)
		Py_FatalError("can't initialize Dlg.Error");
}

/* ========================= End module Dlg ========================= */

