
/* ========================= Module _Scrap ========================== */

#include "Python.h"



#ifdef _WIN32
#include "pywintoolbox.h"
#else
#include "macglue.h"
#include "pymactoolbox.h"
#endif

/* Macro to test whether a weak-loaded CFM function exists */
#define PyMac_PRECHECK(rtn) do { if ( &rtn == NULL )  {\
    	PyErr_SetString(PyExc_NotImplementedError, \
    	"Not available in this shared library/OS version"); \
    	return NULL; \
    }} while(0)


#ifdef WITHOUT_FRAMEWORKS
#include <Scrap.h>
#else
#include <Carbon/Carbon.h>
#endif

#if TARGET_API_MAC_OS8

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

#if !TARGET_API_MAC_OS8
/* ----------------------- Object type Scrap ------------------------ */

PyTypeObject Scrap_Type;

#define ScrapObj_Check(x) ((x)->ob_type == &Scrap_Type)

typedef struct ScrapObject {
	PyObject_HEAD
	ScrapRef ob_itself;
} ScrapObject;

PyObject *ScrapObj_New(ScrapRef itself)
{
	ScrapObject *it;
	it = PyObject_NEW(ScrapObject, &Scrap_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}
int ScrapObj_Convert(PyObject *v, ScrapRef *p_itself)
{
	if (!ScrapObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "Scrap required");
		return 0;
	}
	*p_itself = ((ScrapObject *)v)->ob_itself;
	return 1;
}

static void ScrapObj_dealloc(ScrapObject *self)
{
	/* Cleanup of self->ob_itself goes here */
	PyMem_DEL(self);
}

static PyObject *ScrapObj_GetScrapFlavorFlags(ScrapObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	ScrapFlavorType flavorType;
	ScrapFlavorFlags flavorFlags;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &flavorType))
		return NULL;
	_err = GetScrapFlavorFlags(_self->ob_itself,
	                           flavorType,
	                           &flavorFlags);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     flavorFlags);
	return _res;
}

static PyObject *ScrapObj_GetScrapFlavorSize(ScrapObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	ScrapFlavorType flavorType;
	Size byteCount;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &flavorType))
		return NULL;
	_err = GetScrapFlavorSize(_self->ob_itself,
	                          flavorType,
	                          &byteCount);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     byteCount);
	return _res;
}

static PyObject *ScrapObj_GetScrapFlavorData(ScrapObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	ScrapFlavorType flavorType;
	Size byteCount;

	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &flavorType))
		return NULL;
	_err = GetScrapFlavorSize(_self->ob_itself,
	                          flavorType,
	                          &byteCount);
	if (_err != noErr) return PyMac_Error(_err);
	_res = PyString_FromStringAndSize(NULL, (int)byteCount);
	if ( _res == NULL ) return NULL;
	_err = GetScrapFlavorData(_self->ob_itself,
	                          flavorType,
	                          &byteCount,
	                          PyString_AS_STRING(_res));
	if (_err != noErr) {
		Py_XDECREF(_res);
		return PyMac_Error(_err);
	}
 destination__error__: ;
	return _res;
}

static PyObject *ScrapObj_PutScrapFlavor(ScrapObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	ScrapFlavorType flavorType;
	ScrapFlavorFlags flavorFlags;
	char *flavorData__in__;
	int flavorData__in_len__;
	if (!PyArg_ParseTuple(_args, "O&ls#",
	                      PyMac_GetOSType, &flavorType,
	                      &flavorFlags,
	                      &flavorData__in__, &flavorData__in_len__))
		return NULL;
	_err = PutScrapFlavor(_self->ob_itself,
	                      flavorType,
	                      flavorFlags,
	                      (Size)flavorData__in_len__,
	                      flavorData__in__);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
 flavorData__error__: ;
	return _res;
}

static PyObject *ScrapObj_GetScrapFlavorCount(ScrapObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	UInt32 infoCount;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetScrapFlavorCount(_self->ob_itself,
	                           &infoCount);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     infoCount);
	return _res;
}

static PyObject *ScrapObj_GetScrapFlavorInfoList(ScrapObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	PyObject *item;
	OSStatus _err;
	UInt32 infoCount;
	ScrapFlavorInfo *infolist = NULL;
	int i;
	
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetScrapFlavorCount(_self->ob_itself,
	                           &infoCount);
	if (_err != noErr) return PyMac_Error(_err);
	if (infoCount == 0) return Py_BuildValue("[]");
	
	if ((infolist = (ScrapFlavorInfo *)malloc(infoCount*sizeof(ScrapFlavorInfo))) == NULL )
		return PyErr_NoMemory();
	
	_err = GetScrapFlavorInfoList(_self->ob_itself, &infoCount, infolist);
	if (_err != noErr) {
		free(infolist);
		return NULL;
	}
	if ((_res = PyList_New(infoCount)) == NULL ) {
		free(infolist);
		return NULL;
	}
	for(i=0; i<infoCount; i++) {
		item = Py_BuildValue("O&l", PyMac_BuildOSType, infolist[i].flavorType,
			infolist[i].flavorFlags);
		if ( !item || PyList_SetItem(_res, i, item) < 0 ) {
			Py_DECREF(_res);
			free(infolist);
			return NULL;
		}
	}
	free(infolist);
	return _res;
}

static PyMethodDef ScrapObj_methods[] = {
	{"GetScrapFlavorFlags", (PyCFunction)ScrapObj_GetScrapFlavorFlags, 1,
	 "(ScrapFlavorType flavorType) -> (ScrapFlavorFlags flavorFlags)"},
	{"GetScrapFlavorSize", (PyCFunction)ScrapObj_GetScrapFlavorSize, 1,
	 "(ScrapFlavorType flavorType) -> (Size byteCount)"},
	{"GetScrapFlavorData", (PyCFunction)ScrapObj_GetScrapFlavorData, 1,
	 "(ScrapFlavorType flavorType) -> string"},
	{"PutScrapFlavor", (PyCFunction)ScrapObj_PutScrapFlavor, 1,
	 "(ScrapFlavorType flavorType, ScrapFlavorFlags flavorFlags, Buffer flavorData) -> None"},
	{"GetScrapFlavorCount", (PyCFunction)ScrapObj_GetScrapFlavorCount, 1,
	 "() -> (UInt32 infoCount)"},
	{"GetScrapFlavorInfoList", (PyCFunction)ScrapObj_GetScrapFlavorInfoList, 1,
	 "() -> ([(ScrapFlavorType, ScrapFlavorInfo), ...])"},
	{NULL, NULL, 0}
};

PyMethodChain ScrapObj_chain = { ScrapObj_methods, NULL };

static PyObject *ScrapObj_getattr(ScrapObject *self, char *name)
{
	return Py_FindMethodInChain(&ScrapObj_chain, (PyObject *)self, name);
}

#define ScrapObj_setattr NULL

#define ScrapObj_compare NULL

#define ScrapObj_repr NULL

#define ScrapObj_hash NULL

PyTypeObject Scrap_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"_Scrap.Scrap", /*tp_name*/
	sizeof(ScrapObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) ScrapObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) ScrapObj_getattr, /*tp_getattr*/
	(setattrfunc) ScrapObj_setattr, /*tp_setattr*/
	(cmpfunc) ScrapObj_compare, /*tp_compare*/
	(reprfunc) ScrapObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) ScrapObj_hash, /*tp_hash*/
};

/* --------------------- End object type Scrap ---------------------- */
#endif /* !TARGET_API_MAC_OS8 */

static PyObject *Scrap_LoadScrap(PyObject *_self, PyObject *_args)
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

static PyObject *Scrap_UnloadScrap(PyObject *_self, PyObject *_args)
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

#if TARGET_API_MAC_OS8

static PyObject *Scrap_InfoScrap(PyObject *_self, PyObject *_args)
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

static PyObject *Scrap_GetScrap(PyObject *_self, PyObject *_args)
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

static PyObject *Scrap_ZeroScrap(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = ZeroScrap();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Scrap_PutScrap(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	SInt32 sourceBufferByteCount;
	ScrapFlavorType flavorType;
	char *sourceBuffer__in__;
	int sourceBuffer__len__;
	int sourceBuffer__in_len__;
	if (!PyArg_ParseTuple(_args, "lO&s#",
	                      &sourceBufferByteCount,
	                      PyMac_GetOSType, &flavorType,
	                      &sourceBuffer__in__, &sourceBuffer__in_len__))
		return NULL;
	_err = PutScrap(sourceBufferByteCount,
	                flavorType,
	                sourceBuffer__in__);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
 sourceBuffer__error__: ;
	return _res;
}
#endif /* TARGET_API_MAC_OS8 */

#if !TARGET_API_MAC_OS8
static PyObject *Scrap_GetCurrentScrap(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	ScrapRef scrap;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetCurrentScrap(&scrap);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ScrapObj_New, scrap);
	return _res;
}

static PyObject *Scrap_ClearCurrentScrap(PyObject *_self, PyObject *_args)
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

static PyObject *Scrap_CallInScrapPromises(PyObject *_self, PyObject *_args)
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

#if TARGET_API_MAC_OS8
	{"InfoScrap", (PyCFunction)Scrap_InfoScrap, 1,
	 "() -> (ScrapStuffPtr _rv)"},
	{"GetScrap", (PyCFunction)Scrap_GetScrap, 1,
	 "(Handle destination, ScrapFlavorType flavorType) -> (long _rv, SInt32 offset)"},
	{"ZeroScrap", (PyCFunction)Scrap_ZeroScrap, 1,
	 "() -> None"},
	{"PutScrap", (PyCFunction)Scrap_PutScrap, 1,
	 "(SInt32 sourceBufferByteCount, ScrapFlavorType flavorType, Buffer sourceBuffer) -> None"},
#endif

#if !TARGET_API_MAC_OS8
	{"GetCurrentScrap", (PyCFunction)Scrap_GetCurrentScrap, 1,
	 "() -> (ScrapRef scrap)"},
	{"ClearCurrentScrap", (PyCFunction)Scrap_ClearCurrentScrap, 1,
	 "() -> None"},
	{"CallInScrapPromises", (PyCFunction)Scrap_CallInScrapPromises, 1,
	 "() -> None"},
#endif
	{NULL, NULL, 0}
};




void init_Scrap(void)
{
	PyObject *m;
	PyObject *d;




	m = Py_InitModule("_Scrap", Scrap_methods);
	d = PyModule_GetDict(m);
	Scrap_Error = PyMac_GetOSErrException();
	if (Scrap_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Scrap_Error) != 0)
		return;
#if !TARGET_API_MAC_OS8
	Scrap_Type.ob_type = &PyType_Type;
	Py_INCREF(&Scrap_Type);
	if (PyDict_SetItemString(d, "ScrapType", (PyObject *)&Scrap_Type) != 0)
		Py_FatalError("can't initialize ScrapType");
#endif
}

/* ======================= End module _Scrap ======================== */

