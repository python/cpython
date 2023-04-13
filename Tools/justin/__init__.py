"""The Justin(time) template JIT for CPython 3.12, based on copy-and-patch."""

import contextlib
import ctypes
import dis
import functools
import sys
import types
import typing

_fake_code = {}
_traces = {}
_positions = {}

set_local_events = sys.monitoring.set_local_events
OPTIMIZER_ID = sys.monitoring.OPTIMIZER_ID
JUMP = sys.monitoring.events.JUMP
INSTRUCTION = sys.monitoring.events.INSTRUCTION
DISABLE = sys.monitoring.DISABLE

VERBOSE = True

@contextlib.contextmanager
def _trace(f):
    code = f.__code__
    fake_code = _fake_code[code] = code.replace(co_code=code._co_code_adaptive)
    try:
        if VERBOSE:
            print(f"JUSTIN: - Tracing {f.__qualname__}:")
        set_local_events(OPTIMIZER_ID, code, JUMP)
        yield
    finally:
        if VERBOSE:
            print(f"JUSTIN:   - Done tracing {f.__qualname__}!")
        set_local_events(OPTIMIZER_ID, code, 0)
    f.__code__ = fake_code

def _trace_jump(code: types.CodeType, i: int, j: int):
    if j <= i:
        set_local_events(OPTIMIZER_ID, code, INSTRUCTION | JUMP)
        if VERBOSE and code in _traces:
            print(f"JUSTIN:     - Found inner loop!") 
        _traces[code] = i, []
        if VERBOSE:
            if code not in _positions:
                _positions[code] = [lineno for lineno, _, _, _ in code.co_positions()]
            lines = [lineno for lineno in _positions[code][j // 2: i // 2] if lineno is not None]
            lo = min(lines)
            hi = max(lines)
            print(f"JUSTIN:   - Recording loop at {code.co_filename}:{lo}-{hi}:")
    return DISABLE

def _trace_instruction(code: types.CodeType, i: int):
    jump, trace = _traces[code]
    trace.append(i)
    if i == jump:
        set_local_events(OPTIMIZER_ID, code, JUMP)
        _compile(_fake_code[code], trace)
        if VERBOSE:
            print("JUSTIN:     - Done!")
        del _traces[code]

def trace(f, *, warmup: int = 2):
    @functools.wraps(f)
    def wrapper(*args, **kwargs):
        # This needs to be *fast*.
        nonlocal warmup
        if 0 < warmup:
            warmup -= 1
            return f(*args, **kwargs)
        if 0 == warmup:
            warmup -= 1
            with _trace(f):
                return f(*args, **kwargs)
        return f(*args, **kwargs)
    return wrapper

sys.monitoring.use_tool_id(OPTIMIZER_ID, "Justin")
sys.monitoring.register_callback(OPTIMIZER_ID, INSTRUCTION, _trace_instruction)
sys.monitoring.register_callback(OPTIMIZER_ID, JUMP, _trace_jump)

_OFFSETOF_CO_CODE_ADAPTIVE = 192

def _compile(fake_code, traced):
    traced = _remove_superinstructions(fake_code, traced)
    j = traced[-1]
    code_unit_pointer = ctypes.POINTER(ctypes.c_uint16)
    c_traced_type = code_unit_pointer * len(traced)
    c_traced = c_traced_type()
    first_instr = id(fake_code) + _OFFSETOF_CO_CODE_ADAPTIVE
    c_traced[:] = [ctypes.cast(first_instr + i, code_unit_pointer) for i in traced]
    compile_trace = ctypes.pythonapi._PyJIT_CompileTrace
    compile_trace.argtypes = (ctypes.c_int, c_traced_type)
    compile_trace.restype = ctypes.POINTER(ctypes.c_ubyte)
    buffer = ctypes.cast(compile_trace(len(traced), c_traced), ctypes.c_void_p)
    if buffer.value is not None:
        jump = ctypes.cast(ctypes.c_void_p(first_instr + j), ctypes.POINTER(ctypes.c_uint8))
        assert jump.contents.value == dis._all_opmap["JUMP_BACKWARD"]
        jump.contents.value = dis._all_opmap["JUMP_BACKWARD_INTO_TRACE"]
        ctypes.cast(ctypes.c_void_p(first_instr + j + 4), ctypes.POINTER(ctypes.c_uint64)).contents.value = buffer.value


def _remove_superinstructions(code: types.CodeType, trace: typing.Iterable[int]):
    out = []
    # opnames = []
    t = iter(trace)
    for i in t:
        out.append(i)
        opname = dis._all_opname[code._co_code_adaptive[i]]
        # opnames.append(opname)
        if "__" in opname:
            next(t)
    # print(list(zip(opnames, out, strict=True)))
    return out
