/***********************************************************
Copyright (C) 1994 Steen Lumholt.

                        All Rights Reserved

******************************************************************/

/* _tkinter.c -- Interface to libtk.a and libtcl.a. */

/* TCL/TK VERSION INFO:

	Only Tcl/Tk 8.0 and later are supported.  Older versions are not
	supported.  (Use Python 1.5.2 if you cannot upgrade your Tcl/Tk
	libraries.)
*/

/* XXX Further speed-up ideas, involving Tcl 8.0 features:

   - In Tcl_Call(), create Tcl objects from the arguments, possibly using
   intelligent mappings between Python objects and Tcl objects (e.g. ints,
   floats and Tcl window pointers could be handled specially).

   - Register a new Tcl type, "Python callable", which can be called more
   efficiently and passed to Tcl_EvalObj() directly (if this is possible).

*/


#include "Python.h"
#include <ctype.h>

#ifdef WITH_THREAD
#include "pythread.h"
#endif

#ifdef MS_WINDOWS
#include <windows.h>
#endif

#ifdef macintosh
#define MAC_TCL
#endif

#include <tcl.h>
#include <tk.h>

#define TKMAJORMINOR (TK_MAJOR_VERSION*1000 + TK_MINOR_VERSION)

#if TKMAJORMINOR < 8000
#error "Tk older than 8.0 not supported"
#endif

#if defined(macintosh)
/* Sigh, we have to include this to get at the tcl qd pointer */
#include <tkMac.h>
/* And this one we need to clear the menu bar */
#include <Menus.h>
#endif

#if !(defined(MS_WINDOWS) || defined(__CYGWIN__))
#define HAVE_CREATEFILEHANDLER
#endif

#ifdef HAVE_CREATEFILEHANDLER

/* Tcl_CreateFileHandler() changed several times; these macros deal with the
   messiness.  In Tcl 8.0 and later, it is not available on Windows (and on
   Unix, only because Jack added it back); when available on Windows, it only
   applies to sockets. */

#ifdef MS_WINDOWS
#define FHANDLETYPE TCL_WIN_SOCKET
#else
#define FHANDLETYPE TCL_UNIX_FD
#endif

/* If Tcl can wait for a Unix file descriptor, define the EventHook() routine
   which uses this to handle Tcl events while the user is typing commands. */

#if FHANDLETYPE == TCL_UNIX_FD
#define WAIT_FOR_STDIN
#endif

#endif /* HAVE_CREATEFILEHANDLER */

#ifdef MS_WINDOWS
#include <conio.h>
#define WAIT_FOR_STDIN
#endif

#ifdef WITH_THREAD

/* The threading situation is complicated.  Tcl is not thread-safe, except for
   Tcl 8.1, which will probably remain in alpha status for another 6 months
   (and the README says that Tk will probably remain thread-unsafe forever).
   So we need to use a lock around all uses of Tcl.  Previously, the Python
   interpreter lock was used for this.  However, this causes problems when
   other Python threads need to run while Tcl is blocked waiting for events.

   To solve this problem, a separate lock for Tcl is introduced.  Holding it
   is incompatible with holding Python's interpreter lock.  The following four
   macros manipulate both locks together.

   ENTER_TCL and LEAVE_TCL are brackets, just like Py_BEGIN_ALLOW_THREADS and
   Py_END_ALLOW_THREADS.  They should be used whenever a call into Tcl is made
   that could call an event handler, or otherwise affect the state of a Tcl
   interpreter.  These assume that the surrounding code has the Python
   interpreter lock; inside the brackets, the Python interpreter lock has been 
   released and the lock for Tcl has been acquired.

   Sometimes, it is necessary to have both the Python lock and the Tcl lock.
   (For example, when transferring data from the Tcl interpreter result to a
   Python string object.)  This can be done by using different macros to close
   the ENTER_TCL block: ENTER_OVERLAP reacquires the Python lock (and restores
   the thread state) but doesn't release the Tcl lock; LEAVE_OVERLAP_TCL
   releases the Tcl lock.

   By contrast, ENTER_PYTHON and LEAVE_PYTHON are used in Tcl event
   handlers when the handler needs to use Python.  Such event handlers are
   entered while the lock for Tcl is held; the event handler presumably needs
   to use Python.  ENTER_PYTHON releases the lock for Tcl and acquires
   the Python interpreter lock, restoring the appropriate thread state, and
   LEAVE_PYTHON releases the Python interpreter lock and re-acquires the lock
   for Tcl.  It is okay for ENTER_TCL/LEAVE_TCL pairs to be contained inside
   the code between ENTER_PYTHON and LEAVE_PYTHON.

   These locks expand to several statements and brackets; they should not be
   used in branches of if statements and the like.

*/

static PyThread_type_lock tcl_lock = 0;
static PyThreadState *tcl_tstate = NULL;

#define ENTER_TCL \
	{ PyThreadState *tstate = PyThreadState_Get(); Py_BEGIN_ALLOW_THREADS \
	    PyThread_acquire_lock(tcl_lock, 1); tcl_tstate = tstate;

#define LEAVE_TCL \
    tcl_tstate = NULL; PyThread_release_lock(tcl_lock); Py_END_ALLOW_THREADS}

#define ENTER_OVERLAP \
	Py_END_ALLOW_THREADS

#define LEAVE_OVERLAP_TCL \
	tcl_tstate = NULL; PyThread_release_lock(tcl_lock); }

#define ENTER_PYTHON \
	{ PyThreadState *tstate = tcl_tstate; tcl_tstate = NULL; \
            PyThread_release_lock(tcl_lock); PyEval_RestoreThread((tstate)); }

#define LEAVE_PYTHON \
	{ PyThreadState *tstate = PyEval_SaveThread(); \
            PyThread_acquire_lock(tcl_lock, 1); tcl_tstate = tstate; }

#else

#define ENTER_TCL
#define LEAVE_TCL
#define ENTER_OVERLAP
#define LEAVE_OVERLAP_TCL
#define ENTER_PYTHON
#define LEAVE_PYTHON

#endif

#ifdef macintosh

/*
** Additional cruft needed by Tcl/Tk on the Mac.
** This is for Tcl 7.5 and Tk 4.1 (patch release 1).
*/

/* ckfree() expects a char* */
#define FREECAST (char *)

#include <Events.h> /* For EventRecord */

typedef int (*TclMacConvertEventPtr) (EventRecord *eventPtr);
void Tcl_MacSetEventProc(TclMacConvertEventPtr procPtr);
int TkMacConvertEvent(EventRecord *eventPtr);

staticforward int PyMacConvertEvent(EventRecord *eventPtr);

#if defined(__CFM68K__) && !defined(__USING_STATIC_LIBS__)
	#pragma import on
#endif

#include <SIOUX.h>
extern int SIOUXIsAppWindow(WindowPtr);

#if defined(__CFM68K__) && !defined(__USING_STATIC_LIBS__)
	#pragma import reset
#endif
#endif /* macintosh */

#ifndef FREECAST
#define FREECAST (char *)
#endif

/**** Tkapp Object Declaration ****/

staticforward PyTypeObject Tkapp_Type;

typedef struct {
	PyObject_HEAD
	Tcl_Interp *interp;
} TkappObject;

#define Tkapp_Check(v) ((v)->ob_type == &Tkapp_Type)
#define Tkapp_Interp(v) (((TkappObject *) (v))->interp)
#define Tkapp_Result(v) Tcl_GetStringResult(Tkapp_Interp(v))

#define DEBUG_REFCNT(v) (printf("DEBUG: id=%p, refcnt=%i\n", \
(void *) v, ((PyObject *) v)->ob_refcnt))



/**** Error Handling ****/

static PyObject *Tkinter_TclError;
static int quitMainLoop = 0;
static int errorInCmd = 0;
static PyObject *excInCmd;
static PyObject *valInCmd;
static PyObject *trbInCmd;



static PyObject *
Tkinter_Error(PyObject *v)
{
	PyErr_SetString(Tkinter_TclError, Tkapp_Result(v));
	return NULL;
}



/**** Utils ****/

#ifdef WITH_THREAD
#ifndef MS_WINDOWS

/* Millisecond sleep() for Unix platforms. */

static void
Sleep(int milli)
{
	/* XXX Too bad if you don't have select(). */
	struct timeval t;
	t.tv_sec = milli/1000;
	t.tv_usec = (milli%1000) * 1000;
	select(0, (fd_set *)0, (fd_set *)0, (fd_set *)0, &t);
}
#endif /* MS_WINDOWS */
#endif /* WITH_THREAD */


static char *
AsString(PyObject *value, PyObject *tmp)
{
	if (PyString_Check(value))
		return PyString_AsString(value);
	else if (PyUnicode_Check(value)) {
		PyObject *v = PyUnicode_AsUTF8String(value);
		if (v == NULL)
			return NULL;
		if (PyList_Append(tmp, v) != 0) {
			Py_DECREF(v);
			return NULL;
		}
		Py_DECREF(v);
		return PyString_AsString(v);
	}
	else {
		PyObject *v = PyObject_Str(value);
		if (v == NULL)
			return NULL;
		if (PyList_Append(tmp, v) != 0) {
			Py_DECREF(v);
			return NULL;
		}
		Py_DECREF(v);
		return PyString_AsString(v);
	}
}



#define ARGSZ 64

static char *
Merge(PyObject *args)
{
	PyObject *tmp = NULL;
	char *argvStore[ARGSZ];
	char **argv = NULL;
	int fvStore[ARGSZ];
	int *fv = NULL;
	int argc = 0, fvc = 0, i;
	char *res = NULL;

	if (!(tmp = PyList_New(0)))
	    return NULL;

	argv = argvStore;
	fv = fvStore;

	if (args == NULL)
		argc = 0;

	else if (!PyTuple_Check(args)) {
		argc = 1;
		fv[0] = 0;
		if (!(argv[0] = AsString(args, tmp)))
			goto finally;
	}
	else {
		argc = PyTuple_Size(args);

		if (argc > ARGSZ) {
			argv = (char **)ckalloc(argc * sizeof(char *));
			fv = (int *)ckalloc(argc * sizeof(int));
			if (argv == NULL || fv == NULL) {
				PyErr_NoMemory();
				goto finally;
			}
		}

		for (i = 0; i < argc; i++) {
			PyObject *v = PyTuple_GetItem(args, i);
			if (PyTuple_Check(v)) {
				fv[i] = 1;
				if (!(argv[i] = Merge(v)))
					goto finally;
				fvc++;
			}
			else if (v == Py_None) {
				argc = i;
				break;
			}
			else {
				fv[i] = 0;
				if (!(argv[i] = AsString(v, tmp)))
					goto finally;
				fvc++;
			}
		}
	}
	res = Tcl_Merge(argc, argv);
	if (res == NULL)
		PyErr_SetString(Tkinter_TclError, "merge failed");

  finally:
	for (i = 0; i < fvc; i++)
		if (fv[i]) {
			ckfree(argv[i]);
		}
	if (argv != argvStore)
		ckfree(FREECAST argv);
	if (fv != fvStore)
		ckfree(FREECAST fv);

	Py_DECREF(tmp);
	return res;
}



static PyObject *
Split(char *list)
{
	int argc;
	char **argv;
	PyObject *v;

	if (list == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	if (Tcl_SplitList((Tcl_Interp *)NULL, list, &argc, &argv) != TCL_OK) {
		/* Not a list.
		 * Could be a quoted string containing funnies, e.g. {"}.
		 * Return the string itself.
		 */
		return PyString_FromString(list);
	}

	if (argc == 0)
		v = PyString_FromString("");
	else if (argc == 1)
		v = PyString_FromString(argv[0]);
	else if ((v = PyTuple_New(argc)) != NULL) {
		int i;
		PyObject *w;

		for (i = 0; i < argc; i++) {
			if ((w = Split(argv[i])) == NULL) {
				Py_DECREF(v);
				v = NULL;
				break;
			}
			PyTuple_SetItem(v, i, w);
		}
	}
	Tcl_Free(FREECAST argv);
	return v;
}



/**** Tkapp Object ****/

#ifndef WITH_APPINIT
int
Tcl_AppInit(Tcl_Interp *interp)
{
	Tk_Window main;

	main = Tk_MainWindow(interp);
	if (Tcl_Init(interp) == TCL_ERROR) {
		PySys_WriteStderr("Tcl_Init error: %s\n", Tcl_GetStringResult(interp));
		return TCL_ERROR;
	}
	if (Tk_Init(interp) == TCL_ERROR) {
		PySys_WriteStderr("Tk_Init error: %s\n", Tcl_GetStringResult(interp));
		return TCL_ERROR;
	}
	return TCL_OK;
}
#endif /* !WITH_APPINIT */




/* Initialize the Tk application; see the `main' function in
 * `tkMain.c'.
 */

static void EnableEventHook(void); /* Forward */
static void DisableEventHook(void); /* Forward */

static TkappObject *
Tkapp_New(char *screenName, char *baseName, char *className, int interactive)
{
	TkappObject *v;
	char *argv0;
  
	v = PyObject_New(TkappObject, &Tkapp_Type);
	if (v == NULL)
		return NULL;

	v->interp = Tcl_CreateInterp();

#if defined(macintosh)
	/* This seems to be needed */
	ClearMenuBar();
	TkMacInitMenus(v->interp);
#endif
	/* Delete the 'exit' command, which can screw things up */
	Tcl_DeleteCommand(v->interp, "exit");

	if (screenName != NULL)
		Tcl_SetVar2(v->interp, "env", "DISPLAY",
			    screenName, TCL_GLOBAL_ONLY);

	if (interactive)
		Tcl_SetVar(v->interp, "tcl_interactive", "1", TCL_GLOBAL_ONLY);
	else
		Tcl_SetVar(v->interp, "tcl_interactive", "0", TCL_GLOBAL_ONLY);

	/* This is used to get the application class for Tk 4.1 and up */
	argv0 = (char*)ckalloc(strlen(className) + 1);
	if (!argv0) {
		PyErr_NoMemory();
		Py_DECREF(v);
		return NULL;
	}

	strcpy(argv0, className);
	if (isupper((int)(argv0[0])))
		argv0[0] = tolower(argv0[0]);
	Tcl_SetVar(v->interp, "argv0", argv0, TCL_GLOBAL_ONLY);
	ckfree(argv0);

	if (Tcl_AppInit(v->interp) != TCL_OK)
		return (TkappObject *)Tkinter_Error((PyObject *)v);

	EnableEventHook();

	return v;
}



/** Tcl Eval **/

#if TKMAJORMINOR >= 8001
#define USING_OBJECTS
#endif

#ifdef USING_OBJECTS

static Tcl_Obj*
AsObj(PyObject *value)
{
	Tcl_Obj *result;

	if (PyString_Check(value))
		return Tcl_NewStringObj(PyString_AS_STRING(value),
					PyString_GET_SIZE(value));
	else if (PyInt_Check(value))
		return Tcl_NewLongObj(PyInt_AS_LONG(value));
	else if (PyFloat_Check(value))
		return Tcl_NewDoubleObj(PyFloat_AS_DOUBLE(value));
	else if (PyTuple_Check(value)) {
		Tcl_Obj **argv = (Tcl_Obj**)
			ckalloc(PyTuple_Size(value)*sizeof(Tcl_Obj*));
		int i;
		if(!argv)
		  return 0;
		for(i=0;i<PyTuple_Size(value);i++)
		  argv[i] = AsObj(PyTuple_GetItem(value,i));
		result = Tcl_NewListObj(PyTuple_Size(value), argv);
		ckfree(FREECAST argv);
		return result;
	}
	else if (PyUnicode_Check(value)) {
#if TKMAJORMINOR <= 8001
		/* In Tcl 8.1 we must use UTF-8 */
		PyObject* utf8 = PyUnicode_AsUTF8String(value);
		if (!utf8)
			return 0;
		result = Tcl_NewStringObj(PyString_AS_STRING(utf8),
					  PyString_GET_SIZE(utf8));
		Py_DECREF(utf8);
		return result;
#else /* TKMAJORMINOR > 8001 */
		/* In Tcl 8.2 and later, use Tcl_NewUnicodeObj() */
		if (sizeof(Py_UNICODE) != sizeof(Tcl_UniChar)) {
			/* XXX Should really test this at compile time */
			PyErr_SetString(PyExc_SystemError,
				"Py_UNICODE and Tcl_UniChar differ in size");
			return 0;
		}
		return Tcl_NewUnicodeObj(PyUnicode_AS_UNICODE(value),
					 PyUnicode_GET_SIZE(value));
#endif /* TKMAJORMINOR > 8001 */
	}
	else {
		PyObject *v = PyObject_Str(value);
		if (!v)
			return 0;
		result = AsObj(v);
		Py_DECREF(v);
		return result;
	}
}

static PyObject *
Tkapp_Call(PyObject *self, PyObject *args)
{
	Tcl_Obj *objStore[ARGSZ];
	Tcl_Obj **objv = NULL;
	int objc = 0, i;
	PyObject *res = NULL;
	Tcl_Interp *interp = Tkapp_Interp(self);
	/* Could add TCL_EVAL_GLOBAL if wrapped by GlobalCall... */
	int flags = TCL_EVAL_DIRECT;

	objv = objStore;

	if (args == NULL)
		/* do nothing */;

	else if (!PyTuple_Check(args)) {
		objv[0] = AsObj(args);
		if (objv[0] == 0)
			goto finally;
		objc = 1;
		Tcl_IncrRefCount(objv[0]);
	}
	else {
		objc = PyTuple_Size(args);

		if (objc > ARGSZ) {
			objv = (Tcl_Obj **)ckalloc(objc * sizeof(char *));
			if (objv == NULL) {
				PyErr_NoMemory();
				objc = 0;
				goto finally;
			}
		}

		for (i = 0; i < objc; i++) {
			PyObject *v = PyTuple_GetItem(args, i);
			if (v == Py_None) {
				objc = i;
				break;
			}
			objv[i] = AsObj(v);
			if (!objv[i]) {
				/* Reset objc, so it attempts to clear
				   objects only up to i. */
				objc = i;
				goto finally;
			}
			Tcl_IncrRefCount(objv[i]);
		}
	}

	ENTER_TCL

	i = Tcl_EvalObjv(interp, objc, objv, flags);

	ENTER_OVERLAP
	if (i == TCL_ERROR)
		Tkinter_Error(self);
	else {
		/* We could request the object result here, but doing
		   so would confuse applications that expect a string. */
		char *s = Tcl_GetStringResult(interp);
		char *p = s;
		/* If the result contains any bytes with the top bit set,
		   it's UTF-8 and we should decode it to Unicode */
		while (*p != '\0') {
			if (*p & 0x80)
				break;
			p++;
		}
		if (*p == '\0')
			res = PyString_FromStringAndSize(s, (int)(p-s));
		else {
			/* Convert UTF-8 to Unicode string */
			p = strchr(p, '\0');
			res = PyUnicode_DecodeUTF8(s, (int)(p-s), "strict");
			if (res == NULL) {
			    PyErr_Clear();
			    res = PyString_FromStringAndSize(s, (int)(p-s));
			}
		}
	}

	LEAVE_OVERLAP_TCL

  finally:
	for (i = 0; i < objc; i++)
		Tcl_DecrRefCount(objv[i]);
	if (objv != objStore)
		ckfree(FREECAST objv);
	return res;
}

#else /* !USING_OBJECTS */

static PyObject *
Tkapp_Call(PyObject *self, PyObject *args)
{
	/* This is copied from Merge() */
	PyObject *tmp = NULL;
	char *argvStore[ARGSZ];
	char **argv = NULL;
	int fvStore[ARGSZ];
	int *fv = NULL;
	int argc = 0, fvc = 0, i;
	PyObject *res = NULL; /* except this has a different type */
	Tcl_CmdInfo info; /* and this is added */
	Tcl_Interp *interp = Tkapp_Interp(self); /* and this too */

	if (!(tmp = PyList_New(0)))
	    return NULL;

	argv = argvStore;
	fv = fvStore;

	if (args == NULL)
		argc = 0;

	else if (!PyTuple_Check(args)) {
		argc = 1;
		fv[0] = 0;
		if (!(argv[0] = AsString(args, tmp)))
			goto finally;
	}
	else {
		argc = PyTuple_Size(args);

		if (argc > ARGSZ) {
			argv = (char **)ckalloc(argc * sizeof(char *));
			fv = (int *)ckalloc(argc * sizeof(int));
			if (argv == NULL || fv == NULL) {
				PyErr_NoMemory();
				goto finally;
			}
		}

		for (i = 0; i < argc; i++) {
			PyObject *v = PyTuple_GetItem(args, i);
			if (PyTuple_Check(v)) {
				fv[i] = 1;
				if (!(argv[i] = Merge(v)))
					goto finally;
				fvc++;
			}
			else if (v == Py_None) {
				argc = i;
				break;
			}
			else {
				fv[i] = 0;
				if (!(argv[i] = AsString(v, tmp)))
					goto finally;
				fvc++;
			}
		}
	}
	/* End code copied from Merge() */

	/* All this to avoid a call to Tcl_Merge() and the corresponding call
	   to Tcl_SplitList() inside Tcl_Eval()...  It can save a bundle! */
	if (Py_VerboseFlag >= 2) {
		for (i = 0; i < argc; i++)
			PySys_WriteStderr("%s ", argv[i]);
	}
	ENTER_TCL
	info.proc = NULL;
	if (argc < 1 ||
	    !Tcl_GetCommandInfo(interp, argv[0], &info) ||
	    info.proc == NULL)
	{
		char *cmd;
		cmd = Tcl_Merge(argc, argv);
		i = Tcl_Eval(interp, cmd);
		ckfree(cmd);
	}
	else {
		Tcl_ResetResult(interp);
		i = (*info.proc)(info.clientData, interp, argc, argv);
	}
	ENTER_OVERLAP
	if (info.proc == NULL && Py_VerboseFlag >= 2)
		PySys_WriteStderr("... use TclEval ");
	if (i == TCL_ERROR) {
		if (Py_VerboseFlag >= 2)
			PySys_WriteStderr("... error: '%s'\n",
				Tcl_GetStringResult(interp));
		Tkinter_Error(self);
	}
	else {
		if (Py_VerboseFlag >= 2)
			PySys_WriteStderr("-> '%s'\n", Tcl_GetStringResult(interp));
		res = PyString_FromString(Tcl_GetStringResult(interp));
	}
	LEAVE_OVERLAP_TCL

	/* Copied from Merge() again */
  finally:
	for (i = 0; i < fvc; i++)
		if (fv[i]) {
			ckfree(argv[i]);
		}
	if (argv != argvStore)
		ckfree(FREECAST argv);
	if (fv != fvStore)
		ckfree(FREECAST fv);

	Py_DECREF(tmp);
	return res;
}

#endif /* !USING_OBJECTS */

static PyObject *
Tkapp_GlobalCall(PyObject *self, PyObject *args)
{
	/* Could do the same here as for Tkapp_Call(), but this is not used
	   much, so I can't be bothered.  Unfortunately Tcl doesn't export a
	   way for the user to do what all its Global* variants do (save and
	   reset the scope pointer, call the local version, restore the saved
	   scope pointer). */

	char *cmd;
	PyObject *res = NULL;

	cmd  = Merge(args);
	if (cmd) {
		int err;
		ENTER_TCL
		err = Tcl_GlobalEval(Tkapp_Interp(self), cmd);
		ENTER_OVERLAP
		if (err == TCL_ERROR)
			res = Tkinter_Error(self);
		else
			res = PyString_FromString(Tkapp_Result(self));
		LEAVE_OVERLAP_TCL
		ckfree(cmd);
	}

	return res;
}

static PyObject *
Tkapp_Eval(PyObject *self, PyObject *args)
{
	char *script;
	PyObject *res = NULL;
	int err;
  
	if (!PyArg_ParseTuple(args, "s:eval", &script))
		return NULL;

	ENTER_TCL
	err = Tcl_Eval(Tkapp_Interp(self), script);
	ENTER_OVERLAP
	if (err == TCL_ERROR)
		res = Tkinter_Error(self);
	else
		res = PyString_FromString(Tkapp_Result(self));
	LEAVE_OVERLAP_TCL
	return res;
}

static PyObject *
Tkapp_GlobalEval(PyObject *self, PyObject *args)
{
	char *script;
	PyObject *res = NULL;
	int err;

	if (!PyArg_ParseTuple(args, "s:globaleval", &script))
		return NULL;

	ENTER_TCL
	err = Tcl_GlobalEval(Tkapp_Interp(self), script);
	ENTER_OVERLAP
	if (err == TCL_ERROR)
		res = Tkinter_Error(self);
	else
		res = PyString_FromString(Tkapp_Result(self));
	LEAVE_OVERLAP_TCL
	return res;
}

static PyObject *
Tkapp_EvalFile(PyObject *self, PyObject *args)
{
	char *fileName;
	PyObject *res = NULL;
	int err;

	if (!PyArg_ParseTuple(args, "s:evalfile", &fileName))
		return NULL;

	ENTER_TCL
	err = Tcl_EvalFile(Tkapp_Interp(self), fileName);
	ENTER_OVERLAP
	if (err == TCL_ERROR)
		res = Tkinter_Error(self);

	else
		res = PyString_FromString(Tkapp_Result(self));
	LEAVE_OVERLAP_TCL
	return res;
}

static PyObject *
Tkapp_Record(PyObject *self, PyObject *args)
{
	char *script;
	PyObject *res = NULL;
	int err;

	if (!PyArg_ParseTuple(args, "s", &script))
		return NULL;

	ENTER_TCL
	err = Tcl_RecordAndEval(Tkapp_Interp(self), script, TCL_NO_EVAL);
	ENTER_OVERLAP
	if (err == TCL_ERROR)
		res = Tkinter_Error(self);
	else
		res = PyString_FromString(Tkapp_Result(self));
	LEAVE_OVERLAP_TCL
	return res;
}

static PyObject *
Tkapp_AddErrorInfo(PyObject *self, PyObject *args)
{
	char *msg;

	if (!PyArg_ParseTuple(args, "s:adderrorinfo", &msg))
		return NULL;
	ENTER_TCL
	Tcl_AddErrorInfo(Tkapp_Interp(self), msg);
	LEAVE_TCL

	Py_INCREF(Py_None);
	return Py_None;
}



/** Tcl Variable **/

static PyObject *
SetVar(PyObject *self, PyObject *args, int flags)
{
	char *name1, *name2, *ok, *s;
	PyObject *newValue;
	PyObject *tmp;

	tmp = PyList_New(0);
	if (!tmp)
		return NULL;

	if (PyArg_ParseTuple(args, "sO:setvar", &name1, &newValue)) {
		/* XXX Merge? */
		s = AsString(newValue, tmp);
		if (s == NULL)
			return NULL;
		ENTER_TCL
		ok = Tcl_SetVar(Tkapp_Interp(self), name1, s, flags);
		LEAVE_TCL
	}
	else {
		PyErr_Clear();
		if (PyArg_ParseTuple(args, "ssO:setvar",
				     &name1, &name2, &newValue)) {
			s = AsString(newValue, tmp);
			if (s == NULL)
				return NULL;
			ENTER_TCL
			ok = Tcl_SetVar2(Tkapp_Interp(self), name1, name2, 
					 s, flags);
			LEAVE_TCL
		}
		else {
			Py_DECREF(tmp);
			return NULL;
		}
	}
	Py_DECREF(tmp);

	if (!ok)
		return Tkinter_Error(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Tkapp_SetVar(PyObject *self, PyObject *args)
{
	return SetVar(self, args, TCL_LEAVE_ERR_MSG);
}

static PyObject *
Tkapp_GlobalSetVar(PyObject *self, PyObject *args)
{
	return SetVar(self, args, TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
}



static PyObject *
GetVar(PyObject *self, PyObject *args, int flags)
{
	char *name1, *name2=NULL, *s;
	PyObject *res = NULL;

	if (!PyArg_ParseTuple(args, "s|s:getvar", &name1, &name2))
		return NULL;
	ENTER_TCL
	if (name2 == NULL)
		s = Tcl_GetVar(Tkapp_Interp(self), name1, flags);

	else
		s = Tcl_GetVar2(Tkapp_Interp(self), name1, name2, flags);
	ENTER_OVERLAP

	if (s == NULL)
		res = Tkinter_Error(self);
	else
		res = PyString_FromString(s);
	LEAVE_OVERLAP_TCL
	return res;
}

static PyObject *
Tkapp_GetVar(PyObject *self, PyObject *args)
{
	return GetVar(self, args, TCL_LEAVE_ERR_MSG);
}

static PyObject *
Tkapp_GlobalGetVar(PyObject *self, PyObject *args)
{
	return GetVar(self, args, TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
}



static PyObject *
UnsetVar(PyObject *self, PyObject *args, int flags)
{
	char *name1, *name2=NULL;
	PyObject *res = NULL;
	int code;

	if (!PyArg_ParseTuple(args, "s|s:unsetvar", &name1, &name2))
		return NULL;
	ENTER_TCL
	if (name2 == NULL)
		code = Tcl_UnsetVar(Tkapp_Interp(self), name1, flags);

	else
		code = Tcl_UnsetVar2(Tkapp_Interp(self), name1, name2, flags);
	ENTER_OVERLAP

	if (code == TCL_ERROR)
		res = Tkinter_Error(self);
	else {
		Py_INCREF(Py_None);
		res = Py_None;
	}
	LEAVE_OVERLAP_TCL
	return res;
}

static PyObject *
Tkapp_UnsetVar(PyObject *self, PyObject *args)
{
	return UnsetVar(self, args, TCL_LEAVE_ERR_MSG);
}

static PyObject *
Tkapp_GlobalUnsetVar(PyObject *self, PyObject *args)
{
	return UnsetVar(self, args, TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
}



/** Tcl to Python **/

static PyObject *
Tkapp_GetInt(PyObject *self, PyObject *args)
{
	char *s;
	int v;

	if (!PyArg_ParseTuple(args, "s:getint", &s))
		return NULL;
	if (Tcl_GetInt(Tkapp_Interp(self), s, &v) == TCL_ERROR)
		return Tkinter_Error(self);
	return Py_BuildValue("i", v);
}

static PyObject *
Tkapp_GetDouble(PyObject *self, PyObject *args)
{
	char *s;
	double v;

	if (!PyArg_ParseTuple(args, "s:getdouble", &s))
		return NULL;
	if (Tcl_GetDouble(Tkapp_Interp(self), s, &v) == TCL_ERROR)
		return Tkinter_Error(self);
	return Py_BuildValue("d", v);
}

static PyObject *
Tkapp_GetBoolean(PyObject *self, PyObject *args)
{
	char *s;
	int v;

	if (!PyArg_ParseTuple(args, "s:getboolean", &s))
		return NULL;
	if (Tcl_GetBoolean(Tkapp_Interp(self), s, &v) == TCL_ERROR)
		return Tkinter_Error(self);
	return Py_BuildValue("i", v);
}

static PyObject *
Tkapp_ExprString(PyObject *self, PyObject *args)
{
	char *s;
	PyObject *res = NULL;
	int retval;

	if (!PyArg_ParseTuple(args, "s:exprstring", &s))
		return NULL;
	ENTER_TCL
	retval = Tcl_ExprString(Tkapp_Interp(self), s);
	ENTER_OVERLAP
	if (retval == TCL_ERROR)
		res = Tkinter_Error(self);
	else
		res = Py_BuildValue("s", Tkapp_Result(self));
	LEAVE_OVERLAP_TCL
	return res;
}

static PyObject *
Tkapp_ExprLong(PyObject *self, PyObject *args)
{
	char *s;
	PyObject *res = NULL;
	int retval;
	long v;

	if (!PyArg_ParseTuple(args, "s:exprlong", &s))
		return NULL;
	ENTER_TCL
	retval = Tcl_ExprLong(Tkapp_Interp(self), s, &v);
	ENTER_OVERLAP
	if (retval == TCL_ERROR)
		res = Tkinter_Error(self);
	else
		res = Py_BuildValue("l", v);
	LEAVE_OVERLAP_TCL
	return res;
}

static PyObject *
Tkapp_ExprDouble(PyObject *self, PyObject *args)
{
	char *s;
	PyObject *res = NULL;
	double v;
	int retval;

	if (!PyArg_ParseTuple(args, "s:exprdouble", &s))
		return NULL;
	PyFPE_START_PROTECT("Tkapp_ExprDouble", return 0)
	ENTER_TCL
	retval = Tcl_ExprDouble(Tkapp_Interp(self), s, &v);
	ENTER_OVERLAP
	PyFPE_END_PROTECT(retval)
	if (retval == TCL_ERROR)
		res = Tkinter_Error(self);
	else
		res = Py_BuildValue("d", v);
	LEAVE_OVERLAP_TCL
	return res;
}

static PyObject *
Tkapp_ExprBoolean(PyObject *self, PyObject *args)
{
	char *s;
	PyObject *res = NULL;
	int retval;
	int v;

	if (!PyArg_ParseTuple(args, "s:exprboolean", &s))
		return NULL;
	ENTER_TCL
	retval = Tcl_ExprBoolean(Tkapp_Interp(self), s, &v);
	ENTER_OVERLAP
	if (retval == TCL_ERROR)
		res = Tkinter_Error(self);
	else
		res = Py_BuildValue("i", v);
	LEAVE_OVERLAP_TCL
	return res;
}



static PyObject *
Tkapp_SplitList(PyObject *self, PyObject *args)
{
	char *list;
	int argc;
	char **argv;
	PyObject *v;
	int i;

	if (!PyArg_ParseTuple(args, "s:splitlist", &list))
		return NULL;

	if (Tcl_SplitList(Tkapp_Interp(self), list, &argc, &argv) == TCL_ERROR)
		return Tkinter_Error(self);

	if (!(v = PyTuple_New(argc)))
		return NULL;
	
	for (i = 0; i < argc; i++) {
		PyObject *s = PyString_FromString(argv[i]);
		if (!s || PyTuple_SetItem(v, i, s)) {
			Py_DECREF(v);
			v = NULL;
			goto finally;
		}
	}

  finally:
	ckfree(FREECAST argv);
	return v;
}

static PyObject *
Tkapp_Split(PyObject *self, PyObject *args)
{
	char *list;

	if (!PyArg_ParseTuple(args, "s:split", &list))
		return NULL;
	return Split(list);
}

static PyObject *
Tkapp_Merge(PyObject *self, PyObject *args)
{
	char *s = Merge(args);
	PyObject *res = NULL;

	if (s) {
		res = PyString_FromString(s);
		ckfree(s);
	}

	return res;
}



/** Tcl Command **/

/* Client data struct */
typedef struct {
	PyObject *self;
	PyObject *func;
} PythonCmd_ClientData;

static int
PythonCmd_Error(Tcl_Interp *interp)
{
	errorInCmd = 1;
	PyErr_Fetch(&excInCmd, &valInCmd, &trbInCmd);
	LEAVE_PYTHON
	return TCL_ERROR;
}

/* This is the Tcl command that acts as a wrapper for Python
 * function or method.
 */
static int
PythonCmd(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
	PythonCmd_ClientData *data = (PythonCmd_ClientData *)clientData;
	PyObject *self, *func, *arg, *res, *tmp;
	int i, rv;
	char *s;

	ENTER_PYTHON

	/* TBD: no error checking here since we know, via the
	 * Tkapp_CreateCommand() that the client data is a two-tuple
	 */
	self = data->self;
	func = data->func;

	/* Create argument list (argv1, ..., argvN) */
	if (!(arg = PyTuple_New(argc - 1)))
		return PythonCmd_Error(interp);

	for (i = 0; i < (argc - 1); i++) {
		PyObject *s = PyString_FromString(argv[i + 1]);
		if (!s || PyTuple_SetItem(arg, i, s)) {
			Py_DECREF(arg);
			return PythonCmd_Error(interp);
		}
	}
	res = PyEval_CallObject(func, arg);
	Py_DECREF(arg);

	if (res == NULL)
		return PythonCmd_Error(interp);

	if (!(tmp = PyList_New(0))) {
		Py_DECREF(res);
		return PythonCmd_Error(interp);
	}

	s = AsString(res, tmp);
	if (s == NULL) {
		rv = PythonCmd_Error(interp);
	}
	else {
		Tcl_SetResult(Tkapp_Interp(self), s, TCL_VOLATILE);
		rv = TCL_OK;
	}

	Py_DECREF(res);
	Py_DECREF(tmp);

	LEAVE_PYTHON

	return rv;
}

static void
PythonCmdDelete(ClientData clientData)
{
	PythonCmd_ClientData *data = (PythonCmd_ClientData *)clientData;

	ENTER_PYTHON
	Py_XDECREF(data->self);
	Py_XDECREF(data->func);
	PyMem_DEL(data);
	LEAVE_PYTHON
}



static PyObject *
Tkapp_CreateCommand(PyObject *self, PyObject *args)
{
	PythonCmd_ClientData *data;
	char *cmdName;
	PyObject *func;
	Tcl_Command err;

	if (!PyArg_ParseTuple(args, "sO:createcommand", &cmdName, &func))
		return NULL;
	if (!PyCallable_Check(func)) {
		PyErr_SetString(PyExc_TypeError, "command not callable");
		return NULL;
	}

	data = PyMem_NEW(PythonCmd_ClientData, 1);
	if (!data)
		return NULL;
	Py_XINCREF(self);
	Py_XINCREF(func);
	data->self = self;
	data->func = func;

	ENTER_TCL
	err = Tcl_CreateCommand(Tkapp_Interp(self), cmdName, PythonCmd,
				(ClientData)data, PythonCmdDelete);
	LEAVE_TCL
	if (err == NULL) {
		PyErr_SetString(Tkinter_TclError, "can't create Tcl command");
		PyMem_DEL(data);
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}



static PyObject *
Tkapp_DeleteCommand(PyObject *self, PyObject *args)
{
	char *cmdName;
	int err;

	if (!PyArg_ParseTuple(args, "s:deletecommand", &cmdName))
		return NULL;
	ENTER_TCL
	err = Tcl_DeleteCommand(Tkapp_Interp(self), cmdName);
	LEAVE_TCL
	if (err == -1) {
		PyErr_SetString(Tkinter_TclError, "can't delete Tcl command");
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}



#ifdef HAVE_CREATEFILEHANDLER
/** File Handler **/

typedef struct _fhcdata {
	PyObject *func;
	PyObject *file;
	int id;
	struct _fhcdata *next;
} FileHandler_ClientData;

static FileHandler_ClientData *HeadFHCD;

static FileHandler_ClientData *
NewFHCD(PyObject *func, PyObject *file, int id)
{
	FileHandler_ClientData *p;
	p = PyMem_NEW(FileHandler_ClientData, 1);
	if (p != NULL) {
		Py_XINCREF(func);
		Py_XINCREF(file);
		p->func = func;
		p->file = file;
		p->id = id;
		p->next = HeadFHCD;
		HeadFHCD = p;
	}
	return p;
}

static void
DeleteFHCD(int id)
{
	FileHandler_ClientData *p, **pp;
	
	pp = &HeadFHCD; 
	while ((p = *pp) != NULL) {
		if (p->id == id) {
			*pp = p->next;
			Py_XDECREF(p->func);
			Py_XDECREF(p->file);
			PyMem_DEL(p);
		}
		else
			pp = &p->next;
	}
}

static void
FileHandler(ClientData clientData, int mask)
{
	FileHandler_ClientData *data = (FileHandler_ClientData *)clientData;
	PyObject *func, *file, *arg, *res;

	ENTER_PYTHON
	func = data->func;
	file = data->file;

	arg = Py_BuildValue("(Oi)", file, (long) mask);
	res = PyEval_CallObject(func, arg);
	Py_DECREF(arg);

	if (res == NULL) {
		errorInCmd = 1;
		PyErr_Fetch(&excInCmd, &valInCmd, &trbInCmd);
	}
	Py_XDECREF(res);
	LEAVE_PYTHON
}

static PyObject *
Tkapp_CreateFileHandler(PyObject *self, PyObject *args)
     /* args is (file, mask, func) */
{
	FileHandler_ClientData *data;
	PyObject *file, *func;
	int mask, tfile;

	if (!PyArg_ParseTuple(args, "OiO:createfilehandler",
			      &file, &mask, &func))
		return NULL;
	tfile = PyObject_AsFileDescriptor(file);
	if (tfile < 0)
		return NULL;
	if (!PyCallable_Check(func)) {
		PyErr_SetString(PyExc_TypeError, "bad argument list");
		return NULL;
	}

	data = NewFHCD(func, file, tfile);
	if (data == NULL)
		return NULL;

	/* Ought to check for null Tcl_File object... */
	ENTER_TCL
	Tcl_CreateFileHandler(tfile, mask, FileHandler, (ClientData) data);
	LEAVE_TCL
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Tkapp_DeleteFileHandler(PyObject *self, PyObject *args)
{
	PyObject *file;
	int tfile;
  
	if (!PyArg_ParseTuple(args, "O:deletefilehandler", &file))
		return NULL;
	tfile = PyObject_AsFileDescriptor(file);
	if (tfile < 0)
		return NULL;

	DeleteFHCD(tfile);

	/* Ought to check for null Tcl_File object... */
	ENTER_TCL
	Tcl_DeleteFileHandler(tfile);
	LEAVE_TCL
	Py_INCREF(Py_None);
	return Py_None;
}
#endif /* HAVE_CREATEFILEHANDLER */


/**** Tktt Object (timer token) ****/

staticforward PyTypeObject Tktt_Type;

typedef struct {
	PyObject_HEAD
	Tcl_TimerToken token;
	PyObject *func;
} TkttObject;

static PyObject *
Tktt_DeleteTimerHandler(PyObject *self, PyObject *args)
{
	TkttObject *v = (TkttObject *)self;
	PyObject *func = v->func;

	if (!PyArg_ParseTuple(args, ":deletetimerhandler"))
		return NULL;
	if (v->token != NULL) {
		Tcl_DeleteTimerHandler(v->token);
		v->token = NULL;
	}
	if (func != NULL) {
		v->func = NULL;
		Py_DECREF(func);
		Py_DECREF(v); /* See Tktt_New() */
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef Tktt_methods[] =
{
	{"deletetimerhandler", Tktt_DeleteTimerHandler, 1},
	{NULL, NULL}
};

static TkttObject *
Tktt_New(PyObject *func)
{
	TkttObject *v;
  
	v = PyObject_New(TkttObject, &Tktt_Type);
	if (v == NULL)
		return NULL;

	Py_INCREF(func);
	v->token = NULL;
	v->func = func;

	/* Extra reference, deleted when called or when handler is deleted */
	Py_INCREF(v);
	return v;
}

static void
Tktt_Dealloc(PyObject *self)
{
	TkttObject *v = (TkttObject *)self;
	PyObject *func = v->func;

	Py_XDECREF(func);

	PyObject_Del(self);
}

static PyObject *
Tktt_Repr(PyObject *self)
{
	TkttObject *v = (TkttObject *)self;
	char buf[100];

	sprintf(buf, "<tktimertoken at %p%s>", v,
		v->func == NULL ? ", handler deleted" : "");
	return PyString_FromString(buf);
}

static PyObject *
Tktt_GetAttr(PyObject *self, char *name)
{
	return Py_FindMethod(Tktt_methods, self, name);
}

static PyTypeObject Tktt_Type =
{
	PyObject_HEAD_INIT(NULL)
	0,				     /*ob_size */
	"tktimertoken",			     /*tp_name */
	sizeof(TkttObject),		     /*tp_basicsize */
	0,				     /*tp_itemsize */
	Tktt_Dealloc,			     /*tp_dealloc */
	0,				     /*tp_print */
	Tktt_GetAttr,			     /*tp_getattr */
	0,				     /*tp_setattr */
	0,				     /*tp_compare */
	Tktt_Repr,			     /*tp_repr */
	0,				     /*tp_as_number */
	0,				     /*tp_as_sequence */
	0,				     /*tp_as_mapping */
	0,				     /*tp_hash */
};



/** Timer Handler **/

static void
TimerHandler(ClientData clientData)
{
	TkttObject *v = (TkttObject *)clientData;
	PyObject *func = v->func;
	PyObject *res;

	if (func == NULL)
		return;

	v->func = NULL;

	ENTER_PYTHON

	res  = PyEval_CallObject(func, NULL);
	Py_DECREF(func);
	Py_DECREF(v); /* See Tktt_New() */

	if (res == NULL) {
		errorInCmd = 1;
		PyErr_Fetch(&excInCmd, &valInCmd, &trbInCmd);
	}
	else
		Py_DECREF(res);

	LEAVE_PYTHON
}

static PyObject *
Tkapp_CreateTimerHandler(PyObject *self, PyObject *args)
{
	int milliseconds;
	PyObject *func;
	TkttObject *v;

	if (!PyArg_ParseTuple(args, "iO:createtimerhandler",
			      &milliseconds, &func))
		return NULL;
	if (!PyCallable_Check(func)) {
		PyErr_SetString(PyExc_TypeError, "bad argument list");
		return NULL;
	}
	v = Tktt_New(func);
	v->token = Tcl_CreateTimerHandler(milliseconds, TimerHandler,
					  (ClientData)v);

	return (PyObject *) v;
}


/** Event Loop **/

static PyObject *
Tkapp_MainLoop(PyObject *self, PyObject *args)
{
	int threshold = 0;
#ifdef WITH_THREAD
	PyThreadState *tstate = PyThreadState_Get();
#endif

	if (!PyArg_ParseTuple(args, "|i:mainloop", &threshold))
		return NULL;

	quitMainLoop = 0;
	while (Tk_GetNumMainWindows() > threshold &&
	       !quitMainLoop &&
	       !errorInCmd)
	{
		int result;

#ifdef WITH_THREAD
		Py_BEGIN_ALLOW_THREADS
		PyThread_acquire_lock(tcl_lock, 1);
		tcl_tstate = tstate;
		result = Tcl_DoOneEvent(TCL_DONT_WAIT);
		tcl_tstate = NULL;
		PyThread_release_lock(tcl_lock);
		if (result == 0)
			Sleep(20);
		Py_END_ALLOW_THREADS
#else
		result = Tcl_DoOneEvent(0);
#endif

		if (PyErr_CheckSignals() != 0)
			return NULL;
		if (result < 0)
			break;
	}
	quitMainLoop = 0;

	if (errorInCmd) {
		errorInCmd = 0;
		PyErr_Restore(excInCmd, valInCmd, trbInCmd);
		excInCmd = valInCmd = trbInCmd = NULL;
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Tkapp_DoOneEvent(PyObject *self, PyObject *args)
{
	int flags = 0;
	int rv;

	if (!PyArg_ParseTuple(args, "|i:dooneevent", &flags))
		return NULL;

	ENTER_TCL
	rv = Tcl_DoOneEvent(flags);
	LEAVE_TCL
	return Py_BuildValue("i", rv);
}

static PyObject *
Tkapp_Quit(PyObject *self, PyObject *args)
{

	if (!PyArg_ParseTuple(args, ":quit"))
		return NULL;

	quitMainLoop = 1;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Tkapp_InterpAddr(PyObject *self, PyObject *args)
{

	if (!PyArg_ParseTuple(args, ":interpaddr"))
		return NULL;

	return PyInt_FromLong((long)Tkapp_Interp(self));
}



/**** Tkapp Method List ****/

static PyMethodDef Tkapp_methods[] =
{
	{"call", 	       Tkapp_Call, 0},
	{"globalcall", 	       Tkapp_GlobalCall, 0},
	{"eval", 	       Tkapp_Eval, 1},
	{"globaleval", 	       Tkapp_GlobalEval, 1},
	{"evalfile", 	       Tkapp_EvalFile, 1},
	{"record", 	       Tkapp_Record, 1},
	{"adderrorinfo",       Tkapp_AddErrorInfo, 1},
	{"setvar", 	       Tkapp_SetVar, 1},
	{"globalsetvar",       Tkapp_GlobalSetVar, 1},
	{"getvar", 	       Tkapp_GetVar, 1},
	{"globalgetvar",       Tkapp_GlobalGetVar, 1},
	{"unsetvar", 	       Tkapp_UnsetVar, 1},
	{"globalunsetvar",     Tkapp_GlobalUnsetVar, 1},
	{"getint", 	       Tkapp_GetInt, 1},
	{"getdouble", 	       Tkapp_GetDouble, 1},
	{"getboolean", 	       Tkapp_GetBoolean, 1},
	{"exprstring", 	       Tkapp_ExprString, 1},
	{"exprlong", 	       Tkapp_ExprLong, 1},
	{"exprdouble", 	       Tkapp_ExprDouble, 1},
	{"exprboolean",        Tkapp_ExprBoolean, 1},
	{"splitlist", 	       Tkapp_SplitList, 1},
	{"split", 	       Tkapp_Split, 1},
	{"merge", 	       Tkapp_Merge, 0},
	{"createcommand",      Tkapp_CreateCommand, 1},
	{"deletecommand",      Tkapp_DeleteCommand, 1},
#ifdef HAVE_CREATEFILEHANDLER
	{"createfilehandler",  Tkapp_CreateFileHandler, 1},
	{"deletefilehandler",  Tkapp_DeleteFileHandler, 1},
#endif
	{"createtimerhandler", Tkapp_CreateTimerHandler, 1},
	{"mainloop", 	       Tkapp_MainLoop, 1},
	{"dooneevent", 	       Tkapp_DoOneEvent, 1},
	{"quit", 	       Tkapp_Quit, 1},
	{"interpaddr",         Tkapp_InterpAddr, 1},
	{NULL, 		       NULL}
};



/**** Tkapp Type Methods ****/

static void
Tkapp_Dealloc(PyObject *self)
{
	ENTER_TCL
	Tcl_DeleteInterp(Tkapp_Interp(self));
	LEAVE_TCL
	PyObject_Del(self);
	DisableEventHook();
}

static PyObject *
Tkapp_GetAttr(PyObject *self, char *name)
{
	return Py_FindMethod(Tkapp_methods, self, name);
}

static PyTypeObject Tkapp_Type =
{
	PyObject_HEAD_INIT(NULL)
	0,				     /*ob_size */
	"tkapp",			     /*tp_name */
	sizeof(TkappObject),		     /*tp_basicsize */
	0,				     /*tp_itemsize */
	Tkapp_Dealloc,			     /*tp_dealloc */
	0,				     /*tp_print */
	Tkapp_GetAttr,			     /*tp_getattr */
	0,				     /*tp_setattr */
	0,				     /*tp_compare */
	0,				     /*tp_repr */
	0,				     /*tp_as_number */
	0,				     /*tp_as_sequence */
	0,				     /*tp_as_mapping */
	0,				     /*tp_hash */
};



/**** Tkinter Module ****/

typedef struct {
	PyObject* tuple;
	int size; /* current size */
	int maxsize; /* allocated size */
} FlattenContext;

static int
_bump(FlattenContext* context, int size)
{
	/* expand tuple to hold (at least) size new items.
	   return true if successful, false if an exception was raised */

	int maxsize = context->maxsize * 2;

	if (maxsize < context->size + size)
		maxsize = context->size + size;

	context->maxsize = maxsize;

	return _PyTuple_Resize(&context->tuple, maxsize, 0) >= 0;
}

static int
_flatten1(FlattenContext* context, PyObject* item, int depth)
{
	/* add tuple or list to argument tuple (recursively) */

	int i, size;

	if (depth > 1000) {
		PyErr_SetString(PyExc_ValueError,
				"nesting too deep in _flatten");
		return 0;
	} else if (PyList_Check(item)) {
		size = PyList_GET_SIZE(item);
		/* preallocate (assume no nesting) */
		if (context->size + size > context->maxsize &&
		    !_bump(context, size))
			return 0;
		/* copy items to output tuple */
		for (i = 0; i < size; i++) {
			PyObject *o = PyList_GET_ITEM(item, i);
			if (PyList_Check(o) || PyTuple_Check(o)) {
				if (!_flatten1(context, o, depth + 1))
					return 0;
			} else if (o != Py_None) {
				if (context->size + 1 > context->maxsize &&
				    !_bump(context, 1))
					return 0;
				Py_INCREF(o);
				PyTuple_SET_ITEM(context->tuple,
						 context->size++, o);
			}
		}
	} else if (PyTuple_Check(item)) {
		/* same, for tuples */
		size = PyTuple_GET_SIZE(item);
		if (context->size + size > context->maxsize &&
		    !_bump(context, size))
			return 0;
		for (i = 0; i < size; i++) {
			PyObject *o = PyTuple_GET_ITEM(item, i);
			if (PyList_Check(o) || PyTuple_Check(o)) {
				if (!_flatten1(context, o, depth + 1))
					return 0;
			} else if (o != Py_None) {
				if (context->size + 1 > context->maxsize &&
				    !_bump(context, 1))
					return 0;
				Py_INCREF(o);
				PyTuple_SET_ITEM(context->tuple,
						 context->size++, o);
			}
		}
	} else {
		PyErr_SetString(PyExc_TypeError, "argument must be sequence");
		return 0;
	}
	return 1;
}

static PyObject *
Tkinter_Flatten(PyObject* self, PyObject* args)
{
	FlattenContext context;
	PyObject* item;

	if (!PyArg_ParseTuple(args, "O:_flatten", &item))
		return NULL;

	context.maxsize = PySequence_Size(item);
	if (context.maxsize <= 0)
		return PyTuple_New(0);
	
	context.tuple = PyTuple_New(context.maxsize);
	if (!context.tuple)
		return NULL;
	
	context.size = 0;

	if (!_flatten1(&context, item,0))
		return NULL;

	if (_PyTuple_Resize(&context.tuple, context.size, 0))
		return NULL;

	return context.tuple;
}

static PyObject *
Tkinter_Create(PyObject *self, PyObject *args)
{
	char *screenName = NULL;
	char *baseName = NULL;
	char *className = NULL;
	int interactive = 0;

	baseName = strrchr(Py_GetProgramName(), '/');
	if (baseName != NULL)
		baseName++;
	else
		baseName = Py_GetProgramName();
	className = "Tk";
  
	if (!PyArg_ParseTuple(args, "|zssi:create",
			      &screenName, &baseName, &className,
			      &interactive))
		return NULL;

	return (PyObject *) Tkapp_New(screenName, baseName, className, 
				      interactive);
}

static PyMethodDef moduleMethods[] =
{
	{"_flatten",           Tkinter_Flatten, 1},
	{"create",             Tkinter_Create, 1},
#ifdef HAVE_CREATEFILEHANDLER
	{"createfilehandler",  Tkapp_CreateFileHandler, 1},
	{"deletefilehandler",  Tkapp_DeleteFileHandler, 1},
#endif
	{"createtimerhandler", Tkapp_CreateTimerHandler, 1},
	{"mainloop",           Tkapp_MainLoop, 1},
	{"dooneevent",         Tkapp_DoOneEvent, 1},
	{"quit",               Tkapp_Quit, 1},
	{NULL,                 NULL}
};

#ifdef WAIT_FOR_STDIN

static int stdin_ready = 0;

#ifndef MS_WINDOWS
static void
MyFileProc(void *clientData, int mask)
{
	stdin_ready = 1;
}
#endif

static PyThreadState *event_tstate = NULL;

static int
EventHook(void)
{
#ifndef MS_WINDOWS
	int tfile;
#endif
#ifdef WITH_THREAD
	PyEval_RestoreThread(event_tstate);
#endif
	stdin_ready = 0;
	errorInCmd = 0;
#ifndef MS_WINDOWS
	tfile = fileno(stdin);
	Tcl_CreateFileHandler(tfile, TCL_READABLE, MyFileProc, NULL);
#endif
	while (!errorInCmd && !stdin_ready) {
		int result;
#ifdef MS_WINDOWS
		if (_kbhit()) {
			stdin_ready = 1;
			break;
		}
#endif
#if defined(WITH_THREAD) || defined(MS_WINDOWS)
		Py_BEGIN_ALLOW_THREADS
		PyThread_acquire_lock(tcl_lock, 1);
		tcl_tstate = event_tstate;

		result = Tcl_DoOneEvent(TCL_DONT_WAIT);

		tcl_tstate = NULL;
		PyThread_release_lock(tcl_lock);
		if (result == 0)
			Sleep(20);
		Py_END_ALLOW_THREADS
#else
		result = Tcl_DoOneEvent(0);
#endif

		if (result < 0)
			break;
	}
#ifndef MS_WINDOWS
	Tcl_DeleteFileHandler(tfile);
#endif
	if (errorInCmd) {
		errorInCmd = 0;
		PyErr_Restore(excInCmd, valInCmd, trbInCmd);
		excInCmd = valInCmd = trbInCmd = NULL;
		PyErr_Print();
	}
#ifdef WITH_THREAD
	PyEval_SaveThread();
#endif
	return 0;
}

#endif

static void
EnableEventHook(void)
{
#ifdef WAIT_FOR_STDIN
	if (PyOS_InputHook == NULL) {
#ifdef WITH_THREAD
		event_tstate = PyThreadState_Get();
#endif
		PyOS_InputHook = EventHook;
	}
#endif
}

static void
DisableEventHook(void)
{
#ifdef WAIT_FOR_STDIN
	if (Tk_GetNumMainWindows() == 0 && PyOS_InputHook == EventHook) {
		PyOS_InputHook = NULL;
	}
#endif
}


/* all errors will be checked in one fell swoop in init_tkinter() */
static void
ins_long(PyObject *d, char *name, long val)
{
	PyObject *v = PyInt_FromLong(val);
	if (v) {
		PyDict_SetItemString(d, name, v);
		Py_DECREF(v);
	}
}
static void
ins_string(PyObject *d, char *name, char *val)
{
	PyObject *v = PyString_FromString(val);
	if (v) {
		PyDict_SetItemString(d, name, v);
		Py_DECREF(v);
	}
}


DL_EXPORT(void)
init_tkinter(void)
{
	PyObject *m, *d;

	Tkapp_Type.ob_type = &PyType_Type;

#ifdef WITH_THREAD
	tcl_lock = PyThread_allocate_lock();
#endif

	m = Py_InitModule("_tkinter", moduleMethods);

	d = PyModule_GetDict(m);
	Tkinter_TclError = Py_BuildValue("s", "TclError");
	PyDict_SetItemString(d, "TclError", Tkinter_TclError);

	ins_long(d, "READABLE", TCL_READABLE);
	ins_long(d, "WRITABLE", TCL_WRITABLE);
	ins_long(d, "EXCEPTION", TCL_EXCEPTION);
	ins_long(d, "WINDOW_EVENTS", TCL_WINDOW_EVENTS);
	ins_long(d, "FILE_EVENTS", TCL_FILE_EVENTS);
	ins_long(d, "TIMER_EVENTS", TCL_TIMER_EVENTS);
	ins_long(d, "IDLE_EVENTS", TCL_IDLE_EVENTS);
	ins_long(d, "ALL_EVENTS", TCL_ALL_EVENTS);
	ins_long(d, "DONT_WAIT", TCL_DONT_WAIT);
	ins_string(d, "TK_VERSION", TK_VERSION);
	ins_string(d, "TCL_VERSION", TCL_VERSION);

	PyDict_SetItemString(d, "TkappType", (PyObject *)&Tkapp_Type);

	Tktt_Type.ob_type = &PyType_Type;
	PyDict_SetItemString(d, "TkttType", (PyObject *)&Tktt_Type);

	/* This helps the dynamic loader; in Unicode aware Tcl versions
	   it also helps Tcl find its encodings. */
	Tcl_FindExecutable(Py_GetProgramName());

	if (PyErr_Occurred())
		return;

#if 0
	/* This was not a good idea; through <Destroy> bindings,
	   Tcl_Finalize() may invoke Python code but at that point the
	   interpreter and thread state have already been destroyed! */
	Py_AtExit(Tcl_Finalize);
#endif

#ifdef macintosh
	/*
	** Part of this code is stolen from MacintoshInit in tkMacAppInit.
	** Most of the initializations in that routine (toolbox init calls and
	** such) have already been done for us, so we only need these.
	*/
	tcl_macQdPtr = &qd;

	Tcl_MacSetEventProc(PyMacConvertEvent);
#if GENERATINGCFM
	mac_addlibresources();
#endif /* GENERATINGCFM */
#endif /* macintosh */
}



#ifdef macintosh

/*
** Anyone who embeds Tcl/Tk on the Mac must define panic().
*/

void
panic(char * format, ...)
{
	va_list varg;
	
	va_start(varg, format);
	
	vfprintf(stderr, format, varg);
	(void) fflush(stderr);
	
	va_end(varg);

	Py_FatalError("Tcl/Tk panic");
}

/*
** Pass events to SIOUX before passing them to Tk.
*/

static int
PyMacConvertEvent(EventRecord *eventPtr)
{
	WindowPtr frontwin;
	/*
	** Sioux eats too many events, so we don't pass it everything.  We
	** always pass update events to Sioux, and we only pass other events if
	** the Sioux window is frontmost. This means that Tk menus don't work
	** in that case, but at least we can scroll the sioux window.
	** Note that the SIOUXIsAppWindow() routine we use here is not really
	** part of the external interface of Sioux...
	*/
	frontwin = FrontWindow();
	if ( eventPtr->what == updateEvt || SIOUXIsAppWindow(frontwin) ) {
		if (SIOUXHandleOneEvent(eventPtr))
			return 0; /* Nothing happened to the Tcl event queue */
	}
	return TkMacConvertEvent(eventPtr);
}

#if GENERATINGCFM

/*
** Additional Mac specific code for dealing with shared libraries.
*/

#include <Resources.h>
#include <CodeFragments.h>

static int loaded_from_shlib = 0;
static FSSpec library_fss;

/*
** If this module is dynamically loaded the following routine should
** be the init routine. It takes care of adding the shared library to
** the resource-file chain, so that the tk routines can find their
** resources.
*/
OSErr pascal
init_tkinter_shlib(CFragInitBlockPtr data)
{
	__initialize();
	if ( data == nil ) return noErr;
	if ( data->fragLocator.where == kDataForkCFragLocator ) {
		library_fss = *data->fragLocator.u.onDisk.fileSpec;
		loaded_from_shlib = 1;
	} else if ( data->fragLocator.where == kResourceCFragLocator ) {
		library_fss = *data->fragLocator.u.inSegs.fileSpec;
		loaded_from_shlib = 1;
	}
	return noErr;
}

/*
** Insert the library resources into the search path. Put them after
** the resources from the application. Again, we ignore errors.
*/
static
mac_addlibresources(void)
{
	if ( !loaded_from_shlib ) 
		return;
	(void)FSpOpenResFile(&library_fss, fsRdPerm);
}

#endif /* GENERATINGCFM */
#endif /* macintosh */
