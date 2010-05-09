
/* ========================== Module _Ctl =========================== */

#include "Python.h"

#ifndef __LP64__


#include "pymactoolbox.h"

/* Macro to test whether a weak-loaded CFM function exists */
#define PyMac_PRECHECK(rtn) do { if ( &rtn == NULL )  {\
    PyErr_SetString(PyExc_NotImplementedError, \
    "Not available in this shared library/OS version"); \
    return NULL; \
    }} while(0)


#include <Carbon/Carbon.h>

#ifdef USE_TOOLBOX_OBJECT_GLUE
extern PyObject *_CtlObj_New(ControlHandle);
extern int _CtlObj_Convert(PyObject *, ControlHandle *);

#define CtlObj_New _CtlObj_New
#define CtlObj_Convert _CtlObj_Convert
#endif

static PyObject *CtlObj_WhichControl(ControlHandle);

#define as_Control(h) ((ControlHandle)h)
#define as_Resource(ctl) ((Handle)ctl)
#define GetControlRect(ctl, rectp) GetControlBounds(ctl, rectp)

#define MAXTABS 32  /* maximum number of tabs that we support in a tabs control */
/*
** Parse/generate ControlFontStyleRec records
*/
#if 0 /* Not needed */
static PyObject *
ControlFontStyle_New(ControlFontStyleRec *itself)
{

    return Py_BuildValue("hhhhhhO&O&", itself->flags, itself->font,
        itself->size, itself->style, itself->mode, itself->just,
        QdRGB_New, &itself->foreColor, QdRGB_New, &itself->backColor);
}
#endif

static int
ControlFontStyle_Convert(PyObject *v, ControlFontStyleRec *itself)
{
    return PyArg_Parse(v, "(hhhhhhO&O&)", &itself->flags,
        &itself->font, &itself->size, &itself->style, &itself->mode,
        &itself->just, QdRGB_Convert, &itself->foreColor,
        QdRGB_Convert, &itself->backColor);
}

/*
** Parse/generate ControlID records
*/
static PyObject *
PyControlID_New(ControlID *itself)
{

    return Py_BuildValue("O&l", PyMac_BuildOSType, itself->signature, itself->id);
}

static int
PyControlID_Convert(PyObject *v, ControlID *itself)
{
    return PyArg_Parse(v, "(O&l)", PyMac_GetOSType, &itself->signature, &itself->id);
}

/*
** generate DataBrowserListViewColumnDesc records
*/
static int
DataBrowserTableViewColumnDesc_Convert(PyObject *v, DataBrowserTableViewColumnDesc *itself)
{
    return PyArg_Parse(v, "(lO&l)",
                       &itself->propertyID,
                       PyMac_GetOSType, &itself->propertyType,
                       &itself->propertyFlags);
}

static int
ControlButtonContentInfo_Convert(PyObject *v, ControlButtonContentInfo *itself)
{
    return PyArg_Parse(v, "(hO&)",
                       &itself->contentType,
                       OptResObj_Convert, &itself->u.iconSuite);
}

static int
DataBrowserListViewHeaderDesc_Convert(PyObject *v, DataBrowserListViewHeaderDesc *itself)
{
    itself->version = kDataBrowserListViewLatestHeaderDesc;
    return PyArg_Parse(v, "(HHhO&HO&O&)",
                       &itself->minimumWidth,
                       &itself->maximumWidth,
                       &itself->titleOffset,
                       CFStringRefObj_Convert, &itself->titleString,
                       &itself->initialOrder,
                       ControlFontStyle_Convert, &itself->btnFontStyle,
                       ControlButtonContentInfo_Convert, &itself->btnContentInfo);
}

static int
DataBrowserListViewColumnDesc_Convert(PyObject *v, DataBrowserListViewColumnDesc *itself)
{
    return PyArg_Parse(v, "(O&O&)",
                       DataBrowserTableViewColumnDesc_Convert, &itself->propertyDesc,
                       DataBrowserListViewHeaderDesc_Convert, &itself->headerBtnDesc);
}

/* TrackControl and HandleControlClick callback support */
#define kMyControlActionProcTag 'ACTN'  /* not an official tag, only for internal use */
static PyObject *tracker;
static ControlActionUPP mytracker_upp;
static ControlActionUPP myactionproc_upp;
static ControlUserPaneKeyDownUPP mykeydownproc_upp;
static ControlUserPaneFocusUPP myfocusproc_upp;
static ControlUserPaneDrawUPP mydrawproc_upp;
static ControlUserPaneIdleUPP myidleproc_upp;
static ControlUserPaneHitTestUPP myhittestproc_upp;
static ControlUserPaneTrackingUPP mytrackingproc_upp;

static int settrackfunc(PyObject *);    /* forward */
static void clrtrackfunc(void); /* forward */
static int setcallback(PyObject *, OSType, PyObject *, UniversalProcPtr *);

static PyObject *Ctl_Error;

/* ---------------------- Object type Control ----------------------- */

PyTypeObject Control_Type;

#define CtlObj_Check(x) ((x)->ob_type == &Control_Type || PyObject_TypeCheck((x), &Control_Type))

typedef struct ControlObject {
    PyObject_HEAD
    ControlHandle ob_itself;
    PyObject *ob_callbackdict;
} ControlObject;

PyObject *CtlObj_New(ControlHandle itself)
{
    ControlObject *it;
    if (itself == NULL) return PyMac_Error(resNotFound);
    it = PyObject_NEW(ControlObject, &Control_Type);
    if (it == NULL) return NULL;
    it->ob_itself = itself;
    SetControlReference(itself, (long)it);
    it->ob_callbackdict = NULL;
    return (PyObject *)it;
}

int CtlObj_Convert(PyObject *v, ControlHandle *p_itself)
{
    if (!CtlObj_Check(v))
    {
        PyErr_SetString(PyExc_TypeError, "Control required");
        return 0;
    }
    *p_itself = ((ControlObject *)v)->ob_itself;
    return 1;
}

static void CtlObj_dealloc(ControlObject *self)
{
    Py_XDECREF(self->ob_callbackdict);
    if (self->ob_itself)SetControlReference(self->ob_itself, (long)0); /* Make it forget about us */
    self->ob_type->tp_free((PyObject *)self);
}

static PyObject *CtlObj_HiliteControl(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    ControlPartCode hiliteState;
#ifndef HiliteControl
    PyMac_PRECHECK(HiliteControl);
#endif
    if (!PyArg_ParseTuple(_args, "h",
                          &hiliteState))
        return NULL;
    HiliteControl(_self->ob_itself,
                  hiliteState);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_ShowControl(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
#ifndef ShowControl
    PyMac_PRECHECK(ShowControl);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    ShowControl(_self->ob_itself);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_HideControl(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
#ifndef HideControl
    PyMac_PRECHECK(HideControl);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    HideControl(_self->ob_itself);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_IsControlActive(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
#ifndef IsControlActive
    PyMac_PRECHECK(IsControlActive);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = IsControlActive(_self->ob_itself);
    _res = Py_BuildValue("b",
                         _rv);
    return _res;
}

static PyObject *CtlObj_IsControlVisible(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
#ifndef IsControlVisible
    PyMac_PRECHECK(IsControlVisible);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = IsControlVisible(_self->ob_itself);
    _res = Py_BuildValue("b",
                         _rv);
    return _res;
}

static PyObject *CtlObj_ActivateControl(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
#ifndef ActivateControl
    PyMac_PRECHECK(ActivateControl);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = ActivateControl(_self->ob_itself);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_DeactivateControl(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
#ifndef DeactivateControl
    PyMac_PRECHECK(DeactivateControl);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = DeactivateControl(_self->ob_itself);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_SetControlVisibility(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    Boolean inIsVisible;
    Boolean inDoDraw;
#ifndef SetControlVisibility
    PyMac_PRECHECK(SetControlVisibility);
#endif
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

static PyObject *CtlObj_IsControlEnabled(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
#ifndef IsControlEnabled
    PyMac_PRECHECK(IsControlEnabled);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = IsControlEnabled(_self->ob_itself);
    _res = Py_BuildValue("b",
                         _rv);
    return _res;
}

static PyObject *CtlObj_EnableControl(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
#ifndef EnableControl
    PyMac_PRECHECK(EnableControl);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = EnableControl(_self->ob_itself);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_DisableControl(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
#ifndef DisableControl
    PyMac_PRECHECK(DisableControl);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = DisableControl(_self->ob_itself);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_Draw1Control(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
#ifndef Draw1Control
    PyMac_PRECHECK(Draw1Control);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    Draw1Control(_self->ob_itself);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetBestControlRect(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    Rect outRect;
    SInt16 outBaseLineOffset;
#ifndef GetBestControlRect
    PyMac_PRECHECK(GetBestControlRect);
#endif
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

static PyObject *CtlObj_SetControlFontStyle(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    ControlFontStyleRec inStyle;
#ifndef SetControlFontStyle
    PyMac_PRECHECK(SetControlFontStyle);
#endif
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

static PyObject *CtlObj_DrawControlInCurrentPort(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
#ifndef DrawControlInCurrentPort
    PyMac_PRECHECK(DrawControlInCurrentPort);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    DrawControlInCurrentPort(_self->ob_itself);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_SetUpControlBackground(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    SInt16 inDepth;
    Boolean inIsColorDevice;
#ifndef SetUpControlBackground
    PyMac_PRECHECK(SetUpControlBackground);
#endif
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

static PyObject *CtlObj_SetUpControlTextColor(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    SInt16 inDepth;
    Boolean inIsColorDevice;
#ifndef SetUpControlTextColor
    PyMac_PRECHECK(SetUpControlTextColor);
#endif
    if (!PyArg_ParseTuple(_args, "hb",
                          &inDepth,
                          &inIsColorDevice))
        return NULL;
    _err = SetUpControlTextColor(_self->ob_itself,
                                 inDepth,
                                 inIsColorDevice);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_DragControl(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Point startPoint;
    Rect limitRect;
    Rect slopRect;
    DragConstraint axis;
#ifndef DragControl
    PyMac_PRECHECK(DragControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&O&H",
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

static PyObject *CtlObj_TestControl(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    ControlPartCode _rv;
    Point testPoint;
#ifndef TestControl
    PyMac_PRECHECK(TestControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          PyMac_GetPoint, &testPoint))
        return NULL;
    _rv = TestControl(_self->ob_itself,
                      testPoint);
    _res = Py_BuildValue("h",
                         _rv);
    return _res;
}

static PyObject *CtlObj_HandleControlContextualMenuClick(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Point inWhere;
    Boolean menuDisplayed;
#ifndef HandleControlContextualMenuClick
    PyMac_PRECHECK(HandleControlContextualMenuClick);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          PyMac_GetPoint, &inWhere))
        return NULL;
    _err = HandleControlContextualMenuClick(_self->ob_itself,
                                            inWhere,
                                            &menuDisplayed);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("b",
                         menuDisplayed);
    return _res;
}

static PyObject *CtlObj_GetControlClickActivation(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Point inWhere;
    EventModifiers inModifiers;
    ClickActivationResult outResult;
#ifndef GetControlClickActivation
    PyMac_PRECHECK(GetControlClickActivation);
#endif
    if (!PyArg_ParseTuple(_args, "O&H",
                          PyMac_GetPoint, &inWhere,
                          &inModifiers))
        return NULL;
    _err = GetControlClickActivation(_self->ob_itself,
                                     inWhere,
                                     inModifiers,
                                     &outResult);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("l",
                         outResult);
    return _res;
}

static PyObject *CtlObj_HandleControlKey(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    ControlPartCode _rv;
    SInt16 inKeyCode;
    SInt16 inCharCode;
    EventModifiers inModifiers;
#ifndef HandleControlKey
    PyMac_PRECHECK(HandleControlKey);
#endif
    if (!PyArg_ParseTuple(_args, "hhH",
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

static PyObject *CtlObj_HandleControlSetCursor(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Point localPoint;
    EventModifiers modifiers;
    Boolean cursorWasSet;
#ifndef HandleControlSetCursor
    PyMac_PRECHECK(HandleControlSetCursor);
#endif
    if (!PyArg_ParseTuple(_args, "O&H",
                          PyMac_GetPoint, &localPoint,
                          &modifiers))
        return NULL;
    _err = HandleControlSetCursor(_self->ob_itself,
                                  localPoint,
                                  modifiers,
                                  &cursorWasSet);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("b",
                         cursorWasSet);
    return _res;
}

static PyObject *CtlObj_MoveControl(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    SInt16 h;
    SInt16 v;
#ifndef MoveControl
    PyMac_PRECHECK(MoveControl);
#endif
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

static PyObject *CtlObj_SizeControl(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    SInt16 w;
    SInt16 h;
#ifndef SizeControl
    PyMac_PRECHECK(SizeControl);
#endif
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

static PyObject *CtlObj_SetControlTitle(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Str255 title;
#ifndef SetControlTitle
    PyMac_PRECHECK(SetControlTitle);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          PyMac_GetStr255, title))
        return NULL;
    SetControlTitle(_self->ob_itself,
                    title);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetControlTitle(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Str255 title;
#ifndef GetControlTitle
    PyMac_PRECHECK(GetControlTitle);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    GetControlTitle(_self->ob_itself,
                    title);
    _res = Py_BuildValue("O&",
                         PyMac_BuildStr255, title);
    return _res;
}

static PyObject *CtlObj_SetControlTitleWithCFString(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    CFStringRef inString;
#ifndef SetControlTitleWithCFString
    PyMac_PRECHECK(SetControlTitleWithCFString);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          CFStringRefObj_Convert, &inString))
        return NULL;
    _err = SetControlTitleWithCFString(_self->ob_itself,
                                       inString);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_CopyControlTitleAsCFString(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    CFStringRef outString;
#ifndef CopyControlTitleAsCFString
    PyMac_PRECHECK(CopyControlTitleAsCFString);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = CopyControlTitleAsCFString(_self->ob_itself,
                                      &outString);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, outString);
    return _res;
}

static PyObject *CtlObj_GetControlValue(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    SInt16 _rv;
#ifndef GetControlValue
    PyMac_PRECHECK(GetControlValue);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetControlValue(_self->ob_itself);
    _res = Py_BuildValue("h",
                         _rv);
    return _res;
}

static PyObject *CtlObj_SetControlValue(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    SInt16 newValue;
#ifndef SetControlValue
    PyMac_PRECHECK(SetControlValue);
#endif
    if (!PyArg_ParseTuple(_args, "h",
                          &newValue))
        return NULL;
    SetControlValue(_self->ob_itself,
                    newValue);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetControlMinimum(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    SInt16 _rv;
#ifndef GetControlMinimum
    PyMac_PRECHECK(GetControlMinimum);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetControlMinimum(_self->ob_itself);
    _res = Py_BuildValue("h",
                         _rv);
    return _res;
}

static PyObject *CtlObj_SetControlMinimum(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    SInt16 newMinimum;
#ifndef SetControlMinimum
    PyMac_PRECHECK(SetControlMinimum);
#endif
    if (!PyArg_ParseTuple(_args, "h",
                          &newMinimum))
        return NULL;
    SetControlMinimum(_self->ob_itself,
                      newMinimum);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetControlMaximum(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    SInt16 _rv;
#ifndef GetControlMaximum
    PyMac_PRECHECK(GetControlMaximum);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetControlMaximum(_self->ob_itself);
    _res = Py_BuildValue("h",
                         _rv);
    return _res;
}

static PyObject *CtlObj_SetControlMaximum(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    SInt16 newMaximum;
#ifndef SetControlMaximum
    PyMac_PRECHECK(SetControlMaximum);
#endif
    if (!PyArg_ParseTuple(_args, "h",
                          &newMaximum))
        return NULL;
    SetControlMaximum(_self->ob_itself,
                      newMaximum);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetControlViewSize(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    SInt32 _rv;
#ifndef GetControlViewSize
    PyMac_PRECHECK(GetControlViewSize);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetControlViewSize(_self->ob_itself);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CtlObj_SetControlViewSize(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    SInt32 newViewSize;
#ifndef SetControlViewSize
    PyMac_PRECHECK(SetControlViewSize);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &newViewSize))
        return NULL;
    SetControlViewSize(_self->ob_itself,
                       newViewSize);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetControl32BitValue(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    SInt32 _rv;
#ifndef GetControl32BitValue
    PyMac_PRECHECK(GetControl32BitValue);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetControl32BitValue(_self->ob_itself);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CtlObj_SetControl32BitValue(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    SInt32 newValue;
#ifndef SetControl32BitValue
    PyMac_PRECHECK(SetControl32BitValue);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &newValue))
        return NULL;
    SetControl32BitValue(_self->ob_itself,
                         newValue);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetControl32BitMaximum(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    SInt32 _rv;
#ifndef GetControl32BitMaximum
    PyMac_PRECHECK(GetControl32BitMaximum);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetControl32BitMaximum(_self->ob_itself);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CtlObj_SetControl32BitMaximum(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    SInt32 newMaximum;
#ifndef SetControl32BitMaximum
    PyMac_PRECHECK(SetControl32BitMaximum);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &newMaximum))
        return NULL;
    SetControl32BitMaximum(_self->ob_itself,
                           newMaximum);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetControl32BitMinimum(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    SInt32 _rv;
#ifndef GetControl32BitMinimum
    PyMac_PRECHECK(GetControl32BitMinimum);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetControl32BitMinimum(_self->ob_itself);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CtlObj_SetControl32BitMinimum(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    SInt32 newMinimum;
#ifndef SetControl32BitMinimum
    PyMac_PRECHECK(SetControl32BitMinimum);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &newMinimum))
        return NULL;
    SetControl32BitMinimum(_self->ob_itself,
                           newMinimum);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_IsValidControlHandle(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
#ifndef IsValidControlHandle
    PyMac_PRECHECK(IsValidControlHandle);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = IsValidControlHandle(_self->ob_itself);
    _res = Py_BuildValue("b",
                         _rv);
    return _res;
}

static PyObject *CtlObj_SetControlID(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    ControlID inID;
#ifndef SetControlID
    PyMac_PRECHECK(SetControlID);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          PyControlID_Convert, &inID))
        return NULL;
    _err = SetControlID(_self->ob_itself,
                        &inID);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetControlID(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    ControlID outID;
#ifndef GetControlID
    PyMac_PRECHECK(GetControlID);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetControlID(_self->ob_itself,
                        &outID);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         PyControlID_New, &outID);
    return _res;
}

static PyObject *CtlObj_SetControlCommandID(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 inCommandID;
#ifndef SetControlCommandID
    PyMac_PRECHECK(SetControlCommandID);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &inCommandID))
        return NULL;
    _err = SetControlCommandID(_self->ob_itself,
                               inCommandID);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetControlCommandID(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 outCommandID;
#ifndef GetControlCommandID
    PyMac_PRECHECK(GetControlCommandID);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetControlCommandID(_self->ob_itself,
                               &outCommandID);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("l",
                         outCommandID);
    return _res;
}

static PyObject *CtlObj_RemoveControlProperty(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    OSType propertyCreator;
    OSType propertyTag;
#ifndef RemoveControlProperty
    PyMac_PRECHECK(RemoveControlProperty);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&",
                          PyMac_GetOSType, &propertyCreator,
                          PyMac_GetOSType, &propertyTag))
        return NULL;
    _err = RemoveControlProperty(_self->ob_itself,
                                 propertyCreator,
                                 propertyTag);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetControlPropertyAttributes(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    OSType propertyCreator;
    OSType propertyTag;
    UInt32 attributes;
#ifndef GetControlPropertyAttributes
    PyMac_PRECHECK(GetControlPropertyAttributes);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&",
                          PyMac_GetOSType, &propertyCreator,
                          PyMac_GetOSType, &propertyTag))
        return NULL;
    _err = GetControlPropertyAttributes(_self->ob_itself,
                                        propertyCreator,
                                        propertyTag,
                                        &attributes);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("l",
                         attributes);
    return _res;
}

static PyObject *CtlObj_ChangeControlPropertyAttributes(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    OSType propertyCreator;
    OSType propertyTag;
    UInt32 attributesToSet;
    UInt32 attributesToClear;
#ifndef ChangeControlPropertyAttributes
    PyMac_PRECHECK(ChangeControlPropertyAttributes);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&ll",
                          PyMac_GetOSType, &propertyCreator,
                          PyMac_GetOSType, &propertyTag,
                          &attributesToSet,
                          &attributesToClear))
        return NULL;
    _err = ChangeControlPropertyAttributes(_self->ob_itself,
                                           propertyCreator,
                                           propertyTag,
                                           attributesToSet,
                                           attributesToClear);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetControlRegion(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    ControlPartCode inPart;
    RgnHandle outRegion;
#ifndef GetControlRegion
    PyMac_PRECHECK(GetControlRegion);
#endif
    if (!PyArg_ParseTuple(_args, "hO&",
                          &inPart,
                          ResObj_Convert, &outRegion))
        return NULL;
    _err = GetControlRegion(_self->ob_itself,
                            inPart,
                            outRegion);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetControlVariant(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    ControlVariant _rv;
#ifndef GetControlVariant
    PyMac_PRECHECK(GetControlVariant);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetControlVariant(_self->ob_itself);
    _res = Py_BuildValue("h",
                         _rv);
    return _res;
}

static PyObject *CtlObj_SetControlAction(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    PyObject* actionProc;
    UniversalProcPtr c_callback;
#ifndef SetControlAction
    PyMac_PRECHECK(SetControlAction);
#endif
    if (!PyArg_ParseTuple(_args, "O",
                          &actionProc))
        return NULL;
    SetControlAction(_self->ob_itself,
                     myactionproc_upp);
    Py_INCREF(Py_None);
    _res = Py_None;
    setcallback((PyObject*)_self, kMyControlActionProcTag, actionProc, &c_callback);
    return _res;
}

static PyObject *CtlObj_SetControlReference(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    SInt32 data;
#ifndef SetControlReference
    PyMac_PRECHECK(SetControlReference);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &data))
        return NULL;
    SetControlReference(_self->ob_itself,
                        data);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetControlReference(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    SInt32 _rv;
#ifndef GetControlReference
    PyMac_PRECHECK(GetControlReference);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetControlReference(_self->ob_itself);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *CtlObj_EmbedControl(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    ControlHandle inContainer;
#ifndef EmbedControl
    PyMac_PRECHECK(EmbedControl);
#endif
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

static PyObject *CtlObj_AutoEmbedControl(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    WindowPtr inWindow;
#ifndef AutoEmbedControl
    PyMac_PRECHECK(AutoEmbedControl);
#endif
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

static PyObject *CtlObj_GetSuperControl(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    ControlHandle outParent;
#ifndef GetSuperControl
    PyMac_PRECHECK(GetSuperControl);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetSuperControl(_self->ob_itself,
                           &outParent);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_WhichControl, outParent);
    return _res;
}

static PyObject *CtlObj_CountSubControls(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    UInt16 outNumChildren;
#ifndef CountSubControls
    PyMac_PRECHECK(CountSubControls);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = CountSubControls(_self->ob_itself,
                            &outNumChildren);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("H",
                         outNumChildren);
    return _res;
}

static PyObject *CtlObj_GetIndexedSubControl(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    UInt16 inIndex;
    ControlHandle outSubControl;
#ifndef GetIndexedSubControl
    PyMac_PRECHECK(GetIndexedSubControl);
#endif
    if (!PyArg_ParseTuple(_args, "H",
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

static PyObject *CtlObj_SetControlSupervisor(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    ControlHandle inBoss;
#ifndef SetControlSupervisor
    PyMac_PRECHECK(SetControlSupervisor);
#endif
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

static PyObject *CtlObj_GetControlFeatures(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    UInt32 outFeatures;
#ifndef GetControlFeatures
    PyMac_PRECHECK(GetControlFeatures);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetControlFeatures(_self->ob_itself,
                              &outFeatures);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("l",
                         outFeatures);
    return _res;
}

static PyObject *CtlObj_GetControlDataSize(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    ControlPartCode inPart;
    ResType inTagName;
    Size outMaxSize;
#ifndef GetControlDataSize
    PyMac_PRECHECK(GetControlDataSize);
#endif
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

static PyObject *CtlObj_HandleControlDragTracking(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    DragTrackingMessage inMessage;
    DragReference inDrag;
    Boolean outLikesDrag;
#ifndef HandleControlDragTracking
    PyMac_PRECHECK(HandleControlDragTracking);
#endif
    if (!PyArg_ParseTuple(_args, "hO&",
                          &inMessage,
                          DragObj_Convert, &inDrag))
        return NULL;
    _err = HandleControlDragTracking(_self->ob_itself,
                                     inMessage,
                                     inDrag,
                                     &outLikesDrag);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("b",
                         outLikesDrag);
    return _res;
}

static PyObject *CtlObj_HandleControlDragReceive(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    DragReference inDrag;
#ifndef HandleControlDragReceive
    PyMac_PRECHECK(HandleControlDragReceive);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          DragObj_Convert, &inDrag))
        return NULL;
    _err = HandleControlDragReceive(_self->ob_itself,
                                    inDrag);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_SetControlDragTrackingEnabled(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Boolean inTracks;
#ifndef SetControlDragTrackingEnabled
    PyMac_PRECHECK(SetControlDragTrackingEnabled);
#endif
    if (!PyArg_ParseTuple(_args, "b",
                          &inTracks))
        return NULL;
    _err = SetControlDragTrackingEnabled(_self->ob_itself,
                                         inTracks);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_IsControlDragTrackingEnabled(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Boolean outTracks;
#ifndef IsControlDragTrackingEnabled
    PyMac_PRECHECK(IsControlDragTrackingEnabled);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = IsControlDragTrackingEnabled(_self->ob_itself,
                                        &outTracks);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("b",
                         outTracks);
    return _res;
}

static PyObject *CtlObj_GetControlBounds(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Rect bounds;
#ifndef GetControlBounds
    PyMac_PRECHECK(GetControlBounds);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    GetControlBounds(_self->ob_itself,
                     &bounds);
    _res = Py_BuildValue("O&",
                         PyMac_BuildRect, &bounds);
    return _res;
}

static PyObject *CtlObj_IsControlHilited(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
#ifndef IsControlHilited
    PyMac_PRECHECK(IsControlHilited);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = IsControlHilited(_self->ob_itself);
    _res = Py_BuildValue("b",
                         _rv);
    return _res;
}

static PyObject *CtlObj_GetControlHilite(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    UInt16 _rv;
#ifndef GetControlHilite
    PyMac_PRECHECK(GetControlHilite);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetControlHilite(_self->ob_itself);
    _res = Py_BuildValue("H",
                         _rv);
    return _res;
}

static PyObject *CtlObj_GetControlOwner(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    WindowPtr _rv;
#ifndef GetControlOwner
    PyMac_PRECHECK(GetControlOwner);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetControlOwner(_self->ob_itself);
    _res = Py_BuildValue("O&",
                         WinObj_New, _rv);
    return _res;
}

static PyObject *CtlObj_GetControlDataHandle(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Handle _rv;
#ifndef GetControlDataHandle
    PyMac_PRECHECK(GetControlDataHandle);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetControlDataHandle(_self->ob_itself);
    _res = Py_BuildValue("O&",
                         ResObj_New, _rv);
    return _res;
}

static PyObject *CtlObj_GetControlPopupMenuHandle(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    MenuHandle _rv;
#ifndef GetControlPopupMenuHandle
    PyMac_PRECHECK(GetControlPopupMenuHandle);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetControlPopupMenuHandle(_self->ob_itself);
    _res = Py_BuildValue("O&",
                         MenuObj_New, _rv);
    return _res;
}

static PyObject *CtlObj_GetControlPopupMenuID(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    short _rv;
#ifndef GetControlPopupMenuID
    PyMac_PRECHECK(GetControlPopupMenuID);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetControlPopupMenuID(_self->ob_itself);
    _res = Py_BuildValue("h",
                         _rv);
    return _res;
}

static PyObject *CtlObj_SetControlDataHandle(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Handle dataHandle;
#ifndef SetControlDataHandle
    PyMac_PRECHECK(SetControlDataHandle);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          ResObj_Convert, &dataHandle))
        return NULL;
    SetControlDataHandle(_self->ob_itself,
                         dataHandle);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_SetControlBounds(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Rect bounds;
#ifndef SetControlBounds
    PyMac_PRECHECK(SetControlBounds);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          PyMac_GetRect, &bounds))
        return NULL;
    SetControlBounds(_self->ob_itself,
                     &bounds);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_SetControlPopupMenuHandle(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    MenuHandle popupMenu;
#ifndef SetControlPopupMenuHandle
    PyMac_PRECHECK(SetControlPopupMenuHandle);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          MenuObj_Convert, &popupMenu))
        return NULL;
    SetControlPopupMenuHandle(_self->ob_itself,
                              popupMenu);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_SetControlPopupMenuID(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    short menuID;
#ifndef SetControlPopupMenuID
    PyMac_PRECHECK(SetControlPopupMenuID);
#endif
    if (!PyArg_ParseTuple(_args, "h",
                          &menuID))
        return NULL;
    SetControlPopupMenuID(_self->ob_itself,
                          menuID);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetBevelButtonMenuValue(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    UInt16 outValue;
#ifndef GetBevelButtonMenuValue
    PyMac_PRECHECK(GetBevelButtonMenuValue);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetBevelButtonMenuValue(_self->ob_itself,
                                   &outValue);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("h",
                         outValue);
    return _res;
}

static PyObject *CtlObj_SetBevelButtonMenuValue(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    SInt16 inValue;
#ifndef SetBevelButtonMenuValue
    PyMac_PRECHECK(SetBevelButtonMenuValue);
#endif
    if (!PyArg_ParseTuple(_args, "h",
                          &inValue))
        return NULL;
    _err = SetBevelButtonMenuValue(_self->ob_itself,
                                   inValue);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetBevelButtonMenuHandle(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    MenuHandle outHandle;
#ifndef GetBevelButtonMenuHandle
    PyMac_PRECHECK(GetBevelButtonMenuHandle);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetBevelButtonMenuHandle(_self->ob_itself,
                                    &outHandle);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         MenuObj_New, outHandle);
    return _res;
}

static PyObject *CtlObj_SetBevelButtonContentInfo(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    ControlButtonContentInfo inContent;
#ifndef SetBevelButtonContentInfo
    PyMac_PRECHECK(SetBevelButtonContentInfo);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          ControlButtonContentInfo_Convert, &inContent))
        return NULL;
    _err = SetBevelButtonContentInfo(_self->ob_itself,
                                     &inContent);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_SetBevelButtonTransform(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    IconTransformType transform;
#ifndef SetBevelButtonTransform
    PyMac_PRECHECK(SetBevelButtonTransform);
#endif
    if (!PyArg_ParseTuple(_args, "h",
                          &transform))
        return NULL;
    _err = SetBevelButtonTransform(_self->ob_itself,
                                   transform);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_SetDisclosureTriangleLastValue(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    SInt16 inValue;
#ifndef SetDisclosureTriangleLastValue
    PyMac_PRECHECK(SetDisclosureTriangleLastValue);
#endif
    if (!PyArg_ParseTuple(_args, "h",
                          &inValue))
        return NULL;
    _err = SetDisclosureTriangleLastValue(_self->ob_itself,
                                          inValue);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetTabContentRect(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    Rect outContentRect;
#ifndef GetTabContentRect
    PyMac_PRECHECK(GetTabContentRect);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetTabContentRect(_self->ob_itself,
                             &outContentRect);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         PyMac_BuildRect, &outContentRect);
    return _res;
}

static PyObject *CtlObj_SetTabEnabled(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    SInt16 inTabToHilite;
    Boolean inEnabled;
#ifndef SetTabEnabled
    PyMac_PRECHECK(SetTabEnabled);
#endif
    if (!PyArg_ParseTuple(_args, "hb",
                          &inTabToHilite,
                          &inEnabled))
        return NULL;
    _err = SetTabEnabled(_self->ob_itself,
                         inTabToHilite,
                         inEnabled);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_SetImageWellContentInfo(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    ControlButtonContentInfo inContent;
#ifndef SetImageWellContentInfo
    PyMac_PRECHECK(SetImageWellContentInfo);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          ControlButtonContentInfo_Convert, &inContent))
        return NULL;
    _err = SetImageWellContentInfo(_self->ob_itself,
                                   &inContent);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_SetImageWellTransform(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    IconTransformType inTransform;
#ifndef SetImageWellTransform
    PyMac_PRECHECK(SetImageWellTransform);
#endif
    if (!PyArg_ParseTuple(_args, "h",
                          &inTransform))
        return NULL;
    _err = SetImageWellTransform(_self->ob_itself,
                                 inTransform);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetDataBrowserViewStyle(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    OSType style;
#ifndef GetDataBrowserViewStyle
    PyMac_PRECHECK(GetDataBrowserViewStyle);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetDataBrowserViewStyle(_self->ob_itself,
                                   &style);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         PyMac_BuildOSType, style);
    return _res;
}

static PyObject *CtlObj_SetDataBrowserViewStyle(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    OSType style;
#ifndef SetDataBrowserViewStyle
    PyMac_PRECHECK(SetDataBrowserViewStyle);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          PyMac_GetOSType, &style))
        return NULL;
    _err = SetDataBrowserViewStyle(_self->ob_itself,
                                   style);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_EnableDataBrowserEditCommand(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
    UInt32 command;
#ifndef EnableDataBrowserEditCommand
    PyMac_PRECHECK(EnableDataBrowserEditCommand);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &command))
        return NULL;
    _rv = EnableDataBrowserEditCommand(_self->ob_itself,
                                       command);
    _res = Py_BuildValue("b",
                         _rv);
    return _res;
}

static PyObject *CtlObj_ExecuteDataBrowserEditCommand(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 command;
#ifndef ExecuteDataBrowserEditCommand
    PyMac_PRECHECK(ExecuteDataBrowserEditCommand);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &command))
        return NULL;
    _err = ExecuteDataBrowserEditCommand(_self->ob_itself,
                                         command);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetDataBrowserSelectionAnchor(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 first;
    UInt32 last;
#ifndef GetDataBrowserSelectionAnchor
    PyMac_PRECHECK(GetDataBrowserSelectionAnchor);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetDataBrowserSelectionAnchor(_self->ob_itself,
                                         &first,
                                         &last);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("ll",
                         first,
                         last);
    return _res;
}

static PyObject *CtlObj_MoveDataBrowserSelectionAnchor(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 direction;
    Boolean extendSelection;
#ifndef MoveDataBrowserSelectionAnchor
    PyMac_PRECHECK(MoveDataBrowserSelectionAnchor);
#endif
    if (!PyArg_ParseTuple(_args, "lb",
                          &direction,
                          &extendSelection))
        return NULL;
    _err = MoveDataBrowserSelectionAnchor(_self->ob_itself,
                                          direction,
                                          extendSelection);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_OpenDataBrowserContainer(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 container;
#ifndef OpenDataBrowserContainer
    PyMac_PRECHECK(OpenDataBrowserContainer);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &container))
        return NULL;
    _err = OpenDataBrowserContainer(_self->ob_itself,
                                    container);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_CloseDataBrowserContainer(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 container;
#ifndef CloseDataBrowserContainer
    PyMac_PRECHECK(CloseDataBrowserContainer);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &container))
        return NULL;
    _err = CloseDataBrowserContainer(_self->ob_itself,
                                     container);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_SortDataBrowserContainer(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 container;
    Boolean sortChildren;
#ifndef SortDataBrowserContainer
    PyMac_PRECHECK(SortDataBrowserContainer);
#endif
    if (!PyArg_ParseTuple(_args, "lb",
                          &container,
                          &sortChildren))
        return NULL;
    _err = SortDataBrowserContainer(_self->ob_itself,
                                    container,
                                    sortChildren);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetDataBrowserItems(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 container;
    Boolean recurse;
    UInt32 state;
    Handle items;
#ifndef GetDataBrowserItems
    PyMac_PRECHECK(GetDataBrowserItems);
#endif
    if (!PyArg_ParseTuple(_args, "lblO&",
                          &container,
                          &recurse,
                          &state,
                          ResObj_Convert, &items))
        return NULL;
    _err = GetDataBrowserItems(_self->ob_itself,
                               container,
                               recurse,
                               state,
                               items);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetDataBrowserItemCount(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 container;
    Boolean recurse;
    UInt32 state;
    UInt32 numItems;
#ifndef GetDataBrowserItemCount
    PyMac_PRECHECK(GetDataBrowserItemCount);
#endif
    if (!PyArg_ParseTuple(_args, "lbl",
                          &container,
                          &recurse,
                          &state))
        return NULL;
    _err = GetDataBrowserItemCount(_self->ob_itself,
                                   container,
                                   recurse,
                                   state,
                                   &numItems);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("l",
                         numItems);
    return _res;
}

static PyObject *CtlObj_IsDataBrowserItemSelected(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
    UInt32 item;
#ifndef IsDataBrowserItemSelected
    PyMac_PRECHECK(IsDataBrowserItemSelected);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &item))
        return NULL;
    _rv = IsDataBrowserItemSelected(_self->ob_itself,
                                    item);
    _res = Py_BuildValue("b",
                         _rv);
    return _res;
}

static PyObject *CtlObj_GetDataBrowserItemState(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 item;
    UInt32 state;
#ifndef GetDataBrowserItemState
    PyMac_PRECHECK(GetDataBrowserItemState);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &item))
        return NULL;
    _err = GetDataBrowserItemState(_self->ob_itself,
                                   item,
                                   &state);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("l",
                         state);
    return _res;
}

static PyObject *CtlObj_RevealDataBrowserItem(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 item;
    UInt32 propertyID;
    UInt8 options;
#ifndef RevealDataBrowserItem
    PyMac_PRECHECK(RevealDataBrowserItem);
#endif
    if (!PyArg_ParseTuple(_args, "llb",
                          &item,
                          &propertyID,
                          &options))
        return NULL;
    _err = RevealDataBrowserItem(_self->ob_itself,
                                 item,
                                 propertyID,
                                 options);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_SetDataBrowserActiveItems(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Boolean active;
#ifndef SetDataBrowserActiveItems
    PyMac_PRECHECK(SetDataBrowserActiveItems);
#endif
    if (!PyArg_ParseTuple(_args, "b",
                          &active))
        return NULL;
    _err = SetDataBrowserActiveItems(_self->ob_itself,
                                     active);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetDataBrowserActiveItems(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Boolean active;
#ifndef GetDataBrowserActiveItems
    PyMac_PRECHECK(GetDataBrowserActiveItems);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetDataBrowserActiveItems(_self->ob_itself,
                                     &active);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("b",
                         active);
    return _res;
}

static PyObject *CtlObj_SetDataBrowserScrollBarInset(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect insetRect;
#ifndef SetDataBrowserScrollBarInset
    PyMac_PRECHECK(SetDataBrowserScrollBarInset);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = SetDataBrowserScrollBarInset(_self->ob_itself,
                                        &insetRect);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         PyMac_BuildRect, &insetRect);
    return _res;
}

static PyObject *CtlObj_GetDataBrowserScrollBarInset(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Rect insetRect;
#ifndef GetDataBrowserScrollBarInset
    PyMac_PRECHECK(GetDataBrowserScrollBarInset);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetDataBrowserScrollBarInset(_self->ob_itself,
                                        &insetRect);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         PyMac_BuildRect, &insetRect);
    return _res;
}

static PyObject *CtlObj_SetDataBrowserTarget(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 target;
#ifndef SetDataBrowserTarget
    PyMac_PRECHECK(SetDataBrowserTarget);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &target))
        return NULL;
    _err = SetDataBrowserTarget(_self->ob_itself,
                                target);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetDataBrowserTarget(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 target;
#ifndef GetDataBrowserTarget
    PyMac_PRECHECK(GetDataBrowserTarget);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetDataBrowserTarget(_self->ob_itself,
                                &target);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("l",
                         target);
    return _res;
}

static PyObject *CtlObj_SetDataBrowserSortOrder(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt16 order;
#ifndef SetDataBrowserSortOrder
    PyMac_PRECHECK(SetDataBrowserSortOrder);
#endif
    if (!PyArg_ParseTuple(_args, "H",
                          &order))
        return NULL;
    _err = SetDataBrowserSortOrder(_self->ob_itself,
                                   order);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetDataBrowserSortOrder(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt16 order;
#ifndef GetDataBrowserSortOrder
    PyMac_PRECHECK(GetDataBrowserSortOrder);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetDataBrowserSortOrder(_self->ob_itself,
                                   &order);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("H",
                         order);
    return _res;
}

static PyObject *CtlObj_SetDataBrowserScrollPosition(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 top;
    UInt32 left;
#ifndef SetDataBrowserScrollPosition
    PyMac_PRECHECK(SetDataBrowserScrollPosition);
#endif
    if (!PyArg_ParseTuple(_args, "ll",
                          &top,
                          &left))
        return NULL;
    _err = SetDataBrowserScrollPosition(_self->ob_itself,
                                        top,
                                        left);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetDataBrowserScrollPosition(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 top;
    UInt32 left;
#ifndef GetDataBrowserScrollPosition
    PyMac_PRECHECK(GetDataBrowserScrollPosition);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetDataBrowserScrollPosition(_self->ob_itself,
                                        &top,
                                        &left);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("ll",
                         top,
                         left);
    return _res;
}

static PyObject *CtlObj_SetDataBrowserHasScrollBars(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Boolean horiz;
    Boolean vert;
#ifndef SetDataBrowserHasScrollBars
    PyMac_PRECHECK(SetDataBrowserHasScrollBars);
#endif
    if (!PyArg_ParseTuple(_args, "bb",
                          &horiz,
                          &vert))
        return NULL;
    _err = SetDataBrowserHasScrollBars(_self->ob_itself,
                                       horiz,
                                       vert);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetDataBrowserHasScrollBars(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Boolean horiz;
    Boolean vert;
#ifndef GetDataBrowserHasScrollBars
    PyMac_PRECHECK(GetDataBrowserHasScrollBars);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetDataBrowserHasScrollBars(_self->ob_itself,
                                       &horiz,
                                       &vert);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("bb",
                         horiz,
                         vert);
    return _res;
}

static PyObject *CtlObj_SetDataBrowserSortProperty(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 property;
#ifndef SetDataBrowserSortProperty
    PyMac_PRECHECK(SetDataBrowserSortProperty);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &property))
        return NULL;
    _err = SetDataBrowserSortProperty(_self->ob_itself,
                                      property);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetDataBrowserSortProperty(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 property;
#ifndef GetDataBrowserSortProperty
    PyMac_PRECHECK(GetDataBrowserSortProperty);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetDataBrowserSortProperty(_self->ob_itself,
                                      &property);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("l",
                         property);
    return _res;
}

static PyObject *CtlObj_SetDataBrowserSelectionFlags(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 selectionFlags;
#ifndef SetDataBrowserSelectionFlags
    PyMac_PRECHECK(SetDataBrowserSelectionFlags);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &selectionFlags))
        return NULL;
    _err = SetDataBrowserSelectionFlags(_self->ob_itself,
                                        selectionFlags);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetDataBrowserSelectionFlags(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 selectionFlags;
#ifndef GetDataBrowserSelectionFlags
    PyMac_PRECHECK(GetDataBrowserSelectionFlags);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetDataBrowserSelectionFlags(_self->ob_itself,
                                        &selectionFlags);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("l",
                         selectionFlags);
    return _res;
}

static PyObject *CtlObj_SetDataBrowserPropertyFlags(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 property;
    UInt32 flags;
#ifndef SetDataBrowserPropertyFlags
    PyMac_PRECHECK(SetDataBrowserPropertyFlags);
#endif
    if (!PyArg_ParseTuple(_args, "ll",
                          &property,
                          &flags))
        return NULL;
    _err = SetDataBrowserPropertyFlags(_self->ob_itself,
                                       property,
                                       flags);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetDataBrowserPropertyFlags(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 property;
    UInt32 flags;
#ifndef GetDataBrowserPropertyFlags
    PyMac_PRECHECK(GetDataBrowserPropertyFlags);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &property))
        return NULL;
    _err = GetDataBrowserPropertyFlags(_self->ob_itself,
                                       property,
                                       &flags);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("l",
                         flags);
    return _res;
}

static PyObject *CtlObj_SetDataBrowserEditText(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    CFStringRef text;
#ifndef SetDataBrowserEditText
    PyMac_PRECHECK(SetDataBrowserEditText);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          CFStringRefObj_Convert, &text))
        return NULL;
    _err = SetDataBrowserEditText(_self->ob_itself,
                                  text);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_CopyDataBrowserEditText(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    CFStringRef text;
#ifndef CopyDataBrowserEditText
    PyMac_PRECHECK(CopyDataBrowserEditText);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = CopyDataBrowserEditText(_self->ob_itself,
                                   &text);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CFStringRefObj_New, text);
    return _res;
}

static PyObject *CtlObj_GetDataBrowserEditText(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    CFMutableStringRef text;
#ifndef GetDataBrowserEditText
    PyMac_PRECHECK(GetDataBrowserEditText);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          CFMutableStringRefObj_Convert, &text))
        return NULL;
    _err = GetDataBrowserEditText(_self->ob_itself,
                                  text);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_SetDataBrowserEditItem(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 item;
    UInt32 property;
#ifndef SetDataBrowserEditItem
    PyMac_PRECHECK(SetDataBrowserEditItem);
#endif
    if (!PyArg_ParseTuple(_args, "ll",
                          &item,
                          &property))
        return NULL;
    _err = SetDataBrowserEditItem(_self->ob_itself,
                                  item,
                                  property);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetDataBrowserEditItem(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 item;
    UInt32 property;
#ifndef GetDataBrowserEditItem
    PyMac_PRECHECK(GetDataBrowserEditItem);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetDataBrowserEditItem(_self->ob_itself,
                                  &item,
                                  &property);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("ll",
                         item,
                         property);
    return _res;
}

static PyObject *CtlObj_GetDataBrowserItemPartBounds(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 item;
    UInt32 property;
    OSType part;
    Rect bounds;
#ifndef GetDataBrowserItemPartBounds
    PyMac_PRECHECK(GetDataBrowserItemPartBounds);
#endif
    if (!PyArg_ParseTuple(_args, "llO&",
                          &item,
                          &property,
                          PyMac_GetOSType, &part))
        return NULL;
    _err = GetDataBrowserItemPartBounds(_self->ob_itself,
                                        item,
                                        property,
                                        part,
                                        &bounds);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         PyMac_BuildRect, &bounds);
    return _res;
}

static PyObject *CtlObj_RemoveDataBrowserTableViewColumn(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 column;
#ifndef RemoveDataBrowserTableViewColumn
    PyMac_PRECHECK(RemoveDataBrowserTableViewColumn);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &column))
        return NULL;
    _err = RemoveDataBrowserTableViewColumn(_self->ob_itself,
                                            column);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetDataBrowserTableViewColumnCount(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 numColumns;
#ifndef GetDataBrowserTableViewColumnCount
    PyMac_PRECHECK(GetDataBrowserTableViewColumnCount);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetDataBrowserTableViewColumnCount(_self->ob_itself,
                                              &numColumns);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("l",
                         numColumns);
    return _res;
}

static PyObject *CtlObj_SetDataBrowserTableViewHiliteStyle(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 hiliteStyle;
#ifndef SetDataBrowserTableViewHiliteStyle
    PyMac_PRECHECK(SetDataBrowserTableViewHiliteStyle);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &hiliteStyle))
        return NULL;
    _err = SetDataBrowserTableViewHiliteStyle(_self->ob_itself,
                                              hiliteStyle);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetDataBrowserTableViewHiliteStyle(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 hiliteStyle;
#ifndef GetDataBrowserTableViewHiliteStyle
    PyMac_PRECHECK(GetDataBrowserTableViewHiliteStyle);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetDataBrowserTableViewHiliteStyle(_self->ob_itself,
                                              &hiliteStyle);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("l",
                         hiliteStyle);
    return _res;
}

static PyObject *CtlObj_SetDataBrowserTableViewRowHeight(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt16 height;
#ifndef SetDataBrowserTableViewRowHeight
    PyMac_PRECHECK(SetDataBrowserTableViewRowHeight);
#endif
    if (!PyArg_ParseTuple(_args, "H",
                          &height))
        return NULL;
    _err = SetDataBrowserTableViewRowHeight(_self->ob_itself,
                                            height);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetDataBrowserTableViewRowHeight(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt16 height;
#ifndef GetDataBrowserTableViewRowHeight
    PyMac_PRECHECK(GetDataBrowserTableViewRowHeight);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetDataBrowserTableViewRowHeight(_self->ob_itself,
                                            &height);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("H",
                         height);
    return _res;
}

static PyObject *CtlObj_SetDataBrowserTableViewColumnWidth(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt16 width;
#ifndef SetDataBrowserTableViewColumnWidth
    PyMac_PRECHECK(SetDataBrowserTableViewColumnWidth);
#endif
    if (!PyArg_ParseTuple(_args, "H",
                          &width))
        return NULL;
    _err = SetDataBrowserTableViewColumnWidth(_self->ob_itself,
                                              width);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetDataBrowserTableViewColumnWidth(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt16 width;
#ifndef GetDataBrowserTableViewColumnWidth
    PyMac_PRECHECK(GetDataBrowserTableViewColumnWidth);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetDataBrowserTableViewColumnWidth(_self->ob_itself,
                                              &width);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("H",
                         width);
    return _res;
}

static PyObject *CtlObj_SetDataBrowserTableViewItemRowHeight(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 item;
    UInt16 height;
#ifndef SetDataBrowserTableViewItemRowHeight
    PyMac_PRECHECK(SetDataBrowserTableViewItemRowHeight);
#endif
    if (!PyArg_ParseTuple(_args, "lH",
                          &item,
                          &height))
        return NULL;
    _err = SetDataBrowserTableViewItemRowHeight(_self->ob_itself,
                                                item,
                                                height);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetDataBrowserTableViewItemRowHeight(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 item;
    UInt16 height;
#ifndef GetDataBrowserTableViewItemRowHeight
    PyMac_PRECHECK(GetDataBrowserTableViewItemRowHeight);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &item))
        return NULL;
    _err = GetDataBrowserTableViewItemRowHeight(_self->ob_itself,
                                                item,
                                                &height);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("H",
                         height);
    return _res;
}

static PyObject *CtlObj_SetDataBrowserTableViewNamedColumnWidth(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 column;
    UInt16 width;
#ifndef SetDataBrowserTableViewNamedColumnWidth
    PyMac_PRECHECK(SetDataBrowserTableViewNamedColumnWidth);
#endif
    if (!PyArg_ParseTuple(_args, "lH",
                          &column,
                          &width))
        return NULL;
    _err = SetDataBrowserTableViewNamedColumnWidth(_self->ob_itself,
                                                   column,
                                                   width);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetDataBrowserTableViewNamedColumnWidth(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 column;
    UInt16 width;
#ifndef GetDataBrowserTableViewNamedColumnWidth
    PyMac_PRECHECK(GetDataBrowserTableViewNamedColumnWidth);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &column))
        return NULL;
    _err = GetDataBrowserTableViewNamedColumnWidth(_self->ob_itself,
                                                   column,
                                                   &width);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("H",
                         width);
    return _res;
}

static PyObject *CtlObj_SetDataBrowserTableViewGeometry(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Boolean variableWidthColumns;
    Boolean variableHeightRows;
#ifndef SetDataBrowserTableViewGeometry
    PyMac_PRECHECK(SetDataBrowserTableViewGeometry);
#endif
    if (!PyArg_ParseTuple(_args, "bb",
                          &variableWidthColumns,
                          &variableHeightRows))
        return NULL;
    _err = SetDataBrowserTableViewGeometry(_self->ob_itself,
                                           variableWidthColumns,
                                           variableHeightRows);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetDataBrowserTableViewGeometry(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Boolean variableWidthColumns;
    Boolean variableHeightRows;
#ifndef GetDataBrowserTableViewGeometry
    PyMac_PRECHECK(GetDataBrowserTableViewGeometry);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetDataBrowserTableViewGeometry(_self->ob_itself,
                                           &variableWidthColumns,
                                           &variableHeightRows);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("bb",
                         variableWidthColumns,
                         variableHeightRows);
    return _res;
}

static PyObject *CtlObj_GetDataBrowserTableViewItemID(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 row;
    UInt32 item;
#ifndef GetDataBrowserTableViewItemID
    PyMac_PRECHECK(GetDataBrowserTableViewItemID);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &row))
        return NULL;
    _err = GetDataBrowserTableViewItemID(_self->ob_itself,
                                         row,
                                         &item);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("l",
                         item);
    return _res;
}

static PyObject *CtlObj_SetDataBrowserTableViewItemRow(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 item;
    UInt32 row;
#ifndef SetDataBrowserTableViewItemRow
    PyMac_PRECHECK(SetDataBrowserTableViewItemRow);
#endif
    if (!PyArg_ParseTuple(_args, "ll",
                          &item,
                          &row))
        return NULL;
    _err = SetDataBrowserTableViewItemRow(_self->ob_itself,
                                          item,
                                          row);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetDataBrowserTableViewItemRow(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 item;
    UInt32 row;
#ifndef GetDataBrowserTableViewItemRow
    PyMac_PRECHECK(GetDataBrowserTableViewItemRow);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &item))
        return NULL;
    _err = GetDataBrowserTableViewItemRow(_self->ob_itself,
                                          item,
                                          &row);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("l",
                         row);
    return _res;
}

static PyObject *CtlObj_SetDataBrowserTableViewColumnPosition(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 column;
    UInt32 position;
#ifndef SetDataBrowserTableViewColumnPosition
    PyMac_PRECHECK(SetDataBrowserTableViewColumnPosition);
#endif
    if (!PyArg_ParseTuple(_args, "ll",
                          &column,
                          &position))
        return NULL;
    _err = SetDataBrowserTableViewColumnPosition(_self->ob_itself,
                                                 column,
                                                 position);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetDataBrowserTableViewColumnPosition(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 column;
    UInt32 position;
#ifndef GetDataBrowserTableViewColumnPosition
    PyMac_PRECHECK(GetDataBrowserTableViewColumnPosition);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &column))
        return NULL;
    _err = GetDataBrowserTableViewColumnPosition(_self->ob_itself,
                                                 column,
                                                 &position);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("l",
                         position);
    return _res;
}

static PyObject *CtlObj_GetDataBrowserTableViewColumnProperty(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 column;
    UInt32 property;
#ifndef GetDataBrowserTableViewColumnProperty
    PyMac_PRECHECK(GetDataBrowserTableViewColumnProperty);
#endif
    if (!PyArg_ParseTuple(_args, "l",
                          &column))
        return NULL;
    _err = GetDataBrowserTableViewColumnProperty(_self->ob_itself,
                                                 column,
                                                 &property);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("l",
                         property);
    return _res;
}

static PyObject *CtlObj_AutoSizeDataBrowserListViewColumns(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
#ifndef AutoSizeDataBrowserListViewColumns
    PyMac_PRECHECK(AutoSizeDataBrowserListViewColumns);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = AutoSizeDataBrowserListViewColumns(_self->ob_itself);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_AddDataBrowserListViewColumn(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    DataBrowserListViewColumnDesc columnDesc;
    UInt32 position;
#ifndef AddDataBrowserListViewColumn
    PyMac_PRECHECK(AddDataBrowserListViewColumn);
#endif
    if (!PyArg_ParseTuple(_args, "O&l",
                          DataBrowserListViewColumnDesc_Convert, &columnDesc,
                          &position))
        return NULL;
    _err = AddDataBrowserListViewColumn(_self->ob_itself,
                                        &columnDesc,
                                        position);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_SetDataBrowserListViewHeaderBtnHeight(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt16 height;
#ifndef SetDataBrowserListViewHeaderBtnHeight
    PyMac_PRECHECK(SetDataBrowserListViewHeaderBtnHeight);
#endif
    if (!PyArg_ParseTuple(_args, "H",
                          &height))
        return NULL;
    _err = SetDataBrowserListViewHeaderBtnHeight(_self->ob_itself,
                                                 height);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetDataBrowserListViewHeaderBtnHeight(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt16 height;
#ifndef GetDataBrowserListViewHeaderBtnHeight
    PyMac_PRECHECK(GetDataBrowserListViewHeaderBtnHeight);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetDataBrowserListViewHeaderBtnHeight(_self->ob_itself,
                                                 &height);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("H",
                         height);
    return _res;
}

static PyObject *CtlObj_SetDataBrowserListViewUsePlainBackground(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Boolean usePlainBackground;
#ifndef SetDataBrowserListViewUsePlainBackground
    PyMac_PRECHECK(SetDataBrowserListViewUsePlainBackground);
#endif
    if (!PyArg_ParseTuple(_args, "b",
                          &usePlainBackground))
        return NULL;
    _err = SetDataBrowserListViewUsePlainBackground(_self->ob_itself,
                                                    usePlainBackground);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetDataBrowserListViewUsePlainBackground(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Boolean usePlainBackground;
#ifndef GetDataBrowserListViewUsePlainBackground
    PyMac_PRECHECK(GetDataBrowserListViewUsePlainBackground);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetDataBrowserListViewUsePlainBackground(_self->ob_itself,
                                                    &usePlainBackground);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("b",
                         usePlainBackground);
    return _res;
}

static PyObject *CtlObj_SetDataBrowserListViewDisclosureColumn(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 column;
    Boolean expandableRows;
#ifndef SetDataBrowserListViewDisclosureColumn
    PyMac_PRECHECK(SetDataBrowserListViewDisclosureColumn);
#endif
    if (!PyArg_ParseTuple(_args, "lb",
                          &column,
                          &expandableRows))
        return NULL;
    _err = SetDataBrowserListViewDisclosureColumn(_self->ob_itself,
                                                  column,
                                                  expandableRows);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetDataBrowserListViewDisclosureColumn(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 column;
    Boolean expandableRows;
#ifndef GetDataBrowserListViewDisclosureColumn
    PyMac_PRECHECK(GetDataBrowserListViewDisclosureColumn);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetDataBrowserListViewDisclosureColumn(_self->ob_itself,
                                                  &column,
                                                  &expandableRows);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("lb",
                         column,
                         expandableRows);
    return _res;
}

static PyObject *CtlObj_GetDataBrowserColumnViewPath(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Handle path;
#ifndef GetDataBrowserColumnViewPath
    PyMac_PRECHECK(GetDataBrowserColumnViewPath);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          ResObj_Convert, &path))
        return NULL;
    _err = GetDataBrowserColumnViewPath(_self->ob_itself,
                                        path);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetDataBrowserColumnViewPathLength(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 pathLength;
#ifndef GetDataBrowserColumnViewPathLength
    PyMac_PRECHECK(GetDataBrowserColumnViewPathLength);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetDataBrowserColumnViewPathLength(_self->ob_itself,
                                              &pathLength);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("l",
                         pathLength);
    return _res;
}

static PyObject *CtlObj_SetDataBrowserColumnViewDisplayType(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    OSType propertyType;
#ifndef SetDataBrowserColumnViewDisplayType
    PyMac_PRECHECK(SetDataBrowserColumnViewDisplayType);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          PyMac_GetOSType, &propertyType))
        return NULL;
    _err = SetDataBrowserColumnViewDisplayType(_self->ob_itself,
                                               propertyType);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CtlObj_GetDataBrowserColumnViewDisplayType(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    OSType propertyType;
#ifndef GetDataBrowserColumnViewDisplayType
    PyMac_PRECHECK(GetDataBrowserColumnViewDisplayType);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = GetDataBrowserColumnViewDisplayType(_self->ob_itself,
                                               &propertyType);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         PyMac_BuildOSType, propertyType);
    return _res;
}

static PyObject *CtlObj_as_Resource(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Handle _rv;
#ifndef as_Resource
    PyMac_PRECHECK(as_Resource);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = as_Resource(_self->ob_itself);
    _res = Py_BuildValue("O&",
                         ResObj_New, _rv);
    return _res;
}

static PyObject *CtlObj_GetControlRect(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Rect rect;
#ifndef GetControlRect
    PyMac_PRECHECK(GetControlRect);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    GetControlRect(_self->ob_itself,
                   &rect);
    _res = Py_BuildValue("O&",
                         PyMac_BuildRect, &rect);
    return _res;
}

static PyObject *CtlObj_DisposeControl(ControlObject *_self, PyObject *_args)
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

static PyObject *CtlObj_TrackControl(ControlObject *_self, PyObject *_args)
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

static PyObject *CtlObj_HandleControlClick(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;

    ControlPartCode _rv;
    Point startPoint;
    SInt16 modifiers;
    ControlActionUPP upp = 0;
    PyObject *callback = 0;

    if (!PyArg_ParseTuple(_args, "O&h|O",
                          PyMac_GetPoint, &startPoint,
                          &modifiers,
                          &callback))
        return NULL;
    if (callback && callback != Py_None) {
        if (PyInt_Check(callback) && PyInt_AS_LONG(callback) == -1)
            upp = (ControlActionUPP)-1;
        else {
            settrackfunc(callback);
            upp = mytracker_upp;
        }
    }
    _rv = HandleControlClick(_self->ob_itself,
                       startPoint,
                       modifiers,
                       upp);
    clrtrackfunc();
    _res = Py_BuildValue("h",
                         _rv);
    return _res;

}

static PyObject *CtlObj_SetControlData(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;

    OSErr _err;
    ControlPartCode inPart;
    ResType inTagName;
    Size bufferSize;
    Ptr buffer;

    if (!PyArg_ParseTuple(_args, "hO&s#",
                          &inPart,
                          PyMac_GetOSType, &inTagName,
                          &buffer, &bufferSize))
        return NULL;

    _err = SetControlData(_self->ob_itself,
                          inPart,
                          inTagName,
                          bufferSize,
                          buffer);

    if (_err != noErr)
        return PyMac_Error(_err);
    _res = Py_None;
    return _res;

}

static PyObject *CtlObj_GetControlData(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;

    OSErr _err;
    ControlPartCode inPart;
    ResType inTagName;
    Size bufferSize;
    Ptr buffer;
    Size outSize;

    if (!PyArg_ParseTuple(_args, "hO&",
                          &inPart,
                          PyMac_GetOSType, &inTagName))
        return NULL;

    /* allocate a buffer for the data */
    _err = GetControlDataSize(_self->ob_itself,
                              inPart,
                              inTagName,
                              &bufferSize);
    if (_err != noErr)
        return PyMac_Error(_err);
    buffer = PyMem_NEW(char, bufferSize);
    if (buffer == NULL)
        return PyErr_NoMemory();

    _err = GetControlData(_self->ob_itself,
                          inPart,
                          inTagName,
                          bufferSize,
                          buffer,
                          &outSize);

    if (_err != noErr) {
        PyMem_DEL(buffer);
        return PyMac_Error(_err);
    }
    _res = Py_BuildValue("s#", buffer, outSize);
    PyMem_DEL(buffer);
    return _res;

}

static PyObject *CtlObj_SetControlData_Handle(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;

    OSErr _err;
    ControlPartCode inPart;
    ResType inTagName;
    Handle buffer;

    if (!PyArg_ParseTuple(_args, "hO&O&",
                          &inPart,
                          PyMac_GetOSType, &inTagName,
                          OptResObj_Convert, &buffer))
        return NULL;

    _err = SetControlData(_self->ob_itself,
                          inPart,
                          inTagName,
                          sizeof(buffer),
                          (Ptr)&buffer);

    if (_err != noErr)
        return PyMac_Error(_err);
    _res = Py_None;
    return _res;

}

static PyObject *CtlObj_GetControlData_Handle(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;

    OSErr _err;
    ControlPartCode inPart;
    ResType inTagName;
    Size bufferSize;
    Handle hdl;

    if (!PyArg_ParseTuple(_args, "hO&",
                          &inPart,
                          PyMac_GetOSType, &inTagName))
        return NULL;

    /* Check it is handle-sized */
    _err = GetControlDataSize(_self->ob_itself,
                              inPart,
                              inTagName,
                              &bufferSize);
    if (_err != noErr)
        return PyMac_Error(_err);
    if (bufferSize != sizeof(Handle)) {
        PyErr_SetString(Ctl_Error, "GetControlDataSize() != sizeof(Handle)");
        return NULL;
    }

    _err = GetControlData(_self->ob_itself,
                          inPart,
                          inTagName,
                          sizeof(Handle),
                          (Ptr)&hdl,
                          &bufferSize);

    if (_err != noErr) {
        return PyMac_Error(_err);
    }
    _res = Py_BuildValue("O&", OptResObj_New, hdl);
    return _res;

}

static PyObject *CtlObj_SetControlData_Callback(ControlObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;

    OSErr _err;
    ControlPartCode inPart;
    ResType inTagName;
    PyObject *callback;
    UniversalProcPtr c_callback;

    if (!PyArg_ParseTuple(_args, "hO&O",
                          &inPart,
                          PyMac_GetOSType, &inTagName,
                          &callback))
        return NULL;

    if ( setcallback((PyObject *)_self, inTagName, callback, &c_callback) < 0 )
        return NULL;
    _err = SetControlData(_self->ob_itself,
                          inPart,
                          inTagName,
                          sizeof(c_callback),
                          (Ptr)&c_callback);

    if (_err != noErr)
        return PyMac_Error(_err);
    _res = Py_None;
    return _res;

}

static PyMethodDef CtlObj_methods[] = {
    {"HiliteControl", (PyCFunction)CtlObj_HiliteControl, 1,
     PyDoc_STR("(ControlPartCode hiliteState) -> None")},
    {"ShowControl", (PyCFunction)CtlObj_ShowControl, 1,
     PyDoc_STR("() -> None")},
    {"HideControl", (PyCFunction)CtlObj_HideControl, 1,
     PyDoc_STR("() -> None")},
    {"IsControlActive", (PyCFunction)CtlObj_IsControlActive, 1,
     PyDoc_STR("() -> (Boolean _rv)")},
    {"IsControlVisible", (PyCFunction)CtlObj_IsControlVisible, 1,
     PyDoc_STR("() -> (Boolean _rv)")},
    {"ActivateControl", (PyCFunction)CtlObj_ActivateControl, 1,
     PyDoc_STR("() -> None")},
    {"DeactivateControl", (PyCFunction)CtlObj_DeactivateControl, 1,
     PyDoc_STR("() -> None")},
    {"SetControlVisibility", (PyCFunction)CtlObj_SetControlVisibility, 1,
     PyDoc_STR("(Boolean inIsVisible, Boolean inDoDraw) -> None")},
    {"IsControlEnabled", (PyCFunction)CtlObj_IsControlEnabled, 1,
     PyDoc_STR("() -> (Boolean _rv)")},
    {"EnableControl", (PyCFunction)CtlObj_EnableControl, 1,
     PyDoc_STR("() -> None")},
    {"DisableControl", (PyCFunction)CtlObj_DisableControl, 1,
     PyDoc_STR("() -> None")},
    {"Draw1Control", (PyCFunction)CtlObj_Draw1Control, 1,
     PyDoc_STR("() -> None")},
    {"GetBestControlRect", (PyCFunction)CtlObj_GetBestControlRect, 1,
     PyDoc_STR("() -> (Rect outRect, SInt16 outBaseLineOffset)")},
    {"SetControlFontStyle", (PyCFunction)CtlObj_SetControlFontStyle, 1,
     PyDoc_STR("(ControlFontStyleRec inStyle) -> None")},
    {"DrawControlInCurrentPort", (PyCFunction)CtlObj_DrawControlInCurrentPort, 1,
     PyDoc_STR("() -> None")},
    {"SetUpControlBackground", (PyCFunction)CtlObj_SetUpControlBackground, 1,
     PyDoc_STR("(SInt16 inDepth, Boolean inIsColorDevice) -> None")},
    {"SetUpControlTextColor", (PyCFunction)CtlObj_SetUpControlTextColor, 1,
     PyDoc_STR("(SInt16 inDepth, Boolean inIsColorDevice) -> None")},
    {"DragControl", (PyCFunction)CtlObj_DragControl, 1,
     PyDoc_STR("(Point startPoint, Rect limitRect, Rect slopRect, DragConstraint axis) -> None")},
    {"TestControl", (PyCFunction)CtlObj_TestControl, 1,
     PyDoc_STR("(Point testPoint) -> (ControlPartCode _rv)")},
    {"HandleControlContextualMenuClick", (PyCFunction)CtlObj_HandleControlContextualMenuClick, 1,
     PyDoc_STR("(Point inWhere) -> (Boolean menuDisplayed)")},
    {"GetControlClickActivation", (PyCFunction)CtlObj_GetControlClickActivation, 1,
     PyDoc_STR("(Point inWhere, EventModifiers inModifiers) -> (ClickActivationResult outResult)")},
    {"HandleControlKey", (PyCFunction)CtlObj_HandleControlKey, 1,
     PyDoc_STR("(SInt16 inKeyCode, SInt16 inCharCode, EventModifiers inModifiers) -> (ControlPartCode _rv)")},
    {"HandleControlSetCursor", (PyCFunction)CtlObj_HandleControlSetCursor, 1,
     PyDoc_STR("(Point localPoint, EventModifiers modifiers) -> (Boolean cursorWasSet)")},
    {"MoveControl", (PyCFunction)CtlObj_MoveControl, 1,
     PyDoc_STR("(SInt16 h, SInt16 v) -> None")},
    {"SizeControl", (PyCFunction)CtlObj_SizeControl, 1,
     PyDoc_STR("(SInt16 w, SInt16 h) -> None")},
    {"SetControlTitle", (PyCFunction)CtlObj_SetControlTitle, 1,
     PyDoc_STR("(Str255 title) -> None")},
    {"GetControlTitle", (PyCFunction)CtlObj_GetControlTitle, 1,
     PyDoc_STR("() -> (Str255 title)")},
    {"SetControlTitleWithCFString", (PyCFunction)CtlObj_SetControlTitleWithCFString, 1,
     PyDoc_STR("(CFStringRef inString) -> None")},
    {"CopyControlTitleAsCFString", (PyCFunction)CtlObj_CopyControlTitleAsCFString, 1,
     PyDoc_STR("() -> (CFStringRef outString)")},
    {"GetControlValue", (PyCFunction)CtlObj_GetControlValue, 1,
     PyDoc_STR("() -> (SInt16 _rv)")},
    {"SetControlValue", (PyCFunction)CtlObj_SetControlValue, 1,
     PyDoc_STR("(SInt16 newValue) -> None")},
    {"GetControlMinimum", (PyCFunction)CtlObj_GetControlMinimum, 1,
     PyDoc_STR("() -> (SInt16 _rv)")},
    {"SetControlMinimum", (PyCFunction)CtlObj_SetControlMinimum, 1,
     PyDoc_STR("(SInt16 newMinimum) -> None")},
    {"GetControlMaximum", (PyCFunction)CtlObj_GetControlMaximum, 1,
     PyDoc_STR("() -> (SInt16 _rv)")},
    {"SetControlMaximum", (PyCFunction)CtlObj_SetControlMaximum, 1,
     PyDoc_STR("(SInt16 newMaximum) -> None")},
    {"GetControlViewSize", (PyCFunction)CtlObj_GetControlViewSize, 1,
     PyDoc_STR("() -> (SInt32 _rv)")},
    {"SetControlViewSize", (PyCFunction)CtlObj_SetControlViewSize, 1,
     PyDoc_STR("(SInt32 newViewSize) -> None")},
    {"GetControl32BitValue", (PyCFunction)CtlObj_GetControl32BitValue, 1,
     PyDoc_STR("() -> (SInt32 _rv)")},
    {"SetControl32BitValue", (PyCFunction)CtlObj_SetControl32BitValue, 1,
     PyDoc_STR("(SInt32 newValue) -> None")},
    {"GetControl32BitMaximum", (PyCFunction)CtlObj_GetControl32BitMaximum, 1,
     PyDoc_STR("() -> (SInt32 _rv)")},
    {"SetControl32BitMaximum", (PyCFunction)CtlObj_SetControl32BitMaximum, 1,
     PyDoc_STR("(SInt32 newMaximum) -> None")},
    {"GetControl32BitMinimum", (PyCFunction)CtlObj_GetControl32BitMinimum, 1,
     PyDoc_STR("() -> (SInt32 _rv)")},
    {"SetControl32BitMinimum", (PyCFunction)CtlObj_SetControl32BitMinimum, 1,
     PyDoc_STR("(SInt32 newMinimum) -> None")},
    {"IsValidControlHandle", (PyCFunction)CtlObj_IsValidControlHandle, 1,
     PyDoc_STR("() -> (Boolean _rv)")},
    {"SetControlID", (PyCFunction)CtlObj_SetControlID, 1,
     PyDoc_STR("(ControlID inID) -> None")},
    {"GetControlID", (PyCFunction)CtlObj_GetControlID, 1,
     PyDoc_STR("() -> (ControlID outID)")},
    {"SetControlCommandID", (PyCFunction)CtlObj_SetControlCommandID, 1,
     PyDoc_STR("(UInt32 inCommandID) -> None")},
    {"GetControlCommandID", (PyCFunction)CtlObj_GetControlCommandID, 1,
     PyDoc_STR("() -> (UInt32 outCommandID)")},
    {"RemoveControlProperty", (PyCFunction)CtlObj_RemoveControlProperty, 1,
     PyDoc_STR("(OSType propertyCreator, OSType propertyTag) -> None")},
    {"GetControlPropertyAttributes", (PyCFunction)CtlObj_GetControlPropertyAttributes, 1,
     PyDoc_STR("(OSType propertyCreator, OSType propertyTag) -> (UInt32 attributes)")},
    {"ChangeControlPropertyAttributes", (PyCFunction)CtlObj_ChangeControlPropertyAttributes, 1,
     PyDoc_STR("(OSType propertyCreator, OSType propertyTag, UInt32 attributesToSet, UInt32 attributesToClear) -> None")},
    {"GetControlRegion", (PyCFunction)CtlObj_GetControlRegion, 1,
     PyDoc_STR("(ControlPartCode inPart, RgnHandle outRegion) -> None")},
    {"GetControlVariant", (PyCFunction)CtlObj_GetControlVariant, 1,
     PyDoc_STR("() -> (ControlVariant _rv)")},
    {"SetControlAction", (PyCFunction)CtlObj_SetControlAction, 1,
     PyDoc_STR("(PyObject* actionProc) -> None")},
    {"SetControlReference", (PyCFunction)CtlObj_SetControlReference, 1,
     PyDoc_STR("(SInt32 data) -> None")},
    {"GetControlReference", (PyCFunction)CtlObj_GetControlReference, 1,
     PyDoc_STR("() -> (SInt32 _rv)")},
    {"EmbedControl", (PyCFunction)CtlObj_EmbedControl, 1,
     PyDoc_STR("(ControlHandle inContainer) -> None")},
    {"AutoEmbedControl", (PyCFunction)CtlObj_AutoEmbedControl, 1,
     PyDoc_STR("(WindowPtr inWindow) -> None")},
    {"GetSuperControl", (PyCFunction)CtlObj_GetSuperControl, 1,
     PyDoc_STR("() -> (ControlHandle outParent)")},
    {"CountSubControls", (PyCFunction)CtlObj_CountSubControls, 1,
     PyDoc_STR("() -> (UInt16 outNumChildren)")},
    {"GetIndexedSubControl", (PyCFunction)CtlObj_GetIndexedSubControl, 1,
     PyDoc_STR("(UInt16 inIndex) -> (ControlHandle outSubControl)")},
    {"SetControlSupervisor", (PyCFunction)CtlObj_SetControlSupervisor, 1,
     PyDoc_STR("(ControlHandle inBoss) -> None")},
    {"GetControlFeatures", (PyCFunction)CtlObj_GetControlFeatures, 1,
     PyDoc_STR("() -> (UInt32 outFeatures)")},
    {"GetControlDataSize", (PyCFunction)CtlObj_GetControlDataSize, 1,
     PyDoc_STR("(ControlPartCode inPart, ResType inTagName) -> (Size outMaxSize)")},
    {"HandleControlDragTracking", (PyCFunction)CtlObj_HandleControlDragTracking, 1,
     PyDoc_STR("(DragTrackingMessage inMessage, DragReference inDrag) -> (Boolean outLikesDrag)")},
    {"HandleControlDragReceive", (PyCFunction)CtlObj_HandleControlDragReceive, 1,
     PyDoc_STR("(DragReference inDrag) -> None")},
    {"SetControlDragTrackingEnabled", (PyCFunction)CtlObj_SetControlDragTrackingEnabled, 1,
     PyDoc_STR("(Boolean inTracks) -> None")},
    {"IsControlDragTrackingEnabled", (PyCFunction)CtlObj_IsControlDragTrackingEnabled, 1,
     PyDoc_STR("() -> (Boolean outTracks)")},
    {"GetControlBounds", (PyCFunction)CtlObj_GetControlBounds, 1,
     PyDoc_STR("() -> (Rect bounds)")},
    {"IsControlHilited", (PyCFunction)CtlObj_IsControlHilited, 1,
     PyDoc_STR("() -> (Boolean _rv)")},
    {"GetControlHilite", (PyCFunction)CtlObj_GetControlHilite, 1,
     PyDoc_STR("() -> (UInt16 _rv)")},
    {"GetControlOwner", (PyCFunction)CtlObj_GetControlOwner, 1,
     PyDoc_STR("() -> (WindowPtr _rv)")},
    {"GetControlDataHandle", (PyCFunction)CtlObj_GetControlDataHandle, 1,
     PyDoc_STR("() -> (Handle _rv)")},
    {"GetControlPopupMenuHandle", (PyCFunction)CtlObj_GetControlPopupMenuHandle, 1,
     PyDoc_STR("() -> (MenuHandle _rv)")},
    {"GetControlPopupMenuID", (PyCFunction)CtlObj_GetControlPopupMenuID, 1,
     PyDoc_STR("() -> (short _rv)")},
    {"SetControlDataHandle", (PyCFunction)CtlObj_SetControlDataHandle, 1,
     PyDoc_STR("(Handle dataHandle) -> None")},
    {"SetControlBounds", (PyCFunction)CtlObj_SetControlBounds, 1,
     PyDoc_STR("(Rect bounds) -> None")},
    {"SetControlPopupMenuHandle", (PyCFunction)CtlObj_SetControlPopupMenuHandle, 1,
     PyDoc_STR("(MenuHandle popupMenu) -> None")},
    {"SetControlPopupMenuID", (PyCFunction)CtlObj_SetControlPopupMenuID, 1,
     PyDoc_STR("(short menuID) -> None")},
    {"GetBevelButtonMenuValue", (PyCFunction)CtlObj_GetBevelButtonMenuValue, 1,
     PyDoc_STR("() -> (SInt16 outValue)")},
    {"SetBevelButtonMenuValue", (PyCFunction)CtlObj_SetBevelButtonMenuValue, 1,
     PyDoc_STR("(SInt16 inValue) -> None")},
    {"GetBevelButtonMenuHandle", (PyCFunction)CtlObj_GetBevelButtonMenuHandle, 1,
     PyDoc_STR("() -> (MenuHandle outHandle)")},
    {"SetBevelButtonContentInfo", (PyCFunction)CtlObj_SetBevelButtonContentInfo, 1,
     PyDoc_STR("(ControlButtonContentInfo inContent) -> None")},
    {"SetBevelButtonTransform", (PyCFunction)CtlObj_SetBevelButtonTransform, 1,
     PyDoc_STR("(IconTransformType transform) -> None")},
    {"SetDisclosureTriangleLastValue", (PyCFunction)CtlObj_SetDisclosureTriangleLastValue, 1,
     PyDoc_STR("(SInt16 inValue) -> None")},
    {"GetTabContentRect", (PyCFunction)CtlObj_GetTabContentRect, 1,
     PyDoc_STR("() -> (Rect outContentRect)")},
    {"SetTabEnabled", (PyCFunction)CtlObj_SetTabEnabled, 1,
     PyDoc_STR("(SInt16 inTabToHilite, Boolean inEnabled) -> None")},
    {"SetImageWellContentInfo", (PyCFunction)CtlObj_SetImageWellContentInfo, 1,
     PyDoc_STR("(ControlButtonContentInfo inContent) -> None")},
    {"SetImageWellTransform", (PyCFunction)CtlObj_SetImageWellTransform, 1,
     PyDoc_STR("(IconTransformType inTransform) -> None")},
    {"GetDataBrowserViewStyle", (PyCFunction)CtlObj_GetDataBrowserViewStyle, 1,
     PyDoc_STR("() -> (OSType style)")},
    {"SetDataBrowserViewStyle", (PyCFunction)CtlObj_SetDataBrowserViewStyle, 1,
     PyDoc_STR("(OSType style) -> None")},
    {"EnableDataBrowserEditCommand", (PyCFunction)CtlObj_EnableDataBrowserEditCommand, 1,
     PyDoc_STR("(UInt32 command) -> (Boolean _rv)")},
    {"ExecuteDataBrowserEditCommand", (PyCFunction)CtlObj_ExecuteDataBrowserEditCommand, 1,
     PyDoc_STR("(UInt32 command) -> None")},
    {"GetDataBrowserSelectionAnchor", (PyCFunction)CtlObj_GetDataBrowserSelectionAnchor, 1,
     PyDoc_STR("() -> (UInt32 first, UInt32 last)")},
    {"MoveDataBrowserSelectionAnchor", (PyCFunction)CtlObj_MoveDataBrowserSelectionAnchor, 1,
     PyDoc_STR("(UInt32 direction, Boolean extendSelection) -> None")},
    {"OpenDataBrowserContainer", (PyCFunction)CtlObj_OpenDataBrowserContainer, 1,
     PyDoc_STR("(UInt32 container) -> None")},
    {"CloseDataBrowserContainer", (PyCFunction)CtlObj_CloseDataBrowserContainer, 1,
     PyDoc_STR("(UInt32 container) -> None")},
    {"SortDataBrowserContainer", (PyCFunction)CtlObj_SortDataBrowserContainer, 1,
     PyDoc_STR("(UInt32 container, Boolean sortChildren) -> None")},
    {"GetDataBrowserItems", (PyCFunction)CtlObj_GetDataBrowserItems, 1,
     PyDoc_STR("(UInt32 container, Boolean recurse, UInt32 state, Handle items) -> None")},
    {"GetDataBrowserItemCount", (PyCFunction)CtlObj_GetDataBrowserItemCount, 1,
     PyDoc_STR("(UInt32 container, Boolean recurse, UInt32 state) -> (UInt32 numItems)")},
    {"IsDataBrowserItemSelected", (PyCFunction)CtlObj_IsDataBrowserItemSelected, 1,
     PyDoc_STR("(UInt32 item) -> (Boolean _rv)")},
    {"GetDataBrowserItemState", (PyCFunction)CtlObj_GetDataBrowserItemState, 1,
     PyDoc_STR("(UInt32 item) -> (UInt32 state)")},
    {"RevealDataBrowserItem", (PyCFunction)CtlObj_RevealDataBrowserItem, 1,
     PyDoc_STR("(UInt32 item, UInt32 propertyID, UInt8 options) -> None")},
    {"SetDataBrowserActiveItems", (PyCFunction)CtlObj_SetDataBrowserActiveItems, 1,
     PyDoc_STR("(Boolean active) -> None")},
    {"GetDataBrowserActiveItems", (PyCFunction)CtlObj_GetDataBrowserActiveItems, 1,
     PyDoc_STR("() -> (Boolean active)")},
    {"SetDataBrowserScrollBarInset", (PyCFunction)CtlObj_SetDataBrowserScrollBarInset, 1,
     PyDoc_STR("() -> (Rect insetRect)")},
    {"GetDataBrowserScrollBarInset", (PyCFunction)CtlObj_GetDataBrowserScrollBarInset, 1,
     PyDoc_STR("() -> (Rect insetRect)")},
    {"SetDataBrowserTarget", (PyCFunction)CtlObj_SetDataBrowserTarget, 1,
     PyDoc_STR("(UInt32 target) -> None")},
    {"GetDataBrowserTarget", (PyCFunction)CtlObj_GetDataBrowserTarget, 1,
     PyDoc_STR("() -> (UInt32 target)")},
    {"SetDataBrowserSortOrder", (PyCFunction)CtlObj_SetDataBrowserSortOrder, 1,
     PyDoc_STR("(UInt16 order) -> None")},
    {"GetDataBrowserSortOrder", (PyCFunction)CtlObj_GetDataBrowserSortOrder, 1,
     PyDoc_STR("() -> (UInt16 order)")},
    {"SetDataBrowserScrollPosition", (PyCFunction)CtlObj_SetDataBrowserScrollPosition, 1,
     PyDoc_STR("(UInt32 top, UInt32 left) -> None")},
    {"GetDataBrowserScrollPosition", (PyCFunction)CtlObj_GetDataBrowserScrollPosition, 1,
     PyDoc_STR("() -> (UInt32 top, UInt32 left)")},
    {"SetDataBrowserHasScrollBars", (PyCFunction)CtlObj_SetDataBrowserHasScrollBars, 1,
     PyDoc_STR("(Boolean horiz, Boolean vert) -> None")},
    {"GetDataBrowserHasScrollBars", (PyCFunction)CtlObj_GetDataBrowserHasScrollBars, 1,
     PyDoc_STR("() -> (Boolean horiz, Boolean vert)")},
    {"SetDataBrowserSortProperty", (PyCFunction)CtlObj_SetDataBrowserSortProperty, 1,
     PyDoc_STR("(UInt32 property) -> None")},
    {"GetDataBrowserSortProperty", (PyCFunction)CtlObj_GetDataBrowserSortProperty, 1,
     PyDoc_STR("() -> (UInt32 property)")},
    {"SetDataBrowserSelectionFlags", (PyCFunction)CtlObj_SetDataBrowserSelectionFlags, 1,
     PyDoc_STR("(UInt32 selectionFlags) -> None")},
    {"GetDataBrowserSelectionFlags", (PyCFunction)CtlObj_GetDataBrowserSelectionFlags, 1,
     PyDoc_STR("() -> (UInt32 selectionFlags)")},
    {"SetDataBrowserPropertyFlags", (PyCFunction)CtlObj_SetDataBrowserPropertyFlags, 1,
     PyDoc_STR("(UInt32 property, UInt32 flags) -> None")},
    {"GetDataBrowserPropertyFlags", (PyCFunction)CtlObj_GetDataBrowserPropertyFlags, 1,
     PyDoc_STR("(UInt32 property) -> (UInt32 flags)")},
    {"SetDataBrowserEditText", (PyCFunction)CtlObj_SetDataBrowserEditText, 1,
     PyDoc_STR("(CFStringRef text) -> None")},
    {"CopyDataBrowserEditText", (PyCFunction)CtlObj_CopyDataBrowserEditText, 1,
     PyDoc_STR("() -> (CFStringRef text)")},
    {"GetDataBrowserEditText", (PyCFunction)CtlObj_GetDataBrowserEditText, 1,
     PyDoc_STR("(CFMutableStringRef text) -> None")},
    {"SetDataBrowserEditItem", (PyCFunction)CtlObj_SetDataBrowserEditItem, 1,
     PyDoc_STR("(UInt32 item, UInt32 property) -> None")},
    {"GetDataBrowserEditItem", (PyCFunction)CtlObj_GetDataBrowserEditItem, 1,
     PyDoc_STR("() -> (UInt32 item, UInt32 property)")},
    {"GetDataBrowserItemPartBounds", (PyCFunction)CtlObj_GetDataBrowserItemPartBounds, 1,
     PyDoc_STR("(UInt32 item, UInt32 property, OSType part) -> (Rect bounds)")},
    {"RemoveDataBrowserTableViewColumn", (PyCFunction)CtlObj_RemoveDataBrowserTableViewColumn, 1,
     PyDoc_STR("(UInt32 column) -> None")},
    {"GetDataBrowserTableViewColumnCount", (PyCFunction)CtlObj_GetDataBrowserTableViewColumnCount, 1,
     PyDoc_STR("() -> (UInt32 numColumns)")},
    {"SetDataBrowserTableViewHiliteStyle", (PyCFunction)CtlObj_SetDataBrowserTableViewHiliteStyle, 1,
     PyDoc_STR("(UInt32 hiliteStyle) -> None")},
    {"GetDataBrowserTableViewHiliteStyle", (PyCFunction)CtlObj_GetDataBrowserTableViewHiliteStyle, 1,
     PyDoc_STR("() -> (UInt32 hiliteStyle)")},
    {"SetDataBrowserTableViewRowHeight", (PyCFunction)CtlObj_SetDataBrowserTableViewRowHeight, 1,
     PyDoc_STR("(UInt16 height) -> None")},
    {"GetDataBrowserTableViewRowHeight", (PyCFunction)CtlObj_GetDataBrowserTableViewRowHeight, 1,
     PyDoc_STR("() -> (UInt16 height)")},
    {"SetDataBrowserTableViewColumnWidth", (PyCFunction)CtlObj_SetDataBrowserTableViewColumnWidth, 1,
     PyDoc_STR("(UInt16 width) -> None")},
    {"GetDataBrowserTableViewColumnWidth", (PyCFunction)CtlObj_GetDataBrowserTableViewColumnWidth, 1,
     PyDoc_STR("() -> (UInt16 width)")},
    {"SetDataBrowserTableViewItemRowHeight", (PyCFunction)CtlObj_SetDataBrowserTableViewItemRowHeight, 1,
     PyDoc_STR("(UInt32 item, UInt16 height) -> None")},
    {"GetDataBrowserTableViewItemRowHeight", (PyCFunction)CtlObj_GetDataBrowserTableViewItemRowHeight, 1,
     PyDoc_STR("(UInt32 item) -> (UInt16 height)")},
    {"SetDataBrowserTableViewNamedColumnWidth", (PyCFunction)CtlObj_SetDataBrowserTableViewNamedColumnWidth, 1,
     PyDoc_STR("(UInt32 column, UInt16 width) -> None")},
    {"GetDataBrowserTableViewNamedColumnWidth", (PyCFunction)CtlObj_GetDataBrowserTableViewNamedColumnWidth, 1,
     PyDoc_STR("(UInt32 column) -> (UInt16 width)")},
    {"SetDataBrowserTableViewGeometry", (PyCFunction)CtlObj_SetDataBrowserTableViewGeometry, 1,
     PyDoc_STR("(Boolean variableWidthColumns, Boolean variableHeightRows) -> None")},
    {"GetDataBrowserTableViewGeometry", (PyCFunction)CtlObj_GetDataBrowserTableViewGeometry, 1,
     PyDoc_STR("() -> (Boolean variableWidthColumns, Boolean variableHeightRows)")},
    {"GetDataBrowserTableViewItemID", (PyCFunction)CtlObj_GetDataBrowserTableViewItemID, 1,
     PyDoc_STR("(UInt32 row) -> (UInt32 item)")},
    {"SetDataBrowserTableViewItemRow", (PyCFunction)CtlObj_SetDataBrowserTableViewItemRow, 1,
     PyDoc_STR("(UInt32 item, UInt32 row) -> None")},
    {"GetDataBrowserTableViewItemRow", (PyCFunction)CtlObj_GetDataBrowserTableViewItemRow, 1,
     PyDoc_STR("(UInt32 item) -> (UInt32 row)")},
    {"SetDataBrowserTableViewColumnPosition", (PyCFunction)CtlObj_SetDataBrowserTableViewColumnPosition, 1,
     PyDoc_STR("(UInt32 column, UInt32 position) -> None")},
    {"GetDataBrowserTableViewColumnPosition", (PyCFunction)CtlObj_GetDataBrowserTableViewColumnPosition, 1,
     PyDoc_STR("(UInt32 column) -> (UInt32 position)")},
    {"GetDataBrowserTableViewColumnProperty", (PyCFunction)CtlObj_GetDataBrowserTableViewColumnProperty, 1,
     PyDoc_STR("(UInt32 column) -> (UInt32 property)")},
    {"AutoSizeDataBrowserListViewColumns", (PyCFunction)CtlObj_AutoSizeDataBrowserListViewColumns, 1,
     PyDoc_STR("() -> None")},
    {"AddDataBrowserListViewColumn", (PyCFunction)CtlObj_AddDataBrowserListViewColumn, 1,
     PyDoc_STR("(DataBrowserListViewColumnDesc columnDesc, UInt32 position) -> None")},
    {"SetDataBrowserListViewHeaderBtnHeight", (PyCFunction)CtlObj_SetDataBrowserListViewHeaderBtnHeight, 1,
     PyDoc_STR("(UInt16 height) -> None")},
    {"GetDataBrowserListViewHeaderBtnHeight", (PyCFunction)CtlObj_GetDataBrowserListViewHeaderBtnHeight, 1,
     PyDoc_STR("() -> (UInt16 height)")},
    {"SetDataBrowserListViewUsePlainBackground", (PyCFunction)CtlObj_SetDataBrowserListViewUsePlainBackground, 1,
     PyDoc_STR("(Boolean usePlainBackground) -> None")},
    {"GetDataBrowserListViewUsePlainBackground", (PyCFunction)CtlObj_GetDataBrowserListViewUsePlainBackground, 1,
     PyDoc_STR("() -> (Boolean usePlainBackground)")},
    {"SetDataBrowserListViewDisclosureColumn", (PyCFunction)CtlObj_SetDataBrowserListViewDisclosureColumn, 1,
     PyDoc_STR("(UInt32 column, Boolean expandableRows) -> None")},
    {"GetDataBrowserListViewDisclosureColumn", (PyCFunction)CtlObj_GetDataBrowserListViewDisclosureColumn, 1,
     PyDoc_STR("() -> (UInt32 column, Boolean expandableRows)")},
    {"GetDataBrowserColumnViewPath", (PyCFunction)CtlObj_GetDataBrowserColumnViewPath, 1,
     PyDoc_STR("(Handle path) -> None")},
    {"GetDataBrowserColumnViewPathLength", (PyCFunction)CtlObj_GetDataBrowserColumnViewPathLength, 1,
     PyDoc_STR("() -> (UInt32 pathLength)")},
    {"SetDataBrowserColumnViewDisplayType", (PyCFunction)CtlObj_SetDataBrowserColumnViewDisplayType, 1,
     PyDoc_STR("(OSType propertyType) -> None")},
    {"GetDataBrowserColumnViewDisplayType", (PyCFunction)CtlObj_GetDataBrowserColumnViewDisplayType, 1,
     PyDoc_STR("() -> (OSType propertyType)")},
    {"as_Resource", (PyCFunction)CtlObj_as_Resource, 1,
     PyDoc_STR("() -> (Handle _rv)")},
    {"GetControlRect", (PyCFunction)CtlObj_GetControlRect, 1,
     PyDoc_STR("() -> (Rect rect)")},
    {"DisposeControl", (PyCFunction)CtlObj_DisposeControl, 1,
     PyDoc_STR("() -> None")},
    {"TrackControl", (PyCFunction)CtlObj_TrackControl, 1,
     PyDoc_STR("(Point startPoint [,trackercallback]) -> (ControlPartCode _rv)")},
    {"HandleControlClick", (PyCFunction)CtlObj_HandleControlClick, 1,
     PyDoc_STR("(Point startPoint, Integer modifiers, [,trackercallback]) -> (ControlPartCode _rv)")},
    {"SetControlData", (PyCFunction)CtlObj_SetControlData, 1,
     PyDoc_STR("(stuff) -> None")},
    {"GetControlData", (PyCFunction)CtlObj_GetControlData, 1,
     PyDoc_STR("(part, type) -> String")},
    {"SetControlData_Handle", (PyCFunction)CtlObj_SetControlData_Handle, 1,
     PyDoc_STR("(ResObj) -> None")},
    {"GetControlData_Handle", (PyCFunction)CtlObj_GetControlData_Handle, 1,
     PyDoc_STR("(part, type) -> ResObj")},
    {"SetControlData_Callback", (PyCFunction)CtlObj_SetControlData_Callback, 1,
     PyDoc_STR("(callbackfunc) -> None")},
    {NULL, NULL, 0}
};

#define CtlObj_getsetlist NULL


static int CtlObj_compare(ControlObject *self, ControlObject *other)
{
    unsigned long v, w;

    if (!CtlObj_Check((PyObject *)other))
    {
        v=(unsigned long)self;
        w=(unsigned long)other;
    }
    else
    {
        v=(unsigned long)self->ob_itself;
        w=(unsigned long)other->ob_itself;
    }
    if( v < w ) return -1;
    if( v > w ) return 1;
    return 0;
}

#define CtlObj_repr NULL

static long CtlObj_hash(ControlObject *self)
{
    return (long)self->ob_itself;
}
#define CtlObj_tp_init 0

#define CtlObj_tp_alloc PyType_GenericAlloc

static PyObject *CtlObj_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
    PyObject *_self;
    ControlHandle itself;
    char *kw[] = {"itself", 0};

    if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, CtlObj_Convert, &itself)) return NULL;
    if ((_self = type->tp_alloc(type, 0)) == NULL) return NULL;
    ((ControlObject *)_self)->ob_itself = itself;
    return _self;
}

#define CtlObj_tp_free PyObject_Del


PyTypeObject Control_Type = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "_Ctl.Control", /*tp_name*/
    sizeof(ControlObject), /*tp_basicsize*/
    0, /*tp_itemsize*/
    /* methods */
    (destructor) CtlObj_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)0, /*tp_getattr*/
    (setattrfunc)0, /*tp_setattr*/
    (cmpfunc) CtlObj_compare, /*tp_compare*/
    (reprfunc) CtlObj_repr, /*tp_repr*/
    (PyNumberMethods *)0, /* tp_as_number */
    (PySequenceMethods *)0, /* tp_as_sequence */
    (PyMappingMethods *)0, /* tp_as_mapping */
    (hashfunc) CtlObj_hash, /*tp_hash*/
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
    CtlObj_methods, /* tp_methods */
    0, /*tp_members*/
    CtlObj_getsetlist, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    CtlObj_tp_init, /* tp_init */
    CtlObj_tp_alloc, /* tp_alloc */
    CtlObj_tp_new, /* tp_new */
    CtlObj_tp_free, /* tp_free */
};

/* -------------------- End object type Control --------------------- */


static PyObject *Ctl_NewControl(PyObject *_self, PyObject *_args)
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
#ifndef NewControl
    PyMac_PRECHECK(NewControl);
#endif
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

static PyObject *Ctl_GetNewControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    ControlHandle _rv;
    SInt16 resourceID;
    WindowPtr owningWindow;
#ifndef GetNewControl
    PyMac_PRECHECK(GetNewControl);
#endif
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

static PyObject *Ctl_DrawControls(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    WindowPtr theWindow;
#ifndef DrawControls
    PyMac_PRECHECK(DrawControls);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          WinObj_Convert, &theWindow))
        return NULL;
    DrawControls(theWindow);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Ctl_UpdateControls(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    WindowPtr inWindow;
    RgnHandle inUpdateRegion;
#ifndef UpdateControls
    PyMac_PRECHECK(UpdateControls);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&",
                          WinObj_Convert, &inWindow,
                          ResObj_Convert, &inUpdateRegion))
        return NULL;
    UpdateControls(inWindow,
                   inUpdateRegion);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Ctl_FindControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    ControlPartCode _rv;
    Point testPoint;
    WindowPtr theWindow;
    ControlHandle theControl;
#ifndef FindControl
    PyMac_PRECHECK(FindControl);
#endif
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

static PyObject *Ctl_IdleControls(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    WindowPtr inWindow;
#ifndef IdleControls
    PyMac_PRECHECK(IdleControls);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          WinObj_Convert, &inWindow))
        return NULL;
    IdleControls(inWindow);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Ctl_GetControlByID(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr inWindow;
    ControlID inID;
    ControlHandle outControl;
#ifndef GetControlByID
    PyMac_PRECHECK(GetControlByID);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&",
                          WinObj_Convert, &inWindow,
                          PyControlID_Convert, &inID))
        return NULL;
    _err = GetControlByID(inWindow,
                          &inID,
                          &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_WhichControl, outControl);
    return _res;
}

static PyObject *Ctl_DumpControlHierarchy(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    WindowPtr inWindow;
    FSSpec inDumpFile;
#ifndef DumpControlHierarchy
    PyMac_PRECHECK(DumpControlHierarchy);
#endif
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

static PyObject *Ctl_CreateRootControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    WindowPtr inWindow;
    ControlHandle outControl;
#ifndef CreateRootControl
    PyMac_PRECHECK(CreateRootControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          WinObj_Convert, &inWindow))
        return NULL;
    _err = CreateRootControl(inWindow,
                             &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_GetRootControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    WindowPtr inWindow;
    ControlHandle outControl;
#ifndef GetRootControl
    PyMac_PRECHECK(GetRootControl);
#endif
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

static PyObject *Ctl_GetKeyboardFocus(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    WindowPtr inWindow;
    ControlHandle outControl;
#ifndef GetKeyboardFocus
    PyMac_PRECHECK(GetKeyboardFocus);
#endif
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

static PyObject *Ctl_SetKeyboardFocus(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    WindowPtr inWindow;
    ControlHandle inControl;
    ControlFocusPart inPart;
#ifndef SetKeyboardFocus
    PyMac_PRECHECK(SetKeyboardFocus);
#endif
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

static PyObject *Ctl_AdvanceKeyboardFocus(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    WindowPtr inWindow;
#ifndef AdvanceKeyboardFocus
    PyMac_PRECHECK(AdvanceKeyboardFocus);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          WinObj_Convert, &inWindow))
        return NULL;
    _err = AdvanceKeyboardFocus(inWindow);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Ctl_ReverseKeyboardFocus(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    WindowPtr inWindow;
#ifndef ReverseKeyboardFocus
    PyMac_PRECHECK(ReverseKeyboardFocus);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          WinObj_Convert, &inWindow))
        return NULL;
    _err = ReverseKeyboardFocus(inWindow);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Ctl_ClearKeyboardFocus(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    WindowPtr inWindow;
#ifndef ClearKeyboardFocus
    PyMac_PRECHECK(ClearKeyboardFocus);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          WinObj_Convert, &inWindow))
        return NULL;
    _err = ClearKeyboardFocus(inWindow);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Ctl_SetAutomaticControlDragTrackingEnabledForWindow(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr inWindow;
    Boolean inTracks;
#ifndef SetAutomaticControlDragTrackingEnabledForWindow
    PyMac_PRECHECK(SetAutomaticControlDragTrackingEnabledForWindow);
#endif
    if (!PyArg_ParseTuple(_args, "O&b",
                          WinObj_Convert, &inWindow,
                          &inTracks))
        return NULL;
    _err = SetAutomaticControlDragTrackingEnabledForWindow(inWindow,
                                                           inTracks);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Ctl_IsAutomaticControlDragTrackingEnabledForWindow(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr inWindow;
    Boolean outTracks;
#ifndef IsAutomaticControlDragTrackingEnabledForWindow
    PyMac_PRECHECK(IsAutomaticControlDragTrackingEnabledForWindow);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          WinObj_Convert, &inWindow))
        return NULL;
    _err = IsAutomaticControlDragTrackingEnabledForWindow(inWindow,
                                                          &outTracks);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("b",
                         outTracks);
    return _res;
}

static PyObject *Ctl_CreateBevelButtonControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    CFStringRef title;
    UInt16 thickness;
    UInt16 behavior;
    ControlButtonContentInfo info;
    SInt16 menuID;
    UInt16 menuBehavior;
    UInt16 menuPlacement;
    ControlHandle outControl;
#ifndef CreateBevelButtonControl
    PyMac_PRECHECK(CreateBevelButtonControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&O&HHO&hHH",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect,
                          CFStringRefObj_Convert, &title,
                          &thickness,
                          &behavior,
                          ControlButtonContentInfo_Convert, &info,
                          &menuID,
                          &menuBehavior,
                          &menuPlacement))
        return NULL;
    _err = CreateBevelButtonControl(window,
                                    &boundsRect,
                                    title,
                                    thickness,
                                    behavior,
                                    &info,
                                    menuID,
                                    menuBehavior,
                                    menuPlacement,
                                    &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreateSliderControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    SInt32 value;
    SInt32 minimum;
    SInt32 maximum;
    UInt16 orientation;
    UInt16 numTickMarks;
    Boolean liveTracking;
    PyObject* liveTrackingProc;
    UniversalProcPtr c_callback;
    ControlHandle outControl;
#ifndef CreateSliderControl
    PyMac_PRECHECK(CreateSliderControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&lllHHbO",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect,
                          &value,
                          &minimum,
                          &maximum,
                          &orientation,
                          &numTickMarks,
                          &liveTracking,
                          &liveTrackingProc))
        return NULL;
    _err = CreateSliderControl(window,
                               &boundsRect,
                               value,
                               minimum,
                               maximum,
                               orientation,
                               numTickMarks,
                               liveTracking,
                               myactionproc_upp,
                               &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    setcallback(_res, kMyControlActionProcTag, liveTrackingProc, &c_callback);
    return _res;
}

static PyObject *Ctl_CreateDisclosureTriangleControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr inWindow;
    Rect inBoundsRect;
    UInt16 inOrientation;
    CFStringRef inTitle;
    SInt32 inInitialValue;
    Boolean inDrawTitle;
    Boolean inAutoToggles;
    ControlHandle outControl;
#ifndef CreateDisclosureTriangleControl
    PyMac_PRECHECK(CreateDisclosureTriangleControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&HO&lbb",
                          WinObj_Convert, &inWindow,
                          PyMac_GetRect, &inBoundsRect,
                          &inOrientation,
                          CFStringRefObj_Convert, &inTitle,
                          &inInitialValue,
                          &inDrawTitle,
                          &inAutoToggles))
        return NULL;
    _err = CreateDisclosureTriangleControl(inWindow,
                                           &inBoundsRect,
                                           inOrientation,
                                           inTitle,
                                           inInitialValue,
                                           inDrawTitle,
                                           inAutoToggles,
                                           &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreateProgressBarControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    SInt32 value;
    SInt32 minimum;
    SInt32 maximum;
    Boolean indeterminate;
    ControlHandle outControl;
#ifndef CreateProgressBarControl
    PyMac_PRECHECK(CreateProgressBarControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&lllb",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect,
                          &value,
                          &minimum,
                          &maximum,
                          &indeterminate))
        return NULL;
    _err = CreateProgressBarControl(window,
                                    &boundsRect,
                                    value,
                                    minimum,
                                    maximum,
                                    indeterminate,
                                    &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreateRelevanceBarControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    SInt32 value;
    SInt32 minimum;
    SInt32 maximum;
    ControlHandle outControl;
#ifndef CreateRelevanceBarControl
    PyMac_PRECHECK(CreateRelevanceBarControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&lll",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect,
                          &value,
                          &minimum,
                          &maximum))
        return NULL;
    _err = CreateRelevanceBarControl(window,
                                     &boundsRect,
                                     value,
                                     minimum,
                                     maximum,
                                     &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreateLittleArrowsControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    SInt32 value;
    SInt32 minimum;
    SInt32 maximum;
    SInt32 increment;
    ControlHandle outControl;
#ifndef CreateLittleArrowsControl
    PyMac_PRECHECK(CreateLittleArrowsControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&llll",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect,
                          &value,
                          &minimum,
                          &maximum,
                          &increment))
        return NULL;
    _err = CreateLittleArrowsControl(window,
                                     &boundsRect,
                                     value,
                                     minimum,
                                     maximum,
                                     increment,
                                     &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreateChasingArrowsControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    ControlHandle outControl;
#ifndef CreateChasingArrowsControl
    PyMac_PRECHECK(CreateChasingArrowsControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect))
        return NULL;
    _err = CreateChasingArrowsControl(window,
                                      &boundsRect,
                                      &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreateSeparatorControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    ControlHandle outControl;
#ifndef CreateSeparatorControl
    PyMac_PRECHECK(CreateSeparatorControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect))
        return NULL;
    _err = CreateSeparatorControl(window,
                                  &boundsRect,
                                  &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreateGroupBoxControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    CFStringRef title;
    Boolean primary;
    ControlHandle outControl;
#ifndef CreateGroupBoxControl
    PyMac_PRECHECK(CreateGroupBoxControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&O&b",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect,
                          CFStringRefObj_Convert, &title,
                          &primary))
        return NULL;
    _err = CreateGroupBoxControl(window,
                                 &boundsRect,
                                 title,
                                 primary,
                                 &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreateCheckGroupBoxControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    CFStringRef title;
    SInt32 initialValue;
    Boolean primary;
    Boolean autoToggle;
    ControlHandle outControl;
#ifndef CreateCheckGroupBoxControl
    PyMac_PRECHECK(CreateCheckGroupBoxControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&O&lbb",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect,
                          CFStringRefObj_Convert, &title,
                          &initialValue,
                          &primary,
                          &autoToggle))
        return NULL;
    _err = CreateCheckGroupBoxControl(window,
                                      &boundsRect,
                                      title,
                                      initialValue,
                                      primary,
                                      autoToggle,
                                      &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreatePopupGroupBoxControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    CFStringRef title;
    Boolean primary;
    SInt16 menuID;
    Boolean variableWidth;
    SInt16 titleWidth;
    SInt16 titleJustification;
    Style titleStyle;
    ControlHandle outControl;
#ifndef CreatePopupGroupBoxControl
    PyMac_PRECHECK(CreatePopupGroupBoxControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&O&bhbhhb",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect,
                          CFStringRefObj_Convert, &title,
                          &primary,
                          &menuID,
                          &variableWidth,
                          &titleWidth,
                          &titleJustification,
                          &titleStyle))
        return NULL;
    _err = CreatePopupGroupBoxControl(window,
                                      &boundsRect,
                                      title,
                                      primary,
                                      menuID,
                                      variableWidth,
                                      titleWidth,
                                      titleJustification,
                                      titleStyle,
                                      &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreateImageWellControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    ControlButtonContentInfo info;
    ControlHandle outControl;
#ifndef CreateImageWellControl
    PyMac_PRECHECK(CreateImageWellControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&O&",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect,
                          ControlButtonContentInfo_Convert, &info))
        return NULL;
    _err = CreateImageWellControl(window,
                                  &boundsRect,
                                  &info,
                                  &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreatePopupArrowControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    UInt16 orientation;
    UInt16 size;
    ControlHandle outControl;
#ifndef CreatePopupArrowControl
    PyMac_PRECHECK(CreatePopupArrowControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&HH",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect,
                          &orientation,
                          &size))
        return NULL;
    _err = CreatePopupArrowControl(window,
                                   &boundsRect,
                                   orientation,
                                   size,
                                   &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreatePlacardControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    ControlHandle outControl;
#ifndef CreatePlacardControl
    PyMac_PRECHECK(CreatePlacardControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect))
        return NULL;
    _err = CreatePlacardControl(window,
                                &boundsRect,
                                &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreateClockControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    UInt16 clockType;
    UInt32 clockFlags;
    ControlHandle outControl;
#ifndef CreateClockControl
    PyMac_PRECHECK(CreateClockControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&Hl",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect,
                          &clockType,
                          &clockFlags))
        return NULL;
    _err = CreateClockControl(window,
                              &boundsRect,
                              clockType,
                              clockFlags,
                              &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreateUserPaneControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    UInt32 features;
    ControlHandle outControl;
#ifndef CreateUserPaneControl
    PyMac_PRECHECK(CreateUserPaneControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&l",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect,
                          &features))
        return NULL;
    _err = CreateUserPaneControl(window,
                                 &boundsRect,
                                 features,
                                 &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreateEditTextControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    CFStringRef text;
    Boolean isPassword;
    Boolean useInlineInput;
    ControlFontStyleRec style;
    ControlHandle outControl;
#ifndef CreateEditTextControl
    PyMac_PRECHECK(CreateEditTextControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&O&bbO&",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect,
                          CFStringRefObj_Convert, &text,
                          &isPassword,
                          &useInlineInput,
                          ControlFontStyle_Convert, &style))
        return NULL;
    _err = CreateEditTextControl(window,
                                 &boundsRect,
                                 text,
                                 isPassword,
                                 useInlineInput,
                                 &style,
                                 &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreateStaticTextControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    CFStringRef text;
    ControlFontStyleRec style;
    ControlHandle outControl;
#ifndef CreateStaticTextControl
    PyMac_PRECHECK(CreateStaticTextControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&O&O&",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect,
                          CFStringRefObj_Convert, &text,
                          ControlFontStyle_Convert, &style))
        return NULL;
    _err = CreateStaticTextControl(window,
                                   &boundsRect,
                                   text,
                                   &style,
                                   &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreatePictureControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    ControlButtonContentInfo content;
    Boolean dontTrack;
    ControlHandle outControl;
#ifndef CreatePictureControl
    PyMac_PRECHECK(CreatePictureControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&O&b",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect,
                          ControlButtonContentInfo_Convert, &content,
                          &dontTrack))
        return NULL;
    _err = CreatePictureControl(window,
                                &boundsRect,
                                &content,
                                dontTrack,
                                &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreateIconControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr inWindow;
    Rect inBoundsRect;
    ControlButtonContentInfo inIconContent;
    Boolean inDontTrack;
    ControlHandle outControl;
#ifndef CreateIconControl
    PyMac_PRECHECK(CreateIconControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&O&b",
                          WinObj_Convert, &inWindow,
                          PyMac_GetRect, &inBoundsRect,
                          ControlButtonContentInfo_Convert, &inIconContent,
                          &inDontTrack))
        return NULL;
    _err = CreateIconControl(inWindow,
                             &inBoundsRect,
                             &inIconContent,
                             inDontTrack,
                             &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreateWindowHeaderControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    Boolean isListHeader;
    ControlHandle outControl;
#ifndef CreateWindowHeaderControl
    PyMac_PRECHECK(CreateWindowHeaderControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&b",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect,
                          &isListHeader))
        return NULL;
    _err = CreateWindowHeaderControl(window,
                                     &boundsRect,
                                     isListHeader,
                                     &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreatePushButtonControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    CFStringRef title;
    ControlHandle outControl;
#ifndef CreatePushButtonControl
    PyMac_PRECHECK(CreatePushButtonControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&O&",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect,
                          CFStringRefObj_Convert, &title))
        return NULL;
    _err = CreatePushButtonControl(window,
                                   &boundsRect,
                                   title,
                                   &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreatePushButtonWithIconControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    CFStringRef title;
    ControlButtonContentInfo icon;
    UInt16 iconAlignment;
    ControlHandle outControl;
#ifndef CreatePushButtonWithIconControl
    PyMac_PRECHECK(CreatePushButtonWithIconControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&O&O&H",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect,
                          CFStringRefObj_Convert, &title,
                          ControlButtonContentInfo_Convert, &icon,
                          &iconAlignment))
        return NULL;
    _err = CreatePushButtonWithIconControl(window,
                                           &boundsRect,
                                           title,
                                           &icon,
                                           iconAlignment,
                                           &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreateRadioButtonControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    CFStringRef title;
    SInt32 initialValue;
    Boolean autoToggle;
    ControlHandle outControl;
#ifndef CreateRadioButtonControl
    PyMac_PRECHECK(CreateRadioButtonControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&O&lb",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect,
                          CFStringRefObj_Convert, &title,
                          &initialValue,
                          &autoToggle))
        return NULL;
    _err = CreateRadioButtonControl(window,
                                    &boundsRect,
                                    title,
                                    initialValue,
                                    autoToggle,
                                    &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreateCheckBoxControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    CFStringRef title;
    SInt32 initialValue;
    Boolean autoToggle;
    ControlHandle outControl;
#ifndef CreateCheckBoxControl
    PyMac_PRECHECK(CreateCheckBoxControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&O&lb",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect,
                          CFStringRefObj_Convert, &title,
                          &initialValue,
                          &autoToggle))
        return NULL;
    _err = CreateCheckBoxControl(window,
                                 &boundsRect,
                                 title,
                                 initialValue,
                                 autoToggle,
                                 &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreateScrollBarControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    SInt32 value;
    SInt32 minimum;
    SInt32 maximum;
    SInt32 viewSize;
    Boolean liveTracking;
    PyObject* liveTrackingProc;
    UniversalProcPtr c_callback;
    ControlHandle outControl;
#ifndef CreateScrollBarControl
    PyMac_PRECHECK(CreateScrollBarControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&llllbO",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect,
                          &value,
                          &minimum,
                          &maximum,
                          &viewSize,
                          &liveTracking,
                          &liveTrackingProc))
        return NULL;
    _err = CreateScrollBarControl(window,
                                  &boundsRect,
                                  value,
                                  minimum,
                                  maximum,
                                  viewSize,
                                  liveTracking,
                                  myactionproc_upp,
                                  &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    setcallback(_res, kMyControlActionProcTag, liveTrackingProc, &c_callback);
    return _res;
}

static PyObject *Ctl_CreatePopupButtonControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    CFStringRef title;
    SInt16 menuID;
    Boolean variableWidth;
    SInt16 titleWidth;
    SInt16 titleJustification;
    Style titleStyle;
    ControlHandle outControl;
#ifndef CreatePopupButtonControl
    PyMac_PRECHECK(CreatePopupButtonControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&O&hbhhb",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect,
                          CFStringRefObj_Convert, &title,
                          &menuID,
                          &variableWidth,
                          &titleWidth,
                          &titleJustification,
                          &titleStyle))
        return NULL;
    _err = CreatePopupButtonControl(window,
                                    &boundsRect,
                                    title,
                                    menuID,
                                    variableWidth,
                                    titleWidth,
                                    titleJustification,
                                    titleStyle,
                                    &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreateRadioGroupControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    ControlHandle outControl;
#ifndef CreateRadioGroupControl
    PyMac_PRECHECK(CreateRadioGroupControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect))
        return NULL;
    _err = CreateRadioGroupControl(window,
                                   &boundsRect,
                                   &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreateScrollingTextBoxControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    SInt16 contentResID;
    Boolean autoScroll;
    UInt32 delayBeforeAutoScroll;
    UInt32 delayBetweenAutoScroll;
    UInt16 autoScrollAmount;
    ControlHandle outControl;
#ifndef CreateScrollingTextBoxControl
    PyMac_PRECHECK(CreateScrollingTextBoxControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&hbllH",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect,
                          &contentResID,
                          &autoScroll,
                          &delayBeforeAutoScroll,
                          &delayBetweenAutoScroll,
                          &autoScrollAmount))
        return NULL;
    _err = CreateScrollingTextBoxControl(window,
                                         &boundsRect,
                                         contentResID,
                                         autoScroll,
                                         delayBeforeAutoScroll,
                                         delayBetweenAutoScroll,
                                         autoScrollAmount,
                                         &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreateDisclosureButtonControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr inWindow;
    Rect inBoundsRect;
    SInt32 inValue;
    Boolean inAutoToggles;
    ControlHandle outControl;
#ifndef CreateDisclosureButtonControl
    PyMac_PRECHECK(CreateDisclosureButtonControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&lb",
                          WinObj_Convert, &inWindow,
                          PyMac_GetRect, &inBoundsRect,
                          &inValue,
                          &inAutoToggles))
        return NULL;
    _err = CreateDisclosureButtonControl(inWindow,
                                         &inBoundsRect,
                                         inValue,
                                         inAutoToggles,
                                         &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreateRoundButtonControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr inWindow;
    Rect inBoundsRect;
    SInt16 inSize;
    ControlButtonContentInfo inContent;
    ControlHandle outControl;
#ifndef CreateRoundButtonControl
    PyMac_PRECHECK(CreateRoundButtonControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&hO&",
                          WinObj_Convert, &inWindow,
                          PyMac_GetRect, &inBoundsRect,
                          &inSize,
                          ControlButtonContentInfo_Convert, &inContent))
        return NULL;
    _err = CreateRoundButtonControl(inWindow,
                                    &inBoundsRect,
                                    inSize,
                                    &inContent,
                                    &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreateDataBrowserControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    OSType style;
    ControlHandle outControl;
#ifndef CreateDataBrowserControl
    PyMac_PRECHECK(CreateDataBrowserControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&O&",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect,
                          PyMac_GetOSType, &style))
        return NULL;
    _err = CreateDataBrowserControl(window,
                                    &boundsRect,
                                    style,
                                    &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_CreateEditUnicodeTextControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    CFStringRef text;
    Boolean isPassword;
    ControlFontStyleRec style;
    ControlHandle outControl;
#ifndef CreateEditUnicodeTextControl
    PyMac_PRECHECK(CreateEditUnicodeTextControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&O&bO&",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect,
                          CFStringRefObj_Convert, &text,
                          &isPassword,
                          ControlFontStyle_Convert, &style))
        return NULL;
    _err = CreateEditUnicodeTextControl(window,
                                        &boundsRect,
                                        text,
                                        isPassword,
                                        &style,
                                        &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *Ctl_FindControlUnderMouse(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    ControlHandle _rv;
    Point inWhere;
    WindowPtr inWindow;
    SInt16 outPart;
#ifndef FindControlUnderMouse
    PyMac_PRECHECK(FindControlUnderMouse);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&",
                          PyMac_GetPoint, &inWhere,
                          WinObj_Convert, &inWindow))
        return NULL;
    _rv = FindControlUnderMouse(inWhere,
                                inWindow,
                                &outPart);
    _res = Py_BuildValue("O&h",
                         CtlObj_WhichControl, _rv,
                         outPart);
    return _res;
}

static PyObject *Ctl_as_Control(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    ControlHandle _rv;
    Handle h;
#ifndef as_Control
    PyMac_PRECHECK(as_Control);
#endif
    if (!PyArg_ParseTuple(_args, "O&",
                          ResObj_Convert, &h))
        return NULL;
    _rv = as_Control(h);
    _res = Py_BuildValue("O&",
                         CtlObj_New, _rv);
    return _res;
}

static PyObject *Ctl_CreateTabsControl(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr window;
    Rect boundsRect;
    UInt16 size;
    UInt16 direction;
    int i;
    UInt16 numTabs;
    ControlTabEntry tabArray[MAXTABS];
    ControlHandle outControl;
    PyObject *tabArrayObj, *tabEntry;

#ifndef CreateTabsControl
    PyMac_PRECHECK(CreateTabsControl);
#endif
    if (!PyArg_ParseTuple(_args, "O&O&HHO",
                          WinObj_Convert, &window,
                          PyMac_GetRect, &boundsRect,
                          &size,
                          &direction,
                          &tabArrayObj))
        return NULL;

    i = PySequence_Length(tabArrayObj);
    if (i == -1)
        return NULL;
    if (i > MAXTABS) {
        PyErr_SetString(Ctl_Error, "Too many tabs");
        return NULL;
    }
    numTabs = i;
    for (i=0; i<numTabs; i++) {
        tabEntry = PySequence_GetItem(tabArrayObj, i);
        if (tabEntry == NULL)
            return NULL;
        if (!PyArg_Parse(tabEntry, "(O&O&B)",
                         ControlButtonContentInfo_Convert, &tabArray[i].icon,
                         CFStringRefObj_Convert, &tabArray[i].name,
                         &tabArray[i].enabled
                         ))
            return NULL;
    }

    _err = CreateTabsControl(window,
                             &boundsRect,
                             size,
                             direction,
                             numTabs,
                             tabArray,
                             &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyMethodDef Ctl_methods[] = {
    {"NewControl", (PyCFunction)Ctl_NewControl, 1,
     PyDoc_STR("(WindowPtr owningWindow, Rect boundsRect, Str255 controlTitle, Boolean initiallyVisible, SInt16 initialValue, SInt16 minimumValue, SInt16 maximumValue, SInt16 procID, SInt32 controlReference) -> (ControlHandle _rv)")},
    {"GetNewControl", (PyCFunction)Ctl_GetNewControl, 1,
     PyDoc_STR("(SInt16 resourceID, WindowPtr owningWindow) -> (ControlHandle _rv)")},
    {"DrawControls", (PyCFunction)Ctl_DrawControls, 1,
     PyDoc_STR("(WindowPtr theWindow) -> None")},
    {"UpdateControls", (PyCFunction)Ctl_UpdateControls, 1,
     PyDoc_STR("(WindowPtr inWindow, RgnHandle inUpdateRegion) -> None")},
    {"FindControl", (PyCFunction)Ctl_FindControl, 1,
     PyDoc_STR("(Point testPoint, WindowPtr theWindow) -> (ControlPartCode _rv, ControlHandle theControl)")},
    {"IdleControls", (PyCFunction)Ctl_IdleControls, 1,
     PyDoc_STR("(WindowPtr inWindow) -> None")},
    {"GetControlByID", (PyCFunction)Ctl_GetControlByID, 1,
     PyDoc_STR("(WindowPtr inWindow, ControlID inID) -> (ControlHandle outControl)")},
    {"DumpControlHierarchy", (PyCFunction)Ctl_DumpControlHierarchy, 1,
     PyDoc_STR("(WindowPtr inWindow, FSSpec inDumpFile) -> None")},
    {"CreateRootControl", (PyCFunction)Ctl_CreateRootControl, 1,
     PyDoc_STR("(WindowPtr inWindow) -> (ControlHandle outControl)")},
    {"GetRootControl", (PyCFunction)Ctl_GetRootControl, 1,
     PyDoc_STR("(WindowPtr inWindow) -> (ControlHandle outControl)")},
    {"GetKeyboardFocus", (PyCFunction)Ctl_GetKeyboardFocus, 1,
     PyDoc_STR("(WindowPtr inWindow) -> (ControlHandle outControl)")},
    {"SetKeyboardFocus", (PyCFunction)Ctl_SetKeyboardFocus, 1,
     PyDoc_STR("(WindowPtr inWindow, ControlHandle inControl, ControlFocusPart inPart) -> None")},
    {"AdvanceKeyboardFocus", (PyCFunction)Ctl_AdvanceKeyboardFocus, 1,
     PyDoc_STR("(WindowPtr inWindow) -> None")},
    {"ReverseKeyboardFocus", (PyCFunction)Ctl_ReverseKeyboardFocus, 1,
     PyDoc_STR("(WindowPtr inWindow) -> None")},
    {"ClearKeyboardFocus", (PyCFunction)Ctl_ClearKeyboardFocus, 1,
     PyDoc_STR("(WindowPtr inWindow) -> None")},
    {"SetAutomaticControlDragTrackingEnabledForWindow", (PyCFunction)Ctl_SetAutomaticControlDragTrackingEnabledForWindow, 1,
     PyDoc_STR("(WindowPtr inWindow, Boolean inTracks) -> None")},
    {"IsAutomaticControlDragTrackingEnabledForWindow", (PyCFunction)Ctl_IsAutomaticControlDragTrackingEnabledForWindow, 1,
     PyDoc_STR("(WindowPtr inWindow) -> (Boolean outTracks)")},
    {"CreateBevelButtonControl", (PyCFunction)Ctl_CreateBevelButtonControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect, CFStringRef title, UInt16 thickness, UInt16 behavior, ControlButtonContentInfo info, SInt16 menuID, UInt16 menuBehavior, UInt16 menuPlacement) -> (ControlHandle outControl)")},
    {"CreateSliderControl", (PyCFunction)Ctl_CreateSliderControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect, SInt32 value, SInt32 minimum, SInt32 maximum, UInt16 orientation, UInt16 numTickMarks, Boolean liveTracking, PyObject* liveTrackingProc) -> (ControlHandle outControl)")},
    {"CreateDisclosureTriangleControl", (PyCFunction)Ctl_CreateDisclosureTriangleControl, 1,
     PyDoc_STR("(WindowPtr inWindow, Rect inBoundsRect, UInt16 inOrientation, CFStringRef inTitle, SInt32 inInitialValue, Boolean inDrawTitle, Boolean inAutoToggles) -> (ControlHandle outControl)")},
    {"CreateProgressBarControl", (PyCFunction)Ctl_CreateProgressBarControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect, SInt32 value, SInt32 minimum, SInt32 maximum, Boolean indeterminate) -> (ControlHandle outControl)")},
    {"CreateRelevanceBarControl", (PyCFunction)Ctl_CreateRelevanceBarControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect, SInt32 value, SInt32 minimum, SInt32 maximum) -> (ControlHandle outControl)")},
    {"CreateLittleArrowsControl", (PyCFunction)Ctl_CreateLittleArrowsControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect, SInt32 value, SInt32 minimum, SInt32 maximum, SInt32 increment) -> (ControlHandle outControl)")},
    {"CreateChasingArrowsControl", (PyCFunction)Ctl_CreateChasingArrowsControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect) -> (ControlHandle outControl)")},
    {"CreateSeparatorControl", (PyCFunction)Ctl_CreateSeparatorControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect) -> (ControlHandle outControl)")},
    {"CreateGroupBoxControl", (PyCFunction)Ctl_CreateGroupBoxControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect, CFStringRef title, Boolean primary) -> (ControlHandle outControl)")},
    {"CreateCheckGroupBoxControl", (PyCFunction)Ctl_CreateCheckGroupBoxControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect, CFStringRef title, SInt32 initialValue, Boolean primary, Boolean autoToggle) -> (ControlHandle outControl)")},
    {"CreatePopupGroupBoxControl", (PyCFunction)Ctl_CreatePopupGroupBoxControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect, CFStringRef title, Boolean primary, SInt16 menuID, Boolean variableWidth, SInt16 titleWidth, SInt16 titleJustification, Style titleStyle) -> (ControlHandle outControl)")},
    {"CreateImageWellControl", (PyCFunction)Ctl_CreateImageWellControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect, ControlButtonContentInfo info) -> (ControlHandle outControl)")},
    {"CreatePopupArrowControl", (PyCFunction)Ctl_CreatePopupArrowControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect, UInt16 orientation, UInt16 size) -> (ControlHandle outControl)")},
    {"CreatePlacardControl", (PyCFunction)Ctl_CreatePlacardControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect) -> (ControlHandle outControl)")},
    {"CreateClockControl", (PyCFunction)Ctl_CreateClockControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect, UInt16 clockType, UInt32 clockFlags) -> (ControlHandle outControl)")},
    {"CreateUserPaneControl", (PyCFunction)Ctl_CreateUserPaneControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect, UInt32 features) -> (ControlHandle outControl)")},
    {"CreateEditTextControl", (PyCFunction)Ctl_CreateEditTextControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect, CFStringRef text, Boolean isPassword, Boolean useInlineInput, ControlFontStyleRec style) -> (ControlHandle outControl)")},
    {"CreateStaticTextControl", (PyCFunction)Ctl_CreateStaticTextControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect, CFStringRef text, ControlFontStyleRec style) -> (ControlHandle outControl)")},
    {"CreatePictureControl", (PyCFunction)Ctl_CreatePictureControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect, ControlButtonContentInfo content, Boolean dontTrack) -> (ControlHandle outControl)")},
    {"CreateIconControl", (PyCFunction)Ctl_CreateIconControl, 1,
     PyDoc_STR("(WindowPtr inWindow, Rect inBoundsRect, ControlButtonContentInfo inIconContent, Boolean inDontTrack) -> (ControlHandle outControl)")},
    {"CreateWindowHeaderControl", (PyCFunction)Ctl_CreateWindowHeaderControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect, Boolean isListHeader) -> (ControlHandle outControl)")},
    {"CreatePushButtonControl", (PyCFunction)Ctl_CreatePushButtonControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect, CFStringRef title) -> (ControlHandle outControl)")},
    {"CreatePushButtonWithIconControl", (PyCFunction)Ctl_CreatePushButtonWithIconControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect, CFStringRef title, ControlButtonContentInfo icon, UInt16 iconAlignment) -> (ControlHandle outControl)")},
    {"CreateRadioButtonControl", (PyCFunction)Ctl_CreateRadioButtonControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect, CFStringRef title, SInt32 initialValue, Boolean autoToggle) -> (ControlHandle outControl)")},
    {"CreateCheckBoxControl", (PyCFunction)Ctl_CreateCheckBoxControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect, CFStringRef title, SInt32 initialValue, Boolean autoToggle) -> (ControlHandle outControl)")},
    {"CreateScrollBarControl", (PyCFunction)Ctl_CreateScrollBarControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect, SInt32 value, SInt32 minimum, SInt32 maximum, SInt32 viewSize, Boolean liveTracking, PyObject* liveTrackingProc) -> (ControlHandle outControl)")},
    {"CreatePopupButtonControl", (PyCFunction)Ctl_CreatePopupButtonControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect, CFStringRef title, SInt16 menuID, Boolean variableWidth, SInt16 titleWidth, SInt16 titleJustification, Style titleStyle) -> (ControlHandle outControl)")},
    {"CreateRadioGroupControl", (PyCFunction)Ctl_CreateRadioGroupControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect) -> (ControlHandle outControl)")},
    {"CreateScrollingTextBoxControl", (PyCFunction)Ctl_CreateScrollingTextBoxControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect, SInt16 contentResID, Boolean autoScroll, UInt32 delayBeforeAutoScroll, UInt32 delayBetweenAutoScroll, UInt16 autoScrollAmount) -> (ControlHandle outControl)")},
    {"CreateDisclosureButtonControl", (PyCFunction)Ctl_CreateDisclosureButtonControl, 1,
     PyDoc_STR("(WindowPtr inWindow, Rect inBoundsRect, SInt32 inValue, Boolean inAutoToggles) -> (ControlHandle outControl)")},
    {"CreateRoundButtonControl", (PyCFunction)Ctl_CreateRoundButtonControl, 1,
     PyDoc_STR("(WindowPtr inWindow, Rect inBoundsRect, SInt16 inSize, ControlButtonContentInfo inContent) -> (ControlHandle outControl)")},
    {"CreateDataBrowserControl", (PyCFunction)Ctl_CreateDataBrowserControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect, OSType style) -> (ControlHandle outControl)")},
    {"CreateEditUnicodeTextControl", (PyCFunction)Ctl_CreateEditUnicodeTextControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect, CFStringRef text, Boolean isPassword, ControlFontStyleRec style) -> (ControlHandle outControl)")},
    {"FindControlUnderMouse", (PyCFunction)Ctl_FindControlUnderMouse, 1,
     PyDoc_STR("(Point inWhere, WindowPtr inWindow) -> (ControlHandle _rv, SInt16 outPart)")},
    {"as_Control", (PyCFunction)Ctl_as_Control, 1,
     PyDoc_STR("(Handle h) -> (ControlHandle _rv)")},
    {"CreateTabsControl", (PyCFunction)Ctl_CreateTabsControl, 1,
     PyDoc_STR("(WindowPtr window, Rect boundsRect, UInt16 size, UInt16 direction, ControlTabEntry tabArray) -> (ControlHandle outControl)")},
    {NULL, NULL, 0}
};



static PyObject *
CtlObj_NewUnmanaged(ControlHandle itself)
{
    ControlObject *it;
    if (itself == NULL) return PyMac_Error(resNotFound);
    it = PyObject_NEW(ControlObject, &Control_Type);
    if (it == NULL) return NULL;
    it->ob_itself = itself;
    it->ob_callbackdict = NULL;
    return (PyObject *)it;
}

static PyObject *
CtlObj_WhichControl(ControlHandle c)
{
    PyObject *it;

    if (c == NULL)
        it = Py_None;
    else {
        it = (PyObject *) GetControlReference(c);
        /*
        ** If the refcon is zero or doesn't point back to the Python object
        ** the control is not ours. Return a temporary object.
        */
        if (it == NULL || ((ControlObject *)it)->ob_itself != c)
            return CtlObj_NewUnmanaged(c);
    }
    Py_INCREF(it);
    return it;
}

static int
settrackfunc(PyObject *obj)
{
    if (tracker) {
        PyErr_SetString(Ctl_Error, "Tracker function in use");
        return 0;
    }
    tracker = obj;
    Py_INCREF(tracker);
    return 1;
}

static void
clrtrackfunc(void)
{
    Py_XDECREF(tracker);
    tracker = 0;
}

static pascal void
mytracker(ControlHandle ctl, short part)
{
    PyObject *args, *rv=0;

    args = Py_BuildValue("(O&i)", CtlObj_WhichControl, ctl, (int)part);
    if (args && tracker) {
        rv = PyEval_CallObject(tracker, args);
        Py_DECREF(args);
    }
    if (rv)
        Py_DECREF(rv);
    else {
        PySys_WriteStderr("TrackControl or HandleControlClick: exception in tracker function\n");
        PyErr_Print();
    }
}

static int
setcallback(PyObject *myself, OSType which, PyObject *callback, UniversalProcPtr *uppp)
{
    ControlObject *self = (ControlObject *)myself;
    char keybuf[9];

    if ( which == kMyControlActionProcTag )
        *uppp = (UniversalProcPtr)myactionproc_upp;
    else if ( which == kControlUserPaneKeyDownProcTag )
        *uppp = (UniversalProcPtr)mykeydownproc_upp;
    else if ( which == kControlUserPaneFocusProcTag )
        *uppp = (UniversalProcPtr)myfocusproc_upp;
    else if ( which == kControlUserPaneDrawProcTag )
        *uppp = (UniversalProcPtr)mydrawproc_upp;
    else if ( which == kControlUserPaneIdleProcTag )
        *uppp = (UniversalProcPtr)myidleproc_upp;
    else if ( which == kControlUserPaneHitTestProcTag )
        *uppp = (UniversalProcPtr)myhittestproc_upp;
    else if ( which == kControlUserPaneTrackingProcTag )
        *uppp = (UniversalProcPtr)mytrackingproc_upp;
    else
        return -1;
    /* Only now do we test for clearing of the callback: */
    if ( callback == Py_None )
        *uppp = NULL;
    /* Create the dict if it doesn't exist yet (so we don't get such a dict for every control) */
    if ( self->ob_callbackdict == NULL )
        if ( (self->ob_callbackdict = PyDict_New()) == NULL )
            return -1;
    /* And store the Python callback */
    sprintf(keybuf, "%x", (unsigned)which);
    if (PyDict_SetItemString(self->ob_callbackdict, keybuf, callback) < 0)
        return -1;
    return 0;
}

static PyObject *
callcallback(ControlObject *self, OSType which, PyObject *arglist)
{
    char keybuf[9];
    PyObject *func, *rv;

    sprintf(keybuf, "%x", (unsigned)which);
    if ( self->ob_callbackdict == NULL ||
                    (func = PyDict_GetItemString(self->ob_callbackdict, keybuf)) == NULL ) {
        PySys_WriteStderr("Control callback %x without callback object\n", (unsigned)which);
        return NULL;
    }
    rv = PyEval_CallObject(func, arglist);
    if ( rv == NULL ) {
        PySys_WriteStderr("Exception in control callback %x handler\n", (unsigned)which);
        PyErr_Print();
    }
    return rv;
}

static pascal void
myactionproc(ControlHandle control, SInt16 part)
{
    ControlObject *ctl_obj;
    PyObject *arglist, *rv;

    ctl_obj = (ControlObject *)CtlObj_WhichControl(control);
    arglist = Py_BuildValue("Oh", ctl_obj, part);
    rv = callcallback(ctl_obj, kMyControlActionProcTag, arglist);
    Py_XDECREF(arglist);
    Py_XDECREF(rv);
}

static pascal ControlPartCode
mykeydownproc(ControlHandle control, SInt16 keyCode, SInt16 charCode, SInt16 modifiers)
{
    ControlObject *ctl_obj;
    PyObject *arglist, *rv;
    short c_rv = 0;

    ctl_obj = (ControlObject *)CtlObj_WhichControl(control);
    arglist = Py_BuildValue("Ohhh", ctl_obj, keyCode, charCode, modifiers);
    rv = callcallback(ctl_obj, kControlUserPaneKeyDownProcTag, arglist);
    Py_XDECREF(arglist);
    if ( rv )
        if (!PyArg_Parse(rv, "h", &c_rv))
            PyErr_Clear();
    Py_XDECREF(rv);
    return (ControlPartCode)c_rv;
}

static pascal ControlPartCode
myfocusproc(ControlHandle control, ControlPartCode part)
{
    ControlObject *ctl_obj;
    PyObject *arglist, *rv;
    short c_rv = kControlFocusNoPart;

    ctl_obj = (ControlObject *)CtlObj_WhichControl(control);
    arglist = Py_BuildValue("Oh", ctl_obj, part);
    rv = callcallback(ctl_obj, kControlUserPaneFocusProcTag, arglist);
    Py_XDECREF(arglist);
    if ( rv )
        if (!PyArg_Parse(rv, "h", &c_rv))
            PyErr_Clear();
    Py_XDECREF(rv);
    return (ControlPartCode)c_rv;
}

static pascal void
mydrawproc(ControlHandle control, SInt16 part)
{
    ControlObject *ctl_obj;
    PyObject *arglist, *rv;

    ctl_obj = (ControlObject *)CtlObj_WhichControl(control);
    arglist = Py_BuildValue("Oh", ctl_obj, part);
    rv = callcallback(ctl_obj, kControlUserPaneDrawProcTag, arglist);
    Py_XDECREF(arglist);
    Py_XDECREF(rv);
}

static pascal void
myidleproc(ControlHandle control)
{
    ControlObject *ctl_obj;
    PyObject *arglist, *rv;

    ctl_obj = (ControlObject *)CtlObj_WhichControl(control);
    arglist = Py_BuildValue("O", ctl_obj);
    rv = callcallback(ctl_obj, kControlUserPaneIdleProcTag, arglist);
    Py_XDECREF(arglist);
    Py_XDECREF(rv);
}

static pascal ControlPartCode
myhittestproc(ControlHandle control, Point where)
{
    ControlObject *ctl_obj;
    PyObject *arglist, *rv;
    short c_rv = -1;

    ctl_obj = (ControlObject *)CtlObj_WhichControl(control);
    arglist = Py_BuildValue("OO&", ctl_obj, PyMac_BuildPoint, where);
    rv = callcallback(ctl_obj, kControlUserPaneHitTestProcTag, arglist);
    Py_XDECREF(arglist);
    /* Ignore errors, nothing we can do about them */
    if ( rv )
        if (!PyArg_Parse(rv, "h", &c_rv))
            PyErr_Clear();
    Py_XDECREF(rv);
    return (ControlPartCode)c_rv;
}

static pascal ControlPartCode
mytrackingproc(ControlHandle control, Point startPt, ControlActionUPP actionProc)
{
    ControlObject *ctl_obj;
    PyObject *arglist, *rv;
    short c_rv = -1;

    ctl_obj = (ControlObject *)CtlObj_WhichControl(control);
    /* We cannot pass the actionProc without lots of work */
    arglist = Py_BuildValue("OO&", ctl_obj, PyMac_BuildPoint, startPt);
    rv = callcallback(ctl_obj, kControlUserPaneTrackingProcTag, arglist);
    Py_XDECREF(arglist);
    if ( rv )
        if (!PyArg_Parse(rv, "h", &c_rv))
            PyErr_Clear();
    Py_XDECREF(rv);
    return (ControlPartCode)c_rv;
}

#else /* __LP64__ */

static PyMethodDef Ctl_methods[] = {
    {NULL, NULL, 0}
};

#endif /* __LP64__ */

void init_Ctl(void)
{
    PyObject *m;

#ifndef __LP64__
    PyObject *d;

    mytracker_upp = NewControlActionUPP(mytracker);
    myactionproc_upp = NewControlActionUPP(myactionproc);
    mykeydownproc_upp = NewControlUserPaneKeyDownUPP(mykeydownproc);
    myfocusproc_upp = NewControlUserPaneFocusUPP(myfocusproc);
    mydrawproc_upp = NewControlUserPaneDrawUPP(mydrawproc);
    myidleproc_upp = NewControlUserPaneIdleUPP(myidleproc);
    myhittestproc_upp = NewControlUserPaneHitTestUPP(myhittestproc);
    mytrackingproc_upp = NewControlUserPaneTrackingUPP(mytrackingproc);
    PyMac_INIT_TOOLBOX_OBJECT_NEW(ControlHandle, CtlObj_New);
    PyMac_INIT_TOOLBOX_OBJECT_CONVERT(ControlHandle, CtlObj_Convert);
#endif /* !__LP64__ */

    m = Py_InitModule("_Ctl", Ctl_methods);

#ifndef __LP64__
    d = PyModule_GetDict(m);
    Ctl_Error = PyMac_GetOSErrException();
    if (Ctl_Error == NULL ||
        PyDict_SetItemString(d, "Error", Ctl_Error) != 0)
        return;
    Control_Type.ob_type = &PyType_Type;
    if (PyType_Ready(&Control_Type) < 0) return;
    Py_INCREF(&Control_Type);
    PyModule_AddObject(m, "Control", (PyObject *)&Control_Type);
    /* Backward-compatible name */
    Py_INCREF(&Control_Type);
    PyModule_AddObject(m, "ControlType", (PyObject *)&Control_Type);
#endif /* !__LP64__ */
}

/* ======================== End module _Ctl ========================= */

