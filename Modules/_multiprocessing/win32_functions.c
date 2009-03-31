/*
 * Win32 functions used by multiprocessing package
 *
 * win32_functions.c
 *
 * Copyright (c) 2006-2008, R Oudkerk --- see COPYING.txt
 */

#include "multiprocessing.h"


#define WIN32_FUNCTION(func) \
    {#func, (PyCFunction)win32_ ## func, METH_VARARGS | METH_STATIC, ""}

#define WIN32_CONSTANT(fmt, con) \
    PyDict_SetItemString(Win32Type.tp_dict, #con, Py_BuildValue(fmt, con))


static PyObject *
win32_CloseHandle(PyObject *self, PyObject *args)
{
	HANDLE hObject;
	BOOL success;

	if (!PyArg_ParseTuple(args, F_HANDLE, &hObject))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	success = CloseHandle(hObject); 
	Py_END_ALLOW_THREADS

	if (!success)
		return PyErr_SetFromWindowsErr(0);

	Py_RETURN_NONE;
}

static PyObject *
win32_ConnectNamedPipe(PyObject *self, PyObject *args)
{
	HANDLE hNamedPipe;
	LPOVERLAPPED lpOverlapped;
	BOOL success;

	if (!PyArg_ParseTuple(args, F_HANDLE F_POINTER, 
			      &hNamedPipe, &lpOverlapped))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	success = ConnectNamedPipe(hNamedPipe, lpOverlapped);
	Py_END_ALLOW_THREADS

	if (!success)
		return PyErr_SetFromWindowsErr(0);

	Py_RETURN_NONE;
}

static PyObject *
win32_CreateFile(PyObject *self, PyObject *args)
{
	LPCTSTR lpFileName;
	DWORD dwDesiredAccess;
	DWORD dwShareMode;
	LPSECURITY_ATTRIBUTES lpSecurityAttributes;
	DWORD dwCreationDisposition;
	DWORD dwFlagsAndAttributes;
	HANDLE hTemplateFile;
	HANDLE handle;

	if (!PyArg_ParseTuple(args, "s" F_DWORD F_DWORD F_POINTER 
			      F_DWORD F_DWORD F_HANDLE,
			      &lpFileName, &dwDesiredAccess, &dwShareMode, 
			      &lpSecurityAttributes, &dwCreationDisposition, 
			      &dwFlagsAndAttributes, &hTemplateFile))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	handle = CreateFile(lpFileName, dwDesiredAccess, 
			    dwShareMode, lpSecurityAttributes, 
			    dwCreationDisposition, 
			    dwFlagsAndAttributes, hTemplateFile);
	Py_END_ALLOW_THREADS

	if (handle == INVALID_HANDLE_VALUE)
		return PyErr_SetFromWindowsErr(0);

	return Py_BuildValue(F_HANDLE, handle);
}

static PyObject *
win32_CreateNamedPipe(PyObject *self, PyObject *args)
{
	LPCTSTR lpName;
	DWORD dwOpenMode;
	DWORD dwPipeMode;
	DWORD nMaxInstances;
	DWORD nOutBufferSize;
	DWORD nInBufferSize;
	DWORD nDefaultTimeOut;
	LPSECURITY_ATTRIBUTES lpSecurityAttributes;
	HANDLE handle;

	if (!PyArg_ParseTuple(args, "s" F_DWORD F_DWORD F_DWORD 
			      F_DWORD F_DWORD F_DWORD F_POINTER,
			      &lpName, &dwOpenMode, &dwPipeMode, 
			      &nMaxInstances, &nOutBufferSize, 
			      &nInBufferSize, &nDefaultTimeOut,
			      &lpSecurityAttributes))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	handle = CreateNamedPipe(lpName, dwOpenMode, dwPipeMode, 
				 nMaxInstances, nOutBufferSize, 
				 nInBufferSize, nDefaultTimeOut,
				 lpSecurityAttributes);
	Py_END_ALLOW_THREADS

	if (handle == INVALID_HANDLE_VALUE)
		return PyErr_SetFromWindowsErr(0);

	return Py_BuildValue(F_HANDLE, handle);
}

static PyObject *
win32_ExitProcess(PyObject *self, PyObject *args)
{
	UINT uExitCode;

	if (!PyArg_ParseTuple(args, "I", &uExitCode))
		return NULL;

	#if defined(Py_DEBUG)
		SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOALIGNMENTFAULTEXCEPT|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);
		_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
	#endif


	ExitProcess(uExitCode);

	return NULL;
}

static PyObject *
win32_GetLastError(PyObject *self, PyObject *args)
{
	return Py_BuildValue(F_DWORD, GetLastError());
}

static PyObject *
win32_OpenProcess(PyObject *self, PyObject *args)
{
	DWORD dwDesiredAccess;
	BOOL bInheritHandle;
	DWORD dwProcessId;
	HANDLE handle;

	if (!PyArg_ParseTuple(args, F_DWORD "i" F_DWORD, 
			      &dwDesiredAccess, &bInheritHandle, &dwProcessId))
		return NULL;

	handle = OpenProcess(dwDesiredAccess, bInheritHandle, dwProcessId);    
	if (handle == NULL)
		return PyErr_SetFromWindowsErr(0);

	return Py_BuildValue(F_HANDLE, handle);
}

static PyObject *
win32_SetNamedPipeHandleState(PyObject *self, PyObject *args)
{
	HANDLE hNamedPipe;
	PyObject *oArgs[3];
	DWORD dwArgs[3], *pArgs[3] = {NULL, NULL, NULL};
	int i;

	if (!PyArg_ParseTuple(args, F_HANDLE "OOO", 
			      &hNamedPipe, &oArgs[0], &oArgs[1], &oArgs[2]))
		return NULL;

	PyErr_Clear();

	for (i = 0 ; i < 3 ; i++) {
		if (oArgs[i] != Py_None) {
			dwArgs[i] = PyInt_AsUnsignedLongMask(oArgs[i]);
			if (PyErr_Occurred())
				return NULL;
			pArgs[i] = &dwArgs[i];
		}
	}

	if (!SetNamedPipeHandleState(hNamedPipe, pArgs[0], pArgs[1], pArgs[2]))
		return PyErr_SetFromWindowsErr(0);

	Py_RETURN_NONE;
}

static PyObject *
win32_WaitNamedPipe(PyObject *self, PyObject *args)
{
	LPCTSTR lpNamedPipeName;
	DWORD nTimeOut;
	BOOL success;

	if (!PyArg_ParseTuple(args, "s" F_DWORD, &lpNamedPipeName, &nTimeOut))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	success = WaitNamedPipe(lpNamedPipeName, nTimeOut);
	Py_END_ALLOW_THREADS

	if (!success)
		return PyErr_SetFromWindowsErr(0);

	Py_RETURN_NONE;
}

static PyMethodDef win32_methods[] = {
	WIN32_FUNCTION(CloseHandle),
	WIN32_FUNCTION(GetLastError),
	WIN32_FUNCTION(OpenProcess),
	WIN32_FUNCTION(ExitProcess),
	WIN32_FUNCTION(ConnectNamedPipe),
	WIN32_FUNCTION(CreateFile),
	WIN32_FUNCTION(CreateNamedPipe),
	WIN32_FUNCTION(SetNamedPipeHandleState),
	WIN32_FUNCTION(WaitNamedPipe),
	{NULL}
};


PyTypeObject Win32Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
};


PyObject *
create_win32_namespace(void)
{
	Win32Type.tp_name = "_multiprocessing.win32";
	Win32Type.tp_methods = win32_methods;
	if (PyType_Ready(&Win32Type) < 0)
		return NULL;
	Py_INCREF(&Win32Type);

	WIN32_CONSTANT(F_DWORD, ERROR_ALREADY_EXISTS);
	WIN32_CONSTANT(F_DWORD, ERROR_PIPE_BUSY);
	WIN32_CONSTANT(F_DWORD, ERROR_PIPE_CONNECTED);
	WIN32_CONSTANT(F_DWORD, ERROR_SEM_TIMEOUT);
	WIN32_CONSTANT(F_DWORD, GENERIC_READ);
	WIN32_CONSTANT(F_DWORD, GENERIC_WRITE);
	WIN32_CONSTANT(F_DWORD, INFINITE);
	WIN32_CONSTANT(F_DWORD, NMPWAIT_WAIT_FOREVER);
	WIN32_CONSTANT(F_DWORD, OPEN_EXISTING);
	WIN32_CONSTANT(F_DWORD, PIPE_ACCESS_DUPLEX);
	WIN32_CONSTANT(F_DWORD, PIPE_ACCESS_INBOUND);
	WIN32_CONSTANT(F_DWORD, PIPE_READMODE_MESSAGE);
	WIN32_CONSTANT(F_DWORD, PIPE_TYPE_MESSAGE);
	WIN32_CONSTANT(F_DWORD, PIPE_UNLIMITED_INSTANCES);
	WIN32_CONSTANT(F_DWORD, PIPE_WAIT);
	WIN32_CONSTANT(F_DWORD, PROCESS_ALL_ACCESS);

	WIN32_CONSTANT("i", NULL);

	return (PyObject*)&Win32Type;
}
