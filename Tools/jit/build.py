"""A template JIT for CPython 3.13, based on copy-and-patch."""

import asyncio
import dataclasses
import enum
import hashlib
import json
import os
import pathlib
import re
import shlex
import subprocess
import sys
import tempfile
import typing

TOOLS_JIT_BUILD = pathlib.Path(__file__).resolve()
TOOLS_JIT = TOOLS_JIT_BUILD.parent
TOOLS = TOOLS_JIT.parent
ROOT = TOOLS.parent
INCLUDE = ROOT / "Include"
INCLUDE_INTERNAL = INCLUDE / "internal"
PC = ROOT / "PC"
PC_PYCONFIG_H = PC / "pyconfig.h"
PYCONFIG_H = ROOT / "pyconfig.h"
PYTHON = ROOT / "Python"
PYTHON_EXECUTOR_CASES_C_H = PYTHON / "executor_cases.c.h"
PYTHON_JIT_STENCILS_H = PYTHON / "jit_stencils.h"
TOOLS_JIT_TEMPLATE = TOOLS_JIT / "template.c"
TOOLS_JIT_TRAMPOLINE = TOOLS_JIT / "trampoline.c"


class ValueType(typing.TypedDict):
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


class RelocationType(typing.TypedDict):
    Offset: int
    Type: ValueType
    Symbol: ValueType
    Addend: int


RelocationsType = list[dict[typing.Literal["Relocation"], RelocationType]]


class SymbolType(typing.TypedDict):
    Name: ValueType
    Value: int
    Size: int
    Binding: ValueType
    Type: ValueType
    Other: int
    Section: ValueType


SymbolsType = list[dict[typing.Literal["Symbol"], SymbolType]]


class SectionType(typing.TypedDict):
    Index: int
    Name: ValueType
    Type: ValueType
    Flags: Flags
    Address: int
    Offset: int
    Size: int
    Link: int
    Info: int
    AddressAlignment: int
    EntrySize: int
    Relocations: RelocationsType
    Symbols: SymbolsType
    SectionData: SectionData


class FileSummaryType(typing.TypedDict):
    File: str
    Format: str
    Arch: str
    AddressSize: int
    LoadName: str


SectionsType = list[dict[typing.Literal["Section"], SectionType]]


class FileType(typing.TypedDict):
    FileSummary: FileSummaryType
    Sections: SectionsType


ObjectType = list[dict[str, FileType] | FileType]


HoleKind: typing.TypeAlias = typing.Literal[
    "R_386_32",
    "R_386_PC32",
    "R_AARCH64_ABS64",
    "R_AARCH64_ADR_GOT_PAGE",
    "R_AARCH64_CALL26",
    "R_AARCH64_JUMP26",
    "R_AARCH64_LD64_GOT_LO12_NC",
    "R_AARCH64_MOVW_UABS_G0_NC",
    "R_AARCH64_MOVW_UABS_G1_NC",
    "R_AARCH64_MOVW_UABS_G2_NC",
    "R_AARCH64_MOVW_UABS_G3",
    "R_X86_64_64",
    "R_X86_64_GOTOFF64",
    "R_X86_64_GOTPC32",
    "R_X86_64_GOTPCRELX",
    "R_X86_64_PC32",
    "R_X86_64_PLT32",
    "R_X86_64_REX_GOTPCRELX",
]


@enum.unique
class HoleValue(enum.Enum):
    _JIT_BASE = enum.auto()
    _JIT_CONTINUE = enum.auto()
    _JIT_CONTINUE_OPARG = enum.auto()
    _JIT_CONTINUE_OPERAND = enum.auto()
    _JIT_JUMP = enum.auto()
    _JIT_JUMP_OPARG = enum.auto()
    _JIT_JUMP_OPERAND = enum.auto()


@dataclasses.dataclass(frozen=True)
class Hole:
    kind: HoleKind
    symbol: str
    offset: int
    addend: int


@dataclasses.dataclass(frozen=True)
class Stencil:
    body: bytes
    holes: tuple[Hole, ...]
    disassembly: tuple[str, ...]
    # entry: int


S = typing.TypeVar("S", bound=str)
T = typing.TypeVar("T")


def remove_prefix(s: str, prefix: str) -> str:
    assert s.startswith(prefix), (s, prefix)
    return s.removeprefix(prefix)


def unwrap(source: list[dict[S, T]], wrapper: S) -> list[T]:
    return [child[wrapper] for child in source]


def get_llvm_tool_version(name: str) -> int | None:
    try:
        args = [name, "--version"]
        process = subprocess.run(args, check=True, stdout=subprocess.PIPE)
    except FileNotFoundError:
        return None
    match = re.search(rb"version\s+(\d+)\.\d+\.\d+\s+", process.stdout)
    return match and int(match.group(1))


def find_llvm_tool(tool: str) -> str:
    versions = {14, 15, 16}
    forced_version = os.getenv("PYTHON_LLVM_VERSION")
    if forced_version:
        versions &= {int(forced_version)}
    # Unversioned executables:
    path = tool
    version = get_llvm_tool_version(path)
    if version in versions:
        return path
    for version in sorted(versions, reverse=True):
        # Versioned executables:
        path = f"{tool}-{version}"
        if get_llvm_tool_version(path) == version:
            return path
        # My homebrew homies:
        try:
            args = ["brew", "--prefix", f"llvm@{version}"]
            process = subprocess.run(args, check=True, stdout=subprocess.PIPE)
        except (FileNotFoundError, subprocess.CalledProcessError):
            pass
        else:
            prefix = process.stdout.decode().removesuffix("\n")
            path = f"{prefix}/bin/{tool}"
            if get_llvm_tool_version(path) == version:
                return path
    raise RuntimeError(f"Can't find {tool}!")


# TODO: Divide into read-only data and writable/executable text.

_SEMAPHORE = asyncio.BoundedSemaphore(os.cpu_count() or 1)


async def run(*args: str | os.PathLike, capture: bool = False) -> bytes | None:
    async with _SEMAPHORE:
        print(shlex.join(map(str, args)))
        process = await asyncio.create_subprocess_exec(
            *args, stdout=subprocess.PIPE if capture else None, cwd=ROOT
        )
        stdout, stderr = await process.communicate()
    assert stderr is None, stderr
    if process.returncode:
        raise RuntimeError(f"{args[0]} exited with {process.returncode}")
    return stdout


class Engine:
    _ARGS = [
        "--elf-output-style=JSON",
        "--expand-relocs",
        # "--pretty-print",
        "--section-data",
        "--section-relocations",
        "--section-symbols",
        "--sections",
    ]

    def __init__(self, path: pathlib.Path, reader: str, dumper: str) -> None:
        self.path = path
        self.body = bytearray()
        self.body_symbols = {}
        self.body_offsets = {}
        self.got = {}
        self.relocations = []
        self.reader = reader
        self.dumper = dumper
        self.data_size = 0

    async def parse(self):
        output = await run(
            self.dumper, self.path, "--disassemble", "--reloc", capture=True
        )
        assert output is not None
        disassembly = [
            line.expandtabs().strip() for line in output.decode().splitlines()
        ]
        disassembly = [line for line in disassembly if line]
        output = await run(self.reader, *self._ARGS, self.path, capture=True)
        assert output is not None
        self._data: ObjectType = json.loads(output)
        file = self._data[0]
        if str(self.path) in file:
            file = file[str(self.path)]
        for section in unwrap(typing.cast(SectionsType, file["Sections"]), "Section"):
            self._handle_section(section)
        if "_JIT_ENTRY" in self.body_symbols:
            entry = self.body_symbols["_JIT_ENTRY"]
        else:
            entry = self.body_symbols["_JIT_TRAMPOLINE"]
        assert entry == 0, entry
        holes = []
        padding = 0
        while len(self.body) % 8:
            self.body.append(0)
            padding += 1
        got = len(self.body)
        for base, relocation in self.relocations:
            newhole = self._handle_relocation(base, relocation)
            if newhole is None:
                continue
            if newhole.symbol in self.body_symbols:
                addend = newhole.addend + self.body_symbols[newhole.symbol] - entry
                newhole = Hole(newhole.kind, "_JIT_BASE", newhole.offset, addend)
            holes.append(newhole)
        offset = got - self.data_size - padding
        if self.data_size:
            disassembly.append(
                f"{offset:x}: {str(bytes(self.body[offset:offset + self.data_size])).removeprefix('b')}"
            )
            offset += self.data_size
        if padding:
            disassembly.append(f"{offset:x}: {' '.join(padding * ['00'])}")
            offset += padding
        for symbol, got_offset in self.got.items():
            if symbol in self.body_symbols:
                addend = self.body_symbols[symbol]
                symbol = "_JIT_BASE"
            else:
                addend = 0
            # XXX: ABS_32 on 32-bit platforms?
            holes.append(Hole("R_X86_64_64", symbol, got + got_offset, addend))
            disassembly.append(f"{offset:x}: &{symbol}")
            offset += 8
        self.body.extend([0] * 8 * len(self.got))
        padding = 0
        while len(self.body) % 16:
            self.body.append(0)
            padding += 1
        if padding:
            disassembly.append(f"{offset:x}: {' '.join(padding * ['00'])}")
            offset += padding
        holes.sort(key=lambda hole: hole.offset)
        assert offset == len(self.body), (self.path, offset, len(self.body))
        return Stencil(
            bytes(self.body)[entry:], tuple(holes), tuple(disassembly)
        )  # XXX

    def _handle_section(self, section: SectionType) -> None:
        type = section["Type"]["Value"]
        flags = {flag["Name"] for flag in section["Flags"]["Flags"]}
        if type in {"SHT_REL", "SHT_RELA"}:
            assert "SHF_INFO_LINK" in flags, flags
            base = self.body_offsets[section["Info"]]
            assert not section["Symbols"]
            for relocation in unwrap(section["Relocations"], "Relocation"):
                self.relocations.append((base, relocation))
        elif type == "SHT_PROGBITS":
            self.body_offsets[section["Index"]] = len(self.body)
            for symbol in unwrap(section["Symbols"], "Symbol"):
                offset = len(self.body) + symbol["Value"]
                name = symbol["Name"]["Value"]
                assert name not in self.body_symbols
                self.body_symbols[name] = offset
            if "SHF_ALLOC" not in flags:
                return
            elif flags & {"SHF_EXECINSTR"}:
                # XXX: Merge these
                assert not self.data_size
                section_data = section["SectionData"]
                self.body.extend(section_data["Bytes"])
            else:
                section_data = section["SectionData"]
                self.data_size += len(section_data["Bytes"])
                self.body.extend(section_data["Bytes"])
            assert not section["Relocations"]
        else:
            assert type in {
                "SHT_GROUP",
                "SHT_LLVM_ADDRSIG",
                "SHT_NULL",
                "SHT_STRTAB",
                "SHT_SYMTAB",
            }, type

    def read_u32(self, offset: int) -> int:
        return int.from_bytes(self.body[offset : offset + 4], "little")

    def write_u32(self, offset: int, value: int) -> None:
        length = len(self.body)
        self.body[offset : offset + 4] = (value % (1 << 32)).to_bytes(4, "little")
        assert length == len(self.body), (length, len(self.body))

    def _got_lookup(self, symbol: str) -> int:
        while len(self.body) % 8:
            self.body.append(0)
        return len(self.body) + self.got.setdefault(symbol, 8 * len(self.got))

    def _handle_relocation(self, base: int, relocation: RelocationType) -> Hole | None:
        match relocation:
            case {
                "Type": {"Value": "R_386_32" | "R_386_PC32" as kind},
                "Symbol": {"Value": symbol},
                "Offset": offset,
            }:
                offset += base
                addend = self.read_u32(offset)
            case {
                "Type": {
                    "Value": "R_AARCH64_ADR_GOT_PAGE"
                    | "R_AARCH64_LD64_GOT_LO12_NC" as kind
                },
                "Symbol": {"Value": symbol},
                "Offset": offset,
                "Addend": addend,
            }:
                offset += base
                addend += self._got_lookup(symbol)
                symbol = "_JIT_BASE"
            case {
                "Type": {"Value": "R_X86_64_GOTOFF64" as kind},
                "Symbol": {"Value": symbol},
                "Offset": offset,
                "Addend": addend,
            }:
                offset += base
                addend += offset - len(self.body)
            case {
                "Type": {"Value": "R_X86_64_GOTPC32"},
                "Symbol": {"Value": "_GLOBAL_OFFSET_TABLE_"},
                "Offset": offset,
                "Addend": addend,
            }:
                offset += base
                value = len(self.body) - offset
                self.write_u32(offset, value + addend)
                return None
            case {
                "Type": {"Value": "R_X86_64_GOTPCRELX" | "R_X86_64_REX_GOTPCRELX"},
                "Symbol": {"Value": symbol},
                "Offset": offset,
                "Addend": addend,
            }:
                offset += base
                value = self._got_lookup(symbol) - offset
                self.write_u32(offset, value + addend)
                return None
            case {
                "Type": {"Value": kind},
                "Symbol": {"Value": symbol},
                "Offset": offset,
                "Addend": addend,
            }:
                offset += base
            case _:
                raise NotImplementedError(relocation)
        return Hole(kind, symbol, offset, addend)


@dataclasses.dataclass(frozen=True)
class Target:
    pattern: str
    frontend: str
    backend: str
    model: str
    ghccc: bool
    pyconfig: pathlib.Path


TARGETS = [
    Target(
        pattern=r"aarch64-apple-darwin.*",
        frontend="aarch64-apple-darwin",
        backend="aarch64-elf",
        model="large",
        ghccc=False,
        pyconfig=PYCONFIG_H,
    ),
    Target(
        pattern=r"aarch64-.*-linux-gnu",
        frontend="aarch64-unknown-linux-gnu",
        backend="aarch64-elf",
        model="large",
        ghccc=False,
        pyconfig=PYCONFIG_H,
    ),
    Target(
        pattern=r"i686-pc-windows-msvc",
        frontend="i686-pc-windows-msvc",
        backend="i686-pc-windows-msvc-elf",
        model="small",
        ghccc=True,
        pyconfig=PC_PYCONFIG_H,
    ),
    Target(
        pattern=r"x86_64-apple-darwin.*",
        frontend="x86_64-apple-darwin",
        backend="x86_64-elf",
        model="medium",
        ghccc=True,
        pyconfig=PYCONFIG_H,
    ),
    Target(
        pattern=r"x86_64-pc-windows-msvc",
        frontend="x86_64-pc-windows-msvc",
        backend="x86_64-pc-windows-msvc-elf",
        model="medium",
        ghccc=True,
        pyconfig=PC_PYCONFIG_H,
    ),
    Target(
        pattern=r"x86_64-.*-linux-gnu",
        frontend="x86_64-unknown-linux-gnu",
        backend="x86_64-elf",
        model="medium",
        ghccc=True,
        pyconfig=PYCONFIG_H,
    ),
]


def get_target(host: str) -> Target:
    for target in TARGETS:
        if re.fullmatch(target.pattern, host):
            return target
    raise NotImplementedError(host)


CFLAGS = [
    "-O3",
    "-Wno-override-module",
    # Keep library calls from sneaking in:
    "-ffreestanding",  # XXX
    # Position-independent code adds indirection to every load:
    "-fno-pic",
    # The GHC calling convention uses %rbp as an argument-passing register:
    "-fomit-frame-pointer",  # XXX
]

CPPFLAGS = [
    f"-DPy_BUILD_CORE",
    f"-D_PyJIT_ACTIVE",
    f"-I{INCLUDE}",
    f"-I{INCLUDE_INTERNAL}",
    f"-I{PYTHON}",
]


class Compiler:
    def __init__(
        self,
        *,
        verbose: bool = False,
        ghccc: bool,
        target: Target,
    ) -> None:
        self._stencils_built = {}
        self._verbose = verbose
        self._clang = find_llvm_tool("clang")
        self._readobj = find_llvm_tool("llvm-readobj")
        self._objdump = find_llvm_tool("llvm-objdump")
        self._ghccc = ghccc
        self._target = target

    def _use_ghccc(self, ll: pathlib.Path) -> None:
        if self._ghccc:
            before = ll.read_text()
            after = re.sub(
                r"((?:ptr|%struct._PyInterpreterFrame\*) @_JIT_(?:CONTINUE|ENTRY|JUMP)\b)",
                r"ghccc \1",
                before,
            )
            assert before != after, after
            ll.write_text(after)

    async def _compile(self, opname, c, tempdir) -> None:
        ll = pathlib.Path(tempdir, f"{opname}.ll").resolve()
        o = pathlib.Path(tempdir, f"{opname}.o").resolve()
        backend_flags = [
            *CFLAGS,
            f"--target={self._target.backend}",
            f"-c",
            f"-mcmodel={self._target.model}",
        ]
        frontend_flags = [
            *CFLAGS,
            *CPPFLAGS,
            f"--target={self._target.frontend}",
            f"-D_DEBUG" if sys.argv[2:] == ["-d"] else "-DNDEBUG",  # XXX
            f"-D_JIT_OPCODE={opname}",
            f"-I{self._target.pyconfig.parent}",
            f"-S",
            f"-emit-llvm",
        ]
        await run(self._clang, *frontend_flags, "-o", ll, c)
        self._use_ghccc(ll)
        await run(self._clang, *backend_flags, "-o", o, ll)
        self._stencils_built[opname] = await Engine(
            o, self._readobj, self._objdump
        ).parse()

    async def build(self) -> None:
        generated_cases = PYTHON_EXECUTOR_CASES_C_H.read_text()
        opnames = sorted(re.findall(r"\n {8}case (\w+): \{\n", generated_cases))
        with tempfile.TemporaryDirectory() as tempdir:
            async with asyncio.TaskGroup() as group:
                task = self._compile("trampoline", TOOLS_JIT_TRAMPOLINE, tempdir)
                group.create_task(task)
                for opname in opnames:
                    task = self._compile(opname, TOOLS_JIT_TEMPLATE, tempdir)
                    group.create_task(task)

def as_i64(value: int, width: int = 0) -> str:
    value %= 1 << 64
    width = max(0, width - 3)
    if value & (1 << 63):
        return f"-0x{(1 << 64) - value:0{width}x}"
    return f"+0x{value:0{width}x}"

def dump(stencils: dict[str, Stencil]) -> typing.Generator[str, None, None]:
    yield f"// $ {sys.executable} {' '.join(sys.argv)}"  # XXX
    yield f""
    yield f"typedef enum {{"
    for kind in sorted(typing.get_args(HoleKind)):
        yield f"    {kind},"
    yield f"}} HoleKind;"
    yield f""
    yield f"typedef enum {{"
    for value in HoleValue:
        yield f"    {value.name},"
    yield f"}} HoleValue;"
    yield f""
    yield f"typedef struct {{"
    yield f"    const HoleKind kind;"
    yield f"    const uint64_t offset;"
    yield f"    const uint64_t addend;"
    yield f"    const HoleValue value;"
    yield f"}} Hole;"
    yield f""
    yield f"typedef struct {{"
    yield f"    const HoleKind kind;"
    yield f"    const uint64_t offset;"
    yield f"    const uint64_t addend;"
    yield f"    const uint64_t symbol;"
    yield f"}} SymbolLoad;"
    yield f""
    yield f"typedef struct {{"
    yield f"    const size_t nbytes;"
    yield f"    const unsigned char * const bytes;"
    yield f"    const size_t nholes;"
    yield f"    const Hole * const holes;"
    yield f"    const size_t nloads;"
    yield f"    const SymbolLoad * const loads;"
    yield f"}} Stencil;"
    yield f""
    opnames = []
    symbols = set()
    for stencil in stencils.values():
        for hole in stencil.holes:
            if hole.symbol not in HoleValue.__members__:
                symbols.add(hole.symbol)
    symbols = sorted(symbols)
    for opname, stencil in sorted(stencils.items()):
        opnames.append(opname)
        yield f"// {opname}"
        assert stencil.body
        for line in stencil.disassembly:
            yield f"// {line}"
        body = ",".join(f"0x{byte:02x}" for byte in stencil.body)
        yield f"static const unsigned char {opname}_stencil_bytes[{len(stencil.body)}] = {{{body}}};"
        holes = []
        loads = []
        kind_width = max((len(f"{hole.kind}") for hole in stencil.holes), default=0)
        offset_width = max(
            (len(f"{hole.offset:x}") for hole in stencil.holes), default=0
        )
        addend_width = max(
            (len(f"{as_i64(hole.addend)}") for hole in stencil.holes), default=0
        )
        value_width = max(
            (
                len(f"{hole.symbol}")
                for hole in stencil.holes
                if hole.symbol in HoleValue.__members__
            ),
            default=0,
        )
        symbol_width = max(
            (
                len(f"{hole.symbol}")
                for hole in stencil.holes
                if hole.symbol not in HoleValue.__members__
            ),
            default=0,
        )
        for hole in stencil.holes:
            if hole.symbol in HoleValue.__members__:
                value = HoleValue[hole.symbol]
                holes.append(
                    f"    {{"
                    f".kind={hole.kind:{kind_width}}, "
                    f".offset=0x{hole.offset:0{offset_width}x}, "
                    f".addend={as_i64(hole.addend, addend_width)}, "
                    f".value={value.name:{value_width}}"
                    f"}},"
                )
            else:
                loads.append(
                    f"    {{"
                    f".kind={hole.kind:{kind_width}}, "
                    f".offset=0x{hole.offset:0{offset_width}x}, "
                    f".addend={as_i64(hole.addend, addend_width)}, "
                    f".symbol=(uintptr_t)&{hole.symbol:{symbol_width}}"
                    f"}},"
                )
        if holes:
            yield f"static const Hole {opname}_stencil_holes[{len(holes) + 1}] = {{"
            for hole in holes:
                yield hole
            yield f"}};"
        else:
            yield f"static const Hole {opname}_stencil_holes[{len(holes) + 1}];"
        if loads:
            yield f"static const SymbolLoad {opname}_stencil_loads[{len(loads) + 1}] = {{"
            for load in loads:
                yield load
            yield f"}};"
        else:
            yield f"static const SymbolLoad {opname}_stencil_loads[{len(loads) + 1}];"
        yield f""
    yield f"#define INIT_STENCIL(OP) {{                             \\"
    yield f"    .nbytes = Py_ARRAY_LENGTH(OP##_stencil_bytes),     \\"
    yield f"    .bytes = OP##_stencil_bytes,                       \\"
    yield f"    .nholes = Py_ARRAY_LENGTH(OP##_stencil_holes) - 1, \\"
    yield f"    .holes = OP##_stencil_holes,                       \\"
    yield f"    .nloads = Py_ARRAY_LENGTH(OP##_stencil_loads) - 1, \\"
    yield f"    .loads = OP##_stencil_loads,                       \\"
    yield f"}}"
    yield f""
    yield f"static const Stencil trampoline_stencil = INIT_STENCIL(trampoline);"
    yield f""
    yield f"static const Stencil stencils[512] = {{"
    assert opnames[-1] == "trampoline"
    for opname in opnames[:-1]:
        yield f"    [{opname}] = INIT_STENCIL({opname}),"
    yield f"}};"
    yield f""
    yield f"#define INIT_HOLE(NAME) [NAME] = (uint64_t)0xBADBADBADBADBADB"
    yield f""
    yield f"#define GET_PATCHES() {{ \\"
    for value in HoleValue:
        yield f"    INIT_HOLE({value.name}), \\"
    yield f"}}"
    yield f""


def main(host: str) -> None:
    target = get_target(host)
    hasher = hashlib.sha256()
    hasher.update(PYTHON_EXECUTOR_CASES_C_H.read_bytes())
    hasher.update(target.pyconfig.read_bytes())
    for file in sorted(TOOLS_JIT.iterdir()):
        hasher.update(file.read_bytes())
    for flag in CFLAGS + CPPFLAGS:
        hasher.update(flag.encode())
    digest = hasher.hexdigest()
    if PYTHON_JIT_STENCILS_H.exists():
        with PYTHON_JIT_STENCILS_H.open() as file:
            if file.readline().removeprefix("// ").removesuffix("\n") == digest:
                return
    compiler = Compiler(verbose=True, ghccc=target.ghccc, target=target)
    asyncio.run(compiler.build())
    with PYTHON_JIT_STENCILS_H.open("w") as file:
        file.write(f"// {digest}\n")
        for line in dump(compiler._stencils_built):
            file.write(f"{line}\n")


if __name__ == "__main__":
    main(sys.argv[1])
