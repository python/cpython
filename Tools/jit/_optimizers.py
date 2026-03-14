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
_AARCH64_SHORT_BRANCHES = {
    "tbz": "tbnz",
    "tbnz": "tbz",
}

# Branches are either b.{cond}, bc.{cond}, cbz, cbnz, tbz or tbnz.
# Second tuple element unused (was for relocation fixup, now handled by DynASM).
_AARCH64_BRANCHES: dict[str, tuple[str | None, str | None]] = (
    {
        "b." + cond: (("b." + inverse if inverse else None), None)
        for (cond, inverse) in _AARCH64_COND_CODES.items()
    }
    | {
        "bc." + cond: (("bc." + inverse if inverse else None), None)
        for (cond, inverse) in _AARCH64_COND_CODES.items()
    }
    | {
        "cbz": ("cbnz", None),
        "cbnz": ("cbz", None),
    }
    | {
        cond: (inverse, None)
        for (cond, inverse) in _AARCH64_SHORT_BRANCHES.items()
    }
)


@enum.unique
class InstructionKind(enum.Enum):
    JUMP = enum.auto()
    LONG_BRANCH = enum.auto()
    SHORT_BRANCH = enum.auto()
    CALL = enum.auto()
    RETURN = enum.auto()
    OTHER = enum.auto()


@dataclasses.dataclass
class Instruction:
    kind: InstructionKind
    name: str
    text: str
    target: str | None

    def is_branch(self) -> bool:
        return self.kind in (
            InstructionKind.LONG_BRANCH,
            InstructionKind.SHORT_BRANCH,
        )

    def update_target(self, target: str) -> "Instruction":
        assert self.target is not None
        return Instruction(
            self.kind,
            self.name,
            self.text.replace(self.target, target),
            target,
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
    _labels: dict[str, _Block] = dataclasses.field(
        init=False, default_factory=dict
    )
    # No groups:
    _re_noninstructions: typing.ClassVar[re.Pattern[str]] = re.compile(
        r"\s*(?:\.|#|//|;|$)"
    )
    # One group (label):
    _re_label: typing.ClassVar[re.Pattern[str]] = re.compile(
        r'\s*(?P<label>[\w."$?@]+):'
    )
    # Override everything that follows in subclasses:
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
                if inst.target is not None:
                    block.target = self._lookup_label(inst.target)
                assert block.fallthrough
            elif inst.kind == InstructionKind.JUMP:
                # A block ending in a jump has a target and no fallthrough:
                if inst.target is not None:
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
        text = re.sub(continue_symbol, continue_label, text)
        # Keep metadata-only directives out of the assembled stencil body:
        text = re.sub(
            r'^\s*\.section\s+"?\.note\.GNU-stack.*$\n?',
            "",
            text,
            flags=re.MULTILINE,
        )
        # Cross-section hot/cold splitting can make these non-absolute:
        text = re.sub(
            r"^\s*\.size\s+[\w.]+,\s+.*$\n?",
            "",
            text,
            flags=re.MULTILINE,
        )
        # Keep type directives only for _JIT_ENTRY:
        text = re.sub(
            r"^\s*\.type\s+(?!_JIT_ENTRY\s)[\w.]+,@function\s*$\n?",
            "",
            text,
            flags=re.MULTILINE,
        )
        return text

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
        else:
            name, *_ = line.split(" ")
            kind = InstructionKind.OTHER
        return Instruction(kind, name, line, target)

    def _invert_branch(
        self, inst: Instruction, target: str
    ) -> Instruction | None:
        assert inst.is_branch()
        if inst.kind == InstructionKind.SHORT_BRANCH and self._is_far_target(
            target
        ):
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

    _cold_section_directive: typing.ClassVar[str | None] = None
    _hot_section_directive: typing.ClassVar[str | None] = None

    def _body(self) -> str:
        lines = ["#" + line for line in self.text.splitlines()]
        split_cold = self._cold_section_directive is not None
        if not split_cold:
            hot = True
            for block in self._blocks():
                if hot != block.hot:
                    hot = block.hot
                    # Make it easy to tell at a glance where cold code is:
                    lines.append(
                        f"# JIT: {'HOT' if hot else 'COLD'} ".ljust(80, "#")
                    )
                lines.extend(block.noninstructions)
                for inst in block.instructions:
                    lines.append(inst.text)
            return "\n".join(lines)

        hot_lines: list[str] = []
        cold_lines: list[str] = []
        continue_label = f"{self.label_prefix}_JIT_CONTINUE"
        has_cold = any(not block.hot for block in self._blocks())

        split_label_counter = [0]

        def _ensure_label(block: _Block) -> str:
            if block.label is not None:
                return block.label
            for noninstruction in block.noninstructions:
                stripped = noninstruction.strip()
                if stripped.endswith(":") and not stripped.startswith("#"):
                    label = stripped[:-1]
                    block.label = label
                    return label
            label = f"{self.label_prefix}_JIT_SPLIT_{split_label_counter[0]}"
            split_label_counter[0] += 1
            block.label = label
            block.noninstructions.insert(0, f"{label}:")
            return label

        for block in self._blocks():
            if has_cold and block.label == continue_label:
                continue
            dest = hot_lines if block.hot else cold_lines
            dest.extend(block.noninstructions)
            instructions = list(block.instructions)
            # Splitting sections can make a jump to the next emitted block in
            # the same section redundant.
            if (
                instructions
                and instructions[-1].kind == InstructionKind.JUMP
                and block.target is not None
            ):
                next_same = block.link
                while next_same and (
                    next_same.hot != block.hot
                    or (has_cold and next_same.label == continue_label)
                ):
                    next_same = next_same.link
                if next_same and block.target.resolve() is next_same.resolve():
                    instructions.pop()
            for inst in instructions:
                dest.append(inst.text)
            # Fallthrough crosses sections, so make it explicit:
            if (
                block.fallthrough
                and block.link
                and block.hot != block.link.hot
            ):
                target = _ensure_label(block.link)
                # Prefer inverting a final hot branch to avoid the extra
                # branch+jump sequence:
                #     jcc HOT; jmp COLD
                #  -> jncc COLD   (HOT fallthrough)
                if (
                    block.hot
                    and block.instructions
                    and block.instructions[-1].is_branch()
                    and block.target
                    and block.target.hot
                ):
                    next_hot = block.link
                    while next_hot and not next_hot.hot:
                        next_hot = next_hot.link
                    if (
                        next_hot is not None
                        and block.target.resolve() is next_hot.resolve()
                    ):
                        inverted = self._invert_branch(
                            block.instructions[-1], target
                        )
                        if inverted is not None:
                            dest[-1] = inverted.text
                            continue
                dest.append(f"\tjmp\t{target}")

        lines.extend(hot_lines)
        if has_cold:
            assert self._hot_section_directive is not None
            assert self._cold_section_directive is not None
            lines.append(self._hot_section_directive)
            lines.append(f"{continue_label}:")
            lines.append(self._cold_section_directive)
            lines.extend(cold_lines)
            lines.append(self._hot_section_directive)
        return "\n".join(lines)

    def _predecessors(
        self, block: _Block
    ) -> typing.Generator[_Block, None, None]:
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
            todo.extend(
                pre for pre in self._predecessors(block) if not pre.hot
            )
        self._demote_cold_blocks()

    _call_pattern: typing.ClassVar[re.Pattern[str]] = re.compile(
        r"^\s*(?:callq?|blx?|blr)\b"
    )

    def _is_call_block(self, block: _Block) -> bool:
        if not block.instructions:
            return False
        for inst in block.instructions:
            if inst.kind == InstructionKind.CALL:
                return True
            if self._call_pattern.search(inst.text):
                return True
        return False

    def _successors(self, block: _Block) -> list[_Block]:
        successors = []
        if block.target is not None:
            successors.append(block.target.resolve())
        if block.fallthrough and block.link is not None:
            successors.append(block.link.resolve())
        return successors

    def _has_hot_passthrough(self, block: _Block, chain: set[_Block]) -> bool:
        """Check if demoting block would route hot traffic through cold."""
        has_hot_predecessor = any(
            pred.hot and pred not in chain
            for pred in self._predecessors(block)
        )
        if not has_hot_predecessor:
            return False
        return any(
            succ.hot and succ not in chain for succ in self._successors(block)
        )

    def _demote_cold_blocks(self) -> None:
        """Demote call blocks and their feeder paths to cold.

        First demotes blocks containing function calls, then blocks whose
        only purpose is feeding those cold regions.  The entry block is
        never demoted.
        """
        entry: _Block | None = None
        for block in self._blocks():
            if block.instructions:
                entry = block
                break
        # Phase 1: demote call blocks.
        changed = True
        while changed:
            changed = False
            for block in reversed(list(self._blocks())):
                if (
                    not block.hot
                    or block is entry
                    or not self._is_call_block(block)
                ):
                    continue
                chain = self._find_cold_chain(block)
                if chain is not None and any(b.hot for b in chain):
                    for b in chain:
                        b.hot = False
                    changed = True
        # Phase 2: demote feeder blocks that only exist to reach cold code.
        changed = True
        while changed:
            changed = False
            for block in reversed(list(self._blocks())):
                if (
                    not block.hot
                    or block is entry
                    or not block.instructions
                    or not any(not s.hot for s in self._successors(block))
                ):
                    continue
                chain = self._find_cold_chain(block)
                if chain is not None and any(b.hot for b in chain):
                    for b in chain:
                        b.hot = False
                    changed = True

    def _can_reach_without(
        self, start: _Block, target: _Block, avoid: set[_Block]
    ) -> bool:
        """Check if target is reachable from start without going through avoid."""
        visited: set[int] = set()
        todo = [start]
        while todo:
            block = todo.pop()
            block_id = id(block)
            if block_id in visited or block in avoid:
                continue
            visited.add(block_id)
            if block is target:
                return True
            if block.target is not None:
                todo.append(block.target)
            if block.fallthrough and block.link is not None:
                todo.append(block.link)
        return False

    def _find_cold_chain(self, cold_block: _Block) -> set[_Block] | None:
        chain: set[_Block] = {cold_block}
        changed = True
        while changed:
            changed = False
            for block in list(self._blocks()):
                if not block.hot or block in chain or not block.instructions:
                    continue
                # Pull predecessor gates into the chain if all successors are
                # already in the chain.
                target_ok = block.target is None or block.target in chain
                fallthrough_ok = (
                    not block.fallthrough
                    or block.link is None
                    or block.link in chain
                )
                if target_ok and fallthrough_ok:
                    chain.add(block)
                    changed = True
                    continue
                # Also include blocks only reachable from inside the chain.
                preds = list(self._predecessors(block))
                if preds and all(pred in chain for pred in preds):
                    chain.add(block)
                    changed = True

        # Every edge into the chain from outside must have a hot alternative.
        for block in chain:
            # Keep tiny branch gates that sit between hot predecessors and hot
            # successors in the hot section. They are frequently executed and
            # should not pay a cross-section jump penalty.
            if self._has_hot_passthrough(block, chain):
                return None
            for pred in self._predecessors(block):
                if pred in chain or not pred.hot:
                    continue
                has_hot_alt = False
                if (
                    pred.target
                    and pred.target not in chain
                    and pred.target.hot
                ):
                    has_hot_alt = True
                if (
                    pred.fallthrough
                    and pred.link
                    and pred.link not in chain
                    and pred.link.hot
                ):
                    has_hot_alt = True
                if not has_hot_alt:
                    return None

        # Reject chains that would disconnect the hot path from entry to
        # continuation.  A "hot alternative" like a loop backedge doesn't
        # help if the loop must always exit through the chain.
        entry = next((b for b in self._blocks() if b.instructions), None)
        continuation = self._labels.get(f"{self.label_prefix}_JIT_CONTINUE")
        if entry and continuation:
            if not self._can_reach_without(entry, continuation, chain):
                return None

        return chain

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
        todo: set[_Block] = {
            b for b in self._blocks() if b.label in self.globals
        }
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
        self.path.write_text(self._body())


class OptimizerAArch64(Optimizer):  # pylint: disable = too-few-public-methods
    """aarch64-unknown-linux-gnu"""

    _branches = _AARCH64_BRANCHES
    _short_branches = _AARCH64_SHORT_BRANCHES
    _branch_patterns = [name.replace(".", r"\.") for name in _AARCH64_BRANCHES]
    _re_branch = re.compile(
        rf"\s*(?P<instruction>{'|'.join(_branch_patterns)})\s+(.+,\s+)*(?P<target>[\w.]+)"
    )

    # https://developer.arm.com/documentation/ddi0406/b/Application-Level-Architecture/Instruction-Details/Alphabetical-list-of-instructions/BL--BLX--immediate-
    _re_call = re.compile(r"\s*blx?\s+(?P<target>[\w.]+)")
    # https://developer.arm.com/documentation/ddi0602/2025-03/Base-Instructions/B--Branch-
    _re_jump = re.compile(r"\s*b\s+(?P<target>[\w.]+)")
    # https://developer.arm.com/documentation/ddi0602/2025-09/Base-Instructions/RET--Return-from-subroutine-
    _re_return = re.compile(r"\s*ret\b")


class OptimizerX86(Optimizer):  # pylint: disable = too-few-public-methods
    """x86_64-unknown-linux-gnu"""

    _branches = _X86_BRANCHES
    _short_branches = {}
    _re_branch = re.compile(
        rf"\s*(?P<instruction>{'|'.join(_X86_BRANCHES)})\s+(?P<target>[\w.]+)"
    )
    # https://www.felixcloutier.com/x86/call
    _re_call = re.compile(r"\s*callq?\s+(?P<target>[\w.]+)")
    # https://www.felixcloutier.com/x86/jmp
    _re_jump = re.compile(r"\s*jmpq?\s+(?P<target>[\w.]+)")
    # https://www.felixcloutier.com/x86/ret
    _re_return = re.compile(r"\s*retq?\b")


class OptimizerAArch64ELF(OptimizerAArch64):
    _cold_section_directive = '\t.section\t.text.cold,"ax",@progbits'
    _hot_section_directive = "\t.text"


class OptimizerX86ELF(OptimizerX86):
    _cold_section_directive = '\t.section\t.text.cold,"ax",@progbits'
    _hot_section_directive = "\t.text"
