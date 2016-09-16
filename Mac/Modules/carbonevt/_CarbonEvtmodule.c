
/* ======================= Module _CarbonEvt ======================== */

#include "Python.h"
#include "pymactoolbox.h"

#if APPLE_SUPPORTS_QUICKTIME


/* Macro to test whether a weak-loaded CFM function exists */
#define PyMac_PRECHECK(rtn) do { if ( &rtn == NULL )  {\
    PyErr_SetString(PyExc_NotImplementedError, \
    "Not available in this shared library/OS version"); \
    return NULL; \
    }} while(0)


#include <Carbon/Carbon.h>

extern int CFStringRef_New(CFStringRef *);

extern int CFStringRef_Convert(PyObject *, CFStringRef *);
extern int CFBundleRef_Convert(PyObject *, CFBundleRef *);

int EventTargetRef_Convert(PyObject *, EventTargetRef *);
PyObject *EventHandlerCallRef_New(EventHandlerCallRef itself);
PyObject *EventRef_New(EventRef itself);

/********** EventTypeSpec *******/
static int
EventTypeSpec_Convert(PyObject *v, EventTypeSpec *out)
{
    if (PyArg_Parse(v, "(O&l)",
                    PyMac_GetOSType, &(out->eventClass),
                    &(out->eventKind)))
        return 1;
    return 0;
}

/********** end EventTypeSpec *******/

/********** HIPoint *******/

#if 0  /* XXX doesn't compile */
static PyObject*
HIPoint_New(HIPoint *in)
{
    return Py_BuildValue("ff", in->x, in->y);
}

static int
HIPoint_Convert(PyObject *v, HIPoint *out)
{
    if (PyArg_ParseTuple(v, "ff", &(out->x), &(out->y)))
        return 1;
    return NULL;
}
#endif

/********** end HIPoint *******/

/********** EventHotKeyID *******/

static int
EventHotKeyID_Convert(PyObject *v, EventHotKeyID *out)
{
    if (PyArg_ParseTuple(v, "ll", &out->signature, &out->id))
        return 1;
    return 0;
}

/********** end EventHotKeyID *******/

/******** myEventHandler ***********/

static EventHandlerUPP myEventHandlerUPP;

static pascal OSStatus
myEventHandler(EventHandlerCallRef handlerRef, EventRef event, void *outPyObject) {
    PyObject *retValue;
    int status;

    retValue = PyObject_CallFunction((PyObject *)outPyObject, "O&O&",
                                     EventHandlerCallRef_New, handlerRef,
                                     EventRef_New, event);
    if (retValue == NULL) {
        PySys_WriteStderr("Error in event handler callback:\n");
        PyErr_Print();  /* this also clears the error */
        status = noErr; /* complain? how? */
    } else {
        if (retValue == Py_None)
            status = noErr;
        else if (PyInt_Check(retValue)) {
            status = PyInt_AsLong(retValue);
        } else
            status = noErr; /* wrong object type, complain? */
        Py_DECREF(retValue);
    }

    return status;
}

/******** end myEventHandler ***********/


static PyObject *CarbonEvents_Error;

/* ---------------------- Object type EventRef ---------------------- */

PyTypeObject EventRef_Type;

#define EventRef_Check(x) ((x)->ob_type == &EventRef_Type || PyObject_TypeCheck((x), &EventRef_Type))

typedef struct EventRefObject {
    PyObject_HEAD
    EventRef ob_itself;
} EventRefObject;

PyObject *EventRef_New(EventRef itself)
{
    EventRefObject *it;
    it = PyObject_NEW(EventRefObject, &EventRef_Type);
    if (it == NULL) return NULL;
    it->ob_itself = itself;
    return (PyObject *)it;
}

int EventRef_Convert(PyObject *v, EventRef *p_itself)
{
    if (!EventRef_Check(v))
    {
        PyErr_SetString(PyExc_TypeError, "EventRef required");
        return 0;
    }
    *p_itself = ((EventRefObject *)v)->ob_itself;
    return 1;
}

static void EventRef_dealloc(EventRefObject *self)
{
    /* Cleanup of self->ob_itself goes here */
    self->ob_type->tp_free((PyObject *)self);
}

static PyObject *EventRef_RetainEvent(EventRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    EventRef _rv;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = RetainEvent(_self->ob_itself);
    _res = Py_BuildValue("O&",
                         EventRef_New, _rv);
    return _res;
}

static PyObject *EventRef_GetEventRetainCount(EventRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    UInt32 _rv;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetEventRetainCount(_self->ob_itself);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *EventRef_ReleaseEvent(EventRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    ReleaseEvent(_self->ob_itself);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *EventRef_SetEventParameter(EventRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    OSType inName;
    OSType inType;
    char *inDataPtr__in__;
    long inDataPtr__len__;
    int inDataPtr__in_len__;
    if (!PyArg_ParseTuple(_args, "O&O&s#",
                          PyMac_GetOSType, &inName,
                          PyMac_GetOSType, &inType,
                          &inDataPtr__in__, &inDataPtr__in_len__))
        return NULL;
    inDataPtr__len__ = inDataPtr__in_len__;
    _err = SetEventParameter(_self->ob_itself,
                             inName,
                             inType,
                             inDataPtr__len__, inDataPtr__in__);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *EventRef_GetEventClass(EventRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    UInt32 _rv;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetEventClass(_self->ob_itself);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *EventRef_GetEventKind(EventRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    UInt32 _rv;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetEventKind(_self->ob_itself);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *EventRef_GetEventTime(EventRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    double _rv;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetEventTime(_self->ob_itself);
    _res = Py_BuildValue("d",
                         _rv);
    return _res;
}

static PyObject *EventRef_SetEventTime(EventRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    double inTime;
    if (!PyArg_ParseTuple(_args, "d",
                          &inTime))
        return NULL;
    _err = SetEventTime(_self->ob_itself,
                        inTime);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *EventRef_IsUserCancelEventRef(EventRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = IsUserCancelEventRef(_self->ob_itself);
    _res = Py_BuildValue("b",
                         _rv);
    return _res;
}

static PyObject *EventRef_ConvertEventRefToEventRecord(EventRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
    EventRecord outEvent;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = ConvertEventRefToEventRecord(_self->ob_itself,
                                       &outEvent);
    _res = Py_BuildValue("bO&",
                         _rv,
                         PyMac_BuildEventRecord, &outEvent);
    return _res;
}

static PyObject *EventRef_IsEventInMask(EventRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
    UInt16 inMask;
    if (!PyArg_ParseTuple(_args, "H",
                          &inMask))
        return NULL;
    _rv = IsEventInMask(_self->ob_itself,
                        inMask);
    _res = Py_BuildValue("b",
                         _rv);
    return _res;
}

static PyObject *EventRef_SendEventToEventTarget(EventRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    EventTargetRef inTarget;
    if (!PyArg_ParseTuple(_args, "O&",
                          EventTargetRef_Convert, &inTarget))
        return NULL;
    _err = SendEventToEventTarget(_self->ob_itself,
                                  inTarget);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *EventRef_GetEventParameter(EventRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;

    UInt32 bufferSize;
    EventParamName inName;
    EventParamType inType;
    OSErr _err;
    void * buffer;

    if (!PyArg_ParseTuple(_args, "O&O&", PyMac_GetOSType, &inName, PyMac_GetOSType, &inType))
          return NULL;

    /* Figure out the size by passing a null buffer to GetEventParameter */
    _err = GetEventParameter(_self->ob_itself, inName, inType, NULL, 0, &bufferSize, NULL);

    if (_err != noErr)
          return PyMac_Error(_err);
    buffer = PyMem_NEW(char, bufferSize);
    if (buffer == NULL)
          return PyErr_NoMemory();

    _err = GetEventParameter(_self->ob_itself, inName, inType, NULL, bufferSize, NULL, buffer);

    if (_err != noErr) {
          PyMem_DEL(buffer);
          return PyMac_Error(_err);
    }
    _res = Py_BuildValue("s#", buffer, bufferSize);
    PyMem_DEL(buffer);
    return _res;

}

static PyMethodDef EventRef_methods[] = {
    {"RetainEvent", (PyCFunction)EventRef_RetainEvent, 1,
     PyDoc_STR("() -> (EventRef _rv)")},
    {"GetEventRetainCount", (PyCFunction)EventRef_GetEventRetainCount, 1,
     PyDoc_STR("() -> (UInt32 _rv)")},
    {"ReleaseEvent", (PyCFunction)EventRef_ReleaseEvent, 1,
     PyDoc_STR("() -> None")},
    {"SetEventParameter", (PyCFunction)EventRef_SetEventParameter, 1,
     PyDoc_STR("(OSType inName, OSType inType, Buffer inDataPtr) -> None")},
    {"GetEventClass", (PyCFunction)EventRef_GetEventClass, 1,
     PyDoc_STR("() -> (UInt32 _rv)")},
    {"GetEventKind", (PyCFunction)EventRef_GetEventKind, 1,
     PyDoc_STR("() -> (UInt32 _rv)")},
    {"GetEventTime", (PyCFunction)EventRef_GetEventTime, 1,
     PyDoc_STR("() -> (double _rv)")},
    {"SetEventTime", (PyCFunction)EventRef_SetEventTime, 1,
     PyDoc_STR("(double inTime) -> None")},
    {"IsUserCancelEventRef", (PyCFunction)EventRef_IsUserCancelEventRef, 1,
     PyDoc_STR("() -> (Boolean _rv)")},
    {"ConvertEventRefToEventRecord", (PyCFunction)EventRef_ConvertEventRefToEventRecord, 1,
     PyDoc_STR("() -> (Boolean _rv, EventRecord outEvent)")},
    {"IsEventInMask", (PyCFunction)EventRef_IsEventInMask, 1,
     PyDoc_STR("(UInt16 inMask) -> (Boolean _rv)")},
    {"SendEventToEventTarget", (PyCFunction)EventRef_SendEventToEventTarget, 1,
     PyDoc_STR("(EventTargetRef inTarget) -> None")},
    {"GetEventParameter", (PyCFunction)EventRef_GetEventParameter, 1,
     PyDoc_STR("(EventParamName eventName, EventParamType eventType) -> (String eventParamData)")},
    {NULL, NULL, 0}
};

#define EventRef_getsetlist NULL


#define EventRef_compare NULL

#define EventRef_repr NULL

#define EventRef_hash NULL
#define EventRef_tp_init 0

#define EventRef_tp_alloc PyType_GenericAlloc

static PyObject *EventRef_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
    PyObject *_self;
    EventRef itself;
    char *kw[] = {"itself", 0};

    if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, EventRef_Convert, &itself)) return NULL;
    if ((_self = type->tp_alloc(type, 0)) == NULL) return NULL;
    ((EventRefObject *)_self)->ob_itself = itself;
    return _self;
}

#define EventRef_tp_free PyObject_Del


PyTypeObject EventRef_Type = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "_CarbonEvt.EventRef", /*tp_name*/
    sizeof(EventRefObject), /*tp_basicsize*/
    0, /*tp_itemsize*/
    /* methods */
    (destructor) EventRef_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)0, /*tp_getattr*/
    (setattrfunc)0, /*tp_setattr*/
    (cmpfunc) EventRef_compare, /*tp_compare*/
    (reprfunc) EventRef_repr, /*tp_repr*/
    (PyNumberMethods *)0, /* tp_as_number */
    (PySequenceMethods *)0, /* tp_as_sequence */
    (PyMappingMethods *)0, /* tp_as_mapping */
    (hashfunc) EventRef_hash, /*tp_hash*/
    0, /*tp_call*/
    0, /*tp_str*/
    PyObject_GenericGetAttr, /*tp_getattro*/
    PyObject_GenericSetAttr, /*tp_setattro */
    0, /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, /* tp_flags */
    0, /*tp_doc*/
    0, /*tp_traverse*/
    0, /*tp_clear*/
    0, /*tp_richcompare*/
    0, /*tp_weaklistoffset*/
    0, /*tp_iter*/
    0, /*tp_iternext*/
    EventRef_methods, /* tp_methods */
    0, /*tp_members*/
    EventRef_getsetlist, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    EventRef_tp_init, /* tp_init */
    EventRef_tp_alloc, /* tp_alloc */
    EventRef_tp_new, /* tp_new */
    EventRef_tp_free, /* tp_free */
};

/* -------------------- End object type EventRef -------------------- */


/* ------------------- Object type EventQueueRef -------------------- */

PyTypeObject EventQueueRef_Type;

#define EventQueueRef_Check(x) ((x)->ob_type == &EventQueueRef_Type || PyObject_TypeCheck((x), &EventQueueRef_Type))

typedef struct EventQueueRefObject {
    PyObject_HEAD
    EventQueueRef ob_itself;
} EventQueueRefObject;

PyObject *EventQueueRef_New(EventQueueRef itself)
{
    EventQueueRefObject *it;
    it = PyObject_NEW(EventQueueRefObject, &EventQueueRef_Type);
    if (it == NULL) return NULL;
    it->ob_itself = itself;
    return (PyObject *)it;
}

int EventQueueRef_Convert(PyObject *v, EventQueueRef *p_itself)
{
    if (!EventQueueRef_Check(v))
    {
        PyErr_SetString(PyExc_TypeError, "EventQueueRef required");
        return 0;
    }
    *p_itself = ((EventQueueRefObject *)v)->ob_itself;
    return 1;
}

static void EventQueueRef_dealloc(EventQueueRefObject *self)
{
    /* Cleanup of self->ob_itself goes here */
    self->ob_type->tp_free((PyObject *)self);
}

static PyObject *EventQueueRef_PostEventToQueue(EventQueueRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    EventRef inEvent;
    SInt16 inPriority;
    if (!PyArg_ParseTuple(_args, "O&h",
                          EventRef_Convert, &inEvent,
                          &inPriority))
        return NULL;
    _err = PostEventToQueue(_self->ob_itself,
                            inEvent,
                            inPriority);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *EventQueueRef_FlushEventsMatchingListFromQueue(EventQueueRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 inNumTypes;
    EventTypeSpec inList;
    if (!PyArg_ParseTuple(_args, "lO&",
                          &inNumTypes,
                          EventTypeSpec_Convert, &inList))
        return NULL;
    _err = FlushEventsMatchingListFromQueue(_self->ob_itself,
                                            inNumTypes,
                                            &inList);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *EventQueueRef_FlushEventQueue(EventQueueRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = FlushEventQueue(_self->ob_itself);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *EventQueueRef_GetNumEventsInQueue(EventQueueRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    UInt32 _rv;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetNumEventsInQueue(_self->ob_itself);
    _res = Py_BuildValue("l",
                         _rv);
    return _res;
}

static PyObject *EventQueueRef_RemoveEventFromQueue(EventQueueRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    EventRef inEvent;
    if (!PyArg_ParseTuple(_args, "O&",
                          EventRef_Convert, &inEvent))
        return NULL;
    _err = RemoveEventFromQueue(_self->ob_itself,
                                inEvent);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *EventQueueRef_IsEventInQueue(EventQueueRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
    EventRef inEvent;
    if (!PyArg_ParseTuple(_args, "O&",
                          EventRef_Convert, &inEvent))
        return NULL;
    _rv = IsEventInQueue(_self->ob_itself,
                         inEvent);
    _res = Py_BuildValue("b",
                         _rv);
    return _res;
}

static PyMethodDef EventQueueRef_methods[] = {
    {"PostEventToQueue", (PyCFunction)EventQueueRef_PostEventToQueue, 1,
     PyDoc_STR("(EventRef inEvent, SInt16 inPriority) -> None")},
    {"FlushEventsMatchingListFromQueue", (PyCFunction)EventQueueRef_FlushEventsMatchingListFromQueue, 1,
     PyDoc_STR("(UInt32 inNumTypes, EventTypeSpec inList) -> None")},
    {"FlushEventQueue", (PyCFunction)EventQueueRef_FlushEventQueue, 1,
     PyDoc_STR("() -> None")},
    {"GetNumEventsInQueue", (PyCFunction)EventQueueRef_GetNumEventsInQueue, 1,
     PyDoc_STR("() -> (UInt32 _rv)")},
    {"RemoveEventFromQueue", (PyCFunction)EventQueueRef_RemoveEventFromQueue, 1,
     PyDoc_STR("(EventRef inEvent) -> None")},
    {"IsEventInQueue", (PyCFunction)EventQueueRef_IsEventInQueue, 1,
     PyDoc_STR("(EventRef inEvent) -> (Boolean _rv)")},
    {NULL, NULL, 0}
};

#define EventQueueRef_getsetlist NULL


#define EventQueueRef_compare NULL

#define EventQueueRef_repr NULL

#define EventQueueRef_hash NULL
#define EventQueueRef_tp_init 0

#define EventQueueRef_tp_alloc PyType_GenericAlloc

static PyObject *EventQueueRef_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
    PyObject *_self;
    EventQueueRef itself;
    char *kw[] = {"itself", 0};

    if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, EventQueueRef_Convert, &itself)) return NULL;
    if ((_self = type->tp_alloc(type, 0)) == NULL) return NULL;
    ((EventQueueRefObject *)_self)->ob_itself = itself;
    return _self;
}

#define EventQueueRef_tp_free PyObject_Del


PyTypeObject EventQueueRef_Type = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "_CarbonEvt.EventQueueRef", /*tp_name*/
    sizeof(EventQueueRefObject), /*tp_basicsize*/
    0, /*tp_itemsize*/
    /* methods */
    (destructor) EventQueueRef_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)0, /*tp_getattr*/
    (setattrfunc)0, /*tp_setattr*/
    (cmpfunc) EventQueueRef_compare, /*tp_compare*/
    (reprfunc) EventQueueRef_repr, /*tp_repr*/
    (PyNumberMethods *)0, /* tp_as_number */
    (PySequenceMethods *)0, /* tp_as_sequence */
    (PyMappingMethods *)0, /* tp_as_mapping */
    (hashfunc) EventQueueRef_hash, /*tp_hash*/
    0, /*tp_call*/
    0, /*tp_str*/
    PyObject_GenericGetAttr, /*tp_getattro*/
    PyObject_GenericSetAttr, /*tp_setattro */
    0, /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, /* tp_flags */
    0, /*tp_doc*/
    0, /*tp_traverse*/
    0, /*tp_clear*/
    0, /*tp_richcompare*/
    0, /*tp_weaklistoffset*/
    0, /*tp_iter*/
    0, /*tp_iternext*/
    EventQueueRef_methods, /* tp_methods */
    0, /*tp_members*/
    EventQueueRef_getsetlist, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    EventQueueRef_tp_init, /* tp_init */
    EventQueueRef_tp_alloc, /* tp_alloc */
    EventQueueRef_tp_new, /* tp_new */
    EventQueueRef_tp_free, /* tp_free */
};

/* ----------------- End object type EventQueueRef ------------------ */


/* -------------------- Object type EventLoopRef -------------------- */

PyTypeObject EventLoopRef_Type;

#define EventLoopRef_Check(x) ((x)->ob_type == &EventLoopRef_Type || PyObject_TypeCheck((x), &EventLoopRef_Type))

typedef struct EventLoopRefObject {
    PyObject_HEAD
    EventLoopRef ob_itself;
} EventLoopRefObject;

PyObject *EventLoopRef_New(EventLoopRef itself)
{
    EventLoopRefObject *it;
    it = PyObject_NEW(EventLoopRefObject, &EventLoopRef_Type);
    if (it == NULL) return NULL;
    it->ob_itself = itself;
    return (PyObject *)it;
}

int EventLoopRef_Convert(PyObject *v, EventLoopRef *p_itself)
{
    if (!EventLoopRef_Check(v))
    {
        PyErr_SetString(PyExc_TypeError, "EventLoopRef required");
        return 0;
    }
    *p_itself = ((EventLoopRefObject *)v)->ob_itself;
    return 1;
}

static void EventLoopRef_dealloc(EventLoopRefObject *self)
{
    /* Cleanup of self->ob_itself goes here */
    self->ob_type->tp_free((PyObject *)self);
}

static PyObject *EventLoopRef_QuitEventLoop(EventLoopRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = QuitEventLoop(_self->ob_itself);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyMethodDef EventLoopRef_methods[] = {
    {"QuitEventLoop", (PyCFunction)EventLoopRef_QuitEventLoop, 1,
     PyDoc_STR("() -> None")},
    {NULL, NULL, 0}
};

#define EventLoopRef_getsetlist NULL


#define EventLoopRef_compare NULL

#define EventLoopRef_repr NULL

#define EventLoopRef_hash NULL
#define EventLoopRef_tp_init 0

#define EventLoopRef_tp_alloc PyType_GenericAlloc

static PyObject *EventLoopRef_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
    PyObject *_self;
    EventLoopRef itself;
    char *kw[] = {"itself", 0};

    if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, EventLoopRef_Convert, &itself)) return NULL;
    if ((_self = type->tp_alloc(type, 0)) == NULL) return NULL;
    ((EventLoopRefObject *)_self)->ob_itself = itself;
    return _self;
}

#define EventLoopRef_tp_free PyObject_Del


PyTypeObject EventLoopRef_Type = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "_CarbonEvt.EventLoopRef", /*tp_name*/
    sizeof(EventLoopRefObject), /*tp_basicsize*/
    0, /*tp_itemsize*/
    /* methods */
    (destructor) EventLoopRef_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)0, /*tp_getattr*/
    (setattrfunc)0, /*tp_setattr*/
    (cmpfunc) EventLoopRef_compare, /*tp_compare*/
    (reprfunc) EventLoopRef_repr, /*tp_repr*/
    (PyNumberMethods *)0, /* tp_as_number */
    (PySequenceMethods *)0, /* tp_as_sequence */
    (PyMappingMethods *)0, /* tp_as_mapping */
    (hashfunc) EventLoopRef_hash, /*tp_hash*/
    0, /*tp_call*/
    0, /*tp_str*/
    PyObject_GenericGetAttr, /*tp_getattro*/
    PyObject_GenericSetAttr, /*tp_setattro */
    0, /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, /* tp_flags */
    0, /*tp_doc*/
    0, /*tp_traverse*/
    0, /*tp_clear*/
    0, /*tp_richcompare*/
    0, /*tp_weaklistoffset*/
    0, /*tp_iter*/
    0, /*tp_iternext*/
    EventLoopRef_methods, /* tp_methods */
    0, /*tp_members*/
    EventLoopRef_getsetlist, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    EventLoopRef_tp_init, /* tp_init */
    EventLoopRef_tp_alloc, /* tp_alloc */
    EventLoopRef_tp_new, /* tp_new */
    EventLoopRef_tp_free, /* tp_free */
};

/* ------------------ End object type EventLoopRef ------------------ */


/* ----------------- Object type EventLoopTimerRef ------------------ */

PyTypeObject EventLoopTimerRef_Type;

#define EventLoopTimerRef_Check(x) ((x)->ob_type == &EventLoopTimerRef_Type || PyObject_TypeCheck((x), &EventLoopTimerRef_Type))

typedef struct EventLoopTimerRefObject {
    PyObject_HEAD
    EventLoopTimerRef ob_itself;
} EventLoopTimerRefObject;

PyObject *EventLoopTimerRef_New(EventLoopTimerRef itself)
{
    EventLoopTimerRefObject *it;
    it = PyObject_NEW(EventLoopTimerRefObject, &EventLoopTimerRef_Type);
    if (it == NULL) return NULL;
    it->ob_itself = itself;
    return (PyObject *)it;
}

int EventLoopTimerRef_Convert(PyObject *v, EventLoopTimerRef *p_itself)
{
    if (!EventLoopTimerRef_Check(v))
    {
        PyErr_SetString(PyExc_TypeError, "EventLoopTimerRef required");
        return 0;
    }
    *p_itself = ((EventLoopTimerRefObject *)v)->ob_itself;
    return 1;
}

static void EventLoopTimerRef_dealloc(EventLoopTimerRefObject *self)
{
    /* Cleanup of self->ob_itself goes here */
    self->ob_type->tp_free((PyObject *)self);
}

static PyObject *EventLoopTimerRef_RemoveEventLoopTimer(EventLoopTimerRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = RemoveEventLoopTimer(_self->ob_itself);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *EventLoopTimerRef_SetEventLoopTimerNextFireTime(EventLoopTimerRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    double inNextFire;
    if (!PyArg_ParseTuple(_args, "d",
                          &inNextFire))
        return NULL;
    _err = SetEventLoopTimerNextFireTime(_self->ob_itself,
                                         inNextFire);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyMethodDef EventLoopTimerRef_methods[] = {
    {"RemoveEventLoopTimer", (PyCFunction)EventLoopTimerRef_RemoveEventLoopTimer, 1,
     PyDoc_STR("() -> None")},
    {"SetEventLoopTimerNextFireTime", (PyCFunction)EventLoopTimerRef_SetEventLoopTimerNextFireTime, 1,
     PyDoc_STR("(double inNextFire) -> None")},
    {NULL, NULL, 0}
};

#define EventLoopTimerRef_getsetlist NULL


#define EventLoopTimerRef_compare NULL

#define EventLoopTimerRef_repr NULL

#define EventLoopTimerRef_hash NULL
#define EventLoopTimerRef_tp_init 0

#define EventLoopTimerRef_tp_alloc PyType_GenericAlloc

static PyObject *EventLoopTimerRef_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
    PyObject *_self;
    EventLoopTimerRef itself;
    char *kw[] = {"itself", 0};

    if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, EventLoopTimerRef_Convert, &itself)) return NULL;
    if ((_self = type->tp_alloc(type, 0)) == NULL) return NULL;
    ((EventLoopTimerRefObject *)_self)->ob_itself = itself;
    return _self;
}

#define EventLoopTimerRef_tp_free PyObject_Del


PyTypeObject EventLoopTimerRef_Type = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "_CarbonEvt.EventLoopTimerRef", /*tp_name*/
    sizeof(EventLoopTimerRefObject), /*tp_basicsize*/
    0, /*tp_itemsize*/
    /* methods */
    (destructor) EventLoopTimerRef_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)0, /*tp_getattr*/
    (setattrfunc)0, /*tp_setattr*/
    (cmpfunc) EventLoopTimerRef_compare, /*tp_compare*/
    (reprfunc) EventLoopTimerRef_repr, /*tp_repr*/
    (PyNumberMethods *)0, /* tp_as_number */
    (PySequenceMethods *)0, /* tp_as_sequence */
    (PyMappingMethods *)0, /* tp_as_mapping */
    (hashfunc) EventLoopTimerRef_hash, /*tp_hash*/
    0, /*tp_call*/
    0, /*tp_str*/
    PyObject_GenericGetAttr, /*tp_getattro*/
    PyObject_GenericSetAttr, /*tp_setattro */
    0, /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, /* tp_flags */
    0, /*tp_doc*/
    0, /*tp_traverse*/
    0, /*tp_clear*/
    0, /*tp_richcompare*/
    0, /*tp_weaklistoffset*/
    0, /*tp_iter*/
    0, /*tp_iternext*/
    EventLoopTimerRef_methods, /* tp_methods */
    0, /*tp_members*/
    EventLoopTimerRef_getsetlist, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    EventLoopTimerRef_tp_init, /* tp_init */
    EventLoopTimerRef_tp_alloc, /* tp_alloc */
    EventLoopTimerRef_tp_new, /* tp_new */
    EventLoopTimerRef_tp_free, /* tp_free */
};

/* --------------- End object type EventLoopTimerRef ---------------- */


/* ------------------ Object type EventHandlerRef ------------------- */

PyTypeObject EventHandlerRef_Type;

#define EventHandlerRef_Check(x) ((x)->ob_type == &EventHandlerRef_Type || PyObject_TypeCheck((x), &EventHandlerRef_Type))

typedef struct EventHandlerRefObject {
    PyObject_HEAD
    EventHandlerRef ob_itself;
    PyObject *ob_callback;
} EventHandlerRefObject;

PyObject *EventHandlerRef_New(EventHandlerRef itself)
{
    EventHandlerRefObject *it;
    it = PyObject_NEW(EventHandlerRefObject, &EventHandlerRef_Type);
    if (it == NULL) return NULL;
    it->ob_itself = itself;
    it->ob_callback = NULL;
    return (PyObject *)it;
}

int EventHandlerRef_Convert(PyObject *v, EventHandlerRef *p_itself)
{
    if (!EventHandlerRef_Check(v))
    {
        PyErr_SetString(PyExc_TypeError, "EventHandlerRef required");
        return 0;
    }
    *p_itself = ((EventHandlerRefObject *)v)->ob_itself;
    return 1;
}

static void EventHandlerRef_dealloc(EventHandlerRefObject *self)
{
    if (self->ob_itself != NULL) {
        RemoveEventHandler(self->ob_itself);
        Py_DECREF(self->ob_callback);
    }
    self->ob_type->tp_free((PyObject *)self);
}

static PyObject *EventHandlerRef_AddEventTypesToHandler(EventHandlerRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 inNumTypes;
    EventTypeSpec inList;
    if (_self->ob_itself == NULL) {
        PyErr_SetString(CarbonEvents_Error, "Handler has been removed");
        return NULL;
    }
    if (!PyArg_ParseTuple(_args, "lO&",
                          &inNumTypes,
                          EventTypeSpec_Convert, &inList))
        return NULL;
    _err = AddEventTypesToHandler(_self->ob_itself,
                                  inNumTypes,
                                  &inList);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *EventHandlerRef_RemoveEventTypesFromHandler(EventHandlerRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 inNumTypes;
    EventTypeSpec inList;
    if (_self->ob_itself == NULL) {
        PyErr_SetString(CarbonEvents_Error, "Handler has been removed");
        return NULL;
    }
    if (!PyArg_ParseTuple(_args, "lO&",
                          &inNumTypes,
                          EventTypeSpec_Convert, &inList))
        return NULL;
    _err = RemoveEventTypesFromHandler(_self->ob_itself,
                                       inNumTypes,
                                       &inList);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *EventHandlerRef_RemoveEventHandler(EventHandlerRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;

    OSStatus _err;
    if (_self->ob_itself == NULL) {
        PyErr_SetString(CarbonEvents_Error, "Handler has been removed");
        return NULL;
    }
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = RemoveEventHandler(_self->ob_itself);
    if (_err != noErr) return PyMac_Error(_err);
    _self->ob_itself = NULL;
    Py_CLEAR(_self->ob_callback);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyMethodDef EventHandlerRef_methods[] = {
    {"AddEventTypesToHandler", (PyCFunction)EventHandlerRef_AddEventTypesToHandler, 1,
     PyDoc_STR("(UInt32 inNumTypes, EventTypeSpec inList) -> None")},
    {"RemoveEventTypesFromHandler", (PyCFunction)EventHandlerRef_RemoveEventTypesFromHandler, 1,
     PyDoc_STR("(UInt32 inNumTypes, EventTypeSpec inList) -> None")},
    {"RemoveEventHandler", (PyCFunction)EventHandlerRef_RemoveEventHandler, 1,
     PyDoc_STR("() -> None")},
    {NULL, NULL, 0}
};

#define EventHandlerRef_getsetlist NULL


#define EventHandlerRef_compare NULL

#define EventHandlerRef_repr NULL

#define EventHandlerRef_hash NULL
#define EventHandlerRef_tp_init 0

#define EventHandlerRef_tp_alloc PyType_GenericAlloc

static PyObject *EventHandlerRef_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
    PyObject *_self;
    EventHandlerRef itself;
    char *kw[] = {"itself", 0};

    if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, EventHandlerRef_Convert, &itself)) return NULL;
    if ((_self = type->tp_alloc(type, 0)) == NULL) return NULL;
    ((EventHandlerRefObject *)_self)->ob_itself = itself;
    return _self;
}

#define EventHandlerRef_tp_free PyObject_Del


PyTypeObject EventHandlerRef_Type = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "_CarbonEvt.EventHandlerRef", /*tp_name*/
    sizeof(EventHandlerRefObject), /*tp_basicsize*/
    0, /*tp_itemsize*/
    /* methods */
    (destructor) EventHandlerRef_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)0, /*tp_getattr*/
    (setattrfunc)0, /*tp_setattr*/
    (cmpfunc) EventHandlerRef_compare, /*tp_compare*/
    (reprfunc) EventHandlerRef_repr, /*tp_repr*/
    (PyNumberMethods *)0, /* tp_as_number */
    (PySequenceMethods *)0, /* tp_as_sequence */
    (PyMappingMethods *)0, /* tp_as_mapping */
    (hashfunc) EventHandlerRef_hash, /*tp_hash*/
    0, /*tp_call*/
    0, /*tp_str*/
    PyObject_GenericGetAttr, /*tp_getattro*/
    PyObject_GenericSetAttr, /*tp_setattro */
    0, /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, /* tp_flags */
    0, /*tp_doc*/
    0, /*tp_traverse*/
    0, /*tp_clear*/
    0, /*tp_richcompare*/
    0, /*tp_weaklistoffset*/
    0, /*tp_iter*/
    0, /*tp_iternext*/
    EventHandlerRef_methods, /* tp_methods */
    0, /*tp_members*/
    EventHandlerRef_getsetlist, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    EventHandlerRef_tp_init, /* tp_init */
    EventHandlerRef_tp_alloc, /* tp_alloc */
    EventHandlerRef_tp_new, /* tp_new */
    EventHandlerRef_tp_free, /* tp_free */
};

/* ---------------- End object type EventHandlerRef ----------------- */


/* ---------------- Object type EventHandlerCallRef ----------------- */

PyTypeObject EventHandlerCallRef_Type;

#define EventHandlerCallRef_Check(x) ((x)->ob_type == &EventHandlerCallRef_Type || PyObject_TypeCheck((x), &EventHandlerCallRef_Type))

typedef struct EventHandlerCallRefObject {
    PyObject_HEAD
    EventHandlerCallRef ob_itself;
} EventHandlerCallRefObject;

PyObject *EventHandlerCallRef_New(EventHandlerCallRef itself)
{
    EventHandlerCallRefObject *it;
    it = PyObject_NEW(EventHandlerCallRefObject, &EventHandlerCallRef_Type);
    if (it == NULL) return NULL;
    it->ob_itself = itself;
    return (PyObject *)it;
}

int EventHandlerCallRef_Convert(PyObject *v, EventHandlerCallRef *p_itself)
{
    if (!EventHandlerCallRef_Check(v))
    {
        PyErr_SetString(PyExc_TypeError, "EventHandlerCallRef required");
        return 0;
    }
    *p_itself = ((EventHandlerCallRefObject *)v)->ob_itself;
    return 1;
}

static void EventHandlerCallRef_dealloc(EventHandlerCallRefObject *self)
{
    /* Cleanup of self->ob_itself goes here */
    self->ob_type->tp_free((PyObject *)self);
}

static PyObject *EventHandlerCallRef_CallNextEventHandler(EventHandlerCallRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    EventRef inEvent;
    if (!PyArg_ParseTuple(_args, "O&",
                          EventRef_Convert, &inEvent))
        return NULL;
    _err = CallNextEventHandler(_self->ob_itself,
                                inEvent);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyMethodDef EventHandlerCallRef_methods[] = {
    {"CallNextEventHandler", (PyCFunction)EventHandlerCallRef_CallNextEventHandler, 1,
     PyDoc_STR("(EventRef inEvent) -> None")},
    {NULL, NULL, 0}
};

#define EventHandlerCallRef_getsetlist NULL


#define EventHandlerCallRef_compare NULL

#define EventHandlerCallRef_repr NULL

#define EventHandlerCallRef_hash NULL
#define EventHandlerCallRef_tp_init 0

#define EventHandlerCallRef_tp_alloc PyType_GenericAlloc

static PyObject *EventHandlerCallRef_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
    PyObject *_self;
    EventHandlerCallRef itself;
    char *kw[] = {"itself", 0};

    if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, EventHandlerCallRef_Convert, &itself)) return NULL;
    if ((_self = type->tp_alloc(type, 0)) == NULL) return NULL;
    ((EventHandlerCallRefObject *)_self)->ob_itself = itself;
    return _self;
}

#define EventHandlerCallRef_tp_free PyObject_Del


PyTypeObject EventHandlerCallRef_Type = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "_CarbonEvt.EventHandlerCallRef", /*tp_name*/
    sizeof(EventHandlerCallRefObject), /*tp_basicsize*/
    0, /*tp_itemsize*/
    /* methods */
    (destructor) EventHandlerCallRef_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)0, /*tp_getattr*/
    (setattrfunc)0, /*tp_setattr*/
    (cmpfunc) EventHandlerCallRef_compare, /*tp_compare*/
    (reprfunc) EventHandlerCallRef_repr, /*tp_repr*/
    (PyNumberMethods *)0, /* tp_as_number */
    (PySequenceMethods *)0, /* tp_as_sequence */
    (PyMappingMethods *)0, /* tp_as_mapping */
    (hashfunc) EventHandlerCallRef_hash, /*tp_hash*/
    0, /*tp_call*/
    0, /*tp_str*/
    PyObject_GenericGetAttr, /*tp_getattro*/
    PyObject_GenericSetAttr, /*tp_setattro */
    0, /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, /* tp_flags */
    0, /*tp_doc*/
    0, /*tp_traverse*/
    0, /*tp_clear*/
    0, /*tp_richcompare*/
    0, /*tp_weaklistoffset*/
    0, /*tp_iter*/
    0, /*tp_iternext*/
    EventHandlerCallRef_methods, /* tp_methods */
    0, /*tp_members*/
    EventHandlerCallRef_getsetlist, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    EventHandlerCallRef_tp_init, /* tp_init */
    EventHandlerCallRef_tp_alloc, /* tp_alloc */
    EventHandlerCallRef_tp_new, /* tp_new */
    EventHandlerCallRef_tp_free, /* tp_free */
};

/* -------------- End object type EventHandlerCallRef --------------- */


/* ------------------- Object type EventTargetRef ------------------- */

PyTypeObject EventTargetRef_Type;

#define EventTargetRef_Check(x) ((x)->ob_type == &EventTargetRef_Type || PyObject_TypeCheck((x), &EventTargetRef_Type))

typedef struct EventTargetRefObject {
    PyObject_HEAD
    EventTargetRef ob_itself;
} EventTargetRefObject;

PyObject *EventTargetRef_New(EventTargetRef itself)
{
    EventTargetRefObject *it;
    it = PyObject_NEW(EventTargetRefObject, &EventTargetRef_Type);
    if (it == NULL) return NULL;
    it->ob_itself = itself;
    return (PyObject *)it;
}

int EventTargetRef_Convert(PyObject *v, EventTargetRef *p_itself)
{
    if (!EventTargetRef_Check(v))
    {
        PyErr_SetString(PyExc_TypeError, "EventTargetRef required");
        return 0;
    }
    *p_itself = ((EventTargetRefObject *)v)->ob_itself;
    return 1;
}

static void EventTargetRef_dealloc(EventTargetRefObject *self)
{
    /* Cleanup of self->ob_itself goes here */
    self->ob_type->tp_free((PyObject *)self);
}

static PyObject *EventTargetRef_InstallStandardEventHandler(EventTargetRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = InstallStandardEventHandler(_self->ob_itself);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *EventTargetRef_InstallEventHandler(EventTargetRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;

    EventTypeSpec inSpec;
    PyObject *callback;
    EventHandlerRef outRef;
    OSStatus _err;

    if (!PyArg_ParseTuple(_args, "O&O", EventTypeSpec_Convert, &inSpec, &callback))
        return NULL;

    _err = InstallEventHandler(_self->ob_itself, myEventHandlerUPP, 1, &inSpec, (void *)callback, &outRef);
    if (_err != noErr) return PyMac_Error(_err);

    _res = EventHandlerRef_New(outRef);
    if (_res != NULL) {
        ((EventHandlerRefObject*)_res)->ob_callback = callback;
        Py_INCREF(callback);
    }
    return _res;
}

static PyMethodDef EventTargetRef_methods[] = {
    {"InstallStandardEventHandler", (PyCFunction)EventTargetRef_InstallStandardEventHandler, 1,
     PyDoc_STR("() -> None")},
    {"InstallEventHandler", (PyCFunction)EventTargetRef_InstallEventHandler, 1,
     PyDoc_STR("(EventTypeSpec inSpec, Method callback) -> (EventHandlerRef outRef)")},
    {NULL, NULL, 0}
};

#define EventTargetRef_getsetlist NULL


#define EventTargetRef_compare NULL

#define EventTargetRef_repr NULL

#define EventTargetRef_hash NULL
#define EventTargetRef_tp_init 0

#define EventTargetRef_tp_alloc PyType_GenericAlloc

static PyObject *EventTargetRef_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
    PyObject *_self;
    EventTargetRef itself;
    char *kw[] = {"itself", 0};

    if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, EventTargetRef_Convert, &itself)) return NULL;
    if ((_self = type->tp_alloc(type, 0)) == NULL) return NULL;
    ((EventTargetRefObject *)_self)->ob_itself = itself;
    return _self;
}

#define EventTargetRef_tp_free PyObject_Del


PyTypeObject EventTargetRef_Type = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "_CarbonEvt.EventTargetRef", /*tp_name*/
    sizeof(EventTargetRefObject), /*tp_basicsize*/
    0, /*tp_itemsize*/
    /* methods */
    (destructor) EventTargetRef_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)0, /*tp_getattr*/
    (setattrfunc)0, /*tp_setattr*/
    (cmpfunc) EventTargetRef_compare, /*tp_compare*/
    (reprfunc) EventTargetRef_repr, /*tp_repr*/
    (PyNumberMethods *)0, /* tp_as_number */
    (PySequenceMethods *)0, /* tp_as_sequence */
    (PyMappingMethods *)0, /* tp_as_mapping */
    (hashfunc) EventTargetRef_hash, /*tp_hash*/
    0, /*tp_call*/
    0, /*tp_str*/
    PyObject_GenericGetAttr, /*tp_getattro*/
    PyObject_GenericSetAttr, /*tp_setattro */
    0, /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, /* tp_flags */
    0, /*tp_doc*/
    0, /*tp_traverse*/
    0, /*tp_clear*/
    0, /*tp_richcompare*/
    0, /*tp_weaklistoffset*/
    0, /*tp_iter*/
    0, /*tp_iternext*/
    EventTargetRef_methods, /* tp_methods */
    0, /*tp_members*/
    EventTargetRef_getsetlist, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    EventTargetRef_tp_init, /* tp_init */
    EventTargetRef_tp_alloc, /* tp_alloc */
    EventTargetRef_tp_new, /* tp_new */
    EventTargetRef_tp_free, /* tp_free */
};

/* ----------------- End object type EventTargetRef ----------------- */


/* ------------------- Object type EventHotKeyRef ------------------- */

PyTypeObject EventHotKeyRef_Type;

#define EventHotKeyRef_Check(x) ((x)->ob_type == &EventHotKeyRef_Type || PyObject_TypeCheck((x), &EventHotKeyRef_Type))

typedef struct EventHotKeyRefObject {
    PyObject_HEAD
    EventHotKeyRef ob_itself;
} EventHotKeyRefObject;

PyObject *EventHotKeyRef_New(EventHotKeyRef itself)
{
    EventHotKeyRefObject *it;
    it = PyObject_NEW(EventHotKeyRefObject, &EventHotKeyRef_Type);
    if (it == NULL) return NULL;
    it->ob_itself = itself;
    return (PyObject *)it;
}

int EventHotKeyRef_Convert(PyObject *v, EventHotKeyRef *p_itself)
{
    if (!EventHotKeyRef_Check(v))
    {
        PyErr_SetString(PyExc_TypeError, "EventHotKeyRef required");
        return 0;
    }
    *p_itself = ((EventHotKeyRefObject *)v)->ob_itself;
    return 1;
}

static void EventHotKeyRef_dealloc(EventHotKeyRefObject *self)
{
    /* Cleanup of self->ob_itself goes here */
    self->ob_type->tp_free((PyObject *)self);
}

static PyObject *EventHotKeyRef_UnregisterEventHotKey(EventHotKeyRefObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _err = UnregisterEventHotKey(_self->ob_itself);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyMethodDef EventHotKeyRef_methods[] = {
    {"UnregisterEventHotKey", (PyCFunction)EventHotKeyRef_UnregisterEventHotKey, 1,
     PyDoc_STR("() -> None")},
    {NULL, NULL, 0}
};

#define EventHotKeyRef_getsetlist NULL


#define EventHotKeyRef_compare NULL

#define EventHotKeyRef_repr NULL

#define EventHotKeyRef_hash NULL
#define EventHotKeyRef_tp_init 0

#define EventHotKeyRef_tp_alloc PyType_GenericAlloc

static PyObject *EventHotKeyRef_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
    PyObject *_self;
    EventHotKeyRef itself;
    char *kw[] = {"itself", 0};

    if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, EventHotKeyRef_Convert, &itself)) return NULL;
    if ((_self = type->tp_alloc(type, 0)) == NULL) return NULL;
    ((EventHotKeyRefObject *)_self)->ob_itself = itself;
    return _self;
}

#define EventHotKeyRef_tp_free PyObject_Del


PyTypeObject EventHotKeyRef_Type = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "_CarbonEvt.EventHotKeyRef", /*tp_name*/
    sizeof(EventHotKeyRefObject), /*tp_basicsize*/
    0, /*tp_itemsize*/
    /* methods */
    (destructor) EventHotKeyRef_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    (getattrfunc)0, /*tp_getattr*/
    (setattrfunc)0, /*tp_setattr*/
    (cmpfunc) EventHotKeyRef_compare, /*tp_compare*/
    (reprfunc) EventHotKeyRef_repr, /*tp_repr*/
    (PyNumberMethods *)0, /* tp_as_number */
    (PySequenceMethods *)0, /* tp_as_sequence */
    (PyMappingMethods *)0, /* tp_as_mapping */
    (hashfunc) EventHotKeyRef_hash, /*tp_hash*/
    0, /*tp_call*/
    0, /*tp_str*/
    PyObject_GenericGetAttr, /*tp_getattro*/
    PyObject_GenericSetAttr, /*tp_setattro */
    0, /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, /* tp_flags */
    0, /*tp_doc*/
    0, /*tp_traverse*/
    0, /*tp_clear*/
    0, /*tp_richcompare*/
    0, /*tp_weaklistoffset*/
    0, /*tp_iter*/
    0, /*tp_iternext*/
    EventHotKeyRef_methods, /* tp_methods */
    0, /*tp_members*/
    EventHotKeyRef_getsetlist, /*tp_getset*/
    0, /*tp_base*/
    0, /*tp_dict*/
    0, /*tp_descr_get*/
    0, /*tp_descr_set*/
    0, /*tp_dictoffset*/
    EventHotKeyRef_tp_init, /* tp_init */
    EventHotKeyRef_tp_alloc, /* tp_alloc */
    EventHotKeyRef_tp_new, /* tp_new */
    EventHotKeyRef_tp_free, /* tp_free */
};

/* ----------------- End object type EventHotKeyRef ----------------- */


static PyObject *CarbonEvents_GetCurrentEventLoop(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    EventLoopRef _rv;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetCurrentEventLoop();
    _res = Py_BuildValue("O&",
                         EventLoopRef_New, _rv);
    return _res;
}

static PyObject *CarbonEvents_GetMainEventLoop(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    EventLoopRef _rv;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetMainEventLoop();
    _res = Py_BuildValue("O&",
                         EventLoopRef_New, _rv);
    return _res;
}

static PyObject *CarbonEvents_RunCurrentEventLoop(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    double inTimeout;
    if (!PyArg_ParseTuple(_args, "d",
                          &inTimeout))
        return NULL;
    _err = RunCurrentEventLoop(inTimeout);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CarbonEvents_ReceiveNextEvent(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 inNumTypes;
    EventTypeSpec inList;
    double inTimeout;
    Boolean inPullEvent;
    EventRef outEvent;
    if (!PyArg_ParseTuple(_args, "lO&db",
                          &inNumTypes,
                          EventTypeSpec_Convert, &inList,
                          &inTimeout,
                          &inPullEvent))
        return NULL;
    _err = ReceiveNextEvent(inNumTypes,
                            &inList,
                            inTimeout,
                            inPullEvent,
                            &outEvent);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         EventRef_New, outEvent);
    return _res;
}

static PyObject *CarbonEvents_GetCurrentEventQueue(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    EventQueueRef _rv;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetCurrentEventQueue();
    _res = Py_BuildValue("O&",
                         EventQueueRef_New, _rv);
    return _res;
}

static PyObject *CarbonEvents_GetMainEventQueue(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    EventQueueRef _rv;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetMainEventQueue();
    _res = Py_BuildValue("O&",
                         EventQueueRef_New, _rv);
    return _res;
}

static PyObject *CarbonEvents_GetCurrentEventTime(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    double _rv;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetCurrentEventTime();
    _res = Py_BuildValue("d",
                         _rv);
    return _res;
}

static PyObject *CarbonEvents_TrackMouseLocation(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    GrafPtr inPort;
    Point outPt;
    UInt16 outResult;
    if (!PyArg_ParseTuple(_args, "O&",
                          GrafObj_Convert, &inPort))
        return NULL;
    _err = TrackMouseLocation(inPort,
                              &outPt,
                              &outResult);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&H",
                         PyMac_BuildPoint, outPt,
                         outResult);
    return _res;
}

static PyObject *CarbonEvents_TrackMouseLocationWithOptions(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    GrafPtr inPort;
    OptionBits inOptions;
    double inTimeout;
    Point outPt;
    UInt32 outModifiers;
    UInt16 outResult;
    if (!PyArg_ParseTuple(_args, "O&ld",
                          GrafObj_Convert, &inPort,
                          &inOptions,
                          &inTimeout))
        return NULL;
    _err = TrackMouseLocationWithOptions(inPort,
                                         inOptions,
                                         inTimeout,
                                         &outPt,
                                         &outModifiers,
                                         &outResult);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&lH",
                         PyMac_BuildPoint, outPt,
                         outModifiers,
                         outResult);
    return _res;
}

static PyObject *CarbonEvents_TrackMouseRegion(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    GrafPtr inPort;
    RgnHandle inRegion;
    Boolean ioWasInRgn;
    UInt16 outResult;
    if (!PyArg_ParseTuple(_args, "O&O&b",
                          GrafObj_Convert, &inPort,
                          ResObj_Convert, &inRegion,
                          &ioWasInRgn))
        return NULL;
    _err = TrackMouseRegion(inPort,
                            inRegion,
                            &ioWasInRgn,
                            &outResult);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("bH",
                         ioWasInRgn,
                         outResult);
    return _res;
}

static PyObject *CarbonEvents_GetLastUserEventTime(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    double _rv;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetLastUserEventTime();
    _res = Py_BuildValue("d",
                         _rv);
    return _res;
}

static PyObject *CarbonEvents_IsMouseCoalescingEnabled(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    Boolean _rv;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = IsMouseCoalescingEnabled();
    _res = Py_BuildValue("b",
                         _rv);
    return _res;
}

static PyObject *CarbonEvents_SetMouseCoalescingEnabled(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    Boolean inNewState;
    Boolean outOldState;
    if (!PyArg_ParseTuple(_args, "b",
                          &inNewState))
        return NULL;
    _err = SetMouseCoalescingEnabled(inNewState,
                                     &outOldState);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("b",
                         outOldState);
    return _res;
}

static PyObject *CarbonEvents_GetWindowEventTarget(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    EventTargetRef _rv;
    WindowPtr inWindow;
    if (!PyArg_ParseTuple(_args, "O&",
                          WinObj_Convert, &inWindow))
        return NULL;
    _rv = GetWindowEventTarget(inWindow);
    _res = Py_BuildValue("O&",
                         EventTargetRef_New, _rv);
    return _res;
}

static PyObject *CarbonEvents_GetControlEventTarget(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    EventTargetRef _rv;
    ControlHandle inControl;
    if (!PyArg_ParseTuple(_args, "O&",
                          CtlObj_Convert, &inControl))
        return NULL;
    _rv = GetControlEventTarget(inControl);
    _res = Py_BuildValue("O&",
                         EventTargetRef_New, _rv);
    return _res;
}

static PyObject *CarbonEvents_GetMenuEventTarget(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    EventTargetRef _rv;
    MenuHandle inMenu;
    if (!PyArg_ParseTuple(_args, "O&",
                          MenuObj_Convert, &inMenu))
        return NULL;
    _rv = GetMenuEventTarget(inMenu);
    _res = Py_BuildValue("O&",
                         EventTargetRef_New, _rv);
    return _res;
}

static PyObject *CarbonEvents_GetApplicationEventTarget(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    EventTargetRef _rv;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetApplicationEventTarget();
    _res = Py_BuildValue("O&",
                         EventTargetRef_New, _rv);
    return _res;
}

static PyObject *CarbonEvents_GetUserFocusEventTarget(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    EventTargetRef _rv;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetUserFocusEventTarget();
    _res = Py_BuildValue("O&",
                         EventTargetRef_New, _rv);
    return _res;
}

static PyObject *CarbonEvents_GetEventDispatcherTarget(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    EventTargetRef _rv;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetEventDispatcherTarget();
    _res = Py_BuildValue("O&",
                         EventTargetRef_New, _rv);
    return _res;
}

static PyObject *CarbonEvents_RunApplicationEventLoop(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    RunApplicationEventLoop();
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CarbonEvents_QuitApplicationEventLoop(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    QuitApplicationEventLoop();
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CarbonEvents_RunAppModalLoopForWindow(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr inWindow;
    if (!PyArg_ParseTuple(_args, "O&",
                          WinObj_Convert, &inWindow))
        return NULL;
    _err = RunAppModalLoopForWindow(inWindow);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CarbonEvents_QuitAppModalLoopForWindow(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr inWindow;
    if (!PyArg_ParseTuple(_args, "O&",
                          WinObj_Convert, &inWindow))
        return NULL;
    _err = QuitAppModalLoopForWindow(inWindow);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CarbonEvents_BeginAppModalStateForWindow(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr inWindow;
    if (!PyArg_ParseTuple(_args, "O&",
                          WinObj_Convert, &inWindow))
        return NULL;
    _err = BeginAppModalStateForWindow(inWindow);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CarbonEvents_EndAppModalStateForWindow(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr inWindow;
    if (!PyArg_ParseTuple(_args, "O&",
                          WinObj_Convert, &inWindow))
        return NULL;
    _err = EndAppModalStateForWindow(inWindow);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CarbonEvents_SetUserFocusWindow(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr inWindow;
    if (!PyArg_ParseTuple(_args, "O&",
                          WinObj_Convert, &inWindow))
        return NULL;
    _err = SetUserFocusWindow(inWindow);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CarbonEvents_GetUserFocusWindow(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    WindowPtr _rv;
    if (!PyArg_ParseTuple(_args, ""))
        return NULL;
    _rv = GetUserFocusWindow();
    _res = Py_BuildValue("O&",
                         WinObj_New, _rv);
    return _res;
}

static PyObject *CarbonEvents_SetWindowDefaultButton(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr inWindow;
    ControlHandle inControl;
    if (!PyArg_ParseTuple(_args, "O&O&",
                          WinObj_Convert, &inWindow,
                          CtlObj_Convert, &inControl))
        return NULL;
    _err = SetWindowDefaultButton(inWindow,
                                  inControl);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CarbonEvents_SetWindowCancelButton(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr inWindow;
    ControlHandle inControl;
    if (!PyArg_ParseTuple(_args, "O&O&",
                          WinObj_Convert, &inWindow,
                          CtlObj_Convert, &inControl))
        return NULL;
    _err = SetWindowCancelButton(inWindow,
                                 inControl);
    if (_err != noErr) return PyMac_Error(_err);
    Py_INCREF(Py_None);
    _res = Py_None;
    return _res;
}

static PyObject *CarbonEvents_GetWindowDefaultButton(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr inWindow;
    ControlHandle outControl;
    if (!PyArg_ParseTuple(_args, "O&",
                          WinObj_Convert, &inWindow))
        return NULL;
    _err = GetWindowDefaultButton(inWindow,
                                  &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *CarbonEvents_GetWindowCancelButton(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    WindowPtr inWindow;
    ControlHandle outControl;
    if (!PyArg_ParseTuple(_args, "O&",
                          WinObj_Convert, &inWindow))
        return NULL;
    _err = GetWindowCancelButton(inWindow,
                                 &outControl);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         CtlObj_New, outControl);
    return _res;
}

static PyObject *CarbonEvents_RegisterEventHotKey(PyObject *_self, PyObject *_args)
{
    PyObject *_res = NULL;
    OSStatus _err;
    UInt32 inHotKeyCode;
    UInt32 inHotKeyModifiers;
    EventHotKeyID inHotKeyID;
    EventTargetRef inTarget;
    OptionBits inOptions;
    EventHotKeyRef outRef;
    if (!PyArg_ParseTuple(_args, "llO&O&l",
                          &inHotKeyCode,
                          &inHotKeyModifiers,
                          EventHotKeyID_Convert, &inHotKeyID,
                          EventTargetRef_Convert, &inTarget,
                          &inOptions))
        return NULL;
    _err = RegisterEventHotKey(inHotKeyCode,
                               inHotKeyModifiers,
                               inHotKeyID,
                               inTarget,
                               inOptions,
                               &outRef);
    if (_err != noErr) return PyMac_Error(_err);
    _res = Py_BuildValue("O&",
                         EventHotKeyRef_New, outRef);
    return _res;
}

static PyMethodDef CarbonEvents_methods[] = {
    {"GetCurrentEventLoop", (PyCFunction)CarbonEvents_GetCurrentEventLoop, 1,
     PyDoc_STR("() -> (EventLoopRef _rv)")},
    {"GetMainEventLoop", (PyCFunction)CarbonEvents_GetMainEventLoop, 1,
     PyDoc_STR("() -> (EventLoopRef _rv)")},
    {"RunCurrentEventLoop", (PyCFunction)CarbonEvents_RunCurrentEventLoop, 1,
     PyDoc_STR("(double inTimeout) -> None")},
    {"ReceiveNextEvent", (PyCFunction)CarbonEvents_ReceiveNextEvent, 1,
     PyDoc_STR("(UInt32 inNumTypes, EventTypeSpec inList, double inTimeout, Boolean inPullEvent) -> (EventRef outEvent)")},
    {"GetCurrentEventQueue", (PyCFunction)CarbonEvents_GetCurrentEventQueue, 1,
     PyDoc_STR("() -> (EventQueueRef _rv)")},
    {"GetMainEventQueue", (PyCFunction)CarbonEvents_GetMainEventQueue, 1,
     PyDoc_STR("() -> (EventQueueRef _rv)")},
    {"GetCurrentEventTime", (PyCFunction)CarbonEvents_GetCurrentEventTime, 1,
     PyDoc_STR("() -> (double _rv)")},
    {"TrackMouseLocation", (PyCFunction)CarbonEvents_TrackMouseLocation, 1,
     PyDoc_STR("(GrafPtr inPort) -> (Point outPt, UInt16 outResult)")},
    {"TrackMouseLocationWithOptions", (PyCFunction)CarbonEvents_TrackMouseLocationWithOptions, 1,
     PyDoc_STR("(GrafPtr inPort, OptionBits inOptions, double inTimeout) -> (Point outPt, UInt32 outModifiers, UInt16 outResult)")},
    {"TrackMouseRegion", (PyCFunction)CarbonEvents_TrackMouseRegion, 1,
     PyDoc_STR("(GrafPtr inPort, RgnHandle inRegion, Boolean ioWasInRgn) -> (Boolean ioWasInRgn, UInt16 outResult)")},
    {"GetLastUserEventTime", (PyCFunction)CarbonEvents_GetLastUserEventTime, 1,
     PyDoc_STR("() -> (double _rv)")},
    {"IsMouseCoalescingEnabled", (PyCFunction)CarbonEvents_IsMouseCoalescingEnabled, 1,
     PyDoc_STR("() -> (Boolean _rv)")},
    {"SetMouseCoalescingEnabled", (PyCFunction)CarbonEvents_SetMouseCoalescingEnabled, 1,
     PyDoc_STR("(Boolean inNewState) -> (Boolean outOldState)")},
    {"GetWindowEventTarget", (PyCFunction)CarbonEvents_GetWindowEventTarget, 1,
     PyDoc_STR("(WindowPtr inWindow) -> (EventTargetRef _rv)")},
    {"GetControlEventTarget", (PyCFunction)CarbonEvents_GetControlEventTarget, 1,
     PyDoc_STR("(ControlHandle inControl) -> (EventTargetRef _rv)")},
    {"GetMenuEventTarget", (PyCFunction)CarbonEvents_GetMenuEventTarget, 1,
     PyDoc_STR("(MenuHandle inMenu) -> (EventTargetRef _rv)")},
    {"GetApplicationEventTarget", (PyCFunction)CarbonEvents_GetApplicationEventTarget, 1,
     PyDoc_STR("() -> (EventTargetRef _rv)")},
    {"GetUserFocusEventTarget", (PyCFunction)CarbonEvents_GetUserFocusEventTarget, 1,
     PyDoc_STR("() -> (EventTargetRef _rv)")},
    {"GetEventDispatcherTarget", (PyCFunction)CarbonEvents_GetEventDispatcherTarget, 1,
     PyDoc_STR("() -> (EventTargetRef _rv)")},
    {"RunApplicationEventLoop", (PyCFunction)CarbonEvents_RunApplicationEventLoop, 1,
     PyDoc_STR("() -> None")},
    {"QuitApplicationEventLoop", (PyCFunction)CarbonEvents_QuitApplicationEventLoop, 1,
     PyDoc_STR("() -> None")},
    {"RunAppModalLoopForWindow", (PyCFunction)CarbonEvents_RunAppModalLoopForWindow, 1,
     PyDoc_STR("(WindowPtr inWindow) -> None")},
    {"QuitAppModalLoopForWindow", (PyCFunction)CarbonEvents_QuitAppModalLoopForWindow, 1,
     PyDoc_STR("(WindowPtr inWindow) -> None")},
    {"BeginAppModalStateForWindow", (PyCFunction)CarbonEvents_BeginAppModalStateForWindow, 1,
     PyDoc_STR("(WindowPtr inWindow) -> None")},
    {"EndAppModalStateForWindow", (PyCFunction)CarbonEvents_EndAppModalStateForWindow, 1,
     PyDoc_STR("(WindowPtr inWindow) -> None")},
    {"SetUserFocusWindow", (PyCFunction)CarbonEvents_SetUserFocusWindow, 1,
     PyDoc_STR("(WindowPtr inWindow) -> None")},
    {"GetUserFocusWindow", (PyCFunction)CarbonEvents_GetUserFocusWindow, 1,
     PyDoc_STR("() -> (WindowPtr _rv)")},
    {"SetWindowDefaultButton", (PyCFunction)CarbonEvents_SetWindowDefaultButton, 1,
     PyDoc_STR("(WindowPtr inWindow, ControlHandle inControl) -> None")},
    {"SetWindowCancelButton", (PyCFunction)CarbonEvents_SetWindowCancelButton, 1,
     PyDoc_STR("(WindowPtr inWindow, ControlHandle inControl) -> None")},
    {"GetWindowDefaultButton", (PyCFunction)CarbonEvents_GetWindowDefaultButton, 1,
     PyDoc_STR("(WindowPtr inWindow) -> (ControlHandle outControl)")},
    {"GetWindowCancelButton", (PyCFunction)CarbonEvents_GetWindowCancelButton, 1,
     PyDoc_STR("(WindowPtr inWindow) -> (ControlHandle outControl)")},
    {"RegisterEventHotKey", (PyCFunction)CarbonEvents_RegisterEventHotKey, 1,
     PyDoc_STR("(UInt32 inHotKeyCode, UInt32 inHotKeyModifiers, EventHotKeyID inHotKeyID, EventTargetRef inTarget, OptionBits inOptions) -> (EventHotKeyRef outRef)")},
    {NULL, NULL, 0}
};

#else /* APPLE_SUPPORTS_QUICKTIME */

static PyMethodDef CarbonEvents_methods[] = {
    {NULL, NULL, 0}
};

#endif /* APPLE_SUPPORTS_QUICKTIME */



void init_CarbonEvt(void)
{
    PyObject *m;
#if APPLE_SUPPORTS_QUICKTIME
    PyObject *d;
#endif /* !APPLE_SUPPORTS_QUICKTIME */


    m = Py_InitModule("_CarbonEvt", CarbonEvents_methods);

#if APPLE_SUPPORTS_QUICKTIME
    myEventHandlerUPP = NewEventHandlerUPP(myEventHandler);
    d = PyModule_GetDict(m);
    CarbonEvents_Error = PyMac_GetOSErrException();
    if (CarbonEvents_Error == NULL ||
        PyDict_SetItemString(d, "Error", CarbonEvents_Error) != 0)
        return;
    EventRef_Type.ob_type = &PyType_Type;
    if (PyType_Ready(&EventRef_Type) < 0) return;
    Py_INCREF(&EventRef_Type);
    PyModule_AddObject(m, "EventRef", (PyObject *)&EventRef_Type);
    /* Backward-compatible name */
    Py_INCREF(&EventRef_Type);
    PyModule_AddObject(m, "EventRefType", (PyObject *)&EventRef_Type);
    EventQueueRef_Type.ob_type = &PyType_Type;
    if (PyType_Ready(&EventQueueRef_Type) < 0) return;
    Py_INCREF(&EventQueueRef_Type);
    PyModule_AddObject(m, "EventQueueRef", (PyObject *)&EventQueueRef_Type);
    /* Backward-compatible name */
    Py_INCREF(&EventQueueRef_Type);
    PyModule_AddObject(m, "EventQueueRefType", (PyObject *)&EventQueueRef_Type);
    EventLoopRef_Type.ob_type = &PyType_Type;
    if (PyType_Ready(&EventLoopRef_Type) < 0) return;
    Py_INCREF(&EventLoopRef_Type);
    PyModule_AddObject(m, "EventLoopRef", (PyObject *)&EventLoopRef_Type);
    /* Backward-compatible name */
    Py_INCREF(&EventLoopRef_Type);
    PyModule_AddObject(m, "EventLoopRefType", (PyObject *)&EventLoopRef_Type);
    EventLoopTimerRef_Type.ob_type = &PyType_Type;
    if (PyType_Ready(&EventLoopTimerRef_Type) < 0) return;
    Py_INCREF(&EventLoopTimerRef_Type);
    PyModule_AddObject(m, "EventLoopTimerRef", (PyObject *)&EventLoopTimerRef_Type);
    /* Backward-compatible name */
    Py_INCREF(&EventLoopTimerRef_Type);
    PyModule_AddObject(m, "EventLoopTimerRefType", (PyObject *)&EventLoopTimerRef_Type);
    EventHandlerRef_Type.ob_type = &PyType_Type;
    if (PyType_Ready(&EventHandlerRef_Type) < 0) return;
    Py_INCREF(&EventHandlerRef_Type);
    PyModule_AddObject(m, "EventHandlerRef", (PyObject *)&EventHandlerRef_Type);
    /* Backward-compatible name */
    Py_INCREF(&EventHandlerRef_Type);
    PyModule_AddObject(m, "EventHandlerRefType", (PyObject *)&EventHandlerRef_Type);
    EventHandlerCallRef_Type.ob_type = &PyType_Type;
    if (PyType_Ready(&EventHandlerCallRef_Type) < 0) return;
    Py_INCREF(&EventHandlerCallRef_Type);
    PyModule_AddObject(m, "EventHandlerCallRef", (PyObject *)&EventHandlerCallRef_Type);
    /* Backward-compatible name */
    Py_INCREF(&EventHandlerCallRef_Type);
    PyModule_AddObject(m, "EventHandlerCallRefType", (PyObject *)&EventHandlerCallRef_Type);
    EventTargetRef_Type.ob_type = &PyType_Type;
    if (PyType_Ready(&EventTargetRef_Type) < 0) return;
    Py_INCREF(&EventTargetRef_Type);
    PyModule_AddObject(m, "EventTargetRef", (PyObject *)&EventTargetRef_Type);
    /* Backward-compatible name */
    Py_INCREF(&EventTargetRef_Type);
    PyModule_AddObject(m, "EventTargetRefType", (PyObject *)&EventTargetRef_Type);
    EventHotKeyRef_Type.ob_type = &PyType_Type;
    if (PyType_Ready(&EventHotKeyRef_Type) < 0) return;
    Py_INCREF(&EventHotKeyRef_Type);
    PyModule_AddObject(m, "EventHotKeyRef", (PyObject *)&EventHotKeyRef_Type);
    /* Backward-compatible name */
    Py_INCREF(&EventHotKeyRef_Type);
    PyModule_AddObject(m, "EventHotKeyRefType", (PyObject *)&EventHotKeyRef_Type);
#endif /* APPLE_SUPPORTS_QUICKTIME */
}

/* ===================== End module _CarbonEvt ====================== */

