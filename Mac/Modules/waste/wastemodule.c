
/* ========================== Module waste ========================== */

#include "Python.h"



#include "pymactoolbox.h"

/* Macro to test whether a weak-loaded CFM function exists */
#define PyMac_PRECHECK(rtn) do { if ( &rtn == NULL )  {\
        PyErr_SetString(PyExc_NotImplementedError, \
        "Not available in this shared library/OS version"); \
        return NULL; \
    }} while(0)


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
static PyObject *WEOObj_New(WEObjectReference);
static PyObject *ExistingwasteObj_New(WEReference);

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

/*
** Parse/generate RunInfo records
*/
static PyObject *
RunInfo_New(WERunInfo *itself)
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

int
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
	if ( args ) {
		Py_DECREF(args);
	}
	if ( rv ) {
		Py_DECREF(rv);
	}
	return err;
}

static pascal OSErr
my_dispose_handler(WEObjectReference objref)
{
	PyObject *args=NULL, *rv=NULL;
	OSErr err;
	
	args=Py_BuildValue("(O&)", WEOObj_New, objref);
	err = any_handler(weDisposeHandler, objref, args, &rv);
	if ( args ) {
		Py_DECREF(args);
	}
	if ( rv ) {
		Py_DECREF(rv);
	}
	return err;
}

static pascal OSErr
my_draw_handler(const Rect *destRect, WEObjectReference objref)
{
	PyObject *args=NULL, *rv=NULL;
	OSErr err;
	
	args=Py_BuildValue("O&O&", PyMac_BuildRect, destRect, WEOObj_New, objref);
	err = any_handler(weDrawHandler, objref, args, &rv);
	if ( args ) {
		Py_DECREF(args);
	}
	if ( rv ) {
		Py_DECREF(rv);
	}
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
	if ( args ) {
		Py_DECREF(args);
	}
	if ( rv ) {
		Py_DECREF(rv);
	}
	return retvalue;
}
		


static PyObject *waste_Error;

/* ------------------------ Object type WEO ------------------------- */

PyTypeObject WEO_Type;

#define WEOObj_Check(x) ((x)->ob_type == &WEO_Type || PyObject_TypeCheck((x), &WEO_Type))

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
int WEOObj_Convert(PyObject *v, WEObjectReference *p_itself)
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
	self->ob_type->tp_free((PyObject *)self);
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

static PyObject *WEOObj_WEGetObjectOffset(WEOObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WEGetObjectOffset(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
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

static PyObject *WEOObj_WESetObjectSize(WEOObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Point inObjectSize;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &inObjectSize))
		return NULL;
	_err = WESetObjectSize(_self->ob_itself,
	                       inObjectSize);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *WEOObj_WEGetObjectFrame(WEOObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	LongRect outObjectFrame;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = WEGetObjectFrame(_self->ob_itself,
	                        &outObjectFrame);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     LongRect_New, &outObjectFrame);
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
	SInt32 inRefCon;
	if (!PyArg_ParseTuple(_args, "l",
	                      &inRefCon))
		return NULL;
	WESetObjectRefCon(_self->ob_itself,
	                  inRefCon);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef WEOObj_methods[] = {
	{"WEGetObjectType", (PyCFunction)WEOObj_WEGetObjectType, 1,
	 PyDoc_STR("() -> (FlavorType _rv)")},
	{"WEGetObjectDataHandle", (PyCFunction)WEOObj_WEGetObjectDataHandle, 1,
	 PyDoc_STR("() -> (Handle _rv)")},
	{"WEGetObjectOwner", (PyCFunction)WEOObj_WEGetObjectOwner, 1,
	 PyDoc_STR("() -> (WEReference _rv)")},
	{"WEGetObjectOffset", (PyCFunction)WEOObj_WEGetObjectOffset, 1,
	 PyDoc_STR("() -> (SInt32 _rv)")},
	{"WEGetObjectSize", (PyCFunction)WEOObj_WEGetObjectSize, 1,
	 PyDoc_STR("() -> (Point _rv)")},
	{"WESetObjectSize", (PyCFunction)WEOObj_WESetObjectSize, 1,
	 PyDoc_STR("(Point inObjectSize) -> None")},
	{"WEGetObjectFrame", (PyCFunction)WEOObj_WEGetObjectFrame, 1,
	 PyDoc_STR("() -> (LongRect outObjectFrame)")},
	{"WEGetObjectRefCon", (PyCFunction)WEOObj_WEGetObjectRefCon, 1,
	 PyDoc_STR("() -> (SInt32 _rv)")},
	{"WESetObjectRefCon", (PyCFunction)WEOObj_WESetObjectRefCon, 1,
	 PyDoc_STR("(SInt32 inRefCon) -> None")},
	{NULL, NULL, 0}
};

#define WEOObj_getsetlist NULL


#define WEOObj_compare NULL

#define WEOObj_repr NULL

#define WEOObj_hash NULL
#define WEOObj_tp_init 0

#define WEOObj_tp_alloc PyType_GenericAlloc

static PyObject *WEOObj_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *self;
	WEObjectReference itself;
	char *kw[] = {"itself", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&", kw, WEOObj_Convert, &itself)) return NULL;
	if ((self = type->tp_alloc(type, 0)) == NULL) return NULL;
	((WEOObject *)self)->ob_itself = itself;
	return self;
}

#define WEOObj_tp_free PyObject_Del


PyTypeObject WEO_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"waste.WEO", /*tp_name*/
	sizeof(WEOObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) WEOObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc)0, /*tp_getattr*/
	(setattrfunc)0, /*tp_setattr*/
	(cmpfunc) WEOObj_compare, /*tp_compare*/
	(reprfunc) WEOObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) WEOObj_hash, /*tp_hash*/
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
	WEOObj_methods, /* tp_methods */
	0, /*tp_members*/
	WEOObj_getsetlist, /*tp_getset*/
	0, /*tp_base*/
	0, /*tp_dict*/
	0, /*tp_descr_get*/
	0, /*tp_descr_set*/
	0, /*tp_dictoffset*/
	WEOObj_tp_init, /* tp_init */
	WEOObj_tp_alloc, /* tp_alloc */
	WEOObj_tp_new, /* tp_new */
	WEOObj_tp_free, /* tp_free */
};

/* ---------------------- End object type WEO ----------------------- */


/* ----------------------- Object type waste ------------------------ */

PyTypeObject waste_Type;

#define wasteObj_Check(x) ((x)->ob_type == &waste_Type || PyObject_TypeCheck((x), &waste_Type))

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
int wasteObj_Convert(PyObject *v, WEReference *p_itself)
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
	self->ob_type->tp_free((PyObject *)self);
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
	SInt32 inOffset;
	if (!PyArg_ParseTuple(_args, "l",
	                      &inOffset))
		return NULL;
	_rv = WEGetChar(inOffset,
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

static PyObject *wasteObj_WEGetSelection(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 outSelStart;
	SInt32 outSelEnd;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	WEGetSelection(&outSelStart,
	               &outSelEnd,
	               _self->ob_itself);
	_res = Py_BuildValue("ll",
	                     outSelStart,
	                     outSelEnd);
	return _res;
}

static PyObject *wasteObj_WEGetDestRect(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	LongRect outDestRect;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	WEGetDestRect(&outDestRect,
	              _self->ob_itself);
	_res = Py_BuildValue("O&",
	                     LongRect_New, &outDestRect);
	return _res;
}

static PyObject *wasteObj_WEGetViewRect(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	LongRect outViewRect;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	WEGetViewRect(&outViewRect,
	              _self->ob_itself);
	_res = Py_BuildValue("O&",
	                     LongRect_New, &outViewRect);
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
	SInt32 inSelStart;
	SInt32 inSelEnd;
	if (!PyArg_ParseTuple(_args, "ll",
	                      &inSelStart,
	                      &inSelEnd))
		return NULL;
	WESetSelection(inSelStart,
	               inSelEnd,
	               _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WESetDestRect(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	LongRect inDestRect;
	if (!PyArg_ParseTuple(_args, "O&",
	                      LongRect_Convert, &inDestRect))
		return NULL;
	WESetDestRect(&inDestRect,
	              _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WESetViewRect(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	LongRect inViewRect;
	if (!PyArg_ParseTuple(_args, "O&",
	                      LongRect_Convert, &inViewRect))
		return NULL;
	WESetViewRect(&inViewRect,
	              _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEContinuousStyle(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	WEStyleMode ioMode;
	TextStyle outTextStyle;
	if (!PyArg_ParseTuple(_args, "H",
	                      &ioMode))
		return NULL;
	_rv = WEContinuousStyle(&ioMode,
	                        &outTextStyle,
	                        _self->ob_itself);
	_res = Py_BuildValue("bHO&",
	                     _rv,
	                     ioMode,
	                     TextStyle_New, &outTextStyle);
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

static PyObject *wasteObj_WEOffsetToRun(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 _rv;
	SInt32 inOffset;
	if (!PyArg_ParseTuple(_args, "l",
	                      &inOffset))
		return NULL;
	_rv = WEOffsetToRun(inOffset,
	                    _self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WEGetRunRange(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 inStyleRunIndex;
	SInt32 outStyleRunStart;
	SInt32 outStyleRunEnd;
	if (!PyArg_ParseTuple(_args, "l",
	                      &inStyleRunIndex))
		return NULL;
	WEGetRunRange(inStyleRunIndex,
	              &outStyleRunStart,
	              &outStyleRunEnd,
	              _self->ob_itself);
	_res = Py_BuildValue("ll",
	                     outStyleRunStart,
	                     outStyleRunEnd);
	return _res;
}

static PyObject *wasteObj_WEGetRunInfo(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 inOffset;
	WERunInfo outStyleRunInfo;
	if (!PyArg_ParseTuple(_args, "l",
	                      &inOffset))
		return NULL;
	WEGetRunInfo(inOffset,
	             &outStyleRunInfo,
	             _self->ob_itself);
	_res = Py_BuildValue("O&",
	                     RunInfo_New, &outStyleRunInfo);
	return _res;
}

static PyObject *wasteObj_WEGetIndRunInfo(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 inStyleRunIndex;
	WERunInfo outStyleRunInfo;
	if (!PyArg_ParseTuple(_args, "l",
	                      &inStyleRunIndex))
		return NULL;
	WEGetIndRunInfo(inStyleRunIndex,
	                &outStyleRunInfo,
	                _self->ob_itself);
	_res = Py_BuildValue("O&",
	                     RunInfo_New, &outStyleRunInfo);
	return _res;
}

static PyObject *wasteObj_WEGetRunDirection(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	SInt32 inOffset;
	if (!PyArg_ParseTuple(_args, "l",
	                      &inOffset))
		return NULL;
	_rv = WEGetRunDirection(inOffset,
	                        _self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WECountParaRuns(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WECountParaRuns(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WEOffsetToParaRun(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 _rv;
	SInt32 inOffset;
	if (!PyArg_ParseTuple(_args, "l",
	                      &inOffset))
		return NULL;
	_rv = WEOffsetToParaRun(inOffset,
	                        _self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WEGetParaRunRange(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 inParagraphRunIndex;
	SInt32 outParagraphRunStart;
	SInt32 outParagraphRunEnd;
	if (!PyArg_ParseTuple(_args, "l",
	                      &inParagraphRunIndex))
		return NULL;
	WEGetParaRunRange(inParagraphRunIndex,
	                  &outParagraphRunStart,
	                  &outParagraphRunEnd,
	                  _self->ob_itself);
	_res = Py_BuildValue("ll",
	                     outParagraphRunStart,
	                     outParagraphRunEnd);
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

static PyObject *wasteObj_WEOffsetToLine(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 _rv;
	SInt32 inOffset;
	if (!PyArg_ParseTuple(_args, "l",
	                      &inOffset))
		return NULL;
	_rv = WEOffsetToLine(inOffset,
	                     _self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WEGetLineRange(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 inLineIndex;
	SInt32 outLineStart;
	SInt32 outLineEnd;
	if (!PyArg_ParseTuple(_args, "l",
	                      &inLineIndex))
		return NULL;
	WEGetLineRange(inLineIndex,
	               &outLineStart,
	               &outLineEnd,
	               _self->ob_itself);
	_res = Py_BuildValue("ll",
	                     outLineStart,
	                     outLineEnd);
	return _res;
}

static PyObject *wasteObj_WEGetHeight(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 _rv;
	SInt32 inStartLineIndex;
	SInt32 inEndLineIndex;
	if (!PyArg_ParseTuple(_args, "ll",
	                      &inStartLineIndex,
	                      &inEndLineIndex))
		return NULL;
	_rv = WEGetHeight(inStartLineIndex,
	                  inEndLineIndex,
	                  _self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WEGetOffset(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 _rv;
	LongPt inPoint;
	WEEdge outEdge;
	if (!PyArg_ParseTuple(_args, "O&",
	                      LongPt_Convert, &inPoint))
		return NULL;
	_rv = WEGetOffset(&inPoint,
	                  &outEdge,
	                  _self->ob_itself);
	_res = Py_BuildValue("lB",
	                     _rv,
	                     outEdge);
	return _res;
}

static PyObject *wasteObj_WEGetPoint(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 inOffset;
	SInt16 inDirection;
	LongPt outPoint;
	SInt16 outLineHeight;
	if (!PyArg_ParseTuple(_args, "lh",
	                      &inOffset,
	                      &inDirection))
		return NULL;
	WEGetPoint(inOffset,
	           inDirection,
	           &outPoint,
	           &outLineHeight,
	           _self->ob_itself);
	_res = Py_BuildValue("O&h",
	                     LongPt_New, &outPoint,
	                     outLineHeight);
	return _res;
}

static PyObject *wasteObj_WEFindWord(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 inOffset;
	WEEdge inEdge;
	SInt32 outWordStart;
	SInt32 outWordEnd;
	if (!PyArg_ParseTuple(_args, "lB",
	                      &inOffset,
	                      &inEdge))
		return NULL;
	WEFindWord(inOffset,
	           inEdge,
	           &outWordStart,
	           &outWordEnd,
	           _self->ob_itself);
	_res = Py_BuildValue("ll",
	                     outWordStart,
	                     outWordEnd);
	return _res;
}

static PyObject *wasteObj_WEFindLine(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 inOffset;
	WEEdge inEdge;
	SInt32 outLineStart;
	SInt32 outLineEnd;
	if (!PyArg_ParseTuple(_args, "lB",
	                      &inOffset,
	                      &inEdge))
		return NULL;
	WEFindLine(inOffset,
	           inEdge,
	           &outLineStart,
	           &outLineEnd,
	           _self->ob_itself);
	_res = Py_BuildValue("ll",
	                     outLineStart,
	                     outLineEnd);
	return _res;
}

static PyObject *wasteObj_WEFindParagraph(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 inOffset;
	WEEdge inEdge;
	SInt32 outParagraphStart;
	SInt32 outParagraphEnd;
	if (!PyArg_ParseTuple(_args, "lB",
	                      &inOffset,
	                      &inEdge))
		return NULL;
	WEFindParagraph(inOffset,
	                inEdge,
	                &outParagraphStart,
	                &outParagraphEnd,
	                _self->ob_itself);
	_res = Py_BuildValue("ll",
	                     outParagraphStart,
	                     outParagraphEnd);
	return _res;
}

static PyObject *wasteObj_WEFind(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	char* inKey;
	SInt32 inKeyLength;
	TextEncoding inKeyEncoding;
	OptionBits inMatchOptions;
	SInt32 inRangeStart;
	SInt32 inRangeEnd;
	SInt32 outMatchStart;
	SInt32 outMatchEnd;
	if (!PyArg_ParseTuple(_args, "slllll",
	                      &inKey,
	                      &inKeyLength,
	                      &inKeyEncoding,
	                      &inMatchOptions,
	                      &inRangeStart,
	                      &inRangeEnd))
		return NULL;
	_err = WEFind(inKey,
	              inKeyLength,
	              inKeyEncoding,
	              inMatchOptions,
	              inRangeStart,
	              inRangeEnd,
	              &outMatchStart,
	              &outMatchEnd,
	              _self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("ll",
	                     outMatchStart,
	                     outMatchEnd);
	return _res;
}

static PyObject *wasteObj_WEStreamRange(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt32 inRangeStart;
	SInt32 inRangeEnd;
	FlavorType inRequestedType;
	OptionBits inStreamOptions;
	Handle outData;
	if (!PyArg_ParseTuple(_args, "llO&lO&",
	                      &inRangeStart,
	                      &inRangeEnd,
	                      PyMac_GetOSType, &inRequestedType,
	                      &inStreamOptions,
	                      ResObj_Convert, &outData))
		return NULL;
	_err = WEStreamRange(inRangeStart,
	                     inRangeEnd,
	                     inRequestedType,
	                     inStreamOptions,
	                     outData,
	                     _self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WECopyRange(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt32 inRangeStart;
	SInt32 inRangeEnd;
	Handle outText;
	StScrpHandle outStyles;
	WESoupHandle outSoup;
	if (!PyArg_ParseTuple(_args, "llO&O&O&",
	                      &inRangeStart,
	                      &inRangeEnd,
	                      OptResObj_Convert, &outText,
	                      OptResObj_Convert, &outStyles,
	                      OptResObj_Convert, &outSoup))
		return NULL;
	_err = WECopyRange(inRangeStart,
	                   inRangeEnd,
	                   outText,
	                   outStyles,
	                   outSoup,
	                   _self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEGetTextRangeAsUnicode(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt32 inRangeStart;
	SInt32 inRangeEnd;
	Handle outUnicodeText;
	Handle ioCharFormat;
	Handle ioParaFormat;
	TextEncodingVariant inUnicodeVariant;
	TextEncodingFormat inTransformationFormat;
	OptionBits inGetOptions;
	if (!PyArg_ParseTuple(_args, "llO&O&O&lll",
	                      &inRangeStart,
	                      &inRangeEnd,
	                      ResObj_Convert, &outUnicodeText,
	                      ResObj_Convert, &ioCharFormat,
	                      ResObj_Convert, &ioParaFormat,
	                      &inUnicodeVariant,
	                      &inTransformationFormat,
	                      &inGetOptions))
		return NULL;
	_err = WEGetTextRangeAsUnicode(inRangeStart,
	                               inRangeEnd,
	                               outUnicodeText,
	                               ioCharFormat,
	                               ioParaFormat,
	                               inUnicodeVariant,
	                               inTransformationFormat,
	                               inGetOptions,
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
	WEAlignment inAlignment;
	if (!PyArg_ParseTuple(_args, "B",
	                      &inAlignment))
		return NULL;
	WESetAlignment(inAlignment,
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
	WEDirection inDirection;
	if (!PyArg_ParseTuple(_args, "h",
	                      &inDirection))
		return NULL;
	WESetDirection(inDirection,
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
	RgnHandle inUpdateRgn;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &inUpdateRgn))
		return NULL;
	WEUpdate(inUpdateRgn,
	         _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEScroll(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 inHorizontalOffset;
	SInt32 inVerticalOffset;
	if (!PyArg_ParseTuple(_args, "ll",
	                      &inHorizontalOffset,
	                      &inVerticalOffset))
		return NULL;
	WEScroll(inHorizontalOffset,
	         inVerticalOffset,
	         _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEPinScroll(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 inHorizontalOffset;
	SInt32 inVerticalOffset;
	if (!PyArg_ParseTuple(_args, "ll",
	                      &inHorizontalOffset,
	                      &inVerticalOffset))
		return NULL;
	WEPinScroll(inHorizontalOffset,
	            inVerticalOffset,
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
	CharParameter inKey;
	EventModifiers inModifiers;
	if (!PyArg_ParseTuple(_args, "hH",
	                      &inKey,
	                      &inModifiers))
		return NULL;
	WEKey(inKey,
	      inModifiers,
	      _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEClick(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Point inHitPoint;
	EventModifiers inModifiers;
	UInt32 inClickTime;
	if (!PyArg_ParseTuple(_args, "O&Hl",
	                      PyMac_GetPoint, &inHitPoint,
	                      &inModifiers,
	                      &inClickTime))
		return NULL;
	WEClick(inHitPoint,
	        inModifiers,
	        inClickTime,
	        _self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEAdjustCursor(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	Point inMouseLoc;
	RgnHandle ioMouseRgn;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetPoint, &inMouseLoc,
	                      ResObj_Convert, &ioMouseRgn))
		return NULL;
	_rv = WEAdjustCursor(inMouseLoc,
	                     ioMouseRgn,
	                     _self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WEIdle(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	UInt32 outMaxSleep;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	WEIdle(&outMaxSleep,
	       _self->ob_itself);
	_res = Py_BuildValue("l",
	                     outMaxSleep);
	return _res;
}

static PyObject *wasteObj_WEInsert(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	char *inTextPtr__in__;
	long inTextPtr__len__;
	int inTextPtr__in_len__;
	StScrpHandle inStyles;
	WESoupHandle inSoup;
	if (!PyArg_ParseTuple(_args, "s#O&O&",
	                      &inTextPtr__in__, &inTextPtr__in_len__,
	                      OptResObj_Convert, &inStyles,
	                      OptResObj_Convert, &inSoup))
		return NULL;
	inTextPtr__len__ = inTextPtr__in_len__;
	_err = WEInsert(inTextPtr__in__, inTextPtr__len__,
	                inStyles,
	                inSoup,
	                _self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEInsertFormattedText(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	char *inTextPtr__in__;
	long inTextPtr__len__;
	int inTextPtr__in_len__;
	StScrpHandle inStyles;
	WESoupHandle inSoup;
	Handle inParaFormat;
	Handle inRulerScrap;
	if (!PyArg_ParseTuple(_args, "s#O&O&O&O&",
	                      &inTextPtr__in__, &inTextPtr__in_len__,
	                      OptResObj_Convert, &inStyles,
	                      OptResObj_Convert, &inSoup,
	                      ResObj_Convert, &inParaFormat,
	                      ResObj_Convert, &inRulerScrap))
		return NULL;
	inTextPtr__len__ = inTextPtr__in_len__;
	_err = WEInsertFormattedText(inTextPtr__in__, inTextPtr__len__,
	                             inStyles,
	                             inSoup,
	                             inParaFormat,
	                             inRulerScrap,
	                             _self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
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

static PyObject *wasteObj_WEUseText(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Handle inText;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &inText))
		return NULL;
	_err = WEUseText(inText,
	                 _self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WEChangeCase(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 inCase;
	if (!PyArg_ParseTuple(_args, "h",
	                      &inCase))
		return NULL;
	_err = WEChangeCase(inCase,
	                    _self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WESetOneAttribute(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt32 inRangeStart;
	SInt32 inRangeEnd;
	WESelector inAttributeSelector;
	char *inAttributeValue__in__;
	long inAttributeValue__len__;
	int inAttributeValue__in_len__;
	if (!PyArg_ParseTuple(_args, "llO&s#",
	                      &inRangeStart,
	                      &inRangeEnd,
	                      PyMac_GetOSType, &inAttributeSelector,
	                      &inAttributeValue__in__, &inAttributeValue__in_len__))
		return NULL;
	inAttributeValue__len__ = inAttributeValue__in_len__;
	_err = WESetOneAttribute(inRangeStart,
	                         inRangeEnd,
	                         inAttributeSelector,
	                         inAttributeValue__in__, inAttributeValue__len__,
	                         _self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WESetStyle(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	WEStyleMode inMode;
	TextStyle inTextStyle;
	if (!PyArg_ParseTuple(_args, "HO&",
	                      &inMode,
	                      TextStyle_Convert, &inTextStyle))
		return NULL;
	_err = WESetStyle(inMode,
	                  &inTextStyle,
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
	StScrpHandle inStyles;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &inStyles))
		return NULL;
	_err = WEUseStyleScrap(inStyles,
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

static PyObject *wasteObj_WERedo(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = WERedo(_self->ob_itself);
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
	Boolean outRedoFlag;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = WEGetUndoInfo(&outRedoFlag,
	                    _self->ob_itself);
	_res = Py_BuildValue("hb",
	                     _rv,
	                     outRedoFlag);
	return _res;
}

static PyObject *wasteObj_WEGetIndUndoInfo(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	WEActionKind _rv;
	SInt32 inUndoLevel;
	if (!PyArg_ParseTuple(_args, "l",
	                      &inUndoLevel))
		return NULL;
	_rv = WEGetIndUndoInfo(inUndoLevel,
	                       _self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
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
	WEActionKind inActionKind;
	if (!PyArg_ParseTuple(_args, "h",
	                      &inActionKind))
		return NULL;
	_err = WEEndAction(inActionKind,
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
	FlavorType inObjectType;
	Handle inObjectDataHandle;
	Point inObjectSize;
	if (!PyArg_ParseTuple(_args, "O&O&O&",
	                      PyMac_GetOSType, &inObjectType,
	                      ResObj_Convert, &inObjectDataHandle,
	                      PyMac_GetPoint, &inObjectSize))
		return NULL;
	_err = WEInsertObject(inObjectType,
	                      inObjectDataHandle,
	                      inObjectSize,
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
	WEObjectReference outObject;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = WEGetSelectedObject(&outObject,
	                           _self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     WEOObj_New, outObject);
	return _res;
}

static PyObject *wasteObj_WEGetObjectAtOffset(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt32 inOffset;
	WEObjectReference outObject;
	if (!PyArg_ParseTuple(_args, "l",
	                      &inOffset))
		return NULL;
	_err = WEGetObjectAtOffset(inOffset,
	                           &outObject,
	                           _self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     WEOObj_New, outObject);
	return _res;
}

static PyObject *wasteObj_WEFindNextObject(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 _rv;
	SInt32 inOffset;
	WEObjectReference outObject;
	if (!PyArg_ParseTuple(_args, "l",
	                      &inOffset))
		return NULL;
	_rv = WEFindNextObject(inOffset,
	                       &outObject,
	                       _self->ob_itself);
	_res = Py_BuildValue("lO&",
	                     _rv,
	                     WEOObj_New, outObject);
	return _res;
}

static PyObject *wasteObj_WEUseSoup(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	WESoupHandle inSoup;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &inSoup))
		return NULL;
	_err = WEUseSoup(inSoup,
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
	SInt32 inRangeStart;
	SInt32 inRangeEnd;
	if (!PyArg_ParseTuple(_args, "ll",
	                      &inRangeStart,
	                      &inRangeEnd))
		return NULL;
	_rv = WEGetHiliteRgn(inRangeStart,
	                     inRangeEnd,
	                     _self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *wasteObj_WECharByte(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt16 _rv;
	SInt32 inOffset;
	if (!PyArg_ParseTuple(_args, "l",
	                      &inOffset))
		return NULL;
	_rv = WECharByte(inOffset,
	                 _self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WECharType(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt16 _rv;
	SInt32 inOffset;
	if (!PyArg_ParseTuple(_args, "l",
	                      &inOffset))
		return NULL;
	_rv = WECharType(inOffset,
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
	SInt16 inFeature;
	SInt16 inAction;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &inFeature,
	                      &inAction))
		return NULL;
	_rv = WEFeatureFlag(inFeature,
	                    inAction,
	                    _self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *wasteObj_WEGetUserInfo(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	WESelector inUserTag;
	SInt32 outUserInfo;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &inUserTag))
		return NULL;
	_err = WEGetUserInfo(inUserTag,
	                     &outUserInfo,
	                     _self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outUserInfo);
	return _res;
}

static PyObject *wasteObj_WESetUserInfo(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	WESelector inUserTag;
	SInt32 inUserInfo;
	if (!PyArg_ParseTuple(_args, "O&l",
	                      PyMac_GetOSType, &inUserTag,
	                      &inUserInfo))
		return NULL;
	_err = WESetUserInfo(inUserTag,
	                     inUserInfo,
	                     _self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *wasteObj_WERemoveUserInfo(wasteObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	WESelector inUserTag;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &inUserTag))
		return NULL;
	_err = WERemoveUserInfo(inUserTag,
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
	 PyDoc_STR("() -> (Handle _rv)")},
	{"WEGetChar", (PyCFunction)wasteObj_WEGetChar, 1,
	 PyDoc_STR("(SInt32 inOffset) -> (SInt16 _rv)")},
	{"WEGetTextLength", (PyCFunction)wasteObj_WEGetTextLength, 1,
	 PyDoc_STR("() -> (SInt32 _rv)")},
	{"WEGetSelection", (PyCFunction)wasteObj_WEGetSelection, 1,
	 PyDoc_STR("() -> (SInt32 outSelStart, SInt32 outSelEnd)")},
	{"WEGetDestRect", (PyCFunction)wasteObj_WEGetDestRect, 1,
	 PyDoc_STR("() -> (LongRect outDestRect)")},
	{"WEGetViewRect", (PyCFunction)wasteObj_WEGetViewRect, 1,
	 PyDoc_STR("() -> (LongRect outViewRect)")},
	{"WEIsActive", (PyCFunction)wasteObj_WEIsActive, 1,
	 PyDoc_STR("() -> (Boolean _rv)")},
	{"WEGetClickCount", (PyCFunction)wasteObj_WEGetClickCount, 1,
	 PyDoc_STR("() -> (UInt16 _rv)")},
	{"WESetSelection", (PyCFunction)wasteObj_WESetSelection, 1,
	 PyDoc_STR("(SInt32 inSelStart, SInt32 inSelEnd) -> None")},
	{"WESetDestRect", (PyCFunction)wasteObj_WESetDestRect, 1,
	 PyDoc_STR("(LongRect inDestRect) -> None")},
	{"WESetViewRect", (PyCFunction)wasteObj_WESetViewRect, 1,
	 PyDoc_STR("(LongRect inViewRect) -> None")},
	{"WEContinuousStyle", (PyCFunction)wasteObj_WEContinuousStyle, 1,
	 PyDoc_STR("(WEStyleMode ioMode) -> (Boolean _rv, WEStyleMode ioMode, TextStyle outTextStyle)")},
	{"WECountRuns", (PyCFunction)wasteObj_WECountRuns, 1,
	 PyDoc_STR("() -> (SInt32 _rv)")},
	{"WEOffsetToRun", (PyCFunction)wasteObj_WEOffsetToRun, 1,
	 PyDoc_STR("(SInt32 inOffset) -> (SInt32 _rv)")},
	{"WEGetRunRange", (PyCFunction)wasteObj_WEGetRunRange, 1,
	 PyDoc_STR("(SInt32 inStyleRunIndex) -> (SInt32 outStyleRunStart, SInt32 outStyleRunEnd)")},
	{"WEGetRunInfo", (PyCFunction)wasteObj_WEGetRunInfo, 1,
	 PyDoc_STR("(SInt32 inOffset) -> (WERunInfo outStyleRunInfo)")},
	{"WEGetIndRunInfo", (PyCFunction)wasteObj_WEGetIndRunInfo, 1,
	 PyDoc_STR("(SInt32 inStyleRunIndex) -> (WERunInfo outStyleRunInfo)")},
	{"WEGetRunDirection", (PyCFunction)wasteObj_WEGetRunDirection, 1,
	 PyDoc_STR("(SInt32 inOffset) -> (Boolean _rv)")},
	{"WECountParaRuns", (PyCFunction)wasteObj_WECountParaRuns, 1,
	 PyDoc_STR("() -> (SInt32 _rv)")},
	{"WEOffsetToParaRun", (PyCFunction)wasteObj_WEOffsetToParaRun, 1,
	 PyDoc_STR("(SInt32 inOffset) -> (SInt32 _rv)")},
	{"WEGetParaRunRange", (PyCFunction)wasteObj_WEGetParaRunRange, 1,
	 PyDoc_STR("(SInt32 inParagraphRunIndex) -> (SInt32 outParagraphRunStart, SInt32 outParagraphRunEnd)")},
	{"WECountLines", (PyCFunction)wasteObj_WECountLines, 1,
	 PyDoc_STR("() -> (SInt32 _rv)")},
	{"WEOffsetToLine", (PyCFunction)wasteObj_WEOffsetToLine, 1,
	 PyDoc_STR("(SInt32 inOffset) -> (SInt32 _rv)")},
	{"WEGetLineRange", (PyCFunction)wasteObj_WEGetLineRange, 1,
	 PyDoc_STR("(SInt32 inLineIndex) -> (SInt32 outLineStart, SInt32 outLineEnd)")},
	{"WEGetHeight", (PyCFunction)wasteObj_WEGetHeight, 1,
	 PyDoc_STR("(SInt32 inStartLineIndex, SInt32 inEndLineIndex) -> (SInt32 _rv)")},
	{"WEGetOffset", (PyCFunction)wasteObj_WEGetOffset, 1,
	 PyDoc_STR("(LongPt inPoint) -> (SInt32 _rv, WEEdge outEdge)")},
	{"WEGetPoint", (PyCFunction)wasteObj_WEGetPoint, 1,
	 PyDoc_STR("(SInt32 inOffset, SInt16 inDirection) -> (LongPt outPoint, SInt16 outLineHeight)")},
	{"WEFindWord", (PyCFunction)wasteObj_WEFindWord, 1,
	 PyDoc_STR("(SInt32 inOffset, WEEdge inEdge) -> (SInt32 outWordStart, SInt32 outWordEnd)")},
	{"WEFindLine", (PyCFunction)wasteObj_WEFindLine, 1,
	 PyDoc_STR("(SInt32 inOffset, WEEdge inEdge) -> (SInt32 outLineStart, SInt32 outLineEnd)")},
	{"WEFindParagraph", (PyCFunction)wasteObj_WEFindParagraph, 1,
	 PyDoc_STR("(SInt32 inOffset, WEEdge inEdge) -> (SInt32 outParagraphStart, SInt32 outParagraphEnd)")},
	{"WEFind", (PyCFunction)wasteObj_WEFind, 1,
	 PyDoc_STR("(char* inKey, SInt32 inKeyLength, TextEncoding inKeyEncoding, OptionBits inMatchOptions, SInt32 inRangeStart, SInt32 inRangeEnd) -> (SInt32 outMatchStart, SInt32 outMatchEnd)")},
	{"WEStreamRange", (PyCFunction)wasteObj_WEStreamRange, 1,
	 PyDoc_STR("(SInt32 inRangeStart, SInt32 inRangeEnd, FlavorType inRequestedType, OptionBits inStreamOptions, Handle outData) -> None")},
	{"WECopyRange", (PyCFunction)wasteObj_WECopyRange, 1,
	 PyDoc_STR("(SInt32 inRangeStart, SInt32 inRangeEnd, Handle outText, StScrpHandle outStyles, WESoupHandle outSoup) -> None")},
	{"WEGetTextRangeAsUnicode", (PyCFunction)wasteObj_WEGetTextRangeAsUnicode, 1,
	 PyDoc_STR("(SInt32 inRangeStart, SInt32 inRangeEnd, Handle outUnicodeText, Handle ioCharFormat, Handle ioParaFormat, TextEncodingVariant inUnicodeVariant, TextEncodingFormat inTransformationFormat, OptionBits inGetOptions) -> None")},
	{"WEGetAlignment", (PyCFunction)wasteObj_WEGetAlignment, 1,
	 PyDoc_STR("() -> (WEAlignment _rv)")},
	{"WESetAlignment", (PyCFunction)wasteObj_WESetAlignment, 1,
	 PyDoc_STR("(WEAlignment inAlignment) -> None")},
	{"WEGetDirection", (PyCFunction)wasteObj_WEGetDirection, 1,
	 PyDoc_STR("() -> (WEDirection _rv)")},
	{"WESetDirection", (PyCFunction)wasteObj_WESetDirection, 1,
	 PyDoc_STR("(WEDirection inDirection) -> None")},
	{"WECalText", (PyCFunction)wasteObj_WECalText, 1,
	 PyDoc_STR("() -> None")},
	{"WEUpdate", (PyCFunction)wasteObj_WEUpdate, 1,
	 PyDoc_STR("(RgnHandle inUpdateRgn) -> None")},
	{"WEScroll", (PyCFunction)wasteObj_WEScroll, 1,
	 PyDoc_STR("(SInt32 inHorizontalOffset, SInt32 inVerticalOffset) -> None")},
	{"WEPinScroll", (PyCFunction)wasteObj_WEPinScroll, 1,
	 PyDoc_STR("(SInt32 inHorizontalOffset, SInt32 inVerticalOffset) -> None")},
	{"WESelView", (PyCFunction)wasteObj_WESelView, 1,
	 PyDoc_STR("() -> None")},
	{"WEActivate", (PyCFunction)wasteObj_WEActivate, 1,
	 PyDoc_STR("() -> None")},
	{"WEDeactivate", (PyCFunction)wasteObj_WEDeactivate, 1,
	 PyDoc_STR("() -> None")},
	{"WEKey", (PyCFunction)wasteObj_WEKey, 1,
	 PyDoc_STR("(CharParameter inKey, EventModifiers inModifiers) -> None")},
	{"WEClick", (PyCFunction)wasteObj_WEClick, 1,
	 PyDoc_STR("(Point inHitPoint, EventModifiers inModifiers, UInt32 inClickTime) -> None")},
	{"WEAdjustCursor", (PyCFunction)wasteObj_WEAdjustCursor, 1,
	 PyDoc_STR("(Point inMouseLoc, RgnHandle ioMouseRgn) -> (Boolean _rv)")},
	{"WEIdle", (PyCFunction)wasteObj_WEIdle, 1,
	 PyDoc_STR("() -> (UInt32 outMaxSleep)")},
	{"WEInsert", (PyCFunction)wasteObj_WEInsert, 1,
	 PyDoc_STR("(Buffer inTextPtr, StScrpHandle inStyles, WESoupHandle inSoup) -> None")},
	{"WEInsertFormattedText", (PyCFunction)wasteObj_WEInsertFormattedText, 1,
	 PyDoc_STR("(Buffer inTextPtr, StScrpHandle inStyles, WESoupHandle inSoup, Handle inParaFormat, Handle inRulerScrap) -> None")},
	{"WEDelete", (PyCFunction)wasteObj_WEDelete, 1,
	 PyDoc_STR("() -> None")},
	{"WEUseText", (PyCFunction)wasteObj_WEUseText, 1,
	 PyDoc_STR("(Handle inText) -> None")},
	{"WEChangeCase", (PyCFunction)wasteObj_WEChangeCase, 1,
	 PyDoc_STR("(SInt16 inCase) -> None")},
	{"WESetOneAttribute", (PyCFunction)wasteObj_WESetOneAttribute, 1,
	 PyDoc_STR("(SInt32 inRangeStart, SInt32 inRangeEnd, WESelector inAttributeSelector, Buffer inAttributeValue) -> None")},
	{"WESetStyle", (PyCFunction)wasteObj_WESetStyle, 1,
	 PyDoc_STR("(WEStyleMode inMode, TextStyle inTextStyle) -> None")},
	{"WEUseStyleScrap", (PyCFunction)wasteObj_WEUseStyleScrap, 1,
	 PyDoc_STR("(StScrpHandle inStyles) -> None")},
	{"WEUndo", (PyCFunction)wasteObj_WEUndo, 1,
	 PyDoc_STR("() -> None")},
	{"WERedo", (PyCFunction)wasteObj_WERedo, 1,
	 PyDoc_STR("() -> None")},
	{"WEClearUndo", (PyCFunction)wasteObj_WEClearUndo, 1,
	 PyDoc_STR("() -> None")},
	{"WEGetUndoInfo", (PyCFunction)wasteObj_WEGetUndoInfo, 1,
	 PyDoc_STR("() -> (WEActionKind _rv, Boolean outRedoFlag)")},
	{"WEGetIndUndoInfo", (PyCFunction)wasteObj_WEGetIndUndoInfo, 1,
	 PyDoc_STR("(SInt32 inUndoLevel) -> (WEActionKind _rv)")},
	{"WEIsTyping", (PyCFunction)wasteObj_WEIsTyping, 1,
	 PyDoc_STR("() -> (Boolean _rv)")},
	{"WEBeginAction", (PyCFunction)wasteObj_WEBeginAction, 1,
	 PyDoc_STR("() -> None")},
	{"WEEndAction", (PyCFunction)wasteObj_WEEndAction, 1,
	 PyDoc_STR("(WEActionKind inActionKind) -> None")},
	{"WEGetModCount", (PyCFunction)wasteObj_WEGetModCount, 1,
	 PyDoc_STR("() -> (UInt32 _rv)")},
	{"WEResetModCount", (PyCFunction)wasteObj_WEResetModCount, 1,
	 PyDoc_STR("() -> None")},
	{"WEInsertObject", (PyCFunction)wasteObj_WEInsertObject, 1,
	 PyDoc_STR("(FlavorType inObjectType, Handle inObjectDataHandle, Point inObjectSize) -> None")},
	{"WEGetSelectedObject", (PyCFunction)wasteObj_WEGetSelectedObject, 1,
	 PyDoc_STR("() -> (WEObjectReference outObject)")},
	{"WEGetObjectAtOffset", (PyCFunction)wasteObj_WEGetObjectAtOffset, 1,
	 PyDoc_STR("(SInt32 inOffset) -> (WEObjectReference outObject)")},
	{"WEFindNextObject", (PyCFunction)wasteObj_WEFindNextObject, 1,
	 PyDoc_STR("(SInt32 inOffset) -> (SInt32 _rv, WEObjectReference outObject)")},
	{"WEUseSoup", (PyCFunction)wasteObj_WEUseSoup, 1,
	 PyDoc_STR("(WESoupHandle inSoup) -> None")},
	{"WECut", (PyCFunction)wasteObj_WECut, 1,
	 PyDoc_STR("() -> None")},
	{"WECopy", (PyCFunction)wasteObj_WECopy, 1,
	 PyDoc_STR("() -> None")},
	{"WEPaste", (PyCFunction)wasteObj_WEPaste, 1,
	 PyDoc_STR("() -> None")},
	{"WECanPaste", (PyCFunction)wasteObj_WECanPaste, 1,
	 PyDoc_STR("() -> (Boolean _rv)")},
	{"WEGetHiliteRgn", (PyCFunction)wasteObj_WEGetHiliteRgn, 1,
	 PyDoc_STR("(SInt32 inRangeStart, SInt32 inRangeEnd) -> (RgnHandle _rv)")},
	{"WECharByte", (PyCFunction)wasteObj_WECharByte, 1,
	 PyDoc_STR("(SInt32 inOffset) -> (SInt16 _rv)")},
	{"WECharType", (PyCFunction)wasteObj_WECharType, 1,
	 PyDoc_STR("(SInt32 inOffset) -> (SInt16 _rv)")},
	{"WEStopInlineSession", (PyCFunction)wasteObj_WEStopInlineSession, 1,
	 PyDoc_STR("() -> None")},
	{"WEFeatureFlag", (PyCFunction)wasteObj_WEFeatureFlag, 1,
	 PyDoc_STR("(SInt16 inFeature, SInt16 inAction) -> (SInt16 _rv)")},
	{"WEGetUserInfo", (PyCFunction)wasteObj_WEGetUserInfo, 1,
	 PyDoc_STR("(WESelector inUserTag) -> (SInt32 outUserInfo)")},
	{"WESetUserInfo", (PyCFunction)wasteObj_WESetUserInfo, 1,
	 PyDoc_STR("(WESelector inUserTag, SInt32 inUserInfo) -> None")},
	{"WERemoveUserInfo", (PyCFunction)wasteObj_WERemoveUserInfo, 1,
	 PyDoc_STR("(WESelector inUserTag) -> None")},
	{"WEInstallTabHooks", (PyCFunction)wasteObj_WEInstallTabHooks, 1,
	 PyDoc_STR("() -> None")},
	{"WERemoveTabHooks", (PyCFunction)wasteObj_WERemoveTabHooks, 1,
	 PyDoc_STR("() -> None")},
	{"WEIsTabHooks", (PyCFunction)wasteObj_WEIsTabHooks, 1,
	 PyDoc_STR("() -> (Boolean _rv)")},
	{"WEGetTabSize", (PyCFunction)wasteObj_WEGetTabSize, 1,
	 PyDoc_STR("() -> (SInt16 _rv)")},
	{"WESetTabSize", (PyCFunction)wasteObj_WESetTabSize, 1,
	 PyDoc_STR("(SInt16 tabWidth) -> None")},
	{NULL, NULL, 0}
};

#define wasteObj_getsetlist NULL


#define wasteObj_compare NULL

#define wasteObj_repr NULL

#define wasteObj_hash NULL
#define wasteObj_tp_init 0

#define wasteObj_tp_alloc PyType_GenericAlloc

static PyObject *wasteObj_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *self;
	WEReference itself;
	char *kw[] = {"itself", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&", kw, wasteObj_Convert, &itself)) return NULL;
	if ((self = type->tp_alloc(type, 0)) == NULL) return NULL;
	((wasteObject *)self)->ob_itself = itself;
	return self;
}

#define wasteObj_tp_free PyObject_Del


PyTypeObject waste_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"waste.waste", /*tp_name*/
	sizeof(wasteObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) wasteObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc)0, /*tp_getattr*/
	(setattrfunc)0, /*tp_setattr*/
	(cmpfunc) wasteObj_compare, /*tp_compare*/
	(reprfunc) wasteObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) wasteObj_hash, /*tp_hash*/
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
	wasteObj_methods, /* tp_methods */
	0, /*tp_members*/
	wasteObj_getsetlist, /*tp_getset*/
	0, /*tp_base*/
	0, /*tp_dict*/
	0, /*tp_descr_get*/
	0, /*tp_descr_set*/
	0, /*tp_dictoffset*/
	wasteObj_tp_init, /* tp_init */
	wasteObj_tp_alloc, /* tp_alloc */
	wasteObj_tp_new, /* tp_new */
	wasteObj_tp_free, /* tp_free */
};

/* --------------------- End object type waste ---------------------- */


static PyObject *waste_WENew(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	LongRect inDestRect;
	LongRect inViewRect;
	OptionBits inOptions;
	WEReference outWE;
	if (!PyArg_ParseTuple(_args, "O&O&l",
	                      LongRect_Convert, &inDestRect,
	                      LongRect_Convert, &inViewRect,
	                      &inOptions))
		return NULL;
	_err = WENew(&inDestRect,
	             &inViewRect,
	             inOptions,
	             &outWE);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     wasteObj_New, outWE);
	return _res;
}

static PyObject *waste_WEUpdateStyleScrap(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	StScrpHandle ioStyles;
	WEFontTableHandle inFontTable;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &ioStyles,
	                      ResObj_Convert, &inFontTable))
		return NULL;
	_err = WEUpdateStyleScrap(ioStyles,
	                          inFontTable);
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
	AppleEvent inAppleEvent;
	AppleEvent ioReply;
	if (!PyArg_ParseTuple(_args, "O&",
	                      AEDesc_Convert, &inAppleEvent))
		return NULL;
	_err = WEHandleTSMEvent(&inAppleEvent,
	                        &ioReply);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &ioReply);
	return _res;
}

static PyObject *waste_WELongPointToPoint(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	LongPt inLongPoint;
	Point outPoint;
	if (!PyArg_ParseTuple(_args, "O&",
	                      LongPt_Convert, &inLongPoint))
		return NULL;
	WELongPointToPoint(&inLongPoint,
	                   &outPoint);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildPoint, outPoint);
	return _res;
}

static PyObject *waste_WEPointToLongPoint(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Point inPoint;
	LongPt outLongPoint;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &inPoint))
		return NULL;
	WEPointToLongPoint(inPoint,
	                   &outLongPoint);
	_res = Py_BuildValue("O&",
	                     LongPt_New, &outLongPoint);
	return _res;
}

static PyObject *waste_WESetLongRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	LongRect outLongRect;
	SInt32 inLeft;
	SInt32 inTop;
	SInt32 inRight;
	SInt32 inBottom;
	if (!PyArg_ParseTuple(_args, "llll",
	                      &inLeft,
	                      &inTop,
	                      &inRight,
	                      &inBottom))
		return NULL;
	WESetLongRect(&outLongRect,
	              inLeft,
	              inTop,
	              inRight,
	              inBottom);
	_res = Py_BuildValue("O&",
	                     LongRect_New, &outLongRect);
	return _res;
}

static PyObject *waste_WELongRectToRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	LongRect inLongRect;
	Rect outRect;
	if (!PyArg_ParseTuple(_args, "O&",
	                      LongRect_Convert, &inLongRect))
		return NULL;
	WELongRectToRect(&inLongRect,
	                 &outRect);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &outRect);
	return _res;
}

static PyObject *waste_WERectToLongRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Rect inRect;
	LongRect outLongRect;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetRect, &inRect))
		return NULL;
	WERectToLongRect(&inRect,
	                 &outLongRect);
	_res = Py_BuildValue("O&",
	                     LongRect_New, &outLongRect);
	return _res;
}

static PyObject *waste_WEOffsetLongRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	LongRect ioLongRect;
	SInt32 inHorizontalOffset;
	SInt32 inVerticalOffset;
	if (!PyArg_ParseTuple(_args, "ll",
	                      &inHorizontalOffset,
	                      &inVerticalOffset))
		return NULL;
	WEOffsetLongRect(&ioLongRect,
	                 inHorizontalOffset,
	                 inVerticalOffset);
	_res = Py_BuildValue("O&",
	                     LongRect_New, &ioLongRect);
	return _res;
}

static PyObject *waste_WELongPointInLongRect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	LongPt inLongPoint;
	LongRect inLongRect;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      LongPt_Convert, &inLongPoint,
	                      LongRect_Convert, &inLongRect))
		return NULL;
	_rv = WELongPointInLongRect(&inLongPoint,
	                            &inLongRect);
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
		_res = Py_None;
		return _res;
		
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
		_res = Py_None;
		return _res;

}

static PyMethodDef waste_methods[] = {
	{"WENew", (PyCFunction)waste_WENew, 1,
	 PyDoc_STR("(LongRect inDestRect, LongRect inViewRect, OptionBits inOptions) -> (WEReference outWE)")},
	{"WEUpdateStyleScrap", (PyCFunction)waste_WEUpdateStyleScrap, 1,
	 PyDoc_STR("(StScrpHandle ioStyles, WEFontTableHandle inFontTable) -> None")},
	{"WEInstallTSMHandlers", (PyCFunction)waste_WEInstallTSMHandlers, 1,
	 PyDoc_STR("() -> None")},
	{"WERemoveTSMHandlers", (PyCFunction)waste_WERemoveTSMHandlers, 1,
	 PyDoc_STR("() -> None")},
	{"WEHandleTSMEvent", (PyCFunction)waste_WEHandleTSMEvent, 1,
	 PyDoc_STR("(AppleEvent inAppleEvent) -> (AppleEvent ioReply)")},
	{"WELongPointToPoint", (PyCFunction)waste_WELongPointToPoint, 1,
	 PyDoc_STR("(LongPt inLongPoint) -> (Point outPoint)")},
	{"WEPointToLongPoint", (PyCFunction)waste_WEPointToLongPoint, 1,
	 PyDoc_STR("(Point inPoint) -> (LongPt outLongPoint)")},
	{"WESetLongRect", (PyCFunction)waste_WESetLongRect, 1,
	 PyDoc_STR("(SInt32 inLeft, SInt32 inTop, SInt32 inRight, SInt32 inBottom) -> (LongRect outLongRect)")},
	{"WELongRectToRect", (PyCFunction)waste_WELongRectToRect, 1,
	 PyDoc_STR("(LongRect inLongRect) -> (Rect outRect)")},
	{"WERectToLongRect", (PyCFunction)waste_WERectToLongRect, 1,
	 PyDoc_STR("(Rect inRect) -> (LongRect outLongRect)")},
	{"WEOffsetLongRect", (PyCFunction)waste_WEOffsetLongRect, 1,
	 PyDoc_STR("(SInt32 inHorizontalOffset, SInt32 inVerticalOffset) -> (LongRect ioLongRect)")},
	{"WELongPointInLongRect", (PyCFunction)waste_WELongPointInLongRect, 1,
	 PyDoc_STR("(LongPt inLongPoint, LongRect inLongRect) -> (Boolean _rv)")},
	{"STDObjectHandlers", (PyCFunction)waste_STDObjectHandlers, 1,
	 PyDoc_STR(NULL)},
	{"WEInstallObjectHandler", (PyCFunction)waste_WEInstallObjectHandler, 1,
	 PyDoc_STR(NULL)},
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
	if (PyType_Ready(&WEO_Type) < 0) return;
	Py_INCREF(&WEO_Type);
	PyModule_AddObject(m, "WEO", (PyObject *)&WEO_Type);
	/* Backward-compatible name */
	Py_INCREF(&WEO_Type);
	PyModule_AddObject(m, "WEOType", (PyObject *)&WEO_Type);
	waste_Type.ob_type = &PyType_Type;
	if (PyType_Ready(&waste_Type) < 0) return;
	Py_INCREF(&waste_Type);
	PyModule_AddObject(m, "waste", (PyObject *)&waste_Type);
	/* Backward-compatible name */
	Py_INCREF(&waste_Type);
	PyModule_AddObject(m, "wasteType", (PyObject *)&waste_Type);

		callbackdict = PyDict_New();
		if (callbackdict == NULL || PyDict_SetItemString(d, "callbacks", callbackdict) != 0)
			return;
		upp_new_handler = NewWENewObjectProc(my_new_handler);
		upp_dispose_handler = NewWEDisposeObjectProc(my_dispose_handler);
		upp_draw_handler = NewWEDrawObjectProc(my_draw_handler);
		upp_click_handler = NewWEClickObjectProc(my_click_handler);


}

/* ======================== End module waste ======================== */

