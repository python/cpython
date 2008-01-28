/*****************************************************************
  This file should be kept compatible with Python 2.3, see PEP 291.
 *****************************************************************/

#include "Python.h"
#include "compile.h" /* required only for 2.3, as it seems */
#include "frameobject.h"

#include <ffi.h>
#ifdef MS_WIN32
#include <windows.h>
#endif
#include "ctypes.h"

static void
PrintError(char *msg, ...)
{
	char buf[512];
	PyObject *f = PySys_GetObject("stderr");
	va_list marker;

	va_start(marker, msg);
	vsnprintf(buf, sizeof(buf), msg, marker);
	va_end(marker);
	if (f)
		PyFile_WriteString(buf, f);
	PyErr_Print();
}


/* after code that pyrex generates */
void _AddTraceback(char *funcname, char *filename, int lineno)
{
	PyObject *py_srcfile = 0;
	PyObject *py_funcname = 0;
	PyObject *py_globals = 0;
	PyObject *empty_tuple = 0;
	PyObject *empty_string = 0;
	PyCodeObject *py_code = 0;
	PyFrameObject *py_frame = 0;
    
	py_srcfile = PyString_FromString(filename);
	if (!py_srcfile) goto bad;
	py_funcname = PyString_FromString(funcname);
	if (!py_funcname) goto bad;
	py_globals = PyDict_New();
	if (!py_globals) goto bad;
	empty_tuple = PyTuple_New(0);
	if (!empty_tuple) goto bad;
	empty_string = PyString_FromString("");
	if (!empty_string) goto bad;
	py_code = PyCode_New(
		0,            /*int argcount,*/
		0,            /*int nlocals,*/
		0,            /*int stacksize,*/
		0,            /*int flags,*/
		empty_string, /*PyObject *code,*/
		empty_tuple,  /*PyObject *consts,*/
		empty_tuple,  /*PyObject *names,*/
		empty_tuple,  /*PyObject *varnames,*/
		empty_tuple,  /*PyObject *freevars,*/
		empty_tuple,  /*PyObject *cellvars,*/
		py_srcfile,   /*PyObject *filename,*/
		py_funcname,  /*PyObject *name,*/
		lineno,   /*int firstlineno,*/
		empty_string  /*PyObject *lnotab*/
		);
	if (!py_code) goto bad;
	py_frame = PyFrame_New(
		PyThreadState_Get(), /*PyThreadState *tstate,*/
		py_code,             /*PyCodeObject *code,*/
		py_globals,          /*PyObject *globals,*/
		0                    /*PyObject *locals*/
		);
	if (!py_frame) goto bad;
	py_frame->f_lineno = lineno;
	PyTraceBack_Here(py_frame);
  bad:
	Py_XDECREF(py_globals);
	Py_XDECREF(py_srcfile);
	Py_XDECREF(py_funcname);
	Py_XDECREF(empty_tuple);
	Py_XDECREF(empty_string);
	Py_XDECREF(py_code);
	Py_XDECREF(py_frame);
}

#ifdef MS_WIN32
/*
 * We must call AddRef() on non-NULL COM pointers we receive as arguments
 * to callback functions - these functions are COM method implementations.
 * The Python instances we create have a __del__ method which calls Release().
 *
 * The presence of a class attribute named '_needs_com_addref_' triggers this
 * behaviour.  It would also be possible to call the AddRef() Python method,
 * after checking for PyObject_IsTrue(), but this would probably be somewhat
 * slower.
 */
static void
TryAddRef(StgDictObject *dict, CDataObject *obj)
{
	IUnknown *punk;

	if (NULL == PyDict_GetItemString((PyObject *)dict, "_needs_com_addref_"))
		return;

	punk = *(IUnknown **)obj->b_ptr;
	if (punk)
		punk->lpVtbl->AddRef(punk);
	return;
}
#endif

/******************************************************************************
 *
 * Call the python object with all arguments
 *
 */
static void _CallPythonObject(void *mem,
			      ffi_type *restype,
			      SETFUNC setfunc,
			      PyObject *callable,
			      PyObject *converters,
			      void **pArgs)
{
	Py_ssize_t i;
	PyObject *result;
	PyObject *arglist = NULL;
	Py_ssize_t nArgs;
#ifdef WITH_THREAD
	PyGILState_STATE state = PyGILState_Ensure();
#endif

	nArgs = PySequence_Length(converters);
	/* Hm. What to return in case of error?
	   For COM, 0xFFFFFFFF seems better than 0.
	*/
	if (nArgs < 0) {
		PrintError("BUG: PySequence_Length");
		goto Done;
	}

	arglist = PyTuple_New(nArgs);
	if (!arglist) {
		PrintError("PyTuple_New()");
		goto Done;
	}
	for (i = 0; i < nArgs; ++i) {
		/* Note: new reference! */
		PyObject *cnv = PySequence_GetItem(converters, i);
		StgDictObject *dict;
		if (cnv)
			dict = PyType_stgdict(cnv);
		else {
			PrintError("Getting argument converter %d\n", i);
			goto Done;
		}

		if (dict && dict->getfunc && !IsSimpleSubType(cnv)) {
			PyObject *v = dict->getfunc(*pArgs, dict->size);
			if (!v) {
				PrintError("create argument %d:\n", i);
				Py_DECREF(cnv);
				goto Done;
			}
			PyTuple_SET_ITEM(arglist, i, v);
			/* XXX XXX XX
			   We have the problem that c_byte or c_short have dict->size of
			   1 resp. 4, but these parameters are pushed as sizeof(int) bytes.
			   BTW, the same problem occurrs when they are pushed as parameters
			*/
		} else if (dict) {
			/* Hm, shouldn't we use CData_AtAddress() or something like that instead? */
			CDataObject *obj = (CDataObject *)PyObject_CallFunctionObjArgs(cnv, NULL);
			if (!obj) {
				PrintError("create argument %d:\n", i);
				Py_DECREF(cnv);
				goto Done;
			}
			if (!CDataObject_Check(obj)) {
				Py_DECREF(obj);
				Py_DECREF(cnv);
				PrintError("unexpected result of create argument %d:\n", i);
				goto Done;
			}
			memcpy(obj->b_ptr, *pArgs, dict->size);
			PyTuple_SET_ITEM(arglist, i, (PyObject *)obj);
#ifdef MS_WIN32
			TryAddRef(dict, obj);
#endif
		} else {
			PyErr_SetString(PyExc_TypeError,
					"cannot build parameter");
			PrintError("Parsing argument %d\n", i);
			Py_DECREF(cnv);
			goto Done;
		}
		Py_DECREF(cnv);
		/* XXX error handling! */
		pArgs++;
	}

#define CHECK(what, x) \
if (x == NULL) _AddTraceback(what, "_ctypes/callbacks.c", __LINE__ - 1), PyErr_Print()

	result = PyObject_CallObject(callable, arglist);
	CHECK("'calling callback function'", result);
	if ((restype != &ffi_type_void) && result) {
		PyObject *keep;
		assert(setfunc);
#ifdef WORDS_BIGENDIAN
		/* See the corresponding code in callproc.c, around line 961 */
		if (restype->type != FFI_TYPE_FLOAT && restype->size < sizeof(ffi_arg))
			mem = (char *)mem + sizeof(ffi_arg) - restype->size;
#endif
		keep = setfunc(mem, result, 0);
		CHECK("'converting callback result'", keep);
		/* keep is an object we have to keep alive so that the result
		   stays valid.  If there is no such object, the setfunc will
		   have returned Py_None.

		   If there is such an object, we have no choice than to keep
		   it alive forever - but a refcount and/or memory leak will
		   be the result.  EXCEPT when restype is py_object - Python
		   itself knows how to manage the refcount of these objects.
		*/
		if (keep == NULL) /* Could not convert callback result. */
			PyErr_WriteUnraisable(callable);
		else if (keep == Py_None) /* Nothing to keep */
			Py_DECREF(keep);
		else if (setfunc != getentry("O")->setfunc) {
			if (-1 == PyErr_Warn(PyExc_RuntimeWarning,
					     "memory leak in callback function."))
				PyErr_WriteUnraisable(callable);
		}
	}
	Py_XDECREF(result);
  Done:
	Py_XDECREF(arglist);
#ifdef WITH_THREAD
	PyGILState_Release(state);
#endif
}

static void closure_fcn(ffi_cif *cif,
			void *resp,
			void **args,
			void *userdata)
{
	ffi_info *p = userdata;

	_CallPythonObject(resp,
			  p->restype,
			  p->setfunc,
			  p->callable,
			  p->converters,
			  args);
}

ffi_info *AllocFunctionCallback(PyObject *callable,
				PyObject *converters,
				PyObject *restype,
				int is_cdecl)
{
	int result;
	ffi_info *p;
	Py_ssize_t nArgs, i;
	ffi_abi cc;

	nArgs = PySequence_Size(converters);
	p = (ffi_info *)PyMem_Malloc(sizeof(ffi_info) + sizeof(ffi_type) * (nArgs));
	if (p == NULL) {
		PyErr_NoMemory();
		return NULL;
	}
	p->pcl = MallocClosure();
	if (p->pcl == NULL) {
		PyErr_NoMemory();
		goto error;
	}

	for (i = 0; i < nArgs; ++i) {
		PyObject *cnv = PySequence_GetItem(converters, i);
		if (cnv == NULL)
			goto error;
		p->atypes[i] = GetType(cnv);
		Py_DECREF(cnv);
	}
	p->atypes[i] = NULL;

	if (restype == Py_None) {
		p->setfunc = NULL;
		p->restype = &ffi_type_void;
	} else {
		StgDictObject *dict = PyType_stgdict(restype);
		if (dict == NULL || dict->setfunc == NULL) {
		  PyErr_SetString(PyExc_TypeError,
				  "invalid result type for callback function");
		  goto error;
		}
		p->setfunc = dict->setfunc;
		p->restype = &dict->ffi_type_pointer;
	}

	cc = FFI_DEFAULT_ABI;
#if defined(MS_WIN32) && !defined(_WIN32_WCE) && !defined(MS_WIN64)
	if (is_cdecl == 0)
		cc = FFI_STDCALL;
#endif
	result = ffi_prep_cif(&p->cif, cc,
			      Py_SAFE_DOWNCAST(nArgs, Py_ssize_t, int),
			      GetType(restype),
			      &p->atypes[0]);
	if (result != FFI_OK) {
		PyErr_Format(PyExc_RuntimeError,
			     "ffi_prep_cif failed with %d", result);
		goto error;
	}
	result = ffi_prep_closure(p->pcl, &p->cif, closure_fcn, p);
	if (result != FFI_OK) {
		PyErr_Format(PyExc_RuntimeError,
			     "ffi_prep_closure failed with %d", result);
		goto error;
	}

	p->converters = converters;
	p->callable = callable;
	return p;

  error:
	if (p) {
		if (p->pcl)
			FreeClosure(p->pcl);
		PyMem_Free(p);
	}
	return NULL;
}

/****************************************************************************
 *
 * callback objects: initialization
 */

void init_callbacks_in_module(PyObject *m)
{
	if (PyType_Ready((PyTypeObject *)&PyType_Type) < 0)
		return;
}

#ifdef MS_WIN32

static void LoadPython(void)
{
	if (!Py_IsInitialized()) {
#ifdef WITH_THREAD
		PyEval_InitThreads();
#endif
		Py_Initialize();
	}
}

/******************************************************************/

long Call_GetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
	PyObject *mod, *func, *result;
	long retval;
	static PyObject *context;

	if (context == NULL)
		context = PyString_InternFromString("_ctypes.DllGetClassObject");

	mod = PyImport_ImportModuleNoBlock("ctypes");
	if (!mod) {
		PyErr_WriteUnraisable(context ? context : Py_None);
		/* There has been a warning before about this already */
		return E_FAIL;
	}

	func = PyObject_GetAttrString(mod, "DllGetClassObject");
	Py_DECREF(mod);
	if (!func) {
		PyErr_WriteUnraisable(context ? context : Py_None);
		return E_FAIL;
	}

	{
		PyObject *py_rclsid = PyLong_FromVoidPtr((void *)rclsid);
		PyObject *py_riid = PyLong_FromVoidPtr((void *)riid);
		PyObject *py_ppv = PyLong_FromVoidPtr(ppv);
		if (!py_rclsid || !py_riid || !py_ppv) {
			Py_XDECREF(py_rclsid);
			Py_XDECREF(py_riid);
			Py_XDECREF(py_ppv);
			Py_DECREF(func);
			PyErr_WriteUnraisable(context ? context : Py_None);
			return E_FAIL;
		}
		result = PyObject_CallFunctionObjArgs(func,
						      py_rclsid,
						      py_riid,
						      py_ppv,
						      NULL);
		Py_DECREF(py_rclsid);
		Py_DECREF(py_riid);
		Py_DECREF(py_ppv);
	}
	Py_DECREF(func);
	if (!result) {
		PyErr_WriteUnraisable(context ? context : Py_None);
		return E_FAIL;
	}

	retval = PyInt_AsLong(result);
	if (PyErr_Occurred()) {
		PyErr_WriteUnraisable(context ? context : Py_None);
		retval = E_FAIL;
	}
	Py_DECREF(result);
	return retval;
}

STDAPI DllGetClassObject(REFCLSID rclsid,
			 REFIID riid,
			 LPVOID *ppv)
{
	long result;
#ifdef WITH_THREAD
	PyGILState_STATE state;
#endif

	LoadPython();
#ifdef WITH_THREAD
	state = PyGILState_Ensure();
#endif
	result = Call_GetClassObject(rclsid, riid, ppv);
#ifdef WITH_THREAD
	PyGILState_Release(state);
#endif
	return result;
}

long Call_CanUnloadNow(void)
{
	PyObject *mod, *func, *result;
	long retval;
	static PyObject *context;

	if (context == NULL)
		context = PyString_InternFromString("_ctypes.DllCanUnloadNow");

	mod = PyImport_ImportModuleNoBlock("ctypes");
	if (!mod) {
/*		OutputDebugString("Could not import ctypes"); */
		/* We assume that this error can only occur when shutting
		   down, so we silently ignore it */
		PyErr_Clear();
		return E_FAIL;
	}
	/* Other errors cannot be raised, but are printed to stderr */
	func = PyObject_GetAttrString(mod, "DllCanUnloadNow");
	Py_DECREF(mod);
	if (!func) {
		PyErr_WriteUnraisable(context ? context : Py_None);
		return E_FAIL;
	}

	result = PyObject_CallFunction(func, NULL);
	Py_DECREF(func);
	if (!result) {
		PyErr_WriteUnraisable(context ? context : Py_None);
		return E_FAIL;
	}

	retval = PyInt_AsLong(result);
	if (PyErr_Occurred()) {
		PyErr_WriteUnraisable(context ? context : Py_None);
		retval = E_FAIL;
	}
	Py_DECREF(result);
	return retval;
}

/*
  DllRegisterServer and DllUnregisterServer still missing
*/

STDAPI DllCanUnloadNow(void)
{
	long result;
#ifdef WITH_THREAD
	PyGILState_STATE state = PyGILState_Ensure();
#endif
	result = Call_CanUnloadNow();
#ifdef WITH_THREAD
	PyGILState_Release(state);
#endif
	return result;
}

#ifndef Py_NO_ENABLE_SHARED
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvRes)
{
	switch(fdwReason) {
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hinstDLL);
		break;
	}
	return TRUE;
}
#endif

#endif

/*
 Local Variables:
 compile-command: "cd .. && python setup.py -q build_ext"
 End:
*/
