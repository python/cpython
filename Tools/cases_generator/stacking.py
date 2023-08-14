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

    - At stack_pointer[0], deep and high are both empty.
      (Note that that is an invalid stack reference.)
    - Below stack top, only deep is non-empty.
    - Above stack top, only high is non-empty.
    - In complex cases, both deep and high may be non-empty.

    All this would be much simpler if all stack entries were the same
    size, but with conditional and array effects, they aren't.
    The offsets are each represented by a list of StackEffect objects.
    The name in the StackEffects is unused.
    """

    deep: list[StackEffect] = dataclasses.field(default_factory=list)
    high: list[StackEffect] = dataclasses.field(default_factory=list)

    def clone(self) -> "StackOffset":
        return StackOffset(list(self.deep), list(self.high))

    def negate(self) -> "StackOffset":
        return StackOffset(list(self.high), list(self.deep))

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

    def as_terms(self) -> list[tuple[str, str]]:
        num = 0
        terms: list[tuple[str, str]] = []
        for eff in self.deep:
            if eff.size:
                terms.append(("-", maybe_parenthesize(eff.size)))
            elif eff.cond and eff.cond not in ("0", "1"):
                terms.append(("-", f"({parenthesize_cond(eff.cond)} ? 1 : 0)"))
            elif eff.cond != "0":
                num -= 1
        for eff in self.high:
            if eff.size:
                terms.append(("+", maybe_parenthesize(eff.size)))
            elif eff.cond and eff.cond not in ("0", "1"):
                terms.append(("+", f"({parenthesize_cond(eff.cond)} ? 1 : 0)"))
            elif eff.cond != "0":
                num += 1
        if num < 0:
            terms.insert(0, ("-", str(-num)))
        elif num > 0:
            terms.append(("+", str(num)))
        return terms

    def as_index(self) -> str:
        terms = self.as_terms()
        return make_index(terms)


def make_index(terms: list[tuple[str, str]]) -> str:
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


@dataclasses.dataclass
class StackItem:
    offset: StackOffset
    effect: StackEffect

    def as_variable(self, lax: bool = False) -> str:
        """Return e.g. stack_pointer[-1]."""
        terms = self.offset.as_terms()
        if self.effect.size:
            terms.insert(0, ("+", "stack_pointer"))
        index = make_index(terms)
        if self.effect.size:
            res = index
        else:
            res = f"stack_pointer[{index}]"
        if not lax:
            # Check that we're not reading or writing above stack top.
            # Skip this for output variable initialization (lax=True).
            assert (
                self.effect in self.offset.deep and not self.offset.high
            ), f"Push or pop above current stack level: {res}"
        return res

    def as_stack_effect(self, lax: bool = False) -> StackEffect:
        return StackEffect(
            self.as_variable(lax=lax),
            self.effect.type if self.effect.size else "",
            self.effect.cond,
            self.effect.size,
        )


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

    def __init__(
        self,
        instr: Instruction,
        active_caches: list[ActiveCacheEffect],
        pred: "EffectManager | None" = None,
    ):
        self.instr = instr
        self.active_caches = active_caches
        self.peeks = []
        self.pokes = []
        self.copies = []
        self.final_offset = pred.final_offset.clone() if pred else StackOffset()
        for eff in reversed(instr.input_effects):
            self.final_offset.deeper(eff)
            self.peeks.append(StackItem(offset=self.final_offset.clone(), effect=eff))
        self.min_offset = self.final_offset.clone()
        for eff in instr.output_effects:
            self.pokes.append(StackItem(offset=self.final_offset.clone(), effect=eff))
            self.final_offset.higher(eff)

        if pred:
            # Replace push(x) + pop(y) with copy(x, y).
            # Check that the sources and destinations are disjoint.
            sources: set[str] = set()
            destinations: set[str] = set()
            while (
                pred.pokes
                and self.peeks
                and pred.pokes[-1].effect == self.peeks[-1].effect
            ):
                src = pred.pokes.pop(-1).effect
                dst = self.peeks.pop(0).effect
                pred.final_offset.deeper(src)
                if dst.name != UNUSED:
                    destinations.add(dst.name)
                    if dst.name != src.name:
                        sources.add(src.name)
                    self.copies.append(CopyEffect(src, dst))
            # TODO: Turn this into an error (pass an Analyzer instance?)
            assert sources & destinations == set(), (
                pred.instr.name,
                self.instr.name,
                sources,
                destinations,
            )

    def adjust_deeper(self, eff: StackEffect) -> None:
        for peek in self.peeks:
            peek.offset.deeper(eff)
        for poke in self.pokes:
            poke.offset.deeper(eff)
        self.min_offset.deeper(eff)
        self.final_offset.deeper(eff)

    def adjust_higher(self, eff: StackEffect) -> None:
        for peek in self.peeks:
            peek.offset.higher(eff)
        for poke in self.pokes:
            poke.offset.higher(eff)
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
            add(copy.src)
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


def get_managers(parts: list[Component]) -> list[EffectManager]:
    managers: list[EffectManager] = []
    pred: EffectManager | None = None
    for part in parts:
        mgr = EffectManager(part.instr, part.active_caches, pred)
        managers.append(mgr)
        pred = mgr
    return managers


def get_stack_effect_info_for_macro(mac: MacroInstruction) -> tuple[str, str]:
    """Get the stack effect info for a macro instruction.

    Returns a tuple (popped, pushed) where each is a string giving a
    symbolic expression for the number of values popped/pushed.
    """
    parts = [part for part in mac.parts if isinstance(part, Component)]
    managers = get_managers(parts)
    popped = StackOffset()
    for mgr in managers:
        if less_than(mgr.min_offset, popped):
            popped = mgr.min_offset.clone()
    # Compute pushed = final - popped
    pushed = managers[-1].final_offset.clone()
    for effect in popped.deep:
        pushed.higher(effect)
    for effect in popped.high:
        pushed.deeper(effect)
    return popped.negate().as_index(), pushed.as_index()


def write_single_instr(
    instr: Instruction, out: Formatter, tier: Tiers = TIER_ONE
) -> None:
    try:
        write_components(
            [Component(instr, instr.active_caches)],
            out,
            tier,
        )
    except AssertionError as err:
        raise AssertionError(f"Error writing instruction {instr.name}") from err


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
        try:
            write_components(parts, out, TIER_ONE)
        except AssertionError as err:
            raise AssertionError(f"Error writing macro {mac.name}") from err
        if cache_adjust:
            out.emit(f"next_instr += {cache_adjust};")
        out.emit("DISPATCH();")


def write_components(
    parts: list[Component],
    out: Formatter,
    tier: Tiers,
) -> None:
    managers = get_managers(parts)

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

    # Declare all variables
    for name, eff in all_vars.items():
        out.declare(eff, None)

    for mgr in managers:
        if len(parts) > 1:
            out.emit(f"// {mgr.instr.name}")

        for copy in mgr.copies:
            if copy.src.name != copy.dst.name:
                out.assign(copy.dst, copy.src)
        for peek in mgr.peeks:
            out.assign(
                peek.effect,
                peek.as_stack_effect(),
            )
        # Initialize array outputs
        for poke in mgr.pokes:
            if poke.effect.size and poke.effect.name not in mgr.instr.unmoved_names:
                out.assign(
                    poke.effect,
                    poke.as_stack_effect(lax=True),
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
                    poke.as_stack_effect(),
                    poke.effect,
                )
