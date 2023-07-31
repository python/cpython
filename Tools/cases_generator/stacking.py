from abc import abstractmethod
import dataclasses

from formatting import (
    Formatter,
    UNUSED,
    list_effect_size,
    maybe_parenthesize,
    parenthesize_cond,
)
from instructions import Instruction, Tiers, TIER_ONE, TIER_TWO
from parsing import StackEffect


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

    deep: list[StackEffect]
    high: list[StackEffect]

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
        """Implemented by subclasses PeekEffect, PokeEffect."""
        raise NotImplementedError("itself() not implemented")

    def as_variable(self) -> str:
        """Return e.g. stack_pointer[-1]."""
        temp = StackOffset(list(self.deep), list(self.high))
        me = self.itself()
        temp.deeper(me)
        num = 0
        terms: list[tuple[str, str]] = []
        for eff in temp.deep:
            if eff.size:
                terms.append(("-", maybe_parenthesize(eff.size)))
            elif eff.cond and eff.cond != "1":
                terms.append(("-", f"({parenthesize_cond(eff.cond)} ? 1 : 0)"))
            elif eff.cond != "0":
                num -= 1
        for eff in temp.high:
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
        if me.size:
            terms.insert(0, ("+", "stack_pointer"))
        if not terms:
            terms.append(("+", "0"))  # Avoid 'stack_pointer[]'

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

        if me.size:
            return index
        else:
            return f"stack_pointer[{index}]"


@dataclasses.dataclass
class PeekEffect(StackOffset):
    dst: StackEffect

    def itself(self) -> StackEffect:
        return self.dst


@dataclasses.dataclass
class PokeEffect(StackOffset):
    src: StackEffect

    def itself(self) -> StackEffect:
        return self.src


@dataclasses.dataclass
class CopyEffect:
    src: StackEffect
    dst: StackEffect


class AllStackEffects:
    """Represent all stack effects for an instruction."""

    # List things in order of execution
    copies: list[CopyEffect]  # See merge()
    peeks: list[PeekEffect]
    instr: Instruction
    pokes: list[PokeEffect]
    frozen: bool

    def __init__(self, instr: Instruction):
        self.copies = []
        self.peeks = []
        self.instr = instr
        self.pokes = []
        self.frozen = True
        for eff in reversed(instr.input_effects):
            new_peek = PeekEffect([], [], eff)
            for peek in self.peeks:
                new_peek.deeper(peek.dst)
            self.peeks.append(new_peek)
        for eff in instr.output_effects:
            new_poke = PokeEffect([], [], eff)
            for peek in self.peeks:
                new_poke.deeper(peek.dst)
            for poke in self.pokes:
                new_poke.higher(poke.src)
            new_poke.higher(eff)  # Account for the new poke itself (!)
            self.pokes.append(new_poke)

    def copy(self) -> "AllStackEffects":
        new = AllStackEffects(self.instr)  # Just recompute peeks/pokes
        new.frozen = False  # Make the copy mergeable
        return new

    def merge(self, follow: "AllStackEffects") -> None:
        assert not self.frozen  # Only copies can be merged
        assert not follow.frozen
        sources: set[str] = set()
        destinations: set[str] = set()
        while self.pokes and follow.peeks and self.pokes[-1] == follow.peeks[0]:
            src = self.pokes.pop(-1).src
            dst = follow.peeks.pop(0).dst
            if dst.name != UNUSED:
                if dst.name != src.name:
                    sources.add(src.name)
                destinations.add(dst.name)
            follow.copies.append(CopyEffect(src, dst))
        # TODO: Turn this into an error (pass an Analyzer instance?)
        assert sources & destinations == set(), (
            self.instr.name,
            follow.instr.name,
            sources,
            destinations,
        )
        self.frozen = True  # Can only merge once

    def collect_vars(self) -> set[StackEffect]:
        vars: set[StackEffect] = set()
        for copy in self.copies:
            if copy.dst.name != UNUSED:
                vars.add(copy.dst)
        for peek in self.peeks:
            if peek.dst.name != UNUSED:
                vars.add(peek.dst)
        for poke in self.pokes:
            if poke.src.name != UNUSED:
                vars.add(poke.src)
        return vars


def write_single_instr(
    instr: Instruction, out: Formatter, tier: Tiers = TIER_ONE
) -> None:
    """Replace (most of) Instruction.write()."""
    effects = AllStackEffects(instr)
    assert not effects.copies

    # Emit input stack effect variable declarations and initializations
    input_vars: set[str] = set()
    for peek in effects.peeks:
        if peek.dst.name != UNUSED:
            input_vars.add(peek.dst.name)
            variable = peek.as_variable()
            # TODO: Move this to Formatter.declare() (see TODO there)
            if peek.dst.cond and peek.dst.cond != "1":
                variable = f"{parenthesize_cond(peek.dst.cond)} ? {variable} : NULL"
            elif peek.dst.cond == "0":
                variable = "NULL"
            src = StackEffect(variable, peek.dst.type, peek.dst.cond, peek.dst.size)
            out.declare(peek.dst, src)

    # Emit output stack effect variable declarations
    # (with special cases for array output effects)
    for poke in effects.pokes:
        if poke.src.name != UNUSED and poke.src.name not in input_vars:
            if not poke.src.size:
                dst = None
            else:
                dst = StackEffect(
                    poke.as_variable(), poke.src.type, poke.src.cond, poke.src.size
                )
            out.declare(poke.src, dst)

    # Emit the instruction itself
    instr.write_body(out, 0, instr.active_caches, tier=tier)

    # Skip the rest if the block always exits
    if instr.always_exits:
        return

    # Emit stack adjustments
    endstate: StackOffset | None = None
    if effects.pokes:
        endstate = effects.pokes[-1]
    elif effects.peeks:
        endstate = effects.peeks[-1]
        endstate = StackOffset(list(endstate.deep), list(endstate.high))
        endstate.deeper(effects.peeks[-1].dst)
    if endstate:
        out.stack_adjust(endstate.deep, endstate.high)

    # Emit output stack effects, except for array output effects
    # (Reversed to match the old code, for easier comparison)
    # TODO: Remove reversed() call
    for poke in reversed(effects.pokes):
        if poke.src.name != UNUSED and poke.src.name not in instr.unmoved_names:
            if not poke.src.size:
                poke = PokeEffect(list(poke.deep), list(poke.high), poke.src)
                assert endstate
                for eff in endstate.deep:
                    poke.higher(eff)
                for eff in endstate.high:
                    poke.deeper(eff)
                dst = StackEffect(
                    poke.as_variable(), poke.src.type, poke.src.cond, poke.src.size
                )
                out.assign(dst, poke.src)


# Plan:
# - Do this for single (non-macro) instructions
#   - declare all vars
#     - emit all peeks
#     - emit special cases for array output effects
#     - emit the instruction
#     - emit all pokes
# - Do it for macro instructions
# - Write tests that show e.g. CALL can be split into uops
