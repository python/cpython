#include "Python.h"
#include <CoreFoundation/CFRunLoop.h>

/* These macros are defined in Python 2.3 but not 2.2 */
#ifndef PyMODINIT_FUNC
#define PyMODINIT_FUNC void
#endif
#ifndef PyDoc_STRVAR
#define PyDoc_STRVAR(Var,Str) static char Var[] = Str
#endif


#undef AUTOGIL_DEBUG

static PyObject *AutoGILError;


static void autoGILCallback(CFRunLoopObserverRef observer,
			    CFRunLoopActivity activity,
			    void *info) {
	PyThreadState **p_tstate = (PyThreadState **)info;

	switch (activity) {
	case kCFRunLoopBeforeWaiting:
		/* going to sleep, release GIL */
#ifdef AUTOGIL_DEBUG
		fprintf(stderr, "going to sleep, release GIL\n");
#endif
		*p_tstate = PyEval_SaveThread();
		break;
	case kCFRunLoopAfterWaiting:
		/* waking up, acquire GIL */
#ifdef AUTOGIL_DEBUG
		fprintf(stderr, "waking up, acquire GIL\n");
#endif
		PyEval_RestoreThread(*p_tstate);
		*p_tstate = NULL;
		break;
	default:
		break;
	}
}

static void infoRelease(const void *info) {
	/* XXX This should get called when the run loop is deallocated,
	   but this doesn't seem to happen. So for now: leak. */
	PyMem_Free((void *)info);
}

static PyObject *
autoGIL_installAutoGIL(PyObject *self)
{
	PyObject *tstate_dict = PyThreadState_GetDict();
	PyObject *v;
	CFRunLoopRef rl;
	PyThreadState **p_tstate;  /* for use in the info field */
	CFRunLoopObserverContext context = {0, NULL, NULL, NULL, NULL};
	CFRunLoopObserverRef observer;

	if (tstate_dict == NULL)
		return NULL;
	v = PyDict_GetItemString(tstate_dict, "autoGIL.InstalledAutoGIL");
	if (v != NULL) {
		/* we've already installed a callback for this thread */
		Py_INCREF(Py_None);
		return Py_None;
	}

	rl = CFRunLoopGetCurrent();
	if (rl == NULL) {
		PyErr_SetString(AutoGILError,
				"can't get run loop for current thread");
		return NULL;
	}

	p_tstate = PyMem_Malloc(sizeof(PyThreadState *));
	if (p_tstate == NULL) {
		PyErr_SetString(PyExc_MemoryError,
				"not enough memory to allocate "
				"tstate pointer");
		return NULL;
	}
	*p_tstate = NULL;
	context.info = (void *)p_tstate;
	context.release = infoRelease;

	observer = CFRunLoopObserverCreate(
		NULL,
		kCFRunLoopBeforeWaiting | kCFRunLoopAfterWaiting,
		1, 0, autoGILCallback, &context);
	if (observer == NULL) {
		PyErr_SetString(AutoGILError,
				"can't create event loop observer");
		return NULL;
	}
	CFRunLoopAddObserver(rl, observer, kCFRunLoopDefaultMode);
	/* XXX how to check for errors? */

	/* register that we have installed a callback for this thread */
	if (PyDict_SetItemString(tstate_dict, "autoGIL.InstalledAutoGIL",
				 Py_None) < 0)
		return NULL;

	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(autoGIL_installAutoGIL_doc,
"installAutoGIL() -> None\n\
Install an observer callback in the event loop (CFRunLoop) for the\n\
current thread, that will lock and unlock the Global Interpreter Lock\n\
(GIL) at appropriate times, allowing other Python threads to run while\n\
the event loop is idle."
);

static PyMethodDef autoGIL_methods[] = {
	{
		"installAutoGIL",
		(PyCFunction)autoGIL_installAutoGIL,
		METH_NOARGS,
		autoGIL_installAutoGIL_doc
	},
	{ 0, 0, 0, 0 } /* sentinel */
};

PyDoc_STRVAR(autoGIL_docs,
"The autoGIL module provides a function (installAutoGIL) that\n\
automatically locks and unlocks Python's Global Interpreter Lock\n\
when running an event loop."
);

PyMODINIT_FUNC
initautoGIL(void)
{
	PyObject *mod;

	mod = Py_InitModule4("autoGIL", autoGIL_methods, autoGIL_docs,
			     NULL, PYTHON_API_VERSION);
	if (mod == NULL)
		return;
	AutoGILError = PyErr_NewException("autoGIL.AutoGILError",
					  PyExc_Exception, NULL);
	if (AutoGILError == NULL)
		return;
	Py_INCREF(AutoGILError);
	if (PyModule_AddObject(mod, "AutoGILError",
			       AutoGILError) < 0)
		return;
}
