import collections
import dataclasses
import pathlib
import re
import typing

_RE_NEVER_MATCH = re.compile(r"(?!)")

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
    label: str
    noise: list[str] = dataclasses.field(default_factory=list)
    instructions: list[str] = dataclasses.field(default_factory=list)
    target: typing.Self | None = None
    link: typing.Self | None = None
    fallthrough: bool = True
    hot: bool = False

    def __eq__(self, other: object) -> bool:
        return self is other

    def __hash__(self) -> int:
        return super().__hash__()

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
        r"\s*(?P<label>[\w\.]+):"
    )  # One group: label.
    _re_noise: typing.ClassVar[re.Pattern[str]] = re.compile(
        r"\s*(?:[#\.]|$)"
    )  # No groups.
    _re_return: typing.ClassVar[re.Pattern[str]] = _RE_NEVER_MATCH  # No groups.

    def __post_init__(self) -> None:
        text = self._preprocess(self.path.read_text())
        self._graph = block = self._new_block()
        for line in text.splitlines():
            if label := self._parse_label(line):
                block.link = block = self._lookup_label(label)
                block.noise.append(line)
                continue
            elif block.target or not block.fallthrough:
                block.link = block = self._new_block()
            if self._parse_noise(line):
                if block.instructions:
                    block.link = block = self._new_block()
                block.noise.append(line)
                continue
            block.instructions.append(line)
            if target := self._parse_branch(line):
                block.target = self._lookup_label(target)
                assert block.fallthrough
            elif target := self._parse_jump(line):
                block.target = self._lookup_label(target)
                block.fallthrough = False
            elif self._parse_return(line):
                assert not block.target
                block.fallthrough = False

    @classmethod
    def _parse_branch(cls, line: str) -> str | None:
        branch = cls._re_branch.match(line)
        return branch and branch["target"]

    @classmethod
    def _parse_jump(cls, line: str) -> str | None:
        jump = cls._re_jump.match(line)
        return jump and jump["target"]

    @classmethod
    def _parse_label(cls, line: str) -> str | None:
        label = cls._re_label.match(line)
        return label and label["label"]

    @classmethod
    def _parse_noise(cls, line: str) -> bool:
        return cls._re_noise.match(line) is not None

    @classmethod
    def _parse_return(cls, line: str) -> bool:
        return cls._re_return.match(line) is not None

    @classmethod
    def _invert_branch(cls, line: str, target: str) -> str | None:
        branch = cls._re_branch.match(line)
        assert branch
        inverted = cls._branches.get(branch["instruction"])
        if not inverted:
            return None
        (a, b), (c, d) = branch.span("instruction"), branch.span("target")
        return "".join([line[:a], inverted, line[b:c], target, line[d:]])

    @classmethod
    def _thread_jump(cls, line: str, target: str) -> str:
        jump = cls._re_jump.match(line) or cls._re_branch.match(line)
        assert jump
        a, b = jump.span("target")
        return "".join([line[:a], target, line[b:]])

    @staticmethod
    def _create_jump(target: str) -> str | None:
        return None

    def _lookup_label(self, label: str) -> _Block:
        if label not in self._labels:
            self._labels[label] = _Block(label)
        return self._labels[label]

    def _new_block(self) -> _Block:
        label = f"{self.prefix}_JIT_LABEL_{len(self._labels)}"
        block = self._lookup_label(label)
        block.noise.append(f"{label}:")
        return block

    @staticmethod
    def _preprocess(text: str) -> str:
        return text

    def _blocks(self) -> typing.Generator[_Block, None, None]:
        block = self._graph
        while block:
            yield block
            block = block.link

    def _lines(self) -> typing.Generator[str, None, None]:
        for block in self._blocks():
            yield from block.noise
            yield from block.instructions

    def _insert_continue_label(self) -> None:
        for end in reversed(list(self._blocks())):
            if end.instructions:
                break
        align = self._new_block()
        align.noise.append(f"\t.balign\t{self._alignment}")
        continuation = self._lookup_label(f"{self.prefix}_JIT_CONTINUE")
        continuation.noise.append(f"{continuation.label}:")
        end.link, align.link, continuation.link = align, continuation, end.link

    def _mark_hot_blocks(self) -> None:
        predecessors = collections.defaultdict(set)
        for block in self._blocks():
            if block.target:
                predecessors[block.target].add(block)
            if block.fallthrough and block.link:
                predecessors[block.link].add(block)
        todo = [self._lookup_label(f"{self.prefix}_JIT_CONTINUE")]
        while todo:
            block = todo.pop()
            block.hot = True
            todo.extend(
                predecessor
                for predecessor in predecessors[block]
                if not predecessor.hot
            )

    def _invert_hot_branches(self) -> None:
        for block in self._blocks():
            if (
                block.fallthrough
                and block.target
                and block.target.hot
                and block.link
                and not block.link.hot
            ):
                label_block = self._new_block()
                branch = block.instructions[-1]
                inverted = self._invert_branch(branch, label_block.label)
                jump = self._create_jump(block.target.label)
                if inverted is None or jump is None:
                    continue
                jump_block = self._new_block()
                jump_block.instructions.append(jump)
                jump_block.target = block.target
                jump_block.fallthrough = False
                block.instructions[-1] = inverted
                block.target = label_block
                block.link, jump_block.link, label_block.link = (
                    jump_block,
                    label_block,
                    block.link,
                )

    def _thread_jumps(self) -> None:
        for block in self._blocks():
            while block.target:
                target = block.target.resolve()
                if (
                    not target.fallthrough
                    and target.target
                    and len(target.instructions) == 1
                ):
                    jump = block.instructions[-1]
                    block.instructions[-1] = self._thread_jump(
                        jump, target.target.label
                    )
                    block.target = target.target
                else:
                    break

    def _remove_dead_code(self) -> None:
        reachable = set()
        todo = [self._graph]
        while todo:
            block = todo.pop()
            reachable.add(block)
            if block.target and block.target not in reachable:
                todo.append(block.target)
            if block.fallthrough and block.link and block.link not in reachable:
                todo.append(block.link)
        for block in self._blocks():
            if block not in reachable:
                block.instructions.clear()

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

    def _remove_unused_labels(self) -> None:
        used = set()
        for block in self._blocks():
            if block.target:
                used.add(block.target)
        for block in self._blocks():
            if block not in used and block.label.startswith(
                f"{self.prefix}_JIT_LABEL_"
            ):
                del block.noise[0]

    def run(self) -> None:
        self._insert_continue_label()
        self._mark_hot_blocks()
        self._invert_hot_branches()
        self._thread_jumps()
        self._remove_dead_code()
        self._remove_redundant_jumps()
        self._remove_unused_labels()
        self.path.write_text("\n".join(self._lines()))


class OptimizerAArch64(Optimizer):
    # TODO: @diegorusso
    _alignment = 8


class OptimizerX86(Optimizer):

    _branches = _X86_BRANCHES
    _re_branch = re.compile(
        rf"\s*(?P<instruction>{'|'.join(_X86_BRANCHES)})\s+(?P<target>[\w\.]+)"
    )
    _re_jump = re.compile(r"\s*jmp\s+(?P<target>[\w\.]+)")
    _re_return = re.compile(r"\s*ret\b")

    @staticmethod
    def _create_jump(target: str) -> str:
        return f"\tjmp\t{target}"


class OptimizerX86Windows(OptimizerX86):

    def _preprocess(self, text: str) -> str:
        text = super()._preprocess(text)
        far_indirect_jump = (
            rf"rex64\s+jmpq\s+\*__imp_(?P<target>{self.prefix}_JIT_\w+)\(%rip\)"
        )
        return re.sub(far_indirect_jump, r"jmp\t\g<target>", text)
