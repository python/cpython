

#include "Python.h"
#include "pycore_frame.h"
#include "pycore_interp.h"
#include "pycore_namespace.h"
#include "pycore_opcode.h"
#include "pycore_pyerrors.h"
#include "pycore_pystate.h"

/* Uncomment this to dump debugging output when assertions fail */
// #define INSTRUMENT_DEBUG 1

static PyObject DISABLE =
{
    1 << 30,
    &PyBaseObject_Type
};

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
    [JUMP_FORWARD] = PY_MONITORING_EVENT_JUMP,
    [JUMP_BACKWARD] = PY_MONITORING_EVENT_JUMP,
    [JUMP_IF_FALSE_OR_POP] = PY_MONITORING_EVENT_BRANCH,
    [JUMP_IF_TRUE_OR_POP] = PY_MONITORING_EVENT_BRANCH,
    [POP_JUMP_IF_FALSE] = PY_MONITORING_EVENT_BRANCH,
    [POP_JUMP_IF_TRUE] = PY_MONITORING_EVENT_BRANCH,
    [POP_JUMP_IF_NONE] = PY_MONITORING_EVENT_BRANCH,
    [POP_JUMP_IF_NOT_NONE] = PY_MONITORING_EVENT_BRANCH,
    [COMPARE_AND_BRANCH] = PY_MONITORING_EVENT_BRANCH,
    [INSTRUMENTED_JUMP_FORWARD] = PY_MONITORING_EVENT_JUMP,
    [INSTRUMENTED_JUMP_BACKWARD] = PY_MONITORING_EVENT_JUMP,
    [INSTRUMENTED_JUMP_IF_FALSE_OR_POP] = PY_MONITORING_EVENT_BRANCH,
    [INSTRUMENTED_JUMP_IF_TRUE_OR_POP] = PY_MONITORING_EVENT_BRANCH,
    [INSTRUMENTED_POP_JUMP_IF_FALSE] = PY_MONITORING_EVENT_BRANCH,
    [INSTRUMENTED_POP_JUMP_IF_TRUE] = PY_MONITORING_EVENT_BRANCH,
    [INSTRUMENTED_POP_JUMP_IF_NONE] = PY_MONITORING_EVENT_BRANCH,
    [INSTRUMENTED_POP_JUMP_IF_NOT_NONE] = PY_MONITORING_EVENT_BRANCH,
    [INSTRUMENTED_COMPARE_AND_BRANCH] = PY_MONITORING_EVENT_BRANCH,
    [FOR_ITER] = PY_MONITORING_EVENT_BRANCH,
    [INSTRUMENTED_FOR_ITER] = PY_MONITORING_EVENT_BRANCH,
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
    [JUMP_FORWARD] = true,
    [JUMP_BACKWARD] = true,
    [JUMP_IF_FALSE_OR_POP] = true,
    [JUMP_IF_TRUE_OR_POP] = true,
    [POP_JUMP_IF_FALSE] = true,
    [POP_JUMP_IF_TRUE] = true,
    [POP_JUMP_IF_NONE] = true,
    [POP_JUMP_IF_NOT_NONE] = true,
    [COMPARE_AND_BRANCH] = true,
    [INSTRUMENTED_JUMP_FORWARD] = true,
    [INSTRUMENTED_JUMP_BACKWARD] = true,
    [INSTRUMENTED_JUMP_IF_FALSE_OR_POP] = true,
    [INSTRUMENTED_JUMP_IF_TRUE_OR_POP] = true,
    [INSTRUMENTED_POP_JUMP_IF_FALSE] = true,
    [INSTRUMENTED_POP_JUMP_IF_TRUE] = true,
    [INSTRUMENTED_POP_JUMP_IF_NONE] = true,
    [INSTRUMENTED_POP_JUMP_IF_NOT_NONE] = true,
    [INSTRUMENTED_COMPARE_AND_BRANCH] = true,
    [INSTRUMENTED_FOR_ITER] = true,
};

static const uint8_t DE_INSTRUMENT[256] = {
    [INSTRUMENTED_RESUME] = RESUME,
    [INSTRUMENTED_RETURN_VALUE] = RETURN_VALUE,
    [INSTRUMENTED_CALL] = CALL,
    [INSTRUMENTED_CALL_FUNCTION_EX] = CALL_FUNCTION_EX,
    [INSTRUMENTED_YIELD_VALUE] = YIELD_VALUE,
    [INSTRUMENTED_JUMP_FORWARD] = JUMP_FORWARD,
    [INSTRUMENTED_JUMP_BACKWARD] = JUMP_BACKWARD,
    [INSTRUMENTED_JUMP_IF_FALSE_OR_POP] = JUMP_IF_FALSE_OR_POP,
    [INSTRUMENTED_JUMP_IF_TRUE_OR_POP] = JUMP_IF_TRUE_OR_POP,
    [INSTRUMENTED_POP_JUMP_IF_FALSE] = POP_JUMP_IF_FALSE,
    [INSTRUMENTED_POP_JUMP_IF_TRUE] = POP_JUMP_IF_TRUE,
    [INSTRUMENTED_POP_JUMP_IF_NONE] = POP_JUMP_IF_NONE,
    [INSTRUMENTED_POP_JUMP_IF_NOT_NONE] = POP_JUMP_IF_NOT_NONE,
    [INSTRUMENTED_COMPARE_AND_BRANCH] = COMPARE_AND_BRANCH,
    [INSTRUMENTED_FOR_ITER] = FOR_ITER,
};

static const uint8_t INSTRUMENTED_OPCODES[256] = {
    [RETURN_VALUE] = INSTRUMENTED_RETURN_VALUE,
    [INSTRUMENTED_RETURN_VALUE] = INSTRUMENTED_RETURN_VALUE,
    [CALL] = INSTRUMENTED_CALL,
    [INSTRUMENTED_CALL] = INSTRUMENTED_CALL,
    [CALL_FUNCTION_EX] = INSTRUMENTED_CALL_FUNCTION_EX,
    [INSTRUMENTED_CALL_FUNCTION_EX] = INSTRUMENTED_CALL_FUNCTION_EX,
    [YIELD_VALUE] = INSTRUMENTED_YIELD_VALUE,
    [INSTRUMENTED_YIELD_VALUE] = INSTRUMENTED_YIELD_VALUE,
    [RESUME] = INSTRUMENTED_RESUME,
    [INSTRUMENTED_RESUME] = INSTRUMENTED_RESUME,
    [JUMP_FORWARD] = INSTRUMENTED_JUMP_FORWARD,
    [INSTRUMENTED_JUMP_FORWARD] = INSTRUMENTED_JUMP_FORWARD,
    [JUMP_BACKWARD] = INSTRUMENTED_JUMP_BACKWARD,
    [INSTRUMENTED_JUMP_BACKWARD] = INSTRUMENTED_JUMP_BACKWARD,
    [JUMP_IF_FALSE_OR_POP] = INSTRUMENTED_JUMP_IF_FALSE_OR_POP,
    [INSTRUMENTED_JUMP_IF_FALSE_OR_POP] = INSTRUMENTED_JUMP_IF_FALSE_OR_POP,
    [JUMP_IF_TRUE_OR_POP] = INSTRUMENTED_JUMP_IF_TRUE_OR_POP,
    [INSTRUMENTED_JUMP_IF_TRUE_OR_POP] = INSTRUMENTED_JUMP_IF_TRUE_OR_POP,
    [POP_JUMP_IF_FALSE] = INSTRUMENTED_POP_JUMP_IF_FALSE,
    [INSTRUMENTED_POP_JUMP_IF_FALSE] = INSTRUMENTED_POP_JUMP_IF_FALSE,
    [POP_JUMP_IF_TRUE] = INSTRUMENTED_POP_JUMP_IF_TRUE,
    [INSTRUMENTED_POP_JUMP_IF_TRUE] = INSTRUMENTED_POP_JUMP_IF_TRUE,
    [POP_JUMP_IF_NONE] = INSTRUMENTED_POP_JUMP_IF_NONE,
    [INSTRUMENTED_POP_JUMP_IF_NONE] = INSTRUMENTED_POP_JUMP_IF_NONE,
    [POP_JUMP_IF_NOT_NONE] = INSTRUMENTED_POP_JUMP_IF_NOT_NONE,
    [INSTRUMENTED_POP_JUMP_IF_NOT_NONE] = INSTRUMENTED_POP_JUMP_IF_NOT_NONE,
    [COMPARE_AND_BRANCH] = INSTRUMENTED_COMPARE_AND_BRANCH,
    [INSTRUMENTED_COMPARE_AND_BRANCH] = INSTRUMENTED_COMPARE_AND_BRANCH,

    [INSTRUMENTED_LINE] = INSTRUMENTED_LINE,
    [INSTRUMENTED_INSTRUCTION] = INSTRUMENTED_INSTRUCTION,
};

static inline bool
is_instrumented(int opcode) {
    assert(opcode != 0);
    return INSTRUMENTED_OPCODES[opcode] == opcode;
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

static inline int
multiple_tools(_Py_MonitoringMatrix m)
{
    for (int i = 0; i < PY_MONITORING_EVENTS; i++) {
        if (_Py_popcount32(m.tools[i]) > 1) {
            return true;
        }
    }
    return false;
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

/* Line delta.
 * 8 bit value.
 * if line_delta == -128:
 *     line = None # represented as -1
 * elif line == -127:
 *     line = PyCode_Addr2Line(code, offset * sizeof(_Py_CODEUNIT));
 * else:
 *     line = first_line  + (offset >> OFFSET_SHIFT) + line_delta;
 */

#define OFFSET_SHIFT 4

static int8_t
compute_line_delta(PyCodeObject *code, int offset, int line)
{
    if (line < 0) {
        return -128;
    }
    // assert(line >= code->co_firstlineno);
    // assert(offset >= code->_co_firsttraceable);
    // assert(offset < Py_SIZE(code));
    int delta = line - code->co_firstlineno - (offset >> OFFSET_SHIFT);
    if (delta < 128 && delta > -128) {
        return delta;
    }
    return -127;
}

static int
compute_line(PyCodeObject *code, int offset, int8_t line_delta)
{
    if (line_delta > -127) {
        // assert((offset >> OFFSET_SHIFT) + line_delta >= 0);
        return code->co_firstlineno + (offset >> OFFSET_SHIFT) + line_delta;
    }
    if (line_delta == -128) {
        return -1;
    }
    else {
        assert(line_delta == -127);
        /* Compute from table */
        return PyCode_Addr2Line(code, offset * sizeof(_Py_CODEUNIT));
    }
}

/* We need to parse instructions from code objects a lot,
 * so add a couple of utilities */
int _Py_GetBaseOpcode(PyCodeObject *code, int offset)
{
    int opcode = _Py_OPCODE(_PyCode_CODE(code)[offset]);
    if (!is_instrumented(opcode)) {
        assert(_PyOpcode_Deopt[opcode] != 0);
        return _PyOpcode_Deopt[opcode];
    }
    if (opcode == INSTRUMENTED_INSTRUCTION) {
        opcode = code->_co_instrumentation.monitoring_data->per_instruction_opcodes[offset];
    }
    if (opcode == INSTRUMENTED_LINE) {
        opcode = code->_co_instrumentation.monitoring_data->lines[offset].original_opcode;
    }
    int deinstrumented = DE_INSTRUMENT[opcode];
    if (deinstrumented) {
        return deinstrumented;
    }
    assert(_PyOpcode_Deopt[opcode] != 0);
    return _PyOpcode_Deopt[opcode];
}

uint8_t deinstrument(PyCodeObject *code, int offset)
{
    int opcode = _Py_OPCODE(_PyCode_CODE(code)[offset]);
    if (!is_instrumented(opcode)) {
        assert(_PyOpcode_Deopt[opcode] != 0);
        return _PyOpcode_Deopt[opcode];
    }
    if (opcode == INSTRUMENTED_INSTRUCTION) {
        opcode = code->_co_instrumentation.monitoring_data->per_instruction_opcodes[offset];
    }
    if (opcode == INSTRUMENTED_LINE) {
        opcode = code->_co_instrumentation.monitoring_data->lines[offset].original_opcode;
    }
    int deinstrumented = DE_INSTRUMENT[opcode];
    if (deinstrumented) {
        return deinstrumented;
    }
    return opcode;
}

typedef struct _Instruction {
    uint8_t extended_args;
    uint8_t deinstrumented_opcode;
    uint8_t actual_opcode;
    uint8_t length;
    /* Whether a prefix instrument like INSTRUMENTED_INSTRUCTION is attached */
    bool is_prefix_instrumented;
    /* Whether a specialized instruction like INSTRUMENTED_RETURN_VALUE is used */
    bool is_specialized_instrumented;
} Instruction;

Instruction read_instruction(PyCodeObject *code, int offset)
{
    Instruction result = (Instruction){0};
    int opcode = result.actual_opcode = _PyCode_CODE(code)[offset].opcode;
    if (opcode == INSTRUMENTED_INSTRUCTION) {
        opcode = code->_co_instrumentation.monitoring_data->per_instruction_opcodes[offset];
        result.is_prefix_instrumented = true;
    }

    if (opcode == INSTRUMENTED_LINE) {
        opcode = code->_co_instrumentation.monitoring_data->lines[offset].original_opcode;
        result.is_prefix_instrumented = true;
    }
    while (opcode == EXTENDED_ARG) {
        result.extended_args += 1;
        offset++;
        opcode = _PyCode_CODE(code)[offset].opcode;
    }
    result.length = result.extended_args + 1;
    int deinstrumented = DE_INSTRUMENT[opcode];
    if (deinstrumented) {
        result.deinstrumented_opcode = deinstrumented;
        result.is_specialized_instrumented = true;
    }
    else {
        result.deinstrumented_opcode = opcode;
    }
    int base = _PyOpcode_Deopt[result.deinstrumented_opcode];
    /* TO DO -- Use instruction length, not cache count table */
    result.length += _PyOpcode_Caches[base];
    if (base == COMPARE_AND_BRANCH) {
        /* Skip over following POP_JUMP_IF */
        result.length += 1;
    }
    return result;
}

#ifdef INSTRUMENT_DEBUG

static void
dump_instrumentation_data_tools(PyCodeObject *code, uint8_t *tools, int i, FILE*out)
{
    if (tools == NULL) {
        fprintf(out, "tools = NULL");
    }
    else {
        fprintf(out, "tools = %d", tools[i]);
    }
}

static void
dump_instrumentation_data_lines(PyCodeObject *code, _PyCoLineInstrumentationData *lines, int i, FILE*out)
{
    if (lines == NULL) {
        fprintf(out, ", lines = NULL");
    }
    else if (lines[i].original_opcode == 0) {
        fprintf(out, ", lines = {original_opcode = No LINE (0), line_delta = %d)", lines[i].line_delta);
    }
    else if (lines[i].original_opcode == 255) {
        fprintf(out, ", lines = {original_opcode = LINE_MARKER, line_delta = %d)", lines[i].line_delta);
    }
    else {
        fprintf(out, ", lines = {original_opcode = %s, line_delta = %d)", _PyOpcode_OpName[lines[i].original_opcode], lines[i].line_delta);
    }
}

static void
dump_instrumentation_data_line_tools(PyCodeObject *code, uint8_t *line_tools, int i, FILE*out)
{
    if (line_tools == NULL) {
        fprintf(out, ", line_tools = NULL");
    }
    else {
        fprintf(out, ", line_tools = %d", line_tools[i]);
    }
}

static void
dump_instrumentation_data_per_instruction(PyCodeObject *code, _PyCoInstrumentationData *data, int i, FILE*out)
{
    if (data->per_instruction_opcodes == NULL) {
        fprintf(out, ", per-inst opcode = NULL");
    }
    else {
        fprintf(out, ", per-inst opcode = %s", _PyOpcode_OpName[data->per_instruction_opcodes[i]]);
    }
    if (data->per_instruction_tools == NULL) {
        fprintf(out, ", per-inst tools = NULL");
    }
    else {
        fprintf(out, ", per-inst tools = %d", data->per_instruction_tools[i]);
    }
}



static void
dump_matrix(const char *prefix, _Py_MonitoringMatrix matrix, FILE*out)
{
    fprintf(out, "%s matrix:\n", prefix);
    for (int event = 0; event < PY_MONITORING_EVENTS; event++) {
        fprintf(out, "    Event %d: Tools %x\n", event, matrix.tools[event]);
    }
}

/* Like _Py_GetBaseOpcode but without asserts.
 * Does its best to give the right answer, but won't abort
 * if something is wrong */
int get_base_opcode_best_attempt(PyCodeObject *code, int offset)
{
    int opcode = _Py_OPCODE(_PyCode_CODE(code)[offset]);
    if (INSTRUMENTED_OPCODES[opcode] != opcode) {
        /* Not instrumented */
        return _PyOpcode_Deopt[opcode] == 0 ? opcode : _PyOpcode_Deopt[opcode];
    }
    if (opcode == INSTRUMENTED_INSTRUCTION) {
        if (code->_co_instrumentation.monitoring_data->per_instruction_opcodes[offset] == 0) {
            return opcode;
        }
        opcode = code->_co_instrumentation.monitoring_data->per_instruction_opcodes[offset];
    }
    if (opcode == INSTRUMENTED_LINE) {
        if (code->_co_instrumentation.monitoring_data->lines[offset].original_opcode == 0) {
            return opcode;
        }
        opcode = code->_co_instrumentation.monitoring_data->lines[offset].original_opcode;
    }
    int deinstrumented = DE_INSTRUMENT[opcode];
    if (deinstrumented) {
        return deinstrumented;
    }
    if (_PyOpcode_Deopt[opcode] == 0) {
        return opcode;
    }
    return _PyOpcode_Deopt[opcode];
}

/* No error checking -- Don't use this for anything but experimental debugging */
static void
dump_instrumentation_data(PyCodeObject *code, int star, FILE*out)
{
    _PyCoInstrumentationData *data = code->_co_instrumentation.monitoring_data;
    fprintf(out, "\n");
    if (data == NULL) {
        fprintf(out, "NULL\n");
        return;
    }
    dump_matrix("Global", PyInterpreterState_Get()->monitoring_matrix, out);
    dump_matrix("Code", data->matrix, out);
    int code_len = (int)Py_SIZE(code);
    bool starred = false;
    for (int i = 0; i < code_len; i++) {
        _Py_CODEUNIT *instr = &_PyCode_CODE(code)[i];
        int opcode = instr->opcode;
        if (i == star) {
            fprintf(out, "**  ");
            starred = true;
        }
        fprintf(out, "Offset: %d, line: %d %s: ", i, PyCode_Addr2Line(code, i*2), _PyOpcode_OpName[opcode]);
        dump_instrumentation_data_tools(code, data->tools, i, out);
        dump_instrumentation_data_lines(code, data->lines, i, out);
        dump_instrumentation_data_line_tools(code, data->line_tools, i, out);
        dump_instrumentation_data_per_instruction(code, data, i, out);
        /* TO DO -- per instruction data */
        fprintf(out, "\n");
        /* TO DO -- Use instruction length, not cache count */
        int base_opcode = get_base_opcode_best_attempt(code, i);
        i += _PyOpcode_Caches[base_opcode];
    }
    if (!starred && star >= 0) {
        fprintf(out, "Error offset not at valid instruction offset: %d\n", star);
        fprintf(out, "    ");
        dump_instrumentation_data_tools(code, data->tools, star, out);
        dump_instrumentation_data_lines(code, data->lines, star, out);
        dump_instrumentation_data_line_tools(code, data->line_tools, star, out);
        dump_instrumentation_data_per_instruction(code, data, star, out);
        fprintf(out, "\n");
    }
}

#define CHECK(test) do { \
    if (!(test)) { \
        dump_instrumentation_data(code, i, stderr); \
    } \
    assert(test); \
} while (0)

bool valid_opcode(int opcode) {
    if (opcode > 0 && opcode < 255 &&
        _PyOpcode_OpName[opcode] &&
        _PyOpcode_OpName[opcode][0] != '<'
    ) {
       return true;
    }
    return false;
}

static void
sanity_check_instrumentation(PyCodeObject *code)
{
    _PyCoInstrumentationData *data = code->_co_instrumentation.monitoring_data;
    if (data == NULL) {
        return;
    }
    _Py_MonitoringMatrix tools_matrix = PyInterpreterState_Get()->monitoring_matrix;
    assert(matrix_equals(
        code->_co_instrumentation.monitoring_data->matrix,
        tools_matrix)
    );
    int code_len = (int)Py_SIZE(code);
    for (int i = 0; i < code_len;) {
        Instruction inst = read_instruction(code, i);
        int opcode = inst.actual_opcode;
        if (opcode == INSTRUMENTED_INSTRUCTION) {
            opcode = data->per_instruction_opcodes[i];
            if (!is_instrumented(opcode)) {
                CHECK(_PyOpcode_Deopt[opcode] == opcode);
            }
            /* TO DO -- check tools */
        }
        if (opcode == INSTRUMENTED_LINE) {
            CHECK(data->lines);
            CHECK(valid_opcode(data->lines[i].original_opcode));
            int opcode = data->lines[i].original_opcode;
            CHECK(opcode != END_FOR);
            CHECK(opcode != RESUME);
            CHECK(opcode != INSTRUMENTED_RESUME);
            if (!is_instrumented(opcode)) {
                CHECK(_PyOpcode_Deopt[opcode] == opcode);
            }
            CHECK(opcode != INSTRUMENTED_LINE);
        }
        else if (data->lines) {
            CHECK(data->lines[i].original_opcode == 0 ||
                  data->lines[i].original_opcode == 255);
        }
        if (inst.is_specialized_instrumented) {
            int event = EVENT_FOR_OPCODE[inst.deinstrumented_opcode];
            if (event < 0) {
                /* RESUME fixup */
                event = _PyCode_CODE(code)[i + inst.extended_args].oparg;
            }
            CHECK(tools_matrix.tools[event] != 0);
            CHECK(inst.deinstrumented_opcode != END_FOR);
        }
        if (_PyOpcode_Deopt[inst.deinstrumented_opcode] == COMPARE_AND_BRANCH) {
            CHECK(_PyCode_CODE(code)[i+2].opcode == POP_JUMP_IF_FALSE ||
                  _PyCode_CODE(code)[i+2].opcode == POP_JUMP_IF_TRUE);
        }
        if (data->lines && inst.deinstrumented_opcode != END_FOR) {
            int line1 = compute_line(code, i, data->lines[i].line_delta);
            int line2 = PyCode_Addr2Line(code, i*sizeof(_Py_CODEUNIT));
            CHECK(line1 == line2);
        }
        CHECK(valid_opcode(inst.deinstrumented_opcode));
        if (inst.is_prefix_instrumented) {
            CHECK(inst.extended_args || _PyOpcode_Deopt[inst.deinstrumented_opcode] == inst.deinstrumented_opcode);
        }
        if (inst.is_specialized_instrumented) {
            CHECK(_PyOpcode_Deopt[inst.deinstrumented_opcode] == inst.deinstrumented_opcode);
        }
        if (data->tools) {
            uint8_t local_tools = data->tools[i + inst.extended_args];
            int deopt = _PyOpcode_Deopt[inst.deinstrumented_opcode];
            if (OPCODE_HAS_EVENT[deopt]) {
                int event = EVENT_FOR_OPCODE[deopt];
                if (event == -1) {
                    /* RESUME fixup */
                    event = _PyCode_CODE(code)[i + inst.extended_args].oparg;
                }
                uint8_t global_tools = tools_matrix.tools[event];
                CHECK((global_tools & local_tools) == local_tools);
            }
            else {
                CHECK(local_tools == 0xff);
            }
        }
        i += inst.length;
        assert(i <= code_len);
    }
}
#else

#define CHECK(test) assert(test)

#endif

static void
de_instrument(PyCodeObject *code, int offset, int event)
{
    assert(event != PY_MONITORING_EVENT_INSTRUCTION);
    assert(event != PY_MONITORING_EVENT_LINE);
    _Py_CODEUNIT *instr = &_PyCode_CODE(code)[offset];
    int opcode = _Py_OPCODE(*instr);
    if (!is_instrumented(opcode)) {
        return;
    }
    switch(opcode) {
        case INSTRUMENTED_INSTRUCTION:
            opcode = code->_co_instrumentation.monitoring_data->per_instruction_opcodes[offset];
            if (opcode != INSTRUMENTED_LINE) {
                int deinstrumented = DE_INSTRUMENT[opcode];
                if (deinstrumented) {
                    code->_co_instrumentation.monitoring_data->per_instruction_opcodes[offset] = deinstrumented;
                }
                break;
            }
            /* Intentional fall through */
        case INSTRUMENTED_LINE:
        {
            _PyCoLineInstrumentationData *lines = &code->_co_instrumentation.monitoring_data->lines[offset];
            int orignal_opcode = lines->original_opcode;
            int deinstrumented = DE_INSTRUMENT[orignal_opcode];
            if (deinstrumented) {
                lines->original_opcode = deinstrumented;
            }
            break;
        }
        default:
        {
            int deinstrumented = DE_INSTRUMENT[opcode];
            assert(deinstrumented);
            instr->opcode = deinstrumented;
        }
    }
    int base_opcode = _Py_GetBaseOpcode(code, offset);
    assert(base_opcode != 0);
    assert(_PyOpcode_Deopt[base_opcode] == base_opcode);
    if (_PyOpcode_Caches[base_opcode]) {
        instr[1].cache = adaptive_counter_warmup();
    }
}

static void
de_instrument_line(PyCodeObject *code, int i)
{
    /* TO DO -- handle INSTRUMENTED_INSTRUCTION */
    _Py_CODEUNIT *instr = &_PyCode_CODE(code)[i];
    int opcode = _Py_OPCODE(*instr);
    if (opcode == INSTRUMENTED_INSTRUCTION) {
        opcode = code->_co_instrumentation.monitoring_data->per_instruction_opcodes[i];
    }
    if (opcode != INSTRUMENTED_LINE) {
        return;
    }
    _PyCoLineInstrumentationData *lines = &code->_co_instrumentation.monitoring_data->lines[i];
    int original_opcode = lines->original_opcode;
    assert(is_instrumented(original_opcode) || _PyOpcode_Deopt[original_opcode]);
    if (is_instrumented(original_opcode)) {
        /* Instrumented original */
        instr->opcode = original_opcode;
    }
    else {
        original_opcode = _PyOpcode_Deopt[original_opcode];
        assert(original_opcode != 0);
        if (_PyOpcode_Caches[original_opcode]) {
            instr[1].cache = adaptive_counter_warmup();
        }
    }
    if (instr->opcode == INSTRUMENTED_INSTRUCTION) {
        code->_co_instrumentation.monitoring_data->per_instruction_opcodes[i] = original_opcode;
    }
    else {
        instr->opcode = original_opcode;
    }
    assert(instr->opcode != INSTRUMENTED_LINE);
    /* Mark instruction as candidate for line instrumentation */
    lines->original_opcode = 255;
}


static void
de_instrument_per_instruction(PyCodeObject *code, int offset)
{
    /* TO DO -- handle INSTRUMENTED_INSTRUCTION */
    _Py_CODEUNIT *instr = &_PyCode_CODE(code)[offset];
    int opcode = _Py_OPCODE(*instr);
    if (opcode != INSTRUMENTED_INSTRUCTION) {
        return;
    }
    int original_opcode = code->_co_instrumentation.monitoring_data->per_instruction_opcodes[offset];
    assert(is_instrumented(original_opcode) || _PyOpcode_Deopt[original_opcode]);
    if (is_instrumented(original_opcode)) {
        /* Instrumented original */
        instr->opcode = original_opcode;
    }
    else {
        int base_opcode = _PyOpcode_Deopt[original_opcode];
        assert(base_opcode != 0);
        instr->opcode = base_opcode;
        if (_PyOpcode_Caches[base_opcode]) {
            instr[1].cache = adaptive_counter_warmup();
        }
    }
    assert(instr->opcode != INSTRUMENTED_INSTRUCTION);
    /* Keep things clean for snaity check */
    code->_co_instrumentation.monitoring_data->per_instruction_opcodes[offset] = 0;
}


static void
instrument(PyCodeObject *code, int offset)
{
    /* TO DO -- handle INSTRUMENTED_INSTRUCTION */
    _Py_CODEUNIT *instr = &_PyCode_CODE(code)[offset];
    int opcode = _Py_OPCODE(*instr);
    if (opcode == INSTRUMENTED_LINE) {
        _PyCoLineInstrumentationData *lines = &code->_co_instrumentation.monitoring_data->lines[offset];
        opcode = lines->original_opcode;
        assert(!is_instrumented(opcode));
        lines->original_opcode = INSTRUMENTED_OPCODES[opcode];
    }
    else if (!is_instrumented(opcode)) {
        opcode = _PyOpcode_Deopt[opcode];
        int instrumented = INSTRUMENTED_OPCODES[opcode];
        assert(instrumented);
        instr->opcode = instrumented;
        /* TO DO -- Use instruction length, not cache count */
        if (_PyOpcode_Caches[opcode]) {
            instr[1].cache = adaptive_counter_warmup();
        }
    }
}

static void
instrument_line(PyCodeObject *code, int i)
{
    _Py_CODEUNIT *instr = &_PyCode_CODE(code)[i];
    int opcode = _Py_OPCODE(*instr);
    /* TO DO -- handle INSTRUMENTED_INSTRUCTION */
    if (opcode == INSTRUMENTED_LINE) {
        return;
    }
    _PyCoLineInstrumentationData *lines = &code->_co_instrumentation.monitoring_data->lines[i];
    CHECK(lines->original_opcode == 255);
    if (is_instrumented(opcode)) {
        lines->original_opcode = opcode;
    }
    else {
        assert(opcode != 0);
        assert(_PyOpcode_Deopt[opcode] != 0);
        assert(_PyOpcode_Deopt[opcode] != RESUME);
        lines->original_opcode = _PyOpcode_Deopt[opcode];

    }
    assert(lines->original_opcode > 0);
    instr->opcode = INSTRUMENTED_LINE;
    CHECK(code->_co_instrumentation.monitoring_data->lines[i].original_opcode != 0);
}

static void
instrument_per_instruction(PyCodeObject *code, int i)
{
    _Py_CODEUNIT *instr = &_PyCode_CODE(code)[i];
    int opcode = instr->opcode;
    if (opcode == INSTRUMENTED_INSTRUCTION) {
        return;
    }
    CHECK(opcode != 0);
    if (is_instrumented(opcode)) {
        code->_co_instrumentation.monitoring_data->per_instruction_opcodes[i] = opcode;
    }
    else {
        assert(opcode != 0);
        assert(_PyOpcode_Deopt[opcode] != 0);
        assert(_PyOpcode_Deopt[opcode] != RESUME);
        code->_co_instrumentation.monitoring_data->per_instruction_opcodes[i] = _PyOpcode_Deopt[opcode];
    }
    assert(code->_co_instrumentation.monitoring_data->per_instruction_opcodes[i] > 0);
    instr->opcode = INSTRUMENTED_INSTRUCTION;
}

#ifndef NDEBUG
static bool
instruction_has_event(PyCodeObject *code, int offset)
{
    _Py_CODEUNIT instr = _PyCode_CODE(code)[offset];
    int opcode = instr.opcode;
    if (opcode == INSTRUMENTED_INSTRUCTION) {
        opcode = code->_co_instrumentation.monitoring_data->per_instruction_opcodes[offset];
    }
    if (opcode == INSTRUMENTED_LINE) {
        opcode = code->_co_instrumentation.monitoring_data->lines[offset].original_opcode;
    }
    assert(DE_INSTRUMENT[opcode] == 0 || OPCODE_HAS_EVENT[DE_INSTRUMENT[opcode]] == OPCODE_HAS_EVENT[opcode]);
    return OPCODE_HAS_EVENT[opcode];
}
#endif

static void
remove_tools(PyCodeObject * code, int offset, int event, int tools)
{
    assert(event != PY_MONITORING_EVENT_LINE);
    assert(event != PY_MONITORING_EVENT_INSTRUCTION);
    assert(instruction_has_event(code, offset));
    _PyCoInstrumentationData *monitoring = code->_co_instrumentation.monitoring_data;
    if (monitoring && monitoring->tools) {
        monitoring->tools[offset] &= ~tools;
        if (monitoring->tools[offset] == 0 && event < PY_MONITORING_INSTRUMENTED_EVENTS) {
            de_instrument(code, offset, event);
        }
    }
    else {
        /* Single tool */
        uint8_t single_tool = code->_co_instrumentation.monitoring_data->matrix.tools[event];
        assert(_Py_popcount32(single_tool) <= 1);
        if (((single_tool & tools) == single_tool) && event < PY_MONITORING_INSTRUMENTED_EVENTS) {
            de_instrument(code, offset, event);
        }
    }
}

static void
remove_line_tools(PyCodeObject * code, int offset, int tools)
{
    assert(code->_co_instrumentation.monitoring_data);
    if (code->_co_instrumentation.monitoring_data->line_tools)
    {
        uint8_t *toolsptr = &code->_co_instrumentation.monitoring_data->line_tools[offset];
        *toolsptr &= ~tools;
        if (*toolsptr == 0 ) {
            de_instrument_line(code, offset);
        }
    }
    else {
        /* Single tool */
        uint8_t single_tool = code->_co_instrumentation.monitoring_data->matrix.tools[PY_MONITORING_EVENT_LINE];
        assert(_Py_popcount32(single_tool) <= 1);
        if (((single_tool & tools) == single_tool)) {
            de_instrument_line(code, offset);
        }
    }
}


static void
add_tools(PyCodeObject * code, int offset, int event, int tools)
{
    assert(event != PY_MONITORING_EVENT_LINE);
    assert(event != PY_MONITORING_EVENT_INSTRUCTION);
    assert(code->_co_instrumentation.monitoring_data);
    if (code->_co_instrumentation.monitoring_data &&
        code->_co_instrumentation.monitoring_data->tools
    ) {
        code->_co_instrumentation.monitoring_data->tools[offset] |= tools;
    }
    else {
        /* Single tool */
        assert(PyInterpreterState_Get()->monitoring_matrix.tools[event] == tools);
        assert(_Py_popcount32(tools) == 1);
    }
    if (event < PY_MONITORING_INSTRUMENTED_EVENTS) {
        instrument(code, offset);
    }
}

static void
add_line_tools(PyCodeObject * code, int offset, int tools)
{
    assert((PyInterpreterState_Get()->monitoring_matrix.tools[PY_MONITORING_EVENT_LINE] & tools) == tools);
    assert(code->_co_instrumentation.monitoring_data);
    if (code->_co_instrumentation.monitoring_data->line_tools
    ) {
        code->_co_instrumentation.monitoring_data->line_tools[offset] |= tools;
    }
    else {
        /* Single tool */
        assert(_Py_popcount32(tools) == 1);
    }
    instrument_line(code, offset);
}


static void
add_per_instruction_tools(PyCodeObject * code, int offset, int tools)
{
    assert((PyInterpreterState_Get()->monitoring_matrix.tools[PY_MONITORING_EVENT_INSTRUCTION] & tools) == tools);
    assert(code->_co_instrumentation.monitoring_data);
    if (code->_co_instrumentation.monitoring_data->per_instruction_tools
    ) {
        code->_co_instrumentation.monitoring_data->per_instruction_tools[offset] |= tools;
    }
    else {
        /* Single tool */
        assert(_Py_popcount32(tools) == 1);
    }
    instrument_per_instruction(code, offset);
}


static void
remove_per_instruction_tools(PyCodeObject * code, int offset, int tools)
{
    assert(code->_co_instrumentation.monitoring_data);
    if (code->_co_instrumentation.monitoring_data->per_instruction_tools)
    {
        uint8_t *toolsptr = &code->_co_instrumentation.monitoring_data->per_instruction_tools[offset];
        *toolsptr &= ~tools;
        if (*toolsptr == 0 ) {
            de_instrument_per_instruction(code, offset);
        }
    }
    else {
        /* Single tool */
        uint8_t single_tool = code->_co_instrumentation.monitoring_data->matrix.tools[PY_MONITORING_EVENT_INSTRUCTION];
        assert(_Py_popcount32(single_tool) <= 1);
        if (((single_tool & tools) == single_tool)) {
            de_instrument_per_instruction(code, offset);
        }
    }
}


/* Return 1 if DISABLE returned, -1 if error, 0 otherwise */
static int
call_one_instrument(
    PyInterpreterState *interp, PyThreadState *tstate, PyObject **args,
    Py_ssize_t nargsf, int8_t tool, int event)
{
    assert(0 <= tool && tool < 8);
    assert(tstate->tracing == 0);
    PyObject *instrument = interp->tools[tool].instrument_callables[event];
    if (instrument == NULL) {
       return 0;
    }
    int old_what = tstate->what_event;
    tstate->what_event = event;
    tstate->tracing++;
    PyObject *res = PyObject_Vectorcall(instrument, args, nargsf, NULL);
    tstate->tracing--;
    tstate->what_event = old_what;
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

static inline uint8_t
get_tools_for_instruction(PyCodeObject * code, int i, int event)
{
    uint8_t tools;
    assert(event != PY_MONITORING_EVENT_LINE);
    assert(event != PY_MONITORING_EVENT_INSTRUCTION);
    _PyCoInstrumentationData *monitoring = code->_co_instrumentation.monitoring_data;
    if (event < PY_MONITORING_INSTRUMENTED_EVENTS && monitoring && monitoring->tools) {
        tools = monitoring->tools[i];
    }
    else {
        tools = code->_co_instrumentation.monitoring_data->matrix.tools[event];
    }
    CHECK((tools & PyInterpreterState_Get()->monitoring_matrix.tools[event]) == tools);
    CHECK((tools & code->_co_instrumentation.monitoring_data->matrix.tools[event]) == tools);
    return tools;
}

static int
call_instrument(PyThreadState *tstate, PyCodeObject *code, int event,
    PyObject **args, Py_ssize_t nargsf, int offset, uint8_t tools)
{
    //sanity_check_instrumentation(code);
    if (tstate->tracing) {
        return 0;
    }
    PyInterpreterState *interp = tstate->interp;
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

static bool
is_instrumentation_up_to_date(PyCodeObject *code, PyInterpreterState *interp)
{
    return interp->monitoring_version == code->_co_instrumentation.monitoring_version;
}

int
_Py_call_instrumentation(
    PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _Py_CODEUNIT *instr)
{
    PyCodeObject *code = frame->f_code;
    assert(is_instrumentation_up_to_date(code, tstate->interp));
    assert(matrix_equals(code->_co_instrumentation.monitoring_data->matrix, tstate->interp->monitoring_matrix));
    int offset = instr - _PyCode_CODE(code);
    PyObject *offset_obj = PyLong_FromSsize_t(offset);
    if (offset_obj == NULL) {
        return -1;
    }
    uint8_t tools = get_tools_for_instruction(code, offset, event);
    PyObject *args[3] = { NULL, (PyObject *)code, offset_obj };
    int err = call_instrument(tstate, code, event, &args[1], 2 | PY_VECTORCALL_ARGUMENTS_OFFSET, offset, tools);
    Py_DECREF(offset_obj);
    return err;
}

int
_Py_call_instrumentation_arg(
    PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _Py_CODEUNIT *instr, PyObject *arg)
{
    PyCodeObject *code = frame->f_code;
    assert(is_instrumentation_up_to_date(code, tstate->interp));
    assert(matrix_equals(code->_co_instrumentation.monitoring_data->matrix, tstate->interp->monitoring_matrix));
    int offset = instr - _PyCode_CODE(code);
    PyObject *offset_obj = PyLong_FromSsize_t(offset);
    if (offset_obj == NULL) {
        return -1;
    }
    uint8_t tools = get_tools_for_instruction(code, offset, event);
    PyObject *args[4] = { NULL, (PyObject *)code, offset_obj, arg };
    int err = call_instrument(tstate, code, event, &args[1], 3 | PY_VECTORCALL_ARGUMENTS_OFFSET, offset, tools);
    Py_DECREF(offset_obj);
    return err;
}

int
_Py_call_instrumentation_jump(
    PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _Py_CODEUNIT *instr, _Py_CODEUNIT *target
) {
    assert(event == PY_MONITORING_EVENT_JUMP ||
           event == PY_MONITORING_EVENT_BRANCH);
    assert(frame->prev_instr == instr);
    frame->prev_instr = target;
    PyCodeObject *code = frame->f_code;
    int offset = instr - _PyCode_CODE(code);
    int to = target - _PyCode_CODE(code);
    PyObject *to_obj = PyLong_FromLong(to);
    if (to_obj == NULL) {
        return -1;
    }
    PyObject *from_obj = PyLong_FromLong(offset);
    if (from_obj == NULL) {
        Py_DECREF(to_obj);
        return -1;
    }
    uint8_t tools = get_tools_for_instruction(code, offset, event);
    PyObject *args[4] = { NULL, (PyObject *)code, from_obj, to_obj };
    int err = call_instrument(tstate, code, event, &args[1], 3 | PY_VECTORCALL_ARGUMENTS_OFFSET, offset, tools);
    Py_DECREF(to_obj);
    Py_DECREF(from_obj);
    if (err) {
        /* Error handling expects next_instr to point to instruction + 1.
         * So we add one here. */
        frame->prev_instr++;
    }
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
    assert(matrix_equals(code->_co_instrumentation.monitoring_data->matrix, tstate->interp->monitoring_matrix));
    int offset = instr - _PyCode_CODE(code);
    PyObject *offset_obj = PyLong_FromSsize_t(offset);
    int err;
    if (offset_obj == NULL) {
        err = -1;
    }
    else {
        PyObject *args[4] = { NULL, (PyObject *)code, offset_obj, arg };
        Py_ssize_t nargsf = (2 | PY_VECTORCALL_ARGUMENTS_OFFSET) + (arg != NULL);
        uint8_t tools = code->_co_instrumentation.monitoring_data->matrix.tools[event];
        err = call_instrument(tstate, code, event, &args[1], nargsf, offset, tools);
        Py_DECREF(offset_obj);
    }
    if (err) {
        Py_XDECREF(type);
        Py_XDECREF(value);
        Py_XDECREF(traceback);
    }
    else {
        _PyErr_Restore(tstate, type, value, traceback);
    }
    assert(_PyErr_Occurred(tstate));
}

int
_Py_Instrumentation_GetLine(PyCodeObject *code, int index)
{
    _PyCoInstrumentation *instrumentation = &code->_co_instrumentation;
    assert(instrumentation->monitoring_data != NULL);
    assert(instrumentation->monitoring_data->lines != NULL);
    assert(index >= code->_co_firsttraceable);
    assert(index < Py_SIZE(code));
    _PyCoLineInstrumentationData *line_data = &instrumentation->monitoring_data->lines[index];
    int8_t line_delta = line_data->line_delta;
    int line = compute_line(code, index, line_delta);
    return line;
}

int
_Py_call_instrumentation_line(PyThreadState *tstate, _PyInterpreterFrame* frame, _Py_CODEUNIT *instr)
{
    frame->prev_instr = instr;
    PyCodeObject *code = frame->f_code;
    assert(is_instrumentation_up_to_date(code, tstate->interp));
    assert(matrix_equals(code->_co_instrumentation.monitoring_data->matrix, tstate->interp->monitoring_matrix));
    int i = instr - _PyCode_CODE(code);
    _PyCoInstrumentation *instrumentation = &code->_co_instrumentation;
    _PyCoLineInstrumentationData *line_data = &instrumentation->monitoring_data->lines[i];
    uint8_t original_opcode = line_data->original_opcode;
    if (tstate->tracing) {
        goto done;
    }
    PyInterpreterState *interp = tstate->interp;
    int8_t line_delta = line_data->line_delta;
    int line = compute_line(code, i, line_delta);
    uint8_t tools = code->_co_instrumentation.monitoring_data->line_tools != NULL ?
        code->_co_instrumentation.monitoring_data->line_tools[i] :
        interp->monitoring_matrix.tools[PY_MONITORING_EVENT_LINE];
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
        int res = call_one_instrument(interp, tstate, &args[1],
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
            remove_line_tools(code, i, 1 << tool);
        }
    }
    Py_DECREF(line_obj);
done:
    assert(original_opcode != 0);
    assert(original_opcode < INSTRUMENTED_LINE);
    assert(_PyOpcode_Deopt[original_opcode] == original_opcode);
    return original_opcode;
}

int
_Py_call_instrumentation_instruction(PyThreadState *tstate, _PyInterpreterFrame* frame, _Py_CODEUNIT *instr)
{
    PyCodeObject *code = frame->f_code;
    assert(is_instrumentation_up_to_date(code, tstate->interp));
    assert(matrix_equals(code->_co_instrumentation.monitoring_data->matrix, tstate->interp->monitoring_matrix));
    int offset = instr - _PyCode_CODE(code);
    _PyCoInstrumentationData *instrumentation_data = code->_co_instrumentation.monitoring_data;
    assert(instrumentation_data->per_instruction_opcodes);
    int next_opcode = instrumentation_data->per_instruction_opcodes[offset];
    if (tstate->tracing) {
        return next_opcode;
    }
    PyInterpreterState *interp = tstate->interp;
    uint8_t tools = instrumentation_data->per_instruction_tools != NULL ?
        instrumentation_data->per_instruction_tools[offset] :
        interp->monitoring_matrix.tools[PY_MONITORING_EVENT_INSTRUCTION];

    PyObject *offset_obj = PyLong_FromSsize_t(offset);
    if (offset_obj == NULL) {
        return -1;
    }
    PyObject *args[3] = { NULL, (PyObject *)code, offset_obj };
    while (tools) {
        int tool = most_significant_bit(tools);
        assert(tool < 8);
        assert(tools & (1 << tool));
        tools &= ~(1 << tool);
        int res = call_one_instrument(interp, tstate, &args[1],
                                      2 | PY_VECTORCALL_ARGUMENTS_OFFSET,
                                      tool, PY_MONITORING_EVENT_INSTRUCTION);
        if (res == 0) {
            /* Nothing to do */
        }
        else if (res < 0) {
            /* error */
            Py_DECREF(offset_obj);
            return -1;
        }
        else {
            /* DISABLE  */
            remove_per_instruction_tools(code, offset, 1 << tool);
        }
    }
    Py_DECREF(offset_obj);
    assert(next_opcode != 0);
    return next_opcode;
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

static void
initialize_tools(PyCodeObject *code)
{
    uint8_t* tools = code->_co_instrumentation.monitoring_data->tools;
    assert(tools != NULL);
    int code_len = (int)Py_SIZE(code);
    for (int i = 0; i < code_len; i++) {
        _Py_CODEUNIT *instr = &_PyCode_CODE(code)[i];
        int opcode = instr->opcode;
        if (opcode == INSTRUMENTED_LINE) {
            opcode = code->_co_instrumentation.monitoring_data->lines[i].original_opcode;
        }
        bool instrumented = is_instrumented(opcode);
        if (instrumented) {
            opcode = DE_INSTRUMENT[opcode];
            assert(opcode != 0);
        }
        opcode = _PyOpcode_Deopt[opcode];
        if (OPCODE_HAS_EVENT[opcode]) {
            if (instrumented) {
                int8_t event;
                if (opcode == RESUME) {
                    event = instr->oparg != 0;
                }
                else {
                    event = EVENT_FOR_OPCODE[opcode];
                    assert(event > 0);
                }
                assert(event >= 0);
                tools[i] = code->_co_instrumentation.monitoring_data->matrix.tools[event];
                CHECK(tools[i] != 0);
            }
            else {
                tools[i] = 0;
            }
        }
#ifdef Py_DEBUG
        /* Initialize tools for invalid locations to all ones to try to catch errors */
        else {
            tools[i] = 0xff;
        }
        for (int j = 1; j <= _PyOpcode_Caches[opcode]; j++) {
            tools[i+j] = 0xff;
        }
#endif
        i += _PyOpcode_Caches[opcode];
    }
}

#define LINE_MARKER 255

#define NO_LINE -128

static void
initialize_lines(PyCodeObject *code)
{
    _PyCoLineInstrumentationData *line_data = code->_co_instrumentation.monitoring_data->lines;
    assert(line_data != NULL);
    int code_len = (int)Py_SIZE(code);
    PyCodeAddressRange range;
    _PyCode_InitAddressRange(code, &range);
    for (int i = 0; i < code->_co_firsttraceable && i < code_len; i++) {
        line_data[i].original_opcode = 0;
        line_data[i].line_delta = -127;
    }
    int current_line = -1;
    for (int i = code->_co_firsttraceable; i < code_len; ) {
        Instruction inst = read_instruction(code, i);
        int line = _PyCode_CheckLineNumber(i*(int)sizeof(_Py_CODEUNIT), &range);
        line_data[i].line_delta = compute_line_delta(code, i, line);
        switch (inst.deinstrumented_opcode) {
            case END_ASYNC_FOR:
            case END_FOR:
            case RESUME:
                /* END_FOR cannot start a line, as it is skipped by FOR_ITER
                * RESUME must not be instrumented with INSTRUMENT_LINE */
                line_data[i].original_opcode = 0;
                break;
            default:
                if (line != current_line && line >= 0) {
                    line_data[i].original_opcode = LINE_MARKER;
                }
                else {
                    line_data[i].original_opcode = 0;
                }
                if (line >= 0) {
                    current_line = line;
                }
        }
        for (int j = 1; j < inst.length; j++) {
            line_data[i+j].original_opcode = 0;
            line_data[i+j].line_delta = NO_LINE;
        }
        switch (inst.deinstrumented_opcode) {
            case RETURN_VALUE:
            case RAISE_VARARGS:
            case RERAISE:
                /* Blocks of code after these terminators
                 * should be treated as different lines */
                current_line = -1;
        }
        i += inst.length;
    }
}

static void
initialize_line_tools(PyCodeObject *code, PyInterpreterState *interp)
{
    uint8_t *line_tools = code->_co_instrumentation.monitoring_data->line_tools;
    assert(line_tools != NULL);
    int code_len = (int)Py_SIZE(code);
    for (int i = 0; i < code_len; i++) {
        line_tools[i] = interp->monitoring_matrix.tools[PY_MONITORING_EVENT_LINE];
    }
}

int
update_instrumentation_data(PyCodeObject *code, PyInterpreterState *interp)
{
    int code_len = (int)Py_SIZE(code);
    if (code->_co_instrumentation.monitoring_data == NULL) {
        code->_co_instrumentation.monitoring_data = PyMem_Malloc(sizeof(_PyCoInstrumentationData));
        if (code->_co_instrumentation.monitoring_data == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        code->_co_instrumentation.monitoring_data->matrix = (_Py_MonitoringMatrix){ 0 };
        code->_co_instrumentation.monitoring_data->tools = NULL;
        code->_co_instrumentation.monitoring_data->lines = NULL;
        code->_co_instrumentation.monitoring_data->line_tools = NULL;
        code->_co_instrumentation.monitoring_data->per_instruction_opcodes = NULL;
        code->_co_instrumentation.monitoring_data->per_instruction_tools = NULL;
    }
    bool multitools = multiple_tools(interp->monitoring_matrix);
    if (code->_co_instrumentation.monitoring_data->tools == NULL && multitools) {
        code->_co_instrumentation.monitoring_data->tools = PyMem_Malloc(code_len);
        if (code->_co_instrumentation.monitoring_data->tools == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        initialize_tools(code);
    }
    if (interp->monitoring_matrix.tools[PY_MONITORING_EVENT_LINE]) {
        if (code->_co_instrumentation.monitoring_data->lines == NULL) {
            code->_co_instrumentation.monitoring_data->lines = PyMem_Malloc(code_len * sizeof(_PyCoLineInstrumentationData));
            if (code->_co_instrumentation.monitoring_data->lines == NULL) {
                PyErr_NoMemory();
                return -1;
            }
            initialize_lines(code);
        }
        if (multitools && code->_co_instrumentation.monitoring_data->line_tools == NULL) {
            code->_co_instrumentation.monitoring_data->line_tools = PyMem_Malloc(code_len);
            if (code->_co_instrumentation.monitoring_data->line_tools == NULL) {
                PyErr_NoMemory();
                return -1;
            }
            initialize_line_tools(code, interp);
        }
    }
    if (interp->monitoring_matrix.tools[PY_MONITORING_EVENT_INSTRUCTION]) {
        if (code->_co_instrumentation.monitoring_data->per_instruction_opcodes == NULL) {
            code->_co_instrumentation.monitoring_data->per_instruction_opcodes = PyMem_Malloc(code_len * sizeof(_PyCoLineInstrumentationData));
            if (code->_co_instrumentation.monitoring_data->per_instruction_opcodes == NULL) {
                PyErr_NoMemory();
                return -1;
            }
            /* This may not be necessary, as we can initialize this memory lazily, but it helps catch errors. */
            for (int i = 0; i < code_len; i++) {
                code->_co_instrumentation.monitoring_data->per_instruction_opcodes[i] = 0;
            }
        }
        if (multitools && code->_co_instrumentation.monitoring_data->per_instruction_tools == NULL) {
            code->_co_instrumentation.monitoring_data->per_instruction_tools = PyMem_Malloc(code_len);
            if (code->_co_instrumentation.monitoring_data->per_instruction_tools == NULL) {
                PyErr_NoMemory();
                return -1;
            }
            /* This may not be necessary, as we can initialize this memory lazily, but it helps catch errors. */
            for (int i = 0; i < code_len; i++) {
                code->_co_instrumentation.monitoring_data->per_instruction_tools[i] = 0;
            }
        }
    }
    return 0;
}

static bool super_instructions[256] = {
    [LOAD_FAST__LOAD_FAST] = true,
    [LOAD_FAST__LOAD_CONST] = true,
    [STORE_FAST__LOAD_FAST] = true,
    [STORE_FAST__STORE_FAST] = true,
    [LOAD_CONST__LOAD_FAST] = true,
};

/* Should use instruction metadata for this */
static bool
is_super_instruction(int opcode) {
    return super_instructions[opcode];
}

int
_Py_Instrument(PyCodeObject *code, PyInterpreterState *interp)
{

    if (is_instrumentation_up_to_date(code, interp)) {
        assert(interp->monitoring_version == 0 ||
            matrix_equals(code->_co_instrumentation.monitoring_data->matrix, interp->monitoring_matrix)
        );
        return 0;
    }
    int code_len = (int)Py_SIZE(code);
    if (update_instrumentation_data(code, interp)) {
        return -1;
    }
    _Py_MonitoringMatrix new_events;
    _Py_MonitoringMatrix removed_events;

    bool restarted = interp->last_restart_version > code->_co_instrumentation.monitoring_version;
    if (restarted) {
        removed_events = code->_co_instrumentation.monitoring_data->matrix;
        new_events = interp->monitoring_matrix;
    }
    else {
        removed_events = matrix_sub(code->_co_instrumentation.monitoring_data->matrix, interp->monitoring_matrix);
        new_events = matrix_sub(interp->monitoring_matrix, code->_co_instrumentation.monitoring_data->matrix);
        assert(matrix_empty(matrix_and(new_events, removed_events)));
    }
    code->_co_instrumentation.monitoring_data->matrix = interp->monitoring_matrix;
    code->_co_instrumentation.monitoring_version = interp->monitoring_version;
    if (matrix_empty(new_events) && matrix_empty(removed_events)) {
        //sanity_check_instrumentation(code);
        return 0;
    }
    /* Insert instrumentation */
    for (int i = 0; i < code_len; i++) {
        _Py_CODEUNIT *instr = &_PyCode_CODE(code)[i];
        int opcode = _Py_OPCODE(*instr);
        if (is_super_instruction(opcode)) {
            instr->opcode = _PyOpcode_Deopt[opcode];
        }
        int base_opcode = _Py_GetBaseOpcode(code, i);
        if (OPCODE_HAS_EVENT[base_opcode]) {
            int8_t event;
            if (base_opcode == RESUME) {
                event = instr->oparg > 0;
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
        while (base_opcode == EXTENDED_ARG) {
            i++;
            base_opcode = _Py_GetBaseOpcode(code, i);
        }
        /* TO DO -- Use instruction length, not cache count */
        if (base_opcode == COMPARE_AND_BRANCH) {
            /* Skip over following POP_JUMP_IF */
            CHECK(instr[2].opcode == POP_JUMP_IF_FALSE ||
                   instr[2].opcode == POP_JUMP_IF_TRUE);
            i++;
        }
        i += _PyOpcode_Caches[base_opcode];

    }
    uint8_t new_line_tools = new_events.tools[PY_MONITORING_EVENT_LINE];
    uint8_t removed_line_tools = removed_events.tools[PY_MONITORING_EVENT_LINE];
    if (new_line_tools | removed_line_tools) {
        _PyCoLineInstrumentationData *line_data = code->_co_instrumentation.monitoring_data->lines;
        for (int i = code->_co_firsttraceable; i < code_len;) {
            Instruction inst = read_instruction(code, i);
            if (line_data[i].original_opcode) {
                if (removed_line_tools) {
                    remove_line_tools(code, i, removed_line_tools);
                }
                if (new_line_tools) {
                    add_line_tools(code, i, new_line_tools);
                }
            }
            i += inst.length;
        }
    }
    uint8_t new_per_instruction_tools = new_events.tools[PY_MONITORING_EVENT_INSTRUCTION];
    uint8_t removed_per_instruction_tools = removed_events.tools[PY_MONITORING_EVENT_INSTRUCTION];
    if (new_per_instruction_tools | removed_per_instruction_tools) {
        for (int i = code->_co_firsttraceable; i < code_len;) {
            Instruction inst = read_instruction(code, i);
            int opcode = _PyOpcode_Deopt[inst.deinstrumented_opcode];
            if (opcode == RESUME || opcode == END_FOR) {
                i += inst.length;
                continue;
            }
            if (removed_per_instruction_tools) {
                remove_per_instruction_tools(code, i, removed_per_instruction_tools);
            }
            if (new_per_instruction_tools) {
                add_per_instruction_tools(code, i, new_per_instruction_tools);
            }
            i += inst.length;
        }
    }
#ifdef INSTRUMENT_DEBUG
    sanity_check_instrumentation(code);
#endif
    return 0;
}

#define C_RETURN_EVENTS \
    ((1 << PY_MONITORING_EVENT_C_RETURN) | \
    (1 << PY_MONITORING_EVENT_C_RAISE))

#define C_CALL_EVENTS \
    (C_RETURN_EVENTS | (1 << PY_MONITORING_EVENT_CALL))


static void
instrument_all_executing_code_objects(PyInterpreterState *interp) {
    _PyRuntimeState *runtime = &_PyRuntime;
    HEAD_LOCK(runtime);
    PyThreadState* ts = PyInterpreterState_ThreadHead(interp);
    HEAD_UNLOCK(runtime);
    while (ts) {
        _PyInterpreterFrame *frame = ts->cframe->current_frame;
        while (frame) {
            if (frame->owner != FRAME_OWNED_BY_CSTACK) {
                _Py_Instrument(frame->f_code, interp);
            }
            frame = frame->previous;
        }
        HEAD_LOCK(runtime);
        ts = PyThreadState_Next(ts);
        HEAD_UNLOCK(runtime);
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
    for (int e = 0; e < PY_MONITORING_EVENTS; e++) {
        int val = (events >> e) & 1;
        matrix_set_bit(&interp->monitoring_matrix, e, tool_id, val);
    }
    interp->monitoring_version++;

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
    int err = PyObject_SetAttrString(mod, "events", events);
    Py_DECREF(events);
    if (err) {
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
