
/* =========================== Module _TE =========================== */

#include "Python.h"



#include "pymactoolbox.h"

/* Macro to test whether a weak-loaded CFM function exists */
#define PyMac_PRECHECK(rtn) do { if ( &rtn == NULL )  {\
        PyErr_SetString(PyExc_NotImplementedError, \
        "Not available in this shared library/OS version"); \
        return NULL; \
    }} while(0)


#include <Carbon/Carbon.h>

#ifdef USE_TOOLBOX_OBJECT_GLUE
extern PyObject *_TEObj_New(TEHandle);
extern int _TEObj_Convert(PyObject *, TEHandle *);

#define TEObj_New _TEObj_New
#define TEObj_Convert _TEObj_Convert
#endif

#define as_TE(h) ((TEHandle)h)
#define as_Resource(teh) ((Handle)teh)

/*
** Parse/generate TextStyle records
*/
static PyObject *
TextStyle_New(TextStylePtr itself)
{

	return Py_BuildValue("lllO&", (long)itself->tsFont, (long)itself->tsFace, (long)itself->tsSize, QdRGB_New,
				&itself->tsColor);
}

static int
TextStyle_Convert(PyObject *v, TextStylePtr p_itself)
{
	long font, face, size;
	
	if( !PyArg_ParseTuple(v, "lllO&", &font, &face, &size, QdRGB_Convert, &p_itself->tsColor) )
		return 0;
	p_itself->tsFont = (short)font;
	p_itself->tsFace = (Style)face;
	p_itself->tsSize = (short)size;
	return 1;
}

static PyObject *TE_Error;

/* ------------------------- Object type TE ------------------------- */

PyTypeObject TE_Type;

#define TEObj_Check(x) ((x)->ob_type == &TE_Type || PyObject_TypeCheck((x), &TE_Type))

typedef struct TEObject {
	PyObject_HEAD
	TEHandle ob_itself;
} TEObject;

PyObject *TEObj_New(TEHandle itself)
{
	TEObject *it;
	if (itself == NULL) {
						PyErr_SetString(TE_Error,"Cannot create null TE");
						return NULL;
					}
	it = PyObject_NEW(TEObject, &TE_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}
int TEObj_Convert(PyObject *v, TEHandle *p_itself)
{
	if (!TEObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "TE required");
		return 0;
	}
	*p_itself = ((TEObject *)v)->ob_itself;
	return 1;
}

static void TEObj_dealloc(TEObject *self)
{
	TEDispose(self->ob_itself);
	self->ob_type->tp_free((PyObject *)self);
}

static PyObject *TEObj_TESetText(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	char *text__in__;
	long text__len__;
	int text__in_len__;
#ifndef TESetText
	PyMac_PRECHECK(TESetText);
#endif
	if (!PyArg_ParseTuple(_args, "s#",
	                      &text__in__, &text__in_len__))
		return NULL;
	text__len__ = text__in_len__;
	TESetText(text__in__, text__len__,
	          _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TEObj_TEGetText(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CharsHandle _rv;
#ifndef TEGetText
	PyMac_PRECHECK(TEGetText);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = TEGetText(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *TEObj_TEIdle(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef TEIdle
	PyMac_PRECHECK(TEIdle);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	TEIdle(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TEObj_TESetSelect(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long selStart;
	long selEnd;
#ifndef TESetSelect
	PyMac_PRECHECK(TESetSelect);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &selStart,
	                      &selEnd))
		return NULL;
	TESetSelect(selStart,
	            selEnd,
	            _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TEObj_TEActivate(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef TEActivate
	PyMac_PRECHECK(TEActivate);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	TEActivate(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TEObj_TEDeactivate(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef TEDeactivate
	PyMac_PRECHECK(TEDeactivate);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	TEDeactivate(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TEObj_TEKey(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	CharParameter key;
#ifndef TEKey
	PyMac_PRECHECK(TEKey);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &key))
		return NULL;
	TEKey(key,
	      _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TEObj_TECut(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef TECut
	PyMac_PRECHECK(TECut);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	TECut(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TEObj_TECopy(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef TECopy
	PyMac_PRECHECK(TECopy);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	TECopy(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TEObj_TEPaste(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef TEPaste
	PyMac_PRECHECK(TEPaste);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	TEPaste(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TEObj_TEDelete(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef TEDelete
	PyMac_PRECHECK(TEDelete);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	TEDelete(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TEObj_TEInsert(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	char *text__in__;
	long text__len__;
	int text__in_len__;
#ifndef TEInsert
	PyMac_PRECHECK(TEInsert);
#endif
	if (!PyArg_ParseTuple(_args, "s#",
	                      &text__in__, &text__in_len__))
		return NULL;
	text__len__ = text__in_len__;
	TEInsert(text__in__, text__len__,
	         _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TEObj_TESetAlignment(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short just;
#ifndef TESetAlignment
	PyMac_PRECHECK(TESetAlignment);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &just))
		return NULL;
	TESetAlignment(just,
	               _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TEObj_TEUpdate(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Rect rUpdate;
#ifndef TEUpdate
	PyMac_PRECHECK(TEUpdate);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &rUpdate))
		return NULL;
	TEUpdate(&rUpdate,
	         _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TEObj_TEScroll(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short dh;
	short dv;
#ifndef TEScroll
	PyMac_PRECHECK(TEScroll);
#endif
	if (!PyArg_ParseTuple(_args, "hh",
	                      &dh,
	                      &dv))
		return NULL;
	TEScroll(dh,
	         dv,
	         _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TEObj_TESelView(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef TESelView
	PyMac_PRECHECK(TESelView);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	TESelView(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TEObj_TEPinScroll(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short dh;
	short dv;
#ifndef TEPinScroll
	PyMac_PRECHECK(TEPinScroll);
#endif
	if (!PyArg_ParseTuple(_args, "hh",
	                      &dh,
	                      &dv))
		return NULL;
	TEPinScroll(dh,
	            dv,
	            _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TEObj_TEAutoView(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean fAuto;
#ifndef TEAutoView
	PyMac_PRECHECK(TEAutoView);
#endif
	if (!PyArg_ParseTuple(_args, "b",
	                      &fAuto))
		return NULL;
	TEAutoView(fAuto,
	           _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TEObj_TECalText(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef TECalText
	PyMac_PRECHECK(TECalText);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	TECalText(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TEObj_TEGetOffset(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short _rv;
	Point pt;
#ifndef TEGetOffset
	PyMac_PRECHECK(TEGetOffset);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &pt))
		return NULL;
	_rv = TEGetOffset(pt,
	                  _self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *TEObj_TEGetPoint(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Point _rv;
	short offset;
#ifndef TEGetPoint
	PyMac_PRECHECK(TEGetPoint);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &offset))
		return NULL;
	_rv = TEGetPoint(offset,
	                 _self->ob_itself);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildPoint, _rv);
	return _res;
}

static PyObject *TEObj_TEClick(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Point pt;
	Boolean fExtend;
#ifndef TEClick
	PyMac_PRECHECK(TEClick);
#endif
	if (!PyArg_ParseTuple(_args, "O&b",
	                      PyMac_GetPoint, &pt,
	                      &fExtend))
		return NULL;
	TEClick(pt,
	        fExtend,
	        _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TEObj_TESetStyleHandle(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TEStyleHandle theHandle;
#ifndef TESetStyleHandle
	PyMac_PRECHECK(TESetStyleHandle);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &theHandle))
		return NULL;
	TESetStyleHandle(theHandle,
	                 _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TEObj_TEGetStyleHandle(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TEStyleHandle _rv;
#ifndef TEGetStyleHandle
	PyMac_PRECHECK(TEGetStyleHandle);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = TEGetStyleHandle(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *TEObj_TEGetStyle(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short offset;
	TextStyle theStyle;
	short lineHeight;
	short fontAscent;
#ifndef TEGetStyle
	PyMac_PRECHECK(TEGetStyle);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &offset))
		return NULL;
	TEGetStyle(offset,
	           &theStyle,
	           &lineHeight,
	           &fontAscent,
	           _self->ob_itself);
	_res = Py_BuildValue("O&hh",
	                     TextStyle_New, &theStyle,
	                     lineHeight,
	                     fontAscent);
	return _res;
}

static PyObject *TEObj_TEStylePaste(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef TEStylePaste
	PyMac_PRECHECK(TEStylePaste);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	TEStylePaste(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TEObj_TESetStyle(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short mode;
	TextStyle newStyle;
	Boolean fRedraw;
#ifndef TESetStyle
	PyMac_PRECHECK(TESetStyle);
#endif
	if (!PyArg_ParseTuple(_args, "hO&b",
	                      &mode,
	                      TextStyle_Convert, &newStyle,
	                      &fRedraw))
		return NULL;
	TESetStyle(mode,
	           &newStyle,
	           fRedraw,
	           _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TEObj_TEReplaceStyle(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short mode;
	TextStyle oldStyle;
	TextStyle newStyle;
	Boolean fRedraw;
#ifndef TEReplaceStyle
	PyMac_PRECHECK(TEReplaceStyle);
#endif
	if (!PyArg_ParseTuple(_args, "hO&O&b",
	                      &mode,
	                      TextStyle_Convert, &oldStyle,
	                      TextStyle_Convert, &newStyle,
	                      &fRedraw))
		return NULL;
	TEReplaceStyle(mode,
	               &oldStyle,
	               &newStyle,
	               fRedraw,
	               _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TEObj_TEGetStyleScrapHandle(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	StScrpHandle _rv;
#ifndef TEGetStyleScrapHandle
	PyMac_PRECHECK(TEGetStyleScrapHandle);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = TEGetStyleScrapHandle(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *TEObj_TEStyleInsert(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	char *text__in__;
	long text__len__;
	int text__in_len__;
	StScrpHandle hST;
#ifndef TEStyleInsert
	PyMac_PRECHECK(TEStyleInsert);
#endif
	if (!PyArg_ParseTuple(_args, "s#O&",
	                      &text__in__, &text__in_len__,
	                      ResObj_Convert, &hST))
		return NULL;
	text__len__ = text__in_len__;
	TEStyleInsert(text__in__, text__len__,
	              hST,
	              _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TEObj_TEGetHeight(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
	long endLine;
	long startLine;
#ifndef TEGetHeight
	PyMac_PRECHECK(TEGetHeight);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &endLine,
	                      &startLine))
		return NULL;
	_rv = TEGetHeight(endLine,
	                  startLine,
	                  _self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *TEObj_TEContinuousStyle(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	short mode;
	TextStyle aStyle;
#ifndef TEContinuousStyle
	PyMac_PRECHECK(TEContinuousStyle);
#endif
	if (!PyArg_ParseTuple(_args, "hO&",
	                      &mode,
	                      TextStyle_Convert, &aStyle))
		return NULL;
	_rv = TEContinuousStyle(&mode,
	                        &aStyle,
	                        _self->ob_itself);
	_res = Py_BuildValue("bhO&",
	                     _rv,
	                     mode,
	                     TextStyle_New, &aStyle);
	return _res;
}

static PyObject *TEObj_TEUseStyleScrap(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long rangeStart;
	long rangeEnd;
	StScrpHandle newStyles;
	Boolean fRedraw;
#ifndef TEUseStyleScrap
	PyMac_PRECHECK(TEUseStyleScrap);
#endif
	if (!PyArg_ParseTuple(_args, "llO&b",
	                      &rangeStart,
	                      &rangeEnd,
	                      ResObj_Convert, &newStyles,
	                      &fRedraw))
		return NULL;
	TEUseStyleScrap(rangeStart,
	                rangeEnd,
	                newStyles,
	                fRedraw,
	                _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TEObj_TENumStyles(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
	long rangeStart;
	long rangeEnd;
#ifndef TENumStyles
	PyMac_PRECHECK(TENumStyles);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &rangeStart,
	                      &rangeEnd))
		return NULL;
	_rv = TENumStyles(rangeStart,
	                  rangeEnd,
	                  _self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *TEObj_TEFeatureFlag(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short _rv;
	short feature;
	short action;
#ifndef TEFeatureFlag
	PyMac_PRECHECK(TEFeatureFlag);
#endif
	if (!PyArg_ParseTuple(_args, "hh",
	                      &feature,
	                      &action))
		return NULL;
	_rv = TEFeatureFlag(feature,
	                    action,
	                    _self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *TEObj_TEGetHiliteRgn(TEObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	RgnHandle region;
#ifndef TEGetHiliteRgn
	PyMac_PRECHECK(TEGetHiliteRgn);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &region))
		return NULL;
	_err = TEGetHiliteRgn(region,
	                      _self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TEObj_as_Resource(TEObject *_self, PyObject *_args)
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

static PyMethodDef TEObj_methods[] = {
	{"TESetText", (PyCFunction)TEObj_TESetText, 1,
	 PyDoc_STR("(Buffer text) -> None")},
	{"TEGetText", (PyCFunction)TEObj_TEGetText, 1,
	 PyDoc_STR("() -> (CharsHandle _rv)")},
	{"TEIdle", (PyCFunction)TEObj_TEIdle, 1,
	 PyDoc_STR("() -> None")},
	{"TESetSelect", (PyCFunction)TEObj_TESetSelect, 1,
	 PyDoc_STR("(long selStart, long selEnd) -> None")},
	{"TEActivate", (PyCFunction)TEObj_TEActivate, 1,
	 PyDoc_STR("() -> None")},
	{"TEDeactivate", (PyCFunction)TEObj_TEDeactivate, 1,
	 PyDoc_STR("() -> None")},
	{"TEKey", (PyCFunction)TEObj_TEKey, 1,
	 PyDoc_STR("(CharParameter key) -> None")},
	{"TECut", (PyCFunction)TEObj_TECut, 1,
	 PyDoc_STR("() -> None")},
	{"TECopy", (PyCFunction)TEObj_TECopy, 1,
	 PyDoc_STR("() -> None")},
	{"TEPaste", (PyCFunction)TEObj_TEPaste, 1,
	 PyDoc_STR("() -> None")},
	{"TEDelete", (PyCFunction)TEObj_TEDelete, 1,
	 PyDoc_STR("() -> None")},
	{"TEInsert", (PyCFunction)TEObj_TEInsert, 1,
	 PyDoc_STR("(Buffer text) -> None")},
	{"TESetAlignment", (PyCFunction)TEObj_TESetAlignment, 1,
	 PyDoc_STR("(short just) -> None")},
	{"TEUpdate", (PyCFunction)TEObj_TEUpdate, 1,
	 PyDoc_STR("(Rect rUpdate) -> None")},
	{"TEScroll", (PyCFunction)TEObj_TEScroll, 1,
	 PyDoc_STR("(short dh, short dv) -> None")},
	{"TESelView", (PyCFunction)TEObj_TESelView, 1,
	 PyDoc_STR("() -> None")},
	{"TEPinScroll", (PyCFunction)TEObj_TEPinScroll, 1,
	 PyDoc_STR("(short dh, short dv) -> None")},
	{"TEAutoView", (PyCFunction)TEObj_TEAutoView, 1,
	 PyDoc_STR("(Boolean fAuto) -> None")},
	{"TECalText", (PyCFunction)TEObj_TECalText, 1,
	 PyDoc_STR("() -> None")},
	{"TEGetOffset", (PyCFunction)TEObj_TEGetOffset, 1,
	 PyDoc_STR("(Point pt) -> (short _rv)")},
	{"TEGetPoint", (PyCFunction)TEObj_TEGetPoint, 1,
	 PyDoc_STR("(short offset) -> (Point _rv)")},
	{"TEClick", (PyCFunction)TEObj_TEClick, 1,
	 PyDoc_STR("(Point pt, Boolean fExtend) -> None")},
	{"TESetStyleHandle", (PyCFunction)TEObj_TESetStyleHandle, 1,
	 PyDoc_STR("(TEStyleHandle theHandle) -> None")},
	{"TEGetStyleHandle", (PyCFunction)TEObj_TEGetStyleHandle, 1,
	 PyDoc_STR("() -> (TEStyleHandle _rv)")},
	{"TEGetStyle", (PyCFunction)TEObj_TEGetStyle, 1,
	 PyDoc_STR("(short offset) -> (TextStyle theStyle, short lineHeight, short fontAscent)")},
	{"TEStylePaste", (PyCFunction)TEObj_TEStylePaste, 1,
	 PyDoc_STR("() -> None")},
	{"TESetStyle", (PyCFunction)TEObj_TESetStyle, 1,
	 PyDoc_STR("(short mode, TextStyle newStyle, Boolean fRedraw) -> None")},
	{"TEReplaceStyle", (PyCFunction)TEObj_TEReplaceStyle, 1,
	 PyDoc_STR("(short mode, TextStyle oldStyle, TextStyle newStyle, Boolean fRedraw) -> None")},
	{"TEGetStyleScrapHandle", (PyCFunction)TEObj_TEGetStyleScrapHandle, 1,
	 PyDoc_STR("() -> (StScrpHandle _rv)")},
	{"TEStyleInsert", (PyCFunction)TEObj_TEStyleInsert, 1,
	 PyDoc_STR("(Buffer text, StScrpHandle hST) -> None")},
	{"TEGetHeight", (PyCFunction)TEObj_TEGetHeight, 1,
	 PyDoc_STR("(long endLine, long startLine) -> (long _rv)")},
	{"TEContinuousStyle", (PyCFunction)TEObj_TEContinuousStyle, 1,
	 PyDoc_STR("(short mode, TextStyle aStyle) -> (Boolean _rv, short mode, TextStyle aStyle)")},
	{"TEUseStyleScrap", (PyCFunction)TEObj_TEUseStyleScrap, 1,
	 PyDoc_STR("(long rangeStart, long rangeEnd, StScrpHandle newStyles, Boolean fRedraw) -> None")},
	{"TENumStyles", (PyCFunction)TEObj_TENumStyles, 1,
	 PyDoc_STR("(long rangeStart, long rangeEnd) -> (long _rv)")},
	{"TEFeatureFlag", (PyCFunction)TEObj_TEFeatureFlag, 1,
	 PyDoc_STR("(short feature, short action) -> (short _rv)")},
	{"TEGetHiliteRgn", (PyCFunction)TEObj_TEGetHiliteRgn, 1,
	 PyDoc_STR("(RgnHandle region) -> None")},
	{"as_Resource", (PyCFunction)TEObj_as_Resource, 1,
	 PyDoc_STR("() -> (Handle _rv)")},
	{NULL, NULL, 0}
};

static PyObject *TEObj_get_destRect(TEObject *self, void *closure)
{
	return Py_BuildValue("O&", PyMac_BuildRect, &(*self->ob_itself)->destRect);
}

#define TEObj_set_destRect NULL

static PyObject *TEObj_get_viewRect(TEObject *self, void *closure)
{
	return Py_BuildValue("O&", PyMac_BuildRect, &(*self->ob_itself)->viewRect);
}

#define TEObj_set_viewRect NULL

static PyObject *TEObj_get_selRect(TEObject *self, void *closure)
{
	return Py_BuildValue("O&", PyMac_BuildRect, &(*self->ob_itself)->selRect);
}

#define TEObj_set_selRect NULL

static PyObject *TEObj_get_lineHeight(TEObject *self, void *closure)
{
	return Py_BuildValue("h", (*self->ob_itself)->lineHeight);
}

#define TEObj_set_lineHeight NULL

static PyObject *TEObj_get_fontAscent(TEObject *self, void *closure)
{
	return Py_BuildValue("h", (*self->ob_itself)->fontAscent);
}

#define TEObj_set_fontAscent NULL

static PyObject *TEObj_get_selPoint(TEObject *self, void *closure)
{
	return Py_BuildValue("O&", PyMac_BuildPoint, (*self->ob_itself)->selPoint);
}

#define TEObj_set_selPoint NULL

static PyObject *TEObj_get_selStart(TEObject *self, void *closure)
{
	return Py_BuildValue("h", (*self->ob_itself)->selStart);
}

#define TEObj_set_selStart NULL

static PyObject *TEObj_get_selEnd(TEObject *self, void *closure)
{
	return Py_BuildValue("h", (*self->ob_itself)->selEnd);
}

#define TEObj_set_selEnd NULL

static PyObject *TEObj_get_active(TEObject *self, void *closure)
{
	return Py_BuildValue("h", (*self->ob_itself)->active);
}

#define TEObj_set_active NULL

static PyObject *TEObj_get_just(TEObject *self, void *closure)
{
	return Py_BuildValue("h", (*self->ob_itself)->just);
}

#define TEObj_set_just NULL

static PyObject *TEObj_get_teLength(TEObject *self, void *closure)
{
	return Py_BuildValue("h", (*self->ob_itself)->teLength);
}

#define TEObj_set_teLength NULL

static PyObject *TEObj_get_txFont(TEObject *self, void *closure)
{
	return Py_BuildValue("h", (*self->ob_itself)->txFont);
}

#define TEObj_set_txFont NULL

static PyObject *TEObj_get_txFace(TEObject *self, void *closure)
{
	return Py_BuildValue("h", (*self->ob_itself)->txFace);
}

#define TEObj_set_txFace NULL

static PyObject *TEObj_get_txMode(TEObject *self, void *closure)
{
	return Py_BuildValue("h", (*self->ob_itself)->txMode);
}

#define TEObj_set_txMode NULL

static PyObject *TEObj_get_txSize(TEObject *self, void *closure)
{
	return Py_BuildValue("h", (*self->ob_itself)->txSize);
}

#define TEObj_set_txSize NULL

static PyObject *TEObj_get_nLines(TEObject *self, void *closure)
{
	return Py_BuildValue("h", (*self->ob_itself)->nLines);
}

#define TEObj_set_nLines NULL

static PyGetSetDef TEObj_getsetlist[] = {
	{"destRect", (getter)TEObj_get_destRect, (setter)TEObj_set_destRect, "Destination rectangle"},
	{"viewRect", (getter)TEObj_get_viewRect, (setter)TEObj_set_viewRect, "Viewing rectangle"},
	{"selRect", (getter)TEObj_get_selRect, (setter)TEObj_set_selRect, "Selection rectangle"},
	{"lineHeight", (getter)TEObj_get_lineHeight, (setter)TEObj_set_lineHeight, "Height of a line"},
	{"fontAscent", (getter)TEObj_get_fontAscent, (setter)TEObj_set_fontAscent, "Ascent of a line"},
	{"selPoint", (getter)TEObj_get_selPoint, (setter)TEObj_set_selPoint, "Selection Point"},
	{"selStart", (getter)TEObj_get_selStart, (setter)TEObj_set_selStart, "Start of selection"},
	{"selEnd", (getter)TEObj_get_selEnd, (setter)TEObj_set_selEnd, "End of selection"},
	{"active", (getter)TEObj_get_active, (setter)TEObj_set_active, "TBD"},
	{"just", (getter)TEObj_get_just, (setter)TEObj_set_just, "Justification"},
	{"teLength", (getter)TEObj_get_teLength, (setter)TEObj_set_teLength, "TBD"},
	{"txFont", (getter)TEObj_get_txFont, (setter)TEObj_set_txFont, "Current font"},
	{"txFace", (getter)TEObj_get_txFace, (setter)TEObj_set_txFace, "Current font variant"},
	{"txMode", (getter)TEObj_get_txMode, (setter)TEObj_set_txMode, "Current text-drawing mode"},
	{"txSize", (getter)TEObj_get_txSize, (setter)TEObj_set_txSize, "Current font size"},
	{"nLines", (getter)TEObj_get_nLines, (setter)TEObj_set_nLines, "TBD"},
	{NULL, NULL, NULL, NULL},
};


#define TEObj_compare NULL

#define TEObj_repr NULL

#define TEObj_hash NULL
#define TEObj_tp_init 0

#define TEObj_tp_alloc PyType_GenericAlloc

static PyObject *TEObj_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *self;
	TEHandle itself;
	char *kw[] = {"itself", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&", kw, TEObj_Convert, &itself)) return NULL;
	if ((self = type->tp_alloc(type, 0)) == NULL) return NULL;
	((TEObject *)self)->ob_itself = itself;
	return self;
}

#define TEObj_tp_free PyObject_Del


PyTypeObject TE_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"_TE.TE", /*tp_name*/
	sizeof(TEObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) TEObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc)0, /*tp_getattr*/
	(setattrfunc)0, /*tp_setattr*/
	(cmpfunc) TEObj_compare, /*tp_compare*/
	(reprfunc) TEObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) TEObj_hash, /*tp_hash*/
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
	TEObj_methods, /* tp_methods */
	0, /*tp_members*/
	TEObj_getsetlist, /*tp_getset*/
	0, /*tp_base*/
	0, /*tp_dict*/
	0, /*tp_descr_get*/
	0, /*tp_descr_set*/
	0, /*tp_dictoffset*/
	TEObj_tp_init, /* tp_init */
	TEObj_tp_alloc, /* tp_alloc */
	TEObj_tp_new, /* tp_new */
	TEObj_tp_free, /* tp_free */
};

/* ----------------------- End object type TE ----------------------- */


static PyObject *TE_TEScrapHandle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Handle _rv;
#ifndef TEScrapHandle
	PyMac_PRECHECK(TEScrapHandle);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = TEScrapHandle();
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *TE_TEGetScrapLength(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
#ifndef TEGetScrapLength
	PyMac_PRECHECK(TEGetScrapLength);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = TEGetScrapLength();
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *TE_TENew(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TEHandle _rv;
	Rect destRect;
	Rect viewRect;
#ifndef TENew
	PyMac_PRECHECK(TENew);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetRect, &destRect,
	                      PyMac_GetRect, &viewRect))
		return NULL;
	_rv = TENew(&destRect,
	            &viewRect);
	_res = Py_BuildValue("O&",
	                     TEObj_New, _rv);
	return _res;
}

static PyObject *TE_TETextBox(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	char *text__in__;
	long text__len__;
	int text__in_len__;
	Rect box;
	short just;
#ifndef TETextBox
	PyMac_PRECHECK(TETextBox);
#endif
	if (!PyArg_ParseTuple(_args, "s#O&h",
	                      &text__in__, &text__in_len__,
	                      PyMac_GetRect, &box,
	                      &just))
		return NULL;
	text__len__ = text__in_len__;
	TETextBox(text__in__, text__len__,
	          &box,
	          just);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TE_TEStyleNew(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TEHandle _rv;
	Rect destRect;
	Rect viewRect;
#ifndef TEStyleNew
	PyMac_PRECHECK(TEStyleNew);
#endif
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetRect, &destRect,
	                      PyMac_GetRect, &viewRect))
		return NULL;
	_rv = TEStyleNew(&destRect,
	                 &viewRect);
	_res = Py_BuildValue("O&",
	                     TEObj_New, _rv);
	return _res;
}

static PyObject *TE_TESetScrapLength(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long length;
#ifndef TESetScrapLength
	PyMac_PRECHECK(TESetScrapLength);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &length))
		return NULL;
	TESetScrapLength(length);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TE_TEFromScrap(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
#ifndef TEFromScrap
	PyMac_PRECHECK(TEFromScrap);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = TEFromScrap();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TE_TEToScrap(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
#ifndef TEToScrap
	PyMac_PRECHECK(TEToScrap);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = TEToScrap();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TE_TEGetScrapHandle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Handle _rv;
#ifndef TEGetScrapHandle
	PyMac_PRECHECK(TEGetScrapHandle);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = TEGetScrapHandle();
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *TE_TESetScrapHandle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Handle value;
#ifndef TESetScrapHandle
	PyMac_PRECHECK(TESetScrapHandle);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &value))
		return NULL;
	TESetScrapHandle(value);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TE_LMGetWordRedraw(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	UInt8 _rv;
#ifndef LMGetWordRedraw
	PyMac_PRECHECK(LMGetWordRedraw);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = LMGetWordRedraw();
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *TE_LMSetWordRedraw(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	UInt8 value;
#ifndef LMSetWordRedraw
	PyMac_PRECHECK(LMSetWordRedraw);
#endif
	if (!PyArg_ParseTuple(_args, "b",
	                      &value))
		return NULL;
	LMSetWordRedraw(value);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TE_as_TE(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TEHandle _rv;
	Handle h;
#ifndef as_TE
	PyMac_PRECHECK(as_TE);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &h))
		return NULL;
	_rv = as_TE(h);
	_res = Py_BuildValue("O&",
	                     TEObj_New, _rv);
	return _res;
}

static PyMethodDef TE_methods[] = {
	{"TEScrapHandle", (PyCFunction)TE_TEScrapHandle, 1,
	 PyDoc_STR("() -> (Handle _rv)")},
	{"TEGetScrapLength", (PyCFunction)TE_TEGetScrapLength, 1,
	 PyDoc_STR("() -> (long _rv)")},
	{"TENew", (PyCFunction)TE_TENew, 1,
	 PyDoc_STR("(Rect destRect, Rect viewRect) -> (TEHandle _rv)")},
	{"TETextBox", (PyCFunction)TE_TETextBox, 1,
	 PyDoc_STR("(Buffer text, Rect box, short just) -> None")},
	{"TEStyleNew", (PyCFunction)TE_TEStyleNew, 1,
	 PyDoc_STR("(Rect destRect, Rect viewRect) -> (TEHandle _rv)")},
	{"TESetScrapLength", (PyCFunction)TE_TESetScrapLength, 1,
	 PyDoc_STR("(long length) -> None")},
	{"TEFromScrap", (PyCFunction)TE_TEFromScrap, 1,
	 PyDoc_STR("() -> None")},
	{"TEToScrap", (PyCFunction)TE_TEToScrap, 1,
	 PyDoc_STR("() -> None")},
	{"TEGetScrapHandle", (PyCFunction)TE_TEGetScrapHandle, 1,
	 PyDoc_STR("() -> (Handle _rv)")},
	{"TESetScrapHandle", (PyCFunction)TE_TESetScrapHandle, 1,
	 PyDoc_STR("(Handle value) -> None")},
	{"LMGetWordRedraw", (PyCFunction)TE_LMGetWordRedraw, 1,
	 PyDoc_STR("() -> (UInt8 _rv)")},
	{"LMSetWordRedraw", (PyCFunction)TE_LMSetWordRedraw, 1,
	 PyDoc_STR("(UInt8 value) -> None")},
	{"as_TE", (PyCFunction)TE_as_TE, 1,
	 PyDoc_STR("(Handle h) -> (TEHandle _rv)")},
	{NULL, NULL, 0}
};




void init_TE(void)
{
	PyObject *m;
	PyObject *d;



		PyMac_INIT_TOOLBOX_OBJECT_NEW(TEHandle, TEObj_New);
		PyMac_INIT_TOOLBOX_OBJECT_CONVERT(TEHandle, TEObj_Convert);


	m = Py_InitModule("_TE", TE_methods);
	d = PyModule_GetDict(m);
	TE_Error = PyMac_GetOSErrException();
	if (TE_Error == NULL ||
	    PyDict_SetItemString(d, "Error", TE_Error) != 0)
		return;
	TE_Type.ob_type = &PyType_Type;
	if (PyType_Ready(&TE_Type) < 0) return;
	Py_INCREF(&TE_Type);
	PyModule_AddObject(m, "TE", (PyObject *)&TE_Type);
	/* Backward-compatible name */
	Py_INCREF(&TE_Type);
	PyModule_AddObject(m, "TEType", (PyObject *)&TE_Type);
}

/* ========================= End module _TE ========================= */

