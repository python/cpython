/*********************************************************

	msvcrtmodule.c

	A Python interface to the Microsoft Visual C Runtime
	Library, providing access to those non-portable, but
	still useful routines.

	Only ever compiled with an MS compiler, so no attempt
	has been made to avoid MS language extensions, etc...

***********************************************************/
#include "Python.h"
#include "malloc.h"
// Perform locking operations on a file.
static PyObject *msvcrt_locking(PyObject *self, PyObject *args)
{
	int mode;
	long nBytes;
	PyObject *obFile;
	FILE *pFile;
	if (!PyArg_ParseTuple(args,"O!il:locking", &obFile, PyFile_Type, &mode, &nBytes))
		return NULL;
	if (NULL==(pFile = PyFile_AsFile(obFile)))
		return NULL;
	if (0 != _locking(_fileno(pFile), mode, nBytes))
		return PyErr_SetFromErrno(PyExc_IOError);
	Py_INCREF(Py_None);
	return Py_None;
}

// Forces the malloc heap to clean itself up, and free unused blocks
// back to the OS.
static PyObject *msvcrt_heapmin(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args,":heapmin"))
		return NULL;
	if (_heapmin()!=0)
		return PyErr_SetFromErrno(PyExc_MemoryError); // Is this the correct error???
	Py_INCREF(Py_None);
	return Py_None;
}

/*******
Left this out for now...

// Convert an OS file handle to a Python file object (yay!).
// This may only work on NT
static PyObject *msvcrt_open_osfhandle(PyObject *self, PyObject *args)
{
	// Note that we get the underlying handle using the long
	// "abstract" interface.  This will allow either a native integer
	// or else a Win32 extension PyHANDLE object, which implements an 
	// int() converter.
	PyObject *obHandle;
	PyObject *obInt;
	int flags;
	long handle;
	if (!PyArg_ParseTuple(args,"Oi:open_osfhandle", &obHandle, &flags))
		return NULL;

	if (NULL==(obInt = PyNumber_Int(obHandle))) {
		PyErr_Clear();
		PyErr_SetString(PyExc_TypeError, "The handle param must be an integer, =
or an object able to be converted to an integer");
		return NULL;
	}
	handle = PyInt_AsLong(obInt);
	Py_DECREF(obInt);
	rtHandle = _open_osfhandle(handle, flags);
	if (rtHandle==-1)
		return PyErr_SetFromErrno(PyExc_IOError);

  what mode?  Should I just return here, and expose _fdopen 
  and setvbuf?

	f1=_fdopen(fd1, "w");
	setvbuf(f1, NULL, _IONBF, 0);
	f=PyFile_FromFile(f1, cmdstring, "w", fclose);

}
*****/

/* List of functions exported by this module */
static struct PyMethodDef msvcrt_functions[] = {
	{"locking",             msvcrt_locking, 1},
	{"heapmin",				msvcrt_heapmin, 1},
	{NULL,			NULL}
};

__declspec(dllexport) void
initmsvcrt(void)
{
	Py_InitModule("msvcrt", msvcrt_functions);
}
