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

/* Python interpreter top-level routines, including init/exit */

#include "Python.h"

#include "grammar.h"
#include "node.h"
#include "parsetok.h"
#include "errcode.h"
#include "compile.h"
#include "eval.h"
#include "marshal.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef MS_WIN32
#undef BYTE
#include "windows.h"
#endif

extern char *Py_GetPath();

extern grammar _PyParser_Grammar; /* From graminit.c */

/* Forward */
static void initmain Py_PROTO((void));
static void initsite Py_PROTO((void));
static PyObject *run_err_node Py_PROTO((node *n, char *filename,
				   PyObject *globals, PyObject *locals));
static PyObject *run_node Py_PROTO((node *n, char *filename,
			       PyObject *globals, PyObject *locals));
static PyObject *run_pyc_file Py_PROTO((FILE *fp, char *filename,
				   PyObject *globals, PyObject *locals));
static void err_input Py_PROTO((perrdetail *));
static void initsigs Py_PROTO((void));
static void call_sys_exitfunc Py_PROTO((void));
static void call_ll_exitfuncs Py_PROTO((void));

int Py_DebugFlag; /* Needed by parser.c */
int Py_VerboseFlag; /* Needed by import.c */
int Py_InteractiveFlag; /* Needed by Py_FdIsInteractive() below */
int Py_NoSiteFlag; /* Suppress 'import site' */
int Py_UseClassExceptionsFlag = 1; /* Needed by bltinmodule.c */

static int initialized = 0;

/* API to access the initialized flag -- useful for eroteric use */

int
Py_IsInitialized()
{
	return initialized;
}

/* Global initializations.  Can be undone by Py_Finalize().  Don't
   call this twice without an intervening Py_Finalize() call.  When
   initializations fail, a fatal error is issued and the function does
   not return.  On return, the first thread and interpreter state have
   been created.

   Locking: you must hold the interpreter lock while calling this.
   (If the lock has not yet been initialized, that's equivalent to
   having the lock, but you cannot use multiple threads.)

*/

void
Py_Initialize()
{
	PyInterpreterState *interp;
	PyThreadState *tstate;
	PyObject *bimod, *sysmod;
	char *p;

	if (initialized)
		return;
	initialized = 1;
	
	if ((p = getenv("PYTHONDEBUG")) && *p != '\0')
		Py_DebugFlag = 1;
	if ((p = getenv("PYTHONVERBOSE")) && *p != '\0')
		Py_VerboseFlag = 1;

	interp = PyInterpreterState_New();
	if (interp == NULL)
		Py_FatalError("Py_Initialize: can't make first interpreter");

	tstate = PyThreadState_New(interp);
	if (tstate == NULL)
		Py_FatalError("Py_Initialize: can't make first thread");
	(void) PyThreadState_Swap(tstate);

	interp->modules = PyDict_New();
	if (interp->modules == NULL)
		Py_FatalError("Py_Initialize: can't make modules dictionary");

	bimod = _PyBuiltin_Init_1();
	if (bimod == NULL)
		Py_FatalError("Py_Initialize: can't initialize __builtin__");
	interp->builtins = PyModule_GetDict(bimod);
	Py_INCREF(interp->builtins);

	sysmod = _PySys_Init();
	if (sysmod == NULL)
		Py_FatalError("Py_Initialize: can't initialize sys");
	interp->sysdict = PyModule_GetDict(sysmod);
	Py_INCREF(interp->sysdict);
	_PyImport_FixupExtension("sys", "sys");
	PySys_SetPath(Py_GetPath());
	PyDict_SetItemString(interp->sysdict, "modules",
			     interp->modules);

	/* phase 2 of builtins */
	_PyBuiltin_Init_2(interp->builtins);
	_PyImport_FixupExtension("__builtin__", "__builtin__");

	_PyImport_Init();

	initsigs(); /* Signal handling stuff, including initintr() */

	initmain(); /* Module __main__ */
	if (!Py_NoSiteFlag)
		initsite(); /* Module site */
}

/* Undo the effect of Py_Initialize().

   Beware: if multiple interpreter and/or thread states exist, these
   are not wiped out; only the current thread and interpreter state
   are deleted.  But since everything else is deleted, those other
   interpreter and thread states should no longer be used.

   (XXX We should do better, e.g. wipe out all interpreters and
   threads.)

   Locking: as above.

*/

void
Py_Finalize()
{
	PyInterpreterState *interp;
	PyThreadState *tstate;

	call_sys_exitfunc();

	if (!initialized)
		return;
	initialized = 0;

	/* Get current thread state and interpreter pointer */
	tstate = PyThreadState_Get();
	interp = tstate->interp;

	/* Disable signal handling */
	PyOS_FiniInterrupts();

	/* Destroy PyExc_MemoryErrorInst */
	_PyBuiltin_Fini_1();

	/* Destroy all modules */
	PyImport_Cleanup();

	/* Delete current thread */
	PyInterpreterState_Clear(interp);
	PyThreadState_Swap(NULL);
	PyInterpreterState_Delete(interp);

	_PyImport_Fini();

	/* Now we decref the exception classes.  After this point nothing
	   can raise an exception.  That's okay, because each Fini() method
	   below has been checked to make sure no exceptions are ever
	   raised.
	*/
	_PyBuiltin_Fini_2();
	PyMethod_Fini();
	PyFrame_Fini();
	PyCFunction_Fini();
	PyTuple_Fini();
	PyString_Fini();
	PyInt_Fini();
	PyFloat_Fini();

	/* XXX Still allocated:
	   - various static ad-hoc pointers to interned strings
	   - int and float free list blocks
	   - whatever various modules and libraries allocate
	*/

	PyGrammar_RemoveAccelerators(&_PyParser_Grammar);

	call_ll_exitfuncs();

#ifdef COUNT_ALLOCS
	dump_counts();
#endif

#ifdef Py_REF_DEBUG
	fprintf(stderr, "[%ld refs]\n", _Py_RefTotal);
#endif

#ifdef Py_TRACE_REFS
	if (_Py_AskYesNo("Print left references?")) {
		_Py_PrintReferences(stderr);
	}
	_Py_ResetReferences();
#endif /* Py_TRACE_REFS */
}

/* Create and initialize a new interpreter and thread, and return the
   new thread.  This requires that Py_Initialize() has been called
   first.

   Unsuccessful initialization yields a NULL pointer.  Note that *no*
   exception information is available even in this case -- the
   exception information is held in the thread, and there is no
   thread.

   Locking: as above.

*/

PyThreadState *
Py_NewInterpreter()
{
	PyInterpreterState *interp;
	PyThreadState *tstate, *save_tstate;
	PyObject *bimod, *sysmod;

	if (!initialized)
		Py_FatalError("Py_NewInterpreter: call Py_Initialize first");

	interp = PyInterpreterState_New();
	if (interp == NULL)
		return NULL;

	tstate = PyThreadState_New(interp);
	if (tstate == NULL) {
		PyInterpreterState_Delete(interp);
		return NULL;
	}

	save_tstate = PyThreadState_Swap(tstate);

	/* XXX The following is lax in error checking */

	interp->modules = PyDict_New();

	bimod = _PyImport_FindExtension("__builtin__", "__builtin__");
	if (bimod != NULL) {
		interp->builtins = PyModule_GetDict(bimod);
		Py_INCREF(interp->builtins);
	}
	sysmod = _PyImport_FindExtension("sys", "sys");
	if (bimod != NULL && sysmod != NULL) {
		interp->sysdict = PyModule_GetDict(sysmod);
		Py_INCREF(interp->sysdict);
		PySys_SetPath(Py_GetPath());
		PyDict_SetItemString(interp->sysdict, "modules",
				     interp->modules);
		initmain();
		if (!Py_NoSiteFlag)
			initsite();
	}

	if (!PyErr_Occurred())
		return tstate;

	/* Oops, it didn't work.  Undo it all. */

	PyErr_Print();
	PyThreadState_Clear(tstate);
	PyThreadState_Swap(save_tstate);
	PyThreadState_Delete(tstate);
	PyInterpreterState_Delete(interp);

	return NULL;
}

/* Delete an interpreter and its last thread.  This requires that the
   given thread state is current, that the thread has no remaining
   frames, and that it is its interpreter's only remaining thread.
   It is a fatal error to violate these constraints.

   (Py_Finalize() doesn't have these constraints -- it zaps
   everything, regardless.)

   Locking: as above.

*/

void
Py_EndInterpreter(tstate)
	PyThreadState *tstate;
{
	PyInterpreterState *interp = tstate->interp;

	if (tstate != PyThreadState_Get())
		Py_FatalError("Py_EndInterpreter: thread is not current");
	if (tstate->frame != NULL)
		Py_FatalError("Py_EndInterpreter: thread still has a frame");
	if (tstate != interp->tstate_head || tstate->next != NULL)
		Py_FatalError("Py_EndInterpreter: not the last thread");

	PyImport_Cleanup();
	PyInterpreterState_Clear(interp);
	PyThreadState_Swap(NULL);
	PyInterpreterState_Delete(interp);
}

static char *progname = "python";

void
Py_SetProgramName(pn)
	char *pn;
{
	if (pn && *pn)
		progname = pn;
}

char *
Py_GetProgramName()
{
	return progname;
}

/* Create __main__ module */

static void
initmain()
{
	PyObject *m, *d;
	m = PyImport_AddModule("__main__");
	if (m == NULL)
		Py_FatalError("can't create __main__ module");
	d = PyModule_GetDict(m);
	if (PyDict_GetItemString(d, "__builtins__") == NULL) {
		if (PyDict_SetItemString(d, "__builtins__",
					 PyEval_GetBuiltins()))
			Py_FatalError("can't add __builtins__ to __main__");
	}
}

/* Import the site module (not into __main__ though) */

static void
initsite()
{
	PyObject *m, *f;
	m = PyImport_ImportModule("site");
	if (m == NULL) {
		f = PySys_GetObject("stderr");
		if (Py_VerboseFlag) {
			PyFile_WriteString(
				"'import site' failed; traceback:\n", f);
			PyErr_Print();
		}
		else {
			PyFile_WriteString(
			  "'import site' failed; use -v for traceback\n", f);
			PyErr_Clear();
		}
	}
	else {
		Py_DECREF(m);
	}
}

/* Parse input from a file and execute it */

int
PyRun_AnyFile(fp, filename)
	FILE *fp;
	char *filename;
{
	if (filename == NULL)
		filename = "???";
	if (Py_FdIsInteractive(fp, filename))
		return PyRun_InteractiveLoop(fp, filename);
	else
		return PyRun_SimpleFile(fp, filename);
}

int
PyRun_InteractiveLoop(fp, filename)
	FILE *fp;
	char *filename;
{
	PyObject *v;
	int ret;
	v = PySys_GetObject("ps1");
	if (v == NULL) {
		PySys_SetObject("ps1", v = PyString_FromString(">>> "));
		Py_XDECREF(v);
	}
	v = PySys_GetObject("ps2");
	if (v == NULL) {
		PySys_SetObject("ps2", v = PyString_FromString("... "));
		Py_XDECREF(v);
	}
	for (;;) {
		ret = PyRun_InteractiveOne(fp, filename);
#ifdef Py_REF_DEBUG
		fprintf(stderr, "[%ld refs]\n", _Py_RefTotal);
#endif
		if (ret == E_EOF)
			return 0;
		/*
		if (ret == E_NOMEM)
			return -1;
		*/
	}
}

int
PyRun_InteractiveOne(fp, filename)
	FILE *fp;
	char *filename;
{
	PyObject *m, *d, *v, *w;
	node *n;
	perrdetail err;
	char *ps1, *ps2;
	v = PySys_GetObject("ps1");
	w = PySys_GetObject("ps2");
	if (v != NULL && PyString_Check(v)) {
		Py_INCREF(v);
		ps1 = PyString_AsString(v);
	}
	else {
		v = NULL;
		ps1 = "";
	}
	if (w != NULL && PyString_Check(w)) {
		Py_INCREF(w);
		ps2 = PyString_AsString(w);
	}
	else {
		w = NULL;
		ps2 = "";
	}
	Py_BEGIN_ALLOW_THREADS
	n = PyParser_ParseFile(fp, filename, &_PyParser_Grammar,
			       Py_single_input, ps1, ps2, &err);
	Py_END_ALLOW_THREADS
	Py_XDECREF(v);
	Py_XDECREF(w);
	if (n == NULL) {
		if (err.error == E_EOF) {
			if (err.text)
				free(err.text);
			return E_EOF;
		}
		err_input(&err);
		PyErr_Print();
		return err.error;
	}
	m = PyImport_AddModule("__main__");
	if (m == NULL)
		return -1;
	d = PyModule_GetDict(m);
	v = run_node(n, filename, d, d);
	if (v == NULL) {
		PyErr_Print();
		return -1;
	}
	Py_DECREF(v);
	if (Py_FlushLine())
		PyErr_Clear();
	return 0;
}

int
PyRun_SimpleFile(fp, filename)
	FILE *fp;
	char *filename;
{
	PyObject *m, *d, *v;
	char *ext;

	m = PyImport_AddModule("__main__");
	if (m == NULL)
		return -1;
	d = PyModule_GetDict(m);
	ext = filename + strlen(filename) - 4;
	if (strcmp(ext, ".pyc") == 0 || strcmp(ext, ".pyo") == 0
#ifdef macintosh
	/* On a mac, we also assume a pyc file for types 'PYC ' and 'APPL' */
	    || getfiletype(filename) == 'PYC '
	    || getfiletype(filename) == 'APPL'
#endif /* macintosh */
		) {
		/* Try to run a pyc file. First, re-open in binary */
		/* Don't close, done in main: fclose(fp); */
		if( (fp = fopen(filename, "rb")) == NULL ) {
			fprintf(stderr, "python: Can't reopen .pyc file\n");
			return -1;
		}
		/* Turn on optimization if a .pyo file is given */
		if (strcmp(ext, ".pyo") == 0)
			Py_OptimizeFlag = 1;
		v = run_pyc_file(fp, filename, d, d);
	} else {
		v = PyRun_File(fp, filename, Py_file_input, d, d);
	}
	if (v == NULL) {
		PyErr_Print();
		return -1;
	}
	Py_DECREF(v);
	if (Py_FlushLine())
		PyErr_Clear();
	return 0;
}

int
PyRun_SimpleString(command)
	char *command;
{
	PyObject *m, *d, *v;
	m = PyImport_AddModule("__main__");
	if (m == NULL)
		return -1;
	d = PyModule_GetDict(m);
	v = PyRun_String(command, Py_file_input, d, d);
	if (v == NULL) {
		PyErr_Print();
		return -1;
	}
	Py_DECREF(v);
	if (Py_FlushLine())
		PyErr_Clear();
	return 0;
}

static int
parse_syntax_error(err, message, filename, lineno, offset, text)
     PyObject* err;
     PyObject** message;
     char** filename;
     int* lineno;
     int* offset;
     char** text;
{
	long hold;
	PyObject *v;

	/* old style errors */
	if (PyTuple_Check(err))
		return PyArg_Parse(err, "(O(ziiz))", message, filename,
				   lineno, offset, text);

	/* new style errors.  `err' is an instance */

	if (! (v = PyObject_GetAttrString(err, "msg")))
		goto finally;
	*message = v;

	if (!(v = PyObject_GetAttrString(err, "filename")))
		goto finally;
	if (v == Py_None)
		*filename = NULL;
	else if (! (*filename = PyString_AsString(v)))
		goto finally;

	Py_DECREF(v);
	if (!(v = PyObject_GetAttrString(err, "lineno")))
		goto finally;
	hold = PyInt_AsLong(v);
	Py_DECREF(v);
	v = NULL;
	if (hold < 0 && PyErr_Occurred())
		goto finally;
	*lineno = (int)hold;

	if (!(v = PyObject_GetAttrString(err, "offset")))
		goto finally;
	hold = PyInt_AsLong(v);
	Py_DECREF(v);
	v = NULL;
	if (hold < 0 && PyErr_Occurred())
		goto finally;
	*offset = (int)hold;

	if (!(v = PyObject_GetAttrString(err, "text")))
		goto finally;
	if (v == Py_None)
		*text = NULL;
	else if (! (*text = PyString_AsString(v)))
		goto finally;
	Py_DECREF(v);
	return 1;

finally:
	Py_XDECREF(v);
	return 0;
}

void
PyErr_Print()
{
	int err = 0;
	PyObject *exception, *v, *tb, *f;
	PyErr_Fetch(&exception, &v, &tb);
	PyErr_NormalizeException(&exception, &v, &tb);

	if (exception == NULL)
		return;

	if (PyErr_GivenExceptionMatches(exception, PyExc_SystemExit)) {
		err = Py_FlushLine();
		fflush(stdout);
		if (v == NULL || v == Py_None)
			Py_Exit(0);
		if (PyInstance_Check(v)) {
			/* we expect the error code to be store in the
			   `code' attribute
			*/
			PyObject *code = PyObject_GetAttrString(v, "code");
			if (code) {
				Py_DECREF(v);
				v = code;
				if (v == Py_None)
					Py_Exit(0);
			}
			/* if we failed to dig out the "code" attribute,
			   then just let the else clause below print the
			   error
			*/
		}
		if (PyInt_Check(v))
			Py_Exit((int)PyInt_AsLong(v));
		else {
			/* OK to use real stderr here */
			PyObject_Print(v, stderr, Py_PRINT_RAW);
			fprintf(stderr, "\n");
			Py_Exit(1);
		}
	}
	PySys_SetObject("last_type", exception);
	PySys_SetObject("last_value", v);
	PySys_SetObject("last_traceback", tb);
	f = PySys_GetObject("stderr");
	if (f == NULL)
		fprintf(stderr, "lost sys.stderr\n");
	else {
		err = Py_FlushLine();
		fflush(stdout);
		if (err == 0)
			err = PyTraceBack_Print(tb, f);
		if (err == 0 &&
		    PyErr_GivenExceptionMatches(exception, PyExc_SyntaxError))
		{
			PyObject *message;
			char *filename, *text;
			int lineno, offset;
			if (!parse_syntax_error(v, &message, &filename,
						&lineno, &offset, &text))
				PyErr_Clear();
			else {
				char buf[10];
				PyFile_WriteString("  File \"", f);
				if (filename == NULL)
					PyFile_WriteString("<string>", f);
				else
					PyFile_WriteString(filename, f);
				PyFile_WriteString("\", line ", f);
				sprintf(buf, "%d", lineno);
				PyFile_WriteString(buf, f);
				PyFile_WriteString("\n", f);
				if (text != NULL) {
					char *nl;
					if (offset > 0 &&
					    offset == (int)strlen(text))
						offset--;
					for (;;) {
						nl = strchr(text, '\n');
						if (nl == NULL ||
						    nl-text >= offset)
							break;
						offset -= (nl+1-text);
						text = nl+1;
					}
					while (*text == ' ' || *text == '\t') {
						text++;
						offset--;
					}
					PyFile_WriteString("    ", f);
					PyFile_WriteString(text, f);
					if (*text == '\0' ||
					    text[strlen(text)-1] != '\n')
						PyFile_WriteString("\n", f);
					PyFile_WriteString("    ", f);
					offset--;
					while (offset > 0) {
						PyFile_WriteString(" ", f);
						offset--;
					}
					PyFile_WriteString("^\n", f);
				}
				Py_INCREF(message);
				Py_DECREF(v);
				v = message;
				/* Can't be bothered to check all those
				   PyFile_WriteString() calls */
				if (PyErr_Occurred())
					err = -1;
			}
		}
		if (err) {
			/* Don't do anything else */
		}
		else if (PyClass_Check(exception)) {
			PyClassObject* exc = (PyClassObject*)exception;
			PyObject* className = exc->cl_name;
			PyObject* moduleName =
			      PyDict_GetItemString(exc->cl_dict, "__module__");

			if (moduleName == NULL)
				err = PyFile_WriteString("<unknown>", f);
			else {
				char* modstr = PyString_AsString(moduleName);
				if (modstr && strcmp(modstr, "exceptions")) 
				{
					err = PyFile_WriteString(modstr, f);
					err += PyFile_WriteString(".", f);
				}
			}
			if (err == 0) {
				if (className == NULL)
				      err = PyFile_WriteString("<unknown>", f);
				else
				      err = PyFile_WriteObject(className, f,
							       Py_PRINT_RAW);
			}
		}
		else
			err = PyFile_WriteObject(exception, f, Py_PRINT_RAW);
		if (err == 0) {
			if (v != NULL && v != Py_None) {
				PyObject *s = PyObject_Str(v);
				/* only print colon if the str() of the
				   object is not the empty string
				*/
				if (s == NULL)
					err = -1;
				else if (!PyString_Check(s) ||
					 PyString_GET_SIZE(s) != 0)
					err = PyFile_WriteString(": ", f);
				if (err == 0)
				  err = PyFile_WriteObject(s, f, Py_PRINT_RAW);
				Py_XDECREF(s);
			}
		}
		if (err == 0)
			err = PyFile_WriteString("\n", f);
	}
	Py_XDECREF(exception);
	Py_XDECREF(v);
	Py_XDECREF(tb);
	/* If an error happened here, don't show it.
	   XXX This is wrong, but too many callers rely on this behavior. */
	if (err != 0)
		PyErr_Clear();
}

PyObject *
PyRun_String(str, start, globals, locals)
	char *str;
	int start;
	PyObject *globals, *locals;
{
	return run_err_node(PyParser_SimpleParseString(str, start),
			    "<string>", globals, locals);
}

PyObject *
PyRun_File(fp, filename, start, globals, locals)
	FILE *fp;
	char *filename;
	int start;
	PyObject *globals, *locals;
{
	return run_err_node(PyParser_SimpleParseFile(fp, filename, start),
			    filename, globals, locals);
}

static PyObject *
run_err_node(n, filename, globals, locals)
	node *n;
	char *filename;
	PyObject *globals, *locals;
{
	if (n == NULL)
		return  NULL;
	return run_node(n, filename, globals, locals);
}

static PyObject *
run_node(n, filename, globals, locals)
	node *n;
	char *filename;
	PyObject *globals, *locals;
{
	PyCodeObject *co;
	PyObject *v;
	co = PyNode_Compile(n, filename);
	PyNode_Free(n);
	if (co == NULL)
		return NULL;
	v = PyEval_EvalCode(co, globals, locals);
	Py_DECREF(co);
	return v;
}

static PyObject *
run_pyc_file(fp, filename, globals, locals)
	FILE *fp;
	char *filename;
	PyObject *globals, *locals;
{
	PyCodeObject *co;
	PyObject *v;
	long magic;
	long PyImport_GetMagicNumber();

	magic = PyMarshal_ReadLongFromFile(fp);
	if (magic != PyImport_GetMagicNumber()) {
		PyErr_SetString(PyExc_RuntimeError,
			   "Bad magic number in .pyc file");
		return NULL;
	}
	(void) PyMarshal_ReadLongFromFile(fp);
	v = PyMarshal_ReadObjectFromFile(fp);
	fclose(fp);
	if (v == NULL || !PyCode_Check(v)) {
		Py_XDECREF(v);
		PyErr_SetString(PyExc_RuntimeError,
			   "Bad code object in .pyc file");
		return NULL;
	}
	co = (PyCodeObject *)v;
	v = PyEval_EvalCode(co, globals, locals);
	Py_DECREF(co);
	return v;
}

PyObject *
Py_CompileString(str, filename, start)
	char *str;
	char *filename;
	int start;
{
	node *n;
	PyCodeObject *co;
	n = PyParser_SimpleParseString(str, start);
	if (n == NULL)
		return NULL;
	co = PyNode_Compile(n, filename);
	PyNode_Free(n);
	return (PyObject *)co;
}

/* Simplified interface to parsefile -- return node or set exception */

node *
PyParser_SimpleParseFile(fp, filename, start)
	FILE *fp;
	char *filename;
	int start;
{
	node *n;
	perrdetail err;
	Py_BEGIN_ALLOW_THREADS
	n = PyParser_ParseFile(fp, filename, &_PyParser_Grammar, start,
				(char *)0, (char *)0, &err);
	Py_END_ALLOW_THREADS
	if (n == NULL)
		err_input(&err);
	return n;
}

/* Simplified interface to parsestring -- return node or set exception */

node *
PyParser_SimpleParseString(str, start)
	char *str;
	int start;
{
	node *n;
	perrdetail err;
	n = PyParser_ParseString(str, &_PyParser_Grammar, start, &err);
	if (n == NULL)
		err_input(&err);
	return n;
}

/* Set the error appropriate to the given input error code (see errcode.h) */

static void
err_input(err)
	perrdetail *err;
{
	PyObject *v, *w;
	char *msg = NULL;
	v = Py_BuildValue("(ziiz)", err->filename,
			    err->lineno, err->offset, err->text);
	if (err->text != NULL) {
		free(err->text);
		err->text = NULL;
	}
	switch (err->error) {
	case E_SYNTAX:
		msg = "invalid syntax";
		break;
	case E_TOKEN:
		msg = "invalid token";

		break;
	case E_INTR:
		PyErr_SetNone(PyExc_KeyboardInterrupt);
		return;
	case E_NOMEM:
		PyErr_NoMemory();
		return;
	case E_EOF:
		msg = "unexpected EOF while parsing";
		break;
	default:
		fprintf(stderr, "error=%d\n", err->error);
		msg = "unknown parsing error";
		break;
	}
	w = Py_BuildValue("(sO)", msg, v);
	Py_XDECREF(v);
	PyErr_SetObject(PyExc_SyntaxError, w);
	Py_XDECREF(w);
}

/* Print fatal error message and abort */

void
Py_FatalError(msg)
	char *msg;
{
	fprintf(stderr, "Fatal Python error: %s\n", msg);
#ifdef macintosh
	for (;;);
#endif
#ifdef MS_WIN32
	OutputDebugString("Fatal Python error: ");
	OutputDebugString(msg);
	OutputDebugString("\n");
#endif
	abort();
}

/* Clean up and exit */

#ifdef WITH_THREAD
#include "thread.h"
int _PyThread_Started = 0; /* Set by threadmodule.c and maybe others */
#endif

#define NEXITFUNCS 32
static void (*exitfuncs[NEXITFUNCS])();
static int nexitfuncs = 0;

int Py_AtExit(func)
	void (*func) Py_PROTO((void));
{
	if (nexitfuncs >= NEXITFUNCS)
		return -1;
	exitfuncs[nexitfuncs++] = func;
	return 0;
}

static void
call_sys_exitfunc()
{
	PyObject *exitfunc = PySys_GetObject("exitfunc");

	if (exitfunc) {
		PyObject *res;
		Py_INCREF(exitfunc);
		PySys_SetObject("exitfunc", (PyObject *)NULL);
		res = PyEval_CallObject(exitfunc, (PyObject *)NULL);
		if (res == NULL) {
			fprintf(stderr, "Error in sys.exitfunc:\n");
			PyErr_Print();
		}
		Py_DECREF(exitfunc);
	}

	Py_FlushLine();
}

static void
call_ll_exitfuncs()
{
	while (nexitfuncs > 0)
		(*exitfuncs[--nexitfuncs])();

	fflush(stdout);
	fflush(stderr);
}

#ifdef COUNT_ALLOCS
extern void dump_counts Py_PROTO((void));
#endif

void
Py_Exit(sts)
	int sts;
{
	Py_Finalize();

#ifdef macintosh
	PyMac_Exit(sts);
#else
	exit(sts);
#endif
}

#ifdef HAVE_SIGNAL_H
static RETSIGTYPE
sighandler(sig)
	int sig;
{
	signal(sig, SIG_DFL); /* Don't catch recursive signals */
	/* Do essential exit processing only */
	call_sys_exitfunc();
	call_ll_exitfuncs();
#ifdef HAVE_KILL
	kill(getpid(), sig); /* Pretend the signal killed us */
#else
	exit(1);
#endif
	/*NOTREACHED*/
}
#endif

static void
initsigs()
{
	RETSIGTYPE (*t)();
#ifdef HAVE_SIGNAL_H
#ifdef SIGPIPE
	signal(SIGPIPE, SIG_IGN);
#endif
#ifdef SIGHUP
	t = signal(SIGHUP, SIG_IGN);
	if (t == SIG_DFL)
		signal(SIGHUP, sighandler);
	else
		signal(SIGHUP, t);
#endif              
#ifdef SIGTERM
	t = signal(SIGTERM, SIG_IGN);
	if (t == SIG_DFL)
		signal(SIGTERM, sighandler);
	else
		signal(SIGTERM, t);
#endif
#endif /* HAVE_SIGNAL_H */
	PyOS_InitInterrupts(); /* May imply initsignal() */
}

#ifdef Py_TRACE_REFS
/* Ask a yes/no question */

int
_Py_AskYesNo(prompt)
	char *prompt;
{
	char buf[256];
	
	printf("%s [ny] ", prompt);
	if (fgets(buf, sizeof buf, stdin) == NULL)
		return 0;
	return buf[0] == 'y' || buf[0] == 'Y';
}
#endif

#ifdef MPW

/* Check for file descriptor connected to interactive device.
   Pretend that stdin is always interactive, other files never. */

int
isatty(fd)
	int fd;
{
	return fd == fileno(stdin);
}

#endif

/*
 * The file descriptor fd is considered ``interactive'' if either
 *   a) isatty(fd) is TRUE, or
 *   b) the -i flag was given, and the filename associated with
 *      the descriptor is NULL or "<stdin>" or "???".
 */
int
Py_FdIsInteractive(fp, filename)
	FILE *fp;
	char *filename;
{
	if (isatty((int)fileno(fp)))
		return 1;
	if (!Py_InteractiveFlag)
		return 0;
	return (filename == NULL) ||
	       (strcmp(filename, "<stdin>") == 0) ||
	       (strcmp(filename, "???") == 0);
}
