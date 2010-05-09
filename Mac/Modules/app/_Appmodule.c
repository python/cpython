
/* ========================== Module _App =========================== */

#include "Python.h"

#ifndef __LP64__
    /* Carbon GUI stuff, not available in 64-bit mode */


#include "pymactoolbox.h"

/* Macro to test whether a weak-loaded CFM function exists */
#define PyMac_PRECHECK(rtn) do { if ( &rtn == NULL )  {\
    PyErr_SetString(PyExc_NotImplementedError, \
    "Not available in this shared library/OS version"); \
    return NULL; \
    }} while(0)


#include <Carbon/Carbon.h>


static int ThemeButtonDrawInfo_Convert(PyObject *v, ThemeButtonDrawInfo *p_itself)
{
    return PyArg_Parse(v, "(iHH)", &p_itself->state, &p_itself->value, &p_itself->adornment);
}


static PyObject *App_Error;

/* ----------------- Object type ThemeDrawingState ------------------ */

PyTypeObject ThemeDrawingState_Type;

#define ThemeDrawingStateObj_Check(x) ((x)->ob_type == &ThemeDrawingState_Type || PyObject_TypeCheck((x), &ThemeDrawingState_Type))

typedef struct ThemeDrawingStateObject {
    PyObject_HEAD
    ThemeDrawingState ob_itself;
} ThemeDrawingStateObject;

PyObject *ThemeDrawingStateObj_New(ThemeDrawingState itself)
{
    ThemeDrawingStateObject *it;
    it = PyObject_NEW(ThemeDrawingStateObject, &ThemeDrawingState_Type);
    if (it == NULL) return NULL;
    it->ob_itself = itself;
    return (PyObject *)it;
}

int ThemeDrawingStateObj_Convert(PyObject *v, ThemeDrawingState *p_itself)
{
    if (!ThemeDrawingStateObj_Check(v))
    {
        PyErr_SetString(PyExc_TypeError, "ThemeDrawingState required");
        return 0;
    }
    *p_itself = ((ThemeDrawingStateObject *)v)->ob_itself;
    return 1;
}

static void ThemeDrawingStateObj_dealloc(ThemeDrawingStateObject *self)
{
    /* Cleanup of self->ob_itself goes here */
    self->ob_type->tp_free((PyObject *)self);
}

static PyObject *ThemeDrawingStateObj_SetThemeDrawingState(ThemeDrawingStateObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _rv;
    Boolean inDisposeNow;
#ifndef SetThemeDrawingState
    PyMac_PRECHECK(SetThemeDrawingState);
#endif
    if (!PyArg_ParseTuple(_args, "b",
                          &inDisposeNow))
        return NULL;
    _rv = SetThemeDrawingState(_self->ob_itself,
                               inDisposeNow);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *ThemeDrawingStateObj_DisposeThemeDrawingState(ThemeDrawingStateObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _rv;
#ifndef DisposeThemeDrawingState
    PyMac_PRECHECK(DisposeThemeDrawingState);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = DisposeThemeDrawingState(_self->ob_itself);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyMethodDef ThemeDrawingStateObj_methods[] = {
    {"SetThemeDrawingState", (PyCFunction)ThemeDrawingStateObj_SetThemeDrawingState, 1,
     PyDoc_STR("(Boolean inDisposeNow) -> (OSStatus _rv)")},
    {"DisposeThemeDrawingState", (PyCFunction)ThemeDrawingStateObj_DisposeThemeDrawingState, 1,
     PyDoc_STR("() -> (OSStatus _rv)")},
    {NULL, NULL, 0}
};

#define ThemeDrawingStateObj_getsetlist NULL


#define ThemeDrawingStateObj_compare NULL

#define ThemeDrawingStateObj_repr NULL

#define ThemeDrawingStateObj_hash NULL
#define ThemeDrawingStateObj_tp_init 0

#define ThemeDrawingStateObj_tp_alloc PyType_GenericAlloc

static PyObject *ThemeDrawingStateObj_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
    PyObject *_self;
    ThemeDrawingState itself;
    char *kw[] = {"itself", 0};

    if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, ThemeDrawingStateObj_Convert, &itself)) return NULL;
    if ((_self = type->tp_alloc(type, 0)) == NULL) return NULL;
    ((ThemeDrawingStateObject *)_self)->ob_itself = itself;
    return _self;
}

#define ThemeDrawingStateObj_tp_free PyObject_Del


PyTypeObject ThemeDrawingState_Type = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "_App.ThemeDrawingState", /*tp_name*/
    sizeof(ThemeDrawingStateObject), /*tp_basicsize*/
    0, /*tp_itemsize*/
    /* methods */
    (destructor) ThemeDrawingStateObj_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)0, /*tp_getattr*/
    (setattrfunc)0, /*tp_setattr*/
    (cmpfunc) ThemeDrawingStateObj_compare, /*tp_compare*/
    (reprfunc) ThemeDrawingStateObj_repr, /*tp_repr*/
    (PyNumberMethods *)0, /* tp_as_number */
    (PySequenceMethods *)0, /* tp_as_sequence */
    (PyMappingMethods *)0, /* tp_as_mapping */
    (hashfunc) ThemeDrawingStateObj_hash, /*tp_hash*/
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
    ThemeDrawingStateObj_methods, /* tp_methods */
    0, /*tp_members*/
    ThemeDrawingStateObj_getsetlist, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    ThemeDrawingStateObj_tp_init, /* tp_init */
    ThemeDrawingStateObj_tp_alloc, /* tp_alloc */
    ThemeDrawingStateObj_tp_new, /* tp_new */
    ThemeDrawingStateObj_tp_free, /* tp_free */
};

/* --------------- End object type ThemeDrawingState ---------------- */


static PyObject *App_RegisterAppearanceClient(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
#ifndef RegisterAppearanceClient
    PyMac_PRECHECK(RegisterAppearanceClient);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = RegisterAppearanceClient();
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *App_UnregisterAppearanceClient(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
#ifndef UnregisterAppearanceClient
    PyMac_PRECHECK(UnregisterAppearanceClient);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = UnregisterAppearanceClient();
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *App_SetThemePen(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    ThemeBrush inBrush;
    SInt16 inDepth;
    Boolean inIsColorDevice;
#ifndef SetThemePen
    PyMac_PRECHECK(SetThemePen);
#endif
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

static PyObject *App_SetThemeBackground(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    ThemeBrush inBrush;
    SInt16 inDepth;
    Boolean inIsColorDevice;
#ifndef SetThemeBackground
    PyMac_PRECHECK(SetThemeBackground);
#endif
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

static PyObject *App_SetThemeTextColor(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    ThemeTextColor inColor;
    SInt16 inDepth;
    Boolean inIsColorDevice;
#ifndef SetThemeTextColor
    PyMac_PRECHECK(SetThemeTextColor);
#endif
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

static PyObject *App_SetThemeWindowBackground(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr inWindow;
    ThemeBrush inBrush;
    Boolean inUpdate;
#ifndef SetThemeWindowBackground
    PyMac_PRECHECK(SetThemeWindowBackground);
#endif
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

static PyObject *App_DrawThemeWindowHeader(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect inRect;
    ThemeDrawState inState;
#ifndef DrawThemeWindowHeader
    PyMac_PRECHECK(DrawThemeWindowHeader);
#endif
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

static PyObject *App_DrawThemeWindowListViewHeader(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect inRect;
    ThemeDrawState inState;
#ifndef DrawThemeWindowListViewHeader
    PyMac_PRECHECK(DrawThemeWindowListViewHeader);
#endif
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

static PyObject *App_DrawThemePlacard(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect inRect;
    ThemeDrawState inState;
#ifndef DrawThemePlacard
    PyMac_PRECHECK(DrawThemePlacard);
#endif
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

static PyObject *App_DrawThemeEditTextFrame(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect inRect;
    ThemeDrawState inState;
#ifndef DrawThemeEditTextFrame
    PyMac_PRECHECK(DrawThemeEditTextFrame);
#endif
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

static PyObject *App_DrawThemeListBoxFrame(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect inRect;
    ThemeDrawState inState;
#ifndef DrawThemeListBoxFrame
    PyMac_PRECHECK(DrawThemeListBoxFrame);
#endif
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

static PyObject *App_DrawThemeFocusRect(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect inRect;
    Boolean inHasFocus;
#ifndef DrawThemeFocusRect
    PyMac_PRECHECK(DrawThemeFocusRect);
#endif
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

static PyObject *App_DrawThemePrimaryGroup(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect inRect;
    ThemeDrawState inState;
#ifndef DrawThemePrimaryGroup
    PyMac_PRECHECK(DrawThemePrimaryGroup);
#endif
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

static PyObject *App_DrawThemeSecondaryGroup(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect inRect;
    ThemeDrawState inState;
#ifndef DrawThemeSecondaryGroup
    PyMac_PRECHECK(DrawThemeSecondaryGroup);
#endif
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

static PyObject *App_DrawThemeSeparator(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect inRect;
    ThemeDrawState inState;
#ifndef DrawThemeSeparator
    PyMac_PRECHECK(DrawThemeSeparator);
#endif
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

static PyObject *App_DrawThemeModelessDialogFrame(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect inRect;
    ThemeDrawState inState;
#ifndef DrawThemeModelessDialogFrame
    PyMac_PRECHECK(DrawThemeModelessDialogFrame);
#endif
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

static PyObject *App_DrawThemeGenericWell(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect inRect;
    ThemeDrawState inState;
    Boolean inFillCenter;
#ifndef DrawThemeGenericWell
    PyMac_PRECHECK(DrawThemeGenericWell);
#endif
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

static PyObject *App_DrawThemeFocusRegion(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Boolean inHasFocus;
#ifndef DrawThemeFocusRegion
    PyMac_PRECHECK(DrawThemeFocusRegion);
#endif
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

static PyObject *App_IsThemeInColor(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
    SInt16 inDepth;
    Boolean inIsColorDevice;
#ifndef IsThemeInColor
    PyMac_PRECHECK(IsThemeInColor);
#endif
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

static PyObject *App_GetThemeAccentColors(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    CTabHandle outColors;
#ifndef GetThemeAccentColors
    PyMac_PRECHECK(GetThemeAccentColors);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetThemeAccentColors(&outColors);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         ResObj_New, outColors);
    return _res;
}

static PyObject *App_DrawThemeMenuBarBackground(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect inBounds;
    ThemeMenuBarState inState;
    UInt32 inAttributes;
#ifndef DrawThemeMenuBarBackground
    PyMac_PRECHECK(DrawThemeMenuBarBackground);
#endif
    if (!PyArg_ParseTuple(_args, "O&Hl",
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

static PyObject *App_GetThemeMenuBarHeight(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    SInt16 outHeight;
#ifndef GetThemeMenuBarHeight
    PyMac_PRECHECK(GetThemeMenuBarHeight);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetThemeMenuBarHeight(&outHeight);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("h",
                         outHeight);
    return _res;
}

static PyObject *App_DrawThemeMenuBackground(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect inMenuRect;
    ThemeMenuType inMenuType;
#ifndef DrawThemeMenuBackground
    PyMac_PRECHECK(DrawThemeMenuBackground);
#endif
    if (!PyArg_ParseTuple(_args, "O&H",
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

static PyObject *App_GetThemeMenuBackgroundRegion(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect inMenuRect;
    ThemeMenuType menuType;
#ifndef GetThemeMenuBackgroundRegion
    PyMac_PRECHECK(GetThemeMenuBackgroundRegion);
#endif
    if (!PyArg_ParseTuple(_args, "O&H",
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

static PyObject *App_DrawThemeMenuSeparator(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect inItemRect;
#ifndef DrawThemeMenuSeparator
    PyMac_PRECHECK(DrawThemeMenuSeparator);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          PyMac_GetRect, &inItemRect))
        return NULL;
    _err = DrawThemeMenuSeparator(&inItemRect);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *App_GetThemeMenuSeparatorHeight(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    SInt16 outHeight;
#ifndef GetThemeMenuSeparatorHeight
    PyMac_PRECHECK(GetThemeMenuSeparatorHeight);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetThemeMenuSeparatorHeight(&outHeight);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("h",
                         outHeight);
    return _res;
}

static PyObject *App_GetThemeMenuItemExtra(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    ThemeMenuItemType inItemType;
    SInt16 outHeight;
    SInt16 outWidth;
#ifndef GetThemeMenuItemExtra
    PyMac_PRECHECK(GetThemeMenuItemExtra);
#endif
    if (!PyArg_ParseTuple(_args, "H",
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

static PyObject *App_GetThemeMenuTitleExtra(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    SInt16 outWidth;
    Boolean inIsSquished;
#ifndef GetThemeMenuTitleExtra
    PyMac_PRECHECK(GetThemeMenuTitleExtra);
#endif
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

static PyObject *App_DrawThemeTabPane(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect inRect;
    ThemeDrawState inState;
#ifndef DrawThemeTabPane
    PyMac_PRECHECK(DrawThemeTabPane);
#endif
    if (!PyArg_ParseTuple(_args, "O&l",
                          PyMac_GetRect, &inRect,
                          &inState))
        return NULL;
    _err = DrawThemeTabPane(&inRect,
                            inState);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *App_GetThemeTabRegion(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect inRect;
    ThemeTabStyle inStyle;
    ThemeTabDirection inDirection;
#ifndef GetThemeTabRegion
    PyMac_PRECHECK(GetThemeTabRegion);
#endif
    if (!PyArg_ParseTuple(_args, "O&HH",
                          PyMac_GetRect, &inRect,
                          &inStyle,
                          &inDirection))
        return NULL;
    _err = GetThemeTabRegion(&inRect,
                             inStyle,
                             inDirection,
                             (RgnHandle)0);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *App_SetThemeCursor(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    ThemeCursor inCursor;
#ifndef SetThemeCursor
    PyMac_PRECHECK(SetThemeCursor);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &inCursor))
        return NULL;
    _err = SetThemeCursor(inCursor);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *App_SetAnimatedThemeCursor(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    ThemeCursor inCursor;
    UInt32 inAnimationStep;
#ifndef SetAnimatedThemeCursor
    PyMac_PRECHECK(SetAnimatedThemeCursor);
#endif
    if (!PyArg_ParseTuple(_args, "ll",
                          &inCursor,
                          &inAnimationStep))
        return NULL;
    _err = SetAnimatedThemeCursor(inCursor,
                                  inAnimationStep);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *App_GetThemeScrollBarThumbStyle(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    ThemeScrollBarThumbStyle outStyle;
#ifndef GetThemeScrollBarThumbStyle
    PyMac_PRECHECK(GetThemeScrollBarThumbStyle);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetThemeScrollBarThumbStyle(&outStyle);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("H",
                         outStyle);
    return _res;
}

static PyObject *App_GetThemeScrollBarArrowStyle(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    ThemeScrollBarArrowStyle outStyle;
#ifndef GetThemeScrollBarArrowStyle
    PyMac_PRECHECK(GetThemeScrollBarArrowStyle);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetThemeScrollBarArrowStyle(&outStyle);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("H",
                         outStyle);
    return _res;
}

static PyObject *App_GetThemeCheckBoxStyle(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    ThemeCheckBoxStyle outStyle;
#ifndef GetThemeCheckBoxStyle
    PyMac_PRECHECK(GetThemeCheckBoxStyle);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetThemeCheckBoxStyle(&outStyle);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("H",
                         outStyle);
    return _res;
}

static PyObject *App_UseThemeFont(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    ThemeFontID inFontID;
    ScriptCode inScript;
#ifndef UseThemeFont
    PyMac_PRECHECK(UseThemeFont);
#endif
    if (!PyArg_ParseTuple(_args, "Hh",
                          &inFontID,
                          &inScript))
        return NULL;
    _err = UseThemeFont(inFontID,
                        inScript);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *App_DrawThemeTextBox(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    CFStringRef inString;
    ThemeFontID inFontID;
    ThemeDrawState inState;
    Boolean inWrapToWidth;
    Rect inBoundingBox;
    SInt16 inJust;
#ifndef DrawThemeTextBox
    PyMac_PRECHECK(DrawThemeTextBox);
#endif
    if (!PyArg_ParseTuple(_args, "O&HlbO&h",
                          CFStringRefObj_Convert, &inString,
                          &inFontID,
                          &inState,
                          &inWrapToWidth,
                          PyMac_GetRect, &inBoundingBox,
                          &inJust))
        return NULL;
    _err = DrawThemeTextBox(inString,
                            inFontID,
                            inState,
                            inWrapToWidth,
                            &inBoundingBox,
                            inJust,
                            NULL);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *App_TruncateThemeText(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    CFMutableStringRef inString;
    ThemeFontID inFontID;
    ThemeDrawState inState;
    SInt16 inPixelWidthLimit;
    TruncCode inTruncWhere;
    Boolean outTruncated;
#ifndef TruncateThemeText
    PyMac_PRECHECK(TruncateThemeText);
#endif
    if (!PyArg_ParseTuple(_args, "O&Hlhh",
                          CFMutableStringRefObj_Convert, &inString,
                          &inFontID,
                          &inState,
                          &inPixelWidthLimit,
                          &inTruncWhere))
        return NULL;
    _err = TruncateThemeText(inString,
                             inFontID,
                             inState,
                             inPixelWidthLimit,
                             inTruncWhere,
                             &outTruncated);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("b",
                         outTruncated);
    return _res;
}

static PyObject *App_GetThemeTextDimensions(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    CFStringRef inString;
    ThemeFontID inFontID;
    ThemeDrawState inState;
    Boolean inWrapToWidth;
    Point ioBounds;
    SInt16 outBaseline;
#ifndef GetThemeTextDimensions
    PyMac_PRECHECK(GetThemeTextDimensions);
#endif
    if (!PyArg_ParseTuple(_args, "O&HlbO&",
                          CFStringRefObj_Convert, &inString,
                          &inFontID,
                          &inState,
                          &inWrapToWidth,
                          PyMac_GetPoint, &ioBounds))
        return NULL;
    _err = GetThemeTextDimensions(inString,
                                  inFontID,
                                  inState,
                                  inWrapToWidth,
                                  &ioBounds,
                                  &outBaseline);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&h",
                         PyMac_BuildPoint, ioBounds,
                         outBaseline);
    return _res;
}

static PyObject *App_GetThemeTextShadowOutset(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    ThemeFontID inFontID;
    ThemeDrawState inState;
    Rect outOutset;
#ifndef GetThemeTextShadowOutset
    PyMac_PRECHECK(GetThemeTextShadowOutset);
#endif
    if (!PyArg_ParseTuple(_args, "Hl",
                          &inFontID,
                          &inState))
        return NULL;
    _err = GetThemeTextShadowOutset(inFontID,
                                    inState,
                                    &outOutset);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         PyMac_BuildRect, &outOutset);
    return _res;
}

static PyObject *App_DrawThemeScrollBarArrows(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect bounds;
    ThemeTrackEnableState enableState;
    ThemeTrackPressState pressState;
    Boolean isHoriz;
    Rect trackBounds;
#ifndef DrawThemeScrollBarArrows
    PyMac_PRECHECK(DrawThemeScrollBarArrows);
#endif
    if (!PyArg_ParseTuple(_args, "O&bbb",
                          PyMac_GetRect, &bounds,
                          &enableState,
                          &pressState,
                          &isHoriz))
        return NULL;
    _err = DrawThemeScrollBarArrows(&bounds,
                                    enableState,
                                    pressState,
                                    isHoriz,
                                    &trackBounds);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         PyMac_BuildRect, &trackBounds);
    return _res;
}

static PyObject *App_GetThemeScrollBarTrackRect(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect bounds;
    ThemeTrackEnableState enableState;
    ThemeTrackPressState pressState;
    Boolean isHoriz;
    Rect trackBounds;
#ifndef GetThemeScrollBarTrackRect
    PyMac_PRECHECK(GetThemeScrollBarTrackRect);
#endif
    if (!PyArg_ParseTuple(_args, "O&bbb",
                          PyMac_GetRect, &bounds,
                          &enableState,
                          &pressState,
                          &isHoriz))
        return NULL;
    _err = GetThemeScrollBarTrackRect(&bounds,
                                      enableState,
                                      pressState,
                                      isHoriz,
                                      &trackBounds);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         PyMac_BuildRect, &trackBounds);
    return _res;
}

static PyObject *App_HitTestThemeScrollBarArrows(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
    Rect scrollBarBounds;
    ThemeTrackEnableState enableState;
    ThemeTrackPressState pressState;
    Boolean isHoriz;
    Point ptHit;
    Rect trackBounds;
    ControlPartCode partcode;
#ifndef HitTestThemeScrollBarArrows
    PyMac_PRECHECK(HitTestThemeScrollBarArrows);
#endif
    if (!PyArg_ParseTuple(_args, "O&bbbO&",
                          PyMac_GetRect, &scrollBarBounds,
                          &enableState,
                          &pressState,
                          &isHoriz,
                          PyMac_GetPoint, &ptHit))
        return NULL;
    _rv = HitTestThemeScrollBarArrows(&scrollBarBounds,
                                      enableState,
                                      pressState,
                                      isHoriz,
                                      ptHit,
                                      &trackBounds,
                                      &partcode);
    _res = Py_BuildValue("bO&h",
                         _rv,
                         PyMac_BuildRect, &trackBounds,
                         partcode);
    return _res;
}

static PyObject *App_DrawThemeScrollBarDelimiters(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    ThemeWindowType flavor;
    Rect inContRect;
    ThemeDrawState state;
    ThemeWindowAttributes attributes;
#ifndef DrawThemeScrollBarDelimiters
    PyMac_PRECHECK(DrawThemeScrollBarDelimiters);
#endif
    if (!PyArg_ParseTuple(_args, "HO&ll",
                          &flavor,
                          PyMac_GetRect, &inContRect,
                          &state,
                          &attributes))
        return NULL;
    _err = DrawThemeScrollBarDelimiters(flavor,
                                        &inContRect,
                                        state,
                                        attributes);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *App_DrawThemeButton(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect inBounds;
    UInt16 inKind;
    ThemeButtonDrawInfo inNewInfo;
    ThemeButtonDrawInfo inPrevInfo;
    UInt32 inUserData;
#ifndef DrawThemeButton
    PyMac_PRECHECK(DrawThemeButton);
#endif
    if (!PyArg_ParseTuple(_args, "O&HO&O&l",
                          PyMac_GetRect, &inBounds,
                          &inKind,
                          ThemeButtonDrawInfo_Convert, &inNewInfo,
                          ThemeButtonDrawInfo_Convert, &inPrevInfo,
                          &inUserData))
        return NULL;
    _err = DrawThemeButton(&inBounds,
                           inKind,
                           &inNewInfo,
                           &inPrevInfo,
                           NULL,
                           NULL,
                           inUserData);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *App_GetThemeButtonRegion(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect inBounds;
    UInt16 inKind;
    ThemeButtonDrawInfo inNewInfo;
#ifndef GetThemeButtonRegion
    PyMac_PRECHECK(GetThemeButtonRegion);
#endif
    if (!PyArg_ParseTuple(_args, "O&HO&",
                          PyMac_GetRect, &inBounds,
                          &inKind,
                          ThemeButtonDrawInfo_Convert, &inNewInfo))
        return NULL;
    _err = GetThemeButtonRegion(&inBounds,
                                inKind,
                                &inNewInfo,
                                (RgnHandle)0);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *App_GetThemeButtonContentBounds(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect inBounds;
    UInt16 inKind;
    ThemeButtonDrawInfo inDrawInfo;
    Rect outBounds;
#ifndef GetThemeButtonContentBounds
    PyMac_PRECHECK(GetThemeButtonContentBounds);
#endif
    if (!PyArg_ParseTuple(_args, "O&HO&",
                          PyMac_GetRect, &inBounds,
                          &inKind,
                          ThemeButtonDrawInfo_Convert, &inDrawInfo))
        return NULL;
    _err = GetThemeButtonContentBounds(&inBounds,
                                       inKind,
                                       &inDrawInfo,
                                       &outBounds);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         PyMac_BuildRect, &outBounds);
    return _res;
}

static PyObject *App_GetThemeButtonBackgroundBounds(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect inBounds;
    UInt16 inKind;
    ThemeButtonDrawInfo inDrawInfo;
    Rect outBounds;
#ifndef GetThemeButtonBackgroundBounds
    PyMac_PRECHECK(GetThemeButtonBackgroundBounds);
#endif
    if (!PyArg_ParseTuple(_args, "O&HO&",
                          PyMac_GetRect, &inBounds,
                          &inKind,
                          ThemeButtonDrawInfo_Convert, &inDrawInfo))
        return NULL;
    _err = GetThemeButtonBackgroundBounds(&inBounds,
                                          inKind,
                                          &inDrawInfo,
                                          &outBounds);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         PyMac_BuildRect, &outBounds);
    return _res;
}

static PyObject *App_PlayThemeSound(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    ThemeSoundKind kind;
#ifndef PlayThemeSound
    PyMac_PRECHECK(PlayThemeSound);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          PyMac_GetOSType, &kind))
        return NULL;
    _err = PlayThemeSound(kind);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *App_BeginThemeDragSound(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    ThemeDragSoundKind kind;
#ifndef BeginThemeDragSound
    PyMac_PRECHECK(BeginThemeDragSound);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          PyMac_GetOSType, &kind))
        return NULL;
    _err = BeginThemeDragSound(kind);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *App_EndThemeDragSound(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
#ifndef EndThemeDragSound
    PyMac_PRECHECK(EndThemeDragSound);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = EndThemeDragSound();
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *App_DrawThemeTickMark(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect bounds;
    ThemeDrawState state;
#ifndef DrawThemeTickMark
    PyMac_PRECHECK(DrawThemeTickMark);
#endif
    if (!PyArg_ParseTuple(_args, "O&l",
                          PyMac_GetRect, &bounds,
                          &state))
        return NULL;
    _err = DrawThemeTickMark(&bounds,
                             state);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *App_DrawThemeChasingArrows(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect bounds;
    UInt32 index;
    ThemeDrawState state;
    UInt32 eraseData;
#ifndef DrawThemeChasingArrows
    PyMac_PRECHECK(DrawThemeChasingArrows);
#endif
    if (!PyArg_ParseTuple(_args, "O&lll",
                          PyMac_GetRect, &bounds,
                          &index,
                          &state,
                          &eraseData))
        return NULL;
    _err = DrawThemeChasingArrows(&bounds,
                                  index,
                                  state,
                                  NULL,
                                  eraseData);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *App_DrawThemePopupArrow(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect bounds;
    ThemeArrowOrientation orientation;
    ThemePopupArrowSize size;
    ThemeDrawState state;
    UInt32 eraseData;
#ifndef DrawThemePopupArrow
    PyMac_PRECHECK(DrawThemePopupArrow);
#endif
    if (!PyArg_ParseTuple(_args, "O&HHll",
                          PyMac_GetRect, &bounds,
                          &orientation,
                          &size,
                          &state,
                          &eraseData))
        return NULL;
    _err = DrawThemePopupArrow(&bounds,
                               orientation,
                               size,
                               state,
                               NULL,
                               eraseData);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *App_DrawThemeStandaloneGrowBox(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Point origin;
    ThemeGrowDirection growDirection;
    Boolean isSmall;
    ThemeDrawState state;
#ifndef DrawThemeStandaloneGrowBox
    PyMac_PRECHECK(DrawThemeStandaloneGrowBox);
#endif
    if (!PyArg_ParseTuple(_args, "O&Hbl",
                          PyMac_GetPoint, &origin,
                          &growDirection,
                          &isSmall,
                          &state))
        return NULL;
    _err = DrawThemeStandaloneGrowBox(origin,
                                      growDirection,
                                      isSmall,
                                      state);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *App_DrawThemeStandaloneNoGrowBox(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Point origin;
    ThemeGrowDirection growDirection;
    Boolean isSmall;
    ThemeDrawState state;
#ifndef DrawThemeStandaloneNoGrowBox
    PyMac_PRECHECK(DrawThemeStandaloneNoGrowBox);
#endif
    if (!PyArg_ParseTuple(_args, "O&Hbl",
                          PyMac_GetPoint, &origin,
                          &growDirection,
                          &isSmall,
                          &state))
        return NULL;
    _err = DrawThemeStandaloneNoGrowBox(origin,
                                        growDirection,
                                        isSmall,
                                        state);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *App_GetThemeStandaloneGrowBoxBounds(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Point origin;
    ThemeGrowDirection growDirection;
    Boolean isSmall;
    Rect bounds;
#ifndef GetThemeStandaloneGrowBoxBounds
    PyMac_PRECHECK(GetThemeStandaloneGrowBoxBounds);
#endif
    if (!PyArg_ParseTuple(_args, "O&Hb",
                          PyMac_GetPoint, &origin,
                          &growDirection,
                          &isSmall))
        return NULL;
    _err = GetThemeStandaloneGrowBoxBounds(origin,
                                           growDirection,
                                           isSmall,
                                           &bounds);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         PyMac_BuildRect, &bounds);
    return _res;
}

static PyObject *App_NormalizeThemeDrawingState(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
#ifndef NormalizeThemeDrawingState
    PyMac_PRECHECK(NormalizeThemeDrawingState);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = NormalizeThemeDrawingState();
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *App_GetThemeDrawingState(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    ThemeDrawingState outState;
#ifndef GetThemeDrawingState
    PyMac_PRECHECK(GetThemeDrawingState);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetThemeDrawingState(&outState);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         ThemeDrawingStateObj_New, outState);
    return _res;
}

static PyObject *App_ApplyThemeBackground(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    ThemeBackgroundKind inKind;
    Rect bounds;
    ThemeDrawState inState;
    SInt16 inDepth;
    Boolean inColorDev;
#ifndef ApplyThemeBackground
    PyMac_PRECHECK(ApplyThemeBackground);
#endif
    if (!PyArg_ParseTuple(_args, "lO&lhb",
                          &inKind,
                          PyMac_GetRect, &bounds,
                          &inState,
                          &inDepth,
                          &inColorDev))
        return NULL;
    _err = ApplyThemeBackground(inKind,
                                &bounds,
                                inState,
                                inDepth,
                                inColorDev);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *App_SetThemeTextColorForWindow(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Boolean isActive;
    SInt16 depth;
    Boolean isColorDev;
#ifndef SetThemeTextColorForWindow
    PyMac_PRECHECK(SetThemeTextColorForWindow);
#endif
    if (!PyArg_ParseTuple(_args, "O&bhb",
                          WinObj_Convert, &window,
                          &isActive,
                          &depth,
                          &isColorDev))
        return NULL;
    _err = SetThemeTextColorForWindow(window,
                                      isActive,
                                      depth,
                                      isColorDev);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *App_IsValidAppearanceFileType(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
    OSType fileType;
#ifndef IsValidAppearanceFileType
    PyMac_PRECHECK(IsValidAppearanceFileType);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          PyMac_GetOSType, &fileType))
        return NULL;
    _rv = IsValidAppearanceFileType(fileType);
    _res = Py_BuildValue("b",
                         _rv);
    return _res;
}

static PyObject *App_GetThemeBrushAsColor(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    ThemeBrush inBrush;
    SInt16 inDepth;
    Boolean inColorDev;
    RGBColor outColor;
#ifndef GetThemeBrushAsColor
    PyMac_PRECHECK(GetThemeBrushAsColor);
#endif
    if (!PyArg_ParseTuple(_args, "hhb",
                          &inBrush,
                          &inDepth,
                          &inColorDev))
        return NULL;
    _err = GetThemeBrushAsColor(inBrush,
                                inDepth,
                                inColorDev,
                                &outColor);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         QdRGB_New, &outColor);
    return _res;
}

static PyObject *App_GetThemeTextColor(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    ThemeTextColor inColor;
    SInt16 inDepth;
    Boolean inColorDev;
    RGBColor outColor;
#ifndef GetThemeTextColor
    PyMac_PRECHECK(GetThemeTextColor);
#endif
    if (!PyArg_ParseTuple(_args, "hhb",
                          &inColor,
                          &inDepth,
                          &inColorDev))
        return NULL;
    _err = GetThemeTextColor(inColor,
                             inDepth,
                             inColorDev,
                             &outColor);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         QdRGB_New, &outColor);
    return _res;
}

static PyObject *App_GetThemeMetric(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    ThemeMetric inMetric;
    SInt32 outMetric;
#ifndef GetThemeMetric
    PyMac_PRECHECK(GetThemeMetric);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &inMetric))
        return NULL;
    _err = GetThemeMetric(inMetric,
                          &outMetric);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("l",
                         outMetric);
    return _res;
}

static PyMethodDef App_methods[] = {
    {"RegisterAppearanceClient", (PyCFunction)App_RegisterAppearanceClient, 1,
     PyDoc_STR("() -> None")},
    {"UnregisterAppearanceClient", (PyCFunction)App_UnregisterAppearanceClient, 1,
     PyDoc_STR("() -> None")},
    {"SetThemePen", (PyCFunction)App_SetThemePen, 1,
     PyDoc_STR("(ThemeBrush inBrush, SInt16 inDepth, Boolean inIsColorDevice) -> None")},
    {"SetThemeBackground", (PyCFunction)App_SetThemeBackground, 1,
     PyDoc_STR("(ThemeBrush inBrush, SInt16 inDepth, Boolean inIsColorDevice) -> None")},
    {"SetThemeTextColor", (PyCFunction)App_SetThemeTextColor, 1,
     PyDoc_STR("(ThemeTextColor inColor, SInt16 inDepth, Boolean inIsColorDevice) -> None")},
    {"SetThemeWindowBackground", (PyCFunction)App_SetThemeWindowBackground, 1,
     PyDoc_STR("(WindowPtr inWindow, ThemeBrush inBrush, Boolean inUpdate) -> None")},
    {"DrawThemeWindowHeader", (PyCFunction)App_DrawThemeWindowHeader, 1,
     PyDoc_STR("(Rect inRect, ThemeDrawState inState) -> None")},
    {"DrawThemeWindowListViewHeader", (PyCFunction)App_DrawThemeWindowListViewHeader, 1,
     PyDoc_STR("(Rect inRect, ThemeDrawState inState) -> None")},
    {"DrawThemePlacard", (PyCFunction)App_DrawThemePlacard, 1,
     PyDoc_STR("(Rect inRect, ThemeDrawState inState) -> None")},
    {"DrawThemeEditTextFrame", (PyCFunction)App_DrawThemeEditTextFrame, 1,
     PyDoc_STR("(Rect inRect, ThemeDrawState inState) -> None")},
    {"DrawThemeListBoxFrame", (PyCFunction)App_DrawThemeListBoxFrame, 1,
     PyDoc_STR("(Rect inRect, ThemeDrawState inState) -> None")},
    {"DrawThemeFocusRect", (PyCFunction)App_DrawThemeFocusRect, 1,
     PyDoc_STR("(Rect inRect, Boolean inHasFocus) -> None")},
    {"DrawThemePrimaryGroup", (PyCFunction)App_DrawThemePrimaryGroup, 1,
     PyDoc_STR("(Rect inRect, ThemeDrawState inState) -> None")},
    {"DrawThemeSecondaryGroup", (PyCFunction)App_DrawThemeSecondaryGroup, 1,
     PyDoc_STR("(Rect inRect, ThemeDrawState inState) -> None")},
    {"DrawThemeSeparator", (PyCFunction)App_DrawThemeSeparator, 1,
     PyDoc_STR("(Rect inRect, ThemeDrawState inState) -> None")},
    {"DrawThemeModelessDialogFrame", (PyCFunction)App_DrawThemeModelessDialogFrame, 1,
     PyDoc_STR("(Rect inRect, ThemeDrawState inState) -> None")},
    {"DrawThemeGenericWell", (PyCFunction)App_DrawThemeGenericWell, 1,
     PyDoc_STR("(Rect inRect, ThemeDrawState inState, Boolean inFillCenter) -> None")},
    {"DrawThemeFocusRegion", (PyCFunction)App_DrawThemeFocusRegion, 1,
     PyDoc_STR("(Boolean inHasFocus) -> None")},
    {"IsThemeInColor", (PyCFunction)App_IsThemeInColor, 1,
     PyDoc_STR("(SInt16 inDepth, Boolean inIsColorDevice) -> (Boolean _rv)")},
    {"GetThemeAccentColors", (PyCFunction)App_GetThemeAccentColors, 1,
     PyDoc_STR("() -> (CTabHandle outColors)")},
    {"DrawThemeMenuBarBackground", (PyCFunction)App_DrawThemeMenuBarBackground, 1,
     PyDoc_STR("(Rect inBounds, ThemeMenuBarState inState, UInt32 inAttributes) -> None")},
    {"GetThemeMenuBarHeight", (PyCFunction)App_GetThemeMenuBarHeight, 1,
     PyDoc_STR("() -> (SInt16 outHeight)")},
    {"DrawThemeMenuBackground", (PyCFunction)App_DrawThemeMenuBackground, 1,
     PyDoc_STR("(Rect inMenuRect, ThemeMenuType inMenuType) -> None")},
    {"GetThemeMenuBackgroundRegion", (PyCFunction)App_GetThemeMenuBackgroundRegion, 1,
     PyDoc_STR("(Rect inMenuRect, ThemeMenuType menuType) -> None")},
    {"DrawThemeMenuSeparator", (PyCFunction)App_DrawThemeMenuSeparator, 1,
     PyDoc_STR("(Rect inItemRect) -> None")},
    {"GetThemeMenuSeparatorHeight", (PyCFunction)App_GetThemeMenuSeparatorHeight, 1,
     PyDoc_STR("() -> (SInt16 outHeight)")},
    {"GetThemeMenuItemExtra", (PyCFunction)App_GetThemeMenuItemExtra, 1,
     PyDoc_STR("(ThemeMenuItemType inItemType) -> (SInt16 outHeight, SInt16 outWidth)")},
    {"GetThemeMenuTitleExtra", (PyCFunction)App_GetThemeMenuTitleExtra, 1,
     PyDoc_STR("(Boolean inIsSquished) -> (SInt16 outWidth)")},
    {"DrawThemeTabPane", (PyCFunction)App_DrawThemeTabPane, 1,
     PyDoc_STR("(Rect inRect, ThemeDrawState inState) -> None")},
    {"GetThemeTabRegion", (PyCFunction)App_GetThemeTabRegion, 1,
     PyDoc_STR("(Rect inRect, ThemeTabStyle inStyle, ThemeTabDirection inDirection) -> None")},
    {"SetThemeCursor", (PyCFunction)App_SetThemeCursor, 1,
     PyDoc_STR("(ThemeCursor inCursor) -> None")},
    {"SetAnimatedThemeCursor", (PyCFunction)App_SetAnimatedThemeCursor, 1,
     PyDoc_STR("(ThemeCursor inCursor, UInt32 inAnimationStep) -> None")},
    {"GetThemeScrollBarThumbStyle", (PyCFunction)App_GetThemeScrollBarThumbStyle, 1,
     PyDoc_STR("() -> (ThemeScrollBarThumbStyle outStyle)")},
    {"GetThemeScrollBarArrowStyle", (PyCFunction)App_GetThemeScrollBarArrowStyle, 1,
     PyDoc_STR("() -> (ThemeScrollBarArrowStyle outStyle)")},
    {"GetThemeCheckBoxStyle", (PyCFunction)App_GetThemeCheckBoxStyle, 1,
     PyDoc_STR("() -> (ThemeCheckBoxStyle outStyle)")},
    {"UseThemeFont", (PyCFunction)App_UseThemeFont, 1,
     PyDoc_STR("(ThemeFontID inFontID, ScriptCode inScript) -> None")},
    {"DrawThemeTextBox", (PyCFunction)App_DrawThemeTextBox, 1,
     PyDoc_STR("(CFStringRef inString, ThemeFontID inFontID, ThemeDrawState inState, Boolean inWrapToWidth, Rect inBoundingBox, SInt16 inJust) -> None")},
    {"TruncateThemeText", (PyCFunction)App_TruncateThemeText, 1,
     PyDoc_STR("(CFMutableStringRef inString, ThemeFontID inFontID, ThemeDrawState inState, SInt16 inPixelWidthLimit, TruncCode inTruncWhere) -> (Boolean outTruncated)")},
    {"GetThemeTextDimensions", (PyCFunction)App_GetThemeTextDimensions, 1,
     PyDoc_STR("(CFStringRef inString, ThemeFontID inFontID, ThemeDrawState inState, Boolean inWrapToWidth, Point ioBounds) -> (Point ioBounds, SInt16 outBaseline)")},
    {"GetThemeTextShadowOutset", (PyCFunction)App_GetThemeTextShadowOutset, 1,
     PyDoc_STR("(ThemeFontID inFontID, ThemeDrawState inState) -> (Rect outOutset)")},
    {"DrawThemeScrollBarArrows", (PyCFunction)App_DrawThemeScrollBarArrows, 1,
     PyDoc_STR("(Rect bounds, ThemeTrackEnableState enableState, ThemeTrackPressState pressState, Boolean isHoriz) -> (Rect trackBounds)")},
    {"GetThemeScrollBarTrackRect", (PyCFunction)App_GetThemeScrollBarTrackRect, 1,
     PyDoc_STR("(Rect bounds, ThemeTrackEnableState enableState, ThemeTrackPressState pressState, Boolean isHoriz) -> (Rect trackBounds)")},
    {"HitTestThemeScrollBarArrows", (PyCFunction)App_HitTestThemeScrollBarArrows, 1,
     PyDoc_STR("(Rect scrollBarBounds, ThemeTrackEnableState enableState, ThemeTrackPressState pressState, Boolean isHoriz, Point ptHit) -> (Boolean _rv, Rect trackBounds, ControlPartCode partcode)")},
    {"DrawThemeScrollBarDelimiters", (PyCFunction)App_DrawThemeScrollBarDelimiters, 1,
     PyDoc_STR("(ThemeWindowType flavor, Rect inContRect, ThemeDrawState state, ThemeWindowAttributes attributes) -> None")},
    {"DrawThemeButton", (PyCFunction)App_DrawThemeButton, 1,
     PyDoc_STR("(Rect inBounds, UInt16 inKind, ThemeButtonDrawInfo inNewInfo, ThemeButtonDrawInfo inPrevInfo, UInt32 inUserData) -> None")},
    {"GetThemeButtonRegion", (PyCFunction)App_GetThemeButtonRegion, 1,
     PyDoc_STR("(Rect inBounds, UInt16 inKind, ThemeButtonDrawInfo inNewInfo) -> None")},
    {"GetThemeButtonContentBounds", (PyCFunction)App_GetThemeButtonContentBounds, 1,
     PyDoc_STR("(Rect inBounds, UInt16 inKind, ThemeButtonDrawInfo inDrawInfo) -> (Rect outBounds)")},
    {"GetThemeButtonBackgroundBounds", (PyCFunction)App_GetThemeButtonBackgroundBounds, 1,
     PyDoc_STR("(Rect inBounds, UInt16 inKind, ThemeButtonDrawInfo inDrawInfo) -> (Rect outBounds)")},
    {"PlayThemeSound", (PyCFunction)App_PlayThemeSound, 1,
     PyDoc_STR("(ThemeSoundKind kind) -> None")},
    {"BeginThemeDragSound", (PyCFunction)App_BeginThemeDragSound, 1,
     PyDoc_STR("(ThemeDragSoundKind kind) -> None")},
    {"EndThemeDragSound", (PyCFunction)App_EndThemeDragSound, 1,
     PyDoc_STR("() -> None")},
    {"DrawThemeTickMark", (PyCFunction)App_DrawThemeTickMark, 1,
     PyDoc_STR("(Rect bounds, ThemeDrawState state) -> None")},
    {"DrawThemeChasingArrows", (PyCFunction)App_DrawThemeChasingArrows, 1,
     PyDoc_STR("(Rect bounds, UInt32 index, ThemeDrawState state, UInt32 eraseData) -> None")},
    {"DrawThemePopupArrow", (PyCFunction)App_DrawThemePopupArrow, 1,
     PyDoc_STR("(Rect bounds, ThemeArrowOrientation orientation, ThemePopupArrowSize size, ThemeDrawState state, UInt32 eraseData) -> None")},
    {"DrawThemeStandaloneGrowBox", (PyCFunction)App_DrawThemeStandaloneGrowBox, 1,
     PyDoc_STR("(Point origin, ThemeGrowDirection growDirection, Boolean isSmall, ThemeDrawState state) -> None")},
    {"DrawThemeStandaloneNoGrowBox", (PyCFunction)App_DrawThemeStandaloneNoGrowBox, 1,
     PyDoc_STR("(Point origin, ThemeGrowDirection growDirection, Boolean isSmall, ThemeDrawState state) -> None")},
    {"GetThemeStandaloneGrowBoxBounds", (PyCFunction)App_GetThemeStandaloneGrowBoxBounds, 1,
     PyDoc_STR("(Point origin, ThemeGrowDirection growDirection, Boolean isSmall) -> (Rect bounds)")},
    {"NormalizeThemeDrawingState", (PyCFunction)App_NormalizeThemeDrawingState, 1,
     PyDoc_STR("() -> None")},
    {"GetThemeDrawingState", (PyCFunction)App_GetThemeDrawingState, 1,
     PyDoc_STR("() -> (ThemeDrawingState outState)")},
    {"ApplyThemeBackground", (PyCFunction)App_ApplyThemeBackground, 1,
     PyDoc_STR("(ThemeBackgroundKind inKind, Rect bounds, ThemeDrawState inState, SInt16 inDepth, Boolean inColorDev) -> None")},
    {"SetThemeTextColorForWindow", (PyCFunction)App_SetThemeTextColorForWindow, 1,
     PyDoc_STR("(WindowPtr window, Boolean isActive, SInt16 depth, Boolean isColorDev) -> None")},
    {"IsValidAppearanceFileType", (PyCFunction)App_IsValidAppearanceFileType, 1,
     PyDoc_STR("(OSType fileType) -> (Boolean _rv)")},
    {"GetThemeBrushAsColor", (PyCFunction)App_GetThemeBrushAsColor, 1,
     PyDoc_STR("(ThemeBrush inBrush, SInt16 inDepth, Boolean inColorDev) -> (RGBColor outColor)")},
    {"GetThemeTextColor", (PyCFunction)App_GetThemeTextColor, 1,
     PyDoc_STR("(ThemeTextColor inColor, SInt16 inDepth, Boolean inColorDev) -> (RGBColor outColor)")},
    {"GetThemeMetric", (PyCFunction)App_GetThemeMetric, 1,
     PyDoc_STR("(ThemeMetric inMetric) -> (SInt32 outMetric)")},
    {NULL, NULL, 0}
};


#else   /* __LP64__ */

static PyMethodDef App_methods[] = {
    {NULL, NULL, 0}
};

#endif /* __LP64__ */


void init_App(void)
{
    PyObject *m;
#ifndef __LP64__
    PyObject *d;
#endif /* !__LP64__ */


    m = Py_InitModule("_App", App_methods);
#ifndef __LP64__
    d = PyModule_GetDict(m);
    App_Error = PyMac_GetOSErrException();
    if (App_Error == NULL ||
        PyDict_SetItemString(d, "Error", App_Error) != 0)
        return;
    ThemeDrawingState_Type.ob_type = &PyType_Type;
    if (PyType_Ready(&ThemeDrawingState_Type) < 0) return;
    Py_INCREF(&ThemeDrawingState_Type);
    PyModule_AddObject(m, "ThemeDrawingState", (PyObject *)&ThemeDrawingState_Type);
    /* Backward-compatible name */
    Py_INCREF(&ThemeDrawingState_Type);
    PyModule_AddObject(m, "ThemeDrawingStateType", (PyObject *)&ThemeDrawingState_Type);
#endif /* __LP64__ */
}

/* ======================== End module _App ========================= */

