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

/* Licensed to PSF under a Contributor Agreement. */
/* See http://www.python.org/2.4/license for licensing details. */

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

#if defined(MS_WIN32) && !defined(MS_WIN64)
#define HANDLE_TO_PYNUM(handle) PyInt_FromLong((long) handle)
#define PY_HANDLE_PARAM "l"
#else
#define HANDLE_TO_PYNUM(handle) PyLong_FromLongLong((long long) handle)
#define PY_HANDLE_PARAM "L"
#endif

static PyObject*
sp_handle_detach(sp_handle_object* self, PyObject* args)
{
    HANDLE handle;

    if (! PyArg_ParseTuple(args, ":Detach"))
        return NULL;

    handle = self->handle;

    self->handle = INVALID_HANDLE_VALUE;

    /* note: return the current handle, as an integer */
    return HANDLE_TO_PYNUM(handle);
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
    PyObject_FREE(self);
}

static PyMethodDef sp_handle_methods[] = {
    {"Detach", (PyCFunction) sp_handle_detach, METH_VARARGS},
    {"Close",  (PyCFunction) sp_handle_close,  METH_VARARGS},
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
    return HANDLE_TO_PYNUM(self->handle);
}

static PyNumberMethods sp_handle_as_number;

statichere PyTypeObject sp_handle_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /*ob_size*/
    "_subprocess_handle", sizeof(sp_handle_object), 0,
    (destructor) sp_handle_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc) sp_handle_getattr,/*tp_getattr*/
    0,                                  /*tp_setattr*/
    0,                                  /*tp_compare*/
    0,                                  /*tp_repr*/
    &sp_handle_as_number,               /*tp_as_number */
    0,                                  /*tp_as_sequence */
    0,                                  /*tp_as_mapping */
    0                                   /*tp_hash*/
};

/* -------------------------------------------------------------------- */
/* windows API functions */

PyDoc_STRVAR(GetStdHandle_doc,
"GetStdHandle(handle) -> integer\n\
\n\
Return a handle to the specified standard device\n\
(STD_INPUT_HANDLE, STD_OUTPUT_HANDLE, STD_ERROR_HANDLE).\n\
The integer associated with the handle object is returned.");

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
    return HANDLE_TO_PYNUM(handle);
}

PyDoc_STRVAR(GetCurrentProcess_doc,
"GetCurrentProcess() -> handle\n\
\n\
Return a handle object for the current process.");

static PyObject *
sp_GetCurrentProcess(PyObject* self, PyObject* args)
{
    if (! PyArg_ParseTuple(args, ":GetCurrentProcess"))
        return NULL;

    return sp_handle_new(GetCurrentProcess());
}

PyDoc_STRVAR(DuplicateHandle_doc,
"DuplicateHandle(source_proc_handle, source_handle,\n\
                 target_proc_handle, target_handle, access,\n\
                 inherit[, options]) -> handle\n\
\n\
Return a duplicate handle object.\n\
\n\
The duplicate handle refers to the same object as the original\n\
handle. Therefore, any changes to the object are reflected\n\
through both handles.");

static PyObject *
sp_DuplicateHandle(PyObject* self, PyObject* args)
{
    HANDLE target_handle;
    BOOL result;

    HANDLE source_process_handle;
    HANDLE source_handle;
    HANDLE target_process_handle;
    int desired_access;
    int inherit_handle;
    int options = 0;

    if (! PyArg_ParseTuple(args,
                           PY_HANDLE_PARAM PY_HANDLE_PARAM PY_HANDLE_PARAM
                           "ii|i:DuplicateHandle",
                           &source_process_handle,
                           &source_handle,
                           &target_process_handle,
                           &desired_access,
                           &inherit_handle,
                           &options))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    result = DuplicateHandle(
        source_process_handle,
        source_handle,
        target_process_handle,
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

PyDoc_STRVAR(CreatePipe_doc,
"CreatePipe(pipe_attrs, size) -> (read_handle, write_handle)\n\
\n\
Create an anonymous pipe, and return handles to the read and\n\
write ends of the pipe.\n\
\n\
pipe_attrs is ignored internally and can be None.");

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
    int ret;

    value = PyObject_GetAttrString(obj, name);
    if (! value) {
        PyErr_Clear(); /* FIXME: propagate error? */
        return 0;
    }
    ret = (int) PyInt_AsLong(value);
    Py_DECREF(value);
    return ret;
}

static HANDLE
gethandle(PyObject* obj, char* name)
{
    sp_handle_object* value;
    HANDLE ret;

    value = (sp_handle_object*) PyObject_GetAttrString(obj, name);
    if (! value) {
        PyErr_Clear(); /* FIXME: propagate error? */
        return NULL;
    }
    if (value->ob_type != &sp_handle_type)
        ret = NULL;
    else
        ret = value->handle;
    Py_DECREF(value);
    return ret;
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

    Py_XDECREF(keys);
    Py_XDECREF(values);

    return out;

 error:
    Py_XDECREF(out);
    Py_XDECREF(keys);
    Py_XDECREF(values);
    return NULL;
}

PyDoc_STRVAR(CreateProcess_doc,
"CreateProcess(app_name, cmd_line, proc_attrs, thread_attrs,\n\
               inherit, flags, env_mapping, curdir,\n\
               startup_info) -> (proc_handle, thread_handle,\n\
                                 pid, tid)\n\
\n\
Create a new process and its primary thread. The return\n\
value is a tuple of the process handle, thread handle,\n\
process ID, and thread ID.\n\
\n\
proc_attrs and thread_attrs are ignored internally and can be None.");

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

    if (PyErr_Occurred())
        return NULL;

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

PyDoc_STRVAR(TerminateProcess_doc,
"TerminateProcess(handle, exit_code) -> None\n\
\n\
Terminate the specified process and all of its threads.");

static PyObject *
sp_TerminateProcess(PyObject* self, PyObject* args)
{
    BOOL result;

    HANDLE process;
    int exit_code;
    if (! PyArg_ParseTuple(args, PY_HANDLE_PARAM "i:TerminateProcess",
                           &process, &exit_code))
        return NULL;

    result = TerminateProcess(process, exit_code);

    if (! result)
        return PyErr_SetFromWindowsErr(GetLastError());

    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(GetExitCodeProcess_doc,
"GetExitCodeProcess(handle) -> Exit code\n\
\n\
Return the termination status of the specified process.");

static PyObject *
sp_GetExitCodeProcess(PyObject* self, PyObject* args)
{
    DWORD exit_code;
    BOOL result;

    HANDLE process;
    if (! PyArg_ParseTuple(args, PY_HANDLE_PARAM ":GetExitCodeProcess", &process))
        return NULL;

    result = GetExitCodeProcess(process, &exit_code);

    if (! result)
        return PyErr_SetFromWindowsErr(GetLastError());

    return PyInt_FromLong(exit_code);
}

PyDoc_STRVAR(WaitForSingleObject_doc,
"WaitForSingleObject(handle, timeout) -> result\n\
\n\
Wait until the specified object is in the signaled state or\n\
the time-out interval elapses. The timeout value is specified\n\
in milliseconds.");

static PyObject *
sp_WaitForSingleObject(PyObject* self, PyObject* args)
{
    DWORD result;

    HANDLE handle;
    int milliseconds;
    if (! PyArg_ParseTuple(args, PY_HANDLE_PARAM "i:WaitForSingleObject",
                                 &handle,
                                 &milliseconds))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    result = WaitForSingleObject(handle, (DWORD) milliseconds);
    Py_END_ALLOW_THREADS

    if (result == WAIT_FAILED)
        return PyErr_SetFromWindowsErr(GetLastError());

    return PyInt_FromLong((int) result);
}

PyDoc_STRVAR(GetVersion_doc,
"GetVersion() -> version\n\
\n\
Return the version number of the current operating system.");

static PyObject *
sp_GetVersion(PyObject* self, PyObject* args)
{
    if (! PyArg_ParseTuple(args, ":GetVersion"))
        return NULL;

    return PyInt_FromLong((int) GetVersion());
}

PyDoc_STRVAR(GetModuleFileName_doc,
"GetModuleFileName(module) -> path\n\
\n\
Return the fully-qualified path for the file that contains\n\
the specified module. The module must have been loaded by the\n\
current process.\n\
\n\
The module parameter should be a handle to the loaded module\n\
whose path is being requested. If this parameter is 0, \n\
GetModuleFileName retrieves the path of the executable file\n\
of the current process.");

static PyObject *
sp_GetModuleFileName(PyObject* self, PyObject* args)
{
    BOOL result;
    HMODULE module;
    TCHAR filename[MAX_PATH];

    if (! PyArg_ParseTuple(args, PY_HANDLE_PARAM ":GetModuleFileName",
                           &module))
        return NULL;

    result = GetModuleFileName(module, filename, MAX_PATH);
    filename[MAX_PATH-1] = '\0';

    if (! result)
        return PyErr_SetFromWindowsErr(GetLastError());

    return PyString_FromString(filename);
}

static PyMethodDef sp_functions[] = {
    {"GetStdHandle", sp_GetStdHandle, METH_VARARGS, GetStdHandle_doc},
    {"GetCurrentProcess", sp_GetCurrentProcess,         METH_VARARGS,
                                              GetCurrentProcess_doc},
    {"DuplicateHandle",         sp_DuplicateHandle,     METH_VARARGS,
                                            DuplicateHandle_doc},
    {"CreatePipe", sp_CreatePipe, METH_VARARGS, CreatePipe_doc},
    {"CreateProcess", sp_CreateProcess, METH_VARARGS, CreateProcess_doc},
    {"TerminateProcess", sp_TerminateProcess, METH_VARARGS,
                                             TerminateProcess_doc},
    {"GetExitCodeProcess", sp_GetExitCodeProcess, METH_VARARGS,
                                               GetExitCodeProcess_doc},
    {"WaitForSingleObject", sp_WaitForSingleObject, METH_VARARGS,
                                                    WaitForSingleObject_doc},
    {"GetVersion", sp_GetVersion, METH_VARARGS, GetVersion_doc},
    {"GetModuleFileName", sp_GetModuleFileName, METH_VARARGS,
                                              GetModuleFileName_doc},
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
    if (m == NULL)
        return;
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
    defint(d, "CREATE_NEW_PROCESS_GROUP", CREATE_NEW_PROCESS_GROUP);
}
