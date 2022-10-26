/* This module makes GNU readline available to Python.  It has ideas
 * contributed by Lee Busby, LLNL, and William Magro, Cornell Theory
 * Center.  The completer interface was inspired by Lele Gaifax.  More
 * recently, it was largely rewritten by Guido van Rossum.
 */

/* Standard definitions */
#include "Python.h"
#include <stddef.h>
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

#ifdef SAVE_LOCALE
#  define RESTORE_LOCALE(sl) { setlocale(LC_CTYPE, sl); free(sl); }
#else
#  define RESTORE_LOCALE(sl)
#endif

#ifdef WITH_EDITLINE
#  include <editline/readline.h>
#else
/* GNU readline definitions */
#  undef HAVE_CONFIG_H /* Else readline/chardefs.h includes strings.h */
#  include <readline/readline.h>
#  include <readline/history.h>
#endif

#ifdef HAVE_RL_COMPLETION_MATCHES
#define completion_matches(x, y) \
    rl_completion_matches((x), ((rl_compentry_func_t *)(y)))
#else
#if defined(_RL_FUNCTION_TYPEDEF)
extern char **completion_matches(char *, rl_compentry_func_t *);
#else

#if !defined(__APPLE__)
extern char **completion_matches(char *, CPFunction *);
#endif
#endif
#endif

/*
 * It is possible to link the readline module to the readline
 * emulation library of editline/libedit.
 *
 * This emulation library is not 100% API compatible with the "real" readline
 * and cannot be detected at compile-time,
 * hence we use a runtime check to detect if the Python readline module is
 * linked to libedit.
 *
 * Currently there is one known API incompatibility:
 * - 'get_history' has a 1-based index with GNU readline, and a 0-based
 *   index with older versions of libedit's emulation.
 * - Note that replace_history and remove_history use a 0-based index
 *   with both implementations.
 */
static int using_libedit_emulation = 0;
static const char libedit_version_tag[] = "EditLine wrapper";

static int8_t libedit_history_start = 0;
static int8_t libedit_append_replace_history_offset = 0;

#ifdef HAVE_RL_COMPLETION_DISPLAY_MATCHES_HOOK
static void
on_completion_display_matches_hook(char **matches,
                                   int num_matches, int max_length);
#endif

/* Memory allocated for rl_completer_word_break_characters
   (see issue #17289 for the motivation). */
static char *completer_word_break_characters;

typedef struct {
  /* Specify hook functions in Python */
  PyObject *completion_display_matches_hook;
  PyObject *startup_hook;
  PyObject *pre_input_hook;

  PyObject *completer; /* Specify a word completer in Python */
  PyObject *begidx;
  PyObject *endidx;
} readlinestate;

static inline readlinestate*
get_readline_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (readlinestate *)state;
}

/*[clinic input]
module readline
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=ad49da781b9c8721]*/

static int
readline_clear(PyObject *m)
{
   readlinestate *state = get_readline_state(m);
   Py_CLEAR(state->completion_display_matches_hook);
   Py_CLEAR(state->startup_hook);
   Py_CLEAR(state->pre_input_hook);
   Py_CLEAR(state->completer);
   Py_CLEAR(state->begidx);
   Py_CLEAR(state->endidx);
   return 0;
}

static int
readline_traverse(PyObject *m, visitproc visit, void *arg)
{
    readlinestate *state = get_readline_state(m);
    Py_VISIT(state->completion_display_matches_hook);
    Py_VISIT(state->startup_hook);
    Py_VISIT(state->pre_input_hook);
    Py_VISIT(state->completer);
    Py_VISIT(state->begidx);
    Py_VISIT(state->endidx);
    return 0;
}

static void
readline_free(void *m)
{
    readline_clear((PyObject *)m);
}

static PyModuleDef readlinemodule;

#define readlinestate_global ((readlinestate *)PyModule_GetState(PyState_FindModule(&readlinemodule)))


/* Convert to/from multibyte C strings */

static PyObject *
encode(PyObject *b)
{
    return PyUnicode_EncodeLocale(b, "surrogateescape");
}

static PyObject *
decode(const char *s)
{
    return PyUnicode_DecodeLocale(s, "surrogateescape");
}


/*
Explicitly disable bracketed paste in the interactive interpreter, even if it's
set in the inputrc, is enabled by default (eg GNU Readline 8.1), or a user calls
readline.read_init_file(). The Python REPL has not implemented bracketed
paste support. Also, bracketed mode writes the "\x1b[?2004h" escape sequence
into stdout which causes test failures in applications that don't support it.
It can still be explicitly enabled by calling readline.parse_and_bind("set
enable-bracketed-paste on"). See bpo-42819 for more details.

This should be removed if bracketed paste mode is implemented (bpo-39820).
*/

static void
disable_bracketed_paste(void)
{
    if (!using_libedit_emulation) {
        rl_variable_bind ("enable-bracketed-paste", "off");
    }
}

/* Exported function to send one line to readline's init file parser */

/*[clinic input]
readline.parse_and_bind

    string: object
    /

Execute the init line provided in the string argument.
[clinic start generated code]*/

static PyObject *
readline_parse_and_bind(PyObject *module, PyObject *string)
/*[clinic end generated code: output=1a1ede8afb9546c1 input=8a28a00bb4d61eec]*/
{
    char *copy;
    PyObject *encoded = encode(string);
    if (encoded == NULL) {
        return NULL;
    }
    /* Make a copy -- rl_parse_and_bind() modifies its argument */
    /* Bernard Herzog */
    copy = PyMem_Malloc(1 + PyBytes_GET_SIZE(encoded));
    if (copy == NULL) {
        Py_DECREF(encoded);
        return PyErr_NoMemory();
    }
    strcpy(copy, PyBytes_AS_STRING(encoded));
    Py_DECREF(encoded);
    rl_parse_and_bind(copy);
    PyMem_Free(copy); /* Free the copy */
    Py_RETURN_NONE;
}

/* Exported function to parse a readline init file */

/*[clinic input]
readline.read_init_file

    filename as filename_obj: object = None
    /

Execute a readline initialization file.

The default filename is the last filename used.
[clinic start generated code]*/

static PyObject *
readline_read_init_file_impl(PyObject *module, PyObject *filename_obj)
/*[clinic end generated code: output=8e059b676142831e input=4c80c473e448139d]*/
{
    PyObject *filename_bytes;
    if (filename_obj != Py_None) {
        if (!PyUnicode_FSConverter(filename_obj, &filename_bytes))
            return NULL;
        errno = rl_read_init_file(PyBytes_AS_STRING(filename_bytes));
        Py_DECREF(filename_bytes);
    } else
        errno = rl_read_init_file(NULL);
    if (errno)
        return PyErr_SetFromErrno(PyExc_OSError);
    disable_bracketed_paste();
    Py_RETURN_NONE;
}

/* Exported function to load a readline history file */

/*[clinic input]
readline.read_history_file

    filename as filename_obj: object = None
    /

Load a readline history file.

The default filename is ~/.history.
[clinic start generated code]*/

static PyObject *
readline_read_history_file_impl(PyObject *module, PyObject *filename_obj)
/*[clinic end generated code: output=66a951836fb54fbb input=3d29d755b7e6932e]*/
{
    PyObject *filename_bytes;
    if (filename_obj != Py_None) {
        if (!PyUnicode_FSConverter(filename_obj, &filename_bytes))
            return NULL;
        errno = read_history(PyBytes_AS_STRING(filename_bytes));
        Py_DECREF(filename_bytes);
    } else
        errno = read_history(NULL);
    if (errno)
        return PyErr_SetFromErrno(PyExc_OSError);
    Py_RETURN_NONE;
}

static int _history_length = -1; /* do not truncate history by default */

/* Exported function to save a readline history file */

/*[clinic input]
readline.write_history_file

    filename as filename_obj: object = None
    /

Save a readline history file.

The default filename is ~/.history.
[clinic start generated code]*/

static PyObject *
readline_write_history_file_impl(PyObject *module, PyObject *filename_obj)
/*[clinic end generated code: output=fbcad13d8ef59ae6 input=28a8e062fe363703]*/
{
    PyObject *filename_bytes;
    const char *filename;
    int err;
    if (filename_obj != Py_None) {
        if (!PyUnicode_FSConverter(filename_obj, &filename_bytes))
            return NULL;
        filename = PyBytes_AS_STRING(filename_bytes);
    } else {
        filename_bytes = NULL;
        filename = NULL;
    }
    errno = err = write_history(filename);
    if (!err && _history_length >= 0)
        history_truncate_file(filename, _history_length);
    Py_XDECREF(filename_bytes);
    errno = err;
    if (errno)
        return PyErr_SetFromErrno(PyExc_OSError);
    Py_RETURN_NONE;
}

#ifdef HAVE_RL_APPEND_HISTORY
/* Exported function to save part of a readline history file */

/*[clinic input]
readline.append_history_file

    nelements: int
    filename as filename_obj: object = None
    /

Append the last nelements items of the history list to file.

The default filename is ~/.history.
[clinic start generated code]*/

static PyObject *
readline_append_history_file_impl(PyObject *module, int nelements,
                                  PyObject *filename_obj)
/*[clinic end generated code: output=5df06fc9da56e4e4 input=784b774db3a4b7c5]*/
{
    PyObject *filename_bytes;
    const char *filename;
    int err;
    if (filename_obj != Py_None) {
        if (!PyUnicode_FSConverter(filename_obj, &filename_bytes))
            return NULL;
        filename = PyBytes_AS_STRING(filename_bytes);
    } else {
        filename_bytes = NULL;
        filename = NULL;
    }
    errno = err = append_history(
        nelements - libedit_append_replace_history_offset, filename);
    if (!err && _history_length >= 0)
        history_truncate_file(filename, _history_length);
    Py_XDECREF(filename_bytes);
    errno = err;
    if (errno)
        return PyErr_SetFromErrno(PyExc_OSError);
    Py_RETURN_NONE;
}
#endif


/* Set history length */

/*[clinic input]
readline.set_history_length

    length: int
    /

Set the maximal number of lines which will be written to the history file.

A negative length is used to inhibit history truncation.
[clinic start generated code]*/

static PyObject *
readline_set_history_length_impl(PyObject *module, int length)
/*[clinic end generated code: output=e161a53e45987dc7 input=b8901bf16488b760]*/
{
    _history_length = length;
    Py_RETURN_NONE;
}

/* Get history length */

/*[clinic input]
readline.get_history_length

Return the maximum number of lines that will be written to the history file.
[clinic start generated code]*/

static PyObject *
readline_get_history_length_impl(PyObject *module)
/*[clinic end generated code: output=83a2eeae35b6d2b9 input=5dce2eeba4327817]*/
{
    return PyLong_FromLong(_history_length);
}

/* Generic hook function setter */

static PyObject *
set_hook(const char *funcname, PyObject **hook_var, PyObject *function)
{
    if (function == Py_None) {
        Py_CLEAR(*hook_var);
    }
    else if (PyCallable_Check(function)) {
        Py_INCREF(function);
        Py_XSETREF(*hook_var, function);
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "set_%.50s(func): argument not callable",
                     funcname);
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
readline.set_completion_display_matches_hook

    function: object = None
    /

Set or remove the completion display function.

The function is called as
  function(substitution, [matches], longest_match_length)
once each time matches need to be displayed.
[clinic start generated code]*/

static PyObject *
readline_set_completion_display_matches_hook_impl(PyObject *module,
                                                  PyObject *function)
/*[clinic end generated code: output=516e5cb8db75a328 input=4f0bfd5ab0179a26]*/
{
    PyObject *result = set_hook("completion_display_matches_hook",
                    &readlinestate_global->completion_display_matches_hook,
                    function);
#ifdef HAVE_RL_COMPLETION_DISPLAY_MATCHES_HOOK
    /* We cannot set this hook globally, since it replaces the
       default completion display. */
    rl_completion_display_matches_hook =
        readlinestate_global->completion_display_matches_hook ?
#if defined(_RL_FUNCTION_TYPEDEF)
        (rl_compdisp_func_t *)on_completion_display_matches_hook : 0;
#else
        (VFunction *)on_completion_display_matches_hook : 0;
#endif
#endif
    return result;

}

/*[clinic input]
readline.set_startup_hook

    function: object = None
    /

Set or remove the function invoked by the rl_startup_hook callback.

The function is called with no arguments just
before readline prints the first prompt.
[clinic start generated code]*/

static PyObject *
readline_set_startup_hook_impl(PyObject *module, PyObject *function)
/*[clinic end generated code: output=02cd0e0c4fa082ad input=7783b4334b26d16d]*/
{
    return set_hook("startup_hook", &readlinestate_global->startup_hook,
            function);
}

#ifdef HAVE_RL_PRE_INPUT_HOOK

/* Set pre-input hook */

/*[clinic input]
readline.set_pre_input_hook

    function: object = None
    /

Set or remove the function invoked by the rl_pre_input_hook callback.

The function is called with no arguments after the first prompt
has been printed and just before readline starts reading input
characters.
[clinic start generated code]*/

static PyObject *
readline_set_pre_input_hook_impl(PyObject *module, PyObject *function)
/*[clinic end generated code: output=fe1a96505096f464 input=4f3eaeaf7ce1fdbe]*/
{
    return set_hook("pre_input_hook", &readlinestate_global->pre_input_hook,
            function);
}
#endif


/* Get the completion type for the scope of the tab-completion */

/*[clinic input]
readline.get_completion_type

Get the type of completion being attempted.
[clinic start generated code]*/

static PyObject *
readline_get_completion_type_impl(PyObject *module)
/*[clinic end generated code: output=5c54d58a04997c07 input=04b92bc7a82dac91]*/
{
  return PyLong_FromLong(rl_completion_type);
}

/* Get the beginning index for the scope of the tab-completion */

/*[clinic input]
readline.get_begidx

Get the beginning index of the completion scope.
[clinic start generated code]*/

static PyObject *
readline_get_begidx_impl(PyObject *module)
/*[clinic end generated code: output=362616ee8ed1b2b1 input=e083b81c8eb4bac3]*/
{
    Py_INCREF(readlinestate_global->begidx);
    return readlinestate_global->begidx;
}

/* Get the ending index for the scope of the tab-completion */

/*[clinic input]
readline.get_endidx

Get the ending index of the completion scope.
[clinic start generated code]*/

static PyObject *
readline_get_endidx_impl(PyObject *module)
/*[clinic end generated code: output=7f763350b12d7517 input=d4c7e34a625fd770]*/
{
    Py_INCREF(readlinestate_global->endidx);
    return readlinestate_global->endidx;
}

/* Set the tab-completion word-delimiters that readline uses */

/*[clinic input]
readline.set_completer_delims

    string: object
    /

Set the word delimiters for completion.
[clinic start generated code]*/

static PyObject *
readline_set_completer_delims(PyObject *module, PyObject *string)
/*[clinic end generated code: output=4305b266106c4f1f input=ae945337ebd01e20]*/
{
    char *break_chars;
    PyObject *encoded = encode(string);
    if (encoded == NULL) {
        return NULL;
    }
    /* Keep a reference to the allocated memory in the module state in case
       some other module modifies rl_completer_word_break_characters
       (see issue #17289). */
    break_chars = strdup(PyBytes_AS_STRING(encoded));
    Py_DECREF(encoded);
    if (break_chars) {
        free(completer_word_break_characters);
        completer_word_break_characters = break_chars;
        rl_completer_word_break_characters = break_chars;
        Py_RETURN_NONE;
    }
    else
        return PyErr_NoMemory();
}

/* _py_free_history_entry: Utility function to free a history entry. */

#if defined(RL_READLINE_VERSION) && RL_READLINE_VERSION >= 0x0500

/* Readline version >= 5.0 introduced a timestamp field into the history entry
   structure; this needs to be freed to avoid a memory leak.  This version of
   readline also introduced the handy 'free_history_entry' function, which
   takes care of the timestamp. */

static void
_py_free_history_entry(HIST_ENTRY *entry)
{
    histdata_t data = free_history_entry(entry);
    free(data);
}

#else

/* No free_history_entry function;  free everything manually. */

static void
_py_free_history_entry(HIST_ENTRY *entry)
{
    if (entry->line)
        free((void *)entry->line);
    if (entry->data)
        free(entry->data);
    free(entry);
}

#endif

/*[clinic input]
readline.remove_history_item

    pos as entry_number: int
    /

Remove history item given by its zero-based position.
[clinic start generated code]*/

static PyObject *
readline_remove_history_item_impl(PyObject *module, int entry_number)
/*[clinic end generated code: output=ab114f029208c7e8 input=f248beb720ff1838]*/
{
    HIST_ENTRY *entry;

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
    _py_free_history_entry(entry);
    Py_RETURN_NONE;
}

/*[clinic input]
readline.replace_history_item

    pos as entry_number: int
    line: unicode
    /

Replaces history item given by its position with contents of line.

pos is zero-based.
[clinic start generated code]*/

static PyObject *
readline_replace_history_item_impl(PyObject *module, int entry_number,
                                   PyObject *line)
/*[clinic end generated code: output=f8cec2770ca125eb input=368bb66fe5ee5222]*/
{
    PyObject *encoded;
    HIST_ENTRY *old_entry;

    if (entry_number < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "History index cannot be negative");
        return NULL;
    }
    encoded = encode(line);
    if (encoded == NULL) {
        return NULL;
    }
    old_entry = replace_history_entry(
        entry_number + libedit_append_replace_history_offset,
        PyBytes_AS_STRING(encoded), (void *)NULL);
    Py_DECREF(encoded);
    if (!old_entry) {
        PyErr_Format(PyExc_ValueError,
                     "No history item at position %d",
                     entry_number);
        return NULL;
    }
    /* free memory allocated for the old history entry */
    _py_free_history_entry(old_entry);
    Py_RETURN_NONE;
}

/* Add a line to the history buffer */

/*[clinic input]
readline.add_history

    string: object
    /

Add an item to the history buffer.
[clinic start generated code]*/

static PyObject *
readline_add_history(PyObject *module, PyObject *string)
/*[clinic end generated code: output=b107b7e8106e803d input=e57c1cf6bc68d7e3]*/
{
    PyObject *encoded = encode(string);
    if (encoded == NULL) {
        return NULL;
    }
    add_history(PyBytes_AS_STRING(encoded));
    Py_DECREF(encoded);
    Py_RETURN_NONE;
}

static int should_auto_add_history = 1;

/* Enable or disable automatic history */

/*[clinic input]
readline.set_auto_history

    enabled as _should_auto_add_history: bool
    /

Enables or disables automatic history.
[clinic start generated code]*/

static PyObject *
readline_set_auto_history_impl(PyObject *module,
                               int _should_auto_add_history)
/*[clinic end generated code: output=619c6968246fd82b input=3d413073a1a03355]*/
{
    should_auto_add_history = _should_auto_add_history;
    Py_RETURN_NONE;
}


/* Get the tab-completion word-delimiters that readline uses */

/*[clinic input]
readline.get_completer_delims

Get the word delimiters for completion.
[clinic start generated code]*/

static PyObject *
readline_get_completer_delims_impl(PyObject *module)
/*[clinic end generated code: output=6b060280fa68ef43 input=e36eb14fb8a1f08a]*/
{
    return decode(rl_completer_word_break_characters);
}

/* Set the completer function */

/*[clinic input]
readline.set_completer

    function: object = None
    /

Set or remove the completer function.

The function is called as function(text, state),
for state in 0, 1, 2, ..., until it returns a non-string.
It should return the next possible completion starting with 'text'.
[clinic start generated code]*/

static PyObject *
readline_set_completer_impl(PyObject *module, PyObject *function)
/*[clinic end generated code: output=171a2a60f81d3204 input=51e81e13118eb877]*/
{
    return set_hook("completer", &readlinestate_global->completer, function);
}

/*[clinic input]
readline.get_completer

Get the current completer function.
[clinic start generated code]*/

static PyObject *
readline_get_completer_impl(PyObject *module)
/*[clinic end generated code: output=6e6bbd8226d14475 input=6457522e56d70d13]*/
{
    if (readlinestate_global->completer == NULL) {
        Py_RETURN_NONE;
    }
    Py_INCREF(readlinestate_global->completer);
    return readlinestate_global->completer;
}

/* Private function to get current length of history.  XXX It may be
 * possible to replace this with a direct use of history_length instead,
 * but it's not clear whether BSD's libedit keeps history_length up to date.
 * See issue #8065.*/

static int
_py_get_history_length(void)
{
    HISTORY_STATE *hist_st = history_get_history_state();
    int length = hist_st->length;
    /* the history docs don't say so, but the address of hist_st changes each
       time history_get_history_state is called which makes me think it's
       freshly malloc'd memory...  on the other hand, the address of the last
       line stays the same as long as history isn't extended, so it appears to
       be malloc'd but managed by the history package... */
    free(hist_st);
    return length;
}

/* Exported function to get any element of history */

/*[clinic input]
readline.get_history_item

    index as idx: int
    /

Return the current contents of history item at one-based index.
[clinic start generated code]*/

static PyObject *
readline_get_history_item_impl(PyObject *module, int idx)
/*[clinic end generated code: output=83d3e53ea5f34b3d input=8adf5c80e6c7ff2b]*/
{
    HIST_ENTRY *hist_ent;

    if (using_libedit_emulation) {
        /* Older versions of libedit's readline emulation
         * use 0-based indexes, while readline and newer
         * versions of libedit use 1-based indexes.
         */
        int length = _py_get_history_length();

        idx = idx - 1 + libedit_history_start;

        /*
         * Apple's readline emulation crashes when
         * the index is out of range, therefore
         * test for that and fail gracefully.
         */
        if (idx < (0 + libedit_history_start)
                || idx >= (length + libedit_history_start)) {
            Py_RETURN_NONE;
        }
    }
    if ((hist_ent = history_get(idx)))
        return decode(hist_ent->line);
    else {
        Py_RETURN_NONE;
    }
}

/* Exported function to get current length of history */

/*[clinic input]
readline.get_current_history_length

Return the current (not the maximum) length of history.
[clinic start generated code]*/

static PyObject *
readline_get_current_history_length_impl(PyObject *module)
/*[clinic end generated code: output=436b294f12ba1e3f input=9cb3f431a68d071f]*/
{
    return PyLong_FromLong((long)_py_get_history_length());
}

/* Exported function to read the current line buffer */

/*[clinic input]
readline.get_line_buffer

Return the current contents of the line buffer.
[clinic start generated code]*/

static PyObject *
readline_get_line_buffer_impl(PyObject *module)
/*[clinic end generated code: output=d22f9025ecad80e4 input=5f5fbc0d12c69412]*/
{
    return decode(rl_line_buffer);
}

#ifdef HAVE_RL_COMPLETION_APPEND_CHARACTER

/* Exported function to clear the current history */

/*[clinic input]
readline.clear_history

Clear the current readline history.
[clinic start generated code]*/

static PyObject *
readline_clear_history_impl(PyObject *module)
/*[clinic end generated code: output=1f2dbb0dfa5d5ebb input=208962c4393f5d16]*/
{
    clear_history();
    Py_RETURN_NONE;
}
#endif


/* Exported function to insert text into the line buffer */

/*[clinic input]
readline.insert_text

    string: object
    /

Insert text into the line buffer at the cursor position.
[clinic start generated code]*/

static PyObject *
readline_insert_text(PyObject *module, PyObject *string)
/*[clinic end generated code: output=23d792821d320c19 input=bc96c3c848d5ccb5]*/
{
    PyObject *encoded = encode(string);
    if (encoded == NULL) {
        return NULL;
    }
    rl_insert_text(PyBytes_AS_STRING(encoded));
    Py_DECREF(encoded);
    Py_RETURN_NONE;
}

/* Redisplay the line buffer */

/*[clinic input]
readline.redisplay

Change what's displayed on the screen to reflect contents of the line buffer.
[clinic start generated code]*/

static PyObject *
readline_redisplay_impl(PyObject *module)
/*[clinic end generated code: output=a8b9725827c3c34b input=b485151058d75edc]*/
{
    rl_redisplay();
    Py_RETURN_NONE;
}

#include "clinic/readline.c.h"

/* Table of functions exported by the module */

static struct PyMethodDef readline_methods[] =
{
    READLINE_PARSE_AND_BIND_METHODDEF
    READLINE_GET_LINE_BUFFER_METHODDEF
    READLINE_INSERT_TEXT_METHODDEF
    READLINE_REDISPLAY_METHODDEF
    READLINE_READ_INIT_FILE_METHODDEF
    READLINE_READ_HISTORY_FILE_METHODDEF
    READLINE_WRITE_HISTORY_FILE_METHODDEF
#ifdef HAVE_RL_APPEND_HISTORY
    READLINE_APPEND_HISTORY_FILE_METHODDEF
#endif
    READLINE_GET_HISTORY_ITEM_METHODDEF
    READLINE_GET_CURRENT_HISTORY_LENGTH_METHODDEF
    READLINE_SET_HISTORY_LENGTH_METHODDEF
    READLINE_GET_HISTORY_LENGTH_METHODDEF
    READLINE_SET_COMPLETER_METHODDEF
    READLINE_GET_COMPLETER_METHODDEF
    READLINE_GET_COMPLETION_TYPE_METHODDEF
    READLINE_GET_BEGIDX_METHODDEF
    READLINE_GET_ENDIDX_METHODDEF
    READLINE_SET_COMPLETER_DELIMS_METHODDEF
    READLINE_SET_AUTO_HISTORY_METHODDEF
    READLINE_ADD_HISTORY_METHODDEF
    READLINE_REMOVE_HISTORY_ITEM_METHODDEF
    READLINE_REPLACE_HISTORY_ITEM_METHODDEF
    READLINE_GET_COMPLETER_DELIMS_METHODDEF
    READLINE_SET_COMPLETION_DISPLAY_MATCHES_HOOK_METHODDEF
    READLINE_SET_STARTUP_HOOK_METHODDEF
#ifdef HAVE_RL_PRE_INPUT_HOOK
    READLINE_SET_PRE_INPUT_HOOK_METHODDEF
#endif
#ifdef HAVE_RL_COMPLETION_APPEND_CHARACTER
    READLINE_CLEAR_HISTORY_METHODDEF
#endif
    {0, 0}
};


/* C function to call the Python hooks. */

static int
on_hook(PyObject *func)
{
    int result = 0;
    if (func != NULL) {
        PyObject *r;
        r = PyObject_CallNoArgs(func);
        if (r == NULL)
            goto error;
        if (r == Py_None)
            result = 0;
        else {
            result = _PyLong_AsInt(r);
            if (result == -1 && PyErr_Occurred())
                goto error;
        }
        Py_DECREF(r);
        goto done;
      error:
        PyErr_Clear();
        Py_XDECREF(r);
      done:
        return result;
    }
    return result;
}

static int
#if defined(_RL_FUNCTION_TYPEDEF)
on_startup_hook(void)
#else
on_startup_hook()
#endif
{
    int r;
    PyGILState_STATE gilstate = PyGILState_Ensure();
    r = on_hook(readlinestate_global->startup_hook);
    PyGILState_Release(gilstate);
    return r;
}

#ifdef HAVE_RL_PRE_INPUT_HOOK
static int
#if defined(_RL_FUNCTION_TYPEDEF)
on_pre_input_hook(void)
#else
on_pre_input_hook()
#endif
{
    int r;
    PyGILState_STATE gilstate = PyGILState_Ensure();
    r = on_hook(readlinestate_global->pre_input_hook);
    PyGILState_Release(gilstate);
    return r;
}
#endif


/* C function to call the Python completion_display_matches */

#ifdef HAVE_RL_COMPLETION_DISPLAY_MATCHES_HOOK
static void
on_completion_display_matches_hook(char **matches,
                                   int num_matches, int max_length)
{
    int i;
    PyObject *sub, *m=NULL, *s=NULL, *r=NULL;
    PyGILState_STATE gilstate = PyGILState_Ensure();
    m = PyList_New(num_matches);
    if (m == NULL)
        goto error;
    for (i = 0; i < num_matches; i++) {
        s = decode(matches[i+1]);
        if (s == NULL)
            goto error;
        PyList_SET_ITEM(m, i, s);
    }
    sub = decode(matches[0]);
    r = PyObject_CallFunction(readlinestate_global->completion_display_matches_hook,
                              "NNi", sub, m, max_length);

    m=NULL;

    if (r == NULL ||
        (r != Py_None && PyLong_AsLong(r) == -1 && PyErr_Occurred())) {
        goto error;
    }
    Py_CLEAR(r);

    if (0) {
    error:
        PyErr_Clear();
        Py_XDECREF(m);
        Py_XDECREF(r);
    }
    PyGILState_Release(gilstate);
}

#endif

#ifdef HAVE_RL_RESIZE_TERMINAL
static volatile sig_atomic_t sigwinch_received;
static PyOS_sighandler_t sigwinch_ohandler;

static void
readline_sigwinch_handler(int signum)
{
    sigwinch_received = 1;
    if (sigwinch_ohandler &&
            sigwinch_ohandler != SIG_IGN && sigwinch_ohandler != SIG_DFL)
        sigwinch_ohandler(signum);

#ifndef HAVE_SIGACTION
    /* If the handler was installed with signal() rather than sigaction(),
    we need to reinstall it. */
    PyOS_setsig(SIGWINCH, readline_sigwinch_handler);
#endif
}
#endif

/* C function to call the Python completer. */

static char *
on_completion(const char *text, int state)
{
    char *result = NULL;
    if (readlinestate_global->completer != NULL) {
        PyObject *r = NULL, *t;
        PyGILState_STATE gilstate = PyGILState_Ensure();
        rl_attempted_completion_over = 1;
        t = decode(text);
        r = PyObject_CallFunction(readlinestate_global->completer, "Ni", t, state);
        if (r == NULL)
            goto error;
        if (r == Py_None) {
            result = NULL;
        }
        else {
            PyObject *encoded = encode(r);
            if (encoded == NULL)
                goto error;
            result = strdup(PyBytes_AS_STRING(encoded));
            Py_DECREF(encoded);
        }
        Py_DECREF(r);
        goto done;
      error:
        PyErr_Clear();
        Py_XDECREF(r);
      done:
        PyGILState_Release(gilstate);
        return result;
    }
    return result;
}


/* A more flexible constructor that saves the "begidx" and "endidx"
 * before calling the normal completer */

static char **
flex_complete(const char *text, int start, int end)
{
    char **result;
    char saved;
    size_t start_size, end_size;
    wchar_t *s;
    PyGILState_STATE gilstate = PyGILState_Ensure();
#ifdef HAVE_RL_COMPLETION_APPEND_CHARACTER
    rl_completion_append_character ='\0';
#endif
#ifdef HAVE_RL_COMPLETION_SUPPRESS_APPEND
    rl_completion_suppress_append = 0;
#endif

    saved = rl_line_buffer[start];
    rl_line_buffer[start] = 0;
    s = Py_DecodeLocale(rl_line_buffer, &start_size);
    rl_line_buffer[start] = saved;
    if (s == NULL) {
        goto done;
    }
    PyMem_RawFree(s);
    saved = rl_line_buffer[end];
    rl_line_buffer[end] = 0;
    s = Py_DecodeLocale(rl_line_buffer + start, &end_size);
    rl_line_buffer[end] = saved;
    if (s == NULL) {
        goto done;
    }
    PyMem_RawFree(s);
    start = (int)start_size;
    end = start + (int)end_size;

done:
    Py_XDECREF(readlinestate_global->begidx);
    Py_XDECREF(readlinestate_global->endidx);
    readlinestate_global->begidx = PyLong_FromLong((long) start);
    readlinestate_global->endidx = PyLong_FromLong((long) end);
    result = completion_matches((char *)text, *on_completion);
    PyGILState_Release(gilstate);
    return result;
}


/* Helper to initialize GNU readline properly.
   Return -1 on memory allocation failure, return 0 on success. */
static int
setup_readline(readlinestate *mod_state)
{
#ifdef SAVE_LOCALE
    char *saved_locale = strdup(setlocale(LC_CTYPE, NULL));
    if (!saved_locale) {
        return -1;
    }
#endif

    /* The name must be defined before initialization */
    rl_readline_name = "python";

    /* the libedit readline emulation resets key bindings etc
     * when calling rl_initialize.  So call it upfront
     */
    if (using_libedit_emulation)
        rl_initialize();

    /* Detect if libedit's readline emulation uses 0-based
     * indexing or 1-based indexing.
     */
    add_history("1");
    if (history_get(1) == NULL) {
        libedit_history_start = 0;
    } else {
        libedit_history_start = 1;
    }
    /* Some libedit implementations use 1 based indexing on
     * replace_history_entry where libreadline uses 0 based.
     * The API our module presents is supposed to be 0 based.
     * It's a mad mad mad mad world.
     */
    {
        add_history("2");
        HIST_ENTRY *old_entry = replace_history_entry(1, "X", NULL);
        _py_free_history_entry(old_entry);
        HIST_ENTRY *item = history_get(libedit_history_start);
        if (item && item->line && strcmp(item->line, "X")) {
            libedit_append_replace_history_offset = 0;
        } else {
            libedit_append_replace_history_offset = 1;
        }
    }
    clear_history();

    using_history();

    /* Force rebind of TAB to insert-tab */
    rl_bind_key('\t', rl_insert);
    /* Bind both ESC-TAB and ESC-ESC to the completion function */
    rl_bind_key_in_map ('\t', rl_complete, emacs_meta_keymap);
    rl_bind_key_in_map ('\033', rl_complete, emacs_meta_keymap);
#ifdef HAVE_RL_RESIZE_TERMINAL
    /* Set up signal handler for window resize */
    sigwinch_ohandler = PyOS_setsig(SIGWINCH, readline_sigwinch_handler);
#endif
    /* Set our hook functions */
    rl_startup_hook = on_startup_hook;
#ifdef HAVE_RL_PRE_INPUT_HOOK
    rl_pre_input_hook = on_pre_input_hook;
#endif
    /* Set our completion function */
    rl_attempted_completion_function = flex_complete;
    /* Set Python word break characters */
    completer_word_break_characters =
        strdup(" \t\n`~!@#$%^&*()-=+[{]}\\|;:'\",<>/?");
        /* All nonalphanums except '.' */
    rl_completer_word_break_characters = completer_word_break_characters;

    mod_state->begidx = PyLong_FromLong(0L);
    mod_state->endidx = PyLong_FromLong(0L);

    if (!using_libedit_emulation)
    {
        if (!isatty(STDOUT_FILENO)) {
            /* Issue #19884: stdout is not a terminal. Disable meta modifier
               keys to not write the ANSI sequence "\033[1034h" into stdout. On
               terminals supporting 8 bit characters like TERM=xterm-256color
               (which is now the default Fedora since Fedora 18), the meta key is
               used to enable support of 8 bit characters (ANSI sequence
               "\033[1034h").

               With libedit, this call makes readline() crash. */
            rl_variable_bind ("enable-meta-key", "off");
        }
    }

    /* Initialize (allows .inputrc to override)
     *
     * XXX: A bug in the readline-2.2 library causes a memory leak
     * inside this function.  Nothing we can do about it.
     */
    if (using_libedit_emulation)
        rl_read_init_file(NULL);
    else
        rl_initialize();

    disable_bracketed_paste();

    RESTORE_LOCALE(saved_locale)
    return 0;
}

/* Wrapper around GNU readline that handles signals differently. */

static char *completed_input_string;
static void
rlhandler(char *text)
{
    completed_input_string = text;
    rl_callback_handler_remove();
}

static char *
readline_until_enter_or_signal(const char *prompt, int *signal)
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
        int has_input = 0, err = 0;

        while (!has_input)
        {               struct timeval timeout = {0, 100000}; /* 0.1 seconds */

            /* [Bug #1552726] Only limit the pause if an input hook has been
               defined.  */
            struct timeval *timeoutp = NULL;
            if (PyOS_InputHook)
                timeoutp = &timeout;
#ifdef HAVE_RL_RESIZE_TERMINAL
            /* Update readline's view of the window size after SIGWINCH */
            if (sigwinch_received) {
                sigwinch_received = 0;
                rl_resize_terminal();
            }
#endif
            FD_SET(fileno(rl_instream), &selectset);
            /* select resets selectset if no input was available */
            has_input = select(fileno(rl_instream) + 1, &selectset,
                               NULL, NULL, timeoutp);
            err = errno;
            if(PyOS_InputHook) PyOS_InputHook();
        }

        if (has_input > 0) {
            rl_callback_read_char();
        }
        else if (err == EINTR) {
            int s;
            PyEval_RestoreThread(_PyOS_ReadlineTState);
            s = PyErr_CheckSignals();
            PyEval_SaveThread();
            if (s < 0) {
                rl_free_line_state();
#if defined(RL_READLINE_VERSION) && RL_READLINE_VERSION >= 0x0700
                rl_callback_sigcleanup();
#endif
                rl_cleanup_after_signal();
                rl_callback_handler_remove();
                *signal = 1;
                completed_input_string = NULL;
            }
        }
    }

    return completed_input_string;
}


static char *
call_readline(FILE *sys_stdin, FILE *sys_stdout, const char *prompt)
{
    size_t n;
    char *p;
    int signal;

#ifdef SAVE_LOCALE
    char *saved_locale = strdup(setlocale(LC_CTYPE, NULL));
    if (!saved_locale)
        Py_FatalError("not enough memory to save locale");
    _Py_SetLocaleFromEnv(LC_CTYPE);
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
    if (signal) {
        RESTORE_LOCALE(saved_locale)
        return NULL;
    }

    /* We got an EOF, return an empty string. */
    if (p == NULL) {
        p = PyMem_RawMalloc(1);
        if (p != NULL)
            *p = '\0';
        RESTORE_LOCALE(saved_locale)
        return p;
    }

    /* we have a valid line */
    n = strlen(p);
    if (should_auto_add_history && n > 0) {
        const char *line;
        int length = _py_get_history_length();
        if (length > 0) {
            HIST_ENTRY *hist_ent;
            if (using_libedit_emulation) {
                /* handle older 0-based or newer 1-based indexing */
                hist_ent = history_get(length + libedit_history_start - 1);
            } else
                hist_ent = history_get(length);
            line = hist_ent ? hist_ent->line : "";
        } else
            line = "";
        if (strcmp(p, line))
            add_history(p);
    }
    /* Copy the malloc'ed buffer into a PyMem_Malloc'ed one and
       release the original. */
    char *q = p;
    p = PyMem_RawMalloc(n+2);
    if (p != NULL) {
        memcpy(p, q, n);
        p[n] = '\n';
        p[n+1] = '\0';
    }
    free(q);
    RESTORE_LOCALE(saved_locale)
    return p;
}


/* Initialize the module */

PyDoc_STRVAR(doc_module,
"Importing this module enables command line editing using GNU readline.");

PyDoc_STRVAR(doc_module_le,
"Importing this module enables command line editing using libedit readline.");

static struct PyModuleDef readlinemodule = {
    PyModuleDef_HEAD_INIT,
    "readline",
    doc_module,
    sizeof(readlinestate),
    readline_methods,
    NULL,
    readline_traverse,
    readline_clear,
    readline_free
};


PyMODINIT_FUNC
PyInit_readline(void)
{
    PyObject *m;
    readlinestate *mod_state;

    if (strncmp(rl_library_version, libedit_version_tag, strlen(libedit_version_tag)) == 0) {
        using_libedit_emulation = 1;
    }

    if (using_libedit_emulation)
        readlinemodule.m_doc = doc_module_le;


    m = PyModule_Create(&readlinemodule);

    if (m == NULL)
        return NULL;

    if (PyModule_AddIntConstant(m, "_READLINE_VERSION",
                                RL_READLINE_VERSION) < 0) {
        goto error;
    }
    if (PyModule_AddIntConstant(m, "_READLINE_RUNTIME_VERSION",
                                rl_readline_version) < 0) {
        goto error;
    }
    if (PyModule_AddStringConstant(m, "_READLINE_LIBRARY_VERSION",
                                   rl_library_version) < 0)
    {
        goto error;
    }

    mod_state = (readlinestate *) PyModule_GetState(m);
    PyOS_ReadlineFunctionPointer = call_readline;
    if (setup_readline(mod_state) < 0) {
        PyErr_NoMemory();
        goto error;
    }

    return m;

error:
    Py_DECREF(m);
    return NULL;
}
