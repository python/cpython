
/* ====================== Module CarbonEvents ======================= */

#include "Python.h"



#include <Carbon/Carbon.h>
#include "macglue.h"

#define USE_MAC_MP_MULTITHREADING 1

#if USE_MAC_MP_MULTITHREADING
static PyThreadState *_save;
static MPCriticalRegionID reentrantLock;
#endif /* USE_MAC_MP_MULTITHREADING */

extern int CFStringRef_New(CFStringRef *);

extern int CFStringRef_Convert(PyObject *, CFStringRef *);
extern int CFBundleRef_Convert(PyObject *, CFBundleRef *);

int EventTargetRef_Convert(PyObject *, EventTargetRef *);
PyObject *EventHandlerCallRef_New(EventHandlerCallRef itself);
PyObject *EventRef_New(EventRef itself);

/********** EventTypeSpec *******/
static PyObject*
EventTypeSpec_New(EventTypeSpec *in)
{
	return Py_BuildValue("ll", in->eventClass, in->eventKind);
}

static int
EventTypeSpec_Convert(PyObject *v, EventTypeSpec *out)
{
	if (PyArg_ParseTuple(v, "ll", &(out->eventClass), &(out->eventKind)))
		return 1;
	return NULL;
}

/********** end EventTypeSpec *******/

/********** HIPoint *******/

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

/********** end HIPoint *******/

/********** EventHotKeyID *******/

static PyObject*
EventHotKeyID_New(EventHotKeyID *in)
{
	return Py_BuildValue("ll", in->signature, in->id);
}

static int
EventHotKeyID_Convert(PyObject *v, EventHotKeyID *out)
{
	if (PyArg_ParseTuple(v, "ll", &out->signature, &out->id))
		return 1;
	return NULL;
}

/********** end EventHotKeyID *******/

/******** handlecommand ***********/

pascal OSStatus CarbonEvents_HandleCommand(EventHandlerCallRef handlerRef, EventRef event, void *outPyObject) {
	PyObject *retValue;
	int status;

#if USE_MAC_MP_MULTITHREADING
    MPEnterCriticalRegion(reentrantLock, kDurationForever);
    PyEval_RestoreThread(_save);
#endif /* USE_MAC_MP_MULTITHREADING */

    retValue = PyObject_CallFunction((PyObject *)outPyObject, "O&O&", EventHandlerCallRef_New, handlerRef, EventRef_New, event);
    status = PyInt_AsLong(retValue);

#if USE_MAC_MP_MULTITHREADING
    _save = PyEval_SaveThread();
    MPExitCriticalRegion(reentrantLock);
#endif /* USE_MAC_MP_MULTITHREADING */

    return status;
}

/******** end handlecommand ***********/


static PyObject *CarbonEvents_Error;

/* ---------------------- Object type EventRef ---------------------- */

PyTypeObject EventRef_Type;

#define EventRef_Check(x) ((x)->ob_type == &EventRef_Type)

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
	PyMem_DEL(self);
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
	UInt32 inSize;
	char* inDataPtr;
	if (!PyArg_ParseTuple(_args, "O&O&ls",
	                      PyMac_GetOSType, &inName,
	                      PyMac_GetOSType, &inType,
	                      &inSize,
	                      &inDataPtr))
		return NULL;
	_err = SetEventParameter(_self->ob_itself,
	                         inName,
	                         inType,
	                         inSize,
	                         inDataPtr);
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

static PyMethodDef EventRef_methods[] = {
	{"RetainEvent", (PyCFunction)EventRef_RetainEvent, 1,
	 "() -> (EventRef _rv)"},
	{"GetEventRetainCount", (PyCFunction)EventRef_GetEventRetainCount, 1,
	 "() -> (UInt32 _rv)"},
	{"ReleaseEvent", (PyCFunction)EventRef_ReleaseEvent, 1,
	 "() -> None"},
	{"SetEventParameter", (PyCFunction)EventRef_SetEventParameter, 1,
	 "(OSType inName, OSType inType, UInt32 inSize, char* inDataPtr) -> None"},
	{"GetEventClass", (PyCFunction)EventRef_GetEventClass, 1,
	 "() -> (UInt32 _rv)"},
	{"GetEventKind", (PyCFunction)EventRef_GetEventKind, 1,
	 "() -> (UInt32 _rv)"},
	{"GetEventTime", (PyCFunction)EventRef_GetEventTime, 1,
	 "() -> (double _rv)"},
	{"SetEventTime", (PyCFunction)EventRef_SetEventTime, 1,
	 "(double inTime) -> None"},
	{"IsUserCancelEventRef", (PyCFunction)EventRef_IsUserCancelEventRef, 1,
	 "() -> (Boolean _rv)"},
	{"ConvertEventRefToEventRecord", (PyCFunction)EventRef_ConvertEventRefToEventRecord, 1,
	 "() -> (Boolean _rv, EventRecord outEvent)"},
	{"IsEventInMask", (PyCFunction)EventRef_IsEventInMask, 1,
	 "(UInt16 inMask) -> (Boolean _rv)"},
	{"SendEventToEventTarget", (PyCFunction)EventRef_SendEventToEventTarget, 1,
	 "(EventTargetRef inTarget) -> None"},
	{NULL, NULL, 0}
};

PyMethodChain EventRef_chain = { EventRef_methods, NULL };

static PyObject *EventRef_getattr(EventRefObject *self, char *name)
{
	return Py_FindMethodInChain(&EventRef_chain, (PyObject *)self, name);
}

#define EventRef_setattr NULL

#define EventRef_compare NULL

#define EventRef_repr NULL

#define EventRef_hash NULL

PyTypeObject EventRef_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"CarbonEvents.EventRef", /*tp_name*/
	sizeof(EventRefObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) EventRef_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) EventRef_getattr, /*tp_getattr*/
	(setattrfunc) EventRef_setattr, /*tp_setattr*/
	(cmpfunc) EventRef_compare, /*tp_compare*/
	(reprfunc) EventRef_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) EventRef_hash, /*tp_hash*/
};

/* -------------------- End object type EventRef -------------------- */


/* ------------------- Object type EventQueueRef -------------------- */

PyTypeObject EventQueueRef_Type;

#define EventQueueRef_Check(x) ((x)->ob_type == &EventQueueRef_Type)

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
	PyMem_DEL(self);
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
	EventTypeSpec * inList;
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
	 "(EventRef inEvent, SInt16 inPriority) -> None"},
	{"FlushEventsMatchingListFromQueue", (PyCFunction)EventQueueRef_FlushEventsMatchingListFromQueue, 1,
	 "(UInt32 inNumTypes, EventTypeSpec * inList) -> None"},
	{"FlushEventQueue", (PyCFunction)EventQueueRef_FlushEventQueue, 1,
	 "() -> None"},
	{"GetNumEventsInQueue", (PyCFunction)EventQueueRef_GetNumEventsInQueue, 1,
	 "() -> (UInt32 _rv)"},
	{"RemoveEventFromQueue", (PyCFunction)EventQueueRef_RemoveEventFromQueue, 1,
	 "(EventRef inEvent) -> None"},
	{"IsEventInQueue", (PyCFunction)EventQueueRef_IsEventInQueue, 1,
	 "(EventRef inEvent) -> (Boolean _rv)"},
	{NULL, NULL, 0}
};

PyMethodChain EventQueueRef_chain = { EventQueueRef_methods, NULL };

static PyObject *EventQueueRef_getattr(EventQueueRefObject *self, char *name)
{
	return Py_FindMethodInChain(&EventQueueRef_chain, (PyObject *)self, name);
}

#define EventQueueRef_setattr NULL

#define EventQueueRef_compare NULL

#define EventQueueRef_repr NULL

#define EventQueueRef_hash NULL

PyTypeObject EventQueueRef_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"CarbonEvents.EventQueueRef", /*tp_name*/
	sizeof(EventQueueRefObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) EventQueueRef_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) EventQueueRef_getattr, /*tp_getattr*/
	(setattrfunc) EventQueueRef_setattr, /*tp_setattr*/
	(cmpfunc) EventQueueRef_compare, /*tp_compare*/
	(reprfunc) EventQueueRef_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) EventQueueRef_hash, /*tp_hash*/
};

/* ----------------- End object type EventQueueRef ------------------ */


/* -------------------- Object type EventLoopRef -------------------- */

PyTypeObject EventLoopRef_Type;

#define EventLoopRef_Check(x) ((x)->ob_type == &EventLoopRef_Type)

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
	PyMem_DEL(self);
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
	 "() -> None"},
	{NULL, NULL, 0}
};

PyMethodChain EventLoopRef_chain = { EventLoopRef_methods, NULL };

static PyObject *EventLoopRef_getattr(EventLoopRefObject *self, char *name)
{
	return Py_FindMethodInChain(&EventLoopRef_chain, (PyObject *)self, name);
}

#define EventLoopRef_setattr NULL

#define EventLoopRef_compare NULL

#define EventLoopRef_repr NULL

#define EventLoopRef_hash NULL

PyTypeObject EventLoopRef_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"CarbonEvents.EventLoopRef", /*tp_name*/
	sizeof(EventLoopRefObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) EventLoopRef_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) EventLoopRef_getattr, /*tp_getattr*/
	(setattrfunc) EventLoopRef_setattr, /*tp_setattr*/
	(cmpfunc) EventLoopRef_compare, /*tp_compare*/
	(reprfunc) EventLoopRef_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) EventLoopRef_hash, /*tp_hash*/
};

/* ------------------ End object type EventLoopRef ------------------ */


/* ----------------- Object type EventLoopTimerRef ------------------ */

PyTypeObject EventLoopTimerRef_Type;

#define EventLoopTimerRef_Check(x) ((x)->ob_type == &EventLoopTimerRef_Type)

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
	PyMem_DEL(self);
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
	 "() -> None"},
	{"SetEventLoopTimerNextFireTime", (PyCFunction)EventLoopTimerRef_SetEventLoopTimerNextFireTime, 1,
	 "(double inNextFire) -> None"},
	{NULL, NULL, 0}
};

PyMethodChain EventLoopTimerRef_chain = { EventLoopTimerRef_methods, NULL };

static PyObject *EventLoopTimerRef_getattr(EventLoopTimerRefObject *self, char *name)
{
	return Py_FindMethodInChain(&EventLoopTimerRef_chain, (PyObject *)self, name);
}

#define EventLoopTimerRef_setattr NULL

#define EventLoopTimerRef_compare NULL

#define EventLoopTimerRef_repr NULL

#define EventLoopTimerRef_hash NULL

PyTypeObject EventLoopTimerRef_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"CarbonEvents.EventLoopTimerRef", /*tp_name*/
	sizeof(EventLoopTimerRefObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) EventLoopTimerRef_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) EventLoopTimerRef_getattr, /*tp_getattr*/
	(setattrfunc) EventLoopTimerRef_setattr, /*tp_setattr*/
	(cmpfunc) EventLoopTimerRef_compare, /*tp_compare*/
	(reprfunc) EventLoopTimerRef_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) EventLoopTimerRef_hash, /*tp_hash*/
};

/* --------------- End object type EventLoopTimerRef ---------------- */


/* ------------------ Object type EventHandlerRef ------------------- */

PyTypeObject EventHandlerRef_Type;

#define EventHandlerRef_Check(x) ((x)->ob_type == &EventHandlerRef_Type)

typedef struct EventHandlerRefObject {
	PyObject_HEAD
	EventHandlerRef ob_itself;
} EventHandlerRefObject;

PyObject *EventHandlerRef_New(EventHandlerRef itself)
{
	EventHandlerRefObject *it;
	it = PyObject_NEW(EventHandlerRefObject, &EventHandlerRef_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
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
	/* Cleanup of self->ob_itself goes here */
	PyMem_DEL(self);
}

static PyObject *EventHandlerRef_RemoveEventHandler(EventHandlerRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = RemoveEventHandler(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *EventHandlerRef_AddEventTypesToHandler(EventHandlerRefObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	UInt32 inNumTypes;
	EventTypeSpec * inList;
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
	EventTypeSpec * inList;
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

static PyMethodDef EventHandlerRef_methods[] = {
	{"RemoveEventHandler", (PyCFunction)EventHandlerRef_RemoveEventHandler, 1,
	 "() -> None"},
	{"AddEventTypesToHandler", (PyCFunction)EventHandlerRef_AddEventTypesToHandler, 1,
	 "(UInt32 inNumTypes, EventTypeSpec * inList) -> None"},
	{"RemoveEventTypesFromHandler", (PyCFunction)EventHandlerRef_RemoveEventTypesFromHandler, 1,
	 "(UInt32 inNumTypes, EventTypeSpec * inList) -> None"},
	{NULL, NULL, 0}
};

PyMethodChain EventHandlerRef_chain = { EventHandlerRef_methods, NULL };

static PyObject *EventHandlerRef_getattr(EventHandlerRefObject *self, char *name)
{
	return Py_FindMethodInChain(&EventHandlerRef_chain, (PyObject *)self, name);
}

#define EventHandlerRef_setattr NULL

#define EventHandlerRef_compare NULL

#define EventHandlerRef_repr NULL

#define EventHandlerRef_hash NULL

PyTypeObject EventHandlerRef_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"CarbonEvents.EventHandlerRef", /*tp_name*/
	sizeof(EventHandlerRefObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) EventHandlerRef_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) EventHandlerRef_getattr, /*tp_getattr*/
	(setattrfunc) EventHandlerRef_setattr, /*tp_setattr*/
	(cmpfunc) EventHandlerRef_compare, /*tp_compare*/
	(reprfunc) EventHandlerRef_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) EventHandlerRef_hash, /*tp_hash*/
};

/* ---------------- End object type EventHandlerRef ----------------- */


/* ---------------- Object type EventHandlerCallRef ----------------- */

PyTypeObject EventHandlerCallRef_Type;

#define EventHandlerCallRef_Check(x) ((x)->ob_type == &EventHandlerCallRef_Type)

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
	PyMem_DEL(self);
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
	 "(EventRef inEvent) -> None"},
	{NULL, NULL, 0}
};

PyMethodChain EventHandlerCallRef_chain = { EventHandlerCallRef_methods, NULL };

static PyObject *EventHandlerCallRef_getattr(EventHandlerCallRefObject *self, char *name)
{
	return Py_FindMethodInChain(&EventHandlerCallRef_chain, (PyObject *)self, name);
}

#define EventHandlerCallRef_setattr NULL

#define EventHandlerCallRef_compare NULL

#define EventHandlerCallRef_repr NULL

#define EventHandlerCallRef_hash NULL

PyTypeObject EventHandlerCallRef_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"CarbonEvents.EventHandlerCallRef", /*tp_name*/
	sizeof(EventHandlerCallRefObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) EventHandlerCallRef_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) EventHandlerCallRef_getattr, /*tp_getattr*/
	(setattrfunc) EventHandlerCallRef_setattr, /*tp_setattr*/
	(cmpfunc) EventHandlerCallRef_compare, /*tp_compare*/
	(reprfunc) EventHandlerCallRef_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) EventHandlerCallRef_hash, /*tp_hash*/
};

/* -------------- End object type EventHandlerCallRef --------------- */


/* ------------------- Object type EventTargetRef ------------------- */

PyTypeObject EventTargetRef_Type;

#define EventTargetRef_Check(x) ((x)->ob_type == &EventTargetRef_Type)

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
	PyMem_DEL(self);
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
	EventHandlerUPP event;

	if (!PyArg_ParseTuple(_args, "O&O", EventTypeSpec_Convert, &inSpec, &callback))
		return NULL;

	event = NewEventHandlerUPP(CarbonEvents_HandleCommand);
	_err = InstallEventHandler(_self->ob_itself, event, 1, &inSpec, (void *)callback, &outRef);
	if (_err != noErr) return PyMac_Error(_err);

	return Py_BuildValue("l", outRef);

}

static PyMethodDef EventTargetRef_methods[] = {
	{"InstallStandardEventHandler", (PyCFunction)EventTargetRef_InstallStandardEventHandler, 1,
	 "() -> None"},
	{"InstallEventHandler", (PyCFunction)EventTargetRef_InstallEventHandler, 1,
	 "(EventTargetRef inTarget, EventTypeSpec inSpec, Method callback) -> (EventHandlerRef outRef)"},
	{NULL, NULL, 0}
};

PyMethodChain EventTargetRef_chain = { EventTargetRef_methods, NULL };

static PyObject *EventTargetRef_getattr(EventTargetRefObject *self, char *name)
{
	return Py_FindMethodInChain(&EventTargetRef_chain, (PyObject *)self, name);
}

#define EventTargetRef_setattr NULL

#define EventTargetRef_compare NULL

#define EventTargetRef_repr NULL

#define EventTargetRef_hash NULL

PyTypeObject EventTargetRef_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"CarbonEvents.EventTargetRef", /*tp_name*/
	sizeof(EventTargetRefObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) EventTargetRef_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) EventTargetRef_getattr, /*tp_getattr*/
	(setattrfunc) EventTargetRef_setattr, /*tp_setattr*/
	(cmpfunc) EventTargetRef_compare, /*tp_compare*/
	(reprfunc) EventTargetRef_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) EventTargetRef_hash, /*tp_hash*/
};

/* ----------------- End object type EventTargetRef ----------------- */


/* ------------------- Object type EventHotKeyRef ------------------- */

PyTypeObject EventHotKeyRef_Type;

#define EventHotKeyRef_Check(x) ((x)->ob_type == &EventHotKeyRef_Type)

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
	PyMem_DEL(self);
}

static PyMethodDef EventHotKeyRef_methods[] = {
	{NULL, NULL, 0}
};

PyMethodChain EventHotKeyRef_chain = { EventHotKeyRef_methods, NULL };

static PyObject *EventHotKeyRef_getattr(EventHotKeyRefObject *self, char *name)
{
	return Py_FindMethodInChain(&EventHotKeyRef_chain, (PyObject *)self, name);
}

#define EventHotKeyRef_setattr NULL

#define EventHotKeyRef_compare NULL

#define EventHotKeyRef_repr NULL

#define EventHotKeyRef_hash NULL

PyTypeObject EventHotKeyRef_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"CarbonEvents.EventHotKeyRef", /*tp_name*/
	sizeof(EventHotKeyRefObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) EventHotKeyRef_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) EventHotKeyRef_getattr, /*tp_getattr*/
	(setattrfunc) EventHotKeyRef_setattr, /*tp_setattr*/
	(cmpfunc) EventHotKeyRef_compare, /*tp_compare*/
	(reprfunc) EventHotKeyRef_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) EventHotKeyRef_hash, /*tp_hash*/
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
	EventTypeSpec * inList;
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

static PyObject *CarbonEvents_RunApplicationEventLoop(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;

#if USE_MAC_MP_MULTITHREADING
	if (MPCreateCriticalRegion(&reentrantLock) != noErr) {
		printf("lock failure
	");
		return NULL;
	}
	_save = PyEval_SaveThread();
#endif /* USE_MAC_MP_MULTITHREADING */

	RunApplicationEventLoop();

#if USE_MAC_MP_MULTITHREADING
	PyEval_RestoreThread(_save);

	MPDeleteCriticalRegion(reentrantLock);
#endif /* USE_MAC_MP_MULTITHREADING */

	Py_INCREF(Py_None);

	return Py_None;

}

static PyMethodDef CarbonEvents_methods[] = {
	{"GetCurrentEventLoop", (PyCFunction)CarbonEvents_GetCurrentEventLoop, 1,
	 "() -> (EventLoopRef _rv)"},
	{"GetMainEventLoop", (PyCFunction)CarbonEvents_GetMainEventLoop, 1,
	 "() -> (EventLoopRef _rv)"},
	{"RunCurrentEventLoop", (PyCFunction)CarbonEvents_RunCurrentEventLoop, 1,
	 "(double inTimeout) -> None"},
	{"ReceiveNextEvent", (PyCFunction)CarbonEvents_ReceiveNextEvent, 1,
	 "(UInt32 inNumTypes, EventTypeSpec * inList, double inTimeout, Boolean inPullEvent) -> (EventRef outEvent)"},
	{"GetCurrentEventQueue", (PyCFunction)CarbonEvents_GetCurrentEventQueue, 1,
	 "() -> (EventQueueRef _rv)"},
	{"GetMainEventQueue", (PyCFunction)CarbonEvents_GetMainEventQueue, 1,
	 "() -> (EventQueueRef _rv)"},
	{"GetCurrentEventTime", (PyCFunction)CarbonEvents_GetCurrentEventTime, 1,
	 "() -> (double _rv)"},
	{"GetLastUserEventTime", (PyCFunction)CarbonEvents_GetLastUserEventTime, 1,
	 "() -> (double _rv)"},
	{"GetWindowEventTarget", (PyCFunction)CarbonEvents_GetWindowEventTarget, 1,
	 "(WindowPtr inWindow) -> (EventTargetRef _rv)"},
	{"GetControlEventTarget", (PyCFunction)CarbonEvents_GetControlEventTarget, 1,
	 "(ControlHandle inControl) -> (EventTargetRef _rv)"},
	{"GetMenuEventTarget", (PyCFunction)CarbonEvents_GetMenuEventTarget, 1,
	 "(MenuHandle inMenu) -> (EventTargetRef _rv)"},
	{"GetApplicationEventTarget", (PyCFunction)CarbonEvents_GetApplicationEventTarget, 1,
	 "() -> (EventTargetRef _rv)"},
	{"GetUserFocusEventTarget", (PyCFunction)CarbonEvents_GetUserFocusEventTarget, 1,
	 "() -> (EventTargetRef _rv)"},
	{"QuitApplicationEventLoop", (PyCFunction)CarbonEvents_QuitApplicationEventLoop, 1,
	 "() -> None"},
	{"SetUserFocusWindow", (PyCFunction)CarbonEvents_SetUserFocusWindow, 1,
	 "(WindowPtr inWindow) -> None"},
	{"GetUserFocusWindow", (PyCFunction)CarbonEvents_GetUserFocusWindow, 1,
	 "() -> (WindowPtr _rv)"},
	{"SetWindowDefaultButton", (PyCFunction)CarbonEvents_SetWindowDefaultButton, 1,
	 "(WindowPtr inWindow, ControlHandle inControl) -> None"},
	{"SetWindowCancelButton", (PyCFunction)CarbonEvents_SetWindowCancelButton, 1,
	 "(WindowPtr inWindow, ControlHandle inControl) -> None"},
	{"GetWindowDefaultButton", (PyCFunction)CarbonEvents_GetWindowDefaultButton, 1,
	 "(WindowPtr inWindow) -> (ControlHandle outControl)"},
	{"GetWindowCancelButton", (PyCFunction)CarbonEvents_GetWindowCancelButton, 1,
	 "(WindowPtr inWindow) -> (ControlHandle outControl)"},
	{"RunApplicationEventLoop", (PyCFunction)CarbonEvents_RunApplicationEventLoop, 1,
	 "() -> ()"},
	{NULL, NULL, 0}
};




void initCarbonEvents(void)
{
	PyObject *m;
	PyObject *d;




	m = Py_InitModule("CarbonEvents", CarbonEvents_methods);
	d = PyModule_GetDict(m);
	CarbonEvents_Error = PyMac_GetOSErrException();
	if (CarbonEvents_Error == NULL ||
	    PyDict_SetItemString(d, "Error", CarbonEvents_Error) != 0)
		return;
	EventRef_Type.ob_type = &PyType_Type;
	Py_INCREF(&EventRef_Type);
	if (PyDict_SetItemString(d, "EventRefType", (PyObject *)&EventRef_Type) != 0)
		Py_FatalError("can't initialize EventRefType");
	EventQueueRef_Type.ob_type = &PyType_Type;
	Py_INCREF(&EventQueueRef_Type);
	if (PyDict_SetItemString(d, "EventQueueRefType", (PyObject *)&EventQueueRef_Type) != 0)
		Py_FatalError("can't initialize EventQueueRefType");
	EventLoopRef_Type.ob_type = &PyType_Type;
	Py_INCREF(&EventLoopRef_Type);
	if (PyDict_SetItemString(d, "EventLoopRefType", (PyObject *)&EventLoopRef_Type) != 0)
		Py_FatalError("can't initialize EventLoopRefType");
	EventLoopTimerRef_Type.ob_type = &PyType_Type;
	Py_INCREF(&EventLoopTimerRef_Type);
	if (PyDict_SetItemString(d, "EventLoopTimerRefType", (PyObject *)&EventLoopTimerRef_Type) != 0)
		Py_FatalError("can't initialize EventLoopTimerRefType");
	EventHandlerRef_Type.ob_type = &PyType_Type;
	Py_INCREF(&EventHandlerRef_Type);
	if (PyDict_SetItemString(d, "EventHandlerRefType", (PyObject *)&EventHandlerRef_Type) != 0)
		Py_FatalError("can't initialize EventHandlerRefType");
	EventHandlerCallRef_Type.ob_type = &PyType_Type;
	Py_INCREF(&EventHandlerCallRef_Type);
	if (PyDict_SetItemString(d, "EventHandlerCallRefType", (PyObject *)&EventHandlerCallRef_Type) != 0)
		Py_FatalError("can't initialize EventHandlerCallRefType");
	EventTargetRef_Type.ob_type = &PyType_Type;
	Py_INCREF(&EventTargetRef_Type);
	if (PyDict_SetItemString(d, "EventTargetRefType", (PyObject *)&EventTargetRef_Type) != 0)
		Py_FatalError("can't initialize EventTargetRefType");
	EventHotKeyRef_Type.ob_type = &PyType_Type;
	Py_INCREF(&EventHotKeyRef_Type);
	if (PyDict_SetItemString(d, "EventHotKeyRefType", (PyObject *)&EventHotKeyRef_Type) != 0)
		Py_FatalError("can't initialize EventHotKeyRefType");
}

/* ==================== End module CarbonEvents ===================== */

