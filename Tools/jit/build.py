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
PYTHON = ROOT / "Python"
PYTHON_EXECUTOR_CASES_C_H = PYTHON / "executor_cases.c.h"
PYTHON_JIT_STENCILS_H = PYTHON / "jit_stencils.h"
TOOLS_JIT_TEMPLATE = TOOLS_JIT / "template.c"
TOOLS_JIT_TRAMPOLINE = TOOLS_JIT / "trampoline.c"


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
    SYMBOL_PREFIX = ""

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

    def __init__(self, path: pathlib.Path, reader: str, dumper: str) -> None:
        self.path = path
        self.body = bytearray()
        self.body_symbols = {}
        self.body_offsets = {}
        self.relocations = {}
        self.got_entries = []
        self.relocations_todo = []
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
        output = output.replace(b"PrivateExtern\n", b"\n")  # XXX: MachO
        output = output.replace(b"Extern\n", b"\n")  # XXX: MachO
        start = output.index(b"[", 1)  # XXX: MachO, COFF
        end = output.rindex(b"]", 0, -1) + 1  # XXX: MachO, COFF
        self._data = json.loads(output[start:end])
        for section in unwrap(self._data, "Section"):
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
        for base, relocation in self.relocations_todo:
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
                f"{offset:x}: "
                + f"{str(bytes(self.body[offset:offset + self.data_size])).removeprefix('b')}".expandtabs()
            )
            offset += self.data_size
        if padding:
            disassembly.append(
                f"{offset:x}: " + f"{' '.join(padding * ['00'])}".expandtabs()
            )
            offset += padding
        for i, (got_symbol, addend) in enumerate(self.got_entries):
            if got_symbol in self.body_symbols:
                addend = self.body_symbols[got_symbol] + addend
                got_symbol = "_JIT_BASE"
            # XXX: ABS_32 on 32-bit platforms?
            holes.append(Hole(HoleKind.ABS_64, got_symbol, got + 8 * i, addend))
            symbol_part = f"&{got_symbol}{f' + 0x{addend:x}' if addend else ''}"
            disassembly.append(f"{offset:x}: " + f"{symbol_part}".expandtabs())
            offset += 8
        self.body.extend([0] * 8 * len(self.got_entries))
        padding = 0
        while len(self.body) % 16:
            self.body.append(0)
            padding += 1
        if padding:
            disassembly.append(
                f"{offset:x}: " + f"{' '.join(padding * ['00'])}".expandtabs()
            )
            offset += padding
        holes.sort(key=lambda hole: hole.offset)
        assert offset == len(self.body), (self.path, offset, len(self.body))
        return Stencil(
            bytes(self.body)[entry:], tuple(holes), tuple(disassembly)
        )  # XXX


class CEnum(enum.Enum):
    @staticmethod
    def _generate_next_value_(name: str, start, count, last_values) -> str:
        return name

    def __str__(self) -> str:
        return f"{self.__class__.__name__}_{self.value}"

    @classmethod
    def define(cls) -> typing.Generator[str, None, None]:
        yield f"typedef enum {{"
        for name in cls:
            yield f"    {name},"
        yield f"}} {cls.__name__};"


@enum.unique
class HoleKind(CEnum):
    ABS_12 = enum.auto()
    ABS_16_A = enum.auto()
    ABS_16_B = enum.auto()
    ABS_16_C = enum.auto()
    ABS_16_D = enum.auto()
    ABS_32 = enum.auto()
    ABS_64 = enum.auto()
    REL_21 = enum.auto()
    REL_26 = enum.auto()
    REL_32 = enum.auto()
    REL_64 = enum.auto()


@enum.unique
class HoleValue(CEnum):
    BASE = enum.auto()
    CONTINUE = enum.auto()
    CONTINUE_OPARG = enum.auto()
    CONTINUE_OPERAND = enum.auto()
    JUMP = enum.auto()
    JUMP_OPARG = enum.auto()
    JUMP_OPERAND = enum.auto()


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


def sign_extend_64(value: int, bits: int) -> int:
    """Sign-extend a value to 64 bits."""
    assert 0 <= value < (1 << bits) < (1 << 64)
    return value - ((value & (1 << (bits - 1))) << 1)


class COFF(Engine):
    def _handle_section(self, section: COFFSection) -> None:
        flags = {flag["Name"] for flag in section["Characteristics"]["Flags"]}
        if "SectionData" not in section:
            return
        section_data = section["SectionData"]
        if flags & {
            "IMAGE_SCN_LINK_COMDAT",
            "IMAGE_SCN_MEM_EXECUTE",
            "IMAGE_SCN_MEM_READ",
            "IMAGE_SCN_MEM_WRITE",
        } == {"IMAGE_SCN_LINK_COMDAT", "IMAGE_SCN_MEM_READ"}:
            # XXX: Merge these
            self.data_size += len(section_data["Bytes"])
            before = self.body_offsets[section["Number"]] = len(self.body)
            self.body.extend(section_data["Bytes"])
        elif flags & {"IMAGE_SCN_MEM_EXECUTE"}:
            assert not self.data_size, self.data_size
            before = self.body_offsets[section["Number"]] = len(self.body)
            self.body.extend(section_data["Bytes"])
        elif flags & {"IMAGE_SCN_MEM_READ"}:
            self.data_size += len(section_data["Bytes"])
            before = self.body_offsets[section["Number"]] = len(self.body)
            self.body.extend(section_data["Bytes"])
        else:
            return
        for symbol in unwrap(section["Symbols"], "Symbol"):
            offset = before + symbol["Value"]
            name = symbol["Name"]
            # assert name.startswith(self.SYMBOL_PREFIX)  # XXX
            name = name.removeprefix(self.SYMBOL_PREFIX)  # XXX
            self.body_symbols[name] = offset
        for relocation in unwrap(section["Relocations"], "Relocation"):
            self.relocations_todo.append((before, relocation))


class x86_64_pc_windows_msvc(COFF):
    pattern = re.compile(r"x86_64-pc-windows-msvc")

    def _handle_relocation(
        self,
        base: int,
        relocation: dict[str, typing.Any],
    ) -> Hole | None:
        match relocation:
            case {
                "Offset": int(offset),
                "Symbol": str(symbol),
                "Type": {"Value": "IMAGE_REL_AMD64_ADDR64"},
            }:
                offset += base
                where = slice(offset, offset + 8)
                what = int.from_bytes(self.body[where], sys.byteorder)
                # assert not what, what
                addend = what
                self.body[where] = [0] * 8
                return Hole(HoleKind.ABS_64, symbol, offset, addend)
            case _:
                raise NotImplementedError(relocation)


class i686_pc_windows_msvc(COFF):
    pattern = re.compile(r"i686-pc-windows-msvc")
    SYMBOL_PREFIX = "_"

    def _handle_relocation(
        self,
        base: int,
        relocation: dict[str, typing.Any],
    ) -> Hole | None:
        match relocation:
            case {
                "Offset": int(offset),
                "Symbol": str(symbol),
                "Type": {"Value": "IMAGE_REL_I386_DIR32"},
            }:
                offset += base
                where = slice(offset, offset + 4)
                what = int.from_bytes(self.body[where], sys.byteorder)
                # assert not what, what
                addend = what
                self.body[where] = [0] * 4
                # assert symbol.startswith(self.SYMBOL_PREFIX)
                symbol = symbol.removeprefix(self.SYMBOL_PREFIX)
                return Hole(HoleKind.ABS_32, symbol, offset, addend)
            case _:
                raise NotImplementedError(relocation)


class MachO(Engine):
    SYMBOL_PREFIX = "_"

    def _handle_section(self, section: MachOSection) -> None:
        assert section["Address"] >= len(self.body)
        section_data = section["SectionData"]
        flags = {flag["Name"] for flag in section["Attributes"]["Flags"]}
        if flags & {"SomeInstructions"}:
            assert not self.data_size
            self.body.extend([0] * (section["Address"] - len(self.body)))
            before = self.body_offsets[section["Index"]] = section["Address"]
            self.body.extend(section_data["Bytes"])
        else:
            self.data_size += section["Address"] - len(self.body)
            self.body.extend([0] * (section["Address"] - len(self.body)))
            before = self.body_offsets[section["Index"]] = section["Address"]
            self.data_size += len(section_data["Bytes"])
            self.body.extend(section_data["Bytes"])
        name = section["Name"]["Value"]
        # assert name.startswith(self.SYMBOL_PREFIX)  # XXX
        name = name.removeprefix(self.SYMBOL_PREFIX)  # XXX
        if name == "_eh_frame":
            return
        self.body_symbols[name] = 0  # before
        for symbol in unwrap(section["Symbols"], "Symbol"):
            offset = symbol["Value"]
            name = symbol["Name"]["Value"]
            # assert name.startswith(self.SYMBOL_PREFIX)  # XXX
            name = name.removeprefix(self.SYMBOL_PREFIX)  # XXX
            self.body_symbols[name] = offset
        for relocation in unwrap(section["Relocations"], "Relocation"):
            self.relocations_todo.append((before, relocation))


class aarch64_apple_darwin(MachO):
    pattern = re.compile(r"aarch64-apple-darwin.*")

    def _handle_relocation(
        self,
        base: int,
        relocation: dict[str, typing.Any],
    ) -> Hole | None:
        match relocation:
            case {
                "Length": 2 as length,
                "Offset": int(offset),
                "PCRel": 1 as pcrel,
                "Symbol": {"Value": str(symbol)},
                "Type": {"Value": "ARM64_RELOC_BRANCH26"},
            }:
                offset += base
                where = slice(offset, offset + (1 << length))
                what = int.from_bytes(self.body[where], "little", signed=False)
                # XXX: This nonsense...
                assert (
                    what & 0xFC000000 == 0x14000000 or what & 0xFC000000 == 0x94000000
                ), what
                addend = (what & 0x03FFFFFF) << 2
                addend = sign_extend_64(addend, 28)
                symbol = remove_prefix(symbol, self.SYMBOL_PREFIX)
                return Hole(HoleKind.REL_26, symbol, offset, addend)
            case {
                "Length": 2 as length,
                "Offset": int(offset),
                "PCRel": 1 as pcrel,
                "Symbol": {"Value": str(symbol)},
                "Type": {"Value": "ARM64_RELOC_GOT_LOAD_PAGE21"},
            }:
                offset += base
                where = slice(offset, offset + (1 << length))
                what = int.from_bytes(self.body[where], "little", signed=False)
                # XXX: This nonsense...
                assert what & 0x9F000000 == 0x90000000, what
                addend = ((what & 0x60000000) >> 29) | ((what & 0x01FFFFE0) >> 3) << 12
                addend = sign_extend_64(addend, 33)
                symbol = symbol.removeprefix(self.SYMBOL_PREFIX)
                if (symbol, addend) not in self.got_entries:
                    self.got_entries.append((symbol, addend))
                addend = len(self.body) + self.got_entries.index((symbol, addend)) * 8
                return Hole(HoleKind.REL_21, "_JIT_BASE", offset, addend)
            case {
                "Length": 2 as length,
                "Offset": int(offset),
                "PCRel": 0 as pcrel,
                "Symbol": {"Value": str(symbol)},
                "Type": {"Value": "ARM64_RELOC_GOT_LOAD_PAGEOFF12"},
            }:
                offset += base
                where = slice(offset, offset + (1 << length))
                what = int.from_bytes(self.body[where], "little", signed=False)
                # XXX: This nonsense...
                assert what & 0x3B000000 == 0x39000000, what
                addend = (what & 0x003FFC00) >> 10
                implicit_shift = 0
                if what & 0x3B000000 == 0x39000000:
                    implicit_shift = (what >> 30) & 0x3
                    if implicit_shift == 0:
                        if what & 0x04800000 == 0x04800000:
                            implicit_shift = 4
                addend <<= implicit_shift
                symbol = symbol.removeprefix(self.SYMBOL_PREFIX)
                if (symbol, addend) not in self.got_entries:
                    self.got_entries.append((symbol, addend))
                addend = len(self.body) + self.got_entries.index((symbol, addend)) * 8
                return Hole(HoleKind.ABS_12, "_JIT_BASE", offset, addend)
            case {
                "Length": 2 as length,
                "Offset": int(offset),
                "PCRel": 1 as pcrel,
                "Symbol": {"Value": str(symbol)},
                "Type": {"Value": "ARM64_RELOC_PAGE21"},
            }:
                offset += base
                where = slice(offset, offset + (1 << length))
                what = int.from_bytes(self.body[where], "little", signed=False)
                # XXX: This nonsense...
                assert what & 0x9F000000 == 0x90000000, what
                addend = ((what & 0x60000000) >> 29) | ((what & 0x01FFFFE0) >> 3) << 12
                addend = sign_extend_64(addend, 33)
                symbol = symbol.removeprefix(self.SYMBOL_PREFIX)
                return Hole(HoleKind.REL_21, symbol, offset, addend)
            case {
                "Length": 2 as length,
                "Offset": int(offset),
                "PCRel": 0 as pcrel,
                "Symbol": {"Value": str(symbol)},
                "Type": {"Value": "ARM64_RELOC_PAGEOFF12"},
            }:
                offset += base
                where = slice(offset, offset + (1 << length))
                what = int.from_bytes(self.body[where], "little", signed=False)
                # XXX: This nonsense...
                assert (
                    what & 0x3B000000 == 0x39000000 or what & 0x11C00000 == 0x11000000
                ), what
                addend = (what & 0x003FFC00) >> 10
                implicit_shift = 0
                if what & 0x3B000000 == 0x39000000:
                    implicit_shift = (what >> 30) & 0x3
                    if implicit_shift == 0:
                        if what & 0x04800000 == 0x04800000:
                            implicit_shift = 4
                addend <<= implicit_shift
                symbol = symbol.removeprefix(self.SYMBOL_PREFIX)
                return Hole(HoleKind.ABS_12, symbol, offset, addend)
            case {
                "Length": 3 as length,
                "Offset": int(offset),
                "PCRel": 0 as pcrel,
                "Symbol": {"Value": str(symbol)},
                "Type": {"Value": "ARM64_RELOC_UNSIGNED"},
            }:
                offset += base
                where = slice(offset, offset + (1 << length))
                what = int.from_bytes(self.body[where], "little", signed=False)
                addend = what
                symbol = remove_prefix(symbol, self.SYMBOL_PREFIX)
                return Hole(HoleKind.ABS_64, symbol, offset, addend)
            case _:
                raise NotImplementedError(relocation)


class x86_64_apple_darwin(MachO):
    pattern = re.compile(r"x86_64-apple-darwin.*")

    def _handle_relocation(
        self,
        base: int,
        relocation: dict[str, typing.Any],
    ) -> Hole | None:
        match relocation:
            case {
                "Length": 2,
                "Offset": int(offset),
                "PCRel": 1,
                "Symbol": {"Value": str(symbol)},
                "Type": {"Value": "X86_64_RELOC_BRANCH" | "X86_64_RELOC_SIGNED"},
            }:
                offset += base
                where = slice(offset, offset + 4)
                what = int.from_bytes(self.body[where], sys.byteorder)
                addend = what
                self.body[where] = [0] * 4
                symbol = symbol.removeprefix(self.SYMBOL_PREFIX)
                return Hole(HoleKind.REL_32, symbol, offset, addend - 4)
            case {
                "Length": 2,
                "Offset": int(offset),
                "PCRel": 1,
                "Symbol": {"Value": str(symbol)},
                "Type": {"Value": "X86_64_RELOC_GOT" | "X86_64_RELOC_GOT_LOAD"},
            }:
                offset += base
                where = slice(offset, offset + 4)
                what = int.from_bytes(self.body[where], "little", signed=False)
                addend = what
                self.body[where] = [0] * 4
                symbol = remove_prefix(symbol, self.SYMBOL_PREFIX)
                if (symbol, addend) not in self.got_entries:
                    self.got_entries.append((symbol, addend))
                addend = (
                    len(self.body)
                    + self.got_entries.index((symbol, addend)) * 8
                    - offset
                    - 4
                )
                self.body[where] = addend.to_bytes(4, sys.byteorder)
                return None
            case {
                "Length": 2,
                "Offset": int(offset),
                "PCRel": 1,
                "Section": {"Value": str(section)},
                "Type": {"Value": "X86_64_RELOC_SIGNED"},
            }:
                offset += base
                where = slice(offset, offset + 4)
                what = int.from_bytes(self.body[where], sys.byteorder)
                addend = what
                self.body[where] = [0] * 4
                section = section.removeprefix(self.SYMBOL_PREFIX)
                return Hole(HoleKind.REL_32, section, offset, addend - 4)
            case {
                "Length": 3,
                "Offset": int(offset),
                "PCRel": 0,
                "Symbol": {"Value": str(symbol)},
                "Type": {"Value": "X86_64_RELOC_UNSIGNED"},
            }:
                offset += base
                where = slice(offset, offset + 8)
                what = int.from_bytes(self.body[where], sys.byteorder)
                addend = what
                self.body[where] = [0] * 8
                symbol = remove_prefix(symbol, self.SYMBOL_PREFIX)
                return Hole(HoleKind.ABS_64, symbol, offset, addend)
            case _:
                raise NotImplementedError(relocation)


class ELF(Engine):
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
                self.data_size += len(section_data["Bytes"])
                self.body.extend(section_data["Bytes"])
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
            for symbol in unwrap(section["Symbols"], "Symbol"):
                offset = before + symbol["Value"]
                name = symbol["Name"]["Value"]
                # assert name.startswith(self.SYMBOL_PREFIX)  # XXX
                name = name.removeprefix(self.SYMBOL_PREFIX)  # XXX
                assert name not in self.body_symbols
                self.body_symbols[name] = offset
        else:
            assert type in {
                "SHT_LLVM_ADDRSIG",
                "SHT_NULL",
                "SHT_STRTAB",
                "SHT_SYMTAB",
            }, type


class aarch64_unknown_linux_gnu(ELF):
    pattern = re.compile(r"aarch64-.*-linux-gnu")

    def _handle_relocation(
        self,
        base: int,
        relocation: dict[str, typing.Any],
    ) -> Hole | None:
        match relocation:
            case {
                "Addend": int(addend),
                "Offset": int(offset),
                "Symbol": {"Value": str(symbol)},
                "Type": {"Value": "R_AARCH64_ABS64"},
            }:
                offset += base
                where = slice(offset, offset + 8)
                what = int.from_bytes(self.body[where], sys.byteorder)
                symbol = symbol.removeprefix(self.SYMBOL_PREFIX)
                return Hole(HoleKind.ABS_64, symbol, offset, addend)
            case {
                "Addend": 0,
                "Offset": int(offset),
                "Symbol": {"Value": str(symbol)},
                "Type": {"Value": "R_AARCH64_ADR_GOT_PAGE"},
            }:
                offset += base
                where = slice(offset, offset + 4)
                what = int.from_bytes(self.body[where], "little", signed=False)
                # XXX: This nonsense...
                assert what & 0x9F000000 == 0x90000000, what
                addend = ((what & 0x60000000) >> 29) | ((what & 0x01FFFFE0) >> 3) << 12
                addend = sign_extend_64(addend, 33)
                if (symbol, addend) not in self.got_entries:
                    self.got_entries.append((symbol, addend))
                addend = len(self.body) + self.got_entries.index((symbol, addend)) * 8
                symbol = symbol.removeprefix(self.SYMBOL_PREFIX)
                return Hole(HoleKind.REL_21, "_JIT_BASE", offset, addend)
            case {
                "Addend": int(addend),
                "Offset": int(offset),
                "Symbol": {'Value': str(symbol)},
                "Type": {"Value": "R_AARCH64_ADD_ABS_LO12_NC"},
            }:
                offset += base
                where = slice(offset, offset + 4)
                what = int.from_bytes(self.body[where], "little", signed=False)
                # XXX: This nonsense...
                assert what & 0x3B000000 == 0x39000000 or what & 0x11C00000 == 0x11000000, what
                addend += (what & 0x003FFC00) >> 10
                implicit_shift = 0
                if what & 0x3B000000 == 0x39000000:
                    implicit_shift = (what >> 30) & 0x3
                    if implicit_shift == 0:
                        if what & 0x04800000 == 0x04800000:
                            implicit_shift = 4
                addend <<= implicit_shift
                symbol = symbol.removeprefix(self.SYMBOL_PREFIX)
                return Hole(HoleKind.ABS_12, symbol, offset, addend)
            case {
                "Addend": int(addend),
                "Offset": int(offset),
                "Symbol": {'Value': str(symbol)},
                "Type": {"Value": "R_AARCH64_ADR_PREL_PG_HI21"},
            }:
                offset += base
                where = slice(offset, offset + 4)
                what = int.from_bytes(self.body[where], "little", signed=False)
                # XXX: This nonsense...
                assert what & 0x9F000000 == 0x90000000, what
                addend += ((what & 0x60000000) >> 29) | ((what & 0x01FFFFE0) >> 3) << 12
                addend = sign_extend_64(addend, 33)
                symbol = symbol.removeprefix(self.SYMBOL_PREFIX)
                return Hole(HoleKind.REL_21, symbol, offset, addend)
            case {
                "Addend": 0,
                "Offset": int(offset),
                "Symbol": {"Value": str(symbol)},
                "Type": {"Value": "R_AARCH64_CALL26" | "R_AARCH64_JUMP26"},
            }:
                offset += base
                where = slice(offset, offset + 4)
                what = int.from_bytes(self.body[where], "little", signed=False)
                # XXX: This nonsense...
                assert (
                    what & 0xFC000000 == 0x14000000 or what & 0xFC000000 == 0x94000000
                ), what
                addend = (what & 0x03FFFFFF) << 2
                addend = sign_extend_64(addend, 28)
                symbol = symbol.removeprefix(self.SYMBOL_PREFIX)
                return Hole(HoleKind.REL_26, symbol, offset, addend)
            case {
                "Addend": 0,
                "Offset": int(offset),
                "Symbol": {"Value": str(symbol)},
                "Type": {"Value": "R_AARCH64_LD64_GOT_LO12_NC"},
            }:
                offset += base
                where = slice(offset, offset + 4)
                what = int.from_bytes(self.body[where], "little", signed=False)
                # XXX: This nonsense...
                assert what & 0x3B000000 == 0x39000000, what
                addend = (what & 0x003FFC00) >> 10
                implicit_shift = 0
                if what & 0x3B000000 == 0x39000000:
                    implicit_shift = (what >> 30) & 0x3
                    if implicit_shift == 0:
                        if what & 0x04800000 == 0x04800000:
                            implicit_shift = 4
                addend <<= implicit_shift
                if (symbol, addend) not in self.got_entries:
                    self.got_entries.append((symbol, addend))
                addend = len(self.body) + self.got_entries.index((symbol, addend)) * 8
                symbol = symbol.removeprefix(self.SYMBOL_PREFIX)
                return Hole(HoleKind.ABS_12, "_JIT_BASE", offset, addend)
            case _:
                raise NotImplementedError(relocation)


class x86_64_unknown_linux_gnu(ELF):
    pattern = re.compile(r"x86_64-.*-linux-gnu")

    def _handle_relocation(
        self,
        base: int,
        relocation: dict[str, typing.Any],
    ) -> Hole | None:
        match relocation:
            case {
                "Addend": int(addend),
                "Offset": int(offset),
                "Symbol": {"Value": str(symbol)},
                "Type": {"Value": "R_X86_64_32" | "R_X86_64_32S"},
            }:
                offset += base
                where = slice(offset, offset + 4)
                what = int.from_bytes(self.body[where], sys.byteorder)
                assert not what, what
                return Hole(HoleKind.ABS_32, symbol, offset, addend)
            case {
                "Addend": int(addend),
                "Offset": int(offset),
                "Symbol": {"Value": str(symbol)},
                "Type": {"Value": "R_X86_64_64"},
            }:
                offset += base
                where = slice(offset, offset + 8)
                what = int.from_bytes(self.body[where], sys.byteorder)
                assert not what, what
                return Hole(HoleKind.ABS_64, symbol, offset, addend)
            case {
                "Addend": int(addend),
                "Offset": int(offset),
                "Symbol": {"Value": str(symbol)},
                "Type": {"Value": "R_X86_64_GOT64"},
            }:
                offset += base
                where = slice(offset, offset + 8)
                what = int.from_bytes(self.body[where], sys.byteorder)
                assert not what, what
                if (symbol, addend) not in self.got_entries:
                    self.got_entries.append((symbol, addend))
                addend = self.got_entries.index((symbol, addend)) * 8
                self.body[where] = addend.to_bytes(8, sys.byteorder)
                return None
            case {
                "Addend": int(addend),
                "Offset": int(offset),
                "Symbol": {"Value": str(symbol)},
                "Type": {"Value": "R_X86_64_GOTOFF64"},
            }:
                offset += base
                where = slice(offset, offset + 8)
                what = int.from_bytes(self.body[where], sys.byteorder)
                assert not what, what
                addend += offset - len(self.body)
                return Hole(HoleKind.REL_64, symbol, offset, addend)
            case {
                "Addend": int(addend),
                "Offset": int(offset),
                "Symbol": {"Value": "_GLOBAL_OFFSET_TABLE_"},
                "Type": {"Value": "R_X86_64_GOTPC64"},
            }:
                offset += base
                where = slice(offset, offset + 8)
                what = int.from_bytes(self.body[where], sys.byteorder)
                assert not what, what
                addend += len(self.body) - offset
                self.body[where] = addend.to_bytes(8, sys.byteorder)
                return None
            case {
                "Addend": int(addend),
                "Offset": int(offset),
                "Symbol": {"Value": "_GLOBAL_OFFSET_TABLE_"},
                "Type": {"Value": "R_X86_64_GOTPC32"},
            }:
                offset += base
                where = slice(offset, offset + 4)
                what = int.from_bytes(self.body[where], sys.byteorder)
                assert not what, what
                addend += len(self.body) - offset
                self.body[where] = (addend % (1 << 32)).to_bytes(4, sys.byteorder)
                return None
            case {
                "Addend": int(addend),
                "Offset": int(offset),
                "Symbol": {"Value": str(symbol)},
                "Type": {"Value": "R_X86_64_REX_GOTPCRELX"},
            }:
                offset += base
                where = slice(offset, offset + 4)
                what = int.from_bytes(self.body[where], sys.byteorder)
                assert not what, what
                if (symbol, 0) not in self.got_entries:
                    self.got_entries.append((symbol, 0))
                addend += len(self.body) + self.got_entries.index((symbol, 0)) * 8 - offset
                self.body[where] = (addend % (1 << 32)).to_bytes(4, sys.byteorder)
                return None
            case {
                "Addend": int(addend),
                "Offset": int(offset),
                "Symbol": {"Value": str(symbol)},
                "Type": {"Value": "R_X86_64_PC32" | "R_X86_64_PLT32"},
            }:
                offset += base
                where = slice(offset, offset + 4)
                what = int.from_bytes(self.body[where], sys.byteorder)
                assert not what, what
                return Hole(HoleKind.REL_32, symbol, offset, addend)
            case _:
                raise NotImplementedError(relocation)


CFLAGS = [
    f"-DPy_BUILD_CORE",
    f"-D_PyJIT_ACTIVE",
    f"-I{INCLUDE}",
    f"-I{INCLUDE_INTERNAL}",
    f"-I{PYTHON}",
    f"-O3",
    f"-Wno-unreachable-code",
    f"-Wno-unused-but-set-variable",
    f"-Wno-unused-command-line-argument",
    f"-Wno-unused-label",
    f"-Wno-unused-variable",
    # Keep library calls from sneaking in:
    f"-ffreestanding",  # XXX
    # We don't need this (and it causes weird relocations):
    f"-fno-asynchronous-unwind-tables",  # XXX
    # The GHC calling convention uses %rbp as an argument-passing register:
    f"-fomit-frame-pointer",  # XXX
]


class Compiler:
    def __init__(
        self,
        *,
        verbose: bool = False,
        ghccc: bool,
        parser: type[Engine],
    ) -> None:
        self._stencils_built = {}
        self._verbose = verbose
        self._clang = find_llvm_tool("clang")
        self._readobj = find_llvm_tool("llvm-readobj")
        self._objdump = find_llvm_tool("llvm-objdump")
        self._ghccc = ghccc
        self._parser = parser

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
        defines = [f"-D_JIT_OPCODE={opname}"]
        ll = pathlib.Path(tempdir, f"{opname}.ll").resolve()
        o = pathlib.Path(tempdir, f"{opname}.o").resolve()
        await run(self._clang, *CFLAGS, "-emit-llvm", "-S", *defines, "-o", ll, c)
        self._use_ghccc(ll)
        await run(self._clang, *CFLAGS, "-c", "-o", o, ll)
        self._stencils_built[opname] = await self._parser(
            o, self._readobj, self._objdump
        ).parse()

    async def build(self) -> None:
        generated_cases = PYTHON_EXECUTOR_CASES_C_H.read_text()
        opnames = sorted(
            set(re.findall(r"\n {8}case (\w+): \{\n", generated_cases))
            - {"SET_FUNCTION_ATTRIBUTE"}
        )  # XXX: 32-bit Windows...

        with tempfile.TemporaryDirectory() as tempdir:
            await asyncio.gather(
                self._compile("trampoline", TOOLS_JIT_TRAMPOLINE, tempdir),
                *[
                    self._compile(opname, TOOLS_JIT_TEMPLATE, tempdir)
                    for opname in opnames
                ],
            )

    def dump(self) -> str:
        lines = []
        lines.append(f"// $ {sys.executable} {' '.join(sys.argv)}")  # XXX
        lines.append(f"")
        lines.extend(HoleKind.define())
        lines.append(f"")
        lines.extend(HoleValue.define())
        lines.append(f"")
        lines.append(f"typedef struct {{")
        lines.append(f"    const HoleKind kind;")
        lines.append(f"    const uintptr_t offset;")
        lines.append(f"    const uintptr_t addend;")
        lines.append(f"    const HoleValue value;")
        lines.append(f"}} Hole;")
        lines.append(f"")
        lines.append(f"typedef struct {{")
        lines.append(f"    const HoleKind kind;")
        lines.append(f"    const uintptr_t offset;")
        lines.append(f"    const uintptr_t addend;")
        lines.append(f"    const int symbol;")
        lines.append(f"}} SymbolLoad;")
        lines.append(f"")
        lines.append(f"typedef struct {{")
        lines.append(f"    const size_t nbytes;")
        lines.append(f"    const unsigned char * const bytes;")
        lines.append(f"    const size_t nholes;")
        lines.append(f"    const Hole * const holes;")
        lines.append(f"    const size_t nloads;")
        lines.append(f"    const SymbolLoad * const loads;")
        lines.append(f"}} Stencil;")
        lines.append(f"")
        opnames = []
        symbols = set()
        for stencil in self._stencils_built.values():
            for hole in stencil.holes:
                if not hole.symbol.startswith("_JIT_"):
                    symbols.add(hole.symbol)
        symbols = sorted(symbols)
        for opname, stencil in sorted(self._stencils_built.items()):
            opnames.append(opname)
            lines.append(f"// {opname}")
            assert stencil.body
            for line in stencil.disassembly:
                lines.append(f"// {line}")
            body = ",".join(f"0x{byte:02x}" for byte in stencil.body)
            lines.append(
                f"static const unsigned char {opname}_stencil_bytes[{len(stencil.body)}] = {{{body}}};"
            )
            holes = []
            loads = []
            for hole in stencil.holes:
                if hole.symbol.startswith("_JIT_"):
                    value = HoleValue(hole.symbol.removeprefix("_JIT_"))
                    holes.append(
                        f"    {{.kind = {hole.kind}, .offset = 0x{hole.offset:03x}, .addend = 0x{hole.addend % (1 << 64):03x}, .value = {value}}},"
                    )
                else:
                    loads.append(
                        f"    {{.kind = {hole.kind}, .offset = 0x{hole.offset:03x}, .addend = 0x{hole.addend % (1 << 64):03x}, .symbol = {symbols.index(hole.symbol):3}}},  // {hole.symbol}"
                    )
            lines.append(
                f"static const Hole {opname}_stencil_holes[{len(holes) + 1}] = {{"
            )
            for hole in holes:
                lines.append(hole)
            lines.append(
                f"    {{.kind =               0, .offset = 0x000, .addend = 0x000, .value = 0}},"
            )
            lines.append(f"}};")
            lines.append(
                f"static const SymbolLoad {opname}_stencil_loads[{len(loads) + 1}] = {{"
            )
            for load in loads:
                lines.append(load)
            lines.append(
                f"    {{.kind =               0, .offset = 0x000, .addend = 0x000, .symbol =   0}},"
            )
            lines.append(f"}};")
            lines.append(f"")
        lines.append(f"")
        lines.append(f"static const char *const symbols[{len(symbols)}] = {{")
        for symbol in symbols:
            lines.append(f'    "{symbol}",')
        lines.append(f"}};")
        lines.append(f"")
        lines.append(f"static uintptr_t symbol_addresses[{len(symbols)}];")
        lines.append(f"")
        lines.append(f"#define INIT_STENCIL(OP) {{                             \\")
        lines.append(f"    .nbytes = Py_ARRAY_LENGTH(OP##_stencil_bytes),     \\")
        lines.append(f"    .bytes = OP##_stencil_bytes,                       \\")
        lines.append(f"    .nholes = Py_ARRAY_LENGTH(OP##_stencil_holes) - 1, \\")
        lines.append(f"    .holes = OP##_stencil_holes,                       \\")
        lines.append(f"    .nloads = Py_ARRAY_LENGTH(OP##_stencil_loads) - 1, \\")
        lines.append(f"    .loads = OP##_stencil_loads,                       \\")
        lines.append(f"}}")
        lines.append(f"")
        lines.append(
            f"static const Stencil trampoline_stencil = INIT_STENCIL(trampoline);"
        )
        lines.append(f"")
        lines.append(f"static const Stencil stencils[512] = {{")
        assert opnames[-1] == "trampoline"
        for opname in opnames[:-1]:
            lines.append(f"    [{opname}] = INIT_STENCIL({opname}),")
        lines.append(f"}};")
        lines.append(f"")
        lines.append(f"#define INIT_HOLE(NAME) [NAME] = (uintptr_t)0xBAD0BAD0BAD0BAD0")
        lines.append(f"")
        lines.append(f"#define GET_PATCHES() {{ \\")
        for value in HoleValue:
            lines.append(f"    INIT_HOLE({value}), \\")
        lines.append(f"}}")
        lines.append(f"")
        return "\n".join(lines)


def main(host: str) -> None:
    for engine, ghccc, cflags in [
        (aarch64_apple_darwin, False, [f"-I{ROOT}"]),
        (aarch64_unknown_linux_gnu, False, [f"-I{ROOT}"]),
        (i686_pc_windows_msvc, True, [f"-I{PC}", "-fno-pic", "-mcmodel=large"]),
        (x86_64_apple_darwin, True, [f"-I{ROOT}"]),
        (x86_64_pc_windows_msvc, True, [f"-I{PC}", "-fno-pic", "-mcmodel=large"]),
        (x86_64_unknown_linux_gnu, True, [f"-I{ROOT}"]),
    ]:
        if engine.pattern.fullmatch(host):
            break
    else:
        raise NotImplementedError(host)
    CFLAGS.extend(cflags)  # XXX
    CFLAGS.append("-D_DEBUG" if sys.argv[2:] == ["-d"] else "-DNDEBUG")
    CFLAGS.append(f"--target={host}")
    hasher = hashlib.sha256(host.encode())
    hasher.update(PYTHON_EXECUTOR_CASES_C_H.read_bytes())
    for file in sorted(TOOLS_JIT.iterdir()):
        hasher.update(file.read_bytes())
    digest = hasher.hexdigest()
    if PYTHON_JIT_STENCILS_H.exists():
        with PYTHON_JIT_STENCILS_H.open() as file:
            if file.readline().removeprefix("// ").removesuffix("\n") == digest:
                return
    compiler = Compiler(verbose=True, ghccc=ghccc, parser=engine)
    asyncio.run(compiler.build())
    with PYTHON_JIT_STENCILS_H.open("w") as file:
        file.write(f"// {digest}\n")
        file.write(compiler.dump())


if __name__ == "__main__":
    main(sys.argv[1])
