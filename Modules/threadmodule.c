
/* Thread module */
/* Interface to Sjoerd's portable C thread library */

#include "Python.h"

#ifndef WITH_THREAD
#error "Error!  The rest of Python is not compiled with thread support."
#error "Rerun configure, adding a --with-thread option."
#error "Then run `make clean' followed by `make'."
#endif

#include "pythread.h"

static PyObject *ThreadError;


/* Lock objects */

typedef struct {
	PyObject_HEAD
	PyThread_type_lock lock_lock;
} lockobject;

staticforward PyTypeObject Locktype;

static lockobject *
newlockobject(void)
{
	lockobject *self;
	self = PyObject_New(lockobject, &Locktype);
	if (self == NULL)
		return NULL;
	self->lock_lock = PyThread_allocate_lock();
	if (self->lock_lock == NULL) {
		PyObject_Del(self);
		self = NULL;
		PyErr_SetString(ThreadError, "can't allocate lock");
	}
	return self;
}

static void
lock_dealloc(lockobject *self)
{
	/* Unlock the lock so it's safe to free it */
	PyThread_acquire_lock(self->lock_lock, 0);
	PyThread_release_lock(self->lock_lock);
	
	PyThread_free_lock(self->lock_lock);
	PyObject_Del(self);
}

static PyObject *
lock_PyThread_acquire_lock(lockobject *self, PyObject *args)
{
	int i;

	if (args != NULL) {
		if (!PyArg_Parse(args, "i", &i))
			return NULL;
	}
	else
		i = 1;

	Py_BEGIN_ALLOW_THREADS
	i = PyThread_acquire_lock(self->lock_lock, i);
	Py_END_ALLOW_THREADS

	if (args == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	else
		return PyInt_FromLong((long)i);
}

static char acquire_doc[] =
"acquire([wait]) -> None or Boolean\n\
(PyThread_acquire_lock() is an obsolete synonym)\n\
\n\
Lock the lock.  Without argument, this blocks if the lock is already\n\
locked (even by the same thread), waiting for another thread to release\n\
the lock, and return None when the lock is acquired.\n\
With a Boolean argument, this will only block if the argument is true,\n\
and the return value reflects whether the lock is acquired.\n\
The blocking operation is not interruptible.";

static PyObject *
lock_PyThread_release_lock(lockobject *self, PyObject *args)
{
	if (!PyArg_NoArgs(args))
		return NULL;

	/* Sanity check: the lock must be locked */
	if (PyThread_acquire_lock(self->lock_lock, 0)) {
		PyThread_release_lock(self->lock_lock);
		PyErr_SetString(ThreadError, "release unlocked lock");
		return NULL;
	}

	PyThread_release_lock(self->lock_lock);
	Py_INCREF(Py_None);
	return Py_None;
}

static char release_doc[] =
"release()\n\
(PyThread_release_lock() is an obsolete synonym)\n\
\n\
Release the lock, allowing another thread that is blocked waiting for\n\
the lock to acquire the lock.  The lock must be in the locked state,\n\
but it needn't be locked by the same thread that unlocks it.";

static PyObject *
lock_locked_lock(lockobject *self, PyObject *args)
{
	if (!PyArg_NoArgs(args))
		return NULL;

	if (PyThread_acquire_lock(self->lock_lock, 0)) {
		PyThread_release_lock(self->lock_lock);
		return PyInt_FromLong(0L);
	}
	return PyInt_FromLong(1L);
}

static char locked_doc[] =
"locked() -> Boolean\n\
(locked_lock() is an obsolete synonym)\n\
\n\
Return whether the lock is in the locked state.";

static PyMethodDef lock_methods[] = {
	{"acquire_lock", (PyCFunction)lock_PyThread_acquire_lock, 
	 METH_OLDARGS, acquire_doc},
	{"acquire",      (PyCFunction)lock_PyThread_acquire_lock, 
	 METH_OLDARGS, acquire_doc},
	{"release_lock", (PyCFunction)lock_PyThread_release_lock, 
	 METH_OLDARGS, release_doc},
	{"release",      (PyCFunction)lock_PyThread_release_lock, 
	 METH_OLDARGS, release_doc},
	{"locked_lock",  (PyCFunction)lock_locked_lock,  
	 METH_OLDARGS, locked_doc},
	{"locked",       (PyCFunction)lock_locked_lock,  
	 METH_OLDARGS, locked_doc},
	{NULL,           NULL}		/* sentinel */
};

static PyObject *
lock_getattr(lockobject *self, char *name)
{
	return Py_FindMethod(lock_methods, (PyObject *)self, name);
}

static PyTypeObject Locktype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				/*ob_size*/
	"lock",				/*tp_name*/
	sizeof(lockobject),		/*tp_size*/
	0,				/*tp_itemsize*/
	/* methods */
	(destructor)lock_dealloc,	/*tp_dealloc*/
	0,				/*tp_print*/
	(getattrfunc)lock_getattr,	/*tp_getattr*/
	0,				/*tp_setattr*/
	0,				/*tp_compare*/
	0,				/*tp_repr*/
};


/* Module functions */

struct bootstate {
	PyInterpreterState *interp;
	PyObject *func;
	PyObject *args;
	PyObject *keyw;
};

static void
t_bootstrap(void *boot_raw)
{
	struct bootstate *boot = (struct bootstate *) boot_raw;
	PyThreadState *tstate;
	PyObject *res;

	tstate = PyThreadState_New(boot->interp);
	PyEval_AcquireThread(tstate);
	res = PyEval_CallObjectWithKeywords(
		boot->func, boot->args, boot->keyw);
	Py_DECREF(boot->func);
	Py_DECREF(boot->args);
	Py_XDECREF(boot->keyw);
	PyMem_DEL(boot_raw);
	if (res == NULL) {
		if (PyErr_ExceptionMatches(PyExc_SystemExit))
			PyErr_Clear();
		else {
			fprintf(stderr, "Unhandled exception in thread:\n");
			PyErr_PrintEx(0);
		}
	}
	else
		Py_DECREF(res);
	PyThreadState_Clear(tstate);
	PyEval_ReleaseThread(tstate);
	PyThreadState_Delete(tstate);
	PyThread_exit_thread();
}

static PyObject *
thread_PyThread_start_new_thread(PyObject *self, PyObject *fargs)
{
	PyObject *func, *args, *keyw = NULL;
	struct bootstate *boot;

	if (!PyArg_ParseTuple(fargs, "OO|O:start_new_thread", &func, &args, &keyw))
		return NULL;
	if (!PyCallable_Check(func)) {
		PyErr_SetString(PyExc_TypeError,
				"first arg must be callable");
		return NULL;
	}
	if (!PyTuple_Check(args)) {
		PyErr_SetString(PyExc_TypeError,
				"2nd arg must be a tuple");
		return NULL;
	}
	if (keyw != NULL && !PyDict_Check(keyw)) {
		PyErr_SetString(PyExc_TypeError,
				"optional 3rd arg must be a dictionary");
		return NULL;
	}
	boot = PyMem_NEW(struct bootstate, 1);
	if (boot == NULL)
		return PyErr_NoMemory();
	boot->interp = PyThreadState_Get()->interp;
	boot->func = func;
	boot->args = args;
	boot->keyw = keyw;
	Py_INCREF(func);
	Py_INCREF(args);
	Py_XINCREF(keyw);
	PyEval_InitThreads(); /* Start the interpreter's thread-awareness */
	if (!PyThread_start_new_thread(t_bootstrap, (void*) boot)) {
		PyErr_SetString(ThreadError, "can't start new thread\n");
		Py_DECREF(func);
		Py_DECREF(args);
		Py_XDECREF(keyw);
		PyMem_DEL(boot);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static char start_new_doc[] =
"start_new_thread(functon, args[, kwargs])\n\
(start_new() is an obsolete synonym)\n\
\n\
Start a new thread.  The thread will call the function with positional\n\
arguments from the tuple args and keyword arguments taken from the optional\n\
dictionary kwargs.  The thread exits when the function returns; the return\n\
value is ignored.  The thread will also exit when the function raises an\n\
unhandled exception; a stack trace will be printed unless the exception is\n\
SystemExit.";

static PyObject *
thread_PyThread_exit_thread(PyObject *self, PyObject *args)
{
	if (!PyArg_NoArgs(args))
		return NULL;
	PyErr_SetNone(PyExc_SystemExit);
	return NULL;
}

static char exit_doc[] =
"exit()\n\
(PyThread_exit_thread() is an obsolete synonym)\n\
\n\
This is synonymous to ``raise SystemExit''.  It will cause the current\n\
thread to exit silently unless the exception is caught.";

#ifndef NO_EXIT_PROG
static PyObject *
thread_PyThread_exit_prog(PyObject *self, PyObject *args)
{
	int sts;
	if (!PyArg_Parse(args, "i", &sts))
		return NULL;
	Py_Exit(sts); /* Calls PyThread_exit_prog(sts) or _PyThread_exit_prog(sts) */
	for (;;) { } /* Should not be reached */
}
#endif

static PyObject *
thread_PyThread_allocate_lock(PyObject *self, PyObject *args)
{
	if (!PyArg_NoArgs(args))
		return NULL;
	return (PyObject *) newlockobject();
}

static char allocate_doc[] =
"allocate_lock() -> lock object\n\
(allocate() is an obsolete synonym)\n\
\n\
Create a new lock object.  See LockType.__doc__ for information about locks.";

static PyObject *
thread_get_ident(PyObject *self, PyObject *args)
{
	long ident;
	if (!PyArg_NoArgs(args))
		return NULL;
	ident = PyThread_get_thread_ident();
	if (ident == -1) {
		PyErr_SetString(ThreadError, "no current thread ident");
		return NULL;
	}
	return PyInt_FromLong(ident);
}

static char get_ident_doc[] =
"get_ident() -> integer\n\
\n\
Return a non-zero integer that uniquely identifies the current thread\n\
amongst other threads that exist simultaneously.\n\
This may be used to identify per-thread resources.\n\
Even though on some platforms threads identities may appear to be\n\
allocated consecutive numbers starting at 1, this behavior should not\n\
be relied upon, and the number should be seen purely as a magic cookie.\n\
A thread's identity may be reused for another thread after it exits.";

static PyMethodDef thread_methods[] = {
	{"start_new_thread",	(PyCFunction)thread_PyThread_start_new_thread,
	                        METH_VARARGS,
				start_new_doc},
	{"start_new",		(PyCFunction)thread_PyThread_start_new_thread, 
	                        METH_VARARGS,
				start_new_doc},
	{"allocate_lock",	(PyCFunction)thread_PyThread_allocate_lock, 
	 METH_OLDARGS, allocate_doc},
	{"allocate",		(PyCFunction)thread_PyThread_allocate_lock, 
	 METH_OLDARGS, allocate_doc},
	{"exit_thread",		(PyCFunction)thread_PyThread_exit_thread, 
	 METH_OLDARGS, exit_doc},
	{"exit",		(PyCFunction)thread_PyThread_exit_thread, 
	 METH_OLDARGS, exit_doc},
	{"get_ident",		(PyCFunction)thread_get_ident, 
	 METH_OLDARGS, get_ident_doc},
#ifndef NO_EXIT_PROG
	{"exit_prog",		(PyCFunction)thread_PyThread_exit_prog},
#endif
	{NULL,			NULL}		/* sentinel */
};


/* Initialization function */

static char thread_doc[] =
"This module provides primitive operations to write multi-threaded programs.\n\
The 'threading' module provides a more convenient interface.";

static char lock_doc[] =
"A lock object is a synchronization primitive.  To create a lock,\n\
call the PyThread_allocate_lock() function.  Methods are:\n\
\n\
acquire() -- lock the lock, possibly blocking until it can be obtained\n\
release() -- unlock of the lock\n\
locked() -- test whether the lock is currently locked\n\
\n\
A lock is not owned by the thread that locked it; another thread may\n\
unlock it.  A thread attempting to lock a lock that it has already locked\n\
will block until another thread unlocks it.  Deadlocks may ensue.";

DL_EXPORT(void)
initthread(void)
{
	PyObject *m, *d;

	/* Create the module and add the functions */
	m = Py_InitModule3("thread", thread_methods, thread_doc);

	/* Add a symbolic constant */
	d = PyModule_GetDict(m);
	ThreadError = PyErr_NewException("thread.error", NULL, NULL);
	PyDict_SetItemString(d, "error", ThreadError);
	Locktype.tp_doc = lock_doc;
	Py_INCREF(&Locktype);
	PyDict_SetItemString(d, "LockType", (PyObject *)&Locktype);

	/* Initialize the C thread library */
	PyThread_init_thread();
}
