
/* =========================== Module Icn =========================== */

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

#include <Icons.h>

/* Exported by Qdmodule.c: */
extern PyObject *QdRGB_New(RGBColor *);
extern int QdRGB_Convert(PyObject *, RGBColor *);

#define resNotFound -192 /* Can't include <Errors.h> because of Python's "errors.h" */

static PyObject *Icn_Error;

static PyObject *Icn_GetCIcon(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	CIconHandle _rv;
	SInt16 iconID;
	if (!PyArg_ParseTuple(_args, "h",
	                      &iconID))
		return NULL;
	_rv = GetCIcon(iconID);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *Icn_PlotCIcon(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect theRect;
	CIconHandle theIcon;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetRect, &theRect,
	                      ResObj_Convert, &theIcon))
		return NULL;
	PlotCIcon(&theRect,
	          theIcon);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_DisposeCIcon(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	CIconHandle theIcon;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &theIcon))
		return NULL;
	DisposeCIcon(theIcon);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_GetIcon(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Handle _rv;
	SInt16 iconID;
	if (!PyArg_ParseTuple(_args, "h",
	                      &iconID))
		return NULL;
	_rv = GetIcon(iconID);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *Icn_PlotIcon(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect theRect;
	Handle theIcon;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetRect, &theRect,
	                      ResObj_Convert, &theIcon))
		return NULL;
	PlotIcon(&theRect,
	         theIcon);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_PlotIconID(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	Rect theRect;
	IconAlignmentType align;
	IconTransformType transform;
	SInt16 theResID;
	if (!PyArg_ParseTuple(_args, "O&hhh",
	                      PyMac_GetRect, &theRect,
	                      &align,
	                      &transform,
	                      &theResID))
		return NULL;
	_err = PlotIconID(&theRect,
	                  align,
	                  transform,
	                  theResID);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_NewIconSuite(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	IconSuiteRef theIconSuite;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = NewIconSuite(&theIconSuite);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, theIconSuite);
	return _res;
}

static PyObject *Icn_AddIconToSuite(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle theIconData;
	IconSuiteRef theSuite;
	ResType theType;
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      ResObj_Convert, &theIconData,
	                      ResObj_Convert, &theSuite,
	                      PyMac_GetOSType, &theType))
		return NULL;
	_err = AddIconToSuite(theIconData,
	                      theSuite,
	                      theType);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_GetIconFromSuite(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle theIconData;
	IconSuiteRef theSuite;
	ResType theType;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &theSuite,
	                      PyMac_GetOSType, &theType))
		return NULL;
	_err = GetIconFromSuite(&theIconData,
	                        theSuite,
	                        theType);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, theIconData);
	return _res;
}

static PyObject *Icn_GetIconSuite(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	IconSuiteRef theIconSuite;
	SInt16 theResID;
	IconSelectorValue selector;
	if (!PyArg_ParseTuple(_args, "hl",
	                      &theResID,
	                      &selector))
		return NULL;
	_err = GetIconSuite(&theIconSuite,
	                    theResID,
	                    selector);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, theIconSuite);
	return _res;
}

static PyObject *Icn_DisposeIconSuite(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	IconSuiteRef theIconSuite;
	Boolean disposeData;
	if (!PyArg_ParseTuple(_args, "O&b",
	                      ResObj_Convert, &theIconSuite,
	                      &disposeData))
		return NULL;
	_err = DisposeIconSuite(theIconSuite,
	                        disposeData);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_PlotIconSuite(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	Rect theRect;
	IconAlignmentType align;
	IconTransformType transform;
	IconSuiteRef theIconSuite;
	if (!PyArg_ParseTuple(_args, "O&hhO&",
	                      PyMac_GetRect, &theRect,
	                      &align,
	                      &transform,
	                      ResObj_Convert, &theIconSuite))
		return NULL;
	_err = PlotIconSuite(&theRect,
	                     align,
	                     transform,
	                     theIconSuite);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_LoadIconCache(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	Rect theRect;
	IconAlignmentType align;
	IconTransformType transform;
	IconCacheRef theIconCache;
	if (!PyArg_ParseTuple(_args, "O&hhO&",
	                      PyMac_GetRect, &theRect,
	                      &align,
	                      &transform,
	                      ResObj_Convert, &theIconCache))
		return NULL;
	_err = LoadIconCache(&theRect,
	                     align,
	                     transform,
	                     theIconCache);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_GetLabel(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 labelNumber;
	RGBColor labelColor;
	Str255 labelString;
	if (!PyArg_ParseTuple(_args, "hO&",
	                      &labelNumber,
	                      PyMac_GetStr255, labelString))
		return NULL;
	_err = GetLabel(labelNumber,
	                &labelColor,
	                labelString);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     QdRGB_New, &labelColor);
	return _res;
}

static PyObject *Icn_PtInIconID(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	Point testPt;
	Rect iconRect;
	IconAlignmentType align;
	SInt16 iconID;
	if (!PyArg_ParseTuple(_args, "O&O&hh",
	                      PyMac_GetPoint, &testPt,
	                      PyMac_GetRect, &iconRect,
	                      &align,
	                      &iconID))
		return NULL;
	_rv = PtInIconID(testPt,
	                 &iconRect,
	                 align,
	                 iconID);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Icn_PtInIconSuite(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	Point testPt;
	Rect iconRect;
	IconAlignmentType align;
	IconSuiteRef theIconSuite;
	if (!PyArg_ParseTuple(_args, "O&O&hO&",
	                      PyMac_GetPoint, &testPt,
	                      PyMac_GetRect, &iconRect,
	                      &align,
	                      ResObj_Convert, &theIconSuite))
		return NULL;
	_rv = PtInIconSuite(testPt,
	                    &iconRect,
	                    align,
	                    theIconSuite);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Icn_RectInIconID(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	Rect testRect;
	Rect iconRect;
	IconAlignmentType align;
	SInt16 iconID;
	if (!PyArg_ParseTuple(_args, "O&O&hh",
	                      PyMac_GetRect, &testRect,
	                      PyMac_GetRect, &iconRect,
	                      &align,
	                      &iconID))
		return NULL;
	_rv = RectInIconID(&testRect,
	                   &iconRect,
	                   align,
	                   iconID);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Icn_RectInIconSuite(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	Rect testRect;
	Rect iconRect;
	IconAlignmentType align;
	IconSuiteRef theIconSuite;
	if (!PyArg_ParseTuple(_args, "O&O&hO&",
	                      PyMac_GetRect, &testRect,
	                      PyMac_GetRect, &iconRect,
	                      &align,
	                      ResObj_Convert, &theIconSuite))
		return NULL;
	_rv = RectInIconSuite(&testRect,
	                      &iconRect,
	                      align,
	                      theIconSuite);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Icn_IconIDToRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	RgnHandle theRgn;
	Rect iconRect;
	IconAlignmentType align;
	SInt16 iconID;
	if (!PyArg_ParseTuple(_args, "O&O&hh",
	                      ResObj_Convert, &theRgn,
	                      PyMac_GetRect, &iconRect,
	                      &align,
	                      &iconID))
		return NULL;
	_err = IconIDToRgn(theRgn,
	                   &iconRect,
	                   align,
	                   iconID);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_IconSuiteToRgn(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	RgnHandle theRgn;
	Rect iconRect;
	IconAlignmentType align;
	IconSuiteRef theIconSuite;
	if (!PyArg_ParseTuple(_args, "O&O&hO&",
	                      ResObj_Convert, &theRgn,
	                      PyMac_GetRect, &iconRect,
	                      &align,
	                      ResObj_Convert, &theIconSuite))
		return NULL;
	_err = IconSuiteToRgn(theRgn,
	                      &iconRect,
	                      align,
	                      theIconSuite);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_SetSuiteLabel(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	IconSuiteRef theSuite;
	SInt16 theLabel;
	if (!PyArg_ParseTuple(_args, "O&h",
	                      ResObj_Convert, &theSuite,
	                      &theLabel))
		return NULL;
	_err = SetSuiteLabel(theSuite,
	                     theLabel);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_GetSuiteLabel(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	SInt16 _rv;
	IconSuiteRef theSuite;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &theSuite))
		return NULL;
	_rv = GetSuiteLabel(theSuite);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Icn_PlotIconHandle(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	Rect theRect;
	IconAlignmentType align;
	IconTransformType transform;
	Handle theIcon;
	if (!PyArg_ParseTuple(_args, "O&hhO&",
	                      PyMac_GetRect, &theRect,
	                      &align,
	                      &transform,
	                      ResObj_Convert, &theIcon))
		return NULL;
	_err = PlotIconHandle(&theRect,
	                      align,
	                      transform,
	                      theIcon);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_PlotSICNHandle(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	Rect theRect;
	IconAlignmentType align;
	IconTransformType transform;
	Handle theSICN;
	if (!PyArg_ParseTuple(_args, "O&hhO&",
	                      PyMac_GetRect, &theRect,
	                      &align,
	                      &transform,
	                      ResObj_Convert, &theSICN))
		return NULL;
	_err = PlotSICNHandle(&theRect,
	                      align,
	                      transform,
	                      theSICN);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Icn_PlotCIconHandle(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	Rect theRect;
	IconAlignmentType align;
	IconTransformType transform;
	CIconHandle theCIcon;
	if (!PyArg_ParseTuple(_args, "O&hhO&",
	                      PyMac_GetRect, &theRect,
	                      &align,
	                      &transform,
	                      ResObj_Convert, &theCIcon))
		return NULL;
	_err = PlotCIconHandle(&theRect,
	                       align,
	                       transform,
	                       theCIcon);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef Icn_methods[] = {
	{"GetCIcon", (PyCFunction)Icn_GetCIcon, 1,
	 "(SInt16 iconID) -> (CIconHandle _rv)"},
	{"PlotCIcon", (PyCFunction)Icn_PlotCIcon, 1,
	 "(Rect theRect, CIconHandle theIcon) -> None"},
	{"DisposeCIcon", (PyCFunction)Icn_DisposeCIcon, 1,
	 "(CIconHandle theIcon) -> None"},
	{"GetIcon", (PyCFunction)Icn_GetIcon, 1,
	 "(SInt16 iconID) -> (Handle _rv)"},
	{"PlotIcon", (PyCFunction)Icn_PlotIcon, 1,
	 "(Rect theRect, Handle theIcon) -> None"},
	{"PlotIconID", (PyCFunction)Icn_PlotIconID, 1,
	 "(Rect theRect, IconAlignmentType align, IconTransformType transform, SInt16 theResID) -> None"},
	{"NewIconSuite", (PyCFunction)Icn_NewIconSuite, 1,
	 "() -> (IconSuiteRef theIconSuite)"},
	{"AddIconToSuite", (PyCFunction)Icn_AddIconToSuite, 1,
	 "(Handle theIconData, IconSuiteRef theSuite, ResType theType) -> None"},
	{"GetIconFromSuite", (PyCFunction)Icn_GetIconFromSuite, 1,
	 "(IconSuiteRef theSuite, ResType theType) -> (Handle theIconData)"},
	{"GetIconSuite", (PyCFunction)Icn_GetIconSuite, 1,
	 "(SInt16 theResID, IconSelectorValue selector) -> (IconSuiteRef theIconSuite)"},
	{"DisposeIconSuite", (PyCFunction)Icn_DisposeIconSuite, 1,
	 "(IconSuiteRef theIconSuite, Boolean disposeData) -> None"},
	{"PlotIconSuite", (PyCFunction)Icn_PlotIconSuite, 1,
	 "(Rect theRect, IconAlignmentType align, IconTransformType transform, IconSuiteRef theIconSuite) -> None"},
	{"LoadIconCache", (PyCFunction)Icn_LoadIconCache, 1,
	 "(Rect theRect, IconAlignmentType align, IconTransformType transform, IconCacheRef theIconCache) -> None"},
	{"GetLabel", (PyCFunction)Icn_GetLabel, 1,
	 "(SInt16 labelNumber, Str255 labelString) -> (RGBColor labelColor)"},
	{"PtInIconID", (PyCFunction)Icn_PtInIconID, 1,
	 "(Point testPt, Rect iconRect, IconAlignmentType align, SInt16 iconID) -> (Boolean _rv)"},
	{"PtInIconSuite", (PyCFunction)Icn_PtInIconSuite, 1,
	 "(Point testPt, Rect iconRect, IconAlignmentType align, IconSuiteRef theIconSuite) -> (Boolean _rv)"},
	{"RectInIconID", (PyCFunction)Icn_RectInIconID, 1,
	 "(Rect testRect, Rect iconRect, IconAlignmentType align, SInt16 iconID) -> (Boolean _rv)"},
	{"RectInIconSuite", (PyCFunction)Icn_RectInIconSuite, 1,
	 "(Rect testRect, Rect iconRect, IconAlignmentType align, IconSuiteRef theIconSuite) -> (Boolean _rv)"},
	{"IconIDToRgn", (PyCFunction)Icn_IconIDToRgn, 1,
	 "(RgnHandle theRgn, Rect iconRect, IconAlignmentType align, SInt16 iconID) -> None"},
	{"IconSuiteToRgn", (PyCFunction)Icn_IconSuiteToRgn, 1,
	 "(RgnHandle theRgn, Rect iconRect, IconAlignmentType align, IconSuiteRef theIconSuite) -> None"},
	{"SetSuiteLabel", (PyCFunction)Icn_SetSuiteLabel, 1,
	 "(IconSuiteRef theSuite, SInt16 theLabel) -> None"},
	{"GetSuiteLabel", (PyCFunction)Icn_GetSuiteLabel, 1,
	 "(IconSuiteRef theSuite) -> (SInt16 _rv)"},
	{"PlotIconHandle", (PyCFunction)Icn_PlotIconHandle, 1,
	 "(Rect theRect, IconAlignmentType align, IconTransformType transform, Handle theIcon) -> None"},
	{"PlotSICNHandle", (PyCFunction)Icn_PlotSICNHandle, 1,
	 "(Rect theRect, IconAlignmentType align, IconTransformType transform, Handle theSICN) -> None"},
	{"PlotCIconHandle", (PyCFunction)Icn_PlotCIconHandle, 1,
	 "(Rect theRect, IconAlignmentType align, IconTransformType transform, CIconHandle theCIcon) -> None"},
	{NULL, NULL, 0}
};




void initIcn()
{
	PyObject *m;
	PyObject *d;




	m = Py_InitModule("Icn", Icn_methods);
	d = PyModule_GetDict(m);
	Icn_Error = PyMac_GetOSErrException();
	if (Icn_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Icn_Error) != 0)
		Py_FatalError("can't initialize Icn.Error");
}

/* ========================= End module Icn ========================= */

