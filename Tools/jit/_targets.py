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
import shlex

import _llvm
import _optimizers
import _schema
import _stencils
import _writer

try:
    import _asm_to_dasc
    import _dasc_writer
except ImportError:
    _asm_to_dasc = None  # type: ignore[assignment]
    _dasc_writer = None  # type: ignore[assignment]

if sys.version_info < (3, 11):
    raise RuntimeError("Building the JIT compiler requires Python 3.11 or newer!")

TOOLS_JIT_BUILD = pathlib.Path(__file__).resolve()
TOOLS_JIT = TOOLS_JIT_BUILD.parent
TOOLS = TOOLS_JIT.parent
CPYTHON = TOOLS.parent
EXTERNALS = CPYTHON / "externals"
PYTHON_EXECUTOR_CASES_C_H = CPYTHON / "Python" / "executor_cases.c.h"
TOOLS_JIT_TEMPLATE_C = TOOLS_JIT / "template.c"

ASYNCIO_RUNNER = asyncio.Runner()

_S = typing.TypeVar("_S", _schema.COFFSection, _schema.ELFSection, _schema.MachOSection)
_R = typing.TypeVar(
    "_R", _schema.COFFRelocation, _schema.ELFRelocation, _schema.MachORelocation
)


@dataclasses.dataclass
class _Target(typing.Generic[_S, _R]):
    triple: str
    condition: str
    _: dataclasses.KW_ONLY
    args: typing.Sequence[str] = ()
    optimizer: type[_optimizers.Optimizer] = _optimizers.Optimizer
    label_prefix: typing.ClassVar[str]
    symbol_prefix: typing.ClassVar[str]
    re_global: typing.ClassVar[re.Pattern[str]]
    stable: bool = False
    debug: bool = False
    verbose: bool = False
    cflags: str = ""
    llvm_version: str = _llvm._LLVM_VERSION
    known_symbols: dict[str, int] = dataclasses.field(default_factory=dict)
    pyconfig_dir: pathlib.Path = pathlib.Path.cwd().resolve()
    use_dynasm: bool = False
    _jit_fold_pass: typing.Optional[pathlib.Path] = dataclasses.field(
        default=None, init=False, repr=False
    )

    def _get_nop(self) -> bytes:
        if re.fullmatch(r"aarch64-.*", self.triple):
            nop = b"\x1f\x20\x03\xd5"
        elif re.fullmatch(r"x86_64-.*|i686.*", self.triple):
            nop = b"\x90"
        else:
            raise ValueError(f"NOP not defined for {self.triple}")
        return nop

    def _compute_digest(self) -> str:
        hasher = hashlib.sha256()
        hasher.update(self.triple.encode())
        hasher.update(self.debug.to_bytes())
        hasher.update(self.cflags.encode())
        # These dependencies are also reflected in _JITSources in regen.targets:
        hasher.update(PYTHON_EXECUTOR_CASES_C_H.read_bytes())
        hasher.update((self.pyconfig_dir / "pyconfig.h").read_bytes())
        for dirpath, _, filenames in sorted(os.walk(TOOLS_JIT)):
            # Exclude cache files from digest computation to ensure reproducible builds.
            if dirpath.endswith("__pycache__"):
                continue
            for filename in sorted(filenames):
                hasher.update(pathlib.Path(dirpath, filename).read_bytes())
        return hasher.hexdigest()

    async def _parse(self, path: pathlib.Path) -> _stencils.StencilGroup:
        group = _stencils.StencilGroup()
        args = ["--disassemble", "--reloc", f"{path}"]
        output = await _llvm.maybe_run(
            "llvm-objdump", args, echo=self.verbose, llvm_version=self.llvm_version
        )
        if output is not None:
            # Make sure that full paths don't leak out (for reproducibility):
            long, short = str(path), str(path.name)
            group.code.disassembly.extend(
                line.expandtabs().strip().replace(long, short)
                for line in output.splitlines()
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
        output = await _llvm.run(
            "llvm-readobj", args, echo=self.verbose, llvm_version=self.llvm_version
        )
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
        return group

    def _handle_section(self, section: _S, group: _stencils.StencilGroup) -> None:
        raise NotImplementedError(type(self))

    def _handle_relocation(
        self, base: int, relocation: _R, raw: bytearray
    ) -> _stencils.Hole:
        raise NotImplementedError(type(self))

    async def _compile(
        self, opname: str, c: pathlib.Path, tempdir: pathlib.Path
    ) -> _stencils.StencilGroup:
        s = tempdir / f"{opname}.s"
        o = tempdir / f"{opname}.o"
        common_args = [
            f"--target={self.triple}",
            "-DPy_BUILD_CORE_MODULE",
            "-D_DEBUG" if self.debug else "-DNDEBUG",
            f"-D_JIT_OPCODE={opname}",
            "-D_PyJIT_ACTIVE",
            "-D_Py_JIT",
            f"-I{self.pyconfig_dir}",
            f"-I{CPYTHON / 'Include'}",
            f"-I{CPYTHON / 'Include' / 'internal'}",
            f"-I{CPYTHON / 'Include' / 'internal' / 'mimalloc'}",
            f"-I{CPYTHON / 'Python'}",
            f"-I{CPYTHON / 'Tools' / 'jit'}",
            # -O2 and -O3 include some optimizations that make sense for
            # standalone functions, but not for snippets of code that are going
            # to be laid out end-to-end (like ours)... common examples include
            # passes like tail-duplication, or aligning jump targets with nops.
            # -Os is equivalent to -O2 with many of these problematic passes
            # disabled. Based on manual review, for *our* purposes it usually
            # generates better code than -O2 (and -O2 usually generates better
            # code than -O3). As a nice benefit, it uses less memory too:
            "-Os",
            # Shorten full absolute file paths in the generated code (like the
            # __FILE__ macro and assert failure messages) for reproducibility:
            f"-ffile-prefix-map={CPYTHON}=.",
            f"-ffile-prefix-map={tempdir}=.",
            # This debug info isn't necessary, and bloats out the JIT'ed code.
            # We *may* be able to re-enable this, process it, and JIT it for a
            # nicer debugging experience... but that needs a lot more research:
            "-fno-asynchronous-unwind-tables",
            # Don't call built-in functions that we can't find or patch:
            "-fno-builtin",
            # Don't call stack-smashing canaries that we can't find or patch:
            "-fno-stack-protector",
            "-std=c11",
            *self.args,
            # Allow user-provided CFLAGS to override any defaults
            *shlex.split(self.cflags),
        ]
        if self.use_dynasm and self._jit_fold_pass is not None:
            # DynASM pipeline: compile to LLVM IR, run the JIT fold pass to
            # replace patch-value computation trees with inline-asm markers,
            # then compile the folded IR to assembly.
            ll = tempdir / f"{opname}.ll"
            ll_folded = tempdir / f"{opname}.folded.ll"
            args_ll = [
                *common_args,
                "-emit-llvm",
                "-S",
                "-o", f"{ll}",
                f"{c}",
            ]
            await _llvm.run(
                "clang", args_ll, echo=self.verbose,
                llvm_version=self.llvm_version,
            )
            args_opt = [
                f"--load-pass-plugin={self._jit_fold_pass}",
                "-passes=jit-fold",
                f"{ll}",
                "-S", "-o", f"{ll_folded}",
            ]
            await _llvm.run(
                "opt", args_opt, echo=self.verbose,
                llvm_version=self.llvm_version,
            )
            args_s = [
                f"--target={self.triple}",
                "-Os",
                "-fno-asynchronous-unwind-tables",
                "-fno-builtin",
                "-fno-stack-protector",
                *self.args,
                *shlex.split(self.cflags),
                "-S",
                "-o", f"{s}",
                f"{ll_folded}",
                # DynASM requires Intel syntax for x86:
                "-mllvm", "--x86-asm-syntax=intel",
            ]
            await _llvm.run(
                "clang", args_s, echo=self.verbose,
                llvm_version=self.llvm_version,
            )
        else:
            args_s = [
                *common_args,
                "-S",
                "-o", f"{s}",
                f"{c}",
                # DynASM requires Intel syntax for x86:
                *(("-mllvm", "--x86-asm-syntax=intel") if self.use_dynasm else ()),
            ]
            await _llvm.run(
                "clang", args_s, echo=self.verbose,
                llvm_version=self.llvm_version,
            )
        self.optimizer(
            s,
            label_prefix=self.label_prefix,
            symbol_prefix=self.symbol_prefix,
            re_global=self.re_global,
        ).run()
        args_o = [f"--target={self.triple}", "-c", "-o", f"{o}", f"{s}"]
        if self.use_dynasm:
            args_o.insert(1, "-masm=intel")
        await _llvm.run(
            "clang", args_o, echo=self.verbose, llvm_version=self.llvm_version
        )
        group = await self._parse(o)
        if self.use_dynasm:
            group.assembly_text = s.read_text()
        return group

    async def _build_jit_fold_pass(self) -> None:
        """Build the LLVM JIT fold pass plugin if DynASM is enabled."""
        if not self.use_dynasm:
            return
        src = TOOLS_JIT / "jit_fold_pass.cpp"
        if not src.exists():
            return
        so = TOOLS_JIT / "jit_fold_pass.so"
        # Check if rebuild is needed.
        if so.exists() and so.stat().st_mtime > src.stat().st_mtime:
            self._jit_fold_pass = so
            return
        import subprocess
        # Find the compiler and llvm-config.
        clangxx = await _llvm.maybe_run(
            "clang++", ["--version"], echo=self.verbose,
            llvm_version=self.llvm_version,
        )
        if clangxx is None:
            print("Warning: clang++ not found, skipping JIT fold pass build")
            return
        # Resolve the actual clang++ path.
        clangxx_path = None
        for name in (f"clang++-{self.llvm_version}", "clang++"):
            result = subprocess.run(
                ["which", name], capture_output=True, text=True
            )
            if result.returncode == 0:
                clangxx_path = result.stdout.strip()
                break
        if clangxx_path is None:
            print("Warning: clang++ not found, skipping JIT fold pass build")
            return
        # Find llvm-config.
        llvm_config = None
        for name in (f"llvm-config-{self.llvm_version}", "llvm-config"):
            result = subprocess.run(
                [name, "--version"], capture_output=True, text=True
            )
            if result.returncode == 0 and result.stdout.strip().startswith(
                f"{self.llvm_version}."
            ):
                llvm_config = name
                break
        if llvm_config is None:
            print("Warning: llvm-config not found, skipping JIT fold pass build")
            return
        # Get compile and link flags.
        cxxflags = subprocess.run(
            [llvm_config, "--cxxflags"], capture_output=True, text=True
        ).stdout.strip()
        ldflags = subprocess.run(
            [llvm_config, "--ldflags"], capture_output=True, text=True
        ).stdout.strip()
        # Find the GCC installation directory for C++ stdlib headers.
        gcc_install_dir = ""
        for ver in ("15", "14", "13", "12"):
            candidate = pathlib.Path(f"/usr/lib/gcc/x86_64-linux-gnu/{ver}")
            headers = pathlib.Path(f"/usr/include/c++/{ver}/type_traits")
            if candidate.exists() and headers.exists():
                gcc_install_dir = f"--gcc-install-dir={candidate}"
                break
        args = [
            clangxx_path,
            "-shared", "-fPIC",
            *cxxflags.split(),
            *ldflags.split(),
            *(gcc_install_dir.split() if gcc_install_dir else []),
            "-o", f"{so}",
            f"{src}",
            f"-lLLVM-{self.llvm_version}",
        ]
        if self.verbose:
            import shlex as _shlex
            print(_shlex.join(args))
        result = subprocess.run(args, capture_output=True, text=True)
        if result.returncode != 0:
            print(f"Warning: Failed to build JIT fold pass:\n{result.stderr}")
            return
        self._jit_fold_pass = so

    # Stencils where oparg is provably constrained at the C level.
    # These __builtin_assume hints let LLVM eliminate dead branches
    # that compare oparg to constants.
    #
    # Evidence for each constraint:
    #   COPY_FREE_VARS: Only emitted when nfreevars > 0
    #                   (Python/flowgraph.c:make_cfg_from_code_object).
    #   BUILD_SLICE: oparg is always 2 or 3 (2-arg or 3-arg slice).
    #               (Python/bytecodes.c: inst(BUILD_SLICE))
    #   UNPACK_SEQUENCE_TUPLE: Unpacking requires at least 1 target.
    #   UNPACK_SEQUENCE_LIST: Same as UNPACK_SEQUENCE_TUPLE.
    #   INIT_CALL_PY_EXACT_ARGS (non-specialized): The replicate(5)
    #       creates _0 through _4 variants; the general variant
    #       only runs when oparg >= 5.
    _OPARG_ASSUMES: typing.ClassVar[dict[str, str]] = {
        "COPY_FREE_VARS":          "    __builtin_assume(_oparg > 0);",
        "BUILD_SLICE":             "    __builtin_assume(_oparg >= 2 && _oparg <= 3);",
        "INIT_CALL_PY_EXACT_ARGS": "    __builtin_assume(_oparg >= 5);",
    }

    @staticmethod
    def _oparg_assumes(opname: str) -> str:
        """Return __builtin_assume statements for a stencil's oparg."""
        # Strip leading underscore and register variant suffix
        # e.g. "_COPY_FREE_VARS_r11" -> "COPY_FREE_VARS"
        base = opname.lstrip("_")
        # Remove register variant suffix (e.g. "_r01", "_r11")
        base = re.sub(r"_r\d+$", "", base)
        return _Target._OPARG_ASSUMES.get(base, "")

    async def _build_stencils(self) -> dict[str, _stencils.StencilGroup]:
        # Build the LLVM JIT fold pass before compiling stencils.
        await self._build_jit_fold_pass()
        generated_cases = PYTHON_EXECUTOR_CASES_C_H.read_text()
        cases_and_opnames = sorted(
            re.findall(
                r"\n {8}(case (\w+): \{\n.*?\n {8}\})", generated_cases, flags=re.DOTALL
            )
        )
        tasks = []
        with tempfile.TemporaryDirectory() as tempdir:
            work = pathlib.Path(tempdir).resolve()
            async with asyncio.TaskGroup() as group:
                coro = self._compile("shim", TOOLS_JIT / "shim.c", work)
                tasks.append(group.create_task(coro, name="shim"))
                template = TOOLS_JIT_TEMPLATE_C.read_text()
                for case, opname in cases_and_opnames:
                    # Write out a copy of the template with *only* this case
                    # inserted. This is about twice as fast as #include'ing all
                    # of executor_cases.c.h each time we compile (since the C
                    # compiler wastes a bunch of time parsing the dead code for
                    # all of the other cases):
                    c = work / f"{opname}.c"
                    body = template.replace("CASE", case)
                    assumes = self._oparg_assumes(opname)
                    if assumes:
                        body = body.replace(
                            "PATCH_VALUE(uint16_t, _oparg, _JIT_OPARG)",
                            "PATCH_VALUE(uint16_t, _oparg, _JIT_OPARG)\n"
                            + assumes,
                        )
                    c.write_text(body)
                    coro = self._compile(opname, c, work)
                    tasks.append(group.create_task(coro, name=opname))
        stencil_groups = {task.get_name(): task.result() for task in tasks}
        for stencil_group in stencil_groups.values():
            stencil_group.convert_labels_to_relocations()
            stencil_group.process_relocations(self.known_symbols)
        return stencil_groups

    def _build_dasc(
        self,
        stencil_groups: dict[str, _stencils.StencilGroup],
        jit_stencils: pathlib.Path,
    ) -> str:
        """Convert stencils to DynASM and return the generated header content."""
        assert _asm_to_dasc is not None, "DynASM support requires _asm_to_dasc module"
        assert _dasc_writer is not None, "DynASM support requires _dasc_writer module"
        converted = {}
        shim_stencil = None
        for opname, group in stencil_groups.items():
            if not group.assembly_text:
                continue
            cs = _asm_to_dasc.convert_stencil(opname, group.assembly_text,
                                                is_shim=(opname == "shim"))
            if opname == "shim":
                shim_stencil = cs
            else:
                converted[opname] = cs
        _asm_to_dasc.print_peephole_stats()
        # Write .dasc next to the output header for easy debugging
        dasc_path = jit_stencils.with_suffix(".dasc")
        return _dasc_writer.dump_header(
            converted, shim_stencil, dasc_path=dasc_path
        )

    def build(
        self,
        *,
        comment: str = "",
        force: bool = False,
        jit_stencils: pathlib.Path,
    ) -> None:
        """Build jit_stencils.h in the given directory."""
        jit_stencils.parent.mkdir(parents=True, exist_ok=True)
        if not self.stable:
            warning = f"JIT support for {self.triple} is still experimental!"
            request = "Please report any issues you encounter.".center(len(warning))
            if self.llvm_version != _llvm._LLVM_VERSION:
                request = f"Warning! Building with an LLVM version other than {_llvm._LLVM_VERSION} is not supported."
            outline = "=" * len(warning)
            print("\n".join(["", outline, warning, request, outline, ""]))
        digest = f"// {self._compute_digest()}\n"
        if (
            not force
            and jit_stencils.exists()
            and jit_stencils.read_text().startswith(digest)
        ):
            return
        stencil_groups = ASYNCIO_RUNNER.run(self._build_stencils())
        jit_stencils_new = jit_stencils.parent / "jit_stencils.h.new"
        try:
            with jit_stencils_new.open("w") as file:
                file.write(digest)
                if comment:
                    file.write(f"// {comment}\n")
                file.write("\n")
                if self.use_dynasm:
                    content = self._build_dasc(stencil_groups, jit_stencils)
                    file.write(content)
                else:
                    for line in _writer.dump(stencil_groups, self.known_symbols):
                        file.write(f"{line}\n")
            try:
                jit_stencils_new.replace(jit_stencils)
            except FileNotFoundError:
                # another process probably already moved the file
                if not jit_stencils.is_file():
                    raise
        finally:
            jit_stencils_new.unlink(missing_ok=True)


class _COFF(
    _Target[_schema.COFFSection, _schema.COFFRelocation]
):  # pylint: disable = too-few-public-methods
    def _handle_section(
        self, section: _schema.COFFSection, group: _stencils.StencilGroup
    ) -> None:
        name = section["Name"]["Value"]
        if name == ".debug$S":
            # skip debug sections
            return
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
            name = name.removeprefix(self.symbol_prefix)
            if name not in group.symbols:
                group.symbols[name] = value, offset
        for wrapped_relocation in section["Relocations"]:
            relocation = wrapped_relocation["Relocation"]
            hole = self._handle_relocation(base, relocation, stencil.body)
            stencil.holes.append(hole)

    def _unwrap_dllimport(self, name: str) -> tuple[_stencils.HoleValue, str | None]:
        if name.startswith("__imp_"):
            name = name.removeprefix("__imp_")
            name = name.removeprefix(self.symbol_prefix)
            return _stencils.HoleValue.GOT, name
        name = name.removeprefix(self.symbol_prefix)
        return _stencils.symbol_to_value(name)

    def _handle_relocation(
        self, base: int, relocation: _schema.COFFRelocation, raw: bytearray
    ) -> _stencils.Hole:
        match relocation:
            case {
                "Offset": offset,
                "Symbol": s,
                "Type": {"Name": "IMAGE_REL_I386_DIR32" as kind},
            }:
                offset += base
                value, symbol = self._unwrap_dllimport(s)
                addend = int.from_bytes(raw[offset : offset + 4], "little")
            case {
                "Offset": offset,
                "Symbol": s,
                "Type": {
                    "Name": "IMAGE_REL_AMD64_REL32" | "IMAGE_REL_I386_REL32" as kind
                },
            }:
                offset += base
                value, symbol = self._unwrap_dllimport(s)
                addend = (
                    int.from_bytes(raw[offset : offset + 4], "little", signed=True) - 4
                )
            case {
                "Offset": offset,
                "Symbol": s,
                "Type": {
                    "Name": "IMAGE_REL_ARM64_BRANCH19"
                    | "IMAGE_REL_ARM64_BRANCH26"
                    | "IMAGE_REL_ARM64_PAGEBASE_REL21"
                    | "IMAGE_REL_ARM64_PAGEOFFSET_12A"
                    | "IMAGE_REL_ARM64_PAGEOFFSET_12L" as kind
                },
            }:
                offset += base
                value, symbol = self._unwrap_dllimport(s)
                addend = 0
            case _:
                raise NotImplementedError(relocation)
        return _stencils.Hole(offset, kind, value, symbol, addend)


class _COFF32(_COFF):
    # These mangle like Mach-O and other "older" formats:
    label_prefix = "L"
    symbol_prefix = "_"
    re_global = re.compile(r'\s*\.def\s+(?P<label>[\w."$?@]+);')


class _COFF64(_COFF):
    # These mangle like ELF and other "newer" formats:
    label_prefix = ".L"
    symbol_prefix = ""
    re_global = re.compile(r'\s*\.def\s+(?P<label>[\w."$?@]+);')


class _ELF(
    _Target[_schema.ELFSection, _schema.ELFRelocation]
):  # pylint: disable = too-few-public-methods
    label_prefix = ".L"
    symbol_prefix = ""
    re_global = re.compile(r'\s*\.globl\s+(?P<label>[\w."$?@]+)(\s+.*)?')

    def _handle_section(
        self, section: _schema.ELFSection, group: _stencils.StencilGroup
    ) -> None:
        section_type = section["Type"]["Name"]
        flags = {flag["Name"] for flag in section["Flags"]["Flags"]}
        if section_type == "SHT_RELA":
            assert "SHF_INFO_LINK" in flags, flags
            assert not section["Symbols"]
            maybe_symbol = group.symbols.get(section["Info"])
            if maybe_symbol is None:
                # These are relocations for a section we're not emitting. Skip:
                return
            value, base = maybe_symbol
            if value is _stencils.HoleValue.CODE:
                stencil = group.code
            elif value is _stencils.HoleValue.COLD_CODE:
                stencil = group.cold_code
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
                section_name = section.get("Name", {}).get("Name", "")
                if ".text.cold" in section_name:
                    value = _stencils.HoleValue.COLD_CODE
                    stencil = group.cold_code
                else:
                    value = _stencils.HoleValue.CODE
                    stencil = group.code
            else:
                value = _stencils.HoleValue.DATA
                stencil = group.data
            group.symbols[section["Index"]] = value, len(stencil.body)
            for wrapped_symbol in section["Symbols"]:
                symbol = wrapped_symbol["Symbol"]
                offset = len(stencil.body) + symbol["Value"]
                name = symbol["Name"]["Name"]
                name = name.removeprefix(self.symbol_prefix)
                group.symbols[name] = value, offset
            stencil.body.extend(section["SectionData"]["Bytes"])
            assert not section["Relocations"]
        else:
            assert section_type in {
                "SHT_GROUP",
                "SHT_LLVM_ADDRSIG",
                "SHT_NOTE",
                "SHT_NULL",
                "SHT_STRTAB",
                "SHT_SYMTAB",
            }, section_type

    def _handle_relocation(
        self, base: int, relocation: _schema.ELFRelocation, raw: bytearray
    ) -> _stencils.Hole:
        symbol: str | None
        match relocation:
            case {
                "Addend": addend,
                "Offset": offset,
                "Symbol": {"Name": s},
                "Type": {
                    "Name": "R_AARCH64_ADR_GOT_PAGE"
                    | "R_AARCH64_LD64_GOT_LO12_NC"
                    | "R_X86_64_GOTPCREL"
                    | "R_X86_64_GOTPCRELX"
                    | "R_X86_64_REX_GOTPCRELX" as kind
                },
            }:
                offset += base
                s = s.removeprefix(self.symbol_prefix)
                value, symbol = _stencils.HoleValue.GOT, s
            case {
                "Addend": addend,
                "Offset": offset,
                "Symbol": {"Name": s},
                "Type": {"Name": kind},
            }:
                offset += base
                s = s.removeprefix(self.symbol_prefix)
                value, symbol = _stencils.symbol_to_value(s)
            case _:
                raise NotImplementedError(relocation)
        return _stencils.Hole(offset, kind, value, symbol, addend)


class _MachO(
    _Target[_schema.MachOSection, _schema.MachORelocation]
):  # pylint: disable = too-few-public-methods
    label_prefix = "L"
    symbol_prefix = "_"
    re_global = re.compile(r'\s*\.globl\s+(?P<label>[\w."$?@]+)(\s+.*)?')

    def _handle_section(
        self, section: _schema.MachOSection, group: _stencils.StencilGroup
    ) -> None:
        assert section["Address"] >= len(group.code.body)
        assert "SectionData" in section
        flags = {flag["Name"] for flag in section["Attributes"]["Flags"]}
        name = section["Name"]["Value"]
        name = name.removeprefix(self.symbol_prefix)
        if "Debug" in flags:
            return
        if "PureInstructions" in flags:
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
            name = symbol["Name"]["Name"]
            name = name.removeprefix(self.symbol_prefix)
            group.symbols[name] = value, offset
        assert "Relocations" in section
        for wrapped_relocation in section["Relocations"]:
            relocation = wrapped_relocation["Relocation"]
            hole = self._handle_relocation(base, relocation, stencil.body)
            stencil.holes.append(hole)

    def _handle_relocation(
        self, base: int, relocation: _schema.MachORelocation, raw: bytearray
    ) -> _stencils.Hole:
        symbol: str | None
        match relocation:
            case {
                "Offset": offset,
                "Symbol": {"Name": s},
                "Type": {
                    "Name": "ARM64_RELOC_GOT_LOAD_PAGE21"
                    | "ARM64_RELOC_GOT_LOAD_PAGEOFF12" as kind
                },
            }:
                offset += base
                s = s.removeprefix(self.symbol_prefix)
                value, symbol = _stencils.HoleValue.GOT, s
                addend = 0
            case {
                "Offset": offset,
                "Symbol": {"Name": s},
                "Type": {"Name": "X86_64_RELOC_GOT" | "X86_64_RELOC_GOT_LOAD" as kind},
            }:
                offset += base
                s = s.removeprefix(self.symbol_prefix)
                value, symbol = _stencils.HoleValue.GOT, s
                addend = (
                    int.from_bytes(raw[offset : offset + 4], "little", signed=True) - 4
                )
            case {
                "Offset": offset,
                "Section": {"Name": s},
                "Type": {"Name": "X86_64_RELOC_SIGNED" as kind},
            } | {
                "Offset": offset,
                "Symbol": {"Name": s},
                "Type": {"Name": "X86_64_RELOC_BRANCH" | "X86_64_RELOC_SIGNED" as kind},
            }:
                offset += base
                s = s.removeprefix(self.symbol_prefix)
                value, symbol = _stencils.symbol_to_value(s)
                addend = (
                    int.from_bytes(raw[offset : offset + 4], "little", signed=True) - 4
                )
            case {
                "Offset": offset,
                "Section": {"Name": s},
                "Type": {"Name": kind},
            } | {
                "Offset": offset,
                "Symbol": {"Name": s},
                "Type": {"Name": kind},
            }:
                offset += base
                s = s.removeprefix(self.symbol_prefix)
                value, symbol = _stencils.symbol_to_value(s)
                addend = 0
            case _:
                raise NotImplementedError(relocation)
        return _stencils.Hole(offset, kind, value, symbol, addend)


def get_target(host: str) -> _COFF32 | _COFF64 | _ELF | _MachO:
    """Build a _Target for the given host "triple" and options."""
    optimizer: type[_optimizers.Optimizer]
    target: _COFF32 | _COFF64 | _ELF | _MachO
    if re.fullmatch(r"aarch64-apple-darwin.*", host):
        host = "aarch64-apple-darwin"
        condition = "defined(__aarch64__) && defined(__APPLE__)"
        optimizer = _optimizers.OptimizerAArch64
        target = _MachO(host, condition, optimizer=optimizer)
    elif re.fullmatch(r"aarch64-pc-windows-msvc", host):
        host = "aarch64-pc-windows-msvc"
        condition = "defined(_M_ARM64)"
        args = ["-fms-runtime-lib=dll"]
        optimizer = _optimizers.OptimizerAArch64
        target = _COFF64(host, condition, args=args, optimizer=optimizer)
    elif re.fullmatch(r"aarch64-.*-linux-gnu", host):
        host = "aarch64-unknown-linux-gnu"
        condition = "defined(__aarch64__) && defined(__linux__)"
        # -mno-outline-atomics: Keep intrinsics from being emitted.
        args = ["-fpic", "-mno-outline-atomics", "-fno-plt"]
        optimizer = _optimizers.OptimizerAArch64ELF
        target = _ELF(host, condition, args=args, optimizer=optimizer)
    elif re.fullmatch(r"i686-pc-windows-msvc", host):
        host = "i686-pc-windows-msvc"
        condition = "defined(_M_IX86)"
        # -Wno-ignored-attributes: __attribute__((preserve_none)) is not supported here.
        args = ["-DPy_NO_ENABLE_SHARED", "-Wno-ignored-attributes"]
        optimizer = _optimizers.OptimizerX86
        target = _COFF32(host, condition, args=args, optimizer=optimizer)
    elif re.fullmatch(r"x86_64-apple-darwin.*", host):
        host = "x86_64-apple-darwin"
        condition = "defined(__x86_64__) && defined(__APPLE__)"
        optimizer = _optimizers.OptimizerX86
        target = _MachO(host, condition, optimizer=optimizer)
    elif re.fullmatch(r"x86_64-pc-windows-msvc", host):
        host = "x86_64-pc-windows-msvc"
        condition = "defined(_M_X64)"
        args = ["-fms-runtime-lib=dll"]
        optimizer = _optimizers.OptimizerX86
        target = _COFF64(host, condition, args=args, optimizer=optimizer)
    elif re.fullmatch(r"x86_64-.*-linux-gnu", host):
        host = "x86_64-unknown-linux-gnu"
        condition = "defined(__x86_64__) && defined(__linux__)"
        args = [
            "-fno-pic",
            "-mcmodel=medium",
            "-mlarge-data-threshold=0",
            "-fno-plt",
            "-fno-omit-frame-pointer",
            "-mno-red-zone",
        ]
        optimizer = _optimizers.OptimizerX86ELF
        target = _ELF(host, condition, args=args, optimizer=optimizer, use_dynasm=True)
    else:
        raise ValueError(host)
    return target
