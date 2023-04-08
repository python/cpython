"""The Justin(time) template JIT for CPython 3.12, based on copy-and-patch."""

import collections
import ctypes
import dataclasses
import dis
import functools
import itertools
import mmap
import pathlib
import re
import subprocess
import sys
import tempfile
import time
import types
import typing

TOOLS_JUSTIN = pathlib.Path(__file__).parent
TOOLS_JUSTIN_TEMPLATE = TOOLS_JUSTIN / "template.c"
TOOLS_JUSTIN_TRAMPOLINE = TOOLS_JUSTIN / "trampoline.c"
PYTHON_GENERATED_CASES_C_H = TOOLS_JUSTIN.parent.parent / "Python" / "generated_cases.c.h"

WRAPPER_TYPE = ctypes.PYFUNCTYPE(ctypes.c_double)

COMPILED_TIME = 0.0
COMPILING_TIME = 0.0
TRACING_TIME = 0.0

# XXX: Do --reloc, then --headers, then --full-contents (per-section)
# Maybe need --syms to check that justin_entry is indeed first/only?
class ObjectParser:

    _file_format: typing.ClassVar[str]
    _type: typing.ClassVar[str]
    _section_prefix: typing.ClassVar[str]
    _fix_up_holes: typing.ClassVar[bool]
    _symbol_prefix: typing.ClassVar[str]
    
    def __init__(self, path: str) -> None:
        self._sections = []
        self._path = path

    def _dump(self, *args) -> typing.Iterator[str]:
        args = ["llvm-objdump", *args, self._path]
        process = subprocess.run(args, check=True, capture_output=True)
        lines = filter(None, process.stdout.decode().splitlines())
        line = next(lines, None)
        if line != f"{self._path}:\tfile format {self._file_format}":
            raise NotImplementedError(line)
        return lines

    def _parse_headers(self) -> typing.Generator[tuple[str, int, str], None, None]:
        lines = self._dump("--headers")
        line = next(lines, None)
        assert line == "Sections:"
        line = next(lines, None)
        pattern = r"Idx\s+Name\s+Size\s+VMA\s+Type"
        match = re.fullmatch(pattern, line)
        assert match is not None
        pattern = r"\s+(\d+)\s+([\w\.\-]+)?\s+([0-9a-f]{8})\s+([0-9a-f]{16})\s+(BSS|DATA|TEXT)?"
        for i, line in enumerate(lines):
            match = re.fullmatch(pattern, line)
            assert match is not None, line
            idx, name, size, vma, type = match.groups()
            assert int(idx) == i
            assert int(vma, 16) == 0
            if type is not None:
                yield (name, int(size, 16), type)

    # def _parse_syms(self) -> None:
    #     lines = self._dump("--syms", "--section", ".text")
    #     assert next(lines) == "SYMBOL TABLE:"
    #     pattern = r"([0-9a-f]+)\s+([\sa-zA-Z]*)\s+(\*ABS\*|\*UND\*|[\w\.]+)\s+([0-9a-f]+)\s+([\w\.]+)"
    #     for line in lines:
    #         match = re.fullmatch(pattern, line)
    #         assert match is not None, line
    #         value, flags, section, size, name = match.groups()
    #         assert int(value, 16) == 0
    #         if section == "*ABS*":
    #             assert flags == "l    df"
    #             assert int(size, 16) == 0
    #         elif section == "*UND*":
    #             assert flags == ""
    #             assert int(size, 16) == 0
    #         else:
    #             print(name, int(size, 16))

    def _parse_reloc(self) -> None:
        lines = self._dump("--reloc")
        relocs = collections.defaultdict(list)
        line = next(lines, None)
        while line is not None:
            pattern = r"RELOCATION RECORDS FOR \[([\w+\.]+)\]:"
            match = re.fullmatch(pattern, line)
            assert match is not None
            [section] = match.groups()
            assert section not in relocs
            line = next(lines, None)
            pattern = r"OFFSET\s+TYPE\s+VALUE"
            match = re.fullmatch(pattern, line)
            assert match is not None
            for line in lines:
                pattern = r"([0-9a-f]{16})\s+(\w+)\s+([\w\.]+)(?:\+0x([0-9a-f]+))?"
                match = re.fullmatch(pattern, line)
                if match is None:
                    break
                offset, type, value, addend = match.groups()
                assert type == self._type, type
                addend = int(addend, 16) if addend else 0
                assert value.startswith(self._symbol_prefix)
                value = value.removeprefix(self._symbol_prefix)
                relocs[section].append(Hole(value, type, int(offset, 16), addend))
            else:
                break
        return relocs

    def _parse_full_contents(self, section) -> bytes:
        lines = self._dump("--disassemble", "--reloc", "--section", section)
        print("\n".join(lines))
        lines = self._dump("--full-contents", "--section", section)
        print("\n".join(lines))
        lines = self._dump("--full-contents", "--section", section)
        line = next(lines, None)
        if line is None:
            return b""
        assert line == f"Contents of section {self._section_prefix}{section}:", line
        body = bytearray()
        pattern = r" ([\s0-9a-f]{4}) ([\s0-9a-f]{8}) ([\s0-9a-f]{8}) ([\s0-9a-f]{8}) ([\s0-9a-f]{8}) .*"
        for line in lines:
            match = re.fullmatch(pattern, line)
            assert match is not None, line
            i, *chunks = match.groups()
            assert int(i, 16) == len(body), (i, len(body))
            data = "".join(chunks).rstrip()
            body.extend(int(data, 16).to_bytes(len(data) // 2, "big"))
        return bytes(body)

    def parse(self):
        relocs = self._parse_reloc()
        body = b""
        holes = []
        offsets = {}
        for name, size, type in self._parse_headers():
            size_before = len(body)
            holes += [Hole(hole.symbol, hole.kind, hole.offset+size_before, hole.addend) for hole in relocs[name]]
            offsets[name] = size_before
            body += self._parse_full_contents(name)
            assert len(body) - size_before == size
        fixed = []
        for hole in holes:
            if self._fix_up_holes and hole.symbol in offsets:
                # TODO: fix offset too? Or just assert that relocs are only in main section?
                hole = Hole(hole.symbol, hole.kind, hole.offset, hole.addend + offsets[hole.symbol])
            fixed.append(hole)
        return Stencil(body, fixed)
    
class ObjectParserCOFFX8664(ObjectParser):
    _file_format = "coff-x86-64"
    _type = "IMAGE_REL_AMD64_ADDR64"
    _section_prefix = ""
    _fix_up_holes = True
    _symbol_prefix = ""

class ObjectParserELF64X8664(ObjectParser):
    _file_format = "elf64-x86-64"
    _type = "R_X86_64_64"
    _section_prefix = ""
    _fix_up_holes = True
    _symbol_prefix = ""

class ObjectParserMachO64BitARM64(ObjectParser):
    _file_format = "mach-o 64-bit arm64"
    _type = "ARM64_RELOC_UNSIGNED"
    _section_prefix = "__TEXT,"
    _fix_up_holes = False
    _symbol_prefix = "_"

class ObjectParserMachO64BitX8664(ObjectParser):
    _file_format = "mach-o 64-bit x86-64"
    _type = "X86_64_RELOC_UNSIGNED"
    _section_prefix = "__TEXT,"
    _fix_up_holes = False
    _symbol_prefix = "_"

def _get_object_parser(path: str) -> ObjectParser:
    for parser in [
        ObjectParserCOFFX8664,
        ObjectParserELF64X8664,
        ObjectParserMachO64BitARM64,
        ObjectParserMachO64BitX8664
    ]:
        p = parser(path)
        try:
            p._dump("--syms")
        except NotImplementedError:
            pass
        else:
            return p
    raise NotImplementedError(sys.platform)



def read_uint64(body: bytes, offset: int) -> int:
    return int.from_bytes(body[offset : offset + 8], sys.byteorder, signed=False)

def write_uint64(body: bytearray, offset: int, value: int) -> None:
    value &= (1 << 64) - 1
    body[offset : offset + 8] = value.to_bytes(8, sys.byteorder, signed=False)

@dataclasses.dataclass(frozen=True)
class Hole:
    symbol: str
    kind: str
    offset: int
    addend: int

    def patch(self, body: bytearray, value: int) -> None:
        write_uint64(body, self.offset, value + self.addend)

@dataclasses.dataclass(frozen=True)
class Stencil:
    body: bytes
    holes: tuple[Hole, ...]

    def load(self) -> typing.Self:
        # XXX: Load the addend too.
        new_body = bytearray(self.body)
        new_holes = []
        for hole in self.holes:
            value = self._get_address(hole.symbol)
            if value is not None:
                # print(hole.symbol)
                hole.patch(new_body, value)
            else:
                # if not hole.symbol.startswith("_justin") and hole.symbol != "_cstring":
                #     print(hole.symbol)
                new_holes.append(hole)
        return self.__class__(bytes(new_body), tuple(new_holes))
    
    def copy_and_patch(self, **symbols: int) -> bytes:
        body = bytearray(self.body)
        for hole in self.holes:
            value = symbols[hole.symbol]
            hole.patch(body, value)
        return bytes(body)

    @staticmethod
    def _get_address(name: str) -> int | None:
        wrapper = getattr(ctypes.pythonapi, name, None)
        pointer = ctypes.cast(wrapper, ctypes.c_void_p)
        address = pointer.value
        return address

    def __len__(self) -> int:
        return len(self.body)
    
def mmap_posix(size):
    flags = mmap.MAP_ANONYMOUS | mmap.MAP_PRIVATE
    prot = mmap.PROT_EXEC | mmap.PROT_WRITE
    memory = mmap.mmap(-1, size, flags=flags, prot=prot)
    return (ctypes.c_char * size).from_buffer(memory)
    
def mmap_windows(size):
    kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)
    MEM_COMMIT = 0x00001000
    MEM_RESERVE = 0x00002000
    PAGE_EXECUTE_READWRITE = 0x40
    VirtualAlloc = kernel32.VirtualAlloc
    VirtualAlloc.argtypes = [
        ctypes.c_void_p,
        ctypes.c_size_t,
        ctypes.c_uint32,
        ctypes.c_uint32,
    ]
    VirtualAlloc.restype = ctypes.c_void_p
    memory = VirtualAlloc(None, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE)
    return ctypes.cast(ctypes.c_char_p(memory), ctypes.POINTER(ctypes.c_char * size))[0]


class Engine:
    _CC_FLAGS = [
        "-DNDEBUG",
        "-I.",
        "-IInclude",
        "-IInclude/internal",
        "-IPC",
        "-O3",
        "-Wall",
        "-Werror",
        # Not all instructions use the variables and labels we provide:
        "-Wno-unused-but-set-variable",
        "-Wno-unused-label",
        "-Wno-unused-variable",
        "-c",
        # Don't need these:
        "-fno-asynchronous-unwind-tables",
        # Need this to leave room for patching our 64-bit pointers:
        "-mcmodel=large",
        # Don't want relocations to use the global offset table:
        "-fno-pic",
    ]
    _OFFSETOF_CO_CODE_ADAPTIVE = 192
    _WINDOW = 1
    _OPS = [
        "BINARY_OP",
        "BINARY_OP_ADD_FLOAT",
        "BINARY_OP_MULTIPLY_FLOAT",
        "BINARY_OP_SUBTRACT_FLOAT",
        "BINARY_SUBSCR_LIST_INT",
        "COPY",
        "FOR_ITER_LIST",
        "JUMP_BACKWARD",
        "LOAD_CONST",
        "LOAD_FAST",
        "LOAD_FAST__LOAD_CONST",
        "LOAD_FAST__LOAD_FAST",
        "STORE_FAST",
        "STORE_FAST__LOAD_FAST",
        "STORE_FAST__STORE_FAST",
        "STORE_SUBSCR_LIST_INT",
        "SWAP",
        "UNPACK_SEQUENCE_LIST",
        "UNPACK_SEQUENCE_TUPLE",
        "UNPACK_SEQUENCE_TWO_TUPLE",
    ]

    def __init__(self, *, verbose: bool = False) -> None:
        self._stencils_built = {}
        self._stencils_loaded = {}
        self._trampoline_built = None
        self._trampoline_loaded = None
        self._verbose = verbose

    def _stderr(self, *args, **kwargs) -> None:
        if self._verbose:
            print(*args, **kwargs, file=sys.stderr)

    def _compile(self, opnames, path) -> Stencil:
        self._stderr(f"Building stencil for {' + '.join(opnames)}.")
        defines = []
        for i, opname in enumerate(opnames):
            branches = int("JUMP_IF" in opname or "FOR_ITER" in opname)
            defines.append(f"-D_JUSTIN_CHECK_{i}={branches}")
            defines.append(f"-D_JUSTIN_OPCODE_{i}={opname}")
        with tempfile.NamedTemporaryFile(suffix=".o") as o, tempfile.NamedTemporaryFile(suffix=".ll") as ll:
            subprocess.run(
                ["clang", *self._CC_FLAGS, *defines, path, "-emit-llvm", "-S", "-o",  ll.name],
                check=True,
            )
            llp = pathlib.Path(ll.name)
            ir = llp.read_text()
            ir = ir.replace("i32 @_justin_continue", "ghccc i32 @_justin_continue")
            ir = ir.replace("i32 @_justin_entry", "ghccc i32 @_justin_entry")
            llp.write_text(ir)
            subprocess.run(
                ["llc-14", "-O3", "--code-model", "large", "--filetype", "obj", ll.name, "-o",  o.name],
                check=True,
            )
            return _get_object_parser(o.name).parse()

    def build(self) -> None:
        generated_cases = PYTHON_GENERATED_CASES_C_H.read_text()
        pattern = r"(?s:\n( {8}TARGET\((\w+)\) \{\n.*?\n {8}\})\n)"
        self._cases = {}
        for body, opname in re.findall(pattern, generated_cases):
            self._cases[opname] = body.replace("%", "%%").replace(" " * 8, " " * 4)
        template = TOOLS_JUSTIN_TEMPLATE.read_text()
        self._TRUE_OPS = []
        for w in range(1, self._WINDOW + 1):
            self._TRUE_OPS += itertools.product(self._OPS, repeat=w)
        if ("FOR_ITER_LIST", "FOR_ITER_LIST") in self._TRUE_OPS:
            self._TRUE_OPS.remove(("FOR_ITER_LIST", "FOR_ITER_LIST"))  # XXX
        for opnames in self._TRUE_OPS:
            parts = []
            for i, opname in enumerate(opnames):
                parts.append(f"#undef _JUSTIN_CONTINUE")
                parts.append(f"#define _JUSTIN_CONTINUE _continue_{i}")
                parts.append(f"    _JUSTIN_PART({i});")
                parts.append(self._cases[opname])
                parts.append(f"    Py_UNREACHABLE();")
                parts.append(f"_continue_{i}:")
                parts.append(f"    ;")
            body = template % "\n".join(parts)
            with tempfile.NamedTemporaryFile("w", suffix=".c") as c:
                c.write(body)
                c.flush()
                self._stencils_built[opnames] = self._compile(opnames, c.name)
        self._trampoline_built = self._compile(("<trampoline>",), TOOLS_JUSTIN_TRAMPOLINE)

    def load(self) -> None:
        for opnames, stencil in self._stencils_built.items():
            self._stderr(f"Loading stencil for {' + '.join(opnames)}.")
            self._stencils_loaded[opnames] = stencil.load()
        self._trampoline_loaded = self._trampoline_built.load()

    def trace(self, f, *, warmup: int = 2):
        recorded = {}
        compiled = {}
        def tracer(frame: types.FrameType, event: str, arg: object):
            global TRACING_TIME, COMPILED_TIME
            start = time.perf_counter()
            # This needs to be *fast*.
            assert frame.f_code is f.__code__
            if event == "opcode":
                i = frame.f_lasti
                if i in recorded:
                    ix = recorded[i]
                    traced = list(recorded)[ix:]
                    self._stderr(f"Compiling trace for {frame.f_code.co_filename}:{frame.f_lineno}.")
                    TRACING_TIME += time.perf_counter() - start
                    wrapper = self._compile_trace(frame.f_code, traced)
                    start = time.perf_counter()
                    compiled[i] = wrapper
                if i in compiled:
                    # self._stderr(f"Entering trace for {frame.f_code.co_filename}:{frame.f_lineno}.")
                    TRACING_TIME += time.perf_counter() - start
                    start = time.perf_counter()
                    status = compiled[i]()
                    COMPILED_TIME += time.perf_counter() - start
                    start = time.perf_counter()
                    # self._stderr(f"Exiting trace for {frame.f_code.co_filename}:{frame.f_lineno} with status {status}.")
                    recorded.clear()
                else:
                    recorded[i] = len(recorded)
            elif event == "call":
                frame.f_trace_lines = False
                frame.f_trace_opcodes = True
                recorded.clear()
            TRACING_TIME += time.perf_counter() - start
            return tracer
        @functools.wraps(f)
        def wrapper(*args, **kwargs):
            # This needs to be *fast*.
            nonlocal warmup
            if 0 < warmup:
                warmup -= 1
                return f(*args, **kwargs)
            try:
                sys.settrace(tracer)
                return f(*args, **kwargs)
            finally:
                sys.settrace(None)
        return wrapper

    def _compile_trace(self, code: types.CodeType, trace: typing.Sequence[int]):
        # This needs to be *fast*.
        start = time.perf_counter()
        pre = []
        skip = 0
        for x in range(len(trace)):
            if skip:
                skip -= 1
                continue
            i = trace[x]
            opcode, oparg = code._co_code_adaptive[i : i + 2]
            opname = dis._all_opname[opcode]
            if "__" in opname: # XXX
                skip = 1
            j = trace[(x + skip + 1) % len(trace)]
            pre.append((i, j, opname, oparg))
        bundles = []
        x = 0
        size = len(self._trampoline_loaded)
        while x < len(pre):
            for w in range(self._WINDOW, 0, -1):
                pres = pre[x:x+w]
                i = pres[0][0]
                js = tuple(p[1] for p in pres)
                opnames = tuple(p[2] for p in pres)
                opargs = tuple(p[3] for p in pres)
                if opnames in self._stencils_loaded:
                    stencil = self._stencils_loaded[opnames]
                    bundles.append((stencil, i, js, opargs))
                    size += len(stencil)
                    x += w
                    break
            else:
                print(f"Failed to find stencil for {pre[x]}.")
        lengths = [
            len(self._trampoline_loaded),  *(len(stencil) for stencil, _, _, _ in bundles)
        ]
        first_instr = id(code) + self._OFFSETOF_CO_CODE_ADAPTIVE
        if sys.platform == "win32":
            memory = mmap_windows(size)
        else:
            memory = mmap_posix(size)
        base = ctypes.addressof(memory)
        continuations = list(itertools.accumulate(lengths, initial=base))[1:]
        continuations[-1] = continuations[0]
        buffer = bytearray()
        buffer += (
            self._trampoline_loaded.copy_and_patch(
                _cstring=base + len(buffer),
                _justin_continue=continuations[0],
                _justin_next_instr = first_instr + trace[0],
            )
        )
        for (stencil, i, js, opargs), continuation in zip(
            bundles, continuations[1:], strict=True,
        ):
            patches = {}
            for x, (j, oparg) in enumerate(zip(js, opargs, strict=True)):
                patches[f"_justin_next_trace_{x}"] = first_instr + j
                patches[f"_justin_oparg_{x}"] = oparg
            patches[".rodata.str1.1"] = base + len(buffer)
            patches[".rodata"] = base + len(buffer)
            buffer += (
                stencil.copy_and_patch(
                    _cstring=base + len(buffer),
                    _justin_continue=continuation,
                    _justin_next_instr=first_instr + i,
                    _justin_target=len(buffer),
                    **patches,
                )
            )
        assert len(buffer) == size
        memory[:] = buffer
        wrapper = ctypes.cast(memory, WRAPPER_TYPE)
        global COMPILING_TIME
        diff = time.perf_counter() - start
        COMPILING_TIME += diff
        self._stderr(f"Compiled {size:,} bytes in {diff * 1_000:0.3} ms")
        return wrapper
