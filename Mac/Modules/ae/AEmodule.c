
/* =========================== Module AE ============================ */

#include "Python.h"



#include "macglue.h"
#include "pymactoolbox.h"

#ifdef WITHOUT_FRAMEWORKS
#include <AppleEvents.h>
#include <AEObjects.h>
#else
#include <Carbon/Carbon.h>
#endif

#ifdef USE_TOOLBOX_OBJECT_GLUE
extern PyObject *_AEDesc_New(AEDesc *);
extern int _AEDesc_Convert(PyObject *, AEDesc *);

#define AEDesc_New _AEDesc_New
#define AEDesc_Convert _AEDesc_Convert
#endif

static pascal OSErr GenericEventHandler(); /* Forward */

AEEventHandlerUPP upp_GenericEventHandler;

static pascal Boolean AEIdleProc(EventRecord *theEvent, long *sleepTime, RgnHandle *mouseRgn)
{
	if ( PyOS_InterruptOccurred() )
		return 1;
	if ( PyMac_HandleEvent(theEvent) < 0 ) {
		PySys_WriteStderr("Exception in user event handler during AE processing\n");
		PyErr_Clear();
	}
	return 0;
}

AEIdleUPP upp_AEIdleProc;

static PyObject *AE_Error;

/* ----------------------- Object type AEDesc ----------------------- */

PyTypeObject AEDesc_Type;

#define AEDesc_Check(x) ((x)->ob_type == &AEDesc_Type)

typedef struct AEDescObject {
	PyObject_HEAD
	AEDesc ob_itself;
} AEDescObject;

PyObject *AEDesc_New(AEDesc *itself)
{
	AEDescObject *it;
	it = PyObject_NEW(AEDescObject, &AEDesc_Type);
	if (it == NULL) return NULL;
	it->ob_itself = *itself;
	return (PyObject *)it;
}
AEDesc_Convert(PyObject *v, AEDesc *p_itself)
{
	if (!AEDesc_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "AEDesc required");
		return 0;
	}
	*p_itself = ((AEDescObject *)v)->ob_itself;
	return 1;
}

static void AEDesc_dealloc(AEDescObject *self)
{
	AEDisposeDesc(&self->ob_itself);
	PyMem_DEL(self);
}

static PyObject *AEDesc_AECoerceDesc(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	DescType toType;
	AEDesc result;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &toType))
		return NULL;
	_err = AECoerceDesc(&_self->ob_itself,
	                    toType,
	                    &result);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &result);
	return _res;
}

static PyObject *AEDesc_AEDuplicateDesc(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AEDesc result;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = AEDuplicateDesc(&_self->ob_itself,
	                       &result);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &result);
	return _res;
}

static PyObject *AEDesc_AECountItems(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long theCount;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = AECountItems(&_self->ob_itself,
	                    &theCount);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     theCount);
	return _res;
}

static PyObject *AEDesc_AEPutPtr(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long index;
	DescType typeCode;
	char *dataPtr__in__;
	long dataPtr__len__;
	int dataPtr__in_len__;
	if (!PyArg_ParseTuple(_args, "lO&s#",
	                      &index,
	                      PyMac_GetOSType, &typeCode,
	                      &dataPtr__in__, &dataPtr__in_len__))
		return NULL;
	dataPtr__len__ = dataPtr__in_len__;
	_err = AEPutPtr(&_self->ob_itself,
	                index,
	                typeCode,
	                dataPtr__in__, dataPtr__len__);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
 dataPtr__error__: ;
	return _res;
}

static PyObject *AEDesc_AEPutDesc(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long index;
	AEDesc theAEDesc;
	if (!PyArg_ParseTuple(_args, "lO&",
	                      &index,
	                      AEDesc_Convert, &theAEDesc))
		return NULL;
	_err = AEPutDesc(&_self->ob_itself,
	                 index,
	                 &theAEDesc);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *AEDesc_AEGetNthPtr(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long index;
	DescType desiredType;
	AEKeyword theAEKeyword;
	DescType typeCode;
	char *dataPtr__out__;
	long dataPtr__len__;
	int dataPtr__in_len__;
	if (!PyArg_ParseTuple(_args, "lO&i",
	                      &index,
	                      PyMac_GetOSType, &desiredType,
	                      &dataPtr__in_len__))
		return NULL;
	if ((dataPtr__out__ = malloc(dataPtr__in_len__)) == NULL)
	{
		PyErr_NoMemory();
		goto dataPtr__error__;
	}
	dataPtr__len__ = dataPtr__in_len__;
	_err = AEGetNthPtr(&_self->ob_itself,
	                   index,
	                   desiredType,
	                   &theAEKeyword,
	                   &typeCode,
	                   dataPtr__out__, dataPtr__len__, &dataPtr__len__);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&O&s#",
	                     PyMac_BuildOSType, theAEKeyword,
	                     PyMac_BuildOSType, typeCode,
	                     dataPtr__out__, (int)dataPtr__len__);
	free(dataPtr__out__);
 dataPtr__error__: ;
	return _res;
}

static PyObject *AEDesc_AEGetNthDesc(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long index;
	DescType desiredType;
	AEKeyword theAEKeyword;
	AEDesc result;
	if (!PyArg_ParseTuple(_args, "lO&",
	                      &index,
	                      PyMac_GetOSType, &desiredType))
		return NULL;
	_err = AEGetNthDesc(&_self->ob_itself,
	                    index,
	                    desiredType,
	                    &theAEKeyword,
	                    &result);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&O&",
	                     PyMac_BuildOSType, theAEKeyword,
	                     AEDesc_New, &result);
	return _res;
}

static PyObject *AEDesc_AESizeOfNthItem(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long index;
	DescType typeCode;
	Size dataSize;
	if (!PyArg_ParseTuple(_args, "l",
	                      &index))
		return NULL;
	_err = AESizeOfNthItem(&_self->ob_itself,
	                       index,
	                       &typeCode,
	                       &dataSize);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&l",
	                     PyMac_BuildOSType, typeCode,
	                     dataSize);
	return _res;
}

static PyObject *AEDesc_AEDeleteItem(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long index;
	if (!PyArg_ParseTuple(_args, "l",
	                      &index))
		return NULL;
	_err = AEDeleteItem(&_self->ob_itself,
	                    index);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *AEDesc_AEPutParamPtr(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AEKeyword theAEKeyword;
	DescType typeCode;
	char *dataPtr__in__;
	long dataPtr__len__;
	int dataPtr__in_len__;
	if (!PyArg_ParseTuple(_args, "O&O&s#",
	                      PyMac_GetOSType, &theAEKeyword,
	                      PyMac_GetOSType, &typeCode,
	                      &dataPtr__in__, &dataPtr__in_len__))
		return NULL;
	dataPtr__len__ = dataPtr__in_len__;
	_err = AEPutParamPtr(&_self->ob_itself,
	                     theAEKeyword,
	                     typeCode,
	                     dataPtr__in__, dataPtr__len__);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
 dataPtr__error__: ;
	return _res;
}

static PyObject *AEDesc_AEPutParamDesc(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AEKeyword theAEKeyword;
	AEDesc theAEDesc;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetOSType, &theAEKeyword,
	                      AEDesc_Convert, &theAEDesc))
		return NULL;
	_err = AEPutParamDesc(&_self->ob_itself,
	                      theAEKeyword,
	                      &theAEDesc);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *AEDesc_AEGetParamPtr(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AEKeyword theAEKeyword;
	DescType desiredType;
	DescType typeCode;
	char *dataPtr__out__;
	long dataPtr__len__;
	int dataPtr__in_len__;
	if (!PyArg_ParseTuple(_args, "O&O&i",
	                      PyMac_GetOSType, &theAEKeyword,
	                      PyMac_GetOSType, &desiredType,
	                      &dataPtr__in_len__))
		return NULL;
	if ((dataPtr__out__ = malloc(dataPtr__in_len__)) == NULL)
	{
		PyErr_NoMemory();
		goto dataPtr__error__;
	}
	dataPtr__len__ = dataPtr__in_len__;
	_err = AEGetParamPtr(&_self->ob_itself,
	                     theAEKeyword,
	                     desiredType,
	                     &typeCode,
	                     dataPtr__out__, dataPtr__len__, &dataPtr__len__);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&s#",
	                     PyMac_BuildOSType, typeCode,
	                     dataPtr__out__, (int)dataPtr__len__);
	free(dataPtr__out__);
 dataPtr__error__: ;
	return _res;
}

static PyObject *AEDesc_AEGetParamDesc(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AEKeyword theAEKeyword;
	DescType desiredType;
	AEDesc result;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetOSType, &theAEKeyword,
	                      PyMac_GetOSType, &desiredType))
		return NULL;
	_err = AEGetParamDesc(&_self->ob_itself,
	                      theAEKeyword,
	                      desiredType,
	                      &result);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &result);
	return _res;
}

static PyObject *AEDesc_AESizeOfParam(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AEKeyword theAEKeyword;
	DescType typeCode;
	Size dataSize;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &theAEKeyword))
		return NULL;
	_err = AESizeOfParam(&_self->ob_itself,
	                     theAEKeyword,
	                     &typeCode,
	                     &dataSize);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&l",
	                     PyMac_BuildOSType, typeCode,
	                     dataSize);
	return _res;
}

static PyObject *AEDesc_AEDeleteParam(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AEKeyword theAEKeyword;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &theAEKeyword))
		return NULL;
	_err = AEDeleteParam(&_self->ob_itself,
	                     theAEKeyword);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *AEDesc_AEGetAttributePtr(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AEKeyword theAEKeyword;
	DescType desiredType;
	DescType typeCode;
	char *dataPtr__out__;
	long dataPtr__len__;
	int dataPtr__in_len__;
	if (!PyArg_ParseTuple(_args, "O&O&i",
	                      PyMac_GetOSType, &theAEKeyword,
	                      PyMac_GetOSType, &desiredType,
	                      &dataPtr__in_len__))
		return NULL;
	if ((dataPtr__out__ = malloc(dataPtr__in_len__)) == NULL)
	{
		PyErr_NoMemory();
		goto dataPtr__error__;
	}
	dataPtr__len__ = dataPtr__in_len__;
	_err = AEGetAttributePtr(&_self->ob_itself,
	                         theAEKeyword,
	                         desiredType,
	                         &typeCode,
	                         dataPtr__out__, dataPtr__len__, &dataPtr__len__);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&s#",
	                     PyMac_BuildOSType, typeCode,
	                     dataPtr__out__, (int)dataPtr__len__);
	free(dataPtr__out__);
 dataPtr__error__: ;
	return _res;
}

static PyObject *AEDesc_AEGetAttributeDesc(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AEKeyword theAEKeyword;
	DescType desiredType;
	AEDesc result;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetOSType, &theAEKeyword,
	                      PyMac_GetOSType, &desiredType))
		return NULL;
	_err = AEGetAttributeDesc(&_self->ob_itself,
	                          theAEKeyword,
	                          desiredType,
	                          &result);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &result);
	return _res;
}

static PyObject *AEDesc_AESizeOfAttribute(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AEKeyword theAEKeyword;
	DescType typeCode;
	Size dataSize;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &theAEKeyword))
		return NULL;
	_err = AESizeOfAttribute(&_self->ob_itself,
	                         theAEKeyword,
	                         &typeCode,
	                         &dataSize);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&l",
	                     PyMac_BuildOSType, typeCode,
	                     dataSize);
	return _res;
}

static PyObject *AEDesc_AEPutAttributePtr(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AEKeyword theAEKeyword;
	DescType typeCode;
	char *dataPtr__in__;
	long dataPtr__len__;
	int dataPtr__in_len__;
	if (!PyArg_ParseTuple(_args, "O&O&s#",
	                      PyMac_GetOSType, &theAEKeyword,
	                      PyMac_GetOSType, &typeCode,
	                      &dataPtr__in__, &dataPtr__in_len__))
		return NULL;
	dataPtr__len__ = dataPtr__in_len__;
	_err = AEPutAttributePtr(&_self->ob_itself,
	                         theAEKeyword,
	                         typeCode,
	                         dataPtr__in__, dataPtr__len__);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
 dataPtr__error__: ;
	return _res;
}

static PyObject *AEDesc_AEPutAttributeDesc(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AEKeyword theAEKeyword;
	AEDesc theAEDesc;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetOSType, &theAEKeyword,
	                      AEDesc_Convert, &theAEDesc))
		return NULL;
	_err = AEPutAttributeDesc(&_self->ob_itself,
	                          theAEKeyword,
	                          &theAEDesc);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

#if TARGET_API_MAC_CARBON

static PyObject *AEDesc_AEGetDescDataSize(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Size _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = AEGetDescDataSize(&_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}
#endif

static PyObject *AEDesc_AESend(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AppleEvent reply;
	AESendMode sendMode;
	AESendPriority sendPriority;
	long timeOutInTicks;
	if (!PyArg_ParseTuple(_args, "lhl",
	                      &sendMode,
	                      &sendPriority,
	                      &timeOutInTicks))
		return NULL;
	_err = AESend(&_self->ob_itself,
	              &reply,
	              sendMode,
	              sendPriority,
	              timeOutInTicks,
	              upp_AEIdleProc,
	              (AEFilterUPP)0);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &reply);
	return _res;
}

static PyObject *AEDesc_AEResetTimer(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = AEResetTimer(&_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *AEDesc_AESuspendTheCurrentEvent(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = AESuspendTheCurrentEvent(&_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *AEDesc_AEResumeTheCurrentEvent(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AppleEvent reply;
	AEEventHandlerUPP dispatcher__proc__ = upp_GenericEventHandler;
	PyObject *dispatcher;
	if (!PyArg_ParseTuple(_args, "O&O",
	                      AEDesc_Convert, &reply,
	                      &dispatcher))
		return NULL;
	_err = AEResumeTheCurrentEvent(&_self->ob_itself,
	                               &reply,
	                               dispatcher__proc__, (long)dispatcher);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	Py_INCREF(dispatcher); /* XXX leak, but needed */
	return _res;
}

static PyObject *AEDesc_AEGetTheCurrentEvent(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = AEGetTheCurrentEvent(&_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *AEDesc_AESetTheCurrentEvent(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = AESetTheCurrentEvent(&_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *AEDesc_AEResolve(AEDescObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short callbackFlags;
	AEDesc theToken;
	if (!PyArg_ParseTuple(_args, "h",
	                      &callbackFlags))
		return NULL;
	_err = AEResolve(&_self->ob_itself,
	                 callbackFlags,
	                 &theToken);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &theToken);
	return _res;
}

static PyMethodDef AEDesc_methods[] = {
	{"AECoerceDesc", (PyCFunction)AEDesc_AECoerceDesc, 1,
	 "(DescType toType) -> (AEDesc result)"},
	{"AEDuplicateDesc", (PyCFunction)AEDesc_AEDuplicateDesc, 1,
	 "() -> (AEDesc result)"},
	{"AECountItems", (PyCFunction)AEDesc_AECountItems, 1,
	 "() -> (long theCount)"},
	{"AEPutPtr", (PyCFunction)AEDesc_AEPutPtr, 1,
	 "(long index, DescType typeCode, Buffer dataPtr) -> None"},
	{"AEPutDesc", (PyCFunction)AEDesc_AEPutDesc, 1,
	 "(long index, AEDesc theAEDesc) -> None"},
	{"AEGetNthPtr", (PyCFunction)AEDesc_AEGetNthPtr, 1,
	 "(long index, DescType desiredType, Buffer dataPtr) -> (AEKeyword theAEKeyword, DescType typeCode, Buffer dataPtr)"},
	{"AEGetNthDesc", (PyCFunction)AEDesc_AEGetNthDesc, 1,
	 "(long index, DescType desiredType) -> (AEKeyword theAEKeyword, AEDesc result)"},
	{"AESizeOfNthItem", (PyCFunction)AEDesc_AESizeOfNthItem, 1,
	 "(long index) -> (DescType typeCode, Size dataSize)"},
	{"AEDeleteItem", (PyCFunction)AEDesc_AEDeleteItem, 1,
	 "(long index) -> None"},
	{"AEPutParamPtr", (PyCFunction)AEDesc_AEPutParamPtr, 1,
	 "(AEKeyword theAEKeyword, DescType typeCode, Buffer dataPtr) -> None"},
	{"AEPutParamDesc", (PyCFunction)AEDesc_AEPutParamDesc, 1,
	 "(AEKeyword theAEKeyword, AEDesc theAEDesc) -> None"},
	{"AEGetParamPtr", (PyCFunction)AEDesc_AEGetParamPtr, 1,
	 "(AEKeyword theAEKeyword, DescType desiredType, Buffer dataPtr) -> (DescType typeCode, Buffer dataPtr)"},
	{"AEGetParamDesc", (PyCFunction)AEDesc_AEGetParamDesc, 1,
	 "(AEKeyword theAEKeyword, DescType desiredType) -> (AEDesc result)"},
	{"AESizeOfParam", (PyCFunction)AEDesc_AESizeOfParam, 1,
	 "(AEKeyword theAEKeyword) -> (DescType typeCode, Size dataSize)"},
	{"AEDeleteParam", (PyCFunction)AEDesc_AEDeleteParam, 1,
	 "(AEKeyword theAEKeyword) -> None"},
	{"AEGetAttributePtr", (PyCFunction)AEDesc_AEGetAttributePtr, 1,
	 "(AEKeyword theAEKeyword, DescType desiredType, Buffer dataPtr) -> (DescType typeCode, Buffer dataPtr)"},
	{"AEGetAttributeDesc", (PyCFunction)AEDesc_AEGetAttributeDesc, 1,
	 "(AEKeyword theAEKeyword, DescType desiredType) -> (AEDesc result)"},
	{"AESizeOfAttribute", (PyCFunction)AEDesc_AESizeOfAttribute, 1,
	 "(AEKeyword theAEKeyword) -> (DescType typeCode, Size dataSize)"},
	{"AEPutAttributePtr", (PyCFunction)AEDesc_AEPutAttributePtr, 1,
	 "(AEKeyword theAEKeyword, DescType typeCode, Buffer dataPtr) -> None"},
	{"AEPutAttributeDesc", (PyCFunction)AEDesc_AEPutAttributeDesc, 1,
	 "(AEKeyword theAEKeyword, AEDesc theAEDesc) -> None"},

#if TARGET_API_MAC_CARBON
	{"AEGetDescDataSize", (PyCFunction)AEDesc_AEGetDescDataSize, 1,
	 "() -> (Size _rv)"},
#endif
	{"AESend", (PyCFunction)AEDesc_AESend, 1,
	 "(AESendMode sendMode, AESendPriority sendPriority, long timeOutInTicks) -> (AppleEvent reply)"},
	{"AEResetTimer", (PyCFunction)AEDesc_AEResetTimer, 1,
	 "() -> None"},
	{"AESuspendTheCurrentEvent", (PyCFunction)AEDesc_AESuspendTheCurrentEvent, 1,
	 "() -> None"},
	{"AEResumeTheCurrentEvent", (PyCFunction)AEDesc_AEResumeTheCurrentEvent, 1,
	 "(AppleEvent reply, EventHandler dispatcher) -> None"},
	{"AEGetTheCurrentEvent", (PyCFunction)AEDesc_AEGetTheCurrentEvent, 1,
	 "() -> None"},
	{"AESetTheCurrentEvent", (PyCFunction)AEDesc_AESetTheCurrentEvent, 1,
	 "() -> None"},
	{"AEResolve", (PyCFunction)AEDesc_AEResolve, 1,
	 "(short callbackFlags) -> (AEDesc theToken)"},
	{NULL, NULL, 0}
};

PyMethodChain AEDesc_chain = { AEDesc_methods, NULL };

static PyObject *AEDesc_getattr(AEDescObject *self, char *name)
{

	if (strcmp(name, "type") == 0)
		return PyMac_BuildOSType(self->ob_itself.descriptorType);
	if (strcmp(name, "data") == 0) {
		PyObject *res;
#if !TARGET_API_MAC_CARBON
		char state;
		state = HGetState(self->ob_itself.dataHandle);
		HLock(self->ob_itself.dataHandle);
		res = PyString_FromStringAndSize(
			*self->ob_itself.dataHandle,
			GetHandleSize(self->ob_itself.dataHandle));
		HUnlock(self->ob_itself.dataHandle);
		HSetState(self->ob_itself.dataHandle, state);
#else
		Size size;
		char *ptr;
		OSErr err;
		
		size = AEGetDescDataSize(&self->ob_itself);
		if ( (res = PyString_FromStringAndSize(NULL, size)) == NULL )
			return NULL;
		if ( (ptr = PyString_AsString(res)) == NULL )
			return NULL;
		if ( (err=AEGetDescData(&self->ob_itself, ptr, size)) < 0 )
			return PyMac_Error(err);	
#endif
		return res;
	}
	if (strcmp(name, "__members__") == 0)
		return Py_BuildValue("[ss]", "data", "type");

	return Py_FindMethodInChain(&AEDesc_chain, (PyObject *)self, name);
}

#define AEDesc_setattr NULL

#define AEDesc_compare NULL

#define AEDesc_repr NULL

#define AEDesc_hash NULL

PyTypeObject AEDesc_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"AEDesc", /*tp_name*/
	sizeof(AEDescObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) AEDesc_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) AEDesc_getattr, /*tp_getattr*/
	(setattrfunc) AEDesc_setattr, /*tp_setattr*/
	(cmpfunc) AEDesc_compare, /*tp_compare*/
	(reprfunc) AEDesc_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) AEDesc_hash, /*tp_hash*/
};

/* --------------------- End object type AEDesc --------------------- */


static PyObject *AE_AECoercePtr(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	DescType typeCode;
	char *dataPtr__in__;
	long dataPtr__len__;
	int dataPtr__in_len__;
	DescType toType;
	AEDesc result;
	if (!PyArg_ParseTuple(_args, "O&s#O&",
	                      PyMac_GetOSType, &typeCode,
	                      &dataPtr__in__, &dataPtr__in_len__,
	                      PyMac_GetOSType, &toType))
		return NULL;
	dataPtr__len__ = dataPtr__in_len__;
	_err = AECoercePtr(typeCode,
	                   dataPtr__in__, dataPtr__len__,
	                   toType,
	                   &result);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &result);
 dataPtr__error__: ;
	return _res;
}

static PyObject *AE_AECreateDesc(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	DescType typeCode;
	char *dataPtr__in__;
	long dataPtr__len__;
	int dataPtr__in_len__;
	AEDesc result;
	if (!PyArg_ParseTuple(_args, "O&s#",
	                      PyMac_GetOSType, &typeCode,
	                      &dataPtr__in__, &dataPtr__in_len__))
		return NULL;
	dataPtr__len__ = dataPtr__in_len__;
	_err = AECreateDesc(typeCode,
	                    dataPtr__in__, dataPtr__len__,
	                    &result);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &result);
 dataPtr__error__: ;
	return _res;
}

static PyObject *AE_AECreateList(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	char *factoringPtr__in__;
	long factoringPtr__len__;
	int factoringPtr__in_len__;
	Boolean isRecord;
	AEDescList resultList;
	if (!PyArg_ParseTuple(_args, "s#b",
	                      &factoringPtr__in__, &factoringPtr__in_len__,
	                      &isRecord))
		return NULL;
	factoringPtr__len__ = factoringPtr__in_len__;
	_err = AECreateList(factoringPtr__in__, factoringPtr__len__,
	                    isRecord,
	                    &resultList);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &resultList);
 factoringPtr__error__: ;
	return _res;
}

static PyObject *AE_AECreateAppleEvent(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AEEventClass theAEEventClass;
	AEEventID theAEEventID;
	AEAddressDesc target;
	AEReturnID returnID;
	AETransactionID transactionID;
	AppleEvent result;
	if (!PyArg_ParseTuple(_args, "O&O&O&hl",
	                      PyMac_GetOSType, &theAEEventClass,
	                      PyMac_GetOSType, &theAEEventID,
	                      AEDesc_Convert, &target,
	                      &returnID,
	                      &transactionID))
		return NULL;
	_err = AECreateAppleEvent(theAEEventClass,
	                          theAEEventID,
	                          &target,
	                          returnID,
	                          transactionID,
	                          &result);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &result);
	return _res;
}

#if TARGET_API_MAC_CARBON

static PyObject *AE_AEReplaceDescData(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	DescType typeCode;
	char *dataPtr__in__;
	long dataPtr__len__;
	int dataPtr__in_len__;
	AEDesc theAEDesc;
	if (!PyArg_ParseTuple(_args, "O&s#",
	                      PyMac_GetOSType, &typeCode,
	                      &dataPtr__in__, &dataPtr__in_len__))
		return NULL;
	dataPtr__len__ = dataPtr__in_len__;
	_err = AEReplaceDescData(typeCode,
	                         dataPtr__in__, dataPtr__len__,
	                         &theAEDesc);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &theAEDesc);
 dataPtr__error__: ;
	return _res;
}
#endif

static PyObject *AE_AEProcessAppleEvent(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	EventRecord theEventRecord;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetEventRecord, &theEventRecord))
		return NULL;
	_err = AEProcessAppleEvent(&theEventRecord);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *AE_AEGetInteractionAllowed(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AEInteractAllowed level;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = AEGetInteractionAllowed(&level);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("b",
	                     level);
	return _res;
}

static PyObject *AE_AESetInteractionAllowed(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AEInteractAllowed level;
	if (!PyArg_ParseTuple(_args, "b",
	                      &level))
		return NULL;
	_err = AESetInteractionAllowed(level);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *AE_AEInteractWithUser(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long timeOutInTicks;
	if (!PyArg_ParseTuple(_args, "l",
	                      &timeOutInTicks))
		return NULL;
	_err = AEInteractWithUser(timeOutInTicks,
	                          (NMRecPtr)0,
	                          upp_AEIdleProc);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *AE_AEInstallEventHandler(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AEEventClass theAEEventClass;
	AEEventID theAEEventID;
	AEEventHandlerUPP handler__proc__ = upp_GenericEventHandler;
	PyObject *handler;
	if (!PyArg_ParseTuple(_args, "O&O&O",
	                      PyMac_GetOSType, &theAEEventClass,
	                      PyMac_GetOSType, &theAEEventID,
	                      &handler))
		return NULL;
	_err = AEInstallEventHandler(theAEEventClass,
	                             theAEEventID,
	                             handler__proc__, (long)handler,
	                             0);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	Py_INCREF(handler); /* XXX leak, but needed */
	return _res;
}

static PyObject *AE_AERemoveEventHandler(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AEEventClass theAEEventClass;
	AEEventID theAEEventID;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetOSType, &theAEEventClass,
	                      PyMac_GetOSType, &theAEEventID))
		return NULL;
	_err = AERemoveEventHandler(theAEEventClass,
	                            theAEEventID,
	                            upp_GenericEventHandler,
	                            0);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *AE_AEGetEventHandler(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AEEventClass theAEEventClass;
	AEEventID theAEEventID;
	AEEventHandlerUPP handler__proc__ = upp_GenericEventHandler;
	PyObject *handler;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetOSType, &theAEEventClass,
	                      PyMac_GetOSType, &theAEEventID))
		return NULL;
	_err = AEGetEventHandler(theAEEventClass,
	                         theAEEventID,
	                         &handler__proc__, (long *)&handler,
	                         0);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O",
	                     handler);
	Py_INCREF(handler); /* XXX leak, but needed */
	return _res;
}

static PyObject *AE_AEInstallSpecialHandler(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AEKeyword functionClass;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &functionClass))
		return NULL;
	_err = AEInstallSpecialHandler(functionClass,
	                               upp_GenericEventHandler,
	                               0);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *AE_AERemoveSpecialHandler(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AEKeyword functionClass;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &functionClass))
		return NULL;
	_err = AERemoveSpecialHandler(functionClass,
	                              upp_GenericEventHandler,
	                              0);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *AE_AEManagerInfo(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AEKeyword keyWord;
	long result;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &keyWord))
		return NULL;
	_err = AEManagerInfo(keyWord,
	                     &result);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     result);
	return _res;
}

static PyObject *AE_AEObjectInit(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = AEObjectInit();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *AE_AEDisposeToken(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	AEDesc theToken;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = AEDisposeToken(&theToken);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &theToken);
	return _res;
}

static PyObject *AE_AECallObjectAccessor(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	DescType desiredClass;
	AEDesc containerToken;
	DescType containerClass;
	DescType keyForm;
	AEDesc keyData;
	AEDesc token;
	if (!PyArg_ParseTuple(_args, "O&O&O&O&O&",
	                      PyMac_GetOSType, &desiredClass,
	                      AEDesc_Convert, &containerToken,
	                      PyMac_GetOSType, &containerClass,
	                      PyMac_GetOSType, &keyForm,
	                      AEDesc_Convert, &keyData))
		return NULL;
	_err = AECallObjectAccessor(desiredClass,
	                            &containerToken,
	                            containerClass,
	                            keyForm,
	                            &keyData,
	                            &token);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     AEDesc_New, &token);
	return _res;
}

static PyMethodDef AE_methods[] = {
	{"AECoercePtr", (PyCFunction)AE_AECoercePtr, 1,
	 "(DescType typeCode, Buffer dataPtr, DescType toType) -> (AEDesc result)"},
	{"AECreateDesc", (PyCFunction)AE_AECreateDesc, 1,
	 "(DescType typeCode, Buffer dataPtr) -> (AEDesc result)"},
	{"AECreateList", (PyCFunction)AE_AECreateList, 1,
	 "(Buffer factoringPtr, Boolean isRecord) -> (AEDescList resultList)"},
	{"AECreateAppleEvent", (PyCFunction)AE_AECreateAppleEvent, 1,
	 "(AEEventClass theAEEventClass, AEEventID theAEEventID, AEAddressDesc target, AEReturnID returnID, AETransactionID transactionID) -> (AppleEvent result)"},

#if TARGET_API_MAC_CARBON
	{"AEReplaceDescData", (PyCFunction)AE_AEReplaceDescData, 1,
	 "(DescType typeCode, Buffer dataPtr) -> (AEDesc theAEDesc)"},
#endif
	{"AEProcessAppleEvent", (PyCFunction)AE_AEProcessAppleEvent, 1,
	 "(EventRecord theEventRecord) -> None"},
	{"AEGetInteractionAllowed", (PyCFunction)AE_AEGetInteractionAllowed, 1,
	 "() -> (AEInteractAllowed level)"},
	{"AESetInteractionAllowed", (PyCFunction)AE_AESetInteractionAllowed, 1,
	 "(AEInteractAllowed level) -> None"},
	{"AEInteractWithUser", (PyCFunction)AE_AEInteractWithUser, 1,
	 "(long timeOutInTicks) -> None"},
	{"AEInstallEventHandler", (PyCFunction)AE_AEInstallEventHandler, 1,
	 "(AEEventClass theAEEventClass, AEEventID theAEEventID, EventHandler handler) -> None"},
	{"AERemoveEventHandler", (PyCFunction)AE_AERemoveEventHandler, 1,
	 "(AEEventClass theAEEventClass, AEEventID theAEEventID) -> None"},
	{"AEGetEventHandler", (PyCFunction)AE_AEGetEventHandler, 1,
	 "(AEEventClass theAEEventClass, AEEventID theAEEventID) -> (EventHandler handler)"},
	{"AEInstallSpecialHandler", (PyCFunction)AE_AEInstallSpecialHandler, 1,
	 "(AEKeyword functionClass) -> None"},
	{"AERemoveSpecialHandler", (PyCFunction)AE_AERemoveSpecialHandler, 1,
	 "(AEKeyword functionClass) -> None"},
	{"AEManagerInfo", (PyCFunction)AE_AEManagerInfo, 1,
	 "(AEKeyword keyWord) -> (long result)"},
	{"AEObjectInit", (PyCFunction)AE_AEObjectInit, 1,
	 "() -> None"},
	{"AEDisposeToken", (PyCFunction)AE_AEDisposeToken, 1,
	 "() -> (AEDesc theToken)"},
	{"AECallObjectAccessor", (PyCFunction)AE_AECallObjectAccessor, 1,
	 "(DescType desiredClass, AEDesc containerToken, DescType containerClass, DescType keyForm, AEDesc keyData) -> (AEDesc token)"},
	{NULL, NULL, 0}
};


#if UNIVERSAL_INTERFACES_VERSION >= 0x0340
typedef long refcontype;
#else
typedef unsigned long refcontype;
#endif

static pascal OSErr
GenericEventHandler(const AppleEvent *request, AppleEvent *reply, refcontype refcon)
{
	PyObject *handler = (PyObject *)refcon;
	AEDescObject *requestObject, *replyObject;
	PyObject *args, *res;
	if ((requestObject = (AEDescObject *)AEDesc_New((AppleEvent *)request)) == NULL) {
		return -1;
	}
	if ((replyObject = (AEDescObject *)AEDesc_New(reply)) == NULL) {
		Py_DECREF(requestObject);
		return -1;
	}
	if ((args = Py_BuildValue("OO", requestObject, replyObject)) == NULL) {
		Py_DECREF(requestObject);
		Py_DECREF(replyObject);
		return -1;
	}
	res = PyEval_CallObject(handler, args);
	requestObject->ob_itself.descriptorType = 'null';
	requestObject->ob_itself.dataHandle = NULL;
	replyObject->ob_itself.descriptorType = 'null';
	replyObject->ob_itself.dataHandle = NULL;
	Py_DECREF(args);
	if (res == NULL)
		return -1;
	Py_DECREF(res);
	return noErr;
}


void initAE(void)
{
	PyObject *m;
	PyObject *d;



		upp_AEIdleProc = NewAEIdleUPP(AEIdleProc);
		upp_GenericEventHandler = NewAEEventHandlerUPP(&GenericEventHandler);
		PyMac_INIT_TOOLBOX_OBJECT_NEW(AEDesc *, AEDesc_New);
		PyMac_INIT_TOOLBOX_OBJECT_CONVERT(AEDesc, AEDesc_Convert);


	m = Py_InitModule("AE", AE_methods);
	d = PyModule_GetDict(m);
	AE_Error = PyMac_GetOSErrException();
	if (AE_Error == NULL ||
	    PyDict_SetItemString(d, "Error", AE_Error) != 0)
		return;
	AEDesc_Type.ob_type = &PyType_Type;
	Py_INCREF(&AEDesc_Type);
	if (PyDict_SetItemString(d, "AEDescType", (PyObject *)&AEDesc_Type) != 0)
		Py_FatalError("can't initialize AEDescType");
}

/* ========================= End module AE ========================== */

