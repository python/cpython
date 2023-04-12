"""The Justin(time) template JIT for CPython 3.12, based on copy-and-patch."""

import ctypes
import dis
import functools
import sys
import time
import types
import typing

class Engine:
    _OFFSETOF_CO_CODE_ADAPTIVE = 192

    def __init__(self, *, verbose: bool = False) -> None:
        self._verbose = verbose
        self._compiled_time = 0.0
        self._compiling_time = 0.0
        self._tracing_time = 0.0

    def _stderr(self, *args, **kwargs) -> None:
        if self._verbose:
            print(*args, **kwargs, file=sys.stderr)

    def trace(self, f, *, warmup: int = 0):
        recorded = {}
        compiled = {}
        def tracer(frame: types.FrameType, event: str, arg: object):
            start = time.perf_counter()
            # This needs to be *fast*.
            assert frame.f_code is f.__code__
            if event == "opcode":
                i = frame.f_lasti
                if i in recorded:
                    ix = recorded[i]
                    traced = list(recorded)[ix:]
                    self._stderr(f"Compiling trace for {frame.f_code.co_filename}:{frame.f_lineno}.")
                    self._tracing_time += time.perf_counter() - start
                    start = time.perf_counter()
                    traced = self._clean_trace(frame.f_code, traced)
                    if traced is None:
                        compiled[i] = None
                        print("Failed (ends with super)!")
                    else:
                        j = traced[0] * 2
                        if j != i and compiled.get(i, None) is None:
                            compiled[i] = None
                        code_unit_pointer = ctypes.POINTER(ctypes.c_uint16)
                        c_traced_type = code_unit_pointer * len(traced)
                        c_traced = c_traced_type()
                        first_instr = id(frame.f_code) + self._OFFSETOF_CO_CODE_ADAPTIVE
                        c_traced[:] = [ctypes.cast(first_instr + i * 2, code_unit_pointer) for i in traced]
                        compile_trace = ctypes.pythonapi._PyJIT_CompileTrace
                        compile_trace.argtypes = (ctypes.c_int, c_traced_type)
                        compile_trace.restype = ctypes.POINTER(ctypes.c_ubyte)
                        buffer = ctypes.cast(compile_trace(len(traced), c_traced), ctypes.c_void_p)
                        if buffer is not None:
                            jump = ctypes.cast(ctypes.c_void_p(first_instr + j), ctypes.POINTER(ctypes.c_uint8))
                            assert jump.contents.value == dis._all_opmap["JUMP_BACKWARD"]
                            jump.contents.value = dis._all_opmap["JUMP_BACKWARD_INTO_TRACE"]
                            ctypes.cast(ctypes.c_void_p(first_instr + j + 4), ctypes.POINTER(ctypes.c_uint64)).contents.value = buffer.value
                            compiled[j] = True#WRAPPER_TYPE(buffer.value)
                        else:
                            compiled[j] = None
                            print("Failed (missing opcode)!")
                    self._compiling_time += time.perf_counter() - start
                    start = time.perf_counter()
                if i in compiled:
                    # wrapper = compiled[i]
                    # if wrapper is not None:
                    #     # self._stderr(f"Entering trace for {frame.f_code.co_filename}:{frame.f_lineno}.")
                    #     self._tracing_time += time.perf_counter() - start
                    #     start = time.perf_counter()
                    #     status = wrapper()
                    #     self._compiled_time += time.perf_counter() - start
                    #     start = time.perf_counter()
                    #     # self._stderr(f"Exiting trace for {frame.f_code.co_filename}:{frame.f_lineno} with status {status}.")
                    recorded.clear()
                else:
                    recorded[i] = len(recorded)
            elif event == "call":
                frame.f_trace_lines = False
                frame.f_trace_opcodes = True
                recorded.clear()
            self._tracing_time += time.perf_counter() - start
            return tracer
        @functools.wraps(f)
        def wrapper(*args, **kwargs):
            # This needs to be *fast*.
            nonlocal warmup
            if warmup:
                warmup -= 1
                return f(*args, **kwargs)
            warmup -= 1
            try:
                print("Tracing...")
                sys.settrace(tracer)
                return f(*args, **kwargs)
            finally:
                sys.settrace(None)
                print("...done!")
        return wrapper
    
    @staticmethod
    def _clean_trace(code: types.CodeType, trace: typing.Iterable[int]):
        skip = 0
        out = []
        opnames = []
        for x, i in enumerate(trace):
            if skip:
                skip -= 1
                continue
            opcode, _ = code._co_code_adaptive[i : i + 2]
            opname = dis._all_opname[opcode]
            if "__" in opname: # XXX
                skip = 1
            opnames.append(opname)
            out.append(i // 2)
        # print(list(zip(opnames, out, strict=True)))
        if skip:
            if "__" not in opnames[0]:
                out[0] = out[-1]
                opnames[0] = opnames[-1]
                del out[-1], opnames[-1]
            else:
                return None
        try:
            i = opnames.index("JUMP_BACKWARD")
        except ValueError:
            return None
        out = out[i:] + out[:i]
        opnames = opnames[i:] + opnames[:i]
        return out