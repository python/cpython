
/* ========================== Module _Dlg =========================== */

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
extern PyObject *_DlgObj_New(DialogRef);
extern PyObject *_DlgObj_WhichDialog(DialogRef);
extern int _DlgObj_Convert(PyObject *, DialogRef *);

#define DlgObj_New _DlgObj_New
#define DlgObj_WhichDialog _DlgObj_WhichDialog
#define DlgObj_Convert _DlgObj_Convert
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

#define DlgObj_Check(x) ((x)->ob_type == &Dialog_Type || PyObject_TypeCheck((x), &Dialog_Type))

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
int DlgObj_Convert(PyObject *v, DialogPtr *p_itself)
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
	self->ob_type->tp_free((PyObject *)self);
}

static PyObject *DlgObj_DrawDialog(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef DrawDialog
	PyMac_PRECHECK(DrawDialog);
#endif
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
#ifndef UpdateDialog
	PyMac_PRECHECK(UpdateDialog);
#endif
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
#ifndef HideDialogItem
	PyMac_PRECHECK(HideDialogItem);
#endif
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
#ifndef ShowDialogItem
	PyMac_PRECHECK(ShowDialogItem);
#endif
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
#ifndef FindDialogItem
	PyMac_PRECHECK(FindDialogItem);
#endif
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
#ifndef DialogCut
	PyMac_PRECHECK(DialogCut);
#endif
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
#ifndef DialogPaste
	PyMac_PRECHECK(DialogPaste);
#endif
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
#ifndef DialogCopy
	PyMac_PRECHECK(DialogCopy);
#endif
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
#ifndef DialogDelete
	PyMac_PRECHECK(DialogDelete);
#endif
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
#ifndef GetDialogItem
	PyMac_PRECHECK(GetDialogItem);
#endif
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
#ifndef SetDialogItem
	PyMac_PRECHECK(SetDialogItem);
#endif
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
#ifndef SelectDialogItemText
	PyMac_PRECHECK(SelectDialogItemText);
#endif
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
#ifndef AppendDITL
	PyMac_PRECHECK(AppendDITL);
#endif
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
#ifndef CountDITL
	PyMac_PRECHECK(CountDITL);
#endif
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
#ifndef ShortenDITL
	PyMac_PRECHECK(ShortenDITL);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &numberItems))
		return NULL;
	ShortenDITL(_self->ob_itself,
	            numberItems);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *DlgObj_InsertDialogItem(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	DialogItemIndex afterItem;
	DialogItemType itemType;
	Handle itemHandle;
	Rect box;
#ifndef InsertDialogItem
	PyMac_PRECHECK(InsertDialogItem);
#endif
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

static PyObject *DlgObj_RemoveDialogItems(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	DialogItemIndex itemNo;
	DialogItemIndex amountToRemove;
	Boolean disposeItemData;
#ifndef RemoveDialogItems
	PyMac_PRECHECK(RemoveDialogItems);
#endif
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

static PyObject *DlgObj_StdFilterProc(DialogObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	EventRecord event;
	DialogItemIndex itemHit;
#ifndef StdFilterProc
	PyMac_PRECHECK(StdFilterProc);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      PyMac_GetEventRecord, &event,
	                      &itemHit))
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
#ifndef SetDialogDefaultItem
	PyMac_PRECHECK(SetDialogDefaultItem);
#endif
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
#ifndef SetDialogCancelItem
	PyMac_PRECHECK(SetDialogCancelItem);
#endif
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
#ifndef SetDialogTracksCursor
	PyMac_PRECHECK(SetDialogTracksCursor);
#endif
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
#ifndef AutoSizeDialog
	PyMac_PRECHECK(AutoSizeDialog);
#endif
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
#ifndef GetDialogItemAsControl
	PyMac_PRECHECK(GetDialogItemAsControl);
#endif
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
#ifndef MoveDialogItem
	PyMac_PRECHECK(MoveDialogItem);
#endif
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
#ifndef SizeDialogItem
	PyMac_PRECHECK(SizeDialogItem);
#endif
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
#ifndef AppendDialogItemList
	PyMac_PRECHECK(AppendDialogItemList);
#endif
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
#ifndef SetDialogTimeout
	PyMac_PRECHECK(SetDialogTimeout);
#endif
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
#ifndef GetDialogTimeout
	PyMac_PRECHECK(GetDialogTimeout);
#endif
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
#ifndef SetModalDialogEventMask
	PyMac_PRECHECK(SetModalDialogEventMask);
#endif
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
#ifndef GetModalDialogEventMask
	PyMac_PRECHECK(GetModalDialogEventMask);
#endif
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
#ifndef GetDialogWindow
	PyMac_PRECHECK(GetDialogWindow);
#endif
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
#ifndef GetDialogTextEditHandle
	PyMac_PRECHECK(GetDialogTextEditHandle);
#endif
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
#ifndef GetDialogDefaultItem
	PyMac_PRECHECK(GetDialogDefaultItem);
#endif
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
#ifndef GetDialogCancelItem
	PyMac_PRECHECK(GetDialogCancelItem);
#endif
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
#ifndef GetDialogKeyboardFocusItem
	PyMac_PRECHECK(GetDialogKeyboardFocusItem);
#endif
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
#ifndef SetPortDialogPort
	PyMac_PRECHECK(SetPortDialogPort);
#endif
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
#ifndef GetDialogPort
	PyMac_PRECHECK(GetDialogPort);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetDialogPort(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     GrafObj_New, _rv);
	return _res;
}

static PyMethodDef DlgObj_methods[] = {
	{"DrawDialog", (PyCFunction)DlgObj_DrawDialog, 1,
	 PyDoc_STR("() -> None")},
	{"UpdateDialog", (PyCFunction)DlgObj_UpdateDialog, 1,
	 PyDoc_STR("(RgnHandle updateRgn) -> None")},
	{"HideDialogItem", (PyCFunction)DlgObj_HideDialogItem, 1,
	 PyDoc_STR("(DialogItemIndex itemNo) -> None")},
	{"ShowDialogItem", (PyCFunction)DlgObj_ShowDialogItem, 1,
	 PyDoc_STR("(DialogItemIndex itemNo) -> None")},
	{"FindDialogItem", (PyCFunction)DlgObj_FindDialogItem, 1,
	 PyDoc_STR("(Point thePt) -> (DialogItemIndexZeroBased _rv)")},
	{"DialogCut", (PyCFunction)DlgObj_DialogCut, 1,
	 PyDoc_STR("() -> None")},
	{"DialogPaste", (PyCFunction)DlgObj_DialogPaste, 1,
	 PyDoc_STR("() -> None")},
	{"DialogCopy", (PyCFunction)DlgObj_DialogCopy, 1,
	 PyDoc_STR("() -> None")},
	{"DialogDelete", (PyCFunction)DlgObj_DialogDelete, 1,
	 PyDoc_STR("() -> None")},
	{"GetDialogItem", (PyCFunction)DlgObj_GetDialogItem, 1,
	 PyDoc_STR("(DialogItemIndex itemNo) -> (DialogItemType itemType, Handle item, Rect box)")},
	{"SetDialogItem", (PyCFunction)DlgObj_SetDialogItem, 1,
	 PyDoc_STR("(DialogItemIndex itemNo, DialogItemType itemType, Handle item, Rect box) -> None")},
	{"SelectDialogItemText", (PyCFunction)DlgObj_SelectDialogItemText, 1,
	 PyDoc_STR("(DialogItemIndex itemNo, SInt16 strtSel, SInt16 endSel) -> None")},
	{"AppendDITL", (PyCFunction)DlgObj_AppendDITL, 1,
	 PyDoc_STR("(Handle theHandle, DITLMethod method) -> None")},
	{"CountDITL", (PyCFunction)DlgObj_CountDITL, 1,
	 PyDoc_STR("() -> (DialogItemIndex _rv)")},
	{"ShortenDITL", (PyCFunction)DlgObj_ShortenDITL, 1,
	 PyDoc_STR("(DialogItemIndex numberItems) -> None")},
	{"InsertDialogItem", (PyCFunction)DlgObj_InsertDialogItem, 1,
	 PyDoc_STR("(DialogItemIndex afterItem, DialogItemType itemType, Handle itemHandle, Rect box) -> None")},
	{"RemoveDialogItems", (PyCFunction)DlgObj_RemoveDialogItems, 1,
	 PyDoc_STR("(DialogItemIndex itemNo, DialogItemIndex amountToRemove, Boolean disposeItemData) -> None")},
	{"StdFilterProc", (PyCFunction)DlgObj_StdFilterProc, 1,
	 PyDoc_STR("(EventRecord event, DialogItemIndex itemHit) -> (Boolean _rv, EventRecord event, DialogItemIndex itemHit)")},
	{"SetDialogDefaultItem", (PyCFunction)DlgObj_SetDialogDefaultItem, 1,
	 PyDoc_STR("(DialogItemIndex newItem) -> None")},
	{"SetDialogCancelItem", (PyCFunction)DlgObj_SetDialogCancelItem, 1,
	 PyDoc_STR("(DialogItemIndex newItem) -> None")},
	{"SetDialogTracksCursor", (PyCFunction)DlgObj_SetDialogTracksCursor, 1,
	 PyDoc_STR("(Boolean tracks) -> None")},
	{"AutoSizeDialog", (PyCFunction)DlgObj_AutoSizeDialog, 1,
	 PyDoc_STR("() -> None")},
	{"GetDialogItemAsControl", (PyCFunction)DlgObj_GetDialogItemAsControl, 1,
	 PyDoc_STR("(SInt16 inItemNo) -> (ControlHandle outControl)")},
	{"MoveDialogItem", (PyCFunction)DlgObj_MoveDialogItem, 1,
	 PyDoc_STR("(SInt16 inItemNo, SInt16 inHoriz, SInt16 inVert) -> None")},
	{"SizeDialogItem", (PyCFunction)DlgObj_SizeDialogItem, 1,
	 PyDoc_STR("(SInt16 inItemNo, SInt16 inWidth, SInt16 inHeight) -> None")},
	{"AppendDialogItemList", (PyCFunction)DlgObj_AppendDialogItemList, 1,
	 PyDoc_STR("(SInt16 ditlID, DITLMethod method) -> None")},
	{"SetDialogTimeout", (PyCFunction)DlgObj_SetDialogTimeout, 1,
	 PyDoc_STR("(SInt16 inButtonToPress, UInt32 inSecondsToWait) -> None")},
	{"GetDialogTimeout", (PyCFunction)DlgObj_GetDialogTimeout, 1,
	 PyDoc_STR("() -> (SInt16 outButtonToPress, UInt32 outSecondsToWait, UInt32 outSecondsRemaining)")},
	{"SetModalDialogEventMask", (PyCFunction)DlgObj_SetModalDialogEventMask, 1,
	 PyDoc_STR("(EventMask inMask) -> None")},
	{"GetModalDialogEventMask", (PyCFunction)DlgObj_GetModalDialogEventMask, 1,
	 PyDoc_STR("() -> (EventMask outMask)")},
	{"GetDialogWindow", (PyCFunction)DlgObj_GetDialogWindow, 1,
	 PyDoc_STR("() -> (WindowPtr _rv)")},
	{"GetDialogTextEditHandle", (PyCFunction)DlgObj_GetDialogTextEditHandle, 1,
	 PyDoc_STR("() -> (TEHandle _rv)")},
	{"GetDialogDefaultItem", (PyCFunction)DlgObj_GetDialogDefaultItem, 1,
	 PyDoc_STR("() -> (SInt16 _rv)")},
	{"GetDialogCancelItem", (PyCFunction)DlgObj_GetDialogCancelItem, 1,
	 PyDoc_STR("() -> (SInt16 _rv)")},
	{"GetDialogKeyboardFocusItem", (PyCFunction)DlgObj_GetDialogKeyboardFocusItem, 1,
	 PyDoc_STR("() -> (SInt16 _rv)")},
	{"SetPortDialogPort", (PyCFunction)DlgObj_SetPortDialogPort, 1,
	 PyDoc_STR("() -> None")},
	{"GetDialogPort", (PyCFunction)DlgObj_GetDialogPort, 1,
	 PyDoc_STR("() -> (CGrafPtr _rv)")},
	{NULL, NULL, 0}
};

#define DlgObj_getsetlist NULL


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
#define DlgObj_tp_init 0

#define DlgObj_tp_alloc PyType_GenericAlloc

static PyObject *DlgObj_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *self;
	DialogPtr itself;
	char *kw[] = {"itself", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&", kw, DlgObj_Convert, &itself)) return NULL;
	if ((self = type->tp_alloc(type, 0)) == NULL) return NULL;
	((DialogObject *)self)->ob_itself = itself;
	return self;
}

#define DlgObj_tp_free PyObject_Del


PyTypeObject Dialog_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"_Dlg.Dialog", /*tp_name*/
	sizeof(DialogObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) DlgObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc)0, /*tp_getattr*/
	(setattrfunc)0, /*tp_setattr*/
	(cmpfunc) DlgObj_compare, /*tp_compare*/
	(reprfunc) DlgObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) DlgObj_hash, /*tp_hash*/
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
	DlgObj_methods, /* tp_methods */
	0, /*tp_members*/
	DlgObj_getsetlist, /*tp_getset*/
	0, /*tp_base*/
	0, /*tp_dict*/
	0, /*tp_descr_get*/
	0, /*tp_descr_set*/
	0, /*tp_dictoffset*/
	DlgObj_tp_init, /* tp_init */
	DlgObj_tp_alloc, /* tp_alloc */
	DlgObj_tp_new, /* tp_new */
	DlgObj_tp_free, /* tp_free */
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
#ifndef NewDialog
	PyMac_PRECHECK(NewDialog);
#endif
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
#ifndef GetNewDialog
	PyMac_PRECHECK(GetNewDialog);
#endif
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
#ifndef NewColorDialog
	PyMac_PRECHECK(NewColorDialog);
#endif
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
#ifndef ModalDialog
	PyMac_PRECHECK(ModalDialog);
#endif
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
#ifndef IsDialogEvent
	PyMac_PRECHECK(IsDialogEvent);
#endif
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
#ifndef DialogSelect
	PyMac_PRECHECK(DialogSelect);
#endif
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
#ifndef Alert
	PyMac_PRECHECK(Alert);
#endif
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
#ifndef StopAlert
	PyMac_PRECHECK(StopAlert);
#endif
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
#ifndef NoteAlert
	PyMac_PRECHECK(NoteAlert);
#endif
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
#ifndef CautionAlert
	PyMac_PRECHECK(CautionAlert);
#endif
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
#ifndef ParamText
	PyMac_PRECHECK(ParamText);
#endif
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
#ifndef GetDialogItemText
	PyMac_PRECHECK(GetDialogItemText);
#endif
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
#ifndef SetDialogItemText
	PyMac_PRECHECK(SetDialogItemText);
#endif
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
#ifndef GetAlertStage
	PyMac_PRECHECK(GetAlertStage);
#endif
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
#ifndef SetDialogFont
	PyMac_PRECHECK(SetDialogFont);
#endif
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
#ifndef ResetAlertStage
	PyMac_PRECHECK(ResetAlertStage);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	ResetAlertStage();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Dlg_GetParamText(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Str255 param0;
	Str255 param1;
	Str255 param2;
	Str255 param3;
#ifndef GetParamText
	PyMac_PRECHECK(GetParamText);
#endif
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
#ifndef NewFeaturesDialog
	PyMac_PRECHECK(NewFeaturesDialog);
#endif
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
#ifndef GetDialogFromWindow
	PyMac_PRECHECK(GetDialogFromWindow);
#endif
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
	 PyDoc_STR("(Rect boundsRect, Str255 title, Boolean visible, SInt16 procID, WindowPtr behind, Boolean goAwayFlag, SInt32 refCon, Handle items) -> (DialogPtr _rv)")},
	{"GetNewDialog", (PyCFunction)Dlg_GetNewDialog, 1,
	 PyDoc_STR("(SInt16 dialogID, WindowPtr behind) -> (DialogPtr _rv)")},
	{"NewColorDialog", (PyCFunction)Dlg_NewColorDialog, 1,
	 PyDoc_STR("(Rect boundsRect, Str255 title, Boolean visible, SInt16 procID, WindowPtr behind, Boolean goAwayFlag, SInt32 refCon, Handle items) -> (DialogPtr _rv)")},
	{"ModalDialog", (PyCFunction)Dlg_ModalDialog, 1,
	 PyDoc_STR("(PyObject* modalFilter) -> (DialogItemIndex itemHit)")},
	{"IsDialogEvent", (PyCFunction)Dlg_IsDialogEvent, 1,
	 PyDoc_STR("(EventRecord theEvent) -> (Boolean _rv)")},
	{"DialogSelect", (PyCFunction)Dlg_DialogSelect, 1,
	 PyDoc_STR("(EventRecord theEvent) -> (Boolean _rv, DialogPtr theDialog, DialogItemIndex itemHit)")},
	{"Alert", (PyCFunction)Dlg_Alert, 1,
	 PyDoc_STR("(SInt16 alertID, PyObject* modalFilter) -> (DialogItemIndex _rv)")},
	{"StopAlert", (PyCFunction)Dlg_StopAlert, 1,
	 PyDoc_STR("(SInt16 alertID, PyObject* modalFilter) -> (DialogItemIndex _rv)")},
	{"NoteAlert", (PyCFunction)Dlg_NoteAlert, 1,
	 PyDoc_STR("(SInt16 alertID, PyObject* modalFilter) -> (DialogItemIndex _rv)")},
	{"CautionAlert", (PyCFunction)Dlg_CautionAlert, 1,
	 PyDoc_STR("(SInt16 alertID, PyObject* modalFilter) -> (DialogItemIndex _rv)")},
	{"ParamText", (PyCFunction)Dlg_ParamText, 1,
	 PyDoc_STR("(Str255 param0, Str255 param1, Str255 param2, Str255 param3) -> None")},
	{"GetDialogItemText", (PyCFunction)Dlg_GetDialogItemText, 1,
	 PyDoc_STR("(Handle item) -> (Str255 text)")},
	{"SetDialogItemText", (PyCFunction)Dlg_SetDialogItemText, 1,
	 PyDoc_STR("(Handle item, Str255 text) -> None")},
	{"GetAlertStage", (PyCFunction)Dlg_GetAlertStage, 1,
	 PyDoc_STR("() -> (SInt16 _rv)")},
	{"SetDialogFont", (PyCFunction)Dlg_SetDialogFont, 1,
	 PyDoc_STR("(SInt16 fontNum) -> None")},
	{"ResetAlertStage", (PyCFunction)Dlg_ResetAlertStage, 1,
	 PyDoc_STR("() -> None")},
	{"GetParamText", (PyCFunction)Dlg_GetParamText, 1,
	 PyDoc_STR("(Str255 param0, Str255 param1, Str255 param2, Str255 param3) -> None")},
	{"NewFeaturesDialog", (PyCFunction)Dlg_NewFeaturesDialog, 1,
	 PyDoc_STR("(Rect inBoundsRect, Str255 inTitle, Boolean inIsVisible, SInt16 inProcID, WindowPtr inBehind, Boolean inGoAwayFlag, SInt32 inRefCon, Handle inItemListHandle, UInt32 inFlags) -> (DialogPtr _rv)")},
	{"GetDialogFromWindow", (PyCFunction)Dlg_GetDialogFromWindow, 1,
	 PyDoc_STR("(WindowPtr window) -> (DialogPtr _rv)")},
	{"SetUserItemHandler", (PyCFunction)Dlg_SetUserItemHandler, 1,
	 PyDoc_STR(NULL)},
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


void init_Dlg(void)
{
	PyObject *m;
	PyObject *d;



		PyMac_INIT_TOOLBOX_OBJECT_NEW(DialogPtr, DlgObj_New);
		PyMac_INIT_TOOLBOX_OBJECT_NEW(DialogPtr, DlgObj_WhichDialog);
		PyMac_INIT_TOOLBOX_OBJECT_CONVERT(DialogPtr, DlgObj_Convert);


	m = Py_InitModule("_Dlg", Dlg_methods);
	d = PyModule_GetDict(m);
	Dlg_Error = PyMac_GetOSErrException();
	if (Dlg_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Dlg_Error) != 0)
		return;
	Dialog_Type.ob_type = &PyType_Type;
	if (PyType_Ready(&Dialog_Type) < 0) return;
	Py_INCREF(&Dialog_Type);
	PyModule_AddObject(m, "Dialog", (PyObject *)&Dialog_Type);
	/* Backward-compatible name */
	Py_INCREF(&Dialog_Type);
	PyModule_AddObject(m, "DialogType", (PyObject *)&Dialog_Type);
}

/* ======================== End module _Dlg ========================= */

