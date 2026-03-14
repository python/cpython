"""x86-64 specific peephole optimizations for the DynASM JIT backend.

This module contains all x86-64 architecture-specific code for the peephole
optimizer:

- JIT calling convention (register roles, frame offsets)
- Instruction effect analysis (which registers/flags each instruction touches)
- Peephole optimization patterns (23 patterns, organized by category)
- Pattern registry

Architecture-generic infrastructure (types, parsing, pass management, dead
label elimination) lives in ``_asm_to_dasc.py``.  A future ARM64 backend would
create ``_asm_to_dasc_aarch64.py`` following the same structure.
"""

from __future__ import annotations

import typing

from _asm_to_dasc import (
    # Operand types
    Reg, Mem, Imm, Op,
    # Line types
    Asm, CCall, CCallKind, Label, Section, FuncDef, Blank, CCode, Line,
    # Infrastructure
    _Match, _FoldCtx, _PeepholeState, _PeepholePass,
    _PeepholeProgram, _LineEffect,
    # Functions
    parse_line, parse_lines,
    is_call, is_branch, is_jump, branch_target, line_raw, fmt_op,
    _stat,
    # Data
    _ANY_REG_TO_IDX, _ANY_REG_TO_NAME, _IDX_TO_ALL_NAMES,
    _REG_IDX_NAME, _REG64_TO_REG32, _BRANCH_MNEMONICS, _RE_EMIT_MOV_IMM,
    # Helpers
    _operand_regs, _mem_uses_reg, _reg_bits,
)

# ── JIT calling convention: register roles ──────────────────────────────
# The preserve_none calling convention assigns these fixed roles:
#   r13 = frame pointer (_PyInterpreterFrame *)
#   r14 = cached stack pointer (frame->stackpointer)
#   r15 = thread state (PyThreadState *)
#   r12 = current executor (_PyExecutorObject *)
# These constants are used by the peephole optimizer to recognize
# store/reload patterns without hardcoding register names.
REG_FRAME = "r13"       # _PyInterpreterFrame *frame
REG_STACK_PTR = "r14"   # _PyStackRef *stack_pointer (cached)
REG_TSTATE = "r15"      # PyThreadState *tstate
REG_EXECUTOR = "r12"    # _PyExecutorObject *executor

# Frame struct field offsets (from _PyInterpreterFrame in
# Include/internal/pycore_interpframe_structs.h).
# Used by the peephole optimizer to match store/reload patterns.
FRAME_IP_OFFSET = 56          # offsetof(_PyInterpreterFrame, instr_ptr)
FRAME_STACKPOINTER_OFFSET = 64  # offsetof(_PyInterpreterFrame, stackpointer)


class _C:
    """Constants usable as value patterns in match/case statements.

    In Python structural pattern matching, only dotted names (like ``_C.SP``)
    are treated as value patterns that compare against the constant.  Simple
    names (like ``REG_STACK_PTR``) would be treated as capture patterns.
    """
    FRAME = REG_FRAME                       # "r13"
    SP = REG_STACK_PTR                      # "r14"
    TSTATE = REG_TSTATE                     # "r15"
    EXECUTOR = REG_EXECUTOR                 # "r12"
    FRAME_SP_OFS = FRAME_STACKPOINTER_OFFSET  # 64
    FRAME_IP_OFS = FRAME_IP_OFFSET            # 56

# Derived patterns used by store/reload elimination.
# Store: mov qword [r13 + 64], r14  (frame->stackpointer = stack_pointer)
# Reload: mov r14, qword [r13 + 64] (stack_pointer = frame->stackpointer)
_SP_STORE = f"| mov qword [{REG_FRAME} + {FRAME_STACKPOINTER_OFFSET}], {REG_STACK_PTR}"
_SP_RELOAD = f"| mov {REG_STACK_PTR}, qword [{REG_FRAME} + {FRAME_STACKPOINTER_OFFSET}]"
_SP_RELOAD_LINE = f"    {_SP_RELOAD}\n"

# ── JREG name ↔ index mapping (used by emit_mov_imm patterns) ─────────

_JREG_TO_IDX: dict[str, int] = {
    "JREG_RAX": 0, "JREG_RCX": 1, "JREG_RDX": 2, "JREG_RBX": 3,
    "JREG_RSP": 4, "JREG_RBP": 5, "JREG_RSI": 6, "JREG_RDI": 7,
    "JREG_R8": 8, "JREG_R9": 9, "JREG_R10": 10, "JREG_R11": 11,
    "JREG_R12": 12, "JREG_R13": 13, "JREG_R14": 14, "JREG_R15": 15,
}
_IDX_TO_JREG: dict[int, str] = {v: k for k, v in _JREG_TO_IDX.items()}

# ── SysV ABI register classification ────────────────────────────────────
# Used by _is_dead_before_any_call to determine how opaque calls
# (emit_call_ext / | call) interact with each register.

# Callee-saved registers: preserved across function calls.
_CALLEE_SAVED_REGS = frozenset({3, 5, 12, 13, 14, 15})  # rbx rbp r12 r13 r14 r15

# SysV integer argument registers: may be read by function calls.
_SYSV_ARGUMENT_REGS = frozenset({7, 6, 2, 1, 8, 9})  # rdi rsi rdx rcx r8 r9

# Caller-saved registers: clobbered by function calls (SysV ABI).
_CALLER_SAVED_REGS = frozenset({0, 1, 2, 6, 7, 8, 9, 10, 11})
# rax rcx rdx rsi rdi r8 r9 r10 r11


def _parse_jreg(token: str) -> tuple[int, str]:
    """Parse a JREG_* name or integer → (index, name)."""
    if token in _JREG_TO_IDX:
        return _JREG_TO_IDX[token], token
    idx = int(token)
    return idx, _IDX_TO_JREG.get(idx, str(idx))


def _jreg_arg_index(token: str) -> int | None:
    """Best-effort parse of a helper argument naming a JIT register."""
    token = token.strip()
    if token in _JREG_TO_IDX:
        return _JREG_TO_IDX[token]
    try:
        return int(token, 0)
    except ValueError:
        return None


def _reg_write_sets(reg: Reg) -> tuple[frozenset[int], frozenset[int]]:
    """Return (full_writes, partial_writes) for a register destination."""
    if reg.idx is None:
        return frozenset(), frozenset()
    if reg.bits >= 32:
        return frozenset({reg.idx}), frozenset()
    return frozenset(), frozenset({reg.idx})


def _line_effect(line: Line) -> _LineEffect:
    """Summarize register and flags effects for one parsed line."""
    match line:
        case Blank() | Label() | Section() | FuncDef():
            return _LineEffect()
        case CCode():
            return _LineEffect()
        case CCall(kind=CCallKind.MOV_IMM, argv=argv):
            idx = _jreg_arg_index(argv[0]) if argv else None
            return _LineEffect(
                full_writes=frozenset({idx})
                if idx is not None
                else frozenset()
            )
        case CCall(kind=CCallKind.CALL_EXT):
            # Model the emitted call as clobbering all caller-saved regs
            # per SysV ABI.  We intentionally do NOT model argument registers
            # as reads: the peephole patterns never eliminate register writes
            # that feed call arguments (they only fold emit_mov_imm into
            # memory addressing or ALU patterns), so the risk is negligible.
            return _LineEffect(
                full_writes=_CALLER_SAVED_REGS,
                writes_flags=True,
            )
        case CCall(kind=CCallKind.CMP_REG_IMM, argv=argv):
            reads = frozenset({_jreg_arg_index(argv[0])}) if len(argv) >= 1 else frozenset()
            scratch = _jreg_arg_index(argv[1]) if len(argv) >= 2 else None
            return _LineEffect(
                reads=frozenset(idx for idx in reads if idx is not None),
                full_writes=frozenset({scratch}) if scratch is not None else frozenset(),
                writes_flags=True,
            )
        case CCall(kind=CCallKind.CMP_MEM64_IMM, argv=argv):
            base = _jreg_arg_index(argv[0]) if len(argv) >= 1 else None
            scratch = _jreg_arg_index(argv[2]) if len(argv) >= 3 else None
            return _LineEffect(
                reads=frozenset({base}) if base is not None else frozenset(),
                full_writes=frozenset({scratch}) if scratch is not None else frozenset(),
                writes_flags=True,
            )
        case CCall(kind=CCallKind.ALU_REG_IMM, helper=helper, argv=argv):
            reg_idx = _jreg_arg_index(argv[0]) if len(argv) >= 1 else None
            scratch = _jreg_arg_index(argv[1]) if len(argv) >= 2 else None
            reads = frozenset({reg_idx}) if reg_idx is not None else frozenset()
            full_writes = set()
            if scratch is not None:
                full_writes.add(scratch)
            if helper in {"emit_and_reg_imm", "emit_or_reg_imm", "emit_xor_reg_imm",
                         "emit_add_reg_imm", "emit_sub_reg_imm"}:
                if reg_idx is not None:
                    full_writes.add(reg_idx)
            return _LineEffect(
                reads=reads,
                full_writes=frozenset(full_writes),
                writes_flags=True,
            )
        case Asm(mnemonic=mnemonic, dst=dst, src=src):
            reads = set(_operand_regs(src))
            full_writes: set[int] = set()
            partial_writes: set[int] = set()
            uses_flags = False
            writes_flags = False

            match mnemonic:
                case "jmp" | "ret":
                    return _LineEffect()
                case _ if is_branch(line):
                    return _LineEffect(uses_flags=True)
                case "call":
                    # Reads the target operand (address computation),
                    # clobbers all caller-saved registers per SysV ABI.
                    reads |= set(_operand_regs(dst))
                    return _LineEffect(
                        reads=frozenset(reads),
                        full_writes=_CALLER_SAVED_REGS,
                        writes_flags=True,
                    )
                case _ if mnemonic.startswith("set"):
                    uses_flags = True
                    if isinstance(dst, Reg):
                        full, partial = _reg_write_sets(dst)
                        full_writes |= full
                        partial_writes |= partial
                case _ if mnemonic.startswith("cmov"):
                    uses_flags = True
                    reads |= set(_operand_regs(dst))
                    reads |= set(_operand_regs(src))
                    if isinstance(dst, Reg):
                        full, partial = _reg_write_sets(dst)
                        full_writes |= full
                        partial_writes |= partial
                case "mov":
                    if isinstance(dst, Mem):
                        reads |= set(_operand_regs(dst))
                    elif isinstance(dst, Reg):
                        full, partial = _reg_write_sets(dst)
                        full_writes |= full
                        partial_writes |= partial
                case "movzx" | "movsxd" | "lea":
                    if isinstance(dst, Reg):
                        full, partial = _reg_write_sets(dst)
                        full_writes |= full
                        partial_writes |= partial
                    reads |= set(_operand_regs(src))
                case "cmp" | "test" | "bt" | "ucomisd":
                    reads |= set(_operand_regs(dst))
                    reads |= set(_operand_regs(src))
                    writes_flags = True
                case "pop":
                    # pop writes to the destination register (from stack),
                    # it does NOT read the register.
                    if isinstance(dst, Reg):
                        full, partial = _reg_write_sets(dst)
                        full_writes |= full
                        partial_writes |= partial
                case "push":
                    # push reads the source register to put it on the stack,
                    # but does not write to any GP register.
                    reads |= set(_operand_regs(dst))
                case "neg" | "not" | "inc" | "dec":
                    reads |= set(_operand_regs(dst))
                    if isinstance(dst, Reg):
                        full, partial = _reg_write_sets(dst)
                        full_writes |= full
                        partial_writes |= partial
                    writes_flags = True
                case "xor" if (
                    isinstance(dst, Reg)
                    and isinstance(src, Reg)
                    and (dst.idx is not None and dst.idx == src.idx)
                ):
                    # xor reg, reg is a zeroing idiom — no read dependency.
                    reads.discard(dst.idx)
                    full, partial = _reg_write_sets(dst)
                    full_writes |= full
                    partial_writes |= partial
                    writes_flags = True
                case _:
                    reads |= set(_operand_regs(dst))
                    reads |= set(_operand_regs(src))
                    if isinstance(dst, Reg):
                        full, partial = _reg_write_sets(dst)
                        full_writes |= full
                        partial_writes |= partial
                    writes_flags = mnemonic in {
                        "add", "and", "or", "sub", "xor", "shl", "shr",
                        "sar", "sal", "rol", "ror", "rcl", "rcr",
                    }

            return _LineEffect(
                reads=frozenset(reads),
                full_writes=frozenset(full_writes),
                partial_writes=frozenset(partial_writes),
                uses_flags=uses_flags,
                writes_flags=writes_flags,
            )
    return _LineEffect()


def uses_reg(line: Line, reg_idx: int) -> bool:
    """Does this line reference the given register (by index)?

    Uses structured operand/helper effects where possible, then falls back to
    the raw text for unclassified C lines.
    """
    effect = _line_effect(line)
    if (
        reg_idx in effect.reads
        or reg_idx in effect.full_writes
        or reg_idx in effect.partial_writes
    ):
        return True
    names = _IDX_TO_ALL_NAMES.get(reg_idx, set())
    raw = line.raw
    return any(name in raw for name in names)


def is_store_sp(line: Line) -> bool:
    """Matches: ``| mov qword [r13 + 64], r14`` (store stack pointer)."""
    return line.raw.strip() == _SP_STORE


def is_reload_sp(line: Line) -> bool:
    """Matches: ``| mov r14, qword [r13 + 64]`` (reload stack pointer)."""
    return line.raw.strip() == _SP_RELOAD


def _preserve_flags_mov_imm(line: str) -> str:
    """Rewrite emit_mov_imm(...) to emit_mov_imm_preserve_flags(...)."""
    return line.replace("emit_mov_imm(", "emit_mov_imm_preserve_flags(", 1)


def _is_flag_writer(line: Line) -> bool:
    """Does this instruction define flags for a later consumer?"""
    return _line_effect(line).writes_flags


def _is_flag_consumer(line: Line) -> bool:
    """Does this instruction consume previously computed flags?"""
    return _line_effect(line).uses_flags


def _parse_emit_mov_imm_call(line: str) -> tuple[int, str, str] | None:
    """Parse an emit_mov_imm* helper call into (reg_idx, reg_name, expr)."""
    match parse_line(line):
        case CCall(kind=CCallKind.MOV_IMM, argv=(reg, expr, *_)):
            reg_idx, reg_name = _parse_jreg(reg)
            return reg_idx, reg_name, expr
    return None


# Map jcc mnemonic → C comparison operator for unsigned cmp folding.
# Given "cmp REG, IMM; jcc label", the branch is taken when REG <op> IMM.


def _reg_dead_after(
    program_or_lines: _PeepholeProgram | list[str],
    start: int,
    reg_idx: int,
) -> bool:
    """Control-flow-aware deadness query backed by structured effects."""
    if isinstance(program_or_lines, _PeepholeProgram):
        program = program_or_lines
    else:
        program = _PeepholeProgram.from_lines(program_or_lines)
    return program.reg_dead_after(start, reg_idx)


def _is_dead_before_any_call(
    program: _PeepholeProgram,
    start: int,
    reg_idx: int,
) -> bool:
    """Check that *reg_idx* is dead AND safe to eliminate.

    This performs a CFG-aware scan from *start* verifying that on EVERY
    reachable path, *reg_idx* is fully overwritten before:
      1. an opaque call (``emit_call_ext`` / ``| call``) that might read
         it — only relevant for SysV argument registers (rdi, rsi, rdx,
         rcx, r8, r9), OR
      2. the end of the function or a ``.cold`` section boundary.

    Callee-saved registers (rbx, rbp, r12–r15) are preserved across calls
    by the SysV ABI, so calls are transparent for them.  Caller-saved
    non-argument registers (rax, r10, r11) are clobbered by calls, which
    counts as a full write.

    Condition (2) is needed because in the JIT stencil system, registers
    that are live at the end of a stencil function are inter-stencil
    outputs consumed by the next stencil.  DynASM ``.cold`` sections are
    placed in a separate memory region, so linear fallthrough across a
    section switch does not exist at runtime.
    """
    if not program.reg_dead_after(start, reg_idx):
        return False

    # Callee-saved registers are preserved across calls.
    callee_saved = reg_idx in _CALLEE_SAVED_REGS
    # Argument registers may be read by calls as function arguments.
    is_argument_reg = reg_idx in _SYSV_ARGUMENT_REGS
    # Other caller-saved regs (rax, r10, r11) are clobbered by calls.

    func_end = program.function_end_by_line[start]
    start_depth = program.c_depth[start]
    parsed = program.parsed
    visited: set[int] = set()
    worklist = [start + 1]

    while worklist:
        pos = worklist.pop()
        while pos < func_end:
            if pos in visited:
                break
            visited.add(pos)
            pj = parsed[pos]

            is_opaque_call = (
                isinstance(pj, CCall) and pj.kind == CCallKind.CALL_EXT
            ) or (isinstance(pj, Asm) and is_call(pj))

            if is_opaque_call:
                if is_argument_reg:
                    # Call might read this register — unsafe.
                    return False
                if not callee_saved:
                    # Caller-saved non-argument reg (rax, r10, r11):
                    # the call clobbers it → effectively a full write.
                    break
                # Callee-saved: call preserves it, continue scanning.

            # .cold section boundary — treat as function end.  The
            # register hasn't been written on the hot path so it may
            # be an inter-stencil output.  (.code sections are benign;
            # they just confirm we are already in the hot section.)
            if isinstance(pj, Section) and pj.name == "cold":
                return False

            # R1 fully overwritten — this path is safe
            eff = program.effects[pos]
            if reg_idx in eff.full_writes:
                break
            # Follow the same successor logic as reg_dead_after
            successors = program.successors(pos, start_depth)
            if not successors:
                # No successors and no write — R1 reaches function end
                # alive, so it may be an inter-stencil output register.
                return False
            fallthrough = pos + 1
            if len(successors) == 1 and successors[0] == fallthrough:
                pos = fallthrough
                continue
            for succ in successors[1:]:
                if succ < func_end:
                    worklist.append(succ)
            pos = successors[0]
        else:
            # Inner while exited because pos >= func_end without a
            # full_write — the register reaches function end alive.
            return False

    return True


# ── emit_mov_imm chain patterns ───────────────────────────────────────
#
# These patterns all start from a parsed emit_mov_imm line and attempt
# to fold subsequent instructions.  They share (expr, consumed, src_idx)
# state and compose in sequence: Pattern 1 can modify expr, then Pattern
# 2 refines it further, etc.


def _try_indexed_mem(ctx: _FoldCtx) -> _Match | None:
    """Pattern 6: Fold indexed memory loads/stores with computed index.

    When emit_mov_imm loads a value that's used as an index in a
    memory access [base + REG*scale + disp], precompute the offset
    at JIT compile time.  This eliminates the index register and the
    scaled addressing mode, replacing it with a simple [base + const].

    Example — index into PyObject array:
        emit_mov_imm(Dst, JREG_RCX, instruction->oparg);
        | mov rax, qword [rbx + rcx*8 + 48]
      →
        | mov rax, qword [rbx + ((int)(instruction->oparg) * 8 + 48)]

    Multiple consecutive accesses using the same index are all folded:
        emit_mov_imm(Dst, JREG_RCX, instruction->oparg);
        | mov rax, qword [r14 + rcx*8 + 0]
        | mov rdx, qword [r14 + rcx*8 + 8]
      →
        | mov rax, qword [r14 + ((int)(instruction->oparg) * 8 + 0)]
        | mov rdx, qword [r14 + ((int)(instruction->oparg) * 8 + 8)]

    Safety: the index register must be either overwritten by the load
    destination or dead after all folded accesses.
    """
    lines, i, src_idx, expr = ctx.lines, ctx.i, ctx.src_idx, ctx.expr
    parsed = ctx.parsed
    folded: list[str] = []
    scan = i
    while scan < len(lines):
        p = parsed[scan]
        # Match instructions with indexed memory operand containing our reg
        mem_op: Mem | None = None
        is_load = False
        match p:
            # Load: | mov rax, qword [base + idx*scale + disp]
            # Exclude LEA (handled by P7a/P7b).
            case Asm(mnemonic=mn, dst=Reg(), src=Mem(index=idx, scale=sc)) if (
                mn != "lea" and idx and sc and Reg(idx).idx == src_idx
            ):
                mem_op = p.src
                is_load = True
            # Store: | mov qword [base + idx*scale + disp], reg
            case Asm(
                mnemonic="mov", dst=Mem(index=idx, scale=sc), src=Reg()
            ) if idx and sc and Reg(idx).idx == src_idx:
                mem_op = p.dst
                is_load = False
            case _:
                break
        if mem_op is None or mem_op.base is None:
            break
        # Reconstruct with computed offset
        computed = f"(int)({expr}) * {mem_op.scale} + {mem_op.offset}"
        new_mem = f"[{mem_op.base} + ({computed})]"
        if is_load:
            new_line = (
                f"    | {p.mnemonic} {fmt_op(p.dst)}, {mem_op.size} {new_mem}"
                if mem_op.size
                else f"    | {p.mnemonic} {fmt_op(p.dst)}, {new_mem}"
            )
        else:
            new_line = (
                f"    | {p.mnemonic} {mem_op.size} {new_mem}, {fmt_op(p.src)}"
                if mem_op.size
                else f"    | {p.mnemonic} {new_mem}, {fmt_op(p.src)}"
            )
        folded.append(new_line)
        scan += 1
    if not folded:
        return None
    # Safety: index reg overwritten by load dest, or dead after
    first = parsed[i]
    dest_idx = (
        first.dst.idx
        if isinstance(first, Asm) and isinstance(first.dst, Reg)
        else None
    )
    if dest_idx == src_idx or _reg_dead_after(ctx.program, scan, src_idx):
        _stat("P6_indexed_mem")
        return _Match(scan - i, folded)
    return None


def _try_two_mov_add(ctx: _FoldCtx) -> _Match | None:
    """Pattern 15: Combine two immediate loads followed by add."""
    lines, i, parsed, indent, expr = (
        ctx.lines, ctx.i, ctx.parsed, ctx.indent, ctx.expr)
    mov_info = _parse_emit_mov_imm_call(lines[i])
    if mov_info is None or i + 1 >= len(lines):
        return None
    dst_idx, dst_name, rhs_expr = mov_info
    match parsed[i + 1]:
        case Asm(
            mnemonic="add", dst=Reg(name=add_dst), src=Reg(name=add_src)
        ) if Reg(add_dst).idx == dst_idx and Reg(add_src).idx == ctx.src_idx:
            pass
        case _:
            return None
    if not _reg_dead_after(ctx.program, i + 2, ctx.src_idx):
        return None
    bits = _reg_bits(add_dst)
    if bits <= 32:
        combined = f"(uint{bits}_t)(({rhs_expr}) + ({expr}))"
    else:
        combined = f"({rhs_expr}) + ({expr})"
    _stat("P16_two_mov_add")
    return _Match(
        2,
        [
            f"{indent}emit_mov_imm(Dst, {dst_name}, {combined});",
        ],
    )


def _try_alu_imm(ctx: _FoldCtx) -> _Match | None:
    """Pattern 8: Fold ALU instruction's register operand into immediate.

    When emit_mov_imm loads a value into a register and the next
    instruction uses that register as the second operand of an ALU
    operation (cmp, test, and, or, xor, add, sub), replace the register
    with the immediate value directly.  This eliminates the mov entirely.

    Example — compare with runtime constant:
        emit_mov_imm(Dst, JREG_RAX, instruction->operand0);
        | cmp rcx, rax
      → (if value fits in sign-extended imm32)
        | cmp rcx, (int)(instruction->operand0)
      → (if value does NOT fit in imm32, falls back)
        emit_mov_imm(Dst, JREG_RAX, instruction->operand0);
        | cmp rcx, rax

    Example — OR with type tag:
        emit_mov_imm(Dst, JREG_RDX, instruction->operand0);
        | or qword [rbx + 16], rdx
      → (32-bit value fits)
        | or qword [rbx + 16], (int)(instruction->operand0)

    For 64-bit operands, emits a runtime range check (if/else) to use
    the immediate form when possible, falling back to the register form.
    For 32-bit and 16-bit operands, the immediate always fits.

    The source register must be dead after the ALU instruction (since
    we're eliminating the load).
    """
    _ALU_OPS = {"cmp", "test", "and", "or", "xor", "add", "sub"}
    # Only test is safe for commutative swap because it doesn't write to
    # a register.  For and/or/xor/add, swapping would change which register
    # receives the result, corrupting program state.
    _COMMUTATIVE_OPS = {"test"}
    lines, i, src_idx, indent, src_name, expr = (
        ctx.lines, ctx.i, ctx.src_idx, ctx.indent, ctx.src_name, ctx.expr)
    cur = ctx.cur
    if not isinstance(cur, Asm) or cur.mnemonic not in _ALU_OPS:
        return None
    alu_op = cur.mnemonic
    alu_reg = None
    dst_op = None
    # Standard order: ALU dst, src — where src is our emit_mov_imm register.
    if isinstance(cur.src, Reg) and Reg(cur.src.name).idx == src_idx:
        alu_reg = cur.src.name
        dst_op = cur.dst
    # Commutative swap: ALU dst, src — where dst is our register.
    elif (
        alu_op in _COMMUTATIVE_OPS
        and isinstance(cur.dst, Reg)
        and Reg(cur.dst.name).idx == src_idx
    ):
        alu_reg = cur.dst.name
        dst_op = cur.src
    else:
        return None
    # Format the first operand text for output
    alu_first = fmt_op(dst_op)
    # Don't fold if first operand is also the same register
    match dst_op:
        case Reg(name=first_reg) if Reg(first_reg).idx == src_idx:
            return None
        case Mem() as mem if _mem_uses_reg(mem, src_idx):
            return None
    if not _reg_dead_after(ctx.program, i + 1, src_idx):
        return None
    _stat("P8_alu_imm_fold")
    bits = Reg(alu_reg).bits
    if bits == 32:
        return _Match(
            1,
            [
                f"{indent}| {alu_op} {alu_first}, (int)({expr})",
            ],
        )
    if bits == 64:
        # For register first operand, use emit_{op}_reg_imm helpers.
        # These emit the shortest encoding: imm32 when it fits, otherwise
        # scratch register + reg-reg form.
        if isinstance(dst_op, Reg):
            first_idx_name = _REG_IDX_NAME.get(dst_op.name.lower())
            if first_idx_name:
                return _Match(
                    1,
                    [
                        f"{indent}emit_{alu_op}_reg_imm(Dst, {first_idx_name}, {src_name}, (uintptr_t)({expr}));",
                    ],
                )
        # Simple qword memory compare: route through a dedicated helper so we
        # do not have to emit a multiline if/else template at each call site.
        if (
            alu_op == "cmp"
            and isinstance(dst_op, Mem)
            and dst_op.size == "qword"
            and dst_op.base is not None
            and dst_op.index is None
        ):
            base_name = Reg(dst_op.base).jreg
            if base_name is not None:
                return _Match(
                    1,
                    [
                        f"{indent}emit_cmp_mem64_imm(Dst, {base_name}, {dst_op.offset}, {src_name}, (uintptr_t)({expr}));",
                    ],
                )
        # Memory first operand: fallback inline runtime range check for other
        # cases that still cannot use a dedicated helper.
        c64 = f"(int64_t)({expr})"
        c32 = f"(int32_t)({expr})"
        return _Match(
            1,
            [
                f"{indent}if ({c64} == {c32}) {{",
                f"{indent}| {alu_op} {alu_first}, (int)({expr})",
                f"{indent}}} else {{",
                f"{indent}emit_mov_imm(Dst, {src_name}, {expr});",
                f"{indent}| {alu_op} {alu_first}, {alu_reg}",
                f"{indent}}}",
            ],
        )
    if bits == 16:
        return _Match(
            1,
            [
                f"{indent}| {alu_op} {alu_first}, (short)({expr})",
            ],
        )
    return None


def _try_store_imm(ctx: _FoldCtx) -> _Match | None:
    """Pattern 12: Fold register store into immediate store to memory.

    When emit_mov_imm loads a value into a register and the next
    instruction stores that register to memory, replace with a direct
    immediate-to-memory store (eliminating the register load entirely).

    Example — store byte:
        emit_mov_imm(Dst, JREG_RAX, instruction->oparg);
        | mov byte [rbx + 42], al
      →
        | mov byte [rbx + 42], (char)(instruction->oparg)

    Example — store qword (needs range check):
        emit_mov_imm(Dst, JREG_RCX, instruction->operand0);
        | mov qword [r14 + 8], rcx
      →
        if ((int64_t)(instruction->operand0) ==
            (int32_t)(instruction->operand0)) {
        | mov qword [r14 + 8], (int)(instruction->operand0)
        } else {
        emit_mov_imm(Dst, JREG_RCX, instruction->operand0);
        | mov qword [r14 + 8], rcx
        }

    For byte/word/dword stores the immediate always fits.  For qword
    stores, x86_64 only supports sign-extended imm32, so we emit a
    runtime range check with a fallback.

    The source register must be dead after the store.
    """
    lines, i, src_idx, indent, src_name, expr = (
        ctx.lines, ctx.i, ctx.src_idx, ctx.indent, ctx.src_name, ctx.expr)
    match ctx.cur:
        case Asm(
            mnemonic="mov",
            dst=Mem(size=size, expr=mem_expr),
            src=Reg(name=reg),
        ) if (
            size in ("qword", "dword", "word", "byte")
            and Reg(reg).idx == src_idx
        ):
            pass  # fall through to shared logic
        case _:
            return None
    mem = typing.cast(Mem, ctx.cur.dst)
    if _mem_uses_reg(mem, src_idx):
        return None
    if not _reg_dead_after(ctx.program, i + 1, src_idx):
        return None
    _stat("P12_store_imm")
    _SIZE_CAST = {"byte": "char", "word": "short", "dword": "int"}
    if size in _SIZE_CAST:
        cast = _SIZE_CAST[size]
        return _Match(
            1,
            [
                f"{indent}| mov {size} {mem_expr}, ({cast})({expr})",
            ],
        )
    # qword: use emit_store_mem64_imm for simple [base + offset] forms,
    # fall back to inline if/else for complex addressing modes.
    if mem.base and not mem.index:
        from _asm_to_dasc import Reg as _Reg

        base_reg = _Reg(mem.base)
        base_jreg = base_reg.jreg
        if base_jreg is not None:
            return _Match(
                1,
                [
                    f"{indent}emit_store_mem64_imm(Dst, {base_jreg},"
                    f" {mem.offset}, {src_name}, {expr});",
                ],
            )
    # Complex addressing: inline if/else fallback
    c64 = f"(int64_t)({expr})"
    c32 = f"(int32_t)({expr})"
    return _Match(
        1,
        [
            f"{indent}if ({c64} == {c32}) {{",
            f"{indent}| mov qword {mem_expr}, (int)({expr})",
            f"{indent}}} else {{",
            f"{indent}emit_mov_imm(Dst, {src_name}, {expr});",
            f"{indent}| mov qword {mem_expr}, {reg}",
            f"{indent}}}",
        ],
    )


def _try_shift_fold(ctx: _FoldCtx) -> _Match | None:
    """Fold shift of an emit_mov_imm register into the immediate expression.

    When emit_mov_imm loads a value into a register and the next instruction
    shifts that same register by an immediate amount, absorb the shift into
    the emit_mov_imm expression.

    Example (LOAD_GLOBAL_MODULE — dict key lookup):
        emit_mov_imm(Dst, JREG_RAX, (uint16_t)(instruction->operand1));
        | shl rax, 4
      →
        emit_mov_imm(Dst, JREG_RAX, (uintptr_t)((uint16_t)(instruction->operand1)) << 4);
    """
    match ctx.cur:
        case Asm(
            mnemonic="shl", dst=Reg(name=reg), src=Imm(text=shift_str)
        ) if Reg(reg).idx == ctx.src_idx:
            pass
        case _:
            return None
    _stat("P15_shift_fold")
    return _Match(
        1,
        [
            f"{ctx.indent}emit_mov_imm(Dst, {ctx.src_name},"
            f" (uintptr_t)({ctx.expr}) << {shift_str});",
        ],
    )


def _try_lea_fold(ctx: _FoldCtx) -> _Match | None:
    """Fold emit_mov_imm + lea [base + reg*scale] into lea [base + disp].

    When emit_mov_imm loads a JIT-time value into a register that is then
    only used as a scaled index in a lea, the scaled product can be computed
    at JIT emit time and used as a 32-bit displacement instead.

    Example (stack pointer adjustment):
        emit_mov_imm(Dst, JREG_RBP, (0 - (uint16_t)(instruction->oparg)));
        | lea rdi, [r14 + rbp*8]
      →
        | lea rdi, [r14 + (int)((intptr_t)(0 - (uint16_t)(instruction->oparg)) * 8)]

    Also handles the no-base form [reg*scale+0]:
        emit_mov_imm(Dst, JREG_R15, expr);
        | lea r12, [r15*8+0]
      →
        emit_mov_imm(Dst, JREG_R12, (uintptr_t)(expr) * 8);

    Conditions:
    - The emit_mov_imm register is used as a scaled index in the lea
    - The register is dead after the lea (or overwritten by it)
    - The expression * scale fits in int32_t (guaranteed for oparg-based
      expressions: max |oparg|=65535, max scale=8, product ≤ 524,280)
    """
    import re as _re

    match ctx.cur:
        case Asm(mnemonic="lea", dst=Reg(name=dst_reg), src=Mem() as mem_op):
            pass
        case _:
            return None

    # Check that the emit_mov_imm register is used as a scaled index
    src_reg = _IDX_TO_ALL_NAMES.get(ctx.src_idx, ())
    if not src_reg:
        return None

    mem_text = fmt_op(mem_op)
    reg_alt = "|".join(_re.escape(r) for r in src_reg)

    # Pattern 1: [base + reg*scale]
    m_based = _re.search(
        r"\[(\w+)\s*\+\s*(" + reg_alt + r")\*(\d+)\]", mem_text
    )
    # Pattern 2: [reg*scale+0] (no base register)
    m_nobase = _re.search(r"\[(" + reg_alt + r")\*(\d+)\+0\]", mem_text)

    if m_based:
        base_reg = m_based.group(1)
        scale = int(m_based.group(3))

        # Check if the src register's original value is needed after the lea.
        # The lea itself reads src_reg (scaled index), but we're replacing that
        # with a displacement, so the lea's read doesn't count.
        # If the lea's destination overwrites src_reg, the value is dead anyway.
        lea_dst_idx = Reg(dst_reg).idx
        if lea_dst_idx != ctx.src_idx:
            if not _reg_dead_after(ctx.program, ctx.i + 1, ctx.src_idx):
                return None

        _stat("P17_lea_fold")
        return _Match(
            1,
            [
                f"{ctx.indent}| lea {dst_reg},"
                f" [{base_reg} + (int)((intptr_t)({ctx.expr}) * {scale})]",
            ],
        )
    elif m_nobase:
        scale = int(m_nobase.group(2))

        # For [reg*scale+0], the result is just expr * scale.
        # We emit a new emit_mov_imm into the lea's destination register.
        dst_idx = Reg(dst_reg).idx
        dst_name = (
            _IDX_TO_JREG.get(dst_idx, str(dst_idx))
            if dst_idx is not None
            else dst_reg
        )

        # Check src register is dead after the lea
        lea_dst_idx = Reg(dst_reg).idx
        if lea_dst_idx != ctx.src_idx:
            if not _reg_dead_after(ctx.program, ctx.i + 1, ctx.src_idx):
                return None

        _stat("P17_lea_fold")
        return _Match(
            1,
            [
                f"{ctx.indent}emit_mov_imm(Dst, {dst_name},"
                f" (uintptr_t)({ctx.expr}) * {scale});",
            ],
        )

    return None


def _fold_mov_imm(
    program: _PeepholeProgram,
    i: int,
    result: list[str],
) -> int | None:
    """Try to fold an emit_mov_imm with subsequent instructions.

    Returns the number of lines consumed (advancing past ``i``), or
    None if no fold was possible and the caller should try other patterns.
    """
    lines = program.lines
    parsed = program.parsed
    m_mov = _RE_EMIT_MOV_IMM.match(lines[i])
    if not m_mov or i + 1 >= len(lines):
        return None
    # Guard against re-folding inside "} else {" fallback blocks
    if result and result[-1].rstrip().endswith("} else {"):
        return None

    indent = m_mov.group(1)
    src_idx, src_name = _parse_jreg(m_mov.group(2))
    expr = m_mov.group(3)
    consumed = 1

    # Build shared context for all _try_* functions
    ctx = _FoldCtx(
        program=program,
        lines=lines,
        parsed=parsed,
        i=i + 1,
        src_idx=src_idx,
        src_name=src_name,
        indent=indent,
        expr=expr,
    )

    # Try each fold pattern in priority order (all take ctx: _FoldCtx)
    ctx.i = i + consumed
    if ctx.i < len(lines):
        for try_fn in (
            _try_two_mov_add,
            _try_indexed_mem,
            _try_alu_imm,
            _try_store_imm,
            _try_shift_fold,
            _try_lea_fold,
        ):
            match = try_fn(ctx)
            if match:
                result.extend(match.output)
                return consumed + match.consumed

    return None


# ── Standalone patterns ────────────────────────────────────────────────
#
# Each function: (lines, i, result, state) → int | None
# Returns lines consumed on match, or None.  May append to ``result``
# and mutate ``state``.


def _pattern_preserve_flags_mov_imm(
    program: _PeepholeProgram,
    i: int,
    result: list[str],
    state: _PeepholeState,
) -> int | None:
    """Preserve flags across immediate loads inserted before setcc/cmov/jcc."""
    del state  # unused
    lines = program.lines
    if "emit_mov_imm(" not in lines[i] or i == 0:
        return None
    prev = program.parsed[i - 1]
    if isinstance(prev, CCall) and prev.kind == CCallKind.MOV_IMM:
        return None
    if not _is_flag_writer(prev):
        return None
    j = i
    output: list[str] = []
    while j < len(lines) and "emit_mov_imm(" in lines[j]:
        cur = program.parsed[j]
        if not (isinstance(cur, CCall) and cur.kind == CCallKind.MOV_IMM):
            break
        output.append(_preserve_flags_mov_imm(lines[j]))
        j += 1
    if not output or j >= len(lines):
        return None
    if not _is_flag_consumer(program.parsed[j]):
        return None
    _stat("SP0_preserve_flags_mov_imm")
    result.extend(output)
    return len(output)


def _pattern_store_reload_elim(
    program: _PeepholeProgram,
    i: int,
    result: list[str],
    state: _PeepholeState,
) -> int | None:
    """Eliminate redundant stackpointer reloads on the hot path.

    Matches:
        | mov qword [r13 + 64], r14   (store)
        | test/cmp REG, IMM
        | jcc =>COLD_LABEL
        |=>MERGE_LABEL:
        | mov r14, qword [r13 + 64]   (reload — eliminated)

    The hot path never modifies r14 or [r13+64].  The cold dealloc path
    may modify [r13+64], so we insert a reload there before jumping back.

    Uses structural pattern matching on typed ``Line`` objects for dispatch.
    """
    lines = program.lines
    if i + 4 >= len(lines):
        return None
    window = [program.parsed[i + k] for k in range(5)]
    match window:
        case [
            # mov qword [r13 + FRAME_STACKPOINTER_OFFSET], r14
            Asm(
                mnemonic="mov",
                dst=Mem(base=_C.FRAME, offset=_C.FRAME_SP_OFS),
                src=Reg(name=_C.SP),
            ),
            # test/cmp REG, IMM
            Asm(mnemonic=op),
            # jcc =>LABEL
            Asm(target=branch_tgt),
            # =>MERGE_LABEL:
            Label(name=merge_name),
            # mov r14, qword [r13 + FRAME_STACKPOINTER_OFFSET]
            Asm(
                mnemonic="mov",
                dst=Reg(name=_C.SP),
                src=Mem(base=_C.FRAME, offset=_C.FRAME_SP_OFS),
            ),
        ] if op in ("test", "cmp") and branch_tgt:
            merge_lbl = f"=>{merge_name}"
            for k in range(4):
                result.append(lines[i + k])
            state.need_reload_before_jmp.add(merge_lbl)
            _stat("SP1_store_reload_elim")
            return 5
    return None


def _pattern_cold_reload_insert(
    program: _PeepholeProgram,
    i: int,
    result: list[str],
    state: _PeepholeState,
) -> int | None:
    """Insert stackpointer reload before cold-path jump back to merge label.

    After store-reload elimination, the dealloc cold path needs to reload
    r14 from [r13+64] before jumping back (since _Py_Dealloc may have
    modified [r13+64] via __del__).
    """
    if not state.need_reload_before_jmp:
        return None
    lines = program.lines
    cur = program.parsed[i]
    # Must be a jump or conditional branch
    match cur:
        case Asm(mnemonic=m, target=target_lbl) if target_lbl and (
            m == "jmp" or m in _BRANCH_MNEMONICS
        ):
            pass  # fall through
        case _:
            return None
    if target_lbl not in state.need_reload_before_jmp:
        return None
    # Only insert reload if we're after a call (dealloc path)
    for prev in reversed(result):
        prev_line = parse_line(prev)
        match prev_line:
            case Blank():
                continue
            case CCall() | Asm(mnemonic="call"):
                _stat("SP2_cold_reload_insert")
                result.append(_SP_RELOAD_LINE)
            case Asm(mnemonic="add", dst=Reg(name="rsp")):
                _stat("SP2_cold_reload_insert")
                result.append(_SP_RELOAD_LINE)
        break
    # Don't consume extra lines — just insert before the current line
    return None


def _pattern_inverted_store_reload(
    program: _PeepholeProgram,
    i: int,
    result: list[str],
    state: _PeepholeState,
) -> int | None:
    """Defer stackpointer store to cold path when the hot branch skips it.

    Some stencils (e.g. _POP_TOP_r10) have an "inverted" pattern where the
    conditional branch jumps to the merge point (hot path) and the
    fallthrough goes to the cold path.  The store/reload pair is redundant
    on the hot path since r14 is preserved by callee-saved convention
    across any C calls on the cold path.

    Matches the pattern:
        | mov qword [r13 + 64], r14    ← store (line i)
        | test/cmp REG, IMM            ← branch condition (line i+1)
        | jcc =>L(MERGE)               ← hot: jump to merge (line i+2)
        | jmp =>L(COLD)                ← cold path redirect (line i+3)
        ... intermediate comeback code (labels + instructions) ...
        |=>L(MERGE):                   ← merge label
        | mov r14, qword [r13 + 64]   ← reload (eliminated on hot path)

    Transforms to:
        | test/cmp REG, IMM            ← condition (moved up past store)
        | jcc =>L(MERGE)               ← hot: jump past store+reload
        | mov qword [r13 + 64], r14   ← store (deferred, cold-only)
        | jmp =>L(COLD)               ← cold path redirect
        ... intermediate comeback code ...
        | mov r14, qword [r13 + 64]   ← reload (moved before merge label)
        |=>L(MERGE):                   ← merge point (hot enters here)

    Hot path saves 14 bytes (7-byte store + 7-byte reload) and 2 memory
    accesses.  The cold comeback path still gets the reload.

    Uses structural pattern matching for robust store_sp, reload_sp, branch,
    and jump detection.
    """
    lines = program.lines
    if i + 5 >= len(lines):
        return None

    # First 4 lines: store, test/cmp, jcc, jmp
    w = [program.parsed[i + k] for k in range(4)]
    match w:
        case [
            Asm(
                mnemonic="mov",
                dst=Mem(base=_C.FRAME, offset=_C.FRAME_SP_OFS),
                src=Reg(name=_C.SP),
            ),
            Asm(mnemonic=cmp_op),
            Asm(target=merge_target),
            Asm(mnemonic="jmp"),
        ] if cmp_op in ("test", "cmp") and merge_target:
            pass  # fall through
        case _:
            return None

    # Scan forward to find the merge label and reload
    merge_label_str = (
        f"|=>{merge_target[2:]}:" if merge_target.startswith("=>") else None
    )
    if not merge_label_str:
        return None
    merge_idx = None
    for j in range(i + 4, min(i + 20, len(lines))):
        stripped = lines[j].strip()
        if stripped.replace(" ", "").startswith(
            merge_label_str.replace(" ", "")
        ):
            merge_idx = j
            break
    if merge_idx is None or merge_idx + 1 >= len(lines):
        return None
    match program.parsed[merge_idx + 1]:
        case Asm(
            mnemonic="mov",
            dst=Reg(name=_C.SP),
            src=Mem(base=_C.FRAME, offset=_C.FRAME_SP_OFS),
        ):
            pass  # confirmed reload
        case _:
            return None

    # Pattern matched!  Build the transformed output.
    result.append(lines[i + 1])  # test/cmp
    result.append(lines[i + 2])  # jcc =>L(MERGE)
    result.append(lines[i])  # store (now cold-only)
    result.append(lines[i + 3])  # jmp =>L(COLD)
    for k in range(i + 4, merge_idx):
        result.append(lines[k])
    result.append(lines[merge_idx + 1])  # reload (before merge label)
    result.append(lines[merge_idx])  # |=>L(MERGE):
    _stat("SP3_inverted_store_reload")
    return (merge_idx + 1) - i + 1


def _pattern_test_memory_fold(
    program: _PeepholeProgram,
    i: int,
    result: list[str],
    state: _PeepholeState,
) -> int | None:
    """Pattern 14: Fold mov+test into test [mem] when loaded register is dead.

    When a mov loads a value from memory solely to test a bit/byte
    and the register is dead after the conditional branch, we can
    test the memory location directly, eliminating the mov.

    Matches (3 consecutive lines):
        | mov REG, qword/dword [MEM]
        | test REG_LOW, REG_LOW   (or: test REG_LOW, IMM)
        | jcc =>TARGET

    When REG is dead after the jcc, transforms to:
        | cmp byte [MEM], 0        (for test REG, REG form)
        | test byte [MEM], IMM     (for test REG, IMM form)
        | jcc =>TARGET

    Example (TIER2_RESUME_CHECK — very hot path):
        Before:
            | mov rax, qword [r15 + 24]
            | test al, al
            | jne =>instruction->jump_target
        After:
            | cmp byte [r15 + 24], 0
            | jne =>instruction->jump_target
    """
    del state  # unused
    lines = program.lines
    if i + 2 >= len(lines):
        return None
    window = [program.parsed[i + k] for k in range(3)]
    match window:
        # mov REG, qword/dword [MEM]; test REG_LOW, REG_LOW; jcc
        case [
            Asm(mnemonic="mov", dst=Reg() as mov_dst, src=Mem() as mov_src),
            Asm(mnemonic="test", dst=Reg() as test_dst, src=Reg() as test_src),
            Asm(target=branch_target),
        ] if (
            branch_target
            and mov_dst.idx is not None
            and test_dst.idx == mov_dst.idx
            and test_src.idx == mov_dst.idx
            and mov_src.size in ("qword", "dword")
        ):
            # Start deadness check from the jcc (i+2), not i+3, so
            # both the fall-through AND the branch target are checked.
            if not _reg_dead_after(program, i + 2, mov_dst.idx):
                return None
            mem_expr = mov_src.expr
            _stat("P14_test_memory_fold")
            result.append(f"    | cmp byte {mem_expr}, 0\n")
            result.append(lines[i + 2])
            return 3

        # mov REG, qword/dword [MEM]; test REG_LOW, IMM; jcc
        case [
            Asm(mnemonic="mov", dst=Reg() as mov_dst, src=Mem() as mov_src),
            Asm(mnemonic="test", dst=Reg() as test_dst, src=Imm() as test_imm),
            Asm(target=branch_target),
        ] if (
            branch_target
            and mov_dst.idx is not None
            and test_dst.idx == mov_dst.idx
            and mov_src.size in ("qword", "dword")
        ):
            if not _reg_dead_after(program, i + 2, mov_dst.idx):
                return None
            mem_expr = mov_src.expr
            _stat("P14_test_memory_fold")
            result.append(f"    | test byte {mem_expr}, {test_imm.text}\n")
            result.append(lines[i + 2])
            return 3
    return None


def _pattern_dead_null_check(
    program: _PeepholeProgram,
    i: int,
    result: list[str],
    state: _PeepholeState,
) -> int | None:
    """Pattern 13: Remove dead NULL check after PyStackRef tag creation.

    When creating a tagged _PyStackRef from a raw pointer, Clang emits a
    NULL check that is actually dead code: the preceding ``movzx`` already
    dereferences the pointer (at offset 6), so a NULL pointer would
    segfault before the check could ever fire.

    Matches (5 consecutive lines):
        | movzx edi, word [rax + 6]   ← dereferences rax (proves non-NULL)
        | and edi, 1                  ← extract ob_flags deferred bit
        | or rdi, rax                 ← create tagged ref: ptr | flag
        | cmp rdi, 1                  ← dead: rax!=NULL so rdi!=1
        | je =>L(N)                   ← dead branch (error path)

    Emits only the first 3 lines (tag creation), removing the dead check.

    Example (from _BINARY_OP_MULTIPLY_FLOAT after freelist allocation):
        Before:
            | movzx edi, word [rax + 6]
            | and edi, 1
            | or rdi, rax
            | cmp rdi, 1
            | je =>L(3)             ← removed (dead)
        After:
            | movzx edi, word [rax + 6]
            | and edi, 1
            | or rdi, rax

    Uses structural pattern matching for robust operand checking.
    """
    lines = program.lines
    if i + 4 >= len(lines):
        return None
    window = [program.parsed[i + k] for k in range(5)]
    match window:
        case [
            # movzx edi, word [REG + 6] — dereferences REG (proves non-NULL)
            Asm(mnemonic="movzx", src=Mem(size="word", base=deref_reg)),
            # and edi, 1 — extract ob_flags deferred bit
            Asm(mnemonic="and", src=Imm(value=1)),
            # or rdi, REG — create tagged ref: ptr | flag
            Asm(mnemonic="or", src=Reg(name=tagged_reg)),
            # cmp rdi, 1 — dead NULL check
            Asm(mnemonic="cmp", src=Imm(value=1)),
            # je =>L(N) — dead branch (error path)
            Asm(mnemonic="je", target=branch_target),
        ] if deref_reg == tagged_reg.lower() and branch_target:
            # Emit only the tag creation (first 3 lines), skip cmp + je
            for k in range(3):
                result.append(lines[i + k])
            _stat("P13_dead_null_check")
            return 5
    return None


def _pattern_dead_frame_anchor(
    program: _PeepholeProgram,
    i: int,
    result: list[str],
    state: _PeepholeState,
) -> int | None:
    """Remove dead lea anchors introduced to force canonical stack frames.

    The JIT template intentionally forces Clang to materialize a fixed stack
    frame. That can leave behind dead instructions like ``lea rax, [rbp-144]``
    whose only purpose was to keep the frame allocated. Those writes must not
    leak into the stitched trace, since they clobber live cross-stencil
    registers such as ``rax``.
    """
    del state
    match program.parsed[i]:
        case Asm(
            mnemonic="lea",
            dst=Reg() as dst,
            src=Mem(base="rbp", index=None, scale=None, offset=offset),
        ):
            if offset >= 0 or dst.idx is None:
                return None
            if i + 1 < len(program.parsed):
                next_effect = _line_effect(program.parsed[i + 1])
                if dst.idx in next_effect.reads:
                    return None
            if not _is_dead_before_any_call(program, i, dst.idx):
                return None
            _stat("P18_dead_frame_anchor")
            return 1
    return None


def _pattern_inverse_mov_restore(
    program: _PeepholeProgram,
    i: int,
    result: list[str],
    state: _PeepholeState,
) -> int | None:
    """Drop the redundant second move in ``mov A, B`` / ``mov B, A`` pairs.

    The first move already preserves the original value of ``B`` in ``A`` while
    leaving ``B`` unchanged, so the immediate inverse move is a no-op.
    """
    del state
    if i + 1 >= len(program.parsed):
        return None
    match program.parsed[i], program.parsed[i + 1]:
        case (
            Asm(mnemonic="mov", dst=Reg() as dst1, src=Reg() as src1),
            Asm(mnemonic="mov", dst=Reg() as dst2, src=Reg() as src2),
        ):
            if (
                dst1.idx is None
                or src1.idx is None
                or dst2.idx is None
                or src2.idx is None
            ):
                return None
            if (
                dst1.idx != src2.idx
                or src1.idx != dst2.idx
                or dst1.bits != src1.bits
                or dst2.bits != src2.bits
                or dst1.bits != dst2.bits
            ):
                return None
            result.append(program.lines[i])
            _stat("P19_inverse_mov_restore")
            return 2
    return None


def _pass_fold_mov_imm(
    program: _PeepholeProgram,
    i: int,
    result: list[str],
    state: _PeepholeState,
) -> int | None:
    """Driver wrapper for the emit_mov_imm fold family."""
    del state
    if (
        isinstance(program.parsed[i], CCall)
        and program.parsed[i].kind == CCallKind.MOV_IMM
    ):
        return _fold_mov_imm(program, i, result)
    return None


# Pass registry — order matters for priority
_PEEPHOLE_PASSES = (
    _PeepholePass("mov_imm_folds", _pass_fold_mov_imm),
    _PeepholePass(
        "SP0_preserve_flags_mov_imm", _pattern_preserve_flags_mov_imm
    ),
    _PeepholePass("P13_dead_null_check", _pattern_dead_null_check),
    _PeepholePass("P18_dead_frame_anchor", _pattern_dead_frame_anchor),
    _PeepholePass("P19_inverse_mov_restore", _pattern_inverse_mov_restore),
    _PeepholePass("P14_test_memory_fold", _pattern_test_memory_fold),
    _PeepholePass("SP1_store_reload_elim", _pattern_store_reload_elim),
    _PeepholePass("SP2_cold_reload_insert", _pattern_cold_reload_insert),
    _PeepholePass("SP3_inverted_store_reload", _pattern_inverted_store_reload),
)
