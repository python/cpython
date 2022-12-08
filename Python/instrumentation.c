

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
    [INSTRUMENTED_LINE] = INSTRUMENTED_LINE,
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

_PyInstrumentationOffsets
get_offsets(PyCodeObject *code) {
    return code->_co_instrumentation.layout.offsets;
}

uint8_t *
get_instruction_data(PyCodeObject * code, int offset)
{
    _PyInstrumentationOffsets offsets = get_offsets(code);
    return &code->_co_instrumentation.monitoring_data[offset*offsets.size];
}

static inline uint8_t
get_tools(PyCodeObject * code, int index, int event)
{
    uint8_t tools;
    assert(event != PY_MONITORING_EVENT_LINE);
    assert(event != PY_MONITORING_EVENT_INSTRUCTION);

    _PyInstrumentationOffsets offsets = get_offsets(code);

    if (event < PY_MONITORING_EVENT_PY_THROW && offsets.tools >= 0) {
        assert(offsets.tools == 0);
        tools = get_instruction_data(code, index)[0];
    }
    else {
        tools = code->_co_instrumentation.monitoring_matrix.tools[event];
    }
    assert((tools & PyInterpreterState_Get()->monitoring_matrix.tools[event]) == tools);
    assert((tools & code->_co_instrumentation.monitoring_matrix.tools[event]) == tools);
    return tools;
}

static inline bool
is_instrumented(int opcode) {
    return INSTRUMENTED_OPCODES[opcode] == opcode;
}

static void
de_instrument(PyCodeObject *code, int offset, int event)
{
    assert(event != PY_MONITORING_EVENT_LINE);
    _Py_CODEUNIT *instr = &_PyCode_CODE(code)[offset];
    int opcode = _Py_OPCODE(*instr);
    if (!is_instrumented(opcode)) {
        return;
    }
    int deinstrumented = DE_INSTRUMENT[opcode];
    int base_opcode;
    if (deinstrumented) {
        base_opcode = _PyOpcode_Deopt[deinstrumented];
        assert(base_opcode != 0);
        *instr = _Py_MAKECODEUNIT(base_opcode, _Py_OPARG(*instr));
    }
    else {
        /* TO DO -- handle INSTRUMENTED_INSTRUCTION */
        assert(opcode == INSTRUMENTED_LINE);
        uint8_t *instr_data = get_instruction_data(code, offset);
        uint8_t *orignal_opcode_ptr = instr_data + get_offsets(code).lines;
        int orignal_opcode = *orignal_opcode_ptr;
        base_opcode = _PyOpcode_Deopt[orignal_opcode];
        assert(INSTRUMENTED_OPCODES[orignal_opcode] == orignal_opcode);
        assert(base_opcode != 0);
        *orignal_opcode_ptr = base_opcode;
    }
    if (_PyOpcode_Caches[base_opcode]) {
        instr[1] = adaptive_counter_warmup();
    }
}

static void
de_instrument_line(PyCodeObject *code, int offset)
{
    /* TO DO -- handle INSTRUMENTED_INSTRUCTION */
    _Py_CODEUNIT *instr = &_PyCode_CODE(code)[offset];
    int opcode = _Py_OPCODE(*instr);
    assert(opcode == INSTRUMENTED_LINE);
    uint8_t *instr_data = get_instruction_data(code, offset);
    uint8_t *orignal_opcode_ptr = instr_data + get_offsets(code).lines;
    int orignal_opcode = *orignal_opcode_ptr;
    if (is_instrumented(orignal_opcode)) {
        /* Instrumented original */
        *instr = _Py_MAKECODEUNIT(orignal_opcode,  _Py_OPARG(*instr));
    }
    else {
        int base_opcode = _PyOpcode_Deopt[orignal_opcode];
        assert(base_opcode != 0);
        *instr = _Py_MAKECODEUNIT(base_opcode, _Py_OPARG(*instr));
        if (_PyOpcode_Caches[base_opcode]) {
            instr[1] = adaptive_counter_warmup();
        }
    }
}

static void
instrument(PyCodeObject *code, int offset)
{
    /* TO DO -- handle INSTRUMENTED_INSTRUCTION */
    _Py_CODEUNIT *instr = &_PyCode_CODE(code)[offset];
    int opcode = _Py_OPCODE(*instr);
    if (opcode == INSTRUMENTED_LINE) {
        uint8_t *instr_data = get_instruction_data(code, offset);
        uint8_t *orignal_opcode_ptr = instr_data + get_offsets(code).lines;
        opcode = *orignal_opcode_ptr;
        assert(!is_instrumented(opcode));
        *orignal_opcode_ptr = INSTRUMENTED_OPCODES[opcode];
    }
    else if (!is_instrumented(opcode)) {
        opcode = _PyOpcode_Deopt[opcode];
        int instrumented = INSTRUMENTED_OPCODES[opcode];
        assert(instrumented);
        *instr = _Py_MAKECODEUNIT(instrumented, _Py_OPARG(*instr));
        if (_PyOpcode_Caches[opcode]) {
            instr[1] = adaptive_counter_warmup();
        }
    }
}

static void
instrument_line(PyCodeObject *code, int offset)
{
    /* TO DO -- handle INSTRUMENTED_INSTRUCTION */
    _Py_CODEUNIT *instr = &_PyCode_CODE(code)[offset];
    int opcode = _Py_OPCODE(*instr);
    assert(get_offsets(code).lines >= 0);
    uint8_t *instr_data = get_instruction_data(code, offset);
    uint8_t *orignal_opcode_ptr = instr_data + get_offsets(code).lines;
    if (is_instrumented(opcode)) {
        *orignal_opcode_ptr = opcode;
    }
    else {
        assert(_PyOpcode_Deopt[opcode] != 0);
        *orignal_opcode_ptr = _PyOpcode_Deopt[opcode];
    }
    *instr = _Py_MAKECODEUNIT(INSTRUMENTED_LINE, _Py_OPARG(*instr));
}

static void
remove_tools(PyCodeObject * code, int offset, int event, int tools)
{
    assert(event != PY_MONITORING_EVENT_LINE);
    assert(event != PY_MONITORING_EVENT_INSTRUCTION);
    _PyInstrumentationOffsets offsets = get_offsets(code);
    if (offsets.tools < 0) {
        /* Single tool */
        uint8_t single_tool = code->_co_instrumentation.monitoring_matrix.tools[event];
        assert(_Py_popcount32(single_tool) <= 1);
        if (((single_tool & tools) == single_tool) && event < PY_MONITORING_INSTRUMENTED_EVENTS) {
            de_instrument(code, offset, event);
        }
    }
    else {
        assert(offsets.tools == 0);
        uint8_t *toolsptr = get_instruction_data(code, offset);
        *toolsptr &= ~tools;
        if (*toolsptr == 0 && event < PY_MONITORING_INSTRUMENTED_EVENTS) {
            de_instrument(code, offset, event);
        }
    }
}

static void
remove_line_tools(PyCodeObject * code, int offset, int tools)
{
    assert (code->_co_instrumentation.layout.offsets.lines >= 0);
    if (get_offsets(code).tools < 0) {
        /* Single tool */
        uint8_t single_tool = code->_co_instrumentation.monitoring_matrix.tools[PY_MONITORING_EVENT_LINE];
        assert(_Py_popcount32(single_tool) <= 1);
        if (((single_tool & tools) == single_tool)) {
            de_instrument_line(code, offset);
        }
    }
    else {
        uint8_t *instr_data = get_instruction_data(code, offset);
        uint8_t *toolsptr = instr_data + get_offsets(code).lines + 2;
        *toolsptr &= ~tools;
        if (*toolsptr == 0 ) {
            de_instrument_line(code, offset);
        }
    }
}


static void
add_tools(PyCodeObject * code, int offset, int event, int tools)
{
    assert(event != PY_MONITORING_EVENT_LINE);
    assert(event != PY_MONITORING_EVENT_INSTRUCTION);
    _PyInstrumentationOffsets offsets = get_offsets(code);
    if (offsets.tools < 0) {
        /* Single tool */
        assert(PyInterpreterState_Get()->monitoring_matrix.tools[event] == tools);
        assert(_Py_popcount32(tools) == 1);
    }
    else {
        assert(offsets.tools == 0);
        uint8_t *toolsptr = get_instruction_data(code, offset);
        *toolsptr |= tools;
    }
    if (event < PY_MONITORING_INSTRUMENTED_EVENTS) {
        instrument(code, offset);
    }
}

static void
add_line_tools(PyCodeObject * code, int offset, int tools)
{
    assert (get_offsets(code).lines >= 0);
    if (code->_co_instrumentation.layout.offsets.tools < 0) {
        assert(PyInterpreterState_Get()->monitoring_matrix.tools[PY_MONITORING_EVENT_LINE] == (1 << PY_MONITORING_EVENT_LINE));
        assert(_Py_popcount32(tools) == 1);
        /* Single tool */
        instrument_line(code, offset);
    }
    else {
        uint8_t *toolsptr = get_instruction_data(code, offset) + get_offsets(code).lines + 2;
        instrument_line(code, offset);
        *toolsptr |= tools;
    }
}

/* Return 1 if DISABLE returned, -1 if error, 0 otherwise */
static int
call_one_instrument(
    PyInterpreterState *interp, PyThreadState *tstate, PyObject **args,
    Py_ssize_t nargsf, int8_t tool, int event)
{
    assert(0 <= tool && tool < 8);
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

static const int8_t MOST_SIGNIFICANT_BITS[16] = {
    -1, 0, 1, 1,
    2, 2, 2, 2,
    3, 3, 3, 3,
    3, 3, 3, 3,
};

/* We could use _Py_bit_length here, but that is designed for larger (32/64) bit ints,
  and can perform relatively poorly on platforms without the necessary intrinsics. */
static inline int most_significant_bit(uint8_t bits) {
    assert(bits != 0);
    if (bits > 15) {
        return MOST_SIGNIFICANT_BITS[bits>>4]+4;
    }
    else {
        return MOST_SIGNIFICANT_BITS[bits];
    }
}

static int
call_instrument(PyThreadState *tstate, PyCodeObject *code, int event,
    PyObject **args, Py_ssize_t nargsf, _Py_CODEUNIT *instr)
{
    PyInterpreterState *interp = tstate->interp;
    int offset = instr - _PyCode_CODE(code);
    uint8_t tools = get_tools(code, offset, event);
    /* No per-instruction monitoring yet  */
    assert(code->_co_instrumentation.layout.offsets.instructions < 0);
    // assert(tools);
    while (tools) {
        int tool = most_significant_bit(tools);
        assert(tool >= 0 && tool < 8);
        assert(tools & (1 << tool));
        tools &= ~(1 << tool);
        int res = call_one_instrument(interp, tstate, args, nargsf, tool, event);
        if (res == 0) {
            /* Nothing to do */
        }
        else if (res < 0) {
            /* error */
            return -1;
        }
        else {
            /* DISABLE */
            remove_tools(code, offset, event, 1 << tool);
        }
    }
    return 0;
}

static inline bool
matrix_equals(_Py_MonitoringMatrix a, _Py_MonitoringMatrix b)
{
    for (int i = 0; i < PY_MONITORING_EVENTS; i++) {
        if (a.tools[i] != b.tools[i]) {
            return false;
        }
    }
    return true;
}

int
_Py_call_instrumentation(
    PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _Py_CODEUNIT *instr)
{
    PyCodeObject *code = frame->f_code;
    assert(code->_co_instrumentation.monitoring_version == tstate->interp->monitoring_version);
    assert(matrix_equals(code->_co_instrumentation.monitoring_matrix, tstate->interp->monitoring_matrix));
    int instruction_offset = instr - _PyCode_CODE(code);
    PyObject *instruction_offset_obj = PyLong_FromSsize_t(instruction_offset);
    PyObject *args[3] = { NULL, (PyObject *)code, instruction_offset_obj };
    int err = call_instrument(tstate, code, event, &args[1], 2 | PY_VECTORCALL_ARGUMENTS_OFFSET, instr);
    Py_DECREF(instruction_offset_obj);
    return err;
}

int
_Py_call_instrumentation_arg(
    PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _Py_CODEUNIT *instr, PyObject *arg)
{
    PyCodeObject *code = frame->f_code;
    assert(code->_co_instrumentation.monitoring_version == tstate->interp->monitoring_version);
    assert(matrix_equals(code->_co_instrumentation.monitoring_matrix, tstate->interp->monitoring_matrix));
    int instruction_offset = instr - _PyCode_CODE(code);
    PyObject *instruction_offset_obj = PyLong_FromSsize_t(instruction_offset);
    PyObject *args[4] = { NULL, (PyObject *)code, instruction_offset_obj, arg };
    int err = call_instrument(tstate, code, event, &args[1], 3 | PY_VECTORCALL_ARGUMENTS_OFFSET, instr);
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
    assert(code->_co_instrumentation.monitoring_version == tstate->interp->monitoring_version);
    assert(matrix_equals(code->_co_instrumentation.monitoring_matrix, tstate->interp->monitoring_matrix));
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
}

/* Line delta.
 * 8 bit value.
 * if line_delta != -128:
 *     line = first_line  + (offset >> 3) + line_delta;
 * else:
 *     line = PyCode_Addr2Line(code, offset * sizeof(_Py_CODEUNIT));
 */

static int8_t
compute_line_delta(PyCodeObject *code, int offset, int line)
{
    int delta = line - code->co_firstlineno - (offset >> 3);
    if (delta == (int8_t)delta) {
        return delta;
    }
    return -128;
}

static int
compute_line(PyCodeObject *code, int offset, int8_t line_delta)
{
    if (line_delta == -128) {
        /* Compute from table */
        return PyCode_Addr2Line(code, offset * sizeof(_Py_CODEUNIT));
    }
    return code->co_firstlineno + (offset >> 3) + line_delta;
}

int
_Py_call_instrumentation_line(PyThreadState *tstate, PyCodeObject *code, _Py_CODEUNIT *instr)
{
    PyInterpreterState *interp = tstate->interp;
    int offset = instr - _PyCode_CODE(code);
    assert(code->_co_instrumentation.monitoring_version == tstate->interp->monitoring_version);
    assert(matrix_equals(code->_co_instrumentation.monitoring_matrix, tstate->interp->monitoring_matrix));
    _PyCoInstrumentation *instrumentation = &code->_co_instrumentation;
    _PyInstrumentationLayout layout = instrumentation->layout;
    uint8_t *line_data = &instrumentation->monitoring_data[layout.offsets.size*offset + layout.offsets.lines];
    uint8_t original_opcode = line_data[0];
    int8_t line_delta = ((int8_t *)(char *)line_data)[1];
    int line = compute_line(code, offset, line_delta);
    uint8_t tools = layout.offsets.tools >= 0 ? line_data[2] : interp->monitoring_matrix.tools[PY_MONITORING_EVENT_LINE];
    PyObject *line_obj = PyLong_FromSsize_t(line);
    if (line_obj == NULL) {
        return -1;
    }
    PyObject *args[3] = { NULL, (PyObject *)code, line_obj };
    while (tools) {
        int tool = most_significant_bit(tools);
        assert(tool < 8);
        assert(tools & (1 << tool));
        tools &= ~(1 << tool);
        int res = call_one_instrument(interp, tstate, args,
                                      2 | PY_VECTORCALL_ARGUMENTS_OFFSET,
                                      tool, PY_MONITORING_EVENT_LINE);
        if (res == 0) {
            /* Nothing to do */
        }
        else if (res < 0) {
            /* error */
            Py_DECREF(line_obj);
            return -1;
        }
        else {
            /* DISABLE  */
            remove_line_tools(code, offset, 1 << tool);
        }
    }
    Py_DECREF(line_obj);
    return original_opcode;
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

static const int8_t EVENT_FOR_OPCODE[256] = {
    [RETURN_VALUE] = PY_MONITORING_EVENT_PY_RETURN,
    [INSTRUMENTED_RETURN_VALUE] = PY_MONITORING_EVENT_PY_RETURN,
    [CALL] = PY_MONITORING_EVENT_CALL,
    [INSTRUMENTED_CALL] = PY_MONITORING_EVENT_CALL,
    [CALL_FUNCTION_EX] = PY_MONITORING_EVENT_CALL,
    [INSTRUMENTED_CALL_FUNCTION_EX] = PY_MONITORING_EVENT_CALL,
    [RESUME] = -1,
    [YIELD_VALUE] = PY_MONITORING_EVENT_PY_YIELD,
    [INSTRUMENTED_YIELD_VALUE] = PY_MONITORING_EVENT_PY_YIELD,
};

static const bool OPCODE_HAS_EVENT[256] = {
    [RETURN_VALUE] = true,
    [INSTRUMENTED_RETURN_VALUE] = true,
    [CALL] = true,
    [INSTRUMENTED_CALL] = true,
    [CALL_FUNCTION_EX] = true,
    [INSTRUMENTED_CALL_FUNCTION_EX] = true,
    [RESUME] = true,
    [INSTRUMENTED_RESUME] = true,
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
    assert(val == 0 || val == 1);
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

static bool
is_instrumentation_up_to_date(PyCodeObject *code, PyInterpreterState *interp)
{
    return interp->monitoring_version == code->_co_instrumentation.monitoring_version;
}

static inline _PyInstrumentationLayout
layout_from_flags(bool multitools, bool lines, bool instrs) {
    _PyInstrumentationLayout result;
    int size = multitools;
    result.offsets.tools = multitools - 1;
    result.offsets.lines = -1;
    result.offsets.instructions = -1;
    if (lines) {
        result.offsets.lines = size;
        size += multitools ? 3 : 2;
    }
    if (instrs) {
        result.offsets.instructions = size;
        size += multitools ? 2 : 1;
    }
    result.offsets.size = size;
    return result;
}

static inline int
tools_data_offset() {
    return 0;
}

static void
initialize_tools(PyCodeObject *code, int size)
{
    int code_len = (int)Py_SIZE(code);
    for (int i = 0; i < code_len; i++) {
        _Py_CODEUNIT *instr = &_PyCode_CODE(code)[i];
        int opcode = _Py_OPCODE(*instr);
        bool instrumented = is_instrumented(opcode);
        assert(opcode != INSTRUMENTED_LINE);
        if (instrumented) {
            opcode = DE_INSTRUMENT[opcode];
            assert(opcode != 0);
        }
        opcode = _PyOpcode_Deopt[opcode];
        if (OPCODE_HAS_EVENT[opcode]) {
            uint8_t *tools_ptr = &code->_co_instrumentation.monitoring_data[i*size];
            if (instrumented) {
                int8_t event;
                if (opcode == RESUME) {
                    int oparg = _Py_OPARG(*instr);
                    event = oparg != 0;
                }
                else {
                    event = EVENT_FOR_OPCODE[opcode];
                    assert(event > 0);
                }
                assert(event >= 0);
                *tools_ptr = code->_co_instrumentation.monitoring_matrix.tools[event];
                assert(*tools_ptr != 0);
            }
            else {
                *tools_ptr = 0;
            }
        }
        i += _PyOpcode_Caches[opcode];
    }
}

static void
copy_data(uint8_t *old_data, int old_size, int old_offset, uint8_t *new_data,int new_size, int new_offset, int code_len)
{
    assert(new_size > 0);
    assert(old_offset >= 0);
    assert(new_offset >= 0);
    for (int i = 0; i < code_len; i++) {
        new_data[i*new_size + new_offset] = old_data[i*old_size+old_offset];
    }
}

static void
initialize_lines(PyCodeObject *code, PyInterpreterState *interp)
{
    int code_len = (int)Py_SIZE(code);
    int size = code->_co_instrumentation.layout.offsets.size;
    int offset = code->_co_instrumentation.layout.offsets.lines;
    int prev_line = -1;
    for (int i = 0; i < code_len; i++) {
        uint8_t *line_data = &code->_co_instrumentation.monitoring_data[i*size+offset];
        /* TO DO -- Use line iterator here */
        int line = PyCode_Addr2Line(code, offset * sizeof(_Py_CODEUNIT));
        if (line != prev_line && line <= 0) {
            uint8_t opcode = _Py_OPCODE(_PyCode_CODE(code)[i]);
            assert(opcode != 0);
            line_data[0] = opcode;
            line_data[1] = compute_line_delta(code, i, line);
            prev_line = line;
        }
        else {
            line_data[0] = 0;
            line_data[1] = -128;
        }
    }
}

static void
initialize_line_tools(PyCodeObject *code, PyInterpreterState *interp)
{
    int code_len = (int)Py_SIZE(code);
    int size = code->_co_instrumentation.layout.offsets.size;
    int offset = code->_co_instrumentation.layout.offsets.lines;
    for (int i = 0; i < code_len; i++) {
        code->_co_instrumentation.monitoring_data[i*size+offset+2] =
                interp->monitoring_matrix.tools[PY_MONITORING_EVENT_LINE];
    }
}

int
_Py_Instrument(PyCodeObject *code, PyInterpreterState *interp)
{

    if (is_instrumentation_up_to_date(code, interp)) {
        assert(matrix_equals(code->_co_instrumentation.monitoring_matrix, interp->monitoring_matrix));
        return 0;
    }
    bool restarted = interp->last_restart_version > code->_co_instrumentation.monitoring_version;
    /* First establish how much extra space will need,
     * and free/allocate it if different to what we had before */
    /* For now only allow only one tool per event */
    assert(interp->monitoring_matrix.tools[PY_MONITORING_EVENT_INSTRUCTION] == 0);
    int code_len = (int)Py_SIZE(code);
    _PyInstrumentationLayout new_layout = interp->instrumentation_layout;
    _PyInstrumentationLayout old_layout = code->_co_instrumentation.layout;
    if (new_layout.bits != old_layout.bits) {
        uint8_t *old_data = code->_co_instrumentation.monitoring_data;
        if (new_layout.offsets.size) {
            code->_co_instrumentation.monitoring_data = PyMem_Malloc(code_len * new_layout.offsets.size);
            if (code->_co_instrumentation.monitoring_data == NULL) {
                PyErr_NoMemory();
                code->_co_instrumentation.monitoring_data = old_data;
                return -1;
            }
        }
        else {
            code->_co_instrumentation.monitoring_data = NULL;
        }
        if (new_layout.offsets.tools >= 0) {
            if (restarted) {
                uint8_t zero = 0;
                copy_data(&zero, 0, 0,
                          code->_co_instrumentation.monitoring_data,
                          new_layout.offsets.size, 0, code_len);
            } else if (old_layout.offsets.tools < 0) {
                initialize_tools(code, new_layout.offsets.size);
            }
            else {
                copy_data(old_data, old_layout.offsets.size,
                          0, code->_co_instrumentation.monitoring_data,
                          new_layout.offsets.size, 0, code_len);
            }
        }
        if (new_layout.offsets.lines >= 0) {
            if (old_layout.offsets.lines < 0) {
                initialize_lines(code, interp);
            }
            else {
                copy_data(old_data, old_layout.offsets.size,
                          old_layout.offsets.lines+1,
                          code->_co_instrumentation.monitoring_data,
                          new_layout.offsets.size,
                          new_layout.offsets.lines+1, code_len);
            }
            if (new_layout.offsets.tools >= 0) {
                if (old_layout.offsets.tools < 0) {
                    initialize_line_tools(code, interp);
                }
                else {
                    copy_data(old_data, old_layout.offsets.size,
                            old_layout.offsets.lines+2,
                            code->_co_instrumentation.monitoring_data,
                            new_layout.offsets.size,
                            new_layout.offsets.lines+2, code_len);
                }
            }
        }
        assert(new_layout.offsets.instructions < 0);
        if (old_data) {
            PyMem_Free(old_data);
        }
        code->_co_instrumentation.layout.bits = new_layout.bits;
    }
    //printf("Instrumenting code object at %p, code version: %ld interpreter version %ld\n",
    //       code, code->_co_instrument_version, interp->monitoring_version);
    /* Avoid instrumenting code that has been disabled.
     * Only instrument new events
     */
    _Py_MonitoringMatrix new_events;
    _Py_MonitoringMatrix removed_events;

    if (restarted) {
        removed_events = code->_co_instrumentation.monitoring_matrix;
        new_events = interp->monitoring_matrix;
    }
    else {
        removed_events = matrix_sub(code->_co_instrumentation.monitoring_matrix, interp->monitoring_matrix);
        new_events = matrix_sub(interp->monitoring_matrix, code->_co_instrumentation.monitoring_matrix);
        assert(matrix_empty(matrix_and(new_events, removed_events)));
    }
    code->_co_instrumentation.monitoring_matrix = interp->monitoring_matrix;
    assert(code->_co_instrumentation.layout.bits == interp->instrumentation_layout.bits);
    code->_co_instrumentation.monitoring_version = interp->monitoring_version;
    if (matrix_empty(new_events) && matrix_empty(removed_events)) {
        return 0;
    }
    /* Insert instrumentation */
    for (int i = 0; i < code_len; i++) {
        _Py_CODEUNIT *instr = &_PyCode_CODE(code)[i];
        int opcode = _Py_OPCODE(*instr);
        int base_opcode;
        if (DE_INSTRUMENT[opcode]) {
            base_opcode = _PyOpcode_Deopt[DE_INSTRUMENT[opcode]];
        }
        else {
            base_opcode = _PyOpcode_Deopt[opcode];
        }
        if (OPCODE_HAS_EVENT[base_opcode]) {
            int8_t event;
            if (base_opcode == RESUME) {
                int oparg = _Py_OPARG(*instr);
                event = oparg;
            }
            else {
                event = EVENT_FOR_OPCODE[base_opcode];
                assert(event > 0);
            }
            uint8_t removed_tools = removed_events.tools[event];
            if (removed_tools) {
                remove_tools(code, i, event, removed_tools);
            }
            uint8_t new_tools = new_events.tools[event];
            if (new_tools) {
                add_tools(code, i, event, new_tools);
            }
        }
        i += _PyOpcode_Caches[base_opcode];
    }
    uint8_t new_line_tools = new_events.tools[PY_MONITORING_EVENT_LINE];
    if (new_line_tools) {
        /* Insert line instrumentation */
        int offset = new_layout.offsets.lines;
        assert(offset >= 0);
        for (int i = 0; i < code_len; i++) {
            uint8_t *instr_data = get_instruction_data(code, i);
            if (instr_data[get_offsets(code).lines]) {
                add_line_tools(code, i, new_line_tools);
            }
        }
    }
    uint8_t removed_line_tools = removed_events.tools[PY_MONITORING_EVENT_LINE];
    if (removed_line_tools) {
        for (int i = 0; i < code_len; i++) {
            uint8_t *instr_data = get_instruction_data(code, i);
            if (instr_data[get_offsets(code).lines]) {
                remove_line_tools(code, i, removed_line_tools);
            }
        }
    }
    return 0;
}

#define C_RETURN_EVENTS \
    ((1 << PY_MONITORING_EVENT_C_RETURN) | \
    (1 << PY_MONITORING_EVENT_C_RAISE))

#define C_CALL_EVENTS \
    (C_RETURN_EVENTS | (1 << PY_MONITORING_EVENT_CALL))


static void
instrument_all_executing_code_objects(PyInterpreterState *interp) {
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

void
_PyMonitoring_SetEvents(int tool_id, _PyMonitoringEventSet events)
{
    PyInterpreterState *interp = _PyInterpreterState_Get();
    assert((events & C_CALL_EVENTS) == 0 || (events & C_CALL_EVENTS) == C_CALL_EVENTS);
    uint32_t existing_events = matrix_get_events(&interp->monitoring_matrix, tool_id);
    if (existing_events == events) {
        return;
    }
    bool multitools = false;
    for (int e = 0; e < PY_MONITORING_EVENTS; e++) {
        int val = (events >> e) & 1;
        matrix_set_bit(&interp->monitoring_matrix, e, tool_id, val);
        uint8_t tools = interp->monitoring_matrix.tools[e];
        if (_Py_popcount32(tools) > 1) {
            multitools = true;
        }
    }
    interp->monitoring_version++;
    bool line_monitoring = interp->monitoring_matrix.tools[PY_MONITORING_EVENT_LINE] != 0;
    assert(interp->monitoring_matrix.tools[PY_MONITORING_EVENT_INSTRUCTION] == 0);
    bool instruction_monitoring = false;
    interp->instrumentation_layout = layout_from_flags(multitools, line_monitoring, instruction_monitoring);

    instrument_all_executing_code_objects(interp);
}

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
     * last restart version > instrumented version for all code objects
     * last restart version < current version
     */
    PyInterpreterState *interp = _PyInterpreterState_Get();
    interp->last_restart_version = interp->monitoring_version + 1;
    interp->monitoring_version = interp->last_restart_version + 1;
    instrument_all_executing_code_objects(interp);
    Py_RETURN_NONE;
}

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

/*[clinic input]
monitoring._all_events
[clinic start generated code]*/

static PyObject *
monitoring__all_events_impl(PyObject *module)
/*[clinic end generated code: output=6b7581e2dbb690f6 input=62ee9672c17b7f0e]*/
{
    PyInterpreterState *interp = _PyInterpreterState_Get();
    PyObject *res = PyDict_New();
    if (res == NULL) {
        return NULL;
    }
    for (int e = 0; e < PY_MONITORING_EVENTS; e++) {
        uint8_t tools = interp->monitoring_matrix.tools[e];
        if (tools == 0) {
            continue;
        }
        int err = PyDict_SetItemString(res, event_names[e], PyLong_FromLong(tools));
        if (err < 0) {
            Py_DECREF(res);
            return NULL;
        }
    }
    return res;
}

static PyMethodDef methods[] = {
    MONITORING_USE_TOOL_METHODDEF
    MONITORING_FREE_TOOL_METHODDEF
    MONITORING_GET_TOOL_METHODDEF
    MONITORING_REGISTER_CALLBACK_METHODDEF
    MONITORING_GET_EVENTS_METHODDEF
    MONITORING_SET_EVENTS_METHODDEF
    MONITORING_RESTART_EVENTS_METHODDEF
    MONITORING__ALL_EVENTS_METHODDEF
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
