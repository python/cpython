/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_long.h"          // _PyLong_Size_t_Converter()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_winapi_Overlapped_GetOverlappedResult__doc__,
"GetOverlappedResult($self, wait, /)\n"
"--\n"
"\n");

#define _WINAPI_OVERLAPPED_GETOVERLAPPEDRESULT_METHODDEF    \
    {"GetOverlappedResult", (PyCFunction)_winapi_Overlapped_GetOverlappedResult, METH_O, _winapi_Overlapped_GetOverlappedResult__doc__},

static PyObject *
_winapi_Overlapped_GetOverlappedResult_impl(OverlappedObject *self, int wait);

static PyObject *
_winapi_Overlapped_GetOverlappedResult(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    int wait;

    wait = PyObject_IsTrue(arg);
    if (wait < 0) {
        goto exit;
    }
    return_value = _winapi_Overlapped_GetOverlappedResult_impl((OverlappedObject *)self, wait);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_Overlapped_getbuffer__doc__,
"getbuffer($self, /)\n"
"--\n"
"\n");

#define _WINAPI_OVERLAPPED_GETBUFFER_METHODDEF    \
    {"getbuffer", (PyCFunction)_winapi_Overlapped_getbuffer, METH_NOARGS, _winapi_Overlapped_getbuffer__doc__},

static PyObject *
_winapi_Overlapped_getbuffer_impl(OverlappedObject *self);

static PyObject *
_winapi_Overlapped_getbuffer(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _winapi_Overlapped_getbuffer_impl((OverlappedObject *)self);
}

PyDoc_STRVAR(_winapi_Overlapped_cancel__doc__,
"cancel($self, /)\n"
"--\n"
"\n");

#define _WINAPI_OVERLAPPED_CANCEL_METHODDEF    \
    {"cancel", (PyCFunction)_winapi_Overlapped_cancel, METH_NOARGS, _winapi_Overlapped_cancel__doc__},

static PyObject *
_winapi_Overlapped_cancel_impl(OverlappedObject *self);

static PyObject *
_winapi_Overlapped_cancel(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _winapi_Overlapped_cancel_impl((OverlappedObject *)self);
}

PyDoc_STRVAR(_winapi_CloseHandle__doc__,
"CloseHandle($module, handle, /)\n"
"--\n"
"\n"
"Close handle.");

#define _WINAPI_CLOSEHANDLE_METHODDEF    \
    {"CloseHandle", (PyCFunction)_winapi_CloseHandle, METH_O, _winapi_CloseHandle__doc__},

static PyObject *
_winapi_CloseHandle_impl(PyObject *module, HANDLE handle);

static PyObject *
_winapi_CloseHandle(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    HANDLE handle;

    if (!PyArg_Parse(arg, "" F_HANDLE ":CloseHandle", &handle)) {
        goto exit;
    }
    return_value = _winapi_CloseHandle_impl(module, handle);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_ConnectNamedPipe__doc__,
"ConnectNamedPipe($module, /, handle, overlapped=False)\n"
"--\n"
"\n");

#define _WINAPI_CONNECTNAMEDPIPE_METHODDEF    \
    {"ConnectNamedPipe", _PyCFunction_CAST(_winapi_ConnectNamedPipe), METH_FASTCALL|METH_KEYWORDS, _winapi_ConnectNamedPipe__doc__},

static PyObject *
_winapi_ConnectNamedPipe_impl(PyObject *module, HANDLE handle,
                              int use_overlapped);

static PyObject *
_winapi_ConnectNamedPipe(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(handle), &_Py_ID(overlapped), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"handle", "overlapped", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .format = "" F_HANDLE "|p:ConnectNamedPipe",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    HANDLE handle;
    int use_overlapped = 0;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &handle, &use_overlapped)) {
        goto exit;
    }
    return_value = _winapi_ConnectNamedPipe_impl(module, handle, use_overlapped);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_CreateEventW__doc__,
"CreateEventW($module, /, security_attributes, manual_reset,\n"
"             initial_state, name)\n"
"--\n"
"\n");

#define _WINAPI_CREATEEVENTW_METHODDEF    \
    {"CreateEventW", _PyCFunction_CAST(_winapi_CreateEventW), METH_FASTCALL|METH_KEYWORDS, _winapi_CreateEventW__doc__},

static HANDLE
_winapi_CreateEventW_impl(PyObject *module,
                          LPSECURITY_ATTRIBUTES security_attributes,
                          BOOL manual_reset, BOOL initial_state,
                          LPCWSTR name);

static PyObject *
_winapi_CreateEventW(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(security_attributes), &_Py_ID(manual_reset), &_Py_ID(initial_state), &_Py_ID(name), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"security_attributes", "manual_reset", "initial_state", "name", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .format = "" F_POINTER "iiO&:CreateEventW",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    LPSECURITY_ATTRIBUTES security_attributes;
    BOOL manual_reset;
    BOOL initial_state;
    LPCWSTR name = NULL;
    HANDLE _return_value;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &security_attributes, &manual_reset, &initial_state, _PyUnicode_WideCharString_Opt_Converter, &name)) {
        goto exit;
    }
    _return_value = _winapi_CreateEventW_impl(module, security_attributes, manual_reset, initial_state, name);
    if ((_return_value == INVALID_HANDLE_VALUE) && PyErr_Occurred()) {
        goto exit;
    }
    if (_return_value == NULL) {
        Py_RETURN_NONE;
    }
    return_value = HANDLE_TO_PYNUM(_return_value);

exit:
    /* Cleanup for name */
    PyMem_Free((void *)name);

    return return_value;
}

PyDoc_STRVAR(_winapi_CreateFile__doc__,
"CreateFile($module, file_name, desired_access, share_mode,\n"
"           security_attributes, creation_disposition,\n"
"           flags_and_attributes, template_file, /)\n"
"--\n"
"\n");

#define _WINAPI_CREATEFILE_METHODDEF    \
    {"CreateFile", _PyCFunction_CAST(_winapi_CreateFile), METH_FASTCALL, _winapi_CreateFile__doc__},

static HANDLE
_winapi_CreateFile_impl(PyObject *module, LPCWSTR file_name,
                        DWORD desired_access, DWORD share_mode,
                        LPSECURITY_ATTRIBUTES security_attributes,
                        DWORD creation_disposition,
                        DWORD flags_and_attributes, HANDLE template_file);

static PyObject *
_winapi_CreateFile(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    LPCWSTR file_name = NULL;
    DWORD desired_access;
    DWORD share_mode;
    LPSECURITY_ATTRIBUTES security_attributes;
    DWORD creation_disposition;
    DWORD flags_and_attributes;
    HANDLE template_file;
    HANDLE _return_value;

    if (!_PyArg_ParseStack(args, nargs, "O&kk" F_POINTER "kk" F_HANDLE ":CreateFile",
        _PyUnicode_WideCharString_Converter, &file_name, &desired_access, &share_mode, &security_attributes, &creation_disposition, &flags_and_attributes, &template_file)) {
        goto exit;
    }
    _return_value = _winapi_CreateFile_impl(module, file_name, desired_access, share_mode, security_attributes, creation_disposition, flags_and_attributes, template_file);
    if ((_return_value == INVALID_HANDLE_VALUE) && PyErr_Occurred()) {
        goto exit;
    }
    if (_return_value == NULL) {
        Py_RETURN_NONE;
    }
    return_value = HANDLE_TO_PYNUM(_return_value);

exit:
    /* Cleanup for file_name */
    PyMem_Free((void *)file_name);

    return return_value;
}

PyDoc_STRVAR(_winapi_CreateFileMapping__doc__,
"CreateFileMapping($module, file_handle, security_attributes, protect,\n"
"                  max_size_high, max_size_low, name, /)\n"
"--\n"
"\n");

#define _WINAPI_CREATEFILEMAPPING_METHODDEF    \
    {"CreateFileMapping", _PyCFunction_CAST(_winapi_CreateFileMapping), METH_FASTCALL, _winapi_CreateFileMapping__doc__},

static HANDLE
_winapi_CreateFileMapping_impl(PyObject *module, HANDLE file_handle,
                               LPSECURITY_ATTRIBUTES security_attributes,
                               DWORD protect, DWORD max_size_high,
                               DWORD max_size_low, LPCWSTR name);

static PyObject *
_winapi_CreateFileMapping(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HANDLE file_handle;
    LPSECURITY_ATTRIBUTES security_attributes;
    DWORD protect;
    DWORD max_size_high;
    DWORD max_size_low;
    LPCWSTR name = NULL;
    HANDLE _return_value;

    if (!_PyArg_ParseStack(args, nargs, "" F_HANDLE "" F_POINTER "kkkO&:CreateFileMapping",
        &file_handle, &security_attributes, &protect, &max_size_high, &max_size_low, _PyUnicode_WideCharString_Converter, &name)) {
        goto exit;
    }
    _return_value = _winapi_CreateFileMapping_impl(module, file_handle, security_attributes, protect, max_size_high, max_size_low, name);
    if ((_return_value == INVALID_HANDLE_VALUE) && PyErr_Occurred()) {
        goto exit;
    }
    if (_return_value == NULL) {
        Py_RETURN_NONE;
    }
    return_value = HANDLE_TO_PYNUM(_return_value);

exit:
    /* Cleanup for name */
    PyMem_Free((void *)name);

    return return_value;
}

PyDoc_STRVAR(_winapi_CreateJunction__doc__,
"CreateJunction($module, src_path, dst_path, /)\n"
"--\n"
"\n");

#define _WINAPI_CREATEJUNCTION_METHODDEF    \
    {"CreateJunction", _PyCFunction_CAST(_winapi_CreateJunction), METH_FASTCALL, _winapi_CreateJunction__doc__},

static PyObject *
_winapi_CreateJunction_impl(PyObject *module, LPCWSTR src_path,
                            LPCWSTR dst_path);

static PyObject *
_winapi_CreateJunction(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    LPCWSTR src_path = NULL;
    LPCWSTR dst_path = NULL;

    if (!_PyArg_CheckPositional("CreateJunction", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("CreateJunction", "argument 1", "str", args[0]);
        goto exit;
    }
    src_path = PyUnicode_AsWideCharString(args[0], NULL);
    if (src_path == NULL) {
        goto exit;
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("CreateJunction", "argument 2", "str", args[1]);
        goto exit;
    }
    dst_path = PyUnicode_AsWideCharString(args[1], NULL);
    if (dst_path == NULL) {
        goto exit;
    }
    return_value = _winapi_CreateJunction_impl(module, src_path, dst_path);

exit:
    /* Cleanup for src_path */
    PyMem_Free((void *)src_path);
    /* Cleanup for dst_path */
    PyMem_Free((void *)dst_path);

    return return_value;
}

PyDoc_STRVAR(_winapi_CreateMutexW__doc__,
"CreateMutexW($module, /, security_attributes, initial_owner, name)\n"
"--\n"
"\n");

#define _WINAPI_CREATEMUTEXW_METHODDEF    \
    {"CreateMutexW", _PyCFunction_CAST(_winapi_CreateMutexW), METH_FASTCALL|METH_KEYWORDS, _winapi_CreateMutexW__doc__},

static HANDLE
_winapi_CreateMutexW_impl(PyObject *module,
                          LPSECURITY_ATTRIBUTES security_attributes,
                          BOOL initial_owner, LPCWSTR name);

static PyObject *
_winapi_CreateMutexW(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(security_attributes), &_Py_ID(initial_owner), &_Py_ID(name), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"security_attributes", "initial_owner", "name", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .format = "" F_POINTER "iO&:CreateMutexW",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    LPSECURITY_ATTRIBUTES security_attributes;
    BOOL initial_owner;
    LPCWSTR name = NULL;
    HANDLE _return_value;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &security_attributes, &initial_owner, _PyUnicode_WideCharString_Opt_Converter, &name)) {
        goto exit;
    }
    _return_value = _winapi_CreateMutexW_impl(module, security_attributes, initial_owner, name);
    if ((_return_value == INVALID_HANDLE_VALUE) && PyErr_Occurred()) {
        goto exit;
    }
    if (_return_value == NULL) {
        Py_RETURN_NONE;
    }
    return_value = HANDLE_TO_PYNUM(_return_value);

exit:
    /* Cleanup for name */
    PyMem_Free((void *)name);

    return return_value;
}

PyDoc_STRVAR(_winapi_CreateNamedPipe__doc__,
"CreateNamedPipe($module, name, open_mode, pipe_mode, max_instances,\n"
"                out_buffer_size, in_buffer_size, default_timeout,\n"
"                security_attributes, /)\n"
"--\n"
"\n");

#define _WINAPI_CREATENAMEDPIPE_METHODDEF    \
    {"CreateNamedPipe", _PyCFunction_CAST(_winapi_CreateNamedPipe), METH_FASTCALL, _winapi_CreateNamedPipe__doc__},

static HANDLE
_winapi_CreateNamedPipe_impl(PyObject *module, LPCWSTR name, DWORD open_mode,
                             DWORD pipe_mode, DWORD max_instances,
                             DWORD out_buffer_size, DWORD in_buffer_size,
                             DWORD default_timeout,
                             LPSECURITY_ATTRIBUTES security_attributes);

static PyObject *
_winapi_CreateNamedPipe(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    LPCWSTR name = NULL;
    DWORD open_mode;
    DWORD pipe_mode;
    DWORD max_instances;
    DWORD out_buffer_size;
    DWORD in_buffer_size;
    DWORD default_timeout;
    LPSECURITY_ATTRIBUTES security_attributes;
    HANDLE _return_value;

    if (!_PyArg_ParseStack(args, nargs, "O&kkkkkk" F_POINTER ":CreateNamedPipe",
        _PyUnicode_WideCharString_Converter, &name, &open_mode, &pipe_mode, &max_instances, &out_buffer_size, &in_buffer_size, &default_timeout, &security_attributes)) {
        goto exit;
    }
    _return_value = _winapi_CreateNamedPipe_impl(module, name, open_mode, pipe_mode, max_instances, out_buffer_size, in_buffer_size, default_timeout, security_attributes);
    if ((_return_value == INVALID_HANDLE_VALUE) && PyErr_Occurred()) {
        goto exit;
    }
    if (_return_value == NULL) {
        Py_RETURN_NONE;
    }
    return_value = HANDLE_TO_PYNUM(_return_value);

exit:
    /* Cleanup for name */
    PyMem_Free((void *)name);

    return return_value;
}

PyDoc_STRVAR(_winapi_CreatePipe__doc__,
"CreatePipe($module, pipe_attrs, size, /)\n"
"--\n"
"\n"
"Create an anonymous pipe.\n"
"\n"
"  pipe_attrs\n"
"    Ignored internally, can be None.\n"
"\n"
"Returns a 2-tuple of handles, to the read and write ends of the pipe.");

#define _WINAPI_CREATEPIPE_METHODDEF    \
    {"CreatePipe", _PyCFunction_CAST(_winapi_CreatePipe), METH_FASTCALL, _winapi_CreatePipe__doc__},

static PyObject *
_winapi_CreatePipe_impl(PyObject *module, PyObject *pipe_attrs, DWORD size);

static PyObject *
_winapi_CreatePipe(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *pipe_attrs;
    DWORD size;

    if (!_PyArg_ParseStack(args, nargs, "Ok:CreatePipe",
        &pipe_attrs, &size)) {
        goto exit;
    }
    return_value = _winapi_CreatePipe_impl(module, pipe_attrs, size);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_CreateProcess__doc__,
"CreateProcess($module, application_name, command_line, proc_attrs,\n"
"              thread_attrs, inherit_handles, creation_flags,\n"
"              env_mapping, current_directory, startup_info, /)\n"
"--\n"
"\n"
"Create a new process and its primary thread.\n"
"\n"
"  command_line\n"
"    Can be str or None\n"
"  proc_attrs\n"
"    Ignored internally, can be None.\n"
"  thread_attrs\n"
"    Ignored internally, can be None.\n"
"\n"
"The return value is a tuple of the process handle, thread handle,\n"
"process ID, and thread ID.");

#define _WINAPI_CREATEPROCESS_METHODDEF    \
    {"CreateProcess", _PyCFunction_CAST(_winapi_CreateProcess), METH_FASTCALL, _winapi_CreateProcess__doc__},

static PyObject *
_winapi_CreateProcess_impl(PyObject *module, const wchar_t *application_name,
                           PyObject *command_line, PyObject *proc_attrs,
                           PyObject *thread_attrs, BOOL inherit_handles,
                           DWORD creation_flags, PyObject *env_mapping,
                           const wchar_t *current_directory,
                           PyObject *startup_info);

static PyObject *
_winapi_CreateProcess(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    const wchar_t *application_name = NULL;
    PyObject *command_line;
    PyObject *proc_attrs;
    PyObject *thread_attrs;
    BOOL inherit_handles;
    DWORD creation_flags;
    PyObject *env_mapping;
    const wchar_t *current_directory = NULL;
    PyObject *startup_info;

    if (!_PyArg_ParseStack(args, nargs, "O&OOOikOO&O:CreateProcess",
        _PyUnicode_WideCharString_Opt_Converter, &application_name, &command_line, &proc_attrs, &thread_attrs, &inherit_handles, &creation_flags, &env_mapping, _PyUnicode_WideCharString_Opt_Converter, &current_directory, &startup_info)) {
        goto exit;
    }
    return_value = _winapi_CreateProcess_impl(module, application_name, command_line, proc_attrs, thread_attrs, inherit_handles, creation_flags, env_mapping, current_directory, startup_info);

exit:
    /* Cleanup for application_name */
    PyMem_Free((void *)application_name);
    /* Cleanup for current_directory */
    PyMem_Free((void *)current_directory);

    return return_value;
}

PyDoc_STRVAR(_winapi_DuplicateHandle__doc__,
"DuplicateHandle($module, source_process_handle, source_handle,\n"
"                target_process_handle, desired_access, inherit_handle,\n"
"                options=0, /)\n"
"--\n"
"\n"
"Return a duplicate handle object.\n"
"\n"
"The duplicate handle refers to the same object as the original\n"
"handle. Therefore, any changes to the object are reflected\n"
"through both handles.");

#define _WINAPI_DUPLICATEHANDLE_METHODDEF    \
    {"DuplicateHandle", _PyCFunction_CAST(_winapi_DuplicateHandle), METH_FASTCALL, _winapi_DuplicateHandle__doc__},

static HANDLE
_winapi_DuplicateHandle_impl(PyObject *module, HANDLE source_process_handle,
                             HANDLE source_handle,
                             HANDLE target_process_handle,
                             DWORD desired_access, BOOL inherit_handle,
                             DWORD options);

static PyObject *
_winapi_DuplicateHandle(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HANDLE source_process_handle;
    HANDLE source_handle;
    HANDLE target_process_handle;
    DWORD desired_access;
    BOOL inherit_handle;
    DWORD options = 0;
    HANDLE _return_value;

    if (!_PyArg_ParseStack(args, nargs, "" F_HANDLE "" F_HANDLE "" F_HANDLE "ki|k:DuplicateHandle",
        &source_process_handle, &source_handle, &target_process_handle, &desired_access, &inherit_handle, &options)) {
        goto exit;
    }
    _return_value = _winapi_DuplicateHandle_impl(module, source_process_handle, source_handle, target_process_handle, desired_access, inherit_handle, options);
    if ((_return_value == INVALID_HANDLE_VALUE) && PyErr_Occurred()) {
        goto exit;
    }
    if (_return_value == NULL) {
        Py_RETURN_NONE;
    }
    return_value = HANDLE_TO_PYNUM(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_ExitProcess__doc__,
"ExitProcess($module, ExitCode, /)\n"
"--\n"
"\n");

#define _WINAPI_EXITPROCESS_METHODDEF    \
    {"ExitProcess", (PyCFunction)_winapi_ExitProcess, METH_O, _winapi_ExitProcess__doc__},

static PyObject *
_winapi_ExitProcess_impl(PyObject *module, UINT ExitCode);

static PyObject *
_winapi_ExitProcess(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    UINT ExitCode;

    if (!PyArg_Parse(arg, "I:ExitProcess", &ExitCode)) {
        goto exit;
    }
    return_value = _winapi_ExitProcess_impl(module, ExitCode);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_GetCurrentProcess__doc__,
"GetCurrentProcess($module, /)\n"
"--\n"
"\n"
"Return a handle object for the current process.");

#define _WINAPI_GETCURRENTPROCESS_METHODDEF    \
    {"GetCurrentProcess", (PyCFunction)_winapi_GetCurrentProcess, METH_NOARGS, _winapi_GetCurrentProcess__doc__},

static HANDLE
_winapi_GetCurrentProcess_impl(PyObject *module);

static PyObject *
_winapi_GetCurrentProcess(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    HANDLE _return_value;

    _return_value = _winapi_GetCurrentProcess_impl(module);
    if ((_return_value == INVALID_HANDLE_VALUE) && PyErr_Occurred()) {
        goto exit;
    }
    if (_return_value == NULL) {
        Py_RETURN_NONE;
    }
    return_value = HANDLE_TO_PYNUM(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_GetExitCodeProcess__doc__,
"GetExitCodeProcess($module, process, /)\n"
"--\n"
"\n"
"Return the termination status of the specified process.");

#define _WINAPI_GETEXITCODEPROCESS_METHODDEF    \
    {"GetExitCodeProcess", (PyCFunction)_winapi_GetExitCodeProcess, METH_O, _winapi_GetExitCodeProcess__doc__},

static DWORD
_winapi_GetExitCodeProcess_impl(PyObject *module, HANDLE process);

static PyObject *
_winapi_GetExitCodeProcess(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    HANDLE process;
    DWORD _return_value;

    if (!PyArg_Parse(arg, "" F_HANDLE ":GetExitCodeProcess", &process)) {
        goto exit;
    }
    _return_value = _winapi_GetExitCodeProcess_impl(module, process);
    if ((_return_value == PY_DWORD_MAX) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromUnsignedLong(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_GetLastError__doc__,
"GetLastError($module, /)\n"
"--\n"
"\n");

#define _WINAPI_GETLASTERROR_METHODDEF    \
    {"GetLastError", (PyCFunction)_winapi_GetLastError, METH_NOARGS, _winapi_GetLastError__doc__},

static DWORD
_winapi_GetLastError_impl(PyObject *module);

static PyObject *
_winapi_GetLastError(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    DWORD _return_value;

    _return_value = _winapi_GetLastError_impl(module);
    if ((_return_value == PY_DWORD_MAX) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromUnsignedLong(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_GetLongPathName__doc__,
"GetLongPathName($module, /, path)\n"
"--\n"
"\n"
"Return the long version of the provided path.\n"
"\n"
"If the path is already in its long form, returns the same value.\n"
"\n"
"The path must already be a \'str\'. If the type is not known, use\n"
"os.fsdecode before calling this function.");

#define _WINAPI_GETLONGPATHNAME_METHODDEF    \
    {"GetLongPathName", _PyCFunction_CAST(_winapi_GetLongPathName), METH_FASTCALL|METH_KEYWORDS, _winapi_GetLongPathName__doc__},

static PyObject *
_winapi_GetLongPathName_impl(PyObject *module, LPCWSTR path);

static PyObject *
_winapi_GetLongPathName(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(path), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"path", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "GetLongPathName",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    LPCWSTR path = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("GetLongPathName", "argument 'path'", "str", args[0]);
        goto exit;
    }
    path = PyUnicode_AsWideCharString(args[0], NULL);
    if (path == NULL) {
        goto exit;
    }
    return_value = _winapi_GetLongPathName_impl(module, path);

exit:
    /* Cleanup for path */
    PyMem_Free((void *)path);

    return return_value;
}

PyDoc_STRVAR(_winapi_GetModuleFileName__doc__,
"GetModuleFileName($module, module_handle, /)\n"
"--\n"
"\n"
"Return the fully-qualified path for the file that contains module.\n"
"\n"
"The module must have been loaded by the current process.\n"
"\n"
"The module parameter should be a handle to the loaded module\n"
"whose path is being requested. If this parameter is 0,\n"
"GetModuleFileName retrieves the path of the executable file\n"
"of the current process.");

#define _WINAPI_GETMODULEFILENAME_METHODDEF    \
    {"GetModuleFileName", (PyCFunction)_winapi_GetModuleFileName, METH_O, _winapi_GetModuleFileName__doc__},

static PyObject *
_winapi_GetModuleFileName_impl(PyObject *module, HMODULE module_handle);

static PyObject *
_winapi_GetModuleFileName(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    HMODULE module_handle;

    if (!PyArg_Parse(arg, "" F_HANDLE ":GetModuleFileName", &module_handle)) {
        goto exit;
    }
    return_value = _winapi_GetModuleFileName_impl(module, module_handle);

exit:
    return return_value;
}

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM))

PyDoc_STRVAR(_winapi_GetShortPathName__doc__,
"GetShortPathName($module, /, path)\n"
"--\n"
"\n"
"Return the short version of the provided path.\n"
"\n"
"If the path is already in its short form, returns the same value.\n"
"\n"
"The path must already be a \'str\'. If the type is not known, use\n"
"os.fsdecode before calling this function.");

#define _WINAPI_GETSHORTPATHNAME_METHODDEF    \
    {"GetShortPathName", _PyCFunction_CAST(_winapi_GetShortPathName), METH_FASTCALL|METH_KEYWORDS, _winapi_GetShortPathName__doc__},

static PyObject *
_winapi_GetShortPathName_impl(PyObject *module, LPCWSTR path);

static PyObject *
_winapi_GetShortPathName(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(path), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"path", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "GetShortPathName",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    LPCWSTR path = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("GetShortPathName", "argument 'path'", "str", args[0]);
        goto exit;
    }
    path = PyUnicode_AsWideCharString(args[0], NULL);
    if (path == NULL) {
        goto exit;
    }
    return_value = _winapi_GetShortPathName_impl(module, path);

exit:
    /* Cleanup for path */
    PyMem_Free((void *)path);

    return return_value;
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM)) */

PyDoc_STRVAR(_winapi_GetStdHandle__doc__,
"GetStdHandle($module, std_handle, /)\n"
"--\n"
"\n"
"Return a handle to the specified standard device.\n"
"\n"
"  std_handle\n"
"    One of STD_INPUT_HANDLE, STD_OUTPUT_HANDLE, or STD_ERROR_HANDLE.\n"
"\n"
"The integer associated with the handle object is returned.");

#define _WINAPI_GETSTDHANDLE_METHODDEF    \
    {"GetStdHandle", (PyCFunction)_winapi_GetStdHandle, METH_O, _winapi_GetStdHandle__doc__},

static HANDLE
_winapi_GetStdHandle_impl(PyObject *module, DWORD std_handle);

static PyObject *
_winapi_GetStdHandle(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    DWORD std_handle;
    HANDLE _return_value;

    if (!PyArg_Parse(arg, "k:GetStdHandle", &std_handle)) {
        goto exit;
    }
    _return_value = _winapi_GetStdHandle_impl(module, std_handle);
    if ((_return_value == INVALID_HANDLE_VALUE) && PyErr_Occurred()) {
        goto exit;
    }
    if (_return_value == NULL) {
        Py_RETURN_NONE;
    }
    return_value = HANDLE_TO_PYNUM(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_GetVersion__doc__,
"GetVersion($module, /)\n"
"--\n"
"\n"
"Return the version number of the current operating system.");

#define _WINAPI_GETVERSION_METHODDEF    \
    {"GetVersion", (PyCFunction)_winapi_GetVersion, METH_NOARGS, _winapi_GetVersion__doc__},

static long
_winapi_GetVersion_impl(PyObject *module);

static PyObject *
_winapi_GetVersion(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    long _return_value;

    _return_value = _winapi_GetVersion_impl(module);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_MapViewOfFile__doc__,
"MapViewOfFile($module, file_map, desired_access, file_offset_high,\n"
"              file_offset_low, number_bytes, /)\n"
"--\n"
"\n");

#define _WINAPI_MAPVIEWOFFILE_METHODDEF    \
    {"MapViewOfFile", _PyCFunction_CAST(_winapi_MapViewOfFile), METH_FASTCALL, _winapi_MapViewOfFile__doc__},

static LPVOID
_winapi_MapViewOfFile_impl(PyObject *module, HANDLE file_map,
                           DWORD desired_access, DWORD file_offset_high,
                           DWORD file_offset_low, size_t number_bytes);

static PyObject *
_winapi_MapViewOfFile(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HANDLE file_map;
    DWORD desired_access;
    DWORD file_offset_high;
    DWORD file_offset_low;
    size_t number_bytes;
    LPVOID _return_value;

    if (!_PyArg_ParseStack(args, nargs, "" F_HANDLE "kkkO&:MapViewOfFile",
        &file_map, &desired_access, &file_offset_high, &file_offset_low, _PyLong_Size_t_Converter, &number_bytes)) {
        goto exit;
    }
    _return_value = _winapi_MapViewOfFile_impl(module, file_map, desired_access, file_offset_high, file_offset_low, number_bytes);
    if ((_return_value == NULL) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = HANDLE_TO_PYNUM(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_UnmapViewOfFile__doc__,
"UnmapViewOfFile($module, address, /)\n"
"--\n"
"\n");

#define _WINAPI_UNMAPVIEWOFFILE_METHODDEF    \
    {"UnmapViewOfFile", (PyCFunction)_winapi_UnmapViewOfFile, METH_O, _winapi_UnmapViewOfFile__doc__},

static PyObject *
_winapi_UnmapViewOfFile_impl(PyObject *module, LPCVOID address);

static PyObject *
_winapi_UnmapViewOfFile(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    LPCVOID address;

    if (!PyArg_Parse(arg, "" F_POINTER ":UnmapViewOfFile", &address)) {
        goto exit;
    }
    return_value = _winapi_UnmapViewOfFile_impl(module, address);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_OpenEventW__doc__,
"OpenEventW($module, /, desired_access, inherit_handle, name)\n"
"--\n"
"\n");

#define _WINAPI_OPENEVENTW_METHODDEF    \
    {"OpenEventW", _PyCFunction_CAST(_winapi_OpenEventW), METH_FASTCALL|METH_KEYWORDS, _winapi_OpenEventW__doc__},

static HANDLE
_winapi_OpenEventW_impl(PyObject *module, DWORD desired_access,
                        BOOL inherit_handle, LPCWSTR name);

static PyObject *
_winapi_OpenEventW(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(desired_access), &_Py_ID(inherit_handle), &_Py_ID(name), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"desired_access", "inherit_handle", "name", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .format = "kiO&:OpenEventW",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    DWORD desired_access;
    BOOL inherit_handle;
    LPCWSTR name = NULL;
    HANDLE _return_value;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &desired_access, &inherit_handle, _PyUnicode_WideCharString_Converter, &name)) {
        goto exit;
    }
    _return_value = _winapi_OpenEventW_impl(module, desired_access, inherit_handle, name);
    if ((_return_value == INVALID_HANDLE_VALUE) && PyErr_Occurred()) {
        goto exit;
    }
    if (_return_value == NULL) {
        Py_RETURN_NONE;
    }
    return_value = HANDLE_TO_PYNUM(_return_value);

exit:
    /* Cleanup for name */
    PyMem_Free((void *)name);

    return return_value;
}

PyDoc_STRVAR(_winapi_OpenMutexW__doc__,
"OpenMutexW($module, /, desired_access, inherit_handle, name)\n"
"--\n"
"\n");

#define _WINAPI_OPENMUTEXW_METHODDEF    \
    {"OpenMutexW", _PyCFunction_CAST(_winapi_OpenMutexW), METH_FASTCALL|METH_KEYWORDS, _winapi_OpenMutexW__doc__},

static HANDLE
_winapi_OpenMutexW_impl(PyObject *module, DWORD desired_access,
                        BOOL inherit_handle, LPCWSTR name);

static PyObject *
_winapi_OpenMutexW(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(desired_access), &_Py_ID(inherit_handle), &_Py_ID(name), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"desired_access", "inherit_handle", "name", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .format = "kiO&:OpenMutexW",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    DWORD desired_access;
    BOOL inherit_handle;
    LPCWSTR name = NULL;
    HANDLE _return_value;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &desired_access, &inherit_handle, _PyUnicode_WideCharString_Converter, &name)) {
        goto exit;
    }
    _return_value = _winapi_OpenMutexW_impl(module, desired_access, inherit_handle, name);
    if ((_return_value == INVALID_HANDLE_VALUE) && PyErr_Occurred()) {
        goto exit;
    }
    if (_return_value == NULL) {
        Py_RETURN_NONE;
    }
    return_value = HANDLE_TO_PYNUM(_return_value);

exit:
    /* Cleanup for name */
    PyMem_Free((void *)name);

    return return_value;
}

PyDoc_STRVAR(_winapi_OpenFileMapping__doc__,
"OpenFileMapping($module, desired_access, inherit_handle, name, /)\n"
"--\n"
"\n");

#define _WINAPI_OPENFILEMAPPING_METHODDEF    \
    {"OpenFileMapping", _PyCFunction_CAST(_winapi_OpenFileMapping), METH_FASTCALL, _winapi_OpenFileMapping__doc__},

static HANDLE
_winapi_OpenFileMapping_impl(PyObject *module, DWORD desired_access,
                             BOOL inherit_handle, LPCWSTR name);

static PyObject *
_winapi_OpenFileMapping(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    DWORD desired_access;
    BOOL inherit_handle;
    LPCWSTR name = NULL;
    HANDLE _return_value;

    if (!_PyArg_ParseStack(args, nargs, "kiO&:OpenFileMapping",
        &desired_access, &inherit_handle, _PyUnicode_WideCharString_Converter, &name)) {
        goto exit;
    }
    _return_value = _winapi_OpenFileMapping_impl(module, desired_access, inherit_handle, name);
    if ((_return_value == INVALID_HANDLE_VALUE) && PyErr_Occurred()) {
        goto exit;
    }
    if (_return_value == NULL) {
        Py_RETURN_NONE;
    }
    return_value = HANDLE_TO_PYNUM(_return_value);

exit:
    /* Cleanup for name */
    PyMem_Free((void *)name);

    return return_value;
}

PyDoc_STRVAR(_winapi_OpenProcess__doc__,
"OpenProcess($module, desired_access, inherit_handle, process_id, /)\n"
"--\n"
"\n");

#define _WINAPI_OPENPROCESS_METHODDEF    \
    {"OpenProcess", _PyCFunction_CAST(_winapi_OpenProcess), METH_FASTCALL, _winapi_OpenProcess__doc__},

static HANDLE
_winapi_OpenProcess_impl(PyObject *module, DWORD desired_access,
                         BOOL inherit_handle, DWORD process_id);

static PyObject *
_winapi_OpenProcess(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    DWORD desired_access;
    BOOL inherit_handle;
    DWORD process_id;
    HANDLE _return_value;

    if (!_PyArg_ParseStack(args, nargs, "kik:OpenProcess",
        &desired_access, &inherit_handle, &process_id)) {
        goto exit;
    }
    _return_value = _winapi_OpenProcess_impl(module, desired_access, inherit_handle, process_id);
    if ((_return_value == INVALID_HANDLE_VALUE) && PyErr_Occurred()) {
        goto exit;
    }
    if (_return_value == NULL) {
        Py_RETURN_NONE;
    }
    return_value = HANDLE_TO_PYNUM(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_PeekNamedPipe__doc__,
"PeekNamedPipe($module, handle, size=0, /)\n"
"--\n"
"\n");

#define _WINAPI_PEEKNAMEDPIPE_METHODDEF    \
    {"PeekNamedPipe", _PyCFunction_CAST(_winapi_PeekNamedPipe), METH_FASTCALL, _winapi_PeekNamedPipe__doc__},

static PyObject *
_winapi_PeekNamedPipe_impl(PyObject *module, HANDLE handle, int size);

static PyObject *
_winapi_PeekNamedPipe(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HANDLE handle;
    int size = 0;

    if (!_PyArg_ParseStack(args, nargs, "" F_HANDLE "|i:PeekNamedPipe",
        &handle, &size)) {
        goto exit;
    }
    return_value = _winapi_PeekNamedPipe_impl(module, handle, size);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_LCMapStringEx__doc__,
"LCMapStringEx($module, /, locale, flags, src)\n"
"--\n"
"\n");

#define _WINAPI_LCMAPSTRINGEX_METHODDEF    \
    {"LCMapStringEx", _PyCFunction_CAST(_winapi_LCMapStringEx), METH_FASTCALL|METH_KEYWORDS, _winapi_LCMapStringEx__doc__},

static PyObject *
_winapi_LCMapStringEx_impl(PyObject *module, LPCWSTR locale, DWORD flags,
                           PyObject *src);

static PyObject *
_winapi_LCMapStringEx(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(locale), &_Py_ID(flags), &_Py_ID(src), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"locale", "flags", "src", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .format = "O&kU:LCMapStringEx",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    LPCWSTR locale = NULL;
    DWORD flags;
    PyObject *src;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        _PyUnicode_WideCharString_Converter, &locale, &flags, &src)) {
        goto exit;
    }
    return_value = _winapi_LCMapStringEx_impl(module, locale, flags, src);

exit:
    /* Cleanup for locale */
    PyMem_Free((void *)locale);

    return return_value;
}

PyDoc_STRVAR(_winapi_ReadFile__doc__,
"ReadFile($module, /, handle, size, overlapped=False)\n"
"--\n"
"\n");

#define _WINAPI_READFILE_METHODDEF    \
    {"ReadFile", _PyCFunction_CAST(_winapi_ReadFile), METH_FASTCALL|METH_KEYWORDS, _winapi_ReadFile__doc__},

static PyObject *
_winapi_ReadFile_impl(PyObject *module, HANDLE handle, DWORD size,
                      int use_overlapped);

static PyObject *
_winapi_ReadFile(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(handle), &_Py_ID(size), &_Py_ID(overlapped), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"handle", "size", "overlapped", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .format = "" F_HANDLE "k|p:ReadFile",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    HANDLE handle;
    DWORD size;
    int use_overlapped = 0;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &handle, &size, &use_overlapped)) {
        goto exit;
    }
    return_value = _winapi_ReadFile_impl(module, handle, size, use_overlapped);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_ReleaseMutex__doc__,
"ReleaseMutex($module, /, mutex)\n"
"--\n"
"\n");

#define _WINAPI_RELEASEMUTEX_METHODDEF    \
    {"ReleaseMutex", _PyCFunction_CAST(_winapi_ReleaseMutex), METH_FASTCALL|METH_KEYWORDS, _winapi_ReleaseMutex__doc__},

static PyObject *
_winapi_ReleaseMutex_impl(PyObject *module, HANDLE mutex);

static PyObject *
_winapi_ReleaseMutex(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(mutex), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"mutex", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .format = "" F_HANDLE ":ReleaseMutex",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    HANDLE mutex;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &mutex)) {
        goto exit;
    }
    return_value = _winapi_ReleaseMutex_impl(module, mutex);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_ResetEvent__doc__,
"ResetEvent($module, /, event)\n"
"--\n"
"\n");

#define _WINAPI_RESETEVENT_METHODDEF    \
    {"ResetEvent", _PyCFunction_CAST(_winapi_ResetEvent), METH_FASTCALL|METH_KEYWORDS, _winapi_ResetEvent__doc__},

static PyObject *
_winapi_ResetEvent_impl(PyObject *module, HANDLE event);

static PyObject *
_winapi_ResetEvent(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(event), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"event", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .format = "" F_HANDLE ":ResetEvent",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    HANDLE event;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &event)) {
        goto exit;
    }
    return_value = _winapi_ResetEvent_impl(module, event);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_SetEvent__doc__,
"SetEvent($module, /, event)\n"
"--\n"
"\n");

#define _WINAPI_SETEVENT_METHODDEF    \
    {"SetEvent", _PyCFunction_CAST(_winapi_SetEvent), METH_FASTCALL|METH_KEYWORDS, _winapi_SetEvent__doc__},

static PyObject *
_winapi_SetEvent_impl(PyObject *module, HANDLE event);

static PyObject *
_winapi_SetEvent(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(event), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"event", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .format = "" F_HANDLE ":SetEvent",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    HANDLE event;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &event)) {
        goto exit;
    }
    return_value = _winapi_SetEvent_impl(module, event);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_SetNamedPipeHandleState__doc__,
"SetNamedPipeHandleState($module, named_pipe, mode,\n"
"                        max_collection_count, collect_data_timeout, /)\n"
"--\n"
"\n");

#define _WINAPI_SETNAMEDPIPEHANDLESTATE_METHODDEF    \
    {"SetNamedPipeHandleState", _PyCFunction_CAST(_winapi_SetNamedPipeHandleState), METH_FASTCALL, _winapi_SetNamedPipeHandleState__doc__},

static PyObject *
_winapi_SetNamedPipeHandleState_impl(PyObject *module, HANDLE named_pipe,
                                     PyObject *mode,
                                     PyObject *max_collection_count,
                                     PyObject *collect_data_timeout);

static PyObject *
_winapi_SetNamedPipeHandleState(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HANDLE named_pipe;
    PyObject *mode;
    PyObject *max_collection_count;
    PyObject *collect_data_timeout;

    if (!_PyArg_ParseStack(args, nargs, "" F_HANDLE "OOO:SetNamedPipeHandleState",
        &named_pipe, &mode, &max_collection_count, &collect_data_timeout)) {
        goto exit;
    }
    return_value = _winapi_SetNamedPipeHandleState_impl(module, named_pipe, mode, max_collection_count, collect_data_timeout);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_TerminateProcess__doc__,
"TerminateProcess($module, handle, exit_code, /)\n"
"--\n"
"\n"
"Terminate the specified process and all of its threads.");

#define _WINAPI_TERMINATEPROCESS_METHODDEF    \
    {"TerminateProcess", _PyCFunction_CAST(_winapi_TerminateProcess), METH_FASTCALL, _winapi_TerminateProcess__doc__},

static PyObject *
_winapi_TerminateProcess_impl(PyObject *module, HANDLE handle,
                              UINT exit_code);

static PyObject *
_winapi_TerminateProcess(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HANDLE handle;
    UINT exit_code;

    if (!_PyArg_ParseStack(args, nargs, "" F_HANDLE "I:TerminateProcess",
        &handle, &exit_code)) {
        goto exit;
    }
    return_value = _winapi_TerminateProcess_impl(module, handle, exit_code);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_VirtualQuerySize__doc__,
"VirtualQuerySize($module, address, /)\n"
"--\n"
"\n");

#define _WINAPI_VIRTUALQUERYSIZE_METHODDEF    \
    {"VirtualQuerySize", (PyCFunction)_winapi_VirtualQuerySize, METH_O, _winapi_VirtualQuerySize__doc__},

static size_t
_winapi_VirtualQuerySize_impl(PyObject *module, LPCVOID address);

static PyObject *
_winapi_VirtualQuerySize(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    LPCVOID address;
    size_t _return_value;

    if (!PyArg_Parse(arg, "" F_POINTER ":VirtualQuerySize", &address)) {
        goto exit;
    }
    _return_value = _winapi_VirtualQuerySize_impl(module, address);
    if ((_return_value == (size_t)-1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSize_t(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_WaitNamedPipe__doc__,
"WaitNamedPipe($module, name, timeout, /)\n"
"--\n"
"\n");

#define _WINAPI_WAITNAMEDPIPE_METHODDEF    \
    {"WaitNamedPipe", _PyCFunction_CAST(_winapi_WaitNamedPipe), METH_FASTCALL, _winapi_WaitNamedPipe__doc__},

static PyObject *
_winapi_WaitNamedPipe_impl(PyObject *module, LPCWSTR name, DWORD timeout);

static PyObject *
_winapi_WaitNamedPipe(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    LPCWSTR name = NULL;
    DWORD timeout;

    if (!_PyArg_ParseStack(args, nargs, "O&k:WaitNamedPipe",
        _PyUnicode_WideCharString_Converter, &name, &timeout)) {
        goto exit;
    }
    return_value = _winapi_WaitNamedPipe_impl(module, name, timeout);

exit:
    /* Cleanup for name */
    PyMem_Free((void *)name);

    return return_value;
}

PyDoc_STRVAR(_winapi_BatchedWaitForMultipleObjects__doc__,
"BatchedWaitForMultipleObjects($module, /, handle_seq, wait_all,\n"
"                              milliseconds=_winapi.INFINITE)\n"
"--\n"
"\n"
"Supports a larger number of handles than WaitForMultipleObjects\n"
"\n"
"Note that the handles may be waited on other threads, which could cause\n"
"issues for objects like mutexes that become associated with the thread\n"
"that was waiting for them. Objects may also be left signalled, even if\n"
"the wait fails.\n"
"\n"
"It is recommended to use WaitForMultipleObjects whenever possible, and\n"
"only switch to BatchedWaitForMultipleObjects for scenarios where you\n"
"control all the handles involved, such as your own thread pool or\n"
"files, and all wait objects are left unmodified by a wait (for example,\n"
"manual reset events, threads, and files/pipes).\n"
"\n"
"Overlapped handles returned from this module use manual reset events.");

#define _WINAPI_BATCHEDWAITFORMULTIPLEOBJECTS_METHODDEF    \
    {"BatchedWaitForMultipleObjects", _PyCFunction_CAST(_winapi_BatchedWaitForMultipleObjects), METH_FASTCALL|METH_KEYWORDS, _winapi_BatchedWaitForMultipleObjects__doc__},

static PyObject *
_winapi_BatchedWaitForMultipleObjects_impl(PyObject *module,
                                           PyObject *handle_seq,
                                           BOOL wait_all, DWORD milliseconds);

static PyObject *
_winapi_BatchedWaitForMultipleObjects(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(handle_seq), &_Py_ID(wait_all), &_Py_ID(milliseconds), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"handle_seq", "wait_all", "milliseconds", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .format = "Oi|k:BatchedWaitForMultipleObjects",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *handle_seq;
    BOOL wait_all;
    DWORD milliseconds = INFINITE;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &handle_seq, &wait_all, &milliseconds)) {
        goto exit;
    }
    return_value = _winapi_BatchedWaitForMultipleObjects_impl(module, handle_seq, wait_all, milliseconds);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_WaitForMultipleObjects__doc__,
"WaitForMultipleObjects($module, handle_seq, wait_flag,\n"
"                       milliseconds=_winapi.INFINITE, /)\n"
"--\n"
"\n");

#define _WINAPI_WAITFORMULTIPLEOBJECTS_METHODDEF    \
    {"WaitForMultipleObjects", _PyCFunction_CAST(_winapi_WaitForMultipleObjects), METH_FASTCALL, _winapi_WaitForMultipleObjects__doc__},

static PyObject *
_winapi_WaitForMultipleObjects_impl(PyObject *module, PyObject *handle_seq,
                                    BOOL wait_flag, DWORD milliseconds);

static PyObject *
_winapi_WaitForMultipleObjects(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *handle_seq;
    BOOL wait_flag;
    DWORD milliseconds = INFINITE;

    if (!_PyArg_ParseStack(args, nargs, "Oi|k:WaitForMultipleObjects",
        &handle_seq, &wait_flag, &milliseconds)) {
        goto exit;
    }
    return_value = _winapi_WaitForMultipleObjects_impl(module, handle_seq, wait_flag, milliseconds);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_WaitForSingleObject__doc__,
"WaitForSingleObject($module, handle, milliseconds, /)\n"
"--\n"
"\n"
"Wait for a single object.\n"
"\n"
"Wait until the specified object is in the signaled state or\n"
"the time-out interval elapses. The timeout value is specified\n"
"in milliseconds.");

#define _WINAPI_WAITFORSINGLEOBJECT_METHODDEF    \
    {"WaitForSingleObject", _PyCFunction_CAST(_winapi_WaitForSingleObject), METH_FASTCALL, _winapi_WaitForSingleObject__doc__},

static long
_winapi_WaitForSingleObject_impl(PyObject *module, HANDLE handle,
                                 DWORD milliseconds);

static PyObject *
_winapi_WaitForSingleObject(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    HANDLE handle;
    DWORD milliseconds;
    long _return_value;

    if (!_PyArg_ParseStack(args, nargs, "" F_HANDLE "k:WaitForSingleObject",
        &handle, &milliseconds)) {
        goto exit;
    }
    _return_value = _winapi_WaitForSingleObject_impl(module, handle, milliseconds);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_WriteFile__doc__,
"WriteFile($module, /, handle, buffer, overlapped=False)\n"
"--\n"
"\n");

#define _WINAPI_WRITEFILE_METHODDEF    \
    {"WriteFile", _PyCFunction_CAST(_winapi_WriteFile), METH_FASTCALL|METH_KEYWORDS, _winapi_WriteFile__doc__},

static PyObject *
_winapi_WriteFile_impl(PyObject *module, HANDLE handle, PyObject *buffer,
                       int use_overlapped);

static PyObject *
_winapi_WriteFile(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(handle), &_Py_ID(buffer), &_Py_ID(overlapped), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"handle", "buffer", "overlapped", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .format = "" F_HANDLE "O|p:WriteFile",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    HANDLE handle;
    PyObject *buffer;
    int use_overlapped = 0;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &handle, &buffer, &use_overlapped)) {
        goto exit;
    }
    return_value = _winapi_WriteFile_impl(module, handle, buffer, use_overlapped);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_GetACP__doc__,
"GetACP($module, /)\n"
"--\n"
"\n"
"Get the current Windows ANSI code page identifier.");

#define _WINAPI_GETACP_METHODDEF    \
    {"GetACP", (PyCFunction)_winapi_GetACP, METH_NOARGS, _winapi_GetACP__doc__},

static PyObject *
_winapi_GetACP_impl(PyObject *module);

static PyObject *
_winapi_GetACP(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _winapi_GetACP_impl(module);
}

PyDoc_STRVAR(_winapi_GetOEMCP__doc__,
"GetOEMCP($module, /)\n"
"--\n"
"\n"
"Get the current Windows ANSI code page identifier.");

#define _WINAPI_GETOEMCP_METHODDEF    \
    {"GetOEMCP", (PyCFunction)_winapi_GetOEMCP, METH_NOARGS, _winapi_GetOEMCP__doc__},

static PyObject *
_winapi_GetOEMCP_impl(PyObject *module);

static PyObject *
_winapi_GetOEMCP(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _winapi_GetOEMCP_impl(module);
}

PyDoc_STRVAR(_winapi_GetFileType__doc__,
"GetFileType($module, /, handle)\n"
"--\n"
"\n");

#define _WINAPI_GETFILETYPE_METHODDEF    \
    {"GetFileType", _PyCFunction_CAST(_winapi_GetFileType), METH_FASTCALL|METH_KEYWORDS, _winapi_GetFileType__doc__},

static DWORD
_winapi_GetFileType_impl(PyObject *module, HANDLE handle);

static PyObject *
_winapi_GetFileType(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(handle), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"handle", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .format = "" F_HANDLE ":GetFileType",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    HANDLE handle;
    DWORD _return_value;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &handle)) {
        goto exit;
    }
    _return_value = _winapi_GetFileType_impl(module, handle);
    if ((_return_value == PY_DWORD_MAX) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromUnsignedLong(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi__mimetypes_read_windows_registry__doc__,
"_mimetypes_read_windows_registry($module, /, on_type_read)\n"
"--\n"
"\n"
"Optimized function for reading all known MIME types from the registry.\n"
"\n"
"*on_type_read* is a callable taking *type* and *ext* arguments, as for\n"
"MimeTypes.add_type.");

#define _WINAPI__MIMETYPES_READ_WINDOWS_REGISTRY_METHODDEF    \
    {"_mimetypes_read_windows_registry", _PyCFunction_CAST(_winapi__mimetypes_read_windows_registry), METH_FASTCALL|METH_KEYWORDS, _winapi__mimetypes_read_windows_registry__doc__},

static PyObject *
_winapi__mimetypes_read_windows_registry_impl(PyObject *module,
                                              PyObject *on_type_read);

static PyObject *
_winapi__mimetypes_read_windows_registry(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(on_type_read), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"on_type_read", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_mimetypes_read_windows_registry",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *on_type_read;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    on_type_read = args[0];
    return_value = _winapi__mimetypes_read_windows_registry_impl(module, on_type_read);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_NeedCurrentDirectoryForExePath__doc__,
"NeedCurrentDirectoryForExePath($module, exe_name, /)\n"
"--\n"
"\n");

#define _WINAPI_NEEDCURRENTDIRECTORYFOREXEPATH_METHODDEF    \
    {"NeedCurrentDirectoryForExePath", (PyCFunction)_winapi_NeedCurrentDirectoryForExePath, METH_O, _winapi_NeedCurrentDirectoryForExePath__doc__},

static int
_winapi_NeedCurrentDirectoryForExePath_impl(PyObject *module,
                                            LPCWSTR exe_name);

static PyObject *
_winapi_NeedCurrentDirectoryForExePath(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    LPCWSTR exe_name = NULL;
    int _return_value;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("NeedCurrentDirectoryForExePath", "argument", "str", arg);
        goto exit;
    }
    exe_name = PyUnicode_AsWideCharString(arg, NULL);
    if (exe_name == NULL) {
        goto exit;
    }
    _return_value = _winapi_NeedCurrentDirectoryForExePath_impl(module, exe_name);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    /* Cleanup for exe_name */
    PyMem_Free((void *)exe_name);

    return return_value;
}

PyDoc_STRVAR(_winapi_CopyFile2__doc__,
"CopyFile2($module, /, existing_file_name, new_file_name, flags,\n"
"          progress_routine=None)\n"
"--\n"
"\n"
"Copies a file from one name to a new name.\n"
"\n"
"This is implemented using the CopyFile2 API, which preserves all stat\n"
"and metadata information apart from security attributes.\n"
"\n"
"progress_routine is reserved for future use, but is currently not\n"
"implemented. Its value is ignored.");

#define _WINAPI_COPYFILE2_METHODDEF    \
    {"CopyFile2", _PyCFunction_CAST(_winapi_CopyFile2), METH_FASTCALL|METH_KEYWORDS, _winapi_CopyFile2__doc__},

static PyObject *
_winapi_CopyFile2_impl(PyObject *module, LPCWSTR existing_file_name,
                       LPCWSTR new_file_name, DWORD flags,
                       PyObject *progress_routine);

static PyObject *
_winapi_CopyFile2(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(existing_file_name), &_Py_ID(new_file_name), &_Py_ID(flags), &_Py_ID(progress_routine), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"existing_file_name", "new_file_name", "flags", "progress_routine", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .format = "O&O&k|O:CopyFile2",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    LPCWSTR existing_file_name = NULL;
    LPCWSTR new_file_name = NULL;
    DWORD flags;
    PyObject *progress_routine = Py_None;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        _PyUnicode_WideCharString_Converter, &existing_file_name, _PyUnicode_WideCharString_Converter, &new_file_name, &flags, &progress_routine)) {
        goto exit;
    }
    return_value = _winapi_CopyFile2_impl(module, existing_file_name, new_file_name, flags, progress_routine);

exit:
    /* Cleanup for existing_file_name */
    PyMem_Free((void *)existing_file_name);
    /* Cleanup for new_file_name */
    PyMem_Free((void *)new_file_name);

    return return_value;
}

#ifndef _WINAPI_GETSHORTPATHNAME_METHODDEF
    #define _WINAPI_GETSHORTPATHNAME_METHODDEF
#endif /* !defined(_WINAPI_GETSHORTPATHNAME_METHODDEF) */
/*[clinic end generated code: output=4581fd481c3c6293 input=a9049054013a1b77]*/
