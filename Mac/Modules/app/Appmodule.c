
/* =========================== Module App =========================== */

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

#include <Appearance.h>

#define resNotFound -192 /* Can't include <Errors.h> because of Python's "errors.h" */

static PyObject *App_Error;

static PyObject *App_RegisterAppearanceClient(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = RegisterAppearanceClient();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *App_UnregisterAppearanceClient(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = UnregisterAppearanceClient();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *App_SetThemePen(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	ThemeBrush inBrush;
	SInt16 inDepth;
	Boolean inIsColorDevice;
	if (!PyArg_ParseTuple(_args, "hhb",
	                      &inBrush,
	                      &inDepth,
	                      &inIsColorDevice))
		return NULL;
	_err = SetThemePen(inBrush,
	                   inDepth,
	                   inIsColorDevice);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *App_SetThemeBackground(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	ThemeBrush inBrush;
	SInt16 inDepth;
	Boolean inIsColorDevice;
	if (!PyArg_ParseTuple(_args, "hhb",
	                      &inBrush,
	                      &inDepth,
	                      &inIsColorDevice))
		return NULL;
	_err = SetThemeBackground(inBrush,
	                          inDepth,
	                          inIsColorDevice);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *App_SetThemeTextColor(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	ThemeTextColor inColor;
	SInt16 inDepth;
	Boolean inIsColorDevice;
	if (!PyArg_ParseTuple(_args, "hhb",
	                      &inColor,
	                      &inDepth,
	                      &inIsColorDevice))
		return NULL;
	_err = SetThemeTextColor(inColor,
	                         inDepth,
	                         inIsColorDevice);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *App_SetThemeWindowBackground(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowPtr inWindow;
	ThemeBrush inBrush;
	Boolean inUpdate;
	if (!PyArg_ParseTuple(_args, "O&hb",
	                      WinObj_Convert, &inWindow,
	                      &inBrush,
	                      &inUpdate))
		return NULL;
	_err = SetThemeWindowBackground(inWindow,
	                                inBrush,
	                                inUpdate);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *App_DrawThemeWindowHeader(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Rect inRect;
	ThemeDrawState inState;
	if (!PyArg_ParseTuple(_args, "O&l",
	                      PyMac_GetRect, &inRect,
	                      &inState))
		return NULL;
	_err = DrawThemeWindowHeader(&inRect,
	                             inState);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *App_DrawThemeWindowListViewHeader(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Rect inRect;
	ThemeDrawState inState;
	if (!PyArg_ParseTuple(_args, "O&l",
	                      PyMac_GetRect, &inRect,
	                      &inState))
		return NULL;
	_err = DrawThemeWindowListViewHeader(&inRect,
	                                     inState);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *App_DrawThemePlacard(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Rect inRect;
	ThemeDrawState inState;
	if (!PyArg_ParseTuple(_args, "O&l",
	                      PyMac_GetRect, &inRect,
	                      &inState))
		return NULL;
	_err = DrawThemePlacard(&inRect,
	                        inState);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *App_DrawThemeEditTextFrame(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Rect inRect;
	ThemeDrawState inState;
	if (!PyArg_ParseTuple(_args, "O&l",
	                      PyMac_GetRect, &inRect,
	                      &inState))
		return NULL;
	_err = DrawThemeEditTextFrame(&inRect,
	                              inState);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *App_DrawThemeListBoxFrame(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Rect inRect;
	ThemeDrawState inState;
	if (!PyArg_ParseTuple(_args, "O&l",
	                      PyMac_GetRect, &inRect,
	                      &inState))
		return NULL;
	_err = DrawThemeListBoxFrame(&inRect,
	                             inState);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *App_DrawThemeFocusRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Rect inRect;
	Boolean inHasFocus;
	if (!PyArg_ParseTuple(_args, "O&b",
	                      PyMac_GetRect, &inRect,
	                      &inHasFocus))
		return NULL;
	_err = DrawThemeFocusRect(&inRect,
	                          inHasFocus);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *App_DrawThemePrimaryGroup(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Rect inRect;
	ThemeDrawState inState;
	if (!PyArg_ParseTuple(_args, "O&l",
	                      PyMac_GetRect, &inRect,
	                      &inState))
		return NULL;
	_err = DrawThemePrimaryGroup(&inRect,
	                             inState);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *App_DrawThemeSecondaryGroup(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Rect inRect;
	ThemeDrawState inState;
	if (!PyArg_ParseTuple(_args, "O&l",
	                      PyMac_GetRect, &inRect,
	                      &inState))
		return NULL;
	_err = DrawThemeSecondaryGroup(&inRect,
	                               inState);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *App_DrawThemeSeparator(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Rect inRect;
	ThemeDrawState inState;
	if (!PyArg_ParseTuple(_args, "O&l",
	                      PyMac_GetRect, &inRect,
	                      &inState))
		return NULL;
	_err = DrawThemeSeparator(&inRect,
	                          inState);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *App_DrawThemeModelessDialogFrame(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Rect inRect;
	ThemeDrawState inState;
	if (!PyArg_ParseTuple(_args, "O&l",
	                      PyMac_GetRect, &inRect,
	                      &inState))
		return NULL;
	_err = DrawThemeModelessDialogFrame(&inRect,
	                                    inState);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *App_DrawThemeGenericWell(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Rect inRect;
	ThemeDrawState inState;
	Boolean inFillCenter;
	if (!PyArg_ParseTuple(_args, "O&lb",
	                      PyMac_GetRect, &inRect,
	                      &inState,
	                      &inFillCenter))
		return NULL;
	_err = DrawThemeGenericWell(&inRect,
	                            inState,
	                            inFillCenter);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *App_DrawThemeFocusRegion(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Boolean inHasFocus;
	if (!PyArg_ParseTuple(_args, "b",
	                      &inHasFocus))
		return NULL;
	_err = DrawThemeFocusRegion((RgnHandle)0,
	                            inHasFocus);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *App_IsThemeInColor(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	SInt16 inDepth;
	Boolean inIsColorDevice;
	if (!PyArg_ParseTuple(_args, "hb",
	                      &inDepth,
	                      &inIsColorDevice))
		return NULL;
	_rv = IsThemeInColor(inDepth,
	                     inIsColorDevice);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *App_GetThemeAccentColors(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	CTabHandle outColors;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetThemeAccentColors(&outColors);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, outColors);
	return _res;
}

static PyObject *App_DrawThemeMenuBarBackground(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Rect inBounds;
	ThemeMenuBarState inState;
	UInt32 inAttributes;
	if (!PyArg_ParseTuple(_args, "O&hl",
	                      PyMac_GetRect, &inBounds,
	                      &inState,
	                      &inAttributes))
		return NULL;
	_err = DrawThemeMenuBarBackground(&inBounds,
	                                  inState,
	                                  inAttributes);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *App_GetThemeMenuBarHeight(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	SInt16 outHeight;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetThemeMenuBarHeight(&outHeight);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     outHeight);
	return _res;
}

static PyObject *App_DrawThemeMenuBackground(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Rect inMenuRect;
	ThemeMenuType inMenuType;
	if (!PyArg_ParseTuple(_args, "O&h",
	                      PyMac_GetRect, &inMenuRect,
	                      &inMenuType))
		return NULL;
	_err = DrawThemeMenuBackground(&inMenuRect,
	                               inMenuType);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *App_GetThemeMenuBackgroundRegion(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Rect inMenuRect;
	ThemeMenuType menuType;
	if (!PyArg_ParseTuple(_args, "O&h",
	                      PyMac_GetRect, &inMenuRect,
	                      &menuType))
		return NULL;
	_err = GetThemeMenuBackgroundRegion(&inMenuRect,
	                                    menuType,
	                                    (RgnHandle)0);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *App_DrawThemeMenuSeparator(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	Rect inItemRect;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &inItemRect))
		return NULL;
	_err = DrawThemeMenuSeparator(&inItemRect);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *App_GetThemeMenuSeparatorHeight(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	SInt16 outHeight;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetThemeMenuSeparatorHeight(&outHeight);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     outHeight);
	return _res;
}

static PyObject *App_GetThemeMenuItemExtra(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	ThemeMenuItemType inItemType;
	SInt16 outHeight;
	SInt16 outWidth;
	if (!PyArg_ParseTuple(_args, "h",
	                      &inItemType))
		return NULL;
	_err = GetThemeMenuItemExtra(inItemType,
	                             &outHeight,
	                             &outWidth);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("hh",
	                     outHeight,
	                     outWidth);
	return _res;
}

static PyObject *App_GetThemeMenuTitleExtra(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	SInt16 outWidth;
	Boolean inIsSquished;
	if (!PyArg_ParseTuple(_args, "b",
	                      &inIsSquished))
		return NULL;
	_err = GetThemeMenuTitleExtra(&outWidth,
	                              inIsSquished);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     outWidth);
	return _res;
}

static PyMethodDef App_methods[] = {
	{"RegisterAppearanceClient", (PyCFunction)App_RegisterAppearanceClient, 1,
	 "() -> None"},
	{"UnregisterAppearanceClient", (PyCFunction)App_UnregisterAppearanceClient, 1,
	 "() -> None"},
	{"SetThemePen", (PyCFunction)App_SetThemePen, 1,
	 "(ThemeBrush inBrush, SInt16 inDepth, Boolean inIsColorDevice) -> None"},
	{"SetThemeBackground", (PyCFunction)App_SetThemeBackground, 1,
	 "(ThemeBrush inBrush, SInt16 inDepth, Boolean inIsColorDevice) -> None"},
	{"SetThemeTextColor", (PyCFunction)App_SetThemeTextColor, 1,
	 "(ThemeTextColor inColor, SInt16 inDepth, Boolean inIsColorDevice) -> None"},
	{"SetThemeWindowBackground", (PyCFunction)App_SetThemeWindowBackground, 1,
	 "(WindowPtr inWindow, ThemeBrush inBrush, Boolean inUpdate) -> None"},
	{"DrawThemeWindowHeader", (PyCFunction)App_DrawThemeWindowHeader, 1,
	 "(Rect inRect, ThemeDrawState inState) -> None"},
	{"DrawThemeWindowListViewHeader", (PyCFunction)App_DrawThemeWindowListViewHeader, 1,
	 "(Rect inRect, ThemeDrawState inState) -> None"},
	{"DrawThemePlacard", (PyCFunction)App_DrawThemePlacard, 1,
	 "(Rect inRect, ThemeDrawState inState) -> None"},
	{"DrawThemeEditTextFrame", (PyCFunction)App_DrawThemeEditTextFrame, 1,
	 "(Rect inRect, ThemeDrawState inState) -> None"},
	{"DrawThemeListBoxFrame", (PyCFunction)App_DrawThemeListBoxFrame, 1,
	 "(Rect inRect, ThemeDrawState inState) -> None"},
	{"DrawThemeFocusRect", (PyCFunction)App_DrawThemeFocusRect, 1,
	 "(Rect inRect, Boolean inHasFocus) -> None"},
	{"DrawThemePrimaryGroup", (PyCFunction)App_DrawThemePrimaryGroup, 1,
	 "(Rect inRect, ThemeDrawState inState) -> None"},
	{"DrawThemeSecondaryGroup", (PyCFunction)App_DrawThemeSecondaryGroup, 1,
	 "(Rect inRect, ThemeDrawState inState) -> None"},
	{"DrawThemeSeparator", (PyCFunction)App_DrawThemeSeparator, 1,
	 "(Rect inRect, ThemeDrawState inState) -> None"},
	{"DrawThemeModelessDialogFrame", (PyCFunction)App_DrawThemeModelessDialogFrame, 1,
	 "(Rect inRect, ThemeDrawState inState) -> None"},
	{"DrawThemeGenericWell", (PyCFunction)App_DrawThemeGenericWell, 1,
	 "(Rect inRect, ThemeDrawState inState, Boolean inFillCenter) -> None"},
	{"DrawThemeFocusRegion", (PyCFunction)App_DrawThemeFocusRegion, 1,
	 "(Boolean inHasFocus) -> None"},
	{"IsThemeInColor", (PyCFunction)App_IsThemeInColor, 1,
	 "(SInt16 inDepth, Boolean inIsColorDevice) -> (Boolean _rv)"},
	{"GetThemeAccentColors", (PyCFunction)App_GetThemeAccentColors, 1,
	 "() -> (CTabHandle outColors)"},
	{"DrawThemeMenuBarBackground", (PyCFunction)App_DrawThemeMenuBarBackground, 1,
	 "(Rect inBounds, ThemeMenuBarState inState, UInt32 inAttributes) -> None"},
	{"GetThemeMenuBarHeight", (PyCFunction)App_GetThemeMenuBarHeight, 1,
	 "() -> (SInt16 outHeight)"},
	{"DrawThemeMenuBackground", (PyCFunction)App_DrawThemeMenuBackground, 1,
	 "(Rect inMenuRect, ThemeMenuType inMenuType) -> None"},
	{"GetThemeMenuBackgroundRegion", (PyCFunction)App_GetThemeMenuBackgroundRegion, 1,
	 "(Rect inMenuRect, ThemeMenuType menuType) -> None"},
	{"DrawThemeMenuSeparator", (PyCFunction)App_DrawThemeMenuSeparator, 1,
	 "(Rect inItemRect) -> None"},
	{"GetThemeMenuSeparatorHeight", (PyCFunction)App_GetThemeMenuSeparatorHeight, 1,
	 "() -> (SInt16 outHeight)"},
	{"GetThemeMenuItemExtra", (PyCFunction)App_GetThemeMenuItemExtra, 1,
	 "(ThemeMenuItemType inItemType) -> (SInt16 outHeight, SInt16 outWidth)"},
	{"GetThemeMenuTitleExtra", (PyCFunction)App_GetThemeMenuTitleExtra, 1,
	 "(Boolean inIsSquished) -> (SInt16 outWidth)"},
	{NULL, NULL, 0}
};




void initApp()
{
	PyObject *m;
	PyObject *d;




	m = Py_InitModule("App", App_methods);
	d = PyModule_GetDict(m);
	App_Error = PyMac_GetOSErrException();
	if (App_Error == NULL ||
	    PyDict_SetItemString(d, "Error", App_Error) != 0)
		Py_FatalError("can't initialize App.Error");
}

/* ========================= End module App ========================= */

