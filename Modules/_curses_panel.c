/*
 *   Interface to the ncurses panel library
 *
 * Original version by Thomas Gellekum
 */

/* Release Number */

static const char PyCursesVersion[] = "2.1";

/* Includes */

// clinic/_curses_panel.c.h uses internal pycore_modsupport.h API
#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"

#define CURSES_PANEL_MODULE
#include "py_curses.h"

#if defined(HAVE_NCURSESW_PANEL_H)
#  include <ncursesw/panel.h>
#elif defined(HAVE_NCURSES_PANEL_H)
#  include <ncurses/panel.h>
#elif defined(HAVE_PANEL_H)
#  include <panel.h>
#endif

typedef struct {
    PyObject *error;
    PyTypeObject *PyCursesPanel_Type;
} _curses_panel_state;

typedef struct PyCursesPanelObject PyCursesPanelObject;

static inline _curses_panel_state *
get_curses_panel_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (_curses_panel_state *)state;
}

static inline _curses_panel_state *
get_curses_panel_state_by_panel(PyCursesPanelObject *panel)
{
    /*
     * Note: 'state' may be NULL if Py_TYPE(panel) is not a heap
     * type associated with this module, but the compiler would
     * have likely already complained with an "invalid pointer
     * type" at compile-time.
     *
     * To make it more robust, all functions recovering a module's
     * state from an object should expect to return NULL with an
     * exception set (in contrast to functions recovering a module's
     * state from a module itself).
     */
    void *state = PyType_GetModuleState(Py_TYPE(panel));
    assert(state != NULL);
    return (_curses_panel_state *)state;
}

static int
_curses_panel_clear(PyObject *mod)
{
    _curses_panel_state *state = get_curses_panel_state(mod);
    Py_CLEAR(state->error);
    Py_CLEAR(state->PyCursesPanel_Type);
    return 0;
}

static int
_curses_panel_traverse(PyObject *mod, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(mod));
    _curses_panel_state *state = get_curses_panel_state(mod);
    Py_VISIT(state->error);
    Py_VISIT(state->PyCursesPanel_Type);
    return 0;
}

static void
_curses_panel_free(void *mod)
{
    (void)_curses_panel_clear((PyObject *)mod);
}

/* Utility Error Procedures
 *
 * The naming and implementations are identical to those in _cursesmodule.c.
 * Functions that are not yet needed (for instance, reporting an ERR value
 * from a module-wide function, namely curses_panel_set_error()) are
 * omitted and should only be added if needed.
 */

static void
_curses_panel_format_error(_curses_panel_state *state,
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
_curses_panel_set_error(_curses_panel_state *state,
                        const char *curses_funcname,
                        const char *python_funcname)
{
    _curses_panel_format_error(state, curses_funcname, python_funcname,
                               "ERR", catchall_ERR);
}

/*
 * Format a curses error for a function that returned NULL.
 *
 * Specify a non-NULL 'python_funcname' when the latter differs from
 * 'curses_funcname'. If both names are NULL, uses the 'catchall_NULL'
 * message instead.
 */
static void
_curses_panel_set_null_error(_curses_panel_state *state,
                             const char *curses_funcname,
                             const char *python_funcname)
{
    _curses_panel_format_error(state, curses_funcname, python_funcname,
                               "NULL", catchall_NULL);
}

/* Same as _curses_panel_set_null_error() for a module object. */
static void
curses_panel_set_null_error(PyObject *module,
                            const char *curses_funcname,
                            const char *python_funcname)
{
    _curses_panel_state *state = get_curses_panel_state(module);
    _curses_panel_set_null_error(state, curses_funcname, python_funcname);
}

/* Same as _curses_panel_set_error() for a panel object. */
static void
curses_panel_panel_set_error(PyCursesPanelObject *panel,
                             const char *curses_funcname,
                             const char *python_funcname)
{
    _curses_panel_state *state = get_curses_panel_state_by_panel(panel);
    _curses_panel_set_error(state, curses_funcname, python_funcname);
}

/* Same as _curses_panel_set_null_error() for a panel object. */
static void
curses_panel_panel_set_null_error(PyCursesPanelObject *panel,
                                  const char *curses_funcname,
                                  const char *python_funcname)
{
    _curses_panel_state *state = get_curses_panel_state_by_panel(panel);
    _curses_panel_set_null_error(state, curses_funcname, python_funcname);
}

/*
 * Indicate that a panel object couldn't be found.
 *
 * Use it for the following constructions:
 *
 * PROC caller_funcname:
 *  pan = called_funcname()
 *  find_po(panel)
 *
 * PROC caller_funcname:
 *  find_po(self->pan)
*/
static void
curses_panel_notfound_error(const char *called_funcname,
                            const char *caller_funcname)
{
    assert(!(called_funcname == NULL && caller_funcname == NULL));
    if (caller_funcname == NULL) {
        (void)PyErr_Format(PyExc_RuntimeError,
                           "%s(): cannot find panel object",
                           called_funcname);
    }
    else {
        (void)PyErr_Format(PyExc_RuntimeError,
                           "%s() (called by %s()): cannot find panel object",
                           called_funcname, caller_funcname);
    }
}

/* Utility Functions */

/*
 * Check the return code from a curses function, returning None
 * on success and setting an exception on error.
 */

/*
 * Return None if 'code' is different from ERR (implementation-defined).
 * Otherwise, set an exception using curses_panel_panel_set_error() and
 * the remaining arguments, and return NULL.
 */
static PyObject *
curses_panel_panel_check_err(PyCursesPanelObject *panel, int code,
                             const char *curses_funcname,
                             const char *python_funcname)
{
    if (code != ERR) {
        Py_RETURN_NONE;
    }
    curses_panel_panel_set_error(panel, curses_funcname, python_funcname);
    return NULL;
}

/*****************************************************************************
 The Panel Object
******************************************************************************/

/* Definition of the panel object and panel type */

typedef struct PyCursesPanelObject {
    PyObject_HEAD
    PANEL *pan;
    PyCursesWindowObject *wo;   /* for reference counts */
} PyCursesPanelObject;

#define _PyCursesPanelObject_CAST(op)   ((PyCursesPanelObject *)(op))

/* Some helper functions. The problem is that there's always a window
   associated with a panel. To ensure that Python's GC doesn't pull
   this window from under our feet we need to keep track of references
   to the corresponding window object within Python. We can't use
   dupwin(oldwin) to keep a copy of the curses WINDOW because the
   contents of oldwin is copied only once; code like

   win = newwin(...)
   pan = win.panel()
   win.addstr(some_string)
   pan.window().addstr(other_string)

   will fail. */

/* We keep a linked list of PyCursesPanelObjects, lop. A list should
   suffice, I don't expect more than a handful or at most a few
   dozens of panel objects within a typical program. */
typedef struct _list_of_panels {
    PyCursesPanelObject *po;
    struct _list_of_panels *next;
} list_of_panels;

/* list anchor */
static list_of_panels *lop;

/* Insert a new panel object into lop */
static int
insert_lop(PyCursesPanelObject *po)
{
    list_of_panels *new;

    if ((new = (list_of_panels *)PyMem_Malloc(sizeof(list_of_panels))) == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    new->po = po;
    new->next = lop;
    lop = new;
    return 0;
}

/* Remove the panel object from lop.
 *
 * Return -1 on error but do NOT set an exception; otherwise return 0.
 */
static int
remove_lop(PyCursesPanelObject *po)
{
    list_of_panels *temp, *n;

    temp = lop;
    if (temp->po == po) {
        lop = temp->next;
        PyMem_Free(temp);
        return 0;
    }
    while (temp->next == NULL || temp->next->po != po) {
        if (temp->next == NULL) {
            return -1;
        }
        temp = temp->next;
    }
    n = temp->next->next;
    PyMem_Free(temp->next);
    temp->next = n;
    return 0;
}

/* Return the panel object that corresponds to pan */
static PyCursesPanelObject *
find_po_impl(PANEL *pan)
{
    list_of_panels *temp;
    for (temp = lop; temp->po->pan != pan; temp = temp->next)
        if (temp->next == NULL) return NULL;    /* not found!? */
    return temp->po;
}

/* Same as find_po_impl() but with caller context information. */
static PyCursesPanelObject *
find_po(PANEL *pan, const char *called_funcname, const char *caller_funcname)
{
    PyCursesPanelObject *res = find_po_impl(pan);
    if (res == NULL) {
        curses_panel_notfound_error(called_funcname, caller_funcname);
    }
    return res;
}

/*[clinic input]
module _curses_panel
class _curses_panel.panel "PyCursesPanelObject *" "&PyCursesPanel_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=2f4ef263ca850a31]*/

#include "clinic/_curses_panel.c.h"

/* ------------- PANEL routines --------------- */

/*[clinic input]
_curses_panel.panel.bottom

Push the panel to the bottom of the stack.
[clinic start generated code]*/

static PyObject *
_curses_panel_panel_bottom_impl(PyCursesPanelObject *self)
/*[clinic end generated code: output=7aa7d14d7e1d1ce6 input=b6c920c071b61e2e]*/
{
    int rtn = bottom_panel(self->pan);
    return curses_panel_panel_check_err(self, rtn, "bottom_panel", "bottom");
}

/*[clinic input]
@permit_long_docstring_body
_curses_panel.panel.hide

Hide the panel.

This does not delete the object, it just makes the window on screen invisible.
[clinic start generated code]*/

static PyObject *
_curses_panel_panel_hide_impl(PyCursesPanelObject *self)
/*[clinic end generated code: output=a7bbbd523e1eab49 input=9071b463a39a1a6a]*/
{
    int rtn = hide_panel(self->pan);
    return curses_panel_panel_check_err(self, rtn, "hide_panel", "hide");
}

/*[clinic input]
_curses_panel.panel.show

Display the panel (which might have been hidden).
[clinic start generated code]*/

static PyObject *
_curses_panel_panel_show_impl(PyCursesPanelObject *self)
/*[clinic end generated code: output=6b4553ab45c97769 input=57b167bbefaa3755]*/
{
    int rtn = show_panel(self->pan);
    return curses_panel_panel_check_err(self, rtn, "show_panel", "show");
}

/*[clinic input]
_curses_panel.panel.top

Push panel to the top of the stack.
[clinic start generated code]*/

static PyObject *
_curses_panel_panel_top_impl(PyCursesPanelObject *self)
/*[clinic end generated code: output=0f5f2f8cdd2d1777 input=be33975ec3ca0e9a]*/
{
    int rtn = top_panel(self->pan);
    return curses_panel_panel_check_err(self, rtn, "top_panel", "top");
}

/* Allocation and deallocation of Panel Objects */

static PyObject *
PyCursesPanel_New(_curses_panel_state *state, PANEL *pan,
                  PyCursesWindowObject *wo)
{
    assert(state != NULL);
    PyTypeObject *type = state->PyCursesPanel_Type;
    assert(type != NULL);
    assert(type->tp_alloc != NULL);
    PyCursesPanelObject *po = (PyCursesPanelObject *)type->tp_alloc(type, 0);
    if (po == NULL) {
        return NULL;
    }

    po->pan = pan;
    if (insert_lop(po) < 0) {
        po->wo = NULL;
        Py_DECREF(po);
        return NULL;
    }
    po->wo = (PyCursesWindowObject*)Py_NewRef(wo);
    return (PyObject *)po;
}

static int
PyCursesPanel_Clear(PyObject *op)
{
    PyCursesPanelObject *self = _PyCursesPanelObject_CAST(op);
    PyObject *extra = (PyObject *)panel_userptr(self->pan);
    if (extra != NULL) {
        Py_DECREF(extra);
        if (set_panel_userptr(self->pan, NULL) == ERR) {
            curses_panel_panel_set_error(self, "set_panel_userptr", NULL);
            return -1;
        }
    }
    // self->wo should not be cleared because an associated WINDOW may exist
    return 0;
}

static void
PyCursesPanel_Dealloc(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);

    PyCursesPanelObject *po = _PyCursesPanelObject_CAST(self);
    if (PyCursesPanel_Clear(self) < 0) {
        PyErr_FormatUnraisable("Exception ignored in PyCursesPanel_Dealloc()");
    }
    if (del_panel(po->pan) == ERR && !PyErr_Occurred()) {
        curses_panel_panel_set_error(po, "del_panel", "__del__");
        PyErr_FormatUnraisable("Exception ignored in PyCursesPanel_Dealloc()");
    }
    if (po->wo != NULL) {
        Py_DECREF(po->wo);
        if (remove_lop(po) < 0) {
            PyErr_SetString(PyExc_RuntimeError, "__del__: no panel object to delete");
            PyErr_FormatUnraisable("Exception ignored in PyCursesPanel_Dealloc()");
        }
    }
    tp->tp_free(po);
    Py_DECREF(tp);
}

static int
PyCursesPanel_Traverse(PyObject *op, visitproc visit, void *arg)
{
    PyCursesPanelObject *self = _PyCursesPanelObject_CAST(op);
    Py_VISIT(Py_TYPE(op));
    Py_VISIT(panel_userptr(self->pan));
    Py_VISIT(self->wo);
    return 0;
}

/* panel_above(NULL) returns the bottom panel in the stack. To get
   this behaviour we use curses.panel.bottom_panel(). */
/*[clinic input]
_curses_panel.panel.above

Return the panel above the current panel.
[clinic start generated code]*/

static PyObject *
_curses_panel_panel_above_impl(PyCursesPanelObject *self)
/*[clinic end generated code: output=70ac06d25fd3b4da input=c059994022976788]*/
{
    PANEL *pan;
    PyCursesPanelObject *po;

    pan = panel_above(self->pan);
    if (pan == NULL) {  /* valid output: it means no panel exists yet */
        Py_RETURN_NONE;
    }
    po = find_po(pan, "panel_above", "above");
    return Py_XNewRef(po);
}

/* panel_below(NULL) returns the top panel in the stack. To get
   this behaviour we use curses.panel.top_panel(). */
/*[clinic input]
_curses_panel.panel.below

Return the panel below the current panel.
[clinic start generated code]*/

static PyObject *
_curses_panel_panel_below_impl(PyCursesPanelObject *self)
/*[clinic end generated code: output=282861122e06e3de input=cc08f61936d297c6]*/
{
    PANEL *pan;
    PyCursesPanelObject *po;

    pan = panel_below(self->pan);
    if (pan == NULL) {  /* valid output: it means no panel exists yet */
        Py_RETURN_NONE;
    }
    po = find_po(pan, "panel_below", "below");
    return Py_XNewRef(po);
}

/*[clinic input]
_curses_panel.panel.hidden

Return True if the panel is hidden (not visible), False otherwise.
[clinic start generated code]*/

static PyObject *
_curses_panel_panel_hidden_impl(PyCursesPanelObject *self)
/*[clinic end generated code: output=66eebd1ab4501a71 input=453d4b4fce25e21a]*/
{
    if (panel_hidden(self->pan))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

/*[clinic input]
_curses_panel.panel.move

    y: int
    x: int
    /

Move the panel to the screen coordinates (y, x).
[clinic start generated code]*/

static PyObject *
_curses_panel_panel_move_impl(PyCursesPanelObject *self, int y, int x)
/*[clinic end generated code: output=d867535a89777415 input=e0b36b78acc03fba]*/
{
    int rtn = move_panel(self->pan, y, x);
    return curses_panel_panel_check_err(self, rtn, "move_panel", "move");
}

/*[clinic input]
_curses_panel.panel.window

Return the window object associated with the panel.
[clinic start generated code]*/

static PyObject *
_curses_panel_panel_window_impl(PyCursesPanelObject *self)
/*[clinic end generated code: output=5f05940d4106b4cb input=6067353d2c307901]*/
{
    return Py_NewRef(self->wo);
}

/*[clinic input]
_curses_panel.panel.replace

    win: object(type="PyCursesWindowObject *", subclass_of="&PyCursesWindow_Type")
    /

Change the window associated with the panel to the window win.
[clinic start generated code]*/

static PyObject *
_curses_panel_panel_replace_impl(PyCursesPanelObject *self,
                                 PyCursesWindowObject *win)
/*[clinic end generated code: output=2253a95f7b287255 input=4b1c4283987d9dfa]*/
{
    PyCursesPanelObject *po = find_po(self->pan, "replace", NULL);
    if (po == NULL) {
        return NULL;
    }

    int rtn = replace_panel(self->pan, win->win);
    if (rtn == ERR) {
        curses_panel_panel_set_error(self, "replace_panel", "replace");
        return NULL;
    }
    Py_SETREF(po->wo, (PyCursesWindowObject*)Py_NewRef(win));
    Py_RETURN_NONE;
}

/*[clinic input]
_curses_panel.panel.set_userptr

    obj: object
    /

Set the panel's user pointer to obj.
[clinic start generated code]*/

static PyObject *
_curses_panel_panel_set_userptr_impl(PyCursesPanelObject *self,
                                     PyObject *obj)
/*[clinic end generated code: output=7fa1fd23f69db71e input=d2c6a9dbefabbf39]*/
{
    PyCursesInitialised;
    Py_INCREF(obj);
    PyObject *oldobj = (PyObject *) panel_userptr(self->pan);
    int rc = set_panel_userptr(self->pan, (void*)obj);
    if (rc == ERR) {
        /* In case of an ncurses error, decref the new object again */
        Py_DECREF(obj);
        curses_panel_panel_set_error(self, "set_panel_userptr", "set_userptr");
        return NULL;
    }
    Py_XDECREF(oldobj);
    Py_RETURN_NONE;
}

/*[clinic input]
_curses_panel.panel.userptr

Return the user pointer for the panel.
[clinic start generated code]*/

static PyObject *
_curses_panel_panel_userptr_impl(PyCursesPanelObject *self)
/*[clinic end generated code: output=e849c307b5dc9237 input=f78b7a47aef0fd50]*/
{
    PyCursesInitialised;
    PyObject *obj = (PyObject *) panel_userptr(self->pan);
    if (obj == NULL) {
        curses_panel_panel_set_null_error(self, "panel_userptr", "userptr");
        return NULL;
    }

    return Py_NewRef(obj);
}


/* Module interface */

static PyMethodDef PyCursesPanel_Methods[] = {
    _CURSES_PANEL_PANEL_ABOVE_METHODDEF
    _CURSES_PANEL_PANEL_BELOW_METHODDEF
    _CURSES_PANEL_PANEL_BOTTOM_METHODDEF
    _CURSES_PANEL_PANEL_HIDDEN_METHODDEF
    _CURSES_PANEL_PANEL_HIDE_METHODDEF
    _CURSES_PANEL_PANEL_MOVE_METHODDEF
    _CURSES_PANEL_PANEL_REPLACE_METHODDEF
    _CURSES_PANEL_PANEL_SET_USERPTR_METHODDEF
    _CURSES_PANEL_PANEL_SHOW_METHODDEF
    _CURSES_PANEL_PANEL_TOP_METHODDEF
    _CURSES_PANEL_PANEL_USERPTR_METHODDEF
    _CURSES_PANEL_PANEL_WINDOW_METHODDEF
    {NULL,              NULL}   /* sentinel */
};

/* -------------------------------------------------------*/

static PyType_Slot PyCursesPanel_Type_slots[] = {
    {Py_tp_clear, PyCursesPanel_Clear},
    {Py_tp_dealloc, PyCursesPanel_Dealloc},
    {Py_tp_traverse, PyCursesPanel_Traverse},
    {Py_tp_methods, PyCursesPanel_Methods},
    {0, 0},
};

static PyType_Spec PyCursesPanel_Type_spec = {
    .name = "_curses_panel.panel",
    .basicsize = sizeof(PyCursesPanelObject),
    .flags = (
        Py_TPFLAGS_DEFAULT
        | Py_TPFLAGS_DISALLOW_INSTANTIATION
        | Py_TPFLAGS_IMMUTABLETYPE
        | Py_TPFLAGS_HAVE_GC
    ),
    .slots = PyCursesPanel_Type_slots
};

/* Wrapper for panel_above(NULL). This function returns the bottom
   panel of the stack, so it's renamed to bottom_panel().
   panel.above() *requires* a panel object in the first place which
   may be undesirable. */
/*[clinic input]
_curses_panel.bottom_panel

Return the bottom panel in the panel stack.
[clinic start generated code]*/

static PyObject *
_curses_panel_bottom_panel_impl(PyObject *module)
/*[clinic end generated code: output=3aba9f985f4c2bd0 input=634c2a8078b3d7e4]*/
{
    PANEL *pan;
    PyCursesPanelObject *po;

    PyCursesInitialised;

    pan = panel_above(NULL);
    if (pan == NULL) {  /* valid output: it means no panel exists yet */
        Py_RETURN_NONE;
    }
    po = find_po(pan, "panel_above", "bottom_panel");
    return Py_XNewRef(po);
}

/*[clinic input]
_curses_panel.new_panel

    win: object(type="PyCursesWindowObject *", subclass_of="&PyCursesWindow_Type")
    /

Return a panel object, associating it with the given window win.
[clinic start generated code]*/

static PyObject *
_curses_panel_new_panel_impl(PyObject *module, PyCursesWindowObject *win)
/*[clinic end generated code: output=45e948e0176a9bd2 input=74d4754e0ebe4800]*/
{
    PANEL *pan = new_panel(win->win);
    if (pan == NULL) {
        curses_panel_set_null_error(module, "new_panel", NULL);
        return NULL;
    }
    _curses_panel_state *state = get_curses_panel_state(module);
    return PyCursesPanel_New(state, pan, win);
}


/* Wrapper for panel_below(NULL). This function returns the top panel
   of the stack, so it's renamed to top_panel(). panel.below()
   *requires* a panel object in the first place which may be
   undesirable. */
/*[clinic input]
_curses_panel.top_panel

Return the top panel in the panel stack.
[clinic start generated code]*/

static PyObject *
_curses_panel_top_panel_impl(PyObject *module)
/*[clinic end generated code: output=86704988bea8508e input=e62d6278dba39e79]*/
{
    PANEL *pan;
    PyCursesPanelObject *po;

    PyCursesInitialised;

    pan = panel_below(NULL);
    if (pan == NULL) {  /* valid output: it means no panel exists yet */
        Py_RETURN_NONE;
    }
    po = find_po(pan, "panel_below", "top_panel");
    return Py_XNewRef(po);
}

/*[clinic input]
_curses_panel.update_panels

Updates the virtual screen after changes in the panel stack.

This does not call curses.doupdate(), so you'll have to do this yourself.
[clinic start generated code]*/

static PyObject *
_curses_panel_update_panels_impl(PyObject *module)
/*[clinic end generated code: output=2f3b4c2e03d90ded input=5299624c9a708621]*/
{
    PyCursesInitialised;
    update_panels();
    Py_RETURN_NONE;
}

/* List of functions defined in the module */

static PyMethodDef PyCurses_methods[] = {
    _CURSES_PANEL_BOTTOM_PANEL_METHODDEF
    _CURSES_PANEL_NEW_PANEL_METHODDEF
    _CURSES_PANEL_TOP_PANEL_METHODDEF
    _CURSES_PANEL_UPDATE_PANELS_METHODDEF
    {NULL,              NULL}           /* sentinel */
};

/* Initialization function for the module */
static int
_curses_panel_exec(PyObject *mod)
{
    _curses_panel_state *state = get_curses_panel_state(mod);
    /* Initialize object type */
    state->PyCursesPanel_Type = (PyTypeObject *)PyType_FromModuleAndSpec(
        mod, &PyCursesPanel_Type_spec, NULL);
    if (state->PyCursesPanel_Type == NULL) {
        return -1;
    }

    if (PyModule_AddType(mod, state->PyCursesPanel_Type) < 0) {
        return -1;
    }

    import_curses();
    if (PyErr_Occurred()) {
        return -1;
    }

    /* For exception _curses_panel.error */
    state->error = PyErr_NewException(
        "_curses_panel.error", NULL, NULL);

    if (PyModule_AddObjectRef(mod, "error", state->error) < 0) {
        return -1;
    }

    /* Make the version available */
    PyObject *v = PyUnicode_FromString(PyCursesVersion);
    if (v == NULL) {
        return -1;
    }

    PyObject *d = PyModule_GetDict(mod);
    if (PyDict_SetItemString(d, "version", v) < 0) {
        Py_DECREF(v);
        return -1;
    }
    if (PyDict_SetItemString(d, "__version__", v) < 0) {
        Py_DECREF(v);
        return -1;
    }

    Py_DECREF(v);

    return 0;
}

static PyModuleDef_Slot _curses_slots[] = {
    {Py_mod_exec, _curses_panel_exec},
    // XXX gh-103092: fix isolation.
    {Py_mod_multiple_interpreters, Py_MOD_MULTIPLE_INTERPRETERS_NOT_SUPPORTED},
    //{Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef _curses_panelmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_curses_panel",
    .m_size = sizeof(_curses_panel_state),
    .m_methods = PyCurses_methods,
    .m_slots = _curses_slots,
    .m_traverse = _curses_panel_traverse,
    .m_clear = _curses_panel_clear,
    .m_free = _curses_panel_free
};

PyMODINIT_FUNC
PyInit__curses_panel(void)
{
    return PyModuleDef_Init(&_curses_panelmodule);
}
