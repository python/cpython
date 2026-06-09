#ifndef Py_INTERNAL_EXCEPTIONS_H
#define Py_INTERNAL_EXCEPTIONS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


/* X-macros for static exception types registered by _PyExc_InitTypes
   via static_exceptions[] in Objects/exceptions.c.  Order reflects the
   inheritance levels described in that file.  Used by both
   Objects/exceptions.c (to build the array) and
   pycore_static_builtin_types.h (for the compile-time count). */
#define _Py_FOREACH_STATIC_EXCEPTION_TYPE(EXC) \
    EXC(BaseException) \
    \
    EXC(BaseExceptionGroup) \
    EXC(Exception) \
    EXC(GeneratorExit) \
    EXC(KeyboardInterrupt) \
    EXC(SystemExit) \
    \
    EXC(ArithmeticError) \
    EXC(AssertionError) \
    EXC(AttributeError) \
    EXC(BufferError) \
    EXC(EOFError) \
    EXC(ImportError) \
    EXC(LookupError) \
    EXC(MemoryError) \
    EXC(NameError) \
    EXC(OSError) \
    EXC(ReferenceError) \
    EXC(RuntimeError) \
    EXC(StopAsyncIteration) \
    EXC(StopIteration) \
    EXC(SyntaxError) \
    EXC(SystemError) \
    EXC(TypeError) \
    EXC(ValueError) \
    EXC(Warning) \
    \
    EXC(FloatingPointError) \
    EXC(OverflowError) \
    EXC(ZeroDivisionError) \
    \
    EXC(BytesWarning) \
    EXC(DeprecationWarning) \
    EXC(EncodingWarning) \
    EXC(FutureWarning) \
    EXC(ImportWarning) \
    EXC(PendingDeprecationWarning) \
    EXC(ResourceWarning) \
    EXC(RuntimeWarning) \
    EXC(SyntaxWarning) \
    EXC(UnicodeWarning) \
    EXC(UserWarning) \
    \
    EXC(BlockingIOError) \
    EXC(ChildProcessError) \
    EXC(ConnectionError) \
    EXC(FileExistsError) \
    EXC(FileNotFoundError) \
    EXC(InterruptedError) \
    EXC(IsADirectoryError) \
    EXC(NotADirectoryError) \
    EXC(PermissionError) \
    EXC(ProcessLookupError) \
    EXC(TimeoutError) \
    \
    EXC(IndentationError) \
    EXC(IndexError) \
    EXC(KeyError) \
    EXC(ImportCycleError) \
    EXC(ModuleNotFoundError) \
    EXC(NotImplementedError) \
    EXC(PythonFinalizationError) \
    EXC(RecursionError) \
    EXC(UnboundLocalError) \
    EXC(UnicodeError) \
    \
    EXC(BrokenPipeError) \
    EXC(ConnectionAbortedError) \
    EXC(ConnectionRefusedError) \
    EXC(ConnectionResetError) \
    \
    EXC(TabError) \
    \
    EXC(UnicodeDecodeError) \
    EXC(UnicodeEncodeError) \
    EXC(UnicodeTranslateError)


/* Static exceptions whose exposed name differs from the symbol's
   suffix after _PyExc_.  Each entry is (NAME, "exposed name"). */
#define _Py_FOREACH_RENAMED_EXCEPTION_TYPE(EXC) \
    EXC(IncompleteInputError, "_IncompleteInputError")


/* runtime lifecycle */

extern PyStatus _PyExc_InitState(PyInterpreterState *);
extern PyStatus _PyExc_InitGlobalObjects(PyInterpreterState *);
extern int _PyExc_InitTypes(PyInterpreterState *);
extern void _PyExc_Fini(PyInterpreterState *);


/* other API */

struct _Py_exc_state {
    // The dict mapping from errno codes to OSError subclasses
    PyObject *errnomap;
    PyBaseExceptionObject *memerrors_freelist;
    int memerrors_numfree;
#ifdef Py_GIL_DISABLED
    PyMutex memerrors_lock;
#endif
    // The ExceptionGroup type
    PyObject *PyExc_ExceptionGroup;
};

extern void _PyExc_ClearExceptionGroupType(PyInterpreterState *);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_EXCEPTIONS_H */
