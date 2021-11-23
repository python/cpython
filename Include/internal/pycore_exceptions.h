#ifndef Py_INTERNAL_EXCEPTIONS_H
#define Py_INTERNAL_EXCEPTIONS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


/**************************************
 * lifecycle
 **************************************/

extern PyStatus _PyExc_InitCoreObjects(PyInterpreterState *interp);
extern PyStatus _PyExc_InitCoreTypes(PyInterpreterState *interp);
extern void _PyExc_FiniCoreObjects(PyInterpreterState *interp);

extern PyStatus _PyExc_InitTypes(PyInterpreterState *interp);
extern PyStatus _PyExc_InitTypeStateLast(PyInterpreterState *interp);
extern void _PyExc_FiniObjects(PyInterpreterState *interp);


/**************************************
 * exception and warning types
 **************************************/

// base excptions (in alphabetical order):
extern PyTypeObject _PyExc_BaseException;
extern PyTypeObject _PyExc_BaseExceptionGroup;
extern PyTypeObject _PyExc_Exception;
extern PyTypeObject _PyExc_GeneratorExit;
extern PyTypeObject _PyExc_KeyboardInterrupt;
extern PyTypeObject _PyExc_SystemExit;

// Subclasses of Exception (in alphabetical order):
extern PyTypeObject _PyExc_ArithmeticError;
extern PyTypeObject _PyExc_AssertionError;
extern PyTypeObject _PyExc_AttributeError;
extern PyTypeObject _PyExc_BufferError;
extern PyTypeObject _PyExc_EOFError;
extern PyTypeObject _PyExc_FloatingPointError;
extern PyTypeObject _PyExc_ImportError;
extern PyTypeObject _PyExc_IndentationError;
extern PyTypeObject _PyExc_IndexError;
extern PyTypeObject _PyExc_KeyError;
extern PyTypeObject _PyExc_LookupError;
extern PyTypeObject _PyExc_MemoryError;
extern PyTypeObject _PyExc_ModuleNotFoundError;
extern PyTypeObject _PyExc_NameError;
extern PyTypeObject _PyExc_NotImplementedError;
extern PyTypeObject _PyExc_OSError;
extern PyTypeObject _PyExc_OverflowError;
extern PyTypeObject _PyExc_RecursionError;
extern PyTypeObject _PyExc_ReferenceError;
extern PyTypeObject _PyExc_RuntimeError;
extern PyTypeObject _PyExc_StopAsyncIteration;
extern PyTypeObject _PyExc_StopIteration;
extern PyTypeObject _PyExc_SyntaxError;
extern PyTypeObject _PyExc_SystemError;
extern PyTypeObject _PyExc_TabError;
extern PyTypeObject _PyExc_TypeError;
extern PyTypeObject _PyExc_UnboundLocalError;
extern PyTypeObject _PyExc_UnicodeDecodeError;
extern PyTypeObject _PyExc_UnicodeEncodeError;
extern PyTypeObject _PyExc_UnicodeError;
extern PyTypeObject _PyExc_UnicodeTranslateError;
extern PyTypeObject _PyExc_ValueError;
extern PyTypeObject _PyExc_ZeroDivisionError;

// subclasses of OSError (in alphabetical order):
extern PyTypeObject _PyExc_BlockingIOError;
extern PyTypeObject _PyExc_BrokenPipeError;
extern PyTypeObject _PyExc_ChildProcessError;
extern PyTypeObject _PyExc_ConnectionAbortedError;
extern PyTypeObject _PyExc_ConnectionError;
extern PyTypeObject _PyExc_ConnectionRefusedError;
extern PyTypeObject _PyExc_ConnectionResetError;
extern PyTypeObject _PyExc_FileExistsError;
extern PyTypeObject _PyExc_FileNotFoundError;
extern PyTypeObject _PyExc_InterruptedError;
extern PyTypeObject _PyExc_IsADirectoryError;
extern PyTypeObject _PyExc_NotADirectoryError;
extern PyTypeObject _PyExc_PermissionError;
extern PyTypeObject _PyExc_ProcessLookupError;
extern PyTypeObject _PyExc_TimeoutError;

// warning categories (in alphabetical order):
extern PyTypeObject _PyExc_Warning;  // subclass of Exception
extern PyTypeObject _PyExc_BytesWarning;
extern PyTypeObject _PyExc_DeprecationWarning;
extern PyTypeObject _PyExc_EncodingWarning;
extern PyTypeObject _PyExc_FutureWarning;
extern PyTypeObject _PyExc_ImportWarning;
extern PyTypeObject _PyExc_PendingDeprecationWarning;
extern PyTypeObject _PyExc_ResourceWarning;
extern PyTypeObject _PyExc_RuntimeWarning;
extern PyTypeObject _PyExc_SyntaxWarning;
extern PyTypeObject _PyExc_UnicodeWarning;
extern PyTypeObject _PyExc_UserWarning;


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_EXCEPTIONS_H */
