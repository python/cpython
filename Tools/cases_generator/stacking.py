from abc import abstractmethod
import dataclasses
import typing

from formatting import (
    Formatter,
    UNUSED,
    maybe_parenthesize,
    parenthesize_cond,
)
from instructions import (
    ActiveCacheEffect,
    Instruction,
    MacroInstruction,
    Component,
    Tiers,
    TIER_ONE,
)
from parsing import StackEffect, CacheEffect, Family


@dataclasses.dataclass
class StackOffset:
    """Represent the stack offset for a PEEK or POKE.

    - If at the top of the stack (PEEK(-1) or POKE(-1)),
      both deep and high are empty.
    - If some number of items below stack top, only deep is non-empty.
    - If some number of items above stack top, only high is non-empty.
    - If accessing the entry would require some popping and then some
      pushing, both deep and high are non-empty.

    All this would be much simpler if all stack entries were the same
    size, but with conditional and array effects, they aren't.
    The offsets are each represented by a list of StackEffect objects.
    The name in the StackEffects is unused.

    TODO: Maybe this should *include* the size of the accessed entry.
    """

    deep: list[StackEffect] = dataclasses.field(default_factory=list)
    high: list[StackEffect] = dataclasses.field(default_factory=list)

    def clone(self) -> "StackOffset":
        return StackOffset(list(self.deep), list(self.high))

    def deeper(self, eff: StackEffect) -> None:
        if eff in self.high:
            self.high.remove(eff)
        else:
            self.deep.append(eff)

    def higher(self, eff: StackEffect) -> None:
        if eff in self.deep:
            self.deep.remove(eff)
        else:
            self.high.append(eff)

    @abstractmethod
    def itself(self) -> StackEffect:
        """Implemented by subclass StackItem."""
        raise NotImplementedError("itself() not implemented")

    def as_terms(self) -> list[tuple[str, str]]:
        num = 0
        terms: list[tuple[str, str]] = []
        for eff in self.deep:
            if eff.size:
                terms.append(("-", maybe_parenthesize(eff.size)))
            elif eff.cond and eff.cond != "1":
                terms.append(("-", f"({parenthesize_cond(eff.cond)} ? 1 : 0)"))
            elif eff.cond != "0":
                num -= 1
        for eff in self.high:
            if eff.size:
                terms.append(("+", maybe_parenthesize(eff.size)))
            elif eff.cond and eff.cond != "1":
                terms.append(("+", f"({parenthesize_cond(eff.cond)} ? 1 : 0)"))
            elif eff.cond != "0":
                num += 1
        if num < 0:
            terms.insert(0, ("-", str(-num)))
        elif num > 0:
            terms.append(("+", str(num)))
        return terms

    # TODO: Turn into helper function (not method)
    def make_index(self, terms: list[tuple[str, str]]) -> str:
        # Produce an index expression from the terms honoring PEP 8,
        # surrounding binary ops with spaces but not unary minus
        index = ""
        for sign, term in terms:
            if index:
                index += f" {sign} {term}"
            elif sign == "+":
                index = term
            else:
                index = sign + term
        return index or "0"

    def as_index(self) -> str:
        terms = self.as_terms()
        return self.make_index(terms)

    def as_variable(self) -> str:
        """Return e.g. stack_pointer[-1]."""
        temp = self.clone()
        me = self.itself()
        temp.deeper(me)
        terms = temp.as_terms()
        if me.size:
            terms.insert(0, ("+", "stack_pointer"))
        index = self.make_index(terms)
        if me.size:
            return index
        else:
            return f"stack_pointer[{index}]"


@dataclasses.dataclass
class StackItem(StackOffset):
    effect: StackEffect = dataclasses.field(kw_only=True)

    def clone(self):
        return StackItem(list(self.deep), list(self.high), effect=self.effect)

    def itself(self) -> StackEffect:
        return self.effect


@dataclasses.dataclass
class CopyEffect:
    src: StackEffect
    dst: StackEffect


class EffectManager:
    """Manage stack effects and offsets for an instruction."""

    instr: Instruction
    active_caches: list[ActiveCacheEffect]
    peeks: list[StackItem]
    pokes: list[StackItem]
    copies: list[CopyEffect]  # See merge()
    # Track offsets from stack pointer
    min_offset: StackOffset
    final_offset: StackOffset

    def __init__(self, instr: Instruction, active_caches: list[ActiveCacheEffect]):
        self.instr = instr
        self.active_caches = active_caches
        self.peeks = []
        self.pokes = []
        self.copies = []
        self.min_offset = StackOffset()
        for eff in reversed(instr.input_effects):
            self.min_offset.deeper(eff)
            new_peek = StackItem(effect=eff)
            for peek in self.peeks:
                new_peek.deeper(peek.effect)
            self.peeks.append(new_peek)
        self.final_offset = self.min_offset.clone()
        for eff in instr.output_effects:
            self.final_offset.higher(eff)
            new_poke = StackItem(effect=eff)
            for peek in self.peeks:
                new_poke.deeper(peek.effect)
            for poke in self.pokes:
                new_poke.higher(poke.effect)
            new_poke.higher(eff)  # Account for the new poke itself (!)
            self.pokes.append(new_poke)

    def adjust_deeper(self, eff: StackEffect) -> None:
        for peek in self.peeks:
            peek.deeper(eff)
        for poke in self.pokes:
            poke.deeper(eff)
        self.min_offset.deeper(eff)
        self.final_offset.deeper(eff)

    def adjust_higher(self, eff: StackEffect) -> None:
        for peek in self.peeks:
            peek.higher(eff)
        for poke in self.pokes:
            poke.higher(eff)
        self.min_offset.higher(eff)
        self.final_offset.higher(eff)

    def adjust(self, offset: StackOffset) -> None:
        for down in offset.deep:
            self.adjust_deeper(down)
        for up in offset.high:
            self.adjust_higher(up)

    def adjust_inverse(self, offset: StackOffset) -> None:
        for down in offset.deep:
            self.adjust_higher(down)
        for up in offset.high:
            self.adjust_deeper(up)

    def merge(self, follow: "EffectManager") -> None:
        sources: set[str] = set()
        destinations: set[str] = set()
        follow.adjust(self.final_offset)
        while (
            self.pokes
            and follow.peeks
            and self.pokes[-1].effect == follow.peeks[0].effect
        ):
            src = self.pokes.pop(-1).effect
            dst = follow.peeks.pop(0).effect
            self.final_offset.deeper(src)
            if dst.name != UNUSED:
                destinations.add(dst.name)
                if dst.name != src.name:
                    sources.add(src.name)
                    # TODO: Do we leave redundant copies or skip them?
                    follow.copies.append(CopyEffect(src, dst))
        # TODO: Turn this into an error (pass an Analyzer instance?)
        assert sources & destinations == set(), (
            self.instr.name,
            follow.instr.name,
            sources,
            destinations,
        )

    def collect_vars(self) -> dict[str, StackEffect]:
        """Collect all variables, skipping unused ones."""
        vars: dict[str, StackEffect] = {}

        def add(eff: StackEffect) -> None:
            if eff.name != UNUSED:
                if eff.name in vars:
                    # TODO: Make this an error
                    assert vars[eff.name] == eff, (
                        self.instr.name,
                        eff.name,
                        vars[eff.name],
                        eff,
                    )
                else:
                    vars[eff.name] = eff

        for copy in self.copies:
            add(copy.dst)
        for peek in self.peeks:
            add(peek.effect)
        for poke in self.pokes:
            add(poke.effect)

        return vars


def less_than(a: StackOffset, b: StackOffset) -> bool:
    # TODO: Handle more cases
    if a.high != b.high:
        return False
    return a.deep[: len(b.deep)] == b.deep


def get_stack_effect_info_for_macro(mac: MacroInstruction) -> tuple[str, str]:
    """Get the stack effect info for a macro instruction.

    Returns a tuple (popped, pushed) where each is a string giving a
    symbolic expression for the number of values popped/pushed.
    """
    parts = [part for part in mac.parts if isinstance(part, Component)]
    managers = [EffectManager(part.instr, part.active_caches) for part in parts]
    for prev, follow in zip(managers[:-1], managers[1:], strict=True):
        prev.merge(follow)
    net = StackOffset()
    lowest = StackOffset()
    for mgr in managers:
        for peek in mgr.peeks:
            net.deeper(peek.effect)
        if less_than(net, lowest):
            lowest = StackOffset(net.deep[:], net.high[:])
        else:
            # TODO: Turn this into an error -- the "min" is ambiguous
            assert net == lowest, (mac.name, net, lowest)
        for poke in mgr.pokes:
            net.higher(poke.effect)
    popped = StackOffset(lowest.high[:], lowest.deep[:])  # Reverse direction!
    for eff in popped.deep:
        net.deeper(eff)
    for eff in popped.high:
        net.higher(eff)
    return popped.as_index(), net.as_index()


def write_single_instr(
    instr: Instruction, out: Formatter, tier: Tiers = TIER_ONE
) -> None:
    write_components(
        [Component(instr, instr.active_caches)],
        out,
        tier,
    )


def write_macro_instr(
    mac: MacroInstruction, out: Formatter, family: Family | None
) -> None:
    parts = [part for part in mac.parts if isinstance(part, Component)]
    cache_adjust = 0
    for part in mac.parts:
        match part:
            case CacheEffect(size=size):
                cache_adjust += size
            case Component(instr=instr):
                cache_adjust += instr.cache_offset
            case _:
                typing.assert_never(part)

    out.emit("")
    with out.block(f"TARGET({mac.name})"):
        if mac.predicted:
            out.emit(f"PREDICTED({mac.name});")
        out.static_assert_family_size(mac.name, family, cache_adjust)
        write_components(parts, out, TIER_ONE)
        if cache_adjust:
            out.emit(f"next_instr += {cache_adjust};")
        out.emit("DISPATCH();")


def write_components(
    parts: list[Component],
    out: Formatter,
    tier: Tiers,
) -> None:
    managers = [EffectManager(part.instr, part.active_caches) for part in parts]

    all_vars: dict[str, StackEffect] = {}
    for mgr in managers:
        for name, eff in mgr.collect_vars().items():
            if name in all_vars:
                # TODO: Turn this into an error -- variable conflict
                assert all_vars[name] == eff, (
                    name,
                    mgr.instr.name,
                    all_vars[name],
                    eff,
                )
            else:
                all_vars[name] = eff

    # Do this after collecting variables, so we collect suppressed copies
    # TODO: Maybe suppressed copies should be kept?
    for prev, follow in zip(managers[:-1], managers[1:], strict=True):
        prev.merge(follow)

    # Declare all variables
    for name, eff in all_vars.items():
        out.declare(eff, None)

    for mgr in managers:
        if len(parts) > 1:
            out.emit(f"// {mgr.instr.name}")

        for copy in mgr.copies:
            out.assign(copy.dst, copy.src)
        for peek in mgr.peeks:
            out.assign(
                peek.effect,
                StackEffect(
                    peek.as_variable(),
                    peek.effect.type,
                    peek.effect.cond,
                    peek.effect.size,
                ),
            )
        # Initialize array outputs
        for poke in mgr.pokes:
            if poke.effect.size and poke.effect.name not in mgr.instr.unmoved_names:
                out.assign(
                    poke.effect,
                    StackEffect(
                        poke.as_variable(),
                        poke.effect.type,
                        poke.effect.cond,
                        poke.effect.size,
                    ),
                )

        if len(parts) == 1:
            mgr.instr.write_body(out, 0, mgr.active_caches, tier)
        else:
            with out.block(""):
                mgr.instr.write_body(out, -4, mgr.active_caches, tier)

        if mgr is managers[-1]:
            out.stack_adjust(mgr.final_offset.deep, mgr.final_offset.high)
            # Use clone() since adjust_inverse() mutates final_offset
            mgr.adjust_inverse(mgr.final_offset.clone())

        for poke in mgr.pokes:
            if not poke.effect.size and poke.effect.name not in mgr.instr.unmoved_names:
                out.assign(
                    StackEffect(
                        poke.as_variable(),
                        poke.effect.type,
                        poke.effect.cond,
                        poke.effect.size,
                    ),
                    poke.effect,
                )
