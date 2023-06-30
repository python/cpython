"""Generate the main interpreter switch.

Reads the instruction definitions from bytecodes.c.
Writes the cases to generated_cases.c.h, which is #included in ceval.c.
"""

import argparse
import contextlib
import dataclasses
import os
import posixpath
import re
import sys
import typing

import lexer as lx
import parser
from parser import StackEffect

HERE = os.path.dirname(__file__)
ROOT = os.path.join(HERE, "../..")
THIS = os.path.relpath(__file__, ROOT).replace(os.path.sep, posixpath.sep)

DEFAULT_INPUT = os.path.relpath(os.path.join(ROOT, "Python/bytecodes.c"))
DEFAULT_OUTPUT = os.path.relpath(os.path.join(ROOT, "Python/generated_cases.c.h"))
DEFAULT_METADATA_OUTPUT = os.path.relpath(
    os.path.join(ROOT, "Python/opcode_metadata.h")
)
DEFAULT_PYMETADATA_OUTPUT = os.path.relpath(
    os.path.join(ROOT, "Lib/_opcode_metadata.py")
)
DEFAULT_EXECUTOR_OUTPUT = os.path.relpath(
    os.path.join(ROOT, "Python/executor_cases.c.h")
)
BEGIN_MARKER = "// BEGIN BYTECODES //"
END_MARKER = "// END BYTECODES //"
RE_PREDICTED = (
    r"^\s*(?:GO_TO_INSTRUCTION\(|DEOPT_IF\(.*?,\s*)(\w+)\);\s*(?://.*)?$"
)
UNUSED = "unused"
BITS_PER_CODE_UNIT = 16

RESERVED_WORDS = {
    "co_consts" : "Use FRAME_CO_CONSTS.",
    "co_names": "Use FRAME_CO_NAMES.",
}

arg_parser = argparse.ArgumentParser(
    description="Generate the code for the interpreter switch.",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
)
arg_parser.add_argument(
    "-o", "--output", type=str, help="Generated code", default=DEFAULT_OUTPUT
)
arg_parser.add_argument(
    "-m", "--metadata", type=str, help="Generated C metadata", default=DEFAULT_METADATA_OUTPUT
)
arg_parser.add_argument(
    "-p", "--pymetadata", type=str, help="Generated Python metadata", default=DEFAULT_PYMETADATA_OUTPUT
)
arg_parser.add_argument(
    "-l", "--emit-line-directives", help="Emit #line directives", action="store_true"
)
arg_parser.add_argument(
    "input", nargs=argparse.REMAINDER, help="Instruction definition file(s)"
)
arg_parser.add_argument(
    "-e",
    "--executor-cases",
    type=str,
    help="Write executor cases to this file",
    default=DEFAULT_EXECUTOR_OUTPUT,
)


def effect_size(effect: StackEffect) -> tuple[int, str]:
    """Return the 'size' impact of a stack effect.

    Returns a tuple (numeric, symbolic) where:

    - numeric is an int giving the statically analyzable size of the effect
    - symbolic is a string representing a variable effect (e.g. 'oparg*2')

    At most one of these will be non-zero / non-empty.
    """
    if effect.size:
        assert not effect.cond, "Array effects cannot have a condition"
        return 0, effect.size
    elif effect.cond:
        return 0, f"{maybe_parenthesize(effect.cond)} ? 1 : 0"
    else:
        return 1, ""


def maybe_parenthesize(sym: str) -> str:
    """Add parentheses around a string if it contains an operator.

    An exception is made for '*' which is common and harmless
    in the context where the symbolic size is used.
    """
    if re.match(r"^[\s\w*]+$", sym):
        return sym
    else:
        return f"({sym})"


def list_effect_size(effects: list[StackEffect]) -> tuple[int, str]:
    numeric = 0
    symbolic: list[str] = []
    for effect in effects:
        diff, sym = effect_size(effect)
        numeric += diff
        if sym:
            symbolic.append(maybe_parenthesize(sym))
    return numeric, " + ".join(symbolic)


def string_effect_size(arg: tuple[int, str]) -> str:
    numeric, symbolic = arg
    if numeric and symbolic:
        return f"{numeric} + {symbolic}"
    elif symbolic:
        return symbolic
    else:
        return str(numeric)


class Formatter:
    """Wraps an output stream with the ability to indent etc."""

    stream: typing.TextIO
    prefix: str
    emit_line_directives: bool = False
    lineno: int  # Next line number, 1-based
    filename: str  # Slightly improved stream.filename
    nominal_lineno: int
    nominal_filename: str

    def __init__(
            self, stream: typing.TextIO, indent: int,
                  emit_line_directives: bool = False, comment: str = "//",
    ) -> None:
        self.stream = stream
        self.prefix = " " * indent
        self.emit_line_directives = emit_line_directives
        self.comment = comment
        self.lineno = 1
        filename = os.path.relpath(self.stream.name, ROOT)
        # Make filename more user-friendly and less platform-specific
        filename = filename.replace("\\", "/")
        if filename.startswith("./"):
            filename = filename[2:]
        if filename.endswith(".new"):
            filename = filename[:-4]
        self.filename = filename
        self.nominal_lineno = 1
        self.nominal_filename = filename

    def write_raw(self, s: str) -> None:
        self.stream.write(s)
        newlines = s.count("\n")
        self.lineno += newlines
        self.nominal_lineno += newlines

    def emit(self, arg: str) -> None:
        if arg:
            self.write_raw(f"{self.prefix}{arg}\n")
        else:
            self.write_raw("\n")

    def set_lineno(self, lineno: int, filename: str) -> None:
        if self.emit_line_directives:
            if lineno != self.nominal_lineno or filename != self.nominal_filename:
                self.emit(f'#line {lineno} "{filename}"')
                self.nominal_lineno = lineno
                self.nominal_filename = filename

    def reset_lineno(self) -> None:
        if self.lineno != self.nominal_lineno or self.filename != self.nominal_filename:
            self.set_lineno(self.lineno + 1, self.filename)

    @contextlib.contextmanager
    def indent(self):
        self.prefix += "    "
        yield
        self.prefix = self.prefix[:-4]

    @contextlib.contextmanager
    def block(self, head: str, tail: str = ""):
        if head:
            self.emit(head + " {")
        else:
            self.emit("{")
        with self.indent():
            yield
        self.emit("}" + tail)

    def stack_adjust(
        self,
        input_effects: list[StackEffect],
        output_effects: list[StackEffect],
    ):
        shrink, isym = list_effect_size(input_effects)
        grow, osym = list_effect_size(output_effects)
        diff = grow - shrink
        if isym and isym != osym:
            self.emit(f"STACK_SHRINK({isym});")
        if diff < 0:
            self.emit(f"STACK_SHRINK({-diff});")
        if diff > 0:
            self.emit(f"STACK_GROW({diff});")
        if osym and osym != isym:
            self.emit(f"STACK_GROW({osym});")

    def declare(self, dst: StackEffect, src: StackEffect | None):
        if dst.name == UNUSED:
            return
        typ = f"{dst.type}" if dst.type else "PyObject *"
        if src:
            cast = self.cast(dst, src)
            init = f" = {cast}{src.name}"
        elif dst.cond:
            init = " = NULL"
        else:
            init = ""
        sepa = "" if typ.endswith("*") else " "
        self.emit(f"{typ}{sepa}{dst.name}{init};")

    def assign(self, dst: StackEffect, src: StackEffect):
        if src.name == UNUSED:
            return
        if src.size:
            # Don't write sized arrays -- it's up to the user code.
            return
        cast = self.cast(dst, src)
        if re.match(r"^REG\(oparg(\d+)\)$", dst.name):
            self.emit(f"Py_XSETREF({dst.name}, {cast}{src.name});")
        else:
            stmt = f"{dst.name} = {cast}{src.name};"
            if src.cond:
                stmt = f"if ({src.cond}) {{ {stmt} }}"
            self.emit(stmt)

    def cast(self, dst: StackEffect, src: StackEffect) -> str:
        return f"({dst.type or 'PyObject *'})" if src.type != dst.type else ""

@dataclasses.dataclass
class InstructionFlags:
    """Construct and manipulate instruction flags"""

    HAS_ARG_FLAG: bool
    HAS_CONST_FLAG: bool
    HAS_NAME_FLAG: bool
    HAS_JUMP_FLAG: bool

    def __post_init__(self):
        self.bitmask = {
            name : (1 << i) for i, name in enumerate(self.names())
        }

    @staticmethod
    def fromInstruction(instr: "AnyInstruction"):
        return InstructionFlags(
            HAS_ARG_FLAG=variable_used(instr, "oparg"),
            HAS_CONST_FLAG=variable_used(instr, "FRAME_CO_CONSTS"),
            HAS_NAME_FLAG=variable_used(instr, "FRAME_CO_NAMES"),
            HAS_JUMP_FLAG=variable_used(instr, "JUMPBY"),
        )

    @staticmethod
    def newEmpty():
        return InstructionFlags(False, False, False, False)

    def add(self, other: "InstructionFlags") -> None:
        for name, value in dataclasses.asdict(other).items():
            if value:
                setattr(self, name, value)

    def names(self, value=None):
        if value is None:
            return dataclasses.asdict(self).keys()
        return [n for n, v in dataclasses.asdict(self).items() if v == value]

    def bitmap(self) -> int:
        flags = 0
        for name in self.names():
            if getattr(self, name):
                flags |= self.bitmask[name]
        return flags

    @classmethod
    def emit_macros(cls, out: Formatter):
        flags = cls.newEmpty()
        for name, value in flags.bitmask.items():
            out.emit(f"#define {name} ({value})");

        for name, value in flags.bitmask.items():
            out.emit(
                f"#define OPCODE_{name[:-len('_FLAG')]}(OP) "
                f"(_PyOpcode_opcode_metadata[(OP)].flags & ({name}))")


@dataclasses.dataclass
class ActiveCacheEffect:
    """Wraps a CacheEffect that is actually used, in context."""
    effect: parser.CacheEffect
    offset: int


FORBIDDEN_NAMES_IN_UOPS = (
    "resume_with_error",  # Proxy for "goto", which isn't an IDENTIFIER
    "unbound_local_error",
    "kwnames",
    "next_instr",
    "oparg1",  # Proxy for super-instructions like LOAD_FAST_LOAD_FAST
    "JUMPBY",
    "DISPATCH",
    "INSTRUMENTED_JUMP",
    "throwflag",
    "exception_unwind",
    "import_from",
    "import_name",
    "_PyObject_CallNoArgs",  # Proxy for BEFORE_WITH
)


# Interpreter tiers
TIER_ONE = 1  # Specializing adaptive interpreter (PEP 659)
TIER_TWO = 2  # Experimental tracing interpreter
Tiers: typing.TypeAlias = typing.Literal[1, 2]


@dataclasses.dataclass
class Instruction:
    """An instruction with additional data and code."""

    # Parts of the underlying instruction definition
    inst: parser.InstDef
    kind: typing.Literal["inst", "op"]
    name: str
    block: parser.Block
    block_text: list[str]  # Block.text, less curlies, less PREDICT() calls
    block_line: int  # First line of block in original code

    # Computed by constructor
    always_exits: bool
    cache_offset: int
    cache_effects: list[parser.CacheEffect]
    input_effects: list[StackEffect]
    output_effects: list[StackEffect]
    unmoved_names: frozenset[str]
    instr_fmt: str
    instr_flags: InstructionFlags
    active_caches: list[ActiveCacheEffect]

    # Set later
    family: parser.Family | None = None
    predicted: bool = False

    def __init__(self, inst: parser.InstDef):
        self.inst = inst
        self.kind = inst.kind
        self.name = inst.name
        self.block = inst.block
        self.block_text, self.check_eval_breaker, self.block_line = \
            extract_block_text(self.block)
        self.always_exits = always_exits(self.block_text)
        self.cache_effects = [
            effect for effect in inst.inputs if isinstance(effect, parser.CacheEffect)
        ]
        self.cache_offset = sum(c.size for c in self.cache_effects)
        self.input_effects = [
            effect for effect in inst.inputs if isinstance(effect, StackEffect)
        ]
        self.output_effects = inst.outputs  # For consistency/completeness
        unmoved_names: set[str] = set()
        for ieffect, oeffect in zip(self.input_effects, self.output_effects):
            if ieffect.name == oeffect.name:
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
            fmt += "C" + "0"*(offset-1)
        self.instr_fmt = fmt

    def is_viable_uop(self) -> bool:
        """Whether this instruction is viable as a uop."""
        if self.always_exits:
            return False
        if self.instr_flags.HAS_ARG_FLAG:
            # If the instruction uses oparg, it cannot use any caches
            if self.active_caches:
                return False
        else:
            # If it doesn't use oparg, it can have one cache entry
            if len(self.active_caches) > 1:
                return False
        for forbidden in FORBIDDEN_NAMES_IN_UOPS:
            # TODO: Don't check in '#ifdef ENABLE_SPECIALIZATION' regions
            if variable_used(self.inst, forbidden):
                return False
        return True

    def write(self, out: Formatter, tier: Tiers = TIER_ONE) -> None:
        """Write one instruction, sans prologue and epilogue."""
        # Write a static assertion that a family's cache size is correct
        if family := self.family:
            if self.name == family.members[0]:
                if cache_size := family.size:
                    out.emit(
                        f"static_assert({cache_size} == "
                        f'{self.cache_offset}, "incorrect cache size");'
                    )

        # Write input stack effect variable declarations and initializations
        ieffects = list(reversed(self.input_effects))
        for i, ieffect in enumerate(ieffects):
            isize = string_effect_size(
                list_effect_size([ieff for ieff in ieffects[: i + 1]])
            )
            if ieffect.size:
                src = StackEffect(f"(stack_pointer - {maybe_parenthesize(isize)})", "PyObject **")
            elif ieffect.cond:
                src = StackEffect(f"({ieffect.cond}) ? stack_pointer[-{maybe_parenthesize(isize)}] : NULL", "")
            else:
                src = StackEffect(f"stack_pointer[-{maybe_parenthesize(isize)}]", "")
            out.declare(ieffect, src)

        # Write output stack effect variable declarations
        isize = string_effect_size(list_effect_size(self.input_effects))
        input_names = {ieffect.name for ieffect in self.input_effects}
        for i, oeffect in enumerate(self.output_effects):
            if oeffect.name not in input_names:
                if oeffect.size:
                    osize = string_effect_size(
                        list_effect_size([oeff for oeff in self.output_effects[:i]])
                    )
                    offset = "stack_pointer"
                    if isize != osize:
                        if isize != "0":
                            offset += f" - ({isize})"
                        if osize != "0":
                            offset += f" + {osize}"
                    src = StackEffect(offset, "PyObject **")
                    out.declare(oeffect, src)
                else:
                    out.declare(oeffect, None)

        # out.emit(f"next_instr += OPSIZE({self.inst.name}) - 1;")

        self.write_body(out, 0, self.active_caches, tier=tier)

        # Skip the rest if the block always exits
        if self.always_exits:
            return

        # Write net stack growth/shrinkage
        out.stack_adjust(
            [ieff for ieff in self.input_effects],
            [oeff for oeff in self.output_effects],
        )

        # Write output stack effect assignments
        oeffects = list(reversed(self.output_effects))
        for i, oeffect in enumerate(oeffects):
            if oeffect.name in self.unmoved_names:
                continue
            osize = string_effect_size(
                list_effect_size([oeff for oeff in oeffects[: i + 1]])
            )
            if oeffect.size:
                dst = StackEffect(f"stack_pointer - {maybe_parenthesize(osize)}", "PyObject **")
            else:
                dst = StackEffect(f"stack_pointer[-{maybe_parenthesize(osize)}]", "")
            out.assign(dst, oeffect)

        # Write cache effect
        if tier == TIER_ONE and self.cache_offset:
            out.emit(f"next_instr += {self.cache_offset};")

    def write_body(
            self,
            out: Formatter,
            dedent: int,
            active_caches: list[ActiveCacheEffect],
            tier: Tiers = TIER_ONE,
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
                    f"{typ}{ceffect.name} = {func}(&next_instr[{active.offset}].cache);"
                )
            else:
                out.emit(f"{typ}{ceffect.name} = ({typ.strip()})operand;")

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
                while ieffs and oeffs and ieffs[0] == oeffs[0]:
                    ieffs.pop(0)
                    oeffs.pop(0)
                ninputs, symbolic = list_effect_size(ieffs)
                if ninputs:
                    label = f"pop_{ninputs}_{label}"
                if symbolic:
                    out.write_raw(
                        f"{space}if ({cond}) {{ STACK_SHRINK({symbolic}); goto {label}; }}\n"
                    )
                else:
                    out.write_raw(f"{space}if ({cond}) goto {label};\n")
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


InstructionOrCacheEffect = Instruction | parser.CacheEffect
StackEffectMapping = list[tuple[StackEffect, StackEffect]]


@dataclasses.dataclass
class Component:
    instr: Instruction
    input_mapping: StackEffectMapping
    output_mapping: StackEffectMapping
    active_caches: list[ActiveCacheEffect]

    def write_body(self, out: Formatter) -> None:
        with out.block(""):
            input_names = {ieffect.name for _, ieffect in self.input_mapping}
            for var, ieffect in self.input_mapping:
                out.declare(ieffect, var)
            for _, oeffect in self.output_mapping:
                if oeffect.name not in input_names:
                    out.declare(oeffect, None)

            self.instr.write_body(out, -4, self.active_caches)

            for var, oeffect in self.output_mapping:
                out.assign(var, oeffect)


MacroParts = list[Component | parser.CacheEffect]


@dataclasses.dataclass
class MacroInstruction:
    """A macro instruction."""

    name: str
    stack: list[StackEffect]
    initial_sp: int
    final_sp: int
    instr_fmt: str
    instr_flags: InstructionFlags
    macro: parser.Macro
    parts: MacroParts
    cache_offset: int
    predicted: bool = False


@dataclasses.dataclass
class PseudoInstruction:
    """A pseudo instruction."""

    name: str
    targets: list[Instruction]
    instr_fmt: str
    instr_flags: InstructionFlags


@dataclasses.dataclass
class OverriddenInstructionPlaceHolder:
    name: str


AnyInstruction = Instruction | MacroInstruction | PseudoInstruction
INSTR_FMT_PREFIX = "INSTR_FMT_"


class Analyzer:
    """Parse input, analyze it, and write to output."""

    input_filenames: list[str]
    output_filename: str
    metadata_filename: str
    pymetadata_filename: str
    executor_filename: str
    errors: int = 0
    emit_line_directives: bool = False

    def __init__(
        self,
        input_filenames: list[str],
        output_filename: str,
        metadata_filename: str,
        pymetadata_filename: str,
        executor_filename: str,
    ):
        """Read the input file."""
        self.input_filenames = input_filenames
        self.output_filename = output_filename
        self.metadata_filename = metadata_filename
        self.pymetadata_filename = pymetadata_filename
        self.executor_filename = executor_filename

    def error(self, msg: str, node: parser.Node) -> None:
        lineno = 0
        filename = "<unknown file>"
        if context := node.context:
            filename = context.owner.filename
            # Use line number of first non-comment in the node
            for token in context.owner.tokens[context.begin : context.end]:
                lineno = token.line
                if token.kind != "COMMENT":
                    break
        print(f"{filename}:{lineno}: {msg}", file=sys.stderr)
        self.errors += 1

    everything: list[
        parser.InstDef | parser.Macro | parser.Pseudo | OverriddenInstructionPlaceHolder
    ]
    instrs: dict[str, Instruction]  # Includes ops
    macros: dict[str, parser.Macro]
    macro_instrs: dict[str, MacroInstruction]
    families: dict[str, parser.Family]
    pseudos: dict[str, parser.Pseudo]
    pseudo_instrs: dict[str, PseudoInstruction]

    def parse(self) -> None:
        """Parse the source text.

        We only want the parser to see the stuff between the
        begin and end markers.
        """

        self.everything = []
        self.instrs = {}
        self.macros = {}
        self.families = {}
        self.pseudos = {}

        instrs_idx: dict[str, int] = dict()

        for filename in self.input_filenames:
            self.parse_file(filename, instrs_idx)

        files = " + ".join(self.input_filenames)
        print(
            f"Read {len(self.instrs)} instructions/ops, "
            f"{len(self.macros)} macros, {len(self.pseudos)} pseudos, "
            f"and {len(self.families)} families from {files}",
            file=sys.stderr,
        )

    def parse_file(self, filename: str, instrs_idx: dict[str, int]) -> None:
        with open(filename) as file:
            src = file.read()

        filename = os.path.relpath(filename, ROOT)
        # Make filename more user-friendly and less platform-specific
        filename = filename.replace("\\", "/")
        if filename.startswith("./"):
            filename = filename[2:]
        psr = parser.Parser(src, filename=filename)

        # Skip until begin marker
        while tkn := psr.next(raw=True):
            if tkn.text == BEGIN_MARKER:
                break
        else:
            raise psr.make_syntax_error(
                f"Couldn't find {BEGIN_MARKER!r} in {psr.filename}"
            )
        start = psr.getpos()

        # Find end marker, then delete everything after it
        while tkn := psr.next(raw=True):
            if tkn.text == END_MARKER:
                break
        del psr.tokens[psr.getpos() - 1 :]

        # Parse from start
        psr.setpos(start)
        thing: parser.InstDef | parser.Macro | parser.Pseudo | parser.Family | None
        thing_first_token = psr.peek()
        while thing := psr.definition():
            if ws := [w for w in RESERVED_WORDS if variable_used(thing, w)]:
                self.error(f"'{ws[0]}' is a reserved word. {RESERVED_WORDS[ws[0]]}", thing)

            match thing:
                case parser.InstDef(name=name):
                    if name in self.instrs:
                        if not thing.override:
                            raise psr.make_syntax_error(
                                f"Duplicate definition of '{name}' @ {thing.context} "
                                f"previous definition @ {self.instrs[name].inst.context}",
                                thing_first_token,
                            )
                        self.everything[instrs_idx[name]] = OverriddenInstructionPlaceHolder(name=name)
                    if name not in self.instrs and thing.override:
                        raise psr.make_syntax_error(
                            f"Definition of '{name}' @ {thing.context} is supposed to be "
                            "an override but no previous definition exists.",
                            thing_first_token,
                        )
                    self.instrs[name] = Instruction(thing)
                    instrs_idx[name] = len(self.everything)
                    self.everything.append(thing)
                case parser.Macro(name):
                    self.macros[name] = thing
                    self.everything.append(thing)
                case parser.Family(name):
                    self.families[name] = thing
                case parser.Pseudo(name):
                    self.pseudos[name] = thing
                    self.everything.append(thing)
                case _:
                    typing.assert_never(thing)
        if not psr.eof():
            raise psr.make_syntax_error(f"Extra stuff at the end of {filename}")

    def analyze(self) -> None:
        """Analyze the inputs.

        Raises SystemExit if there is an error.
        """
        self.analyze_macros_and_pseudos()
        self.find_predictions()
        self.map_families()
        self.check_families()

    def find_predictions(self) -> None:
        """Find the instructions that need PREDICTED() labels."""
        for instr in self.instrs.values():
            targets: set[str] = set()
            for line in instr.block_text:
                if m := re.match(RE_PREDICTED, line):
                    targets.add(m.group(1))
            for target in targets:
                if target_instr := self.instrs.get(target):
                    target_instr.predicted = True
                elif target_macro := self.macro_instrs.get(target):
                    target_macro.predicted = True
                else:
                    self.error(
                        f"Unknown instruction {target!r} predicted in {instr.name!r}",
                        instr.inst,  # TODO: Use better location
                    )

    def map_families(self) -> None:
        """Link instruction names back to their family, if they have one."""
        for family in self.families.values():
            for member in family.members:
                if member_instr := self.instrs.get(member):
                    if member_instr.family not in (family, None):
                        self.error(
                            f"Instruction {member} is a member of multiple families "
                            f"({member_instr.family.name}, {family.name}).",
                            family,
                        )
                    else:
                        member_instr.family = family
                elif member_macro := self.macro_instrs.get(member):
                    for part in member_macro.parts:
                        if isinstance(part, Component):
                            if part.instr.family not in (family, None):
                                self.error(
                                    f"Component {part.instr.name} of macro {member} "
                                    f"is a member of multiple families "
                                    f"({part.instr.family.name}, {family.name}).",
                                    family,
                                )
                            else:
                                part.instr.family = family
                else:
                    self.error(
                        f"Unknown instruction {member!r} referenced in family {family.name!r}",
                        family,
                    )

    def check_families(self) -> None:
        """Check each family:

        - Must have at least 2 members
        - All members must be known instructions
        - All members must have the same cache, input and output effects
        """
        for family in self.families.values():
            if len(family.members) < 2:
                self.error(f"Family {family.name!r} has insufficient members", family)
            members = [
                member
                for member in family.members
                if member in self.instrs or member in self.macro_instrs
            ]
            if members != family.members:
                unknown = set(family.members) - set(members)
                self.error(
                    f"Family {family.name!r} has unknown members: {unknown}", family
                )
            if len(members) < 2:
                continue
            expected_effects = self.effect_counts(members[0])
            for member in members[1:]:
                member_effects = self.effect_counts(member)
                if member_effects != expected_effects:
                    self.error(
                        f"Family {family.name!r} has inconsistent "
                        f"(cache, input, output) effects:\n"
                        f"  {family.members[0]} = {expected_effects}; "
                        f"{member} = {member_effects}",
                        family,
                    )

    def effect_counts(self, name: str) -> tuple[int, int, int]:
        if instr := self.instrs.get(name):
            cache = instr.cache_offset
            input = len(instr.input_effects)
            output = len(instr.output_effects)
        elif mac := self.macro_instrs.get(name):
            cache = mac.cache_offset
            input, output = 0, 0
            for part in mac.parts:
                if isinstance(part, Component):
                    # A component may pop what the previous component pushed,
                    # so we offset the input/output counts by that.
                    delta_i = len(part.instr.input_effects)
                    delta_o = len(part.instr.output_effects)
                    offset = min(delta_i, output)
                    input += delta_i - offset
                    output += delta_o - offset
        else:
            assert False, f"Unknown instruction {name!r}"
        return cache, input, output

    def analyze_macros_and_pseudos(self) -> None:
        """Analyze each macro and pseudo instruction."""
        self.macro_instrs = {}
        self.pseudo_instrs = {}
        for name, macro in self.macros.items():
            self.macro_instrs[name] = self.analyze_macro(macro)
        for name, pseudo in self.pseudos.items():
            self.pseudo_instrs[name] = self.analyze_pseudo(pseudo)

    def analyze_macro(self, macro: parser.Macro) -> MacroInstruction:
        components = self.check_macro_components(macro)
        stack, initial_sp = self.stack_analysis(components)
        sp = initial_sp
        parts: MacroParts = []
        flags = InstructionFlags.newEmpty()
        offset = 0
        for component in components:
            match component:
                case parser.CacheEffect() as ceffect:
                    parts.append(ceffect)
                    offset += ceffect.size
                case Instruction() as instr:
                    part, sp, offset = self.analyze_instruction(instr, stack, sp, offset)
                    parts.append(part)
                    flags.add(instr.instr_flags)
                case _:
                    typing.assert_never(component)
        final_sp = sp
        format = "IB"
        if offset:
            format += "C" + "0"*(offset-1)
        return MacroInstruction(
            macro.name, stack, initial_sp, final_sp, format, flags, macro, parts, offset
        )

    def analyze_pseudo(self, pseudo: parser.Pseudo) -> PseudoInstruction:
        targets = [self.instrs[target] for target in pseudo.targets]
        assert targets
        # Make sure the targets have the same fmt
        fmts = list(set([t.instr_fmt for t in targets]))
        assert(len(fmts) == 1)
        assert(len(list(set([t.instr_flags.bitmap() for t in targets]))) == 1)
        return PseudoInstruction(pseudo.name, targets, fmts[0], targets[0].instr_flags)

    def analyze_instruction(
        self, instr: Instruction, stack: list[StackEffect], sp: int, offset: int
    ) -> tuple[Component, int, int]:
        input_mapping: StackEffectMapping = []
        for ieffect in reversed(instr.input_effects):
            sp -= 1
            input_mapping.append((stack[sp], ieffect))
        output_mapping: StackEffectMapping = []
        for oeffect in instr.output_effects:
            output_mapping.append((stack[sp], oeffect))
            sp += 1
        active_effects: list[ActiveCacheEffect] = []
        for ceffect in instr.cache_effects:
            if ceffect.name != UNUSED:
                active_effects.append(ActiveCacheEffect(ceffect, offset))
            offset += ceffect.size
        return Component(instr, input_mapping, output_mapping, active_effects), sp, offset

    def check_macro_components(
        self, macro: parser.Macro
    ) -> list[InstructionOrCacheEffect]:
        components: list[InstructionOrCacheEffect] = []
        for uop in macro.uops:
            match uop:
                case parser.OpName(name):
                    if name not in self.instrs:
                        self.error(f"Unknown instruction {name!r}", macro)
                    components.append(self.instrs[name])
                case parser.CacheEffect():
                    components.append(uop)
                case _:
                    typing.assert_never(uop)
        return components

    def stack_analysis(
        self, components: typing.Iterable[InstructionOrCacheEffect]
    ) -> tuple[list[StackEffect], int]:
        """Analyze a macro.

        Ignore cache effects.

        Return the list of variables (as StackEffects) and the initial stack pointer.
        """
        lowest = current = highest = 0
        conditions: dict[int, str] = {}  # Indexed by 'current'.
        last_instr: Instruction | None = None
        for thing in components:
            if isinstance(thing, Instruction):
                last_instr = thing
        for thing in components:
            match thing:
                case Instruction() as instr:
                    if any(
                        eff.size for eff in instr.input_effects + instr.output_effects
                    ):
                        # TODO: Eventually this will be needed, at least for macros.
                        self.error(
                            f"Instruction {instr.name!r} has variable-sized stack effect, "
                            "which are not supported in macro instructions",
                            instr.inst,  # TODO: Pass name+location of macro
                        )
                    if any(eff.cond for eff in instr.input_effects):
                        self.error(
                            f"Instruction {instr.name!r} has conditional input stack effect, "
                            "which are not supported in macro instructions",
                            instr.inst,  # TODO: Pass name+location of macro
                        )
                    if any(eff.cond for eff in instr.output_effects) and instr is not last_instr:
                        self.error(
                            f"Instruction {instr.name!r} has conditional output stack effect, "
                            "but is not the last instruction in a macro",
                            instr.inst,  # TODO: Pass name+location of macro
                        )
                    current -= len(instr.input_effects)
                    lowest = min(lowest, current)
                    for eff in instr.output_effects:
                        if eff.cond:
                            conditions[current] = eff.cond
                        current += 1
                    highest = max(highest, current)
                case parser.CacheEffect():
                    pass
                case _:
                    typing.assert_never(thing)
        # At this point, 'current' is the net stack effect,
        # and 'lowest' and 'highest' are the extremes.
        # Note that 'lowest' may be negative.
        stack = [
            StackEffect(f"_tmp_{i}", "", conditions.get(highest - i, ""))
            for i in reversed(range(1, highest - lowest + 1))
        ]
        return stack, -lowest

    def get_stack_effect_info(
        self, thing: parser.InstDef | parser.Macro | parser.Pseudo
    ) -> tuple[AnyInstruction | None, str | None, str | None]:
        def effect_str(effects: list[StackEffect]) -> str:
            n_effect, sym_effect = list_effect_size(effects)
            if sym_effect:
                return f"{sym_effect} + {n_effect}" if n_effect else sym_effect
            return str(n_effect)

        instr: AnyInstruction | None
        match thing:
            case parser.InstDef():
                if thing.kind != "op":
                    instr = self.instrs[thing.name]
                    popped = effect_str(instr.input_effects)
                    pushed = effect_str(instr.output_effects)
                else:
                    instr = None
                    popped = ""
                    pushed = ""
            case parser.Macro():
                instr = self.macro_instrs[thing.name]
                parts = [comp for comp in instr.parts if isinstance(comp, Component)]
                # Note: stack_analysis() already verifies that macro components
                # have no variable-sized stack effects.
                low = 0
                sp = 0
                high = 0
                pushed_symbolic: list[str] = []
                for comp in parts:
                    for effect in comp.instr.input_effects:
                        assert not effect.cond, effect
                        assert not effect.size, effect
                        sp -= 1
                        low = min(low, sp)
                    for effect in comp.instr.output_effects:
                        assert not effect.size, effect
                        if effect.cond:
                            pushed_symbolic.append(maybe_parenthesize(f"{maybe_parenthesize(effect.cond)} ? 1 : 0"))
                        sp += 1
                        high = max(sp, high)
                if high != max(0, sp):
                    # If you get this, intermediate stack growth occurs,
                    # and stack size calculations may go awry.
                    # E.g. [push, pop]. The fix would be for stack size
                    # calculations to use the micro ops.
                    self.error("Macro has virtual stack growth", thing)
                popped = str(-low)
                pushed_symbolic.append(str(sp - low - len(pushed_symbolic)))
                pushed = " + ".join(pushed_symbolic)
            case parser.Pseudo():
                instr = self.pseudo_instrs[thing.name]
                popped = pushed = None
                # Calculate stack effect, and check that it's the the same
                # for all targets.
                for target in self.pseudos[thing.name].targets:
                    target_instr = self.instrs.get(target)
                    # Currently target is always an instr. This could change
                    # in the future, e.g., if we have a pseudo targetting a
                    # macro instruction.
                    assert target_instr
                    target_popped = effect_str(target_instr.input_effects)
                    target_pushed = effect_str(target_instr.output_effects)
                    if popped is None and pushed is None:
                        popped, pushed = target_popped, target_pushed
                    else:
                        assert popped == target_popped
                        assert pushed == target_pushed
            case _:
                typing.assert_never(thing)
        return instr, popped, pushed

    def write_stack_effect_functions(self) -> None:
        popped_data: list[tuple[AnyInstruction, str]] = []
        pushed_data: list[tuple[AnyInstruction, str]] = []
        for thing in self.everything:
            if isinstance(thing, OverriddenInstructionPlaceHolder):
                continue
            instr, popped, pushed = self.get_stack_effect_info(thing)
            if instr is not None:
                assert popped is not None and pushed is not None
                popped_data.append((instr, popped))
                pushed_data.append((instr, pushed))

        def write_function(
            direction: str, data: list[tuple[AnyInstruction, str]]
        ) -> None:
            self.out.emit("")
            self.out.emit("#ifndef NEED_OPCODE_METADATA")
            self.out.emit(f"extern int _PyOpcode_num_{direction}(int opcode, int oparg, bool jump);")
            self.out.emit("#else")
            self.out.emit("int")
            self.out.emit(f"_PyOpcode_num_{direction}(int opcode, int oparg, bool jump) {{")
            self.out.emit("    switch(opcode) {")
            for instr, effect in data:
                self.out.emit(f"        case {instr.name}:")
                self.out.emit(f"            return {effect};")
            self.out.emit("        default:")
            self.out.emit("            return -1;")
            self.out.emit("    }")
            self.out.emit("}")
            self.out.emit("#endif")

        write_function("popped", popped_data)
        write_function("pushed", pushed_data)
        self.out.emit("")

    def from_source_files(self) -> str:
        paths = f"\n{self.out.comment}   ".join(
            os.path.relpath(filename, ROOT).replace(os.path.sep, posixpath.sep)
            for filename in self.input_filenames
        )
        return f"{self.out.comment} from:\n{self.out.comment}   {paths}\n"

    def write_provenance_header(self):
        self.out.write_raw(f"{self.out.comment} This file is generated by {THIS}\n")
        self.out.write_raw(self.from_source_files())
        self.out.write_raw(f"{self.out.comment} Do not edit!\n")

    def write_metadata(self) -> None:
        """Write instruction metadata to output file."""

        # Compute the set of all instruction formats.
        all_formats: set[str] = set()
        for thing in self.everything:
            match thing:
                case OverriddenInstructionPlaceHolder():
                    continue
                case parser.InstDef():
                    format = self.instrs[thing.name].instr_fmt
                case parser.Macro():
                    format = self.macro_instrs[thing.name].instr_fmt
                case parser.Pseudo():
                    format = None
                    for target in self.pseudos[thing.name].targets:
                        target_instr = self.instrs.get(target)
                        assert target_instr
                        if format is None:
                            format = target_instr.instr_fmt
                        else:
                            assert format == target_instr.instr_fmt
                case _:
                    typing.assert_never(thing)
            all_formats.add(format)
        # Turn it into a list of enum definitions.
        format_enums = [INSTR_FMT_PREFIX + format for format in sorted(all_formats)]

        with open(self.metadata_filename, "w") as f:
            # Create formatter
            self.out = Formatter(f, 0)

            self.write_provenance_header()

            self.write_pseudo_instrs()

            self.out.emit("")
            self.write_uop_items(lambda name, counter: f"#define {name} {counter}")

            self.write_stack_effect_functions()

            # Write type definitions
            self.out.emit(f"enum InstructionFormat {{ {', '.join(format_enums)} }};")

            InstructionFlags.emit_macros(self.out)

            with self.out.block("struct opcode_metadata", ";"):
                self.out.emit("bool valid_entry;")
                self.out.emit("enum InstructionFormat instr_format;")
                self.out.emit("int flags;")
            self.out.emit("")

            with self.out.block("struct opcode_macro_expansion", ";"):
                self.out.emit("int nuops;")
                self.out.emit("struct { int16_t uop; int8_t size; int8_t offset; } uops[8];")
            self.out.emit("")

            self.out.emit("")
            self.out.emit("#define OPCODE_METADATA_FMT(OP) "
                          "(_PyOpcode_opcode_metadata[(OP)].instr_format)")
            self.out.emit("#define SAME_OPCODE_METADATA(OP1, OP2) \\")
            self.out.emit("        (OPCODE_METADATA_FMT(OP1) == OPCODE_METADATA_FMT(OP2))")
            self.out.emit("")

            # Write metadata array declaration
            self.out.emit("#ifndef NEED_OPCODE_METADATA")
            self.out.emit("extern const struct opcode_metadata _PyOpcode_opcode_metadata[512];")
            self.out.emit("extern const struct opcode_macro_expansion _PyOpcode_macro_expansion[256];")
            self.out.emit("#ifdef Py_DEBUG")
            self.out.emit("extern const char * const _PyOpcode_uop_name[512];")
            self.out.emit("#endif")
            self.out.emit("#else")

            self.out.emit("const struct opcode_metadata _PyOpcode_opcode_metadata[512] = {")

            # Write metadata for each instruction
            for thing in self.everything:
                match thing:
                    case OverriddenInstructionPlaceHolder():
                        continue
                    case parser.InstDef():
                        if thing.kind != "op":
                            self.write_metadata_for_inst(self.instrs[thing.name])
                    case parser.Macro():
                        self.write_metadata_for_macro(self.macro_instrs[thing.name])
                    case parser.Pseudo():
                        self.write_metadata_for_pseudo(self.pseudo_instrs[thing.name])
                    case _:
                        typing.assert_never(thing)

            # Write end of array
            self.out.emit("};")

            with self.out.block(
                "const struct opcode_macro_expansion _PyOpcode_macro_expansion[256] =",
                ";",
            ):
                # Write macro expansion for each non-pseudo instruction
                for thing in self.everything:
                    match thing:
                        case OverriddenInstructionPlaceHolder():
                            pass
                        case parser.InstDef(name=name):
                            instr = self.instrs[name]
                            # Since an 'op' is not a bytecode, it has no expansion; but 'inst' is
                            if instr.kind == "inst" and instr.is_viable_uop():
                                # Construct a dummy Component -- input/output mappings are not used
                                part = Component(instr, [], [], instr.active_caches)
                                self.write_macro_expansions(instr.name, [part])
                        case parser.Macro():
                            mac = self.macro_instrs[thing.name]
                            self.write_macro_expansions(mac.name, mac.parts)
                        case parser.Pseudo():
                            pass
                        case _:
                            typing.assert_never(thing)

            self.out.emit("#ifdef Py_DEBUG")
            with self.out.block("const char * const _PyOpcode_uop_name[512] =", ";"):
                self.write_uop_items(lambda name, counter: f"[{counter}] = \"{name}\",")
            self.out.emit("#endif")

            self.out.emit("#endif")

        with open(self.pymetadata_filename, "w") as f:
            # Create formatter
            self.out = Formatter(f, 0, comment = "#")

            self.write_provenance_header()

            self.out.emit("")
            self.out.emit("_specializations = {")
            for name, family in self.families.items():
                assert len(family.members) > 1
                with self.out.indent():
                    self.out.emit(f"\"{family.members[0]}\": [")
                    with self.out.indent():
                        for m in family.members[1:]:
                            self.out.emit(f"\"{m}\",")
                    self.out.emit(f"],")
            self.out.emit("}")

            # Handle special case
            self.out.emit("")
            self.out.emit("# An irregular case:")
            self.out.emit(
                "_specializations[\"BINARY_OP\"].append("
                    "\"BINARY_OP_INPLACE_ADD_UNICODE\")")

            # Make list of specialized instructions
            self.out.emit("")
            self.out.emit(
                "_specialized_instructions = ["
                    "opcode for family in _specializations.values() for opcode in family"
                "]")

    def write_pseudo_instrs(self) -> None:
        """Write the IS_PSEUDO_INSTR macro"""
        self.out.emit("\n\n#define IS_PSEUDO_INSTR(OP)  ( \\")
        for op in self.pseudos:
            self.out.emit(f"    ((OP) == {op}) || \\")
        self.out.emit(f"    0)")

    def write_uop_items(self, make_text: typing.Callable[[str, int], str]) -> None:
        """Write '#define XXX NNN' for each uop"""
        counter = 300  # TODO: Avoid collision with pseudo instructions
        def add(name: str) -> None:
            nonlocal counter
            self.out.emit(make_text(name, counter))
            counter += 1
        add("EXIT_TRACE")
        add("SET_IP")
        for instr in self.instrs.values():
            if instr.kind == "op" and instr.is_viable_uop():
                add(instr.name)

    def write_macro_expansions(self, name: str, parts: MacroParts) -> None:
        """Write the macro expansions for a macro-instruction."""
        # TODO: Refactor to share code with write_cody(), is_viaible_uop(), etc.
        offset = 0  # Cache effect offset
        expansions: list[tuple[str, int, int]] = []  # [(name, size, offset), ...]
        for part in parts:
            if isinstance(part, Component):
                # All component instructions must be viable uops
                if not part.instr.is_viable_uop():
                    print(f"NOTE: Part {part.instr.name} of {name} is not a viable uop")
                    return
                if part.instr.instr_flags.HAS_ARG_FLAG or not part.active_caches:
                    size, offset = 0, 0
                else:
                    # If this assert triggers, is_viable_uops() lied
                    assert len(part.active_caches) == 1, (name, part.instr.name)
                    cache = part.active_caches[0]
                    size, offset = cache.effect.size, cache.offset
                expansions.append((part.instr.name, size, offset))
        assert len(expansions) > 0, f"Macro {name} has empty expansion?!"
        pieces = [f"{{ {name}, {size}, {offset} }}" for name, size, offset in expansions]
        self.out.emit(
            f"[{name}] = "
            f"{{ .nuops = {len(expansions)}, .uops = {{ {', '.join(pieces)} }} }},"
        )

    def emit_metadata_entry(
        self, name: str, fmt: str, flags: InstructionFlags
    ) -> None:
        flag_names = flags.names(value=True)
        if not flag_names:
            flag_names.append("0")
        self.out.emit(
            f"    [{name}] = {{ true, {INSTR_FMT_PREFIX}{fmt},"
            f" {' | '.join(flag_names)} }},"
        )

    def write_metadata_for_inst(self, instr: Instruction) -> None:
        """Write metadata for a single instruction."""
        self.emit_metadata_entry(instr.name, instr.instr_fmt, instr.instr_flags)

    def write_metadata_for_macro(self, mac: MacroInstruction) -> None:
        """Write metadata for a macro-instruction."""
        self.emit_metadata_entry(mac.name, mac.instr_fmt, mac.instr_flags)

    def write_metadata_for_pseudo(self, ps: PseudoInstruction) -> None:
        """Write metadata for a macro-instruction."""
        self.emit_metadata_entry(ps.name, ps.instr_fmt, ps.instr_flags)

    def write_instructions(self) -> None:
        """Write instructions to output file."""
        with open(self.output_filename, "w") as f:
            # Create formatter
            self.out = Formatter(f, 8, self.emit_line_directives)

            self.write_provenance_header()

            # Write and count instructions of all kinds
            n_instrs = 0
            n_macros = 0
            n_pseudos = 0
            for thing in self.everything:
                match thing:
                    case OverriddenInstructionPlaceHolder():
                        self.write_overridden_instr_place_holder(thing)
                    case parser.InstDef():
                        if thing.kind != "op":
                            n_instrs += 1
                            self.write_instr(self.instrs[thing.name])
                    case parser.Macro():
                        n_macros += 1
                        self.write_macro(self.macro_instrs[thing.name])
                    case parser.Pseudo():
                        n_pseudos += 1
                    case _:
                        typing.assert_never(thing)

        print(
            f"Wrote {n_instrs} instructions, {n_macros} macros, "
            f"and {n_pseudos} pseudos to {self.output_filename}",
            file=sys.stderr,
        )

    def write_executor_instructions(self) -> None:
        """Generate cases for the Tier 2 interpreter."""
        with open(self.executor_filename, "w") as f:
            self.out = Formatter(f, 8, self.emit_line_directives)
            self.write_provenance_header()
            for thing in self.everything:
                match thing:
                    case OverriddenInstructionPlaceHolder():
                        # TODO: Is this helpful?
                        self.write_overridden_instr_place_holder(thing)
                    case parser.InstDef():
                        instr = self.instrs[thing.name]
                        if instr.is_viable_uop():
                            self.out.emit("")
                            with self.out.block(f"case {thing.name}:"):
                                instr.write(self.out, tier=TIER_TWO)
                                self.out.emit("break;")
                    case parser.Macro():
                        pass
                    case parser.Pseudo():
                        pass
                    case _:
                        typing.assert_never(thing)
        print(
            f"Wrote some stuff to {self.executor_filename}",
            file=sys.stderr,
        )

    def write_overridden_instr_place_holder(self,
            place_holder: OverriddenInstructionPlaceHolder) -> None:
        self.out.emit("")
        self.out.emit(
            f"{self.out.comment} TARGET({place_holder.name}) overridden by later definition")

    def write_instr(self, instr: Instruction) -> None:
        name = instr.name
        self.out.emit("")
        if instr.inst.override:
            self.out.emit("{self.out.comment} Override")
        with self.out.block(f"TARGET({name})"):
            if instr.predicted:
                self.out.emit(f"PREDICTED({name});")
            instr.write(self.out)
            if not instr.always_exits:
                if instr.check_eval_breaker:
                    self.out.emit("CHECK_EVAL_BREAKER();")
                self.out.emit(f"DISPATCH();")

    def write_macro(self, mac: MacroInstruction) -> None:
        """Write code for a macro instruction."""
        last_instr: Instruction | None = None
        with self.wrap_macro(mac):
            cache_adjust = 0
            for part in mac.parts:
                match part:
                    case parser.CacheEffect(size=size):
                        cache_adjust += size
                    case Component() as comp:
                        last_instr = comp.instr
                        comp.write_body(self.out)
                        cache_adjust += comp.instr.cache_offset

            if cache_adjust:
                self.out.emit(f"next_instr += {cache_adjust};")

            if (
                last_instr
                and (family := last_instr.family)
                and mac.name == family.members[0]
                and (cache_size := family.size)
            ):
                self.out.emit(
                    f"static_assert({cache_size} == "
                    f'{cache_adjust}, "incorrect cache size");'
                )

    @contextlib.contextmanager
    def wrap_macro(self, mac: MacroInstruction):
        """Boilerplate for macro instructions."""
        # TODO: Somewhere (where?) make it so that if one instruction
        # has an output that is input to another, and the variable names
        # and types match and don't conflict with other instructions,
        # that variable is declared with the right name and type in the
        # outer block, rather than trusting the compiler to optimize it.
        self.out.emit("")
        with self.out.block(f"TARGET({mac.name})"):
            if mac.predicted:
                self.out.emit(f"PREDICTED({mac.name});")

            # The input effects should have no conditionals.
            # Only the output effects do (for now).
            ieffects = [
                StackEffect(eff.name, eff.type) if eff.cond else eff
                for eff in mac.stack
            ]

            for i, var in reversed(list(enumerate(ieffects))):
                src = None
                if i < mac.initial_sp:
                    src = StackEffect(f"stack_pointer[-{mac.initial_sp - i}]", "")
                self.out.declare(var, src)

            yield

            self.out.stack_adjust(ieffects[:mac.initial_sp], mac.stack[:mac.final_sp])

            for i, var in enumerate(reversed(mac.stack[: mac.final_sp]), 1):
                dst = StackEffect(f"stack_pointer[-{i}]", "")
                self.out.assign(dst, var)

            self.out.emit(f"DISPATCH();")


def extract_block_text(block: parser.Block) -> tuple[list[str], bool, int]:
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
    check_eval_breaker = \
        blocklines != [] and blocklines[-1].strip() == "CHECK_EVAL_BREAKER();"
    if check_eval_breaker:
        del blocklines[-1]

    return blocklines, check_eval_breaker, block_line


def always_exits(lines: list[str]) -> bool:
    """Determine whether a block always ends in a return/goto/etc."""
    if not lines:
        return False
    line = lines[-1].rstrip()
    # Indent must match exactly (TODO: Do something better)
    if line[:12] != " " * 12:
        return False
    line = line[12:]
    return line.startswith(
        (
            "goto ",
            "return ",
            "DISPATCH",
            "GO_TO_",
            "Py_UNREACHABLE()",
            "ERROR_IF(true, ",
        )
    )


def variable_used(node: parser.Node, name: str) -> bool:
    """Determine whether a variable with a given name is used in a node."""
    return any(
        token.kind == "IDENTIFIER" and token.text == name for token in node.tokens
    )


def main():
    """Parse command line, parse input, analyze, write output."""
    args = arg_parser.parse_args()  # Prints message and sys.exit(2) on error
    if len(args.input) == 0:
        args.input.append(DEFAULT_INPUT)

    # Raises OSError if input unreadable
    a = Analyzer(args.input, args.output, args.metadata, args.pymetadata, args.executor_cases)

    if args.emit_line_directives:
        a.emit_line_directives = True
    a.parse()  # Raises SyntaxError on failure
    a.analyze()  # Prints messages and sets a.errors on failure
    if a.errors:
        sys.exit(f"Found {a.errors} errors")
    a.write_instructions()  # Raises OSError if output can't be written
    a.write_metadata()
    a.write_executor_instructions()


if __name__ == "__main__":
    main()
