/*
 * support routines for subprocess module
 *
 * Currently, this extension module is only required when using the
 * subprocess module on Windows, but in the future, stubs for other
 * platforms might be added here as well.
 *
 * Copyright (c) 2004 by Fredrik Lundh <fredrik@pythonware.com>
 * Copyright (c) 2004 by Secret Labs AB, http://www.pythonware.com
 * Copyright (c) 2004 by Peter Astrand <astrand@lysator.liu.se>
 *
 * By obtaining, using, and/or copying this software and/or its
 * associated documentation, you agree that you have read, understood,
 * and will comply with the following terms and conditions:
 *
 * Permission to use, copy, modify, and distribute this software and
 * its associated documentation for any purpose and without fee is
 * hereby granted, provided that the above copyright notice appears in
 * all copies, and that both that copyright notice and this permission
 * notice appear in supporting documentation, and that the name of the
 * authors not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.
 *
 * THE AUTHORS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* TODO: handle unicode command lines? */
/* TODO: handle unicode environment? */

#include "Python.h"

#define WINDOWS_LEAN_AND_MEAN
#include "windows.h"

/* -------------------------------------------------------------------- */
/* handle wrapper.  note that this library uses integers when passing
   handles to a function, and handle wrappers when returning handles.
   the wrapper is used to provide Detach and Close methods */

typedef struct {
	PyObject_HEAD
	HANDLE handle;
} sp_handle_object;

staticforward PyTypeObject sp_handle_type;

static PyObject*
sp_handle_new(HANDLE handle)
{
	sp_handle_object* self;

	self = PyObject_NEW(sp_handle_object, &sp_handle_type);
	if (self == NULL)
		return NULL;

	self->handle = handle;

	return (PyObject*) self;
}

static PyObject*
sp_handle_detach(sp_handle_object* self, PyObject* args)
{
	HANDLE handle;

	if (! PyArg_ParseTuple(args, ":Detach"))
		return NULL;

	handle = self->handle;

	self->handle = NULL;

	/* note: return the current handle, as an integer */
	return PyInt_FromLong((long) handle);
}

static PyObject*
sp_handle_close(sp_handle_object* self, PyObject* args)
{
	if (! PyArg_ParseTuple(args, ":Close"))
		return NULL;

	if (self->handle != INVALID_HANDLE_VALUE) {
		CloseHandle(self->handle);
		self->handle = INVALID_HANDLE_VALUE;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static void
sp_handle_dealloc(sp_handle_object* self)
{
	if (self->handle != INVALID_HANDLE_VALUE)
		CloseHandle(self->handle);
	PyMem_DEL(self);
}

static PyMethodDef sp_handle_methods[] = {
	{"Detach", (PyCFunction) sp_handle_detach, 1},
	{"Close", (PyCFunction) sp_handle_close, 1},
	{NULL, NULL}
};

static PyObject*
sp_handle_getattr(sp_handle_object* self, char* name)
{
	return Py_FindMethod(sp_handle_methods, (PyObject*) self, name);
}

static PyObject*
sp_handle_as_int(sp_handle_object* self)
{
	return PyInt_FromLong((long) self->handle);
}

static PyNumberMethods sp_handle_as_number;

statichere PyTypeObject sp_handle_type = {
	PyObject_HEAD_INIT(NULL)
	0,				/*ob_size*/
	"_subprocess_handle", sizeof(sp_handle_object), 0,
	(destructor) sp_handle_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) sp_handle_getattr,/*tp_getattr*/
	0,				/*tp_setattr*/
	0,				/*tp_compare*/
	0,				/*tp_repr*/
	&sp_handle_as_number,		/*tp_as_number */
	0,				/*tp_as_sequence */
	0,				/*tp_as_mapping */
	0				/*tp_hash*/
};

/* -------------------------------------------------------------------- */
/* windows API functions */

static PyObject *
sp_GetStdHandle(PyObject* self, PyObject* args)
{
	HANDLE handle;
	int std_handle;

	if (! PyArg_ParseTuple(args, "i:GetStdHandle", &std_handle))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	handle = GetStdHandle((DWORD) std_handle);
	Py_END_ALLOW_THREADS

	if (handle == INVALID_HANDLE_VALUE)
		return PyErr_SetFromWindowsErr(GetLastError());

	if (! handle) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	/* note: returns integer, not handle object */
	return PyInt_FromLong((long) handle);
}

static PyObject *
sp_GetCurrentProcess(PyObject* self, PyObject* args)
{
	if (! PyArg_ParseTuple(args, ":GetCurrentProcess"))
		return NULL;

	return sp_handle_new(GetCurrentProcess());
}

static PyObject *
sp_DuplicateHandle(PyObject* self, PyObject* args)
{
	HANDLE target_handle;
	BOOL result;

	long source_process_handle;
	long source_handle;
	long target_process_handle;
	int desired_access;
	int inherit_handle;
	int options = 0;

	if (! PyArg_ParseTuple(args, "lllii|i:DuplicateHandle",
	                       &source_process_handle,
	                       &source_handle,
	                       &target_process_handle,
	                       &desired_access,
	                       &inherit_handle,
	                       &options))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	result = DuplicateHandle(
		(HANDLE) source_process_handle,
		(HANDLE) source_handle,
		(HANDLE) target_process_handle,
		&target_handle,
		desired_access,
		inherit_handle,
		options
	);
	Py_END_ALLOW_THREADS

	if (! result)
		return PyErr_SetFromWindowsErr(GetLastError());

	return sp_handle_new(target_handle);
}

static PyObject *
sp_CreatePipe(PyObject* self, PyObject* args)
{
	HANDLE read_pipe;
	HANDLE write_pipe;
	BOOL result;

	PyObject* pipe_attributes; /* ignored */
	int size;

	if (! PyArg_ParseTuple(args, "Oi:CreatePipe", &pipe_attributes, &size))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	result = CreatePipe(&read_pipe, &write_pipe, NULL, size);
	Py_END_ALLOW_THREADS

	if (! result)
		return PyErr_SetFromWindowsErr(GetLastError());

	return Py_BuildValue(
		"NN", sp_handle_new(read_pipe), sp_handle_new(write_pipe));
}

/* helpers for createprocess */

static int
getint(PyObject* obj, char* name)
{
	PyObject* value;

	value = PyObject_GetAttrString(obj, name);
	if (! value) {
		PyErr_Clear(); /* FIXME: propagate error? */
		return 0;
	}
	return (int) PyInt_AsLong(value);
}

static HANDLE
gethandle(PyObject* obj, char* name)
{
	sp_handle_object* value;

	value = (sp_handle_object*) PyObject_GetAttrString(obj, name);
	if (! value) {
		PyErr_Clear(); /* FIXME: propagate error? */
		return NULL;
	}
	if (value->ob_type != &sp_handle_type)
		return NULL;
	return value->handle;
}

static PyObject*
getenvironment(PyObject* environment)
{
	int i, envsize;
	PyObject* out = NULL;
	PyObject* keys;
	PyObject* values;
	char* p;

	/* convert environment dictionary to windows enviroment string */
	if (! PyMapping_Check(environment)) {
		PyErr_SetString(
		    PyExc_TypeError, "environment must be dictionary or None");
		return NULL;
	}

	envsize = PyMapping_Length(environment);

	keys = PyMapping_Keys(environment);
	values = PyMapping_Values(environment);
	if (!keys || !values)
		goto error;

	out = PyString_FromStringAndSize(NULL, 2048);
	if (! out)
		goto error;

	p = PyString_AS_STRING(out);

	for (i = 0; i < envsize; i++) {
		int ksize, vsize, totalsize;
		PyObject* key = PyList_GET_ITEM(keys, i);
		PyObject* value = PyList_GET_ITEM(values, i);

		if (! PyString_Check(key) || ! PyString_Check(value)) {
			PyErr_SetString(PyExc_TypeError,
				"environment can only contain strings");
			goto error;
		}
		ksize = PyString_GET_SIZE(key);
		vsize = PyString_GET_SIZE(value);
		totalsize = (p - PyString_AS_STRING(out)) + ksize + 1 +
							     vsize + 1 + 1;
		if (totalsize > PyString_GET_SIZE(out)) {
			int offset = p - PyString_AS_STRING(out);
			_PyString_Resize(&out, totalsize + 1024);
			p = PyString_AS_STRING(out) + offset;
		}
		memcpy(p, PyString_AS_STRING(key), ksize);
		p += ksize;
		*p++ = '=';
		memcpy(p, PyString_AS_STRING(value), vsize);
		p += vsize;
		*p++ = '\0';
	}

	/* add trailing null byte */
	*p++ = '\0';
	_PyString_Resize(&out, p - PyString_AS_STRING(out));

	/* PyObject_Print(out, stdout, 0); */

	return out;

 error:
	Py_XDECREF(out);
	Py_XDECREF(keys);
	Py_XDECREF(values);
	return NULL;
}

static PyObject *
sp_CreateProcess(PyObject* self, PyObject* args)
{
	BOOL result;
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	PyObject* environment;

	char* application_name;
	char* command_line;
	PyObject* process_attributes; /* ignored */
	PyObject* thread_attributes; /* ignored */
	int inherit_handles;
	int creation_flags;
	PyObject* env_mapping;
	char* current_directory;
	PyObject* startup_info;

	if (! PyArg_ParseTuple(args, "zzOOiiOzO:CreateProcess",
			       &application_name,
			       &command_line,
			       &process_attributes,
			       &thread_attributes,
			       &inherit_handles,
			       &creation_flags,
			       &env_mapping,
			       &current_directory,
			       &startup_info))
		return NULL;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	/* note: we only support a small subset of all SI attributes */
	si.dwFlags = getint(startup_info, "dwFlags");
	si.wShowWindow = getint(startup_info, "wShowWindow");
	si.hStdInput = gethandle(startup_info, "hStdInput");
	si.hStdOutput = gethandle(startup_info, "hStdOutput");
	si.hStdError = gethandle(startup_info, "hStdError");

	if (env_mapping == Py_None)
		environment = NULL;
	else {
		environment = getenvironment(env_mapping);
		if (! environment)
			return NULL;
	}

	Py_BEGIN_ALLOW_THREADS
	result = CreateProcess(application_name,
			       command_line,
			       NULL,
			       NULL,
			       inherit_handles,
			       creation_flags,
			       environment ? PyString_AS_STRING(environment) : NULL,
			       current_directory,
			       &si,
			       &pi);
	Py_END_ALLOW_THREADS

	Py_XDECREF(environment);

	if (! result)
		return PyErr_SetFromWindowsErr(GetLastError());

	return Py_BuildValue("NNii",
			     sp_handle_new(pi.hProcess),
			     sp_handle_new(pi.hThread),
			     pi.dwProcessId,
			     pi.dwThreadId);
}

static PyObject *
sp_GetExitCodeProcess(PyObject* self, PyObject* args)
{
	DWORD exit_code;
	BOOL result;

	long process;
	if (! PyArg_ParseTuple(args, "l:GetExitCodeProcess", &process))
		return NULL;

	result = GetExitCodeProcess((HANDLE) process, &exit_code);

	if (! result)
		return PyErr_SetFromWindowsErr(GetLastError());

	return PyInt_FromLong(exit_code);
}

static PyObject *
sp_WaitForSingleObject(PyObject* self, PyObject* args)
{
	DWORD result;

	long handle;
	int milliseconds;
	if (! PyArg_ParseTuple(args, "li:WaitForSingleObject",
	                  	     &handle,
	                  	     &milliseconds))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	result = WaitForSingleObject((HANDLE) handle, (DWORD) milliseconds);
	Py_END_ALLOW_THREADS

	if (result == WAIT_FAILED)
		return PyErr_SetFromWindowsErr(GetLastError());

	return PyInt_FromLong((int) result);
}

static PyObject *
sp_GetVersion(PyObject* self, PyObject* args)
{
	if (! PyArg_ParseTuple(args, ":GetVersion"))
		return NULL;

	return PyInt_FromLong((int) GetVersion());
}

static PyObject *
sp_GetModuleFileName(PyObject* self, PyObject* args)
{
	BOOL result;
	long module;
	TCHAR filename[MAX_PATH];

	if (! PyArg_ParseTuple(args, "l:GetModuleFileName", &module))
		return NULL;

	result = GetModuleFileName((HMODULE)module, filename, MAX_PATH);
	filename[MAX_PATH-1] = '\0';

	if (! result)
		return PyErr_SetFromWindowsErr(GetLastError());

	return PyString_FromString(filename);
}

static PyMethodDef sp_functions[] = {
	{"GetStdHandle",	sp_GetStdHandle,	METH_VARARGS},
	{"GetCurrentProcess",	sp_GetCurrentProcess,	METH_VARARGS},
	{"DuplicateHandle",	sp_DuplicateHandle,	METH_VARARGS},
	{"CreatePipe",		sp_CreatePipe,		METH_VARARGS},
	{"CreateProcess",	sp_CreateProcess,	METH_VARARGS},
	{"GetExitCodeProcess",	sp_GetExitCodeProcess,	METH_VARARGS},
	{"WaitForSingleObject",	sp_WaitForSingleObject, METH_VARARGS},
	{"GetVersion",		sp_GetVersion,		METH_VARARGS},
	{"GetModuleFileName",	sp_GetModuleFileName,	METH_VARARGS},
	{NULL, NULL}
};

/* -------------------------------------------------------------------- */

static void
defint(PyObject* d, const char* name, int value)
{
	PyObject* v = PyInt_FromLong((long) value);
	if (v) {
		PyDict_SetItemString(d, (char*) name, v);
		Py_DECREF(v);
	}
}

#if PY_VERSION_HEX >= 0x02030000
PyMODINIT_FUNC
#else
DL_EXPORT(void)
#endif
init_subprocess()
{
	PyObject *d;
	PyObject *m;

	/* patch up object descriptors */
	sp_handle_type.ob_type = &PyType_Type;
	sp_handle_as_number.nb_int = (unaryfunc) sp_handle_as_int;

	m = Py_InitModule("_subprocess", sp_functions);
	d = PyModule_GetDict(m);

	/* constants */
	defint(d, "STD_INPUT_HANDLE", STD_INPUT_HANDLE);
	defint(d, "STD_OUTPUT_HANDLE", STD_OUTPUT_HANDLE);
	defint(d, "STD_ERROR_HANDLE", STD_ERROR_HANDLE);
	defint(d, "DUPLICATE_SAME_ACCESS", DUPLICATE_SAME_ACCESS);
	defint(d, "STARTF_USESTDHANDLES", STARTF_USESTDHANDLES);
	defint(d, "STARTF_USESHOWWINDOW", STARTF_USESHOWWINDOW);
	defint(d, "SW_HIDE", SW_HIDE);
	defint(d, "INFINITE", INFINITE);
	defint(d, "WAIT_OBJECT_0", WAIT_OBJECT_0);
	defint(d, "CREATE_NEW_CONSOLE", CREATE_NEW_CONSOLE);
}
