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

/* GNU readline definitions */
#undef HAVE_CONFIG_H /* Else readline/chardefs.h includes strings.h */
#include <readline/readline.h>
#include <readline/history.h>

#ifdef HAVE_RL_COMPLETION_MATCHES
#define completion_matches(x, y) rl_completion_matches((x), ((rl_compentry_func_t *)(y)))
#endif

/* Pointers needed from outside (but not declared in a header file). */
extern DL_IMPORT(int) (*PyOS_InputHook)(void);
extern DL_IMPORT(char) *(*PyOS_ReadlineFunctionPointer)(char *);


/* Exported function to send one line to readline's init file parser */

static PyObject *
parse_and_bind(PyObject *self, PyObject *args)
{
	char *s, *copy;
	if (!PyArg_ParseTuple(args, "s:parse_and_bind", &s))
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

PyDoc_STRVAR(doc_parse_and_bind,
"parse_and_bind(string) -> None\n\
Parse and execute single line of a readline init file.");


/* Exported function to parse a readline init file */

static PyObject *
read_init_file(PyObject *self, PyObject *args)
{
	char *s = NULL;
	if (!PyArg_ParseTuple(args, "|z:read_init_file", &s))
		return NULL;
	errno = rl_read_init_file(s);
	if (errno)
		return PyErr_SetFromErrno(PyExc_IOError);
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(doc_read_init_file,
"read_init_file([filename]) -> None\n\
Parse a readline initialization file.\n\
The default filename is the last filename used.");


/* Exported function to load a readline history file */

static PyObject *
read_history_file(PyObject *self, PyObject *args)
{
	char *s = NULL;
	if (!PyArg_ParseTuple(args, "|z:read_history_file", &s))
		return NULL;
	errno = read_history(s);
	if (errno)
		return PyErr_SetFromErrno(PyExc_IOError);
	Py_INCREF(Py_None);
	return Py_None;
}

static int history_length = -1; /* do not truncate history by default */
PyDoc_STRVAR(doc_read_history_file,
"read_history_file([filename]) -> None\n\
Load a readline history file.\n\
The default filename is ~/.history.");


/* Exported function to save a readline history file */

static PyObject *
write_history_file(PyObject *self, PyObject *args)
{
	char *s = NULL;
	if (!PyArg_ParseTuple(args, "|z:write_history_file", &s))
		return NULL;
	errno = write_history(s);
	if (!errno && history_length >= 0)
		history_truncate_file(s, history_length);
	if (errno)
		return PyErr_SetFromErrno(PyExc_IOError);
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(doc_write_history_file,
"write_history_file([filename]) -> None\n\
Save a readline history file.\n\
The default filename is ~/.history.");


PyDoc_STRVAR(set_history_length_doc,
"set_history_length(length) -> None\n\
set the maximal number of items which will be written to\n\
the history file. A negative length is used to inhibit\n\
history truncation.");

static PyObject*
set_history_length(PyObject *self, PyObject *args)
{
    int length = history_length;
    if (!PyArg_ParseTuple(args, "i:set_history_length", &length))
	return NULL;
    history_length = length;
    Py_INCREF(Py_None);
    return Py_None;
}



PyDoc_STRVAR(get_history_length_doc,
"get_history_length() -> int\n\
return the maximum number of items that will be written to\n\
the history file.");

static PyObject*
get_history_length(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":get_history_length"))
		return NULL;
	return Py_BuildValue("i", history_length);
}

/* Generic hook function setter */

static PyObject *
set_hook(const char * funcname, PyObject **hook_var, PyThreadState **tstate, PyObject *args)
{
	PyObject *function = Py_None;
	char buf[80];
	PyOS_snprintf(buf, sizeof(buf), "|O:set_%.50s", funcname);
	if (!PyArg_ParseTuple(args, buf, &function))
		return NULL;
	if (function == Py_None) {
		Py_XDECREF(*hook_var);
		*hook_var = NULL;
		*tstate = NULL;
	}
	else if (PyCallable_Check(function)) {
		PyObject *tmp = *hook_var;
		Py_INCREF(function);
		*hook_var = function;
		Py_XDECREF(tmp);
		*tstate = PyThreadState_Get();
	}
	else {
		PyOS_snprintf(buf, sizeof(buf),
			      "set_%.50s(func): argument not callable",
			      funcname);
		PyErr_SetString(PyExc_TypeError, buf);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

/* Exported functions to specify hook functions in Python */

static PyObject *startup_hook = NULL;
static PyThreadState *startup_hook_tstate = NULL;

#ifdef HAVE_RL_PRE_INPUT_HOOK
static PyObject *pre_input_hook = NULL;
static PyThreadState *pre_input_hook_tstate = NULL;
#endif

static PyObject *
set_startup_hook(PyObject *self, PyObject *args)
{
	return set_hook("startup_hook", &startup_hook, &startup_hook_tstate, args);
}

PyDoc_STRVAR(doc_set_startup_hook,
"set_startup_hook([function]) -> None\n\
Set or remove the startup_hook function.\n\
The function is called with no arguments just\n\
before readline prints the first prompt.");

#ifdef HAVE_RL_PRE_INPUT_HOOK
static PyObject *
set_pre_input_hook(PyObject *self, PyObject *args)
{
	return set_hook("pre_input_hook", &pre_input_hook,  &pre_input_hook_tstate, args);
}

PyDoc_STRVAR(doc_set_pre_input_hook,
"set_pre_input_hook([function]) -> None\n\
Set or remove the pre_input_hook function.\n\
The function is called with no arguments after the first prompt\n\
has been printed and just before readline starts reading input\n\
characters.");
#endif

/* Exported function to specify a word completer in Python */

static PyObject *completer = NULL;
static PyThreadState *completer_tstate = NULL;

static PyObject *begidx = NULL;
static PyObject *endidx = NULL;

/* get the beginning index for the scope of the tab-completion */
static PyObject *
get_begidx(PyObject *self)
{
	Py_INCREF(begidx);
	return begidx;
}

PyDoc_STRVAR(doc_get_begidx,
"get_begidx() -> int\n\
get the beginning index of the readline tab-completion scope");

/* get the ending index for the scope of the tab-completion */
static PyObject *
get_endidx(PyObject *self)
{
	Py_INCREF(endidx);
	return endidx;
}

PyDoc_STRVAR(doc_get_endidx,
"get_endidx() -> int\n\
get the ending index of the readline tab-completion scope");


/* set the tab-completion word-delimiters that readline uses */

static PyObject *
set_completer_delims(PyObject *self, PyObject *args)
{
	char *break_chars;

	if(!PyArg_ParseTuple(args, "s:set_completer_delims", &break_chars)) {
		return NULL;
	}
	free((void*)rl_completer_word_break_characters);
	rl_completer_word_break_characters = strdup(break_chars);
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(doc_set_completer_delims,
"set_completer_delims(string) -> None\n\
set the readline word delimiters for tab-completion");

static PyObject *
py_add_history(PyObject *self, PyObject *args)
{
	char *line;

	if(!PyArg_ParseTuple(args, "s:add_history", &line)) {
		return NULL;
	}
	add_history(line);
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(doc_add_history,
"add_history(string) -> None\n\
add a line to the history buffer");


/* get the tab-completion word-delimiters that readline uses */

static PyObject *
get_completer_delims(PyObject *self)
{
	return PyString_FromString(rl_completer_word_break_characters);
}
	
PyDoc_STRVAR(doc_get_completer_delims,
"get_completer_delims() -> string\n\
get the readline word delimiters for tab-completion");

static PyObject *
set_completer(PyObject *self, PyObject *args)
{
	return set_hook("completer", &completer, &completer_tstate, args);
}

PyDoc_STRVAR(doc_set_completer,
"set_completer([function]) -> None\n\
Set or remove the completer function.\n\
The function is called as function(text, state),\n\
for state in 0, 1, 2, ..., until it returns a non-string.\n\
It should return the next possible completion starting with 'text'.");

/* Exported function to get any element of history */

static PyObject *
get_history_item(PyObject *self, PyObject *args)
{
	int idx = 0;
	HIST_ENTRY *hist_ent;

	if (!PyArg_ParseTuple(args, "i:index", &idx))
		return NULL;
	if ((hist_ent = history_get(idx)))
	    return PyString_FromString(hist_ent->line);
	else {
		Py_INCREF(Py_None);
		return Py_None;
	}
}

PyDoc_STRVAR(doc_get_history_item,
"get_history_item() -> string\n\
return the current contents of history item at index.");

/* Exported function to get current length of history */

static PyObject *
get_current_history_length(PyObject *self)
{
	HISTORY_STATE *hist_st;

	hist_st = history_get_history_state();
	return PyInt_FromLong(hist_st ? (long) hist_st->length : (long) 0);
}

PyDoc_STRVAR(doc_get_current_history_length,
"get_current_history_length() -> integer\n\
return the current (not the maximum) length of history.");

/* Exported function to read the current line buffer */

static PyObject *
get_line_buffer(PyObject *self)
{
	return PyString_FromString(rl_line_buffer);
}

PyDoc_STRVAR(doc_get_line_buffer,
"get_line_buffer() -> string\n\
return the current contents of the line buffer.");

/* Exported function to insert text into the line buffer */

static PyObject *
insert_text(PyObject *self, PyObject *args)
{
	char *s;
	if (!PyArg_ParseTuple(args, "s:insert_text", &s))
		return NULL;
	rl_insert_text(s);
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(doc_insert_text,
"insert_text(string) -> None\n\
Insert text into the command line.");

static PyObject *
redisplay(PyObject *self)
{
	rl_redisplay();
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(doc_redisplay,
"redisplay() -> None\n\
Change what's displayed on the screen to reflect the current\n\
contents of the line buffer.");

/* Table of functions exported by the module */

static struct PyMethodDef readline_methods[] =
{
	{"parse_and_bind", parse_and_bind, METH_VARARGS, doc_parse_and_bind},
	{"get_line_buffer", (PyCFunction)get_line_buffer, 
	 METH_NOARGS, doc_get_line_buffer},
	{"insert_text", insert_text, METH_VARARGS, doc_insert_text},
	{"redisplay", (PyCFunction)redisplay, METH_NOARGS, doc_redisplay},
	{"read_init_file", read_init_file, METH_VARARGS, doc_read_init_file},
	{"read_history_file", read_history_file, 
	 METH_VARARGS, doc_read_history_file},
	{"write_history_file", write_history_file, 
	 METH_VARARGS, doc_write_history_file},
	{"get_history_item", get_history_item,
	 METH_VARARGS, doc_get_history_item},
	{"get_current_history_length", (PyCFunction)get_current_history_length,
	 METH_NOARGS, doc_get_current_history_length},
 	{"set_history_length", set_history_length, 
	 METH_VARARGS, set_history_length_doc},
 	{"get_history_length", get_history_length, 
	 METH_VARARGS, get_history_length_doc},
	{"set_completer", set_completer, METH_VARARGS, doc_set_completer},
	{"get_begidx", (PyCFunction)get_begidx, METH_NOARGS, doc_get_begidx},
	{"get_endidx", (PyCFunction)get_endidx, METH_NOARGS, doc_get_endidx},

	{"set_completer_delims", set_completer_delims, 
	 METH_VARARGS, doc_set_completer_delims},
	{"add_history", py_add_history, METH_VARARGS, doc_add_history},
	{"get_completer_delims", (PyCFunction)get_completer_delims, 
	 METH_NOARGS, doc_get_completer_delims},
	
	{"set_startup_hook", set_startup_hook, METH_VARARGS, doc_set_startup_hook},
#ifdef HAVE_RL_PRE_INPUT_HOOK
	{"set_pre_input_hook", set_pre_input_hook, METH_VARARGS, doc_set_pre_input_hook},
#endif
	{0, 0}
};

/* C function to call the Python hooks. */

static int
on_hook(PyObject *func, PyThreadState *tstate)
{
	int result = 0;
	if (func != NULL) {
		PyObject *r;
		PyThreadState *save_tstate;
		/* Note that readline is called with the interpreter
		   lock released! */
		save_tstate = PyThreadState_Swap(NULL);
		PyEval_RestoreThread(tstate);
		r = PyObject_CallFunction(func, NULL);
		if (r == NULL)
			goto error;
		if (r == Py_None) 
			result = 0;
		else 
			result = PyInt_AsLong(r);
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

static int
on_startup_hook(void)
{
	return on_hook(startup_hook, startup_hook_tstate);
}

#ifdef HAVE_RL_PRE_INPUT_HOOK
static int
on_pre_input_hook(void)
{
	return on_hook(pre_input_hook, pre_input_hook_tstate);
}
#endif

/* C function to call the Python completer. */

static char *
on_completion(char *text, int state)
{
	char *result = NULL;
	if (completer != NULL) {
		PyObject *r;
		PyThreadState *save_tstate;
		/* Note that readline is called with the interpreter
		   lock released! */
		save_tstate = PyThreadState_Swap(NULL);
		PyEval_RestoreThread(completer_tstate);
		/* Don't use the default filename completion if we
		 * have a custom completion function... */
		rl_attempted_completion_over = 1;
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


/* a more flexible constructor that saves the "begidx" and "endidx"
 * before calling the normal completer */

char **
flex_complete(char *text, int start, int end)
{
	Py_XDECREF(begidx);
	Py_XDECREF(endidx);
	begidx = PyInt_FromLong((long) start);
	endidx = PyInt_FromLong((long) end);
	return completion_matches(text, *on_completion);
}

/* Helper to initialize GNU readline properly. */

static void
setup_readline(void)
{
	using_history();

	rl_readline_name = "python";
#if defined(PYOS_OS2) && defined(PYCC_GCC)
	/* Allow $if term= in .inputrc to work */
	rl_terminal_name = getenv("TERM");
#endif
	/* Force rebind of TAB to insert-tab */
	rl_bind_key('\t', rl_insert);
	/* Bind both ESC-TAB and ESC-ESC to the completion function */
	rl_bind_key_in_map ('\t', rl_complete, emacs_meta_keymap);
	rl_bind_key_in_map ('\033', rl_complete, emacs_meta_keymap);
	/* Set our hook functions */
	rl_startup_hook = (Function *)on_startup_hook;
#ifdef HAVE_RL_PRE_INPUT_HOOK
	rl_pre_input_hook = (Function *)on_pre_input_hook;
#endif
	/* Set our completion function */
	rl_attempted_completion_function = (CPPFunction *)flex_complete;
	/* Set Python word break characters */
	rl_completer_word_break_characters =
		strdup(" \t\n`~!@#$%^&*()-=+[{]}\\|;:'\",<>/?");
		/* All nonalphanums except '.' */
	rl_completion_append_character ='\0';

	begidx = PyInt_FromLong(0L);
	endidx = PyInt_FromLong(0L);
	/* Initialize (allows .inputrc to override)
	 *
	 * XXX: A bug in the readline-2.2 library causes a memory leak
	 * inside this function.  Nothing we can do about it.
	 */
	rl_initialize();
}


/* Interrupt handler */

static jmp_buf jbuf;

/* ARGSUSED */
static void
onintr(int sig)
{
	longjmp(jbuf, 1);
}


/* Wrapper around GNU readline that handles signals differently. */

static char *
call_readline(char *prompt)
{
	size_t n;
	char *p, *q;
	PyOS_sighandler_t old_inthandler;
	
	old_inthandler = PyOS_setsig(SIGINT, onintr);
	if (setjmp(jbuf)) {
#ifdef HAVE_SIGRELSE
		/* This seems necessary on SunOS 4.1 (Rasmus Hahn) */
		sigrelse(SIGINT);
#endif
		PyOS_setsig(SIGINT, old_inthandler);
		return NULL;
	}
	rl_event_hook = PyOS_InputHook;
	p = readline(prompt);
	PyOS_setsig(SIGINT, old_inthandler);

	/* We must return a buffer allocated with PyMem_Malloc. */
	if (p == NULL) {
		p = PyMem_Malloc(1);
		if (p != NULL)
			*p = '\0';
		return p;
	}
	n = strlen(p);
	if (n > 0) {
		char *line;
		HISTORY_STATE *state = history_get_history_state();
		if (state->length > 0)
			line = history_get(state->length)->line;
		else
			line = "";
		if (strcmp(p, line))
			add_history(p);
		/* the history docs don't say so, but the address of state
		   changes each time history_get_history_state is called
		   which makes me think it's freshly malloc'd memory...
		   on the other hand, the address of the last line stays the
		   same as long as history isn't extended, so it appears to
		   be malloc'd but managed by the history package... */
		free(state);
	}
	/* Copy the malloc'ed buffer into a PyMem_Malloc'ed one and
	   release the original. */
	q = p;
	p = PyMem_Malloc(n+2);
	if (p != NULL) {
		strncpy(p, q, n);
		p[n] = '\n';
		p[n+1] = '\0';
	}
	free(q);
	return p;
}


/* Initialize the module */

PyDoc_STRVAR(doc_module,
"Importing this module enables command line editing using GNU readline.");

PyMODINIT_FUNC
initreadline(void)
{
	PyObject *m;

	m = Py_InitModule4("readline", readline_methods, doc_module,
			   (PyObject *)NULL, PYTHON_API_VERSION);
	if (isatty(fileno(stdin))) {
		PyOS_ReadlineFunctionPointer = call_readline;
		setup_readline();
	}
}
