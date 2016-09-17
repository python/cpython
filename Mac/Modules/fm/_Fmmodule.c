
/* =========================== Module _Fm =========================== */

#include "Python.h"
#include <Carbon/Carbon.h>

#if !defined(__LP64__) && !defined(MAC_OS_X_VERSION_10_7)


#include "pymactoolbox.h"

/* Macro to test whether a weak-loaded CFM function exists */
#define PyMac_PRECHECK(rtn) do { if ( &rtn == NULL )  {\
        PyErr_SetString(PyExc_NotImplementedError, \
            "Not available in this shared library/OS version"); \
        return NULL; \
    }} while(0)




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

static PyObject *Fm_GetFontName(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    short familyID;
    Str255 name;
#ifndef GetFontName
    PyMac_PRECHECK(GetFontName);
#endif
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
#ifndef GetFNum
    PyMac_PRECHECK(GetFNum);
#endif
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
#ifndef RealFont
    PyMac_PRECHECK(RealFont);
#endif
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

static PyObject *Fm_SetFScaleDisable(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean fscaleDisable;
#ifndef SetFScaleDisable
    PyMac_PRECHECK(SetFScaleDisable);
#endif
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
#ifndef FontMetrics
    PyMac_PRECHECK(FontMetrics);
#endif
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
#ifndef SetFractEnable
    PyMac_PRECHECK(SetFractEnable);
#endif
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
#ifndef GetDefFontSize
    PyMac_PRECHECK(GetDefFontSize);
#endif
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
#ifndef IsOutline
    PyMac_PRECHECK(IsOutline);
#endif
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
#ifndef SetOutlinePreferred
    PyMac_PRECHECK(SetOutlinePreferred);
#endif
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
#ifndef GetOutlinePreferred
    PyMac_PRECHECK(GetOutlinePreferred);
#endif
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
#ifndef SetPreserveGlyph
    PyMac_PRECHECK(SetPreserveGlyph);
#endif
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
#ifndef GetPreserveGlyph
    PyMac_PRECHECK(GetPreserveGlyph);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetPreserveGlyph();
    _res = Py_BuildValue("b",
                         _rv);
    return _res;
}

static PyObject *Fm_GetSysFont(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    short _rv;
#ifndef GetSysFont
    PyMac_PRECHECK(GetSysFont);
#endif
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
#ifndef GetAppFont
    PyMac_PRECHECK(GetAppFont);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetAppFont();
    _res = Py_BuildValue("h",
                         _rv);
    return _res;
}

static PyObject *Fm_QDTextBounds(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    char *inText__in__;
    int inText__len__;
    int inText__in_len__;
    Rect bounds;
#ifndef QDTextBounds
    PyMac_PRECHECK(QDTextBounds);
#endif
    if (!PyArg_ParseTuple(_args, "s#",
                          &inText__in__, &inText__in_len__))
        return NULL;
    inText__len__ = inText__in_len__;
    QDTextBounds(inText__len__, inText__in__,
                 &bounds);
    _res = Py_BuildValue("O&",
                         PyMac_BuildRect, &bounds);
    return _res;
}

static PyMethodDef Fm_methods[] = {
    {"GetFontName", (PyCFunction)Fm_GetFontName, 1,
     PyDoc_STR("(short familyID) -> (Str255 name)")},
    {"GetFNum", (PyCFunction)Fm_GetFNum, 1,
     PyDoc_STR("(Str255 name) -> (short familyID)")},
    {"RealFont", (PyCFunction)Fm_RealFont, 1,
     PyDoc_STR("(short fontNum, short size) -> (Boolean _rv)")},
    {"SetFScaleDisable", (PyCFunction)Fm_SetFScaleDisable, 1,
     PyDoc_STR("(Boolean fscaleDisable) -> None")},
    {"FontMetrics", (PyCFunction)Fm_FontMetrics, 1,
     PyDoc_STR("() -> (FMetricRec theMetrics)")},
    {"SetFractEnable", (PyCFunction)Fm_SetFractEnable, 1,
     PyDoc_STR("(Boolean fractEnable) -> None")},
    {"GetDefFontSize", (PyCFunction)Fm_GetDefFontSize, 1,
     PyDoc_STR("() -> (short _rv)")},
    {"IsOutline", (PyCFunction)Fm_IsOutline, 1,
     PyDoc_STR("(Point numer, Point denom) -> (Boolean _rv)")},
    {"SetOutlinePreferred", (PyCFunction)Fm_SetOutlinePreferred, 1,
     PyDoc_STR("(Boolean outlinePreferred) -> None")},
    {"GetOutlinePreferred", (PyCFunction)Fm_GetOutlinePreferred, 1,
     PyDoc_STR("() -> (Boolean _rv)")},
    {"SetPreserveGlyph", (PyCFunction)Fm_SetPreserveGlyph, 1,
     PyDoc_STR("(Boolean preserveGlyph) -> None")},
    {"GetPreserveGlyph", (PyCFunction)Fm_GetPreserveGlyph, 1,
     PyDoc_STR("() -> (Boolean _rv)")},
    {"GetSysFont", (PyCFunction)Fm_GetSysFont, 1,
     PyDoc_STR("() -> (short _rv)")},
    {"GetAppFont", (PyCFunction)Fm_GetAppFont, 1,
     PyDoc_STR("() -> (short _rv)")},
    {"QDTextBounds", (PyCFunction)Fm_QDTextBounds, 1,
     PyDoc_STR("(Buffer inText) -> (Rect bounds)")},
    {NULL, NULL, 0}
};

#else  /* __LP64__ */

static PyMethodDef Fm_methods[] = {
    {NULL, NULL, 0}
};

#endif  /* __LP64__ */

void init_Fm(void)
{
    PyObject *m;
#if !defined(__LP64__) && !defined(MAC_OS_X_VERSION_10_7)
    PyObject *d;
#endif  /* __LP64__ */




    m = Py_InitModule("_Fm", Fm_methods);
#if !defined(__LP64__) && !defined(MAC_OS_X_VERSION_10_7)
    d = PyModule_GetDict(m);
    Fm_Error = PyMac_GetOSErrException();
    if (Fm_Error == NULL ||
        PyDict_SetItemString(d, "Error", Fm_Error) != 0)
        return;
#endif  /* __LP64__ */
}

/* ========================= End module _Fm ========================= */

