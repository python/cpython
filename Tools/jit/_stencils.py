"""Core data structures for compiled code templates."""

import dataclasses
import enum
import sys
import typing

import _schema


@enum.unique
class HoleValue(enum.Enum):
    """
    Different "base" values that can be patched into holes (usually combined with the
    address of a symbol and/or an addend).
    """

    # The base address of the machine code for the current uop (exposed as _JIT_ENTRY):
    CODE = enum.auto()
    # The base address of the read-only data for this uop:
    DATA = enum.auto()
    # The address of the current executor (exposed as _JIT_EXECUTOR):
    EXECUTOR = enum.auto()
    # The base address of the "global" offset table located in the read-only data.
    # Shouldn't be present in the final stencils, since these are all replaced with
    # equivalent DATA values:
    GOT = enum.auto()
    # The current uop's oparg (exposed as _JIT_OPARG):
    OPARG = enum.auto()
    # The current uop's operand0 on 64-bit platforms (exposed as _JIT_OPERAND0):
    OPERAND0 = enum.auto()
    # The current uop's operand0 on 32-bit platforms (exposed as _JIT_OPERAND0_HI/LO):
    OPERAND0_HI = enum.auto()
    OPERAND0_LO = enum.auto()
    # 16 and 32 bit versions of OPARG, OPERAND0 and OPERAND1
    OPARG_16 = enum.auto()
    OPERAND0_16 = enum.auto()
    OPERAND1_16 = enum.auto()
    OPERAND0_32 = enum.auto()
    OPERAND1_32 = enum.auto()
    # The current uop's operand1 on 64-bit platforms (exposed as _JIT_OPERAND1):
    OPERAND1 = enum.auto()
    # The current uop's operand1 on 32-bit platforms (exposed as _JIT_OPERAND1_HI/LO):
    OPERAND1_HI = enum.auto()
    OPERAND1_LO = enum.auto()
    # The current uop's target (exposed as _JIT_TARGET):
    TARGET = enum.auto()
    # The base address of the machine code for the jump target (exposed as _JIT_JUMP_TARGET):
    JUMP_TARGET = enum.auto()
    # The base address of the machine code for the error jump target (exposed as _JIT_ERROR_TARGET):
    ERROR_TARGET = enum.auto()
    # A hardcoded value of zero (used for symbol lookups):
    ZERO = enum.auto()


# Map relocation types to our JIT's patch functions. "r" suffixes indicate that
# the patch function is relative. "x" suffixes indicate that they are "relaxing"
# (see comments in jit.c for more info):
_PATCH_FUNCS = {
    # aarch64-apple-darwin:
    "ARM64_RELOC_BRANCH26": "patch_aarch64_26r",
    "ARM64_RELOC_GOT_LOAD_PAGE21": "patch_aarch64_21rx",
    "ARM64_RELOC_GOT_LOAD_PAGEOFF12": "patch_aarch64_12x",
    "ARM64_RELOC_PAGE21": "patch_aarch64_21r",
    "ARM64_RELOC_PAGEOFF12": "patch_aarch64_12",
    "ARM64_RELOC_UNSIGNED": "patch_64",
    "CUSTOM_AARCH64_BRANCH19": "patch_aarch64_19r",
    "CUSTOM_AARCH64_CONST16a": "patch_aarch64_16a",
    "CUSTOM_AARCH64_CONST16b": "patch_aarch64_16b",
    # x86_64-pc-windows-msvc:
    "IMAGE_REL_AMD64_REL32": "patch_x86_64_32rx",
    # aarch64-pc-windows-msvc:
    "IMAGE_REL_ARM64_BRANCH19": "patch_aarch64_19r",
    "IMAGE_REL_ARM64_BRANCH26": "patch_aarch64_26r",
    "IMAGE_REL_ARM64_PAGEBASE_REL21": "patch_aarch64_21rx",
    "IMAGE_REL_ARM64_PAGEOFFSET_12A": "patch_aarch64_12",
    "IMAGE_REL_ARM64_PAGEOFFSET_12L": "patch_aarch64_12x",
    # i686-pc-windows-msvc:
    "IMAGE_REL_I386_DIR32": "patch_32",
    "IMAGE_REL_I386_REL32": "patch_x86_64_32rx",
    # aarch64-unknown-linux-gnu:
    "R_AARCH64_ABS64": "patch_64",
    "R_AARCH64_ADD_ABS_LO12_NC": "patch_aarch64_12",
    "R_AARCH64_ADR_GOT_PAGE": "patch_aarch64_21rx",
    "R_AARCH64_ADR_PREL_PG_HI21": "patch_aarch64_21r",
    "R_AARCH64_CALL26": "patch_aarch64_26r",
    "R_AARCH64_CONDBR19": "patch_aarch64_19r",
    "R_AARCH64_JUMP26": "patch_aarch64_26r",
    "R_AARCH64_LD64_GOT_LO12_NC": "patch_aarch64_12x",
    "R_AARCH64_MOVW_UABS_G0_NC": "patch_aarch64_16a",
    "R_AARCH64_MOVW_UABS_G1_NC": "patch_aarch64_16b",
    "R_AARCH64_MOVW_UABS_G2_NC": "patch_aarch64_16c",
    "R_AARCH64_MOVW_UABS_G3": "patch_aarch64_16d",
    # x86_64-unknown-linux-gnu:
    "R_X86_64_64": "patch_64",
    "R_X86_64_GOTPCRELX": "patch_x86_64_32rx",
    "R_X86_64_PLT32": "patch_32r",
    "R_X86_64_REX_GOTPCRELX": "patch_x86_64_32rx",
    # x86_64-apple-darwin:
    "X86_64_RELOC_BRANCH": "patch_32r",
    "X86_64_RELOC_GOT": "patch_x86_64_32rx",
    "X86_64_RELOC_GOT_LOAD": "patch_x86_64_32rx",
    "X86_64_RELOC_SIGNED": "patch_32r",
    "X86_64_RELOC_UNSIGNED": "patch_64",
}

# Translate HoleValues to C expressions:
_HOLE_EXPRS = {
    HoleValue.CODE: "(uintptr_t)code",
    HoleValue.DATA: "(uintptr_t)data",
    HoleValue.EXECUTOR: "(uintptr_t)executor",
    HoleValue.GOT: "",
    # These should all have been turned into DATA values by process_relocations:
    HoleValue.OPARG: "instruction->oparg",
    HoleValue.OPARG_16: "instruction->oparg",
    HoleValue.OPERAND0: "instruction->operand0",
    HoleValue.OPERAND0_16: "instruction->operand0",
    HoleValue.OPERAND0_32: "instruction->operand0",
    HoleValue.OPERAND0_HI: "(instruction->operand0 >> 32)",
    HoleValue.OPERAND0_LO: "(instruction->operand0 & UINT32_MAX)",
    HoleValue.OPERAND1: "instruction->operand1",
    HoleValue.OPERAND1_16: "instruction->operand1",
    HoleValue.OPERAND1_32: "instruction->operand1",
    HoleValue.OPERAND1_HI: "(instruction->operand1 >> 32)",
    HoleValue.OPERAND1_LO: "(instruction->operand1 & UINT32_MAX)",
    HoleValue.TARGET: "instruction->target",
    HoleValue.JUMP_TARGET: "state->instruction_starts[instruction->jump_target]",
    HoleValue.ERROR_TARGET: "state->instruction_starts[instruction->error_target]",
    HoleValue.ZERO: "",
}

_AARCH64_GOT_RELOCATIONS = {
    "R_AARCH64_ADR_GOT_PAGE",
    "R_AARCH64_LD64_GOT_LO12_NC",
    "ARM64_RELOC_GOT_LOAD_PAGE21",
    "ARM64_RELOC_GOT_LOAD_PAGEOFF12",
    "IMAGE_REL_ARM64_PAGEBASE_REL21",
    "IMAGE_REL_ARM64_PAGEOFFSET_12L",
    "IMAGE_REL_ARM64_PAGEOFFSET_12A",
}

_X86_GOT_RELOCATIONS = {
    "R_X86_64_GOTPCRELX",
    "R_X86_64_REX_GOTPCRELX",
    "X86_64_RELOC_GOT",
    "X86_64_RELOC_GOT_LOAD",
    "IMAGE_REL_AMD64_REL32",
}


@dataclasses.dataclass
class Hole:
    """
    A "hole" in the stencil to be patched with a computed runtime value.

    Analogous to relocation records in an object file.
    """

    offset: int
    kind: _schema.HoleKind
    # Patch with this base value:
    value: HoleValue
    # ...plus the address of this symbol:
    symbol: str | None
    # ...plus this addend:
    addend: int
    need_state: bool = False
    custom_location: str = ""
    custom_value: str = ""
    func: str = dataclasses.field(init=False)
    # Convenience method:
    replace = dataclasses.replace

    def __post_init__(self) -> None:
        self.func = _PATCH_FUNCS[self.kind]

    def fold(self, other: typing.Self, body: bytearray) -> typing.Self | None:
        """Combine two holes into a single hole, if possible."""
        instruction_a = int.from_bytes(
            body[self.offset : self.offset + 4], byteorder=sys.byteorder
        )
        instruction_b = int.from_bytes(
            body[other.offset : other.offset + 4], byteorder=sys.byteorder
        )
        reg_a = instruction_a & 0b11111
        reg_b1 = instruction_b & 0b11111
        reg_b2 = (instruction_b >> 5) & 0b11111

        if (
            self.offset + 4 == other.offset
            and self.value == other.value
            and self.symbol == other.symbol
            and self.addend == other.addend
            and self.func == "patch_aarch64_21rx"
            and other.func == "patch_aarch64_12x"
            and reg_a == reg_b1 == reg_b2
        ):
            # These can *only* be properly relaxed when they appear together and
            # patch the same value:
            folded = self.replace()
            folded.func = "patch_aarch64_33rx"
            return folded
        return None

    def as_c(self, where: str) -> str:
        """Dump this hole as a call to a patch_* function."""
        if self.custom_location:
            location = self.custom_location
        else:
            location = f"{where} + {self.offset:#x}"
        if self.custom_value:
            value = self.custom_value
        else:
            value = _HOLE_EXPRS[self.value]
            if self.symbol:
                if value:
                    value += " + "
                if self.symbol.startswith("CONST"):
                    value += f"instruction->{self.symbol[10:].lower()}"
                else:
                    value += f"(uintptr_t)&{self.symbol}"
            if _signed(self.addend) or not value:
                if value:
                    value += " + "
                value += f"{_signed(self.addend):#x}"
        if self.need_state:
            return f"{self.func}({location}, {value}, state);"
        return f"{self.func}({location}, {value});"


@dataclasses.dataclass
class Stencil:
    """
    A contiguous block of machine code or data to be copied-and-patched.

    Analogous to a section or segment in an object file.
    """

    body: bytearray = dataclasses.field(default_factory=bytearray, init=False)
    holes: list[Hole] = dataclasses.field(default_factory=list, init=False)
    disassembly: list[str] = dataclasses.field(default_factory=list, init=False)

    def pad(self, alignment: int) -> None:
        """Pad the stencil to the given alignment."""
        offset = len(self.body)
        padding = -offset % alignment
        if padding:
            self.disassembly.append(f"{offset:x}: {' '.join(['00'] * padding)}")
        self.body.extend([0] * padding)


@dataclasses.dataclass
class StencilGroup:
    """
    Code and data corresponding to a given micro-opcode.

    Analogous to an entire object file.
    """

    code: Stencil = dataclasses.field(default_factory=Stencil, init=False)
    data: Stencil = dataclasses.field(default_factory=Stencil, init=False)
    symbols: dict[int | str, tuple[HoleValue, int]] = dataclasses.field(
        default_factory=dict, init=False
    )
    _jit_symbol_table: dict[str, int] = dataclasses.field(
        default_factory=dict, init=False
    )
    _trampolines: set[int] = dataclasses.field(default_factory=set, init=False)
    _got_entries: set[int] = dataclasses.field(default_factory=set, init=False)

    def convert_labels_to_relocations(self) -> None:
        for name, hole_plus in self.symbols.items():
            if isinstance(name, str) and "_JIT_RELOCATION_" in name:
                _, offset = hole_plus
                reloc, target, _ = name.split("_JIT_RELOCATION_")
                value, symbol = symbol_to_value(target)
                hole = Hole(
                    int(offset), typing.cast(_schema.HoleKind, reloc), value, symbol, 0
                )
                self.code.holes.append(hole)

    def process_relocations(self, known_symbols: dict[str, int]) -> None:
        """Fix up all GOT and internal relocations for this stencil group."""
        for hole in self.code.holes.copy():
            if (
                hole.kind
                in {"R_AARCH64_CALL26", "R_AARCH64_JUMP26", "ARM64_RELOC_BRANCH26"}
                and hole.value is HoleValue.ZERO
                and hole.symbol not in self.symbols
            ):
                hole.func = "patch_aarch64_trampoline"
                hole.need_state = True
                assert hole.symbol is not None
                if hole.symbol in known_symbols:
                    ordinal = known_symbols[hole.symbol]
                else:
                    ordinal = len(known_symbols)
                    known_symbols[hole.symbol] = ordinal
                self._trampolines.add(ordinal)
                hole.addend = ordinal
                hole.symbol = None
            # x86_64 Darwin trampolines for external symbols
            elif (
                hole.kind == "X86_64_RELOC_BRANCH"
                and hole.value is HoleValue.ZERO
                and hole.symbol not in self.symbols
            ):
                hole.func = "patch_x86_64_trampoline"
                hole.need_state = True
                assert hole.symbol is not None
                if hole.symbol in known_symbols:
                    ordinal = known_symbols[hole.symbol]
                else:
                    ordinal = len(known_symbols)
                    known_symbols[hole.symbol] = ordinal
                self._trampolines.add(ordinal)
                hole.addend = ordinal
                hole.symbol = None
            elif (
                hole.kind in _AARCH64_GOT_RELOCATIONS | _X86_GOT_RELOCATIONS
                and hole.symbol
                and "_JIT_" not in hole.symbol
                and hole.value is HoleValue.GOT
            ):
                if hole.symbol in known_symbols:
                    ordinal = known_symbols[hole.symbol]
                else:
                    ordinal = len(known_symbols)
                    known_symbols[hole.symbol] = ordinal
                self._got_entries.add(ordinal)
        self.data.pad(8)
        for stencil in [self.code, self.data]:
            for hole in stencil.holes:
                if hole.value is HoleValue.GOT:
                    assert hole.symbol is not None
                    if "_JIT_" in hole.symbol:
                        # Relocations for local symbols
                        hole.value = HoleValue.DATA
                        hole.addend += self._jit_symbol_table_lookup(hole.symbol)
                    else:
                        _ordinal = known_symbols[hole.symbol]
                        _custom_value = f"got_symbol_address({_ordinal:#x}, state)"
                        if hole.kind in _X86_GOT_RELOCATIONS:
                            # When patching on x86, subtract the addend -4
                            # that is used to compute the 32 bit RIP relative
                            # displacement to the GOT entry
                            _custom_value = (
                                f"got_symbol_address({_ordinal:#x}, state) - 4"
                            )
                        hole.addend = _ordinal
                        hole.custom_value = _custom_value
                    hole.symbol = None
                elif hole.symbol in self.symbols:
                    hole.value, addend = self.symbols[hole.symbol]
                    hole.addend += addend
                    hole.symbol = None
                elif (
                    hole.kind in {"IMAGE_REL_AMD64_REL32"}
                    and hole.value is HoleValue.ZERO
                ):
                    raise ValueError(
                        f"Add PyAPI_FUNC(...) or PyAPI_DATA(...) to declaration of {hole.symbol}!"
                    )
        self._emit_jit_symbol_table()
        self._emit_global_offset_table()
        self.code.holes.sort(key=lambda hole: hole.offset)
        self.data.holes.sort(key=lambda hole: hole.offset)

    def _jit_symbol_table_lookup(self, symbol: str) -> int:
        return len(self.data.body) + self._jit_symbol_table.setdefault(
            symbol, 8 * len(self._jit_symbol_table)
        )

    def _emit_jit_symbol_table(self) -> None:
        got = len(self.data.body)
        for s, offset in self._jit_symbol_table.items():
            if s in self.symbols:
                value, addend = self.symbols[s]
                symbol = None
            else:
                value, symbol = symbol_to_value(s)
                addend = 0
            self.data.holes.append(
                Hole(got + offset, "R_X86_64_64", value, symbol, addend)
            )
            value_part = value.name if value is not HoleValue.ZERO else ""
            if value_part and not symbol and not addend:
                addend_part = ""
            else:
                signed = "+" if symbol is not None else ""
                addend_part = f"&{symbol}" if symbol else ""
                addend_part += f"{_signed(addend):{signed}#x}"
                if value_part:
                    value_part += "+"
            self.data.disassembly.append(
                f"{len(self.data.body):x}: {value_part}{addend_part}"
            )
            self.data.body.extend([0] * 8)

    def _emit_global_offset_table(self) -> None:
        for hole in self.code.holes:
            if hole.value is HoleValue.GOT:
                _got_hole = Hole(0, "R_X86_64_64", hole.value, None, hole.addend)
                _got_hole.func = "patch_got_symbol"
                _got_hole.custom_location = "state"
                if _got_hole not in self.data.holes:
                    self.data.holes.append(_got_hole)

    def _get_symbol_mask(self, ordinals: set[int]) -> str:
        bitmask: int = 0
        symbol_mask: list[str] = []
        for ordinal in ordinals:
            bitmask |= 1 << ordinal
        while bitmask:
            word = bitmask & ((1 << 32) - 1)
            symbol_mask.append(f"{word:#04x}")
            bitmask >>= 32
        return "{" + (", ".join(symbol_mask) or "0") + "}"

    def _get_trampoline_mask(self) -> str:
        return self._get_symbol_mask(self._trampolines)

    def _get_got_mask(self) -> str:
        return self._get_symbol_mask(self._got_entries)

    def as_c(self, opname: str) -> str:
        """Dump this hole as a StencilGroup initializer."""
        return f"{{emit_{opname}, {len(self.code.body)}, {len(self.data.body)}, {self._get_trampoline_mask()}, {self._get_got_mask()}}}"


def symbol_to_value(symbol: str) -> tuple[HoleValue, str | None]:
    """
    Convert a symbol name to a HoleValue and a symbol name.

    Some symbols (starting with "_JIT_") are special and are converted to their
    own HoleValues.
    """
    if symbol.startswith("_JIT_"):
        try:
            return HoleValue[symbol.removeprefix("_JIT_")], None
        except KeyError:
            pass
    return HoleValue.ZERO, symbol


def _signed(value: int) -> int:
    value %= 1 << 64
    if value & (1 << 63):
        value -= 1 << 64
    return value
