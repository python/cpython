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

import schema

if sys.version_info < (3, 11):
    raise RuntimeError("Building the JIT compiler requires Python 3.11 or newer!")

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
TOOLS_JIT_TEMPLATE_C = TOOLS_JIT / "template.c"

STUBS = ["trampoline", "wrapper"]

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
    body: bytearray
    holes: list[Hole]
    disassembly: list[str]


@dataclasses.dataclass
class StencilGroup:
    text: Stencil
    data: Stencil


def get_llvm_tool_version(name: str) -> int | None:
    try:
        args = [name, "--version"]
        process = subprocess.run(args, check=True, stdout=subprocess.PIPE)
    except FileNotFoundError:
        return None
    match = re.search(rb"version\s+(\d+)\.\d+\.\d+\s+", process.stdout)
    return int(match.group(1)) if match else None


def find_llvm_tool(tool: str) -> str | None:
    # Unversioned executables:
    path = tool
    if get_llvm_tool_version(path) == LLVM_VERSION:
        return path
    # Versioned executables:
    path = f"{tool}-{LLVM_VERSION}"
    if get_llvm_tool_version(path) == LLVM_VERSION:
        return path
    # My homebrew homies:
    try:
        args = ["brew", "--prefix", f"llvm@{LLVM_VERSION}"]
        process = subprocess.run(args, check=True, stdout=subprocess.PIPE)
    except (FileNotFoundError, subprocess.CalledProcessError):
        return None
    prefix = process.stdout.decode().removesuffix("\n")
    path = f"{prefix}/bin/{tool}"
    if get_llvm_tool_version(path) == LLVM_VERSION:
        return path
    return None


def require_llvm_tool(tool: str) -> str:
    path = find_llvm_tool(tool)
    if path is not None:
        return path
    raise RuntimeError(f"Can't find {tool}-{LLVM_VERSION}!")


_SEMAPHORE = asyncio.BoundedSemaphore(os.cpu_count() or 1)


async def run(*args: str | os.PathLike[str], capture: bool = False) -> bytes | None:
    async with _SEMAPHORE:
        # print(shlex.join(map(str, args)))
        process = await asyncio.create_subprocess_exec(
            *args, cwd=ROOT, stdout=subprocess.PIPE if capture else None
        )
        stdout, stderr = await process.communicate()
    assert stderr is None, stderr
    if process.returncode:
        raise RuntimeError(f"{args[0]} exited with {process.returncode}")
    return stdout


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

    def __init__(
        self, path: pathlib.Path, readobj: str, objdump: str | None, target: "Target"
    ) -> None:
        self.path = path
        self.text = bytearray()
        self.data = bytearray()
        self.text_symbols: dict[str, int] = {}
        self.data_symbols: dict[str, int] = {}
        self.text_offsets: dict[int, int] = {}
        self.data_offsets: dict[int, int] = {}
        self.text_relocations: list[tuple[int, R]] = []
        self.data_relocations: list[tuple[int, R]] = []
        self.global_offset_table: dict[str, int] = {}
        self.readobj = readobj
        self.objdump = objdump
        self.target = target

    async def parse(self) -> StencilGroup:
        if self.objdump is not None:
            output = await run(
                self.objdump, self.path, "--disassemble", "--reloc", capture=True
            )
            assert output is not None
            disassembly = [
                line.expandtabs().strip() for line in output.decode().splitlines()
            ]
            disassembly = [line for line in disassembly if line]
        else:
            disassembly = []
        output = await run(self.readobj, *self._ARGS, self.path, capture=True)
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
        holes = []
        holes_data = []
        padding = 0
        while len(self.text) % self.target.alignment:
            self.text.append(0)
            padding += 1
        offset_data = 0
        disassembly_data = []
        padding_data = 0
        if self.data:
            disassembly_data.append(
                f"{offset_data:x}: {str(bytes(self.data)).removeprefix('b')}"
            )
            offset_data += len(self.data)
        while len(self.data) % 8:
            self.data.append(0)
            padding_data += 1
        if padding_data:
            disassembly_data.append(
                f"{offset_data:x}: {' '.join(padding_data * ['00'])}"
            )
            offset_data += padding_data
        global_offset_table = len(self.data)
        for base, relocation in self.text_relocations:
            newhole = self._handle_relocation(base, relocation, self.text)
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
            holes.append(newhole)
        for base, relocation in self.data_relocations:
            newhole = self._handle_relocation(base, relocation, self.data)
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
            holes_data.append(newhole)
        offset = len(self.text) - padding
        if padding:
            disassembly.append(f"{offset:x}: {' '.join(padding * ['00'])}")
            offset += padding
        assert offset == len(self.text), (offset, len(self.text))
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
            holes_data.append(
                Hole(global_offset_table + offset, "R_X86_64_64", value, symbol, addend)
            )
            value_part = value.name if value is not HoleValue.ZERO else ""
            if value_part and not symbol and not addend:
                addend_part = ""
            else:
                addend_part = format_addend(symbol, addend)
                if value_part:
                    value_part += "+"
            disassembly_data.append(f"{offset_data:x}: {value_part}{addend_part}")
            offset_data += 8
        self.data.extend([0] * 8 * len(self.global_offset_table))
        holes.sort(key=lambda hole: hole.offset)
        holes_data = [
            Hole(hole.offset, hole.kind, hole.value, hole.symbol, hole.addend)
            for hole in holes_data
        ]
        holes_data.sort(key=lambda hole: hole.offset)
        assert offset_data == len(self.data), (
            offset_data,
            len(self.data),
            self.data,
            disassembly_data,
        )
        text = Stencil(self.text, holes, disassembly)
        data = Stencil(self.data, holes_data, disassembly_data)
        return StencilGroup(text, data)

    def _global_offset_table_lookup(self, symbol: str | None) -> int:
        while len(self.data) % 8:
            self.data.append(0)
        if symbol is None:
            return len(self.data)
        return len(self.data) + self.global_offset_table.setdefault(
            symbol, 8 * len(self.global_offset_table)
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
                self.text_offsets[section["Index"]] = len(self.text)
                for wrapped_symbol in section["Symbols"]:
                    symbol = wrapped_symbol["Symbol"]
                    offset = len(self.text) + symbol["Value"]
                    name = symbol["Name"]["Value"]
                    name = name.removeprefix(self.target.prefix)
                    assert name not in self.text_symbols
                    self.text_symbols[name] = offset
                section_data = section["SectionData"]
                self.text.extend(section_data["Bytes"])
            else:
                self.data_offsets[section["Index"]] = len(self.data)
                for wrapped_symbol in section["Symbols"]:
                    symbol = wrapped_symbol["Symbol"]
                    offset = len(self.data) + symbol["Value"]
                    name = symbol["Name"]["Value"]
                    name = name.removeprefix(self.target.prefix)
                    assert name not in self.data_symbols
                    self.data_symbols[name] = offset
                section_data = section["SectionData"]
                self.data.extend(section_data["Bytes"])
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
                s = s.removeprefix(self.target.prefix)
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
            assert not self.data, self.data
            base = self.text_offsets[section["Number"]] = len(self.text)
            self.text.extend(section_data["Bytes"])
            for wrapped_symbol in section["Symbols"]:
                symbol = wrapped_symbol["Symbol"]
                offset = base + symbol["Value"]
                name = symbol["Name"]
                name = name.removeprefix(self.target.prefix)
                self.text_symbols[name] = offset
            for wrapped_relocation in section["Relocations"]:
                relocation = wrapped_relocation["Relocation"]
                self.text_relocations.append((base, relocation))
        elif "IMAGE_SCN_MEM_READ" in flags:
            base = self.data_offsets[section["Number"]] = len(self.data)
            self.data.extend(section_data["Bytes"])
            for wrapped_symbol in section["Symbols"]:
                symbol = wrapped_symbol["Symbol"]
                offset = base + symbol["Value"]
                name = symbol["Name"]
                name = name.removeprefix(self.target.prefix)
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
                s = s.removeprefix(self.target.prefix)
                value, symbol = self._symbol_to_value(s)
                addend = int.from_bytes(raw[offset : offset + 8], "little")
            case {
                "Type": {
                    "Value": "IMAGE_REL_AMD64_REL32" | "IMAGE_REL_I386_REL32" as kind
                },
                "Symbol": s,
                "Offset": offset,
            }:
                offset += base
                s = s.removeprefix(self.target.prefix)
                value, symbol = self._symbol_to_value(s)
                addend = int.from_bytes(raw[offset : offset + 4], "little") - 4
            case {
                "Type": {"Value": "IMAGE_REL_I386_DIR32" as kind},
                "Symbol": s,
                "Offset": offset,
            }:
                offset += base
                s = s.removeprefix(self.target.prefix)
                value, symbol = self._symbol_to_value(s)
                addend = int.from_bytes(raw[offset : offset + 4], "little")
            case _:
                raise NotImplementedError(relocation)
        return Hole(offset, kind, value, symbol, addend)


class MachO(Parser[schema.MachOSection, schema.MachORelocation]):
    def _handle_section(self, section: schema.MachOSection) -> None:
        assert section["Address"] >= len(self.text)
        section_data = section["SectionData"]
        flags = {flag["Name"] for flag in section["Attributes"]["Flags"]}
        name = section["Name"]["Value"]
        name = name.removeprefix(self.target.prefix)
        if name == "_eh_frame":
            return
        if "SomeInstructions" in flags:
            assert not self.data, self.data
            self.text.extend([0] * (section["Address"] - len(self.text)))
            before = self.text_offsets[section["Index"]] = section["Address"]
            self.text.extend(section_data["Bytes"])
            self.text_symbols[name] = before
            for wrapped_symbol in section["Symbols"]:
                symbol = wrapped_symbol["Symbol"]
                offset = symbol["Value"]
                name = symbol["Name"]["Value"]
                name = name.removeprefix(self.target.prefix)
                self.text_symbols[name] = offset
            for wrapped_relocation in section["Relocations"]:
                relocation = wrapped_relocation["Relocation"]
                self.text_relocations.append((before, relocation))
        else:
            self.data.extend(
                [0] * (section["Address"] - len(self.data) - len(self.text))
            )
            before = self.data_offsets[section["Index"]] = section["Address"] - len(
                self.text
            )
            self.data.extend(section_data["Bytes"])
            self.data_symbols[name] = len(self.text)
            for wrapped_symbol in section["Symbols"]:
                symbol = wrapped_symbol["Symbol"]
                offset = symbol["Value"] - len(self.text)
                name = symbol["Name"]["Value"]
                name = name.removeprefix(self.target.prefix)
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
                s = s.removeprefix(self.target.prefix)
                value, symbol = HoleValue.DATA, None
                addend = self._global_offset_table_lookup(s)
            case {
                "Type": {"Value": kind},
                "Section": {"Value": s},
                "Offset": offset,
            }:
                offset += base
                s = s.removeprefix(self.target.prefix)
                value, symbol = self._symbol_to_value(s)
                addend = 0
            case {
                "Type": {"Value": "X86_64_RELOC_BRANCH" as kind},
                "Symbol": {"Value": s},
                "Offset": offset,
            }:
                offset += base
                s = s.removeprefix(self.target.prefix)
                value, symbol = self._symbol_to_value(s)
                addend = (
                    int.from_bytes(self.text[offset : offset + 4], sys.byteorder) - 4
                )
            case {
                "Type": {"Value": "X86_64_RELOC_GOT" | "X86_64_RELOC_GOT_LOAD" as kind},
                "Symbol": {"Value": s},
                "Offset": offset,
            }:
                offset += base
                s = s.removeprefix(self.target.prefix)
                value, symbol = HoleValue.DATA, None
                addend = (
                    int.from_bytes(raw[offset : offset + 4], "little")
                    + self._global_offset_table_lookup(s)
                    - 4
                )
            case {
                "Type": {"Value": kind},
                "Symbol": {"Value": s},
                "Offset": offset,
            }:
                offset += base
                s = s.removeprefix(self.target.prefix)
                value, symbol = self._symbol_to_value(s)
                addend = 0
            case _:
                raise NotImplementedError(relocation)
        return Hole(offset, kind, value, symbol, addend)


@dataclasses.dataclass
class Target:
    pattern: str
    model: typing.Literal["small", "medium", "large"]
    pyconfig: pathlib.Path
    alignment: int
    prefix: str
    parser: type[MachO | COFF | ELF]


TARGETS = [
    Target(
        pattern=r"aarch64-apple-darwin.*",
        model="large",
        pyconfig=PYCONFIG_H,
        alignment=8,
        prefix="_",
        parser=MachO,
    ),
    Target(
        pattern=r"aarch64-.*-linux-gnu",
        model="large",
        pyconfig=PYCONFIG_H,
        alignment=8,
        prefix="",
        parser=ELF,
    ),
    Target(
        pattern=r"i686-pc-windows-msvc",
        model="small",
        pyconfig=PC_PYCONFIG_H,
        alignment=1,
        prefix="_",
        parser=COFF,
    ),
    Target(
        pattern=r"x86_64-apple-darwin.*",
        model="medium",
        pyconfig=PYCONFIG_H,
        alignment=1,
        prefix="_",
        parser=MachO,
    ),
    Target(
        pattern=r"x86_64-pc-windows-msvc",
        model="medium",
        pyconfig=PC_PYCONFIG_H,
        alignment=1,
        prefix="",
        parser=COFF,
    ),
    Target(
        pattern=r"x86_64-.*-linux-gnu",
        model="medium",
        pyconfig=PYCONFIG_H,
        alignment=1,
        prefix="",
        parser=ELF,
    ),
]


def get_target(host: str) -> Target:
    for target in TARGETS:
        if re.fullmatch(target.pattern, host):
            return target
    raise NotImplementedError(host)


CFLAGS = [
    "-O3",
    "-fno-asynchronous-unwind-tables",
    # Position-independent code adds indirection to every load and jump:
    "-fno-pic",
    "-fno-jump-tables",  # XXX: SET_FUNCTION_ATTRIBUTE on 32-bit Windows debug builds
    "-fno-stack-protector",
]

CPPFLAGS = [
    "-DPy_BUILD_CORE",
    "-D_PyJIT_ACTIVE",
    f"-I{INCLUDE}",
    f"-I{INCLUDE_INTERNAL}",
    f"-I{PYTHON}",
]


class Compiler:
    def __init__(
        self,
        *,
        verbose: bool = False,
        target: Target,
        host: str,
    ) -> None:
        self._stencils_built: dict[str, StencilGroup] = {}
        self._verbose = verbose
        self._clang = require_llvm_tool("clang")
        self._readobj = require_llvm_tool("llvm-readobj")
        self._objdump = find_llvm_tool("llvm-objdump")
        self._target = target
        self._host = host

    async def _compile(
        self, opname: str, c: pathlib.Path, tempdir: pathlib.Path
    ) -> None:
        o = tempdir / f"{opname}.o"
        flags = [
            *CFLAGS,
            *CPPFLAGS,
            f"--target={self._host}",
            "-D_DEBUG" if sys.argv[2:] == ["-d"] else "-DNDEBUG",  # XXX
            f"-D_JIT_OPCODE={opname}",
            f"-I{self._target.pyconfig.parent}",
            "-c",
            # We have three options for code model:
            # - "small": assumes that code and data reside in the lowest 2GB of
            #   memory (128MB on aarch64)
            # - "medium": assumes that code resides in the lowest 2GB of memory,
            #   and makes no assumptions about data (not available on aarch64)
            # - "large": makes no assumptions about either code or data
            # We need 64-bit addresses for data everywhere, but we'd *really*
            # prefer direct short jumps instead of indirect long ones where
            # possible. So, we use the "large" code model on aarch64 and the
            # "medium" code model elsewhere, which gives us correctly-sized
            # direct jumps and immediate data loads on basically all platforms:
            f"-mcmodel={self._target.model}",
        ]
        await run(self._clang, *flags, "-o", o, c)
        self._stencils_built[opname] = await self._target.parser(
            o, self._readobj, self._objdump, self._target
        ).parse()

    async def build(self) -> None:
        generated_cases = PYTHON_EXECUTOR_CASES_C_H.read_text()
        opnames = sorted(re.findall(r"\n {8}case (\w+): \{\n", generated_cases))
        with tempfile.TemporaryDirectory() as tempdir:
            work = pathlib.Path(tempdir).resolve()
            async with asyncio.TaskGroup() as group:
                for stub in STUBS:
                    task = self._compile(stub, TOOLS_JIT / f"{stub}.c", work)
                    group.create_task(task)
                for opname in opnames:
                    task = self._compile(opname, TOOLS_JIT_TEMPLATE_C, work)
                    group.create_task(task)

    def dump(self) -> typing.Generator[str, None, None]:
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
        for opname, stencil in sorted(self._stencils_built.items()):
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
                        format_addend(hole.symbol, hole.addend),
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
                        format_addend(hole.symbol, hole.addend),
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
        assert opnames[-len(STUBS) :] == STUBS
        for stub in opnames[-len(STUBS) :]:
            yield f"static const StencilGroup {stub}_stencil_group = INIT_STENCIL_GROUP({stub});"
        yield ""
        yield "static const StencilGroup stencil_groups[512] = {"
        for opname in opnames[: -len(STUBS)]:
            yield f"    [{opname}] = INIT_STENCIL_GROUP({opname}),"
        yield "};"
        yield ""
        yield "#define GET_PATCHES() { \\"
        for value in HoleValue:
            yield f"    [HoleValue_{value.name}] = (uint64_t)0xBADBADBADBADBADB, \\"
        yield "}"


def format_addend(symbol: str | None, addend: int) -> str:
    symbol_part = f"(uintptr_t)&{symbol}" if symbol else ""
    addend %= 1 << 64
    if symbol_part and not addend:
        return symbol_part
    if addend & (1 << 63):
        return f"{symbol_part}{hex(addend - (1 << 64))}"
    return f"{f'{symbol_part}+' if symbol_part else ''}{hex(addend)}"


def main(host: str) -> None:
    target = get_target(host)
    hasher = hashlib.sha256(host.encode())
    hasher.update(PYTHON_EXECUTOR_CASES_C_H.read_bytes())
    hasher.update(target.pyconfig.read_bytes())
    for source in sorted(TOOLS_JIT.iterdir()):
        if source.is_file():
            hasher.update(source.read_bytes())
    hasher.update(b"\x00" * ("-d" in sys.argv))  # XXX
    digest = hasher.hexdigest()
    if PYTHON_JIT_STENCILS_H.exists():
        with PYTHON_JIT_STENCILS_H.open() as file:
            if file.readline().removeprefix("// ").removesuffix("\n") == digest:
                return
    compiler = Compiler(verbose=True, target=target, host=host)
    asyncio.run(compiler.build())
    with PYTHON_JIT_STENCILS_H.open("w") as file:
        file.write(f"// {digest}\n")
        for line in compiler.dump():
            file.write(f"{line}\n")


if __name__ == "__main__":
    main(sys.argv[1])
