"""The Justin(time) template JIT for CPython 3.12, based on copy-and-patch.

>>> import jit
>>> jit.enable(verbose=True, warmups=42)
"""

import ctypes
import dis
import sys
import types

import Tools.justin.build

def enable(*, verbose: bool = False, warmup: int = 1 << 1) -> None:
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

# Some tests are skipped in this command for the following reasons:
# crash: test_unittest test_bdb test_bytes
# fail: test_descr test_dis test_functools test_gc test_generators test_genexps test_heapq test_idle test_monitoring
# long: test_asyncio test_concurrent_futures test_imaplib test_import test_importlib test_io test_multiprocessing_fork test_multiprocessing_forkserver test_multiprocessing_spawn
# (Note, some long tests might crash or fail, too.)

# ./python -c 'import jit; jit.enable(); import test.__main__' -x test_unittest test_asyncio test_bdb test_bytes test_concurrent_futures test_descr test_dis test_functools test_gc test_generators test_genexps test_heapq test_idle test_imaplib test_import test_importlib test_io test_monitoring test_multiprocessing_fork test_multiprocessing_forkserver test_multiprocessing_spawn

_DISABLED = {
    # "BEFORE_ASYNC_WITH",
    # "BEFORE_WITH",
    # "BINARY_OP",
    # "BINARY_OP_ADD_FLOAT",
    # "BINARY_OP_ADD_INT",
    # "BINARY_OP_ADD_UNICODE",
    # "BINARY_OP_INPLACE_ADD_UNICODE",
    # "BINARY_OP_MULTIPLY_FLOAT",
    # "BINARY_OP_MULTIPLY_INT",
    # "BINARY_OP_SUBTRACT_FLOAT",
    # "BINARY_OP_SUBTRACT_INT",
    # "BINARY_SLICE",
    # "BINARY_SUBSCR",
    # "BINARY_SUBSCR_DICT",
    # "BINARY_SUBSCR_LIST_INT",
    # "BINARY_SUBSCR_TUPLE_INT",
    # "BUILD_CONST_KEY_MAP",
    # "BUILD_LIST",
    # "BUILD_MAP",
    # "BUILD_SET",
    # "BUILD_SLICE",
    # "BUILD_STRING",
    # "BUILD_TUPLE",
    # "CALL_INTRINSIC_1",
    # "CALL_INTRINSIC_2",
    # "CALL_NO_KW_BUILTIN_FAST",
    # "CALL_NO_KW_ISINSTANCE",
    # "CALL_NO_KW_LEN",
    # "CALL_NO_KW_LIST_APPEND",
    # "CALL_NO_KW_METHOD_DESCRIPTOR_FAST",
    # "CALL_NO_KW_STR_1",
    # "CALL_NO_KW_TUPLE_1",
    # "CALL_NO_KW_TYPE_1",
    # "COMPARE_OP",
    # "COMPARE_OP_FLOAT",
    # "COMPARE_OP_INT",
    # "COMPARE_OP_STR",
    # "CONTAINS_OP",
    # "COPY",
    # "COPY_FREE_VARS",
    # "DELETE_ATTR",
    # "DELETE_SUBSCR",
    # "DICT_UPDATE",
    # "END_FOR",
    # "END_SEND",
    # "FORMAT_VALUE",
    # "FOR_ITER_LIST",
    # "FOR_ITER_RANGE",
    # "FOR_ITER_TUPLE",
    # "GET_AITER",
    # "GET_ANEXT",
    # "GET_ITER",
    # "GET_LEN",
    # "GET_YIELD_FROM_ITER",
    # "IS_OP",
    # "JUMP_BACKWARD",
    # "JUMP_FORWARD",
    # "LIST_APPEND",
    # "LIST_EXTEND",
    # "LOAD_ASSERTION_ERROR",
    # "LOAD_ATTR",
    # "LOAD_ATTR_CLASS",
    # "LOAD_ATTR_INSTANCE_VALUE",
    # "LOAD_ATTR_METHOD_LAZY_DICT",
    # "LOAD_ATTR_METHOD_NO_DICT",
    # "LOAD_ATTR_METHOD_WITH_VALUES",
    # "LOAD_ATTR_MODULE",
    # "LOAD_ATTR_SLOT",
    # "LOAD_ATTR_WITH_HINT",
    # "LOAD_BUILD_CLASS",
    # "LOAD_CONST",
    # "LOAD_CONST__LOAD_FAST",
    # "LOAD_FAST",
    # "LOAD_FAST__LOAD_CONST",
    # "LOAD_FAST__LOAD_FAST",
    # "LOAD_GLOBAL_BUILTIN",
    # "LOAD_GLOBAL_MODULE",
    # "MAKE_FUNCTION",
    # "MAP_ADD",
    # "MATCH_MAPPING",
    # "MATCH_SEQUENCE",
    # "NOP",
    # "POP_EXCEPT",
    # "POP_JUMP_IF_FALSE",
    # "POP_JUMP_IF_NONE",
    # "POP_JUMP_IF_NOT_NONE",
    # "POP_JUMP_IF_TRUE",
    # "POP_TOP",
    # "PUSH_EXC_INFO",
    # "PUSH_NULL",
    # "SETUP_ANNOTATIONS",
    # "SET_ADD",
    # "SET_UPDATE",
    # "STORE_ATTR",
    # "STORE_ATTR_INSTANCE_VALUE",
    # "STORE_ATTR_SLOT",
    # "STORE_ATTR_WITH_HINT",
    # "STORE_DEREF",
    # "STORE_FAST",
    # "STORE_FAST__LOAD_FAST",
    # "STORE_FAST__STORE_FAST",
    # "STORE_GLOBAL",
    # "STORE_NAME",
    # "STORE_SLICE",
    # "STORE_SUBSCR",
    # "STORE_SUBSCR_DICT",
    # "STORE_SUBSCR_LIST_INT",
    # "SWAP",
    # "UNARY_INVERT",
    # "UNARY_NEGATIVE",
    # "UNARY_NOT",
    # "UNPACK_SEQUENCE_LIST",
    # "UNPACK_SEQUENCE_TUPLE",
    # "UNPACK_SEQUENCE_TWO_TUPLE",
    # "WITH_EXCEPT_START",
}

_SUPERINSTRUCTIONS = {
    "BINARY_OP_INPLACE_ADD_UNICODE",
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
        del _traces[code]
        if _compile(code, _co_code_adaptive[code], trace):
            _stderr("  - Succeeded!")
        else:
            _stderr("  - Failed!")
        sys.monitoring.set_local_events(sys.monitoring.OPTIMIZER_ID, code, sys.monitoring.events.JUMP)
    return sys.monitoring.DISABLE

def _compile(code: types.CodeType, co_code_adaptive: bytes, traced: list[int]) -> bool:
    traced = _remove_superinstructions(co_code_adaptive, traced)
    if traced is None:
        return False
    j = traced[-1]
    first_instr = id(code) + _OFFSETOF_CO_CODE_ADAPTIVE
    buff = ctypes.cast(first_instr, _py_codeunit_p)
    ctypes.memmove(buff, (ctypes.c_uint16 * (len(co_code_adaptive) // 2)).from_buffer_copy(co_code_adaptive), len(co_code_adaptive))
    c_traced_type = _py_codeunit_p * len(traced)
    c_traced = c_traced_type()
    c_traced[:] = [ctypes.cast(first_instr + i, _py_codeunit_p) for i in traced]
    jump = ctypes.cast(ctypes.c_void_p(first_instr + j), ctypes.POINTER(ctypes.c_uint8))
    if jump.contents.value != _INSTRUMENTED_JUMP_BACKWARD:
        return False
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
        if dis._all_opname[co_code_adaptive[i]] in _DISABLED:
            return None
        if dis._all_opname[co_code_adaptive[i]] in _SUPERINSTRUCTIONS:
            next(iter_trace)
    return out
