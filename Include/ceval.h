#ifndef Py_CEVAL_H
#define Py_CEVAL_H
#ifdef __cplusplus
extern "C" {
#endif


/* Interface to random parts in ceval.c */

DL_IMPORT(PyObject *) PyEval_CallObjectWithKeywords
	(PyObject *, PyObject *, PyObject *);

/* DLL-level Backwards compatibility: */
#undef PyEval_CallObject
DL_IMPORT(PyObject *) PyEval_CallObject(PyObject *, PyObject *);

/* Inline this */
#define PyEval_CallObject(func,arg) \
        PyEval_CallObjectWithKeywords(func, arg, (PyObject *)NULL)

DL_IMPORT(PyObject *) PyEval_CallFunction(PyObject *obj, char *format, ...);
DL_IMPORT(PyObject *) PyEval_CallMethod(PyObject *obj,
                                        char *methodname, char *format, ...);

DL_IMPORT(PyObject *) PyEval_GetBuiltins(void);
DL_IMPORT(PyObject *) PyEval_GetGlobals(void);
DL_IMPORT(PyObject *) PyEval_GetLocals(void);
DL_IMPORT(PyObject *) PyEval_GetOwner(void);
DL_IMPORT(PyObject *) PyEval_GetFrame(void);
DL_IMPORT(int) PyEval_GetRestricted(void);

DL_IMPORT(int) Py_FlushLine(void);

DL_IMPORT(int) Py_AddPendingCall(int (*func)(void *), void *arg);
DL_IMPORT(int) Py_MakePendingCalls(void);

DL_IMPORT(void) Py_SetRecursionLimit(int);
DL_IMPORT(int) Py_GetRecursionLimit(void);

/* Interface for threads.

   A module that plans to do a blocking system call (or something else
   that lasts a long time and doesn't touch Python data) can allow other
   threads to run as follows:

	...preparations here...
	Py_BEGIN_ALLOW_THREADS
	...blocking system call here...
	Py_END_ALLOW_THREADS
	...interpret result here...

   The Py_BEGIN_ALLOW_THREADS/Py_END_ALLOW_THREADS pair expands to a
   {}-surrounded block.
   To leave the block in the middle (e.g., with return), you must insert
   a line containing Py_BLOCK_THREADS before the return, e.g.

	if (...premature_exit...) {
		Py_BLOCK_THREADS
		PyErr_SetFromErrno(PyExc_IOError);
		return NULL;
	}

   An alternative is:

	Py_BLOCK_THREADS
	if (...premature_exit...) {
		PyErr_SetFromErrno(PyExc_IOError);
		return NULL;
	}
	Py_UNBLOCK_THREADS

   For convenience, that the value of 'errno' is restored across
   Py_END_ALLOW_THREADS and Py_BLOCK_THREADS.

   WARNING: NEVER NEST CALLS TO Py_BEGIN_ALLOW_THREADS AND
   Py_END_ALLOW_THREADS!!!

   The function PyEval_InitThreads() should be called only from
   initthread() in "threadmodule.c".

   Note that not yet all candidates have been converted to use this
   mechanism!
*/

extern DL_IMPORT(PyThreadState *) PyEval_SaveThread(void);
extern DL_IMPORT(void) PyEval_RestoreThread(PyThreadState *);

#ifdef WITH_THREAD

extern DL_IMPORT(void) PyEval_InitThreads(void);
extern DL_IMPORT(void) PyEval_AcquireLock(void);
extern DL_IMPORT(void) PyEval_ReleaseLock(void);
extern DL_IMPORT(void) PyEval_AcquireThread(PyThreadState *tstate);
extern DL_IMPORT(void) PyEval_ReleaseThread(PyThreadState *tstate);
extern DL_IMPORT(void) PyEval_ReInitThreads(void);

#define Py_BEGIN_ALLOW_THREADS { \
			PyThreadState *_save; \
			_save = PyEval_SaveThread();
#define Py_BLOCK_THREADS	PyEval_RestoreThread(_save);
#define Py_UNBLOCK_THREADS	_save = PyEval_SaveThread();
#define Py_END_ALLOW_THREADS	PyEval_RestoreThread(_save); \
		 }

#else /* !WITH_THREAD */

#define Py_BEGIN_ALLOW_THREADS {
#define Py_BLOCK_THREADS
#define Py_UNBLOCK_THREADS
#define Py_END_ALLOW_THREADS }

#endif /* !WITH_THREAD */

extern DL_IMPORT(int) _PyEval_SliceIndex(PyObject *, int *);


#ifdef __cplusplus
}
#endif
#endif /* !Py_CEVAL_H */
