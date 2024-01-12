"""
Build a template JIT for CPython, based on copy-and-patch.
"""

# XXX: I'll probably refactor this file a bit before merging to make it easier
# to understand... it's changed a lot over the last few months, and the way
# things are done here definitely has some historical cruft. The high-level
# architecture and the actual generated code probably won't change, though.

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
INCLUDE_INTERNAL_MIMALLOC = INCLUDE_INTERNAL / "mimalloc"
PYTHON = CPYTHON / "Python"
PYTHON_EXECUTOR_CASES_C_H = PYTHON / "executor_cases.c.h"
TOOLS_JIT_TEMPLATE_C = TOOLS_JIT / "template.c"


@enum.unique
class HoleValue(enum.Enum):
    CODE = enum.auto()
    CONTINUE = enum.auto()
    DATA = enum.auto()
    EXECUTOR = enum.auto()
    GOT = enum.auto()
    OPARG = enum.auto()
    OPERAND = enum.auto()
    TARGET = enum.auto()
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


_S = typing.TypeVar("_S", schema.COFFSection, schema.ELFSection, schema.MachOSection)
_R = typing.TypeVar(
    "_R", schema.COFFRelocation, schema.ELFRelocation, schema.MachORelocation
)


@dataclasses.dataclass
class Stencil:
    body: bytearray = dataclasses.field(default_factory=bytearray)
    holes: list[Hole] = dataclasses.field(default_factory=list)
    disassembly: list[str] = dataclasses.field(default_factory=list)
    symbols: dict[str, int] = dataclasses.field(default_factory=dict, init=False)
    offsets: dict[int, int] = dataclasses.field(default_factory=dict, init=False)
    relocations: list[Hole] = dataclasses.field(default_factory=list, init=False)

    def pad(self, alignment: int) -> None:
        offset = len(self.body)
        padding = -offset % alignment
        self.disassembly.append(f"{offset:x}: {' '.join(['00'] * padding)}")
        self.body.extend([0] * padding)

    def emit_aarch64_trampoline(self, hole: Hole) -> typing.Iterator[Hole]:
        """Even with the large code model, AArch64 Linux insists on 28-bit jumps."""
        base = len(self.body)
        where = slice(hole.offset, hole.offset + 4)
        instruction = int.from_bytes(self.body[where], sys.byteorder)
        instruction &= 0xFC000000
        instruction |= ((base - hole.offset) >> 2) & 0x03FFFFFF
        self.body[where] = instruction.to_bytes(4, sys.byteorder)
        self.disassembly += [
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
        self.body.extend(0xD2800008.to_bytes(4, sys.byteorder))
        self.body.extend(0xF2A00008.to_bytes(4, sys.byteorder))
        self.body.extend(0xF2C00008.to_bytes(4, sys.byteorder))
        self.body.extend(0xF2E00008.to_bytes(4, sys.byteorder))
        self.body.extend(0xD61F0100.to_bytes(4, sys.byteorder))
        yield hole.replace(offset=base + 4 * 0, kind="R_AARCH64_MOVW_UABS_G0_NC")
        yield hole.replace(offset=base + 4 * 1, kind="R_AARCH64_MOVW_UABS_G1_NC")
        yield hole.replace(offset=base + 4 * 2, kind="R_AARCH64_MOVW_UABS_G2_NC")
        yield hole.replace(offset=base + 4 * 3, kind="R_AARCH64_MOVW_UABS_G3")


@dataclasses.dataclass
class StencilGroup:
    code: Stencil = dataclasses.field(default_factory=Stencil)
    data: Stencil = dataclasses.field(default_factory=Stencil)
    global_offset_table: dict[str, int] = dataclasses.field(
        default_factory=dict, init=False
    )

    def global_offset_table_lookup(self, symbol: str | None) -> int:
        """Even when disabling PIC, macOS insists on using the global offset table."""
        if symbol is None:
            return len(self.data.body)
        default = 8 * len(self.global_offset_table)
        return len(self.data.body) + self.global_offset_table.setdefault(
            symbol, default
        )

    def emit_global_offset_table(self) -> None:
        global_offset_table = len(self.data.body)
        for s, offset in self.global_offset_table.items():
            if s in self.code.symbols:
                value, symbol = HoleValue.CODE, None
                addend = self.code.symbols[s]
            elif s in self.data.symbols:
                value, symbol = HoleValue.DATA, None
                addend = self.data.symbols[s]
            else:
                value, symbol = _symbol_to_value(s)
                addend = 0
            self.data.holes.append(
                Hole(global_offset_table + offset, "R_X86_64_64", value, symbol, addend)
            )
            value_part = value.name if value is not HoleValue.ZERO else ""
            if value_part and not symbol and not addend:
                addend_part = ""
            else:
                addend_part = f"&{symbol}" if symbol else ""
                addend_part += format_addend(addend, signed=symbol is not None)
                if value_part:
                    value_part += "+"
            self.data.disassembly.append(
                f"{len(self.data.body):x}: {value_part}{addend_part}"
            )
            self.data.body.extend([0] * 8)


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
    return out or b""


def _symbol_to_value(symbol: str) -> tuple[HoleValue, str | None]:
    try:
        if symbol.startswith("_JIT_"):
            return HoleValue[symbol.removeprefix("_JIT_")], None
    except KeyError:
        pass
    return HoleValue.ZERO, symbol


@dataclasses.dataclass
class Target(typing.Generic[_S, _R]):
    triple: str
    _: dataclasses.KW_ONLY
    alignment: int = 1
    prefix: str = ""
    debug: bool = False
    verbose: bool = False

    def sha256(self) -> bytes:
        hasher = hashlib.sha256()
        hasher.update(self.triple.encode())
        hasher.update(self.alignment.to_bytes())
        hasher.update(self.prefix.encode())
        return hasher.digest()

    async def parse(self, path: pathlib.Path) -> StencilGroup:
        group = StencilGroup()
        objdump = llvm.find_tool("llvm-objdump", echo=self.verbose)
        if objdump is not None:
            flags = ["--disassemble", "--reloc"]
            output = await run(objdump, *flags, path, capture=True, echo=self.verbose)
            assert output is not None
            group.code.disassembly.extend(
                line.expandtabs().strip()
                for line in output.decode().splitlines()
                if not line.isspace()
            )
        readobj = llvm.require_tool("llvm-readobj", echo=self.verbose)
        flags = [
            "--elf-output-style=JSON",
            "--expand-relocs",
            # "--pretty-print",
            "--section-data",
            "--section-relocations",
            "--section-symbols",
            "--sections",
        ]
        output = await run(readobj, *flags, path, capture=True, echo=self.verbose)
        assert output is not None
        # --elf-output-style=JSON is only *slightly* broken on Mach-O...
        output = output.replace(b"PrivateExtern\n", b"\n")
        output = output.replace(b"Extern\n", b"\n")
        # ...and also COFF:
        output = output[output.index(b"[", 1, None) :]
        output = output[: output.rindex(b"]", None, -1) + 1]
        sections: list[dict[typing.Literal["Section"], _S]] = json.loads(output)
        for wrapped_section in sections:
            self._handle_section(wrapped_section["Section"], group)
        assert group.code.symbols["_JIT_ENTRY"] == 0
        if group.data.body:
            line = f"0: {str(bytes(group.data.body)).removeprefix('b')}"
            group.data.disassembly.append(line)
        group.data.pad(8)
        self._process_relocations(group.code, group)
        remaining: list[Hole] = []
        for hole in group.code.holes:
            if (
                hole.kind in {"R_AARCH64_CALL26", "R_AARCH64_JUMP26"}
                and hole.value is HoleValue.ZERO
            ):
                remaining.extend(group.code.emit_aarch64_trampoline(hole))
            else:
                remaining.append(hole)
        group.code.holes[:] = remaining
        group.code.pad(self.alignment)
        self._process_relocations(group.data, group)
        group.emit_global_offset_table()
        group.code.holes.sort(key=lambda hole: hole.offset)
        group.data.holes.sort(key=lambda hole: hole.offset)
        return group

    def _process_relocations(self, stencil: Stencil, group: StencilGroup) -> None:
        for hole in stencil.relocations:
            if hole.value is HoleValue.GOT:
                value, symbol = HoleValue.DATA, None
                addend = hole.addend + group.global_offset_table_lookup(hole.symbol)
                hole = hole.replace(value=value, symbol=symbol, addend=addend)
            elif hole.symbol in group.data.symbols:
                value, symbol = HoleValue.DATA, None
                addend = hole.addend + group.data.symbols[hole.symbol]
                hole = hole.replace(value=value, symbol=symbol, addend=addend)
            elif hole.symbol in group.code.symbols:
                value, symbol = HoleValue.CODE, None
                addend = hole.addend + group.code.symbols[hole.symbol]
                hole = hole.replace(value=value, symbol=symbol, addend=addend)
            stencil.holes.append(hole)

    def _handle_section(self, section: _S, group: StencilGroup) -> None:
        raise NotImplementedError(type(self))

    def _handle_relocation(self, base: int, relocation: _R, raw: bytes) -> Hole:
        raise NotImplementedError(type(self))

    async def _compile(
        self, opname: str, c: pathlib.Path, tempdir: pathlib.Path
    ) -> StencilGroup:
        o = tempdir / f"{opname}.o"
        flags = [
            f"--target={self.triple}",
            "-DPy_BUILD_CORE",
            "-D_DEBUG" if self.debug else "-DNDEBUG",
            f"-D_JIT_OPCODE={opname}",
            "-D_PyJIT_ACTIVE",
            "-D_Py_JIT",
            "-I.",
            f"-I{INCLUDE}",
            f"-I{INCLUDE_INTERNAL}",
            f"-I{INCLUDE_INTERNAL_MIMALLOC}",
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
            # - "small": the default, assumes that code and data reside in the
            #   lowest 2GB of memory (128MB on aarch64)
            # - "medium": assumes that code resides in the lowest 2GB of memory,
            #   and makes no assumptions about data (not available on aarch64)
            # - "large": makes no assumptions about either code or data
            "-mcmodel=large",
            "-std=c11",
        ]
        clang = llvm.require_tool("clang", echo=self.verbose)
        await run(clang, *flags, "-o", o, c, echo=self.verbose)
        return await self.parse(o)

    async def build_stencils(self) -> dict[str, StencilGroup]:
        generated_cases = PYTHON_EXECUTOR_CASES_C_H.read_text()
        opnames = sorted(re.findall(r"\n {8}case (\w+): \{\n", generated_cases))
        tasks = []
        with tempfile.TemporaryDirectory() as tempdir:
            work = pathlib.Path(tempdir).resolve()
            async with asyncio.TaskGroup() as group:
                for opname in opnames:
                    coro = self._compile(opname, TOOLS_JIT_TEMPLATE_C, work)
                    tasks.append(group.create_task(coro, name=opname))
        return {task.get_name(): task.result() for task in tasks}

    def build(self, out: pathlib.Path) -> None:
        jit_stencils = out / "jit_stencils.h"
        hasher = hashlib.sha256()
        hasher.update(self.sha256())
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
        stencil_groups = asyncio.run(self.build_stencils())
        with jit_stencils.open("w") as file:
            file.write(f"// {digest}\n")
            for line in dump(stencil_groups):
                file.write(f"{line}\n")


class ELF(Target[schema.ELFSection, schema.ELFRelocation]):
    def _handle_section(self, section: schema.ELFSection, group: StencilGroup) -> None:
        section_type = section["Type"]["Value"]
        flags = {flag["Name"] for flag in section["Flags"]["Flags"]}
        if section_type == "SHT_RELA":
            assert "SHF_INFO_LINK" in flags, flags
            assert not section["Symbols"]
            if section["Info"] in group.code.offsets:
                stencil = group.code
            else:
                stencil = group.data
            base = stencil.offsets[section["Info"]]
            for wrapped_relocation in section["Relocations"]:
                relocation = wrapped_relocation["Relocation"]
                hole = self._handle_relocation(base, relocation, stencil.body)
                stencil.relocations.append(hole)
        elif section_type == "SHT_PROGBITS":
            if "SHF_ALLOC" not in flags:
                return
            if "SHF_EXECINSTR" in flags:
                stencil = group.code
            else:
                stencil = group.data
            stencil.offsets[section["Index"]] = len(stencil.body)
            for wrapped_symbol in section["Symbols"]:
                symbol = wrapped_symbol["Symbol"]
                offset = len(stencil.body) + symbol["Value"]
                name = symbol["Name"]["Value"]
                name = name.removeprefix(self.prefix)
                assert name not in stencil.symbols
                stencil.symbols[name] = offset
            section_data = section["SectionData"]
            stencil.body.extend(section_data["Bytes"])
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
                s = s.removeprefix(self.prefix)
                value, symbol = _symbol_to_value(s)
            case _:
                raise NotImplementedError(relocation)
        return Hole(offset, kind, value, symbol, addend)


class COFF(Target[schema.COFFSection, schema.COFFRelocation]):
    def _handle_section(self, section: schema.COFFSection, group: StencilGroup) -> None:
        flags = {flag["Name"] for flag in section["Characteristics"]["Flags"]}
        if "SectionData" in section:
            section_data_bytes = section["SectionData"]["Bytes"]
        else:
            # Zeroed BSS data, seen with printf debugging calls:
            section_data_bytes = [0] * section["RawDataSize"]
        if "IMAGE_SCN_MEM_EXECUTE" in flags:
            stencil = group.code
        elif "IMAGE_SCN_MEM_READ" in flags:
            stencil = group.data
        else:
            return
        base = stencil.offsets[section["Number"]] = len(stencil.body)
        stencil.body.extend(section_data_bytes)
        for wrapped_symbol in section["Symbols"]:
            symbol = wrapped_symbol["Symbol"]
            offset = base + symbol["Value"]
            name = symbol["Name"]
            name = name.removeprefix(self.prefix)
            stencil.symbols[name] = offset
        for wrapped_relocation in section["Relocations"]:
            relocation = wrapped_relocation["Relocation"]
            hole = self._handle_relocation(base, relocation, stencil.body)
            stencil.relocations.append(hole)

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
                s = s.removeprefix(self.prefix)
                value, symbol = _symbol_to_value(s)
                addend = int.from_bytes(raw[offset : offset + 8], "little")
            case {
                "Type": {"Value": "IMAGE_REL_I386_DIR32" as kind},
                "Symbol": s,
                "Offset": offset,
            }:
                offset += base
                s = s.removeprefix(self.prefix)
                value, symbol = _symbol_to_value(s)
                addend = int.from_bytes(raw[offset : offset + 4], "little")
            case _:
                raise NotImplementedError(relocation)
        return Hole(offset, kind, value, symbol, addend)


class MachO(Target[schema.MachOSection, schema.MachORelocation]):
    def _handle_section(
        self, section: schema.MachOSection, group: StencilGroup
    ) -> None:
        assert section["Address"] >= len(group.code.body)
        assert "SectionData" in section
        section_data = section["SectionData"]
        flags = {flag["Name"] for flag in section["Attributes"]["Flags"]}
        name = section["Name"]["Value"]
        name = name.removeprefix(self.prefix)
        if "SomeInstructions" in flags:
            stencil = group.code
            bias = 0
            stencil.symbols[name] = section["Address"] - bias
        else:
            stencil = group.data
            bias = len(group.code.body)
            stencil.symbols[name] = len(group.code.body)
        base = stencil.offsets[section["Index"]] = section["Address"] - bias
        stencil.body.extend(
            [0] * (section["Address"] - len(group.code.body) - len(group.data.body))
        )
        stencil.body.extend(section_data["Bytes"])
        assert "Symbols" in section
        for wrapped_symbol in section["Symbols"]:
            symbol = wrapped_symbol["Symbol"]
            offset = symbol["Value"] - bias
            name = symbol["Name"]["Value"]
            name = name.removeprefix(self.prefix)
            stencil.symbols[name] = offset
        assert "Relocations" in section
        for wrapped_relocation in section["Relocations"]:
            relocation = wrapped_relocation["Relocation"]
            hole = self._handle_relocation(base, relocation, stencil.body)
            stencil.relocations.append(hole)

    def _handle_relocation(
        self, base: int, relocation: schema.MachORelocation, raw: bytes
    ) -> Hole:
        symbol: str | None
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
                s = s.removeprefix(self.prefix)
                value, symbol = HoleValue.GOT, s
                addend = 0
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
                s = s.removeprefix(self.prefix)
                value, symbol = _symbol_to_value(s)
                addend = 0
            case _:
                raise NotImplementedError(relocation)
        # Turn Clang's weird __bzero calls into normal bzero calls:
        if symbol == "__bzero":
            symbol = "bzero"
        return Hole(offset, kind, value, symbol, addend)


def get_target(
    host: str, *, debug: bool = False, verbose: bool = False
) -> COFF | ELF | MachO:
    target: COFF | ELF | MachO
    if re.fullmatch(r"aarch64-apple-darwin.*", host):
        target = MachO("aarch64-apple-darwin", alignment=8, prefix="_")
    elif re.fullmatch(r"aarch64-.*-linux-gnu", host):
        target = ELF("aarch64-unknown-linux-gnu", alignment=8)
    elif re.fullmatch(r"i686-pc-windows-msvc", host):
        target = COFF("i686-pc-windows-msvc", prefix="_")
    elif re.fullmatch(r"x86_64-apple-darwin.*", host):
        target = MachO("x86_64-apple-darwin", prefix="_")
    elif re.fullmatch(r"x86_64-pc-windows-msvc", host):
        target = COFF("x86_64-pc-windows-msvc")
    elif re.fullmatch(r"x86_64-.*-linux-gnu", host):
        target = ELF("x86_64-unknown-linux-gnu")
    else:
        raise ValueError(host)
    return dataclasses.replace(target, debug=debug, verbose=verbose)


def dump_header() -> typing.Iterator[str]:
    yield f"// $ {shlex.join([sys.executable, *sys.argv])}"
    yield ""
    yield "typedef enum {"
    for kind in typing.get_args(schema.HoleKind):
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
    yield "    const Stencil code;"
    yield "    const Stencil data;"
    yield "} StencilGroup;"
    yield ""


def dump_footer(opnames: typing.Iterable[str]) -> typing.Iterator[str]:
    yield "#define INIT_STENCIL(STENCIL) {                         \\"
    yield "    .body_size = Py_ARRAY_LENGTH(STENCIL##_body) - 1,   \\"
    yield "    .body = STENCIL##_body,                             \\"
    yield "    .holes_size = Py_ARRAY_LENGTH(STENCIL##_holes) - 1, \\"
    yield "    .holes = STENCIL##_holes,                           \\"
    yield "}"
    yield ""
    yield "#define INIT_STENCIL_GROUP(OP) {     \\"
    yield "    .code = INIT_STENCIL(OP##_code), \\"
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


def dump_stencil(opname: str, group: StencilGroup) -> typing.Iterator[str]:
    yield f"// {opname}"
    for part, stencil in [("code", group.code), ("data", group.data)]:
        for line in stencil.disassembly:
            yield f"// {line}"
        if stencil.body:
            size = len(stencil.body) + 1
            yield f"static const unsigned char {opname}_{part}_body[{size}] = {{"
            for i in range(0, len(stencil.body), 8):
                row = " ".join(f"{byte:#02x}," for byte in stencil.body[i : i + 8])
                yield f"    {row}"
            yield "};"
        else:
            yield f"static const unsigned char {opname}_{part}_body[1];"
        if stencil.holes:
            size = len(stencil.holes) + 1
            yield f"static const Hole {opname}_{part}_holes[{size}] = {{"
            for hole in stencil.holes:
                parts = [
                    f"{hole.offset:#x}",
                    f"HoleKind_{hole.kind}",
                    f"HoleValue_{hole.value.name}",
                    f"&{hole.symbol}" if hole.symbol else "NULL",
                    format_addend(hole.addend),
                ]
                yield f"    {{{', '.join(parts)}}},"
            yield "};"
        else:
            yield f"static const Hole {opname}_{part}_holes[1];"
    yield ""


def dump(groups: dict[str, StencilGroup]) -> typing.Iterator[str]:
    yield from dump_header()
    for opname, group in groups.items():
        yield from dump_stencil(opname, group)
    yield from dump_footer(groups)


def format_addend(addend: int, signed: bool = False) -> str:
    """Convert unsigned 64-bit values to signed 64-bit values, and format as hex."""
    addend %= 1 << 64
    if addend & (1 << 63):
        addend -= 1 << 64
    return f"{addend:{'+#x' if signed else '#x'}}"


def main() -> None:
    """Build the JIT!"""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("target", help="a PEP 11 target triple to compile for")
    parser.add_argument(
        "-d", "--debug", action="store_true", help="compile for a debug build of Python"
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="echo commands as they are run"
    )
    args = parser.parse_args()
    target = get_target(args.target, debug=args.debug, verbose=args.verbose)
    target.build(pathlib.Path.cwd())


if __name__ == "__main__":
    main()
