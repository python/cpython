

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

static const uint8_t DE_INSTRUMENT[256] = {
    [RESUME] = RESUME,
    [INSTRUMENTED_RESUME] = RESUME,
    [INSTRUMENTED_RETURN_VALUE] = RETURN_VALUE,
    [INSTRUMENTED_CALL] = CALL,
    [INSTRUMENTED_CALL_FUNCTION_EX] = CALL_FUNCTION_EX,
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

/* Return 1 if DISABLE returned, -1 if error, 0 otherwise */
static int
call_one_instrument(
    PyInterpreterState *interp, PyThreadState *tstate, PyObject **args,
    Py_ssize_t nargsf, int8_t tool, int event)
{
    if (tstate->tracing) {
        return 0;
    }
    PyObject *instrument = interp->tools[tool].instrument_callables[event];
    if (instrument == NULL) {
       return 0;
    }
    tstate->tracing++;
    PyObject *res = PyObject_Vectorcall(instrument, args, nargsf, NULL);
    tstate->tracing--;
    if (res == NULL) {
        return -1;
    }
    Py_DECREF(res);
    return (res == &DISABLE);
}

static int
call_multiple_instruments(
    PyInterpreterState *interp, PyThreadState *tstate, PyCodeObject *code, int event,
    PyObject **args, Py_ssize_t nargsf, uint8_t *opcode)
{
    int offset = ((_Py_CODEUNIT *)opcode) - _PyCode_CODE(code);
    uint8_t *toolsptr = &((uint8_t *)code->_co_monitoring_data)[offset*code->_co_monitoring_data_per_instruction];
    uint8_t tools = *toolsptr;
    /* No line or instruction yet */
    assert(code->_co_monitoring_data_per_instruction == 1);
    assert(_Py_popcount32(tools) > 0);
    assert(interp->sole_tool_plus1[event] < 0);
    /* To do -- speed this up */
    assert(tools != 0);
    int tool = 0;
    do {
        assert(tool < 8);
        if (tools & (1 << tool)) {
            tools &= ~(1 << tool);
            int res = call_one_instrument(interp, tstate, args, nargsf, tool, event);
            if (res) {
                if (res > 0) {
                    *toolsptr &= ~(1 << tool);
                    if (*toolsptr == 0) {
                        /* Remove this instrument */
                        assert(DE_INSTRUMENT[*opcode] != 0);
                        *opcode = DE_INSTRUMENT[*opcode];
                    }
                }
                else {
                    return -1;
                }
            }
        }
        tool++;
    } while (tools);
    return 0;
}

static int
call_instrument(
    PyThreadState *tstate, PyCodeObject *code, int event,
    PyObject **args, Py_ssize_t nargsf, uint8_t *opcode)
{
    assert(!_PyErr_Occurred(tstate));
    PyInterpreterState *interp = tstate->interp;
    int sole_tool_plus1 = interp->sole_tool_plus1[event];
    if (sole_tool_plus1 <= 0) {
        if (sole_tool_plus1 == 0) {
            /* Why is this instrumented if there is no tool? */
            return 0;
        }
        return call_multiple_instruments(interp, tstate, code, event, args, nargsf, opcode);
    }
    int sole_tool = sole_tool_plus1-1;
    int res = call_one_instrument(interp, tstate, args, nargsf, sole_tool, event);
    if (res > 0) {
        /* Remove this instrument */
        assert(DE_INSTRUMENT[*opcode] != 0);
        *opcode = DE_INSTRUMENT[*opcode];
        res = 0;
    }
    return res;
}


int
_Py_call_instrumentation(
    PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _Py_CODEUNIT *instr)
{
    PyCodeObject *code = frame->f_code;
    int instruction_offset = instr - _PyCode_CODE(code);
    PyObject *instruction_offset_obj = PyLong_FromSsize_t(instruction_offset);
    PyObject *args[3] = { NULL, (PyObject *)code, instruction_offset_obj };
    int err = call_instrument(tstate, code, event, &args[1], 2 | PY_VECTORCALL_ARGUMENTS_OFFSET, (uint8_t*)instr);
    Py_DECREF(instruction_offset_obj);
    return err;
}

int
_Py_call_instrumentation_arg(
    PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _Py_CODEUNIT *instr, PyObject *arg)
{
    PyCodeObject *code = frame->f_code;
    int instruction_offset = instr - _PyCode_CODE(code);
    PyObject *instruction_offset_obj = PyLong_FromSsize_t(instruction_offset);
    PyObject *args[4] = { NULL, (PyObject *)code, instruction_offset_obj, arg };
    int err = call_instrument(tstate, code, event, &args[1], 3 | PY_VECTORCALL_ARGUMENTS_OFFSET, (uint8_t*)instr);
    Py_DECREF(instruction_offset_obj);
    return err;
}

void
_Py_call_instrumentation_exc(
    PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _Py_CODEUNIT *instr, PyObject *arg)
{
    assert(_PyErr_Occurred(tstate));
    PyObject *type, *value, *traceback;
    _PyErr_Fetch(tstate, &type, &value, &traceback);
    PyCodeObject *code = frame->f_code;
    int instruction_offset = instr - _PyCode_CODE(code);
    PyObject *instruction_offset_obj = PyLong_FromSsize_t(instruction_offset);
    PyObject *args[4] = { NULL, (PyObject *)code, instruction_offset_obj, arg };
    Py_ssize_t nargsf = (2 | PY_VECTORCALL_ARGUMENTS_OFFSET) + (arg != NULL);
    int err = call_instrument(tstate, code, event, &args[1], nargsf, (uint8_t*)instr);
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
#define RESUME_0 256
#define RESUME_1 257


static const uint8_t INSTRUMENTED_OPCODES[258] = {
    [RETURN_VALUE] = INSTRUMENTED_RETURN_VALUE,
    [CALL] = INSTRUMENTED_CALL,
    [CALL_FUNCTION_EX] = INSTRUMENTED_CALL_FUNCTION_EX,
    [YIELD_VALUE] = INSTRUMENTED_YIELD_VALUE,
    [RESUME] = INSTRUMENTED_RESUME,
    [RESUME_0] = INSTRUMENTED_RESUME,
    [RESUME_1] = INSTRUMENTED_RESUME,
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


static const int8_t EVENT_FOR_OPCODE[258] = {
    [RETURN_VALUE] = PY_MONITORING_EVENT_PY_RETURN,
    [INSTRUMENTED_RETURN_VALUE] = PY_MONITORING_EVENT_PY_RETURN,
    [CALL] = PY_MONITORING_EVENT_CALL,
    [INSTRUMENTED_CALL] = PY_MONITORING_EVENT_CALL,
    [CALL_FUNCTION_EX] = PY_MONITORING_EVENT_CALL,
    [INSTRUMENTED_CALL_FUNCTION_EX] = PY_MONITORING_EVENT_CALL,
    [RESUME] = -1,
    [RESUME_0] = PY_MONITORING_EVENT_PY_START,
    [RESUME_1] = PY_MONITORING_EVENT_PY_RESUME,
    [YIELD_VALUE] = PY_MONITORING_EVENT_PY_YIELD,
    [INSTRUMENTED_YIELD_VALUE] = PY_MONITORING_EVENT_PY_YIELD,
};

static const bool OPCODE_HAS_EVENT[258] = {
    [RETURN_VALUE] = true,
    [INSTRUMENTED_RETURN_VALUE] = true,
    [CALL] = true,
    [INSTRUMENTED_CALL] = true,
    [CALL_FUNCTION_EX] = true,
    [INSTRUMENTED_CALL_FUNCTION_EX] = true,
    [RESUME] = true,
    [RESUME_0] = true,
    [RESUME_1] = true,
    [YIELD_VALUE] = true,
    [INSTRUMENTED_YIELD_VALUE] = true,
};

static inline _Py_MonitoringMatrix
matrix_sub(_Py_MonitoringMatrix a, _Py_MonitoringMatrix b)
{
    _Py_MonitoringMatrix res;
    for (int i = 0; i < PY_MONITORING_EVENTS; i++) {
        res.tools[i] = a.tools[i] & ~b.tools[i];
    }
    return res;
}

static inline _Py_MonitoringMatrix
matrix_and(_Py_MonitoringMatrix a, _Py_MonitoringMatrix b)
{
    _Py_MonitoringMatrix res;
    for (int i = 0; i < PY_MONITORING_EVENTS; i++) {
        res.tools[i] = a.tools[i] & b.tools[i];
    }
    return res;
}

static inline bool
matrix_empty(_Py_MonitoringMatrix m)
{
    for (int i = 0; i < PY_MONITORING_EVENTS; i++) {
        if (m.tools[i]) {
            return false;
        }
    }
    return true;
}

static inline void
matrix_set_bit(_Py_MonitoringMatrix *m, int event, int tool, int val)
{
    assert(0 <= event && event < PY_MONITORING_EVENTS);
    assert(0 <= tool && tool < PY_MONITORING_TOOL_IDS);
    assert(0 <= val && val <= 1);
    m->tools[event] &= ~(1 << tool);
    m->tools[event] |= (val << tool);
}

static inline _PyMonitoringEventSet
matrix_get_events(_Py_MonitoringMatrix *m, int tool_id)
{
    _PyMonitoringEventSet result = 0;
    for (int e = 0; e < PY_MONITORING_EVENTS; e++) {
        if ((m->tools[e] >> tool_id) & 1) {
            result |= (1 << e);
        }
    }
    return result;
}

int
_Py_Instrument(PyCodeObject *code, PyInterpreterState *interp)
{
    if (interp->monitoring_version == code->_co_instrument_version) {
        return 0;
    }
    /* First establish how much extra space will need,
     * and free/allocate it if different to what we had before */
    /* For now only allow only one tool per event */
    assert(interp->monitoring_matrix.tools[PY_MONITORING_EVENT_LINE] == 0);
    assert(interp->monitoring_matrix.tools[PY_MONITORING_EVENT_INSTRUCTION] == 0);
    int code_len = (int)Py_SIZE(code);
    if (interp->required_monitoring_bytes > code->_co_monitoring_data_per_instruction) {
        /* Don't resize if more space than needed, to avoid churn */
        PyMem_Free(code->_co_monitoring_data);
        code->_co_monitoring_data = PyMem_Malloc(code_len * interp->required_monitoring_bytes);
        if (code->_co_monitoring_data == NULL) {
            code->_co_monitoring_data_per_instruction = 0;
            PyErr_NoMemory();
            return -1;
        }
        code->_co_monitoring_data_per_instruction = interp->required_monitoring_bytes;
    }
    //printf("Instrumenting code object at %p, code version: %ld interpreter version %ld\n",
    //       code, code->_co_instrument_version, interp->monitoring_version);
    /* Avoid instrumenting code that has been disabled.
     * Only instrument new events
     */
    _Py_MonitoringMatrix new_events = matrix_sub(interp->monitoring_matrix, code->_co_monitoring_matrix);
    _Py_MonitoringMatrix removed_events = matrix_sub(code->_co_monitoring_matrix, interp->monitoring_matrix);
    if (interp->last_restart_version > code->_co_instrument_version) {
        new_events = interp->monitoring_matrix;
        removed_events = (_Py_MonitoringMatrix){ 0 };
    }
    code->_co_instrument_version = interp->monitoring_version;
    assert(matrix_empty(matrix_and(new_events, removed_events)));
    if (matrix_empty(new_events) && matrix_empty(removed_events)) {
        return 0;
    }
    /* Insert instrumentation */
    for (int i = 0; i < code_len; i++) {
        _Py_CODEUNIT *instr = &_PyCode_CODE(code)[i];
        int opcode = _Py_OPCODE(*instr);
        int oparg = _Py_OPARG(*instr);
        if (_PyOpcode_Deopt[opcode]) {
            opcode = _PyOpcode_Deopt[opcode];
        }
        if (opcode == RESUME) {
            if (oparg == 0) {
                opcode = RESUME_0;
            }
            else {
                opcode = RESUME_1;
            }
        }
        bool has_event = OPCODE_HAS_EVENT[opcode];
        if (has_event) {
            int8_t event = EVENT_FOR_OPCODE[opcode];
            assert(event >= 0);
            if (interp->sole_tool_plus1[event] >= 0) {
                int sole_tool = interp->sole_tool_plus1[event] - 1;
                if (new_events.tools[event] & (1 << sole_tool)) {
                    assert(INSTRUMENTED_OPCODES[opcode] != 0);
                    *instr = _Py_MAKECODEUNIT(INSTRUMENTED_OPCODES[opcode], oparg);
                }
                else if (removed_events.tools[event] & (1 << sole_tool)) {
                    *instr = _Py_MAKECODEUNIT(opcode, _Py_OPARG(*instr));
                    if (_PyOpcode_Caches[opcode]) {
                        instr[1] = adaptive_counter_warmup();
                    }
                }
            }
            else {
                uint8_t new_tools = new_events.tools[event];
                if (new_tools) {
                    assert(INSTRUMENTED_OPCODES[opcode] != 0);
                    *instr = _Py_MAKECODEUNIT(INSTRUMENTED_OPCODES[opcode], oparg);
                    code->_co_monitoring_data[i*code->_co_monitoring_data_per_instruction] |= new_tools;
                }
                uint8_t removed_tools = removed_events.tools[event];
                if (removed_tools) {
                code->_co_monitoring_data[i*code->_co_monitoring_data_per_instruction] &= ~removed_tools;
                    if (code->_co_monitoring_data[i*code->_co_monitoring_data_per_instruction] == 0) {
                        *instr = _Py_MAKECODEUNIT(opcode, _Py_OPARG(*instr));
                        if (_PyOpcode_Caches[opcode]) {
                            instr[1] = adaptive_counter_warmup();
                        }
                    }
                }
            }
        }
        i += _PyOpcode_Caches[opcode];
    }
    return 0;
}

#define C_RETURN_EVENTS \
    ((1 << PY_MONITORING_EVENT_C_RETURN) | \
    (1 << PY_MONITORING_EVENT_C_RAISE))

#define C_CALL_EVENTS \
    (C_RETURN_EVENTS | (1 << PY_MONITORING_EVENT_CALL))

static int
cross_check_events(PyInterpreterState *interp)
{
    for (int e = 0; e < PY_MONITORING_EVENTS; e++) {
        uint8_t tools = interp->monitoring_matrix.tools[e];
        int popcount = _Py_popcount32(tools);
        switch(popcount) {
            case 0:
                if (interp->sole_tool_plus1[e] != 0) {
                    return 0;
                }
                break;
            case 1:
                if (interp->sole_tool_plus1[e] <= 0) {
                    return 0;
                }
                if ((tools & (1 << (interp->sole_tool_plus1[e]-1))) == 0) {
                    return 0;
                }
                break;
            default:
                if (interp->sole_tool_plus1[e] >= 0) {
                    return 0;
                }
        }
    }
    return 1;
}

void
_PyMonitoring_SetEvents(int tool_id, _PyMonitoringEventSet events)
{
    PyInterpreterState *interp = _PyInterpreterState_Get();
    assert((events & C_CALL_EVENTS) == 0 || (events & C_CALL_EVENTS) == C_CALL_EVENTS);
    uint32_t existing_events = matrix_get_events(&interp->monitoring_matrix, tool_id);
    if (existing_events == events) {
        return;
    }
    int multitools = 0;
    for (int e = 0; e < PY_MONITORING_EVENTS; e++) {
        int val = (events >> e) & 1;
        matrix_set_bit(&interp->monitoring_matrix, e, tool_id, val);
        uint8_t tools = interp->monitoring_matrix.tools[e];
        if (_Py_popcount32(tools) <= 1) {
            interp->sole_tool_plus1[e] = _Py_bit_length(tools);
        }
        else {
            multitools++;
            interp->sole_tool_plus1[e] = -1;
        }
    }
    assert(cross_check_events(interp));
    interp->multiple_tools = multitools;
    if (multitools) {
        interp->multiple_tools = true;
    }
    bool lines = interp->monitoring_matrix.tools[PY_MONITORING_EVENT_LINE] != 0;
    bool opcodes = interp->monitoring_matrix.tools[PY_MONITORING_EVENT_INSTRUCTION] != 0;
    if (interp->multiple_tools) {
        if (opcodes) {
            interp->required_monitoring_bytes = 6;
        }
        else if (lines) {
            interp->required_monitoring_bytes = 4;
        }
        else {
            interp->required_monitoring_bytes = 1;
        }
    }
    else {
        if (opcodes) {
            interp->required_monitoring_bytes = 3;
        }
        else if (lines) {
            interp->required_monitoring_bytes = 2;
        }
        else {
            interp->required_monitoring_bytes = 0;
        }
    }
    interp->monitoring_version++;

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
        PyErr_Format(PyExc_ValueError, "invalid tool %d (must be between 0 and 5)", tool_id);
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
        PyErr_Format(PyExc_ValueError, "tool %d is already in use", tool_id);
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
        PyErr_Format(PyExc_ValueError, "invalid event %d", event);
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
    if (check_valid_tool(tool_id))  {
        return NULL;
    }
    PyInterpreterState *interp = _PyInterpreterState_Get();
    _PyMonitoringEventSet event_set = matrix_get_events(&interp->monitoring_matrix, tool_id);
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
    if (check_valid_tool(tool_id))  {
        return NULL;
    }
    if (event_set < 0 || event_set >= (1 << PY_MONITORING_EVENTS)) {
        PyErr_Format(PyExc_ValueError, "invalid event set 0x%x", event_set);
        return NULL;
    }
    if ((event_set & C_RETURN_EVENTS) && (event_set & C_CALL_EVENTS) != C_CALL_EVENTS) {
        PyErr_Format(PyExc_ValueError, "cannot set C_RETURN or C_RAISE events independently");
        return NULL;
    }
    if (event_set & (1 << PY_MONITORING_EVENT_CALL)) {
        event_set |= C_RETURN_EVENTS;
    }
    _PyMonitoring_SetEvents(tool_id, event_set);
    Py_RETURN_NONE;
}

/*[clinic input]
monitoring.restart_events

[clinic start generated code]*/

static PyObject *
monitoring_restart_events_impl(PyObject *module)
/*[clinic end generated code: output=e025dd5ba33314c4 input=add8a855063c8008]*/
{
    /* We want to ensure that:
     * last restart version < current version
     * last restart version > instrumented version for all code objects
     */
    PyInterpreterState *interp = _PyInterpreterState_Get();
    interp->last_restart_version = interp->monitoring_version + 1;
    interp->monitoring_version += 2;
    Py_RETURN_NONE;
}


static PyMethodDef methods[] = {
    MONITORING_USE_TOOL_METHODDEF
    MONITORING_FREE_TOOL_METHODDEF
    MONITORING_GET_TOOL_METHODDEF
    MONITORING_REGISTER_CALLBACK_METHODDEF
    MONITORING_GET_EVENTS_METHODDEF
    MONITORING_SET_EVENTS_METHODDEF
    MONITORING_RESTART_EVENTS_METHODDEF
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
    [15] = "Unused",
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




