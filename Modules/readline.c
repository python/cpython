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
#include <sys/time.h>

#if defined(HAVE_SETLOCALE)
/* GNU readline() mistakenly sets the LC_CTYPE locale.
 * This is evil.  Only the user or the app's main() should do this!
 * We must save and restore the locale around the rl_initialize() call.
 */
#define SAVE_LOCALE
#include <locale.h>
#endif

/* GNU readline definitions */
#undef HAVE_CONFIG_H /* Else readline/chardefs.h includes strings.h */
#include <readline/readline.h>
#include <readline/history.h>

#ifdef HAVE_RL_COMPLETION_MATCHES
#define completion_matches(x, y) \
	rl_completion_matches((x), ((rl_compentry_func_t *)(y)))
#endif


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

static int _history_length = -1; /* do not truncate history by default */
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
	if (!errno && _history_length >= 0)
		history_truncate_file(s, _history_length);
	if (errno)
		return PyErr_SetFromErrno(PyExc_IOError);
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(doc_write_history_file,
"write_history_file([filename]) -> None\n\
Save a readline history file.\n\
The default filename is ~/.history.");


/* Set history length */

static PyObject*
set_history_length(PyObject *self, PyObject *args)
{
	int length = _history_length;
	if (!PyArg_ParseTuple(args, "i:set_history_length", &length))
		return NULL;
	_history_length = length;
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(set_history_length_doc,
"set_history_length(length) -> None\n\
set the maximal number of items which will be written to\n\
the history file. A negative length is used to inhibit\n\
history truncation.");


/* Get history length */

static PyObject*
get_history_length(PyObject *self, PyObject *noarg)
{
	return PyInt_FromLong(_history_length);
}

PyDoc_STRVAR(get_history_length_doc,
"get_history_length() -> int\n\
return the maximum number of items that will be written to\n\
the history file.");


/* Generic hook function setter */

static PyObject *
set_hook(const char *funcname, PyObject **hook_var,
	 PyThreadState **tstate, PyObject *args)
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
		*tstate = PyThreadState_GET();
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
	return set_hook("startup_hook", &startup_hook,
			&startup_hook_tstate, args);
}

PyDoc_STRVAR(doc_set_startup_hook,
"set_startup_hook([function]) -> None\n\
Set or remove the startup_hook function.\n\
The function is called with no arguments just\n\
before readline prints the first prompt.");


#ifdef HAVE_RL_PRE_INPUT_HOOK

/* Set pre-input hook */

static PyObject *
set_pre_input_hook(PyObject *self, PyObject *args)
{
	return set_hook("pre_input_hook", &pre_input_hook,
			&pre_input_hook_tstate, args);
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


/* Get the beginning index for the scope of the tab-completion */

static PyObject *
get_begidx(PyObject *self, PyObject *noarg)
{
	Py_INCREF(begidx);
	return begidx;
}

PyDoc_STRVAR(doc_get_begidx,
"get_begidx() -> int\n\
get the beginning index of the readline tab-completion scope");


/* Get the ending index for the scope of the tab-completion */

static PyObject *
get_endidx(PyObject *self, PyObject *noarg)
{
	Py_INCREF(endidx);
	return endidx;
}

PyDoc_STRVAR(doc_get_endidx,
"get_endidx() -> int\n\
get the ending index of the readline tab-completion scope");


/* Set the tab-completion word-delimiters that readline uses */

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
py_remove_history(PyObject *self, PyObject *args)
{
        int entry_number;
        HIST_ENTRY *entry;

        if (!PyArg_ParseTuple(args, "i:remove_history", &entry_number))
                return NULL;
        if (entry_number < 0) {
                PyErr_SetString(PyExc_ValueError,
                                "History index cannot be negative");
                return NULL;
        }
        entry = remove_history(entry_number);
        if (!entry) {
                PyErr_Format(PyExc_ValueError,
                             "No history item at position %d",
                             entry_number);
                return NULL;
        }
        /* free memory allocated for the history entry */
        if (entry->line)
                free(entry->line);
        if (entry->data)
                free(entry->data);
        free(entry);

        Py_INCREF(Py_None);
        return Py_None;
}

PyDoc_STRVAR(doc_remove_history,
"remove_history_item(pos) -> None\n\
remove history item given by its position");

static PyObject *
py_replace_history(PyObject *self, PyObject *args)
{
        int entry_number;
        char *line;
        HIST_ENTRY *old_entry;

        if (!PyArg_ParseTuple(args, "is:replace_history", &entry_number, &line)) {
                return NULL;
        }
        if (entry_number < 0) {
                PyErr_SetString(PyExc_ValueError,
                                "History index cannot be negative");
                return NULL;
        }
        old_entry = replace_history_entry(entry_number, line, (void *)NULL);
        if (!old_entry) {
                PyErr_Format(PyExc_ValueError,
                             "No history item at position %d",
                             entry_number);
                return NULL;
        }
        /* free memory allocated for the old history entry */
        if (old_entry->line)
            free(old_entry->line);
        if (old_entry->data)
            free(old_entry->data);
        free(old_entry);

        Py_INCREF(Py_None);
        return Py_None;
}

PyDoc_STRVAR(doc_replace_history,
"replace_history_item(pos, line) -> None\n\
replaces history item given by its position with contents of line");

/* Add a line to the history buffer */

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


/* Get the tab-completion word-delimiters that readline uses */

static PyObject *
get_completer_delims(PyObject *self, PyObject *noarg)
{
	return PyString_FromString(rl_completer_word_break_characters);
}

PyDoc_STRVAR(doc_get_completer_delims,
"get_completer_delims() -> string\n\
get the readline word delimiters for tab-completion");


/* Set the completer function */

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


static PyObject *
get_completer(PyObject *self, PyObject *noargs)
{
	if (completer == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	Py_INCREF(completer);
	return completer;
}

PyDoc_STRVAR(doc_get_completer,
"get_completer() -> function\n\
\n\
Returns current completer function.");

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
get_current_history_length(PyObject *self, PyObject *noarg)
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
get_line_buffer(PyObject *self, PyObject *noarg)
{
	return PyString_FromString(rl_line_buffer);
}

PyDoc_STRVAR(doc_get_line_buffer,
"get_line_buffer() -> string\n\
return the current contents of the line buffer.");


#ifdef HAVE_RL_COMPLETION_APPEND_CHARACTER

/* Exported function to clear the current history */

static PyObject *
py_clear_history(PyObject *self, PyObject *noarg)
{
	clear_history();
	Py_INCREF(Py_None);
	return Py_None;
}

PyDoc_STRVAR(doc_clear_history,
"clear_history() -> None\n\
Clear the current readline history.");
#endif


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


/* Redisplay the line buffer */

static PyObject *
redisplay(PyObject *self, PyObject *noarg)
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
	{"get_line_buffer", get_line_buffer, METH_NOARGS, doc_get_line_buffer},
	{"insert_text", insert_text, METH_VARARGS, doc_insert_text},
	{"redisplay", redisplay, METH_NOARGS, doc_redisplay},
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
	 METH_NOARGS, get_history_length_doc},
	{"set_completer", set_completer, METH_VARARGS, doc_set_completer},
	{"get_completer", get_completer, METH_NOARGS, doc_get_completer},
	{"get_begidx", get_begidx, METH_NOARGS, doc_get_begidx},
	{"get_endidx", get_endidx, METH_NOARGS, doc_get_endidx},

	{"set_completer_delims", set_completer_delims,
	 METH_VARARGS, doc_set_completer_delims},
	{"add_history", py_add_history, METH_VARARGS, doc_add_history},
        {"remove_history_item", py_remove_history, METH_VARARGS, doc_remove_history},
        {"replace_history_item", py_replace_history, METH_VARARGS, doc_replace_history},
	{"get_completer_delims", get_completer_delims,
	 METH_NOARGS, doc_get_completer_delims},

	{"set_startup_hook", set_startup_hook,
	 METH_VARARGS, doc_set_startup_hook},
#ifdef HAVE_RL_PRE_INPUT_HOOK
	{"set_pre_input_hook", set_pre_input_hook,
	 METH_VARARGS, doc_set_pre_input_hook},
#endif
#ifdef HAVE_RL_COMPLETION_APPEND_CHARACTER
	{"clear_history", py_clear_history, METH_NOARGS, doc_clear_history},
#endif
	{0, 0}
};


/* C function to call the Python hooks. */

static int
on_hook(PyObject *func, PyThreadState **tstate)
{
	int result = 0;
	if (func != NULL) {
		PyObject *r;
		/* Note that readline is called with the interpreter
		   lock released! */
		PyEval_RestoreThread(*tstate);
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
		*tstate = PyEval_SaveThread();
	}
	return result;
}

static int
on_startup_hook(void)
{
	return on_hook(startup_hook, &startup_hook_tstate);
}

#ifdef HAVE_RL_PRE_INPUT_HOOK
static int
on_pre_input_hook(void)
{
	return on_hook(pre_input_hook, &pre_input_hook_tstate);
}
#endif


/* C function to call the Python completer. */

static char *
on_completion(char *text, int state)
{
	char *result = NULL;
	if (completer != NULL) {
		PyObject *r;
		/* Note that readline is called with the interpreter
		   lock released! */
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
		completer_tstate = PyEval_SaveThread();
	}
	return result;
}


/* A more flexible constructor that saves the "begidx" and "endidx"
 * before calling the normal completer */

static char **
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
#ifdef SAVE_LOCALE
	char *saved_locale = strdup(setlocale(LC_CTYPE, NULL));
	if (!saved_locale)
		Py_FatalError("not enough memory to save locale");
#endif

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
#ifdef HAVE_RL_COMPLETION_APPEND_CHARACTER
	rl_completion_append_character ='\0';
#endif

	begidx = PyInt_FromLong(0L);
	endidx = PyInt_FromLong(0L);
	/* Initialize (allows .inputrc to override)
	 *
	 * XXX: A bug in the readline-2.2 library causes a memory leak
	 * inside this function.  Nothing we can do about it.
	 */
	rl_initialize();

#ifdef SAVE_LOCALE
	setlocale(LC_CTYPE, saved_locale); /* Restore locale */
	free(saved_locale);
#endif
}

/* Wrapper around GNU readline that handles signals differently. */


#if defined(HAVE_RL_CALLBACK) && defined(HAVE_SELECT)

static	char *completed_input_string;
static void
rlhandler(char *text)
{
	completed_input_string = text;
	rl_callback_handler_remove();
}

extern PyThreadState* _PyOS_ReadlineTState;

static char *
readline_until_enter_or_signal(char *prompt, int *signal)
{
	char * not_done_reading = "";
	fd_set selectset;

	*signal = 0;
#ifdef HAVE_RL_CATCH_SIGNAL
	rl_catch_signals = 0;
#endif

	rl_callback_handler_install (prompt, rlhandler);
	FD_ZERO(&selectset);
	
	completed_input_string = not_done_reading;

	while (completed_input_string == not_done_reading) {
		int has_input = 0;

		while (!has_input)
		{	struct timeval timeout = {0, 100000}; /* 0.1 seconds */
			FD_SET(fileno(rl_instream), &selectset);
			/* select resets selectset if no input was available */
			has_input = select(fileno(rl_instream) + 1, &selectset,
					   NULL, NULL, &timeout);
			if(PyOS_InputHook) PyOS_InputHook();
		}

		if(has_input > 0) {
			rl_callback_read_char();
		}
		else if (errno == EINTR) {
			int s;
			PyEval_RestoreThread(_PyOS_ReadlineTState);
			s = PyErr_CheckSignals();
			PyEval_SaveThread();	
			if (s < 0) {
				rl_free_line_state();
				rl_cleanup_after_signal();
				rl_callback_handler_remove();
				*signal = 1;
				completed_input_string = NULL;
			}
		}
	}

	return completed_input_string;
}


#else

/* Interrupt handler */

static jmp_buf jbuf;

/* ARGSUSED */
static void
onintr(int sig)
{
	longjmp(jbuf, 1);
}


static char *
readline_until_enter_or_signal(char *prompt, int *signal)
{
	PyOS_sighandler_t old_inthandler;
	char *p;
    
	*signal = 0;

	old_inthandler = PyOS_setsig(SIGINT, onintr);
	if (setjmp(jbuf)) {
#ifdef HAVE_SIGRELSE
		/* This seems necessary on SunOS 4.1 (Rasmus Hahn) */
		sigrelse(SIGINT);
#endif
		PyOS_setsig(SIGINT, old_inthandler);
		*signal = 1;
		return NULL;
	}
	rl_event_hook = PyOS_InputHook;
	p = readline(prompt);
	PyOS_setsig(SIGINT, old_inthandler);

    return p;
}
#endif /*defined(HAVE_RL_CALLBACK) && defined(HAVE_SELECT) */


static char *
call_readline(FILE *sys_stdin, FILE *sys_stdout, char *prompt)
{
	size_t n;
	char *p, *q;
	int signal;

#ifdef SAVE_LOCALE
	char *saved_locale = strdup(setlocale(LC_CTYPE, NULL));
	if (!saved_locale)
		Py_FatalError("not enough memory to save locale");
	setlocale(LC_CTYPE, "");
#endif

	if (sys_stdin != rl_instream || sys_stdout != rl_outstream) {
		rl_instream = sys_stdin;
		rl_outstream = sys_stdout;
#ifdef HAVE_RL_COMPLETION_APPEND_CHARACTER
		rl_prep_terminal (1);
#endif
	}

	p = readline_until_enter_or_signal(prompt, &signal);
	
	/* we got an interrupt signal */
	if(signal) {
		return NULL;
	}

	/* We got an EOF, return a empty string. */
	if (p == NULL) {
		p = PyMem_Malloc(1);
		if (p != NULL)
			*p = '\0';
		return p;
	}

	/* we have a valid line */
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
#ifdef SAVE_LOCALE
	setlocale(LC_CTYPE, saved_locale); /* Restore locale */
	free(saved_locale);
#endif
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

	PyOS_ReadlineFunctionPointer = call_readline;
	setup_readline();
}
