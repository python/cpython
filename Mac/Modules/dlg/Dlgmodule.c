
/* =========================== Module Dlg =========================== */

#include "Python.h"



#include "macglue.h"
#include "pymactoolbox.h"

#ifdef WITHOUT_FRAMEWORKS
#include <Dialogs.h>
#else
#include <Carbon/Carbon.h>
#endif

#ifdef USE_TOOLBOX_OBJECT_GLUE
extern PyObject *_DlgObj_New(DialogRef);
extern PyObject *_DlgObj_WhichDialog(DialogRef);
extern int _DlgObj_Convert(PyObject *, DialogRef *);

#define DlgObj_New _DlgObj_New
#define DlgObj_WhichDialog _DlgObj_WhichDialog
#define DlgObj_Convert _DlgObj_Convert
#endif

#if !ACCESSOR_CALLS_ARE_FUNCTIONS
#define GetDialogTextEditHandle(dlg) (((DialogPeek)(dlg))->textH)
#define SetPortDialogPort(dlg) SetPort(dlg)
#define GetDialogPort(dlg) ((CGrafPtr)(dlg))
#define GetDialogFromWindow(win) ((DialogRef)(win))
#endif

/* XXX Shouldn't this be a stack? */
static PyObject *Dlg_FilterProc_callback = NULL;

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
	args = Py_BuildValue("O&O&", DlgObj_WhichDialog, dialog, PyMac_BuildEventRecord, event);
	if (args == NULL)
		res = NULL;
	else {
		res = PyEval_CallObject(callback, args);
		Py_DECREF(args);
	}
	if (res == NULL) {
		PySys_WriteStderr("Exception in Dialog Filter\n");
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

static ModalFilterUPP
Dlg_PassFilterProc(PyObject *callback)
{
	PyObject *tmp = Dlg_FilterProc_callback;
	static ModalFilterUPP UnivFilterUpp = NULL;
	
	Dlg_FilterProc_callback = NULL;
	if (callback == Py_None) {
		Py_XDECREF(tmp);
		return NULL;
	}
	Py_INCREF(callback);
	Dlg_FilterProc_callback = callback;
	Py_XDECREF(tmp);
	if ( UnivFilterUpp == NULL )
		UnivFilterUpp = NewModalFilterUPP(&Dlg_UnivFilterProc);
	return UnivFilterUpp;
}

static PyObject *Dlg_UserItemProc_callback = NULL;

static pascal void Dlg_UnivUserItemProc(DialogPtr dialog,
                                         short item)
{
	PyObject *args, *res;

	if (Dlg_UserItemProc_callback == NULL)
		return; /* Default behavior */
	Dlg_FilterProc_callback = NULL; /* We'll restore it when call successful */
	args = Py_BuildValue("O&h", DlgObj_WhichDialog, dialog, item);
	if (args == NULL)
		res = NULL;
	else {
		res = PyEval_CallObject(Dlg_UserItemProc_callback, args);
		Py_DECREF(args);
	}
	if (res == NULL) {
		PySys_WriteStderr("Exception in Dialog UserItem proc\n");
		PyErr_Print();
	}
	Py_XDECREF(res);
	return;
}

#if 0
/*
** Treating DialogObjects as WindowObjects is (I think) illegal under Carbon.
** However, as they are still identical under MacOS9 Carbon this is a problem, even
** if we neatly call GetDialogWindow() at the right places: there's one refcon field
** and it points to the DialogObject, so WinObj_WhichWindow will smartly return the
** dialog object, and therefore we still don't have a WindowObject.
** I'll leave the chaining code in place for now, with this comment to warn the
** unsuspecting victims (i.e. me, probably, in a few weeks:-)
*/
extern PyMethodChain WinObj_chain;
#endif

static PyObject *Dlg_Error;

/* ----------------------- Object type Dialog ----------------------- */

PyTypeObject Dialog_Type;

#define DlgObj_Check(x) ((x)->ob_type == &Dialog_Type)

typedef struct DialogObject {
	PyObject_HEAD
	DialogPtr ob_itself;
} DialogObject;

PyObject *DlgObj_New(DialogPtr itself)
{
	DialogObject *it;
	if (itself == NULL) return Py_None;
	it = PyObject_NEW(DialogObject, &Dialog_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	SetWRefCon(GetDialogWindow(itself), (long)it);
	return (PyObject *)it;
}
DlgObj_Convert(PyObject *v, DialogPtr *p_itself)
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

static void DlgObj_dealloc(DialogObject *self)
{
	DisposeDialog(self->ob_itself);
	PyMem_DEL(self);
}

static PyObject *DlgObj_DrawDialog(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	DrawDialog(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_UpdateDialog(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	RgnHandle updateRgn;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &updateRgn))
		return NULL;
	UpdateDialog(_self->ob_itself,
	             updateRgn);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_HideDialogItem(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	DialogItemIndex itemNo;
	if (!PyArg_ParseTuple(_args, "h",
	                      &itemNo))
		return NULL;
	HideDialogItem(_self->ob_itself,
	               itemNo);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_ShowDialogItem(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	DialogItemIndex itemNo;
	if (!PyArg_ParseTuple(_args, "h",
	                      &itemNo))
		return NULL;
	ShowDialogItem(_self->ob_itself,
	               itemNo);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_FindDialogItem(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	DialogItemIndexZeroBased _rv;
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

static PyObject *DlgObj_DialogCut(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	DialogCut(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_DialogPaste(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	DialogPaste(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_DialogCopy(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	DialogCopy(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_DialogDelete(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	DialogDelete(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_GetDialogItem(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	DialogItemIndex itemNo;
	DialogItemType itemType;
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
	                     OptResObj_New, item,
	                     PyMac_BuildRect, &box);
	return _res;
}

static PyObject *DlgObj_SetDialogItem(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	DialogItemIndex itemNo;
	DialogItemType itemType;
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

static PyObject *DlgObj_SelectDialogItemText(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	DialogItemIndex itemNo;
	SInt16 strtSel;
	SInt16 endSel;
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

static PyObject *DlgObj_AppendDITL(DialogObject *_self, PyObject *_args)
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

static PyObject *DlgObj_CountDITL(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	DialogItemIndex _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = CountDITL(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *DlgObj_ShortenDITL(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	DialogItemIndex numberItems;
	if (!PyArg_ParseTuple(_args, "h",
	                      &numberItems))
		return NULL;
	ShortenDITL(_self->ob_itself,
	            numberItems);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

#if TARGET_API_MAC_CARBON

static PyObject *DlgObj_InsertDialogItem(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	DialogItemIndex afterItem;
	DialogItemType itemType;
	Handle itemHandle;
	Rect box;
	if (!PyArg_ParseTuple(_args, "hhO&O&",
	                      &afterItem,
	                      &itemType,
	                      ResObj_Convert, &itemHandle,
	                      PyMac_GetRect, &box))
		return NULL;
	_err = InsertDialogItem(_self->ob_itself,
	                        afterItem,
	                        itemType,
	                        itemHandle,
	                        &box);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

#if TARGET_API_MAC_CARBON

static PyObject *DlgObj_RemoveDialogItems(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	DialogItemIndex itemNo;
	DialogItemIndex amountToRemove;
	Boolean disposeItemData;
	if (!PyArg_ParseTuple(_args, "hhb",
	                      &itemNo,
	                      &amountToRemove,
	                      &disposeItemData))
		return NULL;
	_err = RemoveDialogItems(_self->ob_itself,
	                         itemNo,
	                         amountToRemove,
	                         disposeItemData);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

static PyObject *DlgObj_StdFilterProc(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	EventRecord event;
	DialogItemIndex itemHit;
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

static PyObject *DlgObj_SetDialogDefaultItem(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	DialogItemIndex newItem;
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

static PyObject *DlgObj_SetDialogCancelItem(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	DialogItemIndex newItem;
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

static PyObject *DlgObj_SetDialogTracksCursor(DialogObject *_self, PyObject *_args)
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

static PyObject *DlgObj_AutoSizeDialog(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = AutoSizeDialog(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_GetDialogItemAsControl(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 inItemNo;
	ControlHandle outControl;
	if (!PyArg_ParseTuple(_args, "h",
	                      &inItemNo))
		return NULL;
	_err = GetDialogItemAsControl(_self->ob_itself,
	                              inItemNo,
	                              &outControl);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     CtlObj_New, outControl);
	return _res;
}

static PyObject *DlgObj_MoveDialogItem(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 inItemNo;
	SInt16 inHoriz;
	SInt16 inVert;
	if (!PyArg_ParseTuple(_args, "hhh",
	                      &inItemNo,
	                      &inHoriz,
	                      &inVert))
		return NULL;
	_err = MoveDialogItem(_self->ob_itself,
	                      inItemNo,
	                      inHoriz,
	                      inVert);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_SizeDialogItem(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 inItemNo;
	SInt16 inWidth;
	SInt16 inHeight;
	if (!PyArg_ParseTuple(_args, "hhh",
	                      &inItemNo,
	                      &inWidth,
	                      &inHeight))
		return NULL;
	_err = SizeDialogItem(_self->ob_itself,
	                      inItemNo,
	                      inWidth,
	                      inHeight);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_AppendDialogItemList(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 ditlID;
	DITLMethod method;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &ditlID,
	                      &method))
		return NULL;
	_err = AppendDialogItemList(_self->ob_itself,
	                            ditlID,
	                            method);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_SetDialogTimeout(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	SInt16 inButtonToPress;
	UInt32 inSecondsToWait;
	if (!PyArg_ParseTuple(_args, "hl",
	                      &inButtonToPress,
	                      &inSecondsToWait))
		return NULL;
	_err = SetDialogTimeout(_self->ob_itself,
	                        inButtonToPress,
	                        inSecondsToWait);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_GetDialogTimeout(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	SInt16 outButtonToPress;
	UInt32 outSecondsToWait;
	UInt32 outSecondsRemaining;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetDialogTimeout(_self->ob_itself,
	                        &outButtonToPress,
	                        &outSecondsToWait,
	                        &outSecondsRemaining);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("hll",
	                     outButtonToPress,
	                     outSecondsToWait,
	                     outSecondsRemaining);
	return _res;
}

static PyObject *DlgObj_SetModalDialogEventMask(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	EventMask inMask;
	if (!PyArg_ParseTuple(_args, "H",
	                      &inMask))
		return NULL;
	_err = SetModalDialogEventMask(_self->ob_itself,
	                               inMask);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_GetModalDialogEventMask(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	EventMask outMask;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetModalDialogEventMask(_self->ob_itself,
	                               &outMask);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("H",
	                     outMask);
	return _res;
}

static PyObject *DlgObj_GetDialogWindow(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	WindowPtr _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetDialogWindow(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     WinObj_New, _rv);
	return _res;
}

static PyObject *DlgObj_GetDialogTextEditHandle(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TEHandle _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetDialogTextEditHandle(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *DlgObj_GetDialogDefaultItem(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt16 _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetDialogDefaultItem(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *DlgObj_GetDialogCancelItem(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt16 _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetDialogCancelItem(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *DlgObj_GetDialogKeyboardFocusItem(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt16 _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetDialogKeyboardFocusItem(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *DlgObj_SetPortDialogPort(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	SetPortDialogPort(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_GetDialogPort(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CGrafPtr _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetDialogPort(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     GrafObj_New, _rv);
	return _res;
}

#if !TARGET_API_MAC_CARBON

static PyObject *DlgObj_SetGrafPortOfDialog(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	SetGrafPortOfDialog(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

static PyMethodDef DlgObj_methods[] = {
	{"DrawDialog", (PyCFunction)DlgObj_DrawDialog, 1,
	 "() -> None"},
	{"UpdateDialog", (PyCFunction)DlgObj_UpdateDialog, 1,
	 "(RgnHandle updateRgn) -> None"},
	{"HideDialogItem", (PyCFunction)DlgObj_HideDialogItem, 1,
	 "(DialogItemIndex itemNo) -> None"},
	{"ShowDialogItem", (PyCFunction)DlgObj_ShowDialogItem, 1,
	 "(DialogItemIndex itemNo) -> None"},
	{"FindDialogItem", (PyCFunction)DlgObj_FindDialogItem, 1,
	 "(Point thePt) -> (DialogItemIndexZeroBased _rv)"},
	{"DialogCut", (PyCFunction)DlgObj_DialogCut, 1,
	 "() -> None"},
	{"DialogPaste", (PyCFunction)DlgObj_DialogPaste, 1,
	 "() -> None"},
	{"DialogCopy", (PyCFunction)DlgObj_DialogCopy, 1,
	 "() -> None"},
	{"DialogDelete", (PyCFunction)DlgObj_DialogDelete, 1,
	 "() -> None"},
	{"GetDialogItem", (PyCFunction)DlgObj_GetDialogItem, 1,
	 "(DialogItemIndex itemNo) -> (DialogItemType itemType, Handle item, Rect box)"},
	{"SetDialogItem", (PyCFunction)DlgObj_SetDialogItem, 1,
	 "(DialogItemIndex itemNo, DialogItemType itemType, Handle item, Rect box) -> None"},
	{"SelectDialogItemText", (PyCFunction)DlgObj_SelectDialogItemText, 1,
	 "(DialogItemIndex itemNo, SInt16 strtSel, SInt16 endSel) -> None"},
	{"AppendDITL", (PyCFunction)DlgObj_AppendDITL, 1,
	 "(Handle theHandle, DITLMethod method) -> None"},
	{"CountDITL", (PyCFunction)DlgObj_CountDITL, 1,
	 "() -> (DialogItemIndex _rv)"},
	{"ShortenDITL", (PyCFunction)DlgObj_ShortenDITL, 1,
	 "(DialogItemIndex numberItems) -> None"},

#if TARGET_API_MAC_CARBON
	{"InsertDialogItem", (PyCFunction)DlgObj_InsertDialogItem, 1,
	 "(DialogItemIndex afterItem, DialogItemType itemType, Handle itemHandle, Rect box) -> None"},
#endif

#if TARGET_API_MAC_CARBON
	{"RemoveDialogItems", (PyCFunction)DlgObj_RemoveDialogItems, 1,
	 "(DialogItemIndex itemNo, DialogItemIndex amountToRemove, Boolean disposeItemData) -> None"},
#endif
	{"StdFilterProc", (PyCFunction)DlgObj_StdFilterProc, 1,
	 "() -> (Boolean _rv, EventRecord event, DialogItemIndex itemHit)"},
	{"SetDialogDefaultItem", (PyCFunction)DlgObj_SetDialogDefaultItem, 1,
	 "(DialogItemIndex newItem) -> None"},
	{"SetDialogCancelItem", (PyCFunction)DlgObj_SetDialogCancelItem, 1,
	 "(DialogItemIndex newItem) -> None"},
	{"SetDialogTracksCursor", (PyCFunction)DlgObj_SetDialogTracksCursor, 1,
	 "(Boolean tracks) -> None"},
	{"AutoSizeDialog", (PyCFunction)DlgObj_AutoSizeDialog, 1,
	 "() -> None"},
	{"GetDialogItemAsControl", (PyCFunction)DlgObj_GetDialogItemAsControl, 1,
	 "(SInt16 inItemNo) -> (ControlHandle outControl)"},
	{"MoveDialogItem", (PyCFunction)DlgObj_MoveDialogItem, 1,
	 "(SInt16 inItemNo, SInt16 inHoriz, SInt16 inVert) -> None"},
	{"SizeDialogItem", (PyCFunction)DlgObj_SizeDialogItem, 1,
	 "(SInt16 inItemNo, SInt16 inWidth, SInt16 inHeight) -> None"},
	{"AppendDialogItemList", (PyCFunction)DlgObj_AppendDialogItemList, 1,
	 "(SInt16 ditlID, DITLMethod method) -> None"},
	{"SetDialogTimeout", (PyCFunction)DlgObj_SetDialogTimeout, 1,
	 "(SInt16 inButtonToPress, UInt32 inSecondsToWait) -> None"},
	{"GetDialogTimeout", (PyCFunction)DlgObj_GetDialogTimeout, 1,
	 "() -> (SInt16 outButtonToPress, UInt32 outSecondsToWait, UInt32 outSecondsRemaining)"},
	{"SetModalDialogEventMask", (PyCFunction)DlgObj_SetModalDialogEventMask, 1,
	 "(EventMask inMask) -> None"},
	{"GetModalDialogEventMask", (PyCFunction)DlgObj_GetModalDialogEventMask, 1,
	 "() -> (EventMask outMask)"},
	{"GetDialogWindow", (PyCFunction)DlgObj_GetDialogWindow, 1,
	 "() -> (WindowPtr _rv)"},
	{"GetDialogTextEditHandle", (PyCFunction)DlgObj_GetDialogTextEditHandle, 1,
	 "() -> (TEHandle _rv)"},
	{"GetDialogDefaultItem", (PyCFunction)DlgObj_GetDialogDefaultItem, 1,
	 "() -> (SInt16 _rv)"},
	{"GetDialogCancelItem", (PyCFunction)DlgObj_GetDialogCancelItem, 1,
	 "() -> (SInt16 _rv)"},
	{"GetDialogKeyboardFocusItem", (PyCFunction)DlgObj_GetDialogKeyboardFocusItem, 1,
	 "() -> (SInt16 _rv)"},
	{"SetPortDialogPort", (PyCFunction)DlgObj_SetPortDialogPort, 1,
	 "() -> None"},
	{"GetDialogPort", (PyCFunction)DlgObj_GetDialogPort, 1,
	 "() -> (CGrafPtr _rv)"},

#if !TARGET_API_MAC_CARBON
	{"SetGrafPortOfDialog", (PyCFunction)DlgObj_SetGrafPortOfDialog, 1,
	 "() -> None"},
#endif
	{NULL, NULL, 0}
};

PyMethodChain DlgObj_chain = { DlgObj_methods, NULL };

static PyObject *DlgObj_getattr(DialogObject *self, char *name)
{
	return Py_FindMethodInChain(&DlgObj_chain, (PyObject *)self, name);
}

#define DlgObj_setattr NULL

static int DlgObj_compare(DialogObject *self, DialogObject *other)
{
	if ( self->ob_itself > other->ob_itself ) return 1;
	if ( self->ob_itself < other->ob_itself ) return -1;
	return 0;
}

#define DlgObj_repr NULL

static int DlgObj_hash(DialogObject *self)
{
	return (int)self->ob_itself;
}

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
	(cmpfunc) DlgObj_compare, /*tp_compare*/
	(reprfunc) DlgObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) DlgObj_hash, /*tp_hash*/
};

/* --------------------- End object type Dialog --------------------- */


static PyObject *Dlg_NewDialog(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	DialogPtr _rv;
	Rect boundsRect;
	Str255 title;
	Boolean visible;
	SInt16 procID;
	WindowPtr behind;
	Boolean goAwayFlag;
	SInt32 refCon;
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
	_rv = NewDialog((void *)0,
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

static PyObject *Dlg_GetNewDialog(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	DialogPtr _rv;
	SInt16 dialogID;
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

static PyObject *Dlg_NewColorDialog(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	DialogPtr _rv;
	Rect boundsRect;
	Str255 title;
	Boolean visible;
	SInt16 procID;
	WindowPtr behind;
	Boolean goAwayFlag;
	SInt32 refCon;
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

static PyObject *Dlg_ModalDialog(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PyObject* modalFilter;
	DialogItemIndex itemHit;
	if (!PyArg_ParseTuple(_args, "O",
	                      &modalFilter))
		return NULL;
	ModalDialog(Dlg_PassFilterProc(modalFilter),
	            &itemHit);
	_res = Py_BuildValue("h",
	                     itemHit);
	return _res;
}

static PyObject *Dlg_IsDialogEvent(PyObject *_self, PyObject *_args)
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

static PyObject *Dlg_DialogSelect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	EventRecord theEvent;
	DialogPtr theDialog;
	DialogItemIndex itemHit;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetEventRecord, &theEvent))
		return NULL;
	_rv = DialogSelect(&theEvent,
	                   &theDialog,
	                   &itemHit);
	_res = Py_BuildValue("bO&h",
	                     _rv,
	                     DlgObj_WhichDialog, theDialog,
	                     itemHit);
	return _res;
}

static PyObject *Dlg_Alert(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	DialogItemIndex _rv;
	SInt16 alertID;
	PyObject* modalFilter;
	if (!PyArg_ParseTuple(_args, "hO",
	                      &alertID,
	                      &modalFilter))
		return NULL;
	_rv = Alert(alertID,
	            Dlg_PassFilterProc(modalFilter));
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Dlg_StopAlert(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	DialogItemIndex _rv;
	SInt16 alertID;
	PyObject* modalFilter;
	if (!PyArg_ParseTuple(_args, "hO",
	                      &alertID,
	                      &modalFilter))
		return NULL;
	_rv = StopAlert(alertID,
	                Dlg_PassFilterProc(modalFilter));
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Dlg_NoteAlert(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	DialogItemIndex _rv;
	SInt16 alertID;
	PyObject* modalFilter;
	if (!PyArg_ParseTuple(_args, "hO",
	                      &alertID,
	                      &modalFilter))
		return NULL;
	_rv = NoteAlert(alertID,
	                Dlg_PassFilterProc(modalFilter));
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Dlg_CautionAlert(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	DialogItemIndex _rv;
	SInt16 alertID;
	PyObject* modalFilter;
	if (!PyArg_ParseTuple(_args, "hO",
	                      &alertID,
	                      &modalFilter))
		return NULL;
	_rv = CautionAlert(alertID,
	                   Dlg_PassFilterProc(modalFilter));
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Dlg_ParamText(PyObject *_self, PyObject *_args)
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

static PyObject *Dlg_GetDialogItemText(PyObject *_self, PyObject *_args)
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

static PyObject *Dlg_SetDialogItemText(PyObject *_self, PyObject *_args)
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

static PyObject *Dlg_GetAlertStage(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt16 _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetAlertStage();
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Dlg_SetDialogFont(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt16 fontNum;
	if (!PyArg_ParseTuple(_args, "h",
	                      &fontNum))
		return NULL;
	SetDialogFont(fontNum);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Dlg_ResetAlertStage(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	ResetAlertStage();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

#if TARGET_API_MAC_CARBON

static PyObject *Dlg_GetParamText(PyObject *_self, PyObject *_args)
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
	GetParamText(param0,
	             param1,
	             param2,
	             param3);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

static PyObject *Dlg_NewFeaturesDialog(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	DialogPtr _rv;
	Rect inBoundsRect;
	Str255 inTitle;
	Boolean inIsVisible;
	SInt16 inProcID;
	WindowPtr inBehind;
	Boolean inGoAwayFlag;
	SInt32 inRefCon;
	Handle inItemListHandle;
	UInt32 inFlags;
	if (!PyArg_ParseTuple(_args, "O&O&bhO&blO&l",
	                      PyMac_GetRect, &inBoundsRect,
	                      PyMac_GetStr255, inTitle,
	                      &inIsVisible,
	                      &inProcID,
	                      WinObj_Convert, &inBehind,
	                      &inGoAwayFlag,
	                      &inRefCon,
	                      ResObj_Convert, &inItemListHandle,
	                      &inFlags))
		return NULL;
	_rv = NewFeaturesDialog((void *)0,
	                        &inBoundsRect,
	                        inTitle,
	                        inIsVisible,
	                        inProcID,
	                        inBehind,
	                        inGoAwayFlag,
	                        inRefCon,
	                        inItemListHandle,
	                        inFlags);
	_res = Py_BuildValue("O&",
	                     DlgObj_New, _rv);
	return _res;
}

static PyObject *Dlg_GetDialogFromWindow(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	DialogPtr _rv;
	WindowPtr window;
	if (!PyArg_ParseTuple(_args, "O&",
	                      WinObj_Convert, &window))
		return NULL;
	_rv = GetDialogFromWindow(window);
	_res = Py_BuildValue("O&",
	                     DlgObj_New, _rv);
	return _res;
}

static PyObject *Dlg_SetUserItemHandler(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;

		PyObject *new = NULL;
		
		
		if (!PyArg_ParseTuple(_args, "|O", &new))
			return NULL;

		if (Dlg_UserItemProc_callback && new && new != Py_None) {
			PyErr_SetString(Dlg_Error, "Another UserItemProc is already installed");
			return NULL;
		}
		
		if (new == NULL || new == Py_None) {
			new = NULL;
			_res = Py_None;
			Py_INCREF(Py_None);
		} else {
			Py_INCREF(new);
			_res = Py_BuildValue("O&", ResObj_New, (Handle)NewUserItemUPP(Dlg_UnivUserItemProc));
		}
		
		Dlg_UserItemProc_callback = new;
		return _res;

}

static PyMethodDef Dlg_methods[] = {
	{"NewDialog", (PyCFunction)Dlg_NewDialog, 1,
	 "(Rect boundsRect, Str255 title, Boolean visible, SInt16 procID, WindowPtr behind, Boolean goAwayFlag, SInt32 refCon, Handle items) -> (DialogPtr _rv)"},
	{"GetNewDialog", (PyCFunction)Dlg_GetNewDialog, 1,
	 "(SInt16 dialogID, WindowPtr behind) -> (DialogPtr _rv)"},
	{"NewColorDialog", (PyCFunction)Dlg_NewColorDialog, 1,
	 "(Rect boundsRect, Str255 title, Boolean visible, SInt16 procID, WindowPtr behind, Boolean goAwayFlag, SInt32 refCon, Handle items) -> (DialogPtr _rv)"},
	{"ModalDialog", (PyCFunction)Dlg_ModalDialog, 1,
	 "(PyObject* modalFilter) -> (DialogItemIndex itemHit)"},
	{"IsDialogEvent", (PyCFunction)Dlg_IsDialogEvent, 1,
	 "(EventRecord theEvent) -> (Boolean _rv)"},
	{"DialogSelect", (PyCFunction)Dlg_DialogSelect, 1,
	 "(EventRecord theEvent) -> (Boolean _rv, DialogPtr theDialog, DialogItemIndex itemHit)"},
	{"Alert", (PyCFunction)Dlg_Alert, 1,
	 "(SInt16 alertID, PyObject* modalFilter) -> (DialogItemIndex _rv)"},
	{"StopAlert", (PyCFunction)Dlg_StopAlert, 1,
	 "(SInt16 alertID, PyObject* modalFilter) -> (DialogItemIndex _rv)"},
	{"NoteAlert", (PyCFunction)Dlg_NoteAlert, 1,
	 "(SInt16 alertID, PyObject* modalFilter) -> (DialogItemIndex _rv)"},
	{"CautionAlert", (PyCFunction)Dlg_CautionAlert, 1,
	 "(SInt16 alertID, PyObject* modalFilter) -> (DialogItemIndex _rv)"},
	{"ParamText", (PyCFunction)Dlg_ParamText, 1,
	 "(Str255 param0, Str255 param1, Str255 param2, Str255 param3) -> None"},
	{"GetDialogItemText", (PyCFunction)Dlg_GetDialogItemText, 1,
	 "(Handle item) -> (Str255 text)"},
	{"SetDialogItemText", (PyCFunction)Dlg_SetDialogItemText, 1,
	 "(Handle item, Str255 text) -> None"},
	{"GetAlertStage", (PyCFunction)Dlg_GetAlertStage, 1,
	 "() -> (SInt16 _rv)"},
	{"SetDialogFont", (PyCFunction)Dlg_SetDialogFont, 1,
	 "(SInt16 fontNum) -> None"},
	{"ResetAlertStage", (PyCFunction)Dlg_ResetAlertStage, 1,
	 "() -> None"},

#if TARGET_API_MAC_CARBON
	{"GetParamText", (PyCFunction)Dlg_GetParamText, 1,
	 "(Str255 param0, Str255 param1, Str255 param2, Str255 param3) -> None"},
#endif
	{"NewFeaturesDialog", (PyCFunction)Dlg_NewFeaturesDialog, 1,
	 "(Rect inBoundsRect, Str255 inTitle, Boolean inIsVisible, SInt16 inProcID, WindowPtr inBehind, Boolean inGoAwayFlag, SInt32 inRefCon, Handle inItemListHandle, UInt32 inFlags) -> (DialogPtr _rv)"},
	{"GetDialogFromWindow", (PyCFunction)Dlg_GetDialogFromWindow, 1,
	 "(WindowPtr window) -> (DialogPtr _rv)"},
	{"SetUserItemHandler", (PyCFunction)Dlg_SetUserItemHandler, 1,
	 NULL},
	{NULL, NULL, 0}
};



/* Return the WindowPtr corresponding to a DialogObject */
#if 0
WindowPtr
DlgObj_ConvertToWindow(PyObject *self)
{
	if ( DlgObj_Check(self) )
		return GetDialogWindow(((DialogObject *)self)->ob_itself);
	return NULL;
}
#endif
/* Return the object corresponding to the dialog, or None */

PyObject *
DlgObj_WhichDialog(DialogPtr d)
{
	PyObject *it;
	
	if (d == NULL) {
		it = Py_None;
		Py_INCREF(it);
	} else {
		WindowPtr w = GetDialogWindow(d);
		
		it = (PyObject *) GetWRefCon(w);
		if (it == NULL || ((DialogObject *)it)->ob_itself != d || !DlgObj_Check(it)) {
#if 0
			/* Should do this, but we don't have an ob_freeit for dialogs yet. */
			it = WinObj_New(w);
			((WindowObject *)it)->ob_freeit = NULL;
#else
			it = Py_None;
			Py_INCREF(it);
#endif
		} else {
			Py_INCREF(it);
		}
	}
	return it;
}


void initDlg(void)
{
	PyObject *m;
	PyObject *d;



		PyMac_INIT_TOOLBOX_OBJECT_NEW(DialogPtr, DlgObj_New);
		PyMac_INIT_TOOLBOX_OBJECT_NEW(DialogPtr, DlgObj_WhichDialog);
		PyMac_INIT_TOOLBOX_OBJECT_CONVERT(DialogPtr, DlgObj_Convert);


	m = Py_InitModule("Dlg", Dlg_methods);
	d = PyModule_GetDict(m);
	Dlg_Error = PyMac_GetOSErrException();
	if (Dlg_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Dlg_Error) != 0)
		return;
	Dialog_Type.ob_type = &PyType_Type;
	Py_INCREF(&Dialog_Type);
	if (PyDict_SetItemString(d, "DialogType", (PyObject *)&Dialog_Type) != 0)
		Py_FatalError("can't initialize DialogType");
}

/* ========================= End module Dlg ========================= */

