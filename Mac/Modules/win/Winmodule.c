
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

#include <Windows.h>

extern PyObject *QdRGB_New(RGBColor *);
extern int QdRGB_Convert(PyObject *, RGBColor *);

#define resNotFound -192 /* Can't include <Errors.h> because of Python's "errors.h" */


static PyObject *Win_Error;

/* ----------------------- Object type Window ----------------------- */

PyTypeObject Window_Type;

#define WinObj_Check(x) ((x)->ob_type == &Window_Type)

typedef struct WindowObject {
	PyObject_HEAD
	WindowPtr ob_itself;
} WindowObject;

PyObject *WinObj_New(itself)
	WindowPtr itself;
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

static PyObject *WinObj_MacCloseWindow(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	MacCloseWindow(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_GetWindowOwnerCount(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	UInt32 outCount;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetWindowOwnerCount(_self->ob_itself,
	                           &outCount);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outCount);
	return _res;
}

static PyObject *WinObj_CloneWindow(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = CloneWindow(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_GetWindowClass(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowClass outClass;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetWindowClass(_self->ob_itself,
	                      &outClass);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outClass);
	return _res;
}

static PyObject *WinObj_GetWindowAttributes(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowAttributes outAttributes;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetWindowAttributes(_self->ob_itself,
	                           &outAttributes);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outAttributes);
	return _res;
}

static PyObject *WinObj_SetWinColor(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	WCTabHandle newColorTable;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &newColorTable))
		return NULL;
	SetWinColor(_self->ob_itself,
	            newColorTable);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_SetWindowContentColor(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	RGBColor color;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = SetWindowContentColor(_self->ob_itself,
	                             &color);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     QdRGB_New, &color);
	return _res;
}

static PyObject *WinObj_GetWindowContentColor(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	RGBColor color;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetWindowContentColor(_self->ob_itself,
	                             &color);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     QdRGB_New, &color);
	return _res;
}

static PyObject *WinObj_GetWindowContentPattern(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	PixPatHandle outPixPat;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &outPixPat))
		return NULL;
	_err = GetWindowContentPattern(_self->ob_itself,
	                               outPixPat);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_SetWindowContentPattern(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	PixPatHandle pixPat;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &pixPat))
		return NULL;
	_err = SetWindowContentPattern(_self->ob_itself,
	                               pixPat);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_ClipAbove(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	ClipAbove(_self->ob_itself);
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
	SaveOld(_self->ob_itself);
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
	DrawNew(_self->ob_itself,
	        update);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_PaintOne(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle clobberedRgn;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &clobberedRgn))
		return NULL;
	PaintOne(_self->ob_itself,
	         clobberedRgn);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_PaintBehind(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle clobberedRgn;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &clobberedRgn))
		return NULL;
	PaintBehind(_self->ob_itself,
	            clobberedRgn);
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
	CalcVis(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_CalcVisBehind(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle clobberedRgn;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &clobberedRgn))
		return NULL;
	CalcVisBehind(_self->ob_itself,
	              clobberedRgn);
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

static PyObject *WinObj_SetWindowPic(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	PicHandle pic;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &pic))
		return NULL;
	SetWindowPic(_self->ob_itself,
	             pic);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_GetWindowPic(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	PicHandle _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetWindowPic(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
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

static PyObject *WinObj_GetWindowFeatures(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	UInt32 outFeatures;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetWindowFeatures(_self->ob_itself,
	                         &outFeatures);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outFeatures);
	return _res;
}

static PyObject *WinObj_GetWindowRegion(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowRegionCode inRegionCode;
	RgnHandle ioWinRgn;
	if (!PyArg_ParseTuple(_args, "hO&",
	                      &inRegionCode,
	                      ResObj_Convert, &ioWinRgn))
		return NULL;
	_err = GetWindowRegion(_self->ob_itself,
	                       inRegionCode,
	                       ioWinRgn);
	if (_err != noErr) return PyMac_Error(_err);
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

static PyObject *WinObj_InvalWindowRgn(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	RgnHandle region;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &region))
		return NULL;
	_err = InvalWindowRgn(_self->ob_itself,
	                      region);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_InvalWindowRect(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Rect bounds;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &bounds))
		return NULL;
	_err = InvalWindowRect(_self->ob_itself,
	                       &bounds);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_ValidWindowRgn(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	RgnHandle region;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &region))
		return NULL;
	_err = ValidWindowRgn(_self->ob_itself,
	                      region);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_ValidWindowRect(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Rect bounds;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &bounds))
		return NULL;
	_err = ValidWindowRect(_self->ob_itself,
	                       &bounds);
	if (_err != noErr) return PyMac_Error(_err);
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

static PyObject *WinObj_SetWindowProxyFSSpec(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	FSSpec inFile;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetFSSpec, &inFile))
		return NULL;
	_err = SetWindowProxyFSSpec(_self->ob_itself,
	                            &inFile);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_GetWindowProxyFSSpec(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	FSSpec outFile;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetWindowProxyFSSpec(_self->ob_itself,
	                            &outFile);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFSSpec, &outFile);
	return _res;
}

static PyObject *WinObj_SetWindowProxyAlias(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	AliasHandle alias;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &alias))
		return NULL;
	_err = SetWindowProxyAlias(_self->ob_itself,
	                           alias);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_GetWindowProxyAlias(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	AliasHandle alias;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetWindowProxyAlias(_self->ob_itself,
	                           &alias);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, alias);
	return _res;
}

static PyObject *WinObj_SetWindowProxyCreatorAndType(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	OSType fileCreator;
	OSType fileType;
	SInt16 vRefNum;
	if (!PyArg_ParseTuple(_args, "O&O&h",
	                      PyMac_GetOSType, &fileCreator,
	                      PyMac_GetOSType, &fileType,
	                      &vRefNum))
		return NULL;
	_err = SetWindowProxyCreatorAndType(_self->ob_itself,
	                                    fileCreator,
	                                    fileType,
	                                    vRefNum);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_GetWindowProxyIcon(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	IconRef outIcon;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetWindowProxyIcon(_self->ob_itself,
	                          &outIcon);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, outIcon);
	return _res;
}

static PyObject *WinObj_SetWindowProxyIcon(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	IconRef icon;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &icon))
		return NULL;
	_err = SetWindowProxyIcon(_self->ob_itself,
	                          icon);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_RemoveWindowProxy(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = RemoveWindowProxy(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_TrackWindowProxyDrag(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Point startPt;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &startPt))
		return NULL;
	_err = TrackWindowProxyDrag(_self->ob_itself,
	                            startPt);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_IsWindowModified(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = IsWindowModified(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *WinObj_SetWindowModified(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Boolean modified;
	if (!PyArg_ParseTuple(_args, "b",
	                      &modified))
		return NULL;
	_err = SetWindowModified(_self->ob_itself,
	                         modified);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_IsWindowPathSelectClick(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	EventRecord event;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = IsWindowPathSelectClick(_self->ob_itself,
	                              &event);
	_res = Py_BuildValue("bO&",
	                     _rv,
	                     PyMac_BuildEventRecord, &event);
	return _res;
}

static PyObject *WinObj_HiliteWindowFrameForDrag(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Boolean hilited;
	if (!PyArg_ParseTuple(_args, "b",
	                      &hilited))
		return NULL;
	_err = HiliteWindowFrameForDrag(_self->ob_itself,
	                                hilited);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_TransitionWindow(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowTransitionEffect effect;
	WindowTransitionAction action;
	Rect rect;
	if (!PyArg_ParseTuple(_args, "llO&",
	                      &effect,
	                      &action,
	                      PyMac_GetRect, &rect))
		return NULL;
	_err = TransitionWindow(_self->ob_itself,
	                        effect,
	                        action,
	                        &rect);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_MacMoveWindow(_self, _args)
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
	MacMoveWindow(_self->ob_itself,
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

static PyObject *WinObj_IsWindowCollapsable(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = IsWindowCollapsable(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *WinObj_IsWindowCollapsed(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = IsWindowCollapsed(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *WinObj_CollapseWindow(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Boolean collapse;
	if (!PyArg_ParseTuple(_args, "b",
	                      &collapse))
		return NULL;
	_err = CollapseWindow(_self->ob_itself,
	                      collapse);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_RepositionWindow(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowPtr parentWindow;
	WindowPositionMethod method;
	if (!PyArg_ParseTuple(_args, "O&l",
	                      WinObj_Convert, &parentWindow,
	                      &method))
		return NULL;
	_err = RepositionWindow(_self->ob_itself,
	                        parentWindow,
	                        method);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_SetWindowBounds(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowRegionCode regionCode;
	Rect globalBounds;
	if (!PyArg_ParseTuple(_args, "hO&",
	                      &regionCode,
	                      PyMac_GetRect, &globalBounds))
		return NULL;
	_err = SetWindowBounds(_self->ob_itself,
	                       regionCode,
	                       &globalBounds);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_GetWindowBounds(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowRegionCode regionCode;
	Rect globalBounds;
	if (!PyArg_ParseTuple(_args, "h",
	                      &regionCode))
		return NULL;
	_err = GetWindowBounds(_self->ob_itself,
	                       regionCode,
	                       &globalBounds);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &globalBounds);
	return _res;
}

static PyObject *WinObj_MoveWindowStructure(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	short hGlobal;
	short vGlobal;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &hGlobal,
	                      &vGlobal))
		return NULL;
	_err = MoveWindowStructure(_self->ob_itself,
	                           hGlobal,
	                           vGlobal);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_IsWindowInStandardState(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	Point idealSize;
	Rect idealStandardState;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = IsWindowInStandardState(_self->ob_itself,
	                              &idealSize,
	                              &idealStandardState);
	_res = Py_BuildValue("bO&O&",
	                     _rv,
	                     PyMac_BuildPoint, idealSize,
	                     PyMac_BuildRect, &idealStandardState);
	return _res;
}

static PyObject *WinObj_ZoomWindowIdeal(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	SInt16 partCode;
	Point ioIdealSize;
	if (!PyArg_ParseTuple(_args, "h",
	                      &partCode))
		return NULL;
	_err = ZoomWindowIdeal(_self->ob_itself,
	                       partCode,
	                       &ioIdealSize);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildPoint, ioIdealSize);
	return _res;
}

static PyObject *WinObj_GetWindowIdealUserState(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Rect userState;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetWindowIdealUserState(_self->ob_itself,
	                               &userState);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &userState);
	return _res;
}

static PyObject *WinObj_SetWindowIdealUserState(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Rect userState;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = SetWindowIdealUserState(_self->ob_itself,
	                               &userState);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &userState);
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

static PyObject *WinObj_MacShowWindow(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	MacShowWindow(_self->ob_itself);
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

static PyObject *WinObj_GetAuxWin(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	AuxWinHandle awHndl;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetAuxWin(_self->ob_itself,
	                &awHndl);
	_res = Py_BuildValue("bO&",
	                     _rv,
	                     ResObj_New, awHndl);
	return _res;
}

static PyObject *WinObj_GetWindowPort(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	CGrafPtr _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetWindowPort(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     GrafObj_New, _rv);
	return _res;
}

static PyObject *WinObj_SetPortWindowPort(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	SetPortWindowPort(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_GetWindowKind(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetWindowKind(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *WinObj_SetWindowKind(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short wKind;
	if (!PyArg_ParseTuple(_args, "h",
	                      &wKind))
		return NULL;
	SetWindowKind(_self->ob_itself,
	              wKind);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_IsWindowVisible(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = IsWindowVisible(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *WinObj_IsWindowHilited(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = IsWindowHilited(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *WinObj_GetWindowGoAwayFlag(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetWindowGoAwayFlag(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *WinObj_GetWindowZoomFlag(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetWindowZoomFlag(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *WinObj_GetWindowStructureRgn(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle r;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &r))
		return NULL;
	GetWindowStructureRgn(_self->ob_itself,
	                      r);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_GetWindowContentRgn(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle r;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &r))
		return NULL;
	GetWindowContentRgn(_self->ob_itself,
	                    r);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_GetWindowUpdateRgn(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle r;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &r))
		return NULL;
	GetWindowUpdateRgn(_self->ob_itself,
	                   r);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_GetWindowTitleWidth(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetWindowTitleWidth(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *WinObj_GetNextWindow(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	WindowPtr _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetNextWindow(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     WinObj_WhichWindow, _rv);
	return _res;
}

static PyObject *WinObj_GetWindowStandardState(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetWindowStandardState(_self->ob_itself,
	                       &r);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &r);
	return _res;
}

static PyObject *WinObj_GetWindowUserState(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetWindowUserState(_self->ob_itself,
	                   &r);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &r);
	return _res;
}

static PyObject *WinObj_SetWindowStandardState(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &r))
		return NULL;
	SetWindowStandardState(_self->ob_itself,
	                       &r);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_SetWindowUserState(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &r))
		return NULL;
	SetWindowUserState(_self->ob_itself,
	                   &r);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_GetWindowDataHandle(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Handle _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetWindowDataHandle(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *WinObj_SetWindowDataHandle(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Handle data;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &data))
		return NULL;
	SetWindowDataHandle(_self->ob_itself,
	                    data);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_CloseWindow(_self, _args)
	WindowObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	CloseWindow(_self->ob_itself);
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

static PyMethodDef WinObj_methods[] = {
	{"MacCloseWindow", (PyCFunction)WinObj_MacCloseWindow, 1,
	 "() -> None"},
	{"GetWindowOwnerCount", (PyCFunction)WinObj_GetWindowOwnerCount, 1,
	 "() -> (UInt32 outCount)"},
	{"CloneWindow", (PyCFunction)WinObj_CloneWindow, 1,
	 "() -> None"},
	{"GetWindowClass", (PyCFunction)WinObj_GetWindowClass, 1,
	 "() -> (WindowClass outClass)"},
	{"GetWindowAttributes", (PyCFunction)WinObj_GetWindowAttributes, 1,
	 "() -> (WindowAttributes outAttributes)"},
	{"SetWinColor", (PyCFunction)WinObj_SetWinColor, 1,
	 "(WCTabHandle newColorTable) -> None"},
	{"SetWindowContentColor", (PyCFunction)WinObj_SetWindowContentColor, 1,
	 "() -> (RGBColor color)"},
	{"GetWindowContentColor", (PyCFunction)WinObj_GetWindowContentColor, 1,
	 "() -> (RGBColor color)"},
	{"GetWindowContentPattern", (PyCFunction)WinObj_GetWindowContentPattern, 1,
	 "(PixPatHandle outPixPat) -> None"},
	{"SetWindowContentPattern", (PyCFunction)WinObj_SetWindowContentPattern, 1,
	 "(PixPatHandle pixPat) -> None"},
	{"ClipAbove", (PyCFunction)WinObj_ClipAbove, 1,
	 "() -> None"},
	{"SaveOld", (PyCFunction)WinObj_SaveOld, 1,
	 "() -> None"},
	{"DrawNew", (PyCFunction)WinObj_DrawNew, 1,
	 "(Boolean update) -> None"},
	{"PaintOne", (PyCFunction)WinObj_PaintOne, 1,
	 "(RgnHandle clobberedRgn) -> None"},
	{"PaintBehind", (PyCFunction)WinObj_PaintBehind, 1,
	 "(RgnHandle clobberedRgn) -> None"},
	{"CalcVis", (PyCFunction)WinObj_CalcVis, 1,
	 "() -> None"},
	{"CalcVisBehind", (PyCFunction)WinObj_CalcVisBehind, 1,
	 "(RgnHandle clobberedRgn) -> None"},
	{"BringToFront", (PyCFunction)WinObj_BringToFront, 1,
	 "() -> None"},
	{"SendBehind", (PyCFunction)WinObj_SendBehind, 1,
	 "(WindowPtr behindWindow) -> None"},
	{"SelectWindow", (PyCFunction)WinObj_SelectWindow, 1,
	 "() -> None"},
	{"HiliteWindow", (PyCFunction)WinObj_HiliteWindow, 1,
	 "(Boolean fHilite) -> None"},
	{"SetWRefCon", (PyCFunction)WinObj_SetWRefCon, 1,
	 "(long data) -> None"},
	{"GetWRefCon", (PyCFunction)WinObj_GetWRefCon, 1,
	 "() -> (long _rv)"},
	{"SetWindowPic", (PyCFunction)WinObj_SetWindowPic, 1,
	 "(PicHandle pic) -> None"},
	{"GetWindowPic", (PyCFunction)WinObj_GetWindowPic, 1,
	 "() -> (PicHandle _rv)"},
	{"GetWVariant", (PyCFunction)WinObj_GetWVariant, 1,
	 "() -> (short _rv)"},
	{"GetWindowFeatures", (PyCFunction)WinObj_GetWindowFeatures, 1,
	 "() -> (UInt32 outFeatures)"},
	{"GetWindowRegion", (PyCFunction)WinObj_GetWindowRegion, 1,
	 "(WindowRegionCode inRegionCode, RgnHandle ioWinRgn) -> None"},
	{"BeginUpdate", (PyCFunction)WinObj_BeginUpdate, 1,
	 "() -> None"},
	{"EndUpdate", (PyCFunction)WinObj_EndUpdate, 1,
	 "() -> None"},
	{"InvalWindowRgn", (PyCFunction)WinObj_InvalWindowRgn, 1,
	 "(RgnHandle region) -> None"},
	{"InvalWindowRect", (PyCFunction)WinObj_InvalWindowRect, 1,
	 "(Rect bounds) -> None"},
	{"ValidWindowRgn", (PyCFunction)WinObj_ValidWindowRgn, 1,
	 "(RgnHandle region) -> None"},
	{"ValidWindowRect", (PyCFunction)WinObj_ValidWindowRect, 1,
	 "(Rect bounds) -> None"},
	{"DrawGrowIcon", (PyCFunction)WinObj_DrawGrowIcon, 1,
	 "() -> None"},
	{"SetWTitle", (PyCFunction)WinObj_SetWTitle, 1,
	 "(Str255 title) -> None"},
	{"GetWTitle", (PyCFunction)WinObj_GetWTitle, 1,
	 "() -> (Str255 title)"},
	{"SetWindowProxyFSSpec", (PyCFunction)WinObj_SetWindowProxyFSSpec, 1,
	 "(FSSpec inFile) -> None"},
	{"GetWindowProxyFSSpec", (PyCFunction)WinObj_GetWindowProxyFSSpec, 1,
	 "() -> (FSSpec outFile)"},
	{"SetWindowProxyAlias", (PyCFunction)WinObj_SetWindowProxyAlias, 1,
	 "(AliasHandle alias) -> None"},
	{"GetWindowProxyAlias", (PyCFunction)WinObj_GetWindowProxyAlias, 1,
	 "() -> (AliasHandle alias)"},
	{"SetWindowProxyCreatorAndType", (PyCFunction)WinObj_SetWindowProxyCreatorAndType, 1,
	 "(OSType fileCreator, OSType fileType, SInt16 vRefNum) -> None"},
	{"GetWindowProxyIcon", (PyCFunction)WinObj_GetWindowProxyIcon, 1,
	 "() -> (IconRef outIcon)"},
	{"SetWindowProxyIcon", (PyCFunction)WinObj_SetWindowProxyIcon, 1,
	 "(IconRef icon) -> None"},
	{"RemoveWindowProxy", (PyCFunction)WinObj_RemoveWindowProxy, 1,
	 "() -> None"},
	{"TrackWindowProxyDrag", (PyCFunction)WinObj_TrackWindowProxyDrag, 1,
	 "(Point startPt) -> None"},
	{"IsWindowModified", (PyCFunction)WinObj_IsWindowModified, 1,
	 "() -> (Boolean _rv)"},
	{"SetWindowModified", (PyCFunction)WinObj_SetWindowModified, 1,
	 "(Boolean modified) -> None"},
	{"IsWindowPathSelectClick", (PyCFunction)WinObj_IsWindowPathSelectClick, 1,
	 "() -> (Boolean _rv, EventRecord event)"},
	{"HiliteWindowFrameForDrag", (PyCFunction)WinObj_HiliteWindowFrameForDrag, 1,
	 "(Boolean hilited) -> None"},
	{"TransitionWindow", (PyCFunction)WinObj_TransitionWindow, 1,
	 "(WindowTransitionEffect effect, WindowTransitionAction action, Rect rect) -> None"},
	{"MacMoveWindow", (PyCFunction)WinObj_MacMoveWindow, 1,
	 "(short hGlobal, short vGlobal, Boolean front) -> None"},
	{"SizeWindow", (PyCFunction)WinObj_SizeWindow, 1,
	 "(short w, short h, Boolean fUpdate) -> None"},
	{"GrowWindow", (PyCFunction)WinObj_GrowWindow, 1,
	 "(Point startPt, Rect bBox) -> (long _rv)"},
	{"DragWindow", (PyCFunction)WinObj_DragWindow, 1,
	 "(Point startPt, Rect boundsRect) -> None"},
	{"ZoomWindow", (PyCFunction)WinObj_ZoomWindow, 1,
	 "(short partCode, Boolean front) -> None"},
	{"IsWindowCollapsable", (PyCFunction)WinObj_IsWindowCollapsable, 1,
	 "() -> (Boolean _rv)"},
	{"IsWindowCollapsed", (PyCFunction)WinObj_IsWindowCollapsed, 1,
	 "() -> (Boolean _rv)"},
	{"CollapseWindow", (PyCFunction)WinObj_CollapseWindow, 1,
	 "(Boolean collapse) -> None"},
	{"RepositionWindow", (PyCFunction)WinObj_RepositionWindow, 1,
	 "(WindowPtr parentWindow, WindowPositionMethod method) -> None"},
	{"SetWindowBounds", (PyCFunction)WinObj_SetWindowBounds, 1,
	 "(WindowRegionCode regionCode, Rect globalBounds) -> None"},
	{"GetWindowBounds", (PyCFunction)WinObj_GetWindowBounds, 1,
	 "(WindowRegionCode regionCode) -> (Rect globalBounds)"},
	{"MoveWindowStructure", (PyCFunction)WinObj_MoveWindowStructure, 1,
	 "(short hGlobal, short vGlobal) -> None"},
	{"IsWindowInStandardState", (PyCFunction)WinObj_IsWindowInStandardState, 1,
	 "() -> (Boolean _rv, Point idealSize, Rect idealStandardState)"},
	{"ZoomWindowIdeal", (PyCFunction)WinObj_ZoomWindowIdeal, 1,
	 "(SInt16 partCode) -> (Point ioIdealSize)"},
	{"GetWindowIdealUserState", (PyCFunction)WinObj_GetWindowIdealUserState, 1,
	 "() -> (Rect userState)"},
	{"SetWindowIdealUserState", (PyCFunction)WinObj_SetWindowIdealUserState, 1,
	 "() -> (Rect userState)"},
	{"HideWindow", (PyCFunction)WinObj_HideWindow, 1,
	 "() -> None"},
	{"MacShowWindow", (PyCFunction)WinObj_MacShowWindow, 1,
	 "() -> None"},
	{"ShowHide", (PyCFunction)WinObj_ShowHide, 1,
	 "(Boolean showFlag) -> None"},
	{"TrackBox", (PyCFunction)WinObj_TrackBox, 1,
	 "(Point thePt, short partCode) -> (Boolean _rv)"},
	{"TrackGoAway", (PyCFunction)WinObj_TrackGoAway, 1,
	 "(Point thePt) -> (Boolean _rv)"},
	{"GetAuxWin", (PyCFunction)WinObj_GetAuxWin, 1,
	 "() -> (Boolean _rv, AuxWinHandle awHndl)"},
	{"GetWindowPort", (PyCFunction)WinObj_GetWindowPort, 1,
	 "() -> (CGrafPtr _rv)"},
	{"SetPortWindowPort", (PyCFunction)WinObj_SetPortWindowPort, 1,
	 "() -> None"},
	{"GetWindowKind", (PyCFunction)WinObj_GetWindowKind, 1,
	 "() -> (short _rv)"},
	{"SetWindowKind", (PyCFunction)WinObj_SetWindowKind, 1,
	 "(short wKind) -> None"},
	{"IsWindowVisible", (PyCFunction)WinObj_IsWindowVisible, 1,
	 "() -> (Boolean _rv)"},
	{"IsWindowHilited", (PyCFunction)WinObj_IsWindowHilited, 1,
	 "() -> (Boolean _rv)"},
	{"GetWindowGoAwayFlag", (PyCFunction)WinObj_GetWindowGoAwayFlag, 1,
	 "() -> (Boolean _rv)"},
	{"GetWindowZoomFlag", (PyCFunction)WinObj_GetWindowZoomFlag, 1,
	 "() -> (Boolean _rv)"},
	{"GetWindowStructureRgn", (PyCFunction)WinObj_GetWindowStructureRgn, 1,
	 "(RgnHandle r) -> None"},
	{"GetWindowContentRgn", (PyCFunction)WinObj_GetWindowContentRgn, 1,
	 "(RgnHandle r) -> None"},
	{"GetWindowUpdateRgn", (PyCFunction)WinObj_GetWindowUpdateRgn, 1,
	 "(RgnHandle r) -> None"},
	{"GetWindowTitleWidth", (PyCFunction)WinObj_GetWindowTitleWidth, 1,
	 "() -> (short _rv)"},
	{"GetNextWindow", (PyCFunction)WinObj_GetNextWindow, 1,
	 "() -> (WindowPtr _rv)"},
	{"GetWindowStandardState", (PyCFunction)WinObj_GetWindowStandardState, 1,
	 "() -> (Rect r)"},
	{"GetWindowUserState", (PyCFunction)WinObj_GetWindowUserState, 1,
	 "() -> (Rect r)"},
	{"SetWindowStandardState", (PyCFunction)WinObj_SetWindowStandardState, 1,
	 "(Rect r) -> None"},
	{"SetWindowUserState", (PyCFunction)WinObj_SetWindowUserState, 1,
	 "(Rect r) -> None"},
	{"GetWindowDataHandle", (PyCFunction)WinObj_GetWindowDataHandle, 1,
	 "() -> (Handle _rv)"},
	{"SetWindowDataHandle", (PyCFunction)WinObj_SetWindowDataHandle, 1,
	 "(Handle data) -> None"},
	{"CloseWindow", (PyCFunction)WinObj_CloseWindow, 1,
	 "() -> None"},
	{"MoveWindow", (PyCFunction)WinObj_MoveWindow, 1,
	 "(short hGlobal, short vGlobal, Boolean front) -> None"},
	{"ShowWindow", (PyCFunction)WinObj_ShowWindow, 1,
	 "() -> None"},
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

#define WinObj_compare NULL

#define WinObj_repr NULL

#define WinObj_hash NULL

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
	(cmpfunc) WinObj_compare, /*tp_compare*/
	(reprfunc) WinObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) WinObj_hash, /*tp_hash*/
};

/* --------------------- End object type Window --------------------- */


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

static PyObject *Win_CreateNewWindow(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowClass windowClass;
	WindowAttributes attributes;
	Rect bounds;
	WindowPtr outWindow;
	if (!PyArg_ParseTuple(_args, "llO&",
	                      &windowClass,
	                      &attributes,
	                      PyMac_GetRect, &bounds))
		return NULL;
	_err = CreateNewWindow(windowClass,
	                       attributes,
	                       &bounds,
	                       &outWindow);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     WinObj_WhichWindow, outWindow);
	return _res;
}

static PyObject *Win_CreateWindowFromResource(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	SInt16 resID;
	WindowPtr outWindow;
	if (!PyArg_ParseTuple(_args, "h",
	                      &resID))
		return NULL;
	_err = CreateWindowFromResource(resID,
	                                &outWindow);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     WinObj_WhichWindow, outWindow);
	return _res;
}

static PyObject *Win_ShowFloatingWindows(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = ShowFloatingWindows();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Win_HideFloatingWindows(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = HideFloatingWindows();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Win_AreFloatingWindowsVisible(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = AreFloatingWindowsVisible();
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Win_FrontNonFloatingWindow(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	WindowPtr _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = FrontNonFloatingWindow();
	_res = Py_BuildValue("O&",
	                     WinObj_New, _rv);
	return _res;
}

static PyObject *Win_SetDeskCPat(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	PixPatHandle deskPixPat;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &deskPixPat))
		return NULL;
	SetDeskCPat(deskPixPat);
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

static PyObject *Win_MacFindWindow(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	Point thePoint;
	WindowPtr window;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &thePoint))
		return NULL;
	_rv = MacFindWindow(thePoint,
	                    &window);
	_res = Py_BuildValue("hO&",
	                     _rv,
	                     WinObj_WhichWindow, window);
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
	                     WinObj_WhichWindow, _rv);
	return _res;
}

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

static PyObject *Win_GetWMgrPort(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	GrafPtr wPort;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetWMgrPort(&wPort);
	_res = Py_BuildValue("O&",
	                     GrafObj_New, wPort);
	return _res;
}

static PyObject *Win_GetCWMgrPort(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	CGrafPtr wMgrCPort;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetCWMgrPort(&wMgrCPort);
	_res = Py_BuildValue("O&",
	                     GrafObj_New, wMgrCPort);
	return _res;
}

static PyObject *Win_IsValidWindowPtr(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	GrafPtr grafPort;
	if (!PyArg_ParseTuple(_args, "O&",
	                      GrafObj_Convert, &grafPort))
		return NULL;
	_rv = IsValidWindowPtr(grafPort);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Win_InitFloatingWindows(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = InitFloatingWindows();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
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

static PyObject *Win_InvalRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle badRgn;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &badRgn))
		return NULL;
	InvalRgn(badRgn);
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

static PyObject *Win_ValidRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle goodRgn;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &goodRgn))
		return NULL;
	ValidRgn(goodRgn);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Win_CollapseAllWindows(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Boolean collapse;
	if (!PyArg_ParseTuple(_args, "b",
	                      &collapse))
		return NULL;
	_err = CollapseAllWindows(collapse);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
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

static PyObject *Win_GetGrayRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetGrayRgn();
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *Win_WhichWindow(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;

	long ptr;

	if ( !PyArg_ParseTuple(_args, "i", &ptr) )
		return NULL;
	return WinObj_WhichWindow((WindowPtr)ptr);

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

static PyMethodDef Win_methods[] = {
	{"GetNewCWindow", (PyCFunction)Win_GetNewCWindow, 1,
	 "(short windowID, WindowPtr behind) -> (WindowPtr _rv)"},
	{"NewWindow", (PyCFunction)Win_NewWindow, 1,
	 "(Rect boundsRect, Str255 title, Boolean visible, short theProc, WindowPtr behind, Boolean goAwayFlag, long refCon) -> (WindowPtr _rv)"},
	{"GetNewWindow", (PyCFunction)Win_GetNewWindow, 1,
	 "(short windowID, WindowPtr behind) -> (WindowPtr _rv)"},
	{"NewCWindow", (PyCFunction)Win_NewCWindow, 1,
	 "(Rect boundsRect, Str255 title, Boolean visible, short procID, WindowPtr behind, Boolean goAwayFlag, long refCon) -> (WindowPtr _rv)"},
	{"CreateNewWindow", (PyCFunction)Win_CreateNewWindow, 1,
	 "(WindowClass windowClass, WindowAttributes attributes, Rect bounds) -> (WindowPtr outWindow)"},
	{"CreateWindowFromResource", (PyCFunction)Win_CreateWindowFromResource, 1,
	 "(SInt16 resID) -> (WindowPtr outWindow)"},
	{"ShowFloatingWindows", (PyCFunction)Win_ShowFloatingWindows, 1,
	 "() -> None"},
	{"HideFloatingWindows", (PyCFunction)Win_HideFloatingWindows, 1,
	 "() -> None"},
	{"AreFloatingWindowsVisible", (PyCFunction)Win_AreFloatingWindowsVisible, 1,
	 "() -> (Boolean _rv)"},
	{"FrontNonFloatingWindow", (PyCFunction)Win_FrontNonFloatingWindow, 1,
	 "() -> (WindowPtr _rv)"},
	{"SetDeskCPat", (PyCFunction)Win_SetDeskCPat, 1,
	 "(PixPatHandle deskPixPat) -> None"},
	{"CheckUpdate", (PyCFunction)Win_CheckUpdate, 1,
	 "() -> (Boolean _rv, EventRecord theEvent)"},
	{"MacFindWindow", (PyCFunction)Win_MacFindWindow, 1,
	 "(Point thePoint) -> (short _rv, WindowPtr window)"},
	{"FrontWindow", (PyCFunction)Win_FrontWindow, 1,
	 "() -> (WindowPtr _rv)"},
	{"InitWindows", (PyCFunction)Win_InitWindows, 1,
	 "() -> None"},
	{"GetWMgrPort", (PyCFunction)Win_GetWMgrPort, 1,
	 "() -> (GrafPtr wPort)"},
	{"GetCWMgrPort", (PyCFunction)Win_GetCWMgrPort, 1,
	 "() -> (CGrafPtr wMgrCPort)"},
	{"IsValidWindowPtr", (PyCFunction)Win_IsValidWindowPtr, 1,
	 "(GrafPtr grafPort) -> (Boolean _rv)"},
	{"InitFloatingWindows", (PyCFunction)Win_InitFloatingWindows, 1,
	 "() -> None"},
	{"InvalRect", (PyCFunction)Win_InvalRect, 1,
	 "(Rect badRect) -> None"},
	{"InvalRgn", (PyCFunction)Win_InvalRgn, 1,
	 "(RgnHandle badRgn) -> None"},
	{"ValidRect", (PyCFunction)Win_ValidRect, 1,
	 "(Rect goodRect) -> None"},
	{"ValidRgn", (PyCFunction)Win_ValidRgn, 1,
	 "(RgnHandle goodRgn) -> None"},
	{"CollapseAllWindows", (PyCFunction)Win_CollapseAllWindows, 1,
	 "(Boolean collapse) -> None"},
	{"PinRect", (PyCFunction)Win_PinRect, 1,
	 "(Rect theRect, Point thePt) -> (long _rv)"},
	{"GetGrayRgn", (PyCFunction)Win_GetGrayRgn, 1,
	 "() -> (RgnHandle _rv)"},
	{"WhichWindow", (PyCFunction)Win_WhichWindow, 1,
	 "Resolve an integer WindowPtr address to a Window object"},
	{"FindWindow", (PyCFunction)Win_FindWindow, 1,
	 "(Point thePoint) -> (short _rv, WindowPtr theWindow)"},
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
	if (w == NULL)
		it = NULL;
	else
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
	Window_Type.ob_type = &PyType_Type;
	Py_INCREF(&Window_Type);
	if (PyDict_SetItemString(d, "WindowType", (PyObject *)&Window_Type) != 0)
		Py_FatalError("can't initialize WindowType");
}

/* ========================= End module Win ========================= */

