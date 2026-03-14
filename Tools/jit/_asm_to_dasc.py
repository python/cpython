"""Convert Intel-syntax x86-64 assembly (from Clang) to DynASM .dasc format.

This module transforms the optimized .s files produced by Clang (Intel syntax,
medium code model, -fno-pic -fno-plt) into DynASM directives suitable for the
DynASM Lua preprocessor (dynasm.lua).

All labels (uop entry points, internal branch targets, JIT jump/error targets)
use DynASM PC labels (=>N), which are dynamically allocated.  The label
numbering scheme is:

    [0 .. trace_len-1]           : uop entry point labels
    [trace_len .. trace_len+K-1] : internal stencil labels (allocated per-emit)

External symbol references (function pointers, type addresses) use
``emit_call_ext()`` for direct calls and ``emit_mov_imm()`` for address loads,
both of which generate optimal encodings at JIT compile time.

JIT Register Roles
~~~~~~~~~~~~~~~~~~

The preserve_none calling convention assigns fixed register roles (see
REG_FRAME, REG_STACK_PTR, REG_TSTATE, REG_EXECUTOR constants below).
Frame struct offsets (FRAME_IP_OFFSET, FRAME_STACKPOINTER_OFFSET) are
also defined as constants to avoid hardcoded magic numbers.

Peephole Optimization
~~~~~~~~~~~~~~~~~~~~~

After converting each stencil to DynASM assembly, a multi-pass peephole
optimizer folds emit_mov_imm sequences with subsequent instructions.
Since emit_mov_imm values are C expressions evaluated at JIT compile time,
folding allows moving work from runtime to compile time.

Two categories of patterns:

  1. **emit_mov_imm chain patterns** (Patterns 1-15): Start from an
      emit_mov_imm call and attempt to fold the loaded value into subsequent
      instructions (truncation, arithmetic, branch elimination, memory
      indexing, ALU folding, etc.).  Handled by ``_fold_mov_imm()``.

  2. **Standalone patterns** (SP0-SP3): Independent patterns that operate
      on raw DynASM assembly lines:
        SP0 — Preserve flags across immediate loads
        SP1 — Store-reload elimination (hot jcc fallthrough)
        SP2 — Cold-path reload insertion (for __del__ safety)
        SP3 — Inverted store-reload deferral (hot jcc jump-to-merge)
     Registered in ``_STANDALONE_PATTERNS``.

Use ``--peephole-stats`` in ``build.py`` to see how often each fires.

Cross-Stencil Optimizations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Some optimizations span stencil boundaries and are handled at JIT compile
time (in jit.c) rather than at build time in this module:

  - **Frame merging**: Consecutive stencils with matching frame sizes
    can elide the epilogue/prologue pair.  Managed by ``emit_trace()``
    in jit.c.

  - **SET_IP delta encoding**: When consecutive SET_IP values are close,
    emit ``add qword [frame+56], delta`` instead of a full mov.
"""

from __future__ import annotations

import dataclasses
import enum
import re
import typing


# ── Register name mapping ───────────────────────────────────────────────
# REX-prefix byte registers that DynASM doesn't natively understand.
# We teach DynASM these names via .define directives in the .dasc header
# (see _dasc_writer.py), so we keep them as-is in the assembly output
# for readability — no Rb(N) substitution needed.
_REX_BYTE_REGS = frozenset({
    "spl", "bpl", "sil", "dil",
    "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b",
})

# Mapping from 64-bit register name → human-readable register index
# constant name (used for emit_mov_imm calls).  Prefixed with JREG_
# to avoid collisions with system headers (e.g. ucontext.h REG_R8).
_REG_IDX_NAME: dict[str, str] = {
    "rax": "JREG_RAX", "rcx": "JREG_RCX", "rdx": "JREG_RDX", "rbx": "JREG_RBX",
    "rsp": "JREG_RSP", "rbp": "JREG_RBP", "rsi": "JREG_RSI", "rdi": "JREG_RDI",
    "r8": "JREG_R8", "r9": "JREG_R9", "r10": "JREG_R10", "r11": "JREG_R11",
    "r12": "JREG_R12", "r13": "JREG_R13", "r14": "JREG_R14", "r15": "JREG_R15",
}


# ── _JIT_* symbol → C expression ───────────────────────────────────────
_JIT_SYMBOL_EXPR: dict[str, str] = {
    # The stencil template uses PATCH_VALUE(TYPE, NAME, ALIAS) which
    # expands to ``TYPE NAME = (TYPE)(uintptr_t)&ALIAS;``.  The compiler
    # generates ``movabs REG, offset _JIT_*`` to load the symbol's
    # address directly into a register — there is NO dereference.  The
    # original stencil JIT patches the movabs immediate with the VALUE
    # itself (not a pointer), so here we emit the value too.
    "_JIT_OPERAND0": "instruction->operand0",
    "_JIT_OPERAND1": "instruction->operand1",
    "_JIT_OPARG": "instruction->oparg",
    "_JIT_OPARG_16": "instruction->oparg",
    "_JIT_OPERAND0_16": "instruction->operand0",
    "_JIT_OPERAND0_32": "instruction->operand0",
    "_JIT_OPERAND1_16": "instruction->operand1",
    "_JIT_OPERAND1_32": "instruction->operand1",
    "_JIT_TARGET": "instruction->target",
}

# Map 64-bit register name → 32-bit register name.
_REG64_TO_REG32: dict[str, str] = {
    "rax": "eax", "rbx": "ebx", "rcx": "ecx", "rdx": "edx",
    "rsi": "esi", "rdi": "edi", "rbp": "ebp", "rsp": "esp",
    "r8": "r8d", "r9": "r9d", "r10": "r10d", "r11": "r11d",
    "r12": "r12d", "r13": "r13d", "r14": "r14d", "r15": "r15d",
}

# Map 64-bit register name → DynASM register index for Rq()/Rd() macros.
_REG64_TO_IDX: dict[str, int] = {
    "rax": 0, "rcx": 1, "rdx": 2, "rbx": 3,
    "rsp": 4, "rbp": 5, "rsi": 6, "rdi": 7,
    "r8": 8, "r9": 9, "r10": 10, "r11": 11,
    "r12": 12, "r13": 13, "r14": 14, "r15": 15,
}

# Map any register name (64-bit, 32-bit, 16-bit) to DynASM index
_ANY_REG_TO_IDX: dict[str, int] = {**_REG64_TO_IDX}
# Map any register name to the human-readable REG_* constant name
_ANY_REG_TO_NAME: dict[str, str] = {**_REG_IDX_NAME}
# 16-bit register names
_REG16_NAMES: dict[str, str] = {
    "rax": "ax", "rbx": "bx", "rcx": "cx", "rdx": "dx",
    "rsi": "si", "rdi": "di", "rbp": "bp", "rsp": "sp",
    "r8": "r8w", "r9": "r9w", "r10": "r10w", "r11": "r11w",
    "r12": "r12w", "r13": "r13w", "r14": "r14w", "r15": "r15w",
}
for _r64, _idx in list(_REG64_TO_IDX.items()):
    _r32 = _REG64_TO_REG32[_r64]
    _r16 = _REG16_NAMES[_r64]
    _ANY_REG_TO_IDX[_r32] = _idx
    _ANY_REG_TO_IDX[_r16] = _idx
    _name = _REG_IDX_NAME[_r64]
    _ANY_REG_TO_NAME[_r32] = _name
    _ANY_REG_TO_NAME[_r16] = _name

# Map register index → set of all alias names (for liveness analysis)
_IDX_TO_ALL_NAMES: dict[int, set[str]] = {}
for _name, _idx in _ANY_REG_TO_IDX.items():
    _IDX_TO_ALL_NAMES.setdefault(_idx, set()).add(_name)
# Add 8-bit register names manually
_8BIT_NAMES: dict[int, list[str]] = {
    0: ["al", "ah"], 1: ["cl", "ch"], 2: ["dl", "dh"], 3: ["bl", "bh"],
    4: ["spl"], 5: ["bpl"], 6: ["sil"], 7: ["dil"],
    8: ["r8b"], 9: ["r9b"], 10: ["r10b"], 11: ["r11b"],
    12: ["r12b"], 13: ["r13b"], 14: ["r14b"], 15: ["r15b"],
}
for _idx, _names in _8BIT_NAMES.items():
    for _n in _names:
        _IDX_TO_ALL_NAMES.setdefault(_idx, set()).add(_n)
        _ANY_REG_TO_IDX[_n] = _idx

# ── Compiled regexes ───────────────────────────────────────────────────

# movabs REG, offset SYMBOL  or  movabs REG, offset SYMBOL+N
_RE_MOVABS = re.compile(
    r"^\s*movabs\s+(\w+),\s*offset\s+([\w.]+)(?:\+(\d+))?\s*(?:#.*)?$"
)

# movabs REG, IMM  (plain integer immediate, no "offset" keyword)
_RE_MOVABS_IMM = re.compile(
    r"^\s*movabs\s+(\w+),\s*(-?\d+)\s*(?:#.*)?$"
)

# call/jmp qword ptr [rip + SYM@GOTPCREL]
_RE_GOTPCREL_CALL = re.compile(
    r"^\s*(call|jmp)\s+qword\s+ptr\s+\[rip\s*\+\s*([\w.]+)@GOTPCREL\]\s*(?:#.*)?$"
)

# Generic instruction with GOTPCREL in a memory operand
_RE_GOTPCREL_MEM = re.compile(
    r"^(\s*\w+\s+)(.*?)(byte|word|dword|qword)\s+ptr\s+"
    r"\[rip\s*\+\s*([\w.]+)@GOTPCREL\](.*?)$"
)

# jmp/jcc to _JIT_JUMP_TARGET or _JIT_ERROR_TARGET
_RE_JIT_BRANCH = re.compile(
    r"^\s*(j\w+)\s+(_JIT_JUMP_TARGET|_JIT_ERROR_TARGET)\s*(?:#.*)?$"
)

# jmp/jcc to _JIT_CONTINUE or .L_JIT_CONTINUE
_RE_JIT_CONTINUE = re.compile(
    r"^\s*(j\w+)\s+(?:\.L)?_JIT_CONTINUE\s*(?:#.*)?$"
)

# Pattern for recognized local labels from LLVM (broad match for first pass)
# This matches any label-like definition that is NOT a _JIT_* special symbol.
_RE_ANY_LABEL_DEF = re.compile(r"^([\w.]+):\s*(?:#.*)?$")

# Local branch: jmp/jcc/call to a non-_JIT_ label (matched dynamically)
# (Compiled after the first pass discovers which labels are local)

# Local label definition  (compiled after first pass)
# These are all just re-used later as local_map lookups

_RE_ENTRY = re.compile(r"^_JIT_ENTRY:\s*(?:#.*)?$")
_RE_CONTINUE_LABEL = re.compile(r"^(?:\.L)?_JIT_CONTINUE:\s*(?:#.*)?$")
_RE_FUNC_END = re.compile(r"^\.Lfunc_end\d+:\s*$")

# Section directives
_RE_COLD_SECTION = re.compile(r'^\s*\.section\s+(?:\.text\.cold|__llvm_cold)')
_RE_TEXT_SECTION = re.compile(r"^\s*\.text\s*$")
_RE_RODATA_SECTION = re.compile(r"^\s*\.section\s+\.l?rodata")

# Data inside rodata
_RE_ASCIZ = re.compile(r'^\s*\.asciz\s+"(.*?)"')
_RE_DATA_LABEL = re.compile(r"^(\.L[\w.]+):\s*(?:#.*)?$")
_RE_BYTE_DATA = re.compile(r"^\s*\.(byte|short|long|quad)\s+(.*)")

# Directives to skip entirely
_RE_SKIP = re.compile(
    r"^\s*\.(file|globl|type|size|addrsig|addrsig_sym|hidden|ident|"
    r"intel_syntax|section\s+\"\.note|p2align|cfi_\w+)\b"
)

_RE_BLANK = re.compile(r"^\s*(?:#.*)?$")
_RE_ALIGN = re.compile(r"^\s*\.p2align\s+(\d+)")

# LLVM JIT fold pass inline-asm markers.
# Format: nop # @@JIT_MOV_IMM %reg, <C-expression>@@
_RE_JIT_MARKER = re.compile(
    r"^\s*nop\s+#\s*@@(JIT_MOV_IMM|JIT_TEST|JIT_CMP|JIT_FRAME_ANCHOR)(?:\s+(%?\w+)(?:,\s*(.+?))?)?@@\s*$"
)

# ── Peephole optimization patterns ────────────────────────────────────

# emit_mov_imm(Dst, REG_NAME_OR_IDX, EXPR);
# emit_mov_imm_preserve_flags(Dst, REG_NAME_OR_IDX, EXPR);
_RE_EMIT_MOV_IMM = re.compile(
    r"^(\s*)emit_mov_imm(?:_preserve_flags)?\(Dst,\s*(\w+),\s*(.+?)\);$"
)

# ── Regexes for parse_line() ───────────────────────────────────────────
#
# These patterns are used by parse_line() to classify DynASM output lines
# into typed Line objects (Asm, CCall, Label, Section, FuncDef, Blank).

# C helper calls: emit_mov_imm(Dst, REG, EXPR);
#                 emit_mov_imm_preserve_flags(Dst, REG, EXPR);
# Re-uses the existing _RE_EMIT_MOV_IMM but with different group semantics.
_RE_C_CALL_MOV_IMM = re.compile(
    r"^(\s*)(emit_mov_imm(?:_preserve_flags)?)\(Dst,\s*(.+)\);$"
)
# C helper calls: emit_call_ext(Dst, ARGS);
_RE_C_CALL_EXT = re.compile(r"^(\s*)emit_call_ext\(Dst,\s*(.+)\);$")
# C helper calls: emit_cmp_reg_imm(Dst, ARGS);
_RE_C_CALL_CMP = re.compile(r"^(\s*)emit_cmp_reg_imm\(Dst,\s*(.+)\);$")
# C helper calls: emit_cmp_mem64_imm(Dst, ARGS);
_RE_C_CALL_CMP_MEM64 = re.compile(r"^(\s*)emit_cmp_mem64_imm\(Dst,\s*(.+)\);$")
# C helper calls: emit_{test,and,or,xor}_reg_imm(Dst, ARGS);
_RE_C_CALL_ALU = re.compile(
    r"^(\s*)(emit_(?:test|and|or|xor)_reg_imm)\(Dst,\s*(.+)\);$"
)

# DynASM label definition: |=>LABEL_NAME:
_RE_DASC_LABEL = re.compile(r"^\s*\|\s*=>\s*(.+?)\s*:\s*$")
# DynASM section directive: |.code, |.cold, |.data
_RE_DASC_SECTION = re.compile(r"^\s*\|\s*\.(code|cold|data)\b")
# DynASM assembly instruction: | mnemonic [operands]
_RE_ASM_LINE = re.compile(r"^\s*\|\s*(\w+)(?:\s+(.+))?\s*$")
# Function definition: static void emit_OPNAME(...)
_RE_DASC_FUNC_DEF = re.compile(r"^\s*static\s+void\s+emit_\w+\s*\(")

# ── Typed operand and line classification ──────────────────────────────
#
# Instead of parsing lines into flat strings and then re-matching with
# per-pattern regexes, we parse each line *once* into typed objects:
#
#   Operand types:  Reg, Mem, Imm    (what instructions operate on)
#   Line types:     Asm, CCall, Label, Section, FuncDef, Blank, CCode
#
# Pattern functions use Python 3.10+ structural pattern matching
# (match/case) to destructure these objects directly.  For example:
#
#     match parse_line(text):
#         case Asm("mov", dst=Reg(name="rax"), src=Mem(size="qword")):
#             ...  # handle mov rax, qword [...]
#         case CCall(kind=CCallKind.CALL_EXT):
#             ...  # handle emit_call_ext(...)
#
# This replaces the old LineKind enum + monolithic Line dataclass with
# proper typing that enables exhaustive matching and IDE autocompletion.
#
# Design principles:
#   - Operand types are frozen (immutable, hashable) for safe matching
#   - Each line type carries only the fields relevant to that type
#   - Raw text preserved in every line type for output generation
#   - Helper functions (is_call, is_branch, etc.) work across types

# ── Operand types ──────────────────────────────────────────────────────


@dataclasses.dataclass(frozen=True, slots=True)
class Reg:
    """Register operand (e.g., rax, eax, al, r14d).

    Attributes:
        name: Register name as it appears in the assembly (case-preserved).
    """

    name: str

    @property
    def idx(self) -> int | None:
        """Canonical register index (0=rax, 1=rcx, ..., 15=r15)."""
        return _ANY_REG_TO_IDX.get(self.name.lower())

    @property
    def bits(self) -> int:
        """Register width in bits (8, 16, 32, or 64)."""
        return _reg_bits(self.name)

    @property
    def jreg(self) -> str | None:
        """JREG_* constant name (e.g., "JREG_RAX"), or None."""
        return _ANY_REG_TO_NAME.get(self.name.lower())

    def __str__(self) -> str:
        return self.name


@dataclasses.dataclass(frozen=True, slots=True)
class Mem:
    """Memory operand (e.g., qword [r14 + 8], byte [rax]).

    Attributes:
        size:   Size prefix ("qword", "dword", "word", "byte") or None.
        base:   Base register name, or None for complex addressing.
        offset: Displacement (default 0).
        index:  Index register name, or None.
        scale:  Scale factor (1, 2, 4, 8), or None.
        expr:   Full bracket expression for output (e.g., "[r14 + 8]").
    """

    size: str | None
    base: str | None
    offset: int = 0
    index: str | None = None
    scale: int | None = None
    expr: str = ""

    def __str__(self) -> str:
        return f"{self.size} {self.expr}" if self.size else self.expr


@dataclasses.dataclass(frozen=True, slots=True)
class Imm:
    """Immediate operand (e.g., 42, -1, 0xff).

    Attributes:
        value: Numeric value of the immediate.
        text:  Original text representation (preserved for output).
    """

    value: int
    text: str = ""

    def __str__(self) -> str:
        return self.text if self.text else str(self.value)


# Union of all operand types, for type annotations.
Op = Reg | Mem | Imm


# ── Line types ─────────────────────────────────────────────────────────


@enum.unique
class CCallKind(enum.Enum):
    """Sub-classification for C helper calls."""

    MOV_IMM = "emit_mov_imm"
    CALL_EXT = "emit_call_ext"
    CMP_REG_IMM = "emit_cmp_reg_imm"
    CMP_MEM64_IMM = "emit_cmp_mem64_imm"
    ALU_REG_IMM = "emit_alu_reg_imm"  # test/and/or/xor_reg_imm
    OTHER = "other"


@dataclasses.dataclass(slots=True)
class Asm:
    """Assembly instruction (e.g., ``| mov rax, qword [r14 + 8]``).

    Attributes:
        mnemonic: Instruction mnemonic (e.g., "mov", "cmp", "je").
        dst:      First (destination) operand as typed Reg/Mem/Imm, or None.
        src:      Second (source) operand as typed Reg/Mem/Imm, or None.
        target:   Branch target for jmp/jcc (e.g., "=>L(3)"), or None.
        raw:      Original line text (preserved for output).
    """

    mnemonic: str
    dst: Op | None = None
    src: Op | None = None
    target: str | None = None
    raw: str = ""

    def __str__(self) -> str:
        return self.raw


@dataclasses.dataclass(slots=True)
class CCall:
    """C helper call (e.g., ``emit_call_ext(Dst, ...)``).

    Attributes:
        kind:   Which helper (MOV_IMM, CALL_EXT, CMP_REG_IMM).
        helper: Helper function name as emitted in the C source.
        args:   Raw argument string inside parentheses.
        argv:   Parsed argument tokens split at top-level commas.
        indent: Leading whitespace (for replacement line generation).
        raw:    Original line text.
    """

    kind: CCallKind
    helper: str = ""
    args: str = ""
    argv: tuple[str, ...] = ()
    indent: str = ""
    raw: str = ""


@dataclasses.dataclass(slots=True)
class Label:
    """Label definition (e.g., ``|=>L(3):``).

    Attributes:
        name: Label identifier (e.g., "L(3)", "uop_label").
        raw:  Original line text.
    """

    name: str
    raw: str = ""


@dataclasses.dataclass(slots=True)
class Section:
    """Section directive (e.g., ``|.code``, ``|.cold``).

    Attributes:
        name: Section name ("code", "cold", or "data").
        raw:  Original line text.
    """

    name: str
    raw: str = ""


@dataclasses.dataclass(slots=True)
class FuncDef:
    """Function definition (e.g., ``static void emit_BINARY_OP_...``).

    Attributes:
        raw: Original line text.
    """

    raw: str = ""


@dataclasses.dataclass(slots=True)
class Blank:
    """Empty line or comment.

    Attributes:
        raw: Original line text.
    """

    raw: str = ""


@dataclasses.dataclass(slots=True)
class CCode:
    """C code line (if/else/braces/etc.).

    Attributes:
        raw: Original line text.
    """

    raw: str = ""


# Union of all line types — use as type annotation for parsed lines.
Line = Asm | CCall | Label | Section | FuncDef | Blank | CCode


# ── Operand parsing ───────────────────────────────────────────────────

_SIZE_PREFIXES = frozenset(("qword", "dword", "word", "byte"))

_ALL_REGS = frozenset(_ANY_REG_TO_IDX.keys())

_RE_MEM_TERM_SCALED = re.compile(r"^(\d+)\s*\*\s*(\w+)$")
_RE_MEM_TERM_SCALED_REV = re.compile(r"^(\w+)\s*\*\s*(\d+)$")


def _parse_mem_expr(inner: str) -> tuple[str | None, int, str | None, int | None]:
    """Parse the expression inside memory brackets.

    Examples:
        "r14"           → ("r14", 0, None, None)
        "r14 + 8"       → ("r14", 8, None, None)
        "r14 - 8"       → ("r14", -8, None, None)
        "rdi + rcx*4"   → ("rdi", 0, "rcx", 4)
        "rdi + 4*rcx + 8" → ("rdi", 8, "rcx", 4)
        "rcx*8+0"       → (None, 0, "rcx", 8)
    """
    base = None
    offset = 0
    index = None
    scale = None

    # Split on + and - while preserving the sign operator
    terms = re.split(r"\s*([+-])\s*", inner.strip())
    sign = 1
    for term in terms:
        term = term.strip()
        if not term:
            continue
        if term == "+":
            sign = 1
            continue
        if term == "-":
            sign = -1
            continue
        # scale*register (e.g., "4*rcx")
        m = _RE_MEM_TERM_SCALED.match(term)
        if m:
            scale = int(m.group(1))
            index = m.group(2)
            continue
        # register*scale (e.g., "rcx*4")
        m = _RE_MEM_TERM_SCALED_REV.match(term)
        if m:
            index = m.group(1)
            scale = int(m.group(2))
            continue
        # Numeric displacement
        try:
            offset += sign * int(term, 0)
            continue
        except ValueError:
            pass
        # Register — assign to base or index
        if base is None:
            base = term
        elif index is None:
            index = term
            scale = 1

    return base, offset, index, scale


def _parse_operand(text: str) -> Op:
    """Parse a single operand string into a typed Reg, Mem, or Imm.

    Examples:
        "rax"              → Reg("rax")
        "qword [r14 + 8]"  → Mem(size="qword", base="r14", offset=8, ...)
        "[rax]"            → Mem(size=None, base="rax", ...)
        "42"               → Imm(42, "42")
        "-1"               → Imm(-1, "-1")
    """
    text = text.strip()

    # Memory operand: contains brackets
    if "[" in text:
        size = None
        rest = text
        for s in _SIZE_PREFIXES:
            if rest.lower().startswith(s + " "):
                size = s
                rest = rest[len(s) :].strip()
                break
        # Extract bracket contents
        bracket_start = rest.index("[")
        bracket_end = rest.rindex("]")
        inner = rest[bracket_start + 1 : bracket_end].strip()
        expr = rest[bracket_start : bracket_end + 1]
        base, offset, index, scale = _parse_mem_expr(inner)
        return Mem(
            size=size,
            base=base,
            offset=offset,
            index=index,
            scale=scale,
            expr=expr,
        )

    # Register: check all known register names
    if text.lower() in _ALL_REGS:
        return Reg(text)

    # Immediate: try parsing as integer
    try:
        return Imm(int(text, 0), text)
    except ValueError:
        pass

    # DynASM label reference (e.g., "=>L(3)") — treat as Imm-like
    # (used by branch targets, but normally handled via target field)
    return Imm(0, text)


def _split_operands(operands: str) -> list[str]:
    """Split operand string on commas, respecting brackets.

    ``"qword [r13 + 64], r14"``  → ``["qword [r13 + 64]", "r14"]``
    ``"rax"``                     → ``["rax"]``
    ``"qword [rax + rbx*8 + 16]"`` → ``["qword [rax + rbx*8 + 16]"]``
    """
    parts: list[str] = []
    depth = 0
    start = 0
    for j, ch in enumerate(operands):
        if ch == "[":
            depth += 1
        elif ch == "]":
            depth -= 1
        elif ch == "," and depth == 0:
            parts.append(operands[start:j])
            start = j + 1
    parts.append(operands[start:])
    return parts


def _split_call_args(args: str) -> tuple[str, ...]:
    """Split C helper arguments on top-level commas.

    Unlike ``_split_operands()``, this helper understands parentheses as well
    as brackets so expressions like ``(uintptr_t)&PyType_Type`` stay intact.
    """
    parts: list[str] = []
    depth = 0
    start = 0
    for j, ch in enumerate(args):
        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth = max(0, depth - 1)
        elif ch == "," and depth == 0:
            parts.append(args[start:j].strip())
            start = j + 1
    parts.append(args[start:].strip())
    return tuple(part for part in parts if part)


# ── DynASM assembly line parser (produces typed Asm / CCall / etc.) ────

_BRANCH_MNEMONICS = frozenset((
    "jmp", "je", "jne", "jz", "jnz", "ja", "jae", "jb", "jbe",
    "jg", "jge", "jl", "jle", "js", "jns", "jo", "jno", "jp", "jnp",
))


def parse_line(text: str) -> Line:
    """Parse a DynASM output line into a typed Line object.

    Returns one of: Asm, CCall, Label, Section, FuncDef, Blank, CCode.
    Each type carries only the fields relevant to that line kind, with
    structured operands (Reg/Mem/Imm) for assembly instructions.

    Classification priority:
      1. C helper calls (emit_mov_imm, emit_call_ext, emit_cmp_reg_imm)
      2. DynASM labels (|=>NAME:)
      3. DynASM section directives (|.code, |.cold)
      4. DynASM assembly instructions (| mnemonic ...)
      5. Function definitions (static void emit_...)
      6. Blanks / comments
      7. Everything else (C code)
    """
    stripped = text.strip()

    # ── C helper calls ──
    m = _RE_C_CALL_MOV_IMM.match(stripped)
    if m:
        args = m.group(3)
        return CCall(
            kind=CCallKind.MOV_IMM,
            helper=m.group(2),
            indent=m.group(1),
            args=args,
            argv=_split_call_args(args),
            raw=text,
        )
    m = _RE_C_CALL_EXT.match(stripped)
    if m:
        args = m.group(2)
        return CCall(
            kind=CCallKind.CALL_EXT,
            helper="emit_call_ext",
            indent=m.group(1),
            args=args,
            argv=_split_call_args(args),
            raw=text,
        )
    m = _RE_C_CALL_CMP.match(stripped)
    if m:
        args = m.group(2)
        return CCall(
            kind=CCallKind.CMP_REG_IMM,
            helper="emit_cmp_reg_imm",
            indent=m.group(1),
            args=args,
            argv=_split_call_args(args),
            raw=text,
        )
    m = _RE_C_CALL_CMP_MEM64.match(stripped)
    if m:
        args = m.group(2)
        return CCall(
            kind=CCallKind.CMP_MEM64_IMM,
            helper="emit_cmp_mem64_imm",
            indent=m.group(1),
            args=args,
            argv=_split_call_args(args),
            raw=text,
        )
    m = _RE_C_CALL_ALU.match(stripped)
    if m:
        args = m.group(3)
        return CCall(
            kind=CCallKind.ALU_REG_IMM,
            helper=m.group(2),
            indent=m.group(1),
            args=args,
            argv=_split_call_args(args),
            raw=text,
        )

    # ── DynASM labels ──
    m = _RE_DASC_LABEL.match(stripped)
    if m:
        return Label(name=m.group(1), raw=text)

    # ── DynASM section directives ──
    m = _RE_DASC_SECTION.match(stripped)
    if m:
        return Section(name=m.group(1), raw=text)

    # ── DynASM assembly instructions ──
    m = _RE_ASM_LINE.match(stripped)
    if m:
        mnemonic = m.group(1)
        operands_str = m.group(2)
        dst: Op | None = None
        src: Op | None = None
        target: str | None = None

        if operands_str:
            # Branch instructions: operand is a target label, not a dst/src
            if mnemonic in _BRANCH_MNEMONICS:
                target = operands_str.strip()
            else:
                parts = _split_operands(operands_str)
                if parts:
                    dst = _parse_operand(parts[0])
                    if len(parts) > 1:
                        src = _parse_operand(parts[1])

        return Asm(mnemonic=mnemonic, dst=dst, src=src,
                   target=target, raw=text)

    # ── Function definitions ──
    if _RE_DASC_FUNC_DEF.match(stripped):
        return FuncDef(raw=text)

    # ── Blanks / comments ──
    if not stripped or stripped.startswith("//"):
        return Blank(raw=text)

    # ── Everything else (C code) ──
    return CCode(raw=text)


def parse_lines(lines: list[str]) -> list[Line]:
    """Batch-parse a list of DynASM output lines into typed objects."""
    return [parse_line(text) for text in lines]


# ── Query helpers (work across the Line type hierarchy) ────────────────


def is_call(line: Line) -> bool:
    """Is this line a function call (ASM 'call' or C emit_call_ext)?"""
    match line:
        case CCall(kind=CCallKind.CALL_EXT):
            return True
        case Asm(mnemonic="call"):
            return True
    return False


def is_branch(line: Line) -> bool:
    """Is this a conditional jump (jne, je, jae, etc.)?"""
    match line:
        case Asm(mnemonic=m) if m.startswith("j") and m != "jmp":
            return True
    return False


def is_jump(line: Line) -> bool:
    """Is this an unconditional jmp?"""
    return isinstance(line, Asm) and line.mnemonic == "jmp"


def branch_target(line: Line) -> str | None:
    """Extract branch/jump target label (e.g., "=>L(3)"), or None."""
    match line:
        case Asm(target=t) if t and t.startswith("=>"):
            return t
    return None


def line_raw(line: Line) -> str:
    """Get the original text of any Line type."""
    return line.raw


@dataclasses.dataclass(frozen=True, slots=True)
class _LineEffect:
    """Structured dataflow summary for one parsed line."""

    reads: frozenset[int] = dataclasses.field(default_factory=frozenset)
    full_writes: frozenset[int] = dataclasses.field(default_factory=frozenset)
    partial_writes: frozenset[int] = dataclasses.field(default_factory=frozenset)
    uses_flags: bool = False
    writes_flags: bool = False


@dataclasses.dataclass(frozen=True, slots=True)
class _BasicBlock:
    """Basic block inside one emitted stencil function."""

    start: int
    end: int
    labels: tuple[str, ...] = ()
    successors: tuple[int, ...] = ()


@dataclasses.dataclass(frozen=True, slots=True)
class _PeepholeFunction:
    """Structured view of one emitted stencil function."""

    start: int
    end: int
    blocks: tuple[_BasicBlock, ...] = ()


def _operand_regs(op: Op | None) -> frozenset[int]:
    """Registers read while evaluating an operand."""
    regs: set[int] = set()
    match op:
        case Reg(idx=idx) if idx is not None:
            regs.add(idx)
        case Mem(base=base, index=index):
            for name in (base, index):
                if name is None:
                    continue
                idx = Reg(name).idx
                if idx is not None:
                    regs.add(idx)
    return frozenset(regs)


def _mem_uses_reg(mem: Mem, reg_idx: int) -> bool:
    """Does a memory address depend on the given canonical register?"""
    return reg_idx in _operand_regs(mem)


def _compute_c_depth(lines: list[Line]) -> list[int]:
    """Track inline-C nesting depth for each emitted line."""
    c_depth = [0] * len(lines)
    depth = 0
    for i, line in enumerate(lines):
        stripped = line.raw.strip()
        if stripped.endswith("{"):
            depth += 1
        c_depth[i] = depth
        if stripped == "}" or stripped == "} else {":
            depth = max(0, depth - 1)
            c_depth[i] = depth
    return c_depth


def _build_blocks(
    parsed: list[Line],
    label_to_line: dict[str, int],
    start: int,
    end: int,
) -> tuple[_BasicBlock, ...]:
    """Split one stencil function into coarse basic blocks."""

    if start >= end:
        return ()

    starts = {start}
    for i in range(start + 1, end):
        if isinstance(parsed[i], (Label, Section)):
            starts.add(i)
        prev = parsed[i - 1]
        if isinstance(prev, Asm) and (is_branch(prev) or is_jump(prev) or prev.mnemonic == "ret"):
            starts.add(i)

    ordered = sorted(starts)
    blocks: list[_BasicBlock] = []
    for idx, block_start in enumerate(ordered):
        block_end = ordered[idx + 1] if idx + 1 < len(ordered) else end
        labels: list[str] = []
        j = block_start
        while j < block_end and isinstance(parsed[j], Label):
            labels.append(parsed[j].name)
            j += 1

        successors: list[int] = []
        last = parsed[block_end - 1]
        if isinstance(last, Asm):
            target = branch_target(last)
            if is_branch(last):
                if block_end < end:
                    successors.append(block_end)
                if target and target.startswith("=>"):
                    target_idx = label_to_line.get(target[2:])
                    if target_idx is not None:
                        successors.append(target_idx)
            elif is_jump(last):
                if target and target.startswith("=>"):
                    target_idx = label_to_line.get(target[2:])
                    if target_idx is not None:
                        successors.append(target_idx)
            elif last.mnemonic != "ret" and block_end < end:
                successors.append(block_end)
        elif block_end < end:
            successors.append(block_end)

        blocks.append(
            _BasicBlock(
                start=block_start,
                end=block_end,
                labels=tuple(labels),
                successors=tuple(dict.fromkeys(successors)),
            )
        )
    return tuple(blocks)


@dataclasses.dataclass(slots=True)
class _PeepholeProgram:
    """Parsed view of the current emitted DynASM function bodies."""

    lines: list[str]
    parsed: list[Line]
    c_depth: list[int]
    label_to_line: dict[str, int]
    effects: list[_LineEffect]
    functions: tuple[_PeepholeFunction, ...]
    function_starts: frozenset[int]
    function_end_by_line: list[int]

    @classmethod
    def from_lines(cls, lines: list[str]) -> "_PeepholeProgram":
        parsed = parse_lines(lines)
        c_depth = _compute_c_depth(parsed)
        label_to_line = {
            line.name: i for i, line in enumerate(parsed) if isinstance(line, Label)
        }
        effects = [_line_effect(line) for line in parsed]

        func_starts = [i for i, line in enumerate(parsed) if isinstance(line, FuncDef)]
        if not func_starts:
            func_starts = [0]
        ranges = [
            (
                start,
                func_starts[idx + 1]
                if idx + 1 < len(func_starts)
                else len(lines),
            )
            for idx, start in enumerate(func_starts)
        ]

        functions = tuple(
            _PeepholeFunction(
                start=start,
                end=end,
                blocks=_build_blocks(parsed, label_to_line, start, end),
            )
            for start, end in ranges
        )
        function_end_by_line = [len(lines)] * len(lines)
        for start, end in ranges:
            for i in range(start, end):
                function_end_by_line[i] = end

        return cls(
            lines=lines,
            parsed=parsed,
            c_depth=c_depth,
            label_to_line=label_to_line,
            effects=effects,
            functions=functions,
            function_starts=frozenset(func_starts),
            function_end_by_line=function_end_by_line,
        )

    def reg_dead_after(self, start: int, reg_idx: int) -> bool:
        """Control-flow-aware deadness query using structured line effects."""
        if start >= len(self.lines):
            return True
        if reg_idx not in _IDX_TO_ALL_NAMES:
            return False

        func_end = self.function_end_by_line[start]
        start_depth = self.c_depth[start]
        visited: set[int] = set()
        worklist = [start]

        while worklist:
            pos = worklist.pop()
            while pos < func_end:
                if pos in visited:
                    break
                visited.add(pos)

                effect = self.effects[pos]
                if reg_idx in effect.reads or reg_idx in effect.partial_writes:
                    return False
                if reg_idx in effect.full_writes:
                    break

                successors = self.successors(pos, start_depth)
                if not successors:
                    break
                fallthrough = pos + 1
                if len(successors) == 1 and successors[0] == fallthrough:
                    pos = fallthrough
                    continue
                for succ in successors[1:]:
                    if succ < func_end:
                        worklist.append(succ)
                pos = successors[0]

        return True

    def successors(self, pos: int, start_depth: int) -> tuple[int, ...]:
        """Reachable successors from one line in a deadness query."""
        if pos >= len(self.parsed):
            return ()
        line = self.parsed[pos]
        func_end = self.function_end_by_line[pos]
        next_pos = pos + 1
        in_c_block = self.c_depth[pos] > start_depth

        match line:
            case Asm(mnemonic="jmp", target=target):
                if target and target.startswith("=>"):
                    target_idx = self.label_to_line.get(target[2:])
                    if target_idx is not None:
                        if in_c_block and next_pos < func_end:
                            return (next_pos, target_idx + 1)
                        return (target_idx + 1,)
                if in_c_block and next_pos < func_end:
                    return (next_pos,)
                return ()
            case Asm(mnemonic="ret"):
                if in_c_block and next_pos < func_end:
                    return (next_pos,)
                return ()
            case Asm() if is_branch(line):
                succs: list[int] = []
                if next_pos < func_end:
                    succs.append(next_pos)
                target = branch_target(line)
                if target and target.startswith("=>"):
                    target_idx = self.label_to_line.get(target[2:])
                    if target_idx is not None:
                        succs.append(target_idx + 1)
                return tuple(succs)
            case _:
                if next_pos < func_end:
                    return (next_pos,)
                return ()


def fmt_op(op: Op) -> str:
    """Format a typed operand back to assembly text."""
    match op:
        case Reg(name=n):
            return n
        case Mem(size=s, expr=e):
            return f"{s} {e}" if s else e
        case Imm(text=t) if t:
            return t
        case Imm(value=v):
            return str(v)


# Operand size in bits based on register name (for cast selection)
def _reg_bits(name: str) -> int:
    """Return the operand size in bits for a register name."""
    name = name.lower()
    if name in ("al", "ah", "bl", "bh", "cl", "ch", "dl", "dh",
                "spl", "bpl", "sil", "dil") or name.endswith("b"):
        return 8
    if name in ("ax", "bx", "cx", "dx", "si", "di", "bp", "sp") \
            or name.endswith("w"):
        return 16
    if name.startswith("e") or (name.startswith("r") and name.endswith("d")):
        return 32
    return 64


# ── Peephole optimizer — pattern-based architecture ────────────────────
#
# Inspired by _optimizers.py's clean separation of concerns, the peephole
# operates as a *pattern registry* with a simple driver loop.  Each pattern
# is a small, self-contained function that examines lines at position ``i``
# and returns a ``_Match`` (consumed lines + output lines) or ``None``.
#
# Two categories of patterns:
#
#  1. **emit_mov_imm chain patterns** — These fire when the current line
#     is ``emit_mov_imm(Dst, REG, EXPR);`` and try to fold subsequent
#     instructions into the immediate expression.  They compose: e.g.
#     mov_imm → movzx → shl chains.  Handled by ``_fold_mov_imm()``.
#
#  2. **Standalone patterns** — Independent patterns that operate on raw
#     DynASM assembly lines (e.g. store-reload elimination).  Each is a
#     function registered in ``_STANDALONE_PATTERNS``.
#
# Adding a new pattern: write a function matching the ``_PatternFn``
# signature, add it to ``_STANDALONE_PATTERNS``, done.


@dataclasses.dataclass
class _Match:
    """Result of a successful pattern match."""
    consumed: int          # number of input lines consumed
    output: list[str]      # replacement lines to emit


@dataclasses.dataclass
class _FoldCtx:
    """Shared context for emit_mov_imm fold patterns.

    Bundles the parameters that every ``_try_*`` function needs, eliminating
    the 5 different call signatures that previously required a dispatch table
    in ``_fold_mov_imm``.  All terminal ``_try_*`` functions now take a single
    ``ctx: _FoldCtx`` argument and return ``_Match | None``.

    The ``parsed`` list contains typed Line objects (Asm, CCall, etc.)
    parallel to ``lines``.  Pattern functions use ``ctx.cur`` to get the
    current typed instruction and ``match``/``case`` for destructuring.
    """
    program: _PeepholeProgram   # structured program for effects/CFG queries
    lines: list[str]          # all input lines (raw text)
    parsed: list[Line]        # typed Line objects (parallel to lines)
    i: int                    # current look-ahead position
    src_idx: int              # register index (0=RAX, 1=RCX, etc.)
    src_name: str             # "JREG_RAX" etc.
    indent: str               # indentation from the emit_mov_imm
    expr: str                 # current expression (mutated by modifier phases)

    @property
    def cur(self) -> Line:
        """The parsed Line at the current look-ahead position."""
        return self.parsed[self.i]


@dataclasses.dataclass
class _PeepholeState:
    """Mutable state carried across a single peephole pass.

    Reset at function boundaries (``static void emit_...``) to avoid
    cross-stencil interference.
    """

    # Merge labels where we eliminated a stackpointer reload;
    # cold-path jumps back to these need a reload inserted.
    need_reload_before_jmp: set[str] = dataclasses.field(default_factory=set)

    def reset(self) -> None:
        """Reset per-function state at stencil boundaries."""
        self.need_reload_before_jmp.clear()


_PeepholePassFn = typing.Callable[
    [_PeepholeProgram, int, list[str], _PeepholeState],
    int | None,
]


@dataclasses.dataclass(frozen=True, slots=True)
class _PeepholePass:
    """One pass step in the peephole pipeline."""

    name: str
    apply: _PeepholePassFn


# ── Peephole statistics ────────────────────────────────────────────────
#
# Each pattern increments its counter when it fires.  The stats are
# printed at the end of stencil conversion if PEEPHOLE_STATS is True.

PEEPHOLE_STATS = False  # set True or use --peephole-stats to see counts

_peephole_counts: dict[str, int] = {
    "P6_indexed_mem": 0,
    "P8_alu_imm_fold": 0,
    "P12_store_imm": 0,
    "P13_dead_null_check": 0,
    "P14_test_memory_fold": 0,
    "P15_shift_fold": 0,
    "P16_two_mov_add": 0,
    "P17_lea_fold": 0,
    "P18_dead_frame_anchor": 0,
    "P19_inverse_mov_restore": 0,
    "SP0_preserve_flags_mov_imm": 0,
    "SP1_store_reload_elim": 0,
    "SP2_cold_reload_insert": 0,
    "SP3_inverted_store_reload": 0,
    "dead_label_elim": 0,
    "LLVM_fold_marker": 0,
}


def _stat(name: str) -> None:
    """Increment a peephole pattern counter."""
    _peephole_counts[name] = _peephole_counts.get(name, 0) + 1


def get_peephole_stats() -> dict[str, int]:
    """Return current peephole statistics (pattern name → fire count)."""
    return dict(_peephole_counts)


def reset_peephole_stats() -> None:
    """Reset all peephole counters to zero."""
    for key in _peephole_counts:
        _peephole_counts[key] = 0


def print_peephole_stats() -> None:
    """Print peephole statistics to stderr."""
    import sys

    total = sum(_peephole_counts.values())
    if total == 0:
        return
    print(f"\nPeephole optimization statistics ({total} total):", file=sys.stderr)
    for name, count in sorted(_peephole_counts.items()):
        if count > 0:
            print(f"  {name:30s}: {count:5d}", file=sys.stderr)


# ── x86-64 specific imports ────────────────────────────────────────────
# Import architecture-specific peephole patterns, instruction effects,
# and calling convention constants from the x86-64 module.
# These are re-exported for backward compatibility with existing consumers
# (build.py, _dasc_writer.py, _targets.py, test_peephole.py).
#
# This import is placed here (after all generic definitions) because the
# amd64 module imports types and infrastructure from this file.  By this
# point all those symbols are defined, so the circular import resolves
# cleanly.
from _asm_to_dasc_amd64 import (  # noqa: E402
    # Calling convention
    REG_FRAME, REG_STACK_PTR, REG_TSTATE, REG_EXECUTOR,
    FRAME_IP_OFFSET, FRAME_STACKPOINTER_OFFSET,
    _C, _SP_STORE, _SP_RELOAD, _SP_RELOAD_LINE,
    # JREG mapping
    _JREG_TO_IDX, _IDX_TO_JREG, _parse_jreg, _jreg_arg_index,
    # Instruction effects
    _line_effect, _reg_write_sets,
    # Pattern helpers
    _reg_dead_after,
    uses_reg, is_store_sp, is_reload_sp,
    _is_flag_writer, _is_flag_consumer,
    _preserve_flags_mov_imm, _parse_emit_mov_imm_call,
    # Pass registry
    _PEEPHOLE_PASSES,
)


# ── Driver ─────────────────────────────────────────────────────────────


def _peephole_pass(lines: list[str]) -> tuple[list[str], bool]:
    """Single pass of peephole optimization. Returns (result, changed).

    Parses all input lines once into a structured program with typed lines,
    helper-call arguments, block boundaries, and line effects.  Registered
    passes then match against that program rather than raw-text regex state.
    """
    changed = False
    result: list[str] = []
    state = _PeepholeState()
    program = _PeepholeProgram.from_lines(lines)
    i = 0
    while i < len(lines):
        # Reset per-function state at stencil boundaries
        if i in program.function_starts:
            state.reset()

        matched = False
        for peephole_pass in _PEEPHOLE_PASSES:
            advance = peephole_pass.apply(program, i, result, state)
            if advance is not None:
                i += advance
                changed = True
                matched = True
                break
        if matched:
            continue

        # No pattern matched — pass through
        result.append(lines[i])
        i += 1
    return result, changed


def _peephole_optimize(lines: list[str]) -> list[str]:
    """Apply peephole optimizations until fixpoint (max 5 passes).

    Multi-pass iteration enables chained optimizations: e.g. Pattern 10
    creates a new emit_mov_imm that Pattern 6 can then consume.
    """
    for _pass in range(5):
        result, changed = _peephole_pass(lines)
        if not changed:
            break
        lines = result
    else:
        lines = result
    return _eliminate_dead_labels(lines)


def _eliminate_dead_labels(lines: list[str]) -> list[str]:
    """Remove labels that are defined but never jumped to.

    Many stencils contain structural labels (e.g. |=>L(2):) that serve as
    fall-through targets but are never referenced by any jump instruction.
    These dead labels clutter the assembly output and make it harder to read.

    Uses ``parse_lines()`` to identify labels by type, avoiding ad-hoc regex
    duplication.
    """
    parsed = parse_lines(lines)
    # Collect all L(N) references (non-definition uses).
    # A reference is any occurrence of =>L(N) that is NOT a label def.
    text = "".join(lines)
    referenced = set(re.findall(r"=>L\((\d+)\)(?!:)", text))
    result: list[str] = []
    removed = 0
    for line_obj in parsed:
        # Only remove L(N) labels; leave uop_label, continue_label etc.
        if isinstance(line_obj, Label):
            m = re.match(r"L\((\d+)\)", line_obj.name)
            if m and m.group(1) not in referenced:
                removed += 1
                continue
        result.append(line_obj.raw)
    for _ in range(removed):
        _stat("dead_label_elim")
    return result


# ── Data structures ────────────────────────────────────────────────────


@dataclasses.dataclass
class DataItem:
    """A data blob from a .rodata section (e.g. an assert filename string)."""

    label: str
    data: bytearray = dataclasses.field(default_factory=bytearray)


@dataclasses.dataclass
class ConvertedStencil:
    """Result of converting one stencil to DynASM lines."""

    opname: str
    lines: list[str]
    # Number of internal PC labels needed by this stencil (excludes GOT)
    num_internal_labels: int
    # Data items from .rodata sections
    data_items: list[DataItem]
    # Stack frame size requested by the stencil's function prologue
    # (push rbp; [mov rbp, rsp;] sub rsp, N), or 0 if the stencil does not
    # use a standard entry frame. The runtime allocates the maximum such
    # frame once in the shim and the per-stencil prologue/epilogue is then
    # stripped from the emitted DynASM.
    frame_size: int = 0


# ── Helpers ─────────────────────────────────────────────────────────────

# x86 register names used to distinguish scale*reg from scale*immediate
_X86_REGS = frozenset([
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d",
    "ax", "cx", "dx", "bx", "sp", "bp", "si", "di",
])

def _swap_scale_reg(m: re.Match) -> str:
    """Swap scale*register to register*scale in SIB memory operands."""
    scale, name = m.group(1), m.group(2)
    if name.lower() in _X86_REGS:
        return f"{name}*{scale}"
    return m.group(0)


def _fix_syntax(text: str) -> str:
    """Apply DynASM syntax adjustments to an instruction."""
    # Convert tabs to spaces
    text = text.replace("\t", " ")
    # Strip inline comments (# ...)
    text = re.sub(r"\s*#.*$", "", text)
    # Convert xmmword to oword (DynASM uses oword for 128-bit)
    text = re.sub(r"\bxmmword\b", "oword", text)
    # Remove 'ptr' from size specifiers
    text = re.sub(
        r"\b(byte|word|dword|qword|tword|oword)\s+ptr\b", r"\1", text
    )
    # REX-prefix byte register names (sil, dil, bpl, spl, r8b-r15b) are
    # handled by .define directives in the .dasc header — keep them as-is
    # for readability.
    # Remove instruction suffixes (callq→call etc.)
    text = re.sub(r"\b(call|ret|push|pop|jmp)q\b", r"\1", text)
    # Convert negative byte immediates to unsigned (test byte [...], -128 → 0x80)
    text = _fix_negative_byte_imm(text)
    # DynASM requires explicit shift count: shr reg → shr reg, 1
    text = re.sub(
        r"\b(shr|shl|sar|sal|ror|rol|rcr|rcl)\s+(\w+)\s*$", r"\1 \2, 1", text
    )
    # DynASM requires register*scale, not scale*register in memory operands
    # e.g. [8*rcx] → [rcx*8], [rdi + 8*rax + 80] → [rdi + rax*8 + 80]
    text = re.sub(r"(\d+)\*(\w+)", _swap_scale_reg, text)
    # DynASM can't encode SIB-only addressing [reg*scale] without a base register.
    # Add explicit +0 displacement: [reg*N] → [reg*N+0]
    text = re.sub(r"\[(\w+\*\d+)\]", r"[\1+0]", text)
    # DynASM uses "movd" for both 32-bit and 64-bit GPR<->XMM transfers.
    # When the GPR is 64-bit (rax, rcx, etc.), DynASM infers REX.W from the
    # register name, producing the correct movq encoding.
    # E.g. "movq rax, xmm1" → "movd rax, xmm1"
    text = re.sub(r"\bmovq\s+(r\w+),\s*(xmm\d+)", r"movd \1, \2", text)
    text = re.sub(r"\bmovq\s+(xmm\d+),\s*(r\w+)", r"movd \1, \2", text)
    # Normalize whitespace
    text = " ".join(text.split())
    return text


def _fix_negative_byte_imm(text: str) -> str:
    """Convert negative immediates in byte operations to unsigned form.

    DynASM requires unsigned immediates for byte-sized operations.
    E.g. ``test byte [...], -128`` → ``test byte [...], 128``
    """
    m = re.match(r"^(.+\bbyte\b.+,\s*)(-\d+)\s*$", text)
    if m:
        val = int(m.group(2))
        if -128 <= val < 0:
            return f"{m.group(1)}{val & 0xFF}"
    return text


def _jit_expr(symbol: str, offset: int = 0) -> str:
    """C expression for a _JIT_* symbol value."""
    base = _JIT_SYMBOL_EXPR.get(symbol)
    if base is None:
        raise ValueError(f"Unknown _JIT_ symbol: {symbol}")
    if offset:
        return f"((uintptr_t)({base}) + {offset})"
    return f"(uintptr_t)({base})"


# ── Main conversion ────────────────────────────────────────────────────


def convert_stencil(opname: str, assembly: str, *, is_shim: bool = False) -> ConvertedStencil:
    """Convert one stencil's optimized Intel-syntax .s to DynASM lines.

    Internal branch targets use PC labels relative to ``label_base``.
    """
    lines: list[str] = []
    # Map local label name → internal index (0-based)
    local_map: dict[str, int] = {}
    data_items: list[DataItem] = []
    counter = 0

    in_rodata = False
    cur_data: DataItem | None = None

    def _local(name: str) -> int:
        nonlocal counter
        if name not in local_map:
            local_map[name] = counter
            counter += 1
        return local_map[name]

    def _flush_data():
        nonlocal cur_data
        if cur_data and cur_data.data:
            data_items.append(cur_data)
        cur_data = None

    # Special labels we handle separately (not local)
    _SPECIAL_LABELS = {
        "_JIT_ENTRY", "_JIT_CONTINUE", ".L_JIT_CONTINUE",
    }

    # First pass: discover all label definitions and branch/call targets
    # to determine which ones are local (internal to this stencil)
    all_label_defs: set[str] = set()
    all_branch_targets: set[str] = set()
    for raw in assembly.splitlines():
        if m := _RE_ANY_LABEL_DEF.match(raw):
            label = m.group(1)
            if label not in _SPECIAL_LABELS:
                all_label_defs.add(label)
        # Find branch targets (labels only, not register names)
        m_br = re.match(r"^\s*(j\w+|call)\s+([\w.]+)\s*(?:#.*)?$", raw)
        if m_br:
            target = m_br.group(2)
            if target not in _SPECIAL_LABELS and not target.startswith("_JIT_"):
                all_branch_targets.add(target)

    # Local labels are those with actual definitions in this stencil.
    # Only include branch targets that have matching definitions —
    # targets without definitions are register-indirect branches
    # (e.g., "call rax", "jmp r11") and not real label references.
    local_labels = all_label_defs
    for label in sorted(local_labels):
        _local(label)

    # Second pass: emit DynASM lines
    _FRAME_ANCHOR_MARKER = "    // __JIT_FRAME_ANCHOR__"
    cur_section = "code"  # track current section for data entry restoration
    for raw in assembly.splitlines():
        if _RE_BLANK.match(raw):
            continue
        if _RE_SKIP.match(raw):
            continue

        # ── rodata collection ──
        if _RE_RODATA_SECTION.match(raw):
            in_rodata = True
            continue

        if in_rodata:
            if _RE_TEXT_SECTION.match(raw) or _RE_COLD_SECTION.match(raw):
                _flush_data()
                in_rodata = False
                # fall through to handle section switch
            elif m := _RE_DATA_LABEL.match(raw):
                _flush_data()
                cur_data = DataItem(label=m.group(1))
                continue
            elif m := _RE_ASCIZ.match(raw):
                if cur_data is not None:
                    s = (
                        m.group(1)
                        .encode("raw_unicode_escape")
                        .decode("unicode_escape")
                    )
                    cur_data.data.extend(s.encode("utf-8"))
                    cur_data.data.append(0)
                continue
            elif m := _RE_BYTE_DATA.match(raw):
                if cur_data is not None:
                    kind = m.group(1)
                    for v in m.group(2).split(","):
                        # Strip inline comments (# ...)
                        v = re.sub(r"\s*#.*$", "", v).strip()
                        if not v:
                            continue
                        n = int(v, 0)
                        sz = {"byte": 1, "short": 2, "long": 4, "quad": 8}[kind]
                        cur_data.data.extend(
                            n.to_bytes(sz, "little", signed=(n < 0))
                        )
                continue
            else:
                continue  # skip unknown rodata lines

        # ── section switches ──
        if _RE_COLD_SECTION.match(raw):
            cur_section = "cold"
            lines.append("")
            lines.append("    // ---- cold path ----")
            lines.append("    |.cold")
            continue
        if _RE_TEXT_SECTION.match(raw):
            cur_section = "code"
            lines.append("    |.code")
            continue

        # ── special labels ──
        if _RE_ENTRY.match(raw):
            lines.append("    |=>uop_label:")
            continue
        if _RE_CONTINUE_LABEL.match(raw):
            continue  # handled by caller via continue_label

        # ── alignment ──
        if m := _RE_ALIGN.match(raw):
            lines.append(f"    | .align {1 << int(m.group(1))}")
            continue

        # ── local label definitions (any label we discovered in pass 1) ──
        if m := _RE_ANY_LABEL_DEF.match(raw):
            label = m.group(1)
            if label in local_map:
                idx = local_map[label]
                lines.append(f"    |=>L({idx}):")
                continue
            # Skip other label defs we don't recognize
            continue

        # ── LLVM JIT fold pass markers ──
        # Inline asm markers injected by jit_fold_pass.so:
        #   nop  # @@JIT_MOV_IMM %rax, <C-expression>@@
        # The register name has a % prefix from AT&T syntax in inline asm.
        if m := _RE_JIT_MARKER.match(raw):
            kind, reg, expr = m.groups()
            # Strip AT&T % prefix from register name.
            if reg is not None:
                reg = reg.lstrip("%")
            reg_name = _REG_IDX_NAME.get(reg.lower()) if reg else None
            if kind == "JIT_MOV_IMM":
                if reg_name is not None:
                    lines.append(
                        f"    emit_mov_imm_preserve_flags(Dst, {reg_name}, {expr});"
                    )
                else:
                    lines.append(f"    | mov64 {reg}, {expr}")
            elif kind == "JIT_TEST":
                if reg_name is not None:
                    lines.append(
                        f"    emit_test_reg_imm(Dst, {reg_name}, JREG_RAX, {expr});"
                    )
            elif kind == "JIT_CMP":
                if reg_name is not None:
                    lines.append(
                        f"    emit_cmp_reg_imm(Dst, {reg_name}, JREG_RAX, {expr});"
                    )
            elif kind == "JIT_FRAME_ANCHOR":
                if reg is not None and lines:
                    anchor_re = re.compile(
                        rf"^\s*\|\s*(?:"
                        rf"lea\s+{re.escape(reg)},\s*\[(?:rbp|rsp)(?:\s*[+-]\s*\d+)?\]"
                        rf"|mov\s+{re.escape(reg)},\s*(?:rbp|rsp)"
                        rf")\s*$"
                    )
                    if anchor_re.match(lines[-1]):
                        lines.pop()
                lines.append(_FRAME_ANCHOR_MARKER)
            _stat("LLVM_fold_marker")
            continue

        # ── movabs REG, offset SYMBOL[+N] ──
        if m := _RE_MOVABS.match(raw):
            reg, sym, off_s = m.groups()
            off = int(off_s) if off_s else 0
            if sym.startswith("_JIT_"):
                expr = _jit_expr(sym, off)
                reg_name = _REG_IDX_NAME.get(reg.lower())
                if reg_name is not None:
                    # Use emit_mov_imm which picks optimal encoding at
                    # JIT compile time (xor for 0, mov32 for ≤UINT32_MAX,
                    # mov64 otherwise).
                    lines.append(f"    emit_mov_imm(Dst, {reg_name}, {expr});")
                else:
                    lines.append(f"    | mov64 {reg}, {expr}")
            elif sym.startswith(".L"):
                safe = sym.replace(".", "_")
                reg_name = _REG_IDX_NAME.get(reg.lower())
                if reg_name is not None:
                    lines.append(
                        f"    emit_mov_imm(Dst, {reg_name}, (uintptr_t)jit_data_{opname}_{safe});"
                    )
                else:
                    lines.append(
                        f"    | mov64 {reg}, (uintptr_t)jit_data_{opname}_{safe}"
                    )
            else:
                # External symbol: load address via emit_mov_imm which
                # picks the optimal encoding at JIT time (xor/mov32/lea/mov64).
                expr = (
                    f"((uintptr_t)&{sym} + {off})"
                    if off
                    else f"(uintptr_t)&{sym}"
                )
                reg_name = _REG_IDX_NAME.get(reg.lower())
                if reg_name is not None:
                    lines.append(f"    emit_mov_imm(Dst, {reg_name}, {expr});")
                else:
                    lines.append(f"    | mov64 {reg}, {expr}")
            continue

        # ── movabs REG, IMM (plain integer) ──
        if m := _RE_MOVABS_IMM.match(raw):
            reg, imm = m.groups()
            # Use unsigned form for mov64
            val = int(imm)
            if val < 0:
                val = val & 0xFFFFFFFFFFFFFFFF
            lines.append(f"    | mov64 {reg}, {val}ULL")
            continue

        # ── call/jmp via GOTPCREL ──
        if m := _RE_GOTPCREL_CALL.match(raw):
            instr, sym = m.groups()
            if sym.startswith("_JIT_"):
                # _JIT_* symbols are runtime values — emit optimal mov then
                # indirect call/jmp through rax.
                expr = _jit_expr(sym)
                lines.append(f"    emit_mov_imm(Dst, JREG_RAX, {expr});")
                lines.append(f"    | {instr} rax")
            else:
                # External function: emit a direct relative call using
                # DynASM's &addr syntax (5-byte E8 rel32).  Like Pyston's
                # emit_call_ext_func.  Falls back to mov64+call for
                # targets beyond ±2GB.
                lines.append(f"    emit_call_ext(Dst, (void *)&{sym});")
                if instr == "jmp":
                    # Tail call: after the callee returns, we need to exit
                    # the trace.  Emit a ret which the epilogue rewriting
                    # will convert to jmp =>cleanup_ret_label.
                    lines.append("    | ret")
            continue

        # ── generic instruction with GOTPCREL memory operand ──
        if m := _RE_GOTPCREL_MEM.match(raw):
            prefix, before, size, sym, after = m.groups()
            instr = prefix.strip().split()[0]
            dest = before.strip().rstrip(",").strip()

            if instr == "mov" and not after.strip():
                # mov REG, qword ptr [rip + SYM@GOTPCREL]
                # Load the symbol address directly via emit_mov_imm.
                if sym.startswith("_JIT_"):
                    expr = _jit_expr(sym)
                else:
                    expr = f"(uintptr_t)&{sym}"
                reg_name = _ANY_REG_TO_NAME.get(dest.lower())
                if reg_name is not None:
                    lines.append(f"    emit_mov_imm(Dst, {reg_name}, {expr});")
                else:
                    lines.append(f"    | mov64 {dest}, {expr}")
                continue

            if instr == "movzx" and not after.strip():
                # movzx REG, word/byte ptr [rip + SYM@GOTPCREL]
                # Load the symbol value with appropriate truncation.
                if sym.startswith("_JIT_"):
                    expr = _jit_expr(sym)
                else:
                    expr = f"(uintptr_t)&{sym}"
                _SIZE_CAST = {"word": "uint16_t", "byte": "uint8_t"}
                cast = _SIZE_CAST.get(size, None)
                if cast:
                    expr = f"({cast})({expr})"
                reg_name = _ANY_REG_TO_NAME.get(dest.lower())
                if reg_name is not None:
                    lines.append(f"    emit_mov_imm(Dst, {reg_name}, {expr});")
                else:
                    lines.append(f"    | mov64 {dest}, {expr}")
                continue

            if instr == "call":
                # External function: direct relative call via emit_call_ext.
                if not sym.startswith("_JIT_"):
                    lines.append(f"    emit_call_ext(Dst, (void *)&{sym});")
                else:
                    expr = _jit_expr(sym)
                    lines.append(f"    emit_mov_imm(Dst, JREG_RAX, {expr});")
                    lines.append(f"    | call rax")
                continue

            # Other instructions (cmp, test, etc.) with GOTPCREL memory.
            # Emit the symbol address into a per-instruction data section
            # entry, then reference it with a RIP-relative memory operand.
            if sym.startswith("_JIT_"):
                expr = _jit_expr(sym)
            else:
                expr = f"(uintptr_t)&{sym}"
            data_label = counter
            counter += 1
            lines.append(f"    |.data")
            lines.append(f"    |=>L({data_label}):")
            lines.append(f"    | .qword {expr}")
            lines.append(f"    |.{cur_section}")
            new_line = f"{prefix}{before}qword [=>L({data_label})]{after}"
            new_line = _fix_syntax(new_line.strip())
            lines.append(f"    | {new_line}")
            continue

        # ── JIT branch targets ──
        if m := _RE_JIT_BRANCH.match(raw):
            instr, target = m.groups()
            field = (
                "jump_target"
                if target == "_JIT_JUMP_TARGET"
                else "error_target"
            )
            lines.append(f"    | {instr} =>instruction->{field}")
            continue

        # ── JIT continue ──
        if m := _RE_JIT_CONTINUE.match(raw):
            lines.append(f"    | {m.group(1)} =>continue_label")
            continue

        # ── local branches / calls ──
        # Match jmp/jcc/call to a label we discovered in pass 1
        m_br = re.match(r"^\s*(j\w+|call)\s+([\w.]+)\s*(?:#.*)?$", raw)
        if m_br:
            instr, label = m_br.groups()
            if label in local_map:
                idx = local_map[label]
                if instr == "call":
                    lines.append(f"    | call =>L({idx})")
                else:
                    lines.append(f"    | {instr} =>L({idx})")
                continue

        # ── default: plain instruction ──
        stripped = raw.strip()
        if not stripped or stripped.startswith(".section"):
            continue

        fixed = _fix_syntax(stripped)
        if "@" in fixed:
            raise ValueError(
                f"Unhandled @ symbol in stencil {opname}: {raw!r}"
            )
        lines.append(f"    | {fixed}")

    _flush_data()

    # Strip the frame-anchor marker emitted by template.c.
    # With the simplified approach (no ForceFrameCall), the marker stands alone.
    stripped_lines: list[str] = []
    for line in lines:
        if line == _FRAME_ANCHOR_MARKER:
            continue
        stripped_lines.append(line)
    lines = stripped_lines

    # Find the FIRST |.cold (the hot-cold boundary). There may be multiple
    # |.cold directives (e.g. when the optimizer appends cold blocks after
    # LLVM's own cold section), but the first one marks the end of hot code.
    hot_end = len(lines)
    for i in range(len(lines)):
        stripped = lines[i].strip()
        if stripped == "|.cold":
            hot_end = i
            break

    # Strip the stencil's outer function prologue/epilogue and record its
    # requested frame size. The shared JIT shim recreates one canonical
    # frame before calling into the trace, so individual stencils no longer
    # need their function entry/exit stack manipulation.
    frame_size = 0
    _RE_PUSH_RBP = re.compile(r"^\s*\|\s*push rbp\s*$")
    _RE_MOV_RBP_RSP = re.compile(r"^\s*\|\s*mov rbp, rsp\s*$")
    _RE_SUB_RSP = re.compile(r"^\s*\|\s*sub rsp,\s*(\d+)\s*$")
    _RE_ADD_RSP = re.compile(r"^\s*\|\s*add rsp,\s*(\d+)\s*$")
    _RE_LEA_RSP = re.compile(r"^\s*\|\s*lea rsp, \[rsp \+ (\d+)\]\s*$")
    _RE_POP_RBP = re.compile(r"^\s*\|\s*pop rbp\s*$")

    # Detect entry prologue: |=>uop_label: then | push rbp, an optional
    # | mov rbp, rsp, and finally | sub rsp, N.
    prologue_push_idx = -1
    prologue_mov_idx = -1
    prologue_sub_idx = -1
    for i in range(len(lines)):
        if lines[i].strip() == "|=>uop_label:":
            if i + 2 < len(lines) and _RE_PUSH_RBP.match(lines[i + 1]):
                prologue_push_idx = i + 1
                sub_idx = i + 2
                if sub_idx < len(lines) and _RE_MOV_RBP_RSP.match(
                    lines[sub_idx]
                ):
                    prologue_mov_idx = sub_idx
                    sub_idx += 1
                if sub_idx < len(lines) and (
                    m_sub := _RE_SUB_RSP.match(lines[sub_idx])
                ):
                    frame_size = int(m_sub.group(1))
                    prologue_sub_idx = sub_idx
            break

    total_push_rbp = sum(1 for line in lines if _RE_PUSH_RBP.match(line))

    if frame_size > 0 and prologue_push_idx >= 0 and prologue_sub_idx >= 0:
        lines[prologue_push_idx] = ""
        if prologue_mov_idx >= 0:
            lines[prologue_mov_idx] = ""
        lines[prologue_sub_idx] = ""

        # When the entry prologue is the only push rbp in the stencil, every
        # pop rbp belongs to the outer function epilogue even if the compiler
        # hoists the shared add rsp, N before a branch. Strip all such outer
        # unwinds and let the shim own the frame instead.
        if total_push_rbp == 1:
            for i in range(len(lines)):
                if _RE_POP_RBP.match(lines[i]):
                    lines[i] = ""
                    continue
                m_add = _RE_ADD_RSP.match(lines[i]) or _RE_LEA_RSP.match(
                    lines[i]
                )
                if m_add and int(m_add.group(1)) == frame_size:
                    lines[i] = ""
        else:
            # Fall back to the conservative adjacent add/lea + pop pattern
            # when the stencil contains other push rbp saves of its own.
            stripped_any_epilogue = False
            for i in range(len(lines) - 1):
                if not _RE_POP_RBP.match(lines[i + 1]):
                    continue
                m_add = _RE_ADD_RSP.match(lines[i]) or _RE_LEA_RSP.match(
                    lines[i]
                )
                if m_add and int(m_add.group(1)) == frame_size:
                    stripped_any_epilogue = True
                    lines[i] = ""
                    lines[i + 1] = ""
            if not stripped_any_epilogue:
                frame_size = 0
    else:
        frame_size = 0

    # Inline trace exit sequences: tear down the shared trace frame
    # (mov rsp, rbp; pop rbp) and then exit via the original instruction.
    # The shim has its own prologue/epilogue and must not be rewritten.
    if not is_shim:
        _TRACE_EXIT_REWRITES = {
            "| ret": [
                "    | mov rsp, rbp",
                "    | pop rbp",
                "    | ret",
            ],
            "| jmp rax": [
                "    | mov rsp, rbp",
                "    | pop rbp",
                "    | jmp rax",
            ],
            "| jmp rcx": [
                "    | mov rsp, rbp",
                "    | pop rbp",
                "    | jmp rcx",
            ],
            "| jmp qword [rax + 48]": [
                "    | mov rsp, rbp",
                "    | pop rbp",
                "    | jmp qword [rax + 48]",
            ],
        }
        has_internal_push_rbp = total_push_rbp > 1 and frame_size > 0
        # In stencils with internal push/pop rbp pairs (subroutines like
        # _Py_Dealloc called via `call =>L(N)`), exit instructions reachable
        # after a `pop rbp` within the same basic block are internal subroutine
        # exits, not trace exits.  Track per-block state to skip rewriting.
        _RE_LABEL_LINE = re.compile(r"^\s*\|=>")
        in_internal_exit_block = False
        new_lines = []
        for line in lines:
            stripped = line.strip()
            if has_internal_push_rbp:
                if _RE_LABEL_LINE.match(stripped):
                    in_internal_exit_block = False
                if _RE_POP_RBP.match(line):
                    in_internal_exit_block = True
                if in_internal_exit_block and stripped in _TRACE_EXIT_REWRITES:
                    new_lines.append(line)
                    continue
            replacement = _TRACE_EXIT_REWRITES.get(stripped)
            if replacement is not None:
                new_lines.extend(replacement)
            else:
                new_lines.append(line)
        lines = new_lines

    # Apply peephole optimizations (fuse emit_mov_imm + movzx/or)
    lines = _peephole_optimize(lines)

    # Eliminate trampoline jumps: when a label contains only a single
    # `jmp` to another target, rewrite all branches to that label to
    # jump directly to the final target.
    _RE_LABEL_DEF = re.compile(r"^\s*\|=>L\((\d+)\):\s*$")
    _RE_TRAMPOLINE_JMP = re.compile(
        r"^\s*\|\s*jmp\s+=>(instruction->(?:jump_target|error_target)"
        r"|L\(\d+\)|continue_label)\s*$"
    )
    _RE_BRANCH_TO_LABEL = re.compile(
        r"^(\s*\|\s*j\w+)\s+=>(L\(\d+\))\s*$"
    )
    # Pass 1: find trampoline labels (label followed immediately by lone jmp)
    trampoline_targets: dict[str, str] = {}  # "L(N)" → target
    for i in range(len(lines) - 1):
        m_label = _RE_LABEL_DEF.match(lines[i])
        if not m_label:
            continue
        # Skip blank/comment/section lines to find the next real instruction
        j = i + 1
        while j < len(lines) and (
            not lines[j].strip()
            or lines[j].strip().startswith("//")
        ):
            j += 1
        if j >= len(lines):
            continue
        m_jmp = _RE_TRAMPOLINE_JMP.match(lines[j])
        if not m_jmp:
            continue
        label_name = f"L({m_label.group(1)})"
        trampoline_targets[label_name] = m_jmp.group(1)

    # Resolve chains: L(8) → L(1) → instruction->jump_target
    changed = True
    while changed:
        changed = False
        for src, dst in list(trampoline_targets.items()):
            if dst in trampoline_targets:
                trampoline_targets[src] = trampoline_targets[dst]
                changed = True

    if trampoline_targets:
        # Collect line indices of trampoline labels and their jmp targets
        dead_lines: set[int] = set()
        for i in range(len(lines)):
            m_label = _RE_LABEL_DEF.match(lines[i])
            if not m_label:
                continue
            label_name = f"L({m_label.group(1)})"
            if label_name not in trampoline_targets:
                continue
            dead_lines.add(i)
            # Find and mark the jmp line (skipping blanks/comments)
            for j in range(i + 1, len(lines)):
                if _RE_TRAMPOLINE_JMP.match(lines[j]):
                    dead_lines.add(j)
                    break
                if lines[j].strip() and not lines[j].strip().startswith("//"):
                    break

        new_lines = []
        for i, line in enumerate(lines):
            if i in dead_lines:
                continue
            # Rewrite branches to trampolines → direct branches
            m_branch = _RE_BRANCH_TO_LABEL.match(line)
            if m_branch:
                target_label = m_branch.group(2)
                if target_label in trampoline_targets:
                    branch_instr = m_branch.group(1)
                    new_lines.append(
                        f"{branch_instr} =>{trampoline_targets[target_label]}"
                    )
                    continue
            new_lines.append(line)
        lines = new_lines

    # Remove blank lines and trailing whitespace from the output (clang emits
    # blank lines between basic blocks; some peephole patterns add trailing \n).
    lines = [l.rstrip() for l in lines if l.strip()]

    return ConvertedStencil(
        opname=opname,
        lines=lines,
        num_internal_labels=counter,
        data_items=data_items,
        frame_size=frame_size,
    )
