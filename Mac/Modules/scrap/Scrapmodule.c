
/* ========================== Module Scrap ========================== */

#include "Python.h"



#include "macglue.h"
#include "pymactoolbox.h"

#ifdef WITHOUT_FRAMEWORKS
#include <Scrap.h>
#else
#include <Carbon/Carbon.h>
#endif

#if !TARGET_API_MAC_CARBON

/*
** Generate ScrapInfo records
*/
static PyObject *
SCRRec_New(itself)
	ScrapStuff *itself;
{

	return Py_BuildValue("lO&hhO&", itself->scrapSize,
		ResObj_New, itself->scrapHandle, itself->scrapCount, itself->scrapState,
		PyMac_BuildStr255, itself->scrapName);
}
#endif

static PyObject *Scrap_Error;

static PyObject *Scrap_LoadScrap(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = LoadScrap();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Scrap_UnloadScrap(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = UnloadScrap();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

#if !TARGET_API_MAC_CARBON

static PyObject *Scrap_InfoScrap(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	ScrapStuffPtr _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = InfoScrap();
	_res = Py_BuildValue("O&",
	                     SCRRec_New, _rv);
	return _res;
}
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *Scrap_GetScrap(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long _rv;
	Handle destination;
	ScrapFlavorType flavorType;
	SInt32 offset;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      ResObj_Convert, &destination,
	                      PyMac_GetOSType, &flavorType))
		return NULL;
	_rv = GetScrap(destination,
	               flavorType,
	               &offset);
	_res = Py_BuildValue("ll",
	                     _rv,
	                     offset);
	return _res;
}
#endif


static PyObject *Scrap_ZeroScrap(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
#if TARGET_API_MAC_CARBON
	{
		ScrapRef scrap;
		
		_err = ClearCurrentScrap();
		if (_err != noErr) return PyMac_Error(_err);
		_err = GetCurrentScrap(&scrap);
	}
#else
	_err = ZeroScrap();
#endif
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Scrap_PutScrap(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	SInt32 sourceBufferByteCount;
	ScrapFlavorType flavorType;
	char *sourceBuffer__in__;
	int sourceBuffer__len__;
	int sourceBuffer__in_len__;
#if TARGET_API_MAC_CARBON
	ScrapRef scrap;
#endif

	if (!PyArg_ParseTuple(_args, "O&s#",
	                      PyMac_GetOSType, &flavorType,
	                      &sourceBuffer__in__, &sourceBuffer__in_len__))
		return NULL;
	sourceBufferByteCount = sourceBuffer__in_len__;
	sourceBuffer__len__ = sourceBuffer__in_len__;
#if TARGET_API_MAC_CARBON
	_err = GetCurrentScrap(&scrap);
	if (_err != noErr) return PyMac_Error(_err);
	_err = PutScrapFlavor(scrap, flavorType, 0, sourceBufferByteCount, sourceBuffer__in__);
#else
	_err = PutScrap(sourceBufferByteCount,
	                flavorType,
	                sourceBuffer__in__);
#endif
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
 sourceBuffer__error__: ;
	return _res;
}

#if TARGET_API_MAC_CARBON

static PyObject *Scrap_ClearCurrentScrap(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = ClearCurrentScrap();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

#if TARGET_API_MAC_CARBON

static PyObject *Scrap_CallInScrapPromises(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSStatus _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = CallInScrapPromises();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

static PyMethodDef Scrap_methods[] = {
	{"LoadScrap", (PyCFunction)Scrap_LoadScrap, 1,
	 "() -> None"},
	{"UnloadScrap", (PyCFunction)Scrap_UnloadScrap, 1,
	 "() -> None"},

#if !TARGET_API_MAC_CARBON
	{"InfoScrap", (PyCFunction)Scrap_InfoScrap, 1,
	 "() -> (ScrapStuffPtr _rv)"},
#endif

#if !TARGET_API_MAC_CARBON
	{"GetScrap", (PyCFunction)Scrap_GetScrap, 1,
	 "(Handle destination, ScrapFlavorType flavorType) -> (long _rv, SInt32 offset)"},
#endif

	{"ZeroScrap", (PyCFunction)Scrap_ZeroScrap, 1,
	 "() -> None"},

	{"PutScrap", (PyCFunction)Scrap_PutScrap, 1,
	 "(ScrapFlavorType flavorType, Buffer sourceBuffer) -> None"},

#if TARGET_API_MAC_CARBON
	{"ClearCurrentScrap", (PyCFunction)Scrap_ClearCurrentScrap, 1,
	 "() -> None"},
#endif

#if TARGET_API_MAC_CARBON
	{"CallInScrapPromises", (PyCFunction)Scrap_CallInScrapPromises, 1,
	 "() -> None"},
#endif
	{NULL, NULL, 0}
};




void initScrap()
{
	PyObject *m;
	PyObject *d;




	m = Py_InitModule("Scrap", Scrap_methods);
	d = PyModule_GetDict(m);
	Scrap_Error = PyMac_GetOSErrException();
	if (Scrap_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Scrap_Error) != 0)
		return;
}

/* ======================== End module Scrap ======================== */

