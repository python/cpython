
/* Python interpreter top-level routines, including init/exit */

#include "Python.h"

#include "grammar.h"
#include "node.h"
#include "token.h"
#include "parsetok.h"
#include "errcode.h"
#include "compile.h"
#include "symtable.h"
#include "eval.h"
#include "marshal.h"

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef MS_WIN32
#undef BYTE
#include "windows.h"
#endif

#ifdef macintosh
#include "macglue.h"
#endif
extern char *Py_GetPath(void);

extern grammar _PyParser_Grammar; /* From graminit.c */

/* Forward */
static void initmain(void);
static void initsite(void);
static PyObject *run_err_node(node *, char *, PyObject *, PyObject *,
			      PyCompilerFlags *);
static PyObject *run_node(node *, char *, PyObject *, PyObject *,
			  PyCompilerFlags *);
static PyObject *run_pyc_file(FILE *, char *, PyObject *, PyObject *,
			      PyCompilerFlags *);
static void err_input(perrdetail *);
static void initsigs(void);
static void call_sys_exitfunc(void);
static void call_ll_exitfuncs(void);

#ifdef Py_TRACE_REFS
int _Py_AskYesNo(char *prompt);
#endif

extern void _PyUnicode_Init(void);
extern void _PyUnicode_Fini(void);
extern void _PyCodecRegistry_Init(void);
extern void _PyCodecRegistry_Fini(void);

int Py_DebugFlag; /* Needed by parser.c */
int Py_VerboseFlag; /* Needed by import.c */
int Py_InteractiveFlag; /* Needed by Py_FdIsInteractive() below */
int Py_NoSiteFlag; /* Suppress 'import site' */
int Py_UseClassExceptionsFlag = 1; /* Needed by bltinmodule.c: deprecated */
int Py_FrozenFlag; /* Needed by getpath.c */
int Py_UnicodeFlag = 0; /* Needed by compile.c */
int Py_IgnoreEnvironmentFlag; /* e.g. PYTHONPATH, PYTHONHOME */
/* _XXX Py_QnewFlag should go away in 2.3.  It's true iff -Qnew is passed,
  on the command line, and is used in 2.2 by ceval.c to make all "/" divisions
  true divisions (which they will be in 2.3). */
int _Py_QnewFlag = 0;

static int initialized = 0;

/* API to access the initialized flag -- useful for esoteric use */

int
Py_IsInitialized(void)
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

static int
add_flag(int flag, const char *envs)
{
	int env = atoi(envs);
	if (flag < env)
		flag = env;
	if (flag < 1)
		flag = 1;
	return flag;
}

void
Py_Initialize(void)
{
	PyInterpreterState *interp;
	PyThreadState *tstate;
	PyObject *bimod, *sysmod;
	char *p;
	extern void _Py_ReadyTypes(void);

	if (initialized)
		return;
	initialized = 1;
	
	if ((p = Py_GETENV("PYTHONDEBUG")) && *p != '\0')
		Py_DebugFlag = add_flag(Py_DebugFlag, p);
	if ((p = Py_GETENV("PYTHONVERBOSE")) && *p != '\0')
		Py_VerboseFlag = add_flag(Py_VerboseFlag, p);
	if ((p = Py_GETENV("PYTHONOPTIMIZE")) && *p != '\0')
		Py_OptimizeFlag = add_flag(Py_OptimizeFlag, p);

	interp = PyInterpreterState_New();
	if (interp == NULL)
		Py_FatalError("Py_Initialize: can't make first interpreter");

	tstate = PyThreadState_New(interp);
	if (tstate == NULL)
		Py_FatalError("Py_Initialize: can't make first thread");
	(void) PyThreadState_Swap(tstate);

	_Py_ReadyTypes();

	interp->modules = PyDict_New();
	if (interp->modules == NULL)
		Py_FatalError("Py_Initialize: can't make modules dictionary");

	/* Init codec registry */
	_PyCodecRegistry_Init();

#ifdef Py_USING_UNICODE
	/* Init Unicode implementation; relies on the codec registry */
	_PyUnicode_Init();
#endif

	bimod = _PyBuiltin_Init();
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

	_PyImport_Init();

	/* initialize builtin exceptions */
	_PyExc_Init();
	_PyImport_FixupExtension("exceptions", "exceptions");

	/* phase 2 of builtins */
	_PyImport_FixupExtension("__builtin__", "__builtin__");

	initsigs(); /* Signal handling stuff, including initintr() */

	initmain(); /* Module __main__ */
	if (!Py_NoSiteFlag)
		initsite(); /* Module site */
}

#ifdef COUNT_ALLOCS
extern void dump_counts(void);
#endif

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
Py_Finalize(void)
{
	PyInterpreterState *interp;
	PyThreadState *tstate;

	if (!initialized)
		return;

	/* The interpreter is still entirely intact at this point, and the
	 * exit funcs may be relying on that.  In particular, if some thread
	 * or exit func is still waiting to do an import, the import machinery
	 * expects Py_IsInitialized() to return true.  So don't say the
	 * interpreter is uninitialized until after the exit funcs have run.
	 * Note that Threading.py uses an exit func to do a join on all the
	 * threads created thru it, so this also protects pending imports in
	 * the threads created via Threading.
	 */
	call_sys_exitfunc();
	initialized = 0;

	/* Get current thread state and interpreter pointer */
	tstate = PyThreadState_Get();
	interp = tstate->interp;

	/* Disable signal handling */
	PyOS_FiniInterrupts();

	/* Cleanup Codec registry */
	_PyCodecRegistry_Fini();

	/* Destroy all modules */
	PyImport_Cleanup();

	/* Destroy the database used by _PyImport_{Fixup,Find}Extension */
	_PyImport_Fini();

	/* Debugging stuff */
#ifdef COUNT_ALLOCS
	dump_counts();
#endif

#ifdef Py_REF_DEBUG
	fprintf(stderr, "[%ld refs]\n", _Py_RefTotal);
#endif

#ifdef Py_TRACE_REFS
	if (Py_GETENV("PYTHONDUMPREFS")) {
		_Py_PrintReferences(stderr);
	}
#endif /* Py_TRACE_REFS */

	/* Now we decref the exception classes.  After this point nothing
	   can raise an exception.  That's okay, because each Fini() method
	   below has been checked to make sure no exceptions are ever
	   raised.
	*/
	_PyExc_Fini();

	/* Delete current thread */
	PyInterpreterState_Clear(interp);
	PyThreadState_Swap(NULL);
	PyInterpreterState_Delete(interp);

	PyMethod_Fini();
	PyFrame_Fini();
	PyCFunction_Fini();
	PyTuple_Fini();
	PyString_Fini();
	PyInt_Fini();
	PyFloat_Fini();

#ifdef Py_USING_UNICODE
	/* Cleanup Unicode implementation */
	_PyUnicode_Fini();
#endif

	/* XXX Still allocated:
	   - various static ad-hoc pointers to interned strings
	   - int and float free list blocks
	   - whatever various modules and libraries allocate
	*/

	PyGrammar_RemoveAccelerators(&_PyParser_Grammar);

#ifdef PYMALLOC_DEBUG
	if (Py_GETENV("PYTHONMALLOCSTATS"))
		_PyObject_DebugMallocStats();
#endif

	call_ll_exitfuncs();

#ifdef Py_TRACE_REFS
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
Py_NewInterpreter(void)
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
Py_EndInterpreter(PyThreadState *tstate)
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
Py_SetProgramName(char *pn)
{
	if (pn && *pn)
		progname = pn;
}

char *
Py_GetProgramName(void)
{
	return progname;
}

static char *default_home = NULL;

void
Py_SetPythonHome(char *home)
{
	default_home = home;
}

char *
Py_GetPythonHome(void)
{
	char *home = default_home;
	if (home == NULL && !Py_IgnoreEnvironmentFlag)
		home = Py_GETENV("PYTHONHOME");
	return home;
}

/* Create __main__ module */

static void
initmain(void)
{
	PyObject *m, *d;
	m = PyImport_AddModule("__main__");
	if (m == NULL)
		Py_FatalError("can't create __main__ module");
	d = PyModule_GetDict(m);
	if (PyDict_GetItemString(d, "__builtins__") == NULL) {
		PyObject *bimod = PyImport_ImportModule("__builtin__");
		if (bimod == NULL ||
		    PyDict_SetItemString(d, "__builtins__", bimod) != 0)
			Py_FatalError("can't add __builtins__ to __main__");
		Py_DECREF(bimod);
	}
}

/* Import the site module (not into __main__ though) */

static void
initsite(void)
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
PyRun_AnyFile(FILE *fp, char *filename)
{
	return PyRun_AnyFileExFlags(fp, filename, 0, NULL);
}

int
PyRun_AnyFileFlags(FILE *fp, char *filename, PyCompilerFlags *flags)
{
	return PyRun_AnyFileExFlags(fp, filename, 0, flags);
}

int
PyRun_AnyFileEx(FILE *fp, char *filename, int closeit)
{
	return PyRun_AnyFileExFlags(fp, filename, closeit, NULL);
}

int
PyRun_AnyFileExFlags(FILE *fp, char *filename, int closeit, 
		     PyCompilerFlags *flags)
{
	if (filename == NULL)
		filename = "???";
	if (Py_FdIsInteractive(fp, filename)) {
		int err = PyRun_InteractiveLoopFlags(fp, filename, flags);
		if (closeit)
			fclose(fp);
		return err;
	}
	else
		return PyRun_SimpleFileExFlags(fp, filename, closeit, flags);
}

int
PyRun_InteractiveLoop(FILE *fp, char *filename)
{
	return PyRun_InteractiveLoopFlags(fp, filename, NULL);
}

int
PyRun_InteractiveLoopFlags(FILE *fp, char *filename, PyCompilerFlags *flags)
{
	PyObject *v;
	int ret;
	PyCompilerFlags local_flags;

	if (flags == NULL) {
		flags = &local_flags;
		local_flags.cf_flags = 0;
	}
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
		ret = PyRun_InteractiveOneFlags(fp, filename, flags);
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
PyRun_InteractiveOne(FILE *fp, char *filename)
{
	return PyRun_InteractiveOneFlags(fp, filename, NULL);
}

/* compute parser flags based on compiler flags */
#if 0 /* future keyword */
#define PARSER_FLAGS(flags) \
	(((flags) && (flags)->cf_flags & CO_GENERATOR_ALLOWED) ? \
		PyPARSE_YIELD_IS_KEYWORD : 0)
#else
#define PARSER_FLAGS(flags) 0
#endif

int
PyRun_InteractiveOneFlags(FILE *fp, char *filename, PyCompilerFlags *flags)
{
	PyObject *m, *d, *v, *w;
	node *n;
	perrdetail err;
	char *ps1 = "", *ps2 = "";

	v = PySys_GetObject("ps1");
	if (v != NULL) {
		v = PyObject_Str(v);
		if (v == NULL)
			PyErr_Clear();
		else if (PyString_Check(v))
			ps1 = PyString_AsString(v);
	}
	w = PySys_GetObject("ps2");
	if (w != NULL) {
		w = PyObject_Str(w);
		if (w == NULL)
			PyErr_Clear();
		else if (PyString_Check(w))
			ps2 = PyString_AsString(w);
	}
	n = PyParser_ParseFileFlags(fp, filename, &_PyParser_Grammar,
			    	    Py_single_input, ps1, ps2, &err,
			    	    PARSER_FLAGS(flags));
	Py_XDECREF(v);
	Py_XDECREF(w);
	if (n == NULL) {
		if (err.error == E_EOF) {
			if (err.text)
				PyMem_DEL(err.text);
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
	v = run_node(n, filename, d, d, flags);
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
PyRun_SimpleFile(FILE *fp, char *filename)
{
	return PyRun_SimpleFileEx(fp, filename, 0);
}

/* Check whether a file maybe a pyc file: Look at the extension,
   the file type, and, if we may close it, at the first few bytes. */

static int
maybe_pyc_file(FILE *fp, char* filename, char* ext, int closeit)
{
	if (strcmp(ext, ".pyc") == 0 || strcmp(ext, ".pyo") == 0)
		return 1;

#ifdef macintosh
	/* On a mac, we also assume a pyc file for types 'PYC ' and 'APPL' */
	if (PyMac_getfiletype(filename) == 'PYC '
	    || PyMac_getfiletype(filename) == 'APPL')
		return 1;
#endif /* macintosh */

	/* Only look into the file if we are allowed to close it, since
	   it then should also be seekable. */
	if (closeit) {
		/* Read only two bytes of the magic. If the file was opened in
		   text mode, the bytes 3 and 4 of the magic (\r\n) might not
		   be read as they are on disk. */
		unsigned int halfmagic = PyImport_GetMagicNumber() & 0xFFFF;
		unsigned char buf[2];
		/* Mess:  In case of -x, the stream is NOT at its start now,
		   and ungetc() was used to push back the first newline,
		   which makes the current stream position formally undefined,
		   and a x-platform nightmare.
		   Unfortunately, we have no direct way to know whether -x
		   was specified.  So we use a terrible hack:  if the current
		   stream position is not 0, we assume -x was specified, and
		   give up.  Bug 132850 on SourceForge spells out the
		   hopelessness of trying anything else (fseek and ftell
		   don't work predictably x-platform for text-mode files).
		*/
		int ispyc = 0;
		if (ftell(fp) == 0) {
			if (fread(buf, 1, 2, fp) == 2 &&
			    ((unsigned int)buf[1]<<8 | buf[0]) == halfmagic) 
				ispyc = 1;
			rewind(fp);
		}
		return ispyc;
	}
	return 0;
} 

int
PyRun_SimpleFileEx(FILE *fp, char *filename, int closeit)
{
	return PyRun_SimpleFileExFlags(fp, filename, closeit, NULL);
}

int
PyRun_SimpleFileExFlags(FILE *fp, char *filename, int closeit,
			PyCompilerFlags *flags)
{
	PyObject *m, *d, *v;
	char *ext;

	m = PyImport_AddModule("__main__");
	if (m == NULL)
		return -1;
	d = PyModule_GetDict(m);
	ext = filename + strlen(filename) - 4;
	if (maybe_pyc_file(fp, filename, ext, closeit)) {
		/* Try to run a pyc file. First, re-open in binary */
		if (closeit)
			fclose(fp);
		if( (fp = fopen(filename, "rb")) == NULL ) {
			fprintf(stderr, "python: Can't reopen .pyc file\n");
			return -1;
		}
		/* Turn on optimization if a .pyo file is given */
		if (strcmp(ext, ".pyo") == 0)
			Py_OptimizeFlag = 1;
		v = run_pyc_file(fp, filename, d, d, flags);
	} else {
		v = PyRun_FileExFlags(fp, filename, Py_file_input, d, d, 
				      closeit, flags);
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
PyRun_SimpleString(char *command)
{
	return PyRun_SimpleStringFlags(command, NULL);
}

int
PyRun_SimpleStringFlags(char *command, PyCompilerFlags *flags)
{
	PyObject *m, *d, *v;
	m = PyImport_AddModule("__main__");
	if (m == NULL)
		return -1;
	d = PyModule_GetDict(m);
	v = PyRun_StringFlags(command, Py_file_input, d, d, flags);
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
parse_syntax_error(PyObject *err, PyObject **message, char **filename,
		   int *lineno, int *offset, char **text)
{
	long hold;
	PyObject *v;

	/* old style errors */
	if (PyTuple_Check(err))
		return PyArg_ParseTuple(err, "O(ziiz)", message, filename,
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
	if (v == Py_None) {
		*offset = -1;
		Py_DECREF(v);
		v = NULL;
	} else {
		hold = PyInt_AsLong(v);
		Py_DECREF(v);
		v = NULL;
		if (hold < 0 && PyErr_Occurred())
			goto finally;
		*offset = (int)hold;
	}

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
PyErr_Print(void)
{
	PyErr_PrintEx(1);
}

static void
print_error_text(PyObject *f, int offset, char *text)
{
	char *nl;
	if (offset >= 0) {
		if (offset > 0 && offset == (int)strlen(text))
			offset--;
		for (;;) {
			nl = strchr(text, '\n');
			if (nl == NULL || nl-text >= offset)
				break;
			offset -= (nl+1-text);
			text = nl+1;
		}
		while (*text == ' ' || *text == '\t') {
			text++;
			offset--;
		}
	}
	PyFile_WriteString("    ", f);
	PyFile_WriteString(text, f);
	if (*text == '\0' || text[strlen(text)-1] != '\n')
		PyFile_WriteString("\n", f);
	if (offset == -1)
		return;
	PyFile_WriteString("    ", f);
	offset--;
	while (offset > 0) {
		PyFile_WriteString(" ", f);
		offset--;
	}
	PyFile_WriteString("^\n", f);
}

static void
handle_system_exit(void)
{
        PyObject *exception, *value, *tb;
	PyErr_Fetch(&exception, &value, &tb);
	if (Py_FlushLine())
		PyErr_Clear();
	fflush(stdout);
	if (value == NULL || value == Py_None)
		Py_Exit(0);
	if (PyInstance_Check(value)) {
		/* The error code should be in the `code' attribute. */
		PyObject *code = PyObject_GetAttrString(value, "code");
		if (code) {
			Py_DECREF(value);
			value = code;
			if (value == Py_None)
				Py_Exit(0);
		}
		/* If we failed to dig out the 'code' attribute,
		   just let the else clause below print the error. */
	}
	if (PyInt_Check(value))
		Py_Exit((int)PyInt_AsLong(value));
	else {
		PyObject_Print(value, stderr, Py_PRINT_RAW);
		PySys_WriteStderr("\n");
		Py_Exit(1);
	}
}

void
PyErr_PrintEx(int set_sys_last_vars)
{
	PyObject *exception, *v, *tb, *hook;

	if (PyErr_ExceptionMatches(PyExc_SystemExit)) {
		handle_system_exit();
	}
	PyErr_Fetch(&exception, &v, &tb);
	PyErr_NormalizeException(&exception, &v, &tb);
	if (exception == NULL)
		return;
	if (set_sys_last_vars) {
		PySys_SetObject("last_type", exception);
		PySys_SetObject("last_value", v);
		PySys_SetObject("last_traceback", tb);
	}
	hook = PySys_GetObject("excepthook");
	if (hook) {
		PyObject *args = Py_BuildValue("(OOO)",
                    exception, v ? v : Py_None, tb ? tb : Py_None);
		PyObject *result = PyEval_CallObject(hook, args);
		if (result == NULL) {
			PyObject *exception2, *v2, *tb2;
			if (PyErr_ExceptionMatches(PyExc_SystemExit)) {
				handle_system_exit();
			}
			PyErr_Fetch(&exception2, &v2, &tb2);
			PyErr_NormalizeException(&exception2, &v2, &tb2);
			if (Py_FlushLine())
				PyErr_Clear();
			fflush(stdout);
			PySys_WriteStderr("Error in sys.excepthook:\n");
			PyErr_Display(exception2, v2, tb2);
			PySys_WriteStderr("\nOriginal exception was:\n");
			PyErr_Display(exception, v, tb);
			Py_XDECREF(exception2);
			Py_XDECREF(v2);
			Py_XDECREF(tb2);
		}
		Py_XDECREF(result);
		Py_XDECREF(args);
	} else {
		PySys_WriteStderr("sys.excepthook is missing\n");
		PyErr_Display(exception, v, tb);
	}
	Py_XDECREF(exception);
	Py_XDECREF(v);
	Py_XDECREF(tb);
}

void PyErr_Display(PyObject *exception, PyObject *value, PyObject *tb)
{
	int err = 0;
	PyObject *v = value;
	PyObject *f = PySys_GetObject("stderr");
	if (f == NULL)
		fprintf(stderr, "lost sys.stderr\n");
	else {
		if (Py_FlushLine())
			PyErr_Clear();
		fflush(stdout);
		if (tb && tb != Py_None)
			err = PyTraceBack_Print(tb, f);
		if (err == 0 &&
		    PyObject_HasAttrString(v, "print_file_and_line"))
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
				PyOS_snprintf(buf, sizeof(buf), "%d", lineno);
				PyFile_WriteString(buf, f);
				PyFile_WriteString("\n", f);
				if (text != NULL)
					print_error_text(f, offset, text);
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
	/* If an error happened here, don't show it.
	   XXX This is wrong, but too many callers rely on this behavior. */
	if (err != 0)
		PyErr_Clear();
}

PyObject *
PyRun_String(char *str, int start, PyObject *globals, PyObject *locals)
{
	return run_err_node(PyParser_SimpleParseString(str, start),
			    "<string>", globals, locals, NULL);
}

PyObject *
PyRun_File(FILE *fp, char *filename, int start, PyObject *globals,
	   PyObject *locals)
{
	return PyRun_FileEx(fp, filename, start, globals, locals, 0);
}

PyObject *
PyRun_FileEx(FILE *fp, char *filename, int start, PyObject *globals,
	     PyObject *locals, int closeit)
{
	node *n = PyParser_SimpleParseFile(fp, filename, start);
	if (closeit)
		fclose(fp);
	return run_err_node(n, filename, globals, locals, NULL);
}

PyObject *
PyRun_StringFlags(char *str, int start, PyObject *globals, PyObject *locals,
		  PyCompilerFlags *flags)
{
	return run_err_node(PyParser_SimpleParseStringFlags(
				    str, start, PARSER_FLAGS(flags)),
			    "<string>", globals, locals, flags);
}

PyObject *
PyRun_FileFlags(FILE *fp, char *filename, int start, PyObject *globals,
		PyObject *locals, PyCompilerFlags *flags)
{
	return PyRun_FileExFlags(fp, filename, start, globals, locals, 0,
				 flags); 
}

PyObject *
PyRun_FileExFlags(FILE *fp, char *filename, int start, PyObject *globals,
		  PyObject *locals, int closeit, PyCompilerFlags *flags)
{
	node *n = PyParser_SimpleParseFileFlags(fp, filename, start,
						PARSER_FLAGS(flags));
	if (closeit)
		fclose(fp);
	return run_err_node(n, filename, globals, locals, flags);
}

static PyObject *
run_err_node(node *n, char *filename, PyObject *globals, PyObject *locals,
	     PyCompilerFlags *flags)
{
	if (n == NULL)
		return  NULL;
	return run_node(n, filename, globals, locals, flags);
}

static PyObject *
run_node(node *n, char *filename, PyObject *globals, PyObject *locals,
	 PyCompilerFlags *flags)
{
	PyCodeObject *co;
	PyObject *v;
	co = PyNode_CompileFlags(n, filename, flags);
	PyNode_Free(n);
	if (co == NULL)
		return NULL;
	v = PyEval_EvalCode(co, globals, locals);
	Py_DECREF(co);
	return v;
}

static PyObject *
run_pyc_file(FILE *fp, char *filename, PyObject *globals, PyObject *locals,
	     PyCompilerFlags *flags)
{
	PyCodeObject *co;
	PyObject *v;
	long magic;
	long PyImport_GetMagicNumber(void);

	magic = PyMarshal_ReadLongFromFile(fp);
	if (magic != PyImport_GetMagicNumber()) {
		PyErr_SetString(PyExc_RuntimeError,
			   "Bad magic number in .pyc file");
		return NULL;
	}
	(void) PyMarshal_ReadLongFromFile(fp);
	v = PyMarshal_ReadLastObjectFromFile(fp);
	fclose(fp);
	if (v == NULL || !PyCode_Check(v)) {
		Py_XDECREF(v);
		PyErr_SetString(PyExc_RuntimeError,
			   "Bad code object in .pyc file");
		return NULL;
	}
	co = (PyCodeObject *)v;
	v = PyEval_EvalCode(co, globals, locals);
	if (v && flags)
		flags->cf_flags |= (co->co_flags & PyCF_MASK);
	Py_DECREF(co);
	return v;
}

PyObject *
Py_CompileString(char *str, char *filename, int start)
{
	return Py_CompileStringFlags(str, filename, start, NULL);
}

PyObject *
Py_CompileStringFlags(char *str, char *filename, int start, 
		      PyCompilerFlags *flags)
{
	node *n;
	PyCodeObject *co;
	n = PyParser_SimpleParseStringFlags(str, start, PARSER_FLAGS(flags));
	if (n == NULL)
		return NULL;
	co = PyNode_CompileFlags(n, filename, flags);
	PyNode_Free(n);
	return (PyObject *)co;
}

struct symtable *
Py_SymtableString(char *str, char *filename, int start)
{
	node *n;
	struct symtable *st;
	n = PyParser_SimpleParseString(str, start);
	if (n == NULL)
		return NULL;
	st = PyNode_CompileSymtable(n, filename);
	PyNode_Free(n);
	return st;
}

/* Simplified interface to parsefile -- return node or set exception */

node *
PyParser_SimpleParseFileFlags(FILE *fp, char *filename, int start, int flags)
{
	node *n;
	perrdetail err;
	n = PyParser_ParseFileFlags(fp, filename, &_PyParser_Grammar, start,
					(char *)0, (char *)0, &err, flags);
	if (n == NULL)
		err_input(&err);
	return n;
}

node *
PyParser_SimpleParseFile(FILE *fp, char *filename, int start)
{
	return PyParser_SimpleParseFileFlags(fp, filename, start, 0);
}

/* Simplified interface to parsestring -- return node or set exception */

node *
PyParser_SimpleParseStringFlags(char *str, int start, int flags)
{
	node *n;
	perrdetail err;
	n = PyParser_ParseStringFlags(str, &_PyParser_Grammar, start, &err,
				      flags);
	if (n == NULL)
		err_input(&err);
	return n;
}

node *
PyParser_SimpleParseString(char *str, int start)
{
	return PyParser_SimpleParseStringFlags(str, start, 0);
}

/* Set the error appropriate to the given input error code (see errcode.h) */

static void
err_input(perrdetail *err)
{
	PyObject *v, *w, *errtype;
	char *msg = NULL;
	errtype = PyExc_SyntaxError;
	v = Py_BuildValue("(ziiz)", err->filename,
			    err->lineno, err->offset, err->text);
	if (err->text != NULL) {
		PyMem_DEL(err->text);
		err->text = NULL;
	}
	switch (err->error) {
	case E_SYNTAX:
		errtype = PyExc_IndentationError;
		if (err->expected == INDENT)
			msg = "expected an indented block";
		else if (err->token == INDENT)
			msg = "unexpected indent";
		else if (err->token == DEDENT)
			msg = "unexpected unindent";
		else {
			errtype = PyExc_SyntaxError;
			msg = "invalid syntax";
		}
		break;
	case E_TOKEN:
		msg = "invalid token";
		break;
	case E_INTR:
		PyErr_SetNone(PyExc_KeyboardInterrupt);
		Py_XDECREF(v);
		return;
	case E_NOMEM:
		PyErr_NoMemory();
		Py_XDECREF(v);
		return;
	case E_EOF:
		msg = "unexpected EOF while parsing";
		break;
	case E_TABSPACE:
		errtype = PyExc_TabError;
		msg = "inconsistent use of tabs and spaces in indentation";
		break;
	case E_OVERFLOW:
		msg = "expression too long";
		break;
	case E_DEDENT:
		errtype = PyExc_IndentationError;
		msg = "unindent does not match any outer indentation level";
		break;
	case E_TOODEEP:
		errtype = PyExc_IndentationError;
		msg = "too many levels of indentation";
		break;
	default:
		fprintf(stderr, "error=%d\n", err->error);
		msg = "unknown parsing error";
		break;
	}
	w = Py_BuildValue("(sO)", msg, v);
	Py_XDECREF(v);
	PyErr_SetObject(errtype, w);
	Py_XDECREF(w);
}

/* Print fatal error message and abort */

void
Py_FatalError(char *msg)
{
	fprintf(stderr, "Fatal Python error: %s\n", msg);
#ifdef macintosh
	for (;;);
#endif
#ifdef MS_WIN32
	OutputDebugString("Fatal Python error: ");
	OutputDebugString(msg);
	OutputDebugString("\n");
#ifdef _DEBUG
	DebugBreak();
#endif
#endif /* MS_WIN32 */
	abort();
}

/* Clean up and exit */

#ifdef WITH_THREAD
#include "pythread.h"
int _PyThread_Started = 0; /* Set by threadmodule.c and maybe others */
#endif

#define NEXITFUNCS 32
static void (*exitfuncs[NEXITFUNCS])(void);
static int nexitfuncs = 0;

int Py_AtExit(void (*func)(void))
{
	if (nexitfuncs >= NEXITFUNCS)
		return -1;
	exitfuncs[nexitfuncs++] = func;
	return 0;
}

static void
call_sys_exitfunc(void)
{
	PyObject *exitfunc = PySys_GetObject("exitfunc");

	if (exitfunc) {
		PyObject *res;
		Py_INCREF(exitfunc);
		PySys_SetObject("exitfunc", (PyObject *)NULL);
		res = PyEval_CallObject(exitfunc, (PyObject *)NULL);
		if (res == NULL) {
			if (!PyErr_ExceptionMatches(PyExc_SystemExit)) {
				PySys_WriteStderr("Error in sys.exitfunc:\n");
			}
			PyErr_Print();
		}
		Py_DECREF(exitfunc);
	}

	if (Py_FlushLine())
		PyErr_Clear();
}

static void
call_ll_exitfuncs(void)
{
	while (nexitfuncs > 0)
		(*exitfuncs[--nexitfuncs])();

	fflush(stdout);
	fflush(stderr);
}

void
Py_Exit(int sts)
{
	Py_Finalize();

#ifdef macintosh
	PyMac_Exit(sts);
#else
	exit(sts);
#endif
}

static void
initsigs(void)
{
#ifdef HAVE_SIGNAL_H
#ifdef SIGPIPE
	signal(SIGPIPE, SIG_IGN);
#endif
#ifdef SIGXFZ
	signal(SIGXFZ, SIG_IGN);
#endif
#ifdef SIGXFSZ
	signal(SIGXFSZ, SIG_IGN);
#endif
#endif /* HAVE_SIGNAL_H */
	PyOS_InitInterrupts(); /* May imply initsignal() */
}

#ifdef Py_TRACE_REFS
/* Ask a yes/no question */

int
_Py_AskYesNo(char *prompt)
{
	char buf[256];
	
	fprintf(stderr, "%s [ny] ", prompt);
	if (fgets(buf, sizeof buf, stdin) == NULL)
		return 0;
	return buf[0] == 'y' || buf[0] == 'Y';
}
#endif

#ifdef MPW

/* Check for file descriptor connected to interactive device.
   Pretend that stdin is always interactive, other files never. */

int
isatty(int fd)
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
Py_FdIsInteractive(FILE *fp, char *filename)
{
	if (isatty((int)fileno(fp)))
		return 1;
	if (!Py_InteractiveFlag)
		return 0;
	return (filename == NULL) ||
	       (strcmp(filename, "<stdin>") == 0) ||
	       (strcmp(filename, "???") == 0);
}


#if defined(USE_STACKCHECK) 
#if defined(WIN32) && defined(_MSC_VER)

/* Stack checking for Microsoft C */

#include <malloc.h>
#include <excpt.h>

/*
 * Return non-zero when we run out of memory on the stack; zero otherwise.
 */
int
PyOS_CheckStack(void)
{
	__try {
		/* _alloca throws a stack overflow exception if there's
		   not enough space left on the stack */
		_alloca(PYOS_STACK_MARGIN * sizeof(void*));
		return 0;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		/* just ignore all errors */
	}
	return 1;
}

#endif /* WIN32 && _MSC_VER */

/* Alternate implementations can be added here... */

#endif /* USE_STACKCHECK */


/* Wrappers around sigaction() or signal(). */

PyOS_sighandler_t
PyOS_getsig(int sig)
{
#ifdef HAVE_SIGACTION
	struct sigaction context;
	/* Initialize context.sa_handler to SIG_ERR which makes about as
	 * much sense as anything else.  It should get overwritten if
	 * sigaction actually succeeds and otherwise we avoid an
	 * uninitialized memory read.
	 */
	context.sa_handler = SIG_ERR;
	sigaction(sig, NULL, &context);
	return context.sa_handler;
#else
	PyOS_sighandler_t handler;
	handler = signal(sig, SIG_IGN);
	signal(sig, handler);
	return handler;
#endif
}

PyOS_sighandler_t
PyOS_setsig(int sig, PyOS_sighandler_t handler)
{
#ifdef HAVE_SIGACTION
	struct sigaction context;
	PyOS_sighandler_t oldhandler;
	/* Initialize context.sa_handler to SIG_ERR which makes about as
	 * much sense as anything else.  It should get overwritten if
	 * sigaction actually succeeds and otherwise we avoid an
	 * uninitialized memory read.
	 */
	context.sa_handler = SIG_ERR;
	sigaction(sig, NULL, &context);
	oldhandler = context.sa_handler;
	context.sa_handler = handler;
	sigaction(sig, &context, NULL);
	return oldhandler;
#else
	return signal(sig, handler);
#endif
}
