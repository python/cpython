#include "Python.h"

#include "opcode_ids.h"

#include "pycore_bitutils.h"      // _Py_popcount32
#include "pycore_call.h"
#include "pycore_ceval.h"         // _PY_EVAL_EVENTS_BITS
#include "pycore_code.h"          // _PyCode_Clear_Executors()
#include "pycore_critical_section.h"
#include "pycore_frame.h"
#include "pycore_interp.h"
#include "pycore_long.h"
#include "pycore_modsupport.h"    // _PyModule_CreateInitialized()
#include "pycore_namespace.h"
#include "pycore_object.h"
#include "pycore_opcode_metadata.h" // IS_VALID_OPCODE, _PyOpcode_Caches
#include "pycore_pyatomic_ft_wrappers.h" // FT_ATOMIC_STORE_UINTPTR_RELEASE
#include "pycore_pyerrors.h"
#include "pycore_pystate.h"       // _PyInterpreterState_GET()

/* Uncomment this to dump debugging output when assertions fail */
// #define INSTRUMENT_DEBUG 1

#if defined(Py_DEBUG) && defined(Py_GIL_DISABLED)

#define ASSERT_WORLD_STOPPED_OR_LOCKED(obj)                         \
    if (!_PyInterpreterState_GET()->stoptheworld.world_stopped) {   \
        _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(obj);             \
    }
#define ASSERT_WORLD_STOPPED() assert(_PyInterpreterState_GET()->stoptheworld.world_stopped);

#else

#define ASSERT_WORLD_STOPPED_OR_LOCKED(obj)
#define ASSERT_WORLD_STOPPED()

#endif

#ifdef Py_GIL_DISABLED

#define LOCK_CODE(code)                                             \
    assert(!_PyInterpreterState_GET()->stoptheworld.world_stopped); \
    Py_BEGIN_CRITICAL_SECTION(code)

#define UNLOCK_CODE()   Py_END_CRITICAL_SECTION()

#else

#define LOCK_CODE(code)
#define UNLOCK_CODE()

#endif

PyObject _PyInstrumentation_DISABLE = _PyObject_HEAD_INIT(&PyBaseObject_Type);

PyObject _PyInstrumentation_MISSING = _PyObject_HEAD_INIT(&PyBaseObject_Type);

static const int8_t EVENT_FOR_OPCODE[256] = {
    [RETURN_CONST] = PY_MONITORING_EVENT_PY_RETURN,
    [INSTRUMENTED_RETURN_CONST] = PY_MONITORING_EVENT_PY_RETURN,
    [RETURN_VALUE] = PY_MONITORING_EVENT_PY_RETURN,
    [INSTRUMENTED_RETURN_VALUE] = PY_MONITORING_EVENT_PY_RETURN,
    [CALL] = PY_MONITORING_EVENT_CALL,
    [INSTRUMENTED_CALL] = PY_MONITORING_EVENT_CALL,
    [CALL_KW] = PY_MONITORING_EVENT_CALL,
    [INSTRUMENTED_CALL_KW] = PY_MONITORING_EVENT_CALL,
    [CALL_FUNCTION_EX] = PY_MONITORING_EVENT_CALL,
    [INSTRUMENTED_CALL_FUNCTION_EX] = PY_MONITORING_EVENT_CALL,
    [LOAD_SUPER_ATTR] = PY_MONITORING_EVENT_CALL,
    [INSTRUMENTED_LOAD_SUPER_ATTR] = PY_MONITORING_EVENT_CALL,
    [RESUME] = -1,
    [YIELD_VALUE] = PY_MONITORING_EVENT_PY_YIELD,
    [INSTRUMENTED_YIELD_VALUE] = PY_MONITORING_EVENT_PY_YIELD,
    [JUMP_FORWARD] = PY_MONITORING_EVENT_JUMP,
    [JUMP_BACKWARD] = PY_MONITORING_EVENT_JUMP,
    [POP_JUMP_IF_FALSE] = PY_MONITORING_EVENT_BRANCH,
    [POP_JUMP_IF_TRUE] = PY_MONITORING_EVENT_BRANCH,
    [POP_JUMP_IF_NONE] = PY_MONITORING_EVENT_BRANCH,
    [POP_JUMP_IF_NOT_NONE] = PY_MONITORING_EVENT_BRANCH,
    [INSTRUMENTED_JUMP_FORWARD] = PY_MONITORING_EVENT_JUMP,
    [INSTRUMENTED_JUMP_BACKWARD] = PY_MONITORING_EVENT_JUMP,
    [INSTRUMENTED_POP_JUMP_IF_FALSE] = PY_MONITORING_EVENT_BRANCH,
    [INSTRUMENTED_POP_JUMP_IF_TRUE] = PY_MONITORING_EVENT_BRANCH,
    [INSTRUMENTED_POP_JUMP_IF_NONE] = PY_MONITORING_EVENT_BRANCH,
    [INSTRUMENTED_POP_JUMP_IF_NOT_NONE] = PY_MONITORING_EVENT_BRANCH,
    [FOR_ITER] = PY_MONITORING_EVENT_BRANCH,
    [INSTRUMENTED_FOR_ITER] = PY_MONITORING_EVENT_BRANCH,
    [END_FOR] = PY_MONITORING_EVENT_STOP_ITERATION,
    [INSTRUMENTED_END_FOR] = PY_MONITORING_EVENT_STOP_ITERATION,
    [END_SEND] = PY_MONITORING_EVENT_STOP_ITERATION,
    [INSTRUMENTED_END_SEND] = PY_MONITORING_EVENT_STOP_ITERATION,
};

static const uint8_t DE_INSTRUMENT[256] = {
    [INSTRUMENTED_RESUME] = RESUME,
    [INSTRUMENTED_RETURN_VALUE] = RETURN_VALUE,
    [INSTRUMENTED_RETURN_CONST] = RETURN_CONST,
    [INSTRUMENTED_CALL] = CALL,
    [INSTRUMENTED_CALL_KW] = CALL_KW,
    [INSTRUMENTED_CALL_FUNCTION_EX] = CALL_FUNCTION_EX,
    [INSTRUMENTED_YIELD_VALUE] = YIELD_VALUE,
    [INSTRUMENTED_JUMP_FORWARD] = JUMP_FORWARD,
    [INSTRUMENTED_JUMP_BACKWARD] = JUMP_BACKWARD,
    [INSTRUMENTED_POP_JUMP_IF_FALSE] = POP_JUMP_IF_FALSE,
    [INSTRUMENTED_POP_JUMP_IF_TRUE] = POP_JUMP_IF_TRUE,
    [INSTRUMENTED_POP_JUMP_IF_NONE] = POP_JUMP_IF_NONE,
    [INSTRUMENTED_POP_JUMP_IF_NOT_NONE] = POP_JUMP_IF_NOT_NONE,
    [INSTRUMENTED_FOR_ITER] = FOR_ITER,
    [INSTRUMENTED_END_FOR] = END_FOR,
    [INSTRUMENTED_END_SEND] = END_SEND,
    [INSTRUMENTED_LOAD_SUPER_ATTR] = LOAD_SUPER_ATTR,
};

static const uint8_t INSTRUMENTED_OPCODES[256] = {
    [RETURN_CONST] = INSTRUMENTED_RETURN_CONST,
    [INSTRUMENTED_RETURN_CONST] = INSTRUMENTED_RETURN_CONST,
    [RETURN_VALUE] = INSTRUMENTED_RETURN_VALUE,
    [INSTRUMENTED_RETURN_VALUE] = INSTRUMENTED_RETURN_VALUE,
    [CALL] = INSTRUMENTED_CALL,
    [INSTRUMENTED_CALL] = INSTRUMENTED_CALL,
    [CALL_KW] = INSTRUMENTED_CALL_KW,
    [INSTRUMENTED_CALL_KW] = INSTRUMENTED_CALL_KW,
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
    [POP_JUMP_IF_FALSE] = INSTRUMENTED_POP_JUMP_IF_FALSE,
    [INSTRUMENTED_POP_JUMP_IF_FALSE] = INSTRUMENTED_POP_JUMP_IF_FALSE,
    [POP_JUMP_IF_TRUE] = INSTRUMENTED_POP_JUMP_IF_TRUE,
    [INSTRUMENTED_POP_JUMP_IF_TRUE] = INSTRUMENTED_POP_JUMP_IF_TRUE,
    [POP_JUMP_IF_NONE] = INSTRUMENTED_POP_JUMP_IF_NONE,
    [INSTRUMENTED_POP_JUMP_IF_NONE] = INSTRUMENTED_POP_JUMP_IF_NONE,
    [POP_JUMP_IF_NOT_NONE] = INSTRUMENTED_POP_JUMP_IF_NOT_NONE,
    [INSTRUMENTED_POP_JUMP_IF_NOT_NONE] = INSTRUMENTED_POP_JUMP_IF_NOT_NONE,
    [END_FOR] = INSTRUMENTED_END_FOR,
    [INSTRUMENTED_END_FOR] = INSTRUMENTED_END_FOR,
    [END_SEND] = INSTRUMENTED_END_SEND,
    [INSTRUMENTED_END_SEND] = INSTRUMENTED_END_SEND,
    [FOR_ITER] = INSTRUMENTED_FOR_ITER,
    [INSTRUMENTED_FOR_ITER] = INSTRUMENTED_FOR_ITER,
    [LOAD_SUPER_ATTR] = INSTRUMENTED_LOAD_SUPER_ATTR,
    [INSTRUMENTED_LOAD_SUPER_ATTR] = INSTRUMENTED_LOAD_SUPER_ATTR,

    [INSTRUMENTED_LINE] = INSTRUMENTED_LINE,
    [INSTRUMENTED_INSTRUCTION] = INSTRUMENTED_INSTRUCTION,
};

static inline bool
opcode_has_event(int opcode)
{
    return (
        opcode != INSTRUMENTED_LINE &&
        INSTRUMENTED_OPCODES[opcode] > 0
    );
}

static inline bool
is_instrumented(int opcode)
{
    assert(opcode != 0);
    assert(opcode != RESERVED);
    assert(opcode != ENTER_EXECUTOR);
    return opcode >= MIN_INSTRUMENTED_OPCODE;
}

#ifndef NDEBUG
static inline bool
monitors_equals(_Py_LocalMonitors a, _Py_LocalMonitors b)
{
    for (int i = 0; i < _PY_MONITORING_LOCAL_EVENTS; i++) {
        if (a.tools[i] != b.tools[i]) {
            return false;
        }
    }
    return true;
}
#endif

static inline _Py_LocalMonitors
monitors_sub(_Py_LocalMonitors a, _Py_LocalMonitors b)
{
    _Py_LocalMonitors res;
    for (int i = 0; i < _PY_MONITORING_LOCAL_EVENTS; i++) {
        res.tools[i] = a.tools[i] & ~b.tools[i];
    }
    return res;
}

#ifndef NDEBUG
static inline _Py_LocalMonitors
monitors_and(_Py_LocalMonitors a, _Py_LocalMonitors b)
{
    _Py_LocalMonitors res;
    for (int i = 0; i < _PY_MONITORING_LOCAL_EVENTS; i++) {
        res.tools[i] = a.tools[i] & b.tools[i];
    }
    return res;
}
#endif

/* The union of the *local* events in a and b.
 * Global events like RAISE are ignored.
 * Used for instrumentation, as only local
 * events get instrumented.
 */
static inline _Py_LocalMonitors
local_union(_Py_GlobalMonitors a, _Py_LocalMonitors b)
{
    _Py_LocalMonitors res;
    for (int i = 0; i < _PY_MONITORING_LOCAL_EVENTS; i++) {
        res.tools[i] = a.tools[i] | b.tools[i];
    }
    return res;
}

static inline bool
monitors_are_empty(_Py_LocalMonitors m)
{
    for (int i = 0; i < _PY_MONITORING_LOCAL_EVENTS; i++) {
        if (m.tools[i]) {
            return false;
        }
    }
    return true;
}

static inline bool
multiple_tools(_Py_LocalMonitors *m)
{
    for (int i = 0; i < _PY_MONITORING_LOCAL_EVENTS; i++) {
        if (_Py_popcount32(m->tools[i]) > 1) {
            return true;
        }
    }
    return false;
}

static inline _PyMonitoringEventSet
get_local_events(_Py_LocalMonitors *m, int tool_id)
{
    _PyMonitoringEventSet result = 0;
    for (int e = 0; e < _PY_MONITORING_LOCAL_EVENTS; e++) {
        if ((m->tools[e] >> tool_id) & 1) {
            result |= (1 << e);
        }
    }
    return result;
}

static inline _PyMonitoringEventSet
get_events(_Py_GlobalMonitors *m, int tool_id)
{
    _PyMonitoringEventSet result = 0;
    for (int e = 0; e < _PY_MONITORING_UNGROUPED_EVENTS; e++) {
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
 * elif line_delta == -127 or line_delta == -126:
 *     line = PyCode_Addr2Line(code, offset * sizeof(_Py_CODEUNIT));
 * else:
 *     line = first_line  + (offset >> OFFSET_SHIFT) + line_delta;
 */

#define NO_LINE -128
#define COMPUTED_LINE_LINENO_CHANGE -127
#define COMPUTED_LINE -126

#define OFFSET_SHIFT 4

static int8_t
compute_line_delta(PyCodeObject *code, int offset, int line)
{
    if (line < 0) {
        return NO_LINE;
    }
    int delta = line - code->co_firstlineno - (offset >> OFFSET_SHIFT);
    if (delta <= INT8_MAX && delta > COMPUTED_LINE) {
        return delta;
    }
    return COMPUTED_LINE;
}

static int
compute_line(PyCodeObject *code, int offset, int8_t line_delta)
{
    if (line_delta > COMPUTED_LINE) {
        return code->co_firstlineno + (offset >> OFFSET_SHIFT) + line_delta;
    }
    if (line_delta == NO_LINE) {

        return -1;
    }
    assert(line_delta == COMPUTED_LINE || line_delta == COMPUTED_LINE_LINENO_CHANGE);
    /* Look it up */
    return PyCode_Addr2Line(code, offset * sizeof(_Py_CODEUNIT));
}

int
_PyInstruction_GetLength(PyCodeObject *code, int offset)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);

    int opcode = _PyCode_CODE(code)[offset].op.code;
    assert(opcode != 0);
    assert(opcode != RESERVED);
    if (opcode == INSTRUMENTED_LINE) {
        opcode = code->_co_monitoring->lines[offset].original_opcode;
    }
    if (opcode == INSTRUMENTED_INSTRUCTION) {
        opcode = code->_co_monitoring->per_instruction_opcodes[offset];
    }
    int deinstrumented = DE_INSTRUMENT[opcode];
    if (deinstrumented) {
        opcode = deinstrumented;
    }
    else {
        opcode = _PyOpcode_Deopt[opcode];
    }
    assert(opcode != 0);
    if (opcode == ENTER_EXECUTOR) {
        int exec_index = _PyCode_CODE(code)[offset].op.arg;
        _PyExecutorObject *exec = code->co_executors->executors[exec_index];
        opcode = _PyOpcode_Deopt[exec->vm_data.opcode];
    }
    assert(!is_instrumented(opcode));
    assert(opcode != ENTER_EXECUTOR);
    assert(opcode == _PyOpcode_Deopt[opcode]);
    return 1 + _PyOpcode_Caches[opcode];
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
dump_instrumentation_data_per_instruction(PyCodeObject *code, _PyCoMonitoringData *data, int i, FILE*out)
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
dump_global_monitors(const char *prefix, _Py_GlobalMonitors monitors, FILE*out)
{
    fprintf(out, "%s monitors:\n", prefix);
    for (int event = 0; event < _PY_MONITORING_UNGROUPED_EVENTS; event++) {
        fprintf(out, "    Event %d: Tools %x\n", event, monitors.tools[event]);
    }
}

static void
dump_local_monitors(const char *prefix, _Py_LocalMonitors monitors, FILE*out)
{
    fprintf(out, "%s monitors:\n", prefix);
    for (int event = 0; event < _PY_MONITORING_LOCAL_EVENTS; event++) {
        fprintf(out, "    Event %d: Tools %x\n", event, monitors.tools[event]);
    }
}

/* No error checking -- Don't use this for anything but experimental debugging */
static void
dump_instrumentation_data(PyCodeObject *code, int star, FILE*out)
{
    _PyCoMonitoringData *data = code->_co_monitoring;
    fprintf(out, "\n");
    PyObject_Print(code->co_name, out, Py_PRINT_RAW);
    fprintf(out, "\n");
    if (data == NULL) {
        fprintf(out, "NULL\n");
        return;
    }
    dump_global_monitors("Global", _PyInterpreterState_GET()->monitors, out);
    dump_local_monitors("Code", data->local_monitors, out);
    dump_local_monitors("Active", data->active_monitors, out);
    int code_len = (int)Py_SIZE(code);
    bool starred = false;
    for (int i = 0; i < code_len; i += _PyInstruction_GetLength(code, i)) {
        _Py_CODEUNIT *instr = &_PyCode_CODE(code)[i];
        int opcode = instr->op.code;
        if (i == star) {
            fprintf(out, "**  ");
            starred = true;
        }
        fprintf(out, "Offset: %d, line: %d %s: ", i, PyCode_Addr2Line(code, i*2), _PyOpcode_OpName[opcode]);
        dump_instrumentation_data_tools(code, data->tools, i, out);
        dump_instrumentation_data_lines(code, data->lines, i, out);
        dump_instrumentation_data_line_tools(code, data->line_tools, i, out);
        dump_instrumentation_data_per_instruction(code, data, i, out);
        fprintf(out, "\n");
        ;
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

static bool
valid_opcode(int opcode)
{
    if (opcode == INSTRUMENTED_LINE) {
        return true;
    }
    if (IS_VALID_OPCODE(opcode) &&
        opcode != CACHE &&
        opcode != RESERVED &&
        opcode < 255)
    {
       return true;
    }
    return false;
}

static void
sanity_check_instrumentation(PyCodeObject *code)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);

    _PyCoMonitoringData *data = code->_co_monitoring;
    if (data == NULL) {
        return;
    }
    _Py_GlobalMonitors global_monitors = _PyInterpreterState_GET()->monitors;
    _Py_LocalMonitors active_monitors;
    if (code->_co_monitoring) {
        _Py_LocalMonitors local_monitors = code->_co_monitoring->local_monitors;
        active_monitors = local_union(global_monitors, local_monitors);
    }
    else {
        _Py_LocalMonitors empty = (_Py_LocalMonitors) { 0 };
        active_monitors = local_union(global_monitors, empty);
    }
    assert(monitors_equals(
        code->_co_monitoring->active_monitors,
        active_monitors));
    int code_len = (int)Py_SIZE(code);
    for (int i = 0; i < code_len;) {
        _Py_CODEUNIT *instr = &_PyCode_CODE(code)[i];
        int opcode = instr->op.code;
        int base_opcode = _Py_GetBaseCodeUnit(code, offset).op.code;
        CHECK(valid_opcode(opcode));
        CHECK(valid_opcode(base_opcode));
        if (opcode == INSTRUMENTED_INSTRUCTION) {
            opcode = data->per_instruction_opcodes[i];
            if (!is_instrumented(opcode)) {
                CHECK(_PyOpcode_Deopt[opcode] == opcode);
            }
        }
        if (opcode == INSTRUMENTED_LINE) {
            CHECK(data->lines);
            CHECK(valid_opcode(data->lines[i].original_opcode));
            opcode = data->lines[i].original_opcode;
            CHECK(opcode != END_FOR);
            CHECK(opcode != RESUME);
            CHECK(opcode != RESUME_CHECK);
            CHECK(opcode != INSTRUMENTED_RESUME);
            if (!is_instrumented(opcode)) {
                CHECK(_PyOpcode_Deopt[opcode] == opcode);
            }
            CHECK(opcode != INSTRUMENTED_LINE);
        }
        else if (data->lines) {
            /* If original_opcode is INSTRUMENTED_INSTRUCTION
             * *and* we are executing a INSTRUMENTED_LINE instruction
             * that has de-instrumented itself, then we will execute
             * an invalid INSTRUMENTED_INSTRUCTION */
            CHECK(data->lines[i].original_opcode != INSTRUMENTED_INSTRUCTION);
        }
        if (opcode == INSTRUMENTED_INSTRUCTION) {
            CHECK(data->per_instruction_opcodes[i] != 0);
            opcode = data->per_instruction_opcodes[i];
        }
        if (is_instrumented(opcode)) {
            CHECK(DE_INSTRUMENT[opcode] == base_opcode);
            int event = EVENT_FOR_OPCODE[DE_INSTRUMENT[opcode]];
            if (event < 0) {
                /* RESUME fixup */
                event = instr->op.arg ? 1: 0;
            }
            CHECK(active_monitors.tools[event] != 0);
        }
        if (data->lines && base_opcode != END_FOR) {
            int line1 = compute_line(code, i, data->lines[i].line_delta);
            int line2 = PyCode_Addr2Line(code, i*sizeof(_Py_CODEUNIT));
            CHECK(line1 == line2);
        }
        CHECK(valid_opcode(opcode));
        if (data->tools) {
            uint8_t local_tools = data->tools[i];
            if (opcode_has_event(base_opcode)) {
                int event = EVENT_FOR_OPCODE[base_opcode];
                if (event == -1) {
                    /* RESUME fixup */
                    event = _PyCode_CODE(code)[i].op.arg;
                }
                CHECK((active_monitors.tools[event] & local_tools) == local_tools);
            }
            else {
                CHECK(local_tools == 0xff);
            }
        }
        i += _PyInstruction_GetLength(code, i);
        assert(i <= code_len);
    }
}
#else

#define CHECK(test) assert(test)

#endif

/* Get the underlying code unit, stripping instrumentation and ENTER_EXECUTOR */
_Py_CODEUNIT
_Py_GetBaseCodeUnit(PyCodeObject *code, int i)
{
    _Py_CODEUNIT inst = _PyCode_CODE(code)[i];
    int opcode = inst.op.code;
    if (opcode < MIN_INSTRUMENTED_OPCODE) {
        inst.op.code = _PyOpcode_Deopt[opcode];
        assert(inst.op.code <= RESUME);
        return inst;
    }
    if (opcode == ENTER_EXECUTOR) {
        _PyExecutorObject *exec = code->co_executors->executors[inst.op.arg];
        opcode = _PyOpcode_Deopt[exec->vm_data.opcode];
        inst.op.code = opcode;
        assert(opcode <= RESUME);
        inst.op.arg = exec->vm_data.oparg;
        assert(inst.op.code <= RESUME);
        return inst;
    }
    if (opcode == INSTRUMENTED_LINE) {
        opcode = code->_co_monitoring->lines[i].original_opcode;
    }
    if (opcode == INSTRUMENTED_INSTRUCTION) {
        opcode = code->_co_monitoring->per_instruction_opcodes[i];
    }
    CHECK(opcode != INSTRUMENTED_INSTRUCTION);
    CHECK(opcode != INSTRUMENTED_LINE);
    int deinstrumented = DE_INSTRUMENT[opcode];
    if (deinstrumented) {
        inst.op.code = deinstrumented;
    }
    else {
        inst.op.code = _PyOpcode_Deopt[opcode];
    }
    assert(inst.op.code < MIN_SPECIALIZED_OPCODE);
    return inst;
}

static void
de_instrument(PyCodeObject *code, int i, int event)
{
    assert(event != PY_MONITORING_EVENT_INSTRUCTION);
    assert(event != PY_MONITORING_EVENT_LINE);

    _Py_CODEUNIT *instr = &_PyCode_CODE(code)[i];
    uint8_t *opcode_ptr = &instr->op.code;
    int opcode = *opcode_ptr;
    assert(opcode != ENTER_EXECUTOR);
    if (opcode == INSTRUMENTED_LINE) {
        opcode_ptr = &code->_co_monitoring->lines[i].original_opcode;
        opcode = *opcode_ptr;
    }
    if (opcode == INSTRUMENTED_INSTRUCTION) {
        opcode_ptr = &code->_co_monitoring->per_instruction_opcodes[i];
        opcode = *opcode_ptr;
    }
    int deinstrumented = DE_INSTRUMENT[opcode];
    if (deinstrumented == 0) {
        return;
    }
    CHECK(_PyOpcode_Deopt[deinstrumented] == deinstrumented);
    FT_ATOMIC_STORE_UINT8_RELAXED(*opcode_ptr, deinstrumented);
    if (_PyOpcode_Caches[deinstrumented]) {
        FT_ATOMIC_STORE_UINT16_RELAXED(instr[1].counter.as_counter,
                                       adaptive_counter_warmup().as_counter);
    }
}

static void
de_instrument_line(PyCodeObject *code, int i)
{
    _Py_CODEUNIT *instr = &_PyCode_CODE(code)[i];
    int opcode = instr->op.code;
    if (opcode != INSTRUMENTED_LINE) {
        return;
    }
    _PyCoLineInstrumentationData *lines = &code->_co_monitoring->lines[i];
    int original_opcode = lines->original_opcode;
    if (original_opcode == INSTRUMENTED_INSTRUCTION) {
        lines->original_opcode = code->_co_monitoring->per_instruction_opcodes[i];
    }
    CHECK(original_opcode != 0);
    CHECK(original_opcode == _PyOpcode_Deopt[original_opcode]);
    instr->op.code = original_opcode;
    if (_PyOpcode_Caches[original_opcode]) {
        instr[1].counter = adaptive_counter_warmup();
    }
    assert(instr->op.code != INSTRUMENTED_LINE);
}

static void
de_instrument_per_instruction(PyCodeObject *code, int i)
{
    _Py_CODEUNIT *instr = &_PyCode_CODE(code)[i];
    uint8_t *opcode_ptr = &instr->op.code;
    int opcode = *opcode_ptr;
    if (opcode == INSTRUMENTED_LINE) {
        opcode_ptr = &code->_co_monitoring->lines[i].original_opcode;
        opcode = *opcode_ptr;
    }
    if (opcode != INSTRUMENTED_INSTRUCTION) {
        return;
    }
    int original_opcode = code->_co_monitoring->per_instruction_opcodes[i];
    CHECK(original_opcode != 0);
    CHECK(original_opcode == _PyOpcode_Deopt[original_opcode]);
    *opcode_ptr = original_opcode;
    if (_PyOpcode_Caches[original_opcode]) {
        instr[1].counter = adaptive_counter_warmup();
    }
    assert(*opcode_ptr != INSTRUMENTED_INSTRUCTION);
    assert(instr->op.code != INSTRUMENTED_INSTRUCTION);
}


static void
instrument(PyCodeObject *code, int i)
{
    _Py_CODEUNIT *instr = &_PyCode_CODE(code)[i];
    uint8_t *opcode_ptr = &instr->op.code;
    int opcode =*opcode_ptr;
    if (opcode == INSTRUMENTED_LINE) {
        _PyCoLineInstrumentationData *lines = &code->_co_monitoring->lines[i];
        opcode_ptr = &lines->original_opcode;
        opcode = *opcode_ptr;
    }
    if (opcode == INSTRUMENTED_INSTRUCTION) {
        opcode_ptr = &code->_co_monitoring->per_instruction_opcodes[i];
        opcode = *opcode_ptr;
        CHECK(opcode != INSTRUMENTED_INSTRUCTION && opcode != INSTRUMENTED_LINE);
        CHECK(opcode == _PyOpcode_Deopt[opcode]);
    }
    CHECK(opcode != 0);
    if (!is_instrumented(opcode)) {
        int deopt = _PyOpcode_Deopt[opcode];
        int instrumented = INSTRUMENTED_OPCODES[deopt];
        assert(instrumented);
        FT_ATOMIC_STORE_UINT8_RELAXED(*opcode_ptr, instrumented);
        if (_PyOpcode_Caches[deopt]) {
          FT_ATOMIC_STORE_UINT16_RELAXED(instr[1].counter.as_counter,
                                         adaptive_counter_warmup().as_counter);
            instr[1].counter = adaptive_counter_warmup();
        }
    }
}

static void
instrument_line(PyCodeObject *code, int i)
{
    uint8_t *opcode_ptr = &_PyCode_CODE(code)[i].op.code;
    int opcode = *opcode_ptr;
    if (opcode == INSTRUMENTED_LINE) {
        return;
    }
    _PyCoLineInstrumentationData *lines = &code->_co_monitoring->lines[i];
    lines->original_opcode = _PyOpcode_Deopt[opcode];
    CHECK(lines->original_opcode > 0);
    *opcode_ptr = INSTRUMENTED_LINE;
}

static void
instrument_per_instruction(PyCodeObject *code, int i)
{
    _Py_CODEUNIT *instr = &_PyCode_CODE(code)[i];
    uint8_t *opcode_ptr = &instr->op.code;
    int opcode = *opcode_ptr;
    if (opcode == INSTRUMENTED_LINE) {
        _PyCoLineInstrumentationData *lines = &code->_co_monitoring->lines[i];
        opcode_ptr = &lines->original_opcode;
        opcode = *opcode_ptr;
    }
    if (opcode == INSTRUMENTED_INSTRUCTION) {
        assert(code->_co_monitoring->per_instruction_opcodes[i] > 0);
        return;
    }
    CHECK(opcode != 0);
    if (is_instrumented(opcode)) {
        code->_co_monitoring->per_instruction_opcodes[i] = opcode;
    }
    else {
        assert(opcode != 0);
        assert(_PyOpcode_Deopt[opcode] != 0);
        assert(_PyOpcode_Deopt[opcode] != RESUME);
        code->_co_monitoring->per_instruction_opcodes[i] = _PyOpcode_Deopt[opcode];
    }
    assert(code->_co_monitoring->per_instruction_opcodes[i] > 0);
    *opcode_ptr = INSTRUMENTED_INSTRUCTION;
}

static void
remove_tools(PyCodeObject * code, int offset, int event, int tools)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);
    assert(event != PY_MONITORING_EVENT_LINE);
    assert(event != PY_MONITORING_EVENT_INSTRUCTION);
    assert(PY_MONITORING_IS_INSTRUMENTED_EVENT(event));
    assert(opcode_has_event(_Py_GetBaseCodeUnit(code, offset).op.code));
    _PyCoMonitoringData *monitoring = code->_co_monitoring;
    if (monitoring && monitoring->tools) {
        monitoring->tools[offset] &= ~tools;
        if (monitoring->tools[offset] == 0) {
            de_instrument(code, offset, event);
        }
    }
    else {
        /* Single tool */
        uint8_t single_tool = code->_co_monitoring->active_monitors.tools[event];
        assert(_Py_popcount32(single_tool) <= 1);
        if (((single_tool & tools) == single_tool)) {
            de_instrument(code, offset, event);
        }
    }
}

#ifndef NDEBUG
static bool
tools_is_subset_for_event(PyCodeObject * code, int event, int tools)
{
    int global_tools = _PyInterpreterState_GET()->monitors.tools[event];
    int local_tools = code->_co_monitoring->local_monitors.tools[event];
    return tools == ((global_tools | local_tools) & tools);
}
#endif

static void
remove_line_tools(PyCodeObject * code, int offset, int tools)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);

    assert(code->_co_monitoring);
    if (code->_co_monitoring->line_tools)
    {
        uint8_t *toolsptr = &code->_co_monitoring->line_tools[offset];
        *toolsptr &= ~tools;
        if (*toolsptr == 0 ) {
            de_instrument_line(code, offset);
        }
    }
    else {
        /* Single tool */
        uint8_t single_tool = code->_co_monitoring->active_monitors.tools[PY_MONITORING_EVENT_LINE];
        assert(_Py_popcount32(single_tool) <= 1);
        if (((single_tool & tools) == single_tool)) {
            de_instrument_line(code, offset);
        }
    }
}

static void
add_tools(PyCodeObject * code, int offset, int event, int tools)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);
    assert(event != PY_MONITORING_EVENT_LINE);
    assert(event != PY_MONITORING_EVENT_INSTRUCTION);
    assert(PY_MONITORING_IS_INSTRUMENTED_EVENT(event));
    assert(code->_co_monitoring);
    if (code->_co_monitoring &&
        code->_co_monitoring->tools
    ) {
        code->_co_monitoring->tools[offset] |= tools;
    }
    else {
        /* Single tool */
        assert(_Py_popcount32(tools) == 1);
        assert(tools_is_subset_for_event(code, event, tools));
    }
    instrument(code, offset);
}

static void
add_line_tools(PyCodeObject * code, int offset, int tools)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);

    assert(tools_is_subset_for_event(code, PY_MONITORING_EVENT_LINE, tools));
    assert(code->_co_monitoring);
    if (code->_co_monitoring->line_tools) {
        code->_co_monitoring->line_tools[offset] |= tools;
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
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);

    assert(tools_is_subset_for_event(code, PY_MONITORING_EVENT_INSTRUCTION, tools));
    assert(code->_co_monitoring);
    if (code->_co_monitoring->per_instruction_tools) {
        code->_co_monitoring->per_instruction_tools[offset] |= tools;
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
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);

    assert(code->_co_monitoring);
    if (code->_co_monitoring->per_instruction_tools) {
        uint8_t *toolsptr = &code->_co_monitoring->per_instruction_tools[offset];
        *toolsptr &= ~tools;
        if (*toolsptr == 0) {
            de_instrument_per_instruction(code, offset);
        }
    }
    else {
        /* Single tool */
        uint8_t single_tool = code->_co_monitoring->active_monitors.tools[PY_MONITORING_EVENT_INSTRUCTION];
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
    size_t nargsf, int8_t tool, int event)
{
    assert(0 <= tool && tool < 8);
    assert(tstate->tracing == 0);
    PyObject *instrument = interp->monitoring_callables[tool][event];
    if (instrument == NULL) {
        return 0;
    }
    int old_what = tstate->what_event;
    tstate->what_event = event;
    tstate->tracing++;
    PyObject *res = _PyObject_VectorcallTstate(tstate, instrument, args, nargsf, NULL);
    tstate->tracing--;
    tstate->what_event = old_what;
    if (res == NULL) {
        return -1;
    }
    Py_DECREF(res);
    return (res == &_PyInstrumentation_DISABLE);
}

static const int8_t MOST_SIGNIFICANT_BITS[16] = {
    -1, 0, 1, 1,
    2, 2, 2, 2,
    3, 3, 3, 3,
    3, 3, 3, 3,
};

/* We could use _Py_bit_length here, but that is designed for larger (32/64)
 * bit ints, and can perform relatively poorly on platforms without the
 * necessary intrinsics. */
static inline int most_significant_bit(uint8_t bits) {
    assert(bits != 0);
    if (bits > 15) {
        return MOST_SIGNIFICANT_BITS[bits>>4]+4;
    }
    return MOST_SIGNIFICANT_BITS[bits];
}

static uint32_t
global_version(PyInterpreterState *interp)
{
    uint32_t version = (uint32_t)_Py_atomic_load_uintptr_relaxed(
        &interp->ceval.instrumentation_version);
#ifdef Py_DEBUG
    PyThreadState *tstate = _PyThreadState_GET();
    uint32_t thread_version =
        (uint32_t)(_Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker) &
                   ~_PY_EVAL_EVENTS_MASK);
    assert(thread_version == version);
#endif
    return version;
}

/* Atomically set the given version in the given location, without touching
   anything in _PY_EVAL_EVENTS_MASK. */
static void
set_version_raw(uintptr_t *ptr, uint32_t version)
{
    uintptr_t old = _Py_atomic_load_uintptr_relaxed(ptr);
    uintptr_t new;
    do {
        new = (old & _PY_EVAL_EVENTS_MASK) | version;
    } while (!_Py_atomic_compare_exchange_uintptr(ptr, &old, new));
}

static void
set_global_version(PyThreadState *tstate, uint32_t version)
{
    assert((version & _PY_EVAL_EVENTS_MASK) == 0);
    PyInterpreterState *interp = tstate->interp;
    set_version_raw(&interp->ceval.instrumentation_version, version);

#ifdef Py_GIL_DISABLED
    // Set the version on all threads in free-threaded builds.
    _PyRuntimeState *runtime = &_PyRuntime;
    HEAD_LOCK(runtime);
    for (tstate = interp->threads.head; tstate;
         tstate = PyThreadState_Next(tstate)) {
        set_version_raw(&tstate->eval_breaker, version);
    };
    HEAD_UNLOCK(runtime);
#else
    // Normal builds take the current version from instrumentation_version when
    // attaching a thread, so we only have to set the current thread's version.
    set_version_raw(&tstate->eval_breaker, version);
#endif
}

static bool
is_version_up_to_date(PyCodeObject *code, PyInterpreterState *interp)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);
    return global_version(interp) == code->_co_instrumentation_version;
}

#ifndef NDEBUG
static bool
instrumentation_cross_checks(PyInterpreterState *interp, PyCodeObject *code)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);
    _Py_LocalMonitors expected = local_union(
        interp->monitors,
        code->_co_monitoring->local_monitors);
    return monitors_equals(code->_co_monitoring->active_monitors, expected);
}

static int
debug_check_sanity(PyInterpreterState *interp, PyCodeObject *code)
{
    int res;
    LOCK_CODE(code);
    res = is_version_up_to_date(code, interp) &&
          instrumentation_cross_checks(interp, code);
    UNLOCK_CODE();
    return res;
}

#endif

static inline uint8_t
get_tools_for_instruction(PyCodeObject *code, PyInterpreterState *interp, int i, int event)
{
    uint8_t tools;
    assert(event != PY_MONITORING_EVENT_LINE);
    assert(event != PY_MONITORING_EVENT_INSTRUCTION);
    if (event >= _PY_MONITORING_UNGROUPED_EVENTS) {
        assert(event == PY_MONITORING_EVENT_C_RAISE ||
                event == PY_MONITORING_EVENT_C_RETURN);
        event = PY_MONITORING_EVENT_CALL;
    }
    if (PY_MONITORING_IS_INSTRUMENTED_EVENT(event)) {
        CHECK(debug_check_sanity(interp, code));
        if (code->_co_monitoring->tools) {
            tools = code->_co_monitoring->tools[i];
        }
        else {
            tools = code->_co_monitoring->active_monitors.tools[event];
        }
    }
    else {
        tools = interp->monitors.tools[event];
    }
    return tools;
}

static const char *const event_names [] = {
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
    [PY_MONITORING_EVENT_RERAISE] = "RERAISE",
    [PY_MONITORING_EVENT_EXCEPTION_HANDLED] = "EXCEPTION_HANDLED",
    [PY_MONITORING_EVENT_C_RAISE] = "C_RAISE",
    [PY_MONITORING_EVENT_PY_UNWIND] = "PY_UNWIND",
    [PY_MONITORING_EVENT_STOP_ITERATION] = "STOP_ITERATION",
};

static int
call_instrumentation_vector(
    PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _Py_CODEUNIT *instr, Py_ssize_t nargs, PyObject *args[])
{
    if (tstate->tracing) {
        return 0;
    }
    assert(!_PyErr_Occurred(tstate));
    assert(args[0] == NULL);
    PyCodeObject *code = _PyFrame_GetCode(frame);
    assert(args[1] == NULL);
    args[1] = (PyObject *)code;
    int offset = (int)(instr - _PyCode_CODE(code));
    /* Offset visible to user should be the offset in bytes, as that is the
     * convention for APIs involving code offsets. */
    int bytes_offset = offset * (int)sizeof(_Py_CODEUNIT);
    PyObject *offset_obj = PyLong_FromLong(bytes_offset);
    if (offset_obj == NULL) {
        return -1;
    }
    assert(args[2] == NULL);
    args[2] = offset_obj;
    PyInterpreterState *interp = tstate->interp;
    uint8_t tools = get_tools_for_instruction(code, interp, offset, event);
    size_t nargsf = (size_t) nargs | PY_VECTORCALL_ARGUMENTS_OFFSET;
    PyObject **callargs = &args[1];
    int err = 0;
    while (tools) {
        int tool = most_significant_bit(tools);
        assert(tool >= 0 && tool < 8);
        assert(tools & (1 << tool));
        tools ^= (1 << tool);
        int res = call_one_instrument(interp, tstate, callargs, nargsf, tool, event);
        if (res == 0) {
            /* Nothing to do */
        }
        else if (res < 0) {
            /* error */
            err = -1;
            break;
        }
        else {
            /* DISABLE */
            if (!PY_MONITORING_IS_INSTRUMENTED_EVENT(event)) {
                PyErr_Format(PyExc_ValueError,
                              "Cannot disable %s events. Callback removed.",
                             event_names[event]);
                /* Clear tool to prevent infinite loop */
                Py_CLEAR(interp->monitoring_callables[tool][event]);
                err = -1;
                break;
            }
            else {
                LOCK_CODE(code);
                remove_tools(code, offset, event, 1 << tool);
                UNLOCK_CODE();
            }
        }
    }
    Py_DECREF(offset_obj);
    return err;
}

int
_Py_call_instrumentation(
    PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _Py_CODEUNIT *instr)
{
    PyObject *args[3] = { NULL, NULL, NULL };
    return call_instrumentation_vector(tstate, event, frame, instr, 2, args);
}

int
_Py_call_instrumentation_arg(
    PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _Py_CODEUNIT *instr, PyObject *arg)
{
    PyObject *args[4] = { NULL, NULL, NULL, arg };
    return call_instrumentation_vector(tstate, event, frame, instr, 3, args);
}

int
_Py_call_instrumentation_2args(
    PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _Py_CODEUNIT *instr, PyObject *arg0, PyObject *arg1)
{
    PyObject *args[5] = { NULL, NULL, NULL, arg0, arg1 };
    return call_instrumentation_vector(tstate, event, frame, instr, 4, args);
}

_Py_CODEUNIT *
_Py_call_instrumentation_jump(
    PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _Py_CODEUNIT *instr, _Py_CODEUNIT *target)
{
    assert(event == PY_MONITORING_EVENT_JUMP ||
           event == PY_MONITORING_EVENT_BRANCH);
    assert(frame->instr_ptr == instr);
    PyCodeObject *code = _PyFrame_GetCode(frame);
    int to = (int)(target - _PyCode_CODE(code));
    PyObject *to_obj = PyLong_FromLong(to * (int)sizeof(_Py_CODEUNIT));
    if (to_obj == NULL) {
        return NULL;
    }
    PyObject *args[4] = { NULL, NULL, NULL, to_obj };
    int err = call_instrumentation_vector(tstate, event, frame, instr, 3, args);
    Py_DECREF(to_obj);
    if (err) {
        return NULL;
    }
    if (frame->instr_ptr != instr) {
        /* The callback has caused a jump (by setting the line number) */
        return frame->instr_ptr;
    }
    return target;
}

static void
call_instrumentation_vector_protected(
    PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _Py_CODEUNIT *instr, Py_ssize_t nargs, PyObject *args[])
{
    assert(_PyErr_Occurred(tstate));
    PyObject *exc = _PyErr_GetRaisedException(tstate);
    int err = call_instrumentation_vector(tstate, event, frame, instr, nargs, args);
    if (err) {
        Py_XDECREF(exc);
    }
    else {
        _PyErr_SetRaisedException(tstate, exc);
    }
    assert(_PyErr_Occurred(tstate));
}

void
_Py_call_instrumentation_exc2(
    PyThreadState *tstate, int event,
    _PyInterpreterFrame *frame, _Py_CODEUNIT *instr, PyObject *arg0, PyObject *arg1)
{
    assert(_PyErr_Occurred(tstate));
    PyObject *args[5] = { NULL, NULL, NULL, arg0, arg1 };
    call_instrumentation_vector_protected(tstate, event, frame, instr, 4, args);
}


int
_Py_Instrumentation_GetLine(PyCodeObject *code, int index)
{
    _PyCoMonitoringData *monitoring = code->_co_monitoring;
    assert(monitoring != NULL);
    assert(monitoring->lines != NULL);
    assert(index >= code->_co_firsttraceable);
    assert(index < Py_SIZE(code));
    _PyCoLineInstrumentationData *line_data = &monitoring->lines[index];
    int8_t line_delta = line_data->line_delta;
    int line = compute_line(code, index, line_delta);
    return line;
}

int
_Py_call_instrumentation_line(PyThreadState *tstate, _PyInterpreterFrame* frame, _Py_CODEUNIT *instr, _Py_CODEUNIT *prev)
{
    PyCodeObject *code = _PyFrame_GetCode(frame);
    assert(tstate->tracing == 0);
    assert(debug_check_sanity(tstate->interp, code));
    int i = (int)(instr - _PyCode_CODE(code));

    _PyCoMonitoringData *monitoring = code->_co_monitoring;
    _PyCoLineInstrumentationData *line_data = &monitoring->lines[i];
    PyInterpreterState *interp = tstate->interp;
    int8_t line_delta = line_data->line_delta;
    int line = 0;

    if (line_delta == COMPUTED_LINE_LINENO_CHANGE) {
        // We know the line number must have changed, don't need to calculate
        // the line number for now because we might not need it.
        line = -1;
    } else {
        line = compute_line(code, i, line_delta);
        assert(line >= 0);
        assert(prev != NULL);
        int prev_index = (int)(prev - _PyCode_CODE(code));
        int prev_line = _Py_Instrumentation_GetLine(code, prev_index);
        if (prev_line == line) {
            int prev_opcode = _PyCode_CODE(code)[prev_index].op.code;
            /* RESUME and INSTRUMENTED_RESUME are needed for the operation of
             * instrumentation, so must never be hidden by an INSTRUMENTED_LINE.
             */
            if (prev_opcode != RESUME && prev_opcode != INSTRUMENTED_RESUME) {
                goto done;
            }
        }
    }

    uint8_t tools = code->_co_monitoring->line_tools != NULL ?
        code->_co_monitoring->line_tools[i] :
        (interp->monitors.tools[PY_MONITORING_EVENT_LINE] |
         code->_co_monitoring->local_monitors.tools[PY_MONITORING_EVENT_LINE]
        );
    /* Special case sys.settrace to avoid boxing the line number,
     * only to immediately unbox it. */
    if (tools & (1 << PY_MONITORING_SYS_TRACE_ID)) {
        if (tstate->c_tracefunc != NULL) {
            PyFrameObject *frame_obj = _PyFrame_GetFrameObject(frame);
            if (frame_obj == NULL) {
                return -1;
            }
            if (frame_obj->f_trace_lines) {
                /* Need to set tracing and what_event as if using
                 * the instrumentation call. */
                int old_what = tstate->what_event;
                tstate->what_event = PY_MONITORING_EVENT_LINE;
                tstate->tracing++;
                /* Call c_tracefunc directly, having set the line number. */
                Py_INCREF(frame_obj);
                if (line == -1 && line_delta > COMPUTED_LINE) {
                    /* Only assign f_lineno if it's easy to calculate, otherwise
                     * do lazy calculation by setting the f_lineno to 0.
                     */
                    line = compute_line(code, i, line_delta);
                }
                frame_obj->f_lineno = line;
                int err = tstate->c_tracefunc(tstate->c_traceobj, frame_obj, PyTrace_LINE, Py_None);
                frame_obj->f_lineno = 0;
                tstate->tracing--;
                tstate->what_event = old_what;
                Py_DECREF(frame_obj);
                if (err) {
                    return -1;
                }
            }
        }
        tools &= (255 - (1 << PY_MONITORING_SYS_TRACE_ID));
    }
    if (tools == 0) {
        goto done;
    }

    if (line == -1) {
        /* Need to calculate the line number now for monitoring events */
        line = compute_line(code, i, line_delta);
    }
    PyObject *line_obj = PyLong_FromLong(line);
    if (line_obj == NULL) {
        return -1;
    }
    PyObject *args[3] = { NULL, (PyObject *)code, line_obj };
    do {
        int tool = most_significant_bit(tools);
        assert(tool >= 0 && tool < PY_MONITORING_SYS_PROFILE_ID);
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
            LOCK_CODE(code);
            remove_line_tools(code, i, 1 << tool);
            UNLOCK_CODE();
        }
    } while (tools);
    Py_DECREF(line_obj);
    uint8_t original_opcode;
done:
    original_opcode = line_data->original_opcode;
    assert(original_opcode != 0);
    assert(original_opcode != INSTRUMENTED_LINE);
    assert(_PyOpcode_Deopt[original_opcode] == original_opcode);
    return original_opcode;
}

int
_Py_call_instrumentation_instruction(PyThreadState *tstate, _PyInterpreterFrame* frame, _Py_CODEUNIT *instr)
{
    PyCodeObject *code = _PyFrame_GetCode(frame);
    int offset = (int)(instr - _PyCode_CODE(code));
    _PyCoMonitoringData *instrumentation_data = code->_co_monitoring;
    assert(instrumentation_data->per_instruction_opcodes);
    int next_opcode = instrumentation_data->per_instruction_opcodes[offset];
    if (tstate->tracing) {
        return next_opcode;
    }
    assert(debug_check_sanity(tstate->interp, code));
    PyInterpreterState *interp = tstate->interp;
    uint8_t tools = instrumentation_data->per_instruction_tools != NULL ?
        instrumentation_data->per_instruction_tools[offset] :
        (interp->monitors.tools[PY_MONITORING_EVENT_INSTRUCTION] |
         code->_co_monitoring->local_monitors.tools[PY_MONITORING_EVENT_INSTRUCTION]
        );
    int bytes_offset = offset * (int)sizeof(_Py_CODEUNIT);
    PyObject *offset_obj = PyLong_FromLong(bytes_offset);
    if (offset_obj == NULL) {
        return -1;
    }
    PyObject *args[3] = { NULL, (PyObject *)code, offset_obj };
    while (tools) {
        int tool = most_significant_bit(tools);
        assert(tool >= 0 && tool < 8);
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
            LOCK_CODE(code);
            remove_per_instruction_tools(code, offset, 1 << tool);
            UNLOCK_CODE();
        }
    }
    Py_DECREF(offset_obj);
    assert(next_opcode != 0);
    return next_opcode;
}


PyObject *
_PyMonitoring_RegisterCallback(int tool_id, int event_id, PyObject *obj)
{
    PyInterpreterState *is = _PyInterpreterState_GET();
    assert(0 <= tool_id && tool_id < PY_MONITORING_TOOL_IDS);
    assert(0 <= event_id && event_id < _PY_MONITORING_EVENTS);
    PyObject *callback = _Py_atomic_exchange_ptr(&is->monitoring_callables[tool_id][event_id],
                                                 Py_XNewRef(obj));

    return callback;
}

static void
initialize_tools(PyCodeObject *code)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);
    uint8_t* tools = code->_co_monitoring->tools;

    assert(tools != NULL);
    int code_len = (int)Py_SIZE(code);
    for (int i = 0; i < code_len; i++) {
        _Py_CODEUNIT *instr = &_PyCode_CODE(code)[i];
        int opcode = instr->op.code;
        assert(opcode != ENTER_EXECUTOR);
        if (opcode == INSTRUMENTED_LINE) {
            opcode = code->_co_monitoring->lines[i].original_opcode;
        }
        if (opcode == INSTRUMENTED_INSTRUCTION) {
            opcode = code->_co_monitoring->per_instruction_opcodes[i];
        }
        bool instrumented = is_instrumented(opcode);
        if (instrumented) {
            opcode = DE_INSTRUMENT[opcode];
            assert(opcode != 0);
        }
        opcode = _PyOpcode_Deopt[opcode];
        if (opcode_has_event(opcode)) {
            if (instrumented) {
                int8_t event;
                if (opcode == RESUME) {
                    event = instr->op.arg != 0;
                }
                else {
                    event = EVENT_FOR_OPCODE[opcode];
                    assert(event > 0);
                }
                assert(event >= 0);
                assert(PY_MONITORING_IS_INSTRUMENTED_EVENT(event));
                tools[i] = code->_co_monitoring->active_monitors.tools[event];
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

#define NO_LINE -128

static void
initialize_lines(PyCodeObject *code)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);
    _PyCoLineInstrumentationData *line_data = code->_co_monitoring->lines;

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
        int opcode = _Py_GetBaseCodeUnit(code, i).op.code;
        int line = _PyCode_CheckLineNumber(i*(int)sizeof(_Py_CODEUNIT), &range);
        line_data[i].line_delta = compute_line_delta(code, i, line);
        int length = _PyInstruction_GetLength(code, i);
        switch (opcode) {
            case END_ASYNC_FOR:
            case END_FOR:
            case END_SEND:
            case RESUME:
                /* END_FOR cannot start a line, as it is skipped by FOR_ITER
                 * END_SEND cannot start a line, as it is skipped by SEND
                 * RESUME must not be instrumented with INSTRUMENT_LINE */
                line_data[i].original_opcode = 0;
                break;
            default:
                /* Set original_opcode to the opcode iff the instruction
                 * starts a line, and thus should be instrumented.
                 * This saves having to perform this check every time the
                 * we turn instrumentation on or off, and serves as a sanity
                 * check when debugging.
                 */
                if (line != current_line && line >= 0) {
                    line_data[i].original_opcode = opcode;
                    if (line_data[i].line_delta == COMPUTED_LINE) {
                        /* Label this line as a line with a line number change
                         * which could help the monitoring callback to quickly
                         * identify the line number change.
                         */
                        line_data[i].line_delta = COMPUTED_LINE_LINENO_CHANGE;
                    }
                }
                else {
                    line_data[i].original_opcode = 0;
                }
                current_line = line;
        }
        for (int j = 1; j < length; j++) {
            line_data[i+j].original_opcode = 0;
            line_data[i+j].line_delta = NO_LINE;
        }
        i += length;
    }
    for (int i = code->_co_firsttraceable; i < code_len; ) {
        _Py_CODEUNIT inst =_Py_GetBaseCodeUnit(code, i);
        int opcode = inst.op.code;
        int oparg = 0;
        while (opcode == EXTENDED_ARG) {
            oparg = (oparg << 8) | inst.op.arg;
            i++;
            inst =_Py_GetBaseCodeUnit(code, i);
            opcode = inst.op.code;
        }
        oparg = (oparg << 8) | inst.op.arg;
        i += _PyInstruction_GetLength(code, i);
        int target = -1;
        switch (opcode) {
            case POP_JUMP_IF_FALSE:
            case POP_JUMP_IF_TRUE:
            case POP_JUMP_IF_NONE:
            case POP_JUMP_IF_NOT_NONE:
            case JUMP_FORWARD:
            {
                target = i + oparg;
                break;
            }
            case FOR_ITER:
            case SEND:
            {
                /* Skip over END_FOR/END_SEND */
                target = i + oparg + 1;
                break;
            }
            case JUMP_BACKWARD:
            case JUMP_BACKWARD_NO_INTERRUPT:
            {
                target = i - oparg;
                break;
            }
            default:
                continue;
        }
        assert(target >= 0);
        if (line_data[target].line_delta != NO_LINE) {
            line_data[target].original_opcode = _Py_GetBaseCodeUnit(code, target).op.code;
            if (line_data[target].line_delta == COMPUTED_LINE_LINENO_CHANGE) {
                // If the line is a jump target, we are not sure if the line
                // number changes, so we set it to COMPUTED_LINE.
                line_data[target].line_delta = COMPUTED_LINE;
            }
        }
    }
    /* Scan exception table */
    unsigned char *start = (unsigned char *)PyBytes_AS_STRING(code->co_exceptiontable);
    unsigned char *end = start + PyBytes_GET_SIZE(code->co_exceptiontable);
    unsigned char *scan = start;
    while (scan < end) {
        int start_offset, size, handler;
        scan = parse_varint(scan, &start_offset);
        assert(start_offset >= 0 && start_offset < code_len);
        scan = parse_varint(scan, &size);
        assert(size >= 0 && start_offset+size <= code_len);
        scan = parse_varint(scan, &handler);
        assert(handler >= 0 && handler < code_len);
        int depth_and_lasti;
        scan = parse_varint(scan, &depth_and_lasti);
        int original_opcode = _Py_GetBaseCodeUnit(code, handler).op.code;
        /* Skip if not the start of a line.
         * END_ASYNC_FOR is a bit special as it marks the end of
         * an `async for` loop, which should not generate its own
         * line event. */
        if (line_data[handler].line_delta != NO_LINE &&
            original_opcode != END_ASYNC_FOR) {
            line_data[handler].original_opcode = original_opcode;
        }
    }
}

static void
initialize_line_tools(PyCodeObject *code, _Py_LocalMonitors *all_events)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);
    uint8_t *line_tools = code->_co_monitoring->line_tools;

    assert(line_tools != NULL);
    int code_len = (int)Py_SIZE(code);
    for (int i = 0; i < code_len; i++) {
        line_tools[i] = all_events->tools[PY_MONITORING_EVENT_LINE];
    }
}

static int
allocate_instrumentation_data(PyCodeObject *code)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);

    if (code->_co_monitoring == NULL) {
        code->_co_monitoring = PyMem_Malloc(sizeof(_PyCoMonitoringData));
        if (code->_co_monitoring == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        code->_co_monitoring->local_monitors = (_Py_LocalMonitors){ 0 };
        code->_co_monitoring->active_monitors = (_Py_LocalMonitors){ 0 };
        code->_co_monitoring->tools = NULL;
        code->_co_monitoring->lines = NULL;
        code->_co_monitoring->line_tools = NULL;
        code->_co_monitoring->per_instruction_opcodes = NULL;
        code->_co_monitoring->per_instruction_tools = NULL;
    }
    return 0;
}

static int
update_instrumentation_data(PyCodeObject *code, PyInterpreterState *interp)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);

    int code_len = (int)Py_SIZE(code);
    if (allocate_instrumentation_data(code)) {
        return -1;
    }
    // If the local monitors are out of date, clear them up
    _Py_LocalMonitors *local_monitors = &code->_co_monitoring->local_monitors;
    for (int i = 0; i < PY_MONITORING_TOOL_IDS; i++) {
        if (code->_co_monitoring->tool_versions[i] != interp->monitoring_tool_versions[i]) {
            for (int j = 0; j < _PY_MONITORING_LOCAL_EVENTS; j++) {
                local_monitors->tools[j] &= ~(1 << i);
            }
        }
    }

    _Py_LocalMonitors all_events = local_union(
        interp->monitors,
        code->_co_monitoring->local_monitors);
    bool multitools = multiple_tools(&all_events);
    if (code->_co_monitoring->tools == NULL && multitools) {
        code->_co_monitoring->tools = PyMem_Malloc(code_len);
        if (code->_co_monitoring->tools == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        initialize_tools(code);
    }
    if (all_events.tools[PY_MONITORING_EVENT_LINE]) {
        if (code->_co_monitoring->lines == NULL) {
            code->_co_monitoring->lines = PyMem_Malloc(code_len * sizeof(_PyCoLineInstrumentationData));
            if (code->_co_monitoring->lines == NULL) {
                PyErr_NoMemory();
                return -1;
            }
            initialize_lines(code);
        }
        if (multitools && code->_co_monitoring->line_tools == NULL) {
            code->_co_monitoring->line_tools = PyMem_Malloc(code_len);
            if (code->_co_monitoring->line_tools == NULL) {
                PyErr_NoMemory();
                return -1;
            }
            initialize_line_tools(code, &all_events);
        }
    }
    if (all_events.tools[PY_MONITORING_EVENT_INSTRUCTION]) {
        if (code->_co_monitoring->per_instruction_opcodes == NULL) {
            code->_co_monitoring->per_instruction_opcodes = PyMem_Malloc(code_len * sizeof(_PyCoLineInstrumentationData));
            if (code->_co_monitoring->per_instruction_opcodes == NULL) {
                PyErr_NoMemory();
                return -1;
            }
            // Initialize all of the instructions so if local events change while another thread is executing
            // we know what the original opcode was.
            for (int i = 0; i < code_len; i++) {
                int opcode = _PyCode_CODE(code)[i].op.code;
                code->_co_monitoring->per_instruction_opcodes[i] = _PyOpcode_Deopt[opcode];
            }
        }
        if (multitools && code->_co_monitoring->per_instruction_tools == NULL) {
            code->_co_monitoring->per_instruction_tools = PyMem_Malloc(code_len);
            if (code->_co_monitoring->per_instruction_tools == NULL) {
                PyErr_NoMemory();
                return -1;
            }
            for (int i = 0; i < code_len; i++) {
                code->_co_monitoring->per_instruction_tools[i] = 0;
            }
        }
    }
    return 0;
}

static int
force_instrument_lock_held(PyCodeObject *code, PyInterpreterState *interp)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);

#ifdef _Py_TIER2
    if (code->co_executors != NULL) {
        _PyCode_Clear_Executors(code);
    }
    _Py_Executors_InvalidateDependency(interp, code, 1);
#endif
    int code_len = (int)Py_SIZE(code);
    /* Exit early to avoid creating instrumentation
     * data for potential statically allocated code
     * objects.
     * See https://github.com/python/cpython/issues/108390 */
    if (code->co_flags & CO_NO_MONITORING_EVENTS) {
        return 0;
    }
    if (update_instrumentation_data(code, interp)) {
        return -1;
    }
    _Py_LocalMonitors active_events = local_union(
        interp->monitors,
        code->_co_monitoring->local_monitors);
    _Py_LocalMonitors new_events;
    _Py_LocalMonitors removed_events;

    bool restarted = interp->last_restart_version > code->_co_instrumentation_version;
    if (restarted) {
        removed_events = code->_co_monitoring->active_monitors;
        new_events = active_events;
    }
    else {
        removed_events = monitors_sub(code->_co_monitoring->active_monitors, active_events);
        new_events = monitors_sub(active_events, code->_co_monitoring->active_monitors);
        assert(monitors_are_empty(monitors_and(new_events, removed_events)));
    }
    code->_co_monitoring->active_monitors = active_events;
    if (monitors_are_empty(new_events) && monitors_are_empty(removed_events)) {
        goto done;
    }
    /* Insert instrumentation */
    for (int i = code->_co_firsttraceable; i < code_len; i+= _PyInstruction_GetLength(code, i)) {
        assert(_PyCode_CODE(code)[i].op.code != ENTER_EXECUTOR);
        _Py_CODEUNIT instr = _Py_GetBaseCodeUnit(code, i);
        CHECK(instr.op.code != 0);
        int base_opcode = instr.op.code;
        if (opcode_has_event(base_opcode)) {
            int8_t event;
            if (base_opcode == RESUME) {
                event = instr.op.arg > 0;
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
    }

    // GH-103845: We need to remove both the line and instruction instrumentation before
    // adding new ones, otherwise we may remove the newly added instrumentation.
    uint8_t removed_line_tools = removed_events.tools[PY_MONITORING_EVENT_LINE];
    uint8_t removed_per_instruction_tools = removed_events.tools[PY_MONITORING_EVENT_INSTRUCTION];

    if (removed_line_tools) {
        _PyCoLineInstrumentationData *line_data = code->_co_monitoring->lines;
        for (int i = code->_co_firsttraceable; i < code_len;) {
            if (line_data[i].original_opcode) {
                remove_line_tools(code, i, removed_line_tools);
            }
            i += _PyInstruction_GetLength(code, i);
        }
    }
    if (removed_per_instruction_tools) {
        for (int i = code->_co_firsttraceable; i < code_len;) {
            int opcode = _Py_GetBaseCodeUnit(code, i).op.code;
            if (opcode == RESUME || opcode == END_FOR) {
                i += _PyInstruction_GetLength(code, i);
                continue;
            }
            remove_per_instruction_tools(code, i, removed_per_instruction_tools);
            i += _PyInstruction_GetLength(code, i);
        }
    }
#ifdef INSTRUMENT_DEBUG
    sanity_check_instrumentation(code);
#endif

    uint8_t new_line_tools = new_events.tools[PY_MONITORING_EVENT_LINE];
    uint8_t new_per_instruction_tools = new_events.tools[PY_MONITORING_EVENT_INSTRUCTION];

    if (new_line_tools) {
        _PyCoLineInstrumentationData *line_data = code->_co_monitoring->lines;
        for (int i = code->_co_firsttraceable; i < code_len;) {
            if (line_data[i].original_opcode) {
                add_line_tools(code, i, new_line_tools);
            }
            i += _PyInstruction_GetLength(code, i);
        }
    }
    if (new_per_instruction_tools) {
        for (int i = code->_co_firsttraceable; i < code_len;) {
            int opcode = _Py_GetBaseCodeUnit(code, i).op.code;
            if (opcode == RESUME || opcode == END_FOR) {
                i += _PyInstruction_GetLength(code, i);
                continue;
            }
            add_per_instruction_tools(code, i, new_per_instruction_tools);
            i += _PyInstruction_GetLength(code, i);
        }
    }

done:
    FT_ATOMIC_STORE_UINTPTR_RELEASE(code->_co_instrumentation_version,
                                    global_version(interp));

#ifdef INSTRUMENT_DEBUG
    sanity_check_instrumentation(code);
#endif
    return 0;
}

static int
instrument_lock_held(PyCodeObject *code, PyInterpreterState *interp)
{
    ASSERT_WORLD_STOPPED_OR_LOCKED(code);

    if (is_version_up_to_date(code, interp)) {
        assert(
            interp->ceval.instrumentation_version == 0 ||
            instrumentation_cross_checks(interp, code)
        );
        return 0;
    }

    return force_instrument_lock_held(code, interp);
}

int
_Py_Instrument(PyCodeObject *code, PyInterpreterState *interp)
{
    int res;
    LOCK_CODE(code);
    res = instrument_lock_held(code, interp);
    UNLOCK_CODE();
    return res;
}

#define C_RETURN_EVENTS \
    ((1 << PY_MONITORING_EVENT_C_RETURN) | \
     (1 << PY_MONITORING_EVENT_C_RAISE))

#define C_CALL_EVENTS \
    (C_RETURN_EVENTS | (1 << PY_MONITORING_EVENT_CALL))


static int
instrument_all_executing_code_objects(PyInterpreterState *interp) {
    ASSERT_WORLD_STOPPED();

    _PyRuntimeState *runtime = &_PyRuntime;
    HEAD_LOCK(runtime);
    PyThreadState* ts = PyInterpreterState_ThreadHead(interp);
    HEAD_UNLOCK(runtime);
    while (ts) {
        _PyInterpreterFrame *frame = ts->current_frame;
        while (frame) {
            if (frame->owner != FRAME_OWNED_BY_CSTACK) {
                if (instrument_lock_held(_PyFrame_GetCode(frame), interp)) {
                    return -1;
                }
            }
            frame = frame->previous;
        }
        HEAD_LOCK(runtime);
        ts = PyThreadState_Next(ts);
        HEAD_UNLOCK(runtime);
    }
    return 0;
}

static void
set_events(_Py_GlobalMonitors *m, int tool_id, _PyMonitoringEventSet events)
{
    assert(0 <= tool_id && tool_id < PY_MONITORING_TOOL_IDS);
    for (int e = 0; e < _PY_MONITORING_UNGROUPED_EVENTS; e++) {
        uint8_t *tools = &m->tools[e];
        int active = (events >> e) & 1;
        *tools &= ~(1 << tool_id);
        *tools |= (active << tool_id);
    }
}

static void
set_local_events(_Py_LocalMonitors *m, int tool_id, _PyMonitoringEventSet events)
{
    assert(0 <= tool_id && tool_id < PY_MONITORING_TOOL_IDS);
    for (int e = 0; e < _PY_MONITORING_LOCAL_EVENTS; e++) {
        uint8_t *tools = &m->tools[e];
        int val = (events >> e) & 1;
        *tools &= ~(1 << tool_id);
        *tools |= (val << tool_id);
    }
}

static int
check_tool(PyInterpreterState *interp, int tool_id)
{
    if (tool_id < PY_MONITORING_SYS_PROFILE_ID &&
        interp->monitoring_tool_names[tool_id] == NULL)
    {
        PyErr_Format(PyExc_ValueError, "tool %d is not in use", tool_id);
        return -1;
    }
    return 0;
}

/* We share the eval-breaker with flags, so the monitoring
 * version goes in the top 24 bits */
#define MONITORING_VERSION_INCREMENT (1 << _PY_EVAL_EVENTS_BITS)

int
_PyMonitoring_SetEvents(int tool_id, _PyMonitoringEventSet events)
{
    assert(0 <= tool_id && tool_id < PY_MONITORING_TOOL_IDS);
    PyThreadState *tstate = _PyThreadState_GET();
    PyInterpreterState *interp = tstate->interp;
    assert(events < (1 << _PY_MONITORING_UNGROUPED_EVENTS));
    if (check_tool(interp, tool_id)) {
        return -1;
    }

    int res;
    _PyEval_StopTheWorld(interp);
    uint32_t existing_events = get_events(&interp->monitors, tool_id);
    if (existing_events == events) {
        res = 0;
        goto done;
    }
    set_events(&interp->monitors, tool_id, events);
    uint32_t new_version = global_version(interp) + MONITORING_VERSION_INCREMENT;
    if (new_version == 0) {
        PyErr_Format(PyExc_OverflowError, "events set too many times");
        res = -1;
        goto done;
    }
    set_global_version(tstate, new_version);
#ifdef _Py_TIER2
    _Py_Executors_InvalidateAll(interp, 1);
#endif
    res = instrument_all_executing_code_objects(interp);
done:
    _PyEval_StartTheWorld(interp);
    return res;
}

int
_PyMonitoring_SetLocalEvents(PyCodeObject *code, int tool_id, _PyMonitoringEventSet events)
{
    assert(0 <= tool_id && tool_id < PY_MONITORING_TOOL_IDS);
    PyInterpreterState *interp = _PyInterpreterState_GET();
    assert(events < (1 << _PY_MONITORING_LOCAL_EVENTS));
    if (code->_co_firsttraceable >= Py_SIZE(code)) {
        PyErr_Format(PyExc_SystemError, "cannot instrument shim code object '%U'", code->co_name);
        return -1;
    }
    if (check_tool(interp, tool_id)) {
        return -1;
    }

    int res;
    _PyEval_StopTheWorld(interp);
    if (allocate_instrumentation_data(code)) {
        res = -1;
        goto done;
    }

    code->_co_monitoring->tool_versions[tool_id] = interp->monitoring_tool_versions[tool_id];

    _Py_LocalMonitors *local = &code->_co_monitoring->local_monitors;
    uint32_t existing_events = get_local_events(local, tool_id);
    if (existing_events == events) {
        res = 0;
        goto done;
    }
    set_local_events(local, tool_id, events);

    res = force_instrument_lock_held(code, interp);

done:
    _PyEval_StartTheWorld(interp);
    return res;
}

int
_PyMonitoring_GetLocalEvents(PyCodeObject *code, int tool_id, _PyMonitoringEventSet *events)
{
    assert(0 <= tool_id && tool_id < PY_MONITORING_TOOL_IDS);
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (check_tool(interp, tool_id)) {
        return -1;
    }
    if (code->_co_monitoring == NULL) {
        *events = 0;
        return 0;
    }
    _Py_LocalMonitors *local = &code->_co_monitoring->local_monitors;
    *events = get_local_events(local, tool_id);
    return 0;
}

int _PyMonitoring_ClearToolId(int tool_id)
{
    assert(0 <= tool_id && tool_id < PY_MONITORING_TOOL_IDS);
    PyInterpreterState *interp = _PyInterpreterState_GET();

    for (int i = 0; i < _PY_MONITORING_EVENTS; i++) {
        PyObject *func = _PyMonitoring_RegisterCallback(tool_id, i, NULL);
        if (func != NULL) {
            Py_DECREF(func);
        }
    }

    if (_PyMonitoring_SetEvents(tool_id, 0) < 0) {
        return -1;
    }

    _PyEval_StopTheWorld(interp);
    uint32_t version = global_version(interp) + MONITORING_VERSION_INCREMENT;
    if (version == 0) {
        PyErr_Format(PyExc_OverflowError, "events set too many times");
        _PyEval_StartTheWorld(interp);
        return -1;
    }

    // monitoring_tool_versions[tool_id] is set to latest global version here to
    //   1. invalidate local events on all existing code objects
    //   2. be ready for the next call to set local events
    interp->monitoring_tool_versions[tool_id] = version;

    // Set the new global version so all the code objects can refresh the
    // instrumentation.
    set_global_version(_PyThreadState_GET(), version);
    int res = instrument_all_executing_code_objects(interp);
    _PyEval_StartTheWorld(interp);
    return res;
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
    if (tool_id < 0 || tool_id >= PY_MONITORING_SYS_PROFILE_ID) {
        PyErr_Format(PyExc_ValueError, "invalid tool %d (must be between 0 and 5)", tool_id);
        return -1;
    }
    return 0;
}

/*[clinic input]
monitoring.use_tool_id

    tool_id: int
    name: object
    /

[clinic start generated code]*/

static PyObject *
monitoring_use_tool_id_impl(PyObject *module, int tool_id, PyObject *name)
/*[clinic end generated code: output=30d76dc92b7cd653 input=ebc453761c621be1]*/
{
    if (check_valid_tool(tool_id))  {
        return NULL;
    }
    if (!PyUnicode_Check(name)) {
        PyErr_SetString(PyExc_ValueError, "tool name must be a str");
        return NULL;
    }
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (interp->monitoring_tool_names[tool_id] != NULL) {
        PyErr_Format(PyExc_ValueError, "tool %d is already in use", tool_id);
        return NULL;
    }
    interp->monitoring_tool_names[tool_id] = Py_NewRef(name);
    Py_RETURN_NONE;
}

/*[clinic input]
monitoring.clear_tool_id

    tool_id: int
    /

[clinic start generated code]*/

static PyObject *
monitoring_clear_tool_id_impl(PyObject *module, int tool_id)
/*[clinic end generated code: output=04defc23470b1be7 input=af643d6648a66163]*/
{
    if (check_valid_tool(tool_id))  {
        return NULL;
    }

    PyInterpreterState *interp = _PyInterpreterState_GET();

    if (interp->monitoring_tool_names[tool_id] != NULL) {
        if (_PyMonitoring_ClearToolId(tool_id) < 0) {
            return NULL;
        }
    }

    Py_RETURN_NONE;
}

/*[clinic input]
monitoring.free_tool_id

    tool_id: int
    /

[clinic start generated code]*/

static PyObject *
monitoring_free_tool_id_impl(PyObject *module, int tool_id)
/*[clinic end generated code: output=86c2d2a1219a8591 input=a23fb6be3a8618e9]*/
{
    if (check_valid_tool(tool_id))  {
        return NULL;
    }
    PyInterpreterState *interp = _PyInterpreterState_GET();

    if (interp->monitoring_tool_names[tool_id] != NULL) {
        if (_PyMonitoring_ClearToolId(tool_id) < 0) {
            return NULL;
        }
    }

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
    PyInterpreterState *interp = _PyInterpreterState_GET();
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
        PyErr_SetString(PyExc_ValueError, "The callback can only be set for one event at a time");
        return NULL;
    }
    int event_id = _Py_bit_length(event)-1;
    if (event_id < 0 || event_id >= _PY_MONITORING_EVENTS) {
        PyErr_Format(PyExc_ValueError, "invalid event %d", event);
        return NULL;
    }
    if (PySys_Audit("sys.monitoring.register_callback", "O", func) < 0) {
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
monitoring.get_events -> int

    tool_id: int
    /

[clinic start generated code]*/

static int
monitoring_get_events_impl(PyObject *module, int tool_id)
/*[clinic end generated code: output=4450cc13f826c8c0 input=a64b238f76c4b2f7]*/
{
    if (check_valid_tool(tool_id))  {
        return -1;
    }
    _Py_GlobalMonitors *m = &_PyInterpreterState_GET()->monitors;
    _PyMonitoringEventSet event_set = get_events(m, tool_id);
    return event_set;
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
    if (event_set < 0 || event_set >= (1 << _PY_MONITORING_EVENTS)) {
        PyErr_Format(PyExc_ValueError, "invalid event set 0x%x", event_set);
        return NULL;
    }
    if ((event_set & C_RETURN_EVENTS) && (event_set & C_CALL_EVENTS) != C_CALL_EVENTS) {
        PyErr_Format(PyExc_ValueError, "cannot set C_RETURN or C_RAISE events independently");
        return NULL;
    }
    event_set &= ~C_RETURN_EVENTS;
    if (_PyMonitoring_SetEvents(tool_id, event_set)) {
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
monitoring.get_local_events -> int

    tool_id: int
    code: object
    /

[clinic start generated code]*/

static int
monitoring_get_local_events_impl(PyObject *module, int tool_id,
                                 PyObject *code)
/*[clinic end generated code: output=d3e92c1c9c1de8f9 input=bb0f927530386a94]*/
{
    if (!PyCode_Check(code)) {
        PyErr_Format(
            PyExc_TypeError,
            "code must be a code object"
        );
        return -1;
    }
    if (check_valid_tool(tool_id))  {
        return -1;
    }
    _PyMonitoringEventSet event_set = 0;
    _PyCoMonitoringData *data = ((PyCodeObject *)code)->_co_monitoring;
    if (data != NULL) {
        for (int e = 0; e < _PY_MONITORING_LOCAL_EVENTS; e++) {
            if ((data->local_monitors.tools[e] >> tool_id) & 1) {
                event_set |= (1 << e);
            }
        }
    }
    return event_set;
}

/*[clinic input]
monitoring.set_local_events

    tool_id: int
    code: object
    event_set: int
    /

[clinic start generated code]*/

static PyObject *
monitoring_set_local_events_impl(PyObject *module, int tool_id,
                                 PyObject *code, int event_set)
/*[clinic end generated code: output=68cc755a65dfea99 input=5655ecd78d937a29]*/
{
    if (!PyCode_Check(code)) {
        PyErr_Format(
            PyExc_TypeError,
            "code must be a code object"
        );
        return NULL;
    }
    if (check_valid_tool(tool_id))  {
        return NULL;
    }
    if ((event_set & C_RETURN_EVENTS) && (event_set & C_CALL_EVENTS) != C_CALL_EVENTS) {
        PyErr_Format(PyExc_ValueError, "cannot set C_RETURN or C_RAISE events independently");
        return NULL;
    }
    event_set &= ~C_RETURN_EVENTS;
    if (event_set < 0 || event_set >= (1 << _PY_MONITORING_LOCAL_EVENTS)) {
        PyErr_Format(PyExc_ValueError, "invalid local event set 0x%x", event_set);
        return NULL;
    }

    if (_PyMonitoring_SetLocalEvents((PyCodeObject*)code, tool_id, event_set)) {
        return NULL;
    }
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
    PyThreadState *tstate = _PyThreadState_GET();
    PyInterpreterState *interp = tstate->interp;

    _PyEval_StopTheWorld(interp);
    uint32_t restart_version = global_version(interp) + MONITORING_VERSION_INCREMENT;
    uint32_t new_version = restart_version + MONITORING_VERSION_INCREMENT;
    if (new_version <= MONITORING_VERSION_INCREMENT) {
        _PyEval_StartTheWorld(interp);
        PyErr_Format(PyExc_OverflowError, "events set too many times");
        return NULL;
    }
    interp->last_restart_version = restart_version;
    set_global_version(tstate, new_version);
    int res = instrument_all_executing_code_objects(interp);
    _PyEval_StartTheWorld(interp);

    if (res) {
        return NULL;
    }
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

/*[clinic input]
monitoring._all_events
[clinic start generated code]*/

static PyObject *
monitoring__all_events_impl(PyObject *module)
/*[clinic end generated code: output=6b7581e2dbb690f6 input=62ee9672c17b7f0e]*/
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    PyObject *res = PyDict_New();
    if (res == NULL) {
        return NULL;
    }
    for (int e = 0; e < _PY_MONITORING_UNGROUPED_EVENTS; e++) {
        uint8_t tools = interp->monitors.tools[e];
        if (tools == 0) {
            continue;
        }
        PyObject *tools_obj = PyLong_FromLong(tools);
        assert(tools_obj != NULL);
        int err = PyDict_SetItemString(res, event_names[e], tools_obj);
        Py_DECREF(tools_obj);
        if (err < 0) {
            Py_DECREF(res);
            return NULL;
        }
    }
    return res;
}

static PyMethodDef methods[] = {
    MONITORING_USE_TOOL_ID_METHODDEF
    MONITORING_CLEAR_TOOL_ID_METHODDEF
    MONITORING_FREE_TOOL_ID_METHODDEF
    MONITORING_GET_TOOL_METHODDEF
    MONITORING_REGISTER_CALLBACK_METHODDEF
    MONITORING_GET_EVENTS_METHODDEF
    MONITORING_SET_EVENTS_METHODDEF
    MONITORING_GET_LOCAL_EVENTS_METHODDEF
    MONITORING_SET_LOCAL_EVENTS_METHODDEF
    MONITORING_RESTART_EVENTS_METHODDEF
    MONITORING__ALL_EVENTS_METHODDEF
    {NULL, NULL}  // sentinel
};

static struct PyModuleDef monitoring_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "sys.monitoring",
    .m_size = -1, /* multiple "initialization" just copies the module dict. */
    .m_methods = methods,
};

PyObject *_Py_CreateMonitoringObject(void)
{
    PyObject *mod = _PyModule_CreateInitialized(&monitoring_module, PYTHON_API_VERSION);
    if (mod == NULL) {
        return NULL;
    }
    if (PyObject_SetAttrString(mod, "DISABLE", &_PyInstrumentation_DISABLE)) {
        goto error;
    }
    if (PyObject_SetAttrString(mod, "MISSING", &_PyInstrumentation_MISSING)) {
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
    for (int i = 0; i < _PY_MONITORING_EVENTS; i++) {
        if (add_power2_constant(events, event_names[i], i)) {
            goto error;
        }
    }
    err = PyObject_SetAttrString(events, "NO_EVENTS", _PyLong_GetZero());
    if (err) goto error;
    PyObject *val = PyLong_FromLong(PY_MONITORING_DEBUGGER_ID);
    err = PyObject_SetAttrString(mod, "DEBUGGER_ID", val);
    Py_DECREF(val);
    if (err) goto error;
    val = PyLong_FromLong(PY_MONITORING_COVERAGE_ID);
    err = PyObject_SetAttrString(mod, "COVERAGE_ID", val);
    Py_DECREF(val);
    if (err) goto error;
    val = PyLong_FromLong(PY_MONITORING_PROFILER_ID);
    err = PyObject_SetAttrString(mod, "PROFILER_ID", val);
    Py_DECREF(val);
    if (err) goto error;
    val = PyLong_FromLong(PY_MONITORING_OPTIMIZER_ID);
    err = PyObject_SetAttrString(mod, "OPTIMIZER_ID", val);
    Py_DECREF(val);
    if (err) goto error;
    return mod;
error:
    Py_DECREF(mod);
    return NULL;
}


static int
capi_call_instrumentation(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                          PyObject **args, Py_ssize_t nargs, int event)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyInterpreterState *interp = tstate->interp;

    uint8_t tools = state->active;
    assert(args[1] == NULL);
    args[1] = codelike;
    if (offset < 0) {
        PyErr_SetString(PyExc_ValueError, "offset must be non-negative");
        return -1;
    }
    if (event != PY_MONITORING_EVENT_LINE) {
        PyObject *offset_obj = PyLong_FromLong(offset);
        if (offset_obj == NULL) {
            return -1;
        }
        assert(args[2] == NULL);
        args[2] = offset_obj;
    }
    size_t nargsf = (size_t) nargs | PY_VECTORCALL_ARGUMENTS_OFFSET;
    PyObject **callargs = &args[1];
    int err = 0;

    while (tools) {
        int tool = most_significant_bit(tools);
        assert(tool >= 0 && tool < 8);
        assert(tools & (1 << tool));
        tools ^= (1 << tool);
        int res = call_one_instrument(interp, tstate, callargs, nargsf, tool, event);
        if (res == 0) {
            /* Nothing to do */
        }
        else if (res < 0) {
            /* error */
            err = -1;
            break;
        }
        else {
            /* DISABLE */
            if (!PY_MONITORING_IS_INSTRUMENTED_EVENT(event)) {
                PyErr_Format(PyExc_ValueError,
                             "Cannot disable %s events. Callback removed.",
                             event_names[event]);
                /* Clear tool to prevent infinite loop */
                Py_CLEAR(interp->monitoring_callables[tool][event]);
                err = -1;
                break;
            }
            else {
                state->active &= ~(1 << tool);
            }
        }
    }
    return err;
}

int
PyMonitoring_EnterScope(PyMonitoringState *state_array, uint64_t *version,
                         const uint8_t *event_types, Py_ssize_t length)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (global_version(interp) == *version) {
        return 0;
    }

    _Py_GlobalMonitors *m = &interp->monitors;
    for (Py_ssize_t i = 0; i < length; i++) {
        int event = event_types[i];
        state_array[i].active = m->tools[event];
    }
    *version = global_version(interp);
    return 0;
}

int
PyMonitoring_ExitScope(void)
{
    return 0;
}

int
_PyMonitoring_FirePyStartEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)
{
    assert(state->active);
    PyObject *args[3] = { NULL, NULL, NULL };
    return capi_call_instrumentation(state, codelike, offset, args, 2,
                                     PY_MONITORING_EVENT_PY_START);
}

int
_PyMonitoring_FirePyResumeEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)
{
    assert(state->active);
    PyObject *args[3] = { NULL, NULL, NULL };
    return capi_call_instrumentation(state, codelike, offset, args, 2,
                                     PY_MONITORING_EVENT_PY_RESUME);
}



int
_PyMonitoring_FirePyReturnEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                                PyObject* retval)
{
    assert(state->active);
    PyObject *args[4] = { NULL, NULL, NULL, retval };
    return capi_call_instrumentation(state, codelike, offset, args, 3,
                                     PY_MONITORING_EVENT_PY_RETURN);
}

int
_PyMonitoring_FirePyYieldEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                               PyObject* retval)
{
    assert(state->active);
    PyObject *args[4] = { NULL, NULL, NULL, retval };
    return capi_call_instrumentation(state, codelike, offset, args, 3,
                                     PY_MONITORING_EVENT_PY_YIELD);
}

int
_PyMonitoring_FireCallEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                            PyObject* callable, PyObject *arg0)
{
    assert(state->active);
    PyObject *args[5] = { NULL, NULL, NULL, callable, arg0 };
    return capi_call_instrumentation(state, codelike, offset, args, 4,
                                     PY_MONITORING_EVENT_CALL);
}

int
_PyMonitoring_FireLineEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                            int lineno)
{
    assert(state->active);
    PyObject *lno = PyLong_FromLong(lineno);
    if (lno == NULL) {
        return -1;
    }
    PyObject *args[3] = { NULL, NULL, lno };
    int res= capi_call_instrumentation(state, codelike, offset, args, 2,
                                       PY_MONITORING_EVENT_LINE);
    Py_DECREF(lno);
    return res;
}

int
_PyMonitoring_FireJumpEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                            PyObject *target_offset)
{
    assert(state->active);
    PyObject *args[4] = { NULL, NULL, NULL, target_offset };
    return capi_call_instrumentation(state, codelike, offset, args, 3,
                                     PY_MONITORING_EVENT_JUMP);
}

int
_PyMonitoring_FireBranchEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                              PyObject *target_offset)
{
    assert(state->active);
    PyObject *args[4] = { NULL, NULL, NULL, target_offset };
    return capi_call_instrumentation(state, codelike, offset, args, 3,
                                     PY_MONITORING_EVENT_BRANCH);
}

int
_PyMonitoring_FireCReturnEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset,
                               PyObject *retval)
{
    assert(state->active);
    PyObject *args[4] = { NULL, NULL, NULL, retval };
    return capi_call_instrumentation(state, codelike, offset, args, 3,
                                     PY_MONITORING_EVENT_C_RETURN);
}

static inline int
exception_event_setup(PyObject **exc, int event) {
    *exc = PyErr_GetRaisedException();
    if (*exc == NULL) {
        PyErr_Format(PyExc_ValueError,
                     "Firing event %d with no exception set",
                     event);
        return -1;
    }
    return 0;
}


static inline int
exception_event_teardown(int err, PyObject *exc) {
    if (err == 0) {
        PyErr_SetRaisedException(exc);
    }
    else {
        assert(PyErr_Occurred());
        Py_XDECREF(exc);
    }
    return err;
}

int
_PyMonitoring_FirePyThrowEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)
{
    int event = PY_MONITORING_EVENT_PY_THROW;
    assert(state->active);
    PyObject *exc;
    if (exception_event_setup(&exc, event) < 0) {
        return -1;
    }
    PyObject *args[4] = { NULL, NULL, NULL, exc };
    int err = capi_call_instrumentation(state, codelike, offset, args, 3, event);
    return exception_event_teardown(err, exc);
}

int
_PyMonitoring_FireRaiseEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)
{
    int event = PY_MONITORING_EVENT_RAISE;
    assert(state->active);
    PyObject *exc;
    if (exception_event_setup(&exc, event) < 0) {
        return -1;
    }
    PyObject *args[4] = { NULL, NULL, NULL, exc };
    int err = capi_call_instrumentation(state, codelike, offset, args, 3, event);
    return exception_event_teardown(err, exc);
}

int
_PyMonitoring_FireCRaiseEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)
{
    int event = PY_MONITORING_EVENT_C_RAISE;
    assert(state->active);
    PyObject *exc;
    if (exception_event_setup(&exc, event) < 0) {
        return -1;
    }
    PyObject *args[4] = { NULL, NULL, NULL, exc };
    int err = capi_call_instrumentation(state, codelike, offset, args, 3, event);
    return exception_event_teardown(err, exc);
}

int
_PyMonitoring_FireReraiseEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)
{
    int event = PY_MONITORING_EVENT_RERAISE;
    assert(state->active);
    PyObject *exc;
    if (exception_event_setup(&exc, event) < 0) {
        return -1;
    }
    PyObject *args[4] = { NULL, NULL, NULL, exc };
    int err = capi_call_instrumentation(state, codelike, offset, args, 3, event);
    return exception_event_teardown(err, exc);
}

int
_PyMonitoring_FireExceptionHandledEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)
{
    int event = PY_MONITORING_EVENT_EXCEPTION_HANDLED;
    assert(state->active);
    PyObject *exc;
    if (exception_event_setup(&exc, event) < 0) {
        return -1;
    }
    PyObject *args[4] = { NULL, NULL, NULL, exc };
    int err = capi_call_instrumentation(state, codelike, offset, args, 3, event);
    return exception_event_teardown(err, exc);
}

int
_PyMonitoring_FirePyUnwindEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)
{
    int event = PY_MONITORING_EVENT_PY_UNWIND;
    assert(state->active);
    PyObject *exc;
    if (exception_event_setup(&exc, event) < 0) {
        return -1;
    }
    PyObject *args[4] = { NULL, NULL, NULL, exc };
    int err = capi_call_instrumentation(state, codelike, offset, args, 3, event);
    return exception_event_teardown(err, exc);
}

int
_PyMonitoring_FireStopIterationEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset, PyObject *value)
{
    int event = PY_MONITORING_EVENT_STOP_ITERATION;
    assert(state->active);
    assert(!PyErr_Occurred());
    PyErr_SetObject(PyExc_StopIteration, value);
    PyObject *exc;
    if (exception_event_setup(&exc, event) < 0) {
        return -1;
    }
    PyObject *args[4] = { NULL, NULL, NULL, exc };
    int err = capi_call_instrumentation(state, codelike, offset, args, 3, event);
    Py_DECREF(exc);
    return exception_event_teardown(err, NULL);
}
