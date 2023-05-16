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

if sys.platform == "win32":

    class Relocation(typing.TypedDict):
        Offset: int
        Type: _Value
        Symbol: str
        SymbolIndex: int

    class AuxSectionDef(typing.TypedDict):
        Length: int
        RelocationCount: int
        LineNumberCount: int
        Checksum: int
        Number: int
        Selection: int

    class Symbol(typing.TypedDict):
        Name: str
        Value: int
        Section: _Value
        BaseType: _Value
        ComplexType: _Value
        StorageClass: int
        AuxSymbolCount: int
        AuxSectionDef: AuxSectionDef

    class Section(typing.TypedDict):
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
        Relocations: list[dict[typing.Literal["Relocation"], Relocation]]
        Symbols: list[dict[typing.Literal["Symbol"], Symbol]]
        SectionData: SectionData  # XXX

elif sys.platform == "darwin":
    
    class Relocation(typing.TypedDict):
        Offset: int
        PCRel: int
        Length: int
        Type: _Value
        Symbol: _Value  # XXX
        Section: _Value  # XXX

    class Symbol(typing.TypedDict):
        Name: _Value
        Type: _Value
        Section: _Value
        RefType: _Value
        Flags: Flags
        Value: int

    class Section(typing.TypedDict):
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
        Relocations: list[dict[typing.Literal["Relocation"], Relocation]]  # XXX
        Symbols: list[dict[typing.Literal["Symbol"], Symbol]]  # XXX
        SectionData: SectionData  # XXX

elif sys.platform == "linux":

    class Relocation(typing.TypedDict):
        Offset: int
        Type: _Value
        Symbol: _Value
        Addend: int

    class Symbol(typing.TypedDict):
        Name: _Value
        Value: int
        Size: int
        Binding: _Value
        Type: _Value
        Other: int
        Section: _Value

    class Section(typing.TypedDict):
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
        Relocations: list[dict[typing.Literal["Relocation"], Relocation]]
        Symbols: list[dict[typing.Literal["Symbol"], Symbol]]
        SectionData: SectionData
else:
    assert False, sys.platform

Sections = list[dict[typing.Literal["Section"], Section]]

S = typing.TypeVar("S", bound=str)
T = typing.TypeVar("T")


def unwrap(source: list[dict[S, T]], wrapper: S) -> list[T]:
    return [child[wrapper] for child in source]
    
# TODO: Divide into read-only data and writable/executable text.

class NewObjectParser:

    _ARGS = [
        "--demangle",
        "--elf-output-style=JSON",
        "--expand-relocs",
        "--pretty-print",
        "--sections",
        "--section-data",
        "--section-relocations",
        "--section-symbols",
    ]

    def __init__(self, path: str) -> None:
        args = ["llvm-readobj", *self._ARGS, path]
        process = subprocess.run(args, check=True, capture_output=True)
        output = process.stdout
        output = output.replace(b"}Extern\n", b"}\n")  # XXX: macOS
        start = output.index(b"[", 1)  # XXX
        end = output.rindex(b"]", 0, -1) + 1  # XXX
        self._data: Sections = json.loads(output[start:end])

    if sys.platform == "win32":

        def parse(self):
            body = bytearray()
            body_symbols = {}
            body_offsets = {}
            relocations = {}
            dupes = set()
            for section in unwrap(self._data, "Section"):
                flags = {flag["Name"] for flag in section["Characteristics"]["Flags"]}
                if "SectionData" not in section:
                    continue
                if flags & {"IMAGE_SCN_LINK_COMDAT", "IMAGE_SCN_MEM_EXECUTE", "IMAGE_SCN_MEM_READ", "IMAGE_SCN_MEM_WRITE"} == {"IMAGE_SCN_LINK_COMDAT", "IMAGE_SCN_MEM_READ"}:
                    # XXX: Merge these
                    before = body_offsets[section["Number"]] = len(body)
                    section_data = section["SectionData"]
                    body.extend(section_data["Bytes"])
                elif flags & {"IMAGE_SCN_MEM_READ"} == {"IMAGE_SCN_MEM_READ"}:
                    before = body_offsets[section["Number"]] = len(body)
                    section_data = section["SectionData"]
                    body.extend(section_data["Bytes"])
                else:
                    continue
                for symbol in unwrap(section["Symbols"], "Symbol"):
                    offset = before + symbol["Value"]
                    name = symbol["Name"]
                    if name in body_symbols:
                        dupes.add(name)
                    body_symbols[name] = offset
                for relocation in unwrap(section["Relocations"], "Relocation"):
                    offset = before + relocation["Offset"]
                    assert offset not in relocations
                    # XXX: Addend
                    addend = int.from_bytes(body[offset:offset + 8], sys.byteorder)
                    body[offset:offset + 8] = [0] * 8
                    relocations[offset] = (relocation["Symbol"], relocation["Type"]["Value"], addend)
            if "_justin_entry" in body_symbols:
                entry = body_symbols["_justin_entry"]
            else:
                entry = body_symbols["_justin_trampoline"]
            holes = []
            for offset, (symbol, type, addend) in relocations.items():
                assert type == "IMAGE_REL_AMD64_ADDR64"
                assert symbol not in dupes
                if symbol in body_symbols:
                    addend += body_symbols[symbol] - entry
                    symbol = "_justin_base"
                holes.append(Hole(symbol, offset, addend))
            holes.sort(key=lambda hole: hole.offset)
            return Stencil(bytes(body)[entry:], tuple(holes))  # XXX

    elif sys.platform == "darwin":

        def parse(self):
            body = bytearray()
            body_symbols = {}
            body_offsets = {}
            relocations = {}
            dupes = set()
            for section in unwrap(self._data, "Section"):
                assert section["Address"] >= len(body)
                body.extend([0] * (section["Address"] - len(body)))
                before = body_offsets[section["Index"]] = section["Address"]
                section_data = section["SectionData"]
                body.extend(section_data["Bytes"])
                name = section["Name"]["Value"]
                assert name.startswith("_")
                name = name.removeprefix("_")
                if name in body_symbols:
                    dupes.add(name)
                body_symbols[name] = 0  # before
                for symbol in unwrap(section["Symbols"], "Symbol"):
                    offset = symbol["Value"]
                    name = symbol["Name"]["Value"]
                    assert name.startswith("_")
                    name = name.removeprefix("_")
                    if name in body_symbols:
                        dupes.add(name)
                    body_symbols[name] = offset
                for relocation in unwrap(section["Relocations"], "Relocation"):
                    offset = before + relocation["Offset"]
                    assert offset not in relocations
                    # XXX: Addend
                    name = relocation["Symbol"]["Value"] if "Symbol" in relocation else relocation["Section"]["Value"]
                    assert name.startswith("_")
                    name = name.removeprefix("_")
                    if name == "__bzero":  # XXX
                        name = "bzero"  # XXX
                    addend = int.from_bytes(body[offset:offset + 8], sys.byteorder)
                    body[offset:offset + 8] = [0] * 8
                    relocations[offset] = (name, relocation["Type"]["Value"], addend)
            if "_justin_entry" in body_symbols:
                entry = body_symbols["_justin_entry"]
            else:
                entry = body_symbols["_justin_trampoline"]
            holes = []
            for offset, (symbol, type, addend) in relocations.items():
                assert type == "X86_64_RELOC_UNSIGNED", type
                assert symbol not in dupes
                if symbol in body_symbols:
                    addend += body_symbols[symbol] - entry
                    symbol = "_justin_base"
                holes.append(Hole(symbol, offset, addend))
            holes.sort(key=lambda hole: hole.offset)
            return Stencil(bytes(body)[entry:], tuple(holes))  # XXX 

    elif sys.platform == "linux":

        def parse(self):
            body = bytearray()
            body_symbols = {}
            body_offsets = {}
            relocations = {}
            for section in unwrap(self._data, "Section"):
                type = section["Type"]["Value"]
                flags = {flag["Name"] for flag in section["Flags"]["Flags"]}
                if type == "SHT_RELA":
                    assert "SHF_INFO_LINK" in flags, flags
                    before = body_offsets[section["Info"]]
                    assert not section["Symbols"]
                    for relocation in unwrap(section["Relocations"], "Relocation"):
                        offset = before + relocation["Offset"]
                        assert offset not in relocations
                        addend = int.from_bytes(body[offset:offset + 8], sys.byteorder)
                        body[offset:offset + 8] = [0] * 8
                        addend += relocation["Addend"]
                        relocations[offset] = (relocation["Symbol"]["Value"], relocation["Type"]["Value"], addend)
                elif type == "SHT_PROGBITS":
                    if "SHF_ALLOC" not in flags:
                        continue
                    elif flags & {"SHF_EXECINSTR", "SHF_MERGE", "SHF_WRITE"} == {"SHF_MERGE"}:
                        # XXX: Merge these
                        before = body_offsets[section["Index"]] = len(body)
                        section_data = section["SectionData"]
                        body.extend(section_data["Bytes"])
                    else:
                        before = body_offsets[section["Index"]] = len(body)
                        section_data = section["SectionData"]
                        body.extend(section_data["Bytes"])
                    assert not section["Relocations"]
                    for symbol in unwrap(section["Symbols"], "Symbol"):
                        offset = before + symbol["Value"]
                        name = symbol["Name"]["Value"]
                        assert name not in body_symbols
                        body_symbols[name] = offset
                else:
                    assert type in {"SHT_LLVM_ADDRSIG", "SHT_NULL", "SHT_STRTAB", "SHT_SYMTAB"}, type
                    continue
            if "_justin_entry" in body_symbols:
                entry = body_symbols["_justin_entry"]
            else:
                entry = body_symbols["_justin_trampoline"]
            holes = []
            for offset, (symbol, type, addend) in relocations.items():
                assert type == "R_X86_64_64"
                if symbol in body_symbols:
                    addend += body_symbols[symbol] - entry
                    symbol = "_justin_base"
                holes.append(Hole(symbol, offset, addend))
            holes.sort(key=lambda hole: hole.offset)
            return Stencil(bytes(body)[entry:], tuple(holes))  # XXX   

# class ObjectParserCOFFX8664(ObjectParser):
#     _file_format = "coff-x86-64"
#     _type = "IMAGE_REL_AMD64_ADDR64"
#     _section_prefix = ""
#     _fix_up_holes = True
#     _symbol_prefix = ""

# class ObjectParserELF64X8664(ObjectParser):
#     _file_format = "elf64-x86-64"
#     _type = "R_X86_64_64"
#     _section_prefix = ""
#     _fix_up_holes = True
#     _symbol_prefix = ""

# class ObjectParserMachO64BitARM64(ObjectParser):
#     _file_format = "mach-o 64-bit arm64"
#     _type = "ARM64_RELOC_UNSIGNED"
#     _section_prefix = ""
#     _fix_up_holes = True
#     _symbol_prefix = "_"

# class ObjectParserMachO64BitX8664(ObjectParser):
#     _file_format = "mach-o 64-bit x86-64"
#     _type = "X86_64_RELOC_UNSIGNED"
#     _section_prefix = ""
#     _fix_up_holes = True
#     _symbol_prefix = "_"

@dataclasses.dataclass(frozen=True)
class Hole:
    symbol: str
    offset: int
    addend: int

@dataclasses.dataclass(frozen=True)
class Stencil:
    body: bytes
    holes: tuple[Hole, ...]
    # entry: int

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
        # Disable stack-smashing canaries, which use the global offset table:
        "-fno-stack-protector",
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
            return NewObjectParser(o.name).parse()

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