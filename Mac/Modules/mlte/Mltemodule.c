
/* ========================== Module Mlte =========================== */

#include "Python.h"



#include "macglue.h"
#include "pymactoolbox.h"

/* Macro to test whether a weak-loaded CFM function exists */
#define PyMac_PRECHECK(rtn) do { if ( &rtn == NULL )  {\
    	PyErr_SetString(PyExc_NotImplementedError, \
    	"Not available in this shared library/OS version"); \
    	return NULL; \
    }} while(0)


#ifdef WITHOUT_FRAMEWORKS
#include <MacTextEditor.h>
#else
#include <xxxx.h>
#endif

/* For now we declare them forward here. They'll go to mactoolbox later */
staticforward PyObject *TXNObj_New(TXNObject);
staticforward int TXNObj_Convert(PyObject *, TXNObject *);
staticforward PyObject *TXNFontMenuObj_New(TXNFontMenuObject);
staticforward int TXNFontMenuObj_Convert(PyObject *, TXNFontMenuObject *);

// ADD declarations
#ifdef NOTYET_USE_TOOLBOX_OBJECT_GLUE
//extern PyObject *_CFTypeRefObj_New(CFTypeRef);
//extern int _CFTypeRefObj_Convert(PyObject *, CFTypeRef *);

//#define CFTypeRefObj_New _CFTypeRefObj_New
//#define CFTypeRefObj_Convert _CFTypeRefObj_Convert
#endif

/*
** Parse an optional fsspec
*/
static int
OptFSSpecPtr_Convert(PyObject *v, FSSpec **p_itself)
{
	static FSSpec fss;
	if (v == Py_None)
	{
		*p_itself = NULL;
		return 1;
	}
	*p_itself = &fss;
	return PyMac_GetFSSpec(v, *p_itself);
}

/*
** Parse an optional rect
*/
static int
OptRectPtr_Convert(PyObject *v, Rect **p_itself)
{
	static Rect r;
	
	if (v == Py_None)
	{
		*p_itself = NULL;
		return 1;
	}
	*p_itself = &r;
	return PyMac_GetRect(v, *p_itself);
}

/*
** Parse an optional GWorld
*/
static int
OptGWorldObj_Convert(PyObject *v, GWorldPtr *p_itself)
{	
	if (v == Py_None)
	{
		*p_itself = NULL;
		return 1;
	}
	return GWorldObj_Convert(v, p_itself);
}


static PyObject *Mlte_Error;

/* --------------------- Object type TXNObject ---------------------- */

PyTypeObject TXNObject_Type;

#define TXNObj_Check(x) ((x)->ob_type == &TXNObject_Type)

typedef struct TXNObjectObject {
	PyObject_HEAD
	TXNObject ob_itself;
} TXNObjectObject;

PyObject *TXNObj_New(TXNObject itself)
{
	TXNObjectObject *it;
	if (itself == NULL) return PyMac_Error(resNotFound);
	it = PyObject_NEW(TXNObjectObject, &TXNObject_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}
TXNObj_Convert(PyObject *v, TXNObject *p_itself)
{
	if (!TXNObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "TXNObject required");
		return 0;
	}
	*p_itself = ((TXNObjectObject *)v)->ob_itself;
	return 1;
}

static void TXNObj_dealloc(TXNObjectObject *self)
{
	/* Cleanup of self->ob_itself goes here */
	PyMem_DEL(self);
}

static PyObject *TXNObj_TXNDeleteObject(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PyMac_PRECHECK(TXNDeleteObject);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	TXNDeleteObject(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNResizeFrame(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	UInt32 iWidth;
	UInt32 iHeight;
	TXNFrameID iTXNFrameID;
	PyMac_PRECHECK(TXNResizeFrame);
	if (!PyArg_ParseTuple(_args, "lll",
	                      &iWidth,
	                      &iHeight,
	                      &iTXNFrameID))
		return NULL;
	TXNResizeFrame(_self->ob_itself,
	               iWidth,
	               iHeight,
	               iTXNFrameID);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNSetFrameBounds(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt32 iTop;
	SInt32 iLeft;
	SInt32 iBottom;
	SInt32 iRight;
	TXNFrameID iTXNFrameID;
	PyMac_PRECHECK(TXNSetFrameBounds);
	if (!PyArg_ParseTuple(_args, "lllll",
	                      &iTop,
	                      &iLeft,
	                      &iBottom,
	                      &iRight,
	                      &iTXNFrameID))
		return NULL;
	TXNSetFrameBounds(_self->ob_itself,
	                  iTop,
	                  iLeft,
	                  iBottom,
	                  iRight,
	                  iTXNFrameID);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNKeyDown(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	EventRecord iEvent;
	PyMac_PRECHECK(TXNKeyDown);
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetEventRecord, &iEvent))
		return NULL;
	TXNKeyDown(_self->ob_itself,
	           &iEvent);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNAdjustCursor(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	RgnHandle ioCursorRgn;
	PyMac_PRECHECK(TXNAdjustCursor);
	if (!PyArg_ParseTuple(_args, "O&",
	                      OptResObj_Convert, &ioCursorRgn))
		return NULL;
	TXNAdjustCursor(_self->ob_itself,
	                ioCursorRgn);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNClick(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	EventRecord iEvent;
	PyMac_PRECHECK(TXNClick);
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetEventRecord, &iEvent))
		return NULL;
	TXNClick(_self->ob_itself,
	         &iEvent);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNTSMCheck(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	EventRecord iEvent;
	PyMac_PRECHECK(TXNTSMCheck);
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetEventRecord, &iEvent))
		return NULL;
	_rv = TXNTSMCheck(_self->ob_itself,
	                  &iEvent);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *TXNObj_TXNSelectAll(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PyMac_PRECHECK(TXNSelectAll);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	TXNSelectAll(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNFocus(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean iBecomingFocused;
	PyMac_PRECHECK(TXNFocus);
	if (!PyArg_ParseTuple(_args, "b",
	                      &iBecomingFocused))
		return NULL;
	TXNFocus(_self->ob_itself,
	         iBecomingFocused);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNUpdate(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PyMac_PRECHECK(TXNUpdate);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	TXNUpdate(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNDraw(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	GWorldPtr iDrawPort;
	PyMac_PRECHECK(TXNDraw);
	if (!PyArg_ParseTuple(_args, "O&",
	                      OptGWorldObj_Convert, &iDrawPort))
		return NULL;
	TXNDraw(_self->ob_itself,
	        iDrawPort);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNForceUpdate(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PyMac_PRECHECK(TXNForceUpdate);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	TXNForceUpdate(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNGetSleepTicks(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	UInt32 _rv;
	PyMac_PRECHECK(TXNGetSleepTicks);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = TXNGetSleepTicks(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *TXNObj_TXNIdle(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PyMac_PRECHECK(TXNIdle);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	TXNIdle(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNGrowWindow(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	EventRecord iEvent;
	PyMac_PRECHECK(TXNGrowWindow);
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetEventRecord, &iEvent))
		return NULL;
	TXNGrowWindow(_self->ob_itself,
	              &iEvent);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNZoomWindow(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short iPart;
	PyMac_PRECHECK(TXNZoomWindow);
	if (!PyArg_ParseTuple(_args, "h",
	                      &iPart))
		return NULL;
	TXNZoomWindow(_self->ob_itself,
	              iPart);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNCanUndo(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	TXNActionKey oTXNActionKey;
	PyMac_PRECHECK(TXNCanUndo);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = TXNCanUndo(_self->ob_itself,
	                 &oTXNActionKey);
	_res = Py_BuildValue("bl",
	                     _rv,
	                     oTXNActionKey);
	return _res;
}

static PyObject *TXNObj_TXNUndo(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PyMac_PRECHECK(TXNUndo);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	TXNUndo(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNCanRedo(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	TXNActionKey oTXNActionKey;
	PyMac_PRECHECK(TXNCanRedo);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = TXNCanRedo(_self->ob_itself,
	                 &oTXNActionKey);
	_res = Py_BuildValue("bl",
	                     _rv,
	                     oTXNActionKey);
	return _res;
}

static PyObject *TXNObj_TXNRedo(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PyMac_PRECHECK(TXNRedo);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	TXNRedo(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNCut(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	PyMac_PRECHECK(TXNCut);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = TXNCut(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNCopy(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	PyMac_PRECHECK(TXNCopy);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = TXNCopy(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNPaste(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	PyMac_PRECHECK(TXNPaste);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = TXNPaste(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNClear(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	PyMac_PRECHECK(TXNClear);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = TXNClear(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNGetSelection(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TXNOffset oStartOffset;
	TXNOffset oEndOffset;
	PyMac_PRECHECK(TXNGetSelection);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	TXNGetSelection(_self->ob_itself,
	                &oStartOffset,
	                &oEndOffset);
	_res = Py_BuildValue("ll",
	                     oStartOffset,
	                     oEndOffset);
	return _res;
}

static PyObject *TXNObj_TXNShowSelection(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean iShowEnd;
	PyMac_PRECHECK(TXNShowSelection);
	if (!PyArg_ParseTuple(_args, "b",
	                      &iShowEnd))
		return NULL;
	TXNShowSelection(_self->ob_itself,
	                 iShowEnd);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNIsSelectionEmpty(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	PyMac_PRECHECK(TXNIsSelectionEmpty);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = TXNIsSelectionEmpty(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *TXNObj_TXNSetSelection(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	TXNOffset iStartOffset;
	TXNOffset iEndOffset;
	PyMac_PRECHECK(TXNSetSelection);
	if (!PyArg_ParseTuple(_args, "ll",
	                      &iStartOffset,
	                      &iEndOffset))
		return NULL;
	_err = TXNSetSelection(_self->ob_itself,
	                       iStartOffset,
	                       iEndOffset);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNCountRunsInRange(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	UInt32 iStartOffset;
	UInt32 iEndOffset;
	ItemCount oRunCount;
	PyMac_PRECHECK(TXNCountRunsInRange);
	if (!PyArg_ParseTuple(_args, "ll",
	                      &iStartOffset,
	                      &iEndOffset))
		return NULL;
	_err = TXNCountRunsInRange(_self->ob_itself,
	                           iStartOffset,
	                           iEndOffset,
	                           &oRunCount);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     oRunCount);
	return _res;
}

static PyObject *TXNObj_TXNDataSize(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ByteCount _rv;
	PyMac_PRECHECK(TXNDataSize);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = TXNDataSize(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *TXNObj_TXNGetData(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	TXNOffset iStartOffset;
	TXNOffset iEndOffset;
	Handle oDataHandle;
	PyMac_PRECHECK(TXNGetData);
	if (!PyArg_ParseTuple(_args, "ll",
	                      &iStartOffset,
	                      &iEndOffset))
		return NULL;
	_err = TXNGetData(_self->ob_itself,
	                  iStartOffset,
	                  iEndOffset,
	                  &oDataHandle);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, oDataHandle);
	return _res;
}

static PyObject *TXNObj_TXNGetDataEncoded(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	TXNOffset iStartOffset;
	TXNOffset iEndOffset;
	Handle oDataHandle;
	TXNDataType iEncoding;
	PyMac_PRECHECK(TXNGetDataEncoded);
	if (!PyArg_ParseTuple(_args, "llO&",
	                      &iStartOffset,
	                      &iEndOffset,
	                      PyMac_GetOSType, &iEncoding))
		return NULL;
	_err = TXNGetDataEncoded(_self->ob_itself,
	                         iStartOffset,
	                         iEndOffset,
	                         &oDataHandle,
	                         iEncoding);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, oDataHandle);
	return _res;
}

static PyObject *TXNObj_TXNSetDataFromFile(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	SInt16 iFileRefNum;
	OSType iFileType;
	ByteCount iFileLength;
	TXNOffset iStartOffset;
	TXNOffset iEndOffset;
	PyMac_PRECHECK(TXNSetDataFromFile);
	if (!PyArg_ParseTuple(_args, "hO&lll",
	                      &iFileRefNum,
	                      PyMac_GetOSType, &iFileType,
	                      &iFileLength,
	                      &iStartOffset,
	                      &iEndOffset))
		return NULL;
	_err = TXNSetDataFromFile(_self->ob_itself,
	                          iFileRefNum,
	                          iFileType,
	                          iFileLength,
	                          iStartOffset,
	                          iEndOffset);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNSetData(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	TXNDataType iDataType;
	void * *iDataPtr__in__;
	ByteCount iDataPtr__len__;
	int iDataPtr__in_len__;
	TXNOffset iStartOffset;
	TXNOffset iEndOffset;
	PyMac_PRECHECK(TXNSetData);
	if (!PyArg_ParseTuple(_args, "O&s#ll",
	                      PyMac_GetOSType, &iDataType,
	                      &iDataPtr__in__, &iDataPtr__in_len__,
	                      &iStartOffset,
	                      &iEndOffset))
		return NULL;
	iDataPtr__len__ = iDataPtr__in_len__;
	_err = TXNSetData(_self->ob_itself,
	                  iDataType,
	                  iDataPtr__in__, iDataPtr__len__,
	                  iStartOffset,
	                  iEndOffset);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
 iDataPtr__error__: ;
	return _res;
}

static PyObject *TXNObj_TXNGetChangeCount(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ItemCount _rv;
	PyMac_PRECHECK(TXNGetChangeCount);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = TXNGetChangeCount(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *TXNObj_TXNSave(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	OSType iType;
	OSType iResType;
	TXNPermanentTextEncodingType iPermanentEncoding;
	FSSpec iFileSpecification;
	SInt16 iDataReference;
	SInt16 iResourceReference;
	PyMac_PRECHECK(TXNSave);
	if (!PyArg_ParseTuple(_args, "O&O&lO&hh",
	                      PyMac_GetOSType, &iType,
	                      PyMac_GetOSType, &iResType,
	                      &iPermanentEncoding,
	                      PyMac_GetFSSpec, &iFileSpecification,
	                      &iDataReference,
	                      &iResourceReference))
		return NULL;
	_err = TXNSave(_self->ob_itself,
	               iType,
	               iResType,
	               iPermanentEncoding,
	               &iFileSpecification,
	               iDataReference,
	               iResourceReference);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNRevert(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	PyMac_PRECHECK(TXNRevert);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = TXNRevert(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNPageSetup(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	PyMac_PRECHECK(TXNPageSetup);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = TXNPageSetup(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNPrint(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	PyMac_PRECHECK(TXNPrint);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = TXNPrint(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNGetViewRect(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Rect oViewRect;
	PyMac_PRECHECK(TXNGetViewRect);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	TXNGetViewRect(_self->ob_itself,
	               &oViewRect);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildRect, &oViewRect);
	return _res;
}

static PyObject *TXNObj_TXNAttachObjectToWindow(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	GWorldPtr iWindow;
	Boolean iIsActualWindow;
	PyMac_PRECHECK(TXNAttachObjectToWindow);
	if (!PyArg_ParseTuple(_args, "O&b",
	                      GWorldObj_Convert, &iWindow,
	                      &iIsActualWindow))
		return NULL;
	_err = TXNAttachObjectToWindow(_self->ob_itself,
	                               iWindow,
	                               iIsActualWindow);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNIsObjectAttachedToWindow(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	PyMac_PRECHECK(TXNIsObjectAttachedToWindow);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = TXNIsObjectAttachedToWindow(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *TXNObj_TXNDragTracker(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	TXNFrameID iTXNFrameID;
	DragTrackingMessage iMessage;
	WindowPtr iWindow;
	DragReference iDragReference;
	Boolean iDifferentObjectSameWindow;
	PyMac_PRECHECK(TXNDragTracker);
	if (!PyArg_ParseTuple(_args, "lhO&O&b",
	                      &iTXNFrameID,
	                      &iMessage,
	                      WinObj_Convert, &iWindow,
	                      DragObj_Convert, &iDragReference,
	                      &iDifferentObjectSameWindow))
		return NULL;
	_err = TXNDragTracker(_self->ob_itself,
	                      iTXNFrameID,
	                      iMessage,
	                      iWindow,
	                      iDragReference,
	                      iDifferentObjectSameWindow);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNDragReceiver(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	TXNFrameID iTXNFrameID;
	WindowPtr iWindow;
	DragReference iDragReference;
	Boolean iDifferentObjectSameWindow;
	PyMac_PRECHECK(TXNDragReceiver);
	if (!PyArg_ParseTuple(_args, "lO&O&b",
	                      &iTXNFrameID,
	                      WinObj_Convert, &iWindow,
	                      DragObj_Convert, &iDragReference,
	                      &iDifferentObjectSameWindow))
		return NULL;
	_err = TXNDragReceiver(_self->ob_itself,
	                       iTXNFrameID,
	                       iWindow,
	                       iDragReference,
	                       iDifferentObjectSameWindow);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNActivate(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	TXNFrameID iTXNFrameID;
	TXNScrollBarState iActiveState;
	PyMac_PRECHECK(TXNActivate);
	if (!PyArg_ParseTuple(_args, "ll",
	                      &iTXNFrameID,
	                      &iActiveState))
		return NULL;
	_err = TXNActivate(_self->ob_itself,
	                   iTXNFrameID,
	                   iActiveState);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNDoFontMenuSelection(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	TXNFontMenuObject iTXNFontMenuObject;
	SInt16 iMenuID;
	SInt16 iMenuItem;
	PyMac_PRECHECK(TXNDoFontMenuSelection);
	if (!PyArg_ParseTuple(_args, "O&hh",
	                      TXNFontMenuObj_Convert, &iTXNFontMenuObject,
	                      &iMenuID,
	                      &iMenuItem))
		return NULL;
	_err = TXNDoFontMenuSelection(_self->ob_itself,
	                              iTXNFontMenuObject,
	                              iMenuID,
	                              iMenuItem);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *TXNObj_TXNPrepareFontMenu(TXNObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	TXNFontMenuObject iTXNFontMenuObject;
	PyMac_PRECHECK(TXNPrepareFontMenu);
	if (!PyArg_ParseTuple(_args, "O&",
	                      TXNFontMenuObj_Convert, &iTXNFontMenuObject))
		return NULL;
	_err = TXNPrepareFontMenu(_self->ob_itself,
	                          iTXNFontMenuObject);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef TXNObj_methods[] = {
	{"TXNDeleteObject", (PyCFunction)TXNObj_TXNDeleteObject, 1,
	 "() -> None"},
	{"TXNResizeFrame", (PyCFunction)TXNObj_TXNResizeFrame, 1,
	 "(UInt32 iWidth, UInt32 iHeight, TXNFrameID iTXNFrameID) -> None"},
	{"TXNSetFrameBounds", (PyCFunction)TXNObj_TXNSetFrameBounds, 1,
	 "(SInt32 iTop, SInt32 iLeft, SInt32 iBottom, SInt32 iRight, TXNFrameID iTXNFrameID) -> None"},
	{"TXNKeyDown", (PyCFunction)TXNObj_TXNKeyDown, 1,
	 "(EventRecord iEvent) -> None"},
	{"TXNAdjustCursor", (PyCFunction)TXNObj_TXNAdjustCursor, 1,
	 "(RgnHandle ioCursorRgn) -> None"},
	{"TXNClick", (PyCFunction)TXNObj_TXNClick, 1,
	 "(EventRecord iEvent) -> None"},
	{"TXNTSMCheck", (PyCFunction)TXNObj_TXNTSMCheck, 1,
	 "(EventRecord iEvent) -> (Boolean _rv)"},
	{"TXNSelectAll", (PyCFunction)TXNObj_TXNSelectAll, 1,
	 "() -> None"},
	{"TXNFocus", (PyCFunction)TXNObj_TXNFocus, 1,
	 "(Boolean iBecomingFocused) -> None"},
	{"TXNUpdate", (PyCFunction)TXNObj_TXNUpdate, 1,
	 "() -> None"},
	{"TXNDraw", (PyCFunction)TXNObj_TXNDraw, 1,
	 "(GWorldPtr iDrawPort) -> None"},
	{"TXNForceUpdate", (PyCFunction)TXNObj_TXNForceUpdate, 1,
	 "() -> None"},
	{"TXNGetSleepTicks", (PyCFunction)TXNObj_TXNGetSleepTicks, 1,
	 "() -> (UInt32 _rv)"},
	{"TXNIdle", (PyCFunction)TXNObj_TXNIdle, 1,
	 "() -> None"},
	{"TXNGrowWindow", (PyCFunction)TXNObj_TXNGrowWindow, 1,
	 "(EventRecord iEvent) -> None"},
	{"TXNZoomWindow", (PyCFunction)TXNObj_TXNZoomWindow, 1,
	 "(short iPart) -> None"},
	{"TXNCanUndo", (PyCFunction)TXNObj_TXNCanUndo, 1,
	 "() -> (Boolean _rv, TXNActionKey oTXNActionKey)"},
	{"TXNUndo", (PyCFunction)TXNObj_TXNUndo, 1,
	 "() -> None"},
	{"TXNCanRedo", (PyCFunction)TXNObj_TXNCanRedo, 1,
	 "() -> (Boolean _rv, TXNActionKey oTXNActionKey)"},
	{"TXNRedo", (PyCFunction)TXNObj_TXNRedo, 1,
	 "() -> None"},
	{"TXNCut", (PyCFunction)TXNObj_TXNCut, 1,
	 "() -> None"},
	{"TXNCopy", (PyCFunction)TXNObj_TXNCopy, 1,
	 "() -> None"},
	{"TXNPaste", (PyCFunction)TXNObj_TXNPaste, 1,
	 "() -> None"},
	{"TXNClear", (PyCFunction)TXNObj_TXNClear, 1,
	 "() -> None"},
	{"TXNGetSelection", (PyCFunction)TXNObj_TXNGetSelection, 1,
	 "() -> (TXNOffset oStartOffset, TXNOffset oEndOffset)"},
	{"TXNShowSelection", (PyCFunction)TXNObj_TXNShowSelection, 1,
	 "(Boolean iShowEnd) -> None"},
	{"TXNIsSelectionEmpty", (PyCFunction)TXNObj_TXNIsSelectionEmpty, 1,
	 "() -> (Boolean _rv)"},
	{"TXNSetSelection", (PyCFunction)TXNObj_TXNSetSelection, 1,
	 "(TXNOffset iStartOffset, TXNOffset iEndOffset) -> None"},
	{"TXNCountRunsInRange", (PyCFunction)TXNObj_TXNCountRunsInRange, 1,
	 "(UInt32 iStartOffset, UInt32 iEndOffset) -> (ItemCount oRunCount)"},
	{"TXNDataSize", (PyCFunction)TXNObj_TXNDataSize, 1,
	 "() -> (ByteCount _rv)"},
	{"TXNGetData", (PyCFunction)TXNObj_TXNGetData, 1,
	 "(TXNOffset iStartOffset, TXNOffset iEndOffset) -> (Handle oDataHandle)"},
	{"TXNGetDataEncoded", (PyCFunction)TXNObj_TXNGetDataEncoded, 1,
	 "(TXNOffset iStartOffset, TXNOffset iEndOffset, TXNDataType iEncoding) -> (Handle oDataHandle)"},
	{"TXNSetDataFromFile", (PyCFunction)TXNObj_TXNSetDataFromFile, 1,
	 "(SInt16 iFileRefNum, OSType iFileType, ByteCount iFileLength, TXNOffset iStartOffset, TXNOffset iEndOffset) -> None"},
	{"TXNSetData", (PyCFunction)TXNObj_TXNSetData, 1,
	 "(TXNDataType iDataType, Buffer iDataPtr, TXNOffset iStartOffset, TXNOffset iEndOffset) -> None"},
	{"TXNGetChangeCount", (PyCFunction)TXNObj_TXNGetChangeCount, 1,
	 "() -> (ItemCount _rv)"},
	{"TXNSave", (PyCFunction)TXNObj_TXNSave, 1,
	 "(OSType iType, OSType iResType, TXNPermanentTextEncodingType iPermanentEncoding, FSSpec iFileSpecification, SInt16 iDataReference, SInt16 iResourceReference) -> None"},
	{"TXNRevert", (PyCFunction)TXNObj_TXNRevert, 1,
	 "() -> None"},
	{"TXNPageSetup", (PyCFunction)TXNObj_TXNPageSetup, 1,
	 "() -> None"},
	{"TXNPrint", (PyCFunction)TXNObj_TXNPrint, 1,
	 "() -> None"},
	{"TXNGetViewRect", (PyCFunction)TXNObj_TXNGetViewRect, 1,
	 "() -> (Rect oViewRect)"},
	{"TXNAttachObjectToWindow", (PyCFunction)TXNObj_TXNAttachObjectToWindow, 1,
	 "(GWorldPtr iWindow, Boolean iIsActualWindow) -> None"},
	{"TXNIsObjectAttachedToWindow", (PyCFunction)TXNObj_TXNIsObjectAttachedToWindow, 1,
	 "() -> (Boolean _rv)"},
	{"TXNDragTracker", (PyCFunction)TXNObj_TXNDragTracker, 1,
	 "(TXNFrameID iTXNFrameID, DragTrackingMessage iMessage, WindowPtr iWindow, DragReference iDragReference, Boolean iDifferentObjectSameWindow) -> None"},
	{"TXNDragReceiver", (PyCFunction)TXNObj_TXNDragReceiver, 1,
	 "(TXNFrameID iTXNFrameID, WindowPtr iWindow, DragReference iDragReference, Boolean iDifferentObjectSameWindow) -> None"},
	{"TXNActivate", (PyCFunction)TXNObj_TXNActivate, 1,
	 "(TXNFrameID iTXNFrameID, TXNScrollBarState iActiveState) -> None"},
	{"TXNDoFontMenuSelection", (PyCFunction)TXNObj_TXNDoFontMenuSelection, 1,
	 "(TXNFontMenuObject iTXNFontMenuObject, SInt16 iMenuID, SInt16 iMenuItem) -> None"},
	{"TXNPrepareFontMenu", (PyCFunction)TXNObj_TXNPrepareFontMenu, 1,
	 "(TXNFontMenuObject iTXNFontMenuObject) -> None"},
	{NULL, NULL, 0}
};

PyMethodChain TXNObj_chain = { TXNObj_methods, NULL };

static PyObject *TXNObj_getattr(TXNObjectObject *self, char *name)
{
	return Py_FindMethodInChain(&TXNObj_chain, (PyObject *)self, name);
}

#define TXNObj_setattr NULL

#define TXNObj_compare NULL

#define TXNObj_repr NULL

#define TXNObj_hash NULL

PyTypeObject TXNObject_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"TXNObject", /*tp_name*/
	sizeof(TXNObjectObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) TXNObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) TXNObj_getattr, /*tp_getattr*/
	(setattrfunc) TXNObj_setattr, /*tp_setattr*/
	(cmpfunc) TXNObj_compare, /*tp_compare*/
	(reprfunc) TXNObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) TXNObj_hash, /*tp_hash*/
};

/* ------------------- End object type TXNObject -------------------- */


/* ----------------- Object type TXNFontMenuObject ------------------ */

PyTypeObject TXNFontMenuObject_Type;

#define TXNFontMenuObj_Check(x) ((x)->ob_type == &TXNFontMenuObject_Type)

typedef struct TXNFontMenuObjectObject {
	PyObject_HEAD
	TXNFontMenuObject ob_itself;
} TXNFontMenuObjectObject;

PyObject *TXNFontMenuObj_New(TXNFontMenuObject itself)
{
	TXNFontMenuObjectObject *it;
	if (itself == NULL) return PyMac_Error(resNotFound);
	it = PyObject_NEW(TXNFontMenuObjectObject, &TXNFontMenuObject_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}
TXNFontMenuObj_Convert(PyObject *v, TXNFontMenuObject *p_itself)
{
	if (!TXNFontMenuObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "TXNFontMenuObject required");
		return 0;
	}
	*p_itself = ((TXNFontMenuObjectObject *)v)->ob_itself;
	return 1;
}

static void TXNFontMenuObj_dealloc(TXNFontMenuObjectObject *self)
{
	/* Cleanup of self->ob_itself goes here */
	PyMem_DEL(self);
}

static PyObject *TXNFontMenuObj_TXNGetFontMenuHandle(TXNFontMenuObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuHandle oFontMenuHandle;
	PyMac_PRECHECK(TXNGetFontMenuHandle);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = TXNGetFontMenuHandle(_self->ob_itself,
	                            &oFontMenuHandle);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     MenuObj_New, oFontMenuHandle);
	return _res;
}

static PyObject *TXNFontMenuObj_TXNDisposeFontMenuObject(TXNFontMenuObjectObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	PyMac_PRECHECK(TXNDisposeFontMenuObject);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = TXNDisposeFontMenuObject(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef TXNFontMenuObj_methods[] = {
	{"TXNGetFontMenuHandle", (PyCFunction)TXNFontMenuObj_TXNGetFontMenuHandle, 1,
	 "() -> (MenuHandle oFontMenuHandle)"},
	{"TXNDisposeFontMenuObject", (PyCFunction)TXNFontMenuObj_TXNDisposeFontMenuObject, 1,
	 "() -> None"},
	{NULL, NULL, 0}
};

PyMethodChain TXNFontMenuObj_chain = { TXNFontMenuObj_methods, NULL };

static PyObject *TXNFontMenuObj_getattr(TXNFontMenuObjectObject *self, char *name)
{
	return Py_FindMethodInChain(&TXNFontMenuObj_chain, (PyObject *)self, name);
}

#define TXNFontMenuObj_setattr NULL

#define TXNFontMenuObj_compare NULL

#define TXNFontMenuObj_repr NULL

#define TXNFontMenuObj_hash NULL

PyTypeObject TXNFontMenuObject_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"TXNFontMenuObject", /*tp_name*/
	sizeof(TXNFontMenuObjectObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) TXNFontMenuObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) TXNFontMenuObj_getattr, /*tp_getattr*/
	(setattrfunc) TXNFontMenuObj_setattr, /*tp_setattr*/
	(cmpfunc) TXNFontMenuObj_compare, /*tp_compare*/
	(reprfunc) TXNFontMenuObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) TXNFontMenuObj_hash, /*tp_hash*/
};

/* --------------- End object type TXNFontMenuObject ---------------- */


static PyObject *Mlte_TXNNewObject(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	FSSpec * iFileSpec;
	WindowPtr iWindow;
	Rect * iFrame;
	TXNFrameOptions iFrameOptions;
	TXNFrameType iFrameType;
	TXNFileType iFileType;
	TXNPermanentTextEncodingType iPermanentEncoding;
	TXNObject oTXNObject;
	TXNFrameID oTXNFrameID;
	PyMac_PRECHECK(TXNNewObject);
	if (!PyArg_ParseTuple(_args, "O&O&O&llO&l",
	                      OptFSSpecPtr_Convert, &iFileSpec,
	                      WinObj_Convert, &iWindow,
	                      OptRectPtr_Convert, &iFrame,
	                      &iFrameOptions,
	                      &iFrameType,
	                      PyMac_GetOSType, &iFileType,
	                      &iPermanentEncoding))
		return NULL;
	_err = TXNNewObject(iFileSpec,
	                    iWindow,
	                    iFrame,
	                    iFrameOptions,
	                    iFrameType,
	                    iFileType,
	                    iPermanentEncoding,
	                    &oTXNObject,
	                    &oTXNFrameID,
	                    (TXNObjectRefcon)0);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&l",
	                     TXNObj_New, oTXNObject,
	                     oTXNFrameID);
	return _res;
}

static PyObject *Mlte_TXNTerminateTextension(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PyMac_PRECHECK(TXNTerminateTextension);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	TXNTerminateTextension();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Mlte_TXNIsScrapPastable(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	PyMac_PRECHECK(TXNIsScrapPastable);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = TXNIsScrapPastable();
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Mlte_TXNConvertToPublicScrap(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	PyMac_PRECHECK(TXNConvertToPublicScrap);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = TXNConvertToPublicScrap();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Mlte_TXNConvertFromPublicScrap(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	PyMac_PRECHECK(TXNConvertFromPublicScrap);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = TXNConvertFromPublicScrap();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Mlte_TXNNewFontMenuObject(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuHandle iFontMenuHandle;
	SInt16 iMenuID;
	SInt16 iStartHierMenuID;
	TXNFontMenuObject oTXNFontMenuObject;
	PyMac_PRECHECK(TXNNewFontMenuObject);
	if (!PyArg_ParseTuple(_args, "O&hh",
	                      MenuObj_Convert, &iFontMenuHandle,
	                      &iMenuID,
	                      &iStartHierMenuID))
		return NULL;
	_err = TXNNewFontMenuObject(iFontMenuHandle,
	                            iMenuID,
	                            iStartHierMenuID,
	                            &oTXNFontMenuObject);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     TXNFontMenuObj_New, oTXNFontMenuObject);
	return _res;
}

static PyObject *Mlte_TXNVersionInformation(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	TXNVersionValue _rv;
	TXNFeatureBits oFeatureFlags;
	PyMac_PRECHECK(TXNVersionInformation);
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = TXNVersionInformation(&oFeatureFlags);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     oFeatureFlags);
	return _res;
}

static PyObject *Mlte_TXNInitTextension(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;

	OSStatus _err;
	TXNMacOSPreferredFontDescription * iDefaultFonts = NULL;
	ItemCount iCountDefaultFonts = 0;
	TXNInitOptions iUsageFlags;
	PyMac_PRECHECK(TXNInitTextension);
	if (!PyArg_ParseTuple(_args, "l", &iUsageFlags))
		return NULL;
	_err = TXNInitTextension(iDefaultFonts,
	                         iCountDefaultFonts,
	                         iUsageFlags);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;

}

static PyMethodDef Mlte_methods[] = {
	{"TXNNewObject", (PyCFunction)Mlte_TXNNewObject, 1,
	 "(FSSpec * iFileSpec, WindowPtr iWindow, Rect * iFrame, TXNFrameOptions iFrameOptions, TXNFrameType iFrameType, TXNFileType iFileType, TXNPermanentTextEncodingType iPermanentEncoding) -> (TXNObject oTXNObject, TXNFrameID oTXNFrameID)"},
	{"TXNTerminateTextension", (PyCFunction)Mlte_TXNTerminateTextension, 1,
	 "() -> None"},
	{"TXNIsScrapPastable", (PyCFunction)Mlte_TXNIsScrapPastable, 1,
	 "() -> (Boolean _rv)"},
	{"TXNConvertToPublicScrap", (PyCFunction)Mlte_TXNConvertToPublicScrap, 1,
	 "() -> None"},
	{"TXNConvertFromPublicScrap", (PyCFunction)Mlte_TXNConvertFromPublicScrap, 1,
	 "() -> None"},
	{"TXNNewFontMenuObject", (PyCFunction)Mlte_TXNNewFontMenuObject, 1,
	 "(MenuHandle iFontMenuHandle, SInt16 iMenuID, SInt16 iStartHierMenuID) -> (TXNFontMenuObject oTXNFontMenuObject)"},
	{"TXNVersionInformation", (PyCFunction)Mlte_TXNVersionInformation, 1,
	 "() -> (TXNVersionValue _rv, TXNFeatureBits oFeatureFlags)"},
	{"TXNInitTextension", (PyCFunction)Mlte_TXNInitTextension, 1,
	 "(TXNInitOptions) -> None"},
	{NULL, NULL, 0}
};




void initMlte(void)
{
	PyObject *m;
	PyObject *d;



	//	PyMac_INIT_TOOLBOX_OBJECT_NEW(xxxx);


	m = Py_InitModule("Mlte", Mlte_methods);
	d = PyModule_GetDict(m);
	Mlte_Error = PyMac_GetOSErrException();
	if (Mlte_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Mlte_Error) != 0)
		return;
	TXNObject_Type.ob_type = &PyType_Type;
	Py_INCREF(&TXNObject_Type);
	if (PyDict_SetItemString(d, "TXNObjectType", (PyObject *)&TXNObject_Type) != 0)
		Py_FatalError("can't initialize TXNObjectType");
	TXNFontMenuObject_Type.ob_type = &PyType_Type;
	Py_INCREF(&TXNFontMenuObject_Type);
	if (PyDict_SetItemString(d, "TXNFontMenuObjectType", (PyObject *)&TXNFontMenuObject_Type) != 0)
		Py_FatalError("can't initialize TXNFontMenuObjectType");
}

/* ======================== End module Mlte ========================= */

