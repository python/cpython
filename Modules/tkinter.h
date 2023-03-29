#ifndef TKINTER_H
#define TKINTER_H

/* This header is used to share some macros between _tkinter.c and
 * tkappinit.c.
 * Be sure to include tk.h before including this header so
 * TK_HEX_VERSION is properly defined. */

/* TK_RELEASE_LEVEL is always one of the following:
 *  TCL_ALPHA_RELEASE   0
 *  TCL_BETA_RELEASE    1
 *  TCL_FINAL_RELEASE   2
 */
#define TK_HEX_VERSION ((TK_MAJOR_VERSION << 24) | \
                        (TK_MINOR_VERSION << 16) | \
                        (TK_RELEASE_LEVEL << 8) | \
                        (TK_RELEASE_SERIAL << 0))

/* Protect Tk 8.4.13 and older from a deadlock that happens when trying
 * to load tk after a failed attempt. */
#if TK_HEX_VERSION < 0x0804020e
#define TKINTER_PROTECT_LOADTK
#define TKINTER_LOADTK_ERRMSG \
        "Calling Tk_Init again after a previous call failed might deadlock"
#endif

typedef struct {
    // Types and exceptions
    PyObject *PyTclObject_Type;
    PyObject *Tkapp_Type;
    PyObject *Tktt_Type;
    PyObject *Tkinter_TclError;

    // Locking
    PyThread_type_lock tcl_lock;

    // Error handling
    PyObject *excInCmd;
    int errorInCmd;
    int quitMainLoop;

    // Util
    int Tkinter_busywaitinterval;
    struct _fhcdata *HeadFHCD;
    int stdin_ready;
    PyThreadState *event_tstate;
} module_state;

extern module_state global_state;

#define GLOBAL_STATE() (&global_state)

static inline module_state *
get_module_state(PyObject *mod)
{
    void *state = _PyModule_GetState(mod);
    assert(state != NULL);
    return (module_state *)state;
}

static inline module_state *
get_module_state_by_cls(PyTypeObject *cls)
{
    void *state = _PyType_GetModuleState(cls);
    assert(state != NULL);
    return (module_state *)state;
}

extern struct PyModuleDef _tkintermodule;

static inline PyObject *
find_module_by_type(PyTypeObject *tp)
{
    return PyType_GetModuleByDef(tp, &_tkintermodule);
}

static inline module_state *
find_module_state_by_type(PyTypeObject *tp)
{
    PyObject *mod = find_module_by_type(tp);
    assert(mod != NULL);
    return get_module_state(mod);
}

#endif /* !TKINTER_H */
