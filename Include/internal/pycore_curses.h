#ifndef Py_INTERNAL_CURSES_H
#define Py_INTERNAL_CURSES_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    PyTypeObject *window_type;
    int (*setup_term_called)(void);
    int (*initialized)(void);
    int (*initialized_color)(void);
} PyCurses_CAPI;

#ifdef CURSES_MODULE
/* This section is used when compiling _cursesmodule.c */

#else
/* This section is used in modules that use the _cursesmodule API */

static PyCurses_CAPI *PyCurses_C_API;

#undef PyCursesWindow_Type
#undef PyCursesSetupTermCalled
#undef PyCursesInitialised
#undef PyCursesInitialisedColor
#define PyCursesWindow_Type (*PyCurses_C_API->window_type)
#define PyCursesSetupTermCalled  {if (!PyCurses_C_API->setup_term_called()) return NULL;}
#define PyCursesInitialised      {if (!PyCurses_C_API->initialized()) return NULL;}
#define PyCursesInitialisedColor {if (!PyCurses_C_API->initialized_color()) return NULL;}

#undef import_curses
#define import_curses() \
    PyCurses_C_API = PyCapsule_Import(PyCurses_CAPSULE_NAME, 1);

#endif

#ifdef __cplusplus
}
#endif

#endif /* !Py_INTERNAL_CURSES_H */
