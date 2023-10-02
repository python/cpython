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

class FileSummary(typing.TypedDict):
    File: str
    Format: str
    Arch: str
    AddressSize: int
    LoadName: str

class File(typing.TypedDict):
    FileSummary: FileSummary
    Sections: list[dict[typing.Literal["Section"], Section]]

Object = list[dict[str, File] | File]

@enum.unique
class HoleKind(enum.Enum):
    R_386_32 = enum.auto()
    R_386_PC32 = enum.auto()
    R_AARCH64_ABS64 = enum.auto()
    R_AARCH64_ADR_GOT_PAGE = enum.auto()
    R_AARCH64_CALL26 = enum.auto()
    R_AARCH64_JUMP26 = enum.auto()
    R_AARCH64_LD64_GOT_LO12_NC = enum.auto()
    R_AARCH64_MOVW_UABS_G0_NC = enum.auto()
    R_AARCH64_MOVW_UABS_G1_NC = enum.auto()
    R_AARCH64_MOVW_UABS_G2_NC = enum.auto()
    R_AARCH64_MOVW_UABS_G3 = enum.auto()
    R_X86_64_64 = enum.auto()
    R_X86_64_GOTOFF64 = enum.auto()
    R_X86_64_GOTPC32 = enum.auto()
    R_X86_64_GOTPCRELX = enum.auto()
    R_X86_64_PC32 = enum.auto()
    R_X86_64_PLT32 = enum.auto()
    R_X86_64_REX_GOTPCRELX = enum.auto()

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
        self.got_entries = []
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
        self._data: Object = json.loads(output)
        file = self._data[0]
        if str(self.path) in file:
            file = file[str(self.path)]
        for section in unwrap(file["Sections"], "Section"):
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
            holes.append(Hole(HoleKind.R_X86_64_64, got_symbol, got + 8 * i, addend))
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
    def _handle_section(self, section: Section) -> None:
        type = section["Type"]["Value"]
        flags = {flag["Name"] for flag in section["Flags"]["Flags"]}
        if type in {"SHT_REL", "SHT_RELA"}:
            assert "SHF_INFO_LINK" in flags, flags
            before = self.body_offsets[section["Info"]]
            assert not section["Symbols"]
            for relocation in unwrap(section["Relocations"], "Relocation"):
                self.relocations.append((before, relocation))
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
                assert name not in self.body_symbols
                self.body_symbols[name] = offset
        else:
            assert type in {
                "SHT_GROUP",
                "SHT_LLVM_ADDRSIG",
                "SHT_NULL",
                "SHT_STRTAB",
                "SHT_SYMTAB",
            }, type

    def _handle_relocation(
        self,
        base: int,
        relocation: Relocation,
    ) -> Hole | None:
        kind = HoleKind[relocation["Type"]["Value"]]
        symbol = relocation["Symbol"]["Value"]
        offset = relocation["Offset"] + base
        addend = relocation.get("Addend", 0)  # XXX: SPM for SHT_REL (R_386) vs SHT_RELA
        if kind in (HoleKind.R_X86_64_GOTPC32, HoleKind.R_X86_64_GOTPCRELX, HoleKind.R_X86_64_REX_GOTPCRELX):
            if kind == HoleKind.R_X86_64_GOTPC32:
                assert symbol == "_GLOBAL_OFFSET_TABLE_", symbol
            else:
                if (symbol, 0) not in self.got_entries:
                    self.got_entries.append((symbol, 0))
                while len(self.body) % 8:
                    self.body.append(0)
                addend += 8 * self.got_entries.index((symbol, 0))
            addend += len(self.body)
            self.body[offset : offset + 4] = ((addend - offset) % (1 << 32)).to_bytes(4, "little")
            return None
        elif kind in (HoleKind.R_AARCH64_ADR_GOT_PAGE, HoleKind.R_AARCH64_LD64_GOT_LO12_NC):
            if (symbol, 0) not in self.got_entries:
                self.got_entries.append((symbol, 0))
            while len(self.body) % 8:
                self.body.append(0)
            addend += 8 * self.got_entries.index((symbol, 0))
            addend += len(self.body)
            symbol = "_JIT_BASE"
        elif kind == HoleKind.R_X86_64_GOTOFF64:
            addend += offset - len(self.body)
        elif kind in (HoleKind.R_386_32, HoleKind.R_386_PC32):
            assert addend == 0, addend
            addend = int.from_bytes(self.body[offset : offset + 4], "little")
        return Hole(kind, symbol, offset, addend)

class aarch64_apple_darwin(Engine):
    pattern = r"aarch64-apple-darwin.*"
    target_front = "aarch64-apple-darwin"
    target_back = "aarch64-elf"
    code_model = "large"
    ghccc = False

class aarch64_unknown_linux_gnu(Engine):
    pattern = r"aarch64-.*-linux-gnu"
    target_front = "aarch64-unknown-linux-gnu"
    target_back = "aarch64-elf"
    code_model = "large"
    ghccc = False

class i686_pc_windows_msvc(Engine):
    pattern = r"i686-pc-windows-msvc"
    target_front = "i686-pc-windows-msvc"
    target_back = "i686-pc-windows-msvc-elf"
    code_model = "small"
    ghccc = True

class x86_64_apple_darwin(Engine):
    pattern = r"x86_64-apple-darwin.*"
    target_front = "x86_64-apple-darwin"
    target_back = "x86_64-elf"
    code_model = "medium"
    ghccc = True

class x86_64_pc_windows_msvc(Engine):
    pattern = r"x86_64-pc-windows-msvc"
    target_front = "x86_64-pc-windows-msvc"
    target_back = "x86_64-pc-windows-msvc-elf"
    code_model = "medium"
    ghccc = True

class x86_64_unknown_linux_gnu(Engine):
    pattern = r"x86_64-.*-linux-gnu"
    target_front = "x86_64-unknown-linux-gnu"
    target_back = "x86_64-elf"
    code_model = "medium"
    ghccc = True


CFLAGS = [
    "-O3",
    "-Wno-override-module",
    # Keep library calls from sneaking in:
    "-ffreestanding",  # XXX
    # We don't need this (and it causes weird relocations):
    "-fno-asynchronous-unwind-tables",  # XXX
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
        await run(self._clang, *CFLAGS, *CPPFLAGS, "-emit-llvm", "-S", *defines, "-o", ll, c)
        self._use_ghccc(ll)
        await run(self._clang, *CFLAGS, "-c", "-o", o, ll)
        self._stencils_built[opname] = await self._parser(
            o, self._readobj, self._objdump
        ).parse()

    async def build(self) -> None:
        generated_cases = PYTHON_EXECUTOR_CASES_C_H.read_text()
        opnames = sorted(re.findall(r"\n {8}case (\w+): \{\n", generated_cases))
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
        lines.append(f"typedef enum {{")
        for kind in HoleKind:
            lines.append(f"    {kind.name},")
        lines.append(f"}} HoleKind;")
        lines.append(f"")
        lines.append(f"typedef enum {{")
        for value in HoleValue:
            lines.append(f"    {value.name},")
        lines.append(f"}} HoleValue;")
        lines.append(f"")
        lines.append(f"typedef struct {{")
        lines.append(f"    const HoleKind kind;")
        lines.append(f"    const uint64_t offset;")
        lines.append(f"    const uint64_t addend;")
        lines.append(f"    const HoleValue value;")
        lines.append(f"}} Hole;")
        lines.append(f"")
        lines.append(f"typedef struct {{")
        lines.append(f"    const HoleKind kind;")
        lines.append(f"    const uint64_t offset;")
        lines.append(f"    const uint64_t addend;")
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
            kind_width = max([0, *(len(f"{hole.kind.name}") for hole in stencil.holes)])
            offset_width = max([0, *(len(f"{hole.offset:x}") for hole in stencil.holes)])
            addend_width = max([0, *(len(f"{hole.addend % (1 << 64):x}") for hole in stencil.holes)])
            value_width = max([0, *(len(f"{hole.symbol}") for hole in stencil.holes if hole.symbol in HoleValue.__members__)])
            symbol_width = max([0, *(len(f"{symbols.index(hole.symbol)}") for hole in stencil.holes if hole.symbol not in HoleValue.__members__)])
            for hole in stencil.holes:
                if hole.symbol in HoleValue.__members__:
                    value = HoleValue[hole.symbol]
                    holes.append(
                        f"    {{.kind={hole.kind.name:{kind_width}}, .offset=0x{hole.offset:0{offset_width}x}, .addend=0x{hole.addend % (1 << 64):0{addend_width}x}, .value={value.name:{value_width}}}},"
                    )
                else:
                    loads.append(
                        f"    {{.kind={hole.kind.name:{kind_width}}, .offset=0x{hole.offset:0{offset_width}x}, .addend=0x{hole.addend % (1 << 64):0{addend_width}x}, .symbol={symbols.index(hole.symbol):{symbol_width}}}},  // {hole.symbol}"
                    )
            if holes:
                lines.append(
                    f"static const Hole {opname}_stencil_holes[{len(holes) + 1}] = {{"
                )
                for hole in holes:
                    lines.append(hole)
                lines.append(f"}};")
            else:
                lines.append(f"static const Hole {opname}_stencil_holes[{len(holes) + 1}];")
            if loads:
                lines.append(
                    f"static const SymbolLoad {opname}_stencil_loads[{len(loads) + 1}] = {{"
                )
                for load in loads:
                    lines.append(load)
                lines.append(f"}};")
            else:
                lines.append(f"static const SymbolLoad {opname}_stencil_loads[{len(loads) + 1}];")
            lines.append(f"")
        lines.append(f"")
        lines.append(f"static const char *const symbols[{len(symbols)}] = {{")
        for symbol in symbols:
            lines.append(f'    "{symbol}",')
        lines.append(f"}};")
        lines.append(f"")
        lines.append(f"static uint64_t symbol_addresses[{len(symbols)}];")
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
        lines.append(f"#define INIT_HOLE(NAME) [NAME] = (uint64_t)0xBADBADBADBADBADB")
        lines.append(f"")
        lines.append(f"#define GET_PATCHES() {{ \\")
        for value in HoleValue:
            lines.append(f"    INIT_HOLE({value.name}), \\")
        lines.append(f"}}")
        lines.append(f"")
        return "\n".join(lines)


def main(host: str) -> None:
    for engine, cppflags in [
        (aarch64_apple_darwin, [f"-I{ROOT}"]),
        (aarch64_unknown_linux_gnu, [f"-I{ROOT}"]),
        (i686_pc_windows_msvc, [f"-I{PC}"]),
        (x86_64_unknown_linux_gnu, [f"-I{ROOT}"]),
        (x86_64_apple_darwin, [f"-I{ROOT}"]),
        (x86_64_pc_windows_msvc, [f"-I{PC}"]),
    ]:
        if re.fullmatch(engine.pattern, host):
            break
    else:
        raise NotImplementedError(host)
    CPPFLAGS.extend(cppflags)
    CPPFLAGS.append("-D_DEBUG" if sys.argv[2:] == ["-d"] else "-DNDEBUG")
    CPPFLAGS.append(f"--target={engine.target_front}")
    CFLAGS.append(f"--target={engine.target_back}")
    CFLAGS.append(f"-mcmodel={engine.code_model}")
    hasher = hashlib.sha256(host.encode())
    hasher.update(PYTHON_EXECUTOR_CASES_C_H.read_bytes())
    for file in sorted(TOOLS_JIT.iterdir()):
        hasher.update(file.read_bytes())
    digest = hasher.hexdigest()
    if PYTHON_JIT_STENCILS_H.exists():
        with PYTHON_JIT_STENCILS_H.open() as file:
            if file.readline().removeprefix("// ").removesuffix("\n") == digest:
                return
    compiler = Compiler(verbose=True, ghccc=False, parser=engine)
    asyncio.run(compiler.build())
    with PYTHON_JIT_STENCILS_H.open("w") as file:
        file.write(f"// {digest}\n")
        file.write(compiler.dump())

if __name__ == "__main__":
    main(sys.argv[1])
