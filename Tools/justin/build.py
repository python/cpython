"""The Justin(time) template JIT for CPython 3.13, based on copy-and-patch."""

import dataclasses
import itertools
import json
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

class _Value(typing.TypedDict):
    Value: str
    RawValue: int

class Flag(typing.TypedDict):
    Name: str
    Value: int

class Flags(typing.TypedDict):
    RawFlags: int
    Flags: list[Flag]

class SectionData(typing.TypedDict):
    Offset: int
    Bytes: list[int]

class _Name(typing.TypedDict):
    Value: str
    Offset: int
    Bytes: list[int]

class ELFRelocation(typing.TypedDict):
    Offset: int
    Type: _Value
    Symbol: _Value
    Addend: int

class COFFRelocation(typing.TypedDict):
    Offset: int
    Type: _Value
    Symbol: str
    SymbolIndex: int

class MachORelocation(typing.TypedDict):
    Offset: int
    PCRel: int
    Length: int
    Type: _Value
    Symbol: _Value  # XXX
    Section: _Value  # XXX

class COFFAuxSectionDef(typing.TypedDict):
    Length: int
    RelocationCount: int
    LineNumberCount: int
    Checksum: int
    Number: int
    Selection: int

class COFFSymbol(typing.TypedDict):
    Name: str
    Value: int
    Section: _Value
    BaseType: _Value
    ComplexType: _Value
    StorageClass: int
    AuxSymbolCount: int
    AuxSectionDef: COFFAuxSectionDef

class ELFSymbol(typing.TypedDict):
    Name: _Value
    Value: int
    Size: int
    Binding: _Value
    Type: _Value
    Other: int
    Section: _Value

class MachOSymbol(typing.TypedDict):
    Name: _Value
    Type: _Value
    Section: _Value
    RefType: _Value
    Flags: Flags
    Value: int

class ELFSection(typing.TypedDict):
    Index: int
    Name: _Value
    Type: _Value
    Flags: Flags
    Address: int
    Offset: int
    Size: int
    Link: int
    Info: int
    AddressAlignment: int
    EntrySize: int
    Relocations: list[dict[typing.Literal["Relocation"], ELFRelocation]]
    Symbols: list[dict[typing.Literal["Symbol"], ELFSymbol]]
    SectionData: SectionData

class COFFSection(typing.TypedDict):
    Number: int
    Name: _Name
    VirtualSize: int
    VirtualAddress: int
    RawDataSize: int
    PointerToRawData: int
    PointerToRelocations: int
    PointerToLineNumbers: int
    RelocationCount: int
    LineNumberCount: int
    Characteristics: Flags
    Relocations: list[dict[typing.Literal["Relocation"], COFFRelocation]]
    Symbols: list[dict[typing.Literal["Symbol"], COFFSymbol]]
    SectionData: SectionData  # XXX

class MachOSection(typing.TypedDict):
    Index: int
    Name: _Name
    Segment: _Name
    Address: int
    Size: int
    Offset: int
    Alignment: int
    RelocationOffset: int
    RelocationCount: int
    Type: _Value
    Attributes: Flags
    Reserved1: int
    Reserved2: int
    Reserved3: int
    Relocations: list[dict[typing.Literal["Relocation"], MachORelocation]]  # XXX
    Symbols: list[dict[typing.Literal["Symbol"], MachOSymbol]]  # XXX
    SectionData: SectionData  # XXX

S = typing.TypeVar("S", bound=str)
T = typing.TypeVar("T")


def unwrap(source: list[dict[S, T]], wrapper: S) -> list[T]:
    return [child[wrapper] for child in source]

# TODO: Divide into read-only data and writable/executable text.

class ObjectParser:

    _ARGS = [
        # "--demangle",
        "--elf-output-style=JSON",
        "--expand-relocs",
        "--pretty-print",
        "--section-data",
        "--section-relocations",
        "--section-symbols",
        "--sections",
    ]

    def __init__(self, path: str) -> None:
        args = ["llvm-readobj", *self._ARGS, path]
        # subprocess.run(["llvm-objdump", path, "-dr"], check=True)
        process = subprocess.run(args, check=True, capture_output=True)
        output = process.stdout
        output = output.replace(b"PrivateExtern\n", b"\n")  # XXX: MachO
        output = output.replace(b"Extern\n", b"\n")  # XXX: MachO
        start = output.index(b"[", 1)  # XXX: MachO, COFF
        end = output.rindex(b"]", 0, -1) + 1  # XXXL MachO, COFF
        self._data = json.loads(output[start:end])
        self.body = bytearray()
        self.body_symbols = {}
        self.body_offsets = {}
        self.relocations = {}
        self.dupes = set()
        self.got_entries = []
        self.relocations_todo = []

    def parse(self):
        for section in unwrap(self._data, "Section"):
            self._handle_section(section)
        if "_justin_entry" in self.body_symbols:
            entry = self.body_symbols["_justin_entry"]
        else:
            entry = self.body_symbols["_justin_trampoline"]
        holes = []
        for before, relocation in self.relocations_todo:
            for newhole in handle_one_relocation(self.got_entries, self.body, before, relocation):
                assert newhole.symbol not in self.dupes
                if newhole.symbol in self.body_symbols:
                    addend = newhole.addend + self.body_symbols[newhole.symbol] - entry
                    newhole = Hole("_justin_base", newhole.offset, addend, newhole.pc)
                holes.append(newhole)
        got = len(self.body)
        for i, got_symbol in enumerate(self.got_entries):
            if got_symbol in self.body_symbols:
                self.body_symbols[got_symbol] -= entry
            holes.append(Hole(got_symbol, got + 8 * i, 0, 0))
        self.body.extend([0] * 8 * len(self.got_entries))
        holes.sort(key=lambda hole: hole.offset)
        return Stencil(bytes(self.body)[entry:], tuple(holes))  # XXX

@dataclasses.dataclass(frozen=True)
class Hole:
    symbol: str
    offset: int
    addend: int
    pc: int

@dataclasses.dataclass(frozen=True)
class Stencil:
    body: bytes
    holes: tuple[Hole, ...]
    # entry: int

def handle_one_relocation(
    got_entries: list[str],
    body: bytearray,
    base: int,
    relocation: typing.Mapping[str, typing.Any],
) -> typing.Generator[Hole, None, None]:
    match relocation:
        case {
            'Offset': int(offset),
            'Symbol': str(symbol),
            'Type': {'Value': 'IMAGE_REL_AMD64_ADDR64'},
        }:
            offset += base
            where = slice(offset, offset + 8)
            what = int.from_bytes(body[where], sys.byteorder)
            # assert not what, what
            addend = what
            body[where] = [0] * 8
            yield Hole(symbol, offset, addend, 0)
        case {
            "Addend": int(addend),
            "Offset": int(offset),
            "Symbol": {"Value": str(symbol)},
            "Type": {"Value": "R_X86_64_64"},
        }:
            offset += base
            where = slice(offset, offset + 8)
            what = int.from_bytes(body[where], sys.byteorder)
            assert not what, what
            yield Hole(symbol, offset, addend, 0)
        case {
            "Addend": int(addend),
            "Offset": int(offset),
            "Symbol": {"Value": str(symbol)},
            "Type": {"Value": "R_X86_64_GOT64"},
        }:
            offset += base
            where = slice(offset, offset + 8)
            what = int.from_bytes(body[where], sys.byteorder)
            assert not what, what
            if symbol not in got_entries:
                got_entries.append(symbol)
            addend += got_entries.index(symbol) * 8
            body[where] = int(addend).to_bytes(8, sys.byteorder)
        case {
            "Addend": int(addend),
            "Offset": int(offset),
            "Symbol": {"Value": str(symbol)},
            "Type": {"Value": "R_X86_64_GOTOFF64"},
        }:
            offset += base
            where = slice(offset, offset + 8)
            what = int.from_bytes(body[where], sys.byteorder)
            assert not what, what
            addend += offset - len(body)
            yield Hole(symbol, offset, addend, -1)
        case {
            "Addend": int(addend),
            "Offset": int(offset),
            "Symbol": {"Value": "_GLOBAL_OFFSET_TABLE_"},
            "Type": {"Value": "R_X86_64_GOTPC64"},
        }:
            offset += base
            where = slice(offset, offset + 8)
            what = int.from_bytes(body[where], sys.byteorder)
            assert not what, what
            addend += len(body) - offset
            body[where] = int(addend).to_bytes(8, sys.byteorder)
        case {
            "Addend": int(addend),
            "Offset": int(offset),
            "Symbol": {"Value": str(symbol)},
            "Type": {"Value": "R_X86_64_PC32"},
        }:
            offset += base
            where = slice(offset, offset + 4)  # XXX: The jit can only do 8 right now...
            what = int.from_bytes(body[where], sys.byteorder)
            assert not what, what
            yield Hole(symbol, offset, addend, -1)
        case {
            "Length": 3,
            "Offset": int(offset),
            "PCRel": 0,
            "Section": {"Value": str(section)},
            "Type": {"Value": "X86_64_RELOC_UNSIGNED"},
        }:
            offset += base
            where = slice(offset, offset + 8)
            what = int.from_bytes(body[where], sys.byteorder)
            # assert not what, what
            addend = what
            body[where] = [0] * 8
            assert section.startswith("_")
            section = section.removeprefix("_")
            yield Hole(section, offset, addend, 0)
        case {
            "Length": 3,
            "Offset": int(offset),
            "PCRel": 0,
            "Symbol": {"Value": str(symbol)},
            "Type": {"Value": "X86_64_RELOC_UNSIGNED"},
        }:
            offset += base
            where = slice(offset, offset + 8)
            what = int.from_bytes(body[where], sys.byteorder)
            # assert not what, what
            addend = what
            body[where] = [0] * 8
            assert symbol.startswith("_")
            symbol = symbol.removeprefix("_")
            if symbol == "__bzero":  # XXX
                symbol = "bzero"  # XXX
            yield Hole(symbol, offset, addend, 0)
        case _:
            raise NotImplementedError(relocation)


class ObjectParserCOFF(ObjectParser):

    def _handle_section(self, section: COFFSection) -> None:
        flags = {flag["Name"] for flag in section["Characteristics"]["Flags"]}
        if "SectionData" not in section:
            return
        if flags & {"IMAGE_SCN_LINK_COMDAT", "IMAGE_SCN_MEM_EXECUTE", "IMAGE_SCN_MEM_READ", "IMAGE_SCN_MEM_WRITE"} == {"IMAGE_SCN_LINK_COMDAT", "IMAGE_SCN_MEM_READ"}:
            # XXX: Merge these
            before = self.body_offsets[section["Number"]] = len(self.body)
            section_data = section["SectionData"]
            self.body.extend(section_data["Bytes"])
        elif flags & {"IMAGE_SCN_MEM_READ"} == {"IMAGE_SCN_MEM_READ"}:
            before = self.body_offsets[section["Number"]] = len(self.body)
            section_data = section["SectionData"]
            self.body.extend(section_data["Bytes"])
        else:
            return
        for symbol in unwrap(section["Symbols"], "Symbol"):
            offset = before + symbol["Value"]
            name = symbol["Name"]
            if name in self.body_symbols:
                self.dupes.add(name)
            self.body_symbols[name] = offset
        for relocation in unwrap(section["Relocations"], "Relocation"):
            self.relocations_todo.append((before, relocation))

class ObjectParserMachO(ObjectParser):

    def _handle_section(self, section: MachOSection) -> None:
        assert section["Address"] >= len(self.body)
        self.body.extend([0] * (section["Address"] - len(self.body)))
        before = self.body_offsets[section["Index"]] = section["Address"]
        section_data = section["SectionData"]
        self.body.extend(section_data["Bytes"])
        name = section["Name"]["Value"]
        assert name.startswith("_")
        name = name.removeprefix("_")
        if name in self.body_symbols:
            self.dupes.add(name)
        self.body_symbols[name] = 0  # before
        for symbol in unwrap(section["Symbols"], "Symbol"):
            offset = symbol["Value"]
            name = symbol["Name"]["Value"]
            assert name.startswith("_")
            name = name.removeprefix("_")
            if name in self.body_symbols:
                self.dupes.add(name)
            self.body_symbols[name] = offset
        for relocation in unwrap(section["Relocations"], "Relocation"):
            self.relocations_todo.append((before, relocation))


class ObjectParserELF(ObjectParser):

    def _handle_section(self, section: ELFSection) -> None:
        type = section["Type"]["Value"]
        flags = {flag["Name"] for flag in section["Flags"]["Flags"]}
        if type == "SHT_RELA":
            assert "SHF_INFO_LINK" in flags, flags
            before = self.body_offsets[section["Info"]]
            assert not section["Symbols"]
            for relocation in unwrap(section["Relocations"], "Relocation"):
                self.relocations_todo.append((before, relocation))
        elif type == "SHT_PROGBITS":
            before = self.body_offsets[section["Index"]] = len(self.body)
            if "SHF_ALLOC" not in flags:
                return
            elif flags & {"SHF_EXECINSTR", "SHF_MERGE", "SHF_WRITE"} == {"SHF_MERGE"}:
                # XXX: Merge these
                section_data = section["SectionData"]
                self.body.extend(section_data["Bytes"])
            else:
                section_data = section["SectionData"]
                self.body.extend(section_data["Bytes"])
            assert not section["Relocations"]
            for symbol in unwrap(section["Symbols"], "Symbol"):
                offset = before + symbol["Value"]
                name = symbol["Name"]["Value"]
                assert name not in self.body_symbols
                self.body_symbols[name] = offset
        else:
            assert type in {"SHT_LLVM_ADDRSIG", "SHT_NULL", "SHT_STRTAB", "SHT_SYMTAB"}, type


CFLAGS = [
    "-DNDEBUG",  # XXX
    "-DPy_BUILD_CORE",
    "-D_PyJIT_ACTIVE",
    "-I.",
    "-I./Include",
    "-I./Include/internal",
    "-I./PC",
    "-O3",
    "-Wno-unreachable-code",
    "-Wno-unused-but-set-variable",
    "-Wno-unused-command-line-argument",
    "-Wno-unused-label",
    "-Wno-unused-variable",
    # We don't need this (and it causes weird relocations):
    "-fno-asynchronous-unwind-tables",  # XXX
    # # Don't need the overhead of position-independent code, if posssible:
    # "-fno-pic",
    # Disable stack-smashing canaries, which use magic symbols:
    "-fno-stack-protector",  # XXX
    # The GHC calling convention uses %rbp as an argument-passing register:
    "-fomit-frame-pointer",  # XXX
    # Disable debug info:
    "-g0",  # XXX
    # Need this to leave room for patching our 64-bit pointers:
    "-mcmodel=large",  # XXX
]

if sys.platform == "darwin":
    ObjectParserDefault = ObjectParserMachO
elif sys.platform == "linux":
    ObjectParserDefault = ObjectParserELF
elif sys.platform == "win32":
    ObjectParserDefault = ObjectParserCOFF
    assert sys.argv[1] == "--windows"
    if sys.argv[2].startswith("Debug|"):
        CFLAGS += ["-D_DEBUG"]
    # else:  # XXX
    #     CFLAGS += ["-DNDEBUG"]
    sys.argv[1:] = sys.argv[3:]
else:
    raise NotImplementedError(sys.platform)

class Compiler:
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
            "EXTENDED_ARG",  # XXX: Only because we don't handle extended args correctly...
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
            "INSTRUMENTED_LOAD_SUPER_ATTR", # XXX
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
            "KW_NAMES",  # XXX: Only because we don't handle kwnames correctly...
            "LOAD_CLASSDEREF",
            "LOAD_CLOSURE",
            "LOAD_DEREF",
            "LOAD_FAST_CHECK",
            "LOAD_FROM_DICT_OR_DEREF",
            "LOAD_FROM_DICT_OR_GLOBALS",
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
    def _use_ghccc(path: str) -> None:  # XXX
        # ll = pathlib.Path(path)
        path.seek(0)
        ir = path.read().decode()
        ir = ir.replace("i32 @_justin_continue", "ghccc i32 @_justin_continue")
        ir = ir.replace("i32 @_justin_entry", "ghccc i32 @_justin_entry")
        path.seek(0)
        path.write(ir.encode())
        path.seek(0)

    def _compile(self, opname, path) -> Stencil:
        self._stderr(f"Building stencil for {opname}.")
        defines = [f"-D_JUSTIN_OPCODE={opname}"]
        with tempfile.NamedTemporaryFile(suffix=".o") as o, tempfile.NamedTemporaryFile(suffix=".ll") as ll:
            subprocess.run(
                ["clang", *CFLAGS, "-emit-llvm", "-S", *defines, "-o", ll.name, path],
                check=True,
            )
            self._use_ghccc(ll)
            subprocess.run(
                ["clang", *CFLAGS, "-c", "-o", o.name, ll.name],
                check=True,
            )
            return ObjectParserDefault(o.name).parse()

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
            "HOLE_oparg_plus_one",
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
                lines.append(f"    {{.offset = {hole.offset:4}, .addend = {hole.addend:4}, .kind = {kind}, .pc = {hole.pc}}},")
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
        header.append(f"    const int pc;")
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
    engine = Compiler(verbose=True)
    # This performs all of the steps that normally happen at build time:
    # TODO: Actual arg parser...
    engine.build()
    with open(sys.argv[2], "w") as file:
        file.write(engine.dump())