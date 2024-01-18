#include "Python.h"
#include "opcode.h"
#include "pycore_interp.h"
#include "pycore_opcode_metadata.h"
#include "pycore_opcode_utils.h"
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_uop_metadata.h"
#include "pycore_dict.h"
#include "pycore_long.h"
#include "cpython/optimizer.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "pycore_optimizer.h"

static int
builtins_watcher_callback(PyDict_WatchEvent event, PyObject* dict,
                          PyObject* key, PyObject* new_value)
{
    if (event != PyDict_EVENT_CLONED) {
        _Py_Executors_InvalidateAll(_PyInterpreterState_GET());
    }
    return 0;
}

static int
globals_watcher_callback(PyDict_WatchEvent event, PyObject* dict,
                         PyObject* key, PyObject* new_value)
{
    if (event != PyDict_EVENT_CLONED) {
        _Py_Executors_InvalidateDependency(_PyInterpreterState_GET(), dict);
    }
    /* TO DO -- Mark the dict, as a warning for future optimization attempts */
    return 0;
}


static void
global_to_const(_PyUOpInstruction *inst, PyObject *obj)
{
    assert(inst->opcode == _LOAD_GLOBAL_MODULE || inst->opcode == _LOAD_GLOBAL_BUILTINS);
    assert(PyDict_CheckExact(obj));
    PyDictObject *dict = (PyDictObject *)obj;
    assert(dict->ma_keys->dk_kind == DICT_KEYS_UNICODE);
    PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(dict->ma_keys);
    assert(inst->operand < dict->ma_used);
    assert(inst->operand <= UINT16_MAX);
    PyObject *res = entries[inst->operand].me_value;
    if (res == NULL) {
        return;
    }
    if (_Py_IsImmortal(res)) {
        inst->opcode = (inst->oparg & 1) ? _LOAD_CONST_INLINE_BORROW_WITH_NULL : _LOAD_CONST_INLINE_BORROW;
    }
    else {
        inst->opcode = (inst->oparg & 1) ? _LOAD_CONST_INLINE_WITH_NULL : _LOAD_CONST_INLINE;
    }
    inst->operand = (uint64_t)res;
}

static int
check_globals_version(_PyUOpInstruction *inst, PyObject *obj)
{
    if (!PyDict_CheckExact(obj)) {
        return -1;
    }
    PyDictObject *dict = (PyDictObject *)obj;
    if (dict->ma_keys->dk_version != inst->operand) {
        return -1;
    }
    return 0;
}

static void
remove_globals(_PyUOpInstruction *buffer, int buffer_size,
               _PyInterpreterFrame *frame, _PyBloomFilter *dependencies)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    PyFunctionObject *func = (PyFunctionObject *)frame->f_funcobj;
    assert(PyFunction_Check(func));
    PyObject *builtins = frame->f_builtins;
    PyObject *globals = frame->f_globals;
    assert(func->func_builtins == builtins);
    assert(func->func_globals == globals);
    /* In order to treat globals as constants, we need to
     * know that the globals dict is the one we expected, and
     * that it hasn't changed
     * In order to treat builtins as constants,  we need to
     * know that the builtins dict is the one we expected, and
     * that it hasn't changed and that the global dictionary's
     * keys have not changed */
    uint32_t builtins_checked = 0;
    uint32_t builtins_watched = 0;
    uint32_t globals_checked = 0;
    uint32_t globals_watched = 0;
    if (interp->dict_state.watchers[0] == NULL) {
        interp->dict_state.watchers[0] = builtins_watcher_callback;
    }
    if (interp->dict_state.watchers[1] == NULL) {
        interp->dict_state.watchers[1] = globals_watcher_callback;
    }
    for (int pc = 0; pc < buffer_size; pc++) {
        _PyUOpInstruction *inst = &buffer[pc];
        int opcode = inst->opcode;
        switch(opcode) {
            case _GUARD_BUILTINS_VERSION:
                if (check_globals_version(inst, builtins)) {
                    continue;
                }
                if ((builtins_watched & 1) == 0) {
                    PyDict_Watch(0, builtins);
                    builtins_watched = 1;
                }
                if (builtins_checked & 1) {
                    buffer[pc].opcode = NOP;
                }
                else {
                    buffer[pc].opcode = _CHECK_BUILTINS;
                    buffer[pc].operand = (uintptr_t)builtins;
                    builtins_checked |= 1;
                }
                break;
            case _GUARD_GLOBALS_VERSION:
                if (check_globals_version(inst, globals)) {
                    continue;
                }
                if ((globals_watched & 1) == 0) {
                    PyDict_Watch(1, globals);
                    _Py_BloomFilter_Add(dependencies, globals);
                    globals_watched |= 1;
                }
                if (globals_checked & 1) {
                    buffer[pc].opcode = NOP;
                }
                else {
                    buffer[pc].opcode = _CHECK_GLOBALS;
                    buffer[pc].operand = (uintptr_t)globals;
                    globals_checked |= 1;
                }
                break;
            case _LOAD_GLOBAL_BUILTINS:
                if (globals_checked & builtins_checked & globals_watched & builtins_watched & 1) {
                    global_to_const(inst, builtins);
                }
                break;
            case _LOAD_GLOBAL_MODULE:
                if (globals_checked & globals_watched & 1) {
                    global_to_const(inst, globals);
                }
                break;
            case _JUMP_TO_TOP:
            case _EXIT_TRACE:
                goto done;
             case _PUSH_FRAME:
             {
                 globals_checked <<= 1;
                 globals_watched <<= 1;
                 builtins_checked <<= 1;
                 builtins_watched <<= 1;
                 PyFunctionObject *func = (PyFunctionObject *)buffer[pc].operand;
                 assert(PyFunction_Check(func));
                 globals = func->func_globals;
                 builtins = func->func_builtins;
                 break;
             }
             case _POP_FRAME:
                 globals_checked >>= 1;
                 globals_watched >>= 1;
                 builtins_checked >>= 1;
                 builtins_watched >>= 1;
                 PyFunctionObject *func = (PyFunctionObject *)buffer[pc].operand;
                 assert(PyFunction_Check(func));
                 globals = func->func_globals;
                 builtins = func->func_builtins;
                 break;
        }
    }
    done:
        return;
}

static void
peephole_opt(_PyInterpreterFrame *frame, _PyUOpInstruction *buffer, int buffer_size)
{
    PyCodeObject *co = (PyCodeObject *)frame->f_executable;
    for (int pc = 0; pc < buffer_size; pc++) {
        int opcode = buffer[pc].opcode;
        switch(opcode) {
            case _LOAD_CONST: {
                assert(co != NULL);
                PyObject *val = PyTuple_GET_ITEM(co->co_consts, buffer[pc].oparg);
                buffer[pc].opcode = _Py_IsImmortal(val) ? _LOAD_CONST_INLINE_BORROW : _LOAD_CONST_INLINE;
                buffer[pc].operand = (uintptr_t)val;
                break;
            }
            case _CHECK_PEP_523:
            {
                /* Setting the eval frame function invalidates
                 * all executors, so no need to check dynamically */
                if (_PyInterpreterState_GET()->eval_frame == NULL) {
                    buffer[pc].opcode = _NOP;
                }
                break;
            }
            case _PUSH_FRAME:
            case _POP_FRAME:
                co = (PyCodeObject *)buffer[pc].operand;
                break;
            case _JUMP_TO_TOP:
            case _EXIT_TRACE:
                return;
        }
    }
}

static void
remove_unneeded_uops(_PyUOpInstruction *buffer, int buffer_size)
{
    int last_set_ip = -1;
    bool maybe_invalid = false;
    for (int pc = 0; pc < buffer_size; pc++) {
        int opcode = buffer[pc].opcode;
        if (opcode == _SET_IP) {
            buffer[pc].opcode = NOP;
            last_set_ip = pc;
        }
        else if (opcode == _CHECK_VALIDITY) {
            if (maybe_invalid) {
                maybe_invalid = false;
            }
            else {
                buffer[pc].opcode = NOP;
            }
        }
        else if (opcode == _JUMP_TO_TOP || opcode == _EXIT_TRACE) {
            break;
        }
        else {
            if (_PyUop_Flags[opcode] & HAS_ESCAPES_FLAG) {
                maybe_invalid = true;
                if (last_set_ip >= 0) {
                    buffer[last_set_ip].opcode = _SET_IP;
                }
            }
            if ((_PyUop_Flags[opcode] & HAS_ERROR_FLAG) || opcode == _PUSH_FRAME) {
                if (last_set_ip >= 0) {
                    buffer[last_set_ip].opcode = _SET_IP;
                }
            }
        }
    }
}


int
_Py_uop_analyze_and_optimize(
    _PyInterpreterFrame *frame,
    _PyUOpInstruction *buffer,
    int buffer_size,
    int curr_stacklen
)
{
    peephole_opt(frame, buffer, buffer_size);
    remove_unneeded_uops(buffer, buffer_size);
    return 0;
}
