
/* ========================== Module _Win =========================== */

#include "Python.h"



#ifdef _WIN32
#include "pywintoolbox.h"
#else
#include "macglue.h"
#include "pymactoolbox.h"
#endif

/* Macro to test whether a weak-loaded CFM function exists */
#define PyMac_PRECHECK(rtn) do { if ( &rtn == NULL )  {\
    	PyErr_SetString(PyExc_NotImplementedError, \
    	"Not available in this shared library/OS version"); \
    	return NULL; \
    }} while(0)


#ifdef WITHOUT_FRAMEWORKS
#include <Windows.h>
#else
#include <Carbon/Carbon.h>
#endif

#ifdef USE_TOOLBOX_OBJECT_GLUE
extern PyObject *_WinObj_New(WindowRef);
extern PyObject *_WinObj_WhichWindow(WindowRef);
extern int _WinObj_Convert(PyObject *, WindowRef *);

#define WinObj_New _WinObj_New
#define WinObj_WhichWindow _WinObj_WhichWindow
#define WinObj_Convert _WinObj_Convert
#endif

#if !ACCESSOR_CALLS_ARE_FUNCTIONS  && UNIVERSAL_INTERFACES_VERSION < 0x340
/* Carbon calls that we emulate in classic mode */
#define GetWindowSpareFlag(win) (((CWindowPeek)(win))->spareFlag)
#define GetWindowFromPort(port) ((WindowRef)(port))
#define GetWindowPortBounds(win, rectp) (*(rectp) = ((CWindowPeek)(win))->port.portRect)
#endif
#if !ACCESSOR_CALLS_ARE_FUNCTIONS
#define IsPointerValid(p) (((long)p&3) == 0)
#endif
#if ACCESSOR_CALLS_ARE_FUNCTIONS
/* Classic calls that we emulate in carbon mode */
#define GetWindowUpdateRgn(win, rgn) GetWindowRegion((win), kWindowUpdateRgn, (rgn))
#define GetWindowStructureRgn(win, rgn) GetWindowRegion((win), kWindowStructureRgn, (rgn))
#define GetWindowContentRgn(win, rgn) GetWindowRegion((win), kWindowContentRgn, (rgn))
#endif

/* Function to dispose a window, with a "normal" calling sequence */
static void
PyMac_AutoDisposeWindow(WindowPtr w)
{
	DisposeWindow(w);
}

static PyObject *Win_Error;

/* ----------------------- Object type Window ----------------------- */

PyTypeObject Window_Type;

#define WinObj_Check(x) ((x)->ob_type == &Window_Type)

typedef struct WindowObject {
	PyObject_HEAD
	WindowPtr ob_itself;
	void (*ob_freeit)(WindowPtr ptr);
} WindowObject;

PyObject *WinObj_New(WindowPtr itself)
{
	WindowObject *it;
	if (itself == NULL) return PyMac_Error(resNotFound);
	it = PyObject_NEW(WindowObject, &Window_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	it->ob_freeit = NULL;
	if (GetWRefCon(itself) == 0)
	{
		SetWRefCon(itself, (long)it);
		it->ob_freeit = PyMac_AutoDisposeWindow;
	}
	return (PyObject *)it;
}
int WinObj_Convert(PyObject *v, WindowPtr *p_itself)
{

	if (v == Py_None) { *p_itself = NULL; return 1; }
	if (PyInt_Check(v)) { *p_itself = (WindowPtr)PyInt_AsLong(v); return 1; }

	{
		DialogRef dlg;
		if (DlgObj_Convert(v, &dlg) && dlg) {
			*p_itself = GetDialogWindow(dlg);
			return 1;
		}
		PyErr_Clear();
	}
	if (!WinObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "Window required");
		return 0;
	}
	*p_itself = ((WindowObject *)v)->ob_itself;
	return 1;
}

static void WinObj_dealloc(WindowObject *self)
{
	if (self->ob_freeit && self->ob_itself)
	{
		SetWRefCon(self->ob_itself, 0);
		self->ob_freeit(self->ob_itself);
	}
	self->ob_itself = NULL;
	self->ob_freeit = NULL;
	PyObject_Del(self);
}

static PyObject *WinObj_GetWindowOwnerCount(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	UInt32 outCount;
#ifndef GetWindowOwnerCount
	PyMac_PRECHECK(GetWindowOwnerCount);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetWindowOwnerCount(_self->ob_itself,
	                           &outCount);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outCount);
	return _res;
}

static PyObject *WinObj_CloneWindow(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
#ifndef CloneWindow
	PyMac_PRECHECK(CloneWindow);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = CloneWindow(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

#if !TARGET_API_MAC_OS8

static PyObject *WinObj_GetWindowRetainCount(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ItemCount _rv;
#ifndef GetWindowRetainCount
	PyMac_PRECHECK(GetWindowRetainCount);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetWindowRetainCount(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}
#endif

#if !TARGET_API_MAC_OS8

static PyObject *WinObj_RetainWindow(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
#ifndef RetainWindow
	PyMac_PRECHECK(RetainWindow);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = RetainWindow(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

#if !TARGET_API_MAC_OS8

static PyObject *WinObj_ReleaseWindow(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
#ifndef ReleaseWindow
	PyMac_PRECHECK(ReleaseWindow);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = ReleaseWindow(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

#if !TARGET_API_MAC_OS8

static PyObject *WinObj_ReshapeCustomWindow(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
#ifndef ReshapeCustomWindow
	PyMac_PRECHECK(ReshapeCustomWindow);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = ReshapeCustomWindow(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

static PyObject *WinObj_GetWindowWidgetHilite(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowDefPartCode outHilite;
#ifndef GetWindowWidgetHilite
	PyMac_PRECHECK(GetWindowWidgetHilite);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetWindowWidgetHilite(_self->ob_itself,
	                             &outHilite);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     outHilite);
	return _res;
}

static PyObject *WinObj_GetWindowClass(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowClass outClass;
#ifndef GetWindowClass
	PyMac_PRECHECK(GetWindowClass);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetWindowClass(_self->ob_itself,
	                      &outClass);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outClass);
	return _res;
}

static PyObject *WinObj_GetWindowAttributes(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowAttributes outAttributes;
#ifndef GetWindowAttributes
	PyMac_PRECHECK(GetWindowAttributes);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetWindowAttributes(_self->ob_itself,
	                           &outAttributes);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outAttributes);
	return _res;
}

#if !TARGET_API_MAC_OS8

static PyObject *WinObj_ChangeWindowAttributes(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowAttributes setTheseAttributes;
	WindowAttributes clearTheseAttributes;
#ifndef ChangeWindowAttributes
	PyMac_PRECHECK(ChangeWindowAttributes);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &setTheseAttributes,
	                      &clearTheseAttributes))
		return NULL;
	_err = ChangeWindowAttributes(_self->ob_itself,
	                              setTheseAttributes,
	                              clearTheseAttributes);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

#if !TARGET_API_MAC_OS8

static PyObject *WinObj_SetWindowClass(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowClass inWindowClass;
#ifndef SetWindowClass
	PyMac_PRECHECK(SetWindowClass);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &inWindowClass))
		return NULL;
	_err = SetWindowClass(_self->ob_itself,
	                      inWindowClass);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

#if !TARGET_API_MAC_OS8

static PyObject *WinObj_SetWindowModality(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowModality inModalKind;
	WindowPtr inUnavailableWindow;
#ifndef SetWindowModality
	PyMac_PRECHECK(SetWindowModality);
#endif
	if (!PyArg_ParseTuple(_args, "lO&",
	                      &inModalKind,
	                      WinObj_Convert, &inUnavailableWindow))
		return NULL;
	_err = SetWindowModality(_self->ob_itself,
	                         inModalKind,
	                         inUnavailableWindow);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

#if !TARGET_API_MAC_OS8

static PyObject *WinObj_GetWindowModality(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowModality outModalKind;
	WindowPtr outUnavailableWindow;
#ifndef GetWindowModality
	PyMac_PRECHECK(GetWindowModality);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetWindowModality(_self->ob_itself,
	                         &outModalKind,
	                         &outUnavailableWindow);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("lO&",
	                     outModalKind,
	                     WinObj_WhichWindow, outUnavailableWindow);
	return _res;
}
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *WinObj_SetWinColor(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	WCTabHandle newColorTable;
#ifndef SetWinColor
	PyMac_PRECHECK(SetWinColor);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &newColorTable))
		return NULL;
	SetWinColor(_self->ob_itself,
	            newColorTable);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

static PyObject *WinObj_SetWindowContentColor(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	RGBColor color;
#ifndef SetWindowContentColor
	PyMac_PRECHECK(SetWindowContentColor);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      QdRGB_Convert, &color))
		return NULL;
	_err = SetWindowContentColor(_self->ob_itself,
	                             &color);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_GetWindowContentColor(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	RGBColor color;
#ifndef GetWindowContentColor
	PyMac_PRECHECK(GetWindowContentColor);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetWindowContentColor(_self->ob_itself,
	                             &color);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     QdRGB_New, &color);
	return _res;
}

static PyObject *WinObj_GetWindowContentPattern(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	PixPatHandle outPixPat;
#ifndef GetWindowContentPattern
	PyMac_PRECHECK(GetWindowContentPattern);
#endif
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

static PyObject *WinObj_SetWindowContentPattern(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	PixPatHandle pixPat;
#ifndef SetWindowContentPattern
	PyMac_PRECHECK(SetWindowContentPattern);
#endif
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

#if !TARGET_API_MAC_OS8

static PyObject *WinObj_ScrollWindowRect(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	Rect inScrollRect;
	SInt16 inHPixels;
	SInt16 inVPixels;
	ScrollWindowOptions inOptions;
	RgnHandle outExposedRgn;
#ifndef ScrollWindowRect
	PyMac_PRECHECK(ScrollWindowRect);
#endif
	if (!PyArg_ParseTuple(_args, "O&hhlO&",
	                      PyMac_GetRect, &inScrollRect,
	                      &inHPixels,
	                      &inVPixels,
	                      &inOptions,
	                      ResObj_Convert, &outExposedRgn))
		return NULL;
	_err = ScrollWindowRect(_self->ob_itself,
	                        &inScrollRect,
	                        inHPixels,
	                        inVPixels,
	                        inOptions,
	                        outExposedRgn);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

#if !TARGET_API_MAC_OS8

static PyObject *WinObj_ScrollWindowRegion(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	RgnHandle inScrollRgn;
	SInt16 inHPixels;
	SInt16 inVPixels;
	ScrollWindowOptions inOptions;
	RgnHandle outExposedRgn;
#ifndef ScrollWindowRegion
	PyMac_PRECHECK(ScrollWindowRegion);
#endif
	if (!PyArg_ParseTuple(_args, "O&hhlO&",
	                      ResObj_Convert, &inScrollRgn,
	                      &inHPixels,
	                      &inVPixels,
	                      &inOptions,
	                      ResObj_Convert, &outExposedRgn))
		return NULL;
	_err = ScrollWindowRegion(_self->ob_itself,
	                          inScrollRgn,
	                          inHPixels,
	                          inVPixels,
	                          inOptions,
	                          outExposedRgn);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

static PyObject *WinObj_ClipAbove(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef ClipAbove
	PyMac_PRECHECK(ClipAbove);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	ClipAbove(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

#if !TARGET_API_MAC_CARBON

static PyObject *WinObj_SaveOld(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef SaveOld
	PyMac_PRECHECK(SaveOld);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	SaveOld(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *WinObj_DrawNew(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean update;
#ifndef DrawNew
	PyMac_PRECHECK(DrawNew);
#endif
	if (!PyArg_ParseTuple(_args, "b",
	                      &update))
		return NULL;
	DrawNew(_self->ob_itself,
	        update);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

static PyObject *WinObj_PaintOne(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	RgnHandle clobberedRgn;
#ifndef PaintOne
	PyMac_PRECHECK(PaintOne);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &clobberedRgn))
		return NULL;
	PaintOne(_self->ob_itself,
	         clobberedRgn);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_PaintBehind(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	RgnHandle clobberedRgn;
#ifndef PaintBehind
	PyMac_PRECHECK(PaintBehind);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &clobberedRgn))
		return NULL;
	PaintBehind(_self->ob_itself,
	            clobberedRgn);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_CalcVis(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef CalcVis
	PyMac_PRECHECK(CalcVis);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	CalcVis(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_CalcVisBehind(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	RgnHandle clobberedRgn;
#ifndef CalcVisBehind
	PyMac_PRECHECK(CalcVisBehind);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &clobberedRgn))
		return NULL;
	CalcVisBehind(_self->ob_itself,
	              clobberedRgn);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_BringToFront(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef BringToFront
	PyMac_PRECHECK(BringToFront);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	BringToFront(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_SendBehind(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	WindowPtr behindWindow;
#ifndef SendBehind
	PyMac_PRECHECK(SendBehind);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      WinObj_Convert, &behindWindow))
		return NULL;
	SendBehind(_self->ob_itself,
	           behindWindow);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_SelectWindow(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef SelectWindow
	PyMac_PRECHECK(SelectWindow);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	SelectWindow(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

#if !TARGET_API_MAC_OS8

static PyObject *WinObj_GetNextWindowOfClass(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	WindowPtr _rv;
	WindowClass inWindowClass;
	Boolean mustBeVisible;
#ifndef GetNextWindowOfClass
	PyMac_PRECHECK(GetNextWindowOfClass);
#endif
	if (!PyArg_ParseTuple(_args, "lb",
	                      &inWindowClass,
	                      &mustBeVisible))
		return NULL;
	_rv = GetNextWindowOfClass(_self->ob_itself,
	                           inWindowClass,
	                           mustBeVisible);
	_res = Py_BuildValue("O&",
	                     WinObj_New, _rv);
	return _res;
}
#endif

#if !TARGET_API_MAC_OS8

static PyObject *WinObj_SetWindowAlternateTitle(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	CFStringRef inTitle;
#ifndef SetWindowAlternateTitle
	PyMac_PRECHECK(SetWindowAlternateTitle);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CFStringRefObj_Convert, &inTitle))
		return NULL;
	_err = SetWindowAlternateTitle(_self->ob_itself,
	                               inTitle);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

#if !TARGET_API_MAC_OS8

static PyObject *WinObj_CopyWindowAlternateTitle(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	CFStringRef outTitle;
#ifndef CopyWindowAlternateTitle
	PyMac_PRECHECK(CopyWindowAlternateTitle);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = CopyWindowAlternateTitle(_self->ob_itself,
	                                &outTitle);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     CFStringRefObj_New, outTitle);
	return _res;
}
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *WinObj_IsValidWindowPtr(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
#ifndef IsValidWindowPtr
	PyMac_PRECHECK(IsValidWindowPtr);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = IsValidWindowPtr(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}
#endif

static PyObject *WinObj_HiliteWindow(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean fHilite;
#ifndef HiliteWindow
	PyMac_PRECHECK(HiliteWindow);
#endif
	if (!PyArg_ParseTuple(_args, "b",
	                      &fHilite))
		return NULL;
	HiliteWindow(_self->ob_itself,
	             fHilite);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_SetWRefCon(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long data;
#ifndef SetWRefCon
	PyMac_PRECHECK(SetWRefCon);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &data))
		return NULL;
	SetWRefCon(_self->ob_itself,
	           data);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_GetWRefCon(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
#ifndef GetWRefCon
	PyMac_PRECHECK(GetWRefCon);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetWRefCon(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *WinObj_SetWindowPic(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PicHandle pic;
#ifndef SetWindowPic
	PyMac_PRECHECK(SetWindowPic);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &pic))
		return NULL;
	SetWindowPic(_self->ob_itself,
	             pic);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_GetWindowPic(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PicHandle _rv;
#ifndef GetWindowPic
	PyMac_PRECHECK(GetWindowPic);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetWindowPic(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *WinObj_GetWVariant(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short _rv;
#ifndef GetWVariant
	PyMac_PRECHECK(GetWVariant);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetWVariant(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *WinObj_GetWindowFeatures(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	UInt32 outFeatures;
#ifndef GetWindowFeatures
	PyMac_PRECHECK(GetWindowFeatures);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetWindowFeatures(_self->ob_itself,
	                         &outFeatures);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outFeatures);
	return _res;
}

static PyObject *WinObj_GetWindowRegion(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowRegionCode inRegionCode;
	RgnHandle ioWinRgn;
#ifndef GetWindowRegion
	PyMac_PRECHECK(GetWindowRegion);
#endif
	if (!PyArg_ParseTuple(_args, "HO&",
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

static PyObject *WinObj_GetWindowStructureWidths(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	Rect outRect;
#ifndef GetWindowStructureWidths
	PyMac_PRECHECK(GetWindowStructureWidths);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetWindowStructureWidths(_self->ob_itself,
	                                &outRect);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &outRect);
	return _res;
}

static PyObject *WinObj_BeginUpdate(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef BeginUpdate
	PyMac_PRECHECK(BeginUpdate);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	BeginUpdate(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_EndUpdate(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef EndUpdate
	PyMac_PRECHECK(EndUpdate);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	EndUpdate(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_InvalWindowRgn(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	RgnHandle region;
#ifndef InvalWindowRgn
	PyMac_PRECHECK(InvalWindowRgn);
#endif
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

static PyObject *WinObj_InvalWindowRect(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	Rect bounds;
#ifndef InvalWindowRect
	PyMac_PRECHECK(InvalWindowRect);
#endif
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

static PyObject *WinObj_ValidWindowRgn(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	RgnHandle region;
#ifndef ValidWindowRgn
	PyMac_PRECHECK(ValidWindowRgn);
#endif
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

static PyObject *WinObj_ValidWindowRect(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	Rect bounds;
#ifndef ValidWindowRect
	PyMac_PRECHECK(ValidWindowRect);
#endif
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

static PyObject *WinObj_DrawGrowIcon(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef DrawGrowIcon
	PyMac_PRECHECK(DrawGrowIcon);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	DrawGrowIcon(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_SetWTitle(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Str255 title;
#ifndef SetWTitle
	PyMac_PRECHECK(SetWTitle);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetStr255, title))
		return NULL;
	SetWTitle(_self->ob_itself,
	          title);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_GetWTitle(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Str255 title;
#ifndef GetWTitle
	PyMac_PRECHECK(GetWTitle);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetWTitle(_self->ob_itself,
	          title);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildStr255, title);
	return _res;
}

#if !TARGET_API_MAC_OS8

static PyObject *WinObj_SetWindowTitleWithCFString(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	CFStringRef inString;
#ifndef SetWindowTitleWithCFString
	PyMac_PRECHECK(SetWindowTitleWithCFString);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CFStringRefObj_Convert, &inString))
		return NULL;
	_err = SetWindowTitleWithCFString(_self->ob_itself,
	                                  inString);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

#if !TARGET_API_MAC_OS8

static PyObject *WinObj_CopyWindowTitleAsCFString(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	CFStringRef outString;
#ifndef CopyWindowTitleAsCFString
	PyMac_PRECHECK(CopyWindowTitleAsCFString);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = CopyWindowTitleAsCFString(_self->ob_itself,
	                                 &outString);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     CFStringRefObj_New, outString);
	return _res;
}
#endif

static PyObject *WinObj_SetWindowProxyFSSpec(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	FSSpec inFile;
#ifndef SetWindowProxyFSSpec
	PyMac_PRECHECK(SetWindowProxyFSSpec);
#endif
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

static PyObject *WinObj_GetWindowProxyFSSpec(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	FSSpec outFile;
#ifndef GetWindowProxyFSSpec
	PyMac_PRECHECK(GetWindowProxyFSSpec);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetWindowProxyFSSpec(_self->ob_itself,
	                            &outFile);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildFSSpec, &outFile);
	return _res;
}

static PyObject *WinObj_SetWindowProxyAlias(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	AliasHandle alias;
#ifndef SetWindowProxyAlias
	PyMac_PRECHECK(SetWindowProxyAlias);
#endif
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

static PyObject *WinObj_GetWindowProxyAlias(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	AliasHandle alias;
#ifndef GetWindowProxyAlias
	PyMac_PRECHECK(GetWindowProxyAlias);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetWindowProxyAlias(_self->ob_itself,
	                           &alias);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, alias);
	return _res;
}

static PyObject *WinObj_SetWindowProxyCreatorAndType(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	OSType fileCreator;
	OSType fileType;
	SInt16 vRefNum;
#ifndef SetWindowProxyCreatorAndType
	PyMac_PRECHECK(SetWindowProxyCreatorAndType);
#endif
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

static PyObject *WinObj_GetWindowProxyIcon(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	IconRef outIcon;
#ifndef GetWindowProxyIcon
	PyMac_PRECHECK(GetWindowProxyIcon);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetWindowProxyIcon(_self->ob_itself,
	                          &outIcon);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, outIcon);
	return _res;
}

static PyObject *WinObj_SetWindowProxyIcon(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	IconRef icon;
#ifndef SetWindowProxyIcon
	PyMac_PRECHECK(SetWindowProxyIcon);
#endif
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

static PyObject *WinObj_RemoveWindowProxy(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
#ifndef RemoveWindowProxy
	PyMac_PRECHECK(RemoveWindowProxy);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = RemoveWindowProxy(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_BeginWindowProxyDrag(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	DragReference outNewDrag;
	RgnHandle outDragOutlineRgn;
#ifndef BeginWindowProxyDrag
	PyMac_PRECHECK(BeginWindowProxyDrag);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &outDragOutlineRgn))
		return NULL;
	_err = BeginWindowProxyDrag(_self->ob_itself,
	                            &outNewDrag,
	                            outDragOutlineRgn);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     DragObj_New, outNewDrag);
	return _res;
}

static PyObject *WinObj_EndWindowProxyDrag(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	DragReference theDrag;
#ifndef EndWindowProxyDrag
	PyMac_PRECHECK(EndWindowProxyDrag);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      DragObj_Convert, &theDrag))
		return NULL;
	_err = EndWindowProxyDrag(_self->ob_itself,
	                          theDrag);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_TrackWindowProxyFromExistingDrag(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	Point startPt;
	DragReference drag;
	RgnHandle inDragOutlineRgn;
#ifndef TrackWindowProxyFromExistingDrag
	PyMac_PRECHECK(TrackWindowProxyFromExistingDrag);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      PyMac_GetPoint, &startPt,
	                      DragObj_Convert, &drag,
	                      ResObj_Convert, &inDragOutlineRgn))
		return NULL;
	_err = TrackWindowProxyFromExistingDrag(_self->ob_itself,
	                                        startPt,
	                                        drag,
	                                        inDragOutlineRgn);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_TrackWindowProxyDrag(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	Point startPt;
#ifndef TrackWindowProxyDrag
	PyMac_PRECHECK(TrackWindowProxyDrag);
#endif
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

static PyObject *WinObj_IsWindowModified(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
#ifndef IsWindowModified
	PyMac_PRECHECK(IsWindowModified);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = IsWindowModified(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *WinObj_SetWindowModified(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	Boolean modified;
#ifndef SetWindowModified
	PyMac_PRECHECK(SetWindowModified);
#endif
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

static PyObject *WinObj_IsWindowPathSelectClick(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	EventRecord event;
#ifndef IsWindowPathSelectClick
	PyMac_PRECHECK(IsWindowPathSelectClick);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetEventRecord, &event))
		return NULL;
	_rv = IsWindowPathSelectClick(_self->ob_itself,
	                              &event);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *WinObj_WindowPathSelect(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuHandle menu;
	SInt32 outMenuResult;
#ifndef WindowPathSelect
	PyMac_PRECHECK(WindowPathSelect);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      MenuObj_Convert, &menu))
		return NULL;
	_err = WindowPathSelect(_self->ob_itself,
	                        menu,
	                        &outMenuResult);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outMenuResult);
	return _res;
}

static PyObject *WinObj_HiliteWindowFrameForDrag(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	Boolean hilited;
#ifndef HiliteWindowFrameForDrag
	PyMac_PRECHECK(HiliteWindowFrameForDrag);
#endif
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

static PyObject *WinObj_TransitionWindow(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowTransitionEffect effect;
	WindowTransitionAction action;
	Rect rect;
#ifndef TransitionWindow
	PyMac_PRECHECK(TransitionWindow);
#endif
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

#if TARGET_API_MAC_OSX

static PyObject *WinObj_TransitionWindowAndParent(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowPtr parentWindow;
	WindowTransitionEffect effect;
	WindowTransitionAction action;
	Rect rect;
#ifndef TransitionWindowAndParent
	PyMac_PRECHECK(TransitionWindowAndParent);
#endif
	if (!PyArg_ParseTuple(_args, "O&llO&",
	                      WinObj_Convert, &parentWindow,
	                      &effect,
	                      &action,
	                      PyMac_GetRect, &rect))
		return NULL;
	_err = TransitionWindowAndParent(_self->ob_itself,
	                                 parentWindow,
	                                 effect,
	                                 action,
	                                 &rect);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

static PyObject *WinObj_MacMoveWindow(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short hGlobal;
	short vGlobal;
	Boolean front;
#ifndef MacMoveWindow
	PyMac_PRECHECK(MacMoveWindow);
#endif
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

static PyObject *WinObj_SizeWindow(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short w;
	short h;
	Boolean fUpdate;
#ifndef SizeWindow
	PyMac_PRECHECK(SizeWindow);
#endif
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

static PyObject *WinObj_GrowWindow(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
	Point startPt;
	Rect bBox;
#ifndef GrowWindow
	PyMac_PRECHECK(GrowWindow);
#endif
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

static PyObject *WinObj_DragWindow(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Point startPt;
	Rect boundsRect;
#ifndef DragWindow
	PyMac_PRECHECK(DragWindow);
#endif
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

static PyObject *WinObj_ZoomWindow(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	WindowPartCode partCode;
	Boolean front;
#ifndef ZoomWindow
	PyMac_PRECHECK(ZoomWindow);
#endif
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

static PyObject *WinObj_IsWindowCollapsable(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
#ifndef IsWindowCollapsable
	PyMac_PRECHECK(IsWindowCollapsable);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = IsWindowCollapsable(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *WinObj_IsWindowCollapsed(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
#ifndef IsWindowCollapsed
	PyMac_PRECHECK(IsWindowCollapsed);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = IsWindowCollapsed(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *WinObj_CollapseWindow(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	Boolean collapse;
#ifndef CollapseWindow
	PyMac_PRECHECK(CollapseWindow);
#endif
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

static PyObject *WinObj_GetWindowBounds(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowRegionCode regionCode;
	Rect globalBounds;
#ifndef GetWindowBounds
	PyMac_PRECHECK(GetWindowBounds);
#endif
	if (!PyArg_ParseTuple(_args, "H",
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

static PyObject *WinObj_ResizeWindow(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	Point startPoint;
	Rect sizeConstraints;
	Rect newContentRect;
#ifndef ResizeWindow
	PyMac_PRECHECK(ResizeWindow);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetPoint, &startPoint,
	                      PyMac_GetRect, &sizeConstraints))
		return NULL;
	_rv = ResizeWindow(_self->ob_itself,
	                   startPoint,
	                   &sizeConstraints,
	                   &newContentRect);
	_res = Py_BuildValue("bO&",
	                     _rv,
	                     PyMac_BuildRect, &newContentRect);
	return _res;
}

static PyObject *WinObj_SetWindowBounds(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowRegionCode regionCode;
	Rect globalBounds;
#ifndef SetWindowBounds
	PyMac_PRECHECK(SetWindowBounds);
#endif
	if (!PyArg_ParseTuple(_args, "HO&",
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

static PyObject *WinObj_RepositionWindow(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowPtr parentWindow;
	WindowPositionMethod method;
#ifndef RepositionWindow
	PyMac_PRECHECK(RepositionWindow);
#endif
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

static PyObject *WinObj_MoveWindowStructure(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	short hGlobal;
	short vGlobal;
#ifndef MoveWindowStructure
	PyMac_PRECHECK(MoveWindowStructure);
#endif
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

static PyObject *WinObj_IsWindowInStandardState(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	Point idealSize;
	Rect idealStandardState;
#ifndef IsWindowInStandardState
	PyMac_PRECHECK(IsWindowInStandardState);
#endif
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

static PyObject *WinObj_ZoomWindowIdeal(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowPartCode partCode;
	Point ioIdealSize;
#ifndef ZoomWindowIdeal
	PyMac_PRECHECK(ZoomWindowIdeal);
#endif
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

static PyObject *WinObj_GetWindowIdealUserState(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	Rect userState;
#ifndef GetWindowIdealUserState
	PyMac_PRECHECK(GetWindowIdealUserState);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetWindowIdealUserState(_self->ob_itself,
	                               &userState);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &userState);
	return _res;
}

static PyObject *WinObj_SetWindowIdealUserState(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	Rect userState;
#ifndef SetWindowIdealUserState
	PyMac_PRECHECK(SetWindowIdealUserState);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &userState))
		return NULL;
	_err = SetWindowIdealUserState(_self->ob_itself,
	                               &userState);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

#if !TARGET_API_MAC_OS8

static PyObject *WinObj_GetWindowGreatestAreaDevice(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowRegionCode inRegion;
	GDHandle outGreatestDevice;
	Rect outGreatestDeviceRect;
#ifndef GetWindowGreatestAreaDevice
	PyMac_PRECHECK(GetWindowGreatestAreaDevice);
#endif
	if (!PyArg_ParseTuple(_args, "H",
	                      &inRegion))
		return NULL;
	_err = GetWindowGreatestAreaDevice(_self->ob_itself,
	                                   inRegion,
	                                   &outGreatestDevice,
	                                   &outGreatestDeviceRect);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&O&",
	                     ResObj_New, outGreatestDevice,
	                     PyMac_BuildRect, &outGreatestDeviceRect);
	return _res;
}
#endif

#if !TARGET_API_MAC_OS8

static PyObject *WinObj_ConstrainWindowToScreen(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowRegionCode inRegionCode;
	WindowConstrainOptions inOptions;
	Rect inScreenRect;
	Rect outStructure;
#ifndef ConstrainWindowToScreen
	PyMac_PRECHECK(ConstrainWindowToScreen);
#endif
	if (!PyArg_ParseTuple(_args, "HlO&",
	                      &inRegionCode,
	                      &inOptions,
	                      PyMac_GetRect, &inScreenRect))
		return NULL;
	_err = ConstrainWindowToScreen(_self->ob_itself,
	                               inRegionCode,
	                               inOptions,
	                               &inScreenRect,
	                               &outStructure);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &outStructure);
	return _res;
}
#endif

static PyObject *WinObj_HideWindow(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef HideWindow
	PyMac_PRECHECK(HideWindow);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	HideWindow(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_MacShowWindow(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef MacShowWindow
	PyMac_PRECHECK(MacShowWindow);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	MacShowWindow(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_ShowHide(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean showFlag;
#ifndef ShowHide
	PyMac_PRECHECK(ShowHide);
#endif
	if (!PyArg_ParseTuple(_args, "b",
	                      &showFlag))
		return NULL;
	ShowHide(_self->ob_itself,
	         showFlag);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_MacIsWindowVisible(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
#ifndef MacIsWindowVisible
	PyMac_PRECHECK(MacIsWindowVisible);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = MacIsWindowVisible(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

#if !TARGET_API_MAC_OS8

static PyObject *WinObj_ShowSheetWindow(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowPtr inParentWindow;
#ifndef ShowSheetWindow
	PyMac_PRECHECK(ShowSheetWindow);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      WinObj_Convert, &inParentWindow))
		return NULL;
	_err = ShowSheetWindow(_self->ob_itself,
	                       inParentWindow);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

#if !TARGET_API_MAC_OS8

static PyObject *WinObj_HideSheetWindow(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
#ifndef HideSheetWindow
	PyMac_PRECHECK(HideSheetWindow);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = HideSheetWindow(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

#if !TARGET_API_MAC_OS8

static PyObject *WinObj_GetSheetWindowParent(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowPtr outParentWindow;
#ifndef GetSheetWindowParent
	PyMac_PRECHECK(GetSheetWindowParent);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetSheetWindowParent(_self->ob_itself,
	                            &outParentWindow);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     WinObj_WhichWindow, outParentWindow);
	return _res;
}
#endif

#if !TARGET_API_MAC_OS8

static PyObject *WinObj_GetWindowPropertyAttributes(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	OSType propertyCreator;
	OSType propertyTag;
	UInt32 attributes;
#ifndef GetWindowPropertyAttributes
	PyMac_PRECHECK(GetWindowPropertyAttributes);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetOSType, &propertyCreator,
	                      PyMac_GetOSType, &propertyTag))
		return NULL;
	_err = GetWindowPropertyAttributes(_self->ob_itself,
	                                   propertyCreator,
	                                   propertyTag,
	                                   &attributes);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     attributes);
	return _res;
}
#endif

#if !TARGET_API_MAC_OS8

static PyObject *WinObj_ChangeWindowPropertyAttributes(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	OSType propertyCreator;
	OSType propertyTag;
	UInt32 attributesToSet;
	UInt32 attributesToClear;
#ifndef ChangeWindowPropertyAttributes
	PyMac_PRECHECK(ChangeWindowPropertyAttributes);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&ll",
	                      PyMac_GetOSType, &propertyCreator,
	                      PyMac_GetOSType, &propertyTag,
	                      &attributesToSet,
	                      &attributesToClear))
		return NULL;
	_err = ChangeWindowPropertyAttributes(_self->ob_itself,
	                                      propertyCreator,
	                                      propertyTag,
	                                      attributesToSet,
	                                      attributesToClear);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

static PyObject *WinObj_TrackBox(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	Point thePt;
	WindowPartCode partCode;
#ifndef TrackBox
	PyMac_PRECHECK(TrackBox);
#endif
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

static PyObject *WinObj_TrackGoAway(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	Point thePt;
#ifndef TrackGoAway
	PyMac_PRECHECK(TrackGoAway);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &thePt))
		return NULL;
	_rv = TrackGoAway(_self->ob_itself,
	                  thePt);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

#if !TARGET_API_MAC_CARBON

static PyObject *WinObj_GetAuxWin(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	AuxWinHandle awHndl;
#ifndef GetAuxWin
	PyMac_PRECHECK(GetAuxWin);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetAuxWin(_self->ob_itself,
	                &awHndl);
	_res = Py_BuildValue("bO&",
	                     _rv,
	                     ResObj_New, awHndl);
	return _res;
}
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *WinObj_GetWindowGoAwayFlag(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
#ifndef GetWindowGoAwayFlag
	PyMac_PRECHECK(GetWindowGoAwayFlag);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetWindowGoAwayFlag(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *WinObj_GetWindowSpareFlag(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
#ifndef GetWindowSpareFlag
	PyMac_PRECHECK(GetWindowSpareFlag);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetWindowSpareFlag(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}
#endif

static PyObject *WinObj_GetWindowPort(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CGrafPtr _rv;
#ifndef GetWindowPort
	PyMac_PRECHECK(GetWindowPort);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetWindowPort(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     GrafObj_New, _rv);
	return _res;
}

static PyObject *WinObj_GetWindowKind(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short _rv;
#ifndef GetWindowKind
	PyMac_PRECHECK(GetWindowKind);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetWindowKind(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *WinObj_IsWindowHilited(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
#ifndef IsWindowHilited
	PyMac_PRECHECK(IsWindowHilited);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = IsWindowHilited(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

#if !TARGET_API_MAC_OS8

static PyObject *WinObj_IsWindowUpdatePending(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
#ifndef IsWindowUpdatePending
	PyMac_PRECHECK(IsWindowUpdatePending);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = IsWindowUpdatePending(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}
#endif

static PyObject *WinObj_MacGetNextWindow(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	WindowPtr _rv;
#ifndef MacGetNextWindow
	PyMac_PRECHECK(MacGetNextWindow);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = MacGetNextWindow(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     WinObj_New, _rv);
	return _res;
}

static PyObject *WinObj_GetWindowStandardState(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Rect rect;
#ifndef GetWindowStandardState
	PyMac_PRECHECK(GetWindowStandardState);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetWindowStandardState(_self->ob_itself,
	                       &rect);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &rect);
	return _res;
}

static PyObject *WinObj_GetWindowUserState(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Rect rect;
#ifndef GetWindowUserState
	PyMac_PRECHECK(GetWindowUserState);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetWindowUserState(_self->ob_itself,
	                   &rect);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &rect);
	return _res;
}

static PyObject *WinObj_SetWindowKind(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short kind;
#ifndef SetWindowKind
	PyMac_PRECHECK(SetWindowKind);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &kind))
		return NULL;
	SetWindowKind(_self->ob_itself,
	              kind);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_SetWindowStandardState(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Rect rect;
#ifndef SetWindowStandardState
	PyMac_PRECHECK(SetWindowStandardState);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &rect))
		return NULL;
	SetWindowStandardState(_self->ob_itself,
	                       &rect);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_SetWindowUserState(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Rect rect;
#ifndef SetWindowUserState
	PyMac_PRECHECK(SetWindowUserState);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &rect))
		return NULL;
	SetWindowUserState(_self->ob_itself,
	                   &rect);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_SetPortWindowPort(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef SetPortWindowPort
	PyMac_PRECHECK(SetPortWindowPort);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	SetPortWindowPort(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_GetWindowPortBounds(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Rect bounds;
#ifndef GetWindowPortBounds
	PyMac_PRECHECK(GetWindowPortBounds);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetWindowPortBounds(_self->ob_itself,
	                    &bounds);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &bounds);
	return _res;
}

static PyObject *WinObj_IsWindowVisible(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
#ifndef IsWindowVisible
	PyMac_PRECHECK(IsWindowVisible);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = IsWindowVisible(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

#if !TARGET_API_MAC_CARBON

static PyObject *WinObj_GetWindowZoomFlag(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
#ifndef GetWindowZoomFlag
	PyMac_PRECHECK(GetWindowZoomFlag);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetWindowZoomFlag(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}
#endif

static PyObject *WinObj_GetWindowStructureRgn(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	RgnHandle r;
#ifndef GetWindowStructureRgn
	PyMac_PRECHECK(GetWindowStructureRgn);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &r))
		return NULL;
	GetWindowStructureRgn(_self->ob_itself,
	                      r);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_GetWindowContentRgn(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	RgnHandle r;
#ifndef GetWindowContentRgn
	PyMac_PRECHECK(GetWindowContentRgn);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &r))
		return NULL;
	GetWindowContentRgn(_self->ob_itself,
	                    r);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WinObj_GetWindowUpdateRgn(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	RgnHandle r;
#ifndef GetWindowUpdateRgn
	PyMac_PRECHECK(GetWindowUpdateRgn);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &r))
		return NULL;
	GetWindowUpdateRgn(_self->ob_itself,
	                   r);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

#if !TARGET_API_MAC_CARBON

static PyObject *WinObj_GetWindowTitleWidth(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short _rv;
#ifndef GetWindowTitleWidth
	PyMac_PRECHECK(GetWindowTitleWidth);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetWindowTitleWidth(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}
#endif

static PyObject *WinObj_GetNextWindow(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	WindowPtr _rv;
#ifndef GetNextWindow
	PyMac_PRECHECK(GetNextWindow);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetNextWindow(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     WinObj_WhichWindow, _rv);
	return _res;
}

#if !TARGET_API_MAC_CARBON

static PyObject *WinObj_CloseWindow(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef CloseWindow
	PyMac_PRECHECK(CloseWindow);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	CloseWindow(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

static PyObject *WinObj_MoveWindow(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short hGlobal;
	short vGlobal;
	Boolean front;
#ifndef MoveWindow
	PyMac_PRECHECK(MoveWindow);
#endif
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

static PyObject *WinObj_ShowWindow(WindowObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef ShowWindow
	PyMac_PRECHECK(ShowWindow);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	ShowWindow(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef WinObj_methods[] = {
	{"GetWindowOwnerCount", (PyCFunction)WinObj_GetWindowOwnerCount, 1,
	 "() -> (UInt32 outCount)"},
	{"CloneWindow", (PyCFunction)WinObj_CloneWindow, 1,
	 "() -> None"},

#if !TARGET_API_MAC_OS8
	{"GetWindowRetainCount", (PyCFunction)WinObj_GetWindowRetainCount, 1,
	 "() -> (ItemCount _rv)"},
#endif

#if !TARGET_API_MAC_OS8
	{"RetainWindow", (PyCFunction)WinObj_RetainWindow, 1,
	 "() -> None"},
#endif

#if !TARGET_API_MAC_OS8
	{"ReleaseWindow", (PyCFunction)WinObj_ReleaseWindow, 1,
	 "() -> None"},
#endif

#if !TARGET_API_MAC_OS8
	{"ReshapeCustomWindow", (PyCFunction)WinObj_ReshapeCustomWindow, 1,
	 "() -> None"},
#endif
	{"GetWindowWidgetHilite", (PyCFunction)WinObj_GetWindowWidgetHilite, 1,
	 "() -> (WindowDefPartCode outHilite)"},
	{"GetWindowClass", (PyCFunction)WinObj_GetWindowClass, 1,
	 "() -> (WindowClass outClass)"},
	{"GetWindowAttributes", (PyCFunction)WinObj_GetWindowAttributes, 1,
	 "() -> (WindowAttributes outAttributes)"},

#if !TARGET_API_MAC_OS8
	{"ChangeWindowAttributes", (PyCFunction)WinObj_ChangeWindowAttributes, 1,
	 "(WindowAttributes setTheseAttributes, WindowAttributes clearTheseAttributes) -> None"},
#endif

#if !TARGET_API_MAC_OS8
	{"SetWindowClass", (PyCFunction)WinObj_SetWindowClass, 1,
	 "(WindowClass inWindowClass) -> None"},
#endif

#if !TARGET_API_MAC_OS8
	{"SetWindowModality", (PyCFunction)WinObj_SetWindowModality, 1,
	 "(WindowModality inModalKind, WindowPtr inUnavailableWindow) -> None"},
#endif

#if !TARGET_API_MAC_OS8
	{"GetWindowModality", (PyCFunction)WinObj_GetWindowModality, 1,
	 "() -> (WindowModality outModalKind, WindowPtr outUnavailableWindow)"},
#endif

#if !TARGET_API_MAC_CARBON
	{"SetWinColor", (PyCFunction)WinObj_SetWinColor, 1,
	 "(WCTabHandle newColorTable) -> None"},
#endif
	{"SetWindowContentColor", (PyCFunction)WinObj_SetWindowContentColor, 1,
	 "(RGBColor color) -> None"},
	{"GetWindowContentColor", (PyCFunction)WinObj_GetWindowContentColor, 1,
	 "() -> (RGBColor color)"},
	{"GetWindowContentPattern", (PyCFunction)WinObj_GetWindowContentPattern, 1,
	 "(PixPatHandle outPixPat) -> None"},
	{"SetWindowContentPattern", (PyCFunction)WinObj_SetWindowContentPattern, 1,
	 "(PixPatHandle pixPat) -> None"},

#if !TARGET_API_MAC_OS8
	{"ScrollWindowRect", (PyCFunction)WinObj_ScrollWindowRect, 1,
	 "(Rect inScrollRect, SInt16 inHPixels, SInt16 inVPixels, ScrollWindowOptions inOptions, RgnHandle outExposedRgn) -> None"},
#endif

#if !TARGET_API_MAC_OS8
	{"ScrollWindowRegion", (PyCFunction)WinObj_ScrollWindowRegion, 1,
	 "(RgnHandle inScrollRgn, SInt16 inHPixels, SInt16 inVPixels, ScrollWindowOptions inOptions, RgnHandle outExposedRgn) -> None"},
#endif
	{"ClipAbove", (PyCFunction)WinObj_ClipAbove, 1,
	 "() -> None"},

#if !TARGET_API_MAC_CARBON
	{"SaveOld", (PyCFunction)WinObj_SaveOld, 1,
	 "() -> None"},
#endif

#if !TARGET_API_MAC_CARBON
	{"DrawNew", (PyCFunction)WinObj_DrawNew, 1,
	 "(Boolean update) -> None"},
#endif
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

#if !TARGET_API_MAC_OS8
	{"GetNextWindowOfClass", (PyCFunction)WinObj_GetNextWindowOfClass, 1,
	 "(WindowClass inWindowClass, Boolean mustBeVisible) -> (WindowPtr _rv)"},
#endif

#if !TARGET_API_MAC_OS8
	{"SetWindowAlternateTitle", (PyCFunction)WinObj_SetWindowAlternateTitle, 1,
	 "(CFStringRef inTitle) -> None"},
#endif

#if !TARGET_API_MAC_OS8
	{"CopyWindowAlternateTitle", (PyCFunction)WinObj_CopyWindowAlternateTitle, 1,
	 "() -> (CFStringRef outTitle)"},
#endif

#if !TARGET_API_MAC_CARBON
	{"IsValidWindowPtr", (PyCFunction)WinObj_IsValidWindowPtr, 1,
	 "() -> (Boolean _rv)"},
#endif
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
	{"GetWindowStructureWidths", (PyCFunction)WinObj_GetWindowStructureWidths, 1,
	 "() -> (Rect outRect)"},
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

#if !TARGET_API_MAC_OS8
	{"SetWindowTitleWithCFString", (PyCFunction)WinObj_SetWindowTitleWithCFString, 1,
	 "(CFStringRef inString) -> None"},
#endif

#if !TARGET_API_MAC_OS8
	{"CopyWindowTitleAsCFString", (PyCFunction)WinObj_CopyWindowTitleAsCFString, 1,
	 "() -> (CFStringRef outString)"},
#endif
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
	{"BeginWindowProxyDrag", (PyCFunction)WinObj_BeginWindowProxyDrag, 1,
	 "(RgnHandle outDragOutlineRgn) -> (DragReference outNewDrag)"},
	{"EndWindowProxyDrag", (PyCFunction)WinObj_EndWindowProxyDrag, 1,
	 "(DragReference theDrag) -> None"},
	{"TrackWindowProxyFromExistingDrag", (PyCFunction)WinObj_TrackWindowProxyFromExistingDrag, 1,
	 "(Point startPt, DragReference drag, RgnHandle inDragOutlineRgn) -> None"},
	{"TrackWindowProxyDrag", (PyCFunction)WinObj_TrackWindowProxyDrag, 1,
	 "(Point startPt) -> None"},
	{"IsWindowModified", (PyCFunction)WinObj_IsWindowModified, 1,
	 "() -> (Boolean _rv)"},
	{"SetWindowModified", (PyCFunction)WinObj_SetWindowModified, 1,
	 "(Boolean modified) -> None"},
	{"IsWindowPathSelectClick", (PyCFunction)WinObj_IsWindowPathSelectClick, 1,
	 "(EventRecord event) -> (Boolean _rv)"},
	{"WindowPathSelect", (PyCFunction)WinObj_WindowPathSelect, 1,
	 "(MenuHandle menu) -> (SInt32 outMenuResult)"},
	{"HiliteWindowFrameForDrag", (PyCFunction)WinObj_HiliteWindowFrameForDrag, 1,
	 "(Boolean hilited) -> None"},
	{"TransitionWindow", (PyCFunction)WinObj_TransitionWindow, 1,
	 "(WindowTransitionEffect effect, WindowTransitionAction action, Rect rect) -> None"},

#if TARGET_API_MAC_OSX
	{"TransitionWindowAndParent", (PyCFunction)WinObj_TransitionWindowAndParent, 1,
	 "(WindowPtr parentWindow, WindowTransitionEffect effect, WindowTransitionAction action, Rect rect) -> None"},
#endif
	{"MacMoveWindow", (PyCFunction)WinObj_MacMoveWindow, 1,
	 "(short hGlobal, short vGlobal, Boolean front) -> None"},
	{"SizeWindow", (PyCFunction)WinObj_SizeWindow, 1,
	 "(short w, short h, Boolean fUpdate) -> None"},
	{"GrowWindow", (PyCFunction)WinObj_GrowWindow, 1,
	 "(Point startPt, Rect bBox) -> (long _rv)"},
	{"DragWindow", (PyCFunction)WinObj_DragWindow, 1,
	 "(Point startPt, Rect boundsRect) -> None"},
	{"ZoomWindow", (PyCFunction)WinObj_ZoomWindow, 1,
	 "(WindowPartCode partCode, Boolean front) -> None"},
	{"IsWindowCollapsable", (PyCFunction)WinObj_IsWindowCollapsable, 1,
	 "() -> (Boolean _rv)"},
	{"IsWindowCollapsed", (PyCFunction)WinObj_IsWindowCollapsed, 1,
	 "() -> (Boolean _rv)"},
	{"CollapseWindow", (PyCFunction)WinObj_CollapseWindow, 1,
	 "(Boolean collapse) -> None"},
	{"GetWindowBounds", (PyCFunction)WinObj_GetWindowBounds, 1,
	 "(WindowRegionCode regionCode) -> (Rect globalBounds)"},
	{"ResizeWindow", (PyCFunction)WinObj_ResizeWindow, 1,
	 "(Point startPoint, Rect sizeConstraints) -> (Boolean _rv, Rect newContentRect)"},
	{"SetWindowBounds", (PyCFunction)WinObj_SetWindowBounds, 1,
	 "(WindowRegionCode regionCode, Rect globalBounds) -> None"},
	{"RepositionWindow", (PyCFunction)WinObj_RepositionWindow, 1,
	 "(WindowPtr parentWindow, WindowPositionMethod method) -> None"},
	{"MoveWindowStructure", (PyCFunction)WinObj_MoveWindowStructure, 1,
	 "(short hGlobal, short vGlobal) -> None"},
	{"IsWindowInStandardState", (PyCFunction)WinObj_IsWindowInStandardState, 1,
	 "() -> (Boolean _rv, Point idealSize, Rect idealStandardState)"},
	{"ZoomWindowIdeal", (PyCFunction)WinObj_ZoomWindowIdeal, 1,
	 "(WindowPartCode partCode) -> (Point ioIdealSize)"},
	{"GetWindowIdealUserState", (PyCFunction)WinObj_GetWindowIdealUserState, 1,
	 "() -> (Rect userState)"},
	{"SetWindowIdealUserState", (PyCFunction)WinObj_SetWindowIdealUserState, 1,
	 "(Rect userState) -> None"},

#if !TARGET_API_MAC_OS8
	{"GetWindowGreatestAreaDevice", (PyCFunction)WinObj_GetWindowGreatestAreaDevice, 1,
	 "(WindowRegionCode inRegion) -> (GDHandle outGreatestDevice, Rect outGreatestDeviceRect)"},
#endif

#if !TARGET_API_MAC_OS8
	{"ConstrainWindowToScreen", (PyCFunction)WinObj_ConstrainWindowToScreen, 1,
	 "(WindowRegionCode inRegionCode, WindowConstrainOptions inOptions, Rect inScreenRect) -> (Rect outStructure)"},
#endif
	{"HideWindow", (PyCFunction)WinObj_HideWindow, 1,
	 "() -> None"},
	{"MacShowWindow", (PyCFunction)WinObj_MacShowWindow, 1,
	 "() -> None"},
	{"ShowHide", (PyCFunction)WinObj_ShowHide, 1,
	 "(Boolean showFlag) -> None"},
	{"MacIsWindowVisible", (PyCFunction)WinObj_MacIsWindowVisible, 1,
	 "() -> (Boolean _rv)"},

#if !TARGET_API_MAC_OS8
	{"ShowSheetWindow", (PyCFunction)WinObj_ShowSheetWindow, 1,
	 "(WindowPtr inParentWindow) -> None"},
#endif

#if !TARGET_API_MAC_OS8
	{"HideSheetWindow", (PyCFunction)WinObj_HideSheetWindow, 1,
	 "() -> None"},
#endif

#if !TARGET_API_MAC_OS8
	{"GetSheetWindowParent", (PyCFunction)WinObj_GetSheetWindowParent, 1,
	 "() -> (WindowPtr outParentWindow)"},
#endif

#if !TARGET_API_MAC_OS8
	{"GetWindowPropertyAttributes", (PyCFunction)WinObj_GetWindowPropertyAttributes, 1,
	 "(OSType propertyCreator, OSType propertyTag) -> (UInt32 attributes)"},
#endif

#if !TARGET_API_MAC_OS8
	{"ChangeWindowPropertyAttributes", (PyCFunction)WinObj_ChangeWindowPropertyAttributes, 1,
	 "(OSType propertyCreator, OSType propertyTag, UInt32 attributesToSet, UInt32 attributesToClear) -> None"},
#endif
	{"TrackBox", (PyCFunction)WinObj_TrackBox, 1,
	 "(Point thePt, WindowPartCode partCode) -> (Boolean _rv)"},
	{"TrackGoAway", (PyCFunction)WinObj_TrackGoAway, 1,
	 "(Point thePt) -> (Boolean _rv)"},

#if !TARGET_API_MAC_CARBON
	{"GetAuxWin", (PyCFunction)WinObj_GetAuxWin, 1,
	 "() -> (Boolean _rv, AuxWinHandle awHndl)"},
#endif

#if !TARGET_API_MAC_CARBON
	{"GetWindowGoAwayFlag", (PyCFunction)WinObj_GetWindowGoAwayFlag, 1,
	 "() -> (Boolean _rv)"},
#endif

#if !TARGET_API_MAC_CARBON
	{"GetWindowSpareFlag", (PyCFunction)WinObj_GetWindowSpareFlag, 1,
	 "() -> (Boolean _rv)"},
#endif
	{"GetWindowPort", (PyCFunction)WinObj_GetWindowPort, 1,
	 "() -> (CGrafPtr _rv)"},
	{"GetWindowKind", (PyCFunction)WinObj_GetWindowKind, 1,
	 "() -> (short _rv)"},
	{"IsWindowHilited", (PyCFunction)WinObj_IsWindowHilited, 1,
	 "() -> (Boolean _rv)"},

#if !TARGET_API_MAC_OS8
	{"IsWindowUpdatePending", (PyCFunction)WinObj_IsWindowUpdatePending, 1,
	 "() -> (Boolean _rv)"},
#endif
	{"MacGetNextWindow", (PyCFunction)WinObj_MacGetNextWindow, 1,
	 "() -> (WindowPtr _rv)"},
	{"GetWindowStandardState", (PyCFunction)WinObj_GetWindowStandardState, 1,
	 "() -> (Rect rect)"},
	{"GetWindowUserState", (PyCFunction)WinObj_GetWindowUserState, 1,
	 "() -> (Rect rect)"},
	{"SetWindowKind", (PyCFunction)WinObj_SetWindowKind, 1,
	 "(short kind) -> None"},
	{"SetWindowStandardState", (PyCFunction)WinObj_SetWindowStandardState, 1,
	 "(Rect rect) -> None"},
	{"SetWindowUserState", (PyCFunction)WinObj_SetWindowUserState, 1,
	 "(Rect rect) -> None"},
	{"SetPortWindowPort", (PyCFunction)WinObj_SetPortWindowPort, 1,
	 "() -> None"},
	{"GetWindowPortBounds", (PyCFunction)WinObj_GetWindowPortBounds, 1,
	 "() -> (Rect bounds)"},
	{"IsWindowVisible", (PyCFunction)WinObj_IsWindowVisible, 1,
	 "() -> (Boolean _rv)"},

#if !TARGET_API_MAC_CARBON
	{"GetWindowZoomFlag", (PyCFunction)WinObj_GetWindowZoomFlag, 1,
	 "() -> (Boolean _rv)"},
#endif
	{"GetWindowStructureRgn", (PyCFunction)WinObj_GetWindowStructureRgn, 1,
	 "(RgnHandle r) -> None"},
	{"GetWindowContentRgn", (PyCFunction)WinObj_GetWindowContentRgn, 1,
	 "(RgnHandle r) -> None"},
	{"GetWindowUpdateRgn", (PyCFunction)WinObj_GetWindowUpdateRgn, 1,
	 "(RgnHandle r) -> None"},

#if !TARGET_API_MAC_CARBON
	{"GetWindowTitleWidth", (PyCFunction)WinObj_GetWindowTitleWidth, 1,
	 "() -> (short _rv)"},
#endif
	{"GetNextWindow", (PyCFunction)WinObj_GetNextWindow, 1,
	 "() -> (WindowPtr _rv)"},

#if !TARGET_API_MAC_CARBON
	{"CloseWindow", (PyCFunction)WinObj_CloseWindow, 1,
	 "() -> None"},
#endif
	{"MoveWindow", (PyCFunction)WinObj_MoveWindow, 1,
	 "(short hGlobal, short vGlobal, Boolean front) -> None"},
	{"ShowWindow", (PyCFunction)WinObj_ShowWindow, 1,
	 "() -> None"},
	{NULL, NULL, 0}
};

PyMethodChain WinObj_chain = { WinObj_methods, NULL };

static PyObject *WinObj_getattr(WindowObject *self, char *name)
{
	return Py_FindMethodInChain(&WinObj_chain, (PyObject *)self, name);
}

#define WinObj_setattr NULL

static int WinObj_compare(WindowObject *self, WindowObject *other)
{
	if ( self->ob_itself > other->ob_itself ) return 1;
	if ( self->ob_itself < other->ob_itself ) return -1;
	return 0;
}

static PyObject * WinObj_repr(WindowObject *self)
{
	char buf[100];
	sprintf(buf, "<Window object at 0x%8.8x for 0x%8.8x>", (unsigned)self, (unsigned)self->ob_itself);
	return PyString_FromString(buf);
}

static int WinObj_hash(WindowObject *self)
{
	return (int)self->ob_itself;
}

PyTypeObject Window_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"_Win.Window", /*tp_name*/
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


static PyObject *Win_GetNewCWindow(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	WindowPtr _rv;
	short windowID;
	WindowPtr behind;
#ifndef GetNewCWindow
	PyMac_PRECHECK(GetNewCWindow);
#endif
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

static PyObject *Win_NewWindow(PyObject *_self, PyObject *_args)
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
#ifndef NewWindow
	PyMac_PRECHECK(NewWindow);
#endif
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

static PyObject *Win_GetNewWindow(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	WindowPtr _rv;
	short windowID;
	WindowPtr behind;
#ifndef GetNewWindow
	PyMac_PRECHECK(GetNewWindow);
#endif
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

static PyObject *Win_NewCWindow(PyObject *_self, PyObject *_args)
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
#ifndef NewCWindow
	PyMac_PRECHECK(NewCWindow);
#endif
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

static PyObject *Win_CreateNewWindow(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowClass windowClass;
	WindowAttributes attributes;
	Rect contentBounds;
	WindowPtr outWindow;
#ifndef CreateNewWindow
	PyMac_PRECHECK(CreateNewWindow);
#endif
	if (!PyArg_ParseTuple(_args, "llO&",
	                      &windowClass,
	                      &attributes,
	                      PyMac_GetRect, &contentBounds))
		return NULL;
	_err = CreateNewWindow(windowClass,
	                       attributes,
	                       &contentBounds,
	                       &outWindow);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     WinObj_WhichWindow, outWindow);
	return _res;
}

static PyObject *Win_CreateWindowFromResource(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	SInt16 resID;
	WindowPtr outWindow;
#ifndef CreateWindowFromResource
	PyMac_PRECHECK(CreateWindowFromResource);
#endif
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

static PyObject *Win_ShowFloatingWindows(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
#ifndef ShowFloatingWindows
	PyMac_PRECHECK(ShowFloatingWindows);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = ShowFloatingWindows();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Win_HideFloatingWindows(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
#ifndef HideFloatingWindows
	PyMac_PRECHECK(HideFloatingWindows);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = HideFloatingWindows();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Win_AreFloatingWindowsVisible(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
#ifndef AreFloatingWindowsVisible
	PyMac_PRECHECK(AreFloatingWindowsVisible);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = AreFloatingWindowsVisible();
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

#if !TARGET_API_MAC_CARBON

static PyObject *Win_SetDeskCPat(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PixPatHandle deskPixPat;
#ifndef SetDeskCPat
	PyMac_PRECHECK(SetDeskCPat);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &deskPixPat))
		return NULL;
	SetDeskCPat(deskPixPat);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

static PyObject *Win_CheckUpdate(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	EventRecord theEvent;
#ifndef CheckUpdate
	PyMac_PRECHECK(CheckUpdate);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = CheckUpdate(&theEvent);
	_res = Py_BuildValue("bO&",
	                     _rv,
	                     PyMac_BuildEventRecord, &theEvent);
	return _res;
}

static PyObject *Win_MacFindWindow(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	WindowPartCode _rv;
	Point thePoint;
	WindowPtr window;
#ifndef MacFindWindow
	PyMac_PRECHECK(MacFindWindow);
#endif
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

static PyObject *Win_FrontWindow(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	WindowPtr _rv;
#ifndef FrontWindow
	PyMac_PRECHECK(FrontWindow);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = FrontWindow();
	_res = Py_BuildValue("O&",
	                     WinObj_WhichWindow, _rv);
	return _res;
}

static PyObject *Win_FrontNonFloatingWindow(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	WindowPtr _rv;
#ifndef FrontNonFloatingWindow
	PyMac_PRECHECK(FrontNonFloatingWindow);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = FrontNonFloatingWindow();
	_res = Py_BuildValue("O&",
	                     WinObj_WhichWindow, _rv);
	return _res;
}

#if !TARGET_API_MAC_OS8

static PyObject *Win_GetFrontWindowOfClass(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	WindowPtr _rv;
	WindowClass inWindowClass;
	Boolean mustBeVisible;
#ifndef GetFrontWindowOfClass
	PyMac_PRECHECK(GetFrontWindowOfClass);
#endif
	if (!PyArg_ParseTuple(_args, "lb",
	                      &inWindowClass,
	                      &mustBeVisible))
		return NULL;
	_rv = GetFrontWindowOfClass(inWindowClass,
	                            mustBeVisible);
	_res = Py_BuildValue("O&",
	                     WinObj_New, _rv);
	return _res;
}
#endif

#if !TARGET_API_MAC_OS8

static PyObject *Win_FindWindowOfClass(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	Point where;
	WindowClass inWindowClass;
	WindowPtr outWindow;
	WindowPartCode outWindowPart;
#ifndef FindWindowOfClass
	PyMac_PRECHECK(FindWindowOfClass);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      PyMac_GetPoint, &where,
	                      &inWindowClass))
		return NULL;
	_err = FindWindowOfClass(&where,
	                         inWindowClass,
	                         &outWindow,
	                         &outWindowPart);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&h",
	                     WinObj_WhichWindow, outWindow,
	                     outWindowPart);
	return _res;
}
#endif

#if !TARGET_API_MAC_OS8

static PyObject *Win_CreateStandardWindowMenu(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	OptionBits inOptions;
	MenuHandle outMenu;
#ifndef CreateStandardWindowMenu
	PyMac_PRECHECK(CreateStandardWindowMenu);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &inOptions))
		return NULL;
	_err = CreateStandardWindowMenu(inOptions,
	                                &outMenu);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     MenuObj_New, outMenu);
	return _res;
}
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *Win_InitWindows(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef InitWindows
	PyMac_PRECHECK(InitWindows);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	InitWindows();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *Win_GetWMgrPort(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	GrafPtr wPort;
#ifndef GetWMgrPort
	PyMac_PRECHECK(GetWMgrPort);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetWMgrPort(&wPort);
	_res = Py_BuildValue("O&",
	                     GrafObj_New, wPort);
	return _res;
}
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *Win_GetCWMgrPort(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CGrafPtr wMgrCPort;
#ifndef GetCWMgrPort
	PyMac_PRECHECK(GetCWMgrPort);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetCWMgrPort(&wMgrCPort);
	_res = Py_BuildValue("O&",
	                     GrafObj_New, wMgrCPort);
	return _res;
}
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *Win_InitFloatingWindows(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
#ifndef InitFloatingWindows
	PyMac_PRECHECK(InitFloatingWindows);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = InitFloatingWindows();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *Win_InvalRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Rect badRect;
#ifndef InvalRect
	PyMac_PRECHECK(InvalRect);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &badRect))
		return NULL;
	InvalRect(&badRect);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *Win_InvalRgn(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	RgnHandle badRgn;
#ifndef InvalRgn
	PyMac_PRECHECK(InvalRgn);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &badRgn))
		return NULL;
	InvalRgn(badRgn);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *Win_ValidRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Rect goodRect;
#ifndef ValidRect
	PyMac_PRECHECK(ValidRect);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &goodRect))
		return NULL;
	ValidRect(&goodRect);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *Win_ValidRgn(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	RgnHandle goodRgn;
#ifndef ValidRgn
	PyMac_PRECHECK(ValidRgn);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &goodRgn))
		return NULL;
	ValidRgn(goodRgn);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

static PyObject *Win_CollapseAllWindows(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	Boolean collapse;
#ifndef CollapseAllWindows
	PyMac_PRECHECK(CollapseAllWindows);
#endif
	if (!PyArg_ParseTuple(_args, "b",
	                      &collapse))
		return NULL;
	_err = CollapseAllWindows(collapse);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

#if !TARGET_API_MAC_OS8

static PyObject *Win_GetAvailableWindowPositioningBounds(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	GDHandle inDevice;
	Rect availableRect;
#ifndef GetAvailableWindowPositioningBounds
	PyMac_PRECHECK(GetAvailableWindowPositioningBounds);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &inDevice))
		return NULL;
	_err = GetAvailableWindowPositioningBounds(inDevice,
	                                           &availableRect);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &availableRect);
	return _res;
}
#endif

#if !TARGET_API_MAC_OS8

static PyObject *Win_DisableScreenUpdates(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
#ifndef DisableScreenUpdates
	PyMac_PRECHECK(DisableScreenUpdates);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = DisableScreenUpdates();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

#if !TARGET_API_MAC_OS8

static PyObject *Win_EnableScreenUpdates(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
#ifndef EnableScreenUpdates
	PyMac_PRECHECK(EnableScreenUpdates);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = EnableScreenUpdates();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

static PyObject *Win_PinRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
	Rect theRect;
	Point thePt;
#ifndef PinRect
	PyMac_PRECHECK(PinRect);
#endif
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

static PyObject *Win_GetGrayRgn(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	RgnHandle _rv;
#ifndef GetGrayRgn
	PyMac_PRECHECK(GetGrayRgn);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetGrayRgn();
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *Win_GetWindowFromPort(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	WindowPtr _rv;
	CGrafPtr port;
#ifndef GetWindowFromPort
	PyMac_PRECHECK(GetWindowFromPort);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      GrafObj_Convert, &port))
		return NULL;
	_rv = GetWindowFromPort(port);
	_res = Py_BuildValue("O&",
	                     WinObj_New, _rv);
	return _res;
}

static PyObject *Win_WhichWindow(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;

	long ptr;

	if ( !PyArg_ParseTuple(_args, "i", &ptr) )
		return NULL;
	_res = WinObj_WhichWindow((WindowPtr)ptr);
	return _res;

}

static PyObject *Win_FindWindow(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short _rv;
	Point thePoint;
	WindowPtr theWindow;
#ifndef FindWindow
	PyMac_PRECHECK(FindWindow);
#endif
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
	 "(WindowClass windowClass, WindowAttributes attributes, Rect contentBounds) -> (WindowPtr outWindow)"},
	{"CreateWindowFromResource", (PyCFunction)Win_CreateWindowFromResource, 1,
	 "(SInt16 resID) -> (WindowPtr outWindow)"},
	{"ShowFloatingWindows", (PyCFunction)Win_ShowFloatingWindows, 1,
	 "() -> None"},
	{"HideFloatingWindows", (PyCFunction)Win_HideFloatingWindows, 1,
	 "() -> None"},
	{"AreFloatingWindowsVisible", (PyCFunction)Win_AreFloatingWindowsVisible, 1,
	 "() -> (Boolean _rv)"},

#if !TARGET_API_MAC_CARBON
	{"SetDeskCPat", (PyCFunction)Win_SetDeskCPat, 1,
	 "(PixPatHandle deskPixPat) -> None"},
#endif
	{"CheckUpdate", (PyCFunction)Win_CheckUpdate, 1,
	 "() -> (Boolean _rv, EventRecord theEvent)"},
	{"MacFindWindow", (PyCFunction)Win_MacFindWindow, 1,
	 "(Point thePoint) -> (WindowPartCode _rv, WindowPtr window)"},
	{"FrontWindow", (PyCFunction)Win_FrontWindow, 1,
	 "() -> (WindowPtr _rv)"},
	{"FrontNonFloatingWindow", (PyCFunction)Win_FrontNonFloatingWindow, 1,
	 "() -> (WindowPtr _rv)"},

#if !TARGET_API_MAC_OS8
	{"GetFrontWindowOfClass", (PyCFunction)Win_GetFrontWindowOfClass, 1,
	 "(WindowClass inWindowClass, Boolean mustBeVisible) -> (WindowPtr _rv)"},
#endif

#if !TARGET_API_MAC_OS8
	{"FindWindowOfClass", (PyCFunction)Win_FindWindowOfClass, 1,
	 "(Point where, WindowClass inWindowClass) -> (WindowPtr outWindow, WindowPartCode outWindowPart)"},
#endif

#if !TARGET_API_MAC_OS8
	{"CreateStandardWindowMenu", (PyCFunction)Win_CreateStandardWindowMenu, 1,
	 "(OptionBits inOptions) -> (MenuHandle outMenu)"},
#endif

#if !TARGET_API_MAC_CARBON
	{"InitWindows", (PyCFunction)Win_InitWindows, 1,
	 "() -> None"},
#endif

#if !TARGET_API_MAC_CARBON
	{"GetWMgrPort", (PyCFunction)Win_GetWMgrPort, 1,
	 "() -> (GrafPtr wPort)"},
#endif

#if !TARGET_API_MAC_CARBON
	{"GetCWMgrPort", (PyCFunction)Win_GetCWMgrPort, 1,
	 "() -> (CGrafPtr wMgrCPort)"},
#endif

#if !TARGET_API_MAC_CARBON
	{"InitFloatingWindows", (PyCFunction)Win_InitFloatingWindows, 1,
	 "() -> None"},
#endif

#if !TARGET_API_MAC_CARBON
	{"InvalRect", (PyCFunction)Win_InvalRect, 1,
	 "(Rect badRect) -> None"},
#endif

#if !TARGET_API_MAC_CARBON
	{"InvalRgn", (PyCFunction)Win_InvalRgn, 1,
	 "(RgnHandle badRgn) -> None"},
#endif

#if !TARGET_API_MAC_CARBON
	{"ValidRect", (PyCFunction)Win_ValidRect, 1,
	 "(Rect goodRect) -> None"},
#endif

#if !TARGET_API_MAC_CARBON
	{"ValidRgn", (PyCFunction)Win_ValidRgn, 1,
	 "(RgnHandle goodRgn) -> None"},
#endif
	{"CollapseAllWindows", (PyCFunction)Win_CollapseAllWindows, 1,
	 "(Boolean collapse) -> None"},

#if !TARGET_API_MAC_OS8
	{"GetAvailableWindowPositioningBounds", (PyCFunction)Win_GetAvailableWindowPositioningBounds, 1,
	 "(GDHandle inDevice) -> (Rect availableRect)"},
#endif

#if !TARGET_API_MAC_OS8
	{"DisableScreenUpdates", (PyCFunction)Win_DisableScreenUpdates, 1,
	 "() -> None"},
#endif

#if !TARGET_API_MAC_OS8
	{"EnableScreenUpdates", (PyCFunction)Win_EnableScreenUpdates, 1,
	 "() -> None"},
#endif
	{"PinRect", (PyCFunction)Win_PinRect, 1,
	 "(Rect theRect, Point thePt) -> (long _rv)"},
	{"GetGrayRgn", (PyCFunction)Win_GetGrayRgn, 1,
	 "() -> (RgnHandle _rv)"},
	{"GetWindowFromPort", (PyCFunction)Win_GetWindowFromPort, 1,
	 "(CGrafPtr port) -> (WindowPtr _rv)"},
	{"WhichWindow", (PyCFunction)Win_WhichWindow, 1,
	 "Resolve an integer WindowPtr address to a Window object"},
	{"FindWindow", (PyCFunction)Win_FindWindow, 1,
	 "(Point thePoint) -> (short _rv, WindowPtr theWindow)"},
	{NULL, NULL, 0}
};



/* Return the object corresponding to the window, or NULL */

PyObject *
WinObj_WhichWindow(WindowPtr w)
{
	PyObject *it;
	
	if (w == NULL) {
		it = Py_None;
		Py_INCREF(it);
	} else {
		it = (PyObject *) GetWRefCon(w);
		if (it == NULL || !IsPointerValid((Ptr)it) || ((WindowObject *)it)->ob_itself != w || !WinObj_Check(it)) {
			it = WinObj_New(w);
			((WindowObject *)it)->ob_freeit = NULL;
		} else {
			Py_INCREF(it);
		}
	}
	return it;
}


void init_Win(void)
{
	PyObject *m;
	PyObject *d;



		PyMac_INIT_TOOLBOX_OBJECT_NEW(WindowPtr, WinObj_New);
		PyMac_INIT_TOOLBOX_OBJECT_NEW(WindowPtr, WinObj_WhichWindow);
		PyMac_INIT_TOOLBOX_OBJECT_CONVERT(WindowPtr, WinObj_Convert);


	m = Py_InitModule("_Win", Win_methods);
	d = PyModule_GetDict(m);
	Win_Error = PyMac_GetOSErrException();
	if (Win_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Win_Error) != 0)
		return;
	Window_Type.ob_type = &PyType_Type;
	Py_INCREF(&Window_Type);
	if (PyDict_SetItemString(d, "WindowType", (PyObject *)&Window_Type) != 0)
		Py_FatalError("can't initialize WindowType");
}

/* ======================== End module _Win ========================= */

