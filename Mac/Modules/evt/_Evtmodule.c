
/* ========================== Module _Evt =========================== */

#include "Python.h"

#ifndef __LP64__


#include "pymactoolbox.h"

/* Macro to test whether a weak-loaded CFM function exists */
#define PyMac_PRECHECK(rtn) do { if ( &rtn == NULL )  {\
        PyErr_SetString(PyExc_NotImplementedError, \
            "Not available in this shared library/OS version"); \
        return NULL; \
    }} while(0)


#include <Carbon/Carbon.h>


static PyObject *Evt_Error;

static PyObject *Evt_GetMouse(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Point mouseLoc;
#ifndef GetMouse
    PyMac_PRECHECK(GetMouse);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    GetMouse(&mouseLoc);
    _res = Py_BuildValue("O&",
                         PyMac_BuildPoint, mouseLoc);
    return _res;
}

static PyObject *Evt_Button(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
#ifndef Button
    PyMac_PRECHECK(Button);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = Button();
    _res = Py_BuildValue("b",
                         _rv);
    return _res;
}

static PyObject *Evt_StillDown(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
#ifndef StillDown
    PyMac_PRECHECK(StillDown);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = StillDown();
    _res = Py_BuildValue("b",
                         _rv);
    return _res;
}

static PyObject *Evt_WaitMouseUp(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
#ifndef WaitMouseUp
    PyMac_PRECHECK(WaitMouseUp);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = WaitMouseUp();
    _res = Py_BuildValue("b",
                         _rv);
    return _res;
}

static PyObject *Evt_GetCaretTime(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    UInt32 _rv;
#ifndef GetCaretTime
    PyMac_PRECHECK(GetCaretTime);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetCaretTime();
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *Evt_GetKeys(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    KeyMap theKeys__out__;
#ifndef GetKeys
    PyMac_PRECHECK(GetKeys);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    GetKeys(theKeys__out__);
    _res = Py_BuildValue("s#",
                         (char *)&theKeys__out__, (int)sizeof(KeyMap));
    return _res;
}

static PyObject *Evt_GetDblTime(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    UInt32 _rv;
#ifndef GetDblTime
    PyMac_PRECHECK(GetDblTime);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetDblTime();
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *Evt_SetEventMask(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    EventMask value;
#ifndef SetEventMask
    PyMac_PRECHECK(SetEventMask);
#endif
    if (!PyArg_ParseTuple(_args, "H",
                          &value))
        return NULL;
    SetEventMask(value);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Evt_GetNextEvent(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
    EventMask eventMask;
    EventRecord theEvent;
#ifndef GetNextEvent
    PyMac_PRECHECK(GetNextEvent);
#endif
    if (!PyArg_ParseTuple(_args, "H",
                          &eventMask))
        return NULL;
    _rv = GetNextEvent(eventMask,
                       &theEvent);
    _res = Py_BuildValue("bO&",
                         _rv,
                         PyMac_BuildEventRecord, &theEvent);
    return _res;
}

static PyObject *Evt_EventAvail(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
    EventMask eventMask;
    EventRecord theEvent;
#ifndef EventAvail
    PyMac_PRECHECK(EventAvail);
#endif
    if (!PyArg_ParseTuple(_args, "H",
                          &eventMask))
        return NULL;
    _rv = EventAvail(eventMask,
                     &theEvent);
    _res = Py_BuildValue("bO&",
                         _rv,
                         PyMac_BuildEventRecord, &theEvent);
    return _res;
}

static PyObject *Evt_PostEvent(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSErr _err;
    EventKind eventNum;
    UInt32 eventMsg;
#ifndef PostEvent
    PyMac_PRECHECK(PostEvent);
#endif
    if (!PyArg_ParseTuple(_args, "Hl",
                          &eventNum,
                          &eventMsg))
        return NULL;
    _err = PostEvent(eventNum,
                     eventMsg);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Evt_FlushEvents(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    EventMask whichMask;
    EventMask stopMask;
#ifndef FlushEvents
    PyMac_PRECHECK(FlushEvents);
#endif
    if (!PyArg_ParseTuple(_args, "HH",
                          &whichMask,
                          &stopMask))
        return NULL;
    FlushEvents(whichMask,
                stopMask);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Evt_GetGlobalMouse(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Point globalMouse;
#ifndef GetGlobalMouse
    PyMac_PRECHECK(GetGlobalMouse);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    GetGlobalMouse(&globalMouse);
    _res = Py_BuildValue("O&",
                         PyMac_BuildPoint, globalMouse);
    return _res;
}

static PyObject *Evt_GetCurrentKeyModifiers(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    UInt32 _rv;
#ifndef GetCurrentKeyModifiers
    PyMac_PRECHECK(GetCurrentKeyModifiers);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetCurrentKeyModifiers();
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *Evt_CheckEventQueueForUserCancel(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
#ifndef CheckEventQueueForUserCancel
    PyMac_PRECHECK(CheckEventQueueForUserCancel);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = CheckEventQueueForUserCancel();
    _res = Py_BuildValue("b",
                         _rv);
    return _res;
}

static PyObject *Evt_KeyScript(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    short code;
#ifndef KeyScript
    PyMac_PRECHECK(KeyScript);
#endif
    if (!PyArg_ParseTuple(_args, "h",
                          &code))
        return NULL;
    KeyScript(code);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Evt_IsCmdChar(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
    EventRecord event;
    short test;
#ifndef IsCmdChar
    PyMac_PRECHECK(IsCmdChar);
#endif
    if (!PyArg_ParseTuple(_args, "O&h",
                          PyMac_GetEventRecord, &event,
                          &test))
        return NULL;
    _rv = IsCmdChar(&event,
                    test);
    _res = Py_BuildValue("b",
                         _rv);
    return _res;
}

static PyObject *Evt_LMGetKeyThresh(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    SInt16 _rv;
#ifndef LMGetKeyThresh
    PyMac_PRECHECK(LMGetKeyThresh);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = LMGetKeyThresh();
    _res = Py_BuildValue("h",
                         _rv);
    return _res;
}

static PyObject *Evt_LMSetKeyThresh(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    SInt16 value;
#ifndef LMSetKeyThresh
    PyMac_PRECHECK(LMSetKeyThresh);
#endif
    if (!PyArg_ParseTuple(_args, "h",
                          &value))
        return NULL;
    LMSetKeyThresh(value);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Evt_LMGetKeyRepThresh(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    SInt16 _rv;
#ifndef LMGetKeyRepThresh
    PyMac_PRECHECK(LMGetKeyRepThresh);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = LMGetKeyRepThresh();
    _res = Py_BuildValue("h",
                         _rv);
    return _res;
}

static PyObject *Evt_LMSetKeyRepThresh(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    SInt16 value;
#ifndef LMSetKeyRepThresh
    PyMac_PRECHECK(LMSetKeyRepThresh);
#endif
    if (!PyArg_ParseTuple(_args, "h",
                          &value))
        return NULL;
    LMSetKeyRepThresh(value);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Evt_LMGetKbdLast(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    UInt8 _rv;
#ifndef LMGetKbdLast
    PyMac_PRECHECK(LMGetKbdLast);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = LMGetKbdLast();
    _res = Py_BuildValue("b",
                         _rv);
    return _res;
}

static PyObject *Evt_LMSetKbdLast(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    UInt8 value;
#ifndef LMSetKbdLast
    PyMac_PRECHECK(LMSetKbdLast);
#endif
    if (!PyArg_ParseTuple(_args, "b",
                          &value))
        return NULL;
    LMSetKbdLast(value);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Evt_LMGetKbdType(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    UInt8 _rv;
#ifndef LMGetKbdType
    PyMac_PRECHECK(LMGetKbdType);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = LMGetKbdType();
    _res = Py_BuildValue("b",
                         _rv);
    return _res;
}

static PyObject *Evt_LMSetKbdType(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    UInt8 value;
#ifndef LMSetKbdType
    PyMac_PRECHECK(LMSetKbdType);
#endif
    if (!PyArg_ParseTuple(_args, "b",
                          &value))
        return NULL;
    LMSetKbdType(value);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *Evt_TickCount(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    UInt32 _rv;
#ifndef TickCount
    PyMac_PRECHECK(TickCount);
#endif
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = TickCount();
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *Evt_WaitNextEvent(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;

    Boolean _rv;
    EventMask eventMask;
    EventRecord theEvent;
    UInt32 sleep;
    Handle mouseregion = (Handle)0;

    if (!PyArg_ParseTuple(_args, "Hl|O&",
                          &eventMask,
                          &sleep,
                          OptResObj_Convert, &mouseregion))
        return NULL;
    _rv = WaitNextEvent(eventMask,
                        &theEvent,
                        sleep,
                        (RgnHandle)mouseregion);
    _res = Py_BuildValue("bO&",
                         _rv,
                         PyMac_BuildEventRecord, &theEvent);
    return _res;

}

static PyMethodDef Evt_methods[] = {
    {"GetMouse", (PyCFunction)Evt_GetMouse, 1,
     PyDoc_STR("() -> (Point mouseLoc)")},
    {"Button", (PyCFunction)Evt_Button, 1,
     PyDoc_STR("() -> (Boolean _rv)")},
    {"StillDown", (PyCFunction)Evt_StillDown, 1,
     PyDoc_STR("() -> (Boolean _rv)")},
    {"WaitMouseUp", (PyCFunction)Evt_WaitMouseUp, 1,
     PyDoc_STR("() -> (Boolean _rv)")},
    {"GetCaretTime", (PyCFunction)Evt_GetCaretTime, 1,
     PyDoc_STR("() -> (UInt32 _rv)")},
    {"GetKeys", (PyCFunction)Evt_GetKeys, 1,
     PyDoc_STR("() -> (KeyMap theKeys)")},
    {"GetDblTime", (PyCFunction)Evt_GetDblTime, 1,
     PyDoc_STR("() -> (UInt32 _rv)")},
    {"SetEventMask", (PyCFunction)Evt_SetEventMask, 1,
     PyDoc_STR("(EventMask value) -> None")},
    {"GetNextEvent", (PyCFunction)Evt_GetNextEvent, 1,
     PyDoc_STR("(EventMask eventMask) -> (Boolean _rv, EventRecord theEvent)")},
    {"EventAvail", (PyCFunction)Evt_EventAvail, 1,
     PyDoc_STR("(EventMask eventMask) -> (Boolean _rv, EventRecord theEvent)")},
    {"PostEvent", (PyCFunction)Evt_PostEvent, 1,
     PyDoc_STR("(EventKind eventNum, UInt32 eventMsg) -> None")},
    {"FlushEvents", (PyCFunction)Evt_FlushEvents, 1,
     PyDoc_STR("(EventMask whichMask, EventMask stopMask) -> None")},
    {"GetGlobalMouse", (PyCFunction)Evt_GetGlobalMouse, 1,
     PyDoc_STR("() -> (Point globalMouse)")},
    {"GetCurrentKeyModifiers", (PyCFunction)Evt_GetCurrentKeyModifiers, 1,
     PyDoc_STR("() -> (UInt32 _rv)")},
    {"CheckEventQueueForUserCancel", (PyCFunction)Evt_CheckEventQueueForUserCancel, 1,
     PyDoc_STR("() -> (Boolean _rv)")},
    {"KeyScript", (PyCFunction)Evt_KeyScript, 1,
     PyDoc_STR("(short code) -> None")},
    {"IsCmdChar", (PyCFunction)Evt_IsCmdChar, 1,
     PyDoc_STR("(EventRecord event, short test) -> (Boolean _rv)")},
    {"LMGetKeyThresh", (PyCFunction)Evt_LMGetKeyThresh, 1,
     PyDoc_STR("() -> (SInt16 _rv)")},
    {"LMSetKeyThresh", (PyCFunction)Evt_LMSetKeyThresh, 1,
     PyDoc_STR("(SInt16 value) -> None")},
    {"LMGetKeyRepThresh", (PyCFunction)Evt_LMGetKeyRepThresh, 1,
     PyDoc_STR("() -> (SInt16 _rv)")},
    {"LMSetKeyRepThresh", (PyCFunction)Evt_LMSetKeyRepThresh, 1,
     PyDoc_STR("(SInt16 value) -> None")},
    {"LMGetKbdLast", (PyCFunction)Evt_LMGetKbdLast, 1,
     PyDoc_STR("() -> (UInt8 _rv)")},
    {"LMSetKbdLast", (PyCFunction)Evt_LMSetKbdLast, 1,
     PyDoc_STR("(UInt8 value) -> None")},
    {"LMGetKbdType", (PyCFunction)Evt_LMGetKbdType, 1,
     PyDoc_STR("() -> (UInt8 _rv)")},
    {"LMSetKbdType", (PyCFunction)Evt_LMSetKbdType, 1,
     PyDoc_STR("(UInt8 value) -> None")},
    {"TickCount", (PyCFunction)Evt_TickCount, 1,
     PyDoc_STR("() -> (UInt32 _rv)")},
    {"WaitNextEvent", (PyCFunction)Evt_WaitNextEvent, 1,
     PyDoc_STR("(EventMask eventMask, UInt32 sleep [,RegionHandle]) -> (Boolean _rv, EventRecord theEvent)")},
    {NULL, NULL, 0}
};


#else /* __LP64__ */

static PyMethodDef Evt_methods[] = {
    {NULL, NULL, 0}
};
#endif /* __LP64__ */


void init_Evt(void)
{
    PyObject *m;
#ifndef __LP64__
    PyObject *d;
#endif /* __LP64__ */




    m = Py_InitModule("_Evt", Evt_methods);
#ifndef __LP64__
    d = PyModule_GetDict(m);
    Evt_Error = PyMac_GetOSErrException();
    if (Evt_Error == NULL ||
        PyDict_SetItemString(d, "Error", Evt_Error) != 0)
        return;
#endif /* __LP64__ */
}

/* ======================== End module _Evt ========================= */

