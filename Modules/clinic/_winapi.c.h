/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_winapi_Overlapped_GetOverlappedResult__doc__,
"GetOverlappedResult($self, wait, /)\n"
"--\n"
"\n");

#define _WINAPI_OVERLAPPED_GETOVERLAPPEDRESULT_METHODDEF    \
    {"GetOverlappedResult", (PyCFunction)_winapi_Overlapped_GetOverlappedResult, METH_O, _winapi_Overlapped_GetOverlappedResult__doc__},

static PyObject *
_winapi_Overlapped_GetOverlappedResult_impl(OverlappedObject *self, int wait);

static PyObject *
_winapi_Overlapped_GetOverlappedResult(OverlappedObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    int wait;

    wait = PyObject_IsTrue(arg);
    if (wait < 0) {
        goto exit;
    }
    return_value = _winapi_Overlapped_GetOverlappedResult_impl(self, wait);

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
_winapi_Overlapped_getbuffer(OverlappedObject *self, PyObject *Py_UNUSED(ignored))
{
    return _winapi_Overlapped_getbuffer_impl(self);
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
_winapi_Overlapped_cancel(OverlappedObject *self, PyObject *Py_UNUSED(ignored))
{
    return _winapi_Overlapped_cancel_impl(self);
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
    {"ConnectNamedPipe", (PyCFunction)(void(*)(void))_winapi_ConnectNamedPipe, METH_FASTCALL|METH_KEYWORDS, _winapi_ConnectNamedPipe__doc__},

static PyObject *
_winapi_ConnectNamedPipe_impl(PyObject *module, HANDLE handle,
                              int use_overlapped);

static PyObject *
_winapi_ConnectNamedPipe(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"handle", "overlapped", NULL};
    static _PyArg_Parser _parser = {"" F_HANDLE "|i:ConnectNamedPipe", _keywords, 0};
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

PyDoc_STRVAR(_winapi_CreateFile__doc__,
"CreateFile($module, file_name, desired_access, share_mode,\n"
"           security_attributes, creation_disposition,\n"
"           flags_and_attributes, template_file, /)\n"
"--\n"
"\n");

#define _WINAPI_CREATEFILE_METHODDEF    \
    {"CreateFile", (PyCFunction)(void(*)(void))_winapi_CreateFile, METH_FASTCALL, _winapi_CreateFile__doc__},

static HANDLE
_winapi_CreateFile_impl(PyObject *module, LPCTSTR file_name,
                        DWORD desired_access, DWORD share_mode,
                        LPSECURITY_ATTRIBUTES security_attributes,
                        DWORD creation_disposition,
                        DWORD flags_and_attributes, HANDLE template_file);

static PyObject *
_winapi_CreateFile(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    LPCTSTR file_name;
    DWORD desired_access;
    DWORD share_mode;
    LPSECURITY_ATTRIBUTES security_attributes;
    DWORD creation_disposition;
    DWORD flags_and_attributes;
    HANDLE template_file;
    HANDLE _return_value;

    if (!_PyArg_ParseStack(args, nargs, "skk" F_POINTER "kk" F_HANDLE ":CreateFile",
        &file_name, &desired_access, &share_mode, &security_attributes, &creation_disposition, &flags_and_attributes, &template_file)) {
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
    return return_value;
}

PyDoc_STRVAR(_winapi_CreateFileMapping__doc__,
"CreateFileMapping($module, file_handle, security_attributes, protect,\n"
"                  max_size_high, max_size_low, name, /)\n"
"--\n"
"\n");

#define _WINAPI_CREATEFILEMAPPING_METHODDEF    \
    {"CreateFileMapping", (PyCFunction)(void(*)(void))_winapi_CreateFileMapping, METH_FASTCALL, _winapi_CreateFileMapping__doc__},

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
    LPCWSTR name;
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
    #if !USE_UNICODE_WCHAR_CACHE
    PyMem_Free((void *)name);
    #endif /* USE_UNICODE_WCHAR_CACHE */

    return return_value;
}

PyDoc_STRVAR(_winapi_CreateJunction__doc__,
"CreateJunction($module, src_path, dst_path, /)\n"
"--\n"
"\n");

#define _WINAPI_CREATEJUNCTION_METHODDEF    \
    {"CreateJunction", (PyCFunction)(void(*)(void))_winapi_CreateJunction, METH_FASTCALL, _winapi_CreateJunction__doc__},

static PyObject *
_winapi_CreateJunction_impl(PyObject *module, LPCWSTR src_path,
                            LPCWSTR dst_path);

static PyObject *
_winapi_CreateJunction(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    LPCWSTR src_path;
    LPCWSTR dst_path;

    if (!_PyArg_CheckPositional("CreateJunction", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("CreateJunction", "argument 1", "str", args[0]);
        goto exit;
    }
    #if USE_UNICODE_WCHAR_CACHE
    src_path = _PyUnicode_AsUnicode(args[0]);
    #else /* USE_UNICODE_WCHAR_CACHE */
    src_path = PyUnicode_AsWideCharString(args[0], NULL);
    #endif /* USE_UNICODE_WCHAR_CACHE */
    if (src_path == NULL) {
        goto exit;
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("CreateJunction", "argument 2", "str", args[1]);
        goto exit;
    }
    #if USE_UNICODE_WCHAR_CACHE
    dst_path = _PyUnicode_AsUnicode(args[1]);
    #else /* USE_UNICODE_WCHAR_CACHE */
    dst_path = PyUnicode_AsWideCharString(args[1], NULL);
    #endif /* USE_UNICODE_WCHAR_CACHE */
    if (dst_path == NULL) {
        goto exit;
    }
    return_value = _winapi_CreateJunction_impl(module, src_path, dst_path);

exit:
    /* Cleanup for src_path */
    #if !USE_UNICODE_WCHAR_CACHE
    PyMem_Free((void *)src_path);
    #endif /* USE_UNICODE_WCHAR_CACHE */
    /* Cleanup for dst_path */
    #if !USE_UNICODE_WCHAR_CACHE
    PyMem_Free((void *)dst_path);
    #endif /* USE_UNICODE_WCHAR_CACHE */

    return return_value;
}

PyDoc_STRVAR(_winapi_CreateNamedPipe__doc__,
"CreateNamedPipe($module, name, open_mode, pipe_mode, max_instances,\n"
"                out_buffer_size, in_buffer_size, default_timeout,\n"
"                security_attributes, /)\n"
"--\n"
"\n");

#define _WINAPI_CREATENAMEDPIPE_METHODDEF    \
    {"CreateNamedPipe", (PyCFunction)(void(*)(void))_winapi_CreateNamedPipe, METH_FASTCALL, _winapi_CreateNamedPipe__doc__},

static HANDLE
_winapi_CreateNamedPipe_impl(PyObject *module, LPCTSTR name, DWORD open_mode,
                             DWORD pipe_mode, DWORD max_instances,
                             DWORD out_buffer_size, DWORD in_buffer_size,
                             DWORD default_timeout,
                             LPSECURITY_ATTRIBUTES security_attributes);

static PyObject *
_winapi_CreateNamedPipe(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    LPCTSTR name;
    DWORD open_mode;
    DWORD pipe_mode;
    DWORD max_instances;
    DWORD out_buffer_size;
    DWORD in_buffer_size;
    DWORD default_timeout;
    LPSECURITY_ATTRIBUTES security_attributes;
    HANDLE _return_value;

    if (!_PyArg_ParseStack(args, nargs, "skkkkkk" F_POINTER ":CreateNamedPipe",
        &name, &open_mode, &pipe_mode, &max_instances, &out_buffer_size, &in_buffer_size, &default_timeout, &security_attributes)) {
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
    {"CreatePipe", (PyCFunction)(void(*)(void))_winapi_CreatePipe, METH_FASTCALL, _winapi_CreatePipe__doc__},

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
    {"CreateProcess", (PyCFunction)(void(*)(void))_winapi_CreateProcess, METH_FASTCALL, _winapi_CreateProcess__doc__},

static PyObject *
_winapi_CreateProcess_impl(PyObject *module,
                           const Py_UNICODE *application_name,
                           PyObject *command_line, PyObject *proc_attrs,
                           PyObject *thread_attrs, BOOL inherit_handles,
                           DWORD creation_flags, PyObject *env_mapping,
                           const Py_UNICODE *current_directory,
                           PyObject *startup_info);

static PyObject *
_winapi_CreateProcess(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    const Py_UNICODE *application_name;
    PyObject *command_line;
    PyObject *proc_attrs;
    PyObject *thread_attrs;
    BOOL inherit_handles;
    DWORD creation_flags;
    PyObject *env_mapping;
    const Py_UNICODE *current_directory;
    PyObject *startup_info;

    if (!_PyArg_ParseStack(args, nargs, "O&OOOikOO&O:CreateProcess",
        _PyUnicode_WideCharString_Opt_Converter, &application_name, &command_line, &proc_attrs, &thread_attrs, &inherit_handles, &creation_flags, &env_mapping, _PyUnicode_WideCharString_Opt_Converter, &current_directory, &startup_info)) {
        goto exit;
    }
    return_value = _winapi_CreateProcess_impl(module, application_name, command_line, proc_attrs, thread_attrs, inherit_handles, creation_flags, env_mapping, current_directory, startup_info);

exit:
    /* Cleanup for application_name */
    #if !USE_UNICODE_WCHAR_CACHE
    PyMem_Free((void *)application_name);
    #endif /* USE_UNICODE_WCHAR_CACHE */
    /* Cleanup for current_directory */
    #if !USE_UNICODE_WCHAR_CACHE
    PyMem_Free((void *)current_directory);
    #endif /* USE_UNICODE_WCHAR_CACHE */

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
    {"DuplicateHandle", (PyCFunction)(void(*)(void))_winapi_DuplicateHandle, METH_FASTCALL, _winapi_DuplicateHandle__doc__},

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
    return_value = Py_BuildValue("k", _return_value);

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
    return_value = Py_BuildValue("k", _return_value);

exit:
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
    {"MapViewOfFile", (PyCFunction)(void(*)(void))_winapi_MapViewOfFile, METH_FASTCALL, _winapi_MapViewOfFile__doc__},

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

PyDoc_STRVAR(_winapi_OpenFileMapping__doc__,
"OpenFileMapping($module, desired_access, inherit_handle, name, /)\n"
"--\n"
"\n");

#define _WINAPI_OPENFILEMAPPING_METHODDEF    \
    {"OpenFileMapping", (PyCFunction)(void(*)(void))_winapi_OpenFileMapping, METH_FASTCALL, _winapi_OpenFileMapping__doc__},

static HANDLE
_winapi_OpenFileMapping_impl(PyObject *module, DWORD desired_access,
                             BOOL inherit_handle, LPCWSTR name);

static PyObject *
_winapi_OpenFileMapping(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    DWORD desired_access;
    BOOL inherit_handle;
    LPCWSTR name;
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
    #if !USE_UNICODE_WCHAR_CACHE
    PyMem_Free((void *)name);
    #endif /* USE_UNICODE_WCHAR_CACHE */

    return return_value;
}

PyDoc_STRVAR(_winapi_OpenProcess__doc__,
"OpenProcess($module, desired_access, inherit_handle, process_id, /)\n"
"--\n"
"\n");

#define _WINAPI_OPENPROCESS_METHODDEF    \
    {"OpenProcess", (PyCFunction)(void(*)(void))_winapi_OpenProcess, METH_FASTCALL, _winapi_OpenProcess__doc__},

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
    {"PeekNamedPipe", (PyCFunction)(void(*)(void))_winapi_PeekNamedPipe, METH_FASTCALL, _winapi_PeekNamedPipe__doc__},

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
    {"LCMapStringEx", (PyCFunction)(void(*)(void))_winapi_LCMapStringEx, METH_FASTCALL|METH_KEYWORDS, _winapi_LCMapStringEx__doc__},

static PyObject *
_winapi_LCMapStringEx_impl(PyObject *module, PyObject *locale, DWORD flags,
                           PyObject *src);

static PyObject *
_winapi_LCMapStringEx(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"locale", "flags", "src", NULL};
    static _PyArg_Parser _parser = {"UkU:LCMapStringEx", _keywords, 0};
    PyObject *locale;
    DWORD flags;
    PyObject *src;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &locale, &flags, &src)) {
        goto exit;
    }
    return_value = _winapi_LCMapStringEx_impl(module, locale, flags, src);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_ReadFile__doc__,
"ReadFile($module, /, handle, size, overlapped=False)\n"
"--\n"
"\n");

#define _WINAPI_READFILE_METHODDEF    \
    {"ReadFile", (PyCFunction)(void(*)(void))_winapi_ReadFile, METH_FASTCALL|METH_KEYWORDS, _winapi_ReadFile__doc__},

static PyObject *
_winapi_ReadFile_impl(PyObject *module, HANDLE handle, DWORD size,
                      int use_overlapped);

static PyObject *
_winapi_ReadFile(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"handle", "size", "overlapped", NULL};
    static _PyArg_Parser _parser = {"" F_HANDLE "k|i:ReadFile", _keywords, 0};
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

PyDoc_STRVAR(_winapi_SetNamedPipeHandleState__doc__,
"SetNamedPipeHandleState($module, named_pipe, mode,\n"
"                        max_collection_count, collect_data_timeout, /)\n"
"--\n"
"\n");

#define _WINAPI_SETNAMEDPIPEHANDLESTATE_METHODDEF    \
    {"SetNamedPipeHandleState", (PyCFunction)(void(*)(void))_winapi_SetNamedPipeHandleState, METH_FASTCALL, _winapi_SetNamedPipeHandleState__doc__},

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
    {"TerminateProcess", (PyCFunction)(void(*)(void))_winapi_TerminateProcess, METH_FASTCALL, _winapi_TerminateProcess__doc__},

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
    {"WaitNamedPipe", (PyCFunction)(void(*)(void))_winapi_WaitNamedPipe, METH_FASTCALL, _winapi_WaitNamedPipe__doc__},

static PyObject *
_winapi_WaitNamedPipe_impl(PyObject *module, LPCTSTR name, DWORD timeout);

static PyObject *
_winapi_WaitNamedPipe(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    LPCTSTR name;
    DWORD timeout;

    if (!_PyArg_ParseStack(args, nargs, "sk:WaitNamedPipe",
        &name, &timeout)) {
        goto exit;
    }
    return_value = _winapi_WaitNamedPipe_impl(module, name, timeout);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_WaitForMultipleObjects__doc__,
"WaitForMultipleObjects($module, handle_seq, wait_flag,\n"
"                       milliseconds=_winapi.INFINITE, /)\n"
"--\n"
"\n");

#define _WINAPI_WAITFORMULTIPLEOBJECTS_METHODDEF    \
    {"WaitForMultipleObjects", (PyCFunction)(void(*)(void))_winapi_WaitForMultipleObjects, METH_FASTCALL, _winapi_WaitForMultipleObjects__doc__},

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
    {"WaitForSingleObject", (PyCFunction)(void(*)(void))_winapi_WaitForSingleObject, METH_FASTCALL, _winapi_WaitForSingleObject__doc__},

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
    {"WriteFile", (PyCFunction)(void(*)(void))_winapi_WriteFile, METH_FASTCALL|METH_KEYWORDS, _winapi_WriteFile__doc__},

static PyObject *
_winapi_WriteFile_impl(PyObject *module, HANDLE handle, PyObject *buffer,
                       int use_overlapped);

static PyObject *
_winapi_WriteFile(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"handle", "buffer", "overlapped", NULL};
    static _PyArg_Parser _parser = {"" F_HANDLE "O|i:WriteFile", _keywords, 0};
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

PyDoc_STRVAR(_winapi_GetFileType__doc__,
"GetFileType($module, /, handle)\n"
"--\n"
"\n");

#define _WINAPI_GETFILETYPE_METHODDEF    \
    {"GetFileType", (PyCFunction)(void(*)(void))_winapi_GetFileType, METH_FASTCALL|METH_KEYWORDS, _winapi_GetFileType__doc__},

static DWORD
_winapi_GetFileType_impl(PyObject *module, HANDLE handle);

static PyObject *
_winapi_GetFileType(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"handle", NULL};
    static _PyArg_Parser _parser = {"" F_HANDLE ":GetFileType", _keywords, 0};
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
    return_value = Py_BuildValue("k", _return_value);

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
    {"_mimetypes_read_windows_registry", (PyCFunction)(void(*)(void))_winapi__mimetypes_read_windows_registry, METH_FASTCALL|METH_KEYWORDS, _winapi__mimetypes_read_windows_registry__doc__},

static PyObject *
_winapi__mimetypes_read_windows_registry_impl(PyObject *module,
                                              PyObject *on_type_read);

static PyObject *
_winapi__mimetypes_read_windows_registry(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"on_type_read", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "_mimetypes_read_windows_registry", 0};
    PyObject *argsbuf[1];
    PyObject *on_type_read;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    on_type_read = args[0];
    return_value = _winapi__mimetypes_read_windows_registry_impl(module, on_type_read);

exit:
    return return_value;
}
/*[clinic end generated code: output=8e13179bf25bdea5 input=a9049054013a1b77]*/
