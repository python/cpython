"""A template JIT for CPython 3.13, based on copy-and-patch."""

import argparse
import asyncio
import dataclasses
import enum
import functools
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

import schema

if sys.version_info < (3, 11):
    raise RuntimeError("Building the JIT compiler requires Python 3.11 or newer!")

TOOLS_JIT_BUILD = pathlib.Path(__file__).resolve()
TOOLS_JIT = TOOLS_JIT_BUILD.parent
TOOLS = TOOLS_JIT.parent
CPYTHON = TOOLS.parent
INCLUDE = CPYTHON / "Include"
INCLUDE_INTERNAL = INCLUDE / "internal"
PC = CPYTHON / "PC"
PYTHON = CPYTHON / "Python"
PYTHON_EXECUTOR_CASES_C_H = PYTHON / "executor_cases.c.h"
TOOLS_JIT_TEMPLATE_C = TOOLS_JIT / "template.c"

LLVM_VERSION = 16


@enum.unique
class HoleValue(enum.Enum):
    CONTINUE = enum.auto()
    CURRENT_EXECUTOR = enum.auto()
    DATA = enum.auto()
    OPARG = enum.auto()
    OPERAND = enum.auto()
    TARGET = enum.auto()
    TEXT = enum.auto()
    TOP = enum.auto()
    ZERO = enum.auto()


@dataclasses.dataclass
class Hole:
    offset: int
    kind: schema.HoleKind
    value: HoleValue
    symbol: str | None
    addend: int


@dataclasses.dataclass
class Stencil:
    body: bytearray = dataclasses.field(default_factory=bytearray)
    holes: list[Hole] = dataclasses.field(default_factory=list)
    disassembly: list[str] = dataclasses.field(default_factory=list)


@dataclasses.dataclass
class StencilGroup:
    text: Stencil = dataclasses.field(default_factory=Stencil)
    data: Stencil = dataclasses.field(default_factory=Stencil)


def get_llvm_tool_version(name: str, *, echo: bool = False) -> int | None:
    try:
        args = [name, "--version"]
        if echo:
            print(shlex.join(args))
        process = subprocess.run(args, check=True, stdout=subprocess.PIPE)
    except FileNotFoundError:
        return None
    match = re.search(rb"version\s+(\d+)\.\d+\.\d+\s+", process.stdout)
    return int(match.group(1)) if match else None


@functools.cache
def find_llvm_tool(tool: str, *, echo: bool = False) -> str | None:
    # Unversioned executables:
    path = tool
    if get_llvm_tool_version(path, echo=echo) == LLVM_VERSION:
        return path
    # Versioned executables:
    path = f"{tool}-{LLVM_VERSION}"
    if get_llvm_tool_version(path, echo=echo) == LLVM_VERSION:
        return path
    # My homebrew homies:
    try:
        args = ["brew", "--prefix", f"llvm@{LLVM_VERSION}"]
        if echo:
            print(shlex.join(args))
        process = subprocess.run(args, check=True, stdout=subprocess.PIPE)
    except (FileNotFoundError, subprocess.CalledProcessError):
        return None
    prefix = process.stdout.decode().removesuffix("\n")
    path = f"{prefix}/bin/{tool}"
    if get_llvm_tool_version(path, echo=echo) == LLVM_VERSION:
        return path
    return None


def require_llvm_tool(tool: str, *, echo: bool = False) -> str:
    path = find_llvm_tool(tool, echo=echo)
    if path is not None:
        return path
    raise RuntimeError(f"Can't find {tool}-{LLVM_VERSION}!")


_SEMAPHORE = asyncio.BoundedSemaphore(os.cpu_count() or 1)


async def run(
    *args: str | os.PathLike[str], capture: bool = False, echo: bool = False
) -> bytes | None:
    stdout = subprocess.PIPE if capture else None
    async with _SEMAPHORE:
        if echo:
            print(shlex.join(map(str, args)))
        process = await asyncio.create_subprocess_exec(*args, stdout=stdout)
        out, err = await process.communicate()
    assert err is None, err
    if process.returncode:
        raise RuntimeError(f"{args[0]} exited with {process.returncode}")
    return out


S = typing.TypeVar("S", schema.COFFSection, schema.ELFSection, schema.MachOSection)
R = typing.TypeVar(
    "R", schema.COFFRelocation, schema.ELFRelocation, schema.MachORelocation
)


class Parser(typing.Generic[S, R]):
    _ARGS = [
        "--elf-output-style=JSON",
        "--expand-relocs",
        # "--pretty-print",
        "--section-data",
        "--section-relocations",
        "--section-symbols",
        "--sections",
    ]

    def __init__(self, path: pathlib.Path, options: "Options") -> None:
        self.path = path
        self.stencil_group = StencilGroup()
        self.text_symbols: dict[str, int] = {}
        self.data_symbols: dict[str, int] = {}
        self.text_offsets: dict[int, int] = {}
        self.data_offsets: dict[int, int] = {}
        self.text_relocations: list[tuple[int, R]] = []
        self.data_relocations: list[tuple[int, R]] = []
        self.global_offset_table: dict[str, int] = {}
        assert options.target.parser is type(self)
        self.options = options

    async def parse(self) -> StencilGroup:
        objdump = find_llvm_tool("llvm-objdump", echo=self.options.verbose)
        if objdump is not None:
            output = await run(
                objdump, self.path, "--disassemble", "--reloc", capture=True, echo=self.options.verbose
            )
            assert output is not None
            self.stencil_group.text.disassembly = [
                line.expandtabs().strip() for line in output.decode().splitlines()
            ]
            self.stencil_group.text.disassembly = [
                line for line in self.stencil_group.text.disassembly if line
            ]
        readobj = require_llvm_tool("llvm-readobj", echo=self.options.verbose)
        output = await run(readobj, *self._ARGS, self.path, capture=True, echo=self.options.verbose)
        assert output is not None
        # --elf-output-style=JSON is only *slightly* broken on Macho...
        output = output.replace(b"PrivateExtern\n", b"\n")
        output = output.replace(b"Extern\n", b"\n")
        # ...and also COFF:
        start = output.index(b"[", 1)
        end = output.rindex(b"]", start, -1) + 1
        sections: list[dict[typing.Literal["Section"], S]] = json.loads(
            output[start:end]
        )
        for wrapped_section in sections:
            section = wrapped_section["Section"]
            self._handle_section(section)
        entry = self.text_symbols["_JIT_ENTRY"]
        assert entry == 0, entry
        padding = 0
        offset_data = 0
        self.stencil_group.data.disassembly = []
        padding_data = 0
        if self.stencil_group.data.body:
            self.stencil_group.data.disassembly.append(
                f"{offset_data:x}: {str(bytes(self.stencil_group.data.body)).removeprefix('b')}"
            )
            offset_data += len(self.stencil_group.data.body)
        while len(self.stencil_group.data.body) % 8:
            self.stencil_group.data.body.append(0)
            padding_data += 1
        if padding_data:
            self.stencil_group.data.disassembly.append(
                f"{offset_data:x}: {' '.join(padding_data * ['00'])}"
            )
            offset_data += padding_data
        global_offset_table = len(self.stencil_group.data.body)
        for base, relocation in self.text_relocations:
            newhole = self._handle_relocation(
                base, relocation, self.stencil_group.text.body
            )
            if newhole.symbol in self.data_symbols:
                addend = newhole.addend + self.data_symbols[newhole.symbol]
                newhole = Hole(
                    newhole.offset, newhole.kind, HoleValue.DATA, None, addend
                )
            elif newhole.symbol in self.text_symbols:
                addend = newhole.addend + self.text_symbols[newhole.symbol]
                newhole = Hole(
                    newhole.offset, newhole.kind, HoleValue.TEXT, None, addend
                )
            self.stencil_group.text.holes.append(newhole)
        remaining = []
        for hole in self.stencil_group.text.holes:
            if (
                hole.kind in {"R_AARCH64_CALL26", "R_AARCH64_JUMP26"}
                and hole.value is HoleValue.ZERO
            ):
                base = len(self.stencil_group.text.body)
                self.stencil_group.text.body.extend([0xD2, 0x80, 0x00, 0x08][::-1])
                self.stencil_group.text.body.extend([0xF2, 0xA0, 0x00, 0x08][::-1])
                self.stencil_group.text.body.extend([0xF2, 0xC0, 0x00, 0x08][::-1])
                self.stencil_group.text.body.extend([0xF2, 0xE0, 0x00, 0x08][::-1])
                self.stencil_group.text.body.extend([0xD6, 0x1F, 0x01, 0x00][::-1])
                self.stencil_group.text.disassembly += [
                    # XXX: Include addend:
                    f"{base:x}: d2800008      mov     x8, #0x0",
                    f"{base:016x}:  R_AARCH64_MOVW_UABS_G0_NC    {hole.symbol}",
                    f"{base + 4:x}: f2a00008      movk    x8, #0x0, lsl #16",
                    f"{base + 4:016x}:  R_AARCH64_MOVW_UABS_G1_NC    {hole.symbol}",
                    f"{base + 8:x}: f2c00008      movk    x8, #0x0, lsl #32",
                    f"{base + 8:016x}:  R_AARCH64_MOVW_UABS_G2_NC    {hole.symbol}",
                    f"{base + 12:x}: f2e00008      movk    x8, #0x0, lsl #48",
                    f"{base + 12:016x}:  R_AARCH64_MOVW_UABS_G3       {hole.symbol}",
                    f"{base + 16:x}: d61f0100      br      x8",
                ]
                remaining += [
                    dataclasses.replace(
                        hole, offset=base, kind="R_AARCH64_MOVW_UABS_G0_NC"
                    ),
                    dataclasses.replace(
                        hole, offset=base + 4, kind="R_AARCH64_MOVW_UABS_G1_NC"
                    ),
                    dataclasses.replace(
                        hole, offset=base + 8, kind="R_AARCH64_MOVW_UABS_G2_NC"
                    ),
                    dataclasses.replace(
                        hole, offset=base + 12, kind="R_AARCH64_MOVW_UABS_G3"
                    ),
                ]
                instruction = int.from_bytes(
                    self.stencil_group.text.body[hole.offset : hole.offset + 4],
                    sys.byteorder,
                )
                instruction = (instruction & 0xFC000000) | (
                    ((base - hole.offset) >> 2) & 0x03FFFFFF
                )
                self.stencil_group.text.body[
                    hole.offset : hole.offset + 4
                ] = instruction.to_bytes(4, sys.byteorder)
            else:
                remaining.append(hole)
        self.stencil_group.text.holes = remaining
        while len(self.stencil_group.text.body) % self.options.target.alignment:
            self.stencil_group.text.body.append(0)
        for base, relocation in self.data_relocations:
            newhole = self._handle_relocation(
                base, relocation, self.stencil_group.data.body
            )
            if newhole.symbol in self.data_symbols:
                addend = newhole.addend + self.data_symbols[newhole.symbol]
                newhole = Hole(
                    newhole.offset, newhole.kind, HoleValue.DATA, None, addend
                )
            elif newhole.symbol in self.text_symbols:
                addend = newhole.addend + self.text_symbols[newhole.symbol]
                newhole = Hole(
                    newhole.offset, newhole.kind, HoleValue.TEXT, None, addend
                )
            self.stencil_group.data.holes.append(newhole)
        offset = len(self.stencil_group.text.body) - padding
        if padding:
            self.stencil_group.text.disassembly.append(
                f"{offset:x}: {' '.join(padding * ['00'])}"
            )
            offset += padding
        assert offset == len(self.stencil_group.text.body), (
            offset,
            len(self.stencil_group.text.body),
        )
        for s, offset in self.global_offset_table.items():
            if s in self.text_symbols:
                addend = self.text_symbols[s]
                value, symbol = HoleValue.TEXT, None
            elif s in self.data_symbols:
                addend = self.data_symbols[s]
                value, symbol = HoleValue.DATA, None
            else:
                value, symbol = self._symbol_to_value(s)
                addend = 0
            self.stencil_group.data.holes.append(
                Hole(global_offset_table + offset, "R_X86_64_64", value, symbol, addend)
            )
            value_part = value.name if value is not HoleValue.ZERO else ""
            if value_part and not symbol and not addend:
                addend_part = ""
            else:
                addend_part = f"&{symbol} + " if symbol else ""
                addend_part += format_addend(addend)
                if value_part:
                    value_part += " + "
            self.stencil_group.data.disassembly.append(
                f"{offset_data:x}: {value_part}{addend_part}"
            )
            offset_data += 8
        self.stencil_group.data.body.extend([0] * 8 * len(self.global_offset_table))
        self.stencil_group.text.holes.sort(key=lambda hole: hole.offset)
        self.stencil_group.data.holes = [
            Hole(hole.offset, hole.kind, hole.value, hole.symbol, hole.addend)
            for hole in self.stencil_group.data.holes
        ]
        self.stencil_group.data.holes.sort(key=lambda hole: hole.offset)
        assert offset_data == len(self.stencil_group.data.body), (
            offset_data,
            len(self.stencil_group.data.body),
            self.stencil_group.data.body,
            self.stencil_group.data.disassembly,
        )
        return self.stencil_group

    def _global_offset_table_lookup(self, symbol: str | None) -> int:
        while len(self.stencil_group.data.body) % 8:
            self.stencil_group.data.body.append(0)
        if symbol is None:
            return len(self.stencil_group.data.body)
        default = 8 * len(self.global_offset_table)
        return len(self.stencil_group.data.body) + self.global_offset_table.setdefault(
            symbol, default
        )

    def _symbol_to_value(self, symbol: str) -> tuple[HoleValue, str | None]:
        try:
            if symbol.startswith("_JIT_"):
                return HoleValue[symbol.removeprefix("_JIT_")], None
        except KeyError:
            pass
        return HoleValue.ZERO, symbol

    def _handle_section(self, section: S) -> None:
        raise NotImplementedError()

    def _handle_relocation(self, base: int, relocation: R, raw: bytes) -> Hole:
        raise NotImplementedError()


class ELF(Parser[schema.ELFSection, schema.ELFRelocation]):
    def _handle_section(self, section: schema.ELFSection) -> None:
        section_type = section["Type"]["Value"]
        flags = {flag["Name"] for flag in section["Flags"]["Flags"]}
        if section_type == "SHT_RELA":
            assert "SHF_INFO_LINK" in flags, flags
            assert not section["Symbols"]
            if section["Info"] in self.text_offsets:
                base = self.text_offsets[section["Info"]]
                for wrapped_relocation in section["Relocations"]:
                    relocation = wrapped_relocation["Relocation"]
                    self.text_relocations.append((base, relocation))
            else:
                base = self.data_offsets[section["Info"]]
                for wrapped_relocation in section["Relocations"]:
                    relocation = wrapped_relocation["Relocation"]
                    self.data_relocations.append((base, relocation))
        elif section_type == "SHT_PROGBITS":
            if "SHF_ALLOC" not in flags:
                return
            if "SHF_EXECINSTR" in flags:
                self.text_offsets[section["Index"]] = len(self.stencil_group.text.body)
                for wrapped_symbol in section["Symbols"]:
                    symbol = wrapped_symbol["Symbol"]
                    offset = len(self.stencil_group.text.body) + symbol["Value"]
                    name = symbol["Name"]["Value"]
                    name = name.removeprefix(self.options.target.prefix)
                    assert name not in self.text_symbols
                    self.text_symbols[name] = offset
                section_data = section["SectionData"]
                self.stencil_group.text.body.extend(section_data["Bytes"])
            else:
                self.data_offsets[section["Index"]] = len(self.stencil_group.data.body)
                for wrapped_symbol in section["Symbols"]:
                    symbol = wrapped_symbol["Symbol"]
                    offset = len(self.stencil_group.data.body) + symbol["Value"]
                    name = symbol["Name"]["Value"]
                    name = name.removeprefix(self.options.target.prefix)
                    assert name not in self.data_symbols
                    self.data_symbols[name] = offset
                section_data = section["SectionData"]
                self.stencil_group.data.body.extend(section_data["Bytes"])
            assert not section["Relocations"]
        else:
            assert section_type in {
                "SHT_GROUP",
                "SHT_LLVM_ADDRSIG",
                "SHT_NULL",
                "SHT_STRTAB",
                "SHT_SYMTAB",
            }, section_type

    def _handle_relocation(
        self, base: int, relocation: schema.ELFRelocation, raw: bytes
    ) -> Hole:
        match relocation:
            case {
                "Type": {"Value": kind},
                "Symbol": {"Value": s},
                "Offset": offset,
                "Addend": addend,
            }:
                offset += base
                s = s.removeprefix(self.options.target.prefix)
                value, symbol = self._symbol_to_value(s)
            case _:
                raise NotImplementedError(relocation)
        return Hole(offset, kind, value, symbol, addend)


class COFF(Parser[schema.COFFSection, schema.COFFRelocation]):
    def _handle_section(self, section: schema.COFFSection) -> None:
        flags = {flag["Name"] for flag in section["Characteristics"]["Flags"]}
        if "SectionData" not in section:
            return
        section_data = section["SectionData"]
        if "IMAGE_SCN_MEM_EXECUTE" in flags:
            assert not self.stencil_group.data.body, self.stencil_group.data.body
            base = self.text_offsets[section["Number"]] = len(
                self.stencil_group.text.body
            )
            self.stencil_group.text.body.extend(section_data["Bytes"])
            for wrapped_symbol in section["Symbols"]:
                symbol = wrapped_symbol["Symbol"]
                offset = base + symbol["Value"]
                name = symbol["Name"]
                name = name.removeprefix(self.options.target.prefix)
                self.text_symbols[name] = offset
            for wrapped_relocation in section["Relocations"]:
                relocation = wrapped_relocation["Relocation"]
                self.text_relocations.append((base, relocation))
        elif "IMAGE_SCN_MEM_READ" in flags:
            base = self.data_offsets[section["Number"]] = len(
                self.stencil_group.data.body
            )
            self.stencil_group.data.body.extend(section_data["Bytes"])
            for wrapped_symbol in section["Symbols"]:
                symbol = wrapped_symbol["Symbol"]
                offset = base + symbol["Value"]
                name = symbol["Name"]
                name = name.removeprefix(self.options.target.prefix)
                self.data_symbols[name] = offset
            for wrapped_relocation in section["Relocations"]:
                relocation = wrapped_relocation["Relocation"]
                self.data_relocations.append((base, relocation))
        else:
            return

    def _handle_relocation(
        self, base: int, relocation: schema.COFFRelocation, raw: bytes
    ) -> Hole:
        match relocation:
            case {
                "Type": {"Value": "IMAGE_REL_AMD64_ADDR64" as kind},
                "Symbol": s,
                "Offset": offset,
            }:
                offset += base
                s = s.removeprefix(self.options.target.prefix)
                value, symbol = self._symbol_to_value(s)
                addend = int.from_bytes(raw[offset : offset + 8], "little")
            case {
                "Type": {"Value": "IMAGE_REL_I386_DIR32" as kind},
                "Symbol": s,
                "Offset": offset,
            }:
                offset += base
                s = s.removeprefix(self.options.target.prefix)
                value, symbol = self._symbol_to_value(s)
                addend = int.from_bytes(raw[offset : offset + 4], "little")
            case _:
                raise NotImplementedError(relocation)
        return Hole(offset, kind, value, symbol, addend)


class MachO(Parser[schema.MachOSection, schema.MachORelocation]):
    def _handle_section(self, section: schema.MachOSection) -> None:
        assert section["Address"] >= len(self.stencil_group.text.body)
        section_data = section["SectionData"]
        flags = {flag["Name"] for flag in section["Attributes"]["Flags"]}
        name = section["Name"]["Value"]
        name = name.removeprefix(self.options.target.prefix)
        if "SomeInstructions" in flags:
            assert not self.stencil_group.data.body, self.stencil_group.data.body
            self.stencil_group.text.body.extend(
                [0] * (section["Address"] - len(self.stencil_group.text.body))
            )
            before = self.text_offsets[section["Index"]] = section["Address"]
            self.stencil_group.text.body.extend(section_data["Bytes"])
            self.text_symbols[name] = before
            for wrapped_symbol in section["Symbols"]:
                symbol = wrapped_symbol["Symbol"]
                offset = symbol["Value"]
                name = symbol["Name"]["Value"]
                name = name.removeprefix(self.options.target.prefix)
                self.text_symbols[name] = offset
            for wrapped_relocation in section["Relocations"]:
                relocation = wrapped_relocation["Relocation"]
                self.text_relocations.append((before, relocation))
        else:
            self.stencil_group.data.body.extend(
                [0]
                * (
                    section["Address"]
                    - len(self.stencil_group.data.body)
                    - len(self.stencil_group.text.body)
                )
            )
            before = self.data_offsets[section["Index"]] = section["Address"] - len(
                self.stencil_group.text.body
            )
            self.stencil_group.data.body.extend(section_data["Bytes"])
            self.data_symbols[name] = len(self.stencil_group.text.body)
            for wrapped_symbol in section["Symbols"]:
                symbol = wrapped_symbol["Symbol"]
                offset = symbol["Value"] - len(self.stencil_group.text.body)
                name = symbol["Name"]["Value"]
                name = name.removeprefix(self.options.target.prefix)
                self.data_symbols[name] = offset
            for wrapped_relocation in section["Relocations"]:
                relocation = wrapped_relocation["Relocation"]
                self.data_relocations.append((before, relocation))

    def _handle_relocation(
        self, base: int, relocation: schema.MachORelocation, raw: bytes
    ) -> Hole:
        match relocation:
            case {
                "Type": {
                    "Value": "ARM64_RELOC_GOT_LOAD_PAGE21"
                    | "ARM64_RELOC_GOT_LOAD_PAGEOFF12" as kind
                },
                "Symbol": {"Value": s},
                "Offset": offset,
            }:
                offset += base
                s = s.removeprefix(self.options.target.prefix)
                value, symbol = HoleValue.DATA, None
                addend = self._global_offset_table_lookup(s)
            case {
                "Type": {"Value": kind},
                "Section": {"Value": s},
                "Offset": offset,
            } | {
                "Type": {"Value": kind},
                "Symbol": {"Value": s},
                "Offset": offset,
            }:
                offset += base
                s = s.removeprefix(self.options.target.prefix)
                value, symbol = self._symbol_to_value(s)
                addend = 0
            case _:
                raise NotImplementedError(relocation)
        # XXX
        if symbol == "__bzero":
            symbol = "bzero"
        return Hole(offset, kind, value, symbol, addend)


@dataclasses.dataclass(frozen=True)
class Target:
    triple: str
    pattern: str
    alignment: int
    prefix: str
    parser: type[MachO | COFF | ELF]

    def sha256(self) -> bytes:
        hasher = hashlib.sha256()
        hasher.update(self.triple.encode())
        hasher.update(bytes([self.alignment]))
        hasher.update(self.prefix.encode())
        return hasher.digest()


TARGETS = [
    Target(
        triple="aarch64-apple-darwin",
        pattern=r"aarch64-apple-darwin.*",
        alignment=8,
        prefix="_",
        parser=MachO,
    ),
    Target(
        triple="aarch64-unknown-linux-gnu",
        pattern=r"aarch64-.*-linux-gnu",
        alignment=8,
        prefix="",
        parser=ELF,
    ),
    Target(
        triple="i686-pc-windows-msvc",
        pattern=r"i686-pc-windows-msvc",
        alignment=1,
        prefix="_",
        parser=COFF,
    ),
    Target(
        triple="x86_64-apple-darwin",
        pattern=r"x86_64-apple-darwin.*",
        alignment=1,
        prefix="_",
        parser=MachO,
    ),
    Target(
        triple="x86_64-pc-windows-msvc",
        pattern=r"x86_64-pc-windows-msvc",
        alignment=1,
        prefix="",
        parser=COFF,
    ),
    Target(
        triple="x86_64-unknown-linux-gnu",
        pattern=r"x86_64-.*-linux-gnu",
        alignment=1,
        prefix="",
        parser=ELF,
    ),
]


def get_target(host: str) -> Target:
    for target in TARGETS:
        if re.fullmatch(target.pattern, host):
            return target
    raise ValueError(host)


CLANG_FLAGS = [
    "-DPy_BUILD_CORE",
    "-D_PyJIT_ACTIVE",
    "-D_Py_JIT",
    f"-I{INCLUDE}",
    f"-I{INCLUDE_INTERNAL}",
    f"-I{PYTHON}",
    "-O3",
    "-c",
    "-fno-asynchronous-unwind-tables",
    # XXX: SET_FUNCTION_ATTRIBUTE on 32-bit Windows debug builds:
    "-fno-jump-tables",
    # Position-independent code adds indirection to every load and jump:
    "-fno-pic",
    "-fno-stack-protector",
    # We have three options for code model:
    # - "small": the default, assumes that code and data reside in the
    #   lowest 2GB of memory (128MB on aarch64)
    # - "medium": assumes that code resides in the lowest 2GB of memory,
    #   and makes no assumptions about data (not available on aarch64)
    # - "large": makes no assumptions about either code or data
    "-mcmodel=large",
]


@dataclasses.dataclass(frozen=True)
class Options:
    target: Target
    debug: bool
    out: pathlib.Path
    verbose: bool

    def sha256(self) -> bytes:
        hasher = hashlib.sha256()
        hasher.update(self.target.sha256())
        hasher.update(bytes([self.debug]))
        hasher.update(bytes(self.out.resolve()))
        return hasher.digest()


async def _compile(
    options: Options, opname: str, c: pathlib.Path, tempdir: pathlib.Path
) -> StencilGroup:
    o = tempdir / f"{opname}.o"
    flags = [
        *CLANG_FLAGS,
        f"--target={options.target.triple}",
        "-D_DEBUG" if options.debug else "-DNDEBUG",  # XXX
        f"-D_JIT_OPCODE={opname}",
        f"-I.",
    ]
    clang = require_llvm_tool("clang", echo=options.verbose)
    await run(clang, *flags, "-o", o, c, echo=options.verbose)
    return await options.target.parser(o, options).parse()


async def build(options: Options) -> dict[str, StencilGroup]:
    generated_cases = PYTHON_EXECUTOR_CASES_C_H.read_text()
    opnames = sorted(re.findall(r"\n {8}case (\w+): \{\n", generated_cases))
    tasks = []
    with tempfile.TemporaryDirectory() as tempdir:
        work = pathlib.Path(tempdir).resolve()
        async with asyncio.TaskGroup() as group:
            for opname in opnames:
                coro = _compile(options, opname, TOOLS_JIT_TEMPLATE_C, work)
                tasks.append(group.create_task(coro, name=opname))
    return {task.get_name(): task.result() for task in tasks}


def dump(stencil_groups: dict[str, StencilGroup]) -> typing.Generator[str, None, None]:
    yield f"// $ {shlex.join([sys.executable, *sys.argv])}"
    yield ""
    yield "typedef enum {"
    for kind in sorted(typing.get_args(schema.HoleKind)):
        yield f"    HoleKind_{kind},"
    yield "} HoleKind;"
    yield ""
    yield "typedef enum {"
    for value in HoleValue:
        yield f"    HoleValue_{value.name},"
    yield "} HoleValue;"
    yield ""
    yield "typedef struct {"
    yield "    const uint64_t offset;"
    yield "    const HoleKind kind;"
    yield "    const HoleValue value;"
    yield "    const void *symbol;"
    yield "    const uint64_t addend;"
    yield "} Hole;"
    yield ""
    yield "typedef struct {"
    yield "    const size_t body_size;"
    yield "    const unsigned char * const body;"
    yield "    const size_t holes_size;"
    yield "    const Hole * const holes;"
    yield "} Stencil;"
    yield ""
    yield "typedef struct {"
    yield "    const Stencil text;"
    yield "    const Stencil data;"
    yield "} StencilGroup;"
    yield ""
    opnames = []
    for opname, stencil in sorted(stencil_groups.items()):
        opnames.append(opname)
        yield f"// {opname}"
        assert stencil.text
        for line in stencil.text.disassembly:
            yield f"// {line}"
        body = ", ".join(f"0x{byte:02x}" for byte in stencil.text.body)
        size = len(stencil.text.body) + 1
        yield f"static const unsigned char {opname}_text_body[{size}] = {{{body}}};"
        if stencil.text.holes:
            size = len(stencil.text.holes) + 1
            yield f"static const Hole {opname}_text_holes[{size}] = {{"
            for hole in sorted(stencil.text.holes, key=lambda hole: hole.offset):
                parts = [
                    hex(hole.offset),
                    f"HoleKind_{hole.kind}",
                    f"HoleValue_{hole.value.name}",
                    f"&{hole.symbol}" if hole.symbol else "NULL",
                    format_addend(hole.addend),
                ]
                yield f"    {{{', '.join(parts)}}},"
            yield "};"
        else:
            yield f"static const Hole {opname}_text_holes[1];"
        for line in stencil.data.disassembly:
            yield f"// {line}"
        body = ", ".join(f"0x{byte:02x}" for byte in stencil.data.body)
        if stencil.data.body:
            size = len(stencil.data.body) + 1
            yield f"static const unsigned char {opname}_data_body[{size}] = {{{body}}};"
        else:
            yield f"static const unsigned char {opname}_data_body[1];"
        if stencil.data.holes:
            size = len(stencil.data.holes) + 1
            yield f"static const Hole {opname}_data_holes[{size}] = {{"
            for hole in sorted(stencil.data.holes, key=lambda hole: hole.offset):
                parts = [
                    hex(hole.offset),
                    f"HoleKind_{hole.kind}",
                    f"HoleValue_{hole.value.name}",
                    f"&{hole.symbol}" if hole.symbol else "NULL",
                    format_addend(hole.addend),
                ]
                yield f"    {{{', '.join(parts)}}},"
            yield "};"
        else:
            yield f"static const Hole {opname}_data_holes[1];"
        yield ""
    yield "#define INIT_STENCIL(STENCIL) {                         \\"
    yield "    .body_size = Py_ARRAY_LENGTH(STENCIL##_body) - 1,   \\"
    yield "    .body = STENCIL##_body,                             \\"
    yield "    .holes_size = Py_ARRAY_LENGTH(STENCIL##_holes) - 1, \\"
    yield "    .holes = STENCIL##_holes,                           \\"
    yield "}"
    yield ""
    yield "#define INIT_STENCIL_GROUP(OP) {     \\"
    yield "    .text = INIT_STENCIL(OP##_text), \\"
    yield "    .data = INIT_STENCIL(OP##_data), \\"
    yield "}"
    yield ""
    yield "static const StencilGroup stencil_groups[512] = {"
    for opname in opnames:
        yield f"    [{opname}] = INIT_STENCIL_GROUP({opname}),"
    yield "};"
    yield ""
    yield "#define GET_PATCHES() { \\"
    for value in HoleValue:
        yield f"    [HoleValue_{value.name}] = (uint64_t)0xBADBADBADBADBADB, \\"
    yield "}"


def format_addend(addend: int) -> str:
    addend %= 1 << 64
    if addend & (1 << 63):
        addend -= 1 << 64
    return hex(addend)


def main(target: Target, *, debug: bool, out: pathlib.Path, verbose: bool) -> None:
    jit_stencils = out / "jit_stencils.h"
    options = Options(target, debug, out, verbose)
    hasher = hashlib.sha256()
    hasher.update(options.sha256())
    hasher.update(PYTHON_EXECUTOR_CASES_C_H.read_bytes())
    hasher.update((out / "pyconfig.h").read_bytes())
    for dirpath, _, filenames in sorted(os.walk(TOOLS_JIT)):
        for filename in filenames:
            hasher.update(pathlib.Path(dirpath, filename).read_bytes())
    digest = hasher.hexdigest()
    if jit_stencils.exists():
        with jit_stencils.open() as file:
            if file.readline().removeprefix("// ").removesuffix("\n") == digest:
                return
    stencil_groups = asyncio.run(build(options))
    with jit_stencils.open("w") as file:
        file.write(f"// {digest}\n")
        for line in dump(stencil_groups):
            file.write(f"{line}\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("target", type=get_target)
    parser.add_argument(
        "-d", "--debug", action="store_true", help="compile for a debug build of Python"
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="echo commands as they are run"
    )
    parsed = parser.parse_args()
    out = pathlib.Path.cwd()
    main(parsed.target, debug=parsed.debug, out=out, verbose=parsed.verbose)
