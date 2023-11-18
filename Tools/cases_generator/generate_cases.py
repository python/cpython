"""Generate the main interpreter switch.
Reads the instruction definitions from bytecodes.c.
Writes the cases to generated_cases.c.h, which is #included in ceval.c.
"""

import argparse
import contextlib
import itertools
import os
import posixpath
import sys
import textwrap
import typing
from collections.abc import Iterator

import stacking  # Early import to avoid circular import
from _typing_backports import assert_never
from analysis import Analyzer
from formatting import Formatter, list_effect_size
from flags import InstructionFlags, variable_used
from instructions import (
    AnyInstruction,
    AbstractInstruction,
    Component,
    Instruction,
    MacroInstruction,
    MacroParts,
    PseudoInstruction,
    TIER_ONE,
    TIER_TWO,
)
import parsing
from parsing import StackEffect


HERE = os.path.dirname(__file__)
ROOT = os.path.join(HERE, "../..")
THIS = os.path.relpath(__file__, ROOT).replace(os.path.sep, posixpath.sep)

DEFAULT_INPUT = os.path.relpath(os.path.join(ROOT, "Python/bytecodes.c"))
DEFAULT_OUTPUT = os.path.relpath(os.path.join(ROOT, "Python/generated_cases.c.h"))
DEFAULT_OPCODE_IDS_H_OUTPUT = os.path.relpath(
    os.path.join(ROOT, "Include/opcode_ids.h")
)
DEFAULT_OPCODE_TARGETS_H_OUTPUT = os.path.relpath(
    os.path.join(ROOT, "Python/opcode_targets.h")
)
DEFAULT_METADATA_OUTPUT = os.path.relpath(
    os.path.join(ROOT, "Include/internal/pycore_opcode_metadata.h")
)
DEFAULT_PYMETADATA_OUTPUT = os.path.relpath(
    os.path.join(ROOT, "Lib/_opcode_metadata.py")
)
DEFAULT_EXECUTOR_OUTPUT = os.path.relpath(
    os.path.join(ROOT, "Python/executor_cases.c.h")
)
DEFAULT_ABSTRACT_INTERPRETER_OUTPUT = os.path.relpath(
    os.path.join(ROOT, "Python/abstract_interp_cases.c.h")
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
    "OPARG_SAVE_RETURN_OFFSET": 7,
}

INSTR_FMT_PREFIX = "INSTR_FMT_"

# TODO: generate all these after updating the DSL
SPECIALLY_HANDLED_ABSTRACT_INSTR = {
    "LOAD_FAST",
    "LOAD_FAST_CHECK",
    "LOAD_FAST_AND_CLEAR",
    "LOAD_CONST",
    "STORE_FAST",
    "STORE_FAST_MAYBE_NULL",
    "COPY",
    # Arithmetic
    "_BINARY_OP_MULTIPLY_INT",
    "_BINARY_OP_ADD_INT",
    "_BINARY_OP_SUBTRACT_INT",
}

arg_parser = argparse.ArgumentParser(
    description="Generate the code for the interpreter switch.",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
)

arg_parser.add_argument(
    "-v",
    "--viable",
    help="Print list of non-viable uops and exit",
    action="store_true",
)
arg_parser.add_argument(
    "-o", "--output", type=str, help="Generated code", default=DEFAULT_OUTPUT
)
arg_parser.add_argument(
    "-n",
    "--opcode_ids_h",
    type=str,
    help="Header file with opcode number definitions",
    default=DEFAULT_OPCODE_IDS_H_OUTPUT,
)
arg_parser.add_argument(
    "-t",
    "--opcode_targets_h",
    type=str,
    help="File with opcode targets for computed gotos",
    default=DEFAULT_OPCODE_TARGETS_H_OUTPUT,
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
arg_parser.add_argument(
    "-a",
    "--abstract-interpreter-cases",
    type=str,
    help="Write abstract interpreter cases to this file",
    default=DEFAULT_ABSTRACT_INTERPRETER_OUTPUT,
)


class Generator(Analyzer):
    def get_stack_effect_info(
        self, thing: parsing.InstDef | parsing.Macro | parsing.Pseudo
    ) -> tuple[AnyInstruction | None, str, str]:
        def effect_str(effects: list[StackEffect]) -> str:
            n_effect, sym_effect = list_effect_size(effects)
            if sym_effect:
                return f"{sym_effect} + {n_effect}" if n_effect else sym_effect
            return str(n_effect)

        instr: AnyInstruction | None
        popped: str | None = None
        pushed: str | None = None
        match thing:
            case parsing.InstDef():
                instr = self.instrs[thing.name]
                popped = effect_str(instr.input_effects)
                pushed = effect_str(instr.output_effects)
            case parsing.Macro():
                instr = self.macro_instrs[thing.name]
                popped, pushed = stacking.get_stack_effect_info_for_macro(instr)
            case parsing.Pseudo():
                instr = self.pseudo_instrs[thing.name]
                # Calculate stack effect, and check that it's the same
                # for all targets.
                for target in self.pseudos[thing.name].targets:
                    target_instr = self.instrs.get(target)
                    if target_instr is None:
                        macro_instr = self.macro_instrs[target]
                        popped, pushed = stacking.get_stack_effect_info_for_macro(macro_instr)
                    else:
                        target_popped = effect_str(target_instr.input_effects)
                        target_pushed = effect_str(target_instr.output_effects)
                        if popped is None:
                            popped, pushed = target_popped, target_pushed
                        else:
                            assert popped == target_popped
                            assert pushed == target_pushed
            case _:
                assert_never(thing)
        assert popped is not None and pushed is not None
        return instr, popped, pushed

    @contextlib.contextmanager
    def metadata_item(self, signature: str, open: str, close: str) -> Iterator[None]:
        self.out.emit("")
        self.out.emit(f"extern {signature};")
        self.out.emit("#ifdef NEED_OPCODE_METADATA")
        with self.out.block(f"{signature} {open}", close):
            yield
        self.out.emit("#endif // NEED_OPCODE_METADATA")

    def write_stack_effect_functions(self) -> None:
        popped_data: list[tuple[AnyInstruction, str]] = []
        pushed_data: list[tuple[AnyInstruction, str]] = []
        for thing in self.everything:
            if isinstance(thing, parsing.Macro) and thing.name in self.instrs:
                continue
            instr, popped, pushed = self.get_stack_effect_info(thing)
            if instr is not None:
                popped_data.append((instr, popped))
                pushed_data.append((instr, pushed))

        def write_function(
            direction: str, data: list[tuple[AnyInstruction, str]]
        ) -> None:
            with self.metadata_item(
                f"int _PyOpcode_num_{direction}(int opcode, int oparg, bool jump)",
                "",
                "",
            ):
                with self.out.block("switch(opcode)"):
                    for instr, effect in data:
                        self.out.emit(f"case {instr.name}:")
                        self.out.emit(f"    return {effect};")
                    self.out.emit("default:")
                    self.out.emit("    return -1;")

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
            filenames.append(filename.replace(os.path.sep, posixpath.sep))
        paths = f"\n{self.out.comment}   ".join(filenames)
        return f"{self.out.comment} from:\n{self.out.comment}   {paths}\n"

    def write_provenance_header(self) -> None:
        self.out.write_raw(f"{self.out.comment} This file is generated by {THIS}\n")
        self.out.write_raw(self.from_source_files())
        self.out.write_raw(f"{self.out.comment} Do not edit!\n")

    def assign_opcode_ids(self) -> None:
        """Assign IDs to opcodes"""

        ops: list[tuple[bool, str]] = []  # (has_arg, name) for each opcode
        instrumented_ops: list[str] = []

        specialized_ops: set[str] = set()
        for name, family in self.families.items():
            specialized_ops.update(family.members)

        for instr in self.macro_instrs.values():
            name = instr.name
            if name in specialized_ops:
                continue
            if name.startswith("INSTRUMENTED_"):
                instrumented_ops.append(name)
            else:
                ops.append((instr.instr_flags.HAS_ARG_FLAG, name))

        # Special case: this instruction is implemented in ceval.c
        # rather than bytecodes.c, so we need to add it explicitly
        # here (at least until we add something to bytecodes.c to
        # declare external instructions).
        instrumented_ops.append("INSTRUMENTED_LINE")

        # assert lists are unique
        assert len(set(ops)) == len(ops)
        assert len(set(instrumented_ops)) == len(instrumented_ops)

        opname: list[str | None] = [None] * 512
        opmap: dict[str, int] = {}
        markers: dict[str, int] = {}

        def map_op(op: int, name: str) -> None:
            assert op < len(opname)
            assert opname[op] is None, (op, name)
            assert name not in opmap
            opname[op] = name
            opmap[name] = op

        # 0 is reserved for cache entries. This helps debugging.
        map_op(0, "CACHE")

        # 17 is reserved as it is the initial value for the specializing counter.
        # This helps catch cases where we attempt to execute a cache.
        map_op(17, "RESERVED")

        # 149 is RESUME - it is hard coded as such in Tools/build/deepfreeze.py
        map_op(149, "RESUME")

        # Specialized ops appear in their own section
        # Instrumented opcodes are at the end of the valid range
        min_internal = 150
        min_instrumented = 254 - (len(instrumented_ops) - 1)
        assert min_internal + len(specialized_ops) < min_instrumented

        next_opcode = 1
        for has_arg, name in sorted(ops):
            if name in opmap:
                continue  # an anchored name, like CACHE
            map_op(next_opcode, name)
            if has_arg and "HAVE_ARGUMENT" not in markers:
                markers["HAVE_ARGUMENT"] = next_opcode

            while opname[next_opcode] is not None:
                next_opcode += 1

        assert next_opcode < min_internal, next_opcode

        for i, op in enumerate(sorted(specialized_ops)):
            map_op(min_internal + i, op)

        markers["MIN_INSTRUMENTED_OPCODE"] = min_instrumented
        for i, op in enumerate(instrumented_ops):
            map_op(min_instrumented + i, op)

        # Pseudo opcodes are after the valid range
        for i, op in enumerate(sorted(self.pseudos)):
            map_op(256 + i, op)

        assert 255 not in opmap.values()  # 255 is reserved
        self.opmap = opmap
        self.markers = markers

    def write_opcode_ids(
        self, opcode_ids_h_filename: str, opcode_targets_filename: str
    ) -> None:
        """Write header file that defined the opcode IDs"""

        with open(opcode_ids_h_filename, "w") as f:
            # Create formatter
            self.out = Formatter(f, 0)

            self.write_provenance_header()

            self.out.emit("")
            self.out.emit("#ifndef Py_OPCODE_IDS_H")
            self.out.emit("#define Py_OPCODE_IDS_H")
            self.out.emit("#ifdef __cplusplus")
            self.out.emit('extern "C" {')
            self.out.emit("#endif")
            self.out.emit("")
            self.out.emit("/* Instruction opcodes for compiled code */")

            def define(name: str, opcode: int) -> None:
                self.out.emit(f"#define {name:<38} {opcode:>3}")

            all_pairs: list[tuple[int, int, str]] = []
            # the second item in the tuple sorts the markers before the ops
            all_pairs.extend((i, 1, name) for (name, i) in self.markers.items())
            all_pairs.extend((i, 2, name) for (name, i) in self.opmap.items())
            for i, _, name in sorted(all_pairs):
                assert name is not None
                define(name, i)

            self.out.emit("")
            self.out.emit("#ifdef __cplusplus")
            self.out.emit("}")
            self.out.emit("#endif")
            self.out.emit("#endif /* !Py_OPCODE_IDS_H */")

        with open(opcode_targets_filename, "w") as f:
            # Create formatter
            self.out = Formatter(f, 0)

            with self.out.block("static void *opcode_targets[256] =", ";"):
                targets = ["_unknown_opcode"] * 256
                for name, op in self.opmap.items():
                    if op < 256:
                        targets[op] = f"TARGET_{name}"
                f.write(",\n".join([f"    &&{s}" for s in targets]))

    def write_metadata(self, metadata_filename: str, pymetadata_filename: str) -> None:
        """Write instruction metadata to output file."""

        # Compute the set of all instruction formats.
        all_formats: set[str] = set()
        for thing in self.everything:
            format: str | None = None
            match thing:
                case parsing.InstDef():
                    format = self.instrs[thing.name].instr_fmt
                case parsing.Macro():
                    format = self.macro_instrs[thing.name].instr_fmt
                case parsing.Pseudo():
                    # Pseudo instructions exist only in the compiler,
                    # so do not have a format
                    continue
                case _:
                    assert_never(thing)
            assert format is not None
            all_formats.add(format)

        # Turn it into a sorted list of enum values.
        format_enums = [INSTR_FMT_PREFIX + format for format in sorted(all_formats)]

        with open(metadata_filename, "w") as f:
            # Create formatter
            self.out = Formatter(f, 0)

            self.write_provenance_header()

            self.out.emit("")
            self.out.emit("#ifndef Py_BUILD_CORE")
            self.out.emit('#  error "this header requires Py_BUILD_CORE define"')
            self.out.emit("#endif")
            self.out.emit("")
            self.out.emit("#include <stdbool.h>              // bool")

            self.write_pseudo_instrs()

            self.out.emit("")
            self.write_uop_items(lambda name, counter: f"#define {name} {counter}")

            self.write_stack_effect_functions()

            # Write the enum definition for instruction formats.
            with self.out.block("enum InstructionFormat", ";"):
                for enum in format_enums:
                    self.out.emit(enum + ",")

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
                    "struct { int16_t uop; int8_t size; int8_t offset; } uops[12];"
                )
            self.out.emit("")

            for key, value in OPARG_SIZES.items():
                self.out.emit(f"#define {key} {value}")
            self.out.emit("")

            self.out.emit(
                "#define OPCODE_METADATA_FLAGS(OP) "
                "(_PyOpcode_opcode_metadata[(OP)].flags & (HAS_ARG_FLAG | HAS_JUMP_FLAG))"
            )
            self.out.emit("#define SAME_OPCODE_METADATA(OP1, OP2) \\")
            self.out.emit(
                "        (OPCODE_METADATA_FLAGS(OP1) == OPCODE_METADATA_FLAGS(OP2))"
            )
            self.out.emit("")

            # Write metadata array declaration
            self.out.emit("#define OPCODE_METADATA_SIZE 512")
            self.out.emit("#define OPCODE_UOP_NAME_SIZE 512")
            self.out.emit("#define OPCODE_MACRO_EXPANSION_SIZE 256")

            with self.metadata_item(
                "const struct opcode_metadata "
                "_PyOpcode_opcode_metadata[OPCODE_METADATA_SIZE]",
                "=",
                ";",
            ):
                # Write metadata for each instruction
                for thing in self.everything:
                    match thing:
                        case parsing.InstDef():
                            self.write_metadata_for_inst(self.instrs[thing.name])
                        case parsing.Macro():
                            if thing.name not in self.instrs:
                                self.write_metadata_for_macro(
                                    self.macro_instrs[thing.name]
                                )
                        case parsing.Pseudo():
                            self.write_metadata_for_pseudo(
                                self.pseudo_instrs[thing.name]
                            )
                        case _:
                            assert_never(thing)

            with self.metadata_item(
                "const struct opcode_macro_expansion "
                "_PyOpcode_macro_expansion[OPCODE_MACRO_EXPANSION_SIZE]",
                "=",
                ";",
            ):
                # Write macro expansion for each non-pseudo instruction
                for mac in self.macro_instrs.values():
                    if is_super_instruction(mac):
                        # Special-case the heck out of super-instructions
                        self.write_super_expansions(mac.name)
                    else:
                        self.write_macro_expansions(
                            mac.name, mac.parts, mac.cache_offset
                        )

            with self.metadata_item(
                "const char * const _PyOpcode_uop_name[OPCODE_UOP_NAME_SIZE]", "=", ";"
            ):
                self.write_uop_items(lambda name, counter: f'[{name}] = "{name}",')

            with self.metadata_item(
                f"const char *const _PyOpcode_OpName[{1 + max(self.opmap.values())}]",
                "=",
                ";",
            ):
                for name in self.opmap:
                    self.out.emit(f'[{name}] = "{name}",')

            with self.metadata_item(
                f"const uint8_t _PyOpcode_Caches[256]",
                "=",
                ";",
            ):
                family_member_names: set[str] = set()
                for family in self.families.values():
                    family_member_names.update(family.members)
                for mac in self.macro_instrs.values():
                    if (
                        mac.cache_offset > 0
                        and mac.name not in family_member_names
                        and not mac.name.startswith("INSTRUMENTED_")
                    ):
                        self.out.emit(f"[{mac.name}] = {mac.cache_offset},")

            deoptcodes = {}
            for name, op in self.opmap.items():
                if op < 256:
                    deoptcodes[name] = name
            for name, family in self.families.items():
                for m in family.members:
                    deoptcodes[m] = name
            # special case:
            deoptcodes["BINARY_OP_INPLACE_ADD_UNICODE"] = "BINARY_OP"

            with self.metadata_item(f"const uint8_t _PyOpcode_Deopt[256]", "=", ";"):
                for opt, deopt in sorted(deoptcodes.items()):
                    self.out.emit(f"[{opt}] = {deopt},")

            self.out.emit("")
            self.out.emit("#define EXTRA_CASES \\")
            valid_opcodes = set(self.opmap.values())
            with self.out.indent():
                for op in range(256):
                    if op not in valid_opcodes:
                        self.out.emit(f"case {op}: \\")
                self.out.emit("    ;\n")

        with open(pymetadata_filename, "w") as f:
            # Create formatter
            self.out = Formatter(f, 0, comment="#")

            self.write_provenance_header()

            # emit specializations
            specialized_ops = set()

            self.out.emit("")
            self.out.emit("_specializations = {")
            for name, family in self.families.items():
                with self.out.indent():
                    self.out.emit(f'"{family.name}": [')
                    with self.out.indent():
                        for m in family.members:
                            self.out.emit(f'"{m}",')
                        specialized_ops.update(family.members)
                    self.out.emit(f"],")
            self.out.emit("}")

            # Handle special case
            self.out.emit("")
            self.out.emit("# An irregular case:")
            self.out.emit(
                '_specializations["BINARY_OP"].append('
                '"BINARY_OP_INPLACE_ADD_UNICODE")'
            )
            specialized_ops.add("BINARY_OP_INPLACE_ADD_UNICODE")

            ops = sorted((id, name) for (name, id) in self.opmap.items())
            # emit specialized opmap
            self.out.emit("")
            with self.out.block("_specialized_opmap ="):
                for op, name in ops:
                    if name in specialized_ops:
                        self.out.emit(f"'{name}': {op},")

            # emit opmap
            self.out.emit("")
            with self.out.block("opmap ="):
                for op, name in ops:
                    if name not in specialized_ops:
                        self.out.emit(f"'{name}': {op},")

            for name in ["MIN_INSTRUMENTED_OPCODE", "HAVE_ARGUMENT"]:
                self.out.emit(f"{name} = {self.markers[name]}")

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
        add("_EXIT_TRACE")
        add("_SET_IP")

        for instr in self.instrs.values():
            # Skip ops that are also macros -- those are desugared inst()s
            if instr.name not in self.macros:
                add(instr.name)

    def write_macro_expansions(
        self, name: str, parts: MacroParts, cache_offset: int
    ) -> None:
        """Write the macro expansions for a macro-instruction."""
        # TODO: Refactor to share code with write_cody(), is_viaible_uop(), etc.
        offset = 0  # Cache effect offset
        expansions: list[tuple[str, int, int]] = []  # [(name, size, offset), ...]
        for part in parts:
            if isinstance(part, Component):
                # Skip specializations
                if "specializing" in part.instr.annotations:
                    continue
                # All other component instructions must be viable uops
                if not part.instr.is_viable_uop() and "replaced" not in part.instr.annotations:
                    # This note just reminds us about macros that cannot
                    # be expanded to Tier 2 uops. It is not an error.
                    # Suppress it using 'replaced op(...)' for macros having
                    # manual translation in translate_bytecode_to_trace()
                    # in Python/optimizer.c.
                    if len(parts) > 1 or part.instr.name != name:
                        self.note(
                            f"Part {part.instr.name} of {name} is not a viable uop",
                            part.instr.inst,
                        )
                    return
                if not part.active_caches:
                    if part.instr.name == "_SAVE_RETURN_OFFSET":
                        size, offset = OPARG_SIZES["OPARG_SAVE_RETURN_OFFSET"], cache_offset
                    else:
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
        expansions: list[tuple[str, int, int]] = [
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

    def emit_metadata_entry(self, name: str, fmt: str | None, flags: InstructionFlags) -> None:
        flag_names = flags.names(value=True)
        if not flag_names:
            flag_names.append("0")
        fmt_macro = "0" if fmt is None else INSTR_FMT_PREFIX + fmt
        self.out.emit(
            f"[{name}] = {{ true, {fmt_macro},"
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
        self.emit_metadata_entry(ps.name, None, ps.instr_flags)

    def write_instructions(
        self, output_filename: str, emit_line_directives: bool
    ) -> None:
        """Write instructions to output file."""
        with open(output_filename, "w") as f:
            # Create formatter
            self.out = Formatter(f, 8, emit_line_directives)

            self.write_provenance_header()

            self.out.write_raw("\n")
            self.out.write_raw("#ifdef TIER_TWO\n")
            self.out.write_raw("    #error \"This file is for Tier 1 only\"\n")
            self.out.write_raw("#endif\n")
            self.out.write_raw("#define TIER_ONE 1\n")

            # Write and count instructions of all kinds
            n_macros = 0
            for thing in self.everything:
                match thing:
                    case parsing.InstDef():
                        pass
                    case parsing.Macro():
                        n_macros += 1
                        mac = self.macro_instrs[thing.name]
                        stacking.write_macro_instr(mac, self.out)
                    case parsing.Pseudo():
                        pass
                    case _:
                        assert_never(thing)

            self.out.write_raw("\n")
            self.out.write_raw("#undef TIER_ONE\n")

        print(
            f"Wrote {n_macros} cases to {output_filename}",
            file=sys.stderr,
        )

    def write_executor_instructions(
        self, executor_filename: str, emit_line_directives: bool
    ) -> None:
        """Generate cases for the Tier 2 interpreter."""
        n_uops = 0
        with open(executor_filename, "w") as f:
            self.out = Formatter(f, 8, emit_line_directives)
            self.write_provenance_header()

            self.out.write_raw("\n")
            self.out.write_raw("#ifdef TIER_ONE\n")
            self.out.write_raw("    #error \"This file is for Tier 2 only\"\n")
            self.out.write_raw("#endif\n")
            self.out.write_raw("#define TIER_TWO 2\n")

            for instr in self.instrs.values():
                if instr.is_viable_uop():
                    n_uops += 1
                    self.out.emit("")
                    with self.out.block(f"case {instr.name}:"):
                        stacking.write_single_instr(instr, self.out, tier=TIER_TWO)
                        if instr.check_eval_breaker:
                            self.out.emit("CHECK_EVAL_BREAKER();")
                        self.out.emit("break;")

            self.out.write_raw("\n")
            self.out.write_raw("#undef TIER_TWO\n")

        print(
            f"Wrote {n_uops} cases to {executor_filename}",
            file=sys.stderr,
        )

    def write_abstract_interpreter_instructions(
        self, abstract_interpreter_filename: str, emit_line_directives: bool
    ) -> None:
        """Generate cases for the Tier 2 abstract interpreter/analzyer."""
        with open(abstract_interpreter_filename, "w") as f:
            self.out = Formatter(f, 8, emit_line_directives)
            self.write_provenance_header()
            for instr in self.instrs.values():
                instr = AbstractInstruction(instr.inst)
                if (
                    instr.is_viable_uop()
                    and instr.name not in SPECIALLY_HANDLED_ABSTRACT_INSTR
                ):
                    self.out.emit("")
                    with self.out.block(f"case {instr.name}:"):
                        instr.write(self.out, tier=TIER_TWO)
                        self.out.emit("break;")
        print(
            f"Wrote some stuff to {abstract_interpreter_filename}",
            file=sys.stderr,
        )


def is_super_instruction(mac: MacroInstruction) -> bool:
    if (
        len(mac.parts) == 1
        and isinstance(mac.parts[0], Component)
        and variable_used(mac.parts[0].instr.inst, "oparg1")
    ):
        assert variable_used(mac.parts[0].instr.inst, "oparg2")
        return True
    else:
        return False


def main() -> None:
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
    if args.viable:
        # Load execution counts from bmraw.json, if it exists
        a.report_non_viable_uops("bmraw.json")
        return

    # These raise OSError if output can't be written
    a.write_instructions(args.output, args.emit_line_directives)

    a.assign_opcode_ids()
    a.write_opcode_ids(args.opcode_ids_h, args.opcode_targets_h)
    a.write_metadata(args.metadata, args.pymetadata)
    a.write_executor_instructions(args.executor_cases, args.emit_line_directives)
    a.write_abstract_interpreter_instructions(
        args.abstract_interpreter_cases, args.emit_line_directives
    )


if __name__ == "__main__":
    main()
