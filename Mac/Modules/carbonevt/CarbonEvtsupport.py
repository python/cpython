# IBCarbonsupport.py

from macsupport import *

from CarbonEvtscan import RefObjectTypes

# where should this go? macsupport.py?
CFStringRef = OpaqueByValueType('CFStringRef')

for typ in RefObjectTypes:
    execstr = "%(name)s = OpaqueByValueType('%(name)s')" % {"name": typ}
    exec execstr


if 0:
    # these types will have no methods and will merely be opaque blobs
    # should write getattr and setattr for them?

    StructObjectTypes = ["EventTypeSpec",
                                            "HIPoint",
                                            "HICommand",
                                            "EventHotKeyID",
                                            ]

    for typ in StructObjectTypes:
        execstr = "%(name)s = OpaqueType('%(name)s')" % {"name": typ}
        exec execstr

EventHotKeyID = OpaqueByValueType("EventHotKeyID", "EventHotKeyID")
EventTypeSpec_ptr = OpaqueType("EventTypeSpec", "EventTypeSpec")

# is this the right type for the void * in GetEventParameter
#void_ptr = FixedInputBufferType(1024)
void_ptr = stringptr
# here are some types that are really other types

class MyVarInputBufferType(VarInputBufferType):
    def passInput(self, name):
        return "%s__len__, %s__in__" % (name, name)

MyInBuffer = MyVarInputBufferType('char', 'long', 'l')          # (buf, len)

EventTime = double
EventTimeout = EventTime
EventTimerInterval = EventTime
EventAttributes = UInt32
EventParamName = OSType
EventParamType = OSType
EventPriority = SInt16
EventMask = UInt16

EventComparatorUPP = FakeType("(EventComparatorUPP)0")
EventLoopTimerUPP = FakeType("(EventLoopTimerUPP)0")
EventHandlerUPP = FakeType("(EventHandlerUPP)0")
EventHandlerUPP = FakeType("(EventHandlerUPP)0")
EventComparatorProcPtr = FakeType("(EventComparatorProcPtr)0")
EventLoopTimerProcPtr = FakeType("(EventLoopTimerProcPtr)0")
EventHandlerProcPtr = FakeType("(EventHandlerProcPtr)0")

CarbonEventsFunction = OSErrFunctionGenerator
CarbonEventsMethod = OSErrMethodGenerator

class EventHandlerRefMethod(OSErrMethodGenerator):
    def precheck(self):
        OutLbrace('if (_self->ob_itself == NULL)')
        Output('PyErr_SetString(CarbonEvents_Error, "Handler has been removed");')
        Output('return NULL;')
        OutRbrace()


RgnHandle = OpaqueByValueType("RgnHandle", "ResObj")
GrafPtr = OpaqueByValueType("GrafPtr", "GrafObj")
MouseTrackingResult = UInt16


includestuff = includestuff + r"""
#include <Carbon/Carbon.h>

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
        if (PyArg_Parse(v, "(O&l)",
                        PyMac_GetOSType, &(out->eventClass),
                        &(out->eventKind)))
                return 1;
        return NULL;
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

"""

initstuff = initstuff + """
myEventHandlerUPP = NewEventHandlerUPP(myEventHandler);
"""
module = MacModule('_CarbonEvt', 'CarbonEvents', includestuff, finalstuff, initstuff)




class EventHandlerRefObjectDefinition(PEP253Mixin, GlobalObjectDefinition):
    def outputStructMembers(self):
        Output("%s ob_itself;", self.itselftype)
        Output("PyObject *ob_callback;")
    def outputInitStructMembers(self):
        Output("it->ob_itself = %sitself;", self.argref)
        Output("it->ob_callback = NULL;")
    def outputFreeIt(self, name):
        OutLbrace("if (self->ob_itself != NULL)")
        Output("RemoveEventHandler(self->ob_itself);")
        Output("Py_DECREF(self->ob_callback);")
        OutRbrace()

class MyGlobalObjectDefinition(PEP253Mixin, GlobalObjectDefinition):
    pass

for typ in RefObjectTypes:
    if typ == 'EventHandlerRef':
        EventHandlerRefobject = EventHandlerRefObjectDefinition('EventHandlerRef')
    else:
        execstr = typ + 'object = MyGlobalObjectDefinition(typ)'
        exec execstr
    module.addobject(eval(typ + 'object'))


functions = []
for typ in RefObjectTypes: ## go thru all ObjectTypes as defined in CarbonEventsscan.py
    # initialize the lists for carbongen to fill
    execstr = typ + 'methods = []'
    exec execstr

execfile('CarbonEventsgen.py')



for f in functions: module.add(f)       # add all the functions carboneventsgen put in the list

for typ in RefObjectTypes:                               ## go thru all ObjectTypes as defined in CarbonEventsscan.py
    methods = eval(typ + 'methods')  ## get a reference to the method list from the main namespace
    obj = eval(typ + 'object')                ## get a reference to the object
    for m in methods: obj.add(m)    ## add each method in the list to the object


removeeventhandler = """
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
Py_DECREF(_self->ob_callback);
_self->ob_callback = NULL;
Py_INCREF(Py_None);
_res = Py_None;
return _res;"""

f = ManualGenerator("RemoveEventHandler", removeeventhandler);
f.docstring = lambda: "() -> None"
EventHandlerRefobject.add(f)


installeventhandler = """
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
return _res;"""

f = ManualGenerator("InstallEventHandler", installeventhandler);
f.docstring = lambda: "(EventTypeSpec inSpec, Method callback) -> (EventHandlerRef outRef)"
EventTargetRefobject.add(f)

# This may not be the best, but at least it lets you get the raw data back into python as a string. You'll have to cut it up yourself and parse the result.

geteventparameter = """
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
"""

f = ManualGenerator("GetEventParameter", geteventparameter);
f.docstring = lambda: "(EventParamName eventName, EventParamType eventType) -> (String eventParamData)"
EventRefobject.add(f)

SetOutputFileName('_CarbonEvtmodule.c')
module.generate()

##import os
##os.system("python setup.py build")
