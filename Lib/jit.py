"""The Justin(time) template JIT for CPython 3.12, based on copy-and-patch.

>>> import jit
>>> jit.enable(verbose=True, warmups=42)
"""

import ctypes
import dis
import sys
import types

import Tools.justin.build

def enable(*, verbose: bool = False, warmup: int = 1 << 8) -> None:
    sys.monitoring.use_tool_id(sys.monitoring.OPTIMIZER_ID, __name__)
    sys.monitoring.set_events(sys.monitoring.OPTIMIZER_ID, sys.monitoring.events.JUMP)
    sys.monitoring.register_callback(sys.monitoring.OPTIMIZER_ID, sys.monitoring.events.JUMP, _trace_jump)
    sys.monitoring.register_callback(sys.monitoring.OPTIMIZER_ID, sys.monitoring.events.INSTRUCTION, _trace_instruction)
    global _VERBOSE, _WARMUP
    _VERBOSE = verbose
    _WARMUP = warmup

_co_code_adaptive = {}
_traces = {}
_lines = {}
_warmups = {}

_SUPPORTED_OPS = frozenset(Tools.justin.build.Engine._OPS)
_OFFSETOF_CO_CODE_ADAPTIVE = 192
_INSTRUMENTED_JUMP_BACKWARD = dis._all_opmap["INSTRUMENTED_JUMP_BACKWARD"]
_JUMP_BACKWARD = dis._all_opmap["JUMP_BACKWARD"]
_JUMP_BACKWARD_INTO_TRACE = dis._all_opmap["JUMP_BACKWARD_INTO_TRACE"]

_py_codeunit_p = ctypes.POINTER(ctypes.c_uint16)
_py_codeunit_pp = ctypes.POINTER(_py_codeunit_p)

_compile_trace = ctypes.pythonapi._PyJIT_CompileTrace
_compile_trace.argtypes = (ctypes.c_int, _py_codeunit_pp)
_compile_trace.restype = ctypes.POINTER(ctypes.c_ubyte)

_SUPERINSTRUCTIONS = {
    "CALL_NO_KW_LIST_APPEND",
    "LOAD_CONST__LOAD_FAST",
    "LOAD_FAST__LOAD_CONST",
    "LOAD_FAST__LOAD_FAST",
    "STORE_FAST__LOAD_FAST",
    "STORE_FAST__STORE_FAST",
}

def _stderr(*args: object) -> None:
    if _VERBOSE:
        print("JIT:", *args, file=sys.stderr, flush=True)

def _format_range(code: types.CodeType, i: int, j: int) -> str:
    if code not in _lines:
        _lines[code] = [lineno for lineno, _, _, _ in code.co_positions()]
    lines = list(filter(None, _lines[code][i // 2: j // 2]))
    lo = min(lines, default=None)
    hi = max(lines, default=None)
    if not lo or not hi:
        return code.co_filename
    return f"{code.co_filename}:{lo}-{hi}"

def _trace_jump(code: types.CodeType, i: int, j: int) -> object:
    if j <= i:
        key = (code, i)
        warmups = _warmups[key] = _warmups.get(key, 0) + 1
        if warmups <= _WARMUP:
            # _stderr(f"- Warming up {_format_range(code, j, i)} ({warmups}/{_WARMUP}).") 
            return
        _co_code_adaptive[code] = code._co_code_adaptive
        sys.monitoring.set_local_events(sys.monitoring.OPTIMIZER_ID, code, sys.monitoring.events.INSTRUCTION | sys.monitoring.events.JUMP)
        if code in _traces:
            _stderr(f"  - Found inner loop!") 
        _traces[code] = i, []
        _stderr(f"- Recording loop at {_format_range(code, j, i)}:")
    return sys.monitoring.DISABLE

def _trace_instruction(code: types.CodeType, i: int) -> object:
    jump, trace = _traces[code]
    trace.append(i)
    if i == jump:
        if _compile(code, _co_code_adaptive[code], trace):
            _stderr("  - Succeeded!")
        else:
            _stderr("  - Failed!")
        sys.monitoring.set_local_events(sys.monitoring.OPTIMIZER_ID, code, sys.monitoring.events.JUMP)
        del _traces[code]
    return sys.monitoring.DISABLE

def _compile(code: types.CodeType, co_code_adaptive: bytes, traced: list[int]) -> bool:
    traced = _remove_superinstructions(co_code_adaptive, traced)
    j = traced[-1]
    first_instr = id(code) + _OFFSETOF_CO_CODE_ADAPTIVE
    buff = ctypes.cast(first_instr, _py_codeunit_p)
    ctypes.memmove(buff, (ctypes.c_uint16 * (len(co_code_adaptive) // 2)).from_buffer_copy(co_code_adaptive), len(co_code_adaptive))
    c_traced_type = _py_codeunit_p * len(traced)
    c_traced = c_traced_type()
    c_traced[:] = [ctypes.cast(first_instr + i, _py_codeunit_p) for i in traced]
    jump = ctypes.cast(ctypes.c_void_p(first_instr + j), ctypes.POINTER(ctypes.c_uint8))
    assert jump.contents.value == _INSTRUMENTED_JUMP_BACKWARD
    jump.contents.value = _JUMP_BACKWARD
    buffer = ctypes.cast(_compile_trace(len(traced), c_traced), ctypes.c_void_p)
    if buffer.value is None:
        # for i in traced[:-1]:
        #     opname = dis._all_opname[co_code_adaptive[i]]
        #     if opname not in _SUPPORTED_OPS:
        #         _stderr(f"  - Unsupported opcode {opname}!")
        return False
    jump.contents.value = _JUMP_BACKWARD_INTO_TRACE
    cache = ctypes.c_void_p(first_instr + j + 4)
    ctypes.cast(cache, ctypes.POINTER(ctypes.c_uint64)).contents.value = buffer.value
    return True


def _remove_superinstructions(co_code_adaptive: bytes, trace: list[int]) -> list[int] | None:
    out = []
    iter_trace = iter(trace)
    for i in iter_trace:
        out.append(i)
        if dis._all_opname[co_code_adaptive[i]] in _SUPERINSTRUCTIONS:
            next(iter_trace)
    return out
