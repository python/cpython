
/* ========================== Module waste ========================== */

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

#include <WASTE.h>

/* Exported by Qdmodule.c: */
extern PyObject *QdRGB_New(RGBColor *);
extern int QdRGB_Convert(PyObject *, RGBColor *);

/* Forward declaration */
staticforward PyObject *WEOObj_New(WEObjectReference);

/*
** Parse/generate TextStyle records
*/
static
PyObject *TextStyle_New(itself)
	TextStylePtr itself;
{

	return Py_BuildValue("lllO&", (long)itself->tsFont, (long)itself->tsFace, (long)itself->tsSize, QdRGB_New,
				&itself->tsColor);
}

static
TextStyle_Convert(v, p_itself)
	PyObject *v;
	TextStylePtr p_itself;
{
	long font, face, size;
	
	if( !PyArg_ParseTuple(v, "lllO&", &font, &face, &size, QdRGB_Convert, &p_itself->tsColor) )
		return 0;
	p_itself->tsFont = (short)font;
	p_itself->tsFace = (Style)face;
	p_itself->tsSize = (short)size;
	return 1;
}

/*
** Parse/generate RunInfo records
*/
static
PyObject *RunInfo_New(itself)
	WERunInfo *itself;
{

	return Py_BuildValue("llhhO&O&", itself->runStart, itself->runEnd, itself->runHeight,
		itself->runAscent, TextStyle_New, &itself->runStyle, WEOObj_New, itself->runObject);
}

/* Conversion of long points and rects */
int
LongRect_Convert(PyObject *v, LongRect *r)
{
	return PyArg_Parse(v, "(llll)", &r->left, &r->top, &r->right, &r->bottom);
}

PyObject *
LongRect_New(LongRect *r)
{
	return Py_BuildValue("(llll)", r->left, r->top, r->right, r->bottom);
}


LongPt_Convert(PyObject *v, LongPt *p)
{
	return PyArg_Parse(v, "(ll)", &p->h, &p->v);
}

PyObject *
LongPt_New(LongPt *p)
{
	return Py_BuildValue("(ll)", p->h, p->v);
}

static PyObject *waste_Error;

/* ------------------------ Object type WEO ------------------------- */

PyTypeObject WEO_Type;

#define WEOObj_Check(x) ((x)->ob_type == &WEO_Type)

typedef struct WEOObject {
	PyObject_HEAD
	WEObjectReference ob_itself;
} WEOObject;

PyObject *WEOObj_New(itself)
	WEObjectReference itself;
{
	WEOObject *it;
	if (itself == NULL) {
						Py_INCREF(Py_None);
						return Py_None;
					}
	it = PyObject_NEW(WEOObject, &WEO_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}
WEOObj_Convert(v, p_itself)
	PyObject *v;
	WEObjectReference *p_itself;
{
	if (!WEOObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "WEO required");
		return 0;
	}
	*p_itself = ((WEOObject *)v)->ob_itself;
	return 1;
}

static void WEOObj_dealloc(self)
	WEOObject *self;
{
	/* Cleanup of self->ob_itself goes here */
	PyMem_DEL(self);
}

static PyObject *WEOObj_WEGetObjectType(_self, _args)
	WEOObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	FlavorType _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WEGetObjectType(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildOSType, _rv);
	return _res;
}

static PyObject *WEOObj_WEGetObjectDataHandle(_self, _args)
	WEOObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Handle _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WEGetObjectDataHandle(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *WEOObj_WEGetObjectSize(_self, _args)
	WEOObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Point _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WEGetObjectSize(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildPoint, _rv);
	return _res;
}

static PyObject *WEOObj_WEGetObjectRefCon(_self, _args)
	WEOObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WEGetObjectRefCon(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *WEOObj_WESetObjectRefCon(_self, _args)
	WEOObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long refCon;
	if (!PyArg_ParseTuple(_args, "l",
	                      &refCon))
		return NULL;
	WESetObjectRefCon(_self->ob_itself,
	                  refCon);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef WEOObj_methods[] = {
	{"WEGetObjectType", (PyCFunction)WEOObj_WEGetObjectType, 1,
	 "() -> (FlavorType _rv)"},
	{"WEGetObjectDataHandle", (PyCFunction)WEOObj_WEGetObjectDataHandle, 1,
	 "() -> (Handle _rv)"},
	{"WEGetObjectSize", (PyCFunction)WEOObj_WEGetObjectSize, 1,
	 "() -> (Point _rv)"},
	{"WEGetObjectRefCon", (PyCFunction)WEOObj_WEGetObjectRefCon, 1,
	 "() -> (long _rv)"},
	{"WESetObjectRefCon", (PyCFunction)WEOObj_WESetObjectRefCon, 1,
	 "(long refCon) -> None"},
	{NULL, NULL, 0}
};

PyMethodChain WEOObj_chain = { WEOObj_methods, NULL };

static PyObject *WEOObj_getattr(self, name)
	WEOObject *self;
	char *name;
{
	return Py_FindMethodInChain(&WEOObj_chain, (PyObject *)self, name);
}

#define WEOObj_setattr NULL

PyTypeObject WEO_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"WEO", /*tp_name*/
	sizeof(WEOObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) WEOObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) WEOObj_getattr, /*tp_getattr*/
	(setattrfunc) WEOObj_setattr, /*tp_setattr*/
};

/* ---------------------- End object type WEO ----------------------- */


/* ----------------------- Object type waste ------------------------ */

PyTypeObject waste_Type;

#define wasteObj_Check(x) ((x)->ob_type == &waste_Type)

typedef struct wasteObject {
	PyObject_HEAD
	WEReference ob_itself;
} wasteObject;

PyObject *wasteObj_New(itself)
	WEReference itself;
{
	wasteObject *it;
	if (itself == NULL) {
						PyErr_SetString(waste_Error,"Cannot create null WE");
						return NULL;
					}
	it = PyObject_NEW(wasteObject, &waste_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}
wasteObj_Convert(v, p_itself)
	PyObject *v;
	WEReference *p_itself;
{
	if (!wasteObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "waste required");
		return 0;
	}
	*p_itself = ((wasteObject *)v)->ob_itself;
	return 1;
}

static void wasteObj_dealloc(self)
	wasteObject *self;
{
	WEDispose(self->ob_itself);
	PyMem_DEL(self);
}

static PyObject *wasteObj_WEGetText(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Handle _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WEGetText(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *wasteObj_WEGetChar(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	long offset;
	if (!PyArg_ParseTuple(_args, "l",
	                      &offset))
		return NULL;
	_rv = WEGetChar(offset,
	                _self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WEGetTextLength(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WEGetTextLength(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WECountLines(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WECountLines(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WEGetHeight(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long _rv;
	long startLine;
	long endLine;
	if (!PyArg_ParseTuple(_args, "ll",
	                      &startLine,
	                      &endLine))
		return NULL;
	_rv = WEGetHeight(startLine,
	                  endLine,
	                  _self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WEGetSelection(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long selStart;
	long selEnd;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	WEGetSelection(&selStart,
	               &selEnd,
	               _self->ob_itself);
	_res = Py_BuildValue("ll",
	                     selStart,
	                     selEnd);
	return _res;
}

static PyObject *wasteObj_WEGetDestRect(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	LongRect destRect;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	WEGetDestRect(&destRect,
	              _self->ob_itself);
	_res = Py_BuildValue("O&",
	                     LongRect_New, &destRect);
	return _res;
}

static PyObject *wasteObj_WEGetViewRect(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	LongRect viewRect;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	WEGetViewRect(&viewRect,
	              _self->ob_itself);
	_res = Py_BuildValue("O&",
	                     LongRect_New, &viewRect);
	return _res;
}

static PyObject *wasteObj_WEIsActive(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WEIsActive(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WEOffsetToLine(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long _rv;
	long offset;
	if (!PyArg_ParseTuple(_args, "l",
	                      &offset))
		return NULL;
	_rv = WEOffsetToLine(offset,
	                     _self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WEGetLineRange(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long lineNo;
	long lineStart;
	long lineEnd;
	if (!PyArg_ParseTuple(_args, "l",
	                      &lineNo))
		return NULL;
	WEGetLineRange(lineNo,
	               &lineStart,
	               &lineEnd,
	               _self->ob_itself);
	_res = Py_BuildValue("ll",
	                     lineStart,
	                     lineEnd);
	return _res;
}

static PyObject *wasteObj_WESetSelection(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long selStart;
	long selEnd;
	if (!PyArg_ParseTuple(_args, "ll",
	                      &selStart,
	                      &selEnd))
		return NULL;
	WESetSelection(selStart,
	               selEnd,
	               _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WESetDestRect(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	LongRect destRect;
	if (!PyArg_ParseTuple(_args, "O&",
	                      LongRect_Convert, &destRect))
		return NULL;
	WESetDestRect(&destRect,
	              _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WESetViewRect(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	LongRect viewRect;
	if (!PyArg_ParseTuple(_args, "O&",
	                      LongRect_Convert, &viewRect))
		return NULL;
	WESetViewRect(&viewRect,
	              _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEContinuousStyle(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	WEStyleMode mode;
	TextStyle ts;
	if (!PyArg_ParseTuple(_args, "h",
	                      &mode))
		return NULL;
	_rv = WEContinuousStyle(&mode,
	                        &ts,
	                        _self->ob_itself);
	_res = Py_BuildValue("bhO&",
	                     _rv,
	                     mode,
	                     TextStyle_New, &ts);
	return _res;
}

static PyObject *wasteObj_WEGetRunInfo(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long offset;
	WERunInfo runInfo;
	if (!PyArg_ParseTuple(_args, "l",
	                      &offset))
		return NULL;
	WEGetRunInfo(offset,
	             &runInfo,
	             _self->ob_itself);
	_res = Py_BuildValue("O&",
	                     RunInfo_New, &runInfo);
	return _res;
}

static PyObject *wasteObj_WEGetOffset(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long _rv;
	LongPt thePoint;
	char edge;
	if (!PyArg_ParseTuple(_args, "O&",
	                      LongPt_Convert, &thePoint))
		return NULL;
	_rv = WEGetOffset(&thePoint,
	                  &edge,
	                  _self->ob_itself);
	_res = Py_BuildValue("lc",
	                     _rv,
	                     edge);
	return _res;
}

static PyObject *wasteObj_WEGetPoint(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long offset;
	LongPt thePoint;
	short lineHeight;
	if (!PyArg_ParseTuple(_args, "l",
	                      &offset))
		return NULL;
	WEGetPoint(offset,
	           &thePoint,
	           &lineHeight,
	           _self->ob_itself);
	_res = Py_BuildValue("O&h",
	                     LongPt_New, &thePoint,
	                     lineHeight);
	return _res;
}

static PyObject *wasteObj_WEFindWord(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long offset;
	char edge;
	long wordStart;
	long wordEnd;
	if (!PyArg_ParseTuple(_args, "lc",
	                      &offset,
	                      &edge))
		return NULL;
	WEFindWord(offset,
	           edge,
	           &wordStart,
	           &wordEnd,
	           _self->ob_itself);
	_res = Py_BuildValue("ll",
	                     wordStart,
	                     wordEnd);
	return _res;
}

static PyObject *wasteObj_WEFindLine(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long offset;
	char edge;
	long lineStart;
	long lineEnd;
	if (!PyArg_ParseTuple(_args, "lc",
	                      &offset,
	                      &edge))
		return NULL;
	WEFindLine(offset,
	           edge,
	           &lineStart,
	           &lineEnd,
	           _self->ob_itself);
	_res = Py_BuildValue("ll",
	                     lineStart,
	                     lineEnd);
	return _res;
}

static PyObject *wasteObj_WECopyRange(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	long rangeStart;
	long rangeEnd;
	Handle hText;
	StScrpHandle hStyles;
	WESoupHandle hSoup;
	if (!PyArg_ParseTuple(_args, "llO&O&O&",
	                      &rangeStart,
	                      &rangeEnd,
	                      OptResObj_Convert, &hText,
	                      OptResObj_Convert, &hStyles,
	                      OptResObj_Convert, &hSoup))
		return NULL;
	_err = WECopyRange(rangeStart,
	                   rangeEnd,
	                   hText,
	                   hStyles,
	                   hSoup,
	                   _self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEGetAlignment(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	WEAlignment _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WEGetAlignment(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WESetAlignment(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	WEAlignment alignment;
	if (!PyArg_ParseTuple(_args, "b",
	                      &alignment))
		return NULL;
	WESetAlignment(alignment,
	               _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WECalText(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = WECalText(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEUpdate(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle updateRgn;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &updateRgn))
		return NULL;
	WEUpdate(updateRgn,
	         _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEScroll(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long hOffset;
	long vOffset;
	if (!PyArg_ParseTuple(_args, "ll",
	                      &hOffset,
	                      &vOffset))
		return NULL;
	WEScroll(hOffset,
	         vOffset,
	         _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WESelView(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	WESelView(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEActivate(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	WEActivate(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEDeactivate(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	WEDeactivate(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEKey(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short key;
	EventModifiers modifiers;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &key,
	                      &modifiers))
		return NULL;
	WEKey(key,
	      modifiers,
	      _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEClick(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Point hitPt;
	EventModifiers modifiers;
	unsigned long clickTime;
	if (!PyArg_ParseTuple(_args, "O&hl",
	                      PyMac_GetPoint, &hitPt,
	                      &modifiers,
	                      &clickTime))
		return NULL;
	WEClick(hitPt,
	        modifiers,
	        clickTime,
	        _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEAdjustCursor(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	Point mouseLoc;
	RgnHandle mouseRgn;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetPoint, &mouseLoc,
	                      ResObj_Convert, &mouseRgn))
		return NULL;
	_rv = WEAdjustCursor(mouseLoc,
	                     mouseRgn,
	                     _self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WEIdle(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	unsigned long maxSleep;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	WEIdle(&maxSleep,
	       _self->ob_itself);
	_res = Py_BuildValue("l",
	                     maxSleep);
	return _res;
}

static PyObject *wasteObj_WEInsert(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	char *pText__in__;
	long pText__len__;
	int pText__in_len__;
	StScrpHandle hStyles;
	WESoupHandle hSoup;
	if (!PyArg_ParseTuple(_args, "s#O&O&",
	                      &pText__in__, &pText__in_len__,
	                      OptResObj_Convert, &hStyles,
	                      OptResObj_Convert, &hSoup))
		return NULL;
	pText__len__ = pText__in_len__;
	_err = WEInsert(pText__in__, pText__len__,
	                hStyles,
	                hSoup,
	                _self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
 pText__error__: ;
	return _res;
}

static PyObject *wasteObj_WEDelete(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = WEDelete(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WESetStyle(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	WEStyleMode mode;
	TextStyle ts;
	if (!PyArg_ParseTuple(_args, "hO&",
	                      &mode,
	                      TextStyle_Convert, &ts))
		return NULL;
	_err = WESetStyle(mode,
	                  &ts,
	                  _self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEUseStyleScrap(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	StScrpHandle hStyles;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &hStyles))
		return NULL;
	_err = WEUseStyleScrap(hStyles,
	                       _self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEUseText(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle hText;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &hText))
		return NULL;
	_err = WEUseText(hText,
	                 _self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEUndo(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = WEUndo(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEClearUndo(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	WEClearUndo(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEGetUndoInfo(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	WEActionKind _rv;
	Boolean redoFlag;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WEGetUndoInfo(&redoFlag,
	                    _self->ob_itself);
	_res = Py_BuildValue("hb",
	                     _rv,
	                     redoFlag);
	return _res;
}

static PyObject *wasteObj_WEIsTyping(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WEIsTyping(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WEGetModCount(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	unsigned long _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WEGetModCount(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WEResetModCount(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	WEResetModCount(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEInsertObject(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	FlavorType objectType;
	Handle objectDataHandle;
	Point objectSize;
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      PyMac_GetOSType, &objectType,
	                      ResObj_Convert, &objectDataHandle,
	                      PyMac_GetPoint, &objectSize))
		return NULL;
	_err = WEInsertObject(objectType,
	                      objectDataHandle,
	                      objectSize,
	                      _self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEGetSelectedObject(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	WEObjectReference obj;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = WEGetSelectedObject(&obj,
	                           _self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     WEOObj_New, obj);
	return _res;
}

static PyObject *wasteObj_WEFindNextObject(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long _rv;
	long offset;
	WEObjectReference obj;
	if (!PyArg_ParseTuple(_args, "l",
	                      &offset))
		return NULL;
	_rv = WEFindNextObject(offset,
	                       &obj,
	                       _self->ob_itself);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     WEOObj_New, obj);
	return _res;
}

static PyObject *wasteObj_WEUseSoup(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	WESoupHandle hSoup;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &hSoup))
		return NULL;
	_err = WEUseSoup(hSoup,
	                 _self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WECut(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = WECut(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WECopy(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = WECopy(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEPaste(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = WEPaste(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WECanPaste(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WECanPaste(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WEGetHiliteRgn(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	RgnHandle _rv;
	long rangeStart;
	long rangeEnd;
	if (!PyArg_ParseTuple(_args, "ll",
	                      &rangeStart,
	                      &rangeEnd))
		return NULL;
	_rv = WEGetHiliteRgn(rangeStart,
	                     rangeEnd,
	                     _self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *wasteObj_WECharByte(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	long offset;
	if (!PyArg_ParseTuple(_args, "l",
	                      &offset))
		return NULL;
	_rv = WECharByte(offset,
	                 _self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WECharType(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	long offset;
	if (!PyArg_ParseTuple(_args, "l",
	                      &offset))
		return NULL;
	_rv = WECharType(offset,
	                 _self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WEStopInlineSession(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	WEStopInlineSession(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEFeatureFlag(_self, _args)
	wasteObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	short feature;
	short action;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &feature,
	                      &action))
		return NULL;
	_rv = WEFeatureFlag(feature,
	                    action,
	                    _self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyMethodDef wasteObj_methods[] = {
	{"WEGetText", (PyCFunction)wasteObj_WEGetText, 1,
	 "() -> (Handle _rv)"},
	{"WEGetChar", (PyCFunction)wasteObj_WEGetChar, 1,
	 "(long offset) -> (short _rv)"},
	{"WEGetTextLength", (PyCFunction)wasteObj_WEGetTextLength, 1,
	 "() -> (long _rv)"},
	{"WECountLines", (PyCFunction)wasteObj_WECountLines, 1,
	 "() -> (long _rv)"},
	{"WEGetHeight", (PyCFunction)wasteObj_WEGetHeight, 1,
	 "(long startLine, long endLine) -> (long _rv)"},
	{"WEGetSelection", (PyCFunction)wasteObj_WEGetSelection, 1,
	 "() -> (long selStart, long selEnd)"},
	{"WEGetDestRect", (PyCFunction)wasteObj_WEGetDestRect, 1,
	 "() -> (LongRect destRect)"},
	{"WEGetViewRect", (PyCFunction)wasteObj_WEGetViewRect, 1,
	 "() -> (LongRect viewRect)"},
	{"WEIsActive", (PyCFunction)wasteObj_WEIsActive, 1,
	 "() -> (Boolean _rv)"},
	{"WEOffsetToLine", (PyCFunction)wasteObj_WEOffsetToLine, 1,
	 "(long offset) -> (long _rv)"},
	{"WEGetLineRange", (PyCFunction)wasteObj_WEGetLineRange, 1,
	 "(long lineNo) -> (long lineStart, long lineEnd)"},
	{"WESetSelection", (PyCFunction)wasteObj_WESetSelection, 1,
	 "(long selStart, long selEnd) -> None"},
	{"WESetDestRect", (PyCFunction)wasteObj_WESetDestRect, 1,
	 "(LongRect destRect) -> None"},
	{"WESetViewRect", (PyCFunction)wasteObj_WESetViewRect, 1,
	 "(LongRect viewRect) -> None"},
	{"WEContinuousStyle", (PyCFunction)wasteObj_WEContinuousStyle, 1,
	 "(WEStyleMode mode) -> (Boolean _rv, WEStyleMode mode, TextStyle ts)"},
	{"WEGetRunInfo", (PyCFunction)wasteObj_WEGetRunInfo, 1,
	 "(long offset) -> (WERunInfo runInfo)"},
	{"WEGetOffset", (PyCFunction)wasteObj_WEGetOffset, 1,
	 "(LongPt thePoint) -> (long _rv, char edge)"},
	{"WEGetPoint", (PyCFunction)wasteObj_WEGetPoint, 1,
	 "(long offset) -> (LongPt thePoint, short lineHeight)"},
	{"WEFindWord", (PyCFunction)wasteObj_WEFindWord, 1,
	 "(long offset, char edge) -> (long wordStart, long wordEnd)"},
	{"WEFindLine", (PyCFunction)wasteObj_WEFindLine, 1,
	 "(long offset, char edge) -> (long lineStart, long lineEnd)"},
	{"WECopyRange", (PyCFunction)wasteObj_WECopyRange, 1,
	 "(long rangeStart, long rangeEnd, Handle hText, StScrpHandle hStyles, WESoupHandle hSoup) -> None"},
	{"WEGetAlignment", (PyCFunction)wasteObj_WEGetAlignment, 1,
	 "() -> (WEAlignment _rv)"},
	{"WESetAlignment", (PyCFunction)wasteObj_WESetAlignment, 1,
	 "(WEAlignment alignment) -> None"},
	{"WECalText", (PyCFunction)wasteObj_WECalText, 1,
	 "() -> None"},
	{"WEUpdate", (PyCFunction)wasteObj_WEUpdate, 1,
	 "(RgnHandle updateRgn) -> None"},
	{"WEScroll", (PyCFunction)wasteObj_WEScroll, 1,
	 "(long hOffset, long vOffset) -> None"},
	{"WESelView", (PyCFunction)wasteObj_WESelView, 1,
	 "() -> None"},
	{"WEActivate", (PyCFunction)wasteObj_WEActivate, 1,
	 "() -> None"},
	{"WEDeactivate", (PyCFunction)wasteObj_WEDeactivate, 1,
	 "() -> None"},
	{"WEKey", (PyCFunction)wasteObj_WEKey, 1,
	 "(short key, EventModifiers modifiers) -> None"},
	{"WEClick", (PyCFunction)wasteObj_WEClick, 1,
	 "(Point hitPt, EventModifiers modifiers, unsigned long clickTime) -> None"},
	{"WEAdjustCursor", (PyCFunction)wasteObj_WEAdjustCursor, 1,
	 "(Point mouseLoc, RgnHandle mouseRgn) -> (Boolean _rv)"},
	{"WEIdle", (PyCFunction)wasteObj_WEIdle, 1,
	 "() -> (unsigned long maxSleep)"},
	{"WEInsert", (PyCFunction)wasteObj_WEInsert, 1,
	 "(Buffer pText, StScrpHandle hStyles, WESoupHandle hSoup) -> None"},
	{"WEDelete", (PyCFunction)wasteObj_WEDelete, 1,
	 "() -> None"},
	{"WESetStyle", (PyCFunction)wasteObj_WESetStyle, 1,
	 "(WEStyleMode mode, TextStyle ts) -> None"},
	{"WEUseStyleScrap", (PyCFunction)wasteObj_WEUseStyleScrap, 1,
	 "(StScrpHandle hStyles) -> None"},
	{"WEUseText", (PyCFunction)wasteObj_WEUseText, 1,
	 "(Handle hText) -> None"},
	{"WEUndo", (PyCFunction)wasteObj_WEUndo, 1,
	 "() -> None"},
	{"WEClearUndo", (PyCFunction)wasteObj_WEClearUndo, 1,
	 "() -> None"},
	{"WEGetUndoInfo", (PyCFunction)wasteObj_WEGetUndoInfo, 1,
	 "() -> (WEActionKind _rv, Boolean redoFlag)"},
	{"WEIsTyping", (PyCFunction)wasteObj_WEIsTyping, 1,
	 "() -> (Boolean _rv)"},
	{"WEGetModCount", (PyCFunction)wasteObj_WEGetModCount, 1,
	 "() -> (unsigned long _rv)"},
	{"WEResetModCount", (PyCFunction)wasteObj_WEResetModCount, 1,
	 "() -> None"},
	{"WEInsertObject", (PyCFunction)wasteObj_WEInsertObject, 1,
	 "(FlavorType objectType, Handle objectDataHandle, Point objectSize) -> None"},
	{"WEGetSelectedObject", (PyCFunction)wasteObj_WEGetSelectedObject, 1,
	 "() -> (WEObjectReference obj)"},
	{"WEFindNextObject", (PyCFunction)wasteObj_WEFindNextObject, 1,
	 "(long offset) -> (long _rv, WEObjectReference obj)"},
	{"WEUseSoup", (PyCFunction)wasteObj_WEUseSoup, 1,
	 "(WESoupHandle hSoup) -> None"},
	{"WECut", (PyCFunction)wasteObj_WECut, 1,
	 "() -> None"},
	{"WECopy", (PyCFunction)wasteObj_WECopy, 1,
	 "() -> None"},
	{"WEPaste", (PyCFunction)wasteObj_WEPaste, 1,
	 "() -> None"},
	{"WECanPaste", (PyCFunction)wasteObj_WECanPaste, 1,
	 "() -> (Boolean _rv)"},
	{"WEGetHiliteRgn", (PyCFunction)wasteObj_WEGetHiliteRgn, 1,
	 "(long rangeStart, long rangeEnd) -> (RgnHandle _rv)"},
	{"WECharByte", (PyCFunction)wasteObj_WECharByte, 1,
	 "(long offset) -> (short _rv)"},
	{"WECharType", (PyCFunction)wasteObj_WECharType, 1,
	 "(long offset) -> (short _rv)"},
	{"WEStopInlineSession", (PyCFunction)wasteObj_WEStopInlineSession, 1,
	 "() -> None"},
	{"WEFeatureFlag", (PyCFunction)wasteObj_WEFeatureFlag, 1,
	 "(short feature, short action) -> (short _rv)"},
	{NULL, NULL, 0}
};

PyMethodChain wasteObj_chain = { wasteObj_methods, NULL };

static PyObject *wasteObj_getattr(self, name)
	wasteObject *self;
	char *name;
{
	return Py_FindMethodInChain(&wasteObj_chain, (PyObject *)self, name);
}

#define wasteObj_setattr NULL

PyTypeObject waste_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"waste", /*tp_name*/
	sizeof(wasteObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) wasteObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) wasteObj_getattr, /*tp_getattr*/
	(setattrfunc) wasteObj_setattr, /*tp_setattr*/
};

/* --------------------- End object type waste ---------------------- */


static PyObject *waste_WENew(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	LongRect destRect;
	LongRect viewRect;
	unsigned long flags;
	WEReference we;
	if (!PyArg_ParseTuple(_args, "O&O&l",
	                      LongRect_Convert, &destRect,
	                      LongRect_Convert, &viewRect,
	                      &flags))
		return NULL;
	_err = WENew(&destRect,
	             &viewRect,
	             flags,
	             &we);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     wasteObj_New, we);
	return _res;
}

static PyObject *waste_WEInstallTSMHandlers(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = WEInstallTSMHandlers();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *waste_WERemoveTSMHandlers(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = WERemoveTSMHandlers();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *waste_WELongPointToPoint(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	LongPt lp;
	Point p;
	if (!PyArg_ParseTuple(_args, "O&",
	                      LongPt_Convert, &lp))
		return NULL;
	WELongPointToPoint(&lp,
	                   &p);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildPoint, p);
	return _res;
}

static PyObject *waste_WEPointToLongPoint(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Point p;
	LongPt lp;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &p))
		return NULL;
	WEPointToLongPoint(p,
	                   &lp);
	_res = Py_BuildValue("O&",
	                     LongPt_New, &lp);
	return _res;
}

static PyObject *waste_WESetLongRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	LongRect lr;
	long left;
	long top;
	long right;
	long bottom;
	if (!PyArg_ParseTuple(_args, "llll",
	                      &left,
	                      &top,
	                      &right,
	                      &bottom))
		return NULL;
	WESetLongRect(&lr,
	              left,
	              top,
	              right,
	              bottom);
	_res = Py_BuildValue("O&",
	                     LongRect_New, &lr);
	return _res;
}

static PyObject *waste_WELongRectToRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	LongRect lr;
	Rect r;
	if (!PyArg_ParseTuple(_args, "O&",
	                      LongRect_Convert, &lr))
		return NULL;
	WELongRectToRect(&lr,
	                 &r);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &r);
	return _res;
}

static PyObject *waste_WERectToLongRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Rect r;
	LongRect lr;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &r))
		return NULL;
	WERectToLongRect(&r,
	                 &lr);
	_res = Py_BuildValue("O&",
	                     LongRect_New, &lr);
	return _res;
}

static PyObject *waste_WEOffsetLongRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	LongRect lr;
	long hOffset;
	long vOffset;
	if (!PyArg_ParseTuple(_args, "ll",
	                      &hOffset,
	                      &vOffset))
		return NULL;
	WEOffsetLongRect(&lr,
	                 hOffset,
	                 vOffset);
	_res = Py_BuildValue("O&",
	                     LongRect_New, &lr);
	return _res;
}

static PyObject *waste_WELongPointInLongRect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	LongPt lp;
	LongRect lr;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      LongPt_Convert, &lp,
	                      LongRect_Convert, &lr))
		return NULL;
	_rv = WELongPointInLongRect(&lp,
	                            &lr);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyMethodDef waste_methods[] = {
	{"WENew", (PyCFunction)waste_WENew, 1,
	 "(LongRect destRect, LongRect viewRect, unsigned long flags) -> (WEReference we)"},
	{"WEInstallTSMHandlers", (PyCFunction)waste_WEInstallTSMHandlers, 1,
	 "() -> None"},
	{"WERemoveTSMHandlers", (PyCFunction)waste_WERemoveTSMHandlers, 1,
	 "() -> None"},
	{"WELongPointToPoint", (PyCFunction)waste_WELongPointToPoint, 1,
	 "(LongPt lp) -> (Point p)"},
	{"WEPointToLongPoint", (PyCFunction)waste_WEPointToLongPoint, 1,
	 "(Point p) -> (LongPt lp)"},
	{"WESetLongRect", (PyCFunction)waste_WESetLongRect, 1,
	 "(long left, long top, long right, long bottom) -> (LongRect lr)"},
	{"WELongRectToRect", (PyCFunction)waste_WELongRectToRect, 1,
	 "(LongRect lr) -> (Rect r)"},
	{"WERectToLongRect", (PyCFunction)waste_WERectToLongRect, 1,
	 "(Rect r) -> (LongRect lr)"},
	{"WEOffsetLongRect", (PyCFunction)waste_WEOffsetLongRect, 1,
	 "(long hOffset, long vOffset) -> (LongRect lr)"},
	{"WELongPointInLongRect", (PyCFunction)waste_WELongPointInLongRect, 1,
	 "(LongPt lp, LongRect lr) -> (Boolean _rv)"},
	{NULL, NULL, 0}
};




void initwaste()
{
	PyObject *m;
	PyObject *d;




	m = Py_InitModule("waste", waste_methods);
	d = PyModule_GetDict(m);
	waste_Error = PyMac_GetOSErrException();
	if (waste_Error == NULL ||
	    PyDict_SetItemString(d, "Error", waste_Error) != 0)
		Py_FatalError("can't initialize waste.Error");
}

/* ======================== End module waste ======================== */

