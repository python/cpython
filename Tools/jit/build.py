"""A template JIT for CPython 3.13, based on copy-and-patch."""

import argparse
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

import llvm
import schema

if sys.version_info < (3, 11):
    raise RuntimeError("Building the JIT compiler requires Python 3.11 or newer!")

TOOLS_JIT_BUILD = pathlib.Path(__file__).resolve()
TOOLS_JIT = TOOLS_JIT_BUILD.parent
TOOLS = TOOLS_JIT.parent
CPYTHON = TOOLS.parent
INCLUDE = CPYTHON / "Include"
INCLUDE_INTERNAL = INCLUDE / "internal"
PYTHON = CPYTHON / "Python"
PYTHON_EXECUTOR_CASES_C_H = PYTHON / "executor_cases.c.h"
TOOLS_JIT_TEMPLATE_C = TOOLS_JIT / "template.c"


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
    replace = dataclasses.replace


@dataclasses.dataclass
class Stencil:
    body: bytearray = dataclasses.field(default_factory=bytearray)
    holes: list[Hole] = dataclasses.field(default_factory=list)
    disassembly: list[str] = dataclasses.field(default_factory=list)
    symbols: dict[str, int] = dataclasses.field(default_factory=dict, init=False)
    offsets: dict[int, int] = dataclasses.field(default_factory=dict, init=False)


@dataclasses.dataclass
class StencilGroup:
    text: Stencil = dataclasses.field(default_factory=Stencil)
    data: Stencil = dataclasses.field(default_factory=Stencil)


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
    def __init__(self, options: "Options") -> None:
        self.group = StencilGroup()
        self.relocations_text: list[tuple[int, R]] = []
        self.relocations_data: list[tuple[int, R]] = []
        self.global_offset_table: dict[str, int] = {}
        assert options.target.parser is type(self)
        self.options = options

    async def parse(self, path: pathlib.Path) -> StencilGroup:
        objdump = llvm.find_tool("llvm-objdump", echo=self.options.verbose)
        if objdump is not None:
            flags = ["--disassemble", "--reloc"]
            output = await run(
                objdump, *flags, path, capture=True, echo=self.options.verbose
            )
            assert output is not None
            self.group.text.disassembly.extend(
                line.expandtabs().strip()
                for line in output.decode().splitlines()
                if not line.isspace()
            )
        readobj = llvm.require_tool("llvm-readobj", echo=self.options.verbose)
        flags = [
            "--elf-output-style=JSON",
            "--expand-relocs",
            # "--pretty-print",
            "--section-data",
            "--section-relocations",
            "--section-symbols",
            "--sections",
        ]
        output = await run(
            readobj, *flags, path, capture=True, echo=self.options.verbose
        )
        assert output is not None
        # --elf-output-style=JSON is only *slightly* broken on Mach-O...
        output = output.replace(b"PrivateExtern\n", b"\n")
        output = output.replace(b"Extern\n", b"\n")
        # ...and also COFF:
        start = output.index(b"[", 1)
        end = output.rindex(b"]", start, -1) + 1
        sections: list[dict[typing.Literal["Section"], S]] = json.loads(
            output[start:end]
        )
        for wrapped_section in sections:
            self._handle_section(wrapped_section["Section"])
        assert self.group.text.symbols["_JIT_ENTRY"] == 0
        if self.group.data.body:
            self.group.data.disassembly.append(
                f"0: {str(bytes(self.group.data.body)).removeprefix('b')}"
            )
        self._pad(self.group.data, 8)
        self._process_relocations(self.relocations_text, self.group.text)
        remaining: list[Hole] = []
        for hole in self.group.text.holes:
            if (
                hole.kind in {"R_AARCH64_CALL26", "R_AARCH64_JUMP26"}
                and hole.value is HoleValue.ZERO
            ):
                remaining.extend(self._emit_aarch64_trampoline(self.group.text, hole))
            else:
                remaining.append(hole)
        self.group.text.holes[:] = remaining
        self._pad(self.group.text, self.options.target.alignment)
        self._process_relocations(self.relocations_data, self.group.data)
        self._emit_global_offset_table()
        self.group.text.holes.sort(key=lambda hole: hole.offset)
        self.group.data.holes.sort(key=lambda hole: hole.offset)
        return self.group

    def _emit_global_offset_table(self) -> None:
        global_offset_table = len(self.group.data.body)
        for s, offset in self.global_offset_table.items():
            if s in self.group.text.symbols:
                value, symbol = HoleValue.TEXT, None
                addend = self.group.text.symbols[s]
            elif s in self.group.data.symbols:
                value, symbol = HoleValue.DATA, None
                addend = self.group.data.symbols[s]
            else:
                value, symbol = self._symbol_to_value(s)
                addend = 0
            self.group.data.holes.append(
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
            self.group.data.disassembly.append(
                f"{len(self.group.data.body):x}: {value_part}{addend_part}"
            )
            self.group.data.body.extend([0] * 8)

    @staticmethod
    def _emit_aarch64_trampoline(
        stencil: Stencil, hole: Hole
    ) -> typing.Generator[Hole, None, None]:
        base = len(stencil.body)
        instruction = int.from_bytes(
            stencil.body[hole.offset : hole.offset + 4], sys.byteorder
        )
        instruction = (instruction & 0xFC000000) | (
            ((base - hole.offset) >> 2) & 0x03FFFFFF
        )
        stencil.body[hole.offset : hole.offset + 4] = instruction.to_bytes(
            4, sys.byteorder
        )
        stencil.disassembly += [
            f"{base + 4 * 0: x}: d2800008      mov     x8, #0x0",
            f"{base + 4 * 0:016x}:  R_AARCH64_MOVW_UABS_G0_NC    {hole.symbol}",
            f"{base + 4 * 1:x}: f2a00008      movk    x8, #0x0, lsl #16",
            f"{base + 4 * 1:016x}:  R_AARCH64_MOVW_UABS_G1_NC    {hole.symbol}",
            f"{base + 4 * 2:x}: f2c00008      movk    x8, #0x0, lsl #32",
            f"{base + 4 * 2:016x}:  R_AARCH64_MOVW_UABS_G2_NC    {hole.symbol}",
            f"{base + 4 * 3:x}: f2e00008      movk    x8, #0x0, lsl #48",
            f"{base + 4 * 3:016x}:  R_AARCH64_MOVW_UABS_G3       {hole.symbol}",
            f"{base + 4 * 4:x}: d61f0100      br      x8",
        ]
        stencil.body.extend(0xD2800008.to_bytes(4, sys.byteorder))
        stencil.body.extend(0xF2A00008.to_bytes(4, sys.byteorder))
        stencil.body.extend(0xF2C00008.to_bytes(4, sys.byteorder))
        stencil.body.extend(0xF2E00008.to_bytes(4, sys.byteorder))
        stencil.body.extend(0xD61F0100.to_bytes(4, sys.byteorder))
        yield hole.replace(offset=base + 4 * 0, kind="R_AARCH64_MOVW_UABS_G0_NC")
        yield hole.replace(offset=base + 4 * 1, kind="R_AARCH64_MOVW_UABS_G1_NC")
        yield hole.replace(offset=base + 4 * 2, kind="R_AARCH64_MOVW_UABS_G2_NC")
        yield hole.replace(offset=base + 4 * 3, kind="R_AARCH64_MOVW_UABS_G3")

    def _process_relocations(
        self, relocations: list[tuple[int, R]], stencil: Stencil
    ) -> None:
        for base, relocation in relocations:
            hole = self._handle_relocation(base, relocation, stencil.body)
            if hole.symbol in self.group.data.symbols:
                value, symbol = HoleValue.DATA, None
                addend = hole.addend + self.group.data.symbols[hole.symbol]
                hole = hole.replace(value=value, symbol=symbol, addend=addend)
            elif hole.symbol in self.group.text.symbols:
                value, symbol = HoleValue.TEXT, None
                addend = hole.addend + self.group.text.symbols[hole.symbol]
                hole = hole.replace(value=value, symbol=symbol, addend=addend)
            stencil.holes.append(hole)

    @staticmethod
    def _pad(stencil: Stencil, alignment: int) -> None:
        offset = len(stencil.body)
        padding = -offset % alignment
        stencil.disassembly.append(f"{offset:x}: {' '.join(['00'] * padding)}")
        stencil.body.extend([0] * padding)

    def _global_offset_table_lookup(self, symbol: str | None) -> int:
        self._pad(self.group.data, 8)
        if symbol is None:
            return len(self.group.data.body)
        default = 8 * len(self.global_offset_table)
        return len(self.group.data.body) + self.global_offset_table.setdefault(
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
            if section["Info"] in self.group.text.offsets:
                base = self.group.text.offsets[section["Info"]]
                for wrapped_relocation in section["Relocations"]:
                    relocation = wrapped_relocation["Relocation"]
                    self.relocations_text.append((base, relocation))
            else:
                base = self.group.data.offsets[section["Info"]]
                for wrapped_relocation in section["Relocations"]:
                    relocation = wrapped_relocation["Relocation"]
                    self.relocations_data.append((base, relocation))
        elif section_type == "SHT_PROGBITS":
            if "SHF_ALLOC" not in flags:
                return
            if "SHF_EXECINSTR" in flags:
                self._handle_section_data(section, self.group.text)
            else:
                self._handle_section_data(section, self.group.data)
            assert not section["Relocations"]
        else:
            assert section_type in {
                "SHT_GROUP",
                "SHT_LLVM_ADDRSIG",
                "SHT_NULL",
                "SHT_STRTAB",
                "SHT_SYMTAB",
            }, section_type

    def _handle_section_data(
        self, section: schema.ELFSection, stencil: Stencil
    ) -> None:
        stencil.offsets[section["Index"]] = len(stencil.body)
        for wrapped_symbol in section["Symbols"]:
            symbol = wrapped_symbol["Symbol"]
            offset = len(stencil.body) + symbol["Value"]
            name = symbol["Name"]["Value"]
            name = name.removeprefix(self.options.target.prefix)
            assert name not in stencil.symbols
            stencil.symbols[name] = offset
        section_data = section["SectionData"]
        stencil.body.extend(section_data["Bytes"])

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
            assert not self.group.data.body, self.group.data.body
            base = self.group.text.offsets[section["Number"]] = len(
                self.group.text.body
            )
            self.group.text.body.extend(section_data["Bytes"])
            for wrapped_symbol in section["Symbols"]:
                symbol = wrapped_symbol["Symbol"]
                offset = base + symbol["Value"]
                name = symbol["Name"]
                name = name.removeprefix(self.options.target.prefix)
                self.group.text.symbols[name] = offset
            for wrapped_relocation in section["Relocations"]:
                relocation = wrapped_relocation["Relocation"]
                self.relocations_text.append((base, relocation))
        elif "IMAGE_SCN_MEM_READ" in flags:
            base = self.group.data.offsets[section["Number"]] = len(
                self.group.data.body
            )
            self.group.data.body.extend(section_data["Bytes"])
            for wrapped_symbol in section["Symbols"]:
                symbol = wrapped_symbol["Symbol"]
                offset = base + symbol["Value"]
                name = symbol["Name"]
                name = name.removeprefix(self.options.target.prefix)
                self.group.data.symbols[name] = offset
            for wrapped_relocation in section["Relocations"]:
                relocation = wrapped_relocation["Relocation"]
                self.relocations_data.append((base, relocation))
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
        assert section["Address"] >= len(self.group.text.body)
        assert "SectionData" in section
        section_data = section["SectionData"]
        flags = {flag["Name"] for flag in section["Attributes"]["Flags"]}
        name = section["Name"]["Value"]
        name = name.removeprefix(self.options.target.prefix)
        if "SomeInstructions" in flags:
            assert not self.group.data.body, self.group.data.body
            self.group.text.body.extend(
                [0] * (section["Address"] - len(self.group.text.body))
            )
            before = self.group.text.offsets[section["Index"]] = section["Address"]
            self.group.text.body.extend(section_data["Bytes"])
            self.group.text.symbols[name] = before
            assert "Symbols" in section
            for wrapped_symbol in section["Symbols"]:
                symbol = wrapped_symbol["Symbol"]
                offset = symbol["Value"]
                name = symbol["Name"]["Value"]
                name = name.removeprefix(self.options.target.prefix)
                self.group.text.symbols[name] = offset
            assert "Relocations" in section
            for wrapped_relocation in section["Relocations"]:
                relocation = wrapped_relocation["Relocation"]
                self.relocations_text.append((before, relocation))
        else:
            self.group.data.body.extend(
                [0]
                * (
                    section["Address"]
                    - len(self.group.data.body)
                    - len(self.group.text.body)
                )
            )
            before = self.group.data.offsets[section["Index"]] = section[
                "Address"
            ] - len(self.group.text.body)
            self.group.data.body.extend(section_data["Bytes"])
            self.group.data.symbols[name] = len(self.group.text.body)
            assert "Symbols" in section
            for wrapped_symbol in section["Symbols"]:
                symbol = wrapped_symbol["Symbol"]
                offset = symbol["Value"] - len(self.group.text.body)
                name = symbol["Name"]["Value"]
                name = name.removeprefix(self.options.target.prefix)
                self.group.data.symbols[name] = offset
            assert "Relocations" in section
            for wrapped_relocation in section["Relocations"]:
                relocation = wrapped_relocation["Relocation"]
                self.relocations_data.append((before, relocation))

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
        # Turn Clang's weird __bzero calls into normal bzero calls:
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
        f"--target={options.target.triple}",
        "-DPy_BUILD_CORE",
        "-D_DEBUG" if options.debug else "-DNDEBUG",
        f"-D_JIT_OPCODE={opname}",
        "-D_PyJIT_ACTIVE",
        "-D_Py_JIT",
        "-I.",
        f"-I{INCLUDE}",
        f"-I{INCLUDE_INTERNAL}",
        f"-I{PYTHON}",
        "-O3",
        "-c",
        "-fno-asynchronous-unwind-tables",
        # SET_FUNCTION_ATTRIBUTE on 32-bit Windows debug builds:
        "-fno-jump-tables",
        # Position-independent code adds indirection to every load and jump:
        "-fno-pic",
        # Don't make calls to weird stack-smashing canaries:
        "-fno-stack-protector",
        # We have three options for code model:
        # - "small": the default, assumes that code and data reside in the lowest
        #   2GB of memory (128MB on aarch64)
        # - "medium": assumes that code resides in the lowest 2GB of memory, and
        #   makes no assumptions about data (not available on aarch64)
        # - "large": makes no assumptions about either code or data
        "-mcmodel=large",
    ]
    clang = llvm.require_tool("clang", echo=options.verbose)
    await run(clang, *flags, "-o", o, c, echo=options.verbose)
    return await options.target.parser(options).parse(o)


async def build_stencils(options: Options) -> dict[str, StencilGroup]:
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


def dump_header() -> typing.Generator[str, None, None]:
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


def dump_footer(opnames: list[str]) -> typing.Generator[str, None, None]:
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


def dump(stencil_groups: dict[str, StencilGroup]) -> typing.Generator[str, None, None]:
    yield from dump_header()
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
    yield from dump_footer(opnames)


def format_addend(addend: int) -> str:
    addend %= 1 << 64
    if addend & (1 << 63):
        addend -= 1 << 64
    return hex(addend)


def build(target: Target, *, debug: bool, out: pathlib.Path, verbose: bool) -> None:
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
    stencil_groups = asyncio.run(build_stencils(options))
    with jit_stencils.open("w") as file:
        file.write(f"// {digest}\n")
        for line in dump(stencil_groups):
            file.write(f"{line}\n")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "target", type=get_target, help="a PEP 11 target triple to compile for"
    )
    parser.add_argument(
        "-d", "--debug", action="store_true", help="compile for a debug build of Python"
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="echo commands as they are run"
    )
    build(out=pathlib.Path.cwd(), **vars(parser.parse_args()))


if __name__ == "__main__":
    main()
