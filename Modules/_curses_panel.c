/*
 *   Interface to the ncurses panel library
 *
 * Original version by Thomas Gellekum
 */

/* Release Number */

static char *PyCursesVersion = "2.1";

/* Includes */

#include "Python.h"

#include "py_curses.h"

#include <panel.h>

typedef struct {
    PyObject *PyCursesError;
} _curses_panelstate;

#define _curses_panelstate(o) ((_curses_panelstate *)PyModule_GetState(o))

/*static PyObject *PyCursesError;*/

static int
_curses_panel_clear(PyObject *m)
{
    Py_CLEAR(_curses_panelstate(m)->PyCursesError);
    return 0;
}

static int
_curses_panel_traverse(PyObject *m, visitproc visit, void *arg)
{
    Py_VISIT(_curses_panelstate(m)->PyCursesError);
    return 0;
}

static void
_curses_panel_free(void *m)
{
    _curses_panel_clear((PyObject *) m);
}

static struct PyModuleDef _curses_panelmodule;

#define _curses_panelstate_global \
((_curses_panelstate *) PyModule_GetState(PyState_FindModule(&_curses_panelmodule)))

/* Utility Functions */

/*
 * Check the return code from a curses function and return None
 * or raise an exception as appropriate.
 */

static PyObject *
PyCursesCheckERR(int code, char *fname)
{
    if (code != ERR) {
        Py_INCREF(Py_None);
        return Py_None;
    } else {
        if (fname == NULL) {
            PyErr_SetString(_curses_panelstate_global->PyCursesError, catchall_ERR);
        } else {
            PyErr_Format(_curses_panelstate_global->PyCursesError, "%s() returned ERR", fname);
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

PyTypeObject PyCursesPanel_Type;

#define PyCursesPanel_Check(v)   (Py_TYPE(v) == &PyCursesPanel_Type)

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

    if ((new = (list_of_panels *)malloc(sizeof(list_of_panels))) == NULL) {
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
        free(temp);
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
    free(temp->next);
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

/* Function Prototype Macros - They are ugly but very, very useful. ;-)

   X - function name
   TYPE - parameter Type
   ERGSTR - format string for construction of the return value
   PARSESTR - format string for argument parsing */

#define Panel_NoArgNoReturnFunction(X) \
static PyObject *PyCursesPanel_##X(PyCursesPanelObject *self) \
{ return PyCursesCheckERR(X(self->pan), # X); }

#define Panel_NoArgTrueFalseFunction(X) \
static PyObject *PyCursesPanel_##X(PyCursesPanelObject *self) \
{ \
  if (X (self->pan) == FALSE) { Py_INCREF(Py_False); return Py_False; } \
  else { Py_INCREF(Py_True); return Py_True; } }

#define Panel_TwoArgNoReturnFunction(X, TYPE, PARSESTR) \
static PyObject *PyCursesPanel_##X(PyCursesPanelObject *self, PyObject *args) \
{ \
  TYPE arg1, arg2; \
  if (!PyArg_ParseTuple(args, PARSESTR, &arg1, &arg2)) return NULL; \
  return PyCursesCheckERR(X(self->pan, arg1, arg2), # X); }

/* ------------- PANEL routines --------------- */

Panel_NoArgNoReturnFunction(bottom_panel)
Panel_NoArgNoReturnFunction(hide_panel)
Panel_NoArgNoReturnFunction(show_panel)
Panel_NoArgNoReturnFunction(top_panel)
Panel_NoArgTrueFalseFunction(panel_hidden)
Panel_TwoArgNoReturnFunction(move_panel, int, "ii;y,x")

/* Allocation and deallocation of Panel Objects */

static PyObject *
PyCursesPanel_New(PANEL *pan, PyCursesWindowObject *wo)
{
    PyCursesPanelObject *po;

    po = PyObject_NEW(PyCursesPanelObject, &PyCursesPanel_Type);
    if (po == NULL) return NULL;
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
    (void)del_panel(po->pan);
    if (po->wo != NULL) {
        Py_DECREF(po->wo);
        remove_lop(po);
    }
    PyObject_DEL(po);
}

/* panel_above(NULL) returns the bottom panel in the stack. To get
   this behaviour we use curses.panel.bottom_panel(). */
static PyObject *
PyCursesPanel_above(PyCursesPanelObject *self)
{
    PANEL *pan;
    PyCursesPanelObject *po;

    pan = panel_above(self->pan);

    if (pan == NULL) {          /* valid output, it means the calling panel
                                   is on top of the stack */
        Py_INCREF(Py_None);
        return Py_None;
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
static PyObject *
PyCursesPanel_below(PyCursesPanelObject *self)
{
    PANEL *pan;
    PyCursesPanelObject *po;

    pan = panel_below(self->pan);

    if (pan == NULL) {          /* valid output, it means the calling panel
                                   is on the bottom of the stack */
        Py_INCREF(Py_None);
        return Py_None;
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

static PyObject *
PyCursesPanel_window(PyCursesPanelObject *self)
{
    Py_INCREF(self->wo);
    return (PyObject *)self->wo;
}

static PyObject *
PyCursesPanel_replace_panel(PyCursesPanelObject *self, PyObject *args)
{
    PyCursesPanelObject *po;
    PyCursesWindowObject *temp;
    int rtn;

    if (PyTuple_Size(args) != 1) {
        PyErr_SetString(PyExc_TypeError, "replace requires one argument");
        return NULL;
    }
    if (!PyArg_ParseTuple(args, "O!;window object",
                          &PyCursesWindow_Type, &temp))
        return NULL;

    po = find_po(self->pan);
    if (po == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "replace_panel: can't find Panel Object");
        return NULL;
    }

    rtn = replace_panel(self->pan, temp->win);
    if (rtn == ERR) {
        PyErr_SetString(_curses_panelstate_global->PyCursesError, "replace_panel() returned ERR");
        return NULL;
    }
    Py_DECREF(po->wo);
    po->wo = temp;
    Py_INCREF(po->wo);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
PyCursesPanel_set_panel_userptr(PyCursesPanelObject *self, PyObject *obj)
{
    Py_INCREF(obj);
    return PyCursesCheckERR(set_panel_userptr(self->pan, (void*)obj),
                            "set_panel_userptr");
}

static PyObject *
PyCursesPanel_userptr(PyCursesPanelObject *self)
{
    PyObject *obj;
    PyCursesInitialised;
    obj = (PyObject *) panel_userptr(self->pan);
    if (obj == NULL) {
        PyErr_SetString(_curses_panelstate_global->PyCursesError, "no userptr set");
        return NULL;
    }

    Py_INCREF(obj);
    return obj;
}


/* Module interface */

static PyMethodDef PyCursesPanel_Methods[] = {
    {"above",           (PyCFunction)PyCursesPanel_above, METH_NOARGS},
    {"below",           (PyCFunction)PyCursesPanel_below, METH_NOARGS},
    {"bottom",          (PyCFunction)PyCursesPanel_bottom_panel, METH_NOARGS},
    {"hidden",          (PyCFunction)PyCursesPanel_panel_hidden, METH_NOARGS},
    {"hide",            (PyCFunction)PyCursesPanel_hide_panel, METH_NOARGS},
    {"move",            (PyCFunction)PyCursesPanel_move_panel, METH_VARARGS},
    {"replace",         (PyCFunction)PyCursesPanel_replace_panel, METH_VARARGS},
    {"set_userptr",     (PyCFunction)PyCursesPanel_set_panel_userptr, METH_O},
    {"show",            (PyCFunction)PyCursesPanel_show_panel, METH_NOARGS},
    {"top",             (PyCFunction)PyCursesPanel_top_panel, METH_NOARGS},
    {"userptr",         (PyCFunction)PyCursesPanel_userptr, METH_NOARGS},
    {"window",          (PyCFunction)PyCursesPanel_window, METH_NOARGS},
    {NULL,              NULL}   /* sentinel */
};

/* -------------------------------------------------------*/

PyTypeObject PyCursesPanel_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_curses_panel.curses panel",       /*tp_name*/
    sizeof(PyCursesPanelObject),        /*tp_basicsize*/
    0,                  /*tp_itemsize*/
    /* methods */
    (destructor)PyCursesPanel_Dealloc, /*tp_dealloc*/
    0,                  /*tp_print*/
    0,                  /*tp_getattr*/
    0,                  /*tp_setattr*/
    0,                  /*tp_reserved*/
    0,                  /*tp_repr*/
    0,                  /*tp_as_number*/
    0,                  /*tp_as_sequence*/
    0,                  /*tp_as_mapping*/
    0,                  /*tp_hash*/
    0,                  /*tp_call*/
    0,                  /*tp_str*/
    0,                  /*tp_getattro*/
    0,                  /*tp_setattro*/
    0,                  /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT, /*tp_flags*/
    0,                  /*tp_doc*/
    0,                  /*tp_traverse*/
    0,                  /*tp_clear*/
    0,                  /*tp_richcompare*/
    0,                  /*tp_weaklistoffset*/
    0,                  /*tp_iter*/
    0,                  /*tp_iternext*/
    PyCursesPanel_Methods, /*tp_methods*/
};

/* Wrapper for panel_above(NULL). This function returns the bottom
   panel of the stack, so it's renamed to bottom_panel().
   panel.above() *requires* a panel object in the first place which
   may be undesirable. */
static PyObject *
PyCurses_bottom_panel(PyObject *self)
{
    PANEL *pan;
    PyCursesPanelObject *po;

    PyCursesInitialised;

    pan = panel_above(NULL);

    if (pan == NULL) {          /* valid output, it means
                                   there's no panel at all */
        Py_INCREF(Py_None);
        return Py_None;
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

static PyObject *
PyCurses_new_panel(PyObject *self, PyObject *args)
{
    PyCursesWindowObject *win;
    PANEL *pan;

    if (!PyArg_ParseTuple(args, "O!", &PyCursesWindow_Type, &win))
        return NULL;
    pan = new_panel(win->win);
    if (pan == NULL) {
        PyErr_SetString(_curses_panelstate_global->PyCursesError, catchall_NULL);
        return NULL;
    }
    return (PyObject *)PyCursesPanel_New(pan, win);
}


/* Wrapper for panel_below(NULL). This function returns the top panel
   of the stack, so it's renamed to top_panel(). panel.below()
   *requires* a panel object in the first place which may be
   undesirable. */
static PyObject *
PyCurses_top_panel(PyObject *self)
{
    PANEL *pan;
    PyCursesPanelObject *po;

    PyCursesInitialised;

    pan = panel_below(NULL);

    if (pan == NULL) {          /* valid output, it means
                                   there's no panel at all */
        Py_INCREF(Py_None);
        return Py_None;
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

static PyObject *PyCurses_update_panels(PyObject *self)
{
    PyCursesInitialised;
    update_panels();
    Py_INCREF(Py_None);
    return Py_None;
}


/* List of functions defined in the module */

static PyMethodDef PyCurses_methods[] = {
    {"bottom_panel",        (PyCFunction)PyCurses_bottom_panel,  METH_NOARGS},
    {"new_panel",           (PyCFunction)PyCurses_new_panel,     METH_VARARGS},
    {"top_panel",           (PyCFunction)PyCurses_top_panel,     METH_NOARGS},
    {"update_panels",       (PyCFunction)PyCurses_update_panels, METH_NOARGS},
    {NULL,              NULL}           /* sentinel */
};

/* Initialization function for the module */


static struct PyModuleDef _curses_panelmodule = {
        PyModuleDef_HEAD_INIT,
        "_curses_panel",
        NULL,
        sizeof(_curses_panelstate),
        PyCurses_methods,
        NULL,
        _curses_panel_traverse,
        _curses_panel_clear,
        _curses_panel_free
};

PyMODINIT_FUNC
PyInit__curses_panel(void)
{
    PyObject *m, *d, *v;

    /* Initialize object type */
    if (PyType_Ready(&PyCursesPanel_Type) < 0)
        return NULL;

    import_curses();

    /* Create the module and add the functions */
    m = PyModule_Create(&_curses_panelmodule);
    if (m == NULL)
        return NULL;
    d = PyModule_GetDict(m);

    /* For exception _curses_panel.error */
    _curses_panelstate(m)->PyCursesError = PyErr_NewException("_curses_panel.error", NULL, NULL);
    PyDict_SetItemString(d, "error", _curses_panelstate(m)->PyCursesError);

    /* Make the version available */
    v = PyUnicode_FromString(PyCursesVersion);
    PyDict_SetItemString(d, "version", v);
    PyDict_SetItemString(d, "__version__", v);
    Py_DECREF(v);
    return m;
}
