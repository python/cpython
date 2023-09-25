/*
*   Interface to the ncurses form library
*
*   Original version by Thomas Gellekum
*/

/* Release Number */

static const char PyCursesVersion[] = "2.1";

/* Includes */

#include "Python.h"

#include "py_curses.h"

#include <form.h>

typedef struct {
    PyObject *PyCursesError;
    PyTypeObject *PyCursesForm_Type;
} _curses_form_state;


static inline _curses_form_state *
get_curses_form_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (_curses_form_state *)state;
}

static int
_curses_form_clear(PyObject *mod)
{
    _curses_form_state *state = get_curses_form_state(mod);
    Py_CLEAR(state->PyCursesError);
    Py_CLEAR(state->PyCursesForm_Type);
    return 0;
}

static void
_curses_form_free(void *mod)
{
    _curses_form_clear((PyObject *) mod);
}

static PyObject *
PyCursesCheckERR(_curses_form_state *state, int code, const char *fname)
{
    if (code != ERR) {
        Py_RETURN_NONE;
    }
    else {
        if (fname == NULL) {
            if (fname == NULL) {
                PyErr_SetString(state->PyCursesError, catchall_ERR);
            }
            else {
                 PyErr_Format(state->PyCursesError, "%s() returned ERR", fname);
            }
            return NULL;
        }
    }
}

/*******************************************************************************
 The Form Object
*******************************************************************************/

/* Definition of the form object and form type */

typedef struct {
    PyObject HEAD;
    FORM *form;
    PyCursesWindowObject *wo;   /* for reference counts */
} PyCursesFormObject;

typedef struct {
    PyObject HEAD;
    FIELD *field;
    PyCursesWindowObject *wo;   /* for reference counts */
} PyCursesFieldObject;

/* We keep a linked list of PyCursesFieldObjects, lof. A list should suffice. */
typedef struct _list_of_fields {
    PyCursesFieldObject *fo;
    struct _list_of_fields *next;
} list_of_fields;
    
static list_of_fields *lof;

static int
insert_lof(PyCursesFieldObject *fo)
{
    list_of_fields *new;

    if ((new = (list_of_fields *)PyMem_Malloc(sizeof(list_of_fields))) == NULL) {
        PyErr_NoMemory();
        return -1;
    }

    new->fo = fo;
    new->next = lof;
    lof = new;
    return 0;
}

/* Remove the field from lof */
static void
remove_lof(PyCursesFieldObject *fo)
{
    list_of_fields *temp, *n;
    temp = lof;
    if(temp->fo == fo) {
        lof = temp->next;
        PyMem_Free(temp);
        return;
    }
    while (temp->next == NULL || temp->next->fo != fo) {
        if (temp->next == NULL) {
            PyErr_SetString(PyExc_RuntimeError,
                        "remove_lof: can't find Field Object");
            return;
        }
        temp = temp->next;
    }
    n = temp->next->next;
    PyMem_Free(temp->next);
    temp->next = n;
    return;
}

/* Return the field object that corresponds to field */
static PyCursesFieldObject *
find_fo(FIELD *field)
{
    list_of_fields *temp;
    for (temp = lof; temp->fo->field != field; temp = temp->next)
        if (temp->next == NULL) return NULL;    /* not found!? */
    return temp->fo;
}


