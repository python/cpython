/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Thread module */
/* Interface to Sjoerd's portable C thread library */

#include "Python.h"

#ifndef WITH_THREAD
#error "Error!  The rest of Python is not compiled with thread support."
#error "Rerun configure, adding a --with-thread option."
#error "Then run `make clean' followed by `make'."
#endif

#include "thread.h"

static PyObject *ThreadError;


/* Lock objects */

typedef struct {
	PyObject_HEAD
	type_lock lock_lock;
} lockobject;

staticforward PyTypeObject Locktype;

#define is_lockobject(v)		((v)->ob_type == &Locktype)

type_lock
getlocklock(lock)
	PyObject *lock;
{
	if (lock == NULL || !is_lockobject(lock))
		return NULL;
	else
		return ((lockobject *) lock)->lock_lock;
}

static lockobject *
newlockobject()
{
	lockobject *self;
	self = PyObject_NEW(lockobject, &Locktype);
	if (self == NULL)
		return NULL;
	self->lock_lock = allocate_lock();
	if (self->lock_lock == NULL) {
		PyMem_DEL(self);
		self = NULL;
		PyErr_SetString(ThreadError, "can't allocate lock");
	}
	return self;
}

static void
lock_dealloc(self)
	lockobject *self;
{
	/* Unlock the lock so it's safe to free it */
	acquire_lock(self->lock_lock, 0);
	release_lock(self->lock_lock);
	
	free_lock(self->lock_lock);
	PyMem_DEL(self);
}

static PyObject *
lock_acquire_lock(self, args)
	lockobject *self;
	PyObject *args;
{
	int i;

	if (args != NULL) {
		if (!PyArg_Parse(args, "i", &i))
			return NULL;
	}
	else
		i = 1;

	Py_BEGIN_ALLOW_THREADS
	i = acquire_lock(self->lock_lock, i);
	Py_END_ALLOW_THREADS

	if (args == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	else
		return PyInt_FromLong((long)i);
}

static PyObject *
lock_release_lock(self, args)
	lockobject *self;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;

	/* Sanity check: the lock must be locked */
	if (acquire_lock(self->lock_lock, 0)) {
		release_lock(self->lock_lock);
		PyErr_SetString(ThreadError, "release unlocked lock");
		return NULL;
	}

	release_lock(self->lock_lock);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
lock_locked_lock(self, args)
	lockobject *self;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;

	if (acquire_lock(self->lock_lock, 0)) {
		release_lock(self->lock_lock);
		return PyInt_FromLong(0L);
	}
	return PyInt_FromLong(1L);
}

static PyMethodDef lock_methods[] = {
	{"acquire_lock",	(PyCFunction)lock_acquire_lock},
	{"acquire",		(PyCFunction)lock_acquire_lock},
	{"release_lock",	(PyCFunction)lock_release_lock},
	{"release",		(PyCFunction)lock_release_lock},
	{"locked_lock",		(PyCFunction)lock_locked_lock},
	{"locked",		(PyCFunction)lock_locked_lock},
	{NULL,			NULL}		/* sentinel */
};

static PyObject *
lock_getattr(self, name)
	lockobject *self;
	char *name;
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
t_bootstrap(boot_raw)
	void *boot_raw;
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
		if (PyErr_Occurred() == PyExc_SystemExit)
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
	exit_thread();
}

static PyObject *
thread_start_new_thread(self, fargs)
	PyObject *self; /* Not used */
	PyObject *fargs;
{
	PyObject *func, *args = NULL, *keyw = NULL;
	struct bootstate *boot;

	if (!PyArg_ParseTuple(fargs, "OO|O", &func, &args, &keyw))
		return NULL;
	if (!PyCallable_Check(func)) {
		PyErr_SetString(PyExc_TypeError,
				"first arg must be callable");
		return NULL;
	}
	if (!PyTuple_Check(args)) {
		PyErr_SetString(PyExc_TypeError,
				"optional 2nd arg must be a tuple");
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
	if (!start_new_thread(t_bootstrap, (void*) boot)) {
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

static PyObject *
thread_exit_thread(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	PyErr_SetNone(PyExc_SystemExit);
	return NULL;
}

#ifndef NO_EXIT_PROG
static PyObject *
thread_exit_prog(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	int sts;
	if (!PyArg_Parse(args, "i", &sts))
		return NULL;
	Py_Exit(sts); /* Calls exit_prog(sts) or _exit_prog(sts) */
	for (;;) { } /* Should not be reached */
}
#endif

static PyObject *
thread_allocate_lock(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	return (PyObject *) newlockobject();
}

static PyObject *
thread_get_ident(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	long ident;
	if (!PyArg_NoArgs(args))
		return NULL;
	ident = get_thread_ident();
	if (ident == -1) {
		PyErr_SetString(ThreadError, "no current thread ident");
		return NULL;
	}
	return PyInt_FromLong(ident);
}

static PyMethodDef thread_methods[] = {
	{"start_new_thread",	(PyCFunction)thread_start_new_thread, 1},
	{"start_new",		(PyCFunction)thread_start_new_thread, 1},
	{"allocate_lock",	(PyCFunction)thread_allocate_lock},
	{"allocate",		(PyCFunction)thread_allocate_lock},
	{"exit_thread",		(PyCFunction)thread_exit_thread},
	{"exit",		(PyCFunction)thread_exit_thread},
	{"get_ident",		(PyCFunction)thread_get_ident},
#ifndef NO_EXIT_PROG
	{"exit_prog",		(PyCFunction)thread_exit_prog},
#endif
	{NULL,			NULL}		/* sentinel */
};


/* Initialization function */

void
initthread()
{
	PyObject *m, *d;

	/* Create the module and add the functions */
	m = Py_InitModule("thread", thread_methods);

	/* Add a symbolic constant */
	d = PyModule_GetDict(m);
	ThreadError = PyErr_NewException("thread.error", NULL, NULL);
	PyDict_SetItemString(d, "error", ThreadError);

	/* Initialize the C thread library */
	init_thread();
}
