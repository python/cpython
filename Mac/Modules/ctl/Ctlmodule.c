
/* =========================== Module Ctl =========================== */

#include "Python.h"



#include "macglue.h"
#include "pymactoolbox.h"

#ifdef WITHOUT_FRAMEWORKS
#include <Controls.h>
#include <ControlDefinitions.h>
#else
#include <Carbon/Carbon.h>
#endif

#ifdef USE_TOOLBOX_OBJECT_GLUE
extern PyObject *_CtlObj_New(ControlHandle);
extern int _CtlObj_Convert(PyObject *, ControlHandle *);

#define CtlObj_New _CtlObj_New
#define CtlObj_Convert _CtlObj_Convert
#endif

staticforward PyObject *CtlObj_WhichControl(ControlHandle);

#define as_Control(h) ((ControlHandle)h)
#define as_Resource(ctl) ((Handle)ctl)
#if TARGET_API_MAC_CARBON
#define GetControlRect(ctl, rectp) GetControlBounds(ctl, rectp)
#else
#define GetControlRect(ctl, rectp) (*(rectp) = ((*(ctl))->contrlRect))
#endif

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
	return PyArg_ParseTuple(v, "hhhhhhO&O&", &itself->flags,
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
	return PyArg_ParseTuple(v, "O&l", PyMac_GetOSType, &itself->signature, &itself->id);
}


/* TrackControl and HandleControlClick callback support */
static PyObject *tracker;
static ControlActionUPP mytracker_upp;
static ControlUserPaneDrawUPP mydrawproc_upp;
static ControlUserPaneIdleUPP myidleproc_upp;
static ControlUserPaneHitTestUPP myhittestproc_upp;
static ControlUserPaneTrackingUPP mytrackingproc_upp;

staticforward int settrackfunc(PyObject *); 	/* forward */
staticforward void clrtrackfunc(void);	/* forward */
staticforward int setcallback(PyObject *, OSType, PyObject *, UniversalProcPtr *);

static PyObject *Ctl_Error;

/* ---------------------- Object type Control ----------------------- */

PyTypeObject Control_Type;

#define CtlObj_Check(x) ((x)->ob_type == &Control_Type)

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
CtlObj_Convert(PyObject *v, ControlHandle *p_itself)
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
	PyMem_DEL(self);
}

static PyObject *CtlObj_HiliteControl(ControlObject *_self, PyObject *_args)
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

static PyObject *CtlObj_ShowControl(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
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

static PyObject *CtlObj_Draw1Control(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
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
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &testPoint))
		return NULL;
	_rv = TestControl(_self->ob_itself,
	                  testPoint);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

#if TARGET_API_MAC_CARBON

static PyObject *CtlObj_HandleControlContextualMenuClick(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	Point inWhere;
	Boolean menuDisplayed;
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
#endif

#if TARGET_API_MAC_CARBON

static PyObject *CtlObj_GetControlClickActivation(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	Point inWhere;
	EventModifiers inModifiers;
	ClickActivationResult outResult;
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
#endif

static PyObject *CtlObj_HandleControlKey(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt16 _rv;
	SInt16 inKeyCode;
	SInt16 inCharCode;
	EventModifiers inModifiers;
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

#if TARGET_API_MAC_CARBON

static PyObject *CtlObj_HandleControlSetCursor(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	Point localPoint;
	EventModifiers modifiers;
	Boolean cursorWasSet;
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
#endif

static PyObject *CtlObj_MoveControl(ControlObject *_self, PyObject *_args)
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

static PyObject *CtlObj_SizeControl(ControlObject *_self, PyObject *_args)
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

static PyObject *CtlObj_SetControlTitle(ControlObject *_self, PyObject *_args)
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

static PyObject *CtlObj_GetControlTitle(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Str255 title;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetControlTitle(_self->ob_itself,
	                title);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildStr255, title);
	return _res;
}

static PyObject *CtlObj_GetControlValue(ControlObject *_self, PyObject *_args)
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

static PyObject *CtlObj_SetControlValue(ControlObject *_self, PyObject *_args)
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

static PyObject *CtlObj_GetControlMinimum(ControlObject *_self, PyObject *_args)
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

static PyObject *CtlObj_SetControlMinimum(ControlObject *_self, PyObject *_args)
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

static PyObject *CtlObj_GetControlMaximum(ControlObject *_self, PyObject *_args)
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

static PyObject *CtlObj_SetControlMaximum(ControlObject *_self, PyObject *_args)
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

static PyObject *CtlObj_GetControlViewSize(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 _rv;
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
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = IsValidControlHandle(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

#if TARGET_API_MAC_CARBON

static PyObject *CtlObj_SetControlID(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	ControlID inID;
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
#endif

#if TARGET_API_MAC_CARBON

static PyObject *CtlObj_GetControlID(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	ControlID outID;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetControlID(_self->ob_itself,
	                    &outID);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyControlID_New, &outID);
	return _res;
}
#endif

static PyObject *CtlObj_RemoveControlProperty(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	OSType propertyCreator;
	OSType propertyTag;
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

#if TARGET_API_MAC_CARBON

static PyObject *CtlObj_GetControlPropertyAttributes(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	OSType propertyCreator;
	OSType propertyTag;
	UInt32 attributes;
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
#endif

#if TARGET_API_MAC_CARBON

static PyObject *CtlObj_ChangeControlPropertyAttributes(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	OSType propertyCreator;
	OSType propertyTag;
	UInt32 attributesToSet;
	UInt32 attributesToClear;
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
#endif

static PyObject *CtlObj_GetControlRegion(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	ControlPartCode inPart;
	RgnHandle outRegion;
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
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetControlVariant(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *CtlObj_SetControlReference(ControlObject *_self, PyObject *_args)
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

static PyObject *CtlObj_GetControlReference(ControlObject *_self, PyObject *_args)
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

#if !TARGET_API_MAC_CARBON

static PyObject *CtlObj_GetAuxiliaryControlRecord(ControlObject *_self, PyObject *_args)
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
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *CtlObj_SetControlColor(ControlObject *_self, PyObject *_args)
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
#endif

static PyObject *CtlObj_EmbedControl(ControlObject *_self, PyObject *_args)
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

static PyObject *CtlObj_AutoEmbedControl(ControlObject *_self, PyObject *_args)
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

static PyObject *CtlObj_GetSuperControl(ControlObject *_self, PyObject *_args)
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

static PyObject *CtlObj_CountSubControls(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	UInt16 outNumChildren;
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

#if TARGET_API_MAC_CARBON

static PyObject *CtlObj_HandleControlDragTracking(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	DragTrackingMessage inMessage;
	DragReference inDrag;
	Boolean outLikesDrag;
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
#endif

#if TARGET_API_MAC_CARBON

static PyObject *CtlObj_HandleControlDragReceive(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	DragReference inDrag;
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
#endif

#if TARGET_API_MAC_CARBON

static PyObject *CtlObj_SetControlDragTrackingEnabled(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	Boolean tracks;
	if (!PyArg_ParseTuple(_args, "b",
	                      &tracks))
		return NULL;
	_err = SetControlDragTrackingEnabled(_self->ob_itself,
	                                     tracks);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

#if TARGET_API_MAC_CARBON

static PyObject *CtlObj_IsControlDragTrackingEnabled(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	Boolean tracks;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = IsControlDragTrackingEnabled(_self->ob_itself,
	                                    &tracks);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("b",
	                     tracks);
	return _res;
}
#endif

#if ACCESSOR_CALLS_ARE_FUNCTIONS

static PyObject *CtlObj_GetControlBounds(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Rect bounds;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetControlBounds(_self->ob_itself,
	                 &bounds);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &bounds);
	return _res;
}
#endif

#if ACCESSOR_CALLS_ARE_FUNCTIONS

static PyObject *CtlObj_IsControlHilited(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = IsControlHilited(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}
#endif

#if ACCESSOR_CALLS_ARE_FUNCTIONS

static PyObject *CtlObj_GetControlHilite(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	UInt16 _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetControlHilite(_self->ob_itself);
	_res = Py_BuildValue("H",
	                     _rv);
	return _res;
}
#endif

#if ACCESSOR_CALLS_ARE_FUNCTIONS

static PyObject *CtlObj_GetControlOwner(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	WindowPtr _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetControlOwner(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     WinObj_New, _rv);
	return _res;
}
#endif

#if ACCESSOR_CALLS_ARE_FUNCTIONS

static PyObject *CtlObj_GetControlDataHandle(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Handle _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetControlDataHandle(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}
#endif

#if ACCESSOR_CALLS_ARE_FUNCTIONS

static PyObject *CtlObj_GetControlPopupMenuHandle(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MenuHandle _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetControlPopupMenuHandle(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     MenuObj_New, _rv);
	return _res;
}
#endif

#if ACCESSOR_CALLS_ARE_FUNCTIONS

static PyObject *CtlObj_GetControlPopupMenuID(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetControlPopupMenuID(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}
#endif

#if ACCESSOR_CALLS_ARE_FUNCTIONS

static PyObject *CtlObj_SetControlDataHandle(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Handle dataHandle;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &dataHandle))
		return NULL;
	SetControlDataHandle(_self->ob_itself,
	                     dataHandle);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

#if ACCESSOR_CALLS_ARE_FUNCTIONS

static PyObject *CtlObj_SetControlBounds(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Rect bounds;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &bounds))
		return NULL;
	SetControlBounds(_self->ob_itself,
	                 &bounds);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

#if ACCESSOR_CALLS_ARE_FUNCTIONS

static PyObject *CtlObj_SetControlPopupMenuHandle(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MenuHandle popupMenu;
	if (!PyArg_ParseTuple(_args, "O&",
	                      MenuObj_Convert, &popupMenu))
		return NULL;
	SetControlPopupMenuHandle(_self->ob_itself,
	                          popupMenu);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

#if ACCESSOR_CALLS_ARE_FUNCTIONS

static PyObject *CtlObj_SetControlPopupMenuID(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short menuID;
	if (!PyArg_ParseTuple(_args, "h",
	                      &menuID))
		return NULL;
	SetControlPopupMenuID(_self->ob_itself,
	                      menuID);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

static PyObject *CtlObj_GetBevelButtonMenuValue(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 outValue;
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
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetBevelButtonMenuHandle(_self->ob_itself,
	                                &outHandle);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     MenuObj_New, outHandle);
	return _res;
}

static PyObject *CtlObj_SetBevelButtonTransform(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	IconTransformType transform;
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

static PyObject *CtlObj_SetImageWellTransform(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	IconTransformType inTransform;
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

static PyObject *CtlObj_as_Resource(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Handle _rv;
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
	return Py_BuildValue("O&", OptResObj_New, hdl);

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

#if !TARGET_API_MAC_CARBON

static PyObject *CtlObj_GetPopupData(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;

	PopupPrivateDataHandle hdl;

	if ( (*_self->ob_itself)->contrlData == NULL ) {
		PyErr_SetString(Ctl_Error, "No contrlData handle in control");
		return 0;
	}
	hdl = (PopupPrivateDataHandle)(*_self->ob_itself)->contrlData;
	HLock((Handle)hdl);
	_res = Py_BuildValue("O&i", MenuObj_New, (*hdl)->mHandle, (int)(*hdl)->mID);
	HUnlock((Handle)hdl);
	return _res;

}
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *CtlObj_SetPopupData(ControlObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;

	PopupPrivateDataHandle hdl;
	MenuHandle mHandle;
	short mID;

	if (!PyArg_ParseTuple(_args, "O&h", MenuObj_Convert, &mHandle, &mID) )
		return 0;
	if ( (*_self->ob_itself)->contrlData == NULL ) {
		PyErr_SetString(Ctl_Error, "No contrlData handle in control");
		return 0;
	}
	hdl = (PopupPrivateDataHandle)(*_self->ob_itself)->contrlData;
	(*hdl)->mHandle = mHandle;
	(*hdl)->mID = mID;
	Py_INCREF(Py_None);
	return Py_None;

}
#endif

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
	{"SetUpControlTextColor", (PyCFunction)CtlObj_SetUpControlTextColor, 1,
	 "(SInt16 inDepth, Boolean inIsColorDevice) -> None"},
	{"DragControl", (PyCFunction)CtlObj_DragControl, 1,
	 "(Point startPoint, Rect limitRect, Rect slopRect, DragConstraint axis) -> None"},
	{"TestControl", (PyCFunction)CtlObj_TestControl, 1,
	 "(Point testPoint) -> (ControlPartCode _rv)"},

#if TARGET_API_MAC_CARBON
	{"HandleControlContextualMenuClick", (PyCFunction)CtlObj_HandleControlContextualMenuClick, 1,
	 "(Point inWhere) -> (Boolean menuDisplayed)"},
#endif

#if TARGET_API_MAC_CARBON
	{"GetControlClickActivation", (PyCFunction)CtlObj_GetControlClickActivation, 1,
	 "(Point inWhere, EventModifiers inModifiers) -> (ClickActivationResult outResult)"},
#endif
	{"HandleControlKey", (PyCFunction)CtlObj_HandleControlKey, 1,
	 "(SInt16 inKeyCode, SInt16 inCharCode, EventModifiers inModifiers) -> (SInt16 _rv)"},

#if TARGET_API_MAC_CARBON
	{"HandleControlSetCursor", (PyCFunction)CtlObj_HandleControlSetCursor, 1,
	 "(Point localPoint, EventModifiers modifiers) -> (Boolean cursorWasSet)"},
#endif
	{"MoveControl", (PyCFunction)CtlObj_MoveControl, 1,
	 "(SInt16 h, SInt16 v) -> None"},
	{"SizeControl", (PyCFunction)CtlObj_SizeControl, 1,
	 "(SInt16 w, SInt16 h) -> None"},
	{"SetControlTitle", (PyCFunction)CtlObj_SetControlTitle, 1,
	 "(Str255 title) -> None"},
	{"GetControlTitle", (PyCFunction)CtlObj_GetControlTitle, 1,
	 "() -> (Str255 title)"},
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
	{"GetControlViewSize", (PyCFunction)CtlObj_GetControlViewSize, 1,
	 "() -> (SInt32 _rv)"},
	{"SetControlViewSize", (PyCFunction)CtlObj_SetControlViewSize, 1,
	 "(SInt32 newViewSize) -> None"},
	{"GetControl32BitValue", (PyCFunction)CtlObj_GetControl32BitValue, 1,
	 "() -> (SInt32 _rv)"},
	{"SetControl32BitValue", (PyCFunction)CtlObj_SetControl32BitValue, 1,
	 "(SInt32 newValue) -> None"},
	{"GetControl32BitMaximum", (PyCFunction)CtlObj_GetControl32BitMaximum, 1,
	 "() -> (SInt32 _rv)"},
	{"SetControl32BitMaximum", (PyCFunction)CtlObj_SetControl32BitMaximum, 1,
	 "(SInt32 newMaximum) -> None"},
	{"GetControl32BitMinimum", (PyCFunction)CtlObj_GetControl32BitMinimum, 1,
	 "() -> (SInt32 _rv)"},
	{"SetControl32BitMinimum", (PyCFunction)CtlObj_SetControl32BitMinimum, 1,
	 "(SInt32 newMinimum) -> None"},
	{"IsValidControlHandle", (PyCFunction)CtlObj_IsValidControlHandle, 1,
	 "() -> (Boolean _rv)"},

#if TARGET_API_MAC_CARBON
	{"SetControlID", (PyCFunction)CtlObj_SetControlID, 1,
	 "(ControlID inID) -> None"},
#endif

#if TARGET_API_MAC_CARBON
	{"GetControlID", (PyCFunction)CtlObj_GetControlID, 1,
	 "() -> (ControlID outID)"},
#endif
	{"RemoveControlProperty", (PyCFunction)CtlObj_RemoveControlProperty, 1,
	 "(OSType propertyCreator, OSType propertyTag) -> None"},

#if TARGET_API_MAC_CARBON
	{"GetControlPropertyAttributes", (PyCFunction)CtlObj_GetControlPropertyAttributes, 1,
	 "(OSType propertyCreator, OSType propertyTag) -> (UInt32 attributes)"},
#endif

#if TARGET_API_MAC_CARBON
	{"ChangeControlPropertyAttributes", (PyCFunction)CtlObj_ChangeControlPropertyAttributes, 1,
	 "(OSType propertyCreator, OSType propertyTag, UInt32 attributesToSet, UInt32 attributesToClear) -> None"},
#endif
	{"GetControlRegion", (PyCFunction)CtlObj_GetControlRegion, 1,
	 "(ControlPartCode inPart, RgnHandle outRegion) -> None"},
	{"GetControlVariant", (PyCFunction)CtlObj_GetControlVariant, 1,
	 "() -> (ControlVariant _rv)"},
	{"SetControlReference", (PyCFunction)CtlObj_SetControlReference, 1,
	 "(SInt32 data) -> None"},
	{"GetControlReference", (PyCFunction)CtlObj_GetControlReference, 1,
	 "() -> (SInt32 _rv)"},

#if !TARGET_API_MAC_CARBON
	{"GetAuxiliaryControlRecord", (PyCFunction)CtlObj_GetAuxiliaryControlRecord, 1,
	 "() -> (Boolean _rv, AuxCtlHandle acHndl)"},
#endif

#if !TARGET_API_MAC_CARBON
	{"SetControlColor", (PyCFunction)CtlObj_SetControlColor, 1,
	 "(CCTabHandle newColorTable) -> None"},
#endif
	{"EmbedControl", (PyCFunction)CtlObj_EmbedControl, 1,
	 "(ControlHandle inContainer) -> None"},
	{"AutoEmbedControl", (PyCFunction)CtlObj_AutoEmbedControl, 1,
	 "(WindowPtr inWindow) -> None"},
	{"GetSuperControl", (PyCFunction)CtlObj_GetSuperControl, 1,
	 "() -> (ControlHandle outParent)"},
	{"CountSubControls", (PyCFunction)CtlObj_CountSubControls, 1,
	 "() -> (UInt16 outNumChildren)"},
	{"GetIndexedSubControl", (PyCFunction)CtlObj_GetIndexedSubControl, 1,
	 "(UInt16 inIndex) -> (ControlHandle outSubControl)"},
	{"SetControlSupervisor", (PyCFunction)CtlObj_SetControlSupervisor, 1,
	 "(ControlHandle inBoss) -> None"},
	{"GetControlFeatures", (PyCFunction)CtlObj_GetControlFeatures, 1,
	 "() -> (UInt32 outFeatures)"},
	{"GetControlDataSize", (PyCFunction)CtlObj_GetControlDataSize, 1,
	 "(ControlPartCode inPart, ResType inTagName) -> (Size outMaxSize)"},

#if TARGET_API_MAC_CARBON
	{"HandleControlDragTracking", (PyCFunction)CtlObj_HandleControlDragTracking, 1,
	 "(DragTrackingMessage inMessage, DragReference inDrag) -> (Boolean outLikesDrag)"},
#endif

#if TARGET_API_MAC_CARBON
	{"HandleControlDragReceive", (PyCFunction)CtlObj_HandleControlDragReceive, 1,
	 "(DragReference inDrag) -> None"},
#endif

#if TARGET_API_MAC_CARBON
	{"SetControlDragTrackingEnabled", (PyCFunction)CtlObj_SetControlDragTrackingEnabled, 1,
	 "(Boolean tracks) -> None"},
#endif

#if TARGET_API_MAC_CARBON
	{"IsControlDragTrackingEnabled", (PyCFunction)CtlObj_IsControlDragTrackingEnabled, 1,
	 "() -> (Boolean tracks)"},
#endif

#if ACCESSOR_CALLS_ARE_FUNCTIONS
	{"GetControlBounds", (PyCFunction)CtlObj_GetControlBounds, 1,
	 "() -> (Rect bounds)"},
#endif

#if ACCESSOR_CALLS_ARE_FUNCTIONS
	{"IsControlHilited", (PyCFunction)CtlObj_IsControlHilited, 1,
	 "() -> (Boolean _rv)"},
#endif

#if ACCESSOR_CALLS_ARE_FUNCTIONS
	{"GetControlHilite", (PyCFunction)CtlObj_GetControlHilite, 1,
	 "() -> (UInt16 _rv)"},
#endif

#if ACCESSOR_CALLS_ARE_FUNCTIONS
	{"GetControlOwner", (PyCFunction)CtlObj_GetControlOwner, 1,
	 "() -> (WindowPtr _rv)"},
#endif

#if ACCESSOR_CALLS_ARE_FUNCTIONS
	{"GetControlDataHandle", (PyCFunction)CtlObj_GetControlDataHandle, 1,
	 "() -> (Handle _rv)"},
#endif

#if ACCESSOR_CALLS_ARE_FUNCTIONS
	{"GetControlPopupMenuHandle", (PyCFunction)CtlObj_GetControlPopupMenuHandle, 1,
	 "() -> (MenuHandle _rv)"},
#endif

#if ACCESSOR_CALLS_ARE_FUNCTIONS
	{"GetControlPopupMenuID", (PyCFunction)CtlObj_GetControlPopupMenuID, 1,
	 "() -> (short _rv)"},
#endif

#if ACCESSOR_CALLS_ARE_FUNCTIONS
	{"SetControlDataHandle", (PyCFunction)CtlObj_SetControlDataHandle, 1,
	 "(Handle dataHandle) -> None"},
#endif

#if ACCESSOR_CALLS_ARE_FUNCTIONS
	{"SetControlBounds", (PyCFunction)CtlObj_SetControlBounds, 1,
	 "(Rect bounds) -> None"},
#endif

#if ACCESSOR_CALLS_ARE_FUNCTIONS
	{"SetControlPopupMenuHandle", (PyCFunction)CtlObj_SetControlPopupMenuHandle, 1,
	 "(MenuHandle popupMenu) -> None"},
#endif

#if ACCESSOR_CALLS_ARE_FUNCTIONS
	{"SetControlPopupMenuID", (PyCFunction)CtlObj_SetControlPopupMenuID, 1,
	 "(short menuID) -> None"},
#endif
	{"GetBevelButtonMenuValue", (PyCFunction)CtlObj_GetBevelButtonMenuValue, 1,
	 "() -> (SInt16 outValue)"},
	{"SetBevelButtonMenuValue", (PyCFunction)CtlObj_SetBevelButtonMenuValue, 1,
	 "(SInt16 inValue) -> None"},
	{"GetBevelButtonMenuHandle", (PyCFunction)CtlObj_GetBevelButtonMenuHandle, 1,
	 "() -> (MenuHandle outHandle)"},
	{"SetBevelButtonTransform", (PyCFunction)CtlObj_SetBevelButtonTransform, 1,
	 "(IconTransformType transform) -> None"},
	{"SetDisclosureTriangleLastValue", (PyCFunction)CtlObj_SetDisclosureTriangleLastValue, 1,
	 "(SInt16 inValue) -> None"},
	{"GetTabContentRect", (PyCFunction)CtlObj_GetTabContentRect, 1,
	 "() -> (Rect outContentRect)"},
	{"SetTabEnabled", (PyCFunction)CtlObj_SetTabEnabled, 1,
	 "(SInt16 inTabToHilite, Boolean inEnabled) -> None"},
	{"SetImageWellTransform", (PyCFunction)CtlObj_SetImageWellTransform, 1,
	 "(IconTransformType inTransform) -> None"},
	{"as_Resource", (PyCFunction)CtlObj_as_Resource, 1,
	 "() -> (Handle _rv)"},
	{"GetControlRect", (PyCFunction)CtlObj_GetControlRect, 1,
	 "() -> (Rect rect)"},
	{"DisposeControl", (PyCFunction)CtlObj_DisposeControl, 1,
	 "() -> None"},
	{"TrackControl", (PyCFunction)CtlObj_TrackControl, 1,
	 "(Point startPoint [,trackercallback]) -> (ControlPartCode _rv)"},
	{"HandleControlClick", (PyCFunction)CtlObj_HandleControlClick, 1,
	 "(Point startPoint, Integer modifiers, [,trackercallback]) -> (ControlPartCode _rv)"},
	{"SetControlData", (PyCFunction)CtlObj_SetControlData, 1,
	 "(stuff) -> None"},
	{"GetControlData", (PyCFunction)CtlObj_GetControlData, 1,
	 "(part, type) -> String"},
	{"SetControlData_Handle", (PyCFunction)CtlObj_SetControlData_Handle, 1,
	 "(ResObj) -> None"},
	{"GetControlData_Handle", (PyCFunction)CtlObj_GetControlData_Handle, 1,
	 "(part, type) -> ResObj"},
	{"SetControlData_Callback", (PyCFunction)CtlObj_SetControlData_Callback, 1,
	 "(callbackfunc) -> None"},

#if !TARGET_API_MAC_CARBON
	{"GetPopupData", (PyCFunction)CtlObj_GetPopupData, 1,
	 NULL},
#endif

#if !TARGET_API_MAC_CARBON
	{"SetPopupData", (PyCFunction)CtlObj_SetPopupData, 1,
	 NULL},
#endif
	{NULL, NULL, 0}
};

PyMethodChain CtlObj_chain = { CtlObj_methods, NULL };

static PyObject *CtlObj_getattr(ControlObject *self, char *name)
{
	return Py_FindMethodInChain(&CtlObj_chain, (PyObject *)self, name);
}

#define CtlObj_setattr NULL

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
	(cmpfunc) CtlObj_compare, /*tp_compare*/
	(reprfunc) CtlObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) CtlObj_hash, /*tp_hash*/
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

static PyObject *Ctl_FindControl(PyObject *_self, PyObject *_args)
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

static PyObject *Ctl_FindControlUnderMouse(PyObject *_self, PyObject *_args)
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

static PyObject *Ctl_IdleControls(PyObject *_self, PyObject *_args)
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

#if TARGET_API_MAC_CARBON

static PyObject *Ctl_GetControlByID(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowPtr inWindow;
	ControlID inID;
	ControlHandle outControl;
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
#endif

static PyObject *Ctl_DumpControlHierarchy(PyObject *_self, PyObject *_args)
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

static PyObject *Ctl_CreateRootControl(PyObject *_self, PyObject *_args)
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

static PyObject *Ctl_GetRootControl(PyObject *_self, PyObject *_args)
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

static PyObject *Ctl_GetKeyboardFocus(PyObject *_self, PyObject *_args)
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

static PyObject *Ctl_SetKeyboardFocus(PyObject *_self, PyObject *_args)
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

static PyObject *Ctl_AdvanceKeyboardFocus(PyObject *_self, PyObject *_args)
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

static PyObject *Ctl_ReverseKeyboardFocus(PyObject *_self, PyObject *_args)
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

static PyObject *Ctl_ClearKeyboardFocus(PyObject *_self, PyObject *_args)
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

#if TARGET_API_MAC_CARBON

static PyObject *Ctl_SetAutomaticControlDragTrackingEnabledForWindow(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowPtr theWindow;
	Boolean tracks;
	if (!PyArg_ParseTuple(_args, "O&b",
	                      WinObj_Convert, &theWindow,
	                      &tracks))
		return NULL;
	_err = SetAutomaticControlDragTrackingEnabledForWindow(theWindow,
	                                                       tracks);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

#if TARGET_API_MAC_CARBON

static PyObject *Ctl_IsAutomaticControlDragTrackingEnabledForWindow(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	WindowPtr theWindow;
	Boolean tracks;
	if (!PyArg_ParseTuple(_args, "O&",
	                      WinObj_Convert, &theWindow))
		return NULL;
	_err = IsAutomaticControlDragTrackingEnabledForWindow(theWindow,
	                                                      &tracks);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("b",
	                     tracks);
	return _res;
}
#endif

static PyObject *Ctl_as_Control(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ControlHandle _rv;
	Handle h;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &h))
		return NULL;
	_rv = as_Control(h);
	_res = Py_BuildValue("O&",
	                     CtlObj_New, _rv);
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

#if TARGET_API_MAC_CARBON
	{"GetControlByID", (PyCFunction)Ctl_GetControlByID, 1,
	 "(WindowPtr inWindow, ControlID inID) -> (ControlHandle outControl)"},
#endif
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

#if TARGET_API_MAC_CARBON
	{"SetAutomaticControlDragTrackingEnabledForWindow", (PyCFunction)Ctl_SetAutomaticControlDragTrackingEnabledForWindow, 1,
	 "(WindowPtr theWindow, Boolean tracks) -> None"},
#endif

#if TARGET_API_MAC_CARBON
	{"IsAutomaticControlDragTrackingEnabledForWindow", (PyCFunction)Ctl_IsAutomaticControlDragTrackingEnabledForWindow, 1,
	 "(WindowPtr theWindow) -> (Boolean tracks)"},
#endif
	{"as_Control", (PyCFunction)Ctl_as_Control, 1,
	 "(Handle h) -> (ControlHandle _rv)"},
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
	else
		PySys_WriteStderr("TrackControl or HandleControlClick: exception in tracker function\n");
}

static int
setcallback(PyObject *myself, OSType which, PyObject *callback, UniversalProcPtr *uppp)
{
	ControlObject *self = (ControlObject *)myself;
	char keybuf[9];
	
	if ( which == kControlUserPaneDrawProcTag )
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
	sprintf(keybuf, "%x", which);
	if (PyDict_SetItemString(self->ob_callbackdict, keybuf, callback) < 0)
		return -1;
	return 0;
}

static PyObject *
callcallback(ControlObject *self, OSType which, PyObject *arglist)
{
	char keybuf[9];
	PyObject *func, *rv;
	
	sprintf(keybuf, "%x", which);
	if ( self->ob_callbackdict == NULL ||
			(func = PyDict_GetItemString(self->ob_callbackdict, keybuf)) == NULL ) {
		PySys_WriteStderr("Control callback %x without callback object\n", which);
		return NULL;
	}
	rv = PyEval_CallObject(func, arglist);
	if ( rv == NULL )
		PySys_WriteStderr("Exception in control callback %x handler\n", which);
	return rv;
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
		PyArg_Parse(rv, "h", &c_rv);
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
		PyArg_Parse(rv, "h", &c_rv);
	Py_XDECREF(rv);
	return (ControlPartCode)c_rv;
}


void initCtl(void)
{
	PyObject *m;
	PyObject *d;



	mytracker_upp = NewControlActionUPP(mytracker);
	mydrawproc_upp = NewControlUserPaneDrawUPP(mydrawproc);
	myidleproc_upp = NewControlUserPaneIdleUPP(myidleproc);
	myhittestproc_upp = NewControlUserPaneHitTestUPP(myhittestproc);
	mytrackingproc_upp = NewControlUserPaneTrackingUPP(mytrackingproc);
	PyMac_INIT_TOOLBOX_OBJECT_NEW(ControlHandle, CtlObj_New);
	PyMac_INIT_TOOLBOX_OBJECT_CONVERT(ControlHandle, CtlObj_Convert);


	m = Py_InitModule("Ctl", Ctl_methods);
	d = PyModule_GetDict(m);
	Ctl_Error = PyMac_GetOSErrException();
	if (Ctl_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Ctl_Error) != 0)
		return;
	Control_Type.ob_type = &PyType_Type;
	Py_INCREF(&Control_Type);
	if (PyDict_SetItemString(d, "ControlType", (PyObject *)&Control_Type) != 0)
		Py_FatalError("can't initialize ControlType");
}

/* ========================= End module Ctl ========================= */

