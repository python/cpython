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
ARM64_RELOC_BRANCH26: str | None = "ARM64_RELOC_BRANCH26"

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
    SMALL_CONST_MASK = enum.auto()
    LARGE_CONST_1 = enum.auto()
    LARGE_CONST_2 = enum.auto()
    OTHER = enum.auto()


@dataclasses.dataclass
class Instruction:
    kind: InstructionKind
    name: str
    text: str
    register: str | None
    target: str | None

    def is_branch(self) -> bool:
        return self.kind in (InstructionKind.LONG_BRANCH, InstructionKind.SHORT_BRANCH)

    def update_target(self, target: str) -> "Instruction":
        assert self.target is not None
        return Instruction(
            self.kind,
            self.name,
            self.text.replace(self.target, target),
            self.register,
            target,
        )

    def update_name_and_target(self, name: str, target: str) -> "Instruction":
        assert self.target is not None
        return Instruction(
            self.kind,
            name,
            self.text.replace(self.name, name).replace(self.target, target),
            self.register,
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
    # Whether this block should be emitted in the hot section. This is separate
    # from "hot": some cold fallthrough bridges must stay in the hot layout.
    layout_hot: bool | None = None
    # Whether this original assembler metadata/tail block should be preserved
    # even if it is unreachable.
    is_metadata: bool = False

    def resolve(self) -> typing.Self:
        """Find the first non-empty block reachable from this one."""
        block = self
        # Empty blocks are only transparent when control can fall through them:
        #     .L1:
        #     .L2:
        #         mov x0, x1
        # We set block.fallthrough to False for sentinels like _JIT_CONTINUE,
        # which may be empty but still must not resolve through to
        # _JIT_COLD_START.
        while block.link and not block.instructions and block.fallthrough:
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
    frame_pointers: bool
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
    _re_small_const_mask = _RE_NEVER_MATCH
    _re_large_const_1 = _RE_NEVER_MATCH
    _re_large_const_2 = _RE_NEVER_MATCH
    const_reloc = "<Not supported>"
    _frame_pointer_modify: typing.ClassVar[re.Pattern[str]] = _RE_NEVER_MATCH
    label_index: int = 0
    _cold_start: _Block | None = dataclasses.field(init=False, default=None)
    _jump_name = "<Not supported>"
    _jump_reloc: typing.ClassVar[str | None] = None

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
        reg = None
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
            reg = match["register"]
            kind = InstructionKind.SMALL_CONST_1
        elif match := self._re_small_const_2.match(line):
            target = match["value"]
            name = match["instruction"]
            reg = match["register"]
            kind = InstructionKind.SMALL_CONST_2
        elif match := self._re_small_const_mask.match(line):
            target = match["value"]
            name = match["instruction"]
            reg = match["register"]
            if reg.startswith("w"):
                reg = "x" + reg[1:]
            kind = InstructionKind.SMALL_CONST_MASK
        elif match := self._re_large_const_1.match(line):
            target = match["value"]
            name = match["instruction"]
            reg = match["register"]
            kind = InstructionKind.LARGE_CONST_1
        elif match := self._re_large_const_2.match(line):
            target = match["value"]
            name = match["instruction"]
            reg = match["register"]
            kind = InstructionKind.LARGE_CONST_2
        else:
            name, *_ = line.split(" ")
            kind = InstructionKind.OTHER
        return Instruction(kind, name, line, reg, target)

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

    def _continuation(self) -> _Block:
        return self._lookup_label(f"{self.label_prefix}_JIT_CONTINUE")

    def _cold_start_block(self) -> _Block:
        """Create the marker block where the cold code section begins."""
        if self._cold_start is None:
            label = f"{self.symbol_prefix}_JIT_COLD_START"
            self._cold_start = self._lookup_label(label)
            self._cold_start.noninstructions.append(f"{label}:")
            self._cold_start.layout_hot = False
        return self._cold_start

    def _make_label(self) -> str:
        label = f"{self.label_prefix}_JIT_LABEL_{self.label_index}"
        self.label_index += 1
        return label

    def _make_symbol_label(self, name: str) -> str:
        label = f"{self.symbol_prefix}{name}_{self.label_index}"
        self.label_index += 1
        return label

    def _ensure_label(self, block: _Block) -> str:
        if block.label is None:
            block.label = self._make_label()
            self._labels[block.label] = block
            block.noninstructions.insert(0, f"{block.label}:")
        return block.label

    def _make_cold_target(self, block: _Block) -> str:
        label = self._make_symbol_label("_JIT_COLD")
        self._labels[label] = block
        block.noninstructions.insert(0, f"{label}:")
        return label

    def _make_cross_section_target(self, block: _Block) -> str:
        real_target = self._make_cold_target(block).removeprefix(self.symbol_prefix)
        return (
            f"{self.symbol_prefix}_JIT_CROSS_SECTION_"
            f"{real_target}_JIT_CROSS_SECTION"
        )

    def _relocation_marker(self, reloc: str, block: _Block) -> Instruction:
        """Create a marker symbol that _stencils.py converts to a relocation."""
        target = self._make_cross_section_target(block).removeprefix(self.symbol_prefix)
        label = f"{self.symbol_prefix}{reloc}_JIT_RELOCATION_{target}:"
        return Instruction(InstructionKind.OTHER, "", label, None, None)

    def _make_jump(self, target: _Block, *, hot: bool) -> _Block:
        label = self._ensure_label(target)
        return _Block(
            instructions=[
                Instruction(
                    InstructionKind.JUMP,
                    self._jump_name,
                    f"\t{self._jump_name} {label}",
                    None,
                    label,
                )
            ],
            target=target,
            fallthrough=False,
            hot=hot,
            layout_hot=True,
        )

    def _effective_layout_hot(self, block: _Block) -> bool:
        """Return the hot/cold section where a block should be emitted.

        Empty label-only blocks inherit the layout of the block they resolve to.
        Explicit layout_hot overrides are also respected, because a block can be
        cold in the control-flow graph but still need to be emitted in the hot
        section as a bridge.
        """
        resolved = block.resolve()
        if resolved.layout_hot is not None:
            return resolved.layout_hot
        if block.layout_hot is not None:
            return block.layout_hot
        return resolved.hot

    def _same_layout_section(self, left: _Block, right: _Block) -> bool:
        return self._effective_layout_hot(left) == self._effective_layout_hot(right)

    def _can_short_branch_to_layout(
        self, inst: Instruction, source: _Block, target: _Block
    ) -> bool:
        if inst.kind != InstructionKind.SHORT_BRANCH:
            return True
        return self._same_layout_section(source, target)

    def _insert_fallthrough_bridge(self, block: _Block, target: _Block) -> _Block:
        bridge = self._make_jump(target, hot=target.hot)
        bridge.link = block.link
        block.link = bridge
        return bridge

    def _ensure_hot_fallthrough(self, block: _Block) -> None:
        if block is self._continuation() or block is self._cold_start:
            return
        fallthrough = block.link
        if (
            fallthrough is None
            or not self._effective_layout_hot(block)
            or not block.fallthrough
        ):
            return

        fallthrough_is_hot = self._effective_layout_hot(fallthrough)
        target = block.target
        inst = block.instructions[-1] if block.instructions else None

        # Keep AArch64 short branches in the hot layout:
        #     tbz x0, #0, .Lcold      ->      tbnz x0, #0, .Lhot
        # .Lhot:                              b .Lcold
        #                                 .Lhot:
        if (
            fallthrough_is_hot
            and inst is not None
            and inst.kind == InstructionKind.SHORT_BRANCH
            and target is not None
            and not self._effective_layout_hot(target)
        ):
            fallthrough_label = self._ensure_label(fallthrough)
            inverted = self._invert_branch(inst, fallthrough_label)
            assert inverted is not None
            bridge = self._make_jump(target, hot=target.hot)
            bridge.link = fallthrough
            block.instructions[-1] = inverted
            block.target = fallthrough
            block.link = bridge
            return

        if fallthrough_is_hot:
            return

        # Make a hot-to-cold fallthrough explicit, preferably by inverting:
        #     b.eq .Lhot              ->      b.ne .Lcold
        # .Lcold:                         .Lhot:
        if (
            inst is not None
            and inst.is_branch()
            and target is not None
            and self._effective_layout_hot(target)
        ):
            fallthrough_label = self._ensure_label(fallthrough)
            inverted = None
            if self._can_short_branch_to_layout(inst, block, fallthrough):
                inverted = self._invert_branch(inst, fallthrough_label)
            if inverted is not None:
                bridge = self._make_jump(target, hot=True)
                bridge.link = fallthrough
                block.instructions[-1] = inverted
                block.target = fallthrough
                block.link = bridge
                return
        # If no inversion is possible, preserve the old fallthrough with:
        #     b .Lcold
        self._insert_fallthrough_bridge(block, fallthrough)

    def _layout_units(self) -> list[tuple[bool, list[_Block]]]:
        return self._layout_units_from(list(self._layout_blocks()))

    def _layout_units_from(
        self, blocks: list[_Block]
    ) -> list[tuple[bool, list[_Block]]]:
        """Group adjacent blocks that must stay together during layout.

        Label-only fallthrough blocks inherit the layout of the instruction
        block they resolve to, so partitioning must move them as one unit.
        """
        continuation = self._continuation()
        cold_start = self._cold_start_block()
        units: list[tuple[bool, list[_Block]]] = []
        unit: list[_Block] = []

        def finish_unit() -> None:
            nonlocal unit
            if unit:
                layout_hot = self._effective_layout_hot(unit[0])
                for unit_block in unit:
                    unit_block.layout_hot = layout_hot
                units.append((layout_hot, unit))
                unit = []

        for block in blocks:
            if block is continuation or block is cold_start:
                finish_unit()
                continue
            unit.append(block)
            if block.instructions or not block.fallthrough:
                finish_unit()
        finish_unit()
        return units

    def _metadata_blocks(self) -> list[_Block]:
        return [block for block in self._blocks() if block.is_metadata]

    def _relink_blocks(self, blocks: list[_Block]) -> None:
        for current, next_block in zip(blocks, blocks[1:]):
            current.link = next_block
        if blocks:
            blocks[-1].link = None

    def _split_at_entry(self) -> tuple[list[_Block], list[_Block]]:
        entry = self._lookup_label(f"{self.symbol_prefix}_JIT_ENTRY")
        layout_blocks = list(self._layout_blocks())
        index = layout_blocks.index(entry)
        return layout_blocks[:index], layout_blocks[index:]

    def _partition_hot_cold_blocks(self) -> None:
        # The entry point must remain first in the partitioned JIT body, even
        # when it can't reach _JIT_CONTINUE. Blocks before _JIT_ENTRY are
        # assembler prefix material; keep their original order before
        # partitioning the JIT body.
        prefix_blocks, body_blocks = self._split_at_entry()
        for block in prefix_blocks:
            block.layout_hot = True
        body_blocks[0].layout_hot = True

        for block in list(body_blocks):
            self._ensure_hot_fallthrough(block)

        continuation = self._continuation()
        continuation.layout_hot = True
        continuation.fallthrough = False
        cold_start = self._cold_start_block()
        cold_start.layout_hot = False
        cold_start.fallthrough = True

        prefix_blocks, body_blocks = self._split_at_entry()
        for block in prefix_blocks:
            block.layout_hot = True
        units = self._layout_units_from(body_blocks)
        hot_blocks = [
            block for layout_hot, unit in units if layout_hot for block in unit
        ]
        cold_blocks = [
            block for layout_hot, unit in units if not layout_hot for block in unit
        ]
        blocks = [
            *prefix_blocks,
            *hot_blocks,
            continuation,
            cold_start,
            *cold_blocks,
            *self._metadata_blocks(),
        ]
        self._relink_blocks(blocks)
        linked_blocks = list(self._blocks())
        assert linked_blocks[0] is self._root
        assert continuation in linked_blocks
        assert cold_start in linked_blocks

    def _blocks(self) -> typing.Generator[_Block, None, None]:
        block: _Block | None = self._root
        while block:
            yield block
            block = block.link

    def _layout_blocks(self) -> typing.Generator[_Block, None, None]:
        """Yield only blocks that participate in executable code layout."""
        for block in self._blocks():
            if not block.is_metadata:
                yield block

    def _body(self) -> str:
        lines = ["#" + line for line in self.text.splitlines()]
        hot: bool | None = True
        for block in self._blocks():
            layout_hot = block.layout_hot
            if layout_hot is None:
                layout_hot = block.hot
            if hot != layout_hot:
                hot = layout_hot
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
        continuation.layout_hot = True
        tail = end.link
        while tail:
            tail.is_metadata = True
            tail = tail.link
        end.link, continuation.link = continuation, end.link

    def _mark_hot_blocks(self) -> None:
        # Start with the continuation block, and perform a DFS to find all
        # blocks that can eventually reach it:
        todo = [self._lookup_label(f"{self.label_prefix}_JIT_CONTINUE")]
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
                jump.layout_hot = True
                assert jump.target.label
                assert branch.target.label
                inst = branch.instructions[-1]
                if inst.kind == InstructionKind.SHORT_BRANCH:
                    inverted = None
                else:
                    inverted = self._invert_branch(inst, jump.target.label)
                # Check to see if the branch can even be inverted:
                if inverted is None:
                    continue
                branch.instructions[-1] = inverted
                jump.instructions[-1] = jump.instructions[-1].update_target(
                    branch.target.label
                )
                branch.target, jump.target = jump.target, branch.target
                jump.hot = True
                jump.layout_hot = True

    def _remove_redundant_jumps(self) -> None:
        # Zero-length jumps can be introduced by _insert_continue_label and
        # _invert_hot_branches:
        continuation = self._continuation()
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
            if (
                block.link
                and target is block.link.resolve()
                and (self._same_layout_section(block, target) or target is continuation)
            ):
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
                inst = block.instructions[-1]
                if block.instructions[
                    -1
                ].kind == InstructionKind.SHORT_BRANCH and self._is_far_target(
                    target.target.label
                ):
                    continue
                if not self._can_short_branch_to_layout(inst, block, target.target):
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
        continuation = self._continuation()
        entry = self._lookup_label(f"{self.symbol_prefix}_JIT_ENTRY")
        cont_or_cold_blocks = {continuation}
        if self._cold_start is not None:
            cont_or_cold_blocks.add(self._cold_start)
        # Keep only the original assembler tail. Cold code after _JIT_CONTINUE
        # is ordinary code and can be removed when unreachable.
        prev: _Block | None = None
        block: _Block | None = self._root
        seen_entry = False
        # We now walk the whole list, so keep explicit sentinel checks in place
        # of the old "stop at _JIT_CONTINUE" loop invariant.
        seen_continuation = False
        seen_cold_start = self._cold_start is None
        while block is not None:
            preserve_prefix = not seen_entry
            if block is entry:
                seen_entry = True
            if block is continuation:
                seen_continuation = True
            if block is self._cold_start:
                seen_cold_start = True
            next = block.link
            if (
                not preserve_prefix
                and block not in live
                and block not in cont_or_cold_blocks
                and not block.is_metadata
                and prev is not None
            ):
                prev.link = next
            else:
                prev = block
            block = next
            if prev is not None:
                assert prev.link is block
        assert seen_continuation
        assert seen_cold_start

    def _insert_cross_section_branch_relocations(self) -> None:
        """Insert relocation markers for branches crossing the hot/cold split."""
        layout_blocks = set(self._layout_blocks())
        for block in self._layout_blocks():
            target = block.target
            if target is None or target not in layout_blocks:
                continue
            if self._same_layout_section(block, target):
                continue
            inst = block.instructions[-1]
            if inst.kind == InstructionKind.LONG_BRANCH:
                reloc = self._branches[inst.name][1]
                if reloc is not None:
                    block.instructions[-1] = self._relocation_marker(reloc, target)
                    block.instructions.append(inst.update_target("0"))
                    continue
            elif inst.kind == InstructionKind.JUMP and self._jump_reloc is not None:
                block.instructions[-1] = self._relocation_marker(
                    self._jump_reloc, target
                )
                block.instructions.append(inst.update_target("0"))
                continue
            assert inst.kind is not InstructionKind.SHORT_BRANCH
            block.instructions[-1] = inst.update_target(
                self._make_cross_section_target(target)
            )

    def _fixup_external_labels(self) -> None:
        if self._supports_external_relocations:
            # Nothing to fix up
            return
        for index, block in enumerate(self._blocks()):
            if block.target and block.fallthrough:
                branch = block.instructions[-1]
                if branch.kind == InstructionKind.CALL:
                    continue
                assert branch.is_branch()
                target = branch.target
                assert target is not None
                reloc = self._branches[branch.name][1]
                if reloc is not None and self._is_far_target(target):
                    name = target[len(self.symbol_prefix) :]
                    label = f"{self.symbol_prefix}{reloc}_JIT_RELOCATION_{name}_JIT_RELOCATION_{index}:"
                    block.instructions[-1] = Instruction(
                        InstructionKind.OTHER, "", label, None, None
                    )
                    block.instructions.append(branch.update_target("0"))

    def _fixup_constants(self) -> None:
        "Fixup loading of constants. Overridden by OptimizerAArch64"
        pass

    def _validate(self) -> None:
        for block in self._blocks():
            if not block.instructions:
                continue
            for inst in block.instructions:
                if self.frame_pointers:
                    assert (
                        self._frame_pointer_modify.match(inst.text) is None
                    ), "Frame pointer should not be modified"

    def run(self) -> None:
        """Run this optimizer."""
        self._insert_continue_label()
        self._mark_hot_blocks()
        # Removing branches can expose opportunities for more branch removal.
        # Repeat a few times. 2 would probably do, but it's fast enough with 4.
        for iter in range(4):
            self._invert_hot_branches()
            if iter == 0:
                self._partition_hot_cold_blocks()
            self._remove_redundant_jumps()
            self._remove_unreachable()
        self._fixup_external_labels()
        self._insert_cross_section_branch_relocations()
        self._fixup_constants()
        self._validate()
        self.path.write_text(self._body())


class OptimizerAArch64(Optimizer):  # pylint: disable = too-few-public-methods
    """aarch64-pc-windows-msvc/aarch64-apple-darwin/aarch64-unknown-linux-gnu"""

    _branches = _AARCH64_BRANCHES
    _short_branches = _AARCH64_SHORT_BRANCHES
    _jump_name = "b"
    _jump_reloc = ARM64_RELOC_BRANCH26
    # Mach-O does not support the 19 bit branch locations needed for branch reordering
    _supports_external_relocations = False
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

    supports_small_constants = True
    _re_small_const_1 = re.compile(
        r"\s*(?P<instruction>adrp)\s+(?P<register>x\d\d?),.*(?P<value>_JIT_OP(ARG|ERAND(0|1))_(16|32)).*"
    )
    _re_small_const_2 = re.compile(
        r"\s*(?P<instruction>ldr)\s+(?P<register>x\d\d?),.*(?P<value>_JIT_OP(ARG|ERAND(0|1))_(16|32)).*"
    )
    _re_small_const_mask = re.compile(
        r"\s*(?P<instruction>and)\s+[xw]\d\d?, *(?P<register>[xw]\d\d?).*(?P<value>0xffff)"
    )
    _re_large_const_1 = re.compile(
        r"\s*(?P<instruction>adrp)\s+(?P<register>x\d\d?),.*:got:(?P<value>[_A-Za-z0-9]+).*"
    )
    _re_large_const_2 = re.compile(
        r"\s*(?P<instruction>ldr)\s+(?P<register>x\d\d?),.*:got_lo12:(?P<value>[_A-Za-z0-9]+).*"
    )
    const_reloc = "CUSTOM_AARCH64_CONST"
    _frame_pointer_modify = re.compile(r"\s*stp\s+x29.*")

    def _make_temp_label(self, note: object = None) -> Instruction:
        marker = f"jit_temp_{self.label_index}:"
        if note is not None:
            marker = f"{marker[:-1]}_{note}:"
        self.label_index += 1
        return Instruction(InstructionKind.OTHER, "", marker, None, None)

    def _both_registers_same(self, inst: Instruction) -> bool:
        reg = inst.register
        assert reg is not None
        if reg not in inst.text:
            reg = "w" + reg[1:]
        return inst.text.count(reg) == 2

    def _fixup_small_constant_pair(
        self, output: list[Instruction], label_index: int, inst: Instruction
    ) -> str | None:
        first = output[label_index + 1]
        reg = first.register
        if reg is None or inst.register != reg:
            output.append(
                Instruction(InstructionKind.OTHER, "", "# registers differ", None, None)
            )
            output.append(inst)
            return None
        assert first.target is not None
        if first.target != inst.target:
            output.append(
                Instruction(InstructionKind.OTHER, "", "# targets differ", None, None)
            )
            output.append(inst)
            return None
        if not self._both_registers_same(inst):
            output.append(
                Instruction(
                    InstructionKind.OTHER, "", "# not same register", None, None
                )
            )
            output.append(inst)
            return None
        pre, _ = first.text.split(first.name)
        output[label_index + 1] = Instruction(
            InstructionKind.OTHER,
            "movz",
            f"{pre}movz {reg}, 0",
            reg,
            None,
        )
        label_text = f"{self.const_reloc}16a_JIT_RELOCATION_CONST{first.target[:-3]}_JIT_RELOCATION_{self.label_index}:"
        self.label_index += 1
        output[label_index] = Instruction(
            InstructionKind.OTHER, "", label_text, None, None
        )
        assert first.target.endswith("16") or first.target.endswith("32")
        if first.target.endswith("32"):
            label_text = f"{self.const_reloc}16b_JIT_RELOCATION_CONST{first.target[:-3]}_JIT_RELOCATION_{self.label_index}:"
            self.label_index += 1
            output.append(
                Instruction(InstructionKind.OTHER, "", label_text, None, None)
            )
            pre, _ = inst.text.split(inst.name)
            output.append(
                Instruction(
                    InstructionKind.OTHER,
                    "movk",
                    f"{pre}movk {reg}, 0, lsl #16",
                    reg,
                    None,
                )
            )
        return reg

    def may_use_reg(self, inst: Instruction, reg: str | None) -> bool:
        "Return False if `reg` is not explicitly used by this instruction"
        if reg is None:
            return False
        assert reg.startswith("w") or reg.startswith("x")
        xreg = f"x{reg[1:]}"
        wreg = f"w{reg[1:]}"
        if wreg in inst.text:
            return True
        if xreg in inst.text:
            # Exclude false positives like 0x80 for x8
            count = inst.text.count(xreg)
            number_count = inst.text.count("0" + xreg)
            return count > number_count
        return False

    def _fixup_large_constant_pair(
        self, output: list[Instruction], label_index: int, inst: Instruction
    ) -> None:
        first = output[label_index + 1]
        reg = first.register
        if reg is None or inst.register != reg:
            output.append(inst)
            return
        assert first.target is not None
        if first.target != inst.target:
            output.append(inst)
            return
        label = f"{self.const_reloc}33a_JIT_PAIR_{first.target}_JIT_PAIR_{self.label_index}:"
        output[label_index] = Instruction(InstructionKind.OTHER, "", label, None, None)
        label = (
            f"{self.const_reloc}33b_JIT_PAIR_{inst.target}_JIT_PAIR_{self.label_index}:"
        )
        self.label_index += 1
        output.append(Instruction(InstructionKind.OTHER, "", label, None, None))
        output.append(inst)

    def _fixup_mask(self, output: list[Instruction], inst: Instruction) -> None:
        if self._both_registers_same(inst):
            # Nop
            pass
        else:
            output.append(inst)

    def _fixup_constants(self) -> None:
        for block in self._blocks():
            fixed: list[Instruction] = []
            small_const_part: dict[str, int | None] = {}
            small_const_whole: dict[str, str | None] = {}
            large_const_part: dict[str, int | None] = {}
            for inst in block.instructions:
                if inst.kind == InstructionKind.SMALL_CONST_1:
                    assert inst.register is not None
                    small_const_part[inst.register] = len(fixed)
                    small_const_whole[inst.register] = None
                    large_const_part[inst.register] = None
                    fixed.append(self._make_temp_label(inst.register))
                    fixed.append(inst)
                elif inst.kind == InstructionKind.SMALL_CONST_2:
                    assert inst.register is not None
                    index = small_const_part.get(inst.register)
                    small_const_part[inst.register] = None
                    if index is None:
                        fixed.append(inst)
                        continue
                    small_const_whole[inst.register] = self._fixup_small_constant_pair(
                        fixed, index, inst
                    )
                    small_const_part[inst.register] = None
                elif inst.kind == InstructionKind.SMALL_CONST_MASK:
                    assert inst.register is not None
                    reg = small_const_whole.get(inst.register)
                    if reg is not None:
                        self._fixup_mask(fixed, inst)
                    else:
                        fixed.append(inst)
                elif inst.kind == InstructionKind.LARGE_CONST_1:
                    assert inst.register is not None
                    small_const_part[inst.register] = None
                    small_const_whole[inst.register] = None
                    large_const_part[inst.register] = len(fixed)
                    fixed.append(self._make_temp_label())
                    fixed.append(inst)
                elif inst.kind == InstructionKind.LARGE_CONST_2:
                    assert inst.register is not None
                    small_const_part[inst.register] = None
                    small_const_whole[inst.register] = None
                    index = large_const_part.get(inst.register)
                    large_const_part[inst.register] = None
                    if index is None:
                        fixed.append(inst)
                        continue
                    self._fixup_large_constant_pair(fixed, index, inst)
                else:
                    for reg in small_const_part:
                        if self.may_use_reg(inst, reg):
                            small_const_part[reg] = None
                    for reg in small_const_whole:
                        if self.may_use_reg(inst, reg):
                            small_const_whole[reg] = None
                    for reg in small_const_part:
                        if self.may_use_reg(inst, reg):
                            large_const_part[reg] = None
                    fixed.append(inst)
            block.instructions = fixed


class OptimizerX86(Optimizer):  # pylint: disable = too-few-public-methods
    """i686-pc-windows-msvc/x86_64-apple-darwin/x86_64-unknown-linux-gnu"""

    _branches = _X86_BRANCHES
    _short_branches = {}
    _jump_name = "jmp"
    _re_branch = re.compile(
        rf"\s*(?P<instruction>{'|'.join(_X86_BRANCHES)})\s+(?P<target>[\w.]+)"
    )
    # https://www.felixcloutier.com/x86/call
    _re_call = re.compile(r"\s*callq?\s+(?P<target>[\w.]+)")
    # https://www.felixcloutier.com/x86/jmp
    _re_jump = re.compile(r"\s*jmp\s+(?P<target>[\w.]+)")
    # https://www.felixcloutier.com/x86/ret
    _re_return = re.compile(r"\s*retq?\b")
    _frame_pointer_modify = re.compile(r"\s*movq?\s+%(\w+),\s+%rbp.*")
