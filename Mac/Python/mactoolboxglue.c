/*
** mactoolboxglue.c - Glue together the toolbox objects.
**
** Because toolbox modules interdepend on each other, they use each others
** object types, on MacOSX/MachO this leads to the situation that they
** cannot be dynamically loaded (or they would all have to be lumped into
** a single .so, but this would be bad for extensibility).
**
** This file defines wrappers for all the _New and _Convert functions,
** which are the Py_BuildValue and PyArg_ParseTuple helpers. The wrappers
** check an indirection function pointer, and if it isn't filled in yet
** they import the appropriate module, whose init routine should fill in
** the pointer.
*/

#ifdef USE_TOOLBOX_OBJECT_GLUE

#include "python.h"
#include "pymactoolbox.h"

#define GLUE_NEW(object, routinename, module) \
PyObject *(*PyMacGluePtr_##routinename)(object); \
\
PyObject *routinename(object cobj) { \
    if (!PyMacGluePtr_##routinename) { \
       if (!PyImport_ImportModule(module)) return NULL; \
       if (!PyMacGluePtr_##routinename) { \
           PyErr_SetString(PyExc_ImportError, "Module did not provide routine: " module ": " #routinename); \
           return NULL; \
       } \
    } \
    return (*PyMacGluePtr_##routinename)(cobj); \
}

#define GLUE_CONVERT(object, routinename, module) \
int (*PyMacGluePtr_##routinename)(PyObject *, object *); \
\
int routinename(PyObject *pyobj, object *cobj) { \
    if (!PyMacGluePtr_##routinename) { \
       if (!PyImport_ImportModule(module)) return NULL; \
       if (!PyMacGluePtr_##routinename) { \
           PyErr_SetString(PyExc_ImportError, "Module did not provide routine: " module ": " #routinename); \
           return NULL; \
       } \
    } \
    return (*PyMacGluePtr_##routinename)(pyobj, cobj); \
}

GLUE_NEW(AppleEvent *, AEDesc_New, "AE") /* XXXX Why by address? */
GLUE_CONVERT(AppleEvent, AEDesc_Convert, "AE")

GLUE_NEW(Component, CmpObj_New, "Cm")
GLUE_CONVERT(Component, CmpObj_Convert, "Cm")
GLUE_NEW(ComponentInstance, CmpInstObj_New, "Cm")
GLUE_CONVERT(ComponentInstance, CmpInstObj_Convert, "Cm")

GLUE_NEW(ControlHandle, CtlObj_New, "Ctl")
GLUE_CONVERT(ControlHandle, CtlObj_Convert, "Ctl")

GLUE_NEW(DialogPtr, DlgObj_New, "Dlg")
GLUE_CONVERT(DialogPtr, DlgObj_Convert, "Dlg")
GLUE_NEW(DialogPtr, DlgObj_WhichDialog, "Dlg")

GLUE_NEW(DragReference, DragObj_New, "Drag")
GLUE_CONVERT(DragReference, DragObj_Convert, "Drag")

GLUE_NEW(ListHandle, ListObj_New, "List")
GLUE_CONVERT(ListHandle, ListObj_Convert, "List")

GLUE_NEW(MenuHandle, MenuObj_New, "Menu")
GLUE_CONVERT(MenuHandle, MenuObj_Convert, "Menu")

GLUE_NEW(GrafPtr, GrafObj_New, "Qd")
GLUE_CONVERT(GrafPtr, GrafObj_Convert, "Qd")
GLUE_NEW(BitMapPtr, BMObj_New, "Qd")
GLUE_CONVERT(BitMapPtr, BMObj_Convert, "Qd")
GLUE_NEW(RGBColor *, QdRGB_New, "Qd") /* XXXX Why? */
GLUE_CONVERT(RGBColor, QdRGB_Convert, "Qd")

GLUE_NEW(GWorldPtr, GWorldObj_New, "Qdoffs")
GLUE_CONVERT(GWorldPtr, GWorldObj_Convert, "Qdoffs")

GLUE_NEW(Track, TrackObj_New, "Qt")
GLUE_CONVERT(Track, TrackObj_Convert, "Qt")
GLUE_NEW(Movie, MovieObj_New, "Qt")
GLUE_CONVERT(Movie, MovieObj_Convert, "Qt")
GLUE_NEW(MovieController, MovieCtlObj_New, "Qt")
GLUE_CONVERT(MovieController, MovieCtlObj_Convert, "Qt")
GLUE_NEW(TimeBase, TimeBaseObj_New, "Qt")
GLUE_CONVERT(TimeBase, TimeBaseObj_Convert, "Qt")
GLUE_NEW(UserData, UserDataObj_New, "Qt")
GLUE_CONVERT(UserData, UserDataObj_Convert, "Qt")
GLUE_NEW(Media, MediaObj_New, "Qt")
GLUE_CONVERT(Media, MediaObj_Convert, "Qt")

GLUE_NEW(Handle, ResObj_New, "Res")
GLUE_CONVERT(Handle, ResObj_Convert, "Res")
GLUE_NEW(Handle, OptResObj_New, "Res")
GLUE_CONVERT(Handle, OptResObj_Convert, "Res")

GLUE_NEW(TEHandle, TEObj_New, "TE")
GLUE_CONVERT(TEHandle, TEObj_Convert, "TE")

GLUE_NEW(WindowPtr, WinObj_New, "Win")
GLUE_CONVERT(WindowPtr, WinObj_Convert, "Win")
GLUE_NEW(WindowPtr, WinObj_WhichWindow, "Win")

#endif /* USE_TOOLBOX_OBJECT_GLUE */