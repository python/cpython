/*
 *   Interface to the ncurses panel library
 *
 * Original version by Thomas Gellekum
 */

/* Release Number */

static const char PyCursesVersion[] = "2.1";

/* Includes */

#include "Python.h"

#include "py_curses.h"

#include <panel.h>

typedef struct {
    PyObject *PyCursesError;
    PyTypeObject *PyCursesPanel_Type;
} _curses_panel_state;

static inline _curses_panel_state *
get_curses_panel_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (_curses_panel_state *)state;
}

static int
_curses_panel_clear(PyObject *mod)
{
    _curses_panel_state *state = get_curses_panel_state(mod);
    Py_CLEAR(state->PyCursesError);
    Py_CLEAR(state->PyCursesPanel_Type);
    return 0;
}

static int
_curses_panel_traverse(PyObject *mod, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(mod));
    _curses_panel_state *state = get_curses_panel_state(mod);
    Py_VISIT(state->PyCursesError);
    Py_VISIT(state->PyCursesPanel_Type);
    return 0;
}

static void
_curses_panel_free(void *mod)
{
    _curses_panel_clear((PyObject *) mod);
}

/* Utility Functions */

/*
 * Check the return code from a curses function and return None
 * or raise an exception as appropriate.
 */

static PyObject *
PyCursesCheckERR(_curses_panel_state *state, int code, const char *fname)
{
    if (code != ERR) {
        Py_RETURN_NONE;
    }
    else {
        if (fname == NULL) {
            PyErr_SetString(state->PyCursesError, catchall_ERR);
        }
        else {
            PyErr_Format(state->PyCursesError, "%s() returned ERR", fname);
        }
        return NULL;
    }
}

/*****************************************************************************
 The Panel Object
******************************************************************************/

/* Definition of the panel object and panel type */

typedef struct {
    PyObject_HEAD
    PANEL *pan;
    PyCursesWindowObject *wo;   /* for reference counts */
} PyCursesPanelObject;

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

/* Remove the panel object from lop */
static void
remove_lop(PyCursesPanelObject *po)
{
    list_of_panels *temp, *n;

    temp = lop;
    if (temp->po == po) {
        lop = temp->next;
        PyMem_Free(temp);
        return;
    }
    while (temp->next == NULL || temp->next->po != po) {
        if (temp->next == NULL) {
            PyErr_SetString(PyExc_RuntimeError,
                            "remove_lop: can't find Panel Object");
            return;
        }
        temp = temp->next;
    }
    n = temp->next->next;
    PyMem_Free(temp->next);
    temp->next = n;
    return;
}

/* Return the panel object that corresponds to pan */
static PyCursesPanelObject *
find_po(PANEL *pan)
{
    list_of_panels *temp;
    for (temp = lop; temp->po->pan != pan; temp = temp->next)
        if (temp->next == NULL) return NULL;    /* not found!? */
    return temp->po;
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

    cls: defining_class

Push the panel to the bottom of the stack.
[clinic start generated code]*/

static PyObject *
_curses_panel_panel_bottom_impl(PyCursesPanelObject *self, PyTypeObject *cls)
/*[clinic end generated code: output=8ec7fbbc08554021 input=6b7d2c0578b5a1c4]*/
{
    _curses_panel_state *state = PyType_GetModuleState(cls);
    return PyCursesCheckERR(state, bottom_panel(self->pan), "bottom");
}

/*[clinic input]
_curses_panel.panel.hide

    cls: defining_class

Hide the panel.

This does not delete the object, it just makes the window on screen invisible.
[clinic start generated code]*/

static PyObject *
_curses_panel_panel_hide_impl(PyCursesPanelObject *self, PyTypeObject *cls)
/*[clinic end generated code: output=cc6ab7203cdc1450 input=1bfc741f473e6055]*/
{
    _curses_panel_state *state = PyType_GetModuleState(cls);
    return PyCursesCheckERR(state, hide_panel(self->pan), "hide");
}

/*[clinic input]
_curses_panel.panel.show

    cls: defining_class

Display the panel (which might have been hidden).
[clinic start generated code]*/

static PyObject *
_curses_panel_panel_show_impl(PyCursesPanelObject *self, PyTypeObject *cls)
/*[clinic end generated code: output=dc3421de375f0409 input=8122e80151cb4379]*/
{
    _curses_panel_state *state = PyType_GetModuleState(cls);
    return PyCursesCheckERR(state, show_panel(self->pan), "show");
}

/*[clinic input]
_curses_panel.panel.top

    cls: defining_class

Push panel to the top of the stack.
[clinic start generated code]*/

static PyObject *
_curses_panel_panel_top_impl(PyCursesPanelObject *self, PyTypeObject *cls)
/*[clinic end generated code: output=10a072e511e873f7 input=1f372d597dda3379]*/
{
    _curses_panel_state *state = PyType_GetModuleState(cls);
    return PyCursesCheckERR(state, top_panel(self->pan), "top");
}

/* Allocation and deallocation of Panel Objects */

static PyObject *
PyCursesPanel_New(_curses_panel_state *state, PANEL *pan,
                  PyCursesWindowObject *wo)
{
    PyCursesPanelObject *po = PyObject_New(PyCursesPanelObject,
                                           state->PyCursesPanel_Type);
    if (po == NULL) {
        return NULL;
    }

    po->pan = pan;
    if (insert_lop(po) < 0) {
        po->wo = NULL;
        Py_DECREF(po);
        return NULL;
    }
    po->wo = wo;
    Py_INCREF(wo);
    return (PyObject *)po;
}

static void
PyCursesPanel_Dealloc(PyCursesPanelObject *po)
{
    PyObject *tp, *obj;

    tp = (PyObject *) Py_TYPE(po);
    obj = (PyObject *) panel_userptr(po->pan);
    if (obj) {
        (void)set_panel_userptr(po->pan, NULL);
        Py_DECREF(obj);
    }
    (void)del_panel(po->pan);
    if (po->wo != NULL) {
        Py_DECREF(po->wo);
        remove_lop(po);
    }
    PyObject_DEL(po);
    Py_DECREF(tp);
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

    if (pan == NULL) {          /* valid output, it means the calling panel
                                   is on top of the stack */
        Py_RETURN_NONE;
    }
    po = find_po(pan);
    if (po == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "panel_above: can't find Panel Object");
        return NULL;
    }
    Py_INCREF(po);
    return (PyObject *)po;
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

    if (pan == NULL) {          /* valid output, it means the calling panel
                                   is on the bottom of the stack */
        Py_RETURN_NONE;
    }
    po = find_po(pan);
    if (po == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "panel_below: can't find Panel Object");
        return NULL;
    }
    Py_INCREF(po);
    return (PyObject *)po;
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

    cls: defining_class
    y: int
    x: int
    /

Move the panel to the screen coordinates (y, x).
[clinic start generated code]*/

static PyObject *
_curses_panel_panel_move_impl(PyCursesPanelObject *self, PyTypeObject *cls,
                              int y, int x)
/*[clinic end generated code: output=ce546c93e56867da input=60a0e7912ff99849]*/
{
    _curses_panel_state *state = PyType_GetModuleState(cls);
    return PyCursesCheckERR(state, move_panel(self->pan, y, x), "move_panel");
}

/*[clinic input]
_curses_panel.panel.window

Return the window object associated with the panel.
[clinic start generated code]*/

static PyObject *
_curses_panel_panel_window_impl(PyCursesPanelObject *self)
/*[clinic end generated code: output=5f05940d4106b4cb input=6067353d2c307901]*/
{
    Py_INCREF(self->wo);
    return (PyObject *)self->wo;
}

/*[clinic input]
_curses_panel.panel.replace

    cls: defining_class
    win: object(type="PyCursesWindowObject *", subclass_of="&PyCursesWindow_Type")
    /

Change the window associated with the panel to the window win.
[clinic start generated code]*/

static PyObject *
_curses_panel_panel_replace_impl(PyCursesPanelObject *self,
                                 PyTypeObject *cls,
                                 PyCursesWindowObject *win)
/*[clinic end generated code: output=c71f95c212d58ae7 input=dbec7180ece41ff5]*/
{
    _curses_panel_state *state = PyType_GetModuleState(cls);

    PyCursesPanelObject *po = find_po(self->pan);
    if (po == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "replace_panel: can't find Panel Object");
        return NULL;
    }

    int rtn = replace_panel(self->pan, win->win);
    if (rtn == ERR) {
        PyErr_SetString(state->PyCursesError, "replace_panel() returned ERR");
        return NULL;
    }
    Py_INCREF(win);
    Py_SETREF(po->wo, win);
    Py_RETURN_NONE;
}

/*[clinic input]
_curses_panel.panel.set_userptr

    cls: defining_class
    obj: object
    /

Set the panel's user pointer to obj.
[clinic start generated code]*/

static PyObject *
_curses_panel_panel_set_userptr_impl(PyCursesPanelObject *self,
                                     PyTypeObject *cls, PyObject *obj)
/*[clinic end generated code: output=db74f3db07b28080 input=e3fee2ff7b1b8e48]*/
{
    PyCursesInitialised;
    Py_INCREF(obj);
    PyObject *oldobj = (PyObject *) panel_userptr(self->pan);
    int rc = set_panel_userptr(self->pan, (void*)obj);
    if (rc == ERR) {
        /* In case of an ncurses error, decref the new object again */
        Py_DECREF(obj);
    }
    Py_XDECREF(oldobj);

    _curses_panel_state *state = PyType_GetModuleState(cls);
    return PyCursesCheckERR(state, rc, "set_panel_userptr");
}

/*[clinic input]
_curses_panel.panel.userptr

    cls: defining_class

Return the user pointer for the panel.
[clinic start generated code]*/

static PyObject *
_curses_panel_panel_userptr_impl(PyCursesPanelObject *self,
                                 PyTypeObject *cls)
/*[clinic end generated code: output=eea6e6f39ffc0179 input=f22ca4f115e30a80]*/
{
    _curses_panel_state *state = PyType_GetModuleState(cls);

    PyCursesInitialised;
    PyObject *obj = (PyObject *) panel_userptr(self->pan);
    if (obj == NULL) {
        PyErr_SetString(state->PyCursesError, "no userptr set");
        return NULL;
    }

    Py_INCREF(obj);
    return obj;
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
    {Py_tp_dealloc, PyCursesPanel_Dealloc},
    {Py_tp_methods, PyCursesPanel_Methods},
    {0, 0},
};

static PyType_Spec PyCursesPanel_Type_spec = {
    .name = "_curses_panel.panel",
    .basicsize = sizeof(PyCursesPanelObject),
    .flags = Py_TPFLAGS_DEFAULT,
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

    if (pan == NULL) {          /* valid output, it means
                                   there's no panel at all */
        Py_RETURN_NONE;
    }
    po = find_po(pan);
    if (po == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "panel_above: can't find Panel Object");
        return NULL;
    }
    Py_INCREF(po);
    return (PyObject *)po;
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
    _curses_panel_state *state = get_curses_panel_state(module);

    PANEL *pan = new_panel(win->win);
    if (pan == NULL) {
        PyErr_SetString(state->PyCursesError, catchall_NULL);
        return NULL;
    }
    return (PyObject *)PyCursesPanel_New(state, pan, win);
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

    if (pan == NULL) {          /* valid output, it means
                                   there's no panel at all */
        Py_RETURN_NONE;
    }
    po = find_po(pan);
    if (po == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "panel_below: can't find Panel Object");
        return NULL;
    }
    Py_INCREF(po);
    return (PyObject *)po;
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
    state->PyCursesError = PyErr_NewException(
        "_curses_panel.error", NULL, NULL);

    Py_INCREF(state->PyCursesError);
    if (PyModule_AddObject(mod, "error", state->PyCursesError) < 0) {
        Py_DECREF(state->PyCursesError);
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