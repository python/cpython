
/* Thread and interpreter state structures and their interfaces */


#ifndef Py_PYSTATE_H
#define Py_PYSTATE_H
#ifdef __cplusplus
extern "C" {
#endif

/* State shared between threads */

struct _ts; /* Forward */
struct _is; /* Forward */

typedef struct _is {

    struct _is *next;
    struct _ts *tstate_head;

    PyObject *modules;
    PyObject *sysdict;
    PyObject *builtins;

    int checkinterval;
#ifdef HAVE_DLOPEN
    int dlopenflags;
#endif

} PyInterpreterState;


/* State unique per thread */

struct _frame; /* Avoid including frameobject.h */

/* Py_tracefunc return -1 when raising an exception, or 0 for success. */
typedef int (*Py_tracefunc)(PyObject *, struct _frame *, int, PyObject *);

/* The following values are used for 'what' for tracefunc functions: */
#define PyTrace_CALL 0
#define PyTrace_EXCEPTION 1
#define PyTrace_LINE 2
#define PyTrace_RETURN 3

typedef struct _ts {

    struct _ts *next;
    PyInterpreterState *interp;

    struct _frame *frame;
    int recursion_depth;
    int ticker;
    int tracing;
    int use_tracing;

    Py_tracefunc c_profilefunc;
    Py_tracefunc c_tracefunc;
    PyObject *c_profileobj;
    PyObject *c_traceobj;

    PyObject *curexc_type;
    PyObject *curexc_value;
    PyObject *curexc_traceback;

    PyObject *exc_type;
    PyObject *exc_value;
    PyObject *exc_traceback;

    PyObject *dict;

    /* XXX signal handlers should also be here */

} PyThreadState;


DL_IMPORT(PyInterpreterState *) PyInterpreterState_New(void);
DL_IMPORT(void) PyInterpreterState_Clear(PyInterpreterState *);
DL_IMPORT(void) PyInterpreterState_Delete(PyInterpreterState *);

DL_IMPORT(PyThreadState *) PyThreadState_New(PyInterpreterState *);
DL_IMPORT(void) PyThreadState_Clear(PyThreadState *);
DL_IMPORT(void) PyThreadState_Delete(PyThreadState *);
#ifdef WITH_THREAD
DL_IMPORT(void) PyThreadState_DeleteCurrent(void);
#endif

DL_IMPORT(PyThreadState *) PyThreadState_Get(void);
DL_IMPORT(PyThreadState *) PyThreadState_Swap(PyThreadState *);
DL_IMPORT(PyObject *) PyThreadState_GetDict(void);


/* Variable and macro for in-line access to current thread state */

extern DL_IMPORT(PyThreadState *) _PyThreadState_Current;

#ifdef Py_DEBUG
#define PyThreadState_GET() PyThreadState_Get()
#else
#define PyThreadState_GET() (_PyThreadState_Current)
#endif

/* Routines for advanced debuggers, requested by David Beazley.
   Don't use unless you know what you are doing! */
DL_IMPORT(PyInterpreterState *) PyInterpreterState_Head(void);
DL_IMPORT(PyInterpreterState *) PyInterpreterState_Next(PyInterpreterState *);
DL_IMPORT(PyThreadState *) PyInterpreterState_ThreadHead(PyInterpreterState *);
DL_IMPORT(PyThreadState *) PyThreadState_Next(PyThreadState *);

#ifdef __cplusplus
}
#endif
#endif /* !Py_PYSTATE_H */
