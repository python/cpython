

#include "Python.h"
#include "pycore_frame.h"
#include "pycore_interp.h"
#include "pycore_namespace.h"
#include "pycore_opcode.h"
#include "pycore_pyerrors.h"


static PyObject DISABLE =
{
    1 << 30,
    &PyBaseObject_Type
};

static int
call_multiple_instruments(
    PyInterpreterState *interp, PyCodeObject *code, int event,
    PyObject **args, Py_ssize_t nargs, _Py_CODEUNIT *instr)
{
    PyErr_SetString(PyExc_SystemError, "Multiple tools not supported yet");
    return -1;
}

static const uint8_t DE_INSTRUMENT[256] = {
    [INSTRUMENTED_RESUME] = RESUME,
    [INSTRUMENTED_RETURN_VALUE] = RETURN_VALUE,
    [INSTRUMENTED_CALL] = CALL,
    [INSTRUMENTED_YIELD_VALUE] = YIELD_VALUE,
/*
    [INSTRUMENTED_JUMP_FORWARD] = JUMP_FORWARD,
    [INSTRUMENTED_JUMP_BACKWARD] = INSTRUMENTED_JUMP_BACKWARD,
    [INSTRUMENTED_JUMP_IF_FALSE_OR_POP] = INSTRUMENTED_JUMP_IF_FALSE_OR_POP,
    [INSTRUMENTED_JUMP_IF_TRUE_OR_POP] = INSTRUMENTED_JUMP_IF_TRUE_OR_POP,
    [INSTRUMENTED_POP_JUMP_IF_FALSE] = INSTRUMENTED_POP_JUMP_IF_FALSE,
    [INSTRUMENTED_POP_JUMP_IF_TRUE] = INSTRUMENTED_POP_JUMP_IF_TRUE,
    [INSTRUMENTED_POP_JUMP_IF_NONE] = INSTRUMENTED_POP_JUMP_IF_NONE,
    [INSTRUMENTED_POP_JUMP_IF_NOT_NONE] = INSTRUMENTED_POP_JUMP_IF_NOT_NONE,
*/
};

static int
call_instrument(
    PyThreadState *tstate, PyCodeObject *code, int event,
    PyObject **args, Py_ssize_t nargsf, _Py_CODEUNIT *instr)
{
    assert(!_PyErr_Occurred(tstate));
    PyInterpreterState *interp = tstate->interp;
    int sole_tool_plus1 = interp->sole_tool_plus1[event];
    if (sole_tool_plus1 <= 0) {
        if (sole_tool_plus1 == 0) {
            /* Why is this instrumented if there is no tool? */
            return 0;
        }
        return call_multiple_instruments(interp, code, event, args, nargsf, instr);
    }
    PyObject *instrument = interp->tools[sole_tool_plus1-1].instrument_callables[event];
    if (instrument == NULL) {
       return 0;
    }
    PyObject *res = PyObject_Vectorcall(instrument, args, nargsf, NULL);
    if (res == NULL) {
        return -1;
    }
    if (res == &DISABLE) {
        /* Remove this instrument */
        assert(DE_INSTRUMENT[_Py_OPCODE(*instr)] != 0);
        _Py_SET_OPCODE(*instr, DE_INSTRUMENT[_Py_OPCODE(*instr)]);
    }
    Py_DECREF(res);
    return 0;
}


int
_Py_call_instrumentation(
    PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _Py_CODEUNIT *instr)
{
    if (tstate->tracing) {
        return 0;
    }
    tstate->tracing++;
    PyCodeObject *code = frame->f_code;
    int instruction_offset = instr - _PyCode_CODE(code);
    PyObject *instruction_offset_obj = PyLong_FromSsize_t(instruction_offset);
    PyObject *args[3] = { NULL, (PyObject *)code, instruction_offset_obj };
    int err = call_instrument(tstate, code, event, &args[1], 2 | PY_VECTORCALL_ARGUMENTS_OFFSET, instr);
    Py_DECREF(instruction_offset_obj);
    tstate->tracing--;
    return err;
}

int
_Py_call_instrumentation_arg(
    PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _Py_CODEUNIT *instr, PyObject *arg)
{
    if (tstate->tracing) {
        return 0;
    }
    tstate->tracing++;
    PyCodeObject *code = frame->f_code;
    int instruction_offset = instr - _PyCode_CODE(code);
    PyObject *instruction_offset_obj = PyLong_FromSsize_t(instruction_offset);
    PyObject *args[4] = { NULL, (PyObject *)code, instruction_offset_obj, arg };
    int err = call_instrument(tstate, code, event, &args[1], 3 | PY_VECTORCALL_ARGUMENTS_OFFSET, instr);
    Py_DECREF(instruction_offset_obj);
    tstate->tracing--;
    return err;
}

void
_Py_call_instrumentation_exc(
    PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _Py_CODEUNIT *instr, PyObject *arg)
{
    assert(_PyErr_Occurred(tstate));
    if (tstate->tracing) {
        return;
    }
    tstate->tracing++;
    PyObject *type, *value, *traceback;
    _PyErr_Fetch(tstate, &type, &value, &traceback);
    PyCodeObject *code = frame->f_code;
    int instruction_offset = instr - _PyCode_CODE(code);
    PyObject *instruction_offset_obj = PyLong_FromSsize_t(instruction_offset);
    PyObject *args[4] = { NULL, (PyObject *)code, instruction_offset_obj, arg };
    Py_ssize_t nargsf = (2 | PY_VECTORCALL_ARGUMENTS_OFFSET) + (arg != NULL);
    int err = call_instrument(tstate, code, event, &args[1], nargsf, instr);
    if (err) {
        Py_XDECREF(type);
        Py_XDECREF(value);
        Py_XDECREF(traceback);
    }
    else {
        _PyErr_Restore(tstate, type, value, traceback);
    }
    assert(_PyErr_Occurred(tstate));
    Py_DECREF(instruction_offset_obj);
    tstate->tracing--;
}

PyObject *
_PyMonitoring_RegisterCallback(int tool_id, int event_id, PyObject *obj)
{
    PyInterpreterState *is = _PyInterpreterState_Get();
    assert(0 <= tool_id && tool_id < PY_MONITORING_TOOL_IDS);
    assert(0 <= event_id && event_id < PY_MONITORING_EVENTS);
    PyObject *callback = is->tools[tool_id].instrument_callables[event_id];
    is->tools[tool_id].instrument_callables[event_id] = Py_XNewRef(obj);
    return callback;
}

static const uint8_t INSTRUMENTED_OPCODES[256] = {
    [RETURN_VALUE] = INSTRUMENTED_RETURN_VALUE,
    [CALL] = INSTRUMENTED_CALL,
    [CALL_FUNCTION_EX] = INSTRUMENTED_CALL_FUNCTION_EX,
    [YIELD_VALUE] = INSTRUMENTED_YIELD_VALUE,
    [RESUME] = INSTRUMENTED_RESUME,
    [INSTRUMENTED_RETURN_VALUE] = INSTRUMENTED_RETURN_VALUE,
    [INSTRUMENTED_CALL] = INSTRUMENTED_CALL,
    [INSTRUMENTED_CALL_FUNCTION_EX] = INSTRUMENTED_CALL_FUNCTION_EX,
    [INSTRUMENTED_YIELD_VALUE] = INSTRUMENTED_YIELD_VALUE,
    [INSTRUMENTED_RESUME] = INSTRUMENTED_RESUME,
    /*
    [JUMP_FORWARD] = INSTRUMENTED_JUMP_FORWARD,
    [JUMP_BACKWARD] = INSTRUMENTED_JUMP_BACKWARD,
    [JUMP_IF_FALSE_OR_POP] = INSTRUMENTED_JUMP_IF_FALSE_OR_POP,
    [JUMP_IF_TRUE_OR_POP] = INSTRUMENTED_JUMP_IF_TRUE_OR_POP,
    [POP_JUMP_IF_FALSE] = INSTRUMENTED_POP_JUMP_IF_FALSE,
    [POP_JUMP_IF_TRUE] = INSTRUMENTED_POP_JUMP_IF_TRUE,
    [POP_JUMP_IF_NONE] = INSTRUMENTED_POP_JUMP_IF_NONE,
    [POP_JUMP_IF_NOT_NONE] = INSTRUMENTED_POP_JUMP_IF_NOT_NONE,
    */
};

#define SET1(a) (1 << (a))
#define SET2(a, b ) ( (1 << (a)) | (1 << (b)) )
#define SET3(a, b, c) ( (1 << (a)) | (1 << (b)) | (1 << (c)) )

#define CALL_EVENTS \
    SET3(PY_MONITORING_EVENT_CALL, PY_MONITORING_EVENT_C_RAISE, PY_MONITORING_EVENT_C_RETURN)

static const _PyMonitoringEventSet EVENTS_FOR_OPCODE[256] = {
    [RETURN_VALUE] = SET1(PY_MONITORING_EVENT_PY_RETURN),
    [INSTRUMENTED_RETURN_VALUE] = SET1(PY_MONITORING_EVENT_PY_RETURN),
    [CALL] = CALL_EVENTS,
    [INSTRUMENTED_CALL] = CALL_EVENTS,
    [CALL_FUNCTION_EX] = CALL_EVENTS,
    [INSTRUMENTED_CALL_FUNCTION_EX] = CALL_EVENTS,
    [RESUME] = SET2(PY_MONITORING_EVENT_PY_START, PY_MONITORING_EVENT_PY_RESUME),
    [INSTRUMENTED_RESUME] = SET2(PY_MONITORING_EVENT_PY_START, PY_MONITORING_EVENT_PY_RESUME),
    [YIELD_VALUE] = SET1(PY_MONITORING_EVENT_PY_YIELD),
    [INSTRUMENTED_YIELD_VALUE] = SET1(PY_MONITORING_EVENT_PY_YIELD),
};

int
_Py_Instrument(PyCodeObject *code, PyInterpreterState *interp)
{
    if (interp->monitoring_version == code->_co_instrument_version) {
        return 0;
    }
    /* First establish how much extra space will need,
     * and free/allocate it if different to what we had before */
    /* For now only allow only one tool per event */
    assert(interp->monitoring_tools_per_event[PY_MONITORING_EVENT_LINE].tools == 0);
    assert(interp->monitoring_tools_per_event[PY_MONITORING_EVENT_INSTRUCTION].tools == 0);
    for (int i = 0; i < PY_MONITORING_EVENTS; i++) {
       assert(interp->monitoring_tools_per_event[i].tools == 0 || interp->sole_tool_plus1[i] > 0);
    }
    //printf("Instrumenting code object at %p, code version: %ld interpreter version %ld\n",
    //       code, code->_co_instrument_version, interp->monitoring_version);

    /* Insert basic instrumentation */
    int instrumented = 0;
    int code_len = (int)Py_SIZE(code);
    for (int i = 0; i < code_len; i++) {
        _Py_CODEUNIT *instr = &_PyCode_CODE(code)[i];
        int opcode = _Py_OPCODE(*instr);
        if (_PyOpcode_Deopt[opcode]) {
            opcode = _PyOpcode_Deopt[opcode];
        }
        /* Don't instrument RESUME in await and yield from. */
        if (opcode == RESUME && _Py_OPARG(_PyCode_CODE(code)[i]) >= 2) {
            continue;
        }
        _PyMonitoringEventSet events = EVENTS_FOR_OPCODE[opcode];
        if (events & interp->monitored_events) {
            instrumented++;
            assert(INSTRUMENTED_OPCODES[opcode] != 0);
            *instr = _Py_MAKECODEUNIT(INSTRUMENTED_OPCODES[opcode], _Py_OPARG(*instr));
        }
        i += _PyOpcode_Caches[opcode];
    }
    //printf("Instrumented %d instructions\n", instrumented);
    code->_co_instrument_version = interp->monitoring_version;
    return 0;
}

void
_PyMonitoring_SetEvents(int tool_id, _PyMonitoringEventSet events)
{
    PyInterpreterState *interp = _PyInterpreterState_Get();
    if (interp->events_per_monitoring_tool[tool_id] == events) {
        return;
    }
    interp->events_per_monitoring_tool[tool_id] = events;
    for (int e = 0; e < PY_MONITORING_EVENTS; e++) {
        _PyMonitoringToolSet tools = { 0 };
        for (int t = 0; t < PY_MONITORING_TOOL_IDS; t++) {
            int event_tool_pair = (interp->events_per_monitoring_tool[t] >> e) & 1;
            tools.tools |= (event_tool_pair << t);
        }
        interp->monitoring_tools_per_event[e] = tools;
    }
    int multitools = 0;
    for (int e = 0; e < PY_MONITORING_EVENTS; e++) {
        _PyMonitoringToolSet tools = interp->monitoring_tools_per_event[e];
        if (_Py_popcount32(tools.tools) <= 1) {
            interp->sole_tool_plus1[e] = _Py_bit_length(tools.tools);
        }
        else {
            multitools++;
            interp->sole_tool_plus1[e] = -1;
        }
    }
    if (multitools) {
        /* Only support one tool for now */
        assert(0 && "Only support one tool for now");
    }
    interp->monitoring_version++;

    _PyMonitoringEventSet monitored_events = 0;
    for (int t = 0; t < PY_MONITORING_TOOL_IDS; t++) {
        monitored_events |= interp->events_per_monitoring_tool[t];
    }
    interp->monitored_events = monitored_events;
    /* Instrument all executing code objects */

    PyThreadState* ts = PyInterpreterState_ThreadHead(interp);

    while (ts) {
        _PyInterpreterFrame *frame = ts->cframe->current_frame;
        while (frame) {
            if (frame->owner != FRAME_OWNED_BY_CSTACK) {
                _Py_Instrument(frame->f_code, interp);
            }
            frame = frame->previous;
        }
        ts = PyThreadState_Next(ts);
    }
}

/* List of objects in monitoring namespace
    class Event(enum.IntFlag)
    def use_tool_id(id)->None
    def free_tool_id(id)->None
    def get_events(tool_id: int)->Event
    def set_events(tool_id: int, event_set: Event)->None
    def get_local_events(tool_id: int, code: CodeType)->Event
    def set_local_events(tool_id: int, code: CodeType, event_set: Event)->None
    def register_callback(tool_id: int, event: Event, func: Callable)->Optional[Callable]
    def insert_marker(tool_id: int, code: CodeType, offset: Event)->None
    def remove_marker(tool_id: int, code: CodeType, offset: Event)->None
    def restart_events()->None
    DISABLE: object
*/

/*[clinic input]
module monitoring
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=37257f5987a360cf]*/
/*[clinic end generated code]*/

#include "clinic/instrumentation.c.h"


static int
check_valid_tool(int tool_id)
{
    if (tool_id < 0 || tool_id >= PY_INSTRUMENT_SYS_PROFILE) {
        PyErr_Format(PyExc_ValueError, "invalid tool ID: %d (must be between 0 and 5)", tool_id);
        return -1;
    }
    return 0;
}


/*[clinic input]
monitoring.use_tool

    tool_id: int
    name: object
    /

[clinic start generated code]*/

static PyObject *
monitoring_use_tool_impl(PyObject *module, int tool_id, PyObject *name)
/*[clinic end generated code: output=d00b74d147bab1e3 input=506e604e1ea75567]*/

/*[clinic end generated code]*/
{
    if (check_valid_tool(tool_id))  {
        return NULL;
    }
    if (!PyUnicode_Check(name)) {
         PyErr_SetString(PyExc_ValueError, "tool name must be a str");
    }
    PyInterpreterState *interp = _PyInterpreterState_Get();
    if (interp->monitoring_tool_names[tool_id] != NULL) {
        PyErr_SetString(PyExc_ValueError, "tool ID is already in use");
        return NULL;
    }
    interp->monitoring_tool_names[tool_id] = Py_NewRef(name);
    Py_RETURN_NONE;
}

/*[clinic input]
monitoring.free_tool

    tool_id: int
    /

[clinic start generated code]*/

static PyObject *
monitoring_free_tool_impl(PyObject *module, int tool_id)
/*[clinic end generated code: output=7893bfdad26f51fa input=919fecb6f63130ed]*/

/*[clinic end generated code]*/
{
    if (check_valid_tool(tool_id))  {
        return NULL;
    }
    PyInterpreterState *interp = _PyInterpreterState_Get();
    Py_CLEAR(interp->monitoring_tool_names[tool_id]);
    Py_RETURN_NONE;
}

/*[clinic input]
monitoring.get_tool

    tool_id: int
    /

[clinic start generated code]*/

static PyObject *
monitoring_get_tool_impl(PyObject *module, int tool_id)
/*[clinic end generated code: output=1c05a98b404a9a16 input=eeee9bebd0bcae9d]*/

/*[clinic end generated code]*/
{
    if (check_valid_tool(tool_id))  {
        return NULL;
    }
    PyInterpreterState *interp = _PyInterpreterState_Get();
    PyObject *name = interp->monitoring_tool_names[tool_id];
    if (name == NULL) {
        Py_RETURN_NONE;
    }
    return Py_NewRef(name);
}

/*[clinic input]
monitoring.register_callback


    tool_id: int
    event: int
    func: object
    /

[clinic start generated code]*/

static PyObject *
monitoring_register_callback_impl(PyObject *module, int tool_id, int event,
                                  PyObject *func)
/*[clinic end generated code: output=e64daa363004030c input=df6d70ea4cf81007]*/
{
    if (check_valid_tool(tool_id))  {
        return NULL;
    }
    if (_Py_popcount32(event) != 1) {
        PyErr_SetString(PyExc_ValueError, "The callaback can only be set for one event at a time");
    }
    int event_id = _Py_bit_length(event)-1;
    if (event_id < 0 || event_id >= PY_MONITORING_EVENTS) {
        PyErr_Format(PyExc_ValueError, "invalid event: %d", event);
        return NULL;
    }
    if (func == Py_None) {
        func = NULL;
    }
    func = _PyMonitoring_RegisterCallback(tool_id, event_id, func);
    if (func == NULL) {
        Py_RETURN_NONE;
    }
    return func;
}

/*[clinic input]
monitoring.get_events

    tool_id: int
    /

[clinic start generated code]*/

static PyObject *
monitoring_get_events_impl(PyObject *module, int tool_id)
/*[clinic end generated code: output=d8b92576efaa12f9 input=49b77c12cc517025]*/
{
    if (tool_id < 0 || tool_id >= PY_INSTRUMENT_SYS_PROFILE) {
        PyErr_SetString(PyExc_ValueError, "tool ID must be between 0 and 5");
        return NULL;
    }
    PyInterpreterState *interp = _PyInterpreterState_Get();
    _PyMonitoringEventSet event_set = interp->events_per_monitoring_tool[tool_id];
    return PyLong_FromUnsignedLong(event_set);
}

/*[clinic input]
monitoring.set_events

    tool_id: int
    event_set: int
    /

[clinic start generated code]*/

static PyObject *
monitoring_set_events_impl(PyObject *module, int tool_id, int event_set)
/*[clinic end generated code: output=1916c1e49cfb5bdb input=a77ba729a242142b]*/
{
    if (tool_id < 0 || tool_id >= PY_INSTRUMENT_SYS_PROFILE) {
        PyErr_SetString(PyExc_ValueError, "tool ID must be between 0 and 5");
        return NULL;
    }
    if (event_set < 0 || event_set >= (1 << PY_MONITORING_EVENTS)) {
        PyErr_Format(PyExc_ValueError, "invalid event set 0x%x", event_set);
        return NULL;
    }
    _PyMonitoring_SetEvents(tool_id, event_set);
    Py_RETURN_NONE;
}


static PyMethodDef methods[] = {
    MONITORING_USE_TOOL_METHODDEF
    MONITORING_FREE_TOOL_METHODDEF
    MONITORING_GET_TOOL_METHODDEF
    MONITORING_REGISTER_CALLBACK_METHODDEF
    MONITORING_GET_EVENTS_METHODDEF
    MONITORING_SET_EVENTS_METHODDEF
    {NULL, NULL}  // sentinel
};

static struct PyModuleDef monitoring_module = {
    PyModuleDef_HEAD_INIT,
    "sys.monitoring",
    NULL,
    -1, /* multiple "initialization" just copies the module dict. */
    methods,
    NULL,
    NULL,
    NULL,
    NULL
};

static int
add_power2_constant(PyObject *obj, const char *name, int i)
{
    PyObject *val = PyLong_FromLong(1<<i);
    if (val == NULL) {
        return -1;
    }
    int err = PyObject_SetAttrString(obj, name, val);
    Py_DECREF(val);
    return err;
}

const char *event_names[] = {
    [PY_MONITORING_EVENT_PY_START] = "PY_START",
    [PY_MONITORING_EVENT_PY_RESUME] = "PY_RESUME",
    [PY_MONITORING_EVENT_PY_RETURN] = "PY_RETURN",
    [PY_MONITORING_EVENT_PY_YIELD] = "PY_YIELD",
    [PY_MONITORING_EVENT_CALL] = "CALL",
    [PY_MONITORING_EVENT_LINE] = "LINE",
    [PY_MONITORING_EVENT_INSTRUCTION] = "INSTRUCTION",
    [PY_MONITORING_EVENT_JUMP] = "JUMP",
    [PY_MONITORING_EVENT_BRANCH] = "BRANCH",
    [PY_MONITORING_EVENT_C_RETURN] = "C_RETURN",
    [PY_MONITORING_EVENT_PY_THROW] = "PY_THROW",
    [PY_MONITORING_EVENT_RAISE] = "RAISE",
    [PY_MONITORING_EVENT_EXCEPTION_HANDLED] = "EXCEPTION_HANDLED",
    [PY_MONITORING_EVENT_C_RAISE] = "C_RAISE",
    [PY_MONITORING_EVENT_PY_UNWIND] = "PY_UNWIND",
};

PyObject *_Py_CreateMonitoringObject(void)
{
    PyObject *mod = _PyModule_CreateInitialized(&monitoring_module, PYTHON_API_VERSION);
    if (mod == NULL) {
        return NULL;
    }
    if (PyObject_SetAttrString(mod, "DISABLE", &DISABLE)) {
        goto error;
    }
    PyObject *events = _PyNamespace_New(NULL);
    if (events == NULL) {
        goto error;
    }
    if (PyObject_SetAttrString(mod, "events", events)) {
        goto error;
    }
    for (int i = 0; i < PY_MONITORING_EVENTS; i++) {
        if (add_power2_constant(events, event_names[i], i)) {
            goto error;
        }
    }
    return mod;
error:
    Py_DECREF(mod);
    return NULL;
}




