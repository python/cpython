"""Low-level optimization of textual assembly."""

import dataclasses
import pathlib
import re
import typing

# Same as saying "not string.startswith('')":
_RE_NEVER_MATCH = re.compile(r"(?!)")
# Dictionary mapping branch instructions to their inverted branch instructions.
# If a branch cannot be inverted, the value is None:
_X86_BRANCHES = {
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
_X86_BRANCHES |= {v: k for k, v in _X86_BRANCHES.items() if v}


@dataclasses.dataclass
class _Block:
    label: str | None = None
    # Non-instruction lines like labels, directives, and comments:
    noninstructions: list[str] = dataclasses.field(default_factory=list)
    # Instruction lines:
    instructions: list[str] = dataclasses.field(default_factory=list)
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
    # prefix used to mangle symbols on some platforms:
    prefix: str = ""
    # The first block in the linked list:
    _root: _Block = dataclasses.field(init=False, default_factory=_Block)
    _labels: dict[str, _Block] = dataclasses.field(init=False, default_factory=dict)
    # No groups:
    _re_noninstructions: typing.ClassVar[re.Pattern[str]] = re.compile(
        r"\s*(?:\.|#|//|$)"
    )
    # One group (label):
    _re_label: typing.ClassVar[re.Pattern[str]] = re.compile(
        r'\s*(?P<label>[\w."$?@]+):'
    )
    # Override everything that follows in subclasses:
    _alignment: typing.ClassVar[int] = 1
    _branches: typing.ClassVar[dict[str, str | None]] = {}
    # Two groups (instruction and target):
    _re_branch: typing.ClassVar[re.Pattern[str]] = _RE_NEVER_MATCH
    # One group (target):
    _re_jump: typing.ClassVar[re.Pattern[str]] = _RE_NEVER_MATCH
    # No groups:
    _re_return: typing.ClassVar[re.Pattern[str]] = _RE_NEVER_MATCH

    def __post_init__(self) -> None:
        # Split the code into a linked list of basic blocks. A basic block is an
        # optional label, followed by zero or more non-instruction lines,
        # followed by zero or more instruction lines (only the last of which may
        # be a branch, jump, or return):
        text = self._preprocess(self.path.read_text())
        block = self._root
        for line in text.splitlines():
            # See if we need to start a new block:
            if match := self._re_label.match(line):
                # Label. New block:
                block.link = block = self._lookup_label(match["label"])
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
            block.instructions.append(line)
            if match := self._re_branch.match(line):
                # A block ending in a branch has a target and fallthrough:
                block.target = self._lookup_label(match["target"])
                assert block.fallthrough
            elif match := self._re_jump.match(line):
                # A block ending in a jump has a target and no fallthrough:
                block.target = self._lookup_label(match["target"])
                block.fallthrough = False
            elif self._re_return.match(line):
                # A block ending in a return has no target and fallthrough:
                assert not block.target
                block.fallthrough = False

    def _preprocess(self, text: str) -> str:
        # Override this method to do preprocessing of the textual assembly:
        return text

    @classmethod
    def _invert_branch(cls, line: str, target: str) -> str | None:
        match = cls._re_branch.match(line)
        assert match
        inverted = cls._branches.get(match["instruction"])
        if not inverted:
            return None
        (a, b), (c, d) = match.span("instruction"), match.span("target")
        # Before:
        #     je FOO
        # After:
        #     jne BAR
        return "".join([line[:a], inverted, line[b:c], target, line[d:]])

    @classmethod
    def _update_jump(cls, line: str, target: str) -> str:
        match = cls._re_jump.match(line)
        assert match
        a, b = match.span("target")
        # Before:
        #     jmp FOO
        # After:
        #     jmp BAR
        return "".join([line[:a], target, line[b:]])

    def _lookup_label(self, label: str) -> _Block:
        if label not in self._labels:
            self._labels[label] = _Block(label)
        return self._labels[label]

    def _blocks(self) -> typing.Generator[_Block, None, None]:
        block: _Block | None = self._root
        while block:
            yield block
            block = block.link

    def _body(self) -> str:
        lines = []
        hot = True
        for block in self._blocks():
            if hot != block.hot:
                hot = block.hot
                # Make it easy to tell at a glance where cold code is:
                lines.append(f"# JIT: {'HOT' if hot else 'COLD'} ".ljust(80, "#"))
            lines.extend(block.noninstructions)
            lines.extend(block.instructions)
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
        #    .balign 8
        #    _JIT_CONTINUE:
        # This lets the assembler encode _JIT_CONTINUE jumps at build time!
        align = _Block()
        align.noninstructions.append(f"\t.balign\t{self._alignment}")
        continuation = self._lookup_label(f"{self.prefix}_JIT_CONTINUE")
        assert continuation.label
        continuation.noninstructions.append(f"{continuation.label}:")
        end.link, align.link, continuation.link = align, continuation, end.link

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
                jump.instructions[-1] = self._update_jump(
                    jump.instructions[-1], branch.target.label
                )
                branch.target, jump.target = jump.target, branch.target
                jump.hot = True

    def _remove_redundant_jumps(self) -> None:
        # Zero-length jumps can be introduced by _insert_continue_label and
        # _invert_hot_branches:
        for block in self._blocks():
            # Before:
            #    jmp FOO
            #    FOO:
            # After:
            #    FOO:
            if (
                block.target
                and block.link
                and block.target.resolve() is block.link.resolve()
            ):
                block.target = None
                block.fallthrough = True
                block.instructions.pop()

    def run(self) -> None:
        """Run this optimizer."""
        self._insert_continue_label()
        self._mark_hot_blocks()
        self._invert_hot_branches()
        self._remove_redundant_jumps()
        self.path.write_text(self._body())


class OptimizerAArch64(Optimizer):  # pylint: disable = too-few-public-methods
    """aarch64-apple-darwin/aarch64-pc-windows-msvc/aarch64-unknown-linux-gnu"""

    # TODO: @diegorusso
    _alignment = 8
    # https://developer.arm.com/documentation/ddi0602/2025-03/Base-Instructions/B--Branch-
    _re_jump = re.compile(r"\s*b\s+(?P<target>[\w.]+)")


class OptimizerX86(Optimizer):  # pylint: disable = too-few-public-methods
    """i686-pc-windows-msvc/x86_64-apple-darwin/x86_64-unknown-linux-gnu"""

    _branches = _X86_BRANCHES
    _re_branch = re.compile(
        rf"\s*(?P<instruction>{'|'.join(_X86_BRANCHES)})\s+(?P<target>[\w.]+)"
    )
    # https://www.felixcloutier.com/x86/jmp
    _re_jump = re.compile(r"\s*jmp\s+(?P<target>[\w.]+)")
    # https://www.felixcloutier.com/x86/ret
    _re_return = re.compile(r"\s*ret\b")


class OptimizerX8664Windows(OptimizerX86):  # pylint: disable = too-few-public-methods
    """x86_64-pc-windows-msvc"""

    def _preprocess(self, text: str) -> str:
        text = super()._preprocess(text)
        # Before:
        #     rex64 jmpq *__imp__JIT_CONTINUE(%rip)
        # After:
        #     jmp _JIT_CONTINUE
        far_indirect_jump = (
            rf"rex64\s+jmpq\s+\*__imp_(?P<target>{self.prefix}_JIT_\w+)\(%rip\)"
        )
        return re.sub(far_indirect_jump, r"jmp\t\g<target>", text)
