/* This module makes GNU readline available to Python.  It has ideas
 * contributed by Lee Busby, LLNL, and William Magro, Cornell Theory
 * Center.  The completer interface was inspired by Lele Gaifax.
 *
 * More recently, it was largely rewritten by Guido van Rossum who is
 * now maintaining it.
 */

/* Standard definitions */
#include "Python.h"
#include <setjmp.h>
#include <signal.h>
#include <errno.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h> /* For isatty() */
#endif

/* GNU readline definitions */
/* If you have string.h, you might need to add yourself to this #if... [cjh] */
#if defined(__BEOS__)
#undef HAVE_CONFIG_H
/* At max warnings, we need protos for everything. [cjh] */
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#else
#include <readline/readline.h> /* You may need to add an -I option to Setup */

extern int rl_parse_and_bind();
extern int rl_read_init_file();
extern int rl_insert_text();
extern int rl_bind_key();
extern int rl_bind_key_in_map();
extern int rl_initialize();
extern int add_history();
#endif

/* Pointers needed from outside (but not declared in a header file). */
extern int (*PyOS_InputHook)();
extern char *(*PyOS_ReadlineFunctionPointer) Py_PROTO((char *));


/* Exported function to send one line to readline's init file parser */

static PyObject *
parse_and_bind(self, args)
	PyObject *self;
	PyObject *args;
{
	char *s, *copy;
	if (!PyArg_ParseTuple(args, "s", &s))
		return NULL;
	/* Make a copy -- rl_parse_and_bind() modifies its argument */
	/* Bernard Herzog */
	copy = malloc(1 + strlen(s));
	if (copy == NULL)
		return PyErr_NoMemory();
	strcpy(copy, s);
	rl_parse_and_bind(copy);
	free(copy); /* Free the copy */
	Py_INCREF(Py_None);
	return Py_None;
}

static char doc_parse_and_bind[] = "\
parse_and_bind(string) -> None\n\
Parse and execute single line of a readline init file.\
";


/* Exported function to parse a readline init file */

static PyObject *
read_init_file(self, args)
	PyObject *self;
	PyObject *args;
{
	char *s = NULL;
	if (!PyArg_ParseTuple(args, "|z", &s))
		return NULL;
	errno = rl_read_init_file(s);
	if (errno)
		return PyErr_SetFromErrno(PyExc_IOError);
	Py_INCREF(Py_None);
	return Py_None;
}

static char doc_read_init_file[] = "\
read_init_file([filename]) -> None\n\
Parse a readline initialization file.\n\
The default filename is the last filename used.\
";


/* Exported function to specify a word completer in Python */

static PyObject *completer = NULL;
static PyThreadState *tstate = NULL;

static PyObject *
set_completer(self, args)
	PyObject *self;
	PyObject *args;
{
	PyObject *function = Py_None;
	if (!PyArg_ParseTuple(args, "|O", &function))
		return NULL;
	if (function == Py_None) {
		Py_XDECREF(completer);
		completer = NULL;
		tstate = NULL;
	}
	else if (PyCallable_Check(function)) {
		PyObject *tmp = completer;
		Py_INCREF(function);
		completer = function;
		Py_XDECREF(tmp);
		tstate = PyThreadState_Get();
	}
	else {
		PyErr_SetString(PyExc_TypeError,
				"set_completer(func): argument not callable");
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static char doc_set_completer[] = "\
set_completer([function]) -> None\n\
Set or remove the completer function.\n\
The function is called as function(text, state),\n\
for i in [0, 1, 2, ...] until it returns a non-string.\n\
It should return the next possible completion starting with 'text'.\
";

/* Exported function to read the current line buffer */

static PyObject *
get_line_buffer(self, args)
	PyObject *self;
        PyObject *args;
{
	if (PyArg_NoArgs(args))
		return NULL;
	return PyString_FromString(rl_line_buffer);
}

static char doc_get_line_buffer[] = "\
get_line_buffer() -> string\n\
return the current contents of the line buffer.\
";

/* Exported function to insert text into the line buffer */

static PyObject *
insert_text(self, args)
	PyObject *self;
	PyObject *args;
{
	char *s;
	if (!PyArg_ParseTuple(args, "s", &s))
		return NULL;
	rl_insert_text(s);
	Py_INCREF(Py_None);
	return Py_None;
}


static char doc_insert_text[] = "\
insert_text(string) -> None\n\
Insert text into the command line.\
";


/* Table of functions exported by the module */

static struct PyMethodDef readline_methods[] =
{
	{"parse_and_bind", parse_and_bind, 1, doc_parse_and_bind},
	{"get_line_buffer", get_line_buffer, 1, doc_get_line_buffer},
	{"insert_text", insert_text, 1, doc_insert_text},
	{"read_init_file", read_init_file, 1, doc_read_init_file},
	{"set_completer", set_completer, 1, doc_set_completer},
	{0, 0}
};

/* C function to call the Python completer. */

static char *
on_completion(text, state)
	char *text;
	int state;
{
	char *result = NULL;
	if (completer != NULL) {
		PyObject *r;
		PyThreadState *save_tstate;
		/* Note that readline is called with the interpreter
		   lock released! */
		save_tstate = PyThreadState_Swap(NULL);
		PyEval_RestoreThread(tstate);
		r = PyObject_CallFunction(completer, "si", text, state);
		if (r == NULL)
			goto error;
		if (r == Py_None) {
			result = NULL;
		}
		else {
			char *s = PyString_AsString(r);
			if (s == NULL)
				goto error;
			result = strdup(s);
		}
		Py_DECREF(r);
		goto done;
	  error:
		PyErr_Clear();
		Py_XDECREF(r);
	  done:
		PyEval_SaveThread();
		PyThreadState_Swap(save_tstate);
	}
	return result;
}


/* Helper to initialize GNU readline properly. */

static void
setup_readline()
{
	rl_readline_name = "python";
	/* Force rebind of TAB to insert-tab */
	rl_bind_key('\t', rl_insert);
	/* Bind both ESC-TAB and ESC-ESC to the completion function */
	rl_bind_key_in_map ('\t', rl_complete, emacs_meta_keymap);
	rl_bind_key_in_map ('\033', rl_complete, emacs_meta_keymap);
	/* Set our completion function */
	rl_completion_entry_function = (Function *) on_completion;
	/* Set Python word break characters */
	rl_completer_word_break_characters =
		" \t\n`~!@#$%^&*()-=+[{]}\\|;:'\",<>/?";
		/* All nonalphanums except '.' */
	/* Initialize (allows .inputrc to override) */
	rl_initialize();
}


/* Interrupt handler */

static jmp_buf jbuf;

/* ARGSUSED */
static RETSIGTYPE
onintr(sig)
	int sig;
{
	longjmp(jbuf, 1);
}


/* Wrapper around GNU readline that handles signals differently. */

static char *
call_readline(prompt)
	char *prompt;
{
	int n;
	char *p;
	RETSIGTYPE (*old_inthandler)();
	old_inthandler = signal(SIGINT, onintr);
	if (setjmp(jbuf)) {
#ifdef HAVE_SIGRELSE
		/* This seems necessary on SunOS 4.1 (Rasmus Hahn) */
		sigrelse(SIGINT);
#endif
		signal(SIGINT, old_inthandler);
		return NULL;
	}
	rl_event_hook = PyOS_InputHook;
	p = readline(prompt);
	signal(SIGINT, old_inthandler);
	if (p == NULL) {
		p = malloc(1);
		if (p != NULL)
			*p = '\0';
		return p;
	}
	n = strlen(p);
	if (n > 0)
		add_history(p);
	if ((p = realloc(p, n+2)) != NULL) {
		p[n] = '\n';
		p[n+1] = '\0';
	}
	return p;
}


/* Initialize the module */

static char doc_module[] =
"Importing this module enables command line editing using GNU readline.";

DL_EXPORT(void)
initreadline()
{
	PyObject *m;

	m = Py_InitModule4("readline", readline_methods, doc_module,
			   (PyObject *)NULL, PYTHON_API_VERSION);
	if (isatty(fileno(stdin))) {
		PyOS_ReadlineFunctionPointer = call_readline;
		setup_readline();
	}
}
