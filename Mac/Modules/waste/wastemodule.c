
/* ========================== Module waste ========================== */

#include "Python.h"



#include "macglue.h"
#include "pymactoolbox.h"

#include <WASTE.h>
#include <WEObjectHandlers.h>
#include <WETabs.h>

/* Exported by Qdmodule.c: */
extern PyObject *QdRGB_New(RGBColor *);
extern int QdRGB_Convert(PyObject *, RGBColor *);

/* Exported by AEModule.c: */
extern PyObject *AEDesc_New(AppleEvent *);
extern int AEDesc_Convert(PyObject *, AppleEvent *);

/* Forward declaration */
staticforward PyObject *WEOObj_New(WEObjectReference);
staticforward PyObject *ExistingwasteObj_New(WEReference);

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

/* Stuff for the callbacks: */
static PyObject *callbackdict;
WENewObjectUPP upp_new_handler;
WEDisposeObjectUPP upp_dispose_handler;
WEDrawObjectUPP upp_draw_handler;
WEClickObjectUPP upp_click_handler;

static OSErr
any_handler(WESelector what, WEObjectReference who, PyObject *args, PyObject **rv)
{
	FlavorType tp;
	PyObject *key, *func;
	
	if ( args == NULL ) return errAECorruptData;
	
	tp = WEGetObjectType(who);
	
	if( (key=Py_BuildValue("O&O&", PyMac_BuildOSType, tp, PyMac_BuildOSType, what)) == NULL)
		return errAECorruptData;
	if( (func = PyDict_GetItem(callbackdict, key)) == NULL ) {
		Py_DECREF(key);
		return errAEHandlerNotFound;
	}
	Py_INCREF(func);
	*rv = PyEval_CallObject(func, args);
	Py_DECREF(func);
	Py_DECREF(key);
	if ( *rv == NULL ) {
		PySys_WriteStderr("--Exception in callback: ");
		PyErr_Print();
		return errAEReplyNotArrived;
	}
	return 0;
}

static pascal OSErr
my_new_handler(Point *objectSize, WEObjectReference objref)
{
	PyObject *args=NULL, *rv=NULL;
	OSErr err;
	
	args=Py_BuildValue("(O&)", WEOObj_New, objref);
	err = any_handler(weNewHandler, objref, args, &rv);
	if (!err) {
		if (!PyMac_GetPoint(rv, objectSize) )
			err = errAECoercionFail;
	}
	if ( args ) Py_DECREF(args);
	if ( rv ) Py_DECREF(rv);
	return err;
}

static pascal OSErr
my_dispose_handler(WEObjectReference objref)
{
	PyObject *args=NULL, *rv=NULL;
	OSErr err;
	
	args=Py_BuildValue("(O&)", WEOObj_New, objref);
	err = any_handler(weDisposeHandler, objref, args, &rv);
	if ( args ) Py_DECREF(args);
	if ( rv ) Py_DECREF(rv);
	return err;
}

static pascal OSErr
my_draw_handler(const Rect *destRect, WEObjectReference objref)
{
	PyObject *args=NULL, *rv=NULL;
	OSErr err;
	
	args=Py_BuildValue("O&O&", PyMac_BuildRect, destRect, WEOObj_New, objref);
	err = any_handler(weDrawHandler, objref, args, &rv);
	if ( args ) Py_DECREF(args);
	if ( rv ) Py_DECREF(rv);
	return err;
}

static pascal Boolean
my_click_handler(Point hitPt, EventModifiers modifiers,
		unsigned long clickTime, WEObjectReference objref)
{
	PyObject *args=NULL, *rv=NULL;
	int retvalue;
	OSErr err;
	
	args=Py_BuildValue("O&llO&", PyMac_BuildPoint, hitPt,
			(long)modifiers, (long)clickTime, WEOObj_New, objref);
	err = any_handler(weClickHandler, objref, args, &rv);
	if (!err)
		retvalue = PyInt_AsLong(rv);
	else
		retvalue = 0;
	if ( args ) Py_DECREF(args);
	if ( rv ) Py_DECREF(rv);
	return retvalue;
}
		


static PyObject *waste_Error;

/* ------------------------ Object type WEO ------------------------- */

PyTypeObject WEO_Type;

#define WEOObj_Check(x) ((x)->ob_type == &WEO_Type)

typedef struct WEOObject {
	PyObject_HEAD
	WEObjectReference ob_itself;
} WEOObject;

PyObject *WEOObj_New(WEObjectReference itself)
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
WEOObj_Convert(PyObject *v, WEObjectReference *p_itself)
{
	if (!WEOObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "WEO required");
		return 0;
	}
	*p_itself = ((WEOObject *)v)->ob_itself;
	return 1;
}

static void WEOObj_dealloc(WEOObject *self)
{
	/* Cleanup of self->ob_itself goes here */
	PyMem_DEL(self);
}

static PyObject *WEOObj_WEGetObjectType(WEOObject *_self, PyObject *_args)
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

static PyObject *WEOObj_WEGetObjectDataHandle(WEOObject *_self, PyObject *_args)
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

static PyObject *WEOObj_WEGetObjectSize(WEOObject *_self, PyObject *_args)
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

static PyObject *WEOObj_WEGetObjectOwner(WEOObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	WEReference _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WEGetObjectOwner(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ExistingwasteObj_New, _rv);
	return _res;
}

static PyObject *WEOObj_WEGetObjectRefCon(WEOObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WEGetObjectRefCon(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *WEOObj_WESetObjectRefCon(WEOObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 refCon;
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
	{"WEGetObjectOwner", (PyCFunction)WEOObj_WEGetObjectOwner, 1,
	 "() -> (WEReference _rv)"},
	{"WEGetObjectRefCon", (PyCFunction)WEOObj_WEGetObjectRefCon, 1,
	 "() -> (SInt32 _rv)"},
	{"WESetObjectRefCon", (PyCFunction)WEOObj_WESetObjectRefCon, 1,
	 "(SInt32 refCon) -> None"},
	{NULL, NULL, 0}
};

PyMethodChain WEOObj_chain = { WEOObj_methods, NULL };

static PyObject *WEOObj_getattr(WEOObject *self, char *name)
{
	return Py_FindMethodInChain(&WEOObj_chain, (PyObject *)self, name);
}

#define WEOObj_setattr NULL

#define WEOObj_compare NULL

#define WEOObj_repr NULL

#define WEOObj_hash NULL

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
	(cmpfunc) WEOObj_compare, /*tp_compare*/
	(reprfunc) WEOObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) WEOObj_hash, /*tp_hash*/
};

/* ---------------------- End object type WEO ----------------------- */


/* ----------------------- Object type waste ------------------------ */

PyTypeObject waste_Type;

#define wasteObj_Check(x) ((x)->ob_type == &waste_Type)

typedef struct wasteObject {
	PyObject_HEAD
	WEReference ob_itself;
} wasteObject;

PyObject *wasteObj_New(WEReference itself)
{
	wasteObject *it;
	if (itself == NULL) {
						PyErr_SetString(waste_Error,"Cannot create null WE");
						return NULL;
					}
	it = PyObject_NEW(wasteObject, &waste_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	WESetInfo(weRefCon, (void *)&it, itself);
	return (PyObject *)it;
}
wasteObj_Convert(PyObject *v, WEReference *p_itself)
{
	if (!wasteObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "waste required");
		return 0;
	}
	*p_itself = ((wasteObject *)v)->ob_itself;
	return 1;
}

static void wasteObj_dealloc(wasteObject *self)
{
	WEDispose(self->ob_itself);
	PyMem_DEL(self);
}

static PyObject *wasteObj_WEGetText(wasteObject *_self, PyObject *_args)
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

static PyObject *wasteObj_WEGetChar(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt16 _rv;
	SInt32 offset;
	if (!PyArg_ParseTuple(_args, "l",
	                      &offset))
		return NULL;
	_rv = WEGetChar(offset,
	                _self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WEGetTextLength(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WEGetTextLength(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WEGetHeight(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 _rv;
	SInt32 startLine;
	SInt32 endLine;
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

static PyObject *wasteObj_WEGetSelection(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 selStart;
	SInt32 selEnd;
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

static PyObject *wasteObj_WEGetDestRect(wasteObject *_self, PyObject *_args)
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

static PyObject *wasteObj_WEGetViewRect(wasteObject *_self, PyObject *_args)
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

static PyObject *wasteObj_WEIsActive(wasteObject *_self, PyObject *_args)
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

static PyObject *wasteObj_WEOffsetToLine(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 _rv;
	SInt32 offset;
	if (!PyArg_ParseTuple(_args, "l",
	                      &offset))
		return NULL;
	_rv = WEOffsetToLine(offset,
	                     _self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WEGetLineRange(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 lineIndex;
	SInt32 lineStart;
	SInt32 lineEnd;
	if (!PyArg_ParseTuple(_args, "l",
	                      &lineIndex))
		return NULL;
	WEGetLineRange(lineIndex,
	               &lineStart,
	               &lineEnd,
	               _self->ob_itself);
	_res = Py_BuildValue("ll",
	                     lineStart,
	                     lineEnd);
	return _res;
}

static PyObject *wasteObj_WECountLines(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WECountLines(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WEOffsetToRun(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 _rv;
	SInt32 offset;
	if (!PyArg_ParseTuple(_args, "l",
	                      &offset))
		return NULL;
	_rv = WEOffsetToRun(offset,
	                    _self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WEGetRunRange(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 runIndex;
	SInt32 runStart;
	SInt32 runEnd;
	if (!PyArg_ParseTuple(_args, "l",
	                      &runIndex))
		return NULL;
	WEGetRunRange(runIndex,
	              &runStart,
	              &runEnd,
	              _self->ob_itself);
	_res = Py_BuildValue("ll",
	                     runStart,
	                     runEnd);
	return _res;
}

static PyObject *wasteObj_WECountRuns(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WECountRuns(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WEGetClickCount(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	UInt16 _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WEGetClickCount(_self->ob_itself);
	_res = Py_BuildValue("H",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WESetSelection(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 selStart;
	SInt32 selEnd;
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

static PyObject *wasteObj_WESetDestRect(wasteObject *_self, PyObject *_args)
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

static PyObject *wasteObj_WESetViewRect(wasteObject *_self, PyObject *_args)
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

static PyObject *wasteObj_WEContinuousStyle(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	WEStyleMode mode;
	TextStyle ts;
	if (!PyArg_ParseTuple(_args, "H",
	                      &mode))
		return NULL;
	_rv = WEContinuousStyle(&mode,
	                        &ts,
	                        _self->ob_itself);
	_res = Py_BuildValue("bHO&",
	                     _rv,
	                     mode,
	                     TextStyle_New, &ts);
	return _res;
}

static PyObject *wasteObj_WEGetRunInfo(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 offset;
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

static PyObject *wasteObj_WEGetRunDirection(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	SInt32 offset;
	if (!PyArg_ParseTuple(_args, "l",
	                      &offset))
		return NULL;
	_rv = WEGetRunDirection(offset,
	                        _self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WEGetOffset(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 _rv;
	LongPt thePoint;
	WEEdge edge;
	if (!PyArg_ParseTuple(_args, "O&",
	                      LongPt_Convert, &thePoint))
		return NULL;
	_rv = WEGetOffset(&thePoint,
	                  &edge,
	                  _self->ob_itself);
	_res = Py_BuildValue("lB",
	                     _rv,
	                     edge);
	return _res;
}

static PyObject *wasteObj_WEGetPoint(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 offset;
	SInt16 direction;
	LongPt thePoint;
	SInt16 lineHeight;
	if (!PyArg_ParseTuple(_args, "lh",
	                      &offset,
	                      &direction))
		return NULL;
	WEGetPoint(offset,
	           direction,
	           &thePoint,
	           &lineHeight,
	           _self->ob_itself);
	_res = Py_BuildValue("O&h",
	                     LongPt_New, &thePoint,
	                     lineHeight);
	return _res;
}

static PyObject *wasteObj_WEFindWord(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 offset;
	WEEdge edge;
	SInt32 wordStart;
	SInt32 wordEnd;
	if (!PyArg_ParseTuple(_args, "lB",
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

static PyObject *wasteObj_WEFindLine(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 offset;
	WEEdge edge;
	SInt32 lineStart;
	SInt32 lineEnd;
	if (!PyArg_ParseTuple(_args, "lB",
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

static PyObject *wasteObj_WEFindParagraph(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 offset;
	WEEdge edge;
	SInt32 paragraphStart;
	SInt32 paragraphEnd;
	if (!PyArg_ParseTuple(_args, "lB",
	                      &offset,
	                      &edge))
		return NULL;
	WEFindParagraph(offset,
	                edge,
	                &paragraphStart,
	                &paragraphEnd,
	                _self->ob_itself);
	_res = Py_BuildValue("ll",
	                     paragraphStart,
	                     paragraphEnd);
	return _res;
}

static PyObject *wasteObj_WECopyRange(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt32 rangeStart;
	SInt32 rangeEnd;
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

static PyObject *wasteObj_WEGetAlignment(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	WEAlignment _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WEGetAlignment(_self->ob_itself);
	_res = Py_BuildValue("B",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WESetAlignment(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	WEAlignment alignment;
	if (!PyArg_ParseTuple(_args, "B",
	                      &alignment))
		return NULL;
	WESetAlignment(alignment,
	               _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEGetDirection(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	WEDirection _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WEGetDirection(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WESetDirection(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	WEDirection direction;
	if (!PyArg_ParseTuple(_args, "h",
	                      &direction))
		return NULL;
	WESetDirection(direction,
	               _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WECalText(wasteObject *_self, PyObject *_args)
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

static PyObject *wasteObj_WEUpdate(wasteObject *_self, PyObject *_args)
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

static PyObject *wasteObj_WEScroll(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 hOffset;
	SInt32 vOffset;
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

static PyObject *wasteObj_WESelView(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	WESelView(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEActivate(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	WEActivate(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEDeactivate(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	WEDeactivate(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEKey(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt16 key;
	EventModifiers modifiers;
	if (!PyArg_ParseTuple(_args, "hH",
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

static PyObject *wasteObj_WEClick(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Point hitPt;
	EventModifiers modifiers;
	UInt32 clickTime;
	if (!PyArg_ParseTuple(_args, "O&Hl",
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

static PyObject *wasteObj_WEAdjustCursor(wasteObject *_self, PyObject *_args)
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

static PyObject *wasteObj_WEIdle(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	UInt32 maxSleep;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	WEIdle(&maxSleep,
	       _self->ob_itself);
	_res = Py_BuildValue("l",
	                     maxSleep);
	return _res;
}

static PyObject *wasteObj_WEInsert(wasteObject *_self, PyObject *_args)
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

static PyObject *wasteObj_WEDelete(wasteObject *_self, PyObject *_args)
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

static PyObject *wasteObj_WESetStyle(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	WEStyleMode mode;
	TextStyle ts;
	if (!PyArg_ParseTuple(_args, "HO&",
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

static PyObject *wasteObj_WEUseStyleScrap(wasteObject *_self, PyObject *_args)
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

static PyObject *wasteObj_WEUseText(wasteObject *_self, PyObject *_args)
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

static PyObject *wasteObj_WEUndo(wasteObject *_self, PyObject *_args)
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

static PyObject *wasteObj_WEClearUndo(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	WEClearUndo(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEGetUndoInfo(wasteObject *_self, PyObject *_args)
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

static PyObject *wasteObj_WEIsTyping(wasteObject *_self, PyObject *_args)
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

static PyObject *wasteObj_WEBeginAction(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = WEBeginAction(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEEndAction(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	WEActionKind actionKind;
	if (!PyArg_ParseTuple(_args, "h",
	                      &actionKind))
		return NULL;
	_err = WEEndAction(actionKind,
	                   _self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEGetModCount(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	UInt32 _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WEGetModCount(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WEResetModCount(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	WEResetModCount(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEInsertObject(wasteObject *_self, PyObject *_args)
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

static PyObject *wasteObj_WEGetSelectedObject(wasteObject *_self, PyObject *_args)
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

static PyObject *wasteObj_WEFindNextObject(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 _rv;
	SInt32 offset;
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

static PyObject *wasteObj_WEUseSoup(wasteObject *_self, PyObject *_args)
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

static PyObject *wasteObj_WECut(wasteObject *_self, PyObject *_args)
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

static PyObject *wasteObj_WECopy(wasteObject *_self, PyObject *_args)
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

static PyObject *wasteObj_WEPaste(wasteObject *_self, PyObject *_args)
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

static PyObject *wasteObj_WECanPaste(wasteObject *_self, PyObject *_args)
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

static PyObject *wasteObj_WEGetHiliteRgn(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	RgnHandle _rv;
	SInt32 rangeStart;
	SInt32 rangeEnd;
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

static PyObject *wasteObj_WECharByte(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt16 _rv;
	SInt32 offset;
	if (!PyArg_ParseTuple(_args, "l",
	                      &offset))
		return NULL;
	_rv = WECharByte(offset,
	                 _self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WECharType(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt16 _rv;
	SInt32 offset;
	if (!PyArg_ParseTuple(_args, "l",
	                      &offset))
		return NULL;
	_rv = WECharType(offset,
	                 _self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WEStopInlineSession(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	WEStopInlineSession(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEFeatureFlag(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt16 _rv;
	SInt16 feature;
	SInt16 action;
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

static PyObject *wasteObj_WEGetUserInfo(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	WESelector tag;
	SInt32 userInfo;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &tag))
		return NULL;
	_err = WEGetUserInfo(tag,
	                     &userInfo,
	                     _self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     userInfo);
	return _res;
}

static PyObject *wasteObj_WESetUserInfo(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	WESelector tag;
	SInt32 userInfo;
	if (!PyArg_ParseTuple(_args, "O&l",
	                      PyMac_GetOSType, &tag,
	                      &userInfo))
		return NULL;
	_err = WESetUserInfo(tag,
	                     userInfo,
	                     _self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEInstallTabHooks(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = WEInstallTabHooks(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WERemoveTabHooks(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = WERemoveTabHooks(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEIsTabHooks(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WEIsTabHooks(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WEGetTabSize(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt16 _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WEGetTabSize(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WESetTabSize(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 tabWidth;
	if (!PyArg_ParseTuple(_args, "h",
	                      &tabWidth))
		return NULL;
	_err = WESetTabSize(tabWidth,
	                    _self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef wasteObj_methods[] = {
	{"WEGetText", (PyCFunction)wasteObj_WEGetText, 1,
	 "() -> (Handle _rv)"},
	{"WEGetChar", (PyCFunction)wasteObj_WEGetChar, 1,
	 "(SInt32 offset) -> (SInt16 _rv)"},
	{"WEGetTextLength", (PyCFunction)wasteObj_WEGetTextLength, 1,
	 "() -> (SInt32 _rv)"},
	{"WEGetHeight", (PyCFunction)wasteObj_WEGetHeight, 1,
	 "(SInt32 startLine, SInt32 endLine) -> (SInt32 _rv)"},
	{"WEGetSelection", (PyCFunction)wasteObj_WEGetSelection, 1,
	 "() -> (SInt32 selStart, SInt32 selEnd)"},
	{"WEGetDestRect", (PyCFunction)wasteObj_WEGetDestRect, 1,
	 "() -> (LongRect destRect)"},
	{"WEGetViewRect", (PyCFunction)wasteObj_WEGetViewRect, 1,
	 "() -> (LongRect viewRect)"},
	{"WEIsActive", (PyCFunction)wasteObj_WEIsActive, 1,
	 "() -> (Boolean _rv)"},
	{"WEOffsetToLine", (PyCFunction)wasteObj_WEOffsetToLine, 1,
	 "(SInt32 offset) -> (SInt32 _rv)"},
	{"WEGetLineRange", (PyCFunction)wasteObj_WEGetLineRange, 1,
	 "(SInt32 lineIndex) -> (SInt32 lineStart, SInt32 lineEnd)"},
	{"WECountLines", (PyCFunction)wasteObj_WECountLines, 1,
	 "() -> (SInt32 _rv)"},
	{"WEOffsetToRun", (PyCFunction)wasteObj_WEOffsetToRun, 1,
	 "(SInt32 offset) -> (SInt32 _rv)"},
	{"WEGetRunRange", (PyCFunction)wasteObj_WEGetRunRange, 1,
	 "(SInt32 runIndex) -> (SInt32 runStart, SInt32 runEnd)"},
	{"WECountRuns", (PyCFunction)wasteObj_WECountRuns, 1,
	 "() -> (SInt32 _rv)"},
	{"WEGetClickCount", (PyCFunction)wasteObj_WEGetClickCount, 1,
	 "() -> (UInt16 _rv)"},
	{"WESetSelection", (PyCFunction)wasteObj_WESetSelection, 1,
	 "(SInt32 selStart, SInt32 selEnd) -> None"},
	{"WESetDestRect", (PyCFunction)wasteObj_WESetDestRect, 1,
	 "(LongRect destRect) -> None"},
	{"WESetViewRect", (PyCFunction)wasteObj_WESetViewRect, 1,
	 "(LongRect viewRect) -> None"},
	{"WEContinuousStyle", (PyCFunction)wasteObj_WEContinuousStyle, 1,
	 "(WEStyleMode mode) -> (Boolean _rv, WEStyleMode mode, TextStyle ts)"},
	{"WEGetRunInfo", (PyCFunction)wasteObj_WEGetRunInfo, 1,
	 "(SInt32 offset) -> (WERunInfo runInfo)"},
	{"WEGetRunDirection", (PyCFunction)wasteObj_WEGetRunDirection, 1,
	 "(SInt32 offset) -> (Boolean _rv)"},
	{"WEGetOffset", (PyCFunction)wasteObj_WEGetOffset, 1,
	 "(LongPt thePoint) -> (SInt32 _rv, WEEdge edge)"},
	{"WEGetPoint", (PyCFunction)wasteObj_WEGetPoint, 1,
	 "(SInt32 offset, SInt16 direction) -> (LongPt thePoint, SInt16 lineHeight)"},
	{"WEFindWord", (PyCFunction)wasteObj_WEFindWord, 1,
	 "(SInt32 offset, WEEdge edge) -> (SInt32 wordStart, SInt32 wordEnd)"},
	{"WEFindLine", (PyCFunction)wasteObj_WEFindLine, 1,
	 "(SInt32 offset, WEEdge edge) -> (SInt32 lineStart, SInt32 lineEnd)"},
	{"WEFindParagraph", (PyCFunction)wasteObj_WEFindParagraph, 1,
	 "(SInt32 offset, WEEdge edge) -> (SInt32 paragraphStart, SInt32 paragraphEnd)"},
	{"WECopyRange", (PyCFunction)wasteObj_WECopyRange, 1,
	 "(SInt32 rangeStart, SInt32 rangeEnd, Handle hText, StScrpHandle hStyles, WESoupHandle hSoup) -> None"},
	{"WEGetAlignment", (PyCFunction)wasteObj_WEGetAlignment, 1,
	 "() -> (WEAlignment _rv)"},
	{"WESetAlignment", (PyCFunction)wasteObj_WESetAlignment, 1,
	 "(WEAlignment alignment) -> None"},
	{"WEGetDirection", (PyCFunction)wasteObj_WEGetDirection, 1,
	 "() -> (WEDirection _rv)"},
	{"WESetDirection", (PyCFunction)wasteObj_WESetDirection, 1,
	 "(WEDirection direction) -> None"},
	{"WECalText", (PyCFunction)wasteObj_WECalText, 1,
	 "() -> None"},
	{"WEUpdate", (PyCFunction)wasteObj_WEUpdate, 1,
	 "(RgnHandle updateRgn) -> None"},
	{"WEScroll", (PyCFunction)wasteObj_WEScroll, 1,
	 "(SInt32 hOffset, SInt32 vOffset) -> None"},
	{"WESelView", (PyCFunction)wasteObj_WESelView, 1,
	 "() -> None"},
	{"WEActivate", (PyCFunction)wasteObj_WEActivate, 1,
	 "() -> None"},
	{"WEDeactivate", (PyCFunction)wasteObj_WEDeactivate, 1,
	 "() -> None"},
	{"WEKey", (PyCFunction)wasteObj_WEKey, 1,
	 "(SInt16 key, EventModifiers modifiers) -> None"},
	{"WEClick", (PyCFunction)wasteObj_WEClick, 1,
	 "(Point hitPt, EventModifiers modifiers, UInt32 clickTime) -> None"},
	{"WEAdjustCursor", (PyCFunction)wasteObj_WEAdjustCursor, 1,
	 "(Point mouseLoc, RgnHandle mouseRgn) -> (Boolean _rv)"},
	{"WEIdle", (PyCFunction)wasteObj_WEIdle, 1,
	 "() -> (UInt32 maxSleep)"},
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
	{"WEBeginAction", (PyCFunction)wasteObj_WEBeginAction, 1,
	 "() -> None"},
	{"WEEndAction", (PyCFunction)wasteObj_WEEndAction, 1,
	 "(WEActionKind actionKind) -> None"},
	{"WEGetModCount", (PyCFunction)wasteObj_WEGetModCount, 1,
	 "() -> (UInt32 _rv)"},
	{"WEResetModCount", (PyCFunction)wasteObj_WEResetModCount, 1,
	 "() -> None"},
	{"WEInsertObject", (PyCFunction)wasteObj_WEInsertObject, 1,
	 "(FlavorType objectType, Handle objectDataHandle, Point objectSize) -> None"},
	{"WEGetSelectedObject", (PyCFunction)wasteObj_WEGetSelectedObject, 1,
	 "() -> (WEObjectReference obj)"},
	{"WEFindNextObject", (PyCFunction)wasteObj_WEFindNextObject, 1,
	 "(SInt32 offset) -> (SInt32 _rv, WEObjectReference obj)"},
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
	 "(SInt32 rangeStart, SInt32 rangeEnd) -> (RgnHandle _rv)"},
	{"WECharByte", (PyCFunction)wasteObj_WECharByte, 1,
	 "(SInt32 offset) -> (SInt16 _rv)"},
	{"WECharType", (PyCFunction)wasteObj_WECharType, 1,
	 "(SInt32 offset) -> (SInt16 _rv)"},
	{"WEStopInlineSession", (PyCFunction)wasteObj_WEStopInlineSession, 1,
	 "() -> None"},
	{"WEFeatureFlag", (PyCFunction)wasteObj_WEFeatureFlag, 1,
	 "(SInt16 feature, SInt16 action) -> (SInt16 _rv)"},
	{"WEGetUserInfo", (PyCFunction)wasteObj_WEGetUserInfo, 1,
	 "(WESelector tag) -> (SInt32 userInfo)"},
	{"WESetUserInfo", (PyCFunction)wasteObj_WESetUserInfo, 1,
	 "(WESelector tag, SInt32 userInfo) -> None"},
	{"WEInstallTabHooks", (PyCFunction)wasteObj_WEInstallTabHooks, 1,
	 "() -> None"},
	{"WERemoveTabHooks", (PyCFunction)wasteObj_WERemoveTabHooks, 1,
	 "() -> None"},
	{"WEIsTabHooks", (PyCFunction)wasteObj_WEIsTabHooks, 1,
	 "() -> (Boolean _rv)"},
	{"WEGetTabSize", (PyCFunction)wasteObj_WEGetTabSize, 1,
	 "() -> (SInt16 _rv)"},
	{"WESetTabSize", (PyCFunction)wasteObj_WESetTabSize, 1,
	 "(SInt16 tabWidth) -> None"},
	{NULL, NULL, 0}
};

PyMethodChain wasteObj_chain = { wasteObj_methods, NULL };

static PyObject *wasteObj_getattr(wasteObject *self, char *name)
{
	return Py_FindMethodInChain(&wasteObj_chain, (PyObject *)self, name);
}

#define wasteObj_setattr NULL

#define wasteObj_compare NULL

#define wasteObj_repr NULL

#define wasteObj_hash NULL

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
	(cmpfunc) wasteObj_compare, /*tp_compare*/
	(reprfunc) wasteObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) wasteObj_hash, /*tp_hash*/
};

/* --------------------- End object type waste ---------------------- */


static PyObject *waste_WENew(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	LongRect destRect;
	LongRect viewRect;
	UInt32 flags;
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

static PyObject *waste_WEUpdateStyleScrap(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	StScrpHandle hStyles;
	WEFontTableHandle hFontTable;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &hStyles,
	                      ResObj_Convert, &hFontTable))
		return NULL;
	_err = WEUpdateStyleScrap(hStyles,
	                          hFontTable);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *waste_WEInstallTSMHandlers(PyObject *_self, PyObject *_args)
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

static PyObject *waste_WERemoveTSMHandlers(PyObject *_self, PyObject *_args)
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

static PyObject *waste_WEHandleTSMEvent(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AppleEvent ae;
	AppleEvent reply;
	if (!PyArg_ParseTuple(_args, "O&",
	                      AEDesc_Convert, &ae))
		return NULL;
	_err = WEHandleTSMEvent(&ae,
	                        &reply);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &reply);
	return _res;
}

static PyObject *waste_WELongPointToPoint(PyObject *_self, PyObject *_args)
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

static PyObject *waste_WEPointToLongPoint(PyObject *_self, PyObject *_args)
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

static PyObject *waste_WESetLongRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	LongRect lr;
	SInt32 left;
	SInt32 top;
	SInt32 right;
	SInt32 bottom;
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

static PyObject *waste_WELongRectToRect(PyObject *_self, PyObject *_args)
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

static PyObject *waste_WERectToLongRect(PyObject *_self, PyObject *_args)
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

static PyObject *waste_WEOffsetLongRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	LongRect lr;
	SInt32 hOffset;
	SInt32 vOffset;
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

static PyObject *waste_WELongPointInLongRect(PyObject *_self, PyObject *_args)
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

static PyObject *waste_STDObjectHandlers(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;

		OSErr err;
		// install the sample object handlers for pictures and sounds
#define	kTypePicture			'PICT'
#define	kTypeSound				'snd '
		
		if ( !PyArg_ParseTuple(_args, "") ) return NULL;
		
		if ((err = WEInstallObjectHandler(kTypePicture, weNewHandler,
					(UniversalProcPtr) NewWENewObjectProc(HandleNewPicture), NULL)) != noErr)
			goto cleanup;
		
		if ((err = WEInstallObjectHandler(kTypePicture, weDisposeHandler,
					(UniversalProcPtr) NewWEDisposeObjectProc(HandleDisposePicture), NULL)) != noErr)
			goto cleanup;
		
		if ((err = WEInstallObjectHandler(kTypePicture, weDrawHandler,
					(UniversalProcPtr) NewWEDrawObjectProc(HandleDrawPicture), NULL)) != noErr)
			goto cleanup;
		
		if ((err = WEInstallObjectHandler(kTypeSound, weNewHandler,
					(UniversalProcPtr) NewWENewObjectProc(HandleNewSound), NULL)) != noErr)
			goto cleanup;
		
		if ((err = WEInstallObjectHandler(kTypeSound, weDrawHandler,
					(UniversalProcPtr) NewWEDrawObjectProc(HandleDrawSound), NULL)) != noErr)
			goto cleanup;
		
		if ((err = WEInstallObjectHandler(kTypeSound, weClickHandler,
					(UniversalProcPtr) NewWEClickObjectProc(HandleClickSound), NULL)) != noErr)
			goto cleanup;
		Py_INCREF(Py_None);
		return Py_None;
		
	cleanup:
		return PyMac_Error(err);

}

static PyObject *waste_WEInstallObjectHandler(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;

		OSErr err;
		FlavorType objectType;
		WESelector selector;
		PyObject *py_handler;
		UniversalProcPtr handler;
		WEReference we = NULL;
		PyObject *key;
		
		
		if ( !PyArg_ParseTuple(_args, "O&O&O|O&",
				PyMac_GetOSType, &objectType,
				PyMac_GetOSType, &selector,
				&py_handler,
				WEOObj_Convert, &we) ) return NULL;
				
		if ( selector == weNewHandler ) handler = (UniversalProcPtr)upp_new_handler;
		else if ( selector == weDisposeHandler ) handler = (UniversalProcPtr)upp_dispose_handler;
		else if ( selector == weDrawHandler ) handler = (UniversalProcPtr)upp_draw_handler;
		else if ( selector == weClickHandler ) handler = (UniversalProcPtr)upp_click_handler;
		else return PyMac_Error(weUndefinedSelectorErr);
				
		if ((key = Py_BuildValue("O&O&", 
				PyMac_BuildOSType, objectType, 
				PyMac_BuildOSType, selector)) == NULL )
			return NULL;
			
		PyDict_SetItem(callbackdict, key, py_handler);
		
		err = WEInstallObjectHandler(objectType, selector, handler, we);
		if ( err ) return PyMac_Error(err);
		Py_INCREF(Py_None);
		return Py_None;

}

static PyMethodDef waste_methods[] = {
	{"WENew", (PyCFunction)waste_WENew, 1,
	 "(LongRect destRect, LongRect viewRect, UInt32 flags) -> (WEReference we)"},
	{"WEUpdateStyleScrap", (PyCFunction)waste_WEUpdateStyleScrap, 1,
	 "(StScrpHandle hStyles, WEFontTableHandle hFontTable) -> None"},
	{"WEInstallTSMHandlers", (PyCFunction)waste_WEInstallTSMHandlers, 1,
	 "() -> None"},
	{"WERemoveTSMHandlers", (PyCFunction)waste_WERemoveTSMHandlers, 1,
	 "() -> None"},
	{"WEHandleTSMEvent", (PyCFunction)waste_WEHandleTSMEvent, 1,
	 "(AppleEvent ae) -> (AppleEvent reply)"},
	{"WELongPointToPoint", (PyCFunction)waste_WELongPointToPoint, 1,
	 "(LongPt lp) -> (Point p)"},
	{"WEPointToLongPoint", (PyCFunction)waste_WEPointToLongPoint, 1,
	 "(Point p) -> (LongPt lp)"},
	{"WESetLongRect", (PyCFunction)waste_WESetLongRect, 1,
	 "(SInt32 left, SInt32 top, SInt32 right, SInt32 bottom) -> (LongRect lr)"},
	{"WELongRectToRect", (PyCFunction)waste_WELongRectToRect, 1,
	 "(LongRect lr) -> (Rect r)"},
	{"WERectToLongRect", (PyCFunction)waste_WERectToLongRect, 1,
	 "(Rect r) -> (LongRect lr)"},
	{"WEOffsetLongRect", (PyCFunction)waste_WEOffsetLongRect, 1,
	 "(SInt32 hOffset, SInt32 vOffset) -> (LongRect lr)"},
	{"WELongPointInLongRect", (PyCFunction)waste_WELongPointInLongRect, 1,
	 "(LongPt lp, LongRect lr) -> (Boolean _rv)"},
	{"STDObjectHandlers", (PyCFunction)waste_STDObjectHandlers, 1,
	 NULL},
	{"WEInstallObjectHandler", (PyCFunction)waste_WEInstallObjectHandler, 1,
	 NULL},
	{NULL, NULL, 0}
};



/* Return the object corresponding to the window, or NULL */

PyObject *
ExistingwasteObj_New(w)
	WEReference w;
{
	PyObject *it = NULL;
	
	if (w == NULL)
		it = NULL;
	else
		WEGetInfo(weRefCon, (void *)&it, w);
	if (it == NULL || ((wasteObject *)it)->ob_itself != w)
		it = Py_None;
	Py_INCREF(it);
	return it;
}


void initwaste(void)
{
	PyObject *m;
	PyObject *d;




	m = Py_InitModule("waste", waste_methods);
	d = PyModule_GetDict(m);
	waste_Error = PyMac_GetOSErrException();
	if (waste_Error == NULL ||
	    PyDict_SetItemString(d, "Error", waste_Error) != 0)
		return;
	WEO_Type.ob_type = &PyType_Type;
	Py_INCREF(&WEO_Type);
	if (PyDict_SetItemString(d, "WEOType", (PyObject *)&WEO_Type) != 0)
		Py_FatalError("can't initialize WEOType");
	waste_Type.ob_type = &PyType_Type;
	Py_INCREF(&waste_Type);
	if (PyDict_SetItemString(d, "wasteType", (PyObject *)&waste_Type) != 0)
		Py_FatalError("can't initialize wasteType");

		callbackdict = PyDict_New();
		if (callbackdict == NULL || PyDict_SetItemString(d, "callbacks", callbackdict) != 0)
			return;
		upp_new_handler = NewWENewObjectProc(my_new_handler);
		upp_dispose_handler = NewWEDisposeObjectProc(my_dispose_handler);
		upp_draw_handler = NewWEDrawObjectProc(my_draw_handler);
		upp_click_handler = NewWEClickObjectProc(my_click_handler);


}

/* ======================== End module waste ======================== */

