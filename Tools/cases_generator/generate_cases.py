"""Generate the main interpreter switch.
Reads the instruction definitions from bytecodes.c.
Writes the cases to generated_cases.c.h, which is #included in ceval.c.
"""

import argparse
import contextlib
import os
import posixpath
import sys
import typing

from analysis import Analyzer
from formatting import Formatter, list_effect_size, maybe_parenthesize
from flags import InstructionFlags, variable_used
from instructions import (
    AnyInstruction,
    Component,
    Instruction,
    MacroInstruction,
    MacroParts,
    PseudoInstruction,
    StackEffect,
    OverriddenInstructionPlaceHolder,
    TIER_TWO,
)
import parsing
from parsing import StackEffect


HERE = os.path.dirname(__file__)
ROOT = os.path.join(HERE, "../..")
THIS = os.path.relpath(__file__, ROOT).replace(os.path.sep, posixpath.sep)

DEFAULT_INPUT = os.path.relpath(os.path.join(ROOT, "Python/bytecodes.c"))
DEFAULT_OUTPUT = os.path.relpath(os.path.join(ROOT, "Python/generated_cases.c.h"))
DEFAULT_METADATA_OUTPUT = os.path.relpath(
    os.path.join(ROOT, "Include/internal/pycore_opcode_metadata.h")
)
DEFAULT_PYMETADATA_OUTPUT = os.path.relpath(
    os.path.join(ROOT, "Lib/_opcode_metadata.py")
)
DEFAULT_EXECUTOR_OUTPUT = os.path.relpath(
    os.path.join(ROOT, "Python/executor_cases.c.h")
)

# Constants used instead of size for macro expansions.
# Note: 1, 2, 4 must match actual cache entry sizes.
OPARG_SIZES = {
    "OPARG_FULL": 0,
    "OPARG_CACHE_1": 1,
    "OPARG_CACHE_2": 2,
    "OPARG_CACHE_4": 4,
    "OPARG_TOP": 5,
    "OPARG_BOTTOM": 6,
}

INSTR_FMT_PREFIX = "INSTR_FMT_"

arg_parser = argparse.ArgumentParser(
    description="Generate the code for the interpreter switch.",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
)
arg_parser.add_argument(
    "-o", "--output", type=str, help="Generated code", default=DEFAULT_OUTPUT
)
arg_parser.add_argument(
    "-m",
    "--metadata",
    type=str,
    help="Generated C metadata",
    default=DEFAULT_METADATA_OUTPUT,
)
arg_parser.add_argument(
    "-p",
    "--pymetadata",
    type=str,
    help="Generated Python metadata",
    default=DEFAULT_PYMETADATA_OUTPUT,
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


class Generator(Analyzer):
    def get_stack_effect_info(
        self, thing: parsing.InstDef | parsing.Macro | parsing.Pseudo
    ) -> tuple[AnyInstruction | None, str | None, str | None]:
        def effect_str(effects: list[StackEffect]) -> str:
            n_effect, sym_effect = list_effect_size(effects)
            if sym_effect:
                return f"{sym_effect} + {n_effect}" if n_effect else sym_effect
            return str(n_effect)

        instr: AnyInstruction | None
        popped: str | None
        pushed: str | None
        match thing:
            case parsing.InstDef():
                if thing.kind != "op":
                    instr = self.instrs[thing.name]
                    popped = effect_str(instr.input_effects)
                    pushed = effect_str(instr.output_effects)
                else:
                    instr = None
                    popped = ""
                    pushed = ""
            case parsing.Macro():
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
                            if effect.cond in ("0", "1"):
                                pushed_symbolic.append(effect.cond)
                            else:
                                pushed_symbolic.append(
                                    maybe_parenthesize(
                                        f"{maybe_parenthesize(effect.cond)} ? 1 : 0"
                                    )
                                )
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
            case parsing.Pseudo():
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
            self.out.emit(
                f"extern int _PyOpcode_num_{direction}(int opcode, int oparg, bool jump);"
            )
            self.out.emit("#else")
            self.out.emit("int")
            self.out.emit(
                f"_PyOpcode_num_{direction}(int opcode, int oparg, bool jump) {{"
            )
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
        filenames = []
        for filename in self.input_filenames:
            try:
                filename = os.path.relpath(filename, ROOT)
            except ValueError:
                # May happen on Windows if root and temp on different volumes
                pass
            filenames.append(filename)
        paths = f"\n{self.out.comment}   ".join(filenames)
        return f"{self.out.comment} from:\n{self.out.comment}   {paths}\n"

    def write_provenance_header(self):
        self.out.write_raw(f"{self.out.comment} This file is generated by {THIS}\n")
        self.out.write_raw(self.from_source_files())
        self.out.write_raw(f"{self.out.comment} Do not edit!\n")

    def write_metadata(self, metadata_filename: str, pymetadata_filename: str) -> None:
        """Write instruction metadata to output file."""

        # Compute the set of all instruction formats.
        all_formats: set[str] = set()
        for thing in self.everything:
            format: str | None
            match thing:
                case OverriddenInstructionPlaceHolder():
                    continue
                case parsing.InstDef():
                    format = self.instrs[thing.name].instr_fmt
                case parsing.Macro():
                    format = self.macro_instrs[thing.name].instr_fmt
                case parsing.Pseudo():
                    format = None
                    for target in self.pseudos[thing.name].targets:
                        target_instr = self.instrs.get(target)
                        assert target_instr
                        if format is None:
                            format = target_instr.instr_fmt
                        else:
                            assert format == target_instr.instr_fmt
                    assert format is not None
                case _:
                    typing.assert_never(thing)
            all_formats.add(format)
        # Turn it into a list of enum definitions.
        format_enums = [INSTR_FMT_PREFIX + format for format in sorted(all_formats)]

        with open(metadata_filename, "w") as f:
            # Create formatter
            self.out = Formatter(f, 0)

            self.write_provenance_header()

            self.out.emit("\n#include <stdbool.h>")

            self.write_pseudo_instrs()

            self.out.emit("")
            self.write_uop_items(lambda name, counter: f"#define {name} {counter}")

            self.write_stack_effect_functions()

            # Write type definitions
            self.out.emit(f"enum InstructionFormat {{ {', '.join(format_enums)} }};")

            self.out.emit("")
            self.out.emit(
                "#define IS_VALID_OPCODE(OP) \\\n"
                "    (((OP) >= 0) && ((OP) < OPCODE_METADATA_SIZE) && \\\n"
                "     (_PyOpcode_opcode_metadata[(OP)].valid_entry))"
            )

            self.out.emit("")
            InstructionFlags.emit_macros(self.out)

            self.out.emit("")
            with self.out.block("struct opcode_metadata", ";"):
                self.out.emit("bool valid_entry;")
                self.out.emit("enum InstructionFormat instr_format;")
                self.out.emit("int flags;")
            self.out.emit("")

            with self.out.block("struct opcode_macro_expansion", ";"):
                self.out.emit("int nuops;")
                self.out.emit(
                    "struct { int16_t uop; int8_t size; int8_t offset; } uops[8];"
                )
            self.out.emit("")

            for key, value in OPARG_SIZES.items():
                self.out.emit(f"#define {key} {value}")
            self.out.emit("")

            self.out.emit(
                "#define OPCODE_METADATA_FMT(OP) "
                "(_PyOpcode_opcode_metadata[(OP)].instr_format)"
            )
            self.out.emit("#define SAME_OPCODE_METADATA(OP1, OP2) \\")
            self.out.emit(
                "        (OPCODE_METADATA_FMT(OP1) == OPCODE_METADATA_FMT(OP2))"
            )
            self.out.emit("")

            # Write metadata array declaration
            self.out.emit("#define OPCODE_METADATA_SIZE 512")
            self.out.emit("#define OPCODE_UOP_NAME_SIZE 512")
            self.out.emit("#define OPCODE_MACRO_EXPANSION_SIZE 256")
            self.out.emit("")
            self.out.emit("#ifndef NEED_OPCODE_METADATA")
            self.out.emit(
                "extern const struct opcode_metadata "
                "_PyOpcode_opcode_metadata[OPCODE_METADATA_SIZE];"
            )
            self.out.emit(
                "extern const struct opcode_macro_expansion "
                "_PyOpcode_macro_expansion[OPCODE_MACRO_EXPANSION_SIZE];"
            )
            self.out.emit(
                "extern const char * const _PyOpcode_uop_name[OPCODE_UOP_NAME_SIZE];"
            )
            self.out.emit("#else // if NEED_OPCODE_METADATA")

            self.out.emit(
                "const struct opcode_metadata "
                "_PyOpcode_opcode_metadata[OPCODE_METADATA_SIZE] = {"
            )

            # Write metadata for each instruction
            for thing in self.everything:
                match thing:
                    case OverriddenInstructionPlaceHolder():
                        continue
                    case parsing.InstDef():
                        if thing.kind != "op":
                            self.write_metadata_for_inst(self.instrs[thing.name])
                    case parsing.Macro():
                        self.write_metadata_for_macro(self.macro_instrs[thing.name])
                    case parsing.Pseudo():
                        self.write_metadata_for_pseudo(self.pseudo_instrs[thing.name])
                    case _:
                        typing.assert_never(thing)

            # Write end of array
            self.out.emit("};")

            with self.out.block(
                "const struct opcode_macro_expansion "
                "_PyOpcode_macro_expansion[OPCODE_MACRO_EXPANSION_SIZE] =",
                ";",
            ):
                # Write macro expansion for each non-pseudo instruction
                for thing in self.everything:
                    match thing:
                        case OverriddenInstructionPlaceHolder():
                            pass
                        case parsing.InstDef(name=name):
                            instr = self.instrs[name]
                            # Since an 'op' is not a bytecode, it has no expansion; but 'inst' is
                            if instr.kind == "inst" and instr.is_viable_uop():
                                # Construct a dummy Component -- input/output mappings are not used
                                part = Component(instr, [], [], instr.active_caches)
                                self.write_macro_expansions(instr.name, [part])
                            elif instr.kind == "inst" and variable_used(
                                instr.inst, "oparg1"
                            ):
                                assert variable_used(
                                    instr.inst, "oparg2"
                                ), "Half super-instr?"
                                self.write_super_expansions(instr.name)
                        case parsing.Macro():
                            mac = self.macro_instrs[thing.name]
                            self.write_macro_expansions(mac.name, mac.parts)
                        case parsing.Pseudo():
                            pass
                        case _:
                            typing.assert_never(thing)

            with self.out.block(
                "const char * const _PyOpcode_uop_name[OPCODE_UOP_NAME_SIZE] =", ";"
            ):
                self.write_uop_items(lambda name, counter: f'[{name}] = "{name}",')

            self.out.emit("#endif // NEED_OPCODE_METADATA")

        with open(pymetadata_filename, "w") as f:
            # Create formatter
            self.out = Formatter(f, 0, comment="#")

            self.write_provenance_header()

            self.out.emit("")
            self.out.emit("_specializations = {")
            for name, family in self.families.items():
                with self.out.indent():
                    self.out.emit(f'"{family.name}": [')
                    with self.out.indent():
                        for m in family.members:
                            self.out.emit(f'"{m}",')
                    self.out.emit(f"],")
            self.out.emit("}")

            # Handle special case
            self.out.emit("")
            self.out.emit("# An irregular case:")
            self.out.emit(
                '_specializations["BINARY_OP"].append('
                '"BINARY_OP_INPLACE_ADD_UNICODE")'
            )

            # Make list of specialized instructions
            self.out.emit("")
            self.out.emit(
                "_specialized_instructions = ["
                "opcode for family in _specializations.values() for opcode in family"
                "]"
            )

    def write_pseudo_instrs(self) -> None:
        """Write the IS_PSEUDO_INSTR macro"""
        self.out.emit("\n\n#define IS_PSEUDO_INSTR(OP)  ( \\")
        for op in self.pseudos:
            self.out.emit(f"    ((OP) == {op}) || \\")
        self.out.emit(f"    0)")

    def write_uop_items(self, make_text: typing.Callable[[str, int], str]) -> None:
        """Write '#define XXX NNN' for each uop"""
        counter = 300  # TODO: Avoid collision with pseudo instructions
        seen = set()

        def add(name: str) -> None:
            if name in seen:
                return
            nonlocal counter
            self.out.emit(make_text(name, counter))
            counter += 1
            seen.add(name)

        # These two are first by convention
        add("EXIT_TRACE")
        add("SAVE_IP")

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
                if not part.active_caches:
                    size, offset = OPARG_SIZES["OPARG_FULL"], 0
                else:
                    # If this assert triggers, is_viable_uops() lied
                    assert len(part.active_caches) == 1, (name, part.instr.name)
                    cache = part.active_caches[0]
                    size, offset = cache.effect.size, cache.offset
                expansions.append((part.instr.name, size, offset))
        assert len(expansions) > 0, f"Macro {name} has empty expansion?!"
        self.write_expansions(name, expansions)

    def write_super_expansions(self, name: str) -> None:
        """Write special macro expansions for super-instructions.

        If you get an assertion failure here, you probably have accidentally
        violated one of the assumptions here.

        - A super-instruction's name is of the form FIRST_SECOND where
          FIRST and SECOND are regular instructions whose name has the
          form FOO_BAR. Thus, there must be exactly 3 underscores.
          Example: LOAD_CONST_STORE_FAST.

        - A super-instruction's body uses `oparg1 and `oparg2`, and no
          other instruction's body uses those variable names.

        - A super-instruction has no active (used) cache entries.

        In the expansion, the first instruction's operand is all but the
        bottom 4 bits of the super-instruction's oparg, and the second
        instruction's operand is the bottom 4 bits. We use the special
        size codes OPARG_TOP and OPARG_BOTTOM for these.
        """
        pieces = name.split("_")
        assert len(pieces) == 4, f"{name} doesn't look like a super-instr"
        name1 = "_".join(pieces[:2])
        name2 = "_".join(pieces[2:])
        assert name1 in self.instrs, f"{name1} doesn't match any instr"
        assert name2 in self.instrs, f"{name2} doesn't match any instr"
        instr1 = self.instrs[name1]
        instr2 = self.instrs[name2]
        assert not instr1.active_caches, f"{name1} has active caches"
        assert not instr2.active_caches, f"{name2} has active caches"
        expansions = [
            (name1, OPARG_SIZES["OPARG_TOP"], 0),
            (name2, OPARG_SIZES["OPARG_BOTTOM"], 0),
        ]
        self.write_expansions(name, expansions)

    def write_expansions(
        self, name: str, expansions: list[tuple[str, int, int]]
    ) -> None:
        pieces = [
            f"{{ {name}, {size}, {offset} }}" for name, size, offset in expansions
        ]
        self.out.emit(
            f"[{name}] = "
            f"{{ .nuops = {len(pieces)}, .uops = {{ {', '.join(pieces)} }} }},"
        )

    def emit_metadata_entry(self, name: str, fmt: str, flags: InstructionFlags) -> None:
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

    def write_instructions(
        self, output_filename: str, emit_line_directives: bool
    ) -> None:
        """Write instructions to output file."""
        with open(output_filename, "w") as f:
            # Create formatter
            self.out = Formatter(f, 8, emit_line_directives)

            self.write_provenance_header()

            # Write and count instructions of all kinds
            n_instrs = 0
            n_macros = 0
            n_pseudos = 0
            for thing in self.everything:
                match thing:
                    case OverriddenInstructionPlaceHolder():
                        self.write_overridden_instr_place_holder(thing)
                    case parsing.InstDef():
                        if thing.kind != "op":
                            n_instrs += 1
                            self.write_instr(self.instrs[thing.name])
                    case parsing.Macro():
                        n_macros += 1
                        self.write_macro(self.macro_instrs[thing.name])
                    case parsing.Pseudo():
                        n_pseudos += 1
                    case _:
                        typing.assert_never(thing)

        print(
            f"Wrote {n_instrs} instructions, {n_macros} macros, "
            f"and {n_pseudos} pseudos to {output_filename}",
            file=sys.stderr,
        )

    def write_executor_instructions(
        self, executor_filename: str, emit_line_directives: bool
    ) -> None:
        """Generate cases for the Tier 2 interpreter."""
        with open(executor_filename, "w") as f:
            self.out = Formatter(f, 8, emit_line_directives)
            self.write_provenance_header()
            for thing in self.everything:
                match thing:
                    case OverriddenInstructionPlaceHolder():
                        # TODO: Is this helpful?
                        self.write_overridden_instr_place_holder(thing)
                    case parsing.InstDef():
                        instr = self.instrs[thing.name]
                        if instr.is_viable_uop():
                            self.out.emit("")
                            with self.out.block(f"case {thing.name}:"):
                                instr.write(self.out, tier=TIER_TWO)
                                if instr.check_eval_breaker:
                                    self.out.emit("CHECK_EVAL_BREAKER();")
                                self.out.emit("break;")
                        # elif instr.kind != "op":
                        #     print(f"NOTE: {thing.name} is not a viable uop")
                    case parsing.Macro():
                        pass
                    case parsing.Pseudo():
                        pass
                    case _:
                        typing.assert_never(thing)
        print(
            f"Wrote some stuff to {executor_filename}",
            file=sys.stderr,
        )

    def write_overridden_instr_place_holder(
        self, place_holder: OverriddenInstructionPlaceHolder
    ) -> None:
        self.out.emit("")
        self.out.emit(
            f"{self.out.comment} TARGET({place_holder.name}) overridden by later definition"
        )

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
                    case parsing.CacheEffect(size=size):
                        cache_adjust += size
                    case Component() as comp:
                        last_instr = comp.instr
                        comp.write_body(self.out)
                        cache_adjust += comp.instr.cache_offset

            if cache_adjust:
                self.out.emit(f"next_instr += {cache_adjust};")

            if (
                (family := self.families.get(mac.name))
                and mac.name == family.name
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

            self.out.stack_adjust(ieffects[: mac.initial_sp], mac.stack[: mac.final_sp])

            for i, var in enumerate(reversed(mac.stack[: mac.final_sp]), 1):
                dst = StackEffect(f"stack_pointer[-{i}]", "")
                self.out.assign(dst, var)

            self.out.emit(f"DISPATCH();")


def main():
    """Parse command line, parse input, analyze, write output."""
    args = arg_parser.parse_args()  # Prints message and sys.exit(2) on error
    if len(args.input) == 0:
        args.input.append(DEFAULT_INPUT)

    # Raises OSError if input unreadable
    a = Generator(args.input)

    a.parse()  # Raises SyntaxError on failure
    a.analyze()  # Prints messages and sets a.errors on failure
    if a.errors:
        sys.exit(f"Found {a.errors} errors")

    # These raise OSError if output can't be written
    a.write_instructions(args.output, args.emit_line_directives)
    a.write_metadata(args.metadata, args.pymetadata)
    a.write_executor_instructions(args.executor_cases, args.emit_line_directives)


if __name__ == "__main__":
    main()
