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
#undef argument /* Avoid conflict on Mac */
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
#undef arglist
#include "windows.h"
#endif

extern char *Py_GetPath();

extern grammar _PyParser_Grammar; /* From graminit.c */

/* Forward */
static void initmain Py_PROTO((void));
static PyObject *run_err_node Py_PROTO((node *n, char *filename,
				   PyObject *globals, PyObject *locals));
static PyObject *run_node Py_PROTO((node *n, char *filename,
			       PyObject *globals, PyObject *locals));
static PyObject *run_pyc_file Py_PROTO((FILE *fp, char *filename,
				   PyObject *globals, PyObject *locals));
static void err_input Py_PROTO((perrdetail *));
static void initsigs Py_PROTO((void));

int Py_DebugFlag; /* Needed by parser.c */
int Py_VerboseFlag; /* Needed by import.c */
int Py_InteractiveFlag; /* Needed by Py_FdIsInteractive() below */

/* Initialize the current interpreter; pass in the Python path. */

void
Py_Setup()
{
	PyImport_Init();
	
	/* Modules '__builtin__' and 'sys' are initialized here,
	   they are needed by random bits of the interpreter.
	   All other modules are optional and are initialized
	   when they are first imported. */
	
	PyBuiltin_Init(); /* Also initializes builtin exceptions */
	PySys_Init();

	PySys_SetPath(Py_GetPath());

	initsigs(); /* Signal handling stuff, including initintr() */

	initmain();
}

/* Create and interpreter and thread state and initialize them;
   if we already have an interpreter and thread, do nothing.
   Fatal error if the creation fails. */

void
Py_Initialize()
{
	PyThreadState *tstate;
	PyInterpreterState *interp;
	char *p;
	
	if (PyThreadState_Get())
		return;

	if ((p = getenv("PYTHONDEBUG")) && *p != '\0')
		Py_DebugFlag = 1;
	if ((p = getenv("PYTHONVERBOSE")) && *p != '\0')
		Py_VerboseFlag = 1;

	interp = PyInterpreterState_New();
	if (interp == NULL)
		Py_FatalError("PyInterpreterState_New() failed");

	tstate = PyThreadState_New(interp);
	if (tstate == NULL)
		Py_FatalError("PyThreadState_New() failed");
	(void) PyThreadState_Swap(tstate);

	Py_Setup();

	PySys_SetPath(Py_GetPath());
	/* XXX Who should set the path -- Setup or Initialize? */
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

/*
  Py_Initialize()
  -- do everything, no-op on second call, call fatal on failure, set path

  #2
  -- create new interp+tstate & make it current, return NULL on failure,
     make it current, do all setup, set path

  #3
  -- #2 without set path

  #4
  -- is there any point to #3 for caller-provided current interp+tstate?

*/

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

void
PyErr_Print()
{
	int err = 0;
	PyObject *exception, *v, *tb, *f;
	PyErr_Fetch(&exception, &v, &tb);
	if (exception == NULL)
		return;
	if (exception == PyExc_SystemExit) {
		err = Py_FlushLine();
		fflush(stdout);
		if (v == NULL || v == Py_None)
			Py_Exit(0);
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
		if (err == 0 && exception == PyExc_SyntaxError) {
			PyObject *message;
			char *filename, *text;
			int lineno, offset;
			if (!PyArg_Parse(v, "(O(ziiz))", &message,
				     &filename, &lineno, &offset, &text))
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
			PyObject* className =
				((PyClassObject*)exception)->cl_name;
			if (className == NULL)
				err = PyFile_WriteString("<unknown>", f);
			else
				err = PyFile_WriteObject(className, f,
							 Py_PRINT_RAW);
		}
		else
			err = PyFile_WriteObject(exception, f, Py_PRINT_RAW);
		if (err == 0) {
			if (v != NULL && v != Py_None) {
				err = PyFile_WriteString(": ", f);
				if (err == 0)
				  err = PyFile_WriteObject(v, f, Py_PRINT_RAW);
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

void
Py_Cleanup()
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

	while (nexitfuncs > 0)
		(*exitfuncs[--nexitfuncs])();
}

#ifdef COUNT_ALLOCS
extern void dump_counts Py_PROTO((void));
#endif

void
Py_Exit(sts)
	int sts;
{
	Py_Cleanup();

#ifdef COUNT_ALLOCS
	dump_counts();
#endif

#ifdef WITH_THREAD

	/* Other threads may still be active, so skip most of the
	   cleanup actions usually done (these are mostly for
	   debugging anyway). */
	
	(void) PyEval_SaveThread();
#ifndef NO_EXIT_PROG
	if (_PyThread_Started)
		_exit_prog(sts);
	else
		exit_prog(sts);
#else /* !NO_EXIT_PROG */
	if (_PyThread_Started)
		_exit(sts);
	else
		exit(sts);
#endif /* !NO_EXIT_PROG */
	
#else /* WITH_THREAD */
	
	PyImport_Cleanup();
	
	PyErr_Clear();

#ifdef Py_REF_DEBUG
	fprintf(stderr, "[%ld refs]\n", _Py_RefTotal);
#endif

#ifdef Py_TRACE_REFS
	if (_Py_AskYesNo("Print left references?")) {
		_Py_PrintReferences(stderr);
	}
#endif /* Py_TRACE_REFS */

#ifdef macintosh
	PyMac_Exit(sts);
#else
	exit(sts);
#endif
#endif /* WITH_THREAD */
	/*NOTREACHED*/
}

#ifdef HAVE_SIGNAL_H
static RETSIGTYPE
sighandler(sig)
	int sig;
{
	signal(sig, SIG_DFL); /* Don't catch recursive signals */
	Py_Cleanup(); /* Do essential clean-up */
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
