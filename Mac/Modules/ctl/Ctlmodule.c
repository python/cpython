
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

#include <Controls.h>

#define resNotFound -192 /* Can't include <Errors.h> because of Python's "errors.h" */

extern PyObject *CtlObj_WhichControl(ControlHandle); /* Forward */
extern PyObject *QdRGB_New(RGBColorPtr);
extern QdRGB_Convert(PyObject *, RGBColorPtr);

#ifdef THINK_C
#define  ControlActionUPP ProcPtr
#endif

/*
** Parse/generate ControlFontStyleRec records
*/
#if 0 /* Not needed */
PyObject *ControlFontStyle_New(itself)
	ControlFontStyleRec *itself;
{

	return Py_BuildValue("hhhhhhO&O&", itself->flags, itself->font,
		itself->size, itself->style, itself->mode, itself->just,
		QdRGB_New, &itself->foreColor, QdRGB_New, &itself->backColor);
}
#endif

ControlFontStyle_Convert(v, itself)
	PyObject *v;
	ControlFontStyleRec *itself;
{
	return PyArg_ParseTuple(v, "hhhhhhO&O&", &itself->flags,
		&itself->font, &itself->size, &itself->style, &itself->mode, 
		&itself->just, QdRGB_Convert, &itself->foreColor, 
		QdRGB_Convert, &itself->backColor);
}

/* TrackControl callback support */
static PyObject *tracker;
static ControlActionUPP mytracker_upp;

extern int settrackfunc(PyObject *); 	/* forward */
extern void clrtrackfunc(void);	/* forward */

static PyObject *Ctl_Error;

/* ---------------------- Object type Control ----------------------- */

PyTypeObject Control_Type;

#define CtlObj_Check(x) ((x)->ob_type == &Control_Type)

typedef struct ControlObject {
	PyObject_HEAD
	ControlHandle ob_itself;
} ControlObject;

PyObject *CtlObj_New(itself)
	ControlHandle itself;
{
	ControlObject *it;
	if (itself == NULL) return PyMac_Error(resNotFound);
	it = PyObject_NEW(ControlObject, &Control_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	SetControlReference(itself, (long)it);
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
	if (self->ob_itself) SetControlReference(self->ob_itself, (long)0); /* Make it forget about us */
	PyMem_DEL(self);
}

static PyObject *CtlObj_HiliteControl(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	ControlPartCode hiliteState;
	if (!PyArg_ParseTuple(_args, "h",
	                      &hiliteState))
		return NULL;
	HiliteControl(_self->ob_itself,
	              hiliteState);
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

static PyObject *CtlObj_IsControlActive(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = IsControlActive(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *CtlObj_IsControlVisible(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = IsControlVisible(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *CtlObj_ActivateControl(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = ActivateControl(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_DeactivateControl(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = DeactivateControl(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_SetControlVisibility(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	Boolean inIsVisible;
	Boolean inDoDraw;
	if (!PyArg_ParseTuple(_args, "bb",
	                      &inIsVisible,
	                      &inDoDraw))
		return NULL;
	_err = SetControlVisibility(_self->ob_itself,
	                            inIsVisible,
	                            inDoDraw);
	if (_err != noErr) return PyMac_Error(_err);
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

static PyObject *CtlObj_GetBestControlRect(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	Rect outRect;
	SInt16 outBaseLineOffset;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetBestControlRect(_self->ob_itself,
	                          &outRect,
	                          &outBaseLineOffset);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&h",
	                     PyMac_BuildRect, &outRect,
	                     outBaseLineOffset);
	return _res;
}

static PyObject *CtlObj_SetControlFontStyle(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	ControlFontStyleRec inStyle;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ControlFontStyle_Convert, &inStyle))
		return NULL;
	_err = SetControlFontStyle(_self->ob_itself,
	                           &inStyle);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_DrawControlInCurrentPort(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	DrawControlInCurrentPort(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_SetUpControlBackground(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 inDepth;
	Boolean inIsColorDevice;
	if (!PyArg_ParseTuple(_args, "hb",
	                      &inDepth,
	                      &inIsColorDevice))
		return NULL;
	_err = SetUpControlBackground(_self->ob_itself,
	                              inDepth,
	                              inIsColorDevice);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_DragControl(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Point startPoint;
	Rect limitRect;
	Rect slopRect;
	DragConstraint axis;
	if (!PyArg_ParseTuple(_args, "O&O&O&h",
	                      PyMac_GetPoint, &startPoint,
	                      PyMac_GetRect, &limitRect,
	                      PyMac_GetRect, &slopRect,
	                      &axis))
		return NULL;
	DragControl(_self->ob_itself,
	            startPoint,
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
	ControlPartCode _rv;
	Point testPoint;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &testPoint))
		return NULL;
	_rv = TestControl(_self->ob_itself,
	                  testPoint);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *CtlObj_HandleControlKey(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	SInt16 _rv;
	SInt16 inKeyCode;
	SInt16 inCharCode;
	SInt16 inModifiers;
	if (!PyArg_ParseTuple(_args, "hhh",
	                      &inKeyCode,
	                      &inCharCode,
	                      &inModifiers))
		return NULL;
	_rv = HandleControlKey(_self->ob_itself,
	                       inKeyCode,
	                       inCharCode,
	                       inModifiers);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *CtlObj_MoveControl(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	SInt16 h;
	SInt16 v;
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
	SInt16 w;
	SInt16 h;
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

static PyObject *CtlObj_SetControlTitle(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Str255 title;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetStr255, title))
		return NULL;
	SetControlTitle(_self->ob_itself,
	                title);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_GetControlTitle(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Str255 title;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetStr255, title))
		return NULL;
	GetControlTitle(_self->ob_itself,
	                title);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_GetControlValue(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	SInt16 _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetControlValue(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *CtlObj_SetControlValue(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	SInt16 newValue;
	if (!PyArg_ParseTuple(_args, "h",
	                      &newValue))
		return NULL;
	SetControlValue(_self->ob_itself,
	                newValue);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_GetControlMinimum(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	SInt16 _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetControlMinimum(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *CtlObj_SetControlMinimum(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	SInt16 newMinimum;
	if (!PyArg_ParseTuple(_args, "h",
	                      &newMinimum))
		return NULL;
	SetControlMinimum(_self->ob_itself,
	                  newMinimum);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_GetControlMaximum(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	SInt16 _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetControlMaximum(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *CtlObj_SetControlMaximum(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	SInt16 newMaximum;
	if (!PyArg_ParseTuple(_args, "h",
	                      &newMaximum))
		return NULL;
	SetControlMaximum(_self->ob_itself,
	                  newMaximum);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_GetControlVariant(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	ControlVariant _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetControlVariant(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *CtlObj_SetControlReference(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	SInt32 data;
	if (!PyArg_ParseTuple(_args, "l",
	                      &data))
		return NULL;
	SetControlReference(_self->ob_itself,
	                    data);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_GetControlReference(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	SInt32 _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetControlReference(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *CtlObj_GetAuxiliaryControlRecord(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	AuxCtlHandle acHndl;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetAuxiliaryControlRecord(_self->ob_itself,
	                                &acHndl);
	_res = Py_BuildValue("bO&",
	                     _rv,
	                     ResObj_New, acHndl);
	return _res;
}

static PyObject *CtlObj_SetControlColor(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	CCTabHandle newColorTable;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &newColorTable))
		return NULL;
	SetControlColor(_self->ob_itself,
	                newColorTable);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_SendControlMessage(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	SInt32 _rv;
	SInt16 inMessage;
	SInt32 inParam;
	if (!PyArg_ParseTuple(_args, "hl",
	                      &inMessage,
	                      &inParam))
		return NULL;
	_rv = SendControlMessage(_self->ob_itself,
	                         inMessage,
	                         inParam);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *CtlObj_EmbedControl(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	ControlHandle inContainer;
	if (!PyArg_ParseTuple(_args, "O&",
	                      CtlObj_Convert, &inContainer))
		return NULL;
	_err = EmbedControl(_self->ob_itself,
	                    inContainer);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_AutoEmbedControl(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	WindowPtr inWindow;
	if (!PyArg_ParseTuple(_args, "O&",
	                      WinObj_Convert, &inWindow))
		return NULL;
	_err = AutoEmbedControl(_self->ob_itself,
	                        inWindow);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_GetSuperControl(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	ControlHandle outParent;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetSuperControl(_self->ob_itself,
	                       &outParent);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     CtlObj_WhichControl, outParent);
	return _res;
}

static PyObject *CtlObj_CountSubControls(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 outNumChildren;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = CountSubControls(_self->ob_itself,
	                        &outNumChildren);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     outNumChildren);
	return _res;
}

static PyObject *CtlObj_GetIndexedSubControl(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 inIndex;
	ControlHandle outSubControl;
	if (!PyArg_ParseTuple(_args, "h",
	                      &inIndex))
		return NULL;
	_err = GetIndexedSubControl(_self->ob_itself,
	                            inIndex,
	                            &outSubControl);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     CtlObj_WhichControl, outSubControl);
	return _res;
}

static PyObject *CtlObj_SetControlSupervisor(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	ControlHandle inBoss;
	if (!PyArg_ParseTuple(_args, "O&",
	                      CtlObj_Convert, &inBoss))
		return NULL;
	_err = SetControlSupervisor(_self->ob_itself,
	                            inBoss);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *CtlObj_GetControlFeatures(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	UInt32 outFeatures;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetControlFeatures(_self->ob_itself,
	                          &outFeatures);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outFeatures);
	return _res;
}

static PyObject *CtlObj_GetControlDataSize(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	ControlPartCode inPart;
	ResType inTagName;
	Size outMaxSize;
	if (!PyArg_ParseTuple(_args, "hO&",
	                      &inPart,
	                      PyMac_GetOSType, &inTagName))
		return NULL;
	_err = GetControlDataSize(_self->ob_itself,
	                          inPart,
	                          inTagName,
	                          &outMaxSize);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outMaxSize);
	return _res;
}

static PyObject *CtlObj_as_Resource(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;

	return ResObj_New((Handle)_self->ob_itself);

}

static PyObject *CtlObj_DisposeControl(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;

		if (!PyArg_ParseTuple(_args, ""))
			return NULL;
		if ( _self->ob_itself ) {
			SetControlReference(_self->ob_itself, (long)0); /* Make it forget about us */
			DisposeControl(_self->ob_itself);
			_self->ob_itself = NULL;
		}
		Py_INCREF(Py_None);
		_res = Py_None;
		return _res;

}

static PyObject *CtlObj_TrackControl(_self, _args)
	ControlObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;

	ControlPartCode _rv;
	Point startPoint;
	ControlActionUPP upp = 0;
	PyObject *callback = 0;

	if (!PyArg_ParseTuple(_args, "O&|O",
	                      PyMac_GetPoint, &startPoint, &callback))
		return NULL;
	if (callback && callback != Py_None) {
		if (PyInt_Check(callback) && PyInt_AS_LONG(callback) == -1)
			upp = (ControlActionUPP)-1;
		else {
			settrackfunc(callback);
			upp = mytracker_upp;
		}
	}
	_rv = TrackControl(_self->ob_itself,
	                   startPoint,
	                   upp);
	clrtrackfunc();
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;

}

static PyMethodDef CtlObj_methods[] = {
	{"HiliteControl", (PyCFunction)CtlObj_HiliteControl, 1,
	 "(ControlPartCode hiliteState) -> None"},
	{"ShowControl", (PyCFunction)CtlObj_ShowControl, 1,
	 "() -> None"},
	{"HideControl", (PyCFunction)CtlObj_HideControl, 1,
	 "() -> None"},
	{"IsControlActive", (PyCFunction)CtlObj_IsControlActive, 1,
	 "() -> (Boolean _rv)"},
	{"IsControlVisible", (PyCFunction)CtlObj_IsControlVisible, 1,
	 "() -> (Boolean _rv)"},
	{"ActivateControl", (PyCFunction)CtlObj_ActivateControl, 1,
	 "() -> None"},
	{"DeactivateControl", (PyCFunction)CtlObj_DeactivateControl, 1,
	 "() -> None"},
	{"SetControlVisibility", (PyCFunction)CtlObj_SetControlVisibility, 1,
	 "(Boolean inIsVisible, Boolean inDoDraw) -> None"},
	{"Draw1Control", (PyCFunction)CtlObj_Draw1Control, 1,
	 "() -> None"},
	{"GetBestControlRect", (PyCFunction)CtlObj_GetBestControlRect, 1,
	 "() -> (Rect outRect, SInt16 outBaseLineOffset)"},
	{"SetControlFontStyle", (PyCFunction)CtlObj_SetControlFontStyle, 1,
	 "(ControlFontStyleRec inStyle) -> None"},
	{"DrawControlInCurrentPort", (PyCFunction)CtlObj_DrawControlInCurrentPort, 1,
	 "() -> None"},
	{"SetUpControlBackground", (PyCFunction)CtlObj_SetUpControlBackground, 1,
	 "(SInt16 inDepth, Boolean inIsColorDevice) -> None"},
	{"DragControl", (PyCFunction)CtlObj_DragControl, 1,
	 "(Point startPoint, Rect limitRect, Rect slopRect, DragConstraint axis) -> None"},
	{"TestControl", (PyCFunction)CtlObj_TestControl, 1,
	 "(Point testPoint) -> (ControlPartCode _rv)"},
	{"HandleControlKey", (PyCFunction)CtlObj_HandleControlKey, 1,
	 "(SInt16 inKeyCode, SInt16 inCharCode, SInt16 inModifiers) -> (SInt16 _rv)"},
	{"MoveControl", (PyCFunction)CtlObj_MoveControl, 1,
	 "(SInt16 h, SInt16 v) -> None"},
	{"SizeControl", (PyCFunction)CtlObj_SizeControl, 1,
	 "(SInt16 w, SInt16 h) -> None"},
	{"SetControlTitle", (PyCFunction)CtlObj_SetControlTitle, 1,
	 "(Str255 title) -> None"},
	{"GetControlTitle", (PyCFunction)CtlObj_GetControlTitle, 1,
	 "(Str255 title) -> None"},
	{"GetControlValue", (PyCFunction)CtlObj_GetControlValue, 1,
	 "() -> (SInt16 _rv)"},
	{"SetControlValue", (PyCFunction)CtlObj_SetControlValue, 1,
	 "(SInt16 newValue) -> None"},
	{"GetControlMinimum", (PyCFunction)CtlObj_GetControlMinimum, 1,
	 "() -> (SInt16 _rv)"},
	{"SetControlMinimum", (PyCFunction)CtlObj_SetControlMinimum, 1,
	 "(SInt16 newMinimum) -> None"},
	{"GetControlMaximum", (PyCFunction)CtlObj_GetControlMaximum, 1,
	 "() -> (SInt16 _rv)"},
	{"SetControlMaximum", (PyCFunction)CtlObj_SetControlMaximum, 1,
	 "(SInt16 newMaximum) -> None"},
	{"GetControlVariant", (PyCFunction)CtlObj_GetControlVariant, 1,
	 "() -> (ControlVariant _rv)"},
	{"SetControlReference", (PyCFunction)CtlObj_SetControlReference, 1,
	 "(SInt32 data) -> None"},
	{"GetControlReference", (PyCFunction)CtlObj_GetControlReference, 1,
	 "() -> (SInt32 _rv)"},
	{"GetAuxiliaryControlRecord", (PyCFunction)CtlObj_GetAuxiliaryControlRecord, 1,
	 "() -> (Boolean _rv, AuxCtlHandle acHndl)"},
	{"SetControlColor", (PyCFunction)CtlObj_SetControlColor, 1,
	 "(CCTabHandle newColorTable) -> None"},
	{"SendControlMessage", (PyCFunction)CtlObj_SendControlMessage, 1,
	 "(SInt16 inMessage, SInt32 inParam) -> (SInt32 _rv)"},
	{"EmbedControl", (PyCFunction)CtlObj_EmbedControl, 1,
	 "(ControlHandle inContainer) -> None"},
	{"AutoEmbedControl", (PyCFunction)CtlObj_AutoEmbedControl, 1,
	 "(WindowPtr inWindow) -> None"},
	{"GetSuperControl", (PyCFunction)CtlObj_GetSuperControl, 1,
	 "() -> (ControlHandle outParent)"},
	{"CountSubControls", (PyCFunction)CtlObj_CountSubControls, 1,
	 "() -> (SInt16 outNumChildren)"},
	{"GetIndexedSubControl", (PyCFunction)CtlObj_GetIndexedSubControl, 1,
	 "(SInt16 inIndex) -> (ControlHandle outSubControl)"},
	{"SetControlSupervisor", (PyCFunction)CtlObj_SetControlSupervisor, 1,
	 "(ControlHandle inBoss) -> None"},
	{"GetControlFeatures", (PyCFunction)CtlObj_GetControlFeatures, 1,
	 "() -> (UInt32 outFeatures)"},
	{"GetControlDataSize", (PyCFunction)CtlObj_GetControlDataSize, 1,
	 "(ControlPartCode inPart, ResType inTagName) -> (Size outMaxSize)"},
	{"as_Resource", (PyCFunction)CtlObj_as_Resource, 1,
	 "Return this Control as a Resource"},
	{"DisposeControl", (PyCFunction)CtlObj_DisposeControl, 1,
	 "() -> None"},
	{"TrackControl", (PyCFunction)CtlObj_TrackControl, 1,
	 NULL},
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
	WindowPtr owningWindow;
	Rect boundsRect;
	Str255 controlTitle;
	Boolean initiallyVisible;
	SInt16 initialValue;
	SInt16 minimumValue;
	SInt16 maximumValue;
	SInt16 procID;
	SInt32 controlReference;
	if (!PyArg_ParseTuple(_args, "O&O&O&bhhhhl",
	                      WinObj_Convert, &owningWindow,
	                      PyMac_GetRect, &boundsRect,
	                      PyMac_GetStr255, controlTitle,
	                      &initiallyVisible,
	                      &initialValue,
	                      &minimumValue,
	                      &maximumValue,
	                      &procID,
	                      &controlReference))
		return NULL;
	_rv = NewControl(owningWindow,
	                 &boundsRect,
	                 controlTitle,
	                 initiallyVisible,
	                 initialValue,
	                 minimumValue,
	                 maximumValue,
	                 procID,
	                 controlReference);
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
	SInt16 resourceID;
	WindowPtr owningWindow;
	if (!PyArg_ParseTuple(_args, "hO&",
	                      &resourceID,
	                      WinObj_Convert, &owningWindow))
		return NULL;
	_rv = GetNewControl(resourceID,
	                    owningWindow);
	_res = Py_BuildValue("O&",
	                     CtlObj_New, _rv);
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

static PyObject *Ctl_UpdateControls(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	WindowPtr theWindow;
	RgnHandle updateRegion;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      WinObj_Convert, &theWindow,
	                      ResObj_Convert, &updateRegion))
		return NULL;
	UpdateControls(theWindow,
	               updateRegion);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Ctl_FindControl(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	ControlPartCode _rv;
	Point testPoint;
	WindowPtr theWindow;
	ControlHandle theControl;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetPoint, &testPoint,
	                      WinObj_Convert, &theWindow))
		return NULL;
	_rv = FindControl(testPoint,
	                  theWindow,
	                  &theControl);
	_res = Py_BuildValue("hO&",
	                     _rv,
	                     CtlObj_WhichControl, theControl);
	return _res;
}

static PyObject *Ctl_FindControlUnderMouse(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	ControlHandle _rv;
	Point inWhere;
	WindowPtr inWindow;
	SInt16 outPart;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetPoint, &inWhere,
	                      WinObj_Convert, &inWindow))
		return NULL;
	_rv = FindControlUnderMouse(inWhere,
	                            inWindow,
	                            &outPart);
	_res = Py_BuildValue("O&h",
	                     CtlObj_New, _rv,
	                     outPart);
	return _res;
}

static PyObject *Ctl_IdleControls(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	WindowPtr inWindow;
	if (!PyArg_ParseTuple(_args, "O&",
	                      WinObj_Convert, &inWindow))
		return NULL;
	IdleControls(inWindow);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Ctl_DumpControlHierarchy(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	WindowPtr inWindow;
	FSSpec inDumpFile;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      WinObj_Convert, &inWindow,
	                      PyMac_GetFSSpec, &inDumpFile))
		return NULL;
	_err = DumpControlHierarchy(inWindow,
	                            &inDumpFile);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Ctl_CreateRootControl(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	WindowPtr inWindow;
	ControlHandle outControl;
	if (!PyArg_ParseTuple(_args, "O&",
	                      WinObj_Convert, &inWindow))
		return NULL;
	_err = CreateRootControl(inWindow,
	                         &outControl);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     CtlObj_WhichControl, outControl);
	return _res;
}

static PyObject *Ctl_GetRootControl(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	WindowPtr inWindow;
	ControlHandle outControl;
	if (!PyArg_ParseTuple(_args, "O&",
	                      WinObj_Convert, &inWindow))
		return NULL;
	_err = GetRootControl(inWindow,
	                      &outControl);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     CtlObj_WhichControl, outControl);
	return _res;
}

static PyObject *Ctl_GetKeyboardFocus(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	WindowPtr inWindow;
	ControlHandle outControl;
	if (!PyArg_ParseTuple(_args, "O&",
	                      WinObj_Convert, &inWindow))
		return NULL;
	_err = GetKeyboardFocus(inWindow,
	                        &outControl);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     CtlObj_WhichControl, outControl);
	return _res;
}

static PyObject *Ctl_SetKeyboardFocus(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	WindowPtr inWindow;
	ControlHandle inControl;
	ControlFocusPart inPart;
	if (!PyArg_ParseTuple(_args, "O&O&h",
	                      WinObj_Convert, &inWindow,
	                      CtlObj_Convert, &inControl,
	                      &inPart))
		return NULL;
	_err = SetKeyboardFocus(inWindow,
	                        inControl,
	                        inPart);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Ctl_AdvanceKeyboardFocus(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	WindowPtr inWindow;
	if (!PyArg_ParseTuple(_args, "O&",
	                      WinObj_Convert, &inWindow))
		return NULL;
	_err = AdvanceKeyboardFocus(inWindow);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Ctl_ReverseKeyboardFocus(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	WindowPtr inWindow;
	if (!PyArg_ParseTuple(_args, "O&",
	                      WinObj_Convert, &inWindow))
		return NULL;
	_err = ReverseKeyboardFocus(inWindow);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Ctl_ClearKeyboardFocus(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	WindowPtr inWindow;
	if (!PyArg_ParseTuple(_args, "O&",
	                      WinObj_Convert, &inWindow))
		return NULL;
	_err = ClearKeyboardFocus(inWindow);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef Ctl_methods[] = {
	{"NewControl", (PyCFunction)Ctl_NewControl, 1,
	 "(WindowPtr owningWindow, Rect boundsRect, Str255 controlTitle, Boolean initiallyVisible, SInt16 initialValue, SInt16 minimumValue, SInt16 maximumValue, SInt16 procID, SInt32 controlReference) -> (ControlHandle _rv)"},
	{"GetNewControl", (PyCFunction)Ctl_GetNewControl, 1,
	 "(SInt16 resourceID, WindowPtr owningWindow) -> (ControlHandle _rv)"},
	{"DrawControls", (PyCFunction)Ctl_DrawControls, 1,
	 "(WindowPtr theWindow) -> None"},
	{"UpdateControls", (PyCFunction)Ctl_UpdateControls, 1,
	 "(WindowPtr theWindow, RgnHandle updateRegion) -> None"},
	{"FindControl", (PyCFunction)Ctl_FindControl, 1,
	 "(Point testPoint, WindowPtr theWindow) -> (ControlPartCode _rv, ControlHandle theControl)"},
	{"FindControlUnderMouse", (PyCFunction)Ctl_FindControlUnderMouse, 1,
	 "(Point inWhere, WindowPtr inWindow) -> (ControlHandle _rv, SInt16 outPart)"},
	{"IdleControls", (PyCFunction)Ctl_IdleControls, 1,
	 "(WindowPtr inWindow) -> None"},
	{"DumpControlHierarchy", (PyCFunction)Ctl_DumpControlHierarchy, 1,
	 "(WindowPtr inWindow, FSSpec inDumpFile) -> None"},
	{"CreateRootControl", (PyCFunction)Ctl_CreateRootControl, 1,
	 "(WindowPtr inWindow) -> (ControlHandle outControl)"},
	{"GetRootControl", (PyCFunction)Ctl_GetRootControl, 1,
	 "(WindowPtr inWindow) -> (ControlHandle outControl)"},
	{"GetKeyboardFocus", (PyCFunction)Ctl_GetKeyboardFocus, 1,
	 "(WindowPtr inWindow) -> (ControlHandle outControl)"},
	{"SetKeyboardFocus", (PyCFunction)Ctl_SetKeyboardFocus, 1,
	 "(WindowPtr inWindow, ControlHandle inControl, ControlFocusPart inPart) -> None"},
	{"AdvanceKeyboardFocus", (PyCFunction)Ctl_AdvanceKeyboardFocus, 1,
	 "(WindowPtr inWindow) -> None"},
	{"ReverseKeyboardFocus", (PyCFunction)Ctl_ReverseKeyboardFocus, 1,
	 "(WindowPtr inWindow) -> None"},
	{"ClearKeyboardFocus", (PyCFunction)Ctl_ClearKeyboardFocus, 1,
	 "(WindowPtr inWindow) -> None"},
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
		it = (PyObject *) GetControlReference(c);
	if (it == NULL || ((ControlObject *)it)->ob_itself != c)
		it = Py_None;
	Py_INCREF(it);
	return it;
}

static int
settrackfunc(obj)
	PyObject *obj;
{
	if (tracker) {
		PyErr_SetString(Ctl_Error, "Tracker function in use");
		return 0;
	}
	tracker = obj;
	Py_INCREF(tracker);
}

static void
clrtrackfunc()
{
	Py_XDECREF(tracker);
	tracker = 0;
}

static pascal void
mytracker(ctl, part)
	ControlHandle ctl;
	short part;
{
	PyObject *args, *rv=0;
	
	args = Py_BuildValue("(O&i)", CtlObj_WhichControl, ctl, (int)part);
	if (args && tracker) {
		rv = PyEval_CallObject(tracker, args);
		Py_DECREF(args);
	}
	if (rv)
		Py_DECREF(rv);
	else
		fprintf(stderr, "TrackControl: exception in tracker function\n");
}


void initCtl()
{
	PyObject *m;
	PyObject *d;



	mytracker_upp = NewControlActionProc(mytracker);


	m = Py_InitModule("Ctl", Ctl_methods);
	d = PyModule_GetDict(m);
	Ctl_Error = PyMac_GetOSErrException();
	if (Ctl_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Ctl_Error) != 0)
		Py_FatalError("can't initialize Ctl.Error");
	Control_Type.ob_type = &PyType_Type;
	Py_INCREF(&Control_Type);
	if (PyDict_SetItemString(d, "ControlType", (PyObject *)&Control_Type) != 0)
		Py_FatalError("can't initialize ControlType");
}

/* ========================= End module Ctl ========================= */

