/***********************************************************
Copyright (C) 1994 Steen Lumholt.
Copyright 1994-1995 by Stichting Mathematisch Centrum, Amsterdam,
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

/* _tkinter.c -- Interface to libtk.a and libtcl.a. */

/* TCL/TK VERSION INFO:

   Unix:
	This should work with any version from Tcl 4.0 / Tck 7.4.
	Do not use older versions.

   Mac and Windows:
	Use Tcl 4.1p1 / Tk 7.5p1 or possibly newer.
	It does not seem to work reliably with the original 4.1/7.5
	release.  (4.0/7.4 were never released for these platforms.)
*/

#include "Python.h"
#include <ctype.h>

#ifdef macintosh
#define MAC_TCL
#include "myselect.h"
#endif

#include <tcl.h>
#include <tk.h>

extern char *Py_GetProgramName ();

/* Internal declarations from tkInt.h.  */
#if (TK_MAJOR_VERSION*1000 + TK_MINOR_VERSION) >= 4001
extern int Tk_GetNumMainWindows();
#else
extern int tk_NumMainWindows;
#define Tk_GetNumMainWindows() (tk_NumMainWindows)
#define NEED_TKCREATEMAINWINDOW 1
#endif

#if TK_MAJOR_VERSION < 4
extern struct { Tk_Window win; } *tkMainWindowList;
#endif

#ifdef macintosh

/*
** Additional cruft needed by Tcl/Tk on the Mac.
** This is for Tcl 7.5 and Tk 4.1 (patch release 1).
*/

/* ckfree() expects a char* */
#define FREECAST (char *)

#include <Events.h> /* For EventRecord */

typedef int (*TclMacConvertEventPtr) Py_PROTO((EventRecord *eventPtr));
void TclMacSetEventProc Py_PROTO((TclMacConvertEventPtr procPtr));
int TkMacConvertEvent Py_PROTO((EventRecord *eventPtr));

staticforward int PyMacConvertEvent Py_PROTO((EventRecord *eventPtr));

#endif /* macintosh */

#ifndef FREECAST
#define FREECAST (char *)
#endif

/**** Tkapp Object Declaration ****/

staticforward PyTypeObject Tkapp_Type;

typedef struct
{
	PyObject_HEAD
	Tcl_Interp *interp;
#ifdef NEED_TKCREATEMAINWINDOW
	Tk_Window tkwin;
#endif
}
TkappObject;

#define Tkapp_Check(v) ((v)->ob_type == &Tkapp_Type)
#ifdef NEED_TKCREATEMAINWINDOW
#define Tkapp_Tkwin(v)  (((TkappObject *) (v))->tkwin)
#endif
#define Tkapp_Interp(v) (((TkappObject *) (v))->interp)
#define Tkapp_Result(v) (((TkappObject *) (v))->interp->result)

#define DEBUG_REFCNT(v) (printf ("DEBUG: id=%p, refcnt=%i\n", \
(void *) v, ((PyObject *) v)->ob_refcnt))



/**** Error Handling ****/

static PyObject *Tkinter_TclError;
static int quitMainLoop = 0;
static int errorInCmd = 0;
static PyObject *excInCmd;
static PyObject *valInCmd;
static PyObject *trbInCmd;



static PyObject *
Tkinter_Error(v)
	PyObject *v;
{
	PyErr_SetString(Tkinter_TclError, Tkapp_Result(v));
	return NULL;
}


int
PythonCmd_Error(interp)
	Tcl_Interp *interp;
{
	errorInCmd = 1;
	PyErr_Fetch(&excInCmd, &valInCmd, &trbInCmd);
	return TCL_ERROR;
}



/**** Utils ****/
static char *
AsString(value, tmp)
	PyObject *value;
	PyObject *tmp;
{
	if (PyString_Check (value))
		return PyString_AsString (value);
	else {
		PyObject *v = PyObject_Str(value);
		PyList_Append(tmp, v);
		Py_DECREF(v);
		return PyString_AsString(v);
	}
}



#define ARGSZ 64

static char *
Merge(args)
	PyObject *args;
{
	PyObject *tmp = NULL;
	char *argvStore[ARGSZ];
	char **argv = NULL;
	int fvStore[ARGSZ];
	int *fv = NULL;
	int argc = 0, i;
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
		argv[0] = AsString(args, tmp);
	}
	else {
		argc = PyTuple_Size(args);

		if (argc > ARGSZ) {
			argv = (char **)ckalloc(argc * sizeof (char *));
			fv = (int *)ckalloc(argc * sizeof (int));
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
			}
			else if (v == Py_None) {
				argc = i;
				break;
			}
			else {
				fv[i] = 0;
				argv[i] = AsString(v, tmp);
			}
		}
	}
	res = Tcl_Merge(argc, argv);

  finally:
	for (i = 0; i < argc; i++)
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
Split(self, list)
	PyObject *self;
	char *list;
{
	int argc;
	char **argv;
	PyObject *v;

	if (list == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	if (Tcl_SplitList(Tkapp_Interp(self), list, &argc, &argv) == TCL_ERROR)
	{
		/* Not a list.
		 * Could be a quoted string containing funnies, e.g. {"}.
		 * Return the string itself.
		 */
		PyErr_Clear();
		return PyString_FromString(list);
	}

	if (argc == 0)
		v = PyString_FromString("");
	else if (argc == 1)
		v = PyString_FromString(argv[0]);
	else if ((v = PyTuple_New (argc)) != NULL) {
		int i;
		PyObject *w;

		for (i = 0; i < argc; i++) {
			if ((w = Split(self, argv[i])) == NULL) {
				Py_DECREF(v);
				v = NULL;
				break;
			}
			PyTuple_SetItem(v, i, w);
		}
	}
	ckfree (FREECAST argv);
	return v;
}



/**** Tkapp Object ****/

#ifndef WITH_APPINIT
int
Tcl_AppInit (interp)
	Tcl_Interp *interp;
{
	Tk_Window main;

	main = Tk_MainWindow(interp);
	if (Tcl_Init(interp) == TCL_ERROR) {
		fprintf(stderr, "Tcl_Init error: %s\n", interp->result);
		return TCL_ERROR;
	}
	if (Tk_Init(interp) == TCL_ERROR) {
		fprintf(stderr, "Tk_Init error: %s\n", interp->result);
		return TCL_ERROR;
	}
	return TCL_OK;
}
#endif /* !WITH_APPINIT */




/* Initialize the Tk application; see the `main' function in
 * `tkMain.c'.
 */
static TkappObject *
Tkapp_New(screenName, baseName, className, interactive)
	char *screenName;
	char *baseName;
	char *className;
	int interactive;
{
	TkappObject *v;
	char *argv0;
  
	v = PyObject_NEW(TkappObject, &Tkapp_Type);
	if (v == NULL)
		return NULL;

	v->interp = Tcl_CreateInterp();

#ifdef NEED_TKCREATEMAINWINDOW
	v->tkwin = Tk_CreateMainWindow(v->interp, screenName, 
				       baseName, className);
	if (v->tkwin == NULL)
		return (TkappObject *)Tkinter_Error((PyObject *) v);

	Tk_GeometryRequest(v->tkwin, 200, 200);
#endif

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
	if (isupper(argv0[0]))
		argv0[0] = tolower(argv0[0]);
	Tcl_SetVar(v->interp, "argv0", argv0, TCL_GLOBAL_ONLY);
	ckfree(argv0);

	if (Tcl_AppInit(v->interp) != TCL_OK)
		return (TkappObject *)Tkinter_Error(v);

	return v;
}



/** Tcl Eval **/

static PyObject *
Tkapp_Call(self, args)
	PyObject *self;
	PyObject *args;
{
	char *cmd  = Merge(args);
	PyObject *res = NULL;

	if (!cmd)
		PyErr_SetString(Tkinter_TclError, "merge failed");

	else if (Tcl_Eval(Tkapp_Interp(self), cmd) == TCL_ERROR)
		res = Tkinter_Error(self);

	else
		res = PyString_FromString(Tkapp_Result(self));

	if (cmd)
		ckfree(cmd);

	return res;
}


static PyObject *
Tkapp_GlobalCall(self, args)
	PyObject *self;
	PyObject *args;
{
	char *cmd  = Merge(args);
	PyObject *res = NULL;
	

	if (!cmd)
		PyErr_SetString(Tkinter_TclError, "merge failed");

	else if (Tcl_GlobalEval(Tkapp_Interp(self), cmd) == TCL_ERROR)
		res = Tkinter_Error(self);
	else
		res = PyString_FromString(Tkapp_Result(self));

	if (cmd)
		ckfree(cmd);

	return res;
}

static PyObject *
Tkapp_Eval(self, args)
	PyObject *self;
	PyObject *args;
{
	char *script;
  
	if (!PyArg_Parse (args, "s", &script))
		return NULL;

	if (Tcl_Eval(Tkapp_Interp(self), script) == TCL_ERROR)
		return Tkinter_Error(self);
  
	return PyString_FromString(Tkapp_Result(self));
}

static PyObject *
Tkapp_GlobalEval(self, args)
	PyObject *self;
	PyObject *args;
{
	char *script;
  
	if (!PyArg_Parse(args, "s", &script))
		return NULL;

	if (Tcl_GlobalEval(Tkapp_Interp(self), script) == TCL_ERROR)
		return Tkinter_Error (self);

	return PyString_FromString(Tkapp_Result(self));
}

static PyObject *
Tkapp_EvalFile (self, args)
	PyObject *self;
	PyObject *args;
{
	char *fileName;

	if (!PyArg_Parse(args, "s", &fileName))
		return NULL;

	if (Tcl_EvalFile(Tkapp_Interp(self), fileName) == TCL_ERROR)
		return Tkinter_Error (self);

	return PyString_FromString(Tkapp_Result(self));
}

static PyObject *
Tkapp_Record(self, args)
	PyObject *self;
	PyObject *args;
{
	char *script;

	if (!PyArg_Parse(args, "s", &script))
		return NULL;

	if (TCL_ERROR == Tcl_RecordAndEval(Tkapp_Interp(self),
					   script, TCL_NO_EVAL))
		return Tkinter_Error (self);

	return PyString_FromString(Tkapp_Result(self));
}

static PyObject *
Tkapp_AddErrorInfo(self, args)
	PyObject *self;
	PyObject *args;
{
	char *msg;

	if (!PyArg_Parse (args, "s", &msg))
		return NULL;
	Tcl_AddErrorInfo(Tkapp_Interp(self), msg);

	Py_INCREF(Py_None);
	return Py_None;
}



/** Tcl Variable **/

static PyObject *
SetVar(self, args, flags)
	PyObject *self;
	PyObject *args;
	int flags;
{
	char *name1, *name2, *ok;
	PyObject *newValue;
	PyObject *tmp = PyList_New(0);

	if (!tmp)
		return NULL;

	if (PyArg_Parse(args, "(sO)", &name1, &newValue))
		/* XXX Merge? */
		ok = Tcl_SetVar(Tkapp_Interp (self), name1, 
				AsString (newValue, tmp), flags);

	else if (PyArg_Parse(args, "(ssO)", &name1, &name2, &newValue))
		ok = Tcl_SetVar2(Tkapp_Interp (self), name1, name2, 
				 AsString (newValue, tmp), flags);
	else {
		Py_DECREF (tmp);
		return NULL;
	}
	Py_DECREF (tmp);

	if (!ok)
		return Tkinter_Error(self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Tkapp_SetVar (self, args)
	PyObject *self;
	PyObject *args;
{
	return SetVar(self, args, TCL_LEAVE_ERR_MSG);
}

static PyObject *
Tkapp_GlobalSetVar (self, args)
	PyObject *self;
	PyObject *args;
{
	return SetVar(self, args, TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
}



static PyObject *
GetVar (self, args, flags)
	PyObject *self;
	PyObject *args;
	int flags;
{
	char *name1, *name2, *s;

	if (PyArg_Parse(args, "s", &name1))
		s = Tcl_GetVar(Tkapp_Interp (self), name1, flags);

	else if (PyArg_Parse(args, "(ss)", &name1, &name2))
		s = Tcl_GetVar2(Tkapp_Interp(self), name1, name2, flags);
	else
		return NULL;

	if (s == NULL)
		return Tkinter_Error(self);

	return PyString_FromString (s);
}

static PyObject *
Tkapp_GetVar (self, args)
	PyObject *self;
	PyObject *args;
{
	return GetVar(self, args, TCL_LEAVE_ERR_MSG);
}

static PyObject *
Tkapp_GlobalGetVar (self, args)
	PyObject *self;
	PyObject *args;
{
	return GetVar(self, args, TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
}



static PyObject *
UnsetVar (self, args, flags)
	PyObject *self;
	PyObject *args;
	int flags;
{
	char *name1, *name2;
	int code;

	if (PyArg_Parse(args, "s", &name1))
		code = Tcl_UnsetVar(Tkapp_Interp (self), name1, flags);

	else if (PyArg_Parse(args, "(ss)", &name1, &name2))
		code = Tcl_UnsetVar2(Tkapp_Interp(self), name1, name2, flags);
	else
		return NULL;

	if (code == TCL_ERROR)
		return Tkinter_Error (self);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Tkapp_UnsetVar (self, args)
	PyObject *self;
	PyObject *args;
{
	return UnsetVar(self, args, TCL_LEAVE_ERR_MSG);
}

static PyObject *
Tkapp_GlobalUnsetVar (self, args)
	PyObject *self;
	PyObject *args;
{
	return UnsetVar(self, args, TCL_LEAVE_ERR_MSG | TCL_GLOBAL_ONLY);
}



/** Tcl to Python **/

static PyObject *
Tkapp_GetInt (self, args)
	PyObject *self;
	PyObject *args;
{
	char *s;
	int v;

	if (!PyArg_Parse(args, "s", &s))
		return NULL;
	if (Tcl_GetInt(Tkapp_Interp (self), s, &v) == TCL_ERROR)
		return Tkinter_Error(self);
	return Py_BuildValue("i", v);
}

static PyObject *
Tkapp_GetDouble (self, args)
	PyObject *self;
	PyObject *args;
{
	char *s;
	double v;

	if (!PyArg_Parse(args, "s", &s))
		return NULL;
	if (Tcl_GetDouble(Tkapp_Interp (self), s, &v) == TCL_ERROR)
		return Tkinter_Error(self);
	return Py_BuildValue("d", v);
}

static PyObject *
Tkapp_GetBoolean (self, args)
	PyObject *self;
	PyObject *args;
{
	char *s;
	int v;

	if (!PyArg_Parse(args, "s", &s))
		return NULL;
	if (Tcl_GetBoolean(Tkapp_Interp (self), s, &v) == TCL_ERROR)
		return Tkinter_Error (self);
	return Py_BuildValue("i", v);
}

static PyObject *
Tkapp_ExprString (self, args)
	PyObject *self;
	PyObject *args;
{
	char *s;

	if (!PyArg_Parse(args, "s", &s))
		return NULL;
	if (Tcl_ExprString (Tkapp_Interp (self), s) == TCL_ERROR)
		return Tkinter_Error(self);
	return Py_BuildValue("s", Tkapp_Result(self));
}

static PyObject *
Tkapp_ExprLong (self, args)
	PyObject *self;
	PyObject *args;
{
	char *s;
	long v;

	if (!PyArg_Parse(args, "s", &s))
		return NULL;
	if (Tcl_ExprLong(Tkapp_Interp(self), s, &v) == TCL_ERROR)
		return Tkinter_Error(self);
	return Py_BuildValue("l", v);
}

static PyObject *
Tkapp_ExprDouble (self, args)
	PyObject *self;
	PyObject *args;
{
	char *s;
	double v;
	int retval;

	if (!PyArg_Parse(args, "s", &s))
		return NULL;
	PyFPE_START_PROTECT("Tkapp_ExprDouble", return 0)
	retval = Tcl_ExprDouble (Tkapp_Interp (self), s, &v);
	PyFPE_END_PROTECT(retval)
	if (retval == TCL_ERROR)
		return Tkinter_Error(self);
	return Py_BuildValue("d", v);
}

static PyObject *
Tkapp_ExprBoolean (self, args)
	PyObject *self;
	PyObject *args;
{
	char *s;
	int v;

	if (!PyArg_Parse(args, "s", &s))
		return NULL;
	if (Tcl_ExprBoolean(Tkapp_Interp(self), s, &v) == TCL_ERROR)
		return Tkinter_Error(self);
	return Py_BuildValue("i", v);
}



static PyObject *
Tkapp_SplitList (self, args)
	PyObject *self;
	PyObject *args;
{
	char *list;
	int argc;
	char **argv;
	PyObject *v;
	int i;

	if (!PyArg_Parse(args, "s", &list))
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
Tkapp_Split (self, args)
	PyObject *self;
	PyObject *args;
{
	char *list;

	if (!PyArg_Parse(args, "s", &list))
		return NULL;
	return Split(self, list);
}

static PyObject *
Tkapp_Merge (self, args)
	PyObject *self;
	PyObject *args;
{
	char *s = Merge(args);
	PyObject *res = NULL;

	if (s) {
		res = PyString_FromString(s);
		ckfree(s);
	}
	else
		PyErr_SetString(Tkinter_TclError, "merge failed");

	return res;
}



/** Tcl Command **/

/* This is the Tcl command that acts as a wrapper for Python
 * function or method.
 */
static int
PythonCmd (clientData, interp, argc, argv)
	ClientData clientData;		     /* Is (self, func) */
	Tcl_Interp *interp;
	int argc;
	char *argv[];
{
	PyObject *self, *func, *arg, *res, *tmp;
	int i;

	/* TBD: no error checking here since we know, via the
	 * Tkapp_CreateCommand() that the client data is a two-tuple
	 */
	self = PyTuple_GetItem((PyObject *) clientData, 0);
	func = PyTuple_GetItem((PyObject *) clientData, 1);

	/* Create argument list (argv1, ..., argvN) */
	if (!(arg = PyTuple_New(argc - 1)))
		return PythonCmd_Error(interp);

	for (i = 0; i < (argc - 1); i++) {
		PyObject *s = PyString_FromString(argv[i + 1]);
		if (!s || PyTuple_SetItem(arg, i, s)) {
			Py_DECREF(arg);
			PythonCmd_Error(interp);
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

	Tcl_SetResult(Tkapp_Interp(self), AsString(res, tmp), TCL_VOLATILE);
	Py_DECREF(res);
	Py_DECREF(tmp);

	return TCL_OK;
}

static void
PythonCmdDelete (clientData)
	ClientData clientData;	/* Is (self, func) */
{
	Py_DECREF((PyObject *) clientData);
}



static PyObject *
Tkapp_CreateCommand (self, args)
	PyObject *self;
	PyObject *args;
{
	char *cmdName;
	PyObject *data;
	PyObject *func;
  
	/* Args is: (cmdName, func) */
	if (!PyTuple_Check(args) 
	    || !(PyTuple_Size(args) == 2)
	    || !PyString_Check(PyTuple_GetItem(args, 0))
	    || !PyCallable_Check(PyTuple_GetItem(args, 1)))
	{
		PyErr_SetString (PyExc_TypeError, "bad argument list");
		return NULL;
	}

	cmdName = PyString_AsString(PyTuple_GetItem(args, 0));
	func = PyTuple_GetItem(args, 1);

	data = PyTuple_New(2);		     /* ClientData is: (self, func) */
	if (!data)
		return NULL;

	Py_INCREF(self);
	PyTuple_SetItem(data, 0, self);

	Py_INCREF(func);
	PyTuple_SetItem(data, 1, func);

	Tcl_CreateCommand(Tkapp_Interp (self), cmdName, PythonCmd,
			  (ClientData) data, PythonCmdDelete);

	Py_INCREF(Py_None);
	return Py_None;
}



static PyObject *
Tkapp_DeleteCommand (self, args)
	PyObject *self;
	PyObject *args;
{
	char *cmdName;

	if (!PyArg_Parse(args, "s", &cmdName))
		return NULL;
	if (Tcl_DeleteCommand(Tkapp_Interp(self), cmdName) == -1)
	{
		PyErr_SetString(Tkinter_TclError, "can't delete Tcl command");
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}



/** File Handler **/

static void
FileHandler (clientData, mask)
	ClientData clientData;		     /* Is: (func, file) */
	int mask;
{
	PyObject *func, *file, *arg, *res;

	func = PyTuple_GetItem((PyObject *) clientData, 0);
	file = PyTuple_GetItem((PyObject *) clientData, 1);

	arg = Py_BuildValue("(Oi)", file, (long) mask);
	res = PyEval_CallObject(func, arg);
	Py_DECREF (arg);

	if (res == NULL) {
		errorInCmd = 1;
		PyErr_Fetch(&excInCmd, &valInCmd, &trbInCmd);
	}
	Py_XDECREF(res);
}

static int
GetFileNo (file)
	/* Either an int >= 0 or an object with a
	 *.fileno() method that returns an int >= 0
	 */
	PyObject *file;
{
	PyObject *meth, *args, *res;
	int id;
	if (PyInt_Check(file)) {
		id = PyInt_AsLong(file);
		if (id < 0)
			PyErr_SetString(PyExc_ValueError, "invalid file id");
		return id;
	}
	args = PyTuple_New(0);
	if (args == NULL)
		return -1;

	meth = PyObject_GetAttrString(file, "fileno");
	if (meth == NULL) {
		Py_DECREF(args);
		return -1;
	}

	res = PyEval_CallObject(meth, args);
	Py_DECREF(args);
	Py_DECREF(meth);
	if (res == NULL)
		return -1;

	if (PyInt_Check(res))
		id = PyInt_AsLong(res);
	else
		id = -1;

	if (id < 0)
		PyErr_SetString(PyExc_ValueError,
				"invalid fileno() return value");
	Py_DECREF(res);
	return id;
}


static PyObject* Tkapp_ClientDataDict = NULL;

static PyObject *
Tkapp_CreateFileHandler (self, args)
	PyObject *self;
	PyObject *args;			     /* Is (file, mask, func) */
{
	PyObject *file, *func, *data;
	PyObject *idkey;
	int mask, id;
#if (TK_MAJOR_VERSION*1000 + TK_MINOR_VERSION) >= 4001
	Tcl_File tfile;
#endif

	if (!Tkapp_ClientDataDict) {
		if (!(Tkapp_ClientDataDict = PyDict_New()))
			return NULL;
	}

	if (!PyArg_Parse(args, "(OiO)", &file, &mask, &func))
		return NULL;
	id = GetFileNo(file);
	if (id < 0)
		return NULL;
	if (!PyCallable_Check(func)) {
		PyErr_SetString (PyExc_TypeError, "bad argument list");
		return NULL;
	}

	if (!(idkey = PyInt_FromLong(id)))
		return NULL;

	/* ClientData is: (func, file) */
	data = Py_BuildValue ("(OO)", func, file);
	if (!data || PyDict_SetItem(Tkapp_ClientDataDict, idkey, data)) {
		Py_DECREF(idkey);
		Py_XDECREF(data);
		return NULL;
	}
	Py_DECREF(idkey);

#if (TK_MAJOR_VERSION*1000 + TK_MINOR_VERSION) >= 4001
#ifdef MS_WINDOWS
	/* We assume this is a socket... */
	tfile = Tcl_GetFile((ClientData)id, TCL_WIN_SOCKET);
#else
	tfile = Tcl_GetFile((ClientData)id, TCL_UNIX_FD);
#endif
	/* Ought to check for null Tcl_File object... */
	Tcl_CreateFileHandler(tfile, mask, FileHandler, (ClientData) data);
#else
	Tk_CreateFileHandler(id, mask, FileHandler, (ClientData) data);
#endif
	/* XXX fileHandlerDict */
	Py_INCREF (Py_None);
	return Py_None;
}


static PyObject *
Tkapp_DeleteFileHandler (self, args)
	PyObject *self;
	PyObject *args;			     /* Args: file */
{
	PyObject *file;
	PyObject *idkey;
	PyObject *data;
	int id;
#if (TK_MAJOR_VERSION*1000 + TK_MINOR_VERSION) >= 4001
	Tcl_File tfile;
#endif
  
	if (!PyArg_Parse(args, "O", &file))
		return NULL;
	id = GetFileNo(file);
	if (id < 0)
		return NULL;

	if (!(idkey = PyInt_FromLong(id)))
		return NULL;
	
	/* find and free the object created in the
	 * Tkapp_CreateFileHandler() call
	 */
	data = PyDict_GetItem(Tkapp_ClientDataDict, idkey);
	Py_XDECREF(data);
	PyDict_DelItem(Tkapp_ClientDataDict, idkey);
	Py_DECREF(idkey);

#if (TK_MAJOR_VERSION*1000 + TK_MINOR_VERSION) >= 4001
#ifdef MS_WINDOWS
	/* We assume this is a socket... */
	tfile = Tcl_GetFile((ClientData)id, TCL_WIN_SOCKET);
#else
	tfile = Tcl_GetFile((ClientData)id, TCL_UNIX_FD);
#endif
	/* Ought to check for null Tcl_File object... */
	Tcl_DeleteFileHandler(tfile);
#else
	Tk_DeleteFileHandler(id);
#endif
	/* XXX fileHandlerDict */
	Py_INCREF (Py_None);
	return Py_None;
}


/**** Tktt Object (timer token) ****/

staticforward PyTypeObject Tktt_Type;

typedef struct
{
	PyObject_HEAD
	Tk_TimerToken token;
	PyObject *func;
}
TkttObject;

static PyObject *
Tktt_DeleteTimerHandler (self, args)
	PyObject *self;
	PyObject *args;
{
	TkttObject *v = (TkttObject *)self;

	if (!PyArg_Parse(args, ""))
		return NULL;
	if (v->func != NULL) {
		Tk_DeleteTimerHandler(v->token);
		PyMem_DEL(v->func);
		v->func = NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef Tktt_methods[] =
{
	{"deletetimerhandler", Tktt_DeleteTimerHandler},
	{NULL, NULL}
};

static TkttObject *
Tktt_New (token, func)
	Tk_TimerToken token;
	PyObject *func;
{
	TkttObject *v;
  
	v = PyObject_NEW(TkttObject, &Tktt_Type);
	if (v == NULL)
		return NULL;

	v->token = token;
	v->func = func;
	Py_INCREF(v->func);
	return v;
}

static void
Tktt_Dealloc (self)
	PyObject *self;
{
	PyMem_DEL (self);
}

static int
Tktt_Print (self, fp, flags)
	PyObject *self;
	FILE *fp;
	int flags;
{
	TkttObject *v = (TkttObject *)self;

	fprintf(fp, "<tktimertoken at 0x%lx%s>", (long)v,
		v->func == NULL ? ", handler deleted" : "");
	return 0;
}

static PyObject *
Tktt_GetAttr (self, name)
	PyObject *self;
	char *name;
{
	return Py_FindMethod(Tktt_methods, self, name);
}

static PyTypeObject Tktt_Type =
{
	PyObject_HEAD_INIT (NULL)
	0,				     /*ob_size */
	"tktimertoken",			     /*tp_name */
	sizeof (TkttObject),		     /*tp_basicsize */
	0,				     /*tp_itemsize */
	Tktt_Dealloc,			     /*tp_dealloc */
	Tktt_Print,			     /*tp_print */
	Tktt_GetAttr,			     /*tp_getattr */
	0,				     /*tp_setattr */
	0,				     /*tp_compare */
	0,				     /*tp_repr */
	0,				     /*tp_as_number */
	0,				     /*tp_as_sequence */
	0,				     /*tp_as_mapping */
	0,				     /*tp_hash */
};



/** Timer Handler **/

static void
TimerHandler (clientData)
	ClientData clientData;
{
	PyObject *func = (PyObject *)clientData;
	PyObject *res  = PyEval_CallObject(func, NULL);

	if (res == NULL) {
		errorInCmd = 1;
		PyErr_Fetch(&excInCmd, &valInCmd, &trbInCmd);
	}
	else
		Py_DECREF(res);
}

static PyObject *
Tkapp_CreateTimerHandler (self, args)
	PyObject *self;
	PyObject *args;			     /* Is (milliseconds, func) */
{
	int milliseconds;
	PyObject *func;
	Tk_TimerToken token;

	if (!PyArg_Parse(args, "(iO)", &milliseconds, &func))
		return NULL;
	if (!PyCallable_Check(func)) {
		PyErr_SetString (PyExc_TypeError, "bad argument list");
		return NULL;
	}
	token = Tk_CreateTimerHandler(milliseconds, TimerHandler,
				      (ClientData)func);

	return (PyObject *) Tktt_New(token, func);
}



/** Event Loop **/

static PyObject *
Tkapp_MainLoop (self, args)
	PyObject *self;
	PyObject *args;
{
	int threshold = 0;

	if (!PyArg_ParseTuple(args, "|i", &threshold))
		return NULL;

	quitMainLoop = 0;
	while (Tk_GetNumMainWindows() > threshold &&
	       !quitMainLoop &&
	       !errorInCmd)
	{
		/* XXX Ought to check for other signals! */
		if (PyOS_InterruptOccurred()) {
			PyErr_SetNone(PyExc_KeyboardInterrupt);
			return NULL;
		}
		Tk_DoOneEvent(0);
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
Tkapp_DoOneEvent (self, args)
	PyObject *self;
	PyObject *args;
{
	int flags = TK_ALL_EVENTS;
	int rv;

	if (!PyArg_ParseTuple(args, "|i", &flags))
		return NULL;

	rv = Tk_DoOneEvent(flags);
	return Py_BuildValue("i", rv);
}

static PyObject *
Tkapp_Quit (self, args)
	PyObject *self;
	PyObject *args;
{

	if (!PyArg_Parse(args, ""))
		return NULL;

	quitMainLoop = 1;
	Py_INCREF(Py_None);
	return Py_None;
}



/**** Tkapp Method List ****/

static PyMethodDef Tkapp_methods[] =
{
	{"call", 	       Tkapp_Call},
	{"globalcall", 	       Tkapp_GlobalCall},
	{"eval", 	       Tkapp_Eval},
	{"globaleval", 	       Tkapp_GlobalEval},
	{"evalfile", 	       Tkapp_EvalFile},
	{"record", 	       Tkapp_Record},
	{"adderrorinfo",       Tkapp_AddErrorInfo},
	{"setvar", 	       Tkapp_SetVar},
	{"globalsetvar",       Tkapp_GlobalSetVar},
	{"getvar", 	       Tkapp_GetVar},
	{"globalgetvar",       Tkapp_GlobalGetVar},
	{"unsetvar", 	       Tkapp_UnsetVar},
	{"globalunsetvar",     Tkapp_GlobalUnsetVar},
	{"getint", 	       Tkapp_GetInt},
	{"getdouble", 	       Tkapp_GetDouble},
	{"getboolean", 	       Tkapp_GetBoolean},
	{"exprstring", 	       Tkapp_ExprString},
	{"exprlong", 	       Tkapp_ExprLong},
	{"exprdouble", 	       Tkapp_ExprDouble},
	{"exprboolean",        Tkapp_ExprBoolean},
	{"splitlist", 	       Tkapp_SplitList},
	{"split", 	       Tkapp_Split},
	{"merge", 	       Tkapp_Merge},
	{"createcommand",      Tkapp_CreateCommand},
	{"deletecommand",      Tkapp_DeleteCommand},
	{"createfilehandler",  Tkapp_CreateFileHandler},
	{"deletefilehandler",  Tkapp_DeleteFileHandler},
	{"createtimerhandler", Tkapp_CreateTimerHandler},
	{"mainloop", 	       Tkapp_MainLoop, 1},
	{"dooneevent", 	       Tkapp_DoOneEvent, 1},
	{"quit", 	       Tkapp_Quit},
	{NULL, 		       NULL}
};



/**** Tkapp Type Methods ****/

static void
Tkapp_Dealloc (self)
	PyObject *self;
{
#ifdef NEED_TKCREATEMAINWINDOW
	Tk_DestroyWindow (Tkapp_Tkwin (self));
#endif
	Tcl_DeleteInterp (Tkapp_Interp (self));
	PyMem_DEL (self);
}

static PyObject *
Tkapp_GetAttr (self, name)
	PyObject *self;
	char *name;
{
	return Py_FindMethod (Tkapp_methods, self, name);
}

static PyTypeObject Tkapp_Type =
{
	PyObject_HEAD_INIT (NULL)
	0,				     /*ob_size */
	"tkapp",			     /*tp_name */
	sizeof (TkappObject),		     /*tp_basicsize */
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

static PyObject *
Tkinter_Create (self, args)
	PyObject *self;
	PyObject *args;
{
	char *screenName = NULL;
	char *baseName = NULL;
	char *className = NULL;
	int interactive = 0;

	baseName = strrchr(Py_GetProgramName (), '/');
	if (baseName != NULL)
		baseName++;
	else
		baseName = Py_GetProgramName();
	className = "Tk";
  
	if (!PyArg_ParseTuple(args, "|zssi",
			      &screenName, &baseName, &className,
			      &interactive))
		return NULL;

	return (PyObject *) Tkapp_New(screenName, baseName, className, 
				      interactive);
}

static PyMethodDef moduleMethods[] =
{
	{"create",             Tkinter_Create, 1},
	{"createfilehandler",  Tkapp_CreateFileHandler, 0},
	{"deletefilehandler",  Tkapp_DeleteFileHandler, 0},
	{"createtimerhandler", Tkapp_CreateTimerHandler, 0},
	{"mainloop",           Tkapp_MainLoop, 1},
	{"dooneevent",         Tkapp_DoOneEvent, 1},
	{"quit",               Tkapp_Quit},
	{NULL,                 NULL}
};

#ifdef WITH_READLINE
static int
EventHook ()
{
	/* XXX Reset tty */
	if (errorInCmd) {
		errorInCmd = 0;
		PyErr_Restore(excInCmd, valInCmd, trbInCmd);
		excInCmd = valInCmd = trbInCmd = NULL;
		PyErr_Print();
	}
	if (Tk_GetNumMainWindows() > 0)
		Tk_DoOneEvent(TK_DONT_WAIT);
	return 0;
}
#endif /* WITH_READLINE */

static void
Tkinter_Cleanup ()
{
	/* This segfault with Tk 4.0 beta and seems unnecessary there as
	 * well */

#if TK_MAJOR_VERSION < 4
	/* XXX rl_deprep_terminal is static, damned! */
	while (tkMainWindowList != 0)
		Tk_DestroyWindow(tkMainWindowList->win);
#endif
}


/* all errors will be checked in one fell swoop in init_tkinter() */
static void
ins_long(d, name, val)
	PyObject *d;
	char *name;
	long val;
{
	PyObject *v = PyInt_FromLong(val);
	if (v) {
		PyDict_SetItemString(d, name, v);
		Py_DECREF(v);
	}
}
static void
ins_string(d, name, val)
	PyObject *d;
	char *name;
	char *val;
{
	PyObject *v = PyString_FromString(val);
	if (v) {
		PyDict_SetItemString(d, name, v);
		Py_DECREF(v);
	}
}


void
init_tkinter ()
{
	static inited = 0;

#ifdef WITH_READLINE
	extern int (*rl_event_hook) ();
#endif /* WITH_READLINE */
	PyObject *m, *d;

	Tkapp_Type.ob_type = &PyType_Type;
	Tktt_Type.ob_type = &PyType_Type;

	m = Py_InitModule("_tkinter", moduleMethods);

	d = PyModule_GetDict(m);
	Tkinter_TclError = Py_BuildValue("s", "TclError");
	PyDict_SetItemString(d, "TclError", Tkinter_TclError);

	ins_long(d, "READABLE", TK_READABLE);
	ins_long(d, "WRITABLE", TK_WRITABLE);
	ins_long(d, "EXCEPTION", TK_EXCEPTION);
	ins_long(d, "X_EVENTS", TK_X_EVENTS);
	ins_long(d, "FILE_EVENTS", TK_FILE_EVENTS);
	ins_long(d, "TIMER_EVENTS", TK_TIMER_EVENTS);
	ins_long(d, "IDLE_EVENTS", TK_IDLE_EVENTS);
	ins_long(d, "ALL_EVENTS", TK_ALL_EVENTS);
	ins_long(d, "DONT_WAIT", TK_DONT_WAIT);
	ins_string(d, "TK_VERSION", TK_VERSION);
	ins_string(d, "TCL_VERSION", TCL_VERSION);

#ifdef WITH_READLINE
	rl_event_hook = EventHook;
#endif /* WITH_READLINE */

	if (!inited) {
		inited = 1;
		if (Py_AtExit(Tkinter_Cleanup) != 0)
			fprintf(stderr,
		       "Tkinter: warning: cleanup procedure not registered\n");
	}

	if (PyErr_Occurred())
		Py_FatalError ("can't initialize module _tkinter");
#ifdef macintosh
	TclMacSetEventProc(PyMacConvertEvent);
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
PyMacConvertEvent(eventPtr)
	EventRecord *eventPtr;
{
	if (SIOUXHandleOneEvent(eventPtr))
		return 0; /* Nothing happened to the Tcl event queue */
	return TkMacConvertEvent(eventPtr);
}

#ifdef USE_GUSI
/*
 * For Python we have to override this routine (from TclMacNotify),
 * since we use GUSI for our sockets, not Tcl streams. Hence, we have
 * to use GUSI select to see whether our socket is ready. Note that
 * createfilehandler (above) sets the type to TCL_UNIX_FD for our
 * files and sockets.
 *
 * NOTE: this code was lifted from Tcl 7.6, it may need to be modified
 * for other versions.  */

int
Tcl_FileReady(file, mask)
    Tcl_File file;		/* File handle for a stream. */
    int mask;			/* OR'ed combination of TCL_READABLE,
				 * TCL_WRITABLE, and TCL_EXCEPTION:
				 * indicates conditions caller cares about. */
{
    int type;
    int fd;

    fd = (int) Tcl_GetFileInfo(file, &type);

    if (type == TCL_MAC_SOCKET) {
	return TclMacSocketReady(file, mask);
    } else if (type == TCL_MAC_FILE) {
	/*
	 * Under the Macintosh, files are always ready, so we just 
	 * return the mask that was passed in.
	 */

	return mask;
    } else if (type == TCL_UNIX_FD) {
	fd_set readset, writeset, excset;
	struct timeval tv;
	
	FD_ZERO(&readset);
	FD_ZERO(&writeset);
	FD_ZERO(&excset);
	
	if ( mask & TCL_READABLE ) FD_SET(fd, &readset);
	if ( mask & TCL_WRITABLE ) FD_SET(fd, &writeset);
	if ( mask & TCL_EXCEPTION ) FD_SET(fd, &excset);
	
	tv.tv_sec = tv.tv_usec = 0;
	if ( select(fd+1, &readset, &writeset, &excset, &tv) <= 0 )
		return 0;
	
	mask = 0;
	if ( FD_ISSET(fd, &readset) ) mask |= TCL_READABLE;
	if ( FD_ISSET(fd, &writeset) ) mask |= TCL_WRITABLE;
	if ( FD_ISSET(fd, &excset) ) mask |= TCL_EXCEPTION;

	return mask;
    }
    
    return 0;
}
#endif /* USE_GUSI */

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
mac_addlibresources()
{
	if ( !loaded_from_shlib ) 
		return;
	(void)FSpOpenResFile(&library_fss, fsRdPerm);
}

#endif /* GENERATINGCFM */
#endif /* macintosh */
