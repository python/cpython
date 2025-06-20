import dataclasses
import pathlib
import re
import typing

_RE_NEVER_MATCH = re.compile(r"(?!)")  # Same as saying "not string.startswith('')".

_X86_BRANCHES = {
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
    "jrxz": None,
    "js": "jns",
    "jz": "jnz",
    "loop": None,
    "loope": None,
    "loopne": None,
    "loopnz": None,
    "loopz": None,
}
_X86_BRANCHES |= {v: k for k, v in _X86_BRANCHES.items() if v}


@dataclasses.dataclass
class _Block:
    label: str | None = None
    noise: list[str] = dataclasses.field(default_factory=list)
    instructions: list[str] = dataclasses.field(default_factory=list)
    target: typing.Self | None = None
    link: typing.Self | None = None
    fallthrough: bool = True
    hot: bool = False

    def resolve(self) -> typing.Self:
        while self.link and not self.instructions:
            self = self.link
        return self


@dataclasses.dataclass
class Optimizer:

    path: pathlib.Path
    _: dataclasses.KW_ONLY
    prefix: str = ""
    _graph: _Block = dataclasses.field(init=False)
    _labels: dict = dataclasses.field(init=False, default_factory=dict)
    _alignment: typing.ClassVar[int] = 1
    _branches: typing.ClassVar[dict[str, str | None]] = {}
    _re_branch: typing.ClassVar[re.Pattern[str]] = (
        _RE_NEVER_MATCH  # Two groups: instruction and target.
    )
    _re_jump: typing.ClassVar[re.Pattern[str]] = _RE_NEVER_MATCH  # One group: target.
    _re_label: typing.ClassVar[re.Pattern[str]] = re.compile(
        r'\s*(?P<label>[\w."$?@]+):'  # One group: label.
    )
    _re_noise: typing.ClassVar[re.Pattern[str]] = re.compile(
        r"\s*(?:[#.]|$)"  # No groups.
    )
    _re_return: typing.ClassVar[re.Pattern[str]] = _RE_NEVER_MATCH  # No groups.

    def __post_init__(self) -> None:
        text = self._preprocess(self.path.read_text())
        self._graph = block = _Block()
        for line in text.splitlines():
            if match := self._re_label.match(line):
                block.link = block = self._lookup_label(match["label"])
                block.noise.append(line)
                continue
            if self._re_noise.match(line):
                if block.instructions:
                    block.link = block = _Block()
                block.noise.append(line)
                continue
            if block.target or not block.fallthrough:
                block.link = block = _Block()
            block.instructions.append(line)
            if match := self._re_branch.match(line):
                block.target = self._lookup_label(match["target"])
                assert block.fallthrough
            elif match := self._re_jump.match(line):
                block.target = self._lookup_label(match["target"])
                block.fallthrough = False
            elif self._re_return.match(line):
                assert not block.target
                block.fallthrough = False

    @staticmethod
    def _preprocess(text: str) -> str:
        return text

    @classmethod
    def _invert_branch(cls, line: str, target: str) -> str | None:
        match = cls._re_branch.match(line)
        assert match
        inverted = cls._branches.get(match["instruction"])
        if not inverted:
            return None
        (a, b), (c, d) = match.span("instruction"), match.span("target")
        return "".join([line[:a], inverted, line[b:c], target, line[d:]])

    @classmethod
    def _update_jump(cls, line: str, target: str) -> str:
        match = cls._re_jump.match(line)
        assert match
        a, b = match.span("target")
        return "".join([line[:a], target, line[b:]])

    def _lookup_label(self, label: str) -> _Block:
        if label not in self._labels:
            self._labels[label] = _Block(label)
        return self._labels[label]

    def _blocks(self) -> typing.Generator[_Block, None, None]:
        block = self._graph
        while block:
            yield block
            block = block.link

    def _body(self) -> str:
        lines = []
        for block in self._blocks():
            lines.extend(block.noise)
            lines.extend(block.instructions)
        return "\n".join(lines)

    def _predecessors(self, block: _Block) -> typing.Generator[_Block, None, None]:
        for block in self._blocks():
            if block.target is block or (block.fallthrough and block.link is block):
                yield block

    def _insert_continue_label(self) -> None:
        for end in reversed(list(self._blocks())):
            if end.instructions:
                break
        align = _Block()
        align.noise.append(f"\t.balign\t{self._alignment}")
        continuation = self._lookup_label(f"{self.prefix}_JIT_CONTINUE")
        assert continuation.label
        continuation.noise.append(f"{continuation.label}:")
        end.link, align.link, continuation.link = align, continuation, end.link

    def _mark_hot_blocks(self) -> None:
        todo = [self._lookup_label(f"{self.prefix}_JIT_CONTINUE")]
        while todo:
            block = todo.pop()
            block.hot = True
            todo.extend(
                predecessor
                for predecessor in self._predecessors(block)
                if not predecessor.hot
            )

    def _invert_hot_branches(self) -> None:
        # Turn:
        #    branch <hot>
        #    jump <cold>
        # Into:
        #    opposite-branch <cold>
        #    jump <hot>
        for branch in self._blocks():
            link = branch.link
            if link is None:
                continue
            jump = link.resolve()
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
                if inverted is None:
                    continue
                branch.instructions[-1] = inverted
                jump.instructions[-1] = self._update_jump(
                    jump.instructions[-1], branch.target.label
                )
                branch.target, jump.target = jump.target, branch.target
                jump.hot = True

    def _remove_redundant_jumps(self) -> None:
        for block in self._blocks():
            if (
                block.target
                and block.link
                and block.target.resolve() is block.link.resolve()
            ):
                block.target = None
                block.fallthrough = True
                block.instructions.pop()

    def run(self) -> None:
        self._insert_continue_label()
        self._mark_hot_blocks()
        self._invert_hot_branches()
        self._remove_redundant_jumps()
        self.path.write_text(self._body())


class OptimizerAArch64(Optimizer):
    # TODO: @diegorusso
    _alignment = 8


class OptimizerX86(Optimizer):

    _branches = _X86_BRANCHES
    _re_branch = re.compile(
        rf"\s*(?P<instruction>{'|'.join(_X86_BRANCHES)})\s+(?P<target>[\w.]+)"
    )
    _re_jump = re.compile(r"\s*jmp\s+(?P<target>[\w.]+)")
    _re_return = re.compile(r"\s*ret\b")


class OptimizerX86Windows(OptimizerX86):

    def _preprocess(self, text: str) -> str:
        text = super()._preprocess(text)
        # rex64 jumpq *__imp__JIT_CONTINUE(%rip) -> jmp _JIT_CONTINUE
        far_indirect_jump = (
            rf"rex64\s+jmpq\s+\*__imp_(?P<target>{self.prefix}_JIT_\w+)\(%rip\)"
        )
        return re.sub(far_indirect_jump, r"jmp\t\g<target>", text)
