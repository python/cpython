#ifndef Py_CPYTHON_FILEOBJECT_H
#  error "this header file must not be included directly"
#endif

PyAPI_FUNC(char *) Py_UniversalNewlineFgets(char *, int, FILE*, PyObject *);

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03060000
PyAPI_DATA(const char *) Py_FileSystemDefaultEncodeErrors;
#endif

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03070000
PyAPI_DATA(int) Py_UTF8Mode;
#endif

/* The std printer acts as a preliminary sys.stderr until the new io
   infrastructure is in place. */
PyAPI_FUNC(PyObject *) PyFile_NewStdPrinter(int);
PyAPI_DATA(PyTypeObject) PyStdPrinter_Type;

typedef PyObject * (*Py_OpenCodeHookFunction)(PyObject *, void *);

PyAPI_FUNC(PyObject *) PyFile_OpenCode(const char *utf8path);
PyAPI_FUNC(PyObject *) PyFile_OpenCodeObject(PyObject *path);
PyAPI_FUNC(int) PyFile_SetOpenCodeHook(Py_OpenCodeHookFunction hook, void *userData);

PyAPI_FUNC(int) _PyLong_FileDescriptor_Converter(PyObject *, void *);
