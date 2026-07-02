"""Utilities for deriving JIT unwind information from DWARF CFI."""

import dataclasses
import pathlib
import re
import typing

_LLVMRun = typing.Callable[..., typing.Awaitable[str]]


@dataclasses.dataclass(frozen=True)
class UnwindInfo:
    code_alignment_factor: int
    data_alignment_factor: int
    return_address_register: int
    cfa_register: int
    cfa_offset: int
    frame_pointer_register: int
    frame_pointer_offset: int
    return_address_offset: int


@dataclasses.dataclass(frozen=True)
class ELFUnwindConfig:
    frame_pointer: str
    return_address: str
    register_numbers: typing.Mapping[str, int]
    call_instruction_prefixes: tuple[str, ...]

    def is_call_instruction(self, instruction: str) -> bool:
        return instruction.startswith(self.call_instruction_prefixes)


@dataclasses.dataclass(frozen=True)
class _UnwindRow:
    pc: int
    cfa_register: str
    cfa_offset: int
    saved_registers: dict[str, int]


class ELFUnwindInfo:
    def __init__(
        self,
        target_name: str,
        *,
        config: ELFUnwindConfig,
        verbose: bool = False,
        llvm_version: str,
        llvm_tools_install_dir: str | None = None,
        llvm_run: _LLVMRun,
    ) -> None:
        self.target_name = target_name
        self.config = config
        self.verbose = verbose
        self.llvm_version = llvm_version
        self.llvm_tools_install_dir = llvm_tools_install_dir
        self.llvm_run = llvm_run

    @staticmethod
    def _parse_dwarfdump_int(
        dump: str, field: str, *, required: bool = True
    ) -> int | None:
        match = re.search(rf"^\s*{field}:\s+(-?\d+)$", dump, re.MULTILINE)
        if match is None:
            if required:
                raise ValueError(f"missing {field} in llvm-dwarfdump output")
            return None
        return int(match.group(1))

    @staticmethod
    def _parse_dwarfdump_rows(dump: str) -> list[_UnwindRow]:
        row_pattern = re.compile(
            r"^\s*0x(?P<pc>[0-9a-f]+):\s+"
            r"CFA=(?P<cfa_register>[A-Z][A-Z0-9]*)"
            r"(?P<cfa_offset>[+-]\d+)?"
            r"(?::\s*(?P<saved>.*))?$"
        )
        saved_pattern = re.compile(
            r"(?P<register>[A-Z][A-Z0-9]*)=\[CFA(?P<offset>[+-]\d+)?\]"
        )
        rows = []
        for line in dump.splitlines():
            row_match = row_pattern.match(line)
            if row_match is None:
                continue
            saved_registers = {}
            saved = row_match["saved"]
            if saved:
                for saved_match in saved_pattern.finditer(saved):
                    offset = saved_match["offset"]
                    saved_registers[saved_match["register"]] = (
                        int(offset) if offset is not None else 0
                    )
            cfa_offset = row_match["cfa_offset"]
            rows.append(
                _UnwindRow(
                    pc=int(row_match["pc"], 16),
                    cfa_register=row_match["cfa_register"],
                    cfa_offset=int(cfa_offset) if cfa_offset is not None else 0,
                    saved_registers=saved_registers,
                )
            )
        if not rows:
            raise ValueError("missing interpreted CFI rows in llvm-dwarfdump output")
        return rows

    @staticmethod
    def _parse_objdump_instructions(dump: str) -> list[tuple[int, str]]:
        instructions = []
        for line in dump.splitlines():
            match = re.match(
                r"^\s*(?P<pc>[0-9a-f]+):\s+"
                r"(?:(?:[0-9a-f]{2}|[0-9a-f]{8})\s+)+"
                r"(?P<instruction>.+)$",
                line,
            )
            if match:
                instructions.append(
                    (
                        int(match["pc"], 16),
                        re.sub(r"\s+", " ", match["instruction"].strip()),
                    )
                )
        if not instructions:
            raise ValueError("missing instructions in llvm-objdump output")
        return instructions

    def _reg_number(self, register: str) -> int:
        try:
            return self.config.register_numbers[register]
        except KeyError as exc:
            raise ValueError(
                f"unsupported register {register!r} in llvm-dwarfdump output"
            ) from exc

    @staticmethod
    def _encoded_cfa_offset(byte_offset: int, data_alignment_factor: int) -> int:
        if data_alignment_factor == 0:
            raise ValueError("DWARF data alignment factor must not be zero")
        if byte_offset % data_alignment_factor:
            raise ValueError(
                f"offset {byte_offset} is not a multiple of "
                f"data alignment factor {data_alignment_factor}"
            )
        return byte_offset // data_alignment_factor

    async def _read_objdump(self, output: pathlib.Path) -> str:
        return await self.llvm_run(
            "llvm-objdump",
            ["-d", f"{output}"],
            echo=self.verbose,
            llvm_version=self.llvm_version,
            llvm_tools_install_dir=self.llvm_tools_install_dir,
        )

    async def _read_eh_frame(self, output: pathlib.Path) -> str:
        return await self.llvm_run(
            "llvm-dwarfdump",
            ["--eh-frame", f"{output}"],
            echo=self.verbose,
            llvm_version=self.llvm_version,
            llvm_tools_install_dir=self.llvm_tools_install_dir,
        )

    def _executor_call_pc(self, disassembly: str) -> int:
        calls = [
            pc
            for pc, instruction in self._parse_objdump_instructions(disassembly)
            if self.config.is_call_instruction(instruction)
        ]
        if len(calls) != 1:
            raise ValueError(
                f"{self.target_name} JIT shim should contain exactly one executor call"
            )
        call_pc = calls[0]
        return call_pc

    def _active_row(self, eh_frame: str, call_pc: int) -> _UnwindRow:
        rows = self._parse_dwarfdump_rows(eh_frame)
        active_rows = [row for row in rows if row.pc <= call_pc]
        if not active_rows:
            raise ValueError(
                f"{self.target_name} JIT shim has no CFI row for executor call "
                f"at 0x{call_pc:x}"
            )
        return max(active_rows, key=lambda row: row.pc)

    def _check_saved_registers(self, row: _UnwindRow) -> None:
        if (
            self.config.frame_pointer not in row.saved_registers
            or self.config.return_address not in row.saved_registers
        ):
            raise ValueError(
                f"{self.target_name} JIT shim CFI row at 0x{row.pc:x} "
                f"does not save {self.config.frame_pointer} and "
                f"{self.config.return_address}"
            )

    def _build_unwind_info(self, eh_frame: str, active_row: _UnwindRow) -> UnwindInfo:
        code_alignment_factor = self._parse_dwarfdump_int(
            eh_frame, "Code alignment factor"
        )
        data_alignment_factor = self._parse_dwarfdump_int(
            eh_frame, "Data alignment factor"
        )
        return_address_register = self._parse_dwarfdump_int(
            eh_frame, "Return address column"
        )
        assert code_alignment_factor is not None
        assert data_alignment_factor is not None
        assert return_address_register is not None
        return UnwindInfo(
            code_alignment_factor=code_alignment_factor,
            data_alignment_factor=data_alignment_factor,
            return_address_register=return_address_register,
            cfa_register=self._reg_number(active_row.cfa_register),
            cfa_offset=active_row.cfa_offset,
            frame_pointer_register=self._reg_number(self.config.frame_pointer),
            frame_pointer_offset=self._encoded_cfa_offset(
                active_row.saved_registers[self.config.frame_pointer],
                data_alignment_factor,
            ),
            return_address_offset=self._encoded_cfa_offset(
                active_row.saved_registers[self.config.return_address],
                data_alignment_factor,
            ),
        )

    async def extract(self, output: pathlib.Path) -> UnwindInfo:
        disassembly = await self._read_objdump(output)
        call_pc = self._executor_call_pc(disassembly)
        eh_frame = await self._read_eh_frame(output)
        active_row = self._active_row(eh_frame, call_pc)
        self._check_saved_registers(active_row)
        return self._build_unwind_info(eh_frame, active_row)
