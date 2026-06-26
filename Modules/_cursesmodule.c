/*
 *   This is a curses module for Python.
 *
 *   Based on prior work by Lance Ellinghaus and Oliver Andrich
 *   Version 1.2 of this module: Copyright 1994 by Lance Ellinghouse,
 *    Cathedral City, California Republic, United States of America.
 *
 *   Version 1.5b1, heavily extended for ncurses by Oliver Andrich:
 *   Copyright 1996,1997 by Oliver Andrich, Koblenz, Germany.
 *
 *   Tidied for Python 1.6, and currently maintained by <amk@amk.ca>.
 *
 *   Permission is hereby granted, free of charge, to any person obtaining
 *   a copy of this source file to use, copy, modify, merge, or publish it
 *   subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included
 *   in all copies or in any new file that contains a substantial portion of
 *   this file.
 *
 *   THE  AUTHOR  MAKES  NO  REPRESENTATIONS ABOUT  THE  SUITABILITY  OF
 *   THE  SOFTWARE FOR  ANY  PURPOSE.  IT IS  PROVIDED  "AS IS"  WITHOUT
 *   EXPRESS OR  IMPLIED WARRANTY.  THE AUTHOR DISCLAIMS  ALL WARRANTIES
 *   WITH  REGARD TO  THIS  SOFTWARE, INCLUDING  ALL IMPLIED  WARRANTIES
 *   OF   MERCHANTABILITY,  FITNESS   FOR  A   PARTICULAR  PURPOSE   AND
 *   NON-INFRINGEMENT  OF THIRD  PARTY  RIGHTS. IN  NO  EVENT SHALL  THE
 *   AUTHOR  BE LIABLE  TO  YOU  OR ANY  OTHER  PARTY  FOR ANY  SPECIAL,
 *   INDIRECT,  OR  CONSEQUENTIAL  DAMAGES  OR  ANY  DAMAGES  WHATSOEVER
 *   WHETHER IN AN  ACTION OF CONTRACT, NEGLIGENCE,  STRICT LIABILITY OR
 *   ANY OTHER  ACTION ARISING OUT OF  OR IN CONNECTION WITH  THE USE OR
 *   PERFORMANCE OF THIS SOFTWARE.
 */

/*

  A number of SysV or ncurses functions don't have wrappers yet; if you
  need a given function, add it and send a patch.  See
  https://www.python.org/dev/patches/ for instructions on how to submit
  patches to Python.

  Here's a list of currently unsupported functions:

  addchnstr addchstr color_set define_key
  del_curterm dupwin inchnstr inchstr innstr keyok
  mcprint mvaddchnstr mvaddchstr mvcur mvinchnstr
  mvinchstr mvinnstr mmvwaddchnstr mvwaddchstr
  mvwinchnstr mvwinchstr mvwinnstr
  restartterm ripoffline scr_dump
  scr_init scr_restore scr_set scrl set_curterm setterm
  tgetent tgetflag tgetnum tgetstr tgoto timeout tputs
  vidattr vidputs waddchnstr waddchstr
  wcolor_set winchnstr winchstr winnstr wmouse_trafo wscrl

  Low-priority:
  slk_attr slk_attr_off slk_attr_on slk_attr_set slk_attroff
  slk_attron slk_attrset slk_clear slk_color slk_init slk_label
  slk_noutrefresh slk_refresh slk_restore slk_set slk_touch

  Menu extension (ncurses and probably SYSV):
  current_item free_item free_menu item_count item_description
  item_index item_init item_name item_opts item_opts_off
  item_opts_on item_term item_userptr item_value item_visible
  menu_back menu_driver menu_fore menu_format menu_grey
  menu_init menu_items menu_mark menu_opts menu_opts_off
  menu_opts_on menu_pad menu_pattern menu_request_by_name
  menu_request_name menu_spacing menu_sub menu_term menu_userptr
  menu_win new_item new_menu pos_menu_cursor post_menu
  scale_menu set_current_item set_item_init set_item_opts
  set_item_term set_item_userptr set_item_value set_menu_back
  set_menu_fore set_menu_format set_menu_grey set_menu_init
  set_menu_items set_menu_mark set_menu_opts set_menu_pad
  set_menu_pattern set_menu_spacing set_menu_sub set_menu_term
  set_menu_userptr set_menu_win set_top_row top_row unpost_menu

  Form extension (ncurses and probably SYSV):
  current_field data_ahead data_behind dup_field
  dynamic_fieldinfo field_arg field_back field_buffer
  field_count field_fore field_index field_info field_init
  field_just field_opts field_opts_off field_opts_on field_pad
  field_status field_term field_type field_userptr form_driver
  form_fields form_init form_opts form_opts_off form_opts_on
  form_page form_request_by_name form_request_name form_sub
  form_term form_userptr form_win free_field free_form
  link_field link_fieldtype move_field new_field new_form
  new_page pos_form_cursor post_form scale_form
  set_current_field set_field_back set_field_buffer
  set_field_fore set_field_init set_field_just set_field_opts
  set_field_pad set_field_status set_field_term set_field_type
  set_field_userptr set_fieldtype_arg set_fieldtype_choice
  set_form_fields set_form_init set_form_opts set_form_page
  set_form_sub set_form_term set_form_userptr set_form_win
  set_max_field set_new_page unpost_form


*/

/* Release Number */

static const char PyCursesVersion[] = "2.2";

/* Includes */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_capsule.h"     // _PyCapsule_SetTraverse()
#include "pycore_long.h"        // _PyLong_GetZero()
#include "pycore_structseq.h"   // _PyStructSequence_NewType()
#include "pycore_fileutils.h"   // _Py_dup(), _Py_set_inheritable()
#include "pycore_tuple.h"       // _PyTuple_HASH_XXPRIME_1

#ifdef __hpux
#define STRICT_SYSV_CURSES
#endif

#define CURSES_MODULE
#include "py_curses.h"

#if defined(HAVE_TERM_H) || defined(__sgi)
/* For termname, longname, putp, tigetflag, tigetnum, tigetstr, tparm
   which are not declared in SysV curses and for setupterm. */
#include <term.h>
/* Including <term.h> #defines many common symbols. */
#undef lines
#undef columns
#endif

#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif

#if !defined(NCURSES_VERSION) && (defined(sgi) || defined(__sun) || defined(SCO5))
#define STRICT_SYSV_CURSES       /* Don't use ncurses extensions */
typedef chtype attr_t;           /* No attr_t type is available */
#endif

#if defined(_AIX)
#define STRICT_SYSV_CURSES
#endif

#if defined(HAVE_NCURSESW) && NCURSES_EXT_FUNCS+0 >= 20170401 && NCURSES_EXT_COLORS+0 >= 20170401
#define _NCURSES_EXTENDED_COLOR_FUNCS   1
#else
#define _NCURSES_EXTENDED_COLOR_FUNCS   0
#endif

#if _NCURSES_EXTENDED_COLOR_FUNCS
#define _CURSES_COLOR_VAL_TYPE          int
#define _CURSES_COLOR_NUM_TYPE          int
#define _CURSES_INIT_COLOR_FUNC         init_extended_color
#define _CURSES_INIT_PAIR_FUNC          init_extended_pair
#define _COLOR_CONTENT_FUNC             extended_color_content
#define _CURSES_PAIR_CONTENT_FUNC       extended_pair_content
#else
#define _CURSES_COLOR_VAL_TYPE          short
#define _CURSES_COLOR_NUM_TYPE          short
#define _CURSES_INIT_COLOR_FUNC         init_color
#define _CURSES_INIT_PAIR_FUNC          init_pair
#define _COLOR_CONTENT_FUNC             color_content
#define _CURSES_PAIR_CONTENT_FUNC       pair_content
#endif  /* _NCURSES_EXTENDED_COLOR_FUNCS */

typedef struct {
    PyObject *error;                // curses exception type
    PyTypeObject *window_type;      // exposed by PyCursesWindow_Type
    PyTypeObject *screen_type;      // _curses.screen
#ifdef HAVE_NCURSESW
    PyTypeObject *complexchar_type; // _curses.complexchar
    PyTypeObject *complexstr_type;  // _curses.complexstr
#endif
    PyObject *topscreen;            // owned ref to the current screen object,
                                    // or NULL for the initscr() screen
} cursesmodule_state;

static inline cursesmodule_state *
get_cursesmodule_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (cursesmodule_state *)state;
}

static inline cursesmodule_state *
get_cursesmodule_state_by_cls(PyTypeObject *cls)
{
    void *state = PyType_GetModuleState(cls);
    assert(state != NULL);
    return (cursesmodule_state *)state;
}

static inline cursesmodule_state *
get_cursesmodule_state_by_win(PyCursesWindowObject *win)
{
    return get_cursesmodule_state_by_cls(Py_TYPE(win));
}

#define _PyCursesWindowObject_CAST(op)  ((PyCursesWindowObject *)(op))
#define _PyCursesScreenObject_CAST(op)  ((PyCursesScreenObject *)(op))

/*[clinic input]
module _curses
class _curses.window "PyCursesWindowObject *" "clinic_state()->window_type"
class _curses.screen "PyCursesScreenObject *" "clinic_state()->screen_type"
class _curses.complexchar "PyCursesComplexCharObject *" "clinic_state()->complexchar_type"
class _curses.complexstr "PyCursesComplexStrObject *" "get_cursesmodule_state_by_cls(type)->complexstr_type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=e9439fe0a704a26e]*/

/* Indicate whether the module has already been loaded or not. */
static int curses_module_loaded = 0;

/* Tells whether setupterm() has been called to initialise terminfo.  */
static int curses_setupterm_called = FALSE;

/* Tells whether initscr() has been called to initialise curses.  */
static int curses_initscr_called = FALSE;

/* Tells whether start_color() has been called to initialise color usage. */
static int curses_start_color_called = FALSE;

/* Encoding of the initial screen, used by module-level functions that have
   no window object to take it from (e.g. unctrl(), ungetch()).  This is a
   private copy: the window object that initscr() returns may be deallocated
   while these functions are still in use. */
static char *curses_screen_encoding = NULL;

/* Utility Error Procedures */

static void
_curses_format_error(cursesmodule_state *state,
                     const char *curses_funcname,
                     const char *python_funcname,
                     const char *return_value,
                     const char *default_message)
{
    assert(!PyErr_Occurred());
    if (python_funcname == NULL && curses_funcname == NULL) {
        PyErr_SetString(state->error, default_message);
    }
    else if (python_funcname == NULL) {
        (void)PyErr_Format(state->error, CURSES_ERROR_FORMAT,
                           curses_funcname, return_value);
    }
    else {
        assert(python_funcname != NULL);
        (void)PyErr_Format(state->error, CURSES_ERROR_VERBOSE_FORMAT,
                           curses_funcname, python_funcname, return_value);
    }
}

/*
 * Format a curses error for a function that returned ERR.
 *
 * Specify a non-NULL 'python_funcname' when the latter differs from
 * 'curses_funcname'. If both names are NULL, uses the 'catchall_ERR'
 * message instead.
 */
static void
_curses_set_error(cursesmodule_state *state,
                  const char *curses_funcname,
                  const char *python_funcname)
{
    _curses_format_error(state, curses_funcname, python_funcname,
                         "ERR", catchall_ERR);
}

/*
 * Format a curses error for a function that returned NULL.
 *
 * Specify a non-NULL 'python_funcname' when the latter differs from
 * 'curses_funcname'. If both names are NULL, uses the 'catchall_NULL'
 * message instead.
 */
static inline void
_curses_set_null_error(cursesmodule_state *state,
                       const char *curses_funcname,
                       const char *python_funcname)
{
    _curses_format_error(state, curses_funcname, python_funcname,
                         "NULL", catchall_NULL);
}

/* Same as _curses_set_error() for a module object. */
static void
curses_set_error(PyObject *module,
                 const char *curses_funcname,
                 const char *python_funcname)
{
    cursesmodule_state *state = get_cursesmodule_state(module);
    _curses_set_error(state, curses_funcname, python_funcname);
}

/* Same as _curses_set_null_error() for a module object. */
static void
curses_set_null_error(PyObject *module,
                      const char *curses_funcname,
                      const char *python_funcname)
{
    cursesmodule_state *state = get_cursesmodule_state(module);
    _curses_set_null_error(state, curses_funcname, python_funcname);
}

/* Same as _curses_set_error() for a Window object. */
static void
curses_window_set_error(PyCursesWindowObject *win,
                        const char *curses_funcname,
                        const char *python_funcname)
{
    cursesmodule_state *state = get_cursesmodule_state_by_win(win);
    _curses_set_error(state, curses_funcname, python_funcname);
}

/* Same as _curses_set_null_error() for a Window object. */
static void
curses_window_set_null_error(PyCursesWindowObject *win,
                             const char *curses_funcname,
                             const char *python_funcname)
{
    cursesmodule_state *state = get_cursesmodule_state_by_win(win);
    _curses_set_null_error(state, curses_funcname, python_funcname);
}

/* Utility Checking Procedures */

/*
 * Function to check that 'funcname' has been called by testing
 * the 'called' boolean. If an error occurs, an exception is
 * set and this returns 0. Otherwise, this returns 1.
 *
 * Since this function can be called in functions that do not
 * have a direct access to the module's state, '_curses.error'
 * is imported on demand.
 *
 * Use _PyCursesStatefulCheckFunction() if the module is given.
 */
static int
_PyCursesCheckFunction(int called, const char *funcname)
{
    if (called == TRUE) {
        return 1;
    }
    PyObject *exc = PyImport_ImportModuleAttrString("_curses", "error");
    if (exc != NULL) {
        PyErr_Format(exc, CURSES_ERROR_MUST_CALL_FORMAT, funcname);
        Py_DECREF(exc);
    }
    assert(PyErr_Occurred());
    return 0;
}

/*
 * Function to check that 'funcname' has been called by testing
 * the 'called'' boolean. If an error occurs, a PyCursesError is
 * set and this returns 0. Otherwise this returns 1.
 *
 * The exception type is obtained from the 'module' state.
 */
static int
_PyCursesStatefulCheckFunction(PyObject *module,
                               int called, const char *funcname)
{
    if (called == TRUE) {
        return 1;
    }
    cursesmodule_state *state = get_cursesmodule_state(module);
    PyErr_Format(state->error, CURSES_ERROR_MUST_CALL_FORMAT, funcname);
    return 0;
}

#define PyCursesStatefulSetupTermCalled(MODULE)                         \
    do {                                                                \
        if (!_PyCursesStatefulCheckFunction(MODULE,                     \
                                            curses_setupterm_called,    \
                                            "setupterm"))               \
        {                                                               \
            return 0;                                                   \
        }                                                               \
    } while (0)

#define PyCursesStatefulInitialised(MODULE)                         \
    do {                                                            \
        if (!_PyCursesStatefulCheckFunction(MODULE,                 \
                                            curses_initscr_called,  \
                                            "initscr"))             \
        {                                                           \
            return 0;                                               \
        }                                                           \
    } while (0)

#define PyCursesStatefulInitialisedColor(MODULE)                        \
    do {                                                                \
        if (!_PyCursesStatefulCheckFunction(MODULE,                     \
                                            curses_start_color_called,  \
                                            "start_color"))             \
        {                                                               \
            return 0;                                                   \
        }                                                               \
    } while (0)

/* Utility Functions */

/*
 * Check the return code from a curses function, returning None
 * on success and setting an exception on error.
 */

/*
 * Return None if 'code' is different from ERR (implementation-defined).
 * Otherwise, set an exception using curses_set_error() and the remaining
 * arguments, and return NULL.
 */
static PyObject *
curses_check_err(PyObject *module, int code,
                 const char *curses_funcname,
                 const char *python_funcname)
{
    if (code != ERR) {
        Py_RETURN_NONE;
    }
    curses_set_error(module, curses_funcname, python_funcname);
    return NULL;
}

/* Same as curses_check_err() for a Window object. */
static PyObject *
curses_window_check_err(PyCursesWindowObject *win, int code,
                        const char *curses_funcname,
                        const char *python_funcname)
{
    if (code != ERR) {
        Py_RETURN_NONE;
    }
    curses_window_set_error(win, curses_funcname, python_funcname);
    return NULL;
}

/* Convert an object to a byte (an integer of type chtype):

   - int
   - bytes of length 1
   - str of length 1

   Return 1 on success, 0 on error (invalid type or integer overflow). */
static int
PyCurses_ConvertToChtype(PyCursesWindowObject *win, PyObject *obj, chtype *ch)
{
    long value;
    if (PyBytes_Check(obj)) {
        if (PyBytes_GET_SIZE(obj) != 1) {
            PyErr_Format(PyExc_TypeError,
                         "expect int or bytes or str of length 1, "
                         "got a bytes of length %zd",
                         PyBytes_GET_SIZE(obj));
            return 0;
        }
        value = (unsigned char)PyBytes_AsString(obj)[0];
    }
    else if (PyUnicode_Check(obj)) {
        if (PyUnicode_GET_LENGTH(obj) != 1) {
            PyErr_Format(PyExc_TypeError,
                         "expect int or bytes or str of length 1, "
                         "got a str of length %zi",
                         PyUnicode_GET_LENGTH(obj));
            return 0;
        }
        value = PyUnicode_READ_CHAR(obj, 0);
        if (128 < value) {
            PyObject *bytes;
            const char *encoding;
            if (win)
                encoding = win->encoding;
            else
                encoding = curses_screen_encoding;
            bytes = PyUnicode_AsEncodedString(obj, encoding, NULL);
            if (bytes == NULL)
                return 0;
            if (PyBytes_GET_SIZE(bytes) == 1)
                value = (unsigned char)PyBytes_AS_STRING(bytes)[0];
            else
                value = -1;
            Py_DECREF(bytes);
            if (value < 0)
                goto overflow;
        }
    }
    else if (PyLong_CheckExact(obj)) {
        int long_overflow;
        value = PyLong_AsLongAndOverflow(obj, &long_overflow);
        if (long_overflow)
            goto overflow;
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "expect int or bytes or str of length 1, got %s",
                     Py_TYPE(obj)->tp_name);
        return 0;
    }
    *ch = (chtype)value;
    if ((long)*ch != value)
        goto overflow;
    return 1;

overflow:
    PyErr_SetString(PyExc_OverflowError,
                    "byte doesn't fit in chtype");
    return 0;
}

/* Convert an object to a byte (chtype) or a character (cchar_t):

    - int
    - bytes of length 1
    - str of length 1, or a spacing character followed by up to
      CCHARW_MAX - 1 combining characters

   Return:

    - 2 if obj is a character (written into *wch)
    - 1 if obj is a byte (written into *ch)
    - 0 on error: raise an exception */
#ifdef HAVE_NCURSESW
/* Convert a str -- a spacing character optionally followed by up to
   CCHARW_MAX - 1 combining characters -- into a wide-character cell.
   wch must point to a buffer of at least CCHARW_MAX + 1 wide characters.
   Return 0 on success, -1 with an exception set on error. */
static int
PyCurses_ConvertToWideCell(PyObject *obj, wchar_t *wch)
{
    assert(PyUnicode_Check(obj));
    Py_ssize_t nch = PyUnicode_AsWideChar(obj, wch, CCHARW_MAX + 1);
    if (nch < 0) {
        return -1;
    }
    if (nch == 0 || nch > CCHARW_MAX) {
        PyErr_Format(PyExc_TypeError,
                     "expect a string of 1 to %d characters, "
                     "got a str of length %zi",
                     (int)CCHARW_MAX, PyUnicode_GET_LENGTH(obj));
        return -1;
    }
    /* A lone control character is allowed (like addch(ord('\n'))), but in a
       multi-character cell the base must be a printable spacing character and
       the rest zero-width combining characters.  Check explicitly: otherwise
       setcchar() would silently drop a trailing spacing character, or fail
       with a generic error for a control-character base. */
    if (nch > 1) {
        int bad = wcwidth(wch[0]) < 0;
        for (Py_ssize_t i = 1; !bad && i < nch; i++) {
            bad = wcwidth(wch[i]) != 0;
        }
        if (bad) {
            PyErr_Format(PyExc_ValueError,
                         "a character cell must be a single spacing character "
                         "optionally followed by up to %d combining characters",
                         (int)(CCHARW_MAX - 1));
            return -1;
        }
    }
    return 0;
}
#endif

static int
PyCurses_ConvertToCchar_t(PyCursesWindowObject *win, PyObject *obj,
                          chtype *ch
#ifdef HAVE_NCURSESW
                          , wchar_t *wch
#endif
                          )
{
    long value;

    if (PyUnicode_Check(obj)) {
#ifdef HAVE_NCURSESW
        if (PyCurses_ConvertToWideCell(obj, wch) < 0) {
            return 0;
        }
        return 2;
#else
        return PyCurses_ConvertToChtype(win, obj, ch);
#endif
    }
    else if (PyBytes_Check(obj)) {
        if (PyBytes_GET_SIZE(obj) != 1) {
            PyErr_Format(PyExc_TypeError,
                         "expect int or bytes or str of length 1, "
                         "got a bytes of length %zd",
                         PyBytes_GET_SIZE(obj));
            return 0;
        }
        value = (unsigned char)PyBytes_AsString(obj)[0];
    }
    else if (PyLong_CheckExact(obj)) {
        int overflow;
        value = PyLong_AsLongAndOverflow(obj, &overflow);
        if (overflow) {
            PyErr_SetString(PyExc_OverflowError,
                            "int doesn't fit in long");
            return 0;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "expect int or bytes or str of length 1, got %s",
                     Py_TYPE(obj)->tp_name);
        return 0;
    }

    *ch = (chtype)value;
    if ((long)*ch != value) {
        PyErr_Format(PyExc_OverflowError,
                     "byte doesn't fit in chtype");
        return 0;
    }
    return 1;
}

/* Convert an object to a byte string (char*) or a wide character string
   (wchar_t*). Return:

    - 2 if obj is a character string (written into *wch)
    - 1 if obj is a byte string (written into *bytes)
    - 0 on error: raise an exception */
static int
PyCurses_ConvertToString(PyCursesWindowObject *win, PyObject *obj,
                         PyObject **bytes, wchar_t **wstr)
{
    char *str;
    if (PyUnicode_Check(obj)) {
#ifdef HAVE_NCURSESW
        assert (wstr != NULL);

        *wstr = PyUnicode_AsWideCharString(obj, NULL);
        if (*wstr == NULL)
            return 0;
        return 2;
#else
        assert (wstr == NULL);
        *bytes = PyUnicode_AsEncodedString(obj, win->encoding, NULL);
        if (*bytes == NULL)
            return 0;
        /* check for embedded null bytes */
        if (PyBytes_AsStringAndSize(*bytes, &str, NULL) < 0) {
            Py_CLEAR(*bytes);
            return 0;
        }
        return 1;
#endif
    }
    else if (PyBytes_Check(obj)) {
        *bytes = Py_NewRef(obj);
        /* check for embedded null bytes */
        if (PyBytes_AsStringAndSize(*bytes, &str, NULL) < 0) {
            Py_DECREF(obj);
            return 0;
        }
        return 1;
    }

    PyErr_Format(PyExc_TypeError, "expect bytes or str, got %s",
                 Py_TYPE(obj)->tp_name);
    return 0;
}

#ifdef HAVE_NCURSESW
typedef struct {
    PyObject_HEAD
    cchar_t cval;
} PyCursesComplexCharObject;

#define _PyCursesComplexCharObject_CAST(op)  ((PyCursesComplexCharObject *)(op))

/* An immutable packed array of cchar_t cells -- the "complex character
   string" counterpart of complexchar (as str is to a single character).
   It owns the contiguous buffer that win_wchnstr() fills directly, so a read
   and a re-write is a zero-copy round-trip. */
typedef struct {
    PyObject_VAR_HEAD
    cchar_t cells[1];   // ob_size cells, stored inline (variable-size object)
} PyCursesComplexStrObject;

#define _PyCursesComplexStrObject_CAST(op)  ((PyCursesComplexStrObject *)(op))

/* Build a single character cell from obj.

   Return 1 and store a chtype in *pch for an int or bytes, 2 and store a
   cchar_t (with *attr* applied) in *pwc for a str (a spacing character
   optionally followed by combining characters), or 0 with an exception set.

   obj may also be a complexchar, whose cell is used directly; it carries its
   own rendition, so supplying *attr* too (attr_given) is rejected. */
static int
PyCurses_ConvertToCell(PyCursesWindowObject *win, PyObject *obj, long attr,
                       int attr_given, const char *funcname,
                       chtype *pch, cchar_t *pwc)
{
    cursesmodule_state *state = get_cursesmodule_state_by_win(win);
    if (Py_IS_TYPE(obj, state->complexchar_type)) {
        if (attr_given) {
            PyErr_Format(PyExc_TypeError,
                         "%s(): attr cannot be specified together with "
                         "a complexchar", funcname);
            return 0;
        }
        *pwc = _PyCursesComplexCharObject_CAST(obj)->cval;
        return 2;
    }
    wchar_t wstr[CCHARW_MAX + 1];
    int type = PyCurses_ConvertToCchar_t(win, obj, pch, wstr);
    if (type == 2) {
        if (setcchar(pwc, wstr, (attr_t)attr, PAIR_NUMBER(attr), NULL) == ERR) {
            curses_window_set_error(win, "setcchar", funcname);
            return 0;
        }
    }
    return type;
}

/* Pack a wide-character cell, routing the color pair through the
   extended-color opts slot so it is not limited to a short (unlike the
   chtype COLOR_PAIR field).  Without that slot the pair must fit in the
   short that setcchar() takes; raise OverflowError instead of silently
   truncating a larger one. */
static int
curses_setcchar(cchar_t *wcval, const wchar_t *wstr, attr_t attrs, int pair)
{
#if _NCURSES_EXTENDED_COLOR_FUNCS
    /* The pair passed through the opts slot is authoritative and may exceed
       a short; ncurses then ignores the short argument, but clamp it into
       range so the int-to-short narrowing stays well-defined. */
    short spair = pair <= SHRT_MAX ? (short)pair : SHRT_MAX;
    return setcchar(wcval, wstr, attrs, spair, &pair);
#else
    if (pair > SHRT_MAX) {
        PyErr_Format(PyExc_OverflowError,
                     "color pair %d does not fit in a short", pair);
        return ERR;
    }
    return setcchar(wcval, wstr, attrs, (short)pair, NULL);
#endif
}

/* Unpack a wide-character cell into its text, attributes and color pair.
   The pair is read through the extended-color opts slot when available, so
   values above SHRT_MAX are preserved. */
static int
curses_getcchar(const cchar_t *wcval, wchar_t *wstr, attr_t *attrs, int *pair)
{
    short spair = 0;
#if _NCURSES_EXTENDED_COLOR_FUNCS
    int rtn = getcchar(wcval, wstr, attrs, &spair, pair);
#else
    int rtn = getcchar(wcval, wstr, attrs, &spair, NULL);
    if (rtn != ERR) {
        *pair = spair;
    }
#endif
    return rtn;
}

/* Hash one cell by value (text, attributes, pair) -- consistent with the
   equality comparison, not the raw cchar_t whose padding and unused text tail
   it ignores.  Zero the key first so those bytes are deterministic, then
   unpack into it.  Returns -1 with an exception set on the (practically
   impossible) getcchar() failure. */
static Py_hash_t
curses_cchar_hash(cursesmodule_state *state, const cchar_t *cell)
{
    struct {
        attr_t attrs;
        int pair;
        wchar_t wstr[CCHARW_MAX + 1];
    } key;
    memset(&key, 0, sizeof(key));
    if (curses_getcchar(cell, key.wstr, &key.attrs, &key.pair) == ERR) {
        PyErr_SetString(state->error, "getcchar() returned ERR");
        return -1;
    }
    return Py_HashBuffer(&key, sizeof(key));
}
#endif

static int
color_allow_default_converter(PyObject *arg, void *ptr)
{
    long color_number;
    int overflow;

    color_number = PyLong_AsLongAndOverflow(arg, &overflow);
    if (color_number == -1 && PyErr_Occurred())
        return 0;

    if (overflow > 0 || color_number >= COLORS) {
        PyErr_Format(PyExc_ValueError,
                     "Color number is greater than COLORS-1 (%d).",
                     COLORS - 1);
        return 0;
    }
    else if (overflow < 0 || color_number < 0) {
        color_number = -1;
    }

    *(int *)ptr = (int)color_number;
    return 1;
}

static int
color_converter(PyObject *arg, void *ptr)
{
    if (!color_allow_default_converter(arg, ptr)) {
        return 0;
    }
    if (*(int *)ptr < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "Color number is less than 0.");
        return 0;
    }
    return 1;
}

/*[python input]
class color_converter(CConverter):
    type = 'int'
    converter = 'color_converter'
[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=4260d2b6e66b3709]*/

/*[python input]
class color_allow_default_converter(CConverter):
    type = 'int'
    converter = 'color_allow_default_converter'
[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=975602bc058a872d]*/

static int
pair_converter(PyObject *arg, void *ptr)
{
    long pair_number;
    int overflow;

    pair_number = PyLong_AsLongAndOverflow(arg, &overflow);
    if (pair_number == -1 && PyErr_Occurred())
        return 0;

#if _NCURSES_EXTENDED_COLOR_FUNCS
    if (overflow > 0 || pair_number > INT_MAX) {
        PyErr_Format(PyExc_ValueError,
                     "Color pair is greater than maximum (%d).",
                     INT_MAX);
        return 0;
    }
#else
    if (overflow > 0 || pair_number >= COLOR_PAIRS) {
        PyErr_Format(PyExc_ValueError,
                     "Color pair is greater than COLOR_PAIRS-1 (%d).",
                     COLOR_PAIRS - 1);
        return 0;
    }
#endif
    else if (overflow < 0 || pair_number < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "Color pair is less than 0.");
        return 0;
    }

    *(int *)ptr = (int)pair_number;
    return 1;
}

/*[python input]
class pair_converter(CConverter):
    type = 'int'
    converter = 'pair_converter'
[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=1a918ae6a1b32af7]*/

static int
component_converter(PyObject *arg, void *ptr)
{
    long component;
    int overflow;

    component = PyLong_AsLongAndOverflow(arg, &overflow);
    if (component == -1 && PyErr_Occurred())
        return 0;

    if (overflow > 0 || component > 1000) {
        PyErr_SetString(PyExc_ValueError,
                        "Color component is greater than 1000");
        return 0;
    }
    else if (overflow < 0 || component < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "Color component is less than 0");
        return 0;
    }

    *(short *)ptr = (short)component;
    return 1;
}

/*[python input]
class component_converter(CConverter):
    type = 'short'
    converter = 'component_converter'
[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=38e9be01d33927fb]*/

static int
attr_converter(PyObject *arg, void *ptr)
{
    /* attr_t is unsigned and at least as wide as chtype, so an attribute
       value must be a non-negative integer that fits in attr_t. */
    unsigned long attr = PyLong_AsUnsignedLong(arg);
    if (attr == (unsigned long)-1 && PyErr_Occurred()) {
        return 0;
    }
    if (attr > (unsigned long)(attr_t)-1) {
        PyErr_Format(PyExc_OverflowError,
                     "attribute value is greater than maximum (%lu)",
                     (unsigned long)(attr_t)-1);
        return 0;
    }
    *(attr_t *)ptr = (attr_t)attr;
    return 1;
}

/*[python input]
class attr_converter(CConverter):
    type = 'attr_t'
    converter = 'attr_converter'
[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=6132d3d99d3ec25a]*/

#ifdef HAVE_NCURSESW
/* -------------------------------------------------------*/
/* Complex character objects (styled wide-character cells) */
/* -------------------------------------------------------*/

/* Wrap a cchar_t in a new complexchar object (the read side: in_wch,
   getbkgrnd, ...).  The object simply owns a copy of the cell. */
static PyObject *
PyCursesComplexChar_New(cursesmodule_state *state, const cchar_t *wcval)
{
    PyCursesComplexCharObject *cc =
        PyObject_New(PyCursesComplexCharObject, state->complexchar_type);
    if (cc == NULL) {
        return NULL;
    }
    cc->cval = *wcval;
    return (PyObject *)cc;
}

/* Decode the cell, raising curses.error on the (practically impossible)
   getcchar() failure. */
static int
complexchar_unpack(PyObject *self, wchar_t *wstr, attr_t *attrs, int *pair)
{
    cchar_t *cval = &_PyCursesComplexCharObject_CAST(self)->cval;
    if (curses_getcchar(cval, wstr, attrs, pair) == ERR) {
        cursesmodule_state *state =
            get_cursesmodule_state_by_cls(Py_TYPE(self));
        PyErr_SetString(state->error, "getcchar() returned ERR");
        return -1;
    }
    return 0;
}

/*[clinic input]
@classmethod
_curses.complexchar.__new__ as complexchar_new

    text: unicode
        A spacing character optionally followed by combining characters.
    /
    attr: attr = 0
        The attributes of the character cell.
    pair: int = 0
        The color pair number of the character cell.

A styled wide-character cell.

text is a spacing character optionally followed by combining
characters.  attr is a set of attributes (the WA_* constants) and pair
is a color pair number.  The object is immutable; str(cc) returns its
text, and the attr and pair attributes return its rendition.
[clinic start generated code]*/

static PyObject *
complexchar_new_impl(PyTypeObject *type, PyObject *text, attr_t attr,
                     int pair)
/*[clinic end generated code: output=5d8173048826b946 input=c3f3466a2656a196]*/
{
    if (pair < 0) {
        PyErr_SetString(PyExc_ValueError, "color pair is less than 0");
        return NULL;
    }
    wchar_t wstr[CCHARW_MAX + 1];
    if (PyCurses_ConvertToWideCell(text, wstr) < 0) {
        return NULL;
    }
    cchar_t cval;
    if (curses_setcchar(&cval, wstr, attr, pair) == ERR) {
        if (!PyErr_Occurred()) {
            cursesmodule_state *state = get_cursesmodule_state_by_cls(type);
            PyErr_SetString(state->error, "setcchar() returned ERR");
        }
        return NULL;
    }
    PyCursesComplexCharObject *cc =
        (PyCursesComplexCharObject *)type->tp_alloc(type, 0);
    if (cc == NULL) {
        return NULL;
    }
    cc->cval = cval;
    return (PyObject *)cc;
}

static void
complexchar_dealloc(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static PyObject *
complexchar_str(PyObject *self)
{
    wchar_t wstr[CCHARW_MAX + 1];
    attr_t attrs;
    int pair;
    if (complexchar_unpack(self, wstr, &attrs, &pair) < 0) {
        return NULL;
    }
    return PyUnicode_FromWideChar(wstr, -1);
}

static PyObject *
complexchar_repr(PyObject *self)
{
    wchar_t wstr[CCHARW_MAX + 1];
    attr_t attrs;
    int pair;
    if (complexchar_unpack(self, wstr, &attrs, &pair) < 0) {
        return NULL;
    }
    PyObject *text = PyUnicode_FromWideChar(wstr, -1);
    if (text == NULL) {
        return NULL;
    }
    /* Show attr and pair only when not at their defaults. */
    PyObject *res;
    if (attrs == 0 && pair == 0) {
        res = PyUnicode_FromFormat("%T(%R)", self, text);
    }
    else if (pair == 0) {
        res = PyUnicode_FromFormat("%T(%R, attr=%lu)", self, text,
                                   (unsigned long)attrs);
    }
    else if (attrs == 0) {
        res = PyUnicode_FromFormat("%T(%R, pair=%d)", self, text, pair);
    }
    else {
        res = PyUnicode_FromFormat("%T(%R, attr=%lu, pair=%d)", self, text,
                                   (unsigned long)attrs, pair);
    }
    Py_DECREF(text);
    return res;
}

static PyObject *
complexchar_get_attr(PyObject *self, void *Py_UNUSED(closure))
{
    wchar_t wstr[CCHARW_MAX + 1];
    attr_t attrs;
    int pair;
    if (complexchar_unpack(self, wstr, &attrs, &pair) < 0) {
        return NULL;
    }
    return PyLong_FromUnsignedLong((unsigned long)attrs);
}

static PyObject *
complexchar_get_pair(PyObject *self, void *Py_UNUSED(closure))
{
    wchar_t wstr[CCHARW_MAX + 1];
    attr_t attrs;
    int pair;
    if (complexchar_unpack(self, wstr, &attrs, &pair) < 0) {
        return NULL;
    }
    return PyLong_FromLong(pair);
}

static PyObject *
complexchar_richcompare(PyObject *self, PyObject *other, int op)
{
    if ((op != Py_EQ && op != Py_NE) ||
        !Py_IS_TYPE(other, Py_TYPE(self)))
    {
        Py_RETURN_NOTIMPLEMENTED;
    }
    wchar_t wstr1[CCHARW_MAX + 1], wstr2[CCHARW_MAX + 1];
    attr_t attrs1, attrs2;
    int pair1, pair2;
    if (complexchar_unpack(self, wstr1, &attrs1, &pair1) < 0 ||
        complexchar_unpack(other, wstr2, &attrs2, &pair2) < 0)
    {
        return NULL;
    }
    int equal = (attrs1 == attrs2 && pair1 == pair2 &&
                 wcscmp(wstr1, wstr2) == 0);
    return PyBool_FromLong(equal == (op == Py_EQ));
}

static Py_hash_t
complexchar_hash(PyObject *self)
{
    cursesmodule_state *state = get_cursesmodule_state_by_cls(Py_TYPE(self));
    return curses_cchar_hash(state, &_PyCursesComplexCharObject_CAST(self)->cval);
}

static PyGetSetDef complexchar_getsets[] = {
    {"attr", complexchar_get_attr, NULL,
     PyDoc_STR("the attributes of the character cell"), NULL},
    {"pair", complexchar_get_pair, NULL,
     PyDoc_STR("the color pair of the character cell"), NULL},
    {NULL}
};

/* -------------------------------------------------------------*/
/* Complex character strings (immutable arrays of styled cells)  */
/* -------------------------------------------------------------*/

/* Pack a single Python cell -- a complexchar (used as is) or a str (a spacing
   character optionally followed by combining characters, with no attributes
   and color pair 0) -- into *out.  Return 0 on success, -1 with an exception
   set otherwise. */
static int
curses_pack_cell(cursesmodule_state *state, PyObject *item, cchar_t *out)
{
    if (Py_IS_TYPE(item, state->complexchar_type)) {
        *out = _PyCursesComplexCharObject_CAST(item)->cval;
        return 0;
    }
    if (PyUnicode_Check(item)) {
        wchar_t wstr[CCHARW_MAX + 1];
        if (PyCurses_ConvertToWideCell(item, wstr) < 0) {
            return -1;
        }
        if (curses_setcchar(out, wstr, A_NORMAL, 0) == ERR) {
            PyErr_SetString(state->error, "setcchar() returned ERR");
            return -1;
        }
        return 0;
    }
    PyErr_Format(PyExc_TypeError,
                 "complexstr cell must be a complexchar or a str, not %T",
                 item);
    return -1;
}

/* Wrap a buffer of len cells in a new complexstr, copying them in.  tp_alloc
   sizes the variable-size object for len cells and sets ob_size. */
static PyObject *
PyCursesComplexStr_New(cursesmodule_state *state, const cchar_t *cells,
                       Py_ssize_t len)
{
    PyTypeObject *type = state->complexstr_type;
    PyObject *res = type->tp_alloc(type, len);
    if (res != NULL && len > 0) {
        memcpy(_PyCursesComplexStrObject_CAST(res)->cells, cells,
               (size_t)len * sizeof(cchar_t));
    }
    return res;
}

/* Build a complexstr from a string, grouping each base character with its
   trailing combining characters into one cell (so "é" is one cell, not two).
   A string needs this separate path because a generic sequence is packed one
   cell per item, which would not keep combining marks with their base. */
static PyObject *
complexstr_from_string(cursesmodule_state *state, PyObject *str,
                       attr_t attr, int pair)
{
    Py_ssize_t n;
    wchar_t *wbuf = PyUnicode_AsWideCharString(str, &n);
    if (wbuf == NULL) {
        return NULL;
    }
    cchar_t *cells = n > 0 ? PyMem_New(cchar_t, n) : NULL;
    if (n > 0 && cells == NULL) {
        PyMem_Free(wbuf);
        return PyErr_NoMemory();
    }
    Py_ssize_t count = 0;
    for (Py_ssize_t i = 0; i < n; ) {
        wchar_t cell[CCHARW_MAX + 1];
        Py_ssize_t k = 0;
        cell[k++] = wbuf[i++];
        while (i < n && k < CCHARW_MAX && wcwidth(wbuf[i]) == 0) {
            cell[k++] = wbuf[i++];
        }
        cell[k] = L'\0';
        /* A cell's base must be a spacing character.  A combining character
           (wcwidth 0) has no base to attach to, so it cannot start a cell.  A
           control character (wcwidth < 0) may stand alone but cannot carry
           combining marks. */
        int width = wcwidth(cell[0]);
        if (width == 0 || (k > 1 && width < 0)) {
            PyErr_Format(PyExc_ValueError,
                         "a character cell must be a single spacing character "
                         "optionally followed by up to %d combining characters",
                         (int)(CCHARW_MAX - 1));
            PyMem_Free(cells);
            PyMem_Free(wbuf);
            return NULL;
        }
        if (curses_setcchar(&cells[count], cell, attr, pair) == ERR) {
            if (!PyErr_Occurred()) {
                PyErr_SetString(state->error, "setcchar() returned ERR");
            }
            PyMem_Free(cells);
            PyMem_Free(wbuf);
            return NULL;
        }
        count++;
    }
    PyObject *res = PyCursesComplexStr_New(state, cells, count);
    PyMem_Free(cells);
    PyMem_Free(wbuf);
    return res;
}

/*[clinic input]
@classmethod
_curses.complexstr.__new__ as complexstr_new

    cells: object
        An iterable of cells, each a complexchar or a str.
    /
    attr: object = NULL
        Attributes applied to every cell (only with a string).
    pair: object = NULL
        Color pair applied to every cell (only with a string).

An immutable string of styled wide-character cells.

It is the counterpart of complexchar for a run of cells, and the type
returned by window.in_wchstr().  Each cell is a complexchar or a str (a
spacing character optionally followed by combining characters).

When cells is a string it is split into character cells, and attr and
pair (if given) style every cell.  Otherwise each item carries its own
rendition, and attr and pair must be omitted.
[clinic start generated code]*/

static PyObject *
complexstr_new_impl(PyTypeObject *type, PyObject *cells, PyObject *attr,
                    PyObject *pair)
/*[clinic end generated code: output=ef8a53143d35a32a input=9b75aee973cc6565]*/
{
    cursesmodule_state *state = get_cursesmodule_state_by_cls(type);
    /* A string is split into cells (grouping combining characters), not
       iterated as one cell per code point; attr/pair then style every cell. */
    if (PyUnicode_Check(cells)) {
        attr_t cattr = A_NORMAL;
        int cpair = 0;
        if (attr != NULL && !attr_converter(attr, &cattr)) {
            return NULL;
        }
        if (pair != NULL) {
            cpair = PyLong_AsInt(pair);
            if (cpair == -1 && PyErr_Occurred()) {
                return NULL;
            }
            if (cpair < 0) {
                PyErr_SetString(PyExc_ValueError, "color pair is less than 0");
                return NULL;
            }
        }
        return complexstr_from_string(state, cells, cattr, cpair);
    }
    /* For any other sequence each item carries its own rendition, so attr and
       pair cannot be given. */
    if (attr != NULL || pair != NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "attr and pair can only be given with a string");
        return NULL;
    }
    PyObject *seq = PySequence_Fast(cells,
                                    "complexstr() argument must be an iterable");
    if (seq == NULL) {
        return NULL;
    }
    Py_ssize_t n = PySequence_Fast_GET_SIZE(seq);
    PyObject *res = type->tp_alloc(type, n);
    if (res == NULL) {
        Py_DECREF(seq);
        return NULL;
    }
    cchar_t *out = _PyCursesComplexStrObject_CAST(res)->cells;
    for (Py_ssize_t i = 0; i < n; i++) {
        PyObject *item = PySequence_Fast_GET_ITEM(seq, i);  // borrowed
        if (curses_pack_cell(state, item, &out[i]) < 0) {
            Py_DECREF(res);
            Py_DECREF(seq);
            return NULL;
        }
    }
    Py_DECREF(seq);
    return res;
}

static void
complexstr_dealloc(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static Py_ssize_t
complexstr_length(PyObject *self)
{
    return Py_SIZE(self);
}

/* Wrap cell i (no bounds check) in a new complexchar. */
static PyObject *
complexstr_getcell(PyObject *self, Py_ssize_t i)
{
    cursesmodule_state *state = get_cursesmodule_state_by_cls(Py_TYPE(self));
    cchar_t *cells = _PyCursesComplexStrObject_CAST(self)->cells;
    return PyCursesComplexChar_New(state, &cells[i]);
}

static PyObject *
complexstr_item(PyObject *self, Py_ssize_t i)
{
    if (i < 0 || i >= Py_SIZE(self)) {
        PyErr_SetString(PyExc_IndexError, "complexstr index out of range");
        return NULL;
    }
    return complexstr_getcell(self, i);
}

static PyObject *
complexstr_subscript(PyObject *self, PyObject *key)
{
    PyCursesComplexStrObject *s = _PyCursesComplexStrObject_CAST(self);
    if (PyIndex_Check(key)) {
        Py_ssize_t i = PyNumber_AsSsize_t(key, PyExc_IndexError);
        if (i == -1 && PyErr_Occurred()) {
            return NULL;
        }
        if (i < 0) {
            i += Py_SIZE(s);
        }
        return complexstr_item(self, i);
    }
    if (PySlice_Check(key)) {
        Py_ssize_t start, stop, step, slicelen;
        if (PySlice_GetIndicesEx(key, Py_SIZE(s), &start, &stop, &step,
                                 &slicelen) < 0)
        {
            return NULL;
        }
        cursesmodule_state *state =
            get_cursesmodule_state_by_cls(Py_TYPE(self));
        PyTypeObject *type = state->complexstr_type;
        PyObject *res = type->tp_alloc(type, slicelen);
        if (res == NULL) {
            return NULL;
        }
        cchar_t *out = _PyCursesComplexStrObject_CAST(res)->cells;
        for (Py_ssize_t i = 0, idx = start; i < slicelen; i++, idx += step) {
            out[i] = s->cells[idx];
        }
        return res;
    }
    PyErr_Format(PyExc_TypeError,
                 "complexstr indices must be integers or slices, not %T",
                 key);
    return NULL;
}

static PyObject *
complexstr_concat(PyObject *a, PyObject *b)
{
    cursesmodule_state *state = get_cursesmodule_state_by_cls(Py_TYPE(a));
    if (!Py_IS_TYPE(b, state->complexstr_type)) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    PyCursesComplexStrObject *sa = _PyCursesComplexStrObject_CAST(a);
    PyCursesComplexStrObject *sb = _PyCursesComplexStrObject_CAST(b);
    PyTypeObject *type = state->complexstr_type;
    PyObject *res = type->tp_alloc(type, Py_SIZE(sa) + Py_SIZE(sb));
    if (res == NULL) {
        return NULL;
    }
    cchar_t *out = _PyCursesComplexStrObject_CAST(res)->cells;
    if (Py_SIZE(sa)) {
        memcpy(out, sa->cells, (size_t)Py_SIZE(sa) * sizeof(cchar_t));
    }
    if (Py_SIZE(sb)) {
        memcpy(out + Py_SIZE(sa), sb->cells, (size_t)Py_SIZE(sb) * sizeof(cchar_t));
    }
    return res;
}

static PyObject *
complexstr_str(PyObject *self)
{
    PyCursesComplexStrObject *s = _PyCursesComplexStrObject_CAST(self);
    if (Py_SIZE(s) == 0) {
        return Py_GetConstant(Py_CONSTANT_EMPTY_STR);
    }
    wchar_t *buf = PyMem_New(wchar_t, Py_SIZE(s) * CCHARW_MAX + 1);
    if (buf == NULL) {
        return PyErr_NoMemory();
    }
    Py_ssize_t pos = 0;
    for (Py_ssize_t i = 0; i < Py_SIZE(s); i++) {
        attr_t attrs;
        int pair;
        /* getcchar() writes the cell's text (and a terminator) at buf + pos;
           the next cell overwrites the terminator. */
        if (curses_getcchar(&s->cells[i], buf + pos, &attrs, &pair) == ERR) {
            cursesmodule_state *state =
                get_cursesmodule_state_by_cls(Py_TYPE(self));
            PyErr_SetString(state->error, "getcchar() returned ERR");
            PyMem_Free(buf);
            return NULL;
        }
        pos += wcslen(buf + pos);
    }
    PyObject *res = PyUnicode_FromWideChar(buf, pos);
    PyMem_Free(buf);
    return res;
}

static PyObject *
complexstr_repr(PyObject *self)
{
    PyObject *list = PySequence_List(self);
    if (list == NULL) {
        return NULL;
    }
    PyObject *res = PyUnicode_FromFormat("%T(%R)", self, list);
    Py_DECREF(list);
    return res;
}

static Py_hash_t
complexstr_hash(PyObject *self)
{
    PyCursesComplexStrObject *s = _PyCursesComplexStrObject_CAST(self);
    cursesmodule_state *state = get_cursesmodule_state_by_cls(Py_TYPE(self));
    /* Combine the per-cell hashes like a tuple. */
    Py_uhash_t acc = _PyTuple_HASH_XXPRIME_5;
    for (Py_ssize_t i = 0; i < Py_SIZE(s); i++) {
        Py_hash_t lane = curses_cchar_hash(state, &s->cells[i]);
        if (lane == -1) {
            return -1;
        }
        acc += (Py_uhash_t)lane * _PyTuple_HASH_XXPRIME_2;
        acc = _PyTuple_HASH_XXROTATE(acc);
        acc *= _PyTuple_HASH_XXPRIME_1;
    }
    acc += Py_SIZE(s) ^ (_PyTuple_HASH_XXPRIME_5 ^ 3527539);
    if (acc == (Py_uhash_t)-1) {
        acc = 1546275796;
    }
    return (Py_hash_t)acc;
}

static PyObject *
complexstr_richcompare(PyObject *self, PyObject *other, int op)
{
    if ((op != Py_EQ && op != Py_NE) || !Py_IS_TYPE(other, Py_TYPE(self))) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    PyCursesComplexStrObject *a = _PyCursesComplexStrObject_CAST(self);
    PyCursesComplexStrObject *b = _PyCursesComplexStrObject_CAST(other);
    int equal = (Py_SIZE(a) == Py_SIZE(b));
    for (Py_ssize_t i = 0; equal && i < Py_SIZE(a); i++) {
        wchar_t wa[CCHARW_MAX + 1], wb[CCHARW_MAX + 1];
        attr_t aa, ab;
        int pa, pb;
        if (curses_getcchar(&a->cells[i], wa, &aa, &pa) == ERR ||
            curses_getcchar(&b->cells[i], wb, &ab, &pb) == ERR)
        {
            cursesmodule_state *state =
                get_cursesmodule_state_by_cls(Py_TYPE(self));
            PyErr_SetString(state->error, "getcchar() returned ERR");
            return NULL;
        }
        equal = (aa == ab && pa == pb && wcscmp(wa, wb) == 0);
    }
    return PyBool_FromLong(equal == (op == Py_EQ));
}

/* Write (insert=0) or insert (insert=1) a complexstr's cells, using its buffer
   directly, at the current or given position.  n_limit < 0 means the whole run.
   Returns None or NULL with an exception. */
static PyObject *
curses_window_put_cells(PyCursesWindowObject *self, PyObject *obj,
                        int use_xy, int y, int x, int n_limit, int insert,
                        const char *funcname)
{
    const cchar_t *cells = _PyCursesComplexStrObject_CAST(obj)->cells;
    Py_ssize_t count = Py_SIZE(obj);

    if (n_limit >= 0 && count > n_limit) {
        count = n_limit;
    }

    if (count == 0) {
        /* Nothing to write; just honour the optional move, like an empty
           string would. */
        int rtn = use_xy ? wmove(self->win, y, x) : OK;
        return curses_window_check_err(self, rtn, "wmove", funcname);
    }

    int rtn;
    const char *cfuncname;
    if (insert) {
        /* There is no batch cchar_t insert; insert the cells right-to-left at
           the position so they end up in order. */
        if (use_xy && wmove(self->win, y, x) == ERR) {
            curses_window_set_error(self, "wmove", funcname);
            return NULL;
        }
        rtn = OK;
        cfuncname = "wins_wch";
        for (Py_ssize_t i = count - 1; i >= 0; i--) {
            rtn = wins_wch(self->win, &cells[i]);
            if (rtn == ERR) {
                break;
            }
        }
    }
    else if (use_xy) {
        rtn = mvwadd_wchnstr(self->win, y, x, cells, (int)count);
        cfuncname = "mvwadd_wchnstr";
    }
    else {
        rtn = wadd_wchnstr(self->win, cells, (int)count);
        cfuncname = "wadd_wchnstr";
    }
    return curses_window_check_err(self, rtn, cfuncname, funcname);
}

#endif

/*****************************************************************************
 The Window Object
******************************************************************************/

/*
 * Macros for creating a PyCursesWindowObject object's method.
 *
 * Parameters
 *
 *  X           The name of the curses C function or macro to invoke.
 *  TYPE        The function parameter(s) type.
 *  ERGSTR      The format string for construction of the return value.
 *  PARSESTR    The format string for argument parsing.
 */

#define Window_NoArgNoReturnFunction(X)                                 \
    static PyObject *PyCursesWindow_ ## X                               \
    (PyObject *op, PyObject *Py_UNUSED(ignored))                        \
    {                                                                   \
        PyCursesWindowObject *self = _PyCursesWindowObject_CAST(op);    \
        int code = X(self->win);                                        \
        return curses_window_check_err(self, code, # X, NULL);          \
    }

#define Window_NoArgTrueFalseFunction(X)                                \
    static PyObject * PyCursesWindow_ ## X                              \
    (PyObject *op, PyObject *Py_UNUSED(ignored))                        \
    {                                                                   \
        PyCursesWindowObject *self = _PyCursesWindowObject_CAST(op);    \
        return PyBool_FromLong(X(self->win));                           \
    }

#define Window_NoArgNoReturnVoidFunction(X)                             \
    static PyObject * PyCursesWindow_ ## X                              \
    (PyObject *op, PyObject *Py_UNUSED(ignored))                        \
    {                                                                   \
        PyCursesWindowObject *self = _PyCursesWindowObject_CAST(op);    \
        X(self->win);                                                   \
        Py_RETURN_NONE;                                                 \
    }

#define Window_NoArg2TupleReturnFunction(X, TYPE, ERGSTR)               \
    static PyObject * PyCursesWindow_ ## X                              \
    (PyObject *op, PyObject *Py_UNUSED(ignored))                        \
    {                                                                   \
        TYPE arg1, arg2;                                                \
        PyCursesWindowObject *self = _PyCursesWindowObject_CAST(op);    \
        X(self->win, arg1, arg2);                                       \
        return Py_BuildValue(ERGSTR, arg1, arg2);                       \
    }

#define Window_OneArgNoReturnVoidFunction(X, TYPE, PARSESTR)            \
    static PyObject * PyCursesWindow_ ## X                              \
    (PyObject *op, PyObject *args)                                      \
    {                                                                   \
        TYPE arg1;                                                      \
        if (!PyArg_ParseTuple(args, PARSESTR, &arg1)) {                 \
            return NULL;                                                \
        }                                                               \
        PyCursesWindowObject *self = _PyCursesWindowObject_CAST(op);    \
        X(self->win, arg1);                                             \
        Py_RETURN_NONE;                                                 \
    }

#define Window_OneArgNoReturnFunction(X, TYPE, PARSESTR)                \
    static PyObject * PyCursesWindow_ ## X                              \
    (PyObject *op, PyObject *args)                                      \
    {                                                                   \
        TYPE arg1;                                                      \
        if (!PyArg_ParseTuple(args, PARSESTR, &arg1)) {                 \
            return NULL;                                                \
        }                                                               \
        PyCursesWindowObject *self = _PyCursesWindowObject_CAST(op);    \
        int code = X(self->win, arg1);                                  \
        return curses_window_check_err(self, code, # X, NULL);          \
    }

#define Window_TwoArgNoReturnFunction(X, TYPE, PARSESTR)                \
    static PyObject * PyCursesWindow_ ## X                              \
    (PyObject *op, PyObject *args)                                      \
    {                                                                   \
        TYPE arg1, arg2;                                                \
        if (!PyArg_ParseTuple(args,PARSESTR, &arg1, &arg2)) {           \
            return NULL;                                                \
        }                                                               \
        PyCursesWindowObject *self = _PyCursesWindowObject_CAST(op);    \
        int code = X(self->win, arg1, arg2);                            \
        return curses_window_check_err(self, code, # X, NULL);          \
    }

/* ------------- WINDOW routines --------------- */

Window_NoArgNoReturnFunction(untouchwin)
Window_NoArgNoReturnFunction(touchwin)
Window_NoArgNoReturnFunction(redrawwin)
Window_NoArgNoReturnFunction(winsertln)
Window_NoArgNoReturnFunction(werase)
Window_NoArgNoReturnFunction(wdeleteln)

Window_NoArgTrueFalseFunction(is_wintouched)

#if defined(NCURSES_EXT_FUNCS) && NCURSES_EXT_FUNCS >= 20110404
Window_NoArgTrueFalseFunction(is_cleared)
Window_NoArgTrueFalseFunction(is_idcok)
Window_NoArgTrueFalseFunction(is_idlok)
Window_NoArgTrueFalseFunction(is_immedok)
Window_NoArgTrueFalseFunction(is_keypad)
Window_NoArgTrueFalseFunction(is_leaveok)
Window_NoArgTrueFalseFunction(is_nodelay)
Window_NoArgTrueFalseFunction(is_notimeout)
Window_NoArgTrueFalseFunction(is_pad)
Window_NoArgTrueFalseFunction(is_scrollok)
Window_NoArgTrueFalseFunction(is_subwin)
Window_NoArgTrueFalseFunction(is_syncok)

static PyObject *
PyCursesWindow_getdelay(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    PyCursesWindowObject *self = _PyCursesWindowObject_CAST(op);
    return PyLong_FromLong(wgetdelay(self->win));
}

static PyObject *
PyCursesWindow_getscrreg(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    PyCursesWindowObject *self = _PyCursesWindowObject_CAST(op);
    int top, bottom;
    if (wgetscrreg(self->win, &top, &bottom) == ERR) {
        curses_window_set_error(self, "wgetscrreg", "getscrreg");
        return NULL;
    }
    return Py_BuildValue("(ii)", top, bottom);
}

static PyObject *
PyCursesWindow_getparent(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    PyCursesWindowObject *self = _PyCursesWindowObject_CAST(op);
    /* The standard window has no parent; subwindows keep a reference to the
       window they were derived from. */
    if (self->orig == NULL) {
        Py_RETURN_NONE;
    }
    return Py_NewRef((PyObject *)self->orig);
}
#endif /* NCURSES_EXT_FUNCS */

Window_NoArgNoReturnVoidFunction(wsyncup)
Window_NoArgNoReturnVoidFunction(wsyncdown)
Window_NoArgNoReturnVoidFunction(wstandend)
Window_NoArgNoReturnVoidFunction(wstandout)
Window_NoArgNoReturnVoidFunction(wcursyncup)
Window_NoArgNoReturnVoidFunction(wclrtoeol)
Window_NoArgNoReturnVoidFunction(wclrtobot)
Window_NoArgNoReturnVoidFunction(wclear)

Window_OneArgNoReturnVoidFunction(idcok, int, "i;True(1) or False(0)")
#ifdef HAVE_CURSES_IMMEDOK
Window_OneArgNoReturnVoidFunction(immedok, int, "i;True(1) or False(0)")
#endif
Window_OneArgNoReturnVoidFunction(wtimeout, int, "i;delay")

Window_NoArg2TupleReturnFunction(getyx, int, "ii")
Window_NoArg2TupleReturnFunction(getbegyx, int, "ii")
Window_NoArg2TupleReturnFunction(getmaxyx, int, "ii")
Window_NoArg2TupleReturnFunction(getparyx, int, "ii")

Window_OneArgNoReturnFunction(clearok, int, "i;True(1) or False(0)")
Window_OneArgNoReturnFunction(idlok, int, "i;True(1) or False(0)")
Window_OneArgNoReturnFunction(keypad, int, "i;True(1) or False(0)")
Window_OneArgNoReturnFunction(leaveok, int, "i;True(1) or False(0)")
Window_OneArgNoReturnFunction(nodelay, int, "i;True(1) or False(0)")
Window_OneArgNoReturnFunction(notimeout, int, "i;True(1) or False(0)")
Window_OneArgNoReturnFunction(scrollok, int, "i;True(1) or False(0)")
Window_OneArgNoReturnFunction(winsdelln, int, "i;nlines")
#ifdef HAVE_CURSES_SYNCOK
Window_OneArgNoReturnFunction(syncok, int, "i;True(1) or False(0)")
#endif

Window_TwoArgNoReturnFunction(mvwin, int, "ii;y,x")
Window_TwoArgNoReturnFunction(mvderwin, int, "ii;y,x")
Window_TwoArgNoReturnFunction(wmove, int, "ii;y,x")
#ifndef STRICT_SYSV_CURSES
Window_TwoArgNoReturnFunction(wresize, int, "ii;lines,columns")
#endif

/* Allocation and deallocation of Window Objects */

static PyObject *
PyCursesWindow_New(cursesmodule_state *state,
                   WINDOW *win, const char *encoding,
                   PyCursesWindowObject *orig, PyObject *screen)
{
    if (encoding == NULL) {
#if defined(MS_WINDOWS)
        char *buffer[100];
        UINT cp;
        cp = GetConsoleOutputCP();
        if (cp != 0) {
            PyOS_snprintf(buffer, sizeof(buffer), "cp%u", cp);
            encoding = buffer;
        }
#elif defined(CODESET)
        const char *codeset = nl_langinfo(CODESET);
        if (codeset != NULL && codeset[0] != 0) {
            encoding = codeset;
        }
#endif
        if (encoding == NULL) {
            encoding = "utf-8";
        }
    }

    PyCursesWindowObject *wo = PyObject_GC_New(PyCursesWindowObject,
                                               state->window_type);
    if (wo == NULL) {
        return NULL;
    }
    wo->win = win;
    wo->orig = (PyCursesWindowObject *)Py_XNewRef((PyObject *)orig);
    wo->screen = Py_XNewRef(screen);
    wo->encoding = _PyMem_Strdup(encoding);
    if (wo->encoding == NULL) {
        Py_DECREF(wo);
        PyErr_NoMemory();
        return NULL;
    }
    PyObject_GC_Track((PyObject *)wo);
    return (PyObject *)wo;
}

static void
PyCursesWindow_dealloc(PyObject *self)
{
    PyTypeObject *window_type = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    PyCursesWindowObject *wo = (PyCursesWindowObject *)self;
    if (wo->win != stdscr && wo->win != NULL) {
        if (delwin(wo->win) == ERR) {
            curses_window_set_error(wo, "delwin", "__del__");
            PyErr_FormatUnraisable("Exception ignored in delwin()");
        }
    }
    if (wo->encoding != NULL) {
        PyMem_Free(wo->encoding);
    }
    Py_XDECREF(wo->orig);
    Py_XDECREF(wo->screen);
    window_type->tp_free(self);
    Py_DECREF(window_type);
}

static int
PyCursesWindow_traverse(PyObject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    PyCursesWindowObject *wo = (PyCursesWindowObject *)self;
    Py_VISIT(wo->orig);
    Py_VISIT(wo->screen);
    return 0;
}

/* Addch, Addstr, Addnstr */

/*[clinic input]
_curses.window.addch

    [
    y: int
        Y-coordinate.
    x: int
        X-coordinate.
    ]

    ch: object
        Character to add.

    [
    attr: long
        Attributes for the character.
    ]
    /

Paint the character.

Paint character ch at (y, x) with attributes attr,
overwriting any character previously painted at that location.
By default, the character position and attributes are the
current settings for the window object.
[clinic start generated code]*/

static PyObject *
_curses_window_addch_impl(PyCursesWindowObject *self, int group_left_1,
                          int y, int x, PyObject *ch, int group_right_1,
                          long attr)
/*[clinic end generated code: output=00f4c37af3378f45 input=ab196a1dac3d354c]*/
{
    int coordinates_group = group_left_1;
    int rtn;
    int type;
    chtype cch = 0;
#ifdef HAVE_NCURSESW
    cchar_t wcval;
#endif
    const char *funcname;

#ifdef HAVE_NCURSESW
    type = PyCurses_ConvertToCell(self, ch, attr, group_right_1, "addch",
                                  &cch, &wcval);
    if (type == 2) {
        if (coordinates_group) {
            rtn = mvwadd_wch(self->win,y,x, &wcval);
            funcname = "mvwadd_wch";
        }
        else {
            rtn = wadd_wch(self->win, &wcval);
            funcname = "wadd_wch";
        }
    }
    else
#else
    type = PyCurses_ConvertToCchar_t(self, ch, &cch);
#endif
    if (type == 1) {
        if (coordinates_group) {
            rtn = mvwaddch(self->win,y,x, cch | (attr_t) attr);
            funcname = "mvwaddch";
        }
        else {
            rtn = waddch(self->win, cch | (attr_t) attr);
            funcname = "waddch";
        }
    }
    else {
        return NULL;
    }
    return curses_window_check_err(self, rtn, funcname, "addch");
}

#ifdef HAVE_NCURSESW
#define curses_release_wstr(STRTYPE, WSTR)  \
    do {                                    \
        if ((STRTYPE) == 2) {               \
            PyMem_Free((WSTR));             \
        }                                   \
    } while (0)
#else
#define curses_release_wstr(_STRTYPE, _WSTR)
#endif

static int
curses_wattrset(PyCursesWindowObject *self, long attr, const char *funcname)
{
    if (wattrset(self->win, attr) == ERR) {
        curses_window_set_error(self, "wattrset", funcname);
        return -1;
    }
    return 0;
}

/*[clinic input]
_curses.window.addstr

    [
    y: int
        Y-coordinate.
    x: int
        X-coordinate.
    ]

    str: object
        String to add.

    [
    attr: long
        Attributes for characters.
    ]
    /

Paint the string.

Paint the string str at (y, x) with attributes attr,
overwriting anything previously on the display.
By default, the character position and attributes are the
current settings for the window object.
[clinic start generated code]*/

static PyObject *
_curses_window_addstr_impl(PyCursesWindowObject *self, int group_left_1,
                           int y, int x, PyObject *str, int group_right_1,
                           long attr)
/*[clinic end generated code: output=65a928ea85ff3115 input=ff6cbb91448a22a3]*/
{
    int rtn;
    int strtype;
    PyObject *bytesobj = NULL;
#ifdef HAVE_NCURSESW
    wchar_t *wstr = NULL;
#endif
    attr_t attr_old = A_NORMAL;
    int use_xy = group_left_1, use_attr = group_right_1;
    const char *funcname;

#ifdef HAVE_NCURSESW
    {
        cursesmodule_state *state = get_cursesmodule_state_by_win(self);
        if (Py_IS_TYPE(str, state->complexstr_type)) {
            if (use_attr) {
                PyErr_SetString(PyExc_TypeError, "addstr(): attr cannot be "
                                "specified together with a complexstr");
                return NULL;
            }
            return curses_window_put_cells(self, str, use_xy, y, x,
                                           -1, 0, "addstr");
        }
    }
    strtype = PyCurses_ConvertToString(self, str, &bytesobj, &wstr);
#else
    strtype = PyCurses_ConvertToString(self, str, &bytesobj, NULL);
#endif
    if (strtype == 0) {
        return NULL;
    }
    if (use_attr) {
        attr_old = getattrs(self->win);
        if (curses_wattrset(self, attr, "addstr") < 0) {
            curses_release_wstr(strtype, wstr);
            Py_XDECREF(bytesobj);
            return NULL;
        }
    }
#ifdef HAVE_NCURSESW
    if (strtype == 2) {
        assert(bytesobj == NULL);
        if (use_xy) {
            rtn = mvwaddwstr(self->win,y,x,wstr);
            funcname = "mvwaddwstr";
        }
        else {
            rtn = waddwstr(self->win,wstr);
            funcname = "waddwstr";
        }
        PyMem_Free(wstr);
    }
    else
#endif
    {
#ifdef HAVE_NCURSESW
        assert(wstr == NULL);
#endif
        const char *str = PyBytes_AS_STRING(bytesobj);
        if (use_xy) {
            rtn = mvwaddstr(self->win,y,x,str);
            funcname = "mvwaddstr";
        }
        else {
            rtn = waddstr(self->win,str);
            funcname = "waddstr";
        }
        Py_DECREF(bytesobj);
    }
    if (rtn == ERR) {
        curses_window_set_error(self, funcname, "addstr");
        return NULL;
    }
    if (use_attr) {
        rtn = wattrset(self->win, attr_old);
        return curses_window_check_err(self, rtn, "wattrset", "addstr");
    }
    Py_RETURN_NONE;
}

/*[clinic input]
_curses.window.addnstr

    [
    y: int
        Y-coordinate.
    x: int
        X-coordinate.
    ]

    str: object
        String to add.

    n: int
        Maximal number of characters.

    [
    attr: long
        Attributes for characters.
    ]
    /

Paint at most n characters of the string.

Paint at most n characters of the string str at (y, x) with
attributes attr, overwriting anything previously on the display.
By default, the character position and attributes are the
current settings for the window object.
[clinic start generated code]*/

static PyObject *
_curses_window_addnstr_impl(PyCursesWindowObject *self, int group_left_1,
                            int y, int x, PyObject *str, int n,
                            int group_right_1, long attr)
/*[clinic end generated code: output=6d21cee2ce6876d9 input=72718415c2744a2a]*/
{
    int rtn;
    int strtype;
    PyObject *bytesobj = NULL;
#ifdef HAVE_NCURSESW
    wchar_t *wstr = NULL;
#endif
    attr_t attr_old = A_NORMAL;
    int use_xy = group_left_1, use_attr = group_right_1;
    const char *funcname;

#ifdef HAVE_NCURSESW
    {
        cursesmodule_state *state = get_cursesmodule_state_by_win(self);
        if (Py_IS_TYPE(str, state->complexstr_type)) {
            if (use_attr) {
                PyErr_SetString(PyExc_TypeError, "addnstr(): attr cannot be "
                                "specified together with a complexstr");
                return NULL;
            }
            return curses_window_put_cells(self, str, use_xy, y, x,
                                           n, 0, "addnstr");
        }
    }
    strtype = PyCurses_ConvertToString(self, str, &bytesobj, &wstr);
#else
    strtype = PyCurses_ConvertToString(self, str, &bytesobj, NULL);
#endif
    if (strtype == 0)
        return NULL;

    if (use_attr) {
        attr_old = getattrs(self->win);
        if (curses_wattrset(self, attr, "addnstr") < 0) {
            curses_release_wstr(strtype, wstr);
            Py_XDECREF(bytesobj);
            return NULL;
        }
    }
#ifdef HAVE_NCURSESW
    if (strtype == 2) {
        if (use_xy) {
            rtn = mvwaddnwstr(self->win,y,x,wstr,n);
            funcname = "mvwaddnwstr";
        }
        else {
            rtn = waddnwstr(self->win,wstr,n);
            funcname = "waddnwstr";
        }
        PyMem_Free(wstr);
    }
    else
#endif
    {
        const char *str = PyBytes_AS_STRING(bytesobj);
        if (use_xy) {
            rtn = mvwaddnstr(self->win,y,x,str,n);
            funcname = "mvwaddnstr";
        }
        else {
            rtn = waddnstr(self->win,str,n);
            funcname = "waddnstr";
        }
        Py_DECREF(bytesobj);
    }
    if (rtn == ERR) {
        curses_window_set_error(self, funcname, "addnstr");
        return NULL;
    }
    if (use_attr) {
        rtn = wattrset(self->win, attr_old);
        return curses_window_check_err(self, rtn, "wattrset", "addnstr");
    }
    Py_RETURN_NONE;
}

/*[clinic input]
_curses.window.bkgd

    ch: object
        Background character.
    [
    attr: long
        Background attributes.
    ]
    /

Set the background property of the window.
[clinic start generated code]*/

static PyObject *
_curses_window_bkgd_impl(PyCursesWindowObject *self, PyObject *ch,
                         int group_right_1, long attr)
/*[clinic end generated code: output=73cb11ecca59612f input=a2129c1b709db432]*/
{
    chtype bkgd;
#ifdef HAVE_NCURSESW
    cchar_t wch;
    int type = PyCurses_ConvertToCell(self, ch, attr, group_right_1,
                                      "bkgd", &bkgd, &wch);
    if (type == 0) {
        return NULL;
    }
    if (type == 2) {
        int rtn = wbkgrnd(self->win, &wch);
        return curses_window_check_err(self, rtn, "wbkgrnd", "bkgd");
    }
#else
    if (!PyCurses_ConvertToChtype(self, ch, &bkgd))
        return NULL;
#endif

    int rtn = wbkgd(self->win, bkgd | (attr_t)attr);
    return curses_window_check_err(self, rtn, "wbkgd", "bkgd");
}

/*[clinic input]
_curses.window.attroff

    attr: long
    /

Remove attribute attr from the "background" set.
[clinic start generated code]*/

static PyObject *
_curses_window_attroff_impl(PyCursesWindowObject *self, long attr)
/*[clinic end generated code: output=8a2fcd4df682fc64 input=786beedf06a7befe]*/
{
    int rtn = wattroff(self->win, (attr_t)attr);
    return curses_window_check_err(self, rtn, "wattroff", "attroff");
}

/*[clinic input]
_curses.window.attron

    attr: long
    /

Add attribute attr to the "background" set.
[clinic start generated code]*/

static PyObject *
_curses_window_attron_impl(PyCursesWindowObject *self, long attr)
/*[clinic end generated code: output=7afea43b237fa870 input=b57f824e1bf58326]*/
{
    int rtn = wattron(self->win, (attr_t)attr);
    return curses_window_check_err(self, rtn, "wattron", "attron");
}

/*[clinic input]
_curses.window.attrset

    attr: long
    /

Set the "background" set of attributes.
[clinic start generated code]*/

static PyObject *
_curses_window_attrset_impl(PyCursesWindowObject *self, long attr)
/*[clinic end generated code: output=84e379bff20c0433 input=42e400c0d0154ab5]*/
{
    int rtn = wattrset(self->win, (attr_t)attr);
    return curses_window_check_err(self, rtn, "wattrset", "attrset");
}

/*[clinic input]
_curses.window.attr_get

Return the window's attributes and color pair as (attrs, pair).
[clinic start generated code]*/

static PyObject *
_curses_window_attr_get_impl(PyCursesWindowObject *self)
/*[clinic end generated code: output=74b3f805a2958fb8 input=1efd3c450a1373ef]*/
{
    attr_t attrs;
    int rtn;
#if _NCURSES_EXTENDED_COLOR_FUNCS
    int pair;
    short legacy_pair;
    rtn = wattr_get(self->win, &attrs, &legacy_pair, &pair);
#else
    short pair;
    rtn = wattr_get(self->win, &attrs, &pair, NULL);
#endif
    if (curses_window_check_err(self, rtn, "wattr_get", "attr_get") == NULL) {
        return NULL;
    }
    return Py_BuildValue("(ki)", (unsigned long)attrs, (int)pair);
}

/*[clinic input]
_curses.window.attr_set

    attr: attr
    pair: pair = 0
    /

Set the window's attributes and color pair.
[clinic start generated code]*/

static PyObject *
_curses_window_attr_set_impl(PyCursesWindowObject *self, attr_t attr,
                             int pair)
/*[clinic end generated code: output=756416e0d6345d4a input=b7936bd6b73eb3f2]*/
{
    int rtn;
#if _NCURSES_EXTENDED_COLOR_FUNCS
    rtn = wattr_set(self->win, attr, 0, &pair);
#else
    rtn = wattr_set(self->win, attr, (short)pair, NULL);
#endif
    return curses_window_check_err(self, rtn, "wattr_set", "attr_set");
}

/*[clinic input]
_curses.window.attr_on

    attr: attr
    /

Turn on the given attributes without affecting any others.
[clinic start generated code]*/

static PyObject *
_curses_window_attr_on_impl(PyCursesWindowObject *self, attr_t attr)
/*[clinic end generated code: output=712f13a558c5a6cb input=6a51a3d597ddca4b]*/
{
    int rtn = wattr_on(self->win, attr, NULL);
    return curses_window_check_err(self, rtn, "wattr_on", "attr_on");
}

/*[clinic input]
_curses.window.attr_off

    attr: attr
    /

Turn off the given attributes without affecting any others.
[clinic start generated code]*/

static PyObject *
_curses_window_attr_off_impl(PyCursesWindowObject *self, attr_t attr)
/*[clinic end generated code: output=ac680aead16f74fa input=c5d778b84030d388]*/
{
    int rtn = wattr_off(self->win, attr, NULL);
    return curses_window_check_err(self, rtn, "wattr_off", "attr_off");
}

/*[clinic input]
_curses.window.color_set

    pair: pair
    /

Set the window's color pair attribute.
[clinic start generated code]*/

static PyObject *
_curses_window_color_set_impl(PyCursesWindowObject *self, int pair)
/*[clinic end generated code: output=5e9e83f02a29bf1c input=70026f6d411db130]*/
{
    int rtn;
#if _NCURSES_EXTENDED_COLOR_FUNCS
    rtn = wcolor_set(self->win, 0, &pair);
#else
    rtn = wcolor_set(self->win, (short)pair, NULL);
#endif
    return curses_window_check_err(self, rtn, "wcolor_set", "color_set");
}

/*[clinic input]
_curses.window.getattrs

Return the window's current attributes.
[clinic start generated code]*/

static PyObject *
_curses_window_getattrs_impl(PyCursesWindowObject *self)
/*[clinic end generated code: output=835f499205204ec4 input=bf56a0af5b730bd1]*/
{
    return PyLong_FromUnsignedLong((unsigned long)(attr_t)getattrs(self->win));
}

/*[clinic input]
_curses.window.bkgdset

    ch: object
        Background character.
    [
    attr: long
        Background attributes.
    ]
    /

Set the window's background.
[clinic start generated code]*/

static PyObject *
_curses_window_bkgdset_impl(PyCursesWindowObject *self, PyObject *ch,
                            int group_right_1, long attr)
/*[clinic end generated code: output=3c32f2de5685a482 input=1f0811b24af821ca]*/
{
    chtype bkgd;
#ifdef HAVE_NCURSESW
    cchar_t wch;
    int type = PyCurses_ConvertToCell(self, ch, attr, group_right_1,
                                      "bkgdset", &bkgd, &wch);
    if (type == 0) {
        return NULL;
    }
    if (type == 2) {
        wbkgrndset(self->win, &wch);
        Py_RETURN_NONE;
    }
#else
    if (!PyCurses_ConvertToChtype(self, ch, &bkgd))
        return NULL;
#endif

    wbkgdset(self->win, bkgd | (attr_t)attr);
    Py_RETURN_NONE;
}

/*[clinic input]
_curses.window.border

    ls: object(c_default="NULL") = _curses.ACS_VLINE
        Left side.
    rs: object(c_default="NULL") = _curses.ACS_VLINE
        Right side.
    ts: object(c_default="NULL") = _curses.ACS_HLINE
        Top side.
    bs: object(c_default="NULL") = _curses.ACS_HLINE
        Bottom side.
    tl: object(c_default="NULL") = _curses.ACS_ULCORNER
        Upper-left corner.
    tr: object(c_default="NULL") = _curses.ACS_URCORNER
        Upper-right corner.
    bl: object(c_default="NULL") = _curses.ACS_LLCORNER
        Bottom-left corner.
    br: object(c_default="NULL") = _curses.ACS_LRCORNER
        Bottom-right corner.
    /

Draw a border around the edges of the window.

Each parameter specifies the character to use for a specific part of
the border.  The characters can be specified as integers or as
one-character strings.  A 0 value for any parameter will cause the
default character to be used for that parameter.
[clinic start generated code]*/

static PyObject *
_curses_window_border_impl(PyCursesWindowObject *self, PyObject *ls,
                           PyObject *rs, PyObject *ts, PyObject *bs,
                           PyObject *tl, PyObject *tr, PyObject *bl,
                           PyObject *br)
/*[clinic end generated code: output=670ef38d3d7c2aa3 input=42568c1458221d24]*/
{
    chtype ch[8];
    int i, rtn;
    PyObject *objs[8] = {ls, rs, ts, bs, tl, tr, bl, br};

    /* Clear the array of parameters */
    for (i = 0; i < 8; i++)
        ch[i] = 0;

#ifdef HAVE_NCURSESW
    cchar_t wch[8];
    const cchar_t *wch_p[8];
    int use_wide = 0;
    int types[8];
    for (i = 0; i < 8; i++) {
        types[i] = 0;
        if (objs[i] != NULL) {
            types[i] = PyCurses_ConvertToCell(self, objs[i], A_NORMAL, 0,
                                              "border", &ch[i], &wch[i]);
            if (types[i] == 0) {
                return NULL;
            }
            if (types[i] == 2) {
                use_wide = 1;
            }
        }
    }
    if (use_wide) {
        for (i = 0; i < 8; i++) {
            if (objs[i] == NULL) {
                wch_p[i] = NULL;  /* use the default character */
            }
            else if (types[i] == 2) {
                wch_p[i] = &wch[i];
            }
            else {
                PyErr_SetString(PyExc_TypeError,
                                "border() cannot mix integer or bytes "
                                "characters with wide string characters");
                return NULL;
            }
        }
        rtn = wborder_set(self->win,
                          wch_p[0], wch_p[1], wch_p[2], wch_p[3],
                          wch_p[4], wch_p[5], wch_p[6], wch_p[7]);
        return curses_window_check_err(self, rtn, "wborder_set", "border");
    }
#else
    for (i = 0; i < 8; i++) {
        if (objs[i] != NULL && !PyCurses_ConvertToChtype(self, objs[i], &ch[i]))
            return NULL;
    }
#endif

    rtn = wborder(self->win,
                  ch[0], ch[1], ch[2], ch[3],
                  ch[4], ch[5], ch[6], ch[7]);
    return curses_window_check_err(self, rtn, "wborder", "border");
}

/*[clinic input]
_curses.window.box

    [
    verch: object(c_default="_PyLong_GetZero()") = 0
        Left and right side.
    horch: object(c_default="_PyLong_GetZero()") = 0
        Top and bottom side.
    ]
    /

Draw a border around the edges of the window.

Similar to border(), but both ls and rs are verch and both ts and bs
are horch.  The default corner characters are always used by this
function.
[clinic start generated code]*/

static PyObject *
_curses_window_box_impl(PyCursesWindowObject *self, int group_right_1,
                        PyObject *verch, PyObject *horch)
/*[clinic end generated code: output=f3fcb038bb287192 input=e11acb7dbf6790b6]*/
{
    chtype ch1 = 0, ch2 = 0;
#ifdef HAVE_NCURSESW
    cchar_t wch1, wch2;
    int t1 = 0, t2 = 0;
    if (group_right_1) {
        t1 = PyCurses_ConvertToCell(self, verch, A_NORMAL, 0, "box", &ch1, &wch1);
        if (t1 == 0) {
            return NULL;
        }
        t2 = PyCurses_ConvertToCell(self, horch, A_NORMAL, 0, "box", &ch2, &wch2);
        if (t2 == 0) {
            return NULL;
        }
    }
    if (t1 == 2 || t2 == 2) {
        if (t1 != 2 || t2 != 2) {
            PyErr_SetString(PyExc_TypeError,
                            "box() cannot mix integer or bytes characters "
                            "with wide string characters");
            return NULL;
        }
        int rtn = wborder_set(self->win, &wch1, &wch1, &wch2, &wch2,
                              NULL, NULL, NULL, NULL);
        return curses_window_check_err(self, rtn, "wborder_set", "box");
    }
#else
    if (group_right_1) {
        if (!PyCurses_ConvertToChtype(self, verch, &ch1)) {
            return NULL;
        }
        if (!PyCurses_ConvertToChtype(self, horch, &ch2)) {
            return NULL;
        }
    }
#endif
    return curses_window_check_err(self, box(self->win, ch1, ch2), "box", NULL);
}

#if defined(HAVE_NCURSES_H) || defined(MVWDELCH_IS_EXPRESSION)
#define py_mvwdelch mvwdelch
#else
int py_mvwdelch(WINDOW *w, int y, int x)
{
    mvwdelch(w,y,x);
    /* On HP/UX, mvwdelch already returns. On other systems,
       we may well run into this return statement. */
    return 0;
}
#endif

#if defined(HAVE_CURSES_IS_PAD)
// is_pad() is defined, either as a macro or as a function
#define py_is_pad(win)      is_pad(win)
#elif defined(WINDOW_HAS_FLAGS)
// is_pad() is not defined, but we can inspect WINDOW structure members
#define py_is_pad(win)      ((win) ? ((win)->_flags & _ISPAD) != 0 : FALSE)
#endif

/* chgat, added by Fabian Kreutz <fabian.kreutz at gmx.net> */
#ifdef HAVE_CURSES_WCHGAT

PyDoc_STRVAR(_curses_window_chgat__doc__,
"chgat([y, x,] [n=-1,] attr)\n"
"Set the attributes of characters.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"  n\n"
"    Number of characters.\n"
"  attr\n"
"    Attributes for characters.\n"
"\n"
"Set the attributes of num characters at the current cursor position, or at\n"
"position (y, x) if supplied.  If no value of num is given or num = -1, the\n"
"attribute will be set on all the characters to the end of the line.  This\n"
"function does not move the cursor.  The changed line will be touched using\n"
"the touchline() method so that the contents will be redisplayed by the next\n"
"window refresh.");

static PyObject *
PyCursesWindow_ChgAt(PyObject *op, PyObject *args)
{
    PyCursesWindowObject *self = _PyCursesWindowObject_CAST(op);

    int rtn;
    const char *funcname;
    int x, y;
    int num = -1;
    short color;
    attr_t attr = A_NORMAL;
    int use_xy = FALSE;

    switch (PyTuple_Size(args)) {
    case 1:
        if (!PyArg_ParseTuple(args,"O&;attr", attr_converter, &attr))
            return NULL;
        break;
    case 2:
        if (!PyArg_ParseTuple(args,"iO&;n,attr", &num, attr_converter, &attr))
            return NULL;
        break;
    case 3:
        if (!PyArg_ParseTuple(args,"iiO&;y,x,attr", &y, &x, attr_converter, &attr))
            return NULL;
        use_xy = TRUE;
        break;
    case 4:
        if (!PyArg_ParseTuple(args,"iiiO&;y,x,n,attr", &y, &x, &num, attr_converter, &attr))
            return NULL;
        use_xy = TRUE;
        break;
    default:
        PyErr_SetString(PyExc_TypeError,
                        "_curses.window.chgat requires 1 to 4 arguments");
        return NULL;
    }

    color = (short) PAIR_NUMBER(attr);
    attr = attr & A_ATTRIBUTES;

    if (use_xy) {
        rtn = mvwchgat(self->win,y,x,num,attr,color,NULL);
        funcname = "mvwchgat";
    } else {
        getyx(self->win,y,x);
        rtn = wchgat(self->win,num,attr,color,NULL);
        funcname = "wchgat";
    }
    if (rtn == ERR) {
        curses_window_set_error(self, funcname, "chgat");
        return NULL;
    }
    rtn = touchline(self->win,y,1);
    return curses_window_check_err(self, rtn, "touchline", "chgat");
}
#endif

/*[clinic input]
_curses.window.delch

    [
    y: int
        Y-coordinate.
    x: int
        X-coordinate.
    ]
    /

Delete the character under the cursor, or at (y, x) if specified.

All characters to the right on the same line are shifted one
position left.
[clinic start generated code]*/

static PyObject *
_curses_window_delch_impl(PyCursesWindowObject *self, int group_right_1,
                          int y, int x)
/*[clinic end generated code: output=22e77bb9fa11b461 input=61db3c7f4885e90c]*/
{
    int rtn;
    const char *funcname;
    if (!group_right_1) {
        rtn = wdelch(self->win);
        funcname = "wdelch";
    }
    else {
        rtn = py_mvwdelch(self->win, y, x);
        funcname = "mvwdelch";
    }
    return curses_window_check_err(self, rtn, funcname, "delch");
}

/*[clinic input]
_curses.window.derwin

    [
    nlines: int = 0
        Height.
    ncols: int = 0
        Width.
    ]
    begin_y: int
        Top side y-coordinate.
    begin_x: int
        Left side x-coordinate.
    /

Create a sub-window (window-relative coordinates).

derwin() is the same as calling subwin(), except that begin_y and
begin_x are relative to the origin of the window, rather than
relative to the entire screen.
[clinic start generated code]*/

static PyObject *
_curses_window_derwin_impl(PyCursesWindowObject *self, int group_left_1,
                           int nlines, int ncols, int begin_y, int begin_x)
/*[clinic end generated code: output=7924b112d9f70d6e input=6efb50722be444ba]*/
{
    WINDOW *win;

    win = derwin(self->win,nlines,ncols,begin_y,begin_x);

    if (win == NULL) {
        curses_window_set_null_error(self, "derwin", NULL);
        return NULL;
    }

    cursesmodule_state *state = get_cursesmodule_state_by_win(self);
    return PyCursesWindow_New(state, win, NULL, self, self->screen);
}

/*[clinic input]
_curses.window.echochar

    ch: object
        Character to add.

    [
    attr: long
        Attributes for the character.
    ]
    /

Add character ch with attribute attr, and refresh.
[clinic start generated code]*/

static PyObject *
_curses_window_echochar_impl(PyCursesWindowObject *self, PyObject *ch,
                             int group_right_1, long attr)
/*[clinic end generated code: output=f42da9e200c935e5 input=26e16855ec1b0e78]*/
{
    chtype ch_;
#ifdef HAVE_NCURSESW
    cchar_t wch;
    int type = PyCurses_ConvertToCell(self, ch, attr, group_right_1,
                                      "echochar", &ch_, &wch);
    if (type == 0) {
        return NULL;
    }
    if (type == 2) {
        int rtn;
        const char *funcname;
#ifdef py_is_pad
        if (py_is_pad(self->win)) {
            rtn = pecho_wchar(self->win, &wch);
            funcname = "pecho_wchar";
        }
        else
#endif
        {
            rtn = wecho_wchar(self->win, &wch);
            funcname = "wecho_wchar";
        }
        return curses_window_check_err(self, rtn, funcname, "echochar");
    }
#else
    if (!PyCurses_ConvertToChtype(self, ch, &ch_))
        return NULL;
#endif

    int rtn;
    const char *funcname;
#ifdef py_is_pad
    if (py_is_pad(self->win)) {
        rtn = pechochar(self->win, ch_ | (attr_t)attr);
        funcname = "pechochar";
    }
    else
#endif
    {
        rtn = wechochar(self->win, ch_ | (attr_t)attr);
        funcname = "wechochar";
    }
    return curses_window_check_err(self, rtn, funcname, "echochar");
}

#ifdef NCURSES_MOUSE_VERSION
/*[clinic input]
@permit_long_summary
_curses.window.enclose

    y: int
        Y-coordinate.
    x: int
        X-coordinate.
    /

Return True if the screen-relative coordinates are enclosed by the window.
[clinic start generated code]*/

static PyObject *
_curses_window_enclose_impl(PyCursesWindowObject *self, int y, int x)
/*[clinic end generated code: output=8679beef50502648 input=9ba7c894cffe5507]*/
{
    return PyBool_FromLong(wenclose(self->win, y, x));
}
#endif

/*[clinic input]
_curses.window.getbkgd

Return the window's current background character/attribute pair.
[clinic start generated code]*/

static PyObject *
_curses_window_getbkgd_impl(PyCursesWindowObject *self)
/*[clinic end generated code: output=3ff953412b0e6028 input=7cf1f59a31f89df4]*/
{
    chtype rtn = getbkgd(self->win);
    if (rtn == (chtype)ERR) {
        curses_window_set_error(self, "getbkgd", NULL);
        return NULL;
    }
    return PyLong_FromLong(rtn);
}

#ifdef HAVE_NCURSESW
/*[clinic input]
_curses.window.in_wch

    [
    y: int
        Y-coordinate.
    x: int
        X-coordinate.
    ]
    /

Return the complex character at the given position in the window.

The returned object is a complexchar carrying the cell's text,
attributes and color pair.
[clinic start generated code]*/

static PyObject *
_curses_window_in_wch_impl(PyCursesWindowObject *self, int group_right_1,
                           int y, int x)
/*[clinic end generated code: output=846ca8a82f2ecab4 input=a55dd215367dfbb1]*/
{
    cchar_t wcval;
    int rtn;
    const char *funcname;
    if (group_right_1) {
        rtn = mvwin_wch(self->win, y, x, &wcval);
        funcname = "mvwin_wch";
    }
    else {
        rtn = win_wch(self->win, &wcval);
        funcname = "win_wch";
    }
    if (rtn == ERR) {
        curses_window_set_error(self, funcname, "in_wch");
        return NULL;
    }
    cursesmodule_state *state = get_cursesmodule_state_by_win(self);
    return PyCursesComplexChar_New(state, &wcval);
}

/*[clinic input]
_curses.window.getbkgrnd

Return the window's current background complex character.
[clinic start generated code]*/

static PyObject *
_curses_window_getbkgrnd_impl(PyCursesWindowObject *self)
/*[clinic end generated code: output=afec19cad00eff71 input=e06bf3d6bf90d2ec]*/
{
    cchar_t wcval;
    if (wgetbkgrnd(self->win, &wcval) == ERR) {
        curses_window_set_error(self, "wgetbkgrnd", "getbkgrnd");
        return NULL;
    }
    cursesmodule_state *state = get_cursesmodule_state_by_win(self);
    return PyCursesComplexChar_New(state, &wcval);
}

#endif /* HAVE_NCURSESW */

static PyObject *
curses_check_signals_on_input_error(PyCursesWindowObject *self,
                                    const char *curses_funcname,
                                    const char *python_funcname)
{
    assert(!PyErr_Occurred());
    if (PyErr_CheckSignals()) {
        return NULL;
    }
    cursesmodule_state *state = get_cursesmodule_state_by_win(self);
    PyErr_Format(state->error, "%s() (called by %s()): no input",
                 curses_funcname, python_funcname);
    return NULL;
}

/*[clinic input]
_curses.window.getch

    [
    y: int
        Y-coordinate.
    x: int
        X-coordinate.
    ]
    /

Get a character code from terminal keyboard.

The integer returned does not have to be in ASCII range: function
keys, keypad keys and so on return numbers higher than 256.  In
no-delay mode, -1 is returned if there is no input, else getch()
waits until a key is pressed.
[clinic start generated code]*/

static PyObject *
_curses_window_getch_impl(PyCursesWindowObject *self, int group_right_1,
                          int y, int x)
/*[clinic end generated code: output=e1639e87d545e676 input=0dc5ff40e079787a]*/
{
    int rtn;

    Py_BEGIN_ALLOW_THREADS
    if (!group_right_1) {
        rtn = wgetch(self->win);
    }
    else {
        rtn = mvwgetch(self->win, y, x);
    }
    Py_END_ALLOW_THREADS

    if (rtn == ERR) {
        // We suppress ERR returned by wgetch() in nodelay mode
        // after we handled possible interruption signals.
        if (PyErr_CheckSignals()) {
            return NULL;
        }
        // ERR is an implementation detail, so to be on the safe side,
        // we forcibly set the return value to -1 as documented above.
        rtn = -1;
    }
    return PyLong_FromLong(rtn);
}

/*[clinic input]
_curses.window.getkey

    [
    y: int
        Y-coordinate.
    x: int
        X-coordinate.
    ]
    /

Get a character (string) from terminal keyboard.

Returning a string instead of an integer, as getch() does.  Function
keys, keypad keys and other special keys return a multibyte string
containing the key name.  In no-delay mode, an exception is raised
if there is no input.
[clinic start generated code]*/

static PyObject *
_curses_window_getkey_impl(PyCursesWindowObject *self, int group_right_1,
                           int y, int x)
/*[clinic end generated code: output=8490a182db46b10f input=bd24a7da1ed9c73b]*/
{
    int rtn;

    Py_BEGIN_ALLOW_THREADS
    if (!group_right_1) {
        rtn = wgetch(self->win);
    }
    else {
        rtn = mvwgetch(self->win, y, x);
    }
    Py_END_ALLOW_THREADS

    if (rtn == ERR) {
        /* wgetch() returns ERR in nodelay mode */
        const char *funcname = group_right_1 ? "mvwgetch" : "wgetch";
        return curses_check_signals_on_input_error(self, funcname, "getkey");
    } else if (rtn <= 255) {
#ifdef NCURSES_VERSION_MAJOR
#if NCURSES_VERSION_MAJOR*100+NCURSES_VERSION_MINOR <= 507
        /* Work around a bug in ncurses 5.7 and earlier */
        if (rtn < 0) {
            rtn += 256;
        }
#endif
#endif
        return PyUnicode_FromOrdinal(rtn);
    } else {
        const char *knp = keyname(rtn);
        return PyUnicode_FromString((knp == NULL) ? "" : knp);
    }
}

#ifdef HAVE_NCURSESW
/*[clinic input]
_curses.window.get_wch

    [
    y: int
        Y-coordinate.
    x: int
        X-coordinate.
    ]
    /

Get a wide character from terminal keyboard.

Return a character for most keys, or an integer for function keys,
keypad keys, and other special keys.
[clinic start generated code]*/

static PyObject *
_curses_window_get_wch_impl(PyCursesWindowObject *self, int group_right_1,
                            int y, int x)
/*[clinic end generated code: output=9f4f86e91fe50ef3 input=dd7e5367fb49dc48]*/
{
    int ct;
    wint_t rtn;

    Py_BEGIN_ALLOW_THREADS
    if (!group_right_1) {
        ct = wget_wch(self->win ,&rtn);
    }
    else {
        ct = mvwget_wch(self->win, y, x, &rtn);
    }
    Py_END_ALLOW_THREADS

    if (ct == ERR) {
        /* wget_wch() returns ERR in nodelay mode */
        const char *funcname = group_right_1 ? "mvwget_wch" : "wget_wch";
        return curses_check_signals_on_input_error(self, funcname, "get_wch");
    }
    if (ct == KEY_CODE_YES)
        return PyLong_FromLong(rtn);
    else
        return PyUnicode_FromOrdinal(rtn);
}
#endif

/*
 * Helper function for parsing parameters from getstr() and instr().
 * This function is necessary because Argument Clinic does not know
 * how to handle nested optional groups with default values inside.
 *
 * Return 1 on success and 0 on failure, similar to PyArg_ParseTuple().
 */
static int
curses_clinic_parse_optional_xy_n(PyObject *args,
                                  int *y, int *x, unsigned int *n, int *use_xy,
                                  const char *qualname)
{
    switch (PyTuple_GET_SIZE(args)) {
        case 0: {
            *use_xy = 0;
            return 1;
        }
        case 1: {
            *use_xy = 0;
            return PyArg_ParseTuple(args, "O&;n",
                                    _PyLong_UnsignedInt_Converter, n);
        }
        case 2: {
            *use_xy = 1;
            return PyArg_ParseTuple(args, "ii;y,x", y, x);
        }
        case 3: {
            *use_xy = 1;
            return PyArg_ParseTuple(args, "iiO&;y,x,n", y, x,
                                    _PyLong_UnsignedInt_Converter, n);
        }
        default: {
            *use_xy = 0;
            PyErr_Format(PyExc_TypeError, "%s requires 0 to 3 arguments",
                         qualname);
            return 0;
        }
    }
}

PyDoc_STRVAR(_curses_window_getstr__doc__,
"getstr([[y, x,] n=2047])\n"
"Read a string from the user, with primitive line editing capacity.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"  n\n"
"    Maximal number of characters.");

static PyObject *
PyCursesWindow_getstr(PyObject *op, PyObject *args)
{
    PyCursesWindowObject *self = _PyCursesWindowObject_CAST(op);
    int rtn, use_xy = 0, y = 0, x = 0;
    unsigned int max_buf_size = 2048;
    unsigned int n = max_buf_size - 1;

    if (!curses_clinic_parse_optional_xy_n(args, &y, &x, &n, &use_xy,
                                           "_curses.window.instr"))
    {
        return NULL;
    }

    n = Py_MIN(n, max_buf_size - 1);
    PyBytesWriter *writer = PyBytesWriter_Create(n + 1);
    if (writer == NULL) {
        return NULL;
    }
    char *buf = PyBytesWriter_GetData(writer);

    if (use_xy) {
        Py_BEGIN_ALLOW_THREADS
#ifdef STRICT_SYSV_CURSES
        rtn = wmove(self->win, y, x) == ERR
                ? ERR
                : wgetnstr(self->win, buf, n);
#else
        rtn = mvwgetnstr(self->win, y, x, buf, n);
#endif
        Py_END_ALLOW_THREADS
    }
    else {
        Py_BEGIN_ALLOW_THREADS
        rtn = wgetnstr(self->win, buf, n);
        Py_END_ALLOW_THREADS
    }

    if (rtn == ERR) {
        PyBytesWriter_Discard(writer);
        return Py_GetConstant(Py_CONSTANT_EMPTY_BYTES);
    }
    return PyBytesWriter_FinishWithSize(writer, strlen(buf));
}

/*[clinic input]
_curses.window.hline

    [
    y: int
        Starting Y-coordinate.
    x: int
        Starting X-coordinate.
    ]

    ch: object
        Character to draw.
    n: int
        Line length.

    [
    attr: long
        Attributes for the characters.
    ]
    /

Display a horizontal line.
[clinic start generated code]*/

static PyObject *
_curses_window_hline_impl(PyCursesWindowObject *self, int group_left_1,
                          int y, int x, PyObject *ch, int n,
                          int group_right_1, long attr)
/*[clinic end generated code: output=c00d489d61fc9eef input=924f8c28521bc2ec]*/
{
    chtype ch_;
#ifdef HAVE_NCURSESW
    cchar_t wch;
    int type = PyCurses_ConvertToCell(self, ch, attr, group_right_1, "hline",
                                      &ch_, &wch);
    if (type == 0) {
        return NULL;
    }
#else
    if (!PyCurses_ConvertToChtype(self, ch, &ch_))
        return NULL;
#endif
    if (group_left_1) {
        if (wmove(self->win, y, x) == ERR) {
            curses_window_set_error(self, "wmove", "hline");
            return NULL;
        }
    }
#ifdef HAVE_NCURSESW
    if (type == 2) {
        int rtn = whline_set(self->win, &wch, n);
        return curses_window_check_err(self, rtn, "whline_set", "hline");
    }
#endif
    int rtn = whline(self->win, ch_ | (attr_t)attr, n);
    return curses_window_check_err(self, rtn, "whline", "hline");
}

/*[clinic input]
_curses.window.insch

    [
    y: int
        Y-coordinate.
    x: int
        X-coordinate.
    ]

    ch: object
        Character to insert.

    [
    attr: long
        Attributes for the character.
    ]
    /

Insert a character before the current or specified position.

All characters to the right of the cursor are shifted one position
right, with the rightmost characters on the line being lost.
[clinic start generated code]*/

static PyObject *
_curses_window_insch_impl(PyCursesWindowObject *self, int group_left_1,
                          int y, int x, PyObject *ch, int group_right_1,
                          long attr)
/*[clinic end generated code: output=ade8cfe3a3bf3e34 input=47d2989159ae6ca7]*/
{
    int rtn;
    chtype ch_ = 0;
    const char *funcname;
#ifdef HAVE_NCURSESW
    cchar_t wch;
    int type = PyCurses_ConvertToCell(self, ch, attr, group_right_1, "insch",
                                      &ch_, &wch);
    if (type == 0) {
        return NULL;
    }
    if (type == 2) {
        if (!group_left_1) {
            rtn = wins_wch(self->win, &wch);
            funcname = "wins_wch";
        }
        else {
            rtn = mvwins_wch(self->win, y, x, &wch);
            funcname = "mvwins_wch";
        }
        return curses_window_check_err(self, rtn, funcname, "insch");
    }
#else
    if (!PyCurses_ConvertToChtype(self, ch, &ch_))
        return NULL;
#endif

    if (!group_left_1) {
        rtn = winsch(self->win, ch_ | (attr_t)attr);
        funcname = "winsch";
    }
    else {
        rtn = mvwinsch(self->win, y, x, ch_ | (attr_t)attr);
        funcname = "mvwwinsch";
    }

    return curses_window_check_err(self, rtn, funcname, "insch");
}

/*[clinic input]
_curses.window.inch

    [
    y: int
        Y-coordinate.
    x: int
        X-coordinate.
    ]
    /

Return the character at the given position in the window.

The bottom 8 bits are the character proper, and upper bits are the
attributes.
[clinic start generated code]*/

static PyObject *
_curses_window_inch_impl(PyCursesWindowObject *self, int group_right_1,
                         int y, int x)
/*[clinic end generated code: output=97ca8581baaafd06 input=7a03956d94dc9a69]*/
{
    chtype rtn;
    const char *funcname;

    if (!group_right_1) {
        rtn = winch(self->win);
        funcname = "winch";
    }
    else {
        rtn = mvwinch(self->win, y, x);
        funcname = "mvwinch";
    }
    if (rtn == (chtype)ERR) {
        curses_window_set_error(self, funcname, "inch");
        return NULL;
    }
    return PyLong_FromUnsignedLong(rtn);
}

PyDoc_STRVAR(_curses_window_instr__doc__,
"instr([y, x,] n=2047)\n"
"Return a string of characters, extracted from the window.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"  n\n"
"    Maximal number of characters.\n"
"\n"
"Return a string of characters, extracted from the window starting\n"
"at the current cursor position, or at y, x if specified, and\n"
"stopping at the end of the line.  Attributes and color\n"
"information are stripped from the characters.  If n is specified,\n"
"instr() returns a string at most n characters long (exclusive of\n"
"the trailing NUL).");

static PyObject *
PyCursesWindow_instr(PyObject *op, PyObject *args)
{
    PyCursesWindowObject *self = _PyCursesWindowObject_CAST(op);
    int rtn, use_xy = 0, y = 0, x = 0;
    unsigned int max_buf_size = 2048;
    unsigned int n = max_buf_size - 1;

    if (!curses_clinic_parse_optional_xy_n(args, &y, &x, &n, &use_xy,
                                           "_curses.window.instr"))
    {
        return NULL;
    }

    n = Py_MIN(n, max_buf_size - 1);
    PyBytesWriter *writer = PyBytesWriter_Create(n + 1);
    if (writer == NULL) {
        return NULL;
    }
    char *buf = PyBytesWriter_GetData(writer);

    if (use_xy) {
        rtn = mvwinnstr(self->win, y, x, buf, n);
    }
    else {
        rtn = winnstr(self->win, buf, n);
    }

    if (rtn == ERR) {
        PyBytesWriter_Discard(writer);
        return Py_GetConstant(Py_CONSTANT_EMPTY_BYTES);
    }
    return PyBytesWriter_FinishWithSize(writer, strlen(buf));
}

#ifdef HAVE_NCURSESW
PyDoc_STRVAR(_curses_window_get_wstr__doc__,
"get_wstr([[y, x,] n=2047])\n"
"Read a string from the user, with primitive line editing capacity.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"  n\n"
"    Maximal number of characters.\n"
"\n"
"This is the wide-character variant of getstr(); it returns a str.");

static PyObject *
PyCursesWindow_get_wstr(PyObject *op, PyObject *args)
{
    PyCursesWindowObject *self = _PyCursesWindowObject_CAST(op);
    int rtn, use_xy = 0, y = 0, x = 0;
    unsigned int max_buf_size = 2048;
    unsigned int n = max_buf_size - 1;

    if (!curses_clinic_parse_optional_xy_n(args, &y, &x, &n, &use_xy,
                                           "_curses.window.get_wstr"))
    {
        return NULL;
    }

    n = Py_MIN(n, max_buf_size - 1);
    wint_t *buf = PyMem_New(wint_t, n + 1);
    if (buf == NULL) {
        return PyErr_NoMemory();
    }

    if (use_xy) {
        Py_BEGIN_ALLOW_THREADS
        rtn = mvwgetn_wstr(self->win, y, x, buf, n);
        Py_END_ALLOW_THREADS
    }
    else {
        Py_BEGIN_ALLOW_THREADS
        rtn = wgetn_wstr(self->win, buf, n);
        Py_END_ALLOW_THREADS
    }

    if (rtn == ERR) {
        PyMem_Free(buf);
        return Py_GetConstant(Py_CONSTANT_EMPTY_STR);
    }

    /* wgetn_wstr() fills a wint_t buffer; copy it to a wchar_t buffer. */
    Py_ssize_t len = 0;
    while (buf[len]) {
        len++;
    }
    wchar_t *wbuf = PyMem_New(wchar_t, len + 1);
    if (wbuf == NULL) {
        PyMem_Free(buf);
        return PyErr_NoMemory();
    }
    for (Py_ssize_t i = 0; i < len; i++) {
        wbuf[i] = (wchar_t)buf[i];
    }
    PyObject *res = PyUnicode_FromWideChar(wbuf, len);
    PyMem_Free(wbuf);
    PyMem_Free(buf);
    return res;
}

PyDoc_STRVAR(_curses_window_in_wstr__doc__,
"in_wstr([y, x,] n=2047)\n"
"Return a string of characters, extracted from the window.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"  n\n"
"    Maximal number of characters.\n"
"\n"
"This is the wide-character variant of instr(); it returns a str.");

static PyObject *
PyCursesWindow_in_wstr(PyObject *op, PyObject *args)
{
    PyCursesWindowObject *self = _PyCursesWindowObject_CAST(op);
    int rtn, use_xy = 0, y = 0, x = 0;
    unsigned int max_buf_size = 2048;
    unsigned int n = max_buf_size - 1;

    if (!curses_clinic_parse_optional_xy_n(args, &y, &x, &n, &use_xy,
                                           "_curses.window.in_wstr"))
    {
        return NULL;
    }

    n = Py_MIN(n, max_buf_size - 1);
    wchar_t *buf = PyMem_New(wchar_t, n + 1);
    if (buf == NULL) {
        return PyErr_NoMemory();
    }

    if (use_xy) {
        rtn = mvwinnwstr(self->win, y, x, buf, n);
    }
    else {
        rtn = winnwstr(self->win, buf, n);
    }

    if (rtn == ERR) {
        PyMem_Free(buf);
        return Py_GetConstant(Py_CONSTANT_EMPTY_STR);
    }
    PyObject *res = PyUnicode_FromWideChar(buf, -1);
    PyMem_Free(buf);
    return res;
}

PyDoc_STRVAR(_curses_window_in_wchstr__doc__,
"in_wchstr([y, x,] n=2047)\n"
"Return a complexstr of the styled cells extracted from the window.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"  n\n"
"    Maximal number of cells.\n"
"\n"
"This is the wide-character variant of instr() and in_wstr() that keeps\n"
"each cell's attributes and color pair; it returns a complexstr.");

static PyObject *
PyCursesWindow_in_wchstr(PyObject *op, PyObject *args)
{
    PyCursesWindowObject *self = _PyCursesWindowObject_CAST(op);
    int rtn, use_xy = 0, y = 0, x = 0;
    unsigned int max_buf_size = 2048;
    unsigned int n = max_buf_size - 1;

    if (!curses_clinic_parse_optional_xy_n(args, &y, &x, &n, &use_xy,
                                           "_curses.window.in_wchstr"))
    {
        return NULL;
    }

    n = Py_MIN(n, max_buf_size - 1);
    cchar_t *buf = PyMem_New(cchar_t, n + 1);
    if (buf == NULL) {
        return PyErr_NoMemory();
    }

    if (use_xy) {
        rtn = mvwin_wchnstr(self->win, y, x, buf, n);
    }
    else {
        rtn = win_wchnstr(self->win, buf, n);
    }

    cursesmodule_state *state = get_cursesmodule_state_by_win(self);
    if (rtn == ERR) {
        PyMem_Free(buf);
        return PyCursesComplexStr_New(state, NULL, 0);
    }

    /* win_wchnstr() stores at most n cells and zero-terminates the array at
       the actual count; every real cell holds at least a space, so the first
       empty cell marks the end of the run. */
    Py_ssize_t count = 0;
    while (count < (Py_ssize_t)n) {
        wchar_t wstr[CCHARW_MAX + 1];
        attr_t attrs;
        int pair;
        if (curses_getcchar(&buf[count], wstr, &attrs, &pair) == ERR
            || wstr[0] == L'\0')
        {
            break;
        }
        count++;
    }
    PyObject *res = PyCursesComplexStr_New(state, buf, count);
    PyMem_Free(buf);
    return res;
}
#endif /* HAVE_NCURSESW */

/*[clinic input]
_curses.window.insstr

    [
    y: int
        Y-coordinate.
    x: int
        X-coordinate.
    ]

    str: object
        String to insert.

    [
    attr: long
        Attributes for characters.
    ]
    /

Insert the string before the current or specified position.

Insert a character string (as many characters as will fit on the
line) before the character under the cursor.  All characters to the
right of the cursor are shifted right, with the rightmost characters
on the line being lost.  The cursor position does not change (after
moving to y, x, if specified).
[clinic start generated code]*/

static PyObject *
_curses_window_insstr_impl(PyCursesWindowObject *self, int group_left_1,
                           int y, int x, PyObject *str, int group_right_1,
                           long attr)
/*[clinic end generated code: output=c259a5265ad0b777 input=dbfbdd3892155ea6]*/
{
    int rtn;
    int strtype;
    PyObject *bytesobj = NULL;
#ifdef HAVE_NCURSESW
    wchar_t *wstr = NULL;
#endif
    attr_t attr_old = A_NORMAL;
    int use_xy = group_left_1, use_attr = group_right_1;
    const char *funcname;

#ifdef HAVE_NCURSESW
    {
        cursesmodule_state *state = get_cursesmodule_state_by_win(self);
        if (Py_IS_TYPE(str, state->complexstr_type)) {
            if (use_attr) {
                PyErr_SetString(PyExc_TypeError, "insstr(): attr cannot be "
                                "specified together with a complexstr");
                return NULL;
            }
            return curses_window_put_cells(self, str, use_xy, y, x,
                                           -1, 1, "insstr");
        }
    }
    strtype = PyCurses_ConvertToString(self, str, &bytesobj, &wstr);
#else
    strtype = PyCurses_ConvertToString(self, str, &bytesobj, NULL);
#endif
    if (strtype == 0)
        return NULL;

    if (use_attr) {
        attr_old = getattrs(self->win);
        if (curses_wattrset(self, attr, "insstr") < 0) {
            curses_release_wstr(strtype, wstr);
            Py_XDECREF(bytesobj);
            return NULL;
        }
    }
#ifdef HAVE_NCURSESW
    if (strtype == 2) {
        if (use_xy) {
            rtn = mvwins_wstr(self->win,y,x,wstr);
            funcname = "mvwins_wstr";
        }
        else {
            rtn = wins_wstr(self->win,wstr);
            funcname = "wins_wstr";
        }
        PyMem_Free(wstr);
    }
    else
#endif
    {
        const char *str = PyBytes_AS_STRING(bytesobj);
        if (use_xy) {
            rtn = mvwinsstr(self->win,y,x,str);
            funcname = "mvwinsstr";
        }
        else {
            rtn = winsstr(self->win,str);
            funcname = "winsstr";
        }
        Py_DECREF(bytesobj);
    }
    if (rtn == ERR) {
        curses_window_set_error(self, funcname, "insstr");
        return NULL;
    }
    if (use_attr) {
        rtn = wattrset(self->win, attr_old);
        return curses_window_check_err(self, rtn, "wattrset", "insstr");
    }
    Py_RETURN_NONE;
}

/*[clinic input]
_curses.window.insnstr

    [
    y: int
        Y-coordinate.
    x: int
        X-coordinate.
    ]

    str: object
        String to insert.

    n: int
        Maximal number of characters.

    [
    attr: long
        Attributes for characters.
    ]
    /

Insert at most n characters of the string.

Insert a character string (as many characters as will fit on the
line) before the character under the cursor, up to n characters.  If
n is zero or negative, the entire string is inserted.  All
characters to the right of the cursor are shifted right, with the
rightmost characters on the line being lost.  The cursor position
does not change (after moving to y, x, if specified).
[clinic start generated code]*/

static PyObject *
_curses_window_insnstr_impl(PyCursesWindowObject *self, int group_left_1,
                            int y, int x, PyObject *str, int n,
                            int group_right_1, long attr)
/*[clinic end generated code: output=971a32ea6328ec8b input=fd0a9b65b84b385f]*/
{
    int rtn;
    int strtype;
    PyObject *bytesobj = NULL;
#ifdef HAVE_NCURSESW
    wchar_t *wstr = NULL;
#endif
    attr_t attr_old = A_NORMAL;
    int use_xy = group_left_1, use_attr = group_right_1;
    const char *funcname;

#ifdef HAVE_NCURSESW
    {
        cursesmodule_state *state = get_cursesmodule_state_by_win(self);
        if (Py_IS_TYPE(str, state->complexstr_type)) {
            if (use_attr) {
                PyErr_SetString(PyExc_TypeError, "insnstr(): attr cannot be "
                                "specified together with a complexstr");
                return NULL;
            }
            return curses_window_put_cells(self, str, use_xy, y, x,
                                           n, 1, "insnstr");
        }
    }
    strtype = PyCurses_ConvertToString(self, str, &bytesobj, &wstr);
#else
    strtype = PyCurses_ConvertToString(self, str, &bytesobj, NULL);
#endif
    if (strtype == 0)
        return NULL;

    if (use_attr) {
        attr_old = getattrs(self->win);
        if (curses_wattrset(self, attr, "insnstr") < 0) {
            curses_release_wstr(strtype, wstr);
            return NULL;
        }
    }
#ifdef HAVE_NCURSESW
    if (strtype == 2) {
        if (use_xy) {
            rtn = mvwins_nwstr(self->win,y,x,wstr,n);
            funcname = "mvwins_nwstr";
        }
        else {
            rtn = wins_nwstr(self->win,wstr,n);
            funcname = "wins_nwstr";
        }
        PyMem_Free(wstr);
    }
    else
#endif
    {
        const char *str = PyBytes_AS_STRING(bytesobj);
        if (use_xy) {
            rtn = mvwinsnstr(self->win,y,x,str,n);
            funcname = "mvwinsnstr";
        }
        else {
            rtn = winsnstr(self->win,str,n);
            funcname = "winsnstr";
        }
        Py_DECREF(bytesobj);
    }
    if (rtn == ERR) {
        curses_window_set_error(self, funcname, "insnstr");
        return NULL;
    }
    if (use_attr) {
        rtn = wattrset(self->win, attr_old);
        return curses_window_check_err(self, rtn, "wattrset", "insnstr");
    }
    Py_RETURN_NONE;
}

/*[clinic input]
@permit_long_summary
_curses.window.is_linetouched

    line: int
        Line number.
    /

Return True if the specified line was modified, otherwise return False.

Raise a curses.error exception if line is not valid for the given
window.
[clinic start generated code]*/

static PyObject *
_curses_window_is_linetouched_impl(PyCursesWindowObject *self, int line)
/*[clinic end generated code: output=ad4a4edfee2db08c input=18924dfac25ab7f1]*/
{
    int erg;
    erg = is_linetouched(self->win, line);
    if (erg == ERR) {
        curses_window_set_error(self, "is_linetouched", NULL);
        return NULL;
    }
    return PyBool_FromLong(erg);
}

#ifdef py_is_pad
/*[clinic input]
_curses.window.noutrefresh

    [
    pminrow: int
    pmincol: int
    sminrow: int
    smincol: int
    smaxrow: int
    smaxcol: int
    ]
    /

Mark for refresh but wait.

This function updates the data structure representing the desired
state of the window, but does not force an update of the physical
screen.  To accomplish that, call doupdate().
[clinic start generated code]*/

static PyObject *
_curses_window_noutrefresh_impl(PyCursesWindowObject *self,
                                int group_right_1, int pminrow, int pmincol,
                                int sminrow, int smincol, int smaxrow,
                                int smaxcol)
/*[clinic end generated code: output=809a1f3c6a03e23e input=8b4c74bf55008803]*/
#else
/*[clinic input]
_curses.window.noutrefresh

Mark for refresh but wait.

This function updates the data structure representing the desired
state of the window, but does not force an update of the physical
screen.  To accomplish that, call doupdate().
[clinic start generated code]*/

static PyObject *
_curses_window_noutrefresh_impl(PyCursesWindowObject *self)
/*[clinic end generated code: output=6ef6dec666643fee input=a7c6306f8af9d0dd]*/
#endif
{
    int rtn;

#ifdef py_is_pad
    if (py_is_pad(self->win)) {
        if (!group_right_1) {
            PyErr_SetString(PyExc_TypeError,
                            "noutrefresh() called for a pad "
                            "requires 6 arguments");
            return NULL;
        }
        Py_BEGIN_ALLOW_THREADS
        rtn = pnoutrefresh(self->win, pminrow, pmincol,
                           sminrow, smincol, smaxrow, smaxcol);
        Py_END_ALLOW_THREADS
        return curses_window_check_err(self, rtn,
                                       "pnoutrefresh", "noutrefresh");
    }
    if (group_right_1) {
        PyErr_SetString(PyExc_TypeError,
                        "noutrefresh() takes no arguments (6 given)");
        return NULL;
    }
#endif
    Py_BEGIN_ALLOW_THREADS
    rtn = wnoutrefresh(self->win);
    Py_END_ALLOW_THREADS
    return curses_window_check_err(self, rtn, "wnoutrefresh", "noutrefresh");
}

/*[clinic input]
_curses.window.overlay

    destwin: object(type="PyCursesWindowObject *", subclass_of="clinic_state()->window_type")

    [
    sminrow: int
    smincol: int
    dminrow: int
    dmincol: int
    dmaxrow: int
    dmaxcol: int
    ]
    /

Overlay the window on top of destwin.

The windows need not be the same size, only the overlapping region
is copied.  This copy is non-destructive, which means that the
current background character does not overwrite the old contents of
destwin.

To get fine-grained control over the copied region, the second form
of overlay() can be used.  sminrow and smincol are the upper-left
coordinates of the source window, and the other variables mark
a rectangle in the destination window.
[clinic start generated code]*/

static PyObject *
_curses_window_overlay_impl(PyCursesWindowObject *self,
                            PyCursesWindowObject *destwin, int group_right_1,
                            int sminrow, int smincol, int dminrow,
                            int dmincol, int dmaxrow, int dmaxcol)
/*[clinic end generated code: output=82bb2c4cb443ca58 input=da0cec7f7bda1b3f]*/
{
    int rtn;

    if (group_right_1) {
        rtn = copywin(self->win, destwin->win, sminrow, smincol,
                      dminrow, dmincol, dmaxrow, dmaxcol, TRUE);
        return curses_window_check_err(self, rtn, "copywin", "overlay");
    }
    else {
        rtn = overlay(self->win, destwin->win);
        return curses_window_check_err(self, rtn, "overlay", NULL);
    }
}

/*[clinic input]
_curses.window.overwrite

    destwin: object(type="PyCursesWindowObject *", subclass_of="clinic_state()->window_type")

    [
    sminrow: int
    smincol: int
    dminrow: int
    dmincol: int
    dmaxrow: int
    dmaxcol: int
    ]
    /

Overwrite the window on top of destwin.

The windows need not be the same size, in which case only the
overlapping region is copied.  This copy is destructive, which means
that the current background character overwrites the old contents of
destwin.

To get fine-grained control over the copied region, the second form
of overwrite() can be used. sminrow and smincol are the upper-left
coordinates of the source window, the other variables mark
a rectangle in the destination window.
[clinic start generated code]*/

static PyObject *
_curses_window_overwrite_impl(PyCursesWindowObject *self,
                              PyCursesWindowObject *destwin,
                              int group_right_1, int sminrow, int smincol,
                              int dminrow, int dmincol, int dmaxrow,
                              int dmaxcol)
/*[clinic end generated code: output=12ae007d1681be28 input=4244ab8a97087898]*/
{
    int rtn;

    if (group_right_1) {
        rtn = copywin(self->win, destwin->win, sminrow, smincol,
                      dminrow, dmincol, dmaxrow, dmaxcol, FALSE);
        return curses_window_check_err(self, rtn, "copywin", "overwrite");
    }
    else {
        rtn = overwrite(self->win, destwin->win);
        return curses_window_check_err(self, rtn, "overwrite", NULL);
    }
}

/*[clinic input]
@permit_long_summary
_curses.window.putwin

    file: object
    /

Write all data associated with the window into the provided file object.

This information can be later retrieved using the getwin() function.
[clinic start generated code]*/

static PyObject *
_curses_window_putwin_impl(PyCursesWindowObject *self, PyObject *file)
/*[clinic end generated code: output=fdae68ac59b0281b input=959fc85a9e4a31c2]*/
{
    /* We have to simulate this by writing to a temporary FILE*,
       then reading back, then writing to the argument file. */
    FILE *fp;
    PyObject *res = NULL;

    fp = tmpfile();
    if (fp == NULL)
        return PyErr_SetFromErrno(PyExc_OSError);
    if (_Py_set_inheritable(fileno(fp), 0, NULL) < 0)
        goto exit;
    res = curses_window_check_err(self, putwin(self->win, fp), "putwin", NULL);
    if (res == NULL)
        goto exit;
    fseek(fp, 0, 0);
    while (1) {
        char buf[BUFSIZ];
        Py_ssize_t n = fread(buf, 1, BUFSIZ, fp);

        if (n <= 0)
            break;
        Py_DECREF(res);
        res = PyObject_CallMethod(file, "write", "y#", buf, n);
        if (res == NULL)
            break;
    }

exit:
    fclose(fp);
    return res;
}

/*[clinic input]
_curses.window.redrawln

    beg: int
        Starting line number.
    num: int
        The number of lines.
    /

Mark the specified lines corrupted.

They should be completely redrawn on the next refresh() call.
[clinic start generated code]*/

static PyObject *
_curses_window_redrawln_impl(PyCursesWindowObject *self, int beg, int num)
/*[clinic end generated code: output=ea216e334f9ce1b4 input=152155e258a77a7a]*/
{
    int rtn = wredrawln(self->win,beg, num);
    return curses_window_check_err(self, rtn, "wredrawln", "redrawln");
}

/*[clinic input]
_curses.window.refresh

    [
    pminrow: int
    pmincol: int
    sminrow: int
    smincol: int
    smaxrow: int
    smaxcol: int
    ]
    /

Update the display immediately.

Synchronize actual screen with previous drawing/deleting methods.
The 6 optional arguments can only be specified when the window is
a pad created with newpad().  The additional parameters are needed
to indicate what part of the pad and screen are involved.  pminrow
and pmincol specify the upper left-hand corner of the rectangle to
be displayed in the pad.  sminrow, smincol, smaxrow, and smaxcol
specify the edges of the rectangle to be displayed on the screen.
The lower right-hand corner of the rectangle to be displayed in the
pad is calculated from the screen coordinates, since the rectangles
must be the same size.  Both rectangles must be entirely contained
within their respective structures.  Negative values of pminrow,
pmincol, sminrow, or smincol are treated as if they were zero.
[clinic start generated code]*/

static PyObject *
_curses_window_refresh_impl(PyCursesWindowObject *self, int group_right_1,
                            int pminrow, int pmincol, int sminrow,
                            int smincol, int smaxrow, int smaxcol)
/*[clinic end generated code: output=42199543115e6e63 input=ff2e900c6b2696b1]*/
{
    int rtn;

#ifdef py_is_pad
    if (py_is_pad(self->win)) {
        if (!group_right_1) {
            PyErr_SetString(PyExc_TypeError,
                            "refresh() for a pad requires 6 arguments");
            return NULL;
        }
        Py_BEGIN_ALLOW_THREADS
        rtn = prefresh(self->win, pminrow, pmincol,
                       sminrow, smincol, smaxrow, smaxcol);
        Py_END_ALLOW_THREADS
        return curses_window_check_err(self, rtn, "prefresh", "refresh");
    }
#endif
    if (group_right_1) {
        PyErr_SetString(PyExc_TypeError,
                        "refresh() takes no arguments (6 given)");
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    rtn = wrefresh(self->win);
    Py_END_ALLOW_THREADS
    return curses_window_check_err(self, rtn, "wrefresh", "refresh");
}

/*[clinic input]
_curses.window.setscrreg

    top: int
        First line number.
    bottom: int
        Last line number.
    /

Define a software scrolling region.

All scrolling actions will take place in this region.
[clinic start generated code]*/

static PyObject *
_curses_window_setscrreg_impl(PyCursesWindowObject *self, int top,
                              int bottom)
/*[clinic end generated code: output=486ab5db218d2b1a input=1b517b986838bf0e]*/
{
    int rtn = wsetscrreg(self->win, top, bottom);
    return curses_window_check_err(self, rtn, "wsetscrreg", "setscrreg");
}

/*[clinic input]
_curses.window.subwin

    [
    nlines: int = 0
        Height.
    ncols: int = 0
        Width.
    ]
    begin_y: int
        Top side y-coordinate.
    begin_x: int
        Left side x-coordinate.
    /

Create a sub-window (screen-relative coordinates).

By default, the sub-window will extend from the specified position
to the lower right corner of the window.
[clinic start generated code]*/

static PyObject *
_curses_window_subwin_impl(PyCursesWindowObject *self, int group_left_1,
                           int nlines, int ncols, int begin_y, int begin_x)
/*[clinic end generated code: output=93e898afc348f59a input=07b5058cb8820595]*/
{
    WINDOW *win;
    const char *funcname;

    /* printf("Subwin: %i %i %i %i   \n", nlines, ncols, begin_y, begin_x); */
#ifdef py_is_pad
    if (py_is_pad(self->win)) {
        win = subpad(self->win, nlines, ncols, begin_y, begin_x);
        funcname = "subpad";
    }
    else
#endif
    {
        win = subwin(self->win, nlines, ncols, begin_y, begin_x);
        funcname = "subwin";
    }

    if (win == NULL) {
        curses_window_set_null_error(self, funcname, "subwin");
        return NULL;
    }

    cursesmodule_state *state = get_cursesmodule_state_by_win(self);
    return PyCursesWindow_New(state, win, self->encoding, self, self->screen);
}

/*[clinic input]
_curses.window.scroll

    [
    lines: int = 1
        Number of lines to scroll.
    ]
    /

Scroll the screen or scrolling region.

Scroll upward if the argument is positive and downward if it is
negative.
[clinic start generated code]*/

static PyObject *
_curses_window_scroll_impl(PyCursesWindowObject *self, int group_right_1,
                           int lines)
/*[clinic end generated code: output=4541a8a11852d360 input=d8d81a5b52b9b40f]*/
{
    int rtn;
    const char *funcname;
    if (!group_right_1) {
        rtn = scroll(self->win);
        funcname = "scroll";
    }
    else {
        rtn = wscrl(self->win, lines);
        funcname = "wscrl";
    }
    return curses_window_check_err(self, rtn, funcname, "scroll");
}

/*[clinic input]
_curses.window.touchline

    start: int
    count: int
    [
    changed: bool = True
    ]
    /

Pretend count lines have been changed, starting with line start.

If changed is supplied, it specifies whether the affected lines are
marked as having been changed (changed=True) or unchanged
(changed=False).
[clinic start generated code]*/

static PyObject *
_curses_window_touchline_impl(PyCursesWindowObject *self, int start,
                              int count, int group_right_1, int changed)
/*[clinic end generated code: output=65d05b3f7438c61d input=e0dc62f90d9dea55]*/
{
    int rtn;
    const char *funcname;
    if (!group_right_1) {
        rtn = touchline(self->win, start, count);
        funcname = "touchline";
    }
    else {
        rtn = wtouchln(self->win, start, count, changed);
        funcname = "wtouchln";
    }
    return curses_window_check_err(self, rtn, funcname, "touchline");
}

/*[clinic input]
_curses.window.vline

    [
    y: int
        Starting Y-coordinate.
    x: int
        Starting X-coordinate.
    ]

    ch: object
        Character to draw.
    n: int
        Line length.

    [
    attr: long
        Attributes for the character.
    ]
    /

Display a vertical line.
[clinic start generated code]*/

static PyObject *
_curses_window_vline_impl(PyCursesWindowObject *self, int group_left_1,
                          int y, int x, PyObject *ch, int n,
                          int group_right_1, long attr)
/*[clinic end generated code: output=287ad1cc8982217f input=1d4aa27ff0309bbc]*/
{
    chtype ch_;
#ifdef HAVE_NCURSESW
    cchar_t wch;
    int type = PyCurses_ConvertToCell(self, ch, attr, group_right_1, "vline",
                                      &ch_, &wch);
    if (type == 0) {
        return NULL;
    }
#else
    if (!PyCurses_ConvertToChtype(self, ch, &ch_))
        return NULL;
#endif
    if (group_left_1) {
        if (wmove(self->win, y, x) == ERR) {
            curses_window_set_error(self, "wmove", "vline");
            return NULL;
        }
    }
#ifdef HAVE_NCURSESW
    if (type == 2) {
        int rtn = wvline_set(self->win, &wch, n);
        return curses_window_check_err(self, rtn, "wvline_set", "vline");
    }
#endif
    int rtn = wvline(self->win, ch_ | (attr_t)attr, n);
    return curses_window_check_err(self, rtn, "wvline", "vline");
}

static PyObject *
PyCursesWindow_get_encoding(PyObject *op, void *closure)
{
    PyCursesWindowObject *self = _PyCursesWindowObject_CAST(op);
    return PyUnicode_FromString(self->encoding);
}

static int
PyCursesWindow_set_encoding(PyObject *op, PyObject *value, void *Py_UNUSED(ignored))
{
    PyCursesWindowObject *self = _PyCursesWindowObject_CAST(op);

    PyObject *ascii;
    char *encoding;

    /* It is illegal to del win.encoding */
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "encoding may not be deleted");
        return -1;
    }

    if (!PyUnicode_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "setting encoding to a non-string");
        return -1;
    }
    ascii = PyUnicode_AsASCIIString(value);
    if (ascii == NULL)
        return -1;
    encoding = _PyMem_Strdup(PyBytes_AS_STRING(ascii));
    Py_DECREF(ascii);
    if (encoding == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    PyMem_Free(self->encoding);
    self->encoding = encoding;
    return 0;
}

#define clinic_state()  (get_cursesmodule_state_by_cls(Py_TYPE(self)))
#include "clinic/_cursesmodule.c.h"

#ifdef HAVE_NCURSESW
static PyType_Slot PyCursesComplexChar_Type_slots[] = {
    {Py_tp_doc, (void *)complexchar_new__doc__},
    {Py_tp_new, complexchar_new},
    {Py_tp_dealloc, complexchar_dealloc},
    {Py_tp_repr, complexchar_repr},
    {Py_tp_str, complexchar_str},
    {Py_tp_richcompare, complexchar_richcompare},
    {Py_tp_hash, complexchar_hash},
    {Py_tp_getset, complexchar_getsets},
    {0, NULL}
};

static PyType_Spec PyCursesComplexChar_Type_spec = {
    .name = "_curses.complexchar",
    .basicsize = sizeof(PyCursesComplexCharObject),
    .flags = Py_TPFLAGS_DEFAULT
        | Py_TPFLAGS_IMMUTABLETYPE
        | Py_TPFLAGS_HEAPTYPE,
    .slots = PyCursesComplexChar_Type_slots
};

static PyType_Slot PyCursesComplexStr_Type_slots[] = {
    {Py_tp_doc, (void *)complexstr_new__doc__},
    {Py_tp_new, complexstr_new},
    {Py_tp_dealloc, complexstr_dealloc},
    {Py_tp_repr, complexstr_repr},
    {Py_tp_str, complexstr_str},
    {Py_tp_richcompare, complexstr_richcompare},
    {Py_tp_hash, complexstr_hash},
    {Py_sq_length, complexstr_length},
    {Py_sq_concat, complexstr_concat},
    {Py_sq_item, complexstr_item},
    {Py_mp_length, complexstr_length},
    {Py_mp_subscript, complexstr_subscript},
    {0, NULL}
};

static PyType_Spec PyCursesComplexStr_Type_spec = {
    .name = "_curses.complexstr",
    .basicsize = offsetof(PyCursesComplexStrObject, cells),
    .itemsize = sizeof(cchar_t),
    .flags = Py_TPFLAGS_DEFAULT
        | Py_TPFLAGS_IMMUTABLETYPE
        | Py_TPFLAGS_HEAPTYPE,
    .slots = PyCursesComplexStr_Type_slots
};
#endif
#undef clinic_state

#if defined(HAVE_CURSES_USE_SCREEN) || defined(HAVE_CURSES_USE_WINDOW)
/* Shared trampoline for window.use()/screen.use(): call
   func(obj, *extra, **kwargs) and store the result (NULL on exception) in
   data->result. */
typedef struct {
    PyObject *obj;          /* the window or screen object */
    PyObject *func;         /* the callable */
    PyObject *extra;        /* extra positional arguments (a tuple) */
    PyObject *kwargs;       /* keyword arguments (a dict), or NULL */
    PyObject *result;       /* output: the call result, or NULL */
} curses_use_data;

static void
curses_use_call(curses_use_data *data)
{
    Py_ssize_t n = PyTuple_GET_SIZE(data->extra);
    PyObject *callargs = PyTuple_New(n + 1);
    if (callargs == NULL) {
        data->result = NULL;
        return;
    }
    PyTuple_SET_ITEM(callargs, 0, Py_NewRef(data->obj));
    for (Py_ssize_t i = 0; i < n; i++) {
        PyTuple_SET_ITEM(callargs, i + 1,
                         Py_NewRef(PyTuple_GET_ITEM(data->extra, i)));
    }
    data->result = PyObject_Call(data->func, callargs, data->kwargs);
    Py_DECREF(callargs);
}

/* Parse (func, *extra) from a use() method's argument tuple. */
static int
curses_use_parse(PyObject *args, PyObject **func, PyObject **extra)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (nargs < 1) {
        PyErr_SetString(PyExc_TypeError,
                        "use() missing required argument 'func'");
        return -1;
    }
    *func = PyTuple_GET_ITEM(args, 0);
    if (!PyCallable_Check(*func)) {
        PyErr_SetString(PyExc_TypeError, "use(): func must be callable");
        return -1;
    }
    *extra = PyTuple_GetSlice(args, 1, nargs);
    return *extra == NULL ? -1 : 0;
}
#endif

#ifdef HAVE_CURSES_USE_WINDOW
static int
curses_use_window_cb(WINDOW *Py_UNUSED(win), void *data)
{
    curses_use_call((curses_use_data *)data);
    return 0;
}

PyDoc_STRVAR(PyCursesWindow_use__doc__,
"use($self, func, /, *args, **kwargs)\n--\n\n"
"Call func(win, *args, **kwargs) with the window locked,\n"
"and return its result.");

static PyObject *
PyCursesWindow_use(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyCursesWindowObject *wo = _PyCursesWindowObject_CAST(self);
    PyObject *func, *extra;
    if (curses_use_parse(args, &func, &extra) < 0) {
        return NULL;
    }
    curses_use_data data = {self, func, extra, kwargs, NULL};
    use_window(wo->win, curses_use_window_cb, &data);
    Py_DECREF(extra);
    return data.result;
}
#endif /* HAVE_CURSES_USE_WINDOW */

static PyMethodDef PyCursesWindow_methods[] = {
    _CURSES_WINDOW_ADDCH_METHODDEF
    _CURSES_WINDOW_ADDNSTR_METHODDEF
    _CURSES_WINDOW_ADDSTR_METHODDEF
    _CURSES_WINDOW_ATTROFF_METHODDEF
    _CURSES_WINDOW_ATTRON_METHODDEF
    _CURSES_WINDOW_ATTRSET_METHODDEF
    _CURSES_WINDOW_ATTR_GET_METHODDEF
    _CURSES_WINDOW_ATTR_SET_METHODDEF
    _CURSES_WINDOW_ATTR_ON_METHODDEF
    _CURSES_WINDOW_ATTR_OFF_METHODDEF
    _CURSES_WINDOW_COLOR_SET_METHODDEF
    _CURSES_WINDOW_GETATTRS_METHODDEF
    _CURSES_WINDOW_BKGD_METHODDEF
#ifdef HAVE_CURSES_WCHGAT
    {
        "chgat", PyCursesWindow_ChgAt, METH_VARARGS,
        _curses_window_chgat__doc__
    },
#endif
    _CURSES_WINDOW_BKGDSET_METHODDEF
    _CURSES_WINDOW_BORDER_METHODDEF
    _CURSES_WINDOW_BOX_METHODDEF
    {"clear", PyCursesWindow_wclear, METH_NOARGS,
     "clear($self, /)\n--\n\n"
     "Clear the window and repaint it completely on the next refresh()."},
    {"clearok", PyCursesWindow_clearok, METH_VARARGS,
     "clearok($self, flag, /)\n--\n\n"
     "Clear the window on the next refresh() if flag is true."},
    {"clrtobot", PyCursesWindow_wclrtobot, METH_NOARGS,
     "clrtobot($self, /)\n--\n\n"
     "Erase from the cursor to the end of the window."},
    {"clrtoeol", PyCursesWindow_wclrtoeol, METH_NOARGS,
     "clrtoeol($self, /)\n--\n\n"
     "Erase from the cursor to the end of the line."},
    {"cursyncup", PyCursesWindow_wcursyncup, METH_NOARGS,
     "cursyncup($self, /)\n--\n\n"
     "Update the cursor position of all ancestor windows to match."},
    _CURSES_WINDOW_DELCH_METHODDEF
    {"deleteln", PyCursesWindow_wdeleteln, METH_NOARGS,
     "deleteln($self, /)\n--\n\n"
     "Delete the line under the cursor; move following lines up by one."},
    _CURSES_WINDOW_DERWIN_METHODDEF
    _CURSES_WINDOW_ECHOCHAR_METHODDEF
    _CURSES_WINDOW_ENCLOSE_METHODDEF
    {"erase", PyCursesWindow_werase, METH_NOARGS,
     "erase($self, /)\n--\n\n"
     "Clear the window."},
    {"getbegyx", PyCursesWindow_getbegyx, METH_NOARGS,
     "getbegyx($self, /)\n--\n\n"
     "Return a tuple (y, x) of the upper-left corner coordinates."},
    _CURSES_WINDOW_GETBKGD_METHODDEF
    _CURSES_WINDOW_GETBKGRND_METHODDEF
    _CURSES_WINDOW_GETCH_METHODDEF
    _CURSES_WINDOW_GETKEY_METHODDEF
    _CURSES_WINDOW_GET_WCH_METHODDEF
#if defined(NCURSES_EXT_FUNCS) && NCURSES_EXT_FUNCS >= 20110404
    {"getdelay", PyCursesWindow_getdelay, METH_NOARGS,
     "getdelay($self, /)\n--\n\n"
     "Return the window's read timeout in milliseconds.\n\n"
     "-1 means blocking, 0 means non-blocking; see nodelay() and timeout()."},
#endif
    {"getmaxyx", PyCursesWindow_getmaxyx, METH_NOARGS,
     "getmaxyx($self, /)\n--\n\n"
     "Return a tuple (y, x) of the window height and width."},
#if defined(NCURSES_EXT_FUNCS) && NCURSES_EXT_FUNCS >= 20110404
    {"getparent", PyCursesWindow_getparent, METH_NOARGS,
     "getparent($self, /)\n--\n\n"
     "Return the parent window, or None if this is not a subwindow."},
#endif
    {"getparyx", PyCursesWindow_getparyx, METH_NOARGS,
     "getparyx($self, /)\n--\n\n"
     "Return (y, x) relative to the parent window, or (-1, -1) if none."},
#if defined(NCURSES_EXT_FUNCS) && NCURSES_EXT_FUNCS >= 20110404
    {"getscrreg", PyCursesWindow_getscrreg, METH_NOARGS,
     "getscrreg($self, /)\n--\n\n"
     "Return a tuple (top, bottom) of the current scrolling region."},
#endif
    {
        "getstr", PyCursesWindow_getstr, METH_VARARGS,
        _curses_window_getstr__doc__
    },
#ifdef HAVE_NCURSESW
    {
        "get_wstr", PyCursesWindow_get_wstr, METH_VARARGS,
        _curses_window_get_wstr__doc__
    },
#endif
    {"getyx", PyCursesWindow_getyx, METH_NOARGS,
     "getyx($self, /)\n--\n\n"
     "Return a tuple (y, x) of the current cursor position."},
    _CURSES_WINDOW_HLINE_METHODDEF
    {"idcok", PyCursesWindow_idcok, METH_VARARGS,
     "idcok($self, flag, /)\n--\n\n"
     "Enable or disable the hardware insert/delete character feature."},
    {"idlok", PyCursesWindow_idlok, METH_VARARGS,
     "idlok($self, flag, /)\n--\n\n"
     "Enable or disable the hardware insert/delete line feature."},
#ifdef HAVE_CURSES_IMMEDOK
    {"immedok", PyCursesWindow_immedok, METH_VARARGS,
     "immedok($self, flag, /)\n--\n\n"
     "If flag is true, refresh the window on every change to it."},
#endif
    _CURSES_WINDOW_INCH_METHODDEF
    _CURSES_WINDOW_IN_WCH_METHODDEF
    _CURSES_WINDOW_INSCH_METHODDEF
    {"insdelln", PyCursesWindow_winsdelln, METH_VARARGS,
     "insdelln($self, nlines, /)\n--\n\n"
     "Insert (nlines > 0) or delete (nlines < 0) lines above the cursor."},
    {"insertln", PyCursesWindow_winsertln, METH_NOARGS,
     "insertln($self, /)\n--\n\n"
     "Insert a blank line under the cursor; move following lines down."},
    _CURSES_WINDOW_INSNSTR_METHODDEF
    _CURSES_WINDOW_INSSTR_METHODDEF
    {
        "instr", PyCursesWindow_instr, METH_VARARGS,
        _curses_window_instr__doc__
    },
#ifdef HAVE_NCURSESW
    {
        "in_wstr", PyCursesWindow_in_wstr, METH_VARARGS,
        _curses_window_in_wstr__doc__
    },
    {
        "in_wchstr", PyCursesWindow_in_wchstr, METH_VARARGS,
        _curses_window_in_wchstr__doc__
    },
#endif
    _CURSES_WINDOW_IS_LINETOUCHED_METHODDEF
    {"is_wintouched", PyCursesWindow_is_wintouched, METH_NOARGS,
     "is_wintouched($self, /)\n--\n\n"
     "Return True if the window changed since the last refresh()."},
#if defined(NCURSES_EXT_FUNCS) && NCURSES_EXT_FUNCS >= 20110404
    {"is_cleared", PyCursesWindow_is_cleared, METH_NOARGS,
     "is_cleared($self, /)\n--\n\n"
     "Return the current value set by clearok()."},
    {"is_idcok", PyCursesWindow_is_idcok, METH_NOARGS,
     "is_idcok($self, /)\n--\n\n"
     "Return the current value set by idcok()."},
    {"is_idlok", PyCursesWindow_is_idlok, METH_NOARGS,
     "is_idlok($self, /)\n--\n\n"
     "Return the current value set by idlok()."},
    {"is_immedok", PyCursesWindow_is_immedok, METH_NOARGS,
     "is_immedok($self, /)\n--\n\n"
     "Return the current value set by immedok()."},
    {"is_keypad", PyCursesWindow_is_keypad, METH_NOARGS,
     "is_keypad($self, /)\n--\n\n"
     "Return the current value set by keypad()."},
    {"is_leaveok", PyCursesWindow_is_leaveok, METH_NOARGS,
     "is_leaveok($self, /)\n--\n\n"
     "Return the current value set by leaveok()."},
    {"is_nodelay", PyCursesWindow_is_nodelay, METH_NOARGS,
     "is_nodelay($self, /)\n--\n\n"
     "Return the current value set by nodelay()."},
    {"is_notimeout", PyCursesWindow_is_notimeout, METH_NOARGS,
     "is_notimeout($self, /)\n--\n\n"
     "Return the current value set by notimeout()."},
    {"is_pad", PyCursesWindow_is_pad, METH_NOARGS,
     "is_pad($self, /)\n--\n\n"
     "Return True if the window is a pad."},
    {"is_scrollok", PyCursesWindow_is_scrollok, METH_NOARGS,
     "is_scrollok($self, /)\n--\n\n"
     "Return the current value set by scrollok()."},
    {"is_subwin", PyCursesWindow_is_subwin, METH_NOARGS,
     "is_subwin($self, /)\n--\n\n"
     "Return True if the window is a subwindow."},
    {"is_syncok", PyCursesWindow_is_syncok, METH_NOARGS,
     "is_syncok($self, /)\n--\n\n"
     "Return the current value set by syncok()."},
#endif
    {"keypad", PyCursesWindow_keypad, METH_VARARGS,
     "keypad($self, flag, /)\n--\n\n"
     "Interpret escape sequences for special keys if flag is true."},
    {"leaveok", PyCursesWindow_leaveok, METH_VARARGS,
     "leaveok($self, flag, /)\n--\n\n"
     "If flag is true, leave the cursor where the update leaves it."},
    {"move", PyCursesWindow_wmove, METH_VARARGS,
     "move($self, new_y, new_x, /)\n--\n\n"
     "Move the cursor to (new_y, new_x)."},
    {"mvderwin", PyCursesWindow_mvderwin, METH_VARARGS,
     "mvderwin($self, y, x, /)\n--\n\n"
     "Move the window inside its parent window."},
    {"mvwin", PyCursesWindow_mvwin, METH_VARARGS,
     "mvwin($self, new_y, new_x, /)\n--\n\n"
     "Move the window so its upper-left corner is at (new_y, new_x)."},
    {"nodelay", PyCursesWindow_nodelay, METH_VARARGS,
     "nodelay($self, flag, /)\n--\n\n"
     "If flag is true, getch() becomes non-blocking."},
    {"notimeout", PyCursesWindow_notimeout, METH_VARARGS,
     "notimeout($self, flag, /)\n--\n\n"
     "If flag is true, do not time out escape sequences."},
    _CURSES_WINDOW_NOUTREFRESH_METHODDEF
    _CURSES_WINDOW_OVERLAY_METHODDEF
    _CURSES_WINDOW_OVERWRITE_METHODDEF
    _CURSES_WINDOW_PUTWIN_METHODDEF
    _CURSES_WINDOW_REDRAWLN_METHODDEF
    {"redrawwin", PyCursesWindow_redrawwin, METH_NOARGS,
     "redrawwin($self, /)\n--\n\n"
     "Mark the entire window for redraw on the next refresh()."},
    _CURSES_WINDOW_REFRESH_METHODDEF
#ifndef STRICT_SYSV_CURSES
    {"resize", PyCursesWindow_wresize, METH_VARARGS,
     "resize($self, nlines, ncols, /)\n--\n\n"
     "Resize the window to nlines rows and ncols columns."},
#endif
    _CURSES_WINDOW_SCROLL_METHODDEF
    {"scrollok", PyCursesWindow_scrollok, METH_VARARGS,
     "scrollok($self, flag, /)\n--\n\n"
     "Control whether the window scrolls when the cursor moves off it."},
    _CURSES_WINDOW_SETSCRREG_METHODDEF
    {"standend", PyCursesWindow_wstandend, METH_NOARGS,
     "standend($self, /)\n--\n\n"
     "Turn off the standout attribute."},
    {"standout", PyCursesWindow_wstandout, METH_NOARGS,
     "standout($self, /)\n--\n\n"
     "Turn on the A_STANDOUT attribute."},
    {"subpad",          _curses_window_subwin, METH_VARARGS, _curses_window_subwin__doc__},
    _CURSES_WINDOW_SUBWIN_METHODDEF
    {"syncdown", PyCursesWindow_wsyncdown, METH_NOARGS,
     "syncdown($self, /)\n--\n\n"
     "Touch each location changed in any ancestor of the window."},
#ifdef HAVE_CURSES_SYNCOK
    {"syncok", PyCursesWindow_syncok, METH_VARARGS,
     "syncok($self, flag, /)\n--\n\n"
     "If flag is true, call syncup() on every change to the window."},
#endif
    {"syncup", PyCursesWindow_wsyncup, METH_NOARGS,
     "syncup($self, /)\n--\n\n"
     "Touch locations in ancestors that changed in this window."},
    {"timeout", PyCursesWindow_wtimeout, METH_VARARGS,
     "timeout($self, delay, /)\n--\n\n"
     "Set blocking or non-blocking read behavior for the window."},
    _CURSES_WINDOW_TOUCHLINE_METHODDEF
    {"touchwin", PyCursesWindow_touchwin, METH_NOARGS,
     "touchwin($self, /)\n--\n\n"
     "Mark the whole window as changed."},
    {"untouchwin", PyCursesWindow_untouchwin, METH_NOARGS,
     "untouchwin($self, /)\n--\n\n"
     "Mark all lines in the window as unchanged since last refresh()."},
    _CURSES_WINDOW_VLINE_METHODDEF
#ifdef HAVE_CURSES_USE_WINDOW
    {"use", _PyCFunction_CAST(PyCursesWindow_use),
     METH_VARARGS | METH_KEYWORDS, PyCursesWindow_use__doc__},
#endif
    {NULL,                  NULL}   /* sentinel */
};

static PyGetSetDef PyCursesWindow_getsets[] = {
    {
        "encoding",
        PyCursesWindow_get_encoding,
        PyCursesWindow_set_encoding,
        "the typecode character used to create the array"
    },
    {NULL, NULL, NULL, NULL }  /* sentinel */
};

PyDoc_STRVAR(PyCursesWindow_Type_doc,
"A curses window.\n"
"\n"
"Window objects are returned by initscr() and newwin(), and by the\n"
"methods that create subwindows and pads.");

static PyType_Slot PyCursesWindow_Type_slots[] = {
    {Py_tp_doc, (void *)PyCursesWindow_Type_doc},
    {Py_tp_methods, PyCursesWindow_methods},
    {Py_tp_getset, PyCursesWindow_getsets},
    {Py_tp_dealloc, PyCursesWindow_dealloc},
    {Py_tp_traverse, PyCursesWindow_traverse},
    {0, NULL}
};

static PyType_Spec PyCursesWindow_Type_spec = {
    .name = "_curses.window",
    .basicsize =  sizeof(PyCursesWindowObject),
    .flags = Py_TPFLAGS_DEFAULT
        | Py_TPFLAGS_DISALLOW_INSTANTIATION
        | Py_TPFLAGS_IMMUTABLETYPE
        | Py_TPFLAGS_HEAPTYPE
        | Py_TPFLAGS_HAVE_GC,
    .slots = PyCursesWindow_Type_slots
};

/* -------------------------------------------------------*/
/* Screen objects (multiple terminals)                    */
/* -------------------------------------------------------*/

static PyObject *
PyCursesScreen_New(cursesmodule_state *state, SCREEN *screen,
                   FILE *outfp, FILE *infp, PyObject *stdscr)
{
    PyCursesScreenObject *so = PyObject_GC_New(PyCursesScreenObject,
                                               state->screen_type);
    if (so == NULL) {
        return NULL;
    }
    so->screen = screen;
    so->outfp = outfp;
    so->infp = infp;
    so->stdscr = Py_XNewRef(stdscr);
    PyObject_GC_Track((PyObject *)so);
    return (PyObject *)so;
}

/* Free the C SCREEN and the FILE* streams owned by a screen object.
   Safe to call more than once.

   This must run by reference counting (from the dealloc), not from tp_clear:
   it has to happen only once every window on the screen is gone, and thus
   after del_panel() for any panel built on one of those windows.  delscreen()
   tears down the screen that del_panel() needs, so a panel outliving its
   screen would crash. */
static void
curses_screen_close(PyCursesScreenObject *so)
{
    if (so->screen != NULL) {
        delscreen(so->screen);
        so->screen = NULL;
    }
    if (so->outfp != NULL) {
        fclose(so->outfp);
        so->outfp = NULL;
    }
    if (so->infp != NULL) {
        fclose(so->infp);
        so->infp = NULL;
    }
}

static PyObject *
PyCursesScreen_get_stdscr(PyObject *self, void *Py_UNUSED(closure))
{
    PyCursesScreenObject *so = _PyCursesScreenObject_CAST(self);
    if (so->stdscr == NULL) {
        Py_RETURN_NONE;
    }
    return Py_NewRef(so->stdscr);
}

static int
PyCursesScreen_traverse(PyObject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(_PyCursesScreenObject_CAST(self)->stdscr);
    return 0;
}

static int
PyCursesScreen_clear(PyObject *self)
{
    PyCursesScreenObject *so = _PyCursesScreenObject_CAST(self);
    /* Break the reference cycle between a screen and its standard window by
       dropping the reference to that window.  Do NOT delscreen() here: that is
       deferred to the dealloc so it runs after every window (see
       curses_screen_close()).  delscreen() will free the standard window, so
       detach it from its wrapper first: the wrapper must not delwin() a window
       that delscreen() frees.  Any further use of the wrapper operates on a
       NULL window and fails cleanly. */
    if (so->stdscr != NULL) {
        ((PyCursesWindowObject *)so->stdscr)->win = NULL;
    }
    Py_CLEAR(so->stdscr);
    return 0;
}

static void
PyCursesScreen_dealloc(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    (void)PyCursesScreen_clear(self);
    curses_screen_close(_PyCursesScreenObject_CAST(self));
    tp->tp_free(self);
    Py_DECREF(tp);
}

static PyGetSetDef PyCursesScreen_getsets[] = {
    {"stdscr", PyCursesScreen_get_stdscr, NULL,
     "the screen's standard window (stdscr)", NULL},
    {NULL, NULL, NULL, NULL, NULL}  /* sentinel */
};

#ifdef HAVE_CURSES_USE_SCREEN
static int
curses_use_screen_cb(SCREEN *Py_UNUSED(sp), void *data)
{
    curses_use_call((curses_use_data *)data);
    return 0;
}

PyDoc_STRVAR(PyCursesScreen_use__doc__,
"use($self, func, /, *args, **kwargs)\n--\n\n"
"Call func(screen, *args, **kwargs) with the screen locked,\n"
"and return its result.");

static PyObject *
PyCursesScreen_use(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyCursesScreenObject *so = _PyCursesScreenObject_CAST(self);
    if (so->screen == NULL) {
        cursesmodule_state *state = get_cursesmodule_state_by_cls(Py_TYPE(self));
        PyErr_SetString(state->error, "the screen has been deleted");
        return NULL;
    }
    PyObject *func, *extra;
    if (curses_use_parse(args, &func, &extra) < 0) {
        return NULL;
    }
    curses_use_data data = {self, func, extra, kwargs, NULL};
    use_screen(so->screen, curses_use_screen_cb, &data);
    Py_DECREF(extra);
    return data.result;
}
#endif /* HAVE_CURSES_USE_SCREEN */

PyDoc_STRVAR(PyCursesScreen_close__doc__,
"close($self, /)\n--\n\n"
"Detach the screen's standard window, breaking their reference cycle.\n\n"
"Afterwards the stdscr attribute is None and the window it returned earlier\n"
"can no longer be used.  The screen is released once it and its windows are\n"
"no longer referenced.");

static PyObject *
PyCursesScreen_close(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    (void)PyCursesScreen_clear(self);
    Py_RETURN_NONE;
}

static PyMethodDef PyCursesScreen_methods[] = {
    {"close", PyCursesScreen_close, METH_NOARGS,
     PyCursesScreen_close__doc__},
#ifdef HAVE_CURSES_USE_SCREEN
    {"use", _PyCFunction_CAST(PyCursesScreen_use),
     METH_VARARGS | METH_KEYWORDS, PyCursesScreen_use__doc__},
#endif
    {NULL, NULL}  /* sentinel */
};

static PyType_Slot PyCursesScreen_Type_slots[] = {
    {Py_tp_methods, PyCursesScreen_methods},
    {Py_tp_getset, PyCursesScreen_getsets},
    {Py_tp_dealloc, PyCursesScreen_dealloc},
    {Py_tp_traverse, PyCursesScreen_traverse},
    {Py_tp_clear, PyCursesScreen_clear},
    {0, NULL}
};

static PyType_Spec PyCursesScreen_Type_spec = {
    .name = "_curses.screen",
    .basicsize = sizeof(PyCursesScreenObject),
    .flags = Py_TPFLAGS_DEFAULT
        | Py_TPFLAGS_DISALLOW_INSTANTIATION
        | Py_TPFLAGS_IMMUTABLETYPE
        | Py_TPFLAGS_HEAPTYPE
        | Py_TPFLAGS_HAVE_GC,
    .slots = PyCursesScreen_Type_slots
};

/* -------------------------------------------------------*/

/*
 * Macros for implementing simple module's methods.
 *
 * Parameters
 *
 *  X       The name of the curses C function or macro to invoke.
 *  FLAG    When false, prefixes the function name with 'no' at runtime,
 *          This parameter is present in the signature and auto-generated
 *          by Argument Clinic.
 *
 * These macros should only be used for generating the body of
 * the module's methods since they need a module reference.
 *
 * The Python function name must be the same as the curses function name (X).
 */

#define NoArgNoReturnFunctionBody(X)                    \
{                                                       \
    PyCursesStatefulInitialised(module);                \
    return curses_check_err(module, X(), # X, NULL);    \
}

#define NoArgOrFlagNoReturnFunctionBody(X, FLAG)            \
{                                                           \
    PyCursesStatefulInitialised(module);                    \
    int rtn;                                                \
    const char *funcname;                                   \
    if (FLAG) {                                             \
        rtn = X();                                          \
        funcname = # X;                                     \
    }                                                       \
    else {                                                  \
        rtn = no ## X();                                    \
        funcname = "no" # X;                                \
    }                                                       \
    return curses_check_err(module, rtn, funcname, # X);    \
}

#define NoArgReturnIntFunctionBody(X)           \
{                                               \
    PyCursesStatefulInitialised(module);        \
    int rtn = X();                              \
    if (rtn == ERR) {                           \
        curses_set_error(module, # X, NULL);    \
        return NULL;                            \
    }                                           \
    return PyLong_FromLong(rtn);                \
}

#define NoArgReturnStringFunctionBody(X)            \
{                                                   \
    PyCursesStatefulInitialised(module);            \
    const char *res = X();                          \
    if (res == NULL) {                              \
        curses_set_null_error(module, # X, NULL);   \
        return NULL;                                \
    }                                               \
    return PyBytes_FromString(res);                 \
}

#define NoArgTrueFalseFunctionBody(X)       \
{                                           \
    PyCursesStatefulInitialised(module);    \
    return PyBool_FromLong(X());            \
}

#define NoArgNoReturnVoidFunctionBody(X)    \
{                                           \
  PyCursesStatefulInitialised(module);      \
  X();                                      \
  Py_RETURN_NONE;                           \
}

/*********************************************************************
 Global Functions
**********************************************************************/

#ifdef HAVE_CURSES_FILTER
/*[clinic input]
_curses.filter

Restrict screen updates to the current line.

Must be called before initscr().  Afterwards curses confines the cursor
and screen updates to a single line, which is useful for enabling
character-at-a-time line editing without touching the rest of the
screen.
[clinic start generated code]*/

static PyObject *
_curses_filter_impl(PyObject *module)
/*[clinic end generated code: output=fb5b8a3642eb70b5 input=e3c64d6ab2106132]*/
{
    /* not checking for PyCursesInitialised here since filter() must
       be called before initscr() */
    filter();
    Py_RETURN_NONE;
}
#endif

#ifdef HAVE_CURSES_NOFILTER
/*[clinic input]
_curses.nofilter

Undo the effect of a preceding filter() call.

Must be called before initscr().  It restores the normal behaviour that
filter() disables, so that the next initscr() or newterm() uses the full
screen rather than a single line.
[clinic start generated code]*/

static PyObject *
_curses_nofilter_impl(PyObject *module)
/*[clinic end generated code: output=d95ca4d48a6bdbdf input=53183055c0901ab7]*/
{
    /* not checking for PyCursesInitialised here since nofilter() must
       be called before initscr() */
    nofilter();
    Py_RETURN_NONE;
}
#endif

/*[clinic input]
_curses.baudrate

Return the output speed of the terminal in bits per second.
[clinic start generated code]*/

static PyObject *
_curses_baudrate_impl(PyObject *module)
/*[clinic end generated code: output=3c63c6c401d7d9c0 input=921f022ed04a0fd9]*/
NoArgReturnIntFunctionBody(baudrate)

/*[clinic input]
_curses.beep

Emit a short attention sound.
[clinic start generated code]*/

static PyObject *
_curses_beep_impl(PyObject *module)
/*[clinic end generated code: output=425274962abe49a2 input=a35698ca7d0162bc]*/
NoArgNoReturnFunctionBody(beep)

/*[clinic input]
@permit_long_summary
_curses.can_change_color

Return True if the programmer can change the colors displayed by the terminal.
[clinic start generated code]*/

static PyObject *
_curses_can_change_color_impl(PyObject *module)
/*[clinic end generated code: output=359df8c3c77d8bf1 input=8315c364ba1e5b4c]*/
NoArgTrueFalseFunctionBody(can_change_color)

/*[clinic input]
_curses.cbreak

    flag: bool = True
        If false, the effect is the same as calling nocbreak().
    /

Enter cbreak mode.

In cbreak mode (sometimes called "rare" mode) normal tty line buffering
is turned off and characters are available to be read one by one.
However, unlike raw mode, special characters (interrupt, quit, suspend,
and flow control) retain their effects on the tty driver and calling
program.  Calling first raw() then cbreak() leaves the terminal in
cbreak mode.
[clinic start generated code]*/

static PyObject *
_curses_cbreak_impl(PyObject *module, int flag)
/*[clinic end generated code: output=9f9dee9664769751 input=42d81687f11ddbf3]*/
NoArgOrFlagNoReturnFunctionBody(cbreak, flag)

/* is_cbreak()/is_echo()/is_nl()/is_raw() were added in ncurses 6.5. */
#if defined(NCURSES_EXT_FUNCS) && NCURSES_EXT_FUNCS >= 20240427
/*[clinic input]
_curses.is_cbreak

Return True if cbreak mode is enabled, False otherwise.
[clinic start generated code]*/

static PyObject *
_curses_is_cbreak_impl(PyObject *module)
/*[clinic end generated code: output=8a1ad7889fb43daf input=99988df6fd2f1c81]*/
{
    PyCursesStatefulInitialised(module);
    return PyBool_FromLong(is_cbreak());
}

/*[clinic input]
_curses.is_echo

Return True if echo mode is enabled, False otherwise.
[clinic start generated code]*/

static PyObject *
_curses_is_echo_impl(PyObject *module)
/*[clinic end generated code: output=72692d2aa41591c4 input=f6152cf7c00e47eb]*/
{
    PyCursesStatefulInitialised(module);
    return PyBool_FromLong(is_echo());
}

/*[clinic input]
_curses.is_nl

Return True if nl mode is enabled, False otherwise.
[clinic start generated code]*/

static PyObject *
_curses_is_nl_impl(PyObject *module)
/*[clinic end generated code: output=999eb44abc43ce65 input=1e0a2607e45a01e1]*/
{
    PyCursesStatefulInitialised(module);
    return PyBool_FromLong(is_nl());
}

/*[clinic input]
_curses.is_raw

Return True if raw mode is enabled, False otherwise.
[clinic start generated code]*/

static PyObject *
_curses_is_raw_impl(PyObject *module)
/*[clinic end generated code: output=dd9816d777561c35 input=a64fa6a251ed3ece]*/
{
    PyCursesStatefulInitialised(module);
    return PyBool_FromLong(is_raw());
}
#endif /* NCURSES_EXT_FUNCS */

/*[clinic input]
_curses.color_content

    color_number: color
        The number of the color (0 - (COLORS-1)).
    /

Return the red, green, and blue (RGB) components of the specified color.

A 3-tuple is returned, containing the R, G, B values for the given
color, which will be between 0 (no component) and 1000 (maximum amount
of component).
[clinic start generated code]*/

static PyObject *
_curses_color_content_impl(PyObject *module, int color_number)
/*[clinic end generated code: output=17b466df7054e0de input=c95fb50093fa0be0]*/
{
    _CURSES_COLOR_VAL_TYPE r,g,b;

    PyCursesStatefulInitialised(module);
    PyCursesStatefulInitialisedColor(module);

    if (_COLOR_CONTENT_FUNC(color_number, &r, &g, &b) == ERR) {
        const char *funcname = Py_STRINGIFY(_COLOR_CONTENT_FUNC);
        curses_set_error(module, funcname, "color_content");
        return NULL;
    }

    return Py_BuildValue("(iii)", r, g, b);
}

/*[clinic input]
_curses.color_pair

    pair_number: int
        The number of the color pair.
    /

Return the attribute value for displaying text in the specified color.

This attribute value can be combined with A_STANDOUT, A_REVERSE, and the
other A_* attributes.  pair_number() is the counterpart to this
function.
[clinic start generated code]*/

static PyObject *
_curses_color_pair_impl(PyObject *module, int pair_number)
/*[clinic end generated code: output=60718abb10ce9feb input=cf74bb81d3cc3370]*/
{
    PyCursesStatefulInitialised(module);
    PyCursesStatefulInitialisedColor(module);

    return  PyLong_FromLong(COLOR_PAIR(pair_number));
}

/*[clinic input]
_curses.curs_set

    visibility: int
        0 for invisible, 1 for normal visible, or 2 for very visible.
    /

Set the cursor state.

If the terminal supports the visibility requested, the previous cursor
state is returned; otherwise, an exception is raised.  On many
terminals, the "visible" mode is an underline cursor and the "very
visible" mode is a block cursor.
[clinic start generated code]*/

static PyObject *
_curses_curs_set_impl(PyObject *module, int visibility)
/*[clinic end generated code: output=ee8e62483b1d6cd4 input=e010767a328f322b]*/
{
    int erg;

    PyCursesStatefulInitialised(module);

    erg = curs_set(visibility);
    if (erg == ERR) {
        curses_set_error(module, "curs_set", NULL);
        return NULL;
    }

    return PyLong_FromLong((long) erg);
}

/*[clinic input]
_curses.def_prog_mode

Save the current terminal mode as the "program" mode.

The "program" mode is the mode when the running program is using curses.

Subsequent calls to reset_prog_mode() will restore this mode.
[clinic start generated code]*/

static PyObject *
_curses_def_prog_mode_impl(PyObject *module)
/*[clinic end generated code: output=05d5a351fff874aa input=768b9cace620dda5]*/
NoArgNoReturnFunctionBody(def_prog_mode)

/*[clinic input]
_curses.def_shell_mode

Save the current terminal mode as the "shell" mode.

The "shell" mode is the mode when the running program is not using
curses.

Subsequent calls to reset_shell_mode() will restore this mode.
[clinic start generated code]*/

static PyObject *
_curses_def_shell_mode_impl(PyObject *module)
/*[clinic end generated code: output=d6e42f5c768f860f input=3809f85615c0b693]*/
NoArgNoReturnFunctionBody(def_shell_mode)

/*[clinic input]
_curses.delay_output

    ms: int
        Duration in milliseconds.
    /

Insert a pause in output.
[clinic start generated code]*/

static PyObject *
_curses_delay_output_impl(PyObject *module, int ms)
/*[clinic end generated code: output=b6613a67f17fa4f4 input=5316457f5f59196c]*/
{
    PyCursesStatefulInitialised(module);

    return curses_check_err(module, delay_output(ms), "delay_output", NULL);
}

/*[clinic input]
_curses.doupdate

Update the physical screen to match the virtual screen.
[clinic start generated code]*/

static PyObject *
_curses_doupdate_impl(PyObject *module)
/*[clinic end generated code: output=f34536975a75680c input=8da80914432a6489]*/
NoArgNoReturnFunctionBody(doupdate)

/*[clinic input]
_curses.echo

    flag: bool = True
        If false, the effect is the same as calling noecho().
    /

Enter echo mode.

In echo mode, each character input is echoed to the screen as it is
entered.
[clinic start generated code]*/

static PyObject *
_curses_echo_impl(PyObject *module, int flag)
/*[clinic end generated code: output=03acb2ddfa6c8729 input=b4e9064326da9da4]*/
NoArgOrFlagNoReturnFunctionBody(echo, flag)

/*[clinic input]
_curses.endwin

De-initialize the library, and return terminal to normal status.
[clinic start generated code]*/

static PyObject *
_curses_endwin_impl(PyObject *module)
/*[clinic end generated code: output=c0150cd96d2f4128 input=e172cfa43062f3fa]*/
{
    PyCursesStatefulInitialised(module);

    /* endwin() writes to the terminal and may call tcdrain(), which can block
       (e.g. on a pty whose output is not being read); release the GIL so other
       threads -- including one draining that terminal -- can run meanwhile. */
    int code;
    Py_BEGIN_ALLOW_THREADS
    code = endwin();
    Py_END_ALLOW_THREADS
    return curses_check_err(module, code, "endwin", NULL);
}

/*[clinic input]
_curses.erasechar

Return the user's current erase character.
[clinic start generated code]*/

static PyObject *
_curses_erasechar_impl(PyObject *module)
/*[clinic end generated code: output=3df305dc6b926b3f input=628c136c3c5758d3]*/
{
    char ch;

    PyCursesStatefulInitialised(module);

    ch = erasechar();

    return PyBytes_FromStringAndSize(&ch, 1);
}

#ifdef HAVE_NCURSESW
/*[clinic input]
_curses.erasewchar

Return the user's current wide-character erase character.
[clinic start generated code]*/

static PyObject *
_curses_erasewchar_impl(PyObject *module)
/*[clinic end generated code: output=7f3bd8c9097ac456 input=f7e9a3893b4df2f8]*/
{
    wchar_t ch;

    PyCursesStatefulInitialised(module);

    if (erasewchar(&ch) == ERR) {
        curses_set_error(module, "erasewchar", NULL);
        return NULL;
    }
    return PyUnicode_FromWideChar(&ch, 1);
}
#endif /* HAVE_NCURSESW */

/*[clinic input]
_curses.flash

Flash the screen.

That is, change it to reverse-video and then change it back in a short
interval.
[clinic start generated code]*/

static PyObject *
_curses_flash_impl(PyObject *module)
/*[clinic end generated code: output=488b8a0ebd9ea9b8 input=90878e305432add9]*/
NoArgNoReturnFunctionBody(flash)

/*[clinic input]
_curses.flushinp

Flush all input buffers.

This throws away any typeahead that has been typed by the user and has
not yet been processed by the program.
[clinic start generated code]*/

static PyObject *
_curses_flushinp_impl(PyObject *module)
/*[clinic end generated code: output=7e7a1fc1473960f5 input=3a63c7213be8043c]*/
NoArgNoReturnVoidFunctionBody(flushinp)

#ifdef getsyx
/*[clinic input]
_curses.getsyx

Return the current coordinates of the virtual screen cursor.

Return a (y, x) tuple.  If leaveok is currently true, return (-1, -1).
[clinic start generated code]*/

static PyObject *
_curses_getsyx_impl(PyObject *module)
/*[clinic end generated code: output=c8e6c3f42349a038 input=9e1f862f3b4f7cba]*/
{
    int x = 0;
    int y = 0;

    PyCursesStatefulInitialised(module);

    getsyx(y, x);

    return Py_BuildValue("(ii)", y, x);
}
#endif

#ifdef NCURSES_MOUSE_VERSION
/*[clinic input]
_curses.getmouse

Retrieve the queued mouse event.

After getch() returns KEY_MOUSE to signal a mouse event, this function
returns a 5-tuple (id, x, y, z, bstate).
[clinic start generated code]*/

static PyObject *
_curses_getmouse_impl(PyObject *module)
/*[clinic end generated code: output=ccf4242546b9cfa8 input=5b756ee6f5b481b1]*/
{
    int rtn;
    MEVENT event;

    PyCursesStatefulInitialised(module);

    rtn = getmouse( &event );
    if (rtn == ERR) {
        curses_set_error(module, "getmouse", NULL);
        return NULL;
    }
    return Py_BuildValue("(hiiik)",
                         (short)event.id,
                         (int)event.x, (int)event.y, (int)event.z,
                         (unsigned long) event.bstate);
}

/*[clinic input]
_curses.ungetmouse

    id: short
    x: int
    y: int
    z: int
    bstate: unsigned_long(bitwise=True)
    /

Push a KEY_MOUSE event onto the input queue.

The following getmouse() will return the given state data.
[clinic start generated code]*/

static PyObject *
_curses_ungetmouse_impl(PyObject *module, short id, int x, int y, int z,
                        unsigned long bstate)
/*[clinic end generated code: output=3430c9b0fc5c4341 input=fd650b2ca5a01e8f]*/
{
    MEVENT event;

    PyCursesStatefulInitialised(module);

    event.id = id;
    event.x = x;
    event.y = y;
    event.z = z;
    event.bstate = bstate;
    return curses_check_err(module, ungetmouse(&event), "ungetmouse", NULL);
}
#endif

/*[clinic input]
_curses.getwin

    file: object
    /

Read window related data stored in the file by an earlier putwin() call.

The routine then creates and initializes a new window using that data,
returning the new window object.
[clinic start generated code]*/

static PyObject *
_curses_getwin(PyObject *module, PyObject *file)
/*[clinic end generated code: output=a79e0df3379af756 input=f713d2bba0e4c929]*/
{
    FILE *fp;
    PyObject *data;
    size_t datalen;
    WINDOW *win;
    PyObject *res = NULL;

    PyCursesStatefulInitialised(module);

    fp = tmpfile();
    if (fp == NULL)
        return PyErr_SetFromErrno(PyExc_OSError);

    if (_Py_set_inheritable(fileno(fp), 0, NULL) < 0)
        goto error;

    data = PyObject_CallMethod(file, "read", NULL);
    if (data == NULL)
        goto error;
    if (!PyBytes_Check(data)) {
        PyErr_Format(PyExc_TypeError,
                     "f.read() returned %.100s instead of bytes",
                     Py_TYPE(data)->tp_name);
        Py_DECREF(data);
        goto error;
    }
    datalen = PyBytes_GET_SIZE(data);
    if (fwrite(PyBytes_AS_STRING(data), 1, datalen, fp) != datalen) {
        PyErr_SetFromErrno(PyExc_OSError);
        Py_DECREF(data);
        goto error;
    }
    Py_DECREF(data);

    fseek(fp, 0, 0);
    win = getwin(fp);
    if (win == NULL) {
        curses_set_null_error(module, "getwin", NULL);
        goto error;
    }
    cursesmodule_state *state = get_cursesmodule_state(module);
    res = PyCursesWindow_New(state, win, NULL, NULL, state->topscreen);

error:
    fclose(fp);
    return res;
}

/*[clinic input]
_curses.halfdelay

    tenths: byte
        Maximal blocking delay in tenths of seconds (1 - 255).
    /

Enter half-delay mode.

Use nocbreak() to leave half-delay mode.
[clinic start generated code]*/

static PyObject *
_curses_halfdelay_impl(PyObject *module, unsigned char tenths)
/*[clinic end generated code: output=e92cdf0ef33c0663 input=e42dce7259c15100]*/
{
    PyCursesStatefulInitialised(module);

    return curses_check_err(module, halfdelay(tenths), "halfdelay", NULL);
}

/*[clinic input]
_curses.has_colors

Return True if the terminal can display colors; otherwise, return False.
[clinic start generated code]*/

static PyObject *
_curses_has_colors_impl(PyObject *module)
/*[clinic end generated code: output=db5667483139e3e2 input=b2ec41b739d896c6]*/
NoArgTrueFalseFunctionBody(has_colors)

/*[clinic input]
@permit_long_summary
_curses.has_ic

Return True if the terminal has insert- and delete-character capabilities.
[clinic start generated code]*/

static PyObject *
_curses_has_ic_impl(PyObject *module)
/*[clinic end generated code: output=6be24da9cb1268fe input=e37fa080d879f7a9]*/
NoArgTrueFalseFunctionBody(has_ic)

/*[clinic input]
_curses.has_il

Return True if the terminal has insert- and delete-line capabilities.
[clinic start generated code]*/

static PyObject *
_curses_has_il_impl(PyObject *module)
/*[clinic end generated code: output=d45bd7788ff9f5f4 input=cd939d5607ee5427]*/
NoArgTrueFalseFunctionBody(has_il)

#ifdef HAVE_CURSES_HAS_KEY
/*[clinic input]
@permit_long_summary
_curses.has_key

    key: int
        Key number.
    /

Return True if the current terminal type recognizes a key with that value.
[clinic start generated code]*/

static PyObject *
_curses_has_key_impl(PyObject *module, int key)
/*[clinic end generated code: output=19ad48319414d0b1 input=046ac6c72bbc9587]*/
{
    PyCursesStatefulInitialised(module);

    return PyBool_FromLong(has_key(key));
}
#endif

/*[clinic input]
_curses.init_color

    color_number: color
        The number of the color to be changed (0 - (COLORS-1)).
    r: component
        Red component (0 - 1000).
    g: component
        Green component (0 - 1000).
    b: component
        Blue component (0 - 1000).
    /

Change the definition of a color.

When init_color() is used, all occurrences of that color on the screen
immediately change to the new definition.  This function is a no-op on
most terminals; it is active only if can_change_color() returns true.
[clinic start generated code]*/

static PyObject *
_curses_init_color_impl(PyObject *module, int color_number, short r, short g,
                        short b)
/*[clinic end generated code: output=d7ed71b2d818cdf2 input=ae2b8bea0f152c80]*/
{
    PyCursesStatefulInitialised(module);
    PyCursesStatefulInitialisedColor(module);

    return curses_check_err(module,
                            _CURSES_INIT_COLOR_FUNC(color_number, r, g, b),
                            Py_STRINGIFY(_CURSES_INIT_COLOR_FUNC),
                            NULL);
}

/*[clinic input]
_curses.init_pair

    pair_number: pair
        The number of the color-pair to be changed (1 - (COLOR_PAIRS-1)).
    fg: color_allow_default
        Foreground color number (-1 - (COLORS-1)).
    bg: color_allow_default
        Background color number (-1 - (COLORS-1)).
    /

Change the definition of a color-pair.

If the color-pair was previously initialized, the screen is refreshed
and all occurrences of that color-pair are changed to the new
definition.
[clinic start generated code]*/

static PyObject *
_curses_init_pair_impl(PyObject *module, int pair_number, int fg, int bg)
/*[clinic end generated code: output=a0bba03d2bbc3ee6 input=5486c3a105130dae]*/
{
    PyCursesStatefulInitialised(module);
    PyCursesStatefulInitialisedColor(module);

    if (_CURSES_INIT_PAIR_FUNC(pair_number, fg, bg) == ERR) {
        if (pair_number >= COLOR_PAIRS) {
            PyErr_Format(PyExc_ValueError,
                         "Color pair is greater than COLOR_PAIRS-1 (%d).",
                         COLOR_PAIRS - 1);
        }
        else {
            const char *funcname = Py_STRINGIFY(_CURSES_INIT_PAIR_FUNC);
            curses_set_error(module, funcname, "init_pair");
        }
        return NULL;
    }

    Py_RETURN_NONE;
}

#if _NCURSES_EXTENDED_COLOR_FUNCS
/*[clinic input]
_curses.alloc_pair

    fg: color_allow_default
        Foreground color number.
    bg: color_allow_default
        Background color number.
    /

Allocate a color pair for the given foreground and background colors.

If a color pair for the same colors already exists, return its number.
Otherwise allocate a new color pair and return its number.
[clinic start generated code]*/

static PyObject *
_curses_alloc_pair_impl(PyObject *module, int fg, int bg)
/*[clinic end generated code: output=6eb08cb643d4b5a2 input=b29bafd7b360fa35]*/
{
    PyCursesStatefulInitialised(module);
    PyCursesStatefulInitialisedColor(module);

    int pair = alloc_pair(fg, bg);
    if (pair < 0) {
        curses_set_error(module, "alloc_pair", NULL);
        return NULL;
    }
    return PyLong_FromLong(pair);
}

/*[clinic input]
_curses.find_pair

    fg: color_allow_default
        Foreground color number.
    bg: color_allow_default
        Background color number.
    /

Return the number of a color pair for the given colors, or -1.

Return -1 if no color pair for this combination of foreground and
background colors has been allocated.
[clinic start generated code]*/

static PyObject *
_curses_find_pair_impl(PyObject *module, int fg, int bg)
/*[clinic end generated code: output=376026c2a3ac4a9b input=930feac14892c251]*/
{
    PyCursesStatefulInitialised(module);
    PyCursesStatefulInitialisedColor(module);

    return PyLong_FromLong(find_pair(fg, bg));
}

/*[clinic input]
_curses.free_pair

    pair: pair
        The number of the color pair to free.
    /

Free a color pair allocated by alloc_pair().
[clinic start generated code]*/

static PyObject *
_curses_free_pair_impl(PyObject *module, int pair)
/*[clinic end generated code: output=61be0fb2e4bb4e4a input=d24df62feb4161c6]*/
{
    PyCursesStatefulInitialised(module);
    PyCursesStatefulInitialisedColor(module);

    return curses_check_err(module, free_pair(pair), "free_pair", NULL);
}

/*[clinic input]
_curses.reset_color_pairs

Discard all color-pair definitions.
[clinic start generated code]*/

static PyObject *
_curses_reset_color_pairs_impl(PyObject *module)
/*[clinic end generated code: output=117e68c6614e1d06 input=57c1cf7e5447e1ac]*/
{
    PyCursesStatefulInitialised(module);
    PyCursesStatefulInitialisedColor(module);

    reset_color_pairs();
    Py_RETURN_NONE;
}
#endif /* _NCURSES_EXTENDED_COLOR_FUNCS */

/* Refresh the private copy of the screen encoding from a freshly created
   stdscr window object.  Returns 0 on success, -1 with an exception set. */
static int
curses_update_screen_encoding(PyObject *winobj)
{
    char *copy = _PyMem_Strdup(((PyCursesWindowObject *)winobj)->encoding);
    if (copy == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    PyMem_Free(curses_screen_encoding);
    curses_screen_encoding = copy;
    return 0;
}

/* Populate the module dictionary with the ACS_* line-drawing constants and
   LINES/COLS.  These are only meaningful once a screen exists (after
   initscr() or newterm()), which is why this is not done at module
   initialisation.  Returns 0 on success, -1 with an exception set. */
static int
curses_init_dict(PyObject *module)
{
    PyObject *module_dict = PyModule_GetDict(module); // borrowed
    if (module_dict == NULL) {
        return -1;
    }
    /* This was moved from initcurses() because it core dumped on SGI,
       where they're not defined until you've called initscr() */
#define SetDictInt(NAME, VALUE)                                     \
    do {                                                            \
        PyObject *value = PyLong_FromLong((long)(VALUE));           \
        if (value == NULL) {                                        \
            return -1;                                              \
        }                                                           \
        int rc = PyDict_SetItemString(module_dict, (NAME), value);  \
        Py_DECREF(value);                                           \
        if (rc < 0) {                                               \
            return -1;                                              \
        }                                                           \
    } while (0)

    /* Here are some graphic symbols you can use */
    SetDictInt("ACS_ULCORNER",      (ACS_ULCORNER));
    SetDictInt("ACS_LLCORNER",      (ACS_LLCORNER));
    SetDictInt("ACS_URCORNER",      (ACS_URCORNER));
    SetDictInt("ACS_LRCORNER",      (ACS_LRCORNER));
    SetDictInt("ACS_LTEE",          (ACS_LTEE));
    SetDictInt("ACS_RTEE",          (ACS_RTEE));
    SetDictInt("ACS_BTEE",          (ACS_BTEE));
    SetDictInt("ACS_TTEE",          (ACS_TTEE));
    SetDictInt("ACS_HLINE",         (ACS_HLINE));
    SetDictInt("ACS_VLINE",         (ACS_VLINE));
    SetDictInt("ACS_PLUS",          (ACS_PLUS));
#if !defined(__hpux) || defined(HAVE_NCURSES_H)
    /* On HP/UX 11, these are of type cchar_t, which is not an
       integral type. If this is a problem on more platforms, a
       configure test should be added to determine whether ACS_S1
       is of integral type. */
    SetDictInt("ACS_S1",            (ACS_S1));
    SetDictInt("ACS_S9",            (ACS_S9));
    SetDictInt("ACS_DIAMOND",       (ACS_DIAMOND));
    SetDictInt("ACS_CKBOARD",       (ACS_CKBOARD));
    SetDictInt("ACS_DEGREE",        (ACS_DEGREE));
    SetDictInt("ACS_PLMINUS",       (ACS_PLMINUS));
    SetDictInt("ACS_BULLET",        (ACS_BULLET));
    SetDictInt("ACS_LARROW",        (ACS_LARROW));
    SetDictInt("ACS_RARROW",        (ACS_RARROW));
    SetDictInt("ACS_DARROW",        (ACS_DARROW));
    SetDictInt("ACS_UARROW",        (ACS_UARROW));
    SetDictInt("ACS_BOARD",         (ACS_BOARD));
    SetDictInt("ACS_LANTERN",       (ACS_LANTERN));
    SetDictInt("ACS_BLOCK",         (ACS_BLOCK));
#endif
    SetDictInt("ACS_BSSB",          (ACS_ULCORNER));
    SetDictInt("ACS_SSBB",          (ACS_LLCORNER));
    SetDictInt("ACS_BBSS",          (ACS_URCORNER));
    SetDictInt("ACS_SBBS",          (ACS_LRCORNER));
    SetDictInt("ACS_SBSS",          (ACS_RTEE));
    SetDictInt("ACS_SSSB",          (ACS_LTEE));
    SetDictInt("ACS_SSBS",          (ACS_BTEE));
    SetDictInt("ACS_BSSS",          (ACS_TTEE));
    SetDictInt("ACS_BSBS",          (ACS_HLINE));
    SetDictInt("ACS_SBSB",          (ACS_VLINE));
    SetDictInt("ACS_SSSS",          (ACS_PLUS));

    /* The following are never available with strict SYSV curses */
#ifdef ACS_S3
    SetDictInt("ACS_S3",            (ACS_S3));
#endif
#ifdef ACS_S7
    SetDictInt("ACS_S7",            (ACS_S7));
#endif
#ifdef ACS_LEQUAL
    SetDictInt("ACS_LEQUAL",        (ACS_LEQUAL));
#endif
#ifdef ACS_GEQUAL
    SetDictInt("ACS_GEQUAL",        (ACS_GEQUAL));
#endif
#ifdef ACS_PI
    SetDictInt("ACS_PI",            (ACS_PI));
#endif
#ifdef ACS_NEQUAL
    SetDictInt("ACS_NEQUAL",        (ACS_NEQUAL));
#endif
#ifdef ACS_STERLING
    SetDictInt("ACS_STERLING",      (ACS_STERLING));
#endif

    SetDictInt("LINES", LINES);
    SetDictInt("COLS", COLS);
#undef SetDictInt
    return 0;
}

/*[clinic input]
_curses.initscr

Initialize the library.

Return a WindowObject which represents the whole screen.
[clinic start generated code]*/

static PyObject *
_curses_initscr_impl(PyObject *module)
/*[clinic end generated code: output=619fb68443810b7b input=514f4bce1821f6b5]*/
{
    WINDOW *win;

    if (curses_initscr_called) {
        cursesmodule_state *state = get_cursesmodule_state(module);
        int code = wrefresh(stdscr);
        if (code == ERR) {
            _curses_set_null_error(state, "wrefresh", "initscr");
            return NULL;
        }
        PyObject *winobj = PyCursesWindow_New(state, stdscr, NULL, NULL, NULL);
        if (winobj == NULL) {
            return NULL;
        }
        if (curses_update_screen_encoding(winobj) < 0) {
            Py_DECREF(winobj);
            return NULL;
        }
        return winobj;
    }

    win = initscr();

    if (win == NULL) {
        curses_set_null_error(module, "initscr", NULL);
        return NULL;
    }

    curses_initscr_called = curses_setupterm_called = TRUE;

    if (curses_init_dict(module) < 0) {
        return NULL;
    }

    cursesmodule_state *state = get_cursesmodule_state(module);
    PyObject *winobj = PyCursesWindow_New(state, win, NULL, NULL, NULL);
    if (winobj == NULL) {
        return NULL;
    }
    if (curses_update_screen_encoding(winobj) < 0) {
        Py_DECREF(winobj);
        return NULL;
    }
    return winobj;
}

/*[clinic input]
_curses.setupterm

    term: str(accept={str, NoneType}) = None
        Terminal name.
        If omitted, the value of the TERM environment variable will be used.
    fd: int = -1
        File descriptor to which any initialization sequences will be sent.
        If not supplied, the file descriptor for sys.stdout will be used.

Initialize the terminal.
[clinic start generated code]*/

static PyObject *
_curses_setupterm_impl(PyObject *module, const char *term, int fd)
/*[clinic end generated code: output=4584e587350f2848 input=4511472766af0c12]*/
{
    int err;

    if (fd == -1) {
        PyObject* sys_stdout;

        if (PySys_GetOptionalAttrString("stdout", &sys_stdout) < 0) {
            return NULL;
        }

        if (sys_stdout == NULL || sys_stdout == Py_None) {
            cursesmodule_state *state = get_cursesmodule_state(module);
            PyErr_SetString(state->error, "lost sys.stdout");
            Py_XDECREF(sys_stdout);
            return NULL;
        }

        fd = PyObject_AsFileDescriptor(sys_stdout);
        Py_DECREF(sys_stdout);
        if (fd == -1) {
            return NULL;
        }
    }

    if (!curses_setupterm_called && setupterm((char *)term, fd, &err) == ERR) {
        const char* s = "setupterm: unknown error";

        if (err == 0) {
            s = "setupterm: could not find terminal";
        } else if (err == -1) {
            s = "setupterm: could not find terminfo database";
        }

        cursesmodule_state *state = get_cursesmodule_state(module);
        PyErr_SetString(state->error, s);
        return NULL;
    }

    curses_setupterm_called = TRUE;

    Py_RETURN_NONE;
}

static int update_lines_cols(PyObject *private_module);  /* defined below */

/* Return a file descriptor for obj, or, if obj is NULL or None, for the
   sys.<stdname> stream.  Returns -1 with an exception set on error. */
static int
curses_fileno(PyObject *module, PyObject *obj, const char *stdname)
{
    if (obj != NULL && obj != Py_None) {
        return PyObject_AsFileDescriptor(obj);
    }
    PyObject *stream;
    if (PySys_GetOptionalAttrString(stdname, &stream) < 0) {
        return -1;
    }
    if (stream == NULL || stream == Py_None) {
        cursesmodule_state *state = get_cursesmodule_state(module);
        PyErr_Format(state->error, "lost sys.%s", stdname);
        Py_XDECREF(stream);
        return -1;
    }
    int fd = PyObject_AsFileDescriptor(stream);
    Py_DECREF(stream);
    return fd;
}

/* Duplicate fd and wrap it in a new (non-inheritable) stdio stream that the
   screen object will own.  Duplicating means closing the stream later does
   not close the caller's fd.  Returns NULL with an exception set on error. */
static FILE *
curses_fdopen_dup(int fd, const char *mode)
{
    /* _Py_dup() duplicates the descriptor and makes the copy non-inheritable
       atomically (and sets the error on failure). */
    int dfd = _Py_dup(fd);
    if (dfd < 0) {
        return NULL;
    }
    FILE *stream = fdopen(dfd, mode);
    if (stream == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
        close(dfd);
        return NULL;
    }
    return stream;
}

/*[clinic input]
_curses.newterm

    type: str(accept={str, NoneType}) = None
        Terminal name; if None, the TERM environment variable is used.
    fd: object = None
        Output file object or descriptor (default: sys.stdout).
    infd: object = None
        Input file object or descriptor (default: sys.stdin).
    /

Return a new screen for the terminal, in addition to the initial screen.

This is an alternative to initscr() for programs running on more than
one terminal.  Use set_term() to switch between the screens.
[clinic start generated code]*/

static PyObject *
_curses_newterm_impl(PyObject *module, const char *type, PyObject *fd,
                     PyObject *infd)
/*[clinic end generated code: output=62663c31909d796c input=98507fe48c2e93cb]*/
{
    /* Duplicate each descriptor right after resolving it: resolving the other
       one runs arbitrary Python code (e.g. a fileno() method) that could close
       this one before it is duplicated. */
    int out_fd = curses_fileno(module, fd, "stdout");
    if (out_fd < 0) {
        return NULL;
    }
    FILE *outfp = curses_fdopen_dup(out_fd, "wb");
    if (outfp == NULL) {
        return NULL;
    }

    int in_fd = curses_fileno(module, infd, "stdin");
    if (in_fd < 0) {
        fclose(outfp);
        return NULL;
    }
    FILE *infp = curses_fdopen_dup(in_fd, "rb");
    if (infp == NULL) {
        fclose(outfp);
        return NULL;
    }

    SCREEN *screen = newterm((char *)type, outfp, infp);
    if (screen == NULL) {
        curses_set_null_error(module, "newterm", NULL);
        fclose(outfp);
        fclose(infp);
        return NULL;
    }
    /* newterm() makes the new screen the current one, so stdscr now refers
       to its standard window. */
    curses_initscr_called = curses_setupterm_called = TRUE;

    cursesmodule_state *state = get_cursesmodule_state(module);
    /* The screen object owns the SCREEN and the streams; deleting it (when it
       is no longer referenced) calls delscreen() and closes the streams. */
    PyObject *screenobj = PyCursesScreen_New(state, screen, outfp, infp, NULL);
    if (screenobj == NULL) {
        delscreen(screen);
        fclose(outfp);
        fclose(infp);
        return NULL;
    }
    /* The standard window keeps the screen alive for its own lifetime. */
    PyObject *win = PyCursesWindow_New(state, stdscr, NULL, NULL, screenobj);
    if (win == NULL ||
        curses_update_screen_encoding(win) < 0 ||
        curses_init_dict(module) < 0)
    {
        Py_XDECREF(win);
        Py_DECREF(screenobj);
        return NULL;
    }
    ((PyCursesScreenObject *)screenobj)->stdscr = Py_NewRef(win);
    Py_DECREF(win);
    Py_XSETREF(state->topscreen, Py_NewRef(screenobj));
    return screenobj;
}

/* Check that obj is an open screen object; returns it cast, or NULL with
   TypeError/curses.error set. */
static PyCursesScreenObject *
curses_check_screen(PyObject *module, PyObject *obj)
{
    cursesmodule_state *state = get_cursesmodule_state(module);
    if (!PyObject_TypeCheck(obj, state->screen_type)) {
        PyErr_Format(PyExc_TypeError,
                     "expected a curses screen, got %T", obj);
        return NULL;
    }
    PyCursesScreenObject *so = _PyCursesScreenObject_CAST(obj);
    if (so->screen == NULL) {
        PyErr_SetString(state->error, "the screen has been deleted");
        return NULL;
    }
    return so;
}

/*[clinic input]
_curses.set_term

    screen: object
    /

Switch to the given screen and return the previously current screen.

Returns None if the previous screen was the one created by initscr().
[clinic start generated code]*/

static PyObject *
_curses_set_term(PyObject *module, PyObject *screen)
/*[clinic end generated code: output=204cf9c40523bdef input=ed4dba18dd9adf6a]*/
{
    PyCursesScreenObject *so = curses_check_screen(module, screen);
    if (so == NULL) {
        return NULL;
    }
    set_term(so->screen);
    if (!update_lines_cols(module)) {
        return NULL;
    }
    cursesmodule_state *state = get_cursesmodule_state(module);
    PyObject *prev = state->topscreen;          /* steal the owned reference */
    state->topscreen = Py_NewRef(screen);
    return prev != NULL ? prev : Py_NewRef(Py_None);
}

#ifdef HAVE_CURSES_NEW_PRESCR
/*[clinic input]
_curses.new_prescr

Create a screen and return it, without initializing a terminal.

The screen can be used to call functions that affect the screen before
calling newterm() or initscr().
[clinic start generated code]*/

static PyObject *
_curses_new_prescr_impl(PyObject *module)
/*[clinic end generated code: output=e7de5031da7511e2 input=1a3a89d630b641c3]*/
{
    SCREEN *screen = new_prescr();
    if (screen == NULL) {
        curses_set_null_error(module, "new_prescr", NULL);
        return NULL;
    }
    cursesmodule_state *state = get_cursesmodule_state(module);
    return PyCursesScreen_New(state, screen, NULL, NULL, NULL);
}
#endif /* HAVE_CURSES_NEW_PRESCR */

#if defined(NCURSES_EXT_FUNCS) && NCURSES_EXT_FUNCS >= 20081102
// https://invisible-island.net/ncurses/NEWS.html#index-t20080119

/*[clinic input]
_curses.get_escdelay

Gets the curses ESCDELAY setting.

Gets the number of milliseconds to wait after reading an escape
character, to distinguish between an individual escape character entered
on the keyboard from escape sequences sent by cursor and function keys.
[clinic start generated code]*/

static PyObject *
_curses_get_escdelay_impl(PyObject *module)
/*[clinic end generated code: output=222fa1a822555d60 input=b39eeae4b8f169ab]*/
{
    return PyLong_FromLong(ESCDELAY);
}
/*[clinic input]
_curses.set_escdelay
    ms: int
        length of the delay in milliseconds.
    /

Sets the curses ESCDELAY setting.

Sets the number of milliseconds to wait after reading an escape
character, to distinguish between an individual escape character entered
on the keyboard from escape sequences sent by cursor and function keys.
[clinic start generated code]*/

static PyObject *
_curses_set_escdelay_impl(PyObject *module, int ms)
/*[clinic end generated code: output=43818efbf7980ac4 input=cc2529bcdda3b06c]*/
{
    if (ms <= 0) {
        PyErr_SetString(PyExc_ValueError, "ms must be > 0");
        return NULL;
    }

    return curses_check_err(module, set_escdelay(ms), "set_escdelay", NULL);
}

/*[clinic input]
_curses.get_tabsize

Gets the curses TABSIZE setting.

Gets the number of columns used by the curses library when converting
a tab character to spaces as it adds the tab to a window.
[clinic start generated code]*/

static PyObject *
_curses_get_tabsize_impl(PyObject *module)
/*[clinic end generated code: output=7e9e51fb6126fbdf input=58bdaacb337c103b]*/
{
    return PyLong_FromLong(TABSIZE);
}
/*[clinic input]
_curses.set_tabsize
    size: int
        rendered cell width of a tab character.
    /

Sets the curses TABSIZE setting.

Sets the number of columns used by the curses library when converting
a tab character to spaces as it adds the tab to a window.
[clinic start generated code]*/

static PyObject *
_curses_set_tabsize_impl(PyObject *module, int size)
/*[clinic end generated code: output=c1de5a76c0daab1e input=34c1be9a78cd28a2]*/
{
    if (size <= 0) {
        PyErr_SetString(PyExc_ValueError, "size must be > 0");
        return NULL;
    }

    return curses_check_err(module, set_tabsize(size), "set_tabsize", NULL);
}
#endif

/*[clinic input]
_curses.intrflush

    flag: bool
    /

Control flushing of the output buffer when an interrupt key is pressed.

If flag is true, pressing an interrupt key (interrupt, break, or quit)
flushes all output in the terminal driver queue.  If flag is false, no
flushing is done.
[clinic start generated code]*/

static PyObject *
_curses_intrflush_impl(PyObject *module, int flag)
/*[clinic end generated code: output=c1986df35e999a0f input=66588c2bccc7e8fa]*/
{
    PyCursesStatefulInitialised(module);

    return curses_check_err(module, intrflush(NULL, flag), "intrflush", NULL);
}

/*[clinic input]
_curses.isendwin

Return True if endwin() has been called.
[clinic start generated code]*/

static PyObject *
_curses_isendwin_impl(PyObject *module)
/*[clinic end generated code: output=d73179e4a7e1eb8c input=6cdb01a7ebf71397]*/
NoArgTrueFalseFunctionBody(isendwin)

#ifdef HAVE_CURSES_IS_TERM_RESIZED
/*[clinic input]
@permit_long_summary
_curses.is_term_resized

    nlines: int
        Height.
    ncols: int
        Width.
    /

Return True if resize_term() would modify the window structure, False otherwise.
[clinic start generated code]*/

static PyObject *
_curses_is_term_resized_impl(PyObject *module, int nlines, int ncols)
/*[clinic end generated code: output=aafe04afe50f1288 input=5792a3f40cecb010]*/
{
    PyCursesStatefulInitialised(module);

    return PyBool_FromLong(is_term_resized(nlines, ncols));
}
#endif /* HAVE_CURSES_IS_TERM_RESIZED */

/*[clinic input]
_curses.keyname

    key: int
        Key number.
    /

Return the name of specified key.
[clinic start generated code]*/

static PyObject *
_curses_keyname_impl(PyObject *module, int key)
/*[clinic end generated code: output=fa2675ab3f4e056b input=ee4b1d0f243a2a2b]*/
{
    const char *knp;

    PyCursesStatefulInitialised(module);

    if (key < 0) {
        PyErr_SetString(PyExc_ValueError, "invalid key number");
        return NULL;
    }
    knp = keyname(key);

    return PyBytes_FromString((knp == NULL) ? "" : knp);
}

/*[clinic input]
_curses.killchar

Return the user's current line kill character.
[clinic start generated code]*/

static PyObject *
_curses_killchar_impl(PyObject *module)
/*[clinic end generated code: output=31c3a45b2c528269 input=1ff171c38df5ccad]*/
{
    char ch;

    ch = killchar();

    return PyBytes_FromStringAndSize(&ch, 1);
}

#ifdef HAVE_NCURSESW
/*[clinic input]
_curses.killwchar

Return the user's current wide-character line kill character.
[clinic start generated code]*/

static PyObject *
_curses_killwchar_impl(PyObject *module)
/*[clinic end generated code: output=eac1fd72a0c88d42 input=5c2d7d1ab2f24eb7]*/
{
    wchar_t ch;

    if (killwchar(&ch) == ERR) {
        curses_set_error(module, "killwchar", NULL);
        return NULL;
    }
    return PyUnicode_FromWideChar(&ch, 1);
}
#endif /* HAVE_NCURSESW */

/*[clinic input]
_curses.longname

Return the terminfo long name field describing the current terminal.

The maximum length of a verbose description is 128 characters.  It is
defined only after the call to initscr().
[clinic start generated code]*/

static PyObject *
_curses_longname_impl(PyObject *module)
/*[clinic end generated code: output=fdf30433727ef568 input=a924fabba0de78a6]*/
NoArgReturnStringFunctionBody(longname)

/*[clinic input]
_curses.meta

    yes: bool
    /

Enable/disable meta keys.

If yes is True, allow 8-bit characters to be input.  If yes is False,
allow only 7-bit characters.
[clinic start generated code]*/

static PyObject *
_curses_meta_impl(PyObject *module, int yes)
/*[clinic end generated code: output=22f5abda46a605d8 input=cfe7da79f51d0e30]*/
{
    PyCursesStatefulInitialised(module);

    return curses_check_err(module, meta(stdscr, yes), "meta", NULL);
}

#ifdef NCURSES_MOUSE_VERSION
/*[clinic input]
_curses.mouseinterval

    interval: int
        Time in milliseconds.
    /

Set and retrieve the maximum time between press and release in a click.

Set the maximum time that can elapse between press and release events in
order for them to be recognized as a click, and return the previous
interval value.
[clinic start generated code]*/

static PyObject *
_curses_mouseinterval_impl(PyObject *module, int interval)
/*[clinic end generated code: output=c4f5ff04354634c5 input=b90249254389c080]*/
{
    PyCursesStatefulInitialised(module);
    int value = mouseinterval(interval);
    if (value == ERR) {
        curses_set_error(module, "mouseinterval", NULL);
        return NULL;
    }
    return PyLong_FromLong(value);
}

/*[clinic input]
_curses.mousemask

    newmask: unsigned_long(bitwise=True)
    /

Set the mouse events to be reported, and return (availmask, oldmask).

Return a tuple (availmask, oldmask).  availmask indicates which of the
specified mouse events can be reported; on complete failure it returns
0.  oldmask is the previous value of the mouse event mask.  If this
function is never called, no mouse events are ever reported.
[clinic start generated code]*/

static PyObject *
_curses_mousemask_impl(PyObject *module, unsigned long newmask)
/*[clinic end generated code: output=9406cf1b8a36e485 input=b8a9a4ccbce633f4]*/
{
    mmask_t oldmask, availmask;

    PyCursesStatefulInitialised(module);
    availmask = mousemask((mmask_t)newmask, &oldmask);
    return Py_BuildValue("(kk)",
                         (unsigned long)availmask, (unsigned long)oldmask);
}
#endif

/*[clinic input]
_curses.napms -> int

    ms: int
        Duration in milliseconds.
    /

Sleep for specified time.
[clinic start generated code]*/

static int
_curses_napms_impl(PyObject *module, int ms)
/*[clinic end generated code: output=5f292a6a724491bd input=c6d6e01f2f1df9f7]*/
{
    if (!_PyCursesStatefulCheckFunction(module,
                                        curses_initscr_called,
                                        "initscr")) {
        return -1;
    }
    return napms(ms);
}


/*[clinic input]
_curses.newpad

    nlines: int
        Height.
    ncols: int
        Width.
    /

Create and return a pointer to a new pad data structure.
[clinic start generated code]*/

static PyObject *
_curses_newpad_impl(PyObject *module, int nlines, int ncols)
/*[clinic end generated code: output=de52a56eb1098ec9 input=93f1272f240d8894]*/
{
    WINDOW *win;

    PyCursesStatefulInitialised(module);

    win = newpad(nlines, ncols);

    if (win == NULL) {
        curses_set_null_error(module, "newpad", NULL);
        return NULL;
    }

    cursesmodule_state *state = get_cursesmodule_state(module);
    return PyCursesWindow_New(state, win, NULL, NULL, state->topscreen);
}

/*[clinic input]
_curses.newwin

    nlines: int
        Height.
    ncols: int
        Width.
    [
    begin_y: int = 0
        Top side y-coordinate.
    begin_x: int = 0
        Left side x-coordinate.
    ]
    /

Return a new window.

By default, the window will extend from the specified position to the
lower right corner of the screen.
[clinic start generated code]*/

static PyObject *
_curses_newwin_impl(PyObject *module, int nlines, int ncols,
                    int group_right_1, int begin_y, int begin_x)
/*[clinic end generated code: output=c1e0a8dc8ac2826c input=a1517cbfea4ab24b]*/
{
    WINDOW *win;

    PyCursesStatefulInitialised(module);

    win = newwin(nlines,ncols,begin_y,begin_x);
    if (win == NULL) {
        curses_set_null_error(module, "newwin", NULL);
        return NULL;
    }

    cursesmodule_state *state = get_cursesmodule_state(module);
    return PyCursesWindow_New(state, win, NULL, NULL, state->topscreen);
}

/*[clinic input]
_curses.nl

    flag: bool = True
        If false, the effect is the same as calling nonl().
    /

Enter newline mode.

This mode translates the return key into newline on input, and
translates newline into return and line-feed on output.  Newline mode
is initially on.
[clinic start generated code]*/

static PyObject *
_curses_nl_impl(PyObject *module, int flag)
/*[clinic end generated code: output=b39cc0ffc9015003 input=3fb21dcf55521ee4]*/
NoArgOrFlagNoReturnFunctionBody(nl, flag)

/*[clinic input]
_curses.nocbreak

Leave cbreak mode.

Return to normal "cooked" mode with line buffering.
[clinic start generated code]*/

static PyObject *
_curses_nocbreak_impl(PyObject *module)
/*[clinic end generated code: output=eabf3833a4fbf620 input=e4b65f7d734af400]*/
NoArgNoReturnFunctionBody(nocbreak)

/*[clinic input]
_curses.noecho

Leave echo mode.

Echoing of input characters is turned off.
[clinic start generated code]*/

static PyObject *
_curses_noecho_impl(PyObject *module)
/*[clinic end generated code: output=cc95ab45bc98f41b input=76714df529e614c3]*/
NoArgNoReturnFunctionBody(noecho)

/*[clinic input]
_curses.nonl

Leave newline mode.

Disable translation of return into newline on input, and disable
low-level translation of newline into newline/return on output.
[clinic start generated code]*/

static PyObject *
_curses_nonl_impl(PyObject *module)
/*[clinic end generated code: output=99e917e9715770c6 input=75cce08e4b6b3ef1]*/
NoArgNoReturnFunctionBody(nonl)

/*[clinic input]
_curses.noqiflush

Disable queue flushing.

When queue flushing is disabled, normal flush of input and output queues
associated with the INTR, QUIT and SUSP characters will not be done.
[clinic start generated code]*/

static PyObject *
_curses_noqiflush_impl(PyObject *module)
/*[clinic end generated code: output=8b95a4229bbf0877 input=ba3e6b2e3e54c4df]*/
NoArgNoReturnVoidFunctionBody(noqiflush)

/*[clinic input]
_curses.noraw

Leave raw mode.

Return to normal "cooked" mode with line buffering.
[clinic start generated code]*/

static PyObject *
_curses_noraw_impl(PyObject *module)
/*[clinic end generated code: output=39894e5524c430cc input=6ec86692096dffb5]*/
NoArgNoReturnFunctionBody(noraw)

/*[clinic input]
@permit_long_summary
_curses.pair_content

    pair_number: pair
        The number of the color pair (0 - (COLOR_PAIRS-1)).
    /

Return a tuple (fg, bg) containing the colors for the requested color pair.
[clinic start generated code]*/

static PyObject *
_curses_pair_content_impl(PyObject *module, int pair_number)
/*[clinic end generated code: output=4a726dd0e6885f3f input=faede9e26f1f2ca4]*/
{
    _CURSES_COLOR_NUM_TYPE f, b;

    PyCursesStatefulInitialised(module);
    PyCursesStatefulInitialisedColor(module);

    if (_CURSES_PAIR_CONTENT_FUNC(pair_number, &f, &b) == ERR) {
        if (pair_number >= COLOR_PAIRS) {
            PyErr_Format(PyExc_ValueError,
                         "Color pair is greater than COLOR_PAIRS-1 (%d).",
                         COLOR_PAIRS - 1);
        }
        else {
            const char *funcname = Py_STRINGIFY(_CURSES_PAIR_CONTENT_FUNC);
            curses_set_error(module, funcname, "pair_content");
        }
        return NULL;
    }

    return Py_BuildValue("(ii)", f, b);
}

/*[clinic input]
@permit_long_summary
_curses.pair_number

    attr: int
    /

Return the number of the color-pair set by the specified attribute value.

color_pair() is the counterpart to this function.
[clinic start generated code]*/

static PyObject *
_curses_pair_number_impl(PyObject *module, int attr)
/*[clinic end generated code: output=85bce7d65c0aa3f4 input=b11152a78c2f9abf]*/
{
    PyCursesStatefulInitialised(module);
    PyCursesStatefulInitialisedColor(module);

    return PyLong_FromLong(PAIR_NUMBER(attr));
}

/*[clinic input]
@permit_long_summary
_curses.putp

    string: str(accept={robuffer})
    /

Emit the value of a specified terminfo capability for the current terminal.

Note that the output of putp() always goes to standard output.
[clinic start generated code]*/

static PyObject *
_curses_putp_impl(PyObject *module, const char *string)
/*[clinic end generated code: output=e98081d1b8eb5816 input=2f3b9e0f22829ee7]*/
{
    return curses_check_err(module, putp(string), "putp", NULL);
}

/*[clinic input]
_curses.qiflush

    flag: bool = True
        If false, the effect is the same as calling noqiflush().
    /

Enable queue flushing.

If queue flushing is enabled, all output in the display driver queue
will be flushed when the INTR, QUIT and SUSP characters are read.
[clinic start generated code]*/

static PyObject *
_curses_qiflush_impl(PyObject *module, int flag)
/*[clinic end generated code: output=9167e862f760ea30 input=6ec8b3e2b717ec40]*/
{
    PyCursesStatefulInitialised(module);

    if (flag) {
        qiflush();
    }
    else {
        noqiflush();
    }
    Py_RETURN_NONE;
}

#if defined(HAVE_CURSES_RESIZETERM) || defined(HAVE_CURSES_RESIZE_TERM)
/* Internal helper used for updating curses.LINES, curses.COLS, _curses.LINES
 * and _curses.COLS. Returns 1 on success and 0 on failure. */
static int
update_lines_cols(PyObject *private_module)
{
    PyObject *exposed_module = NULL, *o = NULL;

    exposed_module = PyImport_ImportModule("curses");
    if (exposed_module == NULL) {
        goto error;
    }
    PyObject *exposed_module_dict = PyModule_GetDict(exposed_module); // borrowed
    if (exposed_module_dict == NULL) {
        goto error;
    }
    PyObject *private_module_dict = PyModule_GetDict(private_module); // borrowed
    if (private_module_dict == NULL) {
        goto error;
    }

    o = PyLong_FromLong(LINES);
    if (o == NULL) {
        goto error;
    }
    if (PyDict_SetItemString(exposed_module_dict, "LINES", o) < 0) {
        goto error;
    }
    if (PyDict_SetItemString(private_module_dict, "LINES", o) < 0) {
        goto error;
    }
    Py_DECREF(o);

    o = PyLong_FromLong(COLS);
    if (o == NULL) {
        goto error;
    }
    if (PyDict_SetItemString(exposed_module_dict, "COLS", o) < 0) {
        goto error;
    }
    if (PyDict_SetItemString(private_module_dict, "COLS", o) < 0) {
        goto error;
    }
    Py_DECREF(o);
    Py_DECREF(exposed_module);
    return 1;

error:
    Py_XDECREF(o);
    Py_XDECREF(exposed_module);
    return 0;
}

/*[clinic input]
_curses.update_lines_cols

Update the LINES and COLS module variables.

This is useful for detecting manual screen resize.
[clinic start generated code]*/

static PyObject *
_curses_update_lines_cols_impl(PyObject *module)
/*[clinic end generated code: output=423f2b1e63ed0f75 input=1d8ea7c356b61a8b]*/
{
    if (!update_lines_cols(module)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

#endif

/*[clinic input]
_curses.raw

    flag: bool = True
        If false, the effect is the same as calling noraw().
    /

Enter raw mode.

In raw mode, normal line buffering and processing of interrupt, quit,
suspend, and flow control keys are turned off; characters are presented
to curses input functions one by one.
[clinic start generated code]*/

static PyObject *
_curses_raw_impl(PyObject *module, int flag)
/*[clinic end generated code: output=a750e4b342be015b input=18a7de7eef16987a]*/
NoArgOrFlagNoReturnFunctionBody(raw, flag)

/*[clinic input]
@permit_long_summary
_curses.reset_prog_mode

Restore the terminal to "program" mode, as previously saved by def_prog_mode().
[clinic start generated code]*/

static PyObject *
_curses_reset_prog_mode_impl(PyObject *module)
/*[clinic end generated code: output=15eb765abf0b6575 input=a8b44b5261c8cf3a]*/
NoArgNoReturnFunctionBody(reset_prog_mode)

/*[clinic input]
@permit_long_summary
_curses.reset_shell_mode

Restore the terminal to "shell" mode, as previously saved by def_shell_mode().
[clinic start generated code]*/

static PyObject *
_curses_reset_shell_mode_impl(PyObject *module)
/*[clinic end generated code: output=0238de2962090d33 input=f5224034a2c95931]*/
NoArgNoReturnFunctionBody(reset_shell_mode)

/*[clinic input]
_curses.resetty

Restore terminal mode.
[clinic start generated code]*/

static PyObject *
_curses_resetty_impl(PyObject *module)
/*[clinic end generated code: output=ff4b448e80a7cd63 input=940493de03624bb0]*/
NoArgNoReturnFunctionBody(resetty)

#ifdef HAVE_CURSES_RESIZETERM
/*[clinic input]
_curses.resizeterm

    nlines: short
        Height.
    ncols: short
        Width.
    /

Resize the standard and current windows to the specified dimensions.

Adjusts other bookkeeping data used by the curses library that record
the window dimensions (in particular the SIGWINCH handler).
[clinic start generated code]*/

static PyObject *
_curses_resizeterm_impl(PyObject *module, short nlines, short ncols)
/*[clinic end generated code: output=4de3abab50c67f02 input=7f0f077df2da1cf5]*/
{
    PyObject *result;
    int code;

    PyCursesStatefulInitialised(module);

    code = resizeterm(nlines, ncols);
    result = curses_check_err(module, code, "resizeterm", NULL);
    if (!result)
        return NULL;
    if (!update_lines_cols(module)) {
        Py_DECREF(result);
        return NULL;
    }
    return result;
}

#endif

#ifdef HAVE_CURSES_RESIZE_TERM
/*[clinic input]
_curses.resize_term

    nlines: short
        Height.
    ncols: short
        Width.
    /

Backend function used by resizeterm(), performing most of the work.

When resizing the windows, resize_term() blank-fills the areas that are
extended.  The calling application should fill in these areas with
appropriate data.  The resize_term() function attempts to resize all
windows.  However, due to the calling convention of pads, it is not
possible to resize these without additional interaction with the
application.
[clinic start generated code]*/

static PyObject *
_curses_resize_term_impl(PyObject *module, short nlines, short ncols)
/*[clinic end generated code: output=46c6d749fa291dbd input=ff4baaf2320c8ac9]*/
{
    PyObject *result;
    int code;

    PyCursesStatefulInitialised(module);

    code = resize_term(nlines, ncols);
    result = curses_check_err(module, code, "resize_term", NULL);
    if (!result)
        return NULL;
    if (!update_lines_cols(module)) {
        Py_DECREF(result);
        return NULL;
    }
    return result;
}
#endif /* HAVE_CURSES_RESIZE_TERM */

/*[clinic input]
_curses.savetty

Save terminal mode.
[clinic start generated code]*/

static PyObject *
_curses_savetty_impl(PyObject *module)
/*[clinic end generated code: output=6babc49f12b42199 input=fce6b2b7d2200102]*/
NoArgNoReturnFunctionBody(savetty)

#ifdef getsyx
/*[clinic input]
_curses.setsyx

    y: int
        Y-coordinate.
    x: int
        X-coordinate.
    /

Set the virtual screen cursor.

If y and x are both -1, then leaveok is set.
[clinic start generated code]*/

static PyObject *
_curses_setsyx_impl(PyObject *module, int y, int x)
/*[clinic end generated code: output=23dcf753511a2464 input=fa7f2b208e10a557]*/
{
    PyCursesStatefulInitialised(module);

    setsyx(y,x);

    Py_RETURN_NONE;
}
#endif

/*[clinic input]
@permit_long_summary
_curses.start_color

Initializes eight basic colors and global variables COLORS and COLOR_PAIRS.

Must be called if the programmer wants to use colors, and before any
other color manipulation routine is called.  It is good practice to call
this routine right after initscr().

It also restores the colors on the terminal to the values they had when
the terminal was just turned on.
[clinic start generated code]*/

static PyObject *
_curses_start_color_impl(PyObject *module)
/*[clinic end generated code: output=8b772b41d8090ede input=7daacc6b6baba643]*/
{
    PyCursesStatefulInitialised(module);

    if (start_color() == ERR) {
        curses_set_error(module, "start_color", NULL);
        return NULL;
    }

    curses_start_color_called = TRUE;

    PyObject *module_dict = PyModule_GetDict(module); // borrowed
    if (module_dict == NULL) {
        return NULL;
    }
#define DICT_ADD_INT_VALUE(NAME, VALUE)                             \
    do {                                                            \
        PyObject *value = PyLong_FromLong((long)(VALUE));           \
        if (value == NULL) {                                        \
            return NULL;                                            \
        }                                                           \
        int rc = PyDict_SetItemString(module_dict, (NAME), value);  \
        Py_DECREF(value);                                           \
        if (rc < 0) {                                               \
            return NULL;                                            \
        }                                                           \
    } while (0)

    DICT_ADD_INT_VALUE("COLORS", COLORS);
    DICT_ADD_INT_VALUE("COLOR_PAIRS", COLOR_PAIRS);
#undef DICT_ADD_INT_VALUE

    Py_RETURN_NONE;
}

/*[clinic input]
_curses.termattrs

Return a logical OR of all video attributes supported by the terminal.
[clinic start generated code]*/

static PyObject *
_curses_termattrs_impl(PyObject *module)
/*[clinic end generated code: output=b06f437fce1b6fc4 input=0559882a04f84d1d]*/
NoArgReturnIntFunctionBody(termattrs)

/*[clinic input]
@permit_long_summary
_curses.termname

Return the value of the environment variable TERM, truncated to 14 characters.
[clinic start generated code]*/

static PyObject *
_curses_termname_impl(PyObject *module)
/*[clinic end generated code: output=96375577ebbd67fd input=c34f724d8ce8fc4e]*/
NoArgReturnStringFunctionBody(termname)

/*[clinic input]
_curses.tigetflag

    capname: str
        The terminfo capability name.
    /

Return the value of the Boolean capability.

The value -1 is returned if capname is not a Boolean capability, or 0 if
it is canceled or absent from the terminal description.
[clinic start generated code]*/

static PyObject *
_curses_tigetflag_impl(PyObject *module, const char *capname)
/*[clinic end generated code: output=8853c0e55542195b input=b0787af9e3e9a6ce]*/
{
    PyCursesStatefulSetupTermCalled(module);

    return PyLong_FromLong( (long) tigetflag( (char *)capname ) );
}

/*[clinic input]
_curses.tigetnum

    capname: str
        The terminfo capability name.
    /

Return the value of the numeric capability.

The value -2 is returned if capname is not a numeric capability, or -1
if it is canceled or absent from the terminal description.
[clinic start generated code]*/

static PyObject *
_curses_tigetnum_impl(PyObject *module, const char *capname)
/*[clinic end generated code: output=46f8b0a1b5dff42f input=87a64beec16ae077]*/
{
    PyCursesStatefulSetupTermCalled(module);

    return PyLong_FromLong( (long) tigetnum( (char *)capname ) );
}

/*[clinic input]
_curses.tigetstr

    capname: str
        The terminfo capability name.
    /

Return the value of the string capability.

None is returned if capname is not a string capability, or is canceled
or absent from the terminal description.
[clinic start generated code]*/

static PyObject *
_curses_tigetstr_impl(PyObject *module, const char *capname)
/*[clinic end generated code: output=f22b576ad60248f3 input=00bf0feda2207724]*/
{
    PyCursesStatefulSetupTermCalled(module);

    capname = tigetstr( (char *)capname );
    if (capname == NULL || capname == (char*) -1) {
        Py_RETURN_NONE;
    }
    return PyBytes_FromString( capname );
}

/*[clinic input]
_curses.tparm

    str: str(accept={robuffer})
        Parameterized byte string obtained from the terminfo database.
    i1: int = 0
    i2: int = 0
    i3: int = 0
    i4: int = 0
    i5: int = 0
    i6: int = 0
    i7: int = 0
    i8: int = 0
    i9: int = 0
    /

Instantiate the specified byte string with the supplied parameters.
[clinic start generated code]*/

static PyObject *
_curses_tparm_impl(PyObject *module, const char *str, int i1, int i2, int i3,
                   int i4, int i5, int i6, int i7, int i8, int i9)
/*[clinic end generated code: output=599f62b615c667ff input=5e30b15786f032aa]*/
{
    char* result = NULL;

    PyCursesStatefulSetupTermCalled(module);

    result = tparm((char *)str,i1,i2,i3,i4,i5,i6,i7,i8,i9);
    if (!result) {
        curses_set_null_error(module, "tparm", NULL);
        return NULL;
    }

    return PyBytes_FromString(result);
}

#ifdef HAVE_CURSES_TYPEAHEAD
/*[clinic input]
_curses.typeahead

    fd: int
        File descriptor.
    /

Specify that the file descriptor fd be used for typeahead checking.

If fd is -1, then no typeahead checking is done.
[clinic start generated code]*/

static PyObject *
_curses_typeahead_impl(PyObject *module, int fd)
/*[clinic end generated code: output=084bb649d7066583 input=f2968d8e1805051b]*/
{
    PyCursesStatefulInitialised(module);

    return curses_check_err(module, typeahead(fd), "typeahead", NULL);
}
#endif

/*[clinic input]
_curses.unctrl

    ch: object
    /

Return a string which is a printable representation of the character ch.

Control characters are displayed as a caret followed by the character,
for example as ^C.  Printing characters are left as they are.
[clinic start generated code]*/

static PyObject *
_curses_unctrl(PyObject *module, PyObject *ch)
/*[clinic end generated code: output=8e07fafc430c9434 input=cd1e35e16cd1ace4]*/
{
    chtype ch_;

    PyCursesStatefulInitialised(module);

    if (!PyCurses_ConvertToChtype(NULL, ch, &ch_))
        return NULL;

    const char *res = unctrl(ch_);
    if (res == NULL) {
        curses_set_null_error(module, "unctrl", NULL);
        return NULL;
    }
    return PyBytes_FromString(res);
}

#ifdef HAVE_NCURSESW
/*[clinic input]
_curses.wunctrl

    ch: object
    /

Return a printable representation of the wide character ch.

Control characters are displayed as a caret followed by the character,
for example as ^C.  Printing characters are left as they are.
[clinic start generated code]*/

static PyObject *
_curses_wunctrl(PyObject *module, PyObject *ch)
/*[clinic end generated code: output=7b16d5534ff05728 input=9ceb6749118bd07c]*/
{
    chtype ch_;
    wchar_t wstr[CCHARW_MAX + 1];
    cchar_t wcval;

    PyCursesStatefulInitialised(module);

    int type = PyCurses_ConvertToCchar_t(NULL, ch, &ch_, wstr);
    if (type == 0) {
        return NULL;
    }
    if (type == 1) {
        /* A narrow character is the spacing character of the cell. */
        wstr[0] = (wchar_t)(ch_ & A_CHARTEXT);
        wstr[1] = L'\0';
    }
    if (setcchar(&wcval, wstr, A_NORMAL, 0, NULL) == ERR) {
        curses_set_error(module, "setcchar", "wunctrl");
        return NULL;
    }

    wchar_t *res = wunctrl(&wcval);
    if (res == NULL) {
        curses_set_null_error(module, "wunctrl", NULL);
        return NULL;
    }
    return PyUnicode_FromWideChar(res, -1);
}
#endif /* HAVE_NCURSESW */

/*[clinic input]
_curses.ungetch

    ch: object
    /

Push ch so the next getch() will return it.
[clinic start generated code]*/

static PyObject *
_curses_ungetch(PyObject *module, PyObject *ch)
/*[clinic end generated code: output=9b19d8268376d887 input=6681e6ae4c42e5eb]*/
{
    chtype ch_;

    PyCursesStatefulInitialised(module);

    if (!PyCurses_ConvertToChtype(NULL, ch, &ch_))
        return NULL;

    return curses_check_err(module, ungetch(ch_), "ungetch", NULL);
}

#ifdef HAVE_NCURSESW
/* Convert an object to a character (wchar_t):

    - int
    - str of length 1

   Return 1 on success, 0 on error. */
static int
PyCurses_ConvertToWchar_t(PyObject *obj,
                          wchar_t *wch)
{
    if (PyUnicode_Check(obj)) {
        wchar_t buffer[2];
        if (PyUnicode_AsWideChar(obj, buffer, 2) != 1) {
            PyErr_Format(PyExc_TypeError,
                         "expect int or str of length 1, "
                         "got a str of length %zi",
                         PyUnicode_GET_LENGTH(obj));
            return 0;
        }
        *wch = buffer[0];
        return 2;
    }
    else if (PyLong_CheckExact(obj)) {
        long value;
        int overflow;
        value = PyLong_AsLongAndOverflow(obj, &overflow);
        if (overflow) {
            PyErr_SetString(PyExc_OverflowError,
                            "int doesn't fit in long");
            return 0;
        }
        *wch = (wchar_t)value;
        if ((long)*wch != value) {
            PyErr_Format(PyExc_OverflowError,
                         "character doesn't fit in wchar_t");
            return 0;
        }
        return 1;
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "expect int or str of length 1, got %s",
                     Py_TYPE(obj)->tp_name);
        return 0;
    }
}

/*[clinic input]
_curses.unget_wch

    ch: object
    /

Push ch so the next get_wch() will return it.
[clinic start generated code]*/

static PyObject *
_curses_unget_wch(PyObject *module, PyObject *ch)
/*[clinic end generated code: output=1974c9fb01d37863 input=0d56dc65a46feebb]*/
{
    wchar_t wch;

    PyCursesStatefulInitialised(module);

    if (!PyCurses_ConvertToWchar_t(ch, &wch))
        return NULL;
    return curses_check_err(module, unget_wch(wch), "unget_wch", NULL);
}
#endif

#ifdef HAVE_CURSES_USE_ENV
/*[clinic input]
_curses.use_env

    flag: bool
    /

Use environment variables LINES and COLUMNS.

If used, this function should be called before initscr() or newterm()
are called.

When flag is False, the values of lines and columns specified in the
terminfo database will be used, even if environment variables LINES and
COLUMNS (used by default) are set, or if curses is running in a window
(in which case default behavior would be to use the window size if LINES
and COLUMNS are not set).
[clinic start generated code]*/

static PyObject *
_curses_use_env_impl(PyObject *module, int flag)
/*[clinic end generated code: output=b2c445e435c0b164 input=8e8feed746cf7fc1]*/
{
    use_env(flag);
    Py_RETURN_NONE;
}
#endif

#ifndef STRICT_SYSV_CURSES
/*[clinic input]
_curses.use_default_colors

Equivalent to assume_default_colors(-1, -1).
[clinic start generated code]*/

static PyObject *
_curses_use_default_colors_impl(PyObject *module)
/*[clinic end generated code: output=a3b81ff71dd901be input=99ff0b7c69834d1f]*/
{
    int code;

    PyCursesStatefulInitialised(module);
    PyCursesStatefulInitialisedColor(module);

    code = use_default_colors();
    return curses_check_err(module, code, "use_default_colors", NULL);
}

/*[clinic input]
@permit_long_summary
_curses.assume_default_colors
    fg: int
    bg: int
    /

Allow use of default values for colors on terminals supporting this feature.

Assign terminal default foreground/background colors to color number -1.
Change the definition of the color-pair 0 to (fg, bg).

Use this to support transparency in your application.
[clinic start generated code]*/

static PyObject *
_curses_assume_default_colors_impl(PyObject *module, int fg, int bg)
/*[clinic end generated code: output=54985397a7d2b3a5 input=8945333c09893cf2]*/
{
    int code;

    PyCursesStatefulInitialised(module);
    PyCursesStatefulInitialisedColor(module);

    code = assume_default_colors(fg, bg);
    return curses_check_err(module, code, "assume_default_colors", NULL);
}
#endif /* STRICT_SYSV_CURSES */


#ifdef NCURSES_VERSION

PyDoc_STRVAR(ncurses_version__doc__,
"curses.ncurses_version\n\
\n\
Ncurses version information as a named tuple.");

static PyStructSequence_Field ncurses_version_fields[] = {
    {"major", "Major release number"},
    {"minor", "Minor release number"},
    {"patch", "Patch release number"},
    {0}
};

static PyStructSequence_Desc ncurses_version_desc = {
    "curses.ncurses_version",  /* name */
    ncurses_version__doc__,    /* doc */
    ncurses_version_fields,    /* fields */
    3
};

static PyObject *
make_ncurses_version(PyTypeObject *type)
{
    PyObject *ncurses_version = PyStructSequence_New(type);
    if (ncurses_version == NULL) {
        return NULL;
    }
    const char *str = curses_version();
    unsigned long major = 0, minor = 0, patch = 0;
    if (!str || sscanf(str, "%*[^0-9]%lu.%lu.%lu", &major, &minor, &patch) < 3) {
        // Fallback to header version, which cannot be that wrong
        major = NCURSES_VERSION_MAJOR;
        minor = NCURSES_VERSION_MINOR;
        patch = NCURSES_VERSION_PATCH;
    }
#define SET_VERSION_COMPONENT(INDEX, VALUE)                     \
    do {                                                        \
        PyObject *o = PyLong_FromLong(VALUE);                   \
        if (o == NULL) {                                        \
            Py_DECREF(ncurses_version);                         \
            return NULL;                                        \
        }                                                       \
        PyStructSequence_SET_ITEM(ncurses_version, INDEX, o);   \
    } while (0)

    SET_VERSION_COMPONENT(0, major);
    SET_VERSION_COMPONENT(1, minor);
    SET_VERSION_COMPONENT(2, patch);
#undef SET_VERSION_COMPONENT
    return ncurses_version;
}

#endif /* NCURSES_VERSION */

/*[clinic input]
@permit_long_summary
_curses.has_extended_color_support

Return True if the module supports extended colors; otherwise, return False.

Extended color support allows more than 256 color-pairs for terminals
that support more than 16 colors (e.g. xterm-256color).
[clinic start generated code]*/

static PyObject *
_curses_has_extended_color_support_impl(PyObject *module)
/*[clinic end generated code: output=68f1be2b57d92e22 input=40d673471c5056f0]*/
{
    return PyBool_FromLong(_NCURSES_EXTENDED_COLOR_FUNCS);
}

/* List of functions defined in the module */

static PyMethodDef cursesmodule_methods[] = {
    _CURSES_ALLOC_PAIR_METHODDEF
    _CURSES_BAUDRATE_METHODDEF
    _CURSES_BEEP_METHODDEF
    _CURSES_CAN_CHANGE_COLOR_METHODDEF
    _CURSES_CBREAK_METHODDEF
    _CURSES_COLOR_CONTENT_METHODDEF
    _CURSES_COLOR_PAIR_METHODDEF
    _CURSES_CURS_SET_METHODDEF
    _CURSES_DEF_PROG_MODE_METHODDEF
    _CURSES_DEF_SHELL_MODE_METHODDEF
    _CURSES_DELAY_OUTPUT_METHODDEF
    _CURSES_DOUPDATE_METHODDEF
    _CURSES_ECHO_METHODDEF
    _CURSES_ENDWIN_METHODDEF
    _CURSES_ERASECHAR_METHODDEF
    _CURSES_ERASEWCHAR_METHODDEF
    _CURSES_FILTER_METHODDEF
    _CURSES_NOFILTER_METHODDEF
    _CURSES_FIND_PAIR_METHODDEF
    _CURSES_FLASH_METHODDEF
    _CURSES_FLUSHINP_METHODDEF
    _CURSES_FREE_PAIR_METHODDEF
    _CURSES_GETMOUSE_METHODDEF
    _CURSES_UNGETMOUSE_METHODDEF
    _CURSES_GETSYX_METHODDEF
    _CURSES_GETWIN_METHODDEF
    _CURSES_HAS_COLORS_METHODDEF
    _CURSES_HAS_EXTENDED_COLOR_SUPPORT_METHODDEF
    _CURSES_HAS_IC_METHODDEF
    _CURSES_HAS_IL_METHODDEF
    _CURSES_HAS_KEY_METHODDEF
    _CURSES_HALFDELAY_METHODDEF
    _CURSES_INIT_COLOR_METHODDEF
    _CURSES_INIT_PAIR_METHODDEF
    _CURSES_INITSCR_METHODDEF
    _CURSES_INTRFLUSH_METHODDEF
    _CURSES_IS_CBREAK_METHODDEF
    _CURSES_IS_ECHO_METHODDEF
    _CURSES_IS_NL_METHODDEF
    _CURSES_IS_RAW_METHODDEF
    _CURSES_ISENDWIN_METHODDEF
    _CURSES_IS_TERM_RESIZED_METHODDEF
    _CURSES_KEYNAME_METHODDEF
    _CURSES_KILLCHAR_METHODDEF
    _CURSES_KILLWCHAR_METHODDEF
    _CURSES_LONGNAME_METHODDEF
    _CURSES_META_METHODDEF
    _CURSES_MOUSEINTERVAL_METHODDEF
    _CURSES_MOUSEMASK_METHODDEF
    _CURSES_NAPMS_METHODDEF
    _CURSES_NEWPAD_METHODDEF
    _CURSES_NEWTERM_METHODDEF
    _CURSES_NEWWIN_METHODDEF
#ifdef HAVE_CURSES_NEW_PRESCR
    _CURSES_NEW_PRESCR_METHODDEF
#endif
    _CURSES_NL_METHODDEF
    _CURSES_NOCBREAK_METHODDEF
    _CURSES_NOECHO_METHODDEF
    _CURSES_NONL_METHODDEF
    _CURSES_NOQIFLUSH_METHODDEF
    _CURSES_NORAW_METHODDEF
    _CURSES_PAIR_CONTENT_METHODDEF
    _CURSES_PAIR_NUMBER_METHODDEF
    _CURSES_PUTP_METHODDEF
    _CURSES_QIFLUSH_METHODDEF
    _CURSES_RAW_METHODDEF
    _CURSES_RESET_COLOR_PAIRS_METHODDEF
    _CURSES_RESET_PROG_MODE_METHODDEF
    _CURSES_RESET_SHELL_MODE_METHODDEF
    _CURSES_RESETTY_METHODDEF
    _CURSES_RESIZETERM_METHODDEF
    _CURSES_RESIZE_TERM_METHODDEF
    _CURSES_SAVETTY_METHODDEF
#if defined(NCURSES_EXT_FUNCS) && NCURSES_EXT_FUNCS >= 20081102
    _CURSES_GET_ESCDELAY_METHODDEF
    _CURSES_SET_ESCDELAY_METHODDEF
#endif
    _CURSES_GET_TABSIZE_METHODDEF
    _CURSES_SET_TABSIZE_METHODDEF
    _CURSES_SET_TERM_METHODDEF
    _CURSES_SETSYX_METHODDEF
    _CURSES_SETUPTERM_METHODDEF
    _CURSES_START_COLOR_METHODDEF
    _CURSES_TERMATTRS_METHODDEF
    _CURSES_TERMNAME_METHODDEF
    _CURSES_TIGETFLAG_METHODDEF
    _CURSES_TIGETNUM_METHODDEF
    _CURSES_TIGETSTR_METHODDEF
    _CURSES_TPARM_METHODDEF
    _CURSES_TYPEAHEAD_METHODDEF
    _CURSES_UNCTRL_METHODDEF
    _CURSES_WUNCTRL_METHODDEF
    _CURSES_UNGETCH_METHODDEF
    _CURSES_UPDATE_LINES_COLS_METHODDEF
    _CURSES_UNGET_WCH_METHODDEF
    _CURSES_USE_ENV_METHODDEF
    _CURSES_USE_DEFAULT_COLORS_METHODDEF
    _CURSES_ASSUME_DEFAULT_COLORS_METHODDEF
    {NULL,                  NULL}         /* sentinel */
};

/* Module C API */

/* Function versions of the 3 functions for testing whether curses has been
   initialised or not. */

static inline int
curses_capi_setupterm_called(void)
{
    return _PyCursesCheckFunction(curses_setupterm_called, "setupterm");
}

static inline int
curses_capi_initscr_called(void)
{
    return _PyCursesCheckFunction(curses_initscr_called, "initscr");
}

static inline int
curses_capi_start_color_called(void)
{
    return _PyCursesCheckFunction(curses_start_color_called, "start_color");
}

static void *
curses_capi_new(cursesmodule_state *state)
{
    assert(state->window_type != NULL);
    void **capi = (void **)PyMem_Calloc(PyCurses_API_pointers, sizeof(void *));
    if (capi == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    capi[0] = (void *)Py_NewRef(state->window_type);
    capi[1] = curses_capi_setupterm_called;
    capi[2] = curses_capi_initscr_called;
    capi[3] = curses_capi_start_color_called;
    return (void *)capi;
}

static void
curses_capi_free(void *capi)
{
    assert(capi != NULL);
    void **capi_ptr = (void **)capi;
    // In free-threaded builds, capi_ptr[0] may have been already cleared
    // by curses_capi_capsule_destructor(), hence the use of Py_XDECREF().
    Py_XDECREF(capi_ptr[0]); // decref curses window type
    PyMem_Free(capi_ptr);
}

/* Module C API Capsule */

static void
curses_capi_capsule_destructor(PyObject *op)
{
    void *capi = PyCapsule_GetPointer(op, PyCurses_CAPSULE_NAME);
    curses_capi_free(capi);
}

static int
curses_capi_capsule_traverse(PyObject *op, visitproc visit, void *arg)
{
    void **capi_ptr = PyCapsule_GetPointer(op, PyCurses_CAPSULE_NAME);
    assert(capi_ptr != NULL);
    Py_VISIT(capi_ptr[0]);  // visit curses window type
    return 0;
}

static int
curses_capi_capsule_clear(PyObject *op)
{
    void **capi_ptr = PyCapsule_GetPointer(op, PyCurses_CAPSULE_NAME);
    assert(capi_ptr != NULL);
    Py_CLEAR(capi_ptr[0]);  // clear curses window type
    return 0;
}

static PyObject *
curses_capi_capsule_new(void *capi)
{
    PyObject *capsule = PyCapsule_New(capi, PyCurses_CAPSULE_NAME,
                                      curses_capi_capsule_destructor);
    if (capsule == NULL) {
        return NULL;
    }
    if (_PyCapsule_SetTraverse(capsule,
                               curses_capi_capsule_traverse,
                               curses_capi_capsule_clear) < 0)
    {
        Py_DECREF(capsule);
        return NULL;
    }
    return capsule;
}

/* Module initialization and cleanup functions */

static int
cursesmodule_traverse(PyObject *mod, visitproc visit, void *arg)
{
    cursesmodule_state *state = get_cursesmodule_state(mod);
    Py_VISIT(state->error);
    Py_VISIT(state->window_type);
    Py_VISIT(state->screen_type);
#ifdef HAVE_NCURSESW
    Py_VISIT(state->complexchar_type);
    Py_VISIT(state->complexstr_type);
#endif
    Py_VISIT(state->topscreen);
    return 0;
}

static int
cursesmodule_clear(PyObject *mod)
{
    cursesmodule_state *state = get_cursesmodule_state(mod);
    Py_CLEAR(state->error);
    Py_CLEAR(state->window_type);
    Py_CLEAR(state->screen_type);
#ifdef HAVE_NCURSESW
    Py_CLEAR(state->complexchar_type);
    Py_CLEAR(state->complexstr_type);
#endif
    Py_CLEAR(state->topscreen);
    return 0;
}

static void
cursesmodule_free(void *mod)
{
    (void)cursesmodule_clear((PyObject *)mod);
    PyMem_Free(curses_screen_encoding);
    curses_screen_encoding = NULL;
    curses_module_loaded = 0;  // allow reloading once garbage-collected
}

static int
cursesmodule_exec(PyObject *module)
{
    if (curses_module_loaded) {
        PyErr_SetString(PyExc_ImportError,
                        "module 'curses' can only be loaded once per process");
        return -1;
    }
    curses_module_loaded = 1;

    cursesmodule_state *state = get_cursesmodule_state(module);
    /* Initialize object type */
    state->window_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        module, &PyCursesWindow_Type_spec, NULL);
    if (state->window_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, state->window_type) < 0) {
        return -1;
    }
    state->screen_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        module, &PyCursesScreen_Type_spec, NULL);
    if (state->screen_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, state->screen_type) < 0) {
        return -1;
    }
#ifdef HAVE_NCURSESW
    state->complexchar_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        module, &PyCursesComplexChar_Type_spec, NULL);
    if (state->complexchar_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, state->complexchar_type) < 0) {
        return -1;
    }
    state->complexstr_type = (PyTypeObject *)PyType_FromModuleAndSpec(
        module, &PyCursesComplexStr_Type_spec, NULL);
    if (state->complexstr_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, state->complexstr_type) < 0) {
        return -1;
    }
#endif

    /* Add some symbolic constants to the module */
    PyObject *module_dict = PyModule_GetDict(module);
    if (module_dict == NULL) {
        return -1;
    }

    /* Create the C API object */
    void *capi = curses_capi_new(state);
    if (capi == NULL) {
        return -1;
    }
    /* Add a capsule for the C API */
    PyObject *capi_capsule = curses_capi_capsule_new(capi);
    if (capi_capsule == NULL) {
        curses_capi_free(capi);
        return -1;
    }
    int rc = PyDict_SetItemString(module_dict, "_C_API", capi_capsule);
    Py_DECREF(capi_capsule);
    if (rc < 0) {
        return -1;
    }

    /* For exception curses.error */
    state->error = PyErr_NewExceptionWithDoc(
        "_curses.error",
        "Exception raised when a curses library function returns an error.",
        NULL, NULL);
    if (state->error == NULL) {
        return -1;
    }
    rc = PyDict_SetItemString(module_dict, "error", state->error);
    if (rc < 0) {
        return -1;
    }

    /* Make the version available */
    PyObject *curses_version = PyBytes_FromString(PyCursesVersion);
    if (curses_version == NULL) {
        return -1;
    }
    rc = PyDict_SetItemString(module_dict, "version", curses_version);
    if (rc < 0) {
        Py_DECREF(curses_version);
        return -1;
    }
    rc = PyDict_SetItemString(module_dict, "__version__", curses_version);
    Py_CLEAR(curses_version);
    if (rc < 0) {
        return -1;
    }

#ifdef NCURSES_VERSION
    /* ncurses_version */
    PyTypeObject *version_type;
    version_type = _PyStructSequence_NewType(&ncurses_version_desc,
                                             Py_TPFLAGS_DISALLOW_INSTANTIATION);
    if (version_type == NULL) {
        return -1;
    }
    PyObject *ncurses_version = make_ncurses_version(version_type);
    Py_DECREF(version_type);
    if (ncurses_version == NULL) {
        return -1;
    }
    rc = PyDict_SetItemString(module_dict, "ncurses_version", ncurses_version);
    Py_CLEAR(ncurses_version);
    if (rc < 0) {
        return -1;
    }
#endif /* NCURSES_VERSION */

#define SetDictInt(NAME, VALUE)                                     \
    do {                                                            \
        PyObject *value = PyLong_FromLong((long)(VALUE));           \
        if (value == NULL) {                                        \
            return -1;                                              \
        }                                                           \
        int rc = PyDict_SetItemString(module_dict, (NAME), value);  \
        Py_DECREF(value);                                           \
        if (rc < 0) {                                               \
            return -1;                                              \
        }                                                           \
    } while (0)

    SetDictInt("ERR", ERR);
    SetDictInt("OK", OK);

    /* Here are some attributes you can add to chars to print */

    SetDictInt("A_ATTRIBUTES",      A_ATTRIBUTES);
    SetDictInt("A_NORMAL",              A_NORMAL);
    SetDictInt("A_STANDOUT",            A_STANDOUT);
    SetDictInt("A_UNDERLINE",           A_UNDERLINE);
    SetDictInt("A_REVERSE",             A_REVERSE);
    SetDictInt("A_BLINK",               A_BLINK);
    SetDictInt("A_DIM",                 A_DIM);
    SetDictInt("A_BOLD",                A_BOLD);
    SetDictInt("A_ALTCHARSET",          A_ALTCHARSET);
    SetDictInt("A_INVIS",           A_INVIS);
    SetDictInt("A_PROTECT",         A_PROTECT);
    SetDictInt("A_CHARTEXT",        A_CHARTEXT);
    SetDictInt("A_COLOR",           A_COLOR);

    /* The following are never available with strict SYSV curses */
#ifdef A_HORIZONTAL
    SetDictInt("A_HORIZONTAL",      A_HORIZONTAL);
#endif
#ifdef A_LEFT
    SetDictInt("A_LEFT",            A_LEFT);
#endif
#ifdef A_LOW
    SetDictInt("A_LOW",             A_LOW);
#endif
#ifdef A_RIGHT
    SetDictInt("A_RIGHT",           A_RIGHT);
#endif
#ifdef A_TOP
    SetDictInt("A_TOP",             A_TOP);
#endif
#ifdef A_VERTICAL
    SetDictInt("A_VERTICAL",        A_VERTICAL);
#endif

    /* ncurses extension */
#ifdef A_ITALIC
    SetDictInt("A_ITALIC",          A_ITALIC);
#endif

    /* The WA_* attributes are used by the attr_t-based functions
       (attr_get, attr_set, ...).  ncurses defines them bit-identically to the
       matching A_* constants, but X/Open keeps the two sets distinct, so other
       implementations (such as NetBSD curses) may give them different values. */
#ifdef WA_ATTRIBUTES
    SetDictInt("WA_ATTRIBUTES",     WA_ATTRIBUTES);
#endif
#ifdef WA_NORMAL
    SetDictInt("WA_NORMAL",         WA_NORMAL);
#endif
#ifdef WA_STANDOUT
    SetDictInt("WA_STANDOUT",       WA_STANDOUT);
#endif
#ifdef WA_UNDERLINE
    SetDictInt("WA_UNDERLINE",      WA_UNDERLINE);
#endif
#ifdef WA_REVERSE
    SetDictInt("WA_REVERSE",        WA_REVERSE);
#endif
#ifdef WA_BLINK
    SetDictInt("WA_BLINK",          WA_BLINK);
#endif
#ifdef WA_DIM
    SetDictInt("WA_DIM",            WA_DIM);
#endif
#ifdef WA_BOLD
    SetDictInt("WA_BOLD",           WA_BOLD);
#endif
#ifdef WA_ALTCHARSET
    SetDictInt("WA_ALTCHARSET",     WA_ALTCHARSET);
#endif
#ifdef WA_INVIS
    SetDictInt("WA_INVIS",          WA_INVIS);
#endif
#ifdef WA_PROTECT
    SetDictInt("WA_PROTECT",        WA_PROTECT);
#endif
#ifdef WA_HORIZONTAL
    SetDictInt("WA_HORIZONTAL",     WA_HORIZONTAL);
#endif
#ifdef WA_LEFT
    SetDictInt("WA_LEFT",           WA_LEFT);
#endif
#ifdef WA_LOW
    SetDictInt("WA_LOW",            WA_LOW);
#endif
#ifdef WA_RIGHT
    SetDictInt("WA_RIGHT",          WA_RIGHT);
#endif
#ifdef WA_TOP
    SetDictInt("WA_TOP",            WA_TOP);
#endif
#ifdef WA_VERTICAL
    SetDictInt("WA_VERTICAL",       WA_VERTICAL);
#endif
#ifdef WA_ITALIC
    SetDictInt("WA_ITALIC",         WA_ITALIC);
#endif

    SetDictInt("COLOR_BLACK",       COLOR_BLACK);
    SetDictInt("COLOR_RED",         COLOR_RED);
    SetDictInt("COLOR_GREEN",       COLOR_GREEN);
    SetDictInt("COLOR_YELLOW",      COLOR_YELLOW);
    SetDictInt("COLOR_BLUE",        COLOR_BLUE);
    SetDictInt("COLOR_MAGENTA",     COLOR_MAGENTA);
    SetDictInt("COLOR_CYAN",        COLOR_CYAN);
    SetDictInt("COLOR_WHITE",       COLOR_WHITE);

#ifdef NCURSES_MOUSE_VERSION
    /* Mouse-related constants */
    SetDictInt("BUTTON1_PRESSED",          BUTTON1_PRESSED);
    SetDictInt("BUTTON1_RELEASED",         BUTTON1_RELEASED);
    SetDictInt("BUTTON1_CLICKED",          BUTTON1_CLICKED);
    SetDictInt("BUTTON1_DOUBLE_CLICKED",   BUTTON1_DOUBLE_CLICKED);
    SetDictInt("BUTTON1_TRIPLE_CLICKED",   BUTTON1_TRIPLE_CLICKED);

    SetDictInt("BUTTON2_PRESSED",          BUTTON2_PRESSED);
    SetDictInt("BUTTON2_RELEASED",         BUTTON2_RELEASED);
    SetDictInt("BUTTON2_CLICKED",          BUTTON2_CLICKED);
    SetDictInt("BUTTON2_DOUBLE_CLICKED",   BUTTON2_DOUBLE_CLICKED);
    SetDictInt("BUTTON2_TRIPLE_CLICKED",   BUTTON2_TRIPLE_CLICKED);

    SetDictInt("BUTTON3_PRESSED",          BUTTON3_PRESSED);
    SetDictInt("BUTTON3_RELEASED",         BUTTON3_RELEASED);
    SetDictInt("BUTTON3_CLICKED",          BUTTON3_CLICKED);
    SetDictInt("BUTTON3_DOUBLE_CLICKED",   BUTTON3_DOUBLE_CLICKED);
    SetDictInt("BUTTON3_TRIPLE_CLICKED",   BUTTON3_TRIPLE_CLICKED);

    SetDictInt("BUTTON4_PRESSED",          BUTTON4_PRESSED);
    SetDictInt("BUTTON4_RELEASED",         BUTTON4_RELEASED);
    SetDictInt("BUTTON4_CLICKED",          BUTTON4_CLICKED);
    SetDictInt("BUTTON4_DOUBLE_CLICKED",   BUTTON4_DOUBLE_CLICKED);
    SetDictInt("BUTTON4_TRIPLE_CLICKED",   BUTTON4_TRIPLE_CLICKED);

#if NCURSES_MOUSE_VERSION > 1
    SetDictInt("BUTTON5_PRESSED",          BUTTON5_PRESSED);
    SetDictInt("BUTTON5_RELEASED",         BUTTON5_RELEASED);
    SetDictInt("BUTTON5_CLICKED",          BUTTON5_CLICKED);
    SetDictInt("BUTTON5_DOUBLE_CLICKED",   BUTTON5_DOUBLE_CLICKED);
    SetDictInt("BUTTON5_TRIPLE_CLICKED",   BUTTON5_TRIPLE_CLICKED);
#endif

    SetDictInt("BUTTON_SHIFT",             BUTTON_SHIFT);
    SetDictInt("BUTTON_CTRL",              BUTTON_CTRL);
    SetDictInt("BUTTON_ALT",               BUTTON_ALT);

    SetDictInt("ALL_MOUSE_EVENTS",         ALL_MOUSE_EVENTS);
    SetDictInt("REPORT_MOUSE_POSITION",    REPORT_MOUSE_POSITION);
#endif
    /* Now set everything up for KEY_ variables */
    for (int keycode = KEY_MIN; keycode < KEY_MAX; keycode++) {
        const char *key_name = keyname(keycode);
        if (key_name == NULL || strcmp(key_name, "UNKNOWN KEY") == 0) {
            continue;
        }
        if (strncmp(key_name, "KEY_F(", 6) == 0) {
            char *fn_key_name = PyMem_Malloc(strlen(key_name) + 1);
            if (!fn_key_name) {
                PyErr_NoMemory();
                return -1;
            }
            const char *p1 = key_name;
            char *p2 = fn_key_name;
            while (*p1) {
                if (*p1 != '(' && *p1 != ')') {
                    *p2 = *p1;
                    p2++;
                }
                p1++;
            }
            *p2 = (char)0;
            PyObject *p_keycode = PyLong_FromLong((long)keycode);
            if (p_keycode == NULL) {
                PyMem_Free(fn_key_name);
                return -1;
            }
            int rc = PyDict_SetItemString(module_dict, fn_key_name, p_keycode);
            Py_DECREF(p_keycode);
            PyMem_Free(fn_key_name);
            if (rc < 0) {
                return -1;
            }
        }
        else {
            SetDictInt(key_name, keycode);
        }
    }
    SetDictInt("KEY_MIN", KEY_MIN);
    SetDictInt("KEY_MAX", KEY_MAX);
#undef SetDictInt
    return 0;
}

/* Initialization function for the module */

static PyModuleDef_Slot cursesmodule_slots[] = {
    _Py_ABI_SLOT,
    {Py_mod_exec, cursesmodule_exec},
    {Py_mod_multiple_interpreters, Py_MOD_MULTIPLE_INTERPRETERS_NOT_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef cursesmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_curses",
    .m_size = sizeof(cursesmodule_state),
    .m_methods = cursesmodule_methods,
    .m_slots = cursesmodule_slots,
    .m_traverse = cursesmodule_traverse,
    .m_clear = cursesmodule_clear,
    .m_free = cursesmodule_free
};

PyMODINIT_FUNC
PyInit__curses(void)
{
    return PyModuleDef_Init(&cursesmodule);
}
