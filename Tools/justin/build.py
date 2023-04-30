"""The Justin(time) template JIT for CPython 3.12, based on copy-and-patch."""

import collections
import dataclasses
import itertools
import pathlib
import re
import subprocess
import sys
import tempfile
import typing

TOOLS_JUSTIN = pathlib.Path(__file__).parent
TOOLS_JUSTIN_TEMPLATE = TOOLS_JUSTIN / "template.c"
TOOLS_JUSTIN_TRAMPOLINE = TOOLS_JUSTIN / "trampoline.c"
PYTHON_GENERATED_CASES_C_H = TOOLS_JUSTIN.parent.parent / "Python" / "generated_cases.c.h"

def batched(iterable, n):
    """Batch an iterable into lists of size n."""
    it = iter(iterable)
    while True:
        batch = list(itertools.islice(it, n))
        if not batch:
            return
        yield batch

# XXX: This parsing needs to be way cleaned up. syms, headers, then disassembly and relocs per section
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

    def _parse_headers(self) -> typing.Generator[tuple[str, int, int, str], None, None]:
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
            # assert int(vma, 16) == 0
            if type is not None:
                yield (name, int(size, 16), int(vma, 16), type)

    def _parse_syms(self) -> None:
        lines = self._dump("--syms", "--section", ".text")
        assert next(lines) == "SYMBOL TABLE:"
        syms = {}
        pattern = r"([0-9a-f]+)\s+([\sa-zA-Z]*)\s+(\*ABS\*|\*UND\*|[\w\.,]+)(?:\s+([0-9a-f]+))?\s+(\w+)"
        for line in lines:
            match = re.fullmatch(pattern, line)
            assert match is not None, repr(line)
            value, flags, section, size, name = match.groups()
            if size is None:
                size = "0"
            if section == "*ABS*":
                assert flags == "l    df"
                assert int(size, 16) == 0
            elif section == "*UND*":
                assert flags == ""
                assert int(size, 16) == 0
            else:
                syms[name] = value
                if name == "_justin_entry":
                    assert flags == "g     F"
                    assert int(size, 16) == 0
                    assert int(value, 16) == 0
        
        return syms

    def _parse_reloc(self) -> None:
        lines = self._dump("--reloc")
        relocs = collections.defaultdict(list)
        line = next(lines, None)
        while line is not None:
            pattern = r"RELOCATION RECORDS FOR \[([\w+\.]+)\]:"
            match = re.fullmatch(pattern, line)
            assert match is not None, line
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
                relocs[section].append(Hole(value, int(offset, 16), addend))
            else:
                break
        return relocs

    def _parse_full_contents(self, section, vma) -> bytes:
        lines = self._dump("--full-contents", "--section", section)
        line = next(lines, None)
        if line is None:
            return b""
        # assert line == f"Contents of section {prefix}{section}:", line
        body = bytearray()
        pattern = r" ([\s0-9a-f]{4}) ([\s0-9a-f]{8}) ([\s0-9a-f]{8}) ([\s0-9a-f]{8}) ([\s0-9a-f]{8}) .*"
        for line in lines:
            match = re.fullmatch(pattern, line)
            assert match is not None, line
            i, *chunks = match.groups()
            assert int(i, 16) - vma == len(body), (i, len(body))
            data = "".join(chunks).rstrip()
            body.extend(int(data, 16).to_bytes(len(data) // 2, "big"))
        return bytes(body)

    def parse(self):
        # self._parse_syms()
        relocs = self._parse_reloc()
        offsets = {}
        # breakpoint()
        body = b""
        holes = []
        for name, size, vma, type in self._parse_headers():
            if vma:
                body += bytes(vma - len(body))
            size_before = len(body)
            holes += [Hole(hole.symbol, hole.offset+size_before, hole.addend) for hole in relocs[name]]
            offsets[name] = size_before
            body += self._parse_full_contents(name, vma)
            assert len(body) - size_before == size
        fixed = []
        for hole in holes:
            if self._fix_up_holes and hole.symbol in offsets:
                # TODO: fix offset too? Or just assert that relocs are only in main section?
                hole = Hole(hole.symbol, hole.offset, hole.addend + offsets[hole.symbol])
            if hole.symbol.startswith(".rodata") or hole.symbol.startswith(".text") or hole.symbol.startswith("_cstring"):
                hole = Hole("_justin_base", hole.offset, hole.addend)
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
    _section_prefix = ""
    _fix_up_holes = True
    _symbol_prefix = "_"

class ObjectParserMachO64BitX8664(ObjectParser):
    _file_format = "mach-o 64-bit x86-64"
    _type = "X86_64_RELOC_UNSIGNED"
    _section_prefix = ""
    _fix_up_holes = True
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

@dataclasses.dataclass(frozen=True)
class Hole:
    symbol: str
    offset: int
    addend: int

@dataclasses.dataclass(frozen=True)
class Stencil:
    body: bytes
    holes: tuple[Hole, ...]


class Engine:
    _CPPFLAGS = [
        "-DNDEBUG",
        "-DPy_BUILD_CORE",
        "-D_PyJIT_ACTIVE",
        "-I.",
        "-I./Include",
        "-I./Include/internal",
        "-I./PC",
    ]
    _CFLAGS = [
        "-O3",
        "-Wall",
        "-Wextra",
        "-Wno-unused-but-set-variable",
        "-Wno-unused-label",
        "-Wno-unused-variable",
        # We don't need this (and it causes weird relocations):
        "-fno-asynchronous-unwind-tables",
        # Don't want relocations to use the global offset table:
        "-fno-pic",
        # The GHC calling convention uses %rbp as an argument-passing register:
        "-fomit-frame-pointer",
        # Need this to leave room for patching our 64-bit pointers:
        "-mcmodel=large",
    ]
    _OFFSETOF_CO_CODE_ADAPTIVE = 192
    _SKIP = frozenset(
        {
            "CALL_BOUND_METHOD_EXACT_ARGS",
            "CALL_FUNCTION_EX",
            "CHECK_EG_MATCH",
            "CHECK_EXC_MATCH",
            "CLEANUP_THROW",
            "DELETE_DEREF",
            "DELETE_FAST",
            "DELETE_GLOBAL",
            "DELETE_NAME",
            "DICT_MERGE",
            "END_ASYNC_FOR",
            "EXTENDED_ARG",  # XXX
            "FOR_ITER",
            "GET_AWAITABLE",
            "IMPORT_FROM",
            "IMPORT_NAME",
            "INSTRUMENTED_CALL", # XXX
            "INSTRUMENTED_CALL_FUNCTION_EX", # XXX
            "INSTRUMENTED_END_FOR", # XXX
            "INSTRUMENTED_END_SEND", # XXX
            "INSTRUMENTED_FOR_ITER", # XXX
            "INSTRUMENTED_INSTRUCTION", # XXX
            "INSTRUMENTED_JUMP_BACKWARD", # XXX
            "INSTRUMENTED_JUMP_FORWARD", # XXX
            "INSTRUMENTED_LINE", # XXX
            "INSTRUMENTED_POP_JUMP_IF_FALSE", # XXX
            "INSTRUMENTED_POP_JUMP_IF_NONE", # XXX
            "INSTRUMENTED_POP_JUMP_IF_NOT_NONE", # XXX
            "INSTRUMENTED_POP_JUMP_IF_TRUE", # XXX
            "INSTRUMENTED_RESUME", # XXX
            "INSTRUMENTED_RETURN_CONST", # XXX
            "INSTRUMENTED_RETURN_VALUE", # XXX
            "INSTRUMENTED_YIELD_VALUE", # XXX
            "INTERPRETER_EXIT", # XXX
            "JUMP_BACKWARD",  # XXX: Is this a problem?
            "JUMP_BACKWARD_INTO_TRACE",
            "JUMP_BACKWARD_NO_INTERRUPT",
            "KW_NAMES",  # XXX
            "LOAD_CLASSDEREF",
            "LOAD_CLOSURE",
            "LOAD_DEREF",
            "LOAD_FAST_CHECK",
            "LOAD_GLOBAL",
            "LOAD_NAME",
            "MAKE_CELL",
            "MATCH_CLASS",
            "MATCH_KEYS",
            "RAISE_VARARGS",
            "RERAISE",
            "SEND",
            "STORE_ATTR_WITH_HINT",
            "UNPACK_EX",
            "UNPACK_SEQUENCE",
        }
    )

    def __init__(self, *, verbose: bool = False) -> None:
        self._stencils_built = {}
        self._stencils_loaded = {}
        self._trampoline_built = None
        self._trampoline_loaded = None
        self._verbose = verbose

    def _stderr(self, *args, **kwargs) -> None:
        if self._verbose:
            print(*args, **kwargs, file=sys.stderr)

    @staticmethod
    def _use_ghccc(path: str) -> None:
        ll = pathlib.Path(path)
        ir = ll.read_text()
        ir = ir.replace("i32 @_justin_continue", "ghccc i32 @_justin_continue")
        ir = ir.replace("i32 @_justin_entry", "ghccc i32 @_justin_entry")
        ll.write_text(ir)

    def _compile(self, opname, path) -> Stencil:
        self._stderr(f"Building stencil for {opname}.")
        defines = [f"-D_JUSTIN_OPCODE={opname}"]
        with tempfile.NamedTemporaryFile(suffix=".o") as o, tempfile.NamedTemporaryFile(suffix=".ll") as ll:
            subprocess.run(
                ["clang", *self._CPPFLAGS, *defines, *self._CFLAGS, "-emit-llvm", "-S", "-o", ll.name, path],
                check=True,
            )
            self._use_ghccc(ll.name)
            subprocess.run(
                ["clang", *self._CFLAGS, "-c", "-o", o.name, ll.name],
                check=True,
            )
            return _get_object_parser(o.name).parse()

    def build(self) -> None:
        generated_cases = PYTHON_GENERATED_CASES_C_H.read_text()
        pattern = r"(?s:\n( {8}TARGET\((\w+)\) \{\n.*?\n {8}\})\n)"
        self._cases = {}
        for body, opname in re.findall(pattern, generated_cases):
            self._cases[opname] = body.replace(" " * 8, " " * 4)
        template = TOOLS_JUSTIN_TEMPLATE.read_text()
        for opname in sorted(self._cases.keys() - self._SKIP):
            body = template % self._cases[opname]
            with tempfile.NamedTemporaryFile("w", suffix=".c") as c:
                c.write(body)
                c.flush()
                self._stencils_built[opname] = self._compile(opname, c.name)
        self._trampoline_built = self._compile("<trampoline>", TOOLS_JUSTIN_TRAMPOLINE)

    def dump(self) -> str:
        lines = []
        kinds = {
            "HOLE_base",
            "HOLE_continue",
            "HOLE_next_instr",
            "HOLE_next_trace",
            "HOLE_oparg",
        }
        opnames = []
        for opname, stencil in sorted(self._stencils_built.items()) + [("trampoline", self._trampoline_built)]:
            opnames.append(opname)
            lines.append(f"// {opname}")
            lines.append(f"static const unsigned char {opname}_stencil_bytes[] = {{")
            for chunk in batched(stencil.body, 8):
                lines.append(f"    {', '.join(f'0x{byte:02X}' for byte in chunk)},")
            lines.append(f"}};")
            lines.append(f"static const Hole {opname}_stencil_holes[] = {{")
            for hole in stencil.holes:
                if hole.symbol.startswith("_justin_"):
                    kind = f"HOLE_{hole.symbol.removeprefix('_justin_')}"
                    assert kind in kinds, kind
                else:
                    kind = f"LOAD_{hole.symbol}"
                    kinds.add(kind)
                lines.append(f"    {{.offset = {hole.offset:3}, .addend = {hole.addend:3}, .kind = {kind}}},")
            lines.append(f"}};")
            lines.append(f"")
        lines.append(f"static const Stencil trampoline_stencil = {{")
        lines.append(f"    .nbytes = Py_ARRAY_LENGTH(trampoline_stencil_bytes),")
        lines.append(f"    .bytes = trampoline_stencil_bytes,")
        lines.append(f"    .nholes = Py_ARRAY_LENGTH(trampoline_stencil_holes),")
        lines.append(f"    .holes = trampoline_stencil_holes,")
        lines.append(f"}};")
        lines.append(f"")
        lines.append(f"#define INIT_STENCIL(OP) [(OP)] = {{                \\")
        lines.append(f"    .nbytes = Py_ARRAY_LENGTH(OP##_stencil_bytes), \\")
        lines.append(f"    .bytes = OP##_stencil_bytes,                   \\")
        lines.append(f"    .nholes = Py_ARRAY_LENGTH(OP##_stencil_holes), \\")
        lines.append(f"    .holes = OP##_stencil_holes,                   \\")
        lines.append(f"}}")
        lines.append(f"")
        lines.append(f"static const Stencil stencils[256] = {{")
        assert opnames[-1] == "trampoline"
        for opname in opnames[:-1]:
            lines.append(f"    INIT_STENCIL({opname}),")
        lines.append(f"}};")
        lines.append(f"")
        lines.append(f"#define INIT_HOLE(NAME) [HOLE_##NAME] = (uintptr_t)0xBAD0BAD0BAD0BAD0")
        lines.append(f"#define INIT_LOAD(NAME) [LOAD_##NAME] = (uintptr_t)&(NAME)")
        lines.append(f"")
        lines.append(f"#define GET_PATCHES() {{ \\")
        for kind in sorted(kinds):
            if kind.startswith("HOLE_"):
                name = kind.removeprefix("HOLE_")
                lines.append(f"    INIT_HOLE({name}), \\")
            else:
                assert kind.startswith("LOAD_")
                name = kind.removeprefix("LOAD_")
                lines.append(f"    INIT_LOAD({name}), \\")
        lines.append(f"}}")
        header = []
        header.append(f"// Don't be scared... this entire file is generated by Justin!")
        header.append(f"")
        header.append(f"typedef enum {{")
        for kind in sorted(kinds):
            header.append(f"    {kind},")
        header.append(f"}} HoleKind;")
        header.append(f"")
        header.append(f"typedef struct {{")
        header.append(f"    const uintptr_t offset;")
        header.append(f"    const uintptr_t addend;")
        header.append(f"    const HoleKind kind;")
        header.append(f"}} Hole;")
        header.append(f"")
        header.append(f"typedef struct {{")
        header.append(f"    const size_t nbytes;")
        header.append(f"    const unsigned char * const bytes;")
        header.append(f"    const size_t nholes;")
        header.append(f"    const Hole * const holes;")
        header.append(f"}} Stencil;")
        header.append(f"")
        lines[:0] = header
        lines.append("")
        return "\n".join(lines)

if __name__ == "__main__":
    # First, create our JIT engine:
    engine = Engine(verbose=True)
    # This performs all of the steps that normally happen at build time:
    engine.build()
    with open(sys.argv[2], "w") as file:
        file.write(engine.dump())