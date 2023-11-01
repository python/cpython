import dataclasses
import re
import typing

from flags import InstructionFlags, variable_used, variable_used_unspecialized
from formatting import (
    Formatter,
    UNUSED,
    list_effect_size,
)
import lexer as lx
import parsing
from parsing import StackEffect
import stacking

BITS_PER_CODE_UNIT = 16


@dataclasses.dataclass
class ActiveCacheEffect:
    """Wraps a CacheEffect that is actually used, in context."""

    effect: parsing.CacheEffect
    offset: int


FORBIDDEN_NAMES_IN_UOPS = (
    "next_instr",
    "oparg1",  # Proxy for super-instructions like LOAD_FAST_LOAD_FAST
    "JUMPBY",
    "DISPATCH",
    "TIER_ONE_ONLY",
)


# Interpreter tiers
TIER_ONE: typing.Final = 1  # Specializing adaptive interpreter (PEP 659)
TIER_TWO: typing.Final = 2  # Experimental tracing interpreter
Tiers: typing.TypeAlias = typing.Literal[1, 2]


@dataclasses.dataclass
class Instruction:
    """An instruction with additional data and code."""

    # Parts of the underlying instruction definition
    inst: parsing.InstDef
    name: str
    block: parsing.Block
    block_text: list[str]  # Block.text, less curlies, less PREDICT() calls
    block_line: int  # First line of block in original code

    # Computed by constructor
    always_exits: str  # If the block always exits, its last line; else ""
    has_deopt: bool
    needs_this_instr: bool
    cache_offset: int
    cache_effects: list[parsing.CacheEffect]
    input_effects: list[StackEffect]
    output_effects: list[StackEffect]
    unmoved_names: frozenset[str]
    instr_fmt: str
    instr_flags: InstructionFlags
    active_caches: list[ActiveCacheEffect]

    # Set later
    family: parsing.Family | None = None
    predicted: bool = False

    def __init__(self, inst: parsing.InstDef):
        self.inst = inst
        self.name = inst.name
        self.block = inst.block
        self.block_text, self.check_eval_breaker, self.block_line = extract_block_text(
            self.block
        )
        self.always_exits = always_exits(self.block_text)
        self.has_deopt = variable_used(self.inst, "DEOPT_IF")
        self.cache_effects = [
            effect for effect in inst.inputs if isinstance(effect, parsing.CacheEffect)
        ]
        self.cache_offset = sum(c.size for c in self.cache_effects)
        self.needs_this_instr = variable_used(self.inst, "this_instr") or any(c.name != UNUSED for c in self.cache_effects)
        self.input_effects = [
            effect for effect in inst.inputs if isinstance(effect, StackEffect)
        ]
        self.output_effects = inst.outputs  # For consistency/completeness
        unmoved_names: set[str] = set()
        for ieffect, oeffect in zip(self.input_effects, self.output_effects):
            if ieffect == oeffect and ieffect.name == oeffect.name:
                unmoved_names.add(ieffect.name)
            else:
                break
        self.unmoved_names = frozenset(unmoved_names)

        self.instr_flags = InstructionFlags.fromInstruction(inst)

        self.active_caches = []
        offset = 0
        for effect in self.cache_effects:
            if effect.name != UNUSED:
                self.active_caches.append(ActiveCacheEffect(effect, offset))
            offset += effect.size

        if self.instr_flags.HAS_ARG_FLAG:
            fmt = "IB"
        else:
            fmt = "IX"
        if offset:
            fmt += "C" + "0" * (offset - 1)
        self.instr_fmt = fmt

    def is_viable_uop(self) -> bool:
        """Whether this instruction is viable as a uop."""
        dprint: typing.Callable[..., None] = lambda *args, **kwargs: None
        if "FRAME" in self.name:
            dprint = print

        if self.name == "_EXIT_TRACE":
            return True  # This has 'return frame' but it's okay
        if self.always_exits:
            dprint(f"Skipping {self.name} because it always exits: {self.always_exits}")
            return False
        if len(self.active_caches) > 1:
            # print(f"Skipping {self.name} because it has >1 cache entries")
            return False
        res = True
        for forbidden in FORBIDDEN_NAMES_IN_UOPS:
            # NOTE: To disallow unspecialized uops, use
            # if variable_used(self.inst, forbidden):
            if variable_used_unspecialized(self.inst, forbidden):
                dprint(f"Skipping {self.name} because it uses {forbidden}")
                res = False
        return res

    def write_body(
        self,
        out: Formatter,
        dedent: int,
        active_caches: list[ActiveCacheEffect],
        tier: Tiers,
        family: parsing.Family | None,
    ) -> None:
        """Write the instruction body."""
        # Write cache effect variable declarations and initializations
        for active in active_caches:
            ceffect = active.effect
            bits = ceffect.size * BITS_PER_CODE_UNIT
            if bits == 64:
                # NOTE: We assume that 64-bit data in the cache
                # is always an object pointer.
                # If this becomes false, we need a way to specify
                # syntactically what type the cache data is.
                typ = "PyObject *"
                func = "read_obj"
            else:
                typ = f"uint{bits}_t "
                func = f"read_u{bits}"
            if tier == TIER_ONE:
                out.emit(
                    f"{typ}{ceffect.name} = "
                    f"{func}(&this_instr[{active.offset + 1}].cache);"
                )
            else:
                out.emit(f"{typ}{ceffect.name} = ({typ.strip()})next_uop[-1].operand;")

        # Write the body, substituting a goto for ERROR_IF() and other stuff
        assert dedent <= 0
        extra = " " * -dedent
        names_to_skip = self.unmoved_names | frozenset({UNUSED, "null"})
        offset = 0
        context = self.block.context
        assert context is not None and context.owner is not None
        filename = context.owner.filename
        for line in self.block_text:
            out.set_lineno(self.block_line + offset, filename)
            offset += 1
            if m := re.match(r"(\s*)ERROR_IF\((.+), (\w+)\);\s*(?://.*)?$", line):
                space, cond, label = m.groups()
                space = extra + space
                # ERROR_IF() must pop the inputs from the stack.
                # The code block is responsible for DECREF()ing them.
                # NOTE: If the label doesn't exist, just add it to ceval.c.

                # Don't pop common input/output effects at the bottom!
                # These aren't DECREF'ed so they can stay.
                ieffs = list(self.input_effects)
                oeffs = list(self.output_effects)
                while (
                    ieffs
                    and oeffs
                    and ieffs[0] == oeffs[0]
                    and ieffs[0].name == oeffs[0].name
                ):
                    ieffs.pop(0)
                    oeffs.pop(0)
                ninputs, symbolic = list_effect_size(ieffs)
                if ninputs:
                    label = f"pop_{ninputs}_{label}"
                if tier == TIER_TWO:
                    label = label + "_tier_two"
                if symbolic:
                    out.write_raw(
                        f"{space}if ({cond}) {{ STACK_SHRINK({symbolic}); goto {label}; }}\n"
                    )
                else:
                    out.write_raw(f"{space}if ({cond}) goto {label};\n")
            elif m := re.match(r"(\s*)DEOPT_IF\((.+)\);\s*(?://.*)?$", line):
                space, cond = m.groups()
                space = extra + space
                target = family.name if family else self.name
                out.write_raw(f"{space}DEOPT_IF({cond}, {target});\n")
            elif "DEOPT" in line:
                filename = context.owner.filename
                lineno = context.owner.tokens[context.begin].line
                print(f"{filename}:{lineno}: ERROR: DEOPT_IF() must be all on one line")
                out.write_raw(extra + line)
            elif m := re.match(r"(\s*)DECREF_INPUTS\(\);\s*(?://.*)?$", line):
                out.reset_lineno()
                space = extra + m.group(1)
                for ieff in self.input_effects:
                    if ieff.name in names_to_skip:
                        continue
                    if ieff.size:
                        out.write_raw(
                            f"{space}for (int _i = {ieff.size}; --_i >= 0;) {{\n"
                        )
                        out.write_raw(f"{space}    Py_DECREF({ieff.name}[_i]);\n")
                        out.write_raw(f"{space}}}\n")
                    else:
                        decref = "XDECREF" if ieff.cond else "DECREF"
                        out.write_raw(f"{space}Py_{decref}({ieff.name});\n")
            else:
                out.write_raw(extra + line)
        out.reset_lineno()


InstructionOrCacheEffect = Instruction | parsing.CacheEffect


# Instruction used for abstract interpretation.
class AbstractInstruction(Instruction):
    def __init__(self, inst: parsing.InstDef):
        super().__init__(inst)

    def write(self, out: Formatter, tier: Tiers = TIER_ONE) -> None:
        """Write one abstract instruction, sans prologue and epilogue."""
        stacking.write_single_instr_for_abstract_interp(self, out)

    def write_body(
        self,
        out: Formatter,
        dedent: int,
        active_caches: list[ActiveCacheEffect],
        tier: Tiers,
        family: parsing.Family | None,
    ) -> None:
        pass


@dataclasses.dataclass
class Component:
    instr: Instruction
    active_caches: list[ActiveCacheEffect]


MacroParts = list[Component | parsing.CacheEffect]


@dataclasses.dataclass
class MacroInstruction:
    """A macro instruction."""

    name: str
    instr_fmt: str
    instr_flags: InstructionFlags
    macro: parsing.Macro
    parts: MacroParts
    cache_offset: int
    # Set later
    predicted: bool = False
    family: parsing.Family | None = None


@dataclasses.dataclass
class PseudoInstruction:
    """A pseudo instruction."""

    name: str
    targets: list[Instruction | MacroInstruction]
    instr_flags: InstructionFlags


AnyInstruction = Instruction | MacroInstruction | PseudoInstruction


def extract_block_text(block: parsing.Block) -> tuple[list[str], bool, int]:
    # Get lines of text with proper dedent
    blocklines = block.text.splitlines(True)
    first_token: lx.Token = block.tokens[0]  # IndexError means the context is broken
    block_line = first_token.begin[0]

    # Remove blank lines from both ends
    while blocklines and not blocklines[0].strip():
        blocklines.pop(0)
        block_line += 1
    while blocklines and not blocklines[-1].strip():
        blocklines.pop()

    # Remove leading and trailing braces
    assert blocklines and blocklines[0].strip() == "{"
    assert blocklines and blocklines[-1].strip() == "}"
    blocklines.pop()
    blocklines.pop(0)
    block_line += 1

    # Remove trailing blank lines
    while blocklines and not blocklines[-1].strip():
        blocklines.pop()

    # Separate CHECK_EVAL_BREAKER() macro from end
    check_eval_breaker = (
        blocklines != [] and blocklines[-1].strip() == "CHECK_EVAL_BREAKER();"
    )
    if check_eval_breaker:
        del blocklines[-1]

    return blocklines, check_eval_breaker, block_line


def always_exits(lines: list[str]) -> str:
    """Determine whether a block always ends in a return/goto/etc."""
    if not lines:
        return ""
    line = lines[-1].rstrip()
    # Indent must match exactly (TODO: Do something better)
    if line[:12] != " " * 12:
        return ""
    line = line[12:]
    if line.startswith(
        (
            "goto ",
            "return ",
            "DISPATCH",
            "GO_TO_",
            "Py_UNREACHABLE()",
            "ERROR_IF(true, ",
        )
    ):
        return line
    return ""
