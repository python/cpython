"""Target-specific code generation, parsing, and processing."""
import asyncio
import dataclasses
import hashlib
import json
import os
import pathlib
import re
import sys
import tempfile
import typing

import _llvm
import _schema
import _stencils
import _writer

if sys.version_info < (3, 11):
    raise RuntimeError("Building the JIT compiler requires Python 3.11 or newer!")

TOOLS_JIT_BUILD = pathlib.Path(__file__).resolve()
TOOLS_JIT = TOOLS_JIT_BUILD.parent
TOOLS = TOOLS_JIT.parent
CPYTHON = TOOLS.parent
PYTHON_EXECUTOR_CASES_C_H = CPYTHON / "Python" / "executor_cases.c.h"
TOOLS_JIT_TEMPLATE_C = TOOLS_JIT / "template.c"


_S = typing.TypeVar("_S", _schema.COFFSection, _schema.ELFSection, _schema.MachOSection)
_R = typing.TypeVar(
    "_R", _schema.COFFRelocation, _schema.ELFRelocation, _schema.MachORelocation
)


@dataclasses.dataclass
class _Target(typing.Generic[_S, _R]):
    triple: str
    _: dataclasses.KW_ONLY
    alignment: int = 1
    prefix: str = ""
    debug: bool = False
    force: bool = False
    verbose: bool = False

    def _compute_digest(self, out: pathlib.Path) -> str:
        hasher = hashlib.sha256()
        hasher.update(self.triple.encode())
        hasher.update(self.debug.to_bytes())
        # These dependencies are also reflected in _JITSources in regen.targets:
        hasher.update(PYTHON_EXECUTOR_CASES_C_H.read_bytes())
        hasher.update((out / "pyconfig.h").read_bytes())
        for dirpath, _, filenames in sorted(os.walk(TOOLS_JIT)):
            for filename in filenames:
                hasher.update(pathlib.Path(dirpath, filename).read_bytes())
        return hasher.hexdigest()

    async def _parse(self, path: pathlib.Path) -> _stencils.StencilGroup:
        group = _stencils.StencilGroup()
        args = ["--disassemble", "--reloc", f"{path}"]
        output = await _llvm.maybe_run("llvm-objdump", args, echo=self.verbose)
        if output is not None:
            group.code.disassembly.extend(
                line.expandtabs().strip()
                for line in output.splitlines()
                if not line.isspace()
            )
        args = [
            "--elf-output-style=JSON",
            "--expand-relocs",
            # "--pretty-print",
            "--section-data",
            "--section-relocations",
            "--section-symbols",
            "--sections",
            f"{path}",
        ]
        output = await _llvm.run("llvm-readobj", args, echo=self.verbose)
        # --elf-output-style=JSON is only *slightly* broken on Mach-O...
        output = output.replace("PrivateExtern\n", "\n")
        output = output.replace("Extern\n", "\n")
        # ...and also COFF:
        output = output[output.index("[", 1, None) :]
        output = output[: output.rindex("]", None, -1) + 1]
        sections: list[dict[typing.Literal["Section"], _S]] = json.loads(output)
        for wrapped_section in sections:
            self._handle_section(wrapped_section["Section"], group)
        assert group.symbols["_JIT_ENTRY"] == (_stencils.HoleValue.CODE, 0)
        if group.data.body:
            line = f"0: {str(bytes(group.data.body)).removeprefix('b')}"
            group.data.disassembly.append(line)
        group.process_relocations()
        return group

    def _handle_section(self, section: _S, group: _stencils.StencilGroup) -> None:
        raise NotImplementedError(type(self))

    def _handle_relocation(
        self, base: int, relocation: _R, raw: bytes
    ) -> _stencils.Hole:
        raise NotImplementedError(type(self))

    async def _compile(
        self, opname: str, c: pathlib.Path, tempdir: pathlib.Path
    ) -> _stencils.StencilGroup:
        o = tempdir / f"{opname}.o"
        args = [
            f"--target={self.triple}",
            "-DPy_BUILD_CORE",
            "-D_DEBUG" if self.debug else "-DNDEBUG",
            f"-D_JIT_OPCODE={opname}",
            "-D_PyJIT_ACTIVE",
            "-D_Py_JIT",
            "-I.",
            f"-I{CPYTHON / 'Include'}",
            f"-I{CPYTHON / 'Include' / 'internal'}",
            f"-I{CPYTHON / 'Include' / 'internal' / 'mimalloc'}",
            f"-I{CPYTHON / 'Python'}",
            "-O3",
            "-c",
            "-fno-asynchronous-unwind-tables",
            "-fno-builtin",
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
            "-o",
            f"{o}",
            "-std=c11",
            f"{c}",
        ]
        await _llvm.run("clang", args, echo=self.verbose)
        return await self._parse(o)

    async def _build_stencils(self) -> dict[str, _stencils.StencilGroup]:
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

    def build(self, out: pathlib.Path, *, comment: str = "") -> None:
        """Build jit_stencils.h in the given directory."""
        digest = f"// {self._compute_digest(out)}\n"
        jit_stencils = out / "jit_stencils.h"
        if (
            not self.force
            and jit_stencils.exists()
            and jit_stencils.read_text().startswith(digest)
        ):
            return
        stencil_groups = asyncio.run(self._build_stencils())
        with jit_stencils.open("w") as file:
            file.write(digest)
            if comment:
                file.write(f"// {comment}\n\n")
            file.write("")
            for line in _writer.dump(stencil_groups):
                file.write(f"{line}\n")


class _COFF(
    _Target[_schema.COFFSection, _schema.COFFRelocation]
):  # pylint: disable = too-few-public-methods
    def _handle_section(
        self, section: _schema.COFFSection, group: _stencils.StencilGroup
    ) -> None:
        flags = {flag["Name"] for flag in section["Characteristics"]["Flags"]}
        if "SectionData" in section:
            section_data_bytes = section["SectionData"]["Bytes"]
        else:
            # Zeroed BSS data, seen with printf debugging calls:
            section_data_bytes = [0] * section["RawDataSize"]
        if "IMAGE_SCN_MEM_EXECUTE" in flags:
            value = _stencils.HoleValue.CODE
            stencil = group.code
        elif "IMAGE_SCN_MEM_READ" in flags:
            value = _stencils.HoleValue.DATA
            stencil = group.data
        else:
            return
        base = len(stencil.body)
        group.symbols[section["Number"]] = value, base
        stencil.body.extend(section_data_bytes)
        for wrapped_symbol in section["Symbols"]:
            symbol = wrapped_symbol["Symbol"]
            offset = base + symbol["Value"]
            name = symbol["Name"]
            name = name.removeprefix(self.prefix)
            group.symbols[name] = value, offset
        for wrapped_relocation in section["Relocations"]:
            relocation = wrapped_relocation["Relocation"]
            hole = self._handle_relocation(base, relocation, stencil.body)
            stencil.holes.append(hole)

    def _handle_relocation(
        self, base: int, relocation: _schema.COFFRelocation, raw: bytes
    ) -> _stencils.Hole:
        match relocation:
            case {
                "Offset": offset,
                "Symbol": s,
                "Type": {"Value": "IMAGE_REL_AMD64_ADDR64" as kind},
            }:
                offset += base
                s = s.removeprefix(self.prefix)
                value, symbol = _stencils.symbol_to_value(s)
                addend = int.from_bytes(raw[offset : offset + 8], "little")
            case {
                "Offset": offset,
                "Symbol": s,
                "Type": {"Value": "IMAGE_REL_I386_DIR32" as kind},
            }:
                offset += base
                s = s.removeprefix(self.prefix)
                value, symbol = _stencils.symbol_to_value(s)
                addend = int.from_bytes(raw[offset : offset + 4], "little")
            case _:
                raise NotImplementedError(relocation)
        return _stencils.Hole(offset, kind, value, symbol, addend)


class _ELF(
    _Target[_schema.ELFSection, _schema.ELFRelocation]
):  # pylint: disable = too-few-public-methods
    def _handle_section(
        self, section: _schema.ELFSection, group: _stencils.StencilGroup
    ) -> None:
        section_type = section["Type"]["Value"]
        flags = {flag["Name"] for flag in section["Flags"]["Flags"]}
        if section_type == "SHT_RELA":
            assert "SHF_INFO_LINK" in flags, flags
            assert not section["Symbols"]
            value, base = group.symbols[section["Info"]]
            if value is _stencils.HoleValue.CODE:
                stencil = group.code
            else:
                assert value is _stencils.HoleValue.DATA
                stencil = group.data
            for wrapped_relocation in section["Relocations"]:
                relocation = wrapped_relocation["Relocation"]
                hole = self._handle_relocation(base, relocation, stencil.body)
                stencil.holes.append(hole)
        elif section_type == "SHT_PROGBITS":
            if "SHF_ALLOC" not in flags:
                return
            if "SHF_EXECINSTR" in flags:
                value = _stencils.HoleValue.CODE
                stencil = group.code
            else:
                value = _stencils.HoleValue.DATA
                stencil = group.data
            group.symbols[section["Index"]] = value, len(stencil.body)
            for wrapped_symbol in section["Symbols"]:
                symbol = wrapped_symbol["Symbol"]
                offset = len(stencil.body) + symbol["Value"]
                name = symbol["Name"]["Value"]
                name = name.removeprefix(self.prefix)
                group.symbols[name] = value, offset
            stencil.body.extend(section["SectionData"]["Bytes"])
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
        self, base: int, relocation: _schema.ELFRelocation, raw: bytes
    ) -> _stencils.Hole:
        match relocation:
            case {
                "Addend": addend,
                "Offset": offset,
                "Symbol": {"Value": s},
                "Type": {"Value": kind},
            }:
                offset += base
                s = s.removeprefix(self.prefix)
                value, symbol = _stencils.symbol_to_value(s)
            case _:
                raise NotImplementedError(relocation)
        return _stencils.Hole(offset, kind, value, symbol, addend)


class _MachO(
    _Target[_schema.MachOSection, _schema.MachORelocation]
):  # pylint: disable = too-few-public-methods
    def _handle_section(
        self, section: _schema.MachOSection, group: _stencils.StencilGroup
    ) -> None:
        assert section["Address"] >= len(group.code.body)
        assert "SectionData" in section
        flags = {flag["Name"] for flag in section["Attributes"]["Flags"]}
        name = section["Name"]["Value"]
        name = name.removeprefix(self.prefix)
        if "Debug" in flags:
            return
        if "SomeInstructions" in flags:
            value = _stencils.HoleValue.CODE
            stencil = group.code
            start_address = 0
            group.symbols[name] = value, section["Address"] - start_address
        else:
            value = _stencils.HoleValue.DATA
            stencil = group.data
            start_address = len(group.code.body)
            group.symbols[name] = value, len(group.code.body)
        base = section["Address"] - start_address
        group.symbols[section["Index"]] = value, base
        stencil.body.extend(
            [0] * (section["Address"] - len(group.code.body) - len(group.data.body))
        )
        stencil.body.extend(section["SectionData"]["Bytes"])
        assert "Symbols" in section
        for wrapped_symbol in section["Symbols"]:
            symbol = wrapped_symbol["Symbol"]
            offset = symbol["Value"] - start_address
            name = symbol["Name"]["Value"]
            name = name.removeprefix(self.prefix)
            group.symbols[name] = value, offset
        assert "Relocations" in section
        for wrapped_relocation in section["Relocations"]:
            relocation = wrapped_relocation["Relocation"]
            hole = self._handle_relocation(base, relocation, stencil.body)
            stencil.holes.append(hole)

    def _handle_relocation(
        self, base: int, relocation: _schema.MachORelocation, raw: bytes
    ) -> _stencils.Hole:
        symbol: str | None
        match relocation:
            case {
                "Offset": offset,
                "Symbol": {"Value": s},
                "Type": {
                    "Value": "ARM64_RELOC_GOT_LOAD_PAGE21"
                    | "ARM64_RELOC_GOT_LOAD_PAGEOFF12" as kind
                },
            }:
                offset += base
                s = s.removeprefix(self.prefix)
                value, symbol = _stencils.HoleValue.GOT, s
                addend = 0
            case {
                "Offset": offset,
                "Section": {"Value": s},
                "Type": {"Value": kind},
            } | {
                "Offset": offset,
                "Symbol": {"Value": s},
                "Type": {"Value": kind},
            }:
                offset += base
                s = s.removeprefix(self.prefix)
                value, symbol = _stencils.symbol_to_value(s)
                addend = 0
            case _:
                raise NotImplementedError(relocation)
        return _stencils.Hole(offset, kind, value, symbol, addend)


def get_target(host: str) -> _COFF | _ELF | _MachO:
    """Build a _Target for the given host "triple" and options."""
    if re.fullmatch(r"aarch64-apple-darwin.*", host):
        return _MachO(host, alignment=8, prefix="_")
    if re.fullmatch(r"aarch64-.*-linux-gnu", host):
        return _ELF(host, alignment=8)
    if re.fullmatch(r"i686-pc-windows-msvc", host):
        return _COFF(host, prefix="_")
    if re.fullmatch(r"x86_64-apple-darwin.*", host):
        return _MachO(host, prefix="_")
    if re.fullmatch(r"x86_64-pc-windows-msvc", host):
        return _COFF(host)
    if re.fullmatch(r"x86_64-.*-linux-gnu", host):
        return _ELF(host)
    raise ValueError(host)
