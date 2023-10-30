import dataclasses
import typing

from flags import variable_used_unspecialized
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

    def equivalent_to(self, other: "StackOffset") -> bool:
        if self.deep == other.deep and self.high == other.high:
            return True
        deep = list(self.deep)
        for x in other.deep:
            try:
                deep.remove(x)
            except ValueError:
                return False
        if deep:
            return False
        high = list(self.high)
        for x in other.high:
            try:
                high.remove(x)
            except ValueError:
                return False
        if high:
            return False
        return True


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
class CopyItem:
    src: StackItem
    dst: StackItem


class EffectManager:
    """Manage stack effects and offsets for an instruction."""

    instr: Instruction
    active_caches: list[ActiveCacheEffect]
    peeks: list[StackItem]
    pokes: list[StackItem]
    copies: list[CopyItem]  # See merge()
    # Track offsets from stack pointer
    min_offset: StackOffset
    final_offset: StackOffset
    # Link to previous manager
    pred: "EffectManager | None" = None

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

        self.pred = pred
        while pred:
            # Replace push(x) + pop(y) with copy(x, y).
            # Check that the sources and destinations are disjoint.
            sources: set[str] = set()
            destinations: set[str] = set()
            while (
                pred.pokes
                and self.peeks
                and pred.pokes[-1].effect == self.peeks[0].effect
            ):
                src = pred.pokes.pop(-1)
                dst = self.peeks.pop(0)
                assert src.offset.equivalent_to(dst.offset), (src, dst)
                pred.final_offset.deeper(src.effect)
                if dst.effect.name != src.effect.name:
                    if dst.effect.name != UNUSED:
                        destinations.add(dst.effect.name)
                    if src.effect.name != UNUSED:
                        sources.add(src.effect.name)
                self.copies.append(CopyItem(src, dst))
            # TODO: Turn this into an error (pass an Analyzer instance?)
            assert sources & destinations == set(), (
                pred.instr.name,
                self.instr.name,
                sources,
                destinations,
            )
            # See if we can get more copies of a earlier predecessor.
            if self.peeks and not pred.pokes and not pred.peeks:
                pred = pred.pred
            else:
                pred = None  # Break

        # Fix up patterns of copies through UNUSED,
        # e.g. cp(a, UNUSED) + cp(UNUSED, b) -> cp(a, b).
        if any(copy.src.effect.name == UNUSED for copy in self.copies):
            pred = self.pred
            while pred is not None:
                for copy in self.copies:
                    if copy.src.effect.name == UNUSED:
                        for pred_copy in pred.copies:
                            if pred_copy.dst == copy.src:
                                copy.src = pred_copy.src
                                break
                pred = pred.pred

    def adjust_deeper(self, eff: StackEffect) -> None:
        for peek in self.peeks:
            peek.offset.deeper(eff)
        for poke in self.pokes:
            poke.offset.deeper(eff)
        for copy in self.copies:
            copy.src.offset.deeper(eff)
            copy.dst.offset.deeper(eff)
        self.min_offset.deeper(eff)
        self.final_offset.deeper(eff)

    def adjust_higher(self, eff: StackEffect) -> None:
        for peek in self.peeks:
            peek.offset.higher(eff)
        for poke in self.pokes:
            poke.offset.higher(eff)
        for copy in self.copies:
            copy.src.offset.higher(eff)
            copy.dst.offset.higher(eff)
        self.min_offset.higher(eff)
        self.final_offset.higher(eff)

    def adjust(self, offset: StackOffset) -> None:
        deep = list(offset.deep)
        high = list(offset.high)
        for down in deep:
            self.adjust_deeper(down)
        for up in high:
            self.adjust_higher(up)

    def adjust_inverse(self, offset: StackOffset) -> None:
        deep = list(offset.deep)
        high = list(offset.high)
        for down in deep:
            self.adjust_higher(down)
        for up in high:
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
            add(copy.src.effect)
            add(copy.dst.effect)
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
            0,
            instr.family,
        )
    except AssertionError as err:
        raise AssertionError(f"Error writing instruction {instr.name}") from err


def write_macro_instr(mac: MacroInstruction, out: Formatter) -> None:
    parts = [
        part
        for part in mac.parts
        if isinstance(part, Component) and part.instr.name != "_SET_IP"
    ]
    out.emit("")
    with out.block(f"TARGET({mac.name})"):
        if mac.predicted:
            out.emit(f"PREDICTED({mac.name});")
        out.static_assert_family_size(mac.name, mac.family, mac.cache_offset)
        try:
            next_instr_is_set = write_components(
                parts, out, TIER_ONE, mac.cache_offset, mac.family
            )
        except AssertionError as err:
            raise AssertionError(f"Error writing macro {mac.name}") from err
        if not parts[-1].instr.always_exits:
            if not next_instr_is_set and mac.cache_offset:
                out.emit(f"next_instr += {mac.cache_offset};")
            if parts[-1].instr.check_eval_breaker:
                out.emit("CHECK_EVAL_BREAKER();")
            out.emit("DISPATCH();")


def write_components(
    parts: list[Component],
    out: Formatter,
    tier: Tiers,
    cache_offset: int,
    family: Family | None,
) -> bool:
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

    next_instr_is_set = False
    for mgr in managers:
        if len(parts) > 1:
            out.emit(f"// {mgr.instr.name}")

        for copy in mgr.copies:
            copy_src_effect = copy.src.effect
            if copy_src_effect.name != copy.dst.effect.name:
                if copy_src_effect.name == UNUSED:
                    copy_src_effect = copy.src.as_stack_effect()
                out.assign(copy.dst.effect, copy_src_effect)
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

        if mgr.instr.name in ("_PUSH_FRAME", "_POP_FRAME"):
            # Adjust stack to min_offset.
            # This means that all input effects of this instruction
            # are materialized, but not its output effects.
            # That's as intended, since these two are so special.
            out.stack_adjust(mgr.min_offset.deep, mgr.min_offset.high)
            # However, for tier 2, pretend the stack is at final offset.
            mgr.adjust_inverse(mgr.final_offset)
            if tier == TIER_ONE:
                # TODO: Check in analyzer that _{PUSH,POP}_FRAME is last.
                assert (
                    mgr is managers[-1]
                ), f"Expected {mgr.instr.name!r} to be the last uop"
                assert_no_pokes(managers)

        if mgr.instr.name == "_SAVE_RETURN_OFFSET":
            next_instr_is_set = True
            if cache_offset:
                out.emit(f"next_instr += {cache_offset};")
            if tier == TIER_ONE:
                assert_no_pokes(managers)

        if len(parts) == 1:
            mgr.instr.write_body(out, 0, mgr.active_caches, tier, family)
        else:
            with out.block(""):
                mgr.instr.write_body(out, -4, mgr.active_caches, tier, family)

        if mgr is managers[-1] and not next_instr_is_set and not mgr.instr.always_exits:
            # Adjust the stack to its final depth, *then* write the
            # pokes for all preceding uops.
            # Note that for array output effects we may still write
            # past the stack top.
            out.stack_adjust(mgr.final_offset.deep, mgr.final_offset.high)
            write_all_pokes(mgr.final_offset, managers, out)

    return next_instr_is_set


def assert_no_pokes(managers: list[EffectManager]) -> None:
    for mgr in managers:
        for poke in mgr.pokes:
            if not poke.effect.size and poke.effect.name not in mgr.instr.unmoved_names:
                assert (
                    poke.effect.name == UNUSED
                ), f"Unexpected poke of {poke.effect.name} in {mgr.instr.name!r}"


def write_all_pokes(
    offset: StackOffset, managers: list[EffectManager], out: Formatter
) -> None:
    # Emit all remaining pushes (pokes)
    for m in managers:
        m.adjust_inverse(offset)
        write_pokes(m, out)


def write_pokes(mgr: EffectManager, out: Formatter) -> None:
    for poke in mgr.pokes:
        if not poke.effect.size and poke.effect.name not in mgr.instr.unmoved_names:
            out.assign(
                poke.as_stack_effect(),
                poke.effect,
            )


def write_single_instr_for_abstract_interp(instr: Instruction, out: Formatter) -> None:
    try:
        _write_components_for_abstract_interp(
            [Component(instr, instr.active_caches)],
            out,
        )
    except AssertionError as err:
        raise AssertionError(
            f"Error writing abstract instruction {instr.name}"
        ) from err


def _write_components_for_abstract_interp(
    parts: list[Component],
    out: Formatter,
) -> None:
    managers = get_managers(parts)
    for mgr in managers:
        if mgr is managers[-1]:
            out.stack_adjust(mgr.final_offset.deep, mgr.final_offset.high)
            mgr.adjust_inverse(mgr.final_offset)
        # NULL out the output stack effects
        for poke in mgr.pokes:
            if not poke.effect.size and poke.effect.name not in mgr.instr.unmoved_names:
                out.emit(
                    f"PARTITIONNODE_OVERWRITE((_Py_PARTITIONNODE_t *)"
                    f"PARTITIONNODE_NULLROOT, PEEK(-({poke.offset.as_index()})), true);"
                )
