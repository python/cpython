
/* =========================== Module Fm ============================ */

#include "Python.h"



#include "macglue.h"
#include "pymactoolbox.h"

#ifdef WITHOUT_FRAMEWORKS
#include <Fonts.h>
#else
#include <Carbon/Carbon.h>
#endif


/*
** Parse/generate ComponentDescriptor records
*/
static PyObject *
FMRec_New(FMetricRec *itself)
{

	return Py_BuildValue("O&O&O&O&O&", 
		PyMac_BuildFixed, itself->ascent,
		PyMac_BuildFixed, itself->descent,
		PyMac_BuildFixed, itself->leading,
		PyMac_BuildFixed, itself->widMax,
		ResObj_New, itself->wTabHandle);
}

#if 0
/* Not needed... */
static int
FMRec_Convert(PyObject *v, FMetricRec *p_itself)
{
	return PyArg_ParseTuple(v, "O&O&O&O&O&",
		PyMac_GetFixed, &itself->ascent,
		PyMac_GetFixed, &itself->descent,
		PyMac_GetFixed, &itself->leading,
		PyMac_GetFixed, &itself->widMax,
		ResObj_Convert, &itself->wTabHandle);
}
#endif


static PyObject *Fm_Error;

#if !TARGET_API_MAC_CARBON

static PyObject *Fm_InitFonts(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	InitFonts();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

static PyObject *Fm_GetFontName(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short familyID;
	Str255 name;
	if (!PyArg_ParseTuple(_args, "h",
	                      &familyID))
		return NULL;
	GetFontName(familyID,
	            name);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildStr255, name);
	return _res;
}

static PyObject *Fm_GetFNum(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Str255 name;
	short familyID;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetStr255, name))
		return NULL;
	GetFNum(name,
	        &familyID);
	_res = Py_BuildValue("h",
	                     familyID);
	return _res;
}

static PyObject *Fm_RealFont(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	short fontNum;
	short size;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &fontNum,
	                      &size))
		return NULL;
	_rv = RealFont(fontNum,
	               size);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

#if !TARGET_API_MAC_CARBON

static PyObject *Fm_SetFontLock(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean lockFlag;
	if (!PyArg_ParseTuple(_args, "b",
	                      &lockFlag))
		return NULL;
	SetFontLock(lockFlag);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

static PyObject *Fm_SetFScaleDisable(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean fscaleDisable;
	if (!PyArg_ParseTuple(_args, "b",
	                      &fscaleDisable))
		return NULL;
	SetFScaleDisable(fscaleDisable);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Fm_FontMetrics(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	FMetricRec theMetrics;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	FontMetrics(&theMetrics);
	_res = Py_BuildValue("O&",
	                     FMRec_New, &theMetrics);
	return _res;
}

static PyObject *Fm_SetFractEnable(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean fractEnable;
	if (!PyArg_ParseTuple(_args, "b",
	                      &fractEnable))
		return NULL;
	SetFractEnable(fractEnable);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Fm_GetDefFontSize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetDefFontSize();
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Fm_IsOutline(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	Point numer;
	Point denom;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetPoint, &numer,
	                      PyMac_GetPoint, &denom))
		return NULL;
	_rv = IsOutline(numer,
	                denom);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Fm_SetOutlinePreferred(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean outlinePreferred;
	if (!PyArg_ParseTuple(_args, "b",
	                      &outlinePreferred))
		return NULL;
	SetOutlinePreferred(outlinePreferred);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Fm_GetOutlinePreferred(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetOutlinePreferred();
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Fm_SetPreserveGlyph(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean preserveGlyph;
	if (!PyArg_ParseTuple(_args, "b",
	                      &preserveGlyph))
		return NULL;
	SetPreserveGlyph(preserveGlyph);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Fm_GetPreserveGlyph(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetPreserveGlyph();
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

#if !TARGET_API_MAC_CARBON

static PyObject *Fm_FlushFonts(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = FlushFonts();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

static PyObject *Fm_GetSysFont(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetSysFont();
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Fm_GetAppFont(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetAppFont();
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyMethodDef Fm_methods[] = {

#if !TARGET_API_MAC_CARBON
	{"InitFonts", (PyCFunction)Fm_InitFonts, 1,
	 "() -> None"},
#endif
	{"GetFontName", (PyCFunction)Fm_GetFontName, 1,
	 "(short familyID) -> (Str255 name)"},
	{"GetFNum", (PyCFunction)Fm_GetFNum, 1,
	 "(Str255 name) -> (short familyID)"},
	{"RealFont", (PyCFunction)Fm_RealFont, 1,
	 "(short fontNum, short size) -> (Boolean _rv)"},

#if !TARGET_API_MAC_CARBON
	{"SetFontLock", (PyCFunction)Fm_SetFontLock, 1,
	 "(Boolean lockFlag) -> None"},
#endif
	{"SetFScaleDisable", (PyCFunction)Fm_SetFScaleDisable, 1,
	 "(Boolean fscaleDisable) -> None"},
	{"FontMetrics", (PyCFunction)Fm_FontMetrics, 1,
	 "() -> (FMetricRec theMetrics)"},
	{"SetFractEnable", (PyCFunction)Fm_SetFractEnable, 1,
	 "(Boolean fractEnable) -> None"},
	{"GetDefFontSize", (PyCFunction)Fm_GetDefFontSize, 1,
	 "() -> (short _rv)"},
	{"IsOutline", (PyCFunction)Fm_IsOutline, 1,
	 "(Point numer, Point denom) -> (Boolean _rv)"},
	{"SetOutlinePreferred", (PyCFunction)Fm_SetOutlinePreferred, 1,
	 "(Boolean outlinePreferred) -> None"},
	{"GetOutlinePreferred", (PyCFunction)Fm_GetOutlinePreferred, 1,
	 "() -> (Boolean _rv)"},
	{"SetPreserveGlyph", (PyCFunction)Fm_SetPreserveGlyph, 1,
	 "(Boolean preserveGlyph) -> None"},
	{"GetPreserveGlyph", (PyCFunction)Fm_GetPreserveGlyph, 1,
	 "() -> (Boolean _rv)"},

#if !TARGET_API_MAC_CARBON
	{"FlushFonts", (PyCFunction)Fm_FlushFonts, 1,
	 "() -> None"},
#endif
	{"GetSysFont", (PyCFunction)Fm_GetSysFont, 1,
	 "() -> (short _rv)"},
	{"GetAppFont", (PyCFunction)Fm_GetAppFont, 1,
	 "() -> (short _rv)"},
	{NULL, NULL, 0}
};




void initFm(void)
{
	PyObject *m;
	PyObject *d;




	m = Py_InitModule("Fm", Fm_methods);
	d = PyModule_GetDict(m);
	Fm_Error = PyMac_GetOSErrException();
	if (Fm_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Fm_Error) != 0)
		return;
}

/* ========================= End module Fm ========================== */

