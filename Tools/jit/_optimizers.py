import collections
import dataclasses
import pathlib
import re
import typing

branches = {}
for op, nop in [
    ("ja", "jna"),
    ("jae", "jnae"),
    ("jb", "jnb"),
    ("jbe", "jnbe"),
    ("jc", "jnc"),
    ("jcxz", None),
    ("je", "jne"),
    ("jecxz", None),
    ("jg", "jng"),
    ("jge", "jnge"),
    ("jl", "jnl"),
    ("jle", "jnle"),
    ("jo", "jno"),
    ("jp", "jnp"),
    ("jpe", "jpo"),
    ("jrxz", None),
    ("js", "jns"),
    ("jz", "jnz"),
    ("loop", None),
    ("loope", None),
    ("loopne", None),
    ("loopnz", None),
    ("loopz", None),
]:
    branches[op] = nop
    if nop:
        branches[nop] = op


def _get_branch(line: str) -> str | None:
    branch = re.match(rf"\s*({'|'.join(branches)})\s+([\w\.]+)", line)
    return branch and branch[2]


def _invert_branch(line: str, label: str) -> str | None:
    branch = re.match(rf"\s*({'|'.join(branches)})\s+([\w\.]+)", line)
    assert branch
    inverted = branches.get(branch[1])
    if not inverted:
        return None
    line = line.replace(branch[1], inverted, 1)  # XXX
    line = line.replace(branch[2], label, 1)  # XXX
    return line


def _get_jump(line: str) -> str | None:
    jump = re.match(r"\s*(?:rex64\s+)?jmpq?\s+\*?([\w\.]+)", line)
    return jump and jump[1]


def _get_label(line: str) -> str | None:
    label = re.match(r"\s*([\w\.]+):", line)
    return label and label[1]


def _is_return(line: str) -> bool:
    return re.match(r"\s*ret\s+", line) is not None


def _is_noise(line: str) -> bool:
    return re.match(r"\s*([#\.]|$)", line) is not None


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


class _Labels(dict):
    def __missing__(self, key: str) -> _Block:
        self[key] = _Block(key)
        return self[key]


@dataclasses.dataclass
class Optimizer:

    path: pathlib.Path
    _: dataclasses.KW_ONLY
    prefix: str = ""
    _graph: _Block = dataclasses.field(init=False)
    _labels: _Labels = dataclasses.field(init=False, default_factory=_Labels)

    _re_branch: typing.ClassVar[re.Pattern[str]]  # Two groups: instruction and target.
    _re_jump: typing.ClassVar[re.Pattern[str]]  # One group: target.
    _re_return: typing.ClassVar[re.Pattern[str]]  # No groups.

    def __post_init__(self) -> None:
        text = self._preprocess(self.path.read_text())
        self._graph = block = self._new_block()
        for line in text.splitlines():
            if label := _get_label(line):
                block.link = block = self._labels[label]
            elif block.target or not block.fallthrough:
                block.link = block = self._new_block()
            if _is_noise(line) or _get_label(line):
                if block.instructions:
                    block.link = block = self._new_block()
                block.noise.append(line)
                continue
            block.instructions.append(line)
            if target := _get_branch(line):
                block.target = self._labels[target]
                assert block.fallthrough
            elif target := _get_jump(line):
                block.target = self._labels[target]
                block.fallthrough = False
            elif _is_return(line):
                assert not block.target
                block.fallthrough = False

    def _new_block(self, label: str | None = None) -> _Block:
        if not label:
            label = f"{self.prefix}_JIT_LABEL_{len(self._labels)}"
        assert label not in self._labels, label
        block = self._labels[label] = _Block(label, [f"{label}:"])
        return block

    def _preprocess(self, text: str) -> str:
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
        continuation = self._labels[f"{self.prefix}_JIT_CONTINUE"]
        continuation.noise.append(f"{continuation.label}:")
        end.link, continuation.link = continuation, end.link

    def _mark_hot_blocks(self) -> None:
        predecessors = collections.defaultdict(set)
        for block in self._blocks():
            if block.target:
                predecessors[block.target].add(block)
            if block.fallthrough and block.link:
                predecessors[block.link].add(block)
        todo = [self._labels[f"{self.prefix}_JIT_CONTINUE"]]
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
                and block.link
                and block.target.hot
                and not block.link.hot
            ):
                # Turn...
                #         branch hot
                # ...into..
                #         opposite-branch ._JIT_LABEL_N
                #         jmp hot
                #     ._JIT_LABEL_N:
                label_block = self._new_block()
                inverted = _invert_branch(block.instructions[-1], label_block.label)
                if inverted is None:
                    continue
                jump_block = self._new_block()
                jump_block.instructions.append(f"\tjmp\t{block.target.label}")
                jump_block.target = block.target
                jump_block.fallthrough = False
                block.instructions[-1] = inverted
                block.target = label_block
                label_block.link = block.link
                jump_block.link = label_block
                block.link = jump_block

    def _thread_jumps(self) -> None:
        for block in self._blocks():
            while block.target:
                label = block.target.label
                target = block.target.resolve()
                if (
                    not target.fallthrough
                    and target.target
                    and len(target.instructions) == 1
                ):
                    block.instructions[-1] = block.instructions[-1].replace(
                        label, target.target.label
                    )  # XXX
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


class OptimizerX86(Optimizer):

    _re_branch = re.compile(
        rf"\s*(?P<instruction>{'|'.join(branches)})\s+(?P<target>[\w\.]+)"
    )
    _re_jump = re.compile(r"\s*jmp\s+(?P<target>[\w\.]+)")
    _re_return = re.compile(r"\s*ret\b")


class OptimizerX86Windows(OptimizerX86):

    def _preprocess(self, text: str) -> str:
        text = super()._preprocess(text)
        far_indirect_jump = (
            rf"rex64\s+jmpq\s+\*__imp_(?P<target>{self.prefix}_JIT_\w+)\(%rip\)"
        )
        return re.sub(far_indirect_jump, r"jmp\t\g<target>", text)
