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

    if (!PyArg_Parse(arg, "p:GetOverlappedResult", &wait)) {
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
    {"ConnectNamedPipe", (PyCFunction)_winapi_ConnectNamedPipe, METH_FASTCALL, _winapi_ConnectNamedPipe__doc__},

static PyObject *
_winapi_ConnectNamedPipe_impl(PyObject *module, HANDLE handle,
                              int use_overlapped);

static PyObject *
_winapi_ConnectNamedPipe(PyObject *module, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"handle", "overlapped", NULL};
    static _PyArg_Parser _parser = {"" F_HANDLE "|i:ConnectNamedPipe", _keywords, 0};
    HANDLE handle;
    int use_overlapped = 0;

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
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
    {"CreateFile", (PyCFunction)_winapi_CreateFile, METH_VARARGS, _winapi_CreateFile__doc__},

static HANDLE
_winapi_CreateFile_impl(PyObject *module, LPCTSTR file_name,
                        DWORD desired_access, DWORD share_mode,
                        LPSECURITY_ATTRIBUTES security_attributes,
                        DWORD creation_disposition,
                        DWORD flags_and_attributes, HANDLE template_file);

static PyObject *
_winapi_CreateFile(PyObject *module, PyObject *args)
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

    if (!PyArg_ParseTuple(args, "skk" F_POINTER "kk" F_HANDLE ":CreateFile",
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

PyDoc_STRVAR(_winapi_CreateJunction__doc__,
"CreateJunction($module, src_path, dst_path, /)\n"
"--\n"
"\n");

#define _WINAPI_CREATEJUNCTION_METHODDEF    \
    {"CreateJunction", (PyCFunction)_winapi_CreateJunction, METH_VARARGS, _winapi_CreateJunction__doc__},

static PyObject *
_winapi_CreateJunction_impl(PyObject *module, LPWSTR src_path,
                            LPWSTR dst_path);

static PyObject *
_winapi_CreateJunction(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    LPWSTR src_path;
    LPWSTR dst_path;

    if (!PyArg_ParseTuple(args, "uu:CreateJunction",
        &src_path, &dst_path)) {
        goto exit;
    }
    return_value = _winapi_CreateJunction_impl(module, src_path, dst_path);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_CreateNamedPipe__doc__,
"CreateNamedPipe($module, name, open_mode, pipe_mode, max_instances,\n"
"                out_buffer_size, in_buffer_size, default_timeout,\n"
"                security_attributes, /)\n"
"--\n"
"\n");

#define _WINAPI_CREATENAMEDPIPE_METHODDEF    \
    {"CreateNamedPipe", (PyCFunction)_winapi_CreateNamedPipe, METH_VARARGS, _winapi_CreateNamedPipe__doc__},

static HANDLE
_winapi_CreateNamedPipe_impl(PyObject *module, LPCTSTR name, DWORD open_mode,
                             DWORD pipe_mode, DWORD max_instances,
                             DWORD out_buffer_size, DWORD in_buffer_size,
                             DWORD default_timeout,
                             LPSECURITY_ATTRIBUTES security_attributes);

static PyObject *
_winapi_CreateNamedPipe(PyObject *module, PyObject *args)
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

    if (!PyArg_ParseTuple(args, "skkkkkk" F_POINTER ":CreateNamedPipe",
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
    {"CreatePipe", (PyCFunction)_winapi_CreatePipe, METH_VARARGS, _winapi_CreatePipe__doc__},

static PyObject *
_winapi_CreatePipe_impl(PyObject *module, PyObject *pipe_attrs, DWORD size);

static PyObject *
_winapi_CreatePipe(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *pipe_attrs;
    DWORD size;

    if (!PyArg_ParseTuple(args, "Ok:CreatePipe",
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
"  proc_attrs\n"
"    Ignored internally, can be None.\n"
"  thread_attrs\n"
"    Ignored internally, can be None.\n"
"\n"
"The return value is a tuple of the process handle, thread handle,\n"
"process ID, and thread ID.");

#define _WINAPI_CREATEPROCESS_METHODDEF    \
    {"CreateProcess", (PyCFunction)_winapi_CreateProcess, METH_VARARGS, _winapi_CreateProcess__doc__},

static PyObject *
_winapi_CreateProcess_impl(PyObject *module, Py_UNICODE *application_name,
                           Py_UNICODE *command_line, PyObject *proc_attrs,
                           PyObject *thread_attrs, BOOL inherit_handles,
                           DWORD creation_flags, PyObject *env_mapping,
                           Py_UNICODE *current_directory,
                           PyObject *startup_info);

static PyObject *
_winapi_CreateProcess(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_UNICODE *application_name;
    Py_UNICODE *command_line;
    PyObject *proc_attrs;
    PyObject *thread_attrs;
    BOOL inherit_handles;
    DWORD creation_flags;
    PyObject *env_mapping;
    Py_UNICODE *current_directory;
    PyObject *startup_info;

    if (!PyArg_ParseTuple(args, "ZZOOikOZO:CreateProcess",
        &application_name, &command_line, &proc_attrs, &thread_attrs, &inherit_handles, &creation_flags, &env_mapping, &current_directory, &startup_info)) {
        goto exit;
    }
    return_value = _winapi_CreateProcess_impl(module, application_name, command_line, proc_attrs, thread_attrs, inherit_handles, creation_flags, env_mapping, current_directory, startup_info);

exit:
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
    {"DuplicateHandle", (PyCFunction)_winapi_DuplicateHandle, METH_VARARGS, _winapi_DuplicateHandle__doc__},

static HANDLE
_winapi_DuplicateHandle_impl(PyObject *module, HANDLE source_process_handle,
                             HANDLE source_handle,
                             HANDLE target_process_handle,
                             DWORD desired_access, BOOL inherit_handle,
                             DWORD options);

static PyObject *
_winapi_DuplicateHandle(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    HANDLE source_process_handle;
    HANDLE source_handle;
    HANDLE target_process_handle;
    DWORD desired_access;
    BOOL inherit_handle;
    DWORD options = 0;
    HANDLE _return_value;

    if (!PyArg_ParseTuple(args, "" F_HANDLE "" F_HANDLE "" F_HANDLE "ki|k:DuplicateHandle",
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
    if ((_return_value == DWORD_MAX) && PyErr_Occurred()) {
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
    if ((_return_value == DWORD_MAX) && PyErr_Occurred()) {
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

PyDoc_STRVAR(_winapi_OpenProcess__doc__,
"OpenProcess($module, desired_access, inherit_handle, process_id, /)\n"
"--\n"
"\n");

#define _WINAPI_OPENPROCESS_METHODDEF    \
    {"OpenProcess", (PyCFunction)_winapi_OpenProcess, METH_VARARGS, _winapi_OpenProcess__doc__},

static HANDLE
_winapi_OpenProcess_impl(PyObject *module, DWORD desired_access,
                         BOOL inherit_handle, DWORD process_id);

static PyObject *
_winapi_OpenProcess(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    DWORD desired_access;
    BOOL inherit_handle;
    DWORD process_id;
    HANDLE _return_value;

    if (!PyArg_ParseTuple(args, "kik:OpenProcess",
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
    {"PeekNamedPipe", (PyCFunction)_winapi_PeekNamedPipe, METH_VARARGS, _winapi_PeekNamedPipe__doc__},

static PyObject *
_winapi_PeekNamedPipe_impl(PyObject *module, HANDLE handle, int size);

static PyObject *
_winapi_PeekNamedPipe(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    HANDLE handle;
    int size = 0;

    if (!PyArg_ParseTuple(args, "" F_HANDLE "|i:PeekNamedPipe",
        &handle, &size)) {
        goto exit;
    }
    return_value = _winapi_PeekNamedPipe_impl(module, handle, size);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_ReadFile__doc__,
"ReadFile($module, /, handle, size, overlapped=False)\n"
"--\n"
"\n");

#define _WINAPI_READFILE_METHODDEF    \
    {"ReadFile", (PyCFunction)_winapi_ReadFile, METH_FASTCALL, _winapi_ReadFile__doc__},

static PyObject *
_winapi_ReadFile_impl(PyObject *module, HANDLE handle, int size,
                      int use_overlapped);

static PyObject *
_winapi_ReadFile(PyObject *module, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"handle", "size", "overlapped", NULL};
    static _PyArg_Parser _parser = {"" F_HANDLE "i|i:ReadFile", _keywords, 0};
    HANDLE handle;
    int size;
    int use_overlapped = 0;

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
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
    {"SetNamedPipeHandleState", (PyCFunction)_winapi_SetNamedPipeHandleState, METH_VARARGS, _winapi_SetNamedPipeHandleState__doc__},

static PyObject *
_winapi_SetNamedPipeHandleState_impl(PyObject *module, HANDLE named_pipe,
                                     PyObject *mode,
                                     PyObject *max_collection_count,
                                     PyObject *collect_data_timeout);

static PyObject *
_winapi_SetNamedPipeHandleState(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    HANDLE named_pipe;
    PyObject *mode;
    PyObject *max_collection_count;
    PyObject *collect_data_timeout;

    if (!PyArg_ParseTuple(args, "" F_HANDLE "OOO:SetNamedPipeHandleState",
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
    {"TerminateProcess", (PyCFunction)_winapi_TerminateProcess, METH_VARARGS, _winapi_TerminateProcess__doc__},

static PyObject *
_winapi_TerminateProcess_impl(PyObject *module, HANDLE handle,
                              UINT exit_code);

static PyObject *
_winapi_TerminateProcess(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    HANDLE handle;
    UINT exit_code;

    if (!PyArg_ParseTuple(args, "" F_HANDLE "I:TerminateProcess",
        &handle, &exit_code)) {
        goto exit;
    }
    return_value = _winapi_TerminateProcess_impl(module, handle, exit_code);

exit:
    return return_value;
}

PyDoc_STRVAR(_winapi_WaitNamedPipe__doc__,
"WaitNamedPipe($module, name, timeout, /)\n"
"--\n"
"\n");

#define _WINAPI_WAITNAMEDPIPE_METHODDEF    \
    {"WaitNamedPipe", (PyCFunction)_winapi_WaitNamedPipe, METH_VARARGS, _winapi_WaitNamedPipe__doc__},

static PyObject *
_winapi_WaitNamedPipe_impl(PyObject *module, LPCTSTR name, DWORD timeout);

static PyObject *
_winapi_WaitNamedPipe(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    LPCTSTR name;
    DWORD timeout;

    if (!PyArg_ParseTuple(args, "sk:WaitNamedPipe",
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
    {"WaitForMultipleObjects", (PyCFunction)_winapi_WaitForMultipleObjects, METH_VARARGS, _winapi_WaitForMultipleObjects__doc__},

static PyObject *
_winapi_WaitForMultipleObjects_impl(PyObject *module, PyObject *handle_seq,
                                    BOOL wait_flag, DWORD milliseconds);

static PyObject *
_winapi_WaitForMultipleObjects(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *handle_seq;
    BOOL wait_flag;
    DWORD milliseconds = INFINITE;

    if (!PyArg_ParseTuple(args, "Oi|k:WaitForMultipleObjects",
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
    {"WaitForSingleObject", (PyCFunction)_winapi_WaitForSingleObject, METH_VARARGS, _winapi_WaitForSingleObject__doc__},

static long
_winapi_WaitForSingleObject_impl(PyObject *module, HANDLE handle,
                                 DWORD milliseconds);

static PyObject *
_winapi_WaitForSingleObject(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    HANDLE handle;
    DWORD milliseconds;
    long _return_value;

    if (!PyArg_ParseTuple(args, "" F_HANDLE "k:WaitForSingleObject",
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
    {"WriteFile", (PyCFunction)_winapi_WriteFile, METH_FASTCALL, _winapi_WriteFile__doc__},

static PyObject *
_winapi_WriteFile_impl(PyObject *module, HANDLE handle, PyObject *buffer,
                       int use_overlapped);

static PyObject *
_winapi_WriteFile(PyObject *module, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"handle", "buffer", "overlapped", NULL};
    static _PyArg_Parser _parser = {"" F_HANDLE "O|i:WriteFile", _keywords, 0};
    HANDLE handle;
    PyObject *buffer;
    int use_overlapped = 0;

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
        &handle, &buffer, &use_overlapped)) {
        goto exit;
    }
    return_value = _winapi_WriteFile_impl(module, handle, buffer, use_overlapped);

exit:
    return return_value;
}
/*[clinic end generated code: output=46d6382a6662c4a9 input=a9049054013a1b77]*/
