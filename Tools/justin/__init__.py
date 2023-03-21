"""The Justin(time) template JIT for CPython 3.12, based on copy-and-patch."""

import ctypes
import dataclasses
import dis
import itertools
import mmap
import pathlib
import re
import subprocess
import sys
import tempfile
import time
import timeit
import types
import typing

TOOLS_JUSTIN = pathlib.Path(__file__).parent
TOOLS_JUSTIN_TEMPLATE = TOOLS_JUSTIN / "template.c"
TOOLS_JUSTIN_TRAMPOLINE = TOOLS_JUSTIN / "trampoline.c"
PYTHON_GENERATED_CASES_C_H = TOOLS_JUSTIN.parent.parent / "Python" / "generated_cases.c.h"

WRAPPER_TYPE = ctypes.PYFUNCTYPE(ctypes.c_int)

SUPPORTED_RELOCATIONS = {
    "X86_64_RELOC_UNSIGNED",  # Mach-O (Intel)
    # "ARM64_RELOC_UNSIGNED",  # Mach-O (ARM)
    # "R_X86_64_64",  # ELF
    # "IMAGE_REL_AMD64_ADDR64",  # COFF
}

def _parse_mach_o_check_entry_is_0(path: str) -> None:
    process = subprocess.run(
        ["objdump", "--syms", path], check=True, capture_output=True
    )
    header, table, *lines = filter(None, process.stdout.decode().splitlines())
    assert re.fullmatch(fr"{path}:\s+file format mach-o 64-bit x86-64", header), header
    assert re.fullmatch(r"SYMBOL TABLE:", table), table
    pattern = r"0000000000000000\s+g\s+F\s+__TEXT,__text\s+__justin_(?:target|trampoline)"
    assert any(re.fullmatch(pattern, line) for line in lines), lines

def _parse_mach_o_holes(path: str) -> tuple["Hole", ...]:
    process = subprocess.run(
        ["objdump", "--reloc", path], check=True, capture_output=True
    )
    header, *lines = filter(None, process.stdout.decode().splitlines())
    assert re.fullmatch(fr"{path}:\s+file format mach-o 64-bit x86-64", header), header
    holes = []
    if lines:
        section, table, *lines = lines
        assert re.fullmatch(r"RELOCATION RECORDS FOR \[__text\]:", section), section
        assert re.fullmatch(r"OFFSET\s+TYPE\s+VALUE", table), table
        for line in lines:
            pattern = r"([0-9a-f]{16}) (X86_64_RELOC_UNSIGNED)    _(\w+)"
            match = re.fullmatch(pattern, line)
            offset, kind, value = match.groups()
            holes.append(Hole(value, kind, int(offset, 16)))
    return tuple(holes)

def _parse_mach_o_body(path: str) -> bytes:
    process = subprocess.run(
        ["objdump", "--full-contents", path], check=True, capture_output=True
    )
    header, *lines = filter(None, process.stdout.decode().splitlines())
    assert re.fullmatch(fr"{path}:\s+file format mach-o 64-bit x86-64", header), header
    body = bytearray()
    for line in lines:
        line, _, _ = line.partition("  ")
        if re.fullmatch(r"Contents of section __TEXT,__(?:text|cstring):", line):
            continue
        i, *rest = line.split()
        assert int(i, 16) == len(body), (i, len(body))
        for word in rest:
            body.extend(int(word, 16).to_bytes(len(word) // 2, "big"))
    return bytes(body)

def parse_mach_o(path: str):
    _parse_mach_o_check_entry_is_0(path)
    holes = _parse_mach_o_holes(path)
    body = _parse_mach_o_body(path)
    return Stencil(body, holes)

@dataclasses.dataclass(frozen=True)
class Hole:
    symbol: str
    kind: str
    offset: int

    def patch(self, body: bytearray, value: int) -> None:
        assert self.kind == "X86_64_RELOC_UNSIGNED", self.kind
        size = 8
        hole = slice(self.offset, self.offset + size)
        value += int.from_bytes(body[hole], sys.byteorder, signed=False)
        body[hole] = value.to_bytes(size, sys.byteorder, signed=False)

@dataclasses.dataclass(frozen=True)
class Stencil:
    body: bytes
    holes: tuple[Hole, ...]

    def load(self) -> typing.Self:
        new_body = bytearray(self.body)
        new_holes = []
        for hole in self.holes:
            value = self._get_address(hole.symbol)
            if value is not None:
                hole.patch(new_body, value)
            else:
                if not hole.symbol.startswith("_justin") and hole.symbol != "_cstring":
                    print(hole.symbol)
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

class Trace:
    def __init__(self, func: typing.Callable[[], int], code: types.CodeType) -> None:
        self.run_here = func
        self._ref = code

class Engine:
    _CC_FLAGS = [
        "-DNDEBUG",
        "-I.",
        "-IInclude",
        "-IInclude/internal",
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
    ]
    _OFFSETOF_CO_CODE_ADAPTIVE = 192
    _WINDOW = 2
    _OPS = [
        "BINARY_OP_ADD_FLOAT",
        "BINARY_OP_SUBTRACT_FLOAT",
        "COMPARE_AND_BRANCH_FLOAT",
        "JUMP_BACKWARD",
        "LOAD_CONST",
        "LOAD_FAST",
        "LOAD_FAST__LOAD_CONST",
        "LOAD_FAST__LOAD_FAST",
        "STORE_FAST__LOAD_FAST",
        "STORE_FAST__STORE_FAST",
    ]

    def __init__(self) -> None:
        self._stencils_built = {}
        self._stencils_loaded = {}
        self._trampoline_built = None
        self._trampoline_loaded = None

    def _compile(self, opnames, path) -> Stencil:
        print(f"Building: {' -> '.join(opnames)}")
        defines = []
        for i, opname in enumerate(opnames):
            branches = str("BRANCH" in opname or "JUMP_IF" in opname).lower()
            defines.append(f"-D_JUSTIN_CHECK_{i}={branches}")
            defines.append(f"-D_JUSTIN_OPCODE_{i}={opname}")
        with tempfile.NamedTemporaryFile(suffix=".o") as o:
            subprocess.run(
                ["clang", *self._CC_FLAGS, *defines, path, "-o",  o.name],
                check=True,
            )
            return parse_mach_o(o.name)

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
            print(f"Loading: {' -> '.join(opnames)}")
            self._stencils_loaded[opnames] = stencil.load()
        self._trampoline_loaded = self._trampoline_built.load()

    def compile_trace(
        self, code: types.CodeType, trace: typing.Sequence[int], stacklevel: int = 0
    ) -> Trace:
        start = time.time()
        pre = []
        for x in range(len(trace)):
            i = trace[x]
            j = trace[(x + 1) % len(trace)]
            opcode, oparg = code._co_code_adaptive[i : i + 2]
            opname = dis._all_opname[opcode]
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
                assert False
        lengths = [
            len(self._trampoline_loaded),  *(len(stencil) for stencil, _, _, _ in bundles)
        ]
        print(f"Size: {size:,} bytes")
        flags = mmap.MAP_ANONYMOUS | mmap.MAP_PRIVATE
        prot = mmap.PROT_EXEC | mmap.PROT_WRITE
        memory = mmap.mmap(-1, size, flags=flags, prot=prot)
        buffer = (ctypes.c_char * size).from_buffer(memory)
        first_instr = id(code) + self._OFFSETOF_CO_CODE_ADAPTIVE
        continuations = list(itertools.accumulate(lengths, initial=ctypes.addressof(buffer)))[1:]
        continuations[-1] = continuations[0]
        memory.write(
            self._trampoline_loaded.copy_and_patch(
                _cstring=memory.tell(),
                _justin_continue=continuations[0],
                _justin_next_instr = first_instr + trace[0],
                _justin_stacklevel = stacklevel,
            )
        )
        for (stencil, i, js, opargs), continuation in zip(
            bundles, continuations[1:], strict=True,
        ):
            patches = {}
            for x, (j, oparg) in enumerate(zip(js, opargs, strict=True)):
                patches[f"_justin_next_trace_{x}"] = first_instr + j
                patches[f"_justin_oparg_{x}"] = oparg
            memory.write(
                stencil.copy_and_patch(
                    _cstring=memory.tell(),
                    _justin_continue=continuation,
                    _justin_next_instr=first_instr + i,
                    _justin_target=memory.tell(),
                    **patches,
                )
            )
        assert memory.tell() == len(memory) == size
        wrapper = ctypes.cast(buffer, WRAPPER_TYPE)
        print(f"Time: {(time.time() - start) * 1_000:0.3} ms")
        return Trace(wrapper, code)
