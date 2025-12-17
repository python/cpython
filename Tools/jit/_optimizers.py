"""Low-level optimization of textual assembly."""

import dataclasses
import enum
import pathlib
import re
import typing

# Same as saying "not string.startswith('')":
_RE_NEVER_MATCH = re.compile(r"(?!)")
# Dictionary mapping branch instructions to their inverted branch instructions.
# If a branch cannot be inverted, the value is None:
_X86_BRANCH_NAMES = {
    # https://www.felixcloutier.com/x86/jcc
    "ja": "jna",
    "jae": "jnae",
    "jb": "jnb",
    "jbe": "jnbe",
    "jc": "jnc",
    "jcxz": None,
    "je": "jne",
    "jecxz": None,
    "jg": "jng",
    "jge": "jnge",
    "jl": "jnl",
    "jle": "jnle",
    "jo": "jno",
    "jp": "jnp",
    "jpe": "jpo",
    "jrcxz": None,
    "js": "jns",
    "jz": "jnz",
    # https://www.felixcloutier.com/x86/loop:loopcc
    "loop": None,
    "loope": None,
    "loopne": None,
    "loopnz": None,
    "loopz": None,
}
# Update with all of the inverted branches, too:
_X86_BRANCH_NAMES |= {v: k for k, v in _X86_BRANCH_NAMES.items() if v}
# No custom relocations needed
_X86_BRANCHES: dict[str, tuple[str | None, str | None]] = {
    k: (v, None) for k, v in _X86_BRANCH_NAMES.items()
}

_AARCH64_COND_CODES = {
    # https://developer.arm.com/documentation/dui0801/b/CJAJIHAD?lang=en
    "eq": "ne",
    "ne": "eq",
    "lt": "ge",
    "ge": "lt",
    "gt": "le",
    "le": "gt",
    "vs": "vc",
    "vc": "vs",
    "mi": "pl",
    "pl": "mi",
    "cs": "cc",
    "cc": "cs",
    "hs": "lo",
    "lo": "hs",
    "hi": "ls",
    "ls": "hi",
}
# MyPy doesn't understand that a invariant variable can be initialized by a covariant value
CUSTOM_AARCH64_BRANCH19: str | None = "CUSTOM_AARCH64_BRANCH19"

_AARCH64_SHORT_BRANCHES = {
    "tbz": "tbnz",
    "tbnz": "tbz",
}

# Branches are either b.{cond}, bc.{cond}, cbz, cbnz, tbz or tbnz
_AARCH64_BRANCHES: dict[str, tuple[str | None, str | None]] = (
    {
        "b." + cond: (("b." + inverse if inverse else None), CUSTOM_AARCH64_BRANCH19)
        for (cond, inverse) in _AARCH64_COND_CODES.items()
    }
    | {
        "bc." + cond: (("bc." + inverse if inverse else None), CUSTOM_AARCH64_BRANCH19)
        for (cond, inverse) in _AARCH64_COND_CODES.items()
    }
    | {
        "cbz": ("cbnz", CUSTOM_AARCH64_BRANCH19),
        "cbnz": ("cbz", CUSTOM_AARCH64_BRANCH19),
    }
    | {cond: (inverse, None) for (cond, inverse) in _AARCH64_SHORT_BRANCHES.items()}
)


@enum.unique
class InstructionKind(enum.Enum):

    JUMP = enum.auto()
    LONG_BRANCH = enum.auto()
    SHORT_BRANCH = enum.auto()
    CALL = enum.auto()
    RETURN = enum.auto()
    SMALL_CONST_1 = enum.auto()
    SMALL_CONST_2 = enum.auto()
    OTHER = enum.auto()


@dataclasses.dataclass
class Instruction:
    kind: InstructionKind
    name: str
    text: str
    target: str | None

    def is_branch(self) -> bool:
        return self.kind in (InstructionKind.LONG_BRANCH, InstructionKind.SHORT_BRANCH)

    def update_target(self, target: str) -> "Instruction":
        assert self.target is not None
        return Instruction(
            self.kind, self.name, self.text.replace(self.target, target), target
        )

    def update_name_and_target(self, name: str, target: str) -> "Instruction":
        assert self.target is not None
        return Instruction(
            self.kind,
            name,
            self.text.replace(self.name, name).replace(self.target, target),
            target,
        )


@dataclasses.dataclass(eq=False)
class _Block:
    label: str | None = None
    # Non-instruction lines like labels, directives, and comments:
    noninstructions: list[str] = dataclasses.field(default_factory=list)
    # Instruction lines:
    instructions: list[Instruction] = dataclasses.field(default_factory=list)
    # If this block ends in a jump, where to?
    target: typing.Self | None = None
    # The next block in the linked list:
    link: typing.Self | None = None
    # Whether control flow can fall through to the linked block above:
    fallthrough: bool = True
    # Whether this block can eventually reach the next uop (_JIT_CONTINUE):
    hot: bool = False

    def resolve(self) -> typing.Self:
        """Find the first non-empty block reachable from this one."""
        block = self
        while block.link and not block.instructions:
            block = block.link
        return block


@dataclasses.dataclass
class Optimizer:
    """Several passes of analysis and optimization for textual assembly."""

    path: pathlib.Path
    _: dataclasses.KW_ONLY
    # Prefixes used to mangle local labels and symbols:
    label_prefix: str
    symbol_prefix: str
    re_global: re.Pattern[str]
    # The first block in the linked list:
    _root: _Block = dataclasses.field(init=False, default_factory=_Block)
    _labels: dict[str, _Block] = dataclasses.field(init=False, default_factory=dict)
    # No groups:
    _re_noninstructions: typing.ClassVar[re.Pattern[str]] = re.compile(
        r"\s*(?:\.|#|//|;|$)"
    )
    # One group (label):
    _re_label: typing.ClassVar[re.Pattern[str]] = re.compile(
        r'\s*(?P<label>[\w."$?@]+):'
    )
    # Override everything that follows in subclasses:
    _supports_external_relocations = True
    supports_small_constants = False
    _branches: typing.ClassVar[dict[str, tuple[str | None, str | None]]] = {}
    # Short branches are instructions that can branch within a micro-op,
    # but might not have the reach to branch anywhere within a trace.
    _short_branches: typing.ClassVar[dict[str, str]] = {}
    # Two groups (instruction and target):
    _re_branch: typing.ClassVar[re.Pattern[str]] = _RE_NEVER_MATCH
    # One group (target):
    _re_call: typing.ClassVar[re.Pattern[str]] = _RE_NEVER_MATCH
    # One group (target):
    _re_jump: typing.ClassVar[re.Pattern[str]] = _RE_NEVER_MATCH
    # No groups:
    _re_return: typing.ClassVar[re.Pattern[str]] = _RE_NEVER_MATCH
    text: str = ""
    globals: set[str] = dataclasses.field(default_factory=set)
    _re_small_const_1 = _RE_NEVER_MATCH
    _re_small_const_2 = _RE_NEVER_MATCH
    const_reloc = "<Not supported>"

    def __post_init__(self) -> None:
        # Split the code into a linked list of basic blocks. A basic block is an
        # optional label, followed by zero or more non-instruction lines,
        # followed by zero or more instruction lines (only the last of which may
        # be a branch, jump, or return):
        self.text = self._preprocess(self.path.read_text())
        block = self._root
        for line in self.text.splitlines():
            # See if we need to start a new block:
            if match := self._re_label.match(line):
                # Label. New block:
                block.link = block = self._lookup_label(match["label"])
                block.noninstructions.append(line)
                continue
            if match := self.re_global.match(line):
                self.globals.add(match["label"])
                block.noninstructions.append(line)
                continue
            if self._re_noninstructions.match(line):
                if block.instructions:
                    # Non-instruction lines. New block:
                    block.link = block = _Block()
                block.noninstructions.append(line)
                continue
            if block.target or not block.fallthrough:
                # Current block ends with a branch, jump, or return. New block:
                block.link = block = _Block()
            inst = self._parse_instruction(line)
            block.instructions.append(inst)
            if inst.is_branch():
                # A block ending in a branch has a target and fallthrough:
                assert inst.target is not None
                block.target = self._lookup_label(inst.target)
                assert block.fallthrough
            elif inst.kind == InstructionKind.CALL:
                # A block ending in a call has a target and return point after call:
                assert inst.target is not None
                block.target = self._lookup_label(inst.target)
                assert block.fallthrough
            elif inst.kind == InstructionKind.JUMP:
                # A block ending in a jump has a target and no fallthrough:
                assert inst.target is not None
                block.target = self._lookup_label(inst.target)
                block.fallthrough = False
            elif inst.kind == InstructionKind.RETURN:
                # A block ending in a return has no target and fallthrough:
                assert not block.target
                block.fallthrough = False

    def _preprocess(self, text: str) -> str:
        # Override this method to do preprocessing of the textual assembly.
        # In all cases, replace references to the _JIT_CONTINUE symbol with
        # references to a local _JIT_CONTINUE label (which we will add later):
        continue_symbol = rf"\b{re.escape(self.symbol_prefix)}_JIT_CONTINUE\b"
        continue_label = f"{self.label_prefix}_JIT_CONTINUE"
        return re.sub(continue_symbol, continue_label, text)

    def _parse_instruction(self, line: str) -> Instruction:
        target = None
        if match := self._re_branch.match(line):
            target = match["target"]
            name = match["instruction"]
            if name in self._short_branches:
                kind = InstructionKind.SHORT_BRANCH
            else:
                kind = InstructionKind.LONG_BRANCH
        elif match := self._re_jump.match(line):
            target = match["target"]
            name = line[: -len(target)].strip()
            kind = InstructionKind.JUMP
        elif match := self._re_call.match(line):
            target = match["target"]
            name = line[: -len(target)].strip()
            kind = InstructionKind.CALL
        elif match := self._re_return.match(line):
            name = line
            kind = InstructionKind.RETURN
        elif match := self._re_small_const_1.match(line):
            target = match["value"]
            name = match["instruction"]
            kind = InstructionKind.SMALL_CONST_1
        elif match := self._re_small_const_2.match(line):
            target = match["value"]
            name = match["instruction"]
            kind = InstructionKind.SMALL_CONST_2
        else:
            name, *_ = line.split(" ")
            kind = InstructionKind.OTHER
        return Instruction(kind, name, line, target)

    def _invert_branch(self, inst: Instruction, target: str) -> Instruction | None:
        assert inst.is_branch()
        if inst.kind == InstructionKind.SHORT_BRANCH and self._is_far_target(target):
            return None
        inverted_reloc = self._branches.get(inst.name)
        if inverted_reloc is None:
            return None
        inverted = inverted_reloc[0]
        if not inverted:
            return None
        return inst.update_name_and_target(inverted, target)

    def _lookup_label(self, label: str) -> _Block:
        if label not in self._labels:
            self._labels[label] = _Block(label)
        return self._labels[label]

    def _is_far_target(self, label: str) -> bool:
        return not label.startswith(self.label_prefix)

    def _blocks(self) -> typing.Generator[_Block, None, None]:
        block: _Block | None = self._root
        while block:
            yield block
            block = block.link

    def _body(self) -> str:
        lines = ["#" + line for line in self.text.splitlines()]
        hot = True
        for block in self._blocks():
            if hot != block.hot:
                hot = block.hot
                # Make it easy to tell at a glance where cold code is:
                lines.append(f"# JIT: {'HOT' if hot else 'COLD'} ".ljust(80, "#"))
            lines.extend(block.noninstructions)
            for inst in block.instructions:
                lines.append(inst.text)
        return "\n".join(lines)

    def _predecessors(self, block: _Block) -> typing.Generator[_Block, None, None]:
        # This is inefficient, but it's never wrong:
        for pre in self._blocks():
            if pre.target is block or pre.fallthrough and pre.link is block:
                yield pre

    def _insert_continue_label(self) -> None:
        # Find the block with the last instruction:
        for end in reversed(list(self._blocks())):
            if end.instructions:
                break
        # Before:
        #    jmp FOO
        # After:
        #    jmp FOO
        #    _JIT_CONTINUE:
        # This lets the assembler encode _JIT_CONTINUE jumps at build time!
        continuation = self._lookup_label(f"{self.label_prefix}_JIT_CONTINUE")
        assert continuation.label
        continuation.noninstructions.append(f"{continuation.label}:")
        end.link, continuation.link = continuation, end.link

    def _mark_hot_blocks(self) -> None:
        # Start with the last block, and perform a DFS to find all blocks that
        # can eventually reach it:
        todo = list(self._blocks())[-1:]
        while todo:
            block = todo.pop()
            block.hot = True
            todo.extend(pre for pre in self._predecessors(block) if not pre.hot)

    def _invert_hot_branches(self) -> None:
        for branch in self._blocks():
            link = branch.link
            if link is None:
                continue
            jump = link.resolve()
            # Before:
            #    je HOT
            #    jmp COLD
            # After:
            #    jne COLD
            #    jmp HOT
            if (
                # block ends with a branch to hot code...
                branch.target
                and branch.fallthrough
                and branch.target.hot
                # ...followed by a jump to cold code with no other predecessors:
                and jump.target
                and not jump.fallthrough
                and not jump.target.hot
                and len(jump.instructions) == 1
                and list(self._predecessors(jump)) == [branch]
            ):
                assert jump.target.label
                assert branch.target.label
                inverted = self._invert_branch(
                    branch.instructions[-1], jump.target.label
                )
                # Check to see if the branch can even be inverted:
                if inverted is None:
                    continue
                branch.instructions[-1] = inverted
                jump.instructions[-1] = jump.instructions[-1].update_target(
                    branch.target.label
                )
                branch.target, jump.target = jump.target, branch.target
                jump.hot = True

    def _remove_redundant_jumps(self) -> None:
        # Zero-length jumps can be introduced by _insert_continue_label and
        # _invert_hot_branches:
        for block in self._blocks():
            target = block.target
            if target is None:
                continue
            target = target.resolve()
            # Before:
            #    jmp FOO
            #    FOO:
            # After:
            #    FOO:
            if block.link and target is block.link.resolve():
                block.target = None
                block.fallthrough = True
                block.instructions.pop()
            # Before:
            #    branch  FOO:
            #    ...
            #    FOO:
            #    jump BAR
            # After:
            #    br cond BAR
            #    ...
            elif (
                len(target.instructions) == 1
                and target.instructions[0].kind == InstructionKind.JUMP
            ):
                assert target.target is not None
                assert target.target.label is not None
                if block.instructions[
                    -1
                ].kind == InstructionKind.SHORT_BRANCH and self._is_far_target(
                    target.target.label
                ):
                    continue
                block.target = target.target
                block.instructions[-1] = block.instructions[-1].update_target(
                    target.target.label
                )

    def _find_live_blocks(self) -> set[_Block]:
        live: set[_Block] = set()
        # Externally reachable blocks are live
        todo: set[_Block] = {b for b in self._blocks() if b.label in self.globals}
        while todo:
            block = todo.pop()
            live.add(block)
            if block.fallthrough:
                next = block.link
                if next is not None and next not in live:
                    todo.add(next)
            next = block.target
            if next is not None and next not in live:
                todo.add(next)
        return live

    def _remove_unreachable(self) -> None:
        live = self._find_live_blocks()
        continuation = self._lookup_label(f"{self.label_prefix}_JIT_CONTINUE")
        # Keep blocks after continuation as they may contain data and
        # metadata that the assembler needs
        prev: _Block | None = None
        block = self._root
        while block is not continuation:
            next = block.link
            assert next is not None
            if not block in live and prev:
                prev.link = next
            else:
                prev = block
            block = next
            assert prev.link is block

    def _fixup_external_labels(self) -> None:
        if self._supports_external_relocations:
            # Nothing to fix up
            return
        for index, block in enumerate(self._blocks()):
            if block.target and block.fallthrough:
                branch = block.instructions[-1]
                assert branch.is_branch() or branch.kind == InstructionKind.CALL
                target = branch.target
                assert target is not None
                reloc = self._branches[branch.name][1]
                if reloc is not None and self._is_far_target(target):
                    name = target[len(self.symbol_prefix) :]
                    label = f"{self.symbol_prefix}{reloc}_JIT_RELOCATION_{name}_JIT_RELOCATION_{index}:"
                    block.instructions[-1] = Instruction(
                        InstructionKind.OTHER, "", label, None
                    )
                    block.instructions.append(branch.update_target("0"))

    def _make_temp_label(self, index: int) -> Instruction:
        marker = f"jit_temp_{index}:"
        return Instruction(InstructionKind.OTHER, "", marker, None)

    def _fixup_constants(self) -> None:
        if not self.supports_small_constants:
            return
        index = 0
        for block in self._blocks():
            fixed: list[Instruction] = []
            small_const_index = -1
            for inst in block.instructions:
                if inst.kind == InstructionKind.SMALL_CONST_1:
                    marker = f"jit_pending_{inst.target}{index}:"
                    fixed.append(self._make_temp_label(index))
                    index += 1
                    small_const_index = len(fixed)
                    fixed.append(inst)
                elif inst.kind == InstructionKind.SMALL_CONST_2:
                    if small_const_index < 0:
                        fixed.append(inst)
                        continue
                    small_const_1 = fixed[small_const_index]
                    if not self._small_consts_match(small_const_1, inst):
                        small_const_index = -1
                        fixed.append(inst)
                        continue
                    assert small_const_1.target is not None
                    if small_const_1.target.endswith("16"):
                        fixed[small_const_index] = self._make_temp_label(index)
                        index += 1
                    else:
                        assert small_const_1.target.endswith("32")
                        patch_kind, replacement = self._small_const_1(small_const_1)
                        if replacement is not None:
                            label = f"{self.const_reloc}{patch_kind}_JIT_RELOCATION_CONST{small_const_1.target[:-3]}_JIT_RELOCATION_{index}:"
                            index += 1
                            fixed[small_const_index - 1] = Instruction(
                                InstructionKind.OTHER, "", label, None
                            )
                            fixed[small_const_index] = replacement
                    patch_kind, replacement = self._small_const_2(inst)
                    if replacement is not None:
                        assert inst.target is not None
                        label = f"{self.const_reloc}{patch_kind}_JIT_RELOCATION_CONST{inst.target[:-3]}_JIT_RELOCATION_{index}:"
                        index += 1
                        fixed.append(
                            Instruction(InstructionKind.OTHER, "", label, None)
                        )
                        fixed.append(replacement)
                    small_const_index = -1
                else:
                    fixed.append(inst)
            block.instructions = fixed

    def _small_const_1(self, inst: Instruction) -> tuple[str, Instruction | None]:
        raise NotImplementedError()

    def _small_const_2(self, inst: Instruction) -> tuple[str, Instruction | None]:
        raise NotImplementedError()

    def _small_consts_match(self, inst1: Instruction, inst2: Instruction) -> bool:
        raise NotImplementedError()

    def run(self) -> None:
        """Run this optimizer."""
        self._insert_continue_label()
        self._mark_hot_blocks()
        # Removing branches can expose opportunities for more branch removal.
        # Repeat a few times. 2 would probably do, but it's fast enough with 4.
        for _ in range(4):
            self._invert_hot_branches()
            self._remove_redundant_jumps()
            self._remove_unreachable()
        self._fixup_external_labels()
        self._fixup_constants()
        self.path.write_text(self._body())


class OptimizerAArch64(Optimizer):  # pylint: disable = too-few-public-methods
    """aarch64-pc-windows-msvc/aarch64-apple-darwin/aarch64-unknown-linux-gnu"""

    _branches = _AARCH64_BRANCHES
    _short_branches = _AARCH64_SHORT_BRANCHES
    # Mach-O does not support the 19 bit branch locations needed for branch reordering
    _supports_external_relocations = False
    _branch_patterns = [name.replace(".", r"\.") for name in _AARCH64_BRANCHES]
    _re_branch = re.compile(
        rf"\s*(?P<instruction>{'|'.join(_branch_patterns)})\s+(.+,\s+)*(?P<target>[\w.]+)"
    )

    # https://developer.arm.com/documentation/ddi0406/b/Application-Level-Architecture/Instruction-Details/Alphabetical-list-of-instructions/BL--BLX--immediate-
    _re_call = re.compile(r"\s*blx??\s+(?P<target>[\w.]+)")
    # https://developer.arm.com/documentation/ddi0602/2025-03/Base-Instructions/B--Branch-
    _re_jump = re.compile(r"\s*b\s+(?P<target>[\w.]+)")
    # https://developer.arm.com/documentation/ddi0602/2025-09/Base-Instructions/RET--Return-from-subroutine-
    _re_return = re.compile(r"\s*ret\b")

    supports_small_constants = True
    _re_small_const_1 = re.compile(
        r"\s*(?P<instruction>adrp)\s+.*(?P<value>_JIT_OP(ARG|ERAND(0|1))_(16|32)).*"
    )
    _re_small_const_2 = re.compile(
        r"\s*(?P<instruction>ldr)\s+.*(?P<value>_JIT_OP(ARG|ERAND(0|1))_(16|32)).*"
    )
    const_reloc = "CUSTOM_AARCH64_CONST"

    def _get_reg(self, inst: Instruction) -> str:
        _, rest = inst.text.split(inst.name)
        reg, *_ = rest.split(",")
        return reg.strip()

    def _small_const_1(self, inst: Instruction) -> tuple[str, Instruction | None]:
        assert inst.kind is InstructionKind.SMALL_CONST_1
        assert inst.target is not None
        if "16" in inst.target:
            return "", None
        pre, _ = inst.text.split(inst.name)
        return "16a", Instruction(
            InstructionKind.OTHER, "movz", f"{pre}movz {self._get_reg(inst)}, 0", None
        )

    def _small_const_2(self, inst: Instruction) -> tuple[str, Instruction | None]:
        assert inst.kind is InstructionKind.SMALL_CONST_2
        assert inst.target is not None
        pre, _ = inst.text.split(inst.name)
        if "16" in inst.target:
            return "16a", Instruction(
                InstructionKind.OTHER,
                "movz",
                f"{pre}movz {self._get_reg(inst)}, 0",
                None,
            )
        else:
            return "16b", Instruction(
                InstructionKind.OTHER,
                "movk",
                f"{pre}movk {self._get_reg(inst)}, 0, lsl #16",
                None,
            )

    def _small_consts_match(self, inst1: Instruction, inst2: Instruction) -> bool:
        reg1 = self._get_reg(inst1)
        reg2 = self._get_reg(inst2)
        return reg1 == reg2


class OptimizerX86(Optimizer):  # pylint: disable = too-few-public-methods
    """i686-pc-windows-msvc/x86_64-apple-darwin/x86_64-unknown-linux-gnu"""

    _branches = _X86_BRANCHES
    _short_branches = {}
    _re_branch = re.compile(
        rf"\s*(?P<instruction>{'|'.join(_X86_BRANCHES)})\s+(?P<target>[\w.]+)"
    )
    # https://www.felixcloutier.com/x86/call
    _re_call = re.compile(r"\s*callq?\s+(?P<target>[\w.]+)")
    # https://www.felixcloutier.com/x86/jmp
    _re_jump = re.compile(r"\s*jmp\s+(?P<target>[\w.]+)")
    # https://www.felixcloutier.com/x86/ret
    _re_return = re.compile(r"\s*ret\b")
