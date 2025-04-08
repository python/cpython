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
  del_curterm delscreen dupwin inchnstr inchstr innstr keyok
  mcprint mvaddchnstr mvaddchstr mvcur mvinchnstr
  mvinchstr mvinnstr mmvwaddchnstr mvwaddchstr
  mvwinchnstr mvwinchstr mvwinnstr newterm
  restartterm ripoffline scr_dump
  scr_init scr_restore scr_set scrl set_curterm set_term setterm
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
#include "pycore_sysmodule.h"   // _PySys_GetOptionalAttrString()
#include "pycore_fileutils.h"   // _Py_set_inheritable

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

/*[clinic input]
module _curses
class _curses.window "PyCursesWindowObject *" "clinic_state()->window_type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=ae6cb623018f2cbc]*/

/* Indicate whether the module has already been loaded or not. */
static int curses_module_loaded = 0;

/* Tells whether setupterm() has been called to initialise terminfo.  */
static int curses_setupterm_called = FALSE;

/* Tells whether initscr() has been called to initialise curses.  */
static int curses_initscr_called = FALSE;

/* Tells whether start_color() has been called to initialise color usage. */
static int curses_start_color_called = FALSE;

static const char *curses_screen_encoding = NULL;

/* Utility Checking Procedures */

/*
 * Function to check that 'funcname' has been called by testing
 * the 'called' boolean. If an error occurs, a PyCursesError is
 * set and this returns 0. Otherwise, this returns 1.
 *
 * Since this function can be called in functions that do not
 * have a direct access to the module's state, '_curses.error'
 * is imported on demand.
 */
static inline int
_PyCursesCheckFunction(int called, const char *funcname)
{
    if (called == TRUE) {
        return 1;
    }
    PyObject *exc = PyImport_ImportModuleAttrString("_curses", "error");
    if (exc != NULL) {
        PyErr_Format(exc, "must call %s() first", funcname);
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
static inline int
_PyCursesStatefulCheckFunction(PyObject *module, int called, const char *funcname)
{
    if (called == TRUE) {
        return 1;
    }
    cursesmodule_state *state = get_cursesmodule_state(module);
    PyErr_Format(state->error, "must call %s() first", funcname);
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

static inline void
_PyCursesSetError(cursesmodule_state *state, const char *funcname)
{
    if (funcname == NULL) {
        PyErr_SetString(state->error, catchall_ERR);
    }
    else {
        PyErr_Format(state->error, "%s() returned ERR", funcname);
    }
}

/*
 * Check the return code from a curses function and return None
 * or raise an exception as appropriate.
 */

static PyObject *
PyCursesCheckERR(PyObject *module, int code, const char *fname)
{
    if (code != ERR) {
        Py_RETURN_NONE;
    } else {
        cursesmodule_state *state = get_cursesmodule_state(module);
        _PyCursesSetError(state, fname);
        return NULL;
    }
}

static PyObject *
PyCursesCheckERR_ForWin(PyCursesWindowObject *win, int code, const char *fname)
{
    if (code != ERR) {
        Py_RETURN_NONE;
    } else {
        cursesmodule_state *state = get_cursesmodule_state_by_win(win);
        _PyCursesSetError(state, fname);
        return NULL;
    }
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
    - str of length 1

   Return:

    - 2 if obj is a character (written into *wch)
    - 1 if obj is a byte (written into *ch)
    - 0 on error: raise an exception */
static int
PyCurses_ConvertToCchar_t(PyCursesWindowObject *win, PyObject *obj,
                          chtype *ch
#ifdef HAVE_NCURSESW
                          , wchar_t *wch
#endif
                          )
{
    long value;
#ifdef HAVE_NCURSESW
    wchar_t buffer[2];
#endif

    if (PyUnicode_Check(obj)) {
#ifdef HAVE_NCURSESW
        if (PyUnicode_AsWideChar(obj, buffer, 2) != 1) {
            PyErr_Format(PyExc_TypeError,
                         "expect int or bytes or str of length 1, "
                         "got a str of length %zi",
                         PyUnicode_GET_LENGTH(obj));
            return 0;
        }
        *wch = buffer[0];
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

/*****************************************************************************
 The Window Object
******************************************************************************/

/* Function prototype macros for Window object

   X - function name
   TYPE - parameter Type
   ERGSTR - format string for construction of the return value
   PARSESTR - format string for argument parsing
*/

#define Window_NoArgNoReturnFunction(X)                                 \
    static PyObject *PyCursesWindow_ ## X                               \
    (PyObject *op, PyObject *Py_UNUSED(ignored))                        \
    {                                                                   \
        PyCursesWindowObject *self = _PyCursesWindowObject_CAST(op);    \
        int code = X(self->win);                                        \
        return PyCursesCheckERR_ForWin(self, code, # X);                \
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
        return PyCursesCheckERR_ForWin(self, code, # X);                \
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
        return PyCursesCheckERR_ForWin(self, code, # X);                \
    }

/* ------------- WINDOW routines --------------- */

Window_NoArgNoReturnFunction(untouchwin)
Window_NoArgNoReturnFunction(touchwin)
Window_NoArgNoReturnFunction(redrawwin)
Window_NoArgNoReturnFunction(winsertln)
Window_NoArgNoReturnFunction(werase)
Window_NoArgNoReturnFunction(wdeleteln)

Window_NoArgTrueFalseFunction(is_wintouched)

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
                   WINDOW *win, const char *encoding)
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
        // silently ignore errors in delwin(3)
        (void)delwin(wo->win);
    }
    if (wo->encoding != NULL) {
        PyMem_Free(wo->encoding);
    }
    window_type->tp_free(self);
    Py_DECREF(window_type);
}

static int
PyCursesWindow_traverse(PyObject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
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
    attr: long(c_default="A_NORMAL") = _curses.A_NORMAL
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
/*[clinic end generated code: output=00f4c37af3378f45 input=95ce131578458196]*/
{
    int coordinates_group = group_left_1;
    int rtn;
    int type;
    chtype cch = 0;
#ifdef HAVE_NCURSESW
    wchar_t wstr[2];
    cchar_t wcval;
#endif
    const char *funcname;

#ifdef HAVE_NCURSESW
    type = PyCurses_ConvertToCchar_t(self, ch, &cch, wstr);
    if (type == 2) {
        funcname = "add_wch";
        wstr[1] = L'\0';
        setcchar(&wcval, wstr, attr, PAIR_NUMBER(attr), NULL);
        if (coordinates_group)
            rtn = mvwadd_wch(self->win,y,x, &wcval);
        else {
            rtn = wadd_wch(self->win, &wcval);
        }
    }
    else
#else
    type = PyCurses_ConvertToCchar_t(self, ch, &cch);
#endif
    if (type == 1) {
        funcname = "addch";
        if (coordinates_group)
            rtn = mvwaddch(self->win,y,x, cch | (attr_t) attr);
        else {
            rtn = waddch(self->win, cch | (attr_t) attr);
        }
    }
    else {
        return NULL;
    }
    return PyCursesCheckERR_ForWin(self, rtn, funcname);
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
    strtype = PyCurses_ConvertToString(self, str, &bytesobj, &wstr);
#else
    strtype = PyCurses_ConvertToString(self, str, &bytesobj, NULL);
#endif
    if (strtype == 0) {
        return NULL;
    }
    if (use_attr) {
        attr_old = getattrs(self->win);
        (void)wattrset(self->win,attr);
    }
#ifdef HAVE_NCURSESW
    if (strtype == 2) {
        funcname = "addwstr";
        if (use_xy)
            rtn = mvwaddwstr(self->win,y,x,wstr);
        else
            rtn = waddwstr(self->win,wstr);
        PyMem_Free(wstr);
    }
    else
#endif
    {
        const char *str = PyBytes_AS_STRING(bytesobj);
        funcname = "addstr";
        if (use_xy)
            rtn = mvwaddstr(self->win,y,x,str);
        else
            rtn = waddstr(self->win,str);
        Py_DECREF(bytesobj);
    }
    if (use_attr)
        (void)wattrset(self->win,attr_old);
    return PyCursesCheckERR_ForWin(self, rtn, funcname);
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
    strtype = PyCurses_ConvertToString(self, str, &bytesobj, &wstr);
#else
    strtype = PyCurses_ConvertToString(self, str, &bytesobj, NULL);
#endif
    if (strtype == 0)
        return NULL;

    if (use_attr) {
        attr_old = getattrs(self->win);
        (void)wattrset(self->win,attr);
    }
#ifdef HAVE_NCURSESW
    if (strtype == 2) {
        funcname = "addnwstr";
        if (use_xy)
            rtn = mvwaddnwstr(self->win,y,x,wstr,n);
        else
            rtn = waddnwstr(self->win,wstr,n);
        PyMem_Free(wstr);
    }
    else
#endif
    {
        const char *str = PyBytes_AS_STRING(bytesobj);
        funcname = "addnstr";
        if (use_xy)
            rtn = mvwaddnstr(self->win,y,x,str,n);
        else
            rtn = waddnstr(self->win,str,n);
        Py_DECREF(bytesobj);
    }
    if (use_attr)
        (void)wattrset(self->win,attr_old);
    return PyCursesCheckERR_ForWin(self, rtn, funcname);
}

/*[clinic input]
_curses.window.bkgd

    ch: object
        Background character.
    attr: long(c_default="A_NORMAL") = _curses.A_NORMAL
        Background attributes.
    /

Set the background property of the window.
[clinic start generated code]*/

static PyObject *
_curses_window_bkgd_impl(PyCursesWindowObject *self, PyObject *ch, long attr)
/*[clinic end generated code: output=058290afb2cf4034 input=634015bcb339283d]*/
{
    chtype bkgd;

    if (!PyCurses_ConvertToChtype(self, ch, &bkgd))
        return NULL;

    return PyCursesCheckERR_ForWin(self, wbkgd(self->win, bkgd | attr), "bkgd");
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
    return PyCursesCheckERR_ForWin(self, wattroff(self->win, (attr_t)attr), "attroff");
}

/*[clinic input]
_curses.window.attron

    attr: long
    /

Add attribute attr from the "background" set.
[clinic start generated code]*/

static PyObject *
_curses_window_attron_impl(PyCursesWindowObject *self, long attr)
/*[clinic end generated code: output=7afea43b237fa870 input=5a88fba7b1524f32]*/
{
    return PyCursesCheckERR_ForWin(self, wattron(self->win, (attr_t)attr), "attron");
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
    return PyCursesCheckERR_ForWin(self, wattrset(self->win, (attr_t)attr), "attrset");
}

/*[clinic input]
_curses.window.bkgdset

    ch: object
        Background character.
    attr: long(c_default="A_NORMAL") = _curses.A_NORMAL
        Background attributes.
    /

Set the window's background.
[clinic start generated code]*/

static PyObject *
_curses_window_bkgdset_impl(PyCursesWindowObject *self, PyObject *ch,
                            long attr)
/*[clinic end generated code: output=8cb994fc4d7e2496 input=e09c682425c9e45b]*/
{
    chtype bkgd;

    if (!PyCurses_ConvertToChtype(self, ch, &bkgd))
        return NULL;

    wbkgdset(self->win, bkgd | attr);
    return PyCursesCheckERR_ForWin(self, 0, "bkgdset");
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

Each parameter specifies the character to use for a specific part of the
border.  The characters can be specified as integers or as one-character
strings.  A 0 value for any parameter will cause the default character to be
used for that parameter.
[clinic start generated code]*/

static PyObject *
_curses_window_border_impl(PyCursesWindowObject *self, PyObject *ls,
                           PyObject *rs, PyObject *ts, PyObject *bs,
                           PyObject *tl, PyObject *tr, PyObject *bl,
                           PyObject *br)
/*[clinic end generated code: output=670ef38d3d7c2aa3 input=e015f735d67a240b]*/
{
    chtype ch[8];
    int i;

    /* Clear the array of parameters */
    for(i=0; i<8; i++)
        ch[i] = 0;

#define CONVERTTOCHTYPE(obj, i) \
    if ((obj) != NULL && !PyCurses_ConvertToChtype(self, (obj), &ch[(i)])) \
        return NULL;

    CONVERTTOCHTYPE(ls, 0);
    CONVERTTOCHTYPE(rs, 1);
    CONVERTTOCHTYPE(ts, 2);
    CONVERTTOCHTYPE(bs, 3);
    CONVERTTOCHTYPE(tl, 4);
    CONVERTTOCHTYPE(tr, 5);
    CONVERTTOCHTYPE(bl, 6);
    CONVERTTOCHTYPE(br, 7);

#undef CONVERTTOCHTYPE

    wborder(self->win,
            ch[0], ch[1], ch[2], ch[3],
            ch[4], ch[5], ch[6], ch[7]);
    Py_RETURN_NONE;
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

Similar to border(), but both ls and rs are verch and both ts and bs are
horch.  The default corner characters are always used by this function.
[clinic start generated code]*/

static PyObject *
_curses_window_box_impl(PyCursesWindowObject *self, int group_right_1,
                        PyObject *verch, PyObject *horch)
/*[clinic end generated code: output=f3fcb038bb287192 input=f00435f9c8c98f60]*/
{
    chtype ch1 = 0, ch2 = 0;
    if (group_right_1) {
        if (!PyCurses_ConvertToChtype(self, verch, &ch1)) {
            return NULL;
        }
        if (!PyCurses_ConvertToChtype(self, horch, &ch2)) {
            return NULL;
        }
    }
    box(self->win,ch1,ch2);
    Py_RETURN_NONE;
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
/*[-clinic input]
_curses.window.chgat

    [
    y: int
        Y-coordinate.
    x: int
        X-coordinate.
    ]

    n: int = -1
        Number of characters.

    attr: long
        Attributes for characters.
    /

Set the attributes of characters.

Set the attributes of num characters at the current cursor position, or at
position (y, x) if supplied.  If no value of num is given or num = -1, the
attribute will be set on all the characters to the end of the line.  This
function does not move the cursor.  The changed line will be touched using
the touchline() method so that the contents will be redisplayed by the next
window refresh.
[-clinic start generated code]*/
static PyObject *
PyCursesWindow_ChgAt(PyObject *op, PyObject *args)
{
    PyCursesWindowObject *self = _PyCursesWindowObject_CAST(op);

    int rtn;
    int x, y;
    int num = -1;
    short color;
    attr_t attr = A_NORMAL;
    long lattr;
    int use_xy = FALSE;

    switch (PyTuple_Size(args)) {
    case 1:
        if (!PyArg_ParseTuple(args,"l;attr", &lattr))
            return NULL;
        attr = lattr;
        break;
    case 2:
        if (!PyArg_ParseTuple(args,"il;n,attr", &num, &lattr))
            return NULL;
        attr = lattr;
        break;
    case 3:
        if (!PyArg_ParseTuple(args,"iil;int,int,attr", &y, &x, &lattr))
            return NULL;
        attr = lattr;
        use_xy = TRUE;
        break;
    case 4:
        if (!PyArg_ParseTuple(args,"iiil;int,int,n,attr", &y, &x, &num, &lattr))
            return NULL;
        attr = lattr;
        use_xy = TRUE;
        break;
    default:
        PyErr_SetString(PyExc_TypeError, "chgat requires 1 to 4 arguments");
        return NULL;
    }

    color = (short) PAIR_NUMBER(attr);
    attr = attr & A_ATTRIBUTES;

    if (use_xy) {
        rtn = mvwchgat(self->win,y,x,num,attr,color,NULL);
        touchline(self->win,y,1);
    } else {
        getyx(self->win,y,x);
        rtn = wchgat(self->win,num,attr,color,NULL);
        touchline(self->win,y,1);
    }
    return PyCursesCheckERR_ForWin(self, rtn, "chgat");
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

Delete any character at (y, x).
[clinic start generated code]*/

static PyObject *
_curses_window_delch_impl(PyCursesWindowObject *self, int group_right_1,
                          int y, int x)
/*[clinic end generated code: output=22e77bb9fa11b461 input=d2f79e630a4fc6d0]*/
{
    if (!group_right_1) {
        return PyCursesCheckERR_ForWin(self, wdelch(self->win), "wdelch");
    }
    else {
        return PyCursesCheckERR_ForWin(self, py_mvwdelch(self->win, y, x), "mvwdelch");
    }
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

derwin() is the same as calling subwin(), except that begin_y and begin_x
are relative to the origin of the window, rather than relative to the entire
screen.
[clinic start generated code]*/

static PyObject *
_curses_window_derwin_impl(PyCursesWindowObject *self, int group_left_1,
                           int nlines, int ncols, int begin_y, int begin_x)
/*[clinic end generated code: output=7924b112d9f70d6e input=966d9481f7f5022e]*/
{
    WINDOW *win;

    win = derwin(self->win,nlines,ncols,begin_y,begin_x);

    if (win == NULL) {
        cursesmodule_state *state = get_cursesmodule_state_by_win(self);
        PyErr_SetString(state->error, catchall_NULL);
        return NULL;
    }

    cursesmodule_state *state = get_cursesmodule_state_by_win(self);
    return PyCursesWindow_New(state, win, NULL);
}

/*[clinic input]
_curses.window.echochar

    ch: object
        Character to add.

    attr: long(c_default="A_NORMAL") = _curses.A_NORMAL
        Attributes for the character.
    /

Add character ch with attribute attr, and refresh.
[clinic start generated code]*/

static PyObject *
_curses_window_echochar_impl(PyCursesWindowObject *self, PyObject *ch,
                             long attr)
/*[clinic end generated code: output=13e7dd875d4b9642 input=e7f34b964e92b156]*/
{
    chtype ch_;

    if (!PyCurses_ConvertToChtype(self, ch, &ch_))
        return NULL;

#ifdef py_is_pad
    if (py_is_pad(self->win)) {
        return PyCursesCheckERR_ForWin(self,
                                       pechochar(self->win, ch_ | (attr_t)attr),
                                       "echochar");
    }
    else
#endif
        return PyCursesCheckERR_ForWin(self,
                                       wechochar(self->win, ch_ | (attr_t)attr),
                                       "echochar");
}

#ifdef NCURSES_MOUSE_VERSION
/*[clinic input]
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
/*[clinic end generated code: output=8679beef50502648 input=4fd3355d723f7bc9]*/
{
    return PyBool_FromLong(wenclose(self->win, y, x));
}
#endif

/*[clinic input]
_curses.window.getbkgd -> long

Return the window's current background character/attribute pair.
[clinic start generated code]*/

static long
_curses_window_getbkgd_impl(PyCursesWindowObject *self)
/*[clinic end generated code: output=c52b25dc16b215c3 input=a69db882fa35426c]*/
{
    return (long) getbkgd(self->win);
}

/*[clinic input]
_curses.window.getch -> int

    [
    y: int
        Y-coordinate.
    x: int
        X-coordinate.
    ]
    /

Get a character code from terminal keyboard.

The integer returned does not have to be in ASCII range: function keys,
keypad keys and so on return numbers higher than 256.  In no-delay mode, -1
is returned if there is no input, else getch() waits until a key is pressed.
[clinic start generated code]*/

static int
_curses_window_getch_impl(PyCursesWindowObject *self, int group_right_1,
                          int y, int x)
/*[clinic end generated code: output=980aa6af0c0ca387 input=bb24ebfb379f991f]*/
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

    return rtn;
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

Returning a string instead of an integer, as getch() does.  Function keys,
keypad keys and other special keys return a multibyte string containing the
key name.  In no-delay mode, an exception is raised if there is no input.
[clinic start generated code]*/

static PyObject *
_curses_window_getkey_impl(PyCursesWindowObject *self, int group_right_1,
                           int y, int x)
/*[clinic end generated code: output=8490a182db46b10f input=be2dee34f5cf57f8]*/
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
        /* getch() returns ERR in nodelay mode */
        PyErr_CheckSignals();
        if (!PyErr_Occurred()) {
            cursesmodule_state *state = get_cursesmodule_state_by_win(self);
            PyErr_SetString(state->error, "no input");
        }
        return NULL;
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
        if (PyErr_CheckSignals())
            return NULL;

        /* get_wch() returns ERR in nodelay mode */
        cursesmodule_state *state = get_cursesmodule_state_by_win(self);
        PyErr_SetString(state->error, "no input");
        return NULL;
    }
    if (ct == KEY_CODE_YES)
        return PyLong_FromLong(rtn);
    else
        return PyUnicode_FromOrdinal(rtn);
}
#endif

/*[-clinic input]
_curses.window.getstr

    [
    y: int
        Y-coordinate.
    x: int
        X-coordinate.
    ]
    n: int = 1023
        Maximal number of characters.
    /

Read a string from the user, with primitive line editing capacity.
[-clinic start generated code]*/

static PyObject *
PyCursesWindow_GetStr(PyObject *op, PyObject *args)
{
    PyCursesWindowObject *self = _PyCursesWindowObject_CAST(op);

    int x, y, n;
    char rtn[1024]; /* This should be big enough.. I hope */
    int rtn2;

    switch (PyTuple_Size(args)) {
    case 0:
        Py_BEGIN_ALLOW_THREADS
        rtn2 = wgetnstr(self->win,rtn, 1023);
        Py_END_ALLOW_THREADS
        break;
    case 1:
        if (!PyArg_ParseTuple(args,"i;n", &n))
            return NULL;
        if (n < 0) {
            PyErr_SetString(PyExc_ValueError, "'n' must be nonnegative");
            return NULL;
        }
        Py_BEGIN_ALLOW_THREADS
        rtn2 = wgetnstr(self->win, rtn, Py_MIN(n, 1023));
        Py_END_ALLOW_THREADS
        break;
    case 2:
        if (!PyArg_ParseTuple(args,"ii;y,x",&y,&x))
            return NULL;
        Py_BEGIN_ALLOW_THREADS
#ifdef STRICT_SYSV_CURSES
        rtn2 = wmove(self->win,y,x)==ERR ? ERR : wgetnstr(self->win, rtn, 1023);
#else
        rtn2 = mvwgetnstr(self->win,y,x,rtn, 1023);
#endif
        Py_END_ALLOW_THREADS
        break;
    case 3:
        if (!PyArg_ParseTuple(args,"iii;y,x,n", &y, &x, &n))
            return NULL;
        if (n < 0) {
            PyErr_SetString(PyExc_ValueError, "'n' must be nonnegative");
            return NULL;
        }
#ifdef STRICT_SYSV_CURSES
        Py_BEGIN_ALLOW_THREADS
        rtn2 = wmove(self->win,y,x)==ERR ? ERR :
        wgetnstr(self->win, rtn, Py_MIN(n, 1023));
        Py_END_ALLOW_THREADS
#else
        Py_BEGIN_ALLOW_THREADS
        rtn2 = mvwgetnstr(self->win, y, x, rtn, Py_MIN(n, 1023));
        Py_END_ALLOW_THREADS
#endif
        break;
    default:
        PyErr_SetString(PyExc_TypeError, "getstr requires 0 to 3 arguments");
        return NULL;
    }
    if (rtn2 == ERR)
        rtn[0] = 0;
    return PyBytes_FromString(rtn);
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
    attr: long(c_default="A_NORMAL") = _curses.A_NORMAL
        Attributes for the characters.
    ]
    /

Display a horizontal line.
[clinic start generated code]*/

static PyObject *
_curses_window_hline_impl(PyCursesWindowObject *self, int group_left_1,
                          int y, int x, PyObject *ch, int n,
                          int group_right_1, long attr)
/*[clinic end generated code: output=c00d489d61fc9eef input=81a4dea47268163e]*/
{
    chtype ch_;

    if (!PyCurses_ConvertToChtype(self, ch, &ch_))
        return NULL;
    if (group_left_1) {
        if (wmove(self->win, y, x) == ERR) {
            return PyCursesCheckERR_ForWin(self, ERR, "wmove");
        }
    }
    return PyCursesCheckERR_ForWin(self, whline(self->win, ch_ | (attr_t)attr, n), "hline");
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
    attr: long(c_default="A_NORMAL") = _curses.A_NORMAL
        Attributes for the character.
    ]
    /

Insert a character before the current or specified position.

All characters to the right of the cursor are shifted one position right, with
the rightmost characters on the line being lost.
[clinic start generated code]*/

static PyObject *
_curses_window_insch_impl(PyCursesWindowObject *self, int group_left_1,
                          int y, int x, PyObject *ch, int group_right_1,
                          long attr)
/*[clinic end generated code: output=ade8cfe3a3bf3e34 input=336342756ee19812]*/
{
    int rtn;
    chtype ch_ = 0;

    if (!PyCurses_ConvertToChtype(self, ch, &ch_))
        return NULL;

    if (!group_left_1) {
        rtn = winsch(self->win, ch_ | (attr_t)attr);
    }
    else {
        rtn = mvwinsch(self->win, y, x, ch_ | (attr_t)attr);
    }

    return PyCursesCheckERR_ForWin(self, rtn, "insch");
}

/*[clinic input]
_curses.window.inch -> unsigned_long

    [
    y: int
        Y-coordinate.
    x: int
        X-coordinate.
    ]
    /

Return the character at the given position in the window.

The bottom 8 bits are the character proper, and upper bits are the attributes.
[clinic start generated code]*/

static unsigned long
_curses_window_inch_impl(PyCursesWindowObject *self, int group_right_1,
                         int y, int x)
/*[clinic end generated code: output=6c4719fe978fe86a input=fac23ee11e3b3a66]*/
{
    unsigned long rtn;

    if (!group_right_1) {
        rtn = winch(self->win);
    }
    else {
        rtn = mvwinch(self->win, y, x);
    }

    return rtn;
}

/*[-clinic input]
_curses.window.instr

    [
    y: int
        Y-coordinate.
    x: int
        X-coordinate.
    ]
    n: int = 1023
        Maximal number of characters.
    /

Return a string of characters, extracted from the window.

Return a string of characters, extracted from the window starting at the
current cursor position, or at y, x if specified.  Attributes are stripped
from the characters.  If n is specified, instr() returns a string at most
n characters long (exclusive of the trailing NUL).
[-clinic start generated code]*/
static PyObject *
PyCursesWindow_InStr(PyObject *op, PyObject *args)
{
    PyCursesWindowObject *self = _PyCursesWindowObject_CAST(op);

    int x, y, n;
    char rtn[1024]; /* This should be big enough.. I hope */
    int rtn2;

    switch (PyTuple_Size(args)) {
    case 0:
        rtn2 = winnstr(self->win,rtn, 1023);
        break;
    case 1:
        if (!PyArg_ParseTuple(args,"i;n", &n))
            return NULL;
        if (n < 0) {
            PyErr_SetString(PyExc_ValueError, "'n' must be nonnegative");
            return NULL;
        }
        rtn2 = winnstr(self->win, rtn, Py_MIN(n, 1023));
        break;
    case 2:
        if (!PyArg_ParseTuple(args,"ii;y,x",&y,&x))
            return NULL;
        rtn2 = mvwinnstr(self->win,y,x,rtn,1023);
        break;
    case 3:
        if (!PyArg_ParseTuple(args, "iii;y,x,n", &y, &x, &n))
            return NULL;
        if (n < 0) {
            PyErr_SetString(PyExc_ValueError, "'n' must be nonnegative");
            return NULL;
        }
        rtn2 = mvwinnstr(self->win, y, x, rtn, Py_MIN(n,1023));
        break;
    default:
        PyErr_SetString(PyExc_TypeError, "instr requires 0 or 3 arguments");
        return NULL;
    }
    if (rtn2 == ERR)
        rtn[0] = 0;
    return PyBytes_FromString(rtn);
}

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

Insert a character string (as many characters as will fit on the line)
before the character under the cursor.  All characters to the right of
the cursor are shifted right, with the rightmost characters on the line
being lost.  The cursor position does not change (after moving to y, x,
if specified).
[clinic start generated code]*/

static PyObject *
_curses_window_insstr_impl(PyCursesWindowObject *self, int group_left_1,
                           int y, int x, PyObject *str, int group_right_1,
                           long attr)
/*[clinic end generated code: output=c259a5265ad0b777 input=6827cddc6340a7f3]*/
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
    strtype = PyCurses_ConvertToString(self, str, &bytesobj, &wstr);
#else
    strtype = PyCurses_ConvertToString(self, str, &bytesobj, NULL);
#endif
    if (strtype == 0)
        return NULL;

    if (use_attr) {
        attr_old = getattrs(self->win);
        (void)wattrset(self->win, (attr_t)attr);
    }
#ifdef HAVE_NCURSESW
    if (strtype == 2) {
        funcname = "inswstr";
        if (use_xy)
            rtn = mvwins_wstr(self->win,y,x,wstr);
        else
            rtn = wins_wstr(self->win,wstr);
        PyMem_Free(wstr);
    }
    else
#endif
    {
        const char *str = PyBytes_AS_STRING(bytesobj);
        funcname = "insstr";
        if (use_xy)
            rtn = mvwinsstr(self->win,y,x,str);
        else
            rtn = winsstr(self->win,str);
        Py_DECREF(bytesobj);
    }
    if (use_attr)
        (void)wattrset(self->win,attr_old);
    return PyCursesCheckERR_ForWin(self, rtn, funcname);
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

Insert a character string (as many characters as will fit on the line)
before the character under the cursor, up to n characters.  If n is zero
or negative, the entire string is inserted.  All characters to the right
of the cursor are shifted right, with the rightmost characters on the line
being lost.  The cursor position does not change (after moving to y, x, if
specified).
[clinic start generated code]*/

static PyObject *
_curses_window_insnstr_impl(PyCursesWindowObject *self, int group_left_1,
                            int y, int x, PyObject *str, int n,
                            int group_right_1, long attr)
/*[clinic end generated code: output=971a32ea6328ec8b input=70fa0cd543901a4c]*/
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
    strtype = PyCurses_ConvertToString(self, str, &bytesobj, &wstr);
#else
    strtype = PyCurses_ConvertToString(self, str, &bytesobj, NULL);
#endif
    if (strtype == 0)
        return NULL;

    if (use_attr) {
        attr_old = getattrs(self->win);
        (void)wattrset(self->win, (attr_t)attr);
    }
#ifdef HAVE_NCURSESW
    if (strtype == 2) {
        funcname = "insn_wstr";
        if (use_xy)
            rtn = mvwins_nwstr(self->win,y,x,wstr,n);
        else
            rtn = wins_nwstr(self->win,wstr,n);
        PyMem_Free(wstr);
    }
    else
#endif
    {
        const char *str = PyBytes_AS_STRING(bytesobj);
        funcname = "insnstr";
        if (use_xy)
            rtn = mvwinsnstr(self->win,y,x,str,n);
        else
            rtn = winsnstr(self->win,str,n);
        Py_DECREF(bytesobj);
    }
    if (use_attr)
        (void)wattrset(self->win,attr_old);
    return PyCursesCheckERR_ForWin(self, rtn, funcname);
}

/*[clinic input]
_curses.window.is_linetouched

    line: int
        Line number.
    /

Return True if the specified line was modified, otherwise return False.

Raise a curses.error exception if line is not valid for the given window.
[clinic start generated code]*/

static PyObject *
_curses_window_is_linetouched_impl(PyCursesWindowObject *self, int line)
/*[clinic end generated code: output=ad4a4edfee2db08c input=a7be0c189f243914]*/
{
    int erg;
    erg = is_linetouched(self->win, line);
    if (erg == ERR) {
        PyErr_SetString(PyExc_TypeError,
                        "is_linetouched: line number outside of boundaries");
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

This function updates the data structure representing the desired state of the
window, but does not force an update of the physical screen.  To accomplish
that, call doupdate().
[clinic start generated code]*/

static PyObject *
_curses_window_noutrefresh_impl(PyCursesWindowObject *self,
                                int group_right_1, int pminrow, int pmincol,
                                int sminrow, int smincol, int smaxrow,
                                int smaxcol)
/*[clinic end generated code: output=809a1f3c6a03e23e input=3e56898388cd739e]*/
#else
/*[clinic input]
_curses.window.noutrefresh

Mark for refresh but wait.

This function updates the data structure representing the desired state of the
window, but does not force an update of the physical screen.  To accomplish
that, call doupdate().
[clinic start generated code]*/

static PyObject *
_curses_window_noutrefresh_impl(PyCursesWindowObject *self)
/*[clinic end generated code: output=6ef6dec666643fee input=876902e3fa431dbd]*/
#endif
{
    int rtn;

#ifdef py_is_pad
    if (py_is_pad(self->win)) {
        if (!group_right_1) {
            cursesmodule_state *state = get_cursesmodule_state_by_win(self);
            PyErr_SetString(state->error,
                            "noutrefresh() called for a pad "
                            "requires 6 arguments");
            return NULL;
        }
        Py_BEGIN_ALLOW_THREADS
        rtn = pnoutrefresh(self->win, pminrow, pmincol,
                           sminrow, smincol, smaxrow, smaxcol);
        Py_END_ALLOW_THREADS
        return PyCursesCheckERR_ForWin(self, rtn, "pnoutrefresh");
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
    return PyCursesCheckERR_ForWin(self, rtn, "wnoutrefresh");
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

The windows need not be the same size, only the overlapping region is copied.
This copy is non-destructive, which means that the current background
character does not overwrite the old contents of destwin.

To get fine-grained control over the copied region, the second form of
overlay() can be used.  sminrow and smincol are the upper-left coordinates
of the source window, and the other variables mark a rectangle in the
destination window.
[clinic start generated code]*/

static PyObject *
_curses_window_overlay_impl(PyCursesWindowObject *self,
                            PyCursesWindowObject *destwin, int group_right_1,
                            int sminrow, int smincol, int dminrow,
                            int dmincol, int dmaxrow, int dmaxcol)
/*[clinic end generated code: output=82bb2c4cb443ca58 input=6e4b32a7c627a356]*/
{
    int rtn;

    if (group_right_1) {
        rtn = copywin(self->win, destwin->win, sminrow, smincol,
                      dminrow, dmincol, dmaxrow, dmaxcol, TRUE);
        return PyCursesCheckERR_ForWin(self, rtn, "copywin");
    }
    else {
        rtn = overlay(self->win, destwin->win);
        return PyCursesCheckERR_ForWin(self, rtn, "overlay");
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

The windows need not be the same size, in which case only the overlapping
region is copied.  This copy is destructive, which means that the current
background character overwrites the old contents of destwin.

To get fine-grained control over the copied region, the second form of
overwrite() can be used. sminrow and smincol are the upper-left coordinates
of the source window, the other variables mark a rectangle in the destination
window.
[clinic start generated code]*/

static PyObject *
_curses_window_overwrite_impl(PyCursesWindowObject *self,
                              PyCursesWindowObject *destwin,
                              int group_right_1, int sminrow, int smincol,
                              int dminrow, int dmincol, int dmaxrow,
                              int dmaxcol)
/*[clinic end generated code: output=12ae007d1681be28 input=d83dd8b24ff2bcc9]*/
{
    int rtn;

    if (group_right_1) {
        rtn = copywin(self->win, destwin->win, sminrow, smincol,
                      dminrow, dmincol, dmaxrow, dmaxcol, FALSE);
        return PyCursesCheckERR_ForWin(self, rtn, "copywin");
    }
    else {
        rtn = overwrite(self->win, destwin->win);
        return PyCursesCheckERR_ForWin(self, rtn, "overwrite");
    }
}

/*[clinic input]
_curses.window.putwin

    file: object
    /

Write all data associated with the window into the provided file object.

This information can be later retrieved using the getwin() function.
[clinic start generated code]*/

static PyObject *
_curses_window_putwin_impl(PyCursesWindowObject *self, PyObject *file)
/*[clinic end generated code: output=fdae68ac59b0281b input=0608648e09c8ea0a]*/
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
    res = PyCursesCheckERR_ForWin(self, putwin(self->win, fp), "putwin");
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
    return PyCursesCheckERR_ForWin(self, wredrawln(self->win,beg,num), "redrawln");
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
The 6 optional arguments can only be specified when the window is a pad
created with newpad().  The additional parameters are needed to indicate
what part of the pad and screen are involved.  pminrow and pmincol specify
the upper left-hand corner of the rectangle to be displayed in the pad.
sminrow, smincol, smaxrow, and smaxcol specify the edges of the rectangle to
be displayed on the screen.  The lower right-hand corner of the rectangle to
be displayed in the pad is calculated from the screen coordinates, since the
rectangles must be the same size.  Both rectangles must be entirely contained
within their respective structures.  Negative values of pminrow, pmincol,
sminrow, or smincol are treated as if they were zero.
[clinic start generated code]*/

static PyObject *
_curses_window_refresh_impl(PyCursesWindowObject *self, int group_right_1,
                            int pminrow, int pmincol, int sminrow,
                            int smincol, int smaxrow, int smaxcol)
/*[clinic end generated code: output=42199543115e6e63 input=95e01cb5ffc635d0]*/
{
    int rtn;

#ifdef py_is_pad
    if (py_is_pad(self->win)) {
        if (!group_right_1) {
            cursesmodule_state *state = get_cursesmodule_state_by_win(self);
            PyErr_SetString(state->error,
                            "refresh() for a pad requires 6 arguments");
            return NULL;
        }
        Py_BEGIN_ALLOW_THREADS
        rtn = prefresh(self->win, pminrow, pmincol,
                       sminrow, smincol, smaxrow, smaxcol);
        Py_END_ALLOW_THREADS
        return PyCursesCheckERR_ForWin(self, rtn, "prefresh");
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
    return PyCursesCheckERR_ForWin(self, rtn, "prefresh");
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
    return PyCursesCheckERR_ForWin(self, wsetscrreg(self->win, top, bottom), "wsetscrreg");
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

By default, the sub-window will extend from the specified position to the
lower right corner of the window.
[clinic start generated code]*/

static PyObject *
_curses_window_subwin_impl(PyCursesWindowObject *self, int group_left_1,
                           int nlines, int ncols, int begin_y, int begin_x)
/*[clinic end generated code: output=93e898afc348f59a input=2129fa47fd57721c]*/
{
    WINDOW *win;

    /* printf("Subwin: %i %i %i %i   \n", nlines, ncols, begin_y, begin_x); */
#ifdef py_is_pad
    if (py_is_pad(self->win)) {
        win = subpad(self->win, nlines, ncols, begin_y, begin_x);
    }
    else
#endif
        win = subwin(self->win, nlines, ncols, begin_y, begin_x);

    if (win == NULL) {
        cursesmodule_state *state = get_cursesmodule_state_by_win(self);
        PyErr_SetString(state->error, catchall_NULL);
        return NULL;
    }

    cursesmodule_state *state = get_cursesmodule_state_by_win(self);
    return PyCursesWindow_New(state, win, self->encoding);
}

/*[clinic input]
_curses.window.scroll

    [
    lines: int = 1
        Number of lines to scroll.
    ]
    /

Scroll the screen or scrolling region.

Scroll upward if the argument is positive and downward if it is negative.
[clinic start generated code]*/

static PyObject *
_curses_window_scroll_impl(PyCursesWindowObject *self, int group_right_1,
                           int lines)
/*[clinic end generated code: output=4541a8a11852d360 input=c969ca0cfabbdbec]*/
{
    if (!group_right_1) {
        return PyCursesCheckERR_ForWin(self, scroll(self->win), "scroll");
    }
    else {
        return PyCursesCheckERR_ForWin(self, wscrl(self->win, lines), "scroll");
    }
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

If changed is supplied, it specifies whether the affected lines are marked
as having been changed (changed=True) or unchanged (changed=False).
[clinic start generated code]*/

static PyObject *
_curses_window_touchline_impl(PyCursesWindowObject *self, int start,
                              int count, int group_right_1, int changed)
/*[clinic end generated code: output=65d05b3f7438c61d input=a98aa4f79b6be845]*/
{
    if (!group_right_1) {
        return PyCursesCheckERR_ForWin(self, touchline(self->win, start, count), "touchline");
    }
    else {
        return PyCursesCheckERR_ForWin(self, wtouchln(self->win, start, count, changed), "touchline");
    }
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
    attr: long(c_default="A_NORMAL") = _curses.A_NORMAL
        Attributes for the character.
    ]
    /

Display a vertical line.
[clinic start generated code]*/

static PyObject *
_curses_window_vline_impl(PyCursesWindowObject *self, int group_left_1,
                          int y, int x, PyObject *ch, int n,
                          int group_right_1, long attr)
/*[clinic end generated code: output=287ad1cc8982217f input=a6f2dc86a4648b32]*/
{
    chtype ch_;

    if (!PyCurses_ConvertToChtype(self, ch, &ch_))
        return NULL;
    if (group_left_1) {
        if (wmove(self->win, y, x) == ERR)
            return PyCursesCheckERR_ForWin(self, ERR, "wmove");
    }
    return PyCursesCheckERR_ForWin(self, wvline(self->win, ch_ | (attr_t)attr, n), "vline");
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
#undef clinic_state

static PyMethodDef PyCursesWindow_methods[] = {
    _CURSES_WINDOW_ADDCH_METHODDEF
    _CURSES_WINDOW_ADDNSTR_METHODDEF
    _CURSES_WINDOW_ADDSTR_METHODDEF
    _CURSES_WINDOW_ATTROFF_METHODDEF
    _CURSES_WINDOW_ATTRON_METHODDEF
    _CURSES_WINDOW_ATTRSET_METHODDEF
    _CURSES_WINDOW_BKGD_METHODDEF
#ifdef HAVE_CURSES_WCHGAT
    {"chgat",           PyCursesWindow_ChgAt, METH_VARARGS},
#endif
    _CURSES_WINDOW_BKGDSET_METHODDEF
    _CURSES_WINDOW_BORDER_METHODDEF
    _CURSES_WINDOW_BOX_METHODDEF
    {"clear",           PyCursesWindow_wclear, METH_NOARGS},
    {"clearok",         PyCursesWindow_clearok, METH_VARARGS},
    {"clrtobot",        PyCursesWindow_wclrtobot, METH_NOARGS},
    {"clrtoeol",        PyCursesWindow_wclrtoeol, METH_NOARGS},
    {"cursyncup",       PyCursesWindow_wcursyncup, METH_NOARGS},
    _CURSES_WINDOW_DELCH_METHODDEF
    {"deleteln",        PyCursesWindow_wdeleteln, METH_NOARGS},
    _CURSES_WINDOW_DERWIN_METHODDEF
    _CURSES_WINDOW_ECHOCHAR_METHODDEF
    _CURSES_WINDOW_ENCLOSE_METHODDEF
    {"erase",           PyCursesWindow_werase, METH_NOARGS},
    {"getbegyx",        PyCursesWindow_getbegyx, METH_NOARGS},
    _CURSES_WINDOW_GETBKGD_METHODDEF
    _CURSES_WINDOW_GETCH_METHODDEF
    _CURSES_WINDOW_GETKEY_METHODDEF
    _CURSES_WINDOW_GET_WCH_METHODDEF
    {"getmaxyx",        PyCursesWindow_getmaxyx, METH_NOARGS},
    {"getparyx",        PyCursesWindow_getparyx, METH_NOARGS},
    {"getstr",          PyCursesWindow_GetStr, METH_VARARGS},
    {"getyx",           PyCursesWindow_getyx, METH_NOARGS},
    _CURSES_WINDOW_HLINE_METHODDEF
    {"idcok",           PyCursesWindow_idcok, METH_VARARGS},
    {"idlok",           PyCursesWindow_idlok, METH_VARARGS},
#ifdef HAVE_CURSES_IMMEDOK
    {"immedok",         PyCursesWindow_immedok, METH_VARARGS},
#endif
    _CURSES_WINDOW_INCH_METHODDEF
    _CURSES_WINDOW_INSCH_METHODDEF
    {"insdelln",        PyCursesWindow_winsdelln, METH_VARARGS},
    {"insertln",        PyCursesWindow_winsertln, METH_NOARGS},
    _CURSES_WINDOW_INSNSTR_METHODDEF
    _CURSES_WINDOW_INSSTR_METHODDEF
    {"instr",           PyCursesWindow_InStr, METH_VARARGS},
    _CURSES_WINDOW_IS_LINETOUCHED_METHODDEF
    {"is_wintouched",   PyCursesWindow_is_wintouched, METH_NOARGS},
    {"keypad",          PyCursesWindow_keypad, METH_VARARGS},
    {"leaveok",         PyCursesWindow_leaveok, METH_VARARGS},
    {"move",            PyCursesWindow_wmove, METH_VARARGS},
    {"mvderwin",        PyCursesWindow_mvderwin, METH_VARARGS},
    {"mvwin",           PyCursesWindow_mvwin, METH_VARARGS},
    {"nodelay",         PyCursesWindow_nodelay, METH_VARARGS},
    {"notimeout",       PyCursesWindow_notimeout, METH_VARARGS},
    _CURSES_WINDOW_NOUTREFRESH_METHODDEF
    _CURSES_WINDOW_OVERLAY_METHODDEF
    _CURSES_WINDOW_OVERWRITE_METHODDEF
    _CURSES_WINDOW_PUTWIN_METHODDEF
    _CURSES_WINDOW_REDRAWLN_METHODDEF
    {"redrawwin",       PyCursesWindow_redrawwin, METH_NOARGS},
    _CURSES_WINDOW_REFRESH_METHODDEF
#ifndef STRICT_SYSV_CURSES
    {"resize",          PyCursesWindow_wresize, METH_VARARGS},
#endif
    _CURSES_WINDOW_SCROLL_METHODDEF
    {"scrollok",        PyCursesWindow_scrollok, METH_VARARGS},
    _CURSES_WINDOW_SETSCRREG_METHODDEF
    {"standend",        PyCursesWindow_wstandend, METH_NOARGS},
    {"standout",        PyCursesWindow_wstandout, METH_NOARGS},
    {"subpad",          _curses_window_subwin, METH_VARARGS, _curses_window_subwin__doc__},
    _CURSES_WINDOW_SUBWIN_METHODDEF
    {"syncdown",        PyCursesWindow_wsyncdown, METH_NOARGS},
#ifdef HAVE_CURSES_SYNCOK
    {"syncok",          PyCursesWindow_syncok, METH_VARARGS},
#endif
    {"syncup",          PyCursesWindow_wsyncup, METH_NOARGS},
    {"timeout",         PyCursesWindow_wtimeout, METH_VARARGS},
    _CURSES_WINDOW_TOUCHLINE_METHODDEF
    {"touchwin",        PyCursesWindow_touchwin, METH_NOARGS},
    {"untouchwin",      PyCursesWindow_untouchwin, METH_NOARGS},
    _CURSES_WINDOW_VLINE_METHODDEF
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

static PyType_Slot PyCursesWindow_Type_slots[] = {
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

/* Function Body Macros - They are ugly but very, very useful. ;-)

   X - function name
   TYPE - parameter Type
   ERGSTR - format string for construction of the return value
   PARSESTR - format string for argument parsing
   */

#define NoArgNoReturnFunctionBody(X) \
{ \
  PyCursesStatefulInitialised(module); \
  return PyCursesCheckERR(module, X(), # X); }

#define NoArgOrFlagNoReturnFunctionBody(X, flag) \
{ \
    PyCursesStatefulInitialised(module); \
    if (flag) \
        return PyCursesCheckERR(module, X(), # X); \
    else \
        return PyCursesCheckERR(module, no ## X(), # X); \
}

#define NoArgReturnIntFunctionBody(X) \
{ \
 PyCursesStatefulInitialised(module); \
 return PyLong_FromLong((long) X()); }


#define NoArgReturnStringFunctionBody(X) \
{ \
  PyCursesStatefulInitialised(module); \
  return PyBytes_FromString(X()); }

#define NoArgTrueFalseFunctionBody(X) \
{ \
  PyCursesStatefulInitialised(module); \
  return PyBool_FromLong(X()); }

#define NoArgNoReturnVoidFunctionBody(X) \
{ \
  PyCursesStatefulInitialised(module); \
  X(); \
  Py_RETURN_NONE; }

/*********************************************************************
 Global Functions
**********************************************************************/

#ifdef HAVE_CURSES_FILTER
/*[clinic input]
_curses.filter

[clinic start generated code]*/

static PyObject *
_curses_filter_impl(PyObject *module)
/*[clinic end generated code: output=fb5b8a3642eb70b5 input=668c75a6992d3624]*/
{
    /* not checking for PyCursesInitialised here since filter() must
       be called before initscr() */
    filter();
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
_curses.can_change_color

Return True if the programmer can change the colors displayed by the terminal.
[clinic start generated code]*/

static PyObject *
_curses_can_change_color_impl(PyObject *module)
/*[clinic end generated code: output=359df8c3c77d8bf1 input=d7718884de0092f2]*/
NoArgTrueFalseFunctionBody(can_change_color)

/*[clinic input]
_curses.cbreak

    flag: bool = True
        If false, the effect is the same as calling nocbreak().
    /

Enter cbreak mode.

In cbreak mode (sometimes called "rare" mode) normal tty line buffering is
turned off and characters are available to be read one by one.  However,
unlike raw mode, special characters (interrupt, quit, suspend, and flow
control) retain their effects on the tty driver and calling program.
Calling first raw() then cbreak() leaves the terminal in cbreak mode.
[clinic start generated code]*/

static PyObject *
_curses_cbreak_impl(PyObject *module, int flag)
/*[clinic end generated code: output=9f9dee9664769751 input=c7d0bddda93016c1]*/
NoArgOrFlagNoReturnFunctionBody(cbreak, flag)

/*[clinic input]
_curses.color_content

    color_number: color
        The number of the color (0 - (COLORS-1)).
    /

Return the red, green, and blue (RGB) components of the specified color.

A 3-tuple is returned, containing the R, G, B values for the given color,
which will be between 0 (no component) and 1000 (maximum amount of component).
[clinic start generated code]*/

static PyObject *
_curses_color_content_impl(PyObject *module, int color_number)
/*[clinic end generated code: output=17b466df7054e0de input=03b5ed0472662aea]*/
{
    _CURSES_COLOR_VAL_TYPE r,g,b;

    PyCursesStatefulInitialised(module);
    PyCursesStatefulInitialisedColor(module);

    if (_COLOR_CONTENT_FUNC(color_number, &r, &g, &b) == ERR) {
        cursesmodule_state *state = get_cursesmodule_state(module);
        PyErr_Format(state->error, "%s() returned ERR",
                     Py_STRINGIFY(_COLOR_CONTENT_FUNC));
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
other A_* attributes.  pair_number() is the counterpart to this function.
[clinic start generated code]*/

static PyObject *
_curses_color_pair_impl(PyObject *module, int pair_number)
/*[clinic end generated code: output=60718abb10ce9feb input=6034e9146f343802]*/
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
state is returned; otherwise, an exception is raised.  On many terminals,
the "visible" mode is an underline cursor and the "very visible" mode is
a block cursor.
[clinic start generated code]*/

static PyObject *
_curses_curs_set_impl(PyObject *module, int visibility)
/*[clinic end generated code: output=ee8e62483b1d6cd4 input=81a7924a65d29504]*/
{
    int erg;

    PyCursesStatefulInitialised(module);

    erg = curs_set(visibility);
    if (erg == ERR) return PyCursesCheckERR(module, erg, "curs_set");

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

The "shell" mode is the mode when the running program is not using curses.

Subsequent calls to reset_shell_mode() will restore this mode.
[clinic start generated code]*/

static PyObject *
_curses_def_shell_mode_impl(PyObject *module)
/*[clinic end generated code: output=d6e42f5c768f860f input=5ead21f6f0baa894]*/
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

    return PyCursesCheckERR(module, delay_output(ms), "delay_output");
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

In echo mode, each character input is echoed to the screen as it is entered.
[clinic start generated code]*/

static PyObject *
_curses_echo_impl(PyObject *module, int flag)
/*[clinic end generated code: output=03acb2ddfa6c8729 input=86cd4d5bb1d569c0]*/
NoArgOrFlagNoReturnFunctionBody(echo, flag)

/*[clinic input]
_curses.endwin

De-initialize the library, and return terminal to normal status.
[clinic start generated code]*/

static PyObject *
_curses_endwin_impl(PyObject *module)
/*[clinic end generated code: output=c0150cd96d2f4128 input=e172cfa43062f3fa]*/
NoArgNoReturnFunctionBody(endwin)

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

/*[clinic input]
_curses.flash

Flash the screen.

That is, change it to reverse-video and then change it back in a short interval.
[clinic start generated code]*/

static PyObject *
_curses_flash_impl(PyObject *module)
/*[clinic end generated code: output=488b8a0ebd9ea9b8 input=02fdfb06c8fc3171]*/
NoArgNoReturnFunctionBody(flash)

/*[clinic input]
_curses.flushinp

Flush all input buffers.

This throws away any typeahead that has been typed by the user and has not
yet been processed by the program.
[clinic start generated code]*/

static PyObject *
_curses_flushinp_impl(PyObject *module)
/*[clinic end generated code: output=7e7a1fc1473960f5 input=59d042e705cef5ec]*/
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
        cursesmodule_state *state = get_cursesmodule_state(module);
        PyErr_SetString(state->error, "getmouse() returned ERR");
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
    return PyCursesCheckERR(module, ungetmouse(&event), "ungetmouse");
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
        cursesmodule_state *state = get_cursesmodule_state(module);
        PyErr_SetString(state->error, catchall_NULL);
        goto error;
    }
    cursesmodule_state *state = get_cursesmodule_state(module);
    res = PyCursesWindow_New(state, win, NULL);

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

    return PyCursesCheckERR(module, halfdelay(tenths), "halfdelay");
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
_curses.has_ic

Return True if the terminal has insert- and delete-character capabilities.
[clinic start generated code]*/

static PyObject *
_curses_has_ic_impl(PyObject *module)
/*[clinic end generated code: output=6be24da9cb1268fe input=9bc2d3a797cc7324]*/
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
_curses.has_key

    key: int
        Key number.
    /

Return True if the current terminal type recognizes a key with that value.
[clinic start generated code]*/

static PyObject *
_curses_has_key_impl(PyObject *module, int key)
/*[clinic end generated code: output=19ad48319414d0b1 input=78bd44acf1a4997c]*/
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

    return PyCursesCheckERR(module,
                            _CURSES_INIT_COLOR_FUNC(color_number, r, g, b),
                            Py_STRINGIFY(_CURSES_INIT_COLOR_FUNC));
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

If the color-pair was previously initialized, the screen is refreshed and
all occurrences of that color-pair are changed to the new definition.
[clinic start generated code]*/

static PyObject *
_curses_init_pair_impl(PyObject *module, int pair_number, int fg, int bg)
/*[clinic end generated code: output=a0bba03d2bbc3ee6 input=54b421b44c12c389]*/
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
            cursesmodule_state *state = get_cursesmodule_state(module);
            PyErr_Format(state->error, "%s() returned ERR",
                         Py_STRINGIFY(_CURSES_INIT_PAIR_FUNC));
        }
        return NULL;
    }

    Py_RETURN_NONE;
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
        wrefresh(stdscr);
        cursesmodule_state *state = get_cursesmodule_state(module);
        return PyCursesWindow_New(state, stdscr, NULL);
    }

    win = initscr();

    if (win == NULL) {
        cursesmodule_state *state = get_cursesmodule_state(module);
        PyErr_SetString(state->error, catchall_NULL);
        return NULL;
    }

    curses_initscr_called = curses_setupterm_called = TRUE;

    PyObject *module_dict = PyModule_GetDict(module); // borrowed
    if (module_dict == NULL) {
        return NULL;
    }
    /* This was moved from initcurses() because it core dumped on SGI,
       where they're not defined until you've called initscr() */
#define SetDictInt(NAME, VALUE)                                     \
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

    cursesmodule_state *state = get_cursesmodule_state(module);
    PyObject *winobj = PyCursesWindow_New(state, win, NULL);
    if (winobj == NULL) {
        return NULL;
    }
    curses_screen_encoding = ((PyCursesWindowObject *)winobj)->encoding;
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

        if (_PySys_GetOptionalAttrString("stdout", &sys_stdout) < 0) {
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

#if defined(NCURSES_EXT_FUNCS) && NCURSES_EXT_FUNCS >= 20081102
// https://invisible-island.net/ncurses/NEWS.html#index-t20080119

/*[clinic input]
_curses.get_escdelay

Gets the curses ESCDELAY setting.

Gets the number of milliseconds to wait after reading an escape character,
to distinguish between an individual escape character entered on the
keyboard from escape sequences sent by cursor and function keys.
[clinic start generated code]*/

static PyObject *
_curses_get_escdelay_impl(PyObject *module)
/*[clinic end generated code: output=222fa1a822555d60 input=be2d5b3dd974d0a4]*/
{
    return PyLong_FromLong(ESCDELAY);
}
/*[clinic input]
_curses.set_escdelay
    ms: int
        length of the delay in milliseconds.
    /

Sets the curses ESCDELAY setting.

Sets the number of milliseconds to wait after reading an escape character,
to distinguish between an individual escape character entered on the
keyboard from escape sequences sent by cursor and function keys.
[clinic start generated code]*/

static PyObject *
_curses_set_escdelay_impl(PyObject *module, int ms)
/*[clinic end generated code: output=43818efbf7980ac4 input=7796fe19f111e250]*/
{
    if (ms <= 0) {
        PyErr_SetString(PyExc_ValueError, "ms must be > 0");
        return NULL;
    }

    return PyCursesCheckERR(module, set_escdelay(ms), "set_escdelay");
}

/*[clinic input]
_curses.get_tabsize

Gets the curses TABSIZE setting.

Gets the number of columns used by the curses library when converting a tab
character to spaces as it adds the tab to a window.
[clinic start generated code]*/

static PyObject *
_curses_get_tabsize_impl(PyObject *module)
/*[clinic end generated code: output=7e9e51fb6126fbdf input=74af86bf6c9f5d7e]*/
{
    return PyLong_FromLong(TABSIZE);
}
/*[clinic input]
_curses.set_tabsize
    size: int
        rendered cell width of a tab character.
    /

Sets the curses TABSIZE setting.

Sets the number of columns used by the curses library when converting a tab
character to spaces as it adds the tab to a window.
[clinic start generated code]*/

static PyObject *
_curses_set_tabsize_impl(PyObject *module, int size)
/*[clinic end generated code: output=c1de5a76c0daab1e input=78cba6a3021ad061]*/
{
    if (size <= 0) {
        PyErr_SetString(PyExc_ValueError, "size must be > 0");
        return NULL;
    }

    return PyCursesCheckERR(module, set_tabsize(size), "set_tabsize");
}
#endif

/*[clinic input]
_curses.intrflush

    flag: bool
    /

[clinic start generated code]*/

static PyObject *
_curses_intrflush_impl(PyObject *module, int flag)
/*[clinic end generated code: output=c1986df35e999a0f input=c65fe2ef973fe40a]*/
{
    PyCursesStatefulInitialised(module);

    return PyCursesCheckERR(module, intrflush(NULL, flag), "intrflush");
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
/*[clinic end generated code: output=aafe04afe50f1288 input=ca9c0bd0fb8ab444]*/
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

/*[clinic input]
_curses.longname

Return the terminfo long name field describing the current terminal.

The maximum length of a verbose description is 128 characters.  It is defined
only after the call to initscr().
[clinic start generated code]*/

static PyObject *
_curses_longname_impl(PyObject *module)
/*[clinic end generated code: output=fdf30433727ef568 input=84c3f20201b1098e]*/
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

    return PyCursesCheckERR(module, meta(stdscr, yes), "meta");
}

#ifdef NCURSES_MOUSE_VERSION
/*[clinic input]
_curses.mouseinterval

    interval: int
        Time in milliseconds.
    /

Set and retrieve the maximum time between press and release in a click.

Set the maximum time that can elapse between press and release events in
order for them to be recognized as a click, and return the previous interval
value.
[clinic start generated code]*/

static PyObject *
_curses_mouseinterval_impl(PyObject *module, int interval)
/*[clinic end generated code: output=c4f5ff04354634c5 input=75aaa3f0db10ac4e]*/
{
    PyCursesStatefulInitialised(module);

    return PyCursesCheckERR(module, mouseinterval(interval), "mouseinterval");
}

/*[clinic input]
_curses.mousemask

    newmask: unsigned_long(bitwise=True)
    /

Set the mouse events to be reported, and return a tuple (availmask, oldmask).

Return a tuple (availmask, oldmask).  availmask indicates which of the
specified mouse events can be reported; on complete failure it returns 0.
oldmask is the previous value of the given window's mouse event mask.
If this function is never called, no mouse events are ever reported.
[clinic start generated code]*/

static PyObject *
_curses_mousemask_impl(PyObject *module, unsigned long newmask)
/*[clinic end generated code: output=9406cf1b8a36e485 input=bdf76b7568a3c541]*/
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
        cursesmodule_state *state = get_cursesmodule_state(module);
        PyErr_SetString(state->error, catchall_NULL);
        return NULL;
    }

    cursesmodule_state *state = get_cursesmodule_state(module);
    return PyCursesWindow_New(state, win, NULL);
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

By default, the window will extend from the specified position to the lower
right corner of the screen.
[clinic start generated code]*/

static PyObject *
_curses_newwin_impl(PyObject *module, int nlines, int ncols,
                    int group_right_1, int begin_y, int begin_x)
/*[clinic end generated code: output=c1e0a8dc8ac2826c input=29312c15a72a003d]*/
{
    WINDOW *win;

    PyCursesStatefulInitialised(module);

    win = newwin(nlines,ncols,begin_y,begin_x);
    if (win == NULL) {
        cursesmodule_state *state = get_cursesmodule_state(module);
        PyErr_SetString(state->error, catchall_NULL);
        return NULL;
    }

    cursesmodule_state *state = get_cursesmodule_state(module);
    return PyCursesWindow_New(state, win, NULL);
}

/*[clinic input]
_curses.nl

    flag: bool = True
        If false, the effect is the same as calling nonl().
    /

Enter newline mode.

This mode translates the return key into newline on input, and translates
newline into return and line-feed on output.  Newline mode is initially on.
[clinic start generated code]*/

static PyObject *
_curses_nl_impl(PyObject *module, int flag)
/*[clinic end generated code: output=b39cc0ffc9015003 input=18e3e9c6e8cfcf6f]*/
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

Disable translation of return into newline on input, and disable low-level
translation of newline into newline/return on output.
[clinic start generated code]*/

static PyObject *
_curses_nonl_impl(PyObject *module)
/*[clinic end generated code: output=99e917e9715770c6 input=9d37dd122d3022fc]*/
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
_curses.pair_content

    pair_number: pair
        The number of the color pair (0 - (COLOR_PAIRS-1)).
    /

Return a tuple (fg, bg) containing the colors for the requested color pair.
[clinic start generated code]*/

static PyObject *
_curses_pair_content_impl(PyObject *module, int pair_number)
/*[clinic end generated code: output=4a726dd0e6885f3f input=03970f840fc7b739]*/
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
            cursesmodule_state *state = get_cursesmodule_state(module);
            PyErr_Format(state->error, "%s() returned ERR",
                         Py_STRINGIFY(_CURSES_PAIR_CONTENT_FUNC));
        }
        return NULL;
    }

    return Py_BuildValue("(ii)", f, b);
}

/*[clinic input]
_curses.pair_number

    attr: int
    /

Return the number of the color-pair set by the specified attribute value.

color_pair() is the counterpart to this function.
[clinic start generated code]*/

static PyObject *
_curses_pair_number_impl(PyObject *module, int attr)
/*[clinic end generated code: output=85bce7d65c0aa3f4 input=d478548e33f5e61a]*/
{
    PyCursesStatefulInitialised(module);
    PyCursesStatefulInitialisedColor(module);

    return PyLong_FromLong(PAIR_NUMBER(attr));
}

/*[clinic input]
_curses.putp

    string: str(accept={robuffer})
    /

Emit the value of a specified terminfo capability for the current terminal.

Note that the output of putp() always goes to standard output.
[clinic start generated code]*/

static PyObject *
_curses_putp_impl(PyObject *module, const char *string)
/*[clinic end generated code: output=e98081d1b8eb5816 input=1601faa828b44cb3]*/
{
    return PyCursesCheckERR(module, putp(string), "putp");
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

[clinic start generated code]*/

static PyObject *
_curses_update_lines_cols_impl(PyObject *module)
/*[clinic end generated code: output=423f2b1e63ed0f75 input=5f065ab7a28a5d90]*/
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
suspend, and flow control keys are turned off; characters are presented to
curses input functions one by one.
[clinic start generated code]*/

static PyObject *
_curses_raw_impl(PyObject *module, int flag)
/*[clinic end generated code: output=a750e4b342be015b input=4b447701389fb4df]*/
NoArgOrFlagNoReturnFunctionBody(raw, flag)

/*[clinic input]
_curses.reset_prog_mode

Restore the terminal to "program" mode, as previously saved by def_prog_mode().
[clinic start generated code]*/

static PyObject *
_curses_reset_prog_mode_impl(PyObject *module)
/*[clinic end generated code: output=15eb765abf0b6575 input=3d82bea2b3243471]*/
NoArgNoReturnFunctionBody(reset_prog_mode)

/*[clinic input]
_curses.reset_shell_mode

Restore the terminal to "shell" mode, as previously saved by def_shell_mode().
[clinic start generated code]*/

static PyObject *
_curses_reset_shell_mode_impl(PyObject *module)
/*[clinic end generated code: output=0238de2962090d33 input=1c738fa64bd1a24f]*/
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

Adjusts other bookkeeping data used by the curses library that record the
window dimensions (in particular the SIGWINCH handler).
[clinic start generated code]*/

static PyObject *
_curses_resizeterm_impl(PyObject *module, short nlines, short ncols)
/*[clinic end generated code: output=4de3abab50c67f02 input=414e92a63e3e9899]*/
{
    PyObject *result;

    PyCursesStatefulInitialised(module);

    result = PyCursesCheckERR(module, resizeterm(nlines, ncols), "resizeterm");
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
extended.  The calling application should fill in these areas with appropriate
data.  The resize_term() function attempts to resize all windows.  However,
due to the calling convention of pads, it is not possible to resize these
without additional interaction with the application.
[clinic start generated code]*/

static PyObject *
_curses_resize_term_impl(PyObject *module, short nlines, short ncols)
/*[clinic end generated code: output=46c6d749fa291dbd input=276afa43d8ea7091]*/
{
    PyObject *result;

    PyCursesStatefulInitialised(module);

    result = PyCursesCheckERR(module, resize_term(nlines, ncols), "resize_term");
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
_curses.start_color

Initializes eight basic colors and global variables COLORS and COLOR_PAIRS.

Must be called if the programmer wants to use colors, and before any other
color manipulation routine is called.  It is good practice to call this
routine right after initscr().

It also restores the colors on the terminal to the values they had when the
terminal was just turned on.
[clinic start generated code]*/

static PyObject *
_curses_start_color_impl(PyObject *module)
/*[clinic end generated code: output=8b772b41d8090ede input=0ca0ecb2b77e1a12]*/
{
    PyCursesStatefulInitialised(module);

    if (start_color() == ERR) {
        cursesmodule_state *state = get_cursesmodule_state(module);
        PyErr_SetString(state->error, "start_color() returned ERR");
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
_curses.termname

Return the value of the environment variable TERM, truncated to 14 characters.
[clinic start generated code]*/

static PyObject *
_curses_termname_impl(PyObject *module)
/*[clinic end generated code: output=96375577ebbd67fd input=33c08d000944f33f]*/
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

The value -2 is returned if capname is not a numeric capability, or -1 if
it is canceled or absent from the terminal description.
[clinic start generated code]*/

static PyObject *
_curses_tigetnum_impl(PyObject *module, const char *capname)
/*[clinic end generated code: output=46f8b0a1b5dff42f input=5cdf2f410b109720]*/
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

None is returned if capname is not a string capability, or is canceled or
absent from the terminal description.
[clinic start generated code]*/

static PyObject *
_curses_tigetstr_impl(PyObject *module, const char *capname)
/*[clinic end generated code: output=f22b576ad60248f3 input=36644df25c73c0a7]*/
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
        cursesmodule_state *state = get_cursesmodule_state(module);
        PyErr_SetString(state->error, "tparm() returned NULL");
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

    return PyCursesCheckERR(module, typeahead( fd ), "typeahead");
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

    return PyBytes_FromString(unctrl(ch_));
}

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

    return PyCursesCheckERR(module, ungetch(ch_), "ungetch");
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
    return PyCursesCheckERR(module, unget_wch(wch), "unget_wch");
}
#endif

#ifdef HAVE_CURSES_USE_ENV
/*[clinic input]
_curses.use_env

    flag: bool
    /

Use environment variables LINES and COLUMNS.

If used, this function should be called before initscr() or newterm() are
called.

When flag is False, the values of lines and columns specified in the terminfo
database will be used, even if environment variables LINES and COLUMNS (used
by default) are set, or if curses is running in a window (in which case
default behavior would be to use the window size if LINES and COLUMNS are
not set).
[clinic start generated code]*/

static PyObject *
_curses_use_env_impl(PyObject *module, int flag)
/*[clinic end generated code: output=b2c445e435c0b164 input=06ac30948f2d78e4]*/
{
    use_env(flag);
    Py_RETURN_NONE;
}
#endif

#ifndef STRICT_SYSV_CURSES
/*[clinic input]
_curses.use_default_colors

Allow use of default values for colors on terminals supporting this feature.

Use this to support transparency in your application.  The default color
is assigned to the color number -1.
[clinic start generated code]*/

static PyObject *
_curses_use_default_colors_impl(PyObject *module)
/*[clinic end generated code: output=a3b81ff71dd901be input=656844367470e8fc]*/
{
    int code;

    PyCursesStatefulInitialised(module);
    PyCursesStatefulInitialisedColor(module);

    code = use_default_colors();
    if (code != ERR) {
        Py_RETURN_NONE;
    } else {
        cursesmodule_state *state = get_cursesmodule_state(module);
        PyErr_SetString(state->error, "use_default_colors() returned ERR");
        return NULL;
    }
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
_curses.has_extended_color_support

Return True if the module supports extended colors; otherwise, return False.

Extended color support allows more than 256 color-pairs for terminals
that support more than 16 colors (e.g. xterm-256color).
[clinic start generated code]*/

static PyObject *
_curses_has_extended_color_support_impl(PyObject *module)
/*[clinic end generated code: output=68f1be2b57d92e22 input=4b905f046e35ee9f]*/
{
    return PyBool_FromLong(_NCURSES_EXTENDED_COLOR_FUNCS);
}

/* List of functions defined in the module */

static PyMethodDef cursesmodule_methods[] = {
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
    _CURSES_FILTER_METHODDEF
    _CURSES_FLASH_METHODDEF
    _CURSES_FLUSHINP_METHODDEF
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
    _CURSES_ISENDWIN_METHODDEF
    _CURSES_IS_TERM_RESIZED_METHODDEF
    _CURSES_KEYNAME_METHODDEF
    _CURSES_KILLCHAR_METHODDEF
    _CURSES_LONGNAME_METHODDEF
    _CURSES_META_METHODDEF
    _CURSES_MOUSEINTERVAL_METHODDEF
    _CURSES_MOUSEMASK_METHODDEF
    _CURSES_NAPMS_METHODDEF
    _CURSES_NEWPAD_METHODDEF
    _CURSES_NEWWIN_METHODDEF
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
    _CURSES_UNGETCH_METHODDEF
    _CURSES_UPDATE_LINES_COLS_METHODDEF
    _CURSES_UNGET_WCH_METHODDEF
    _CURSES_USE_ENV_METHODDEF
    _CURSES_USE_DEFAULT_COLORS_METHODDEF
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
    return 0;
}

static int
cursesmodule_clear(PyObject *mod)
{
    cursesmodule_state *state = get_cursesmodule_state(mod);
    Py_CLEAR(state->error);
    Py_CLEAR(state->window_type);
    return 0;
}

static void
cursesmodule_free(void *mod)
{
    (void)cursesmodule_clear((PyObject *)mod);
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
    state->error = PyErr_NewException("_curses.error", NULL, NULL);
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
