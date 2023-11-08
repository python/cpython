import re
import sys
import typing

from _typing_backports import assert_never
from flags import InstructionFlags, variable_used
from formatting import prettify_filename, UNUSED
from instructions import (
    ActiveCacheEffect,
    Component,
    Instruction,
    InstructionOrCacheEffect,
    MacroInstruction,
    MacroParts,
    PseudoInstruction,
)
import parsing
from parsing import StackEffect

BEGIN_MARKER = "// BEGIN BYTECODES //"
END_MARKER = "// END BYTECODES //"

RESERVED_WORDS = {
    "co_consts": "Use FRAME_CO_CONSTS.",
    "co_names": "Use FRAME_CO_NAMES.",
}

RE_GO_TO_INSTR = r"^\s*GO_TO_INSTRUCTION\((\w+)\);\s*(?://.*)?$"


class Analyzer:
    """Parse input, analyze it, and write to output."""

    input_filenames: list[str]
    errors: int = 0
    warnings: int = 0

    def __init__(self, input_filenames: list[str]):
        self.input_filenames = input_filenames

    def message(self, msg: str, node: parsing.Node) -> None:
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

    def error(self, msg: str, node: parsing.Node) -> None:
        self.message("error: " + msg, node)
        self.errors += 1

    def warning(self, msg: str, node: parsing.Node) -> None:
        self.message("warning: " + msg, node)
        self.warnings += 1

    def note(self, msg: str, node: parsing.Node) -> None:
        self.message("note: " + msg, node)

    everything: list[
        parsing.InstDef
        | parsing.Macro
        | parsing.Pseudo
    ]
    instrs: dict[str, Instruction]  # Includes ops
    macros: dict[str, parsing.Macro]
    macro_instrs: dict[str, MacroInstruction]
    families: dict[str, parsing.Family]
    pseudos: dict[str, parsing.Pseudo]
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
        n_instrs = len(set(self.instrs) & set(self.macros))
        n_ops = len(self.instrs) - n_instrs
        print(
            f"Read {n_instrs} instructions, {n_ops} ops, "
            f"{len(self.macros)} macros, {len(self.pseudos)} pseudos, "
            f"and {len(self.families)} families from {files}",
            file=sys.stderr,
        )

    def parse_file(self, filename: str, instrs_idx: dict[str, int]) -> None:
        with open(filename) as file:
            src = file.read()

        psr = parsing.Parser(src, filename=prettify_filename(filename))

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
        thing: parsing.Node | None
        thing_first_token = psr.peek()
        while thing := psr.definition():
            thing = typing.cast(
                parsing.InstDef | parsing.Macro | parsing.Pseudo | parsing.Family, thing
            )
            if ws := [w for w in RESERVED_WORDS if variable_used(thing, w)]:
                self.error(
                    f"'{ws[0]}' is a reserved word. {RESERVED_WORDS[ws[0]]}", thing
                )

            match thing:
                case parsing.InstDef(name=name):
                    macro: parsing.Macro | None = None
                    if thing.kind == "inst" and "override" not in thing.annotations:
                        macro = parsing.Macro(name, [parsing.OpName(name)])
                    if name in self.instrs:
                        if "override" not in thing.annotations:
                            raise psr.make_syntax_error(
                                f"Duplicate definition of '{name}' @ {thing.context} "
                                f"previous definition @ {self.instrs[name].inst.context}",
                                thing_first_token,
                            )
                        self.everything[instrs_idx[name]] = thing
                    if name not in self.instrs and "override" in thing.annotations:
                        raise psr.make_syntax_error(
                            f"Definition of '{name}' @ {thing.context} is supposed to be "
                            "an override but no previous definition exists.",
                            thing_first_token,
                        )
                    self.instrs[name] = Instruction(thing)
                    instrs_idx[name] = len(self.everything)
                    self.everything.append(thing)
                    if macro is not None:
                        self.macros[macro.name] = macro
                        self.everything.append(macro)
                case parsing.Macro(name):
                    self.macros[name] = thing
                    self.everything.append(thing)
                case parsing.Family(name):
                    self.families[name] = thing
                case parsing.Pseudo(name):
                    self.pseudos[name] = thing
                    self.everything.append(thing)
                case _:
                    assert_never(thing)
        if not psr.eof():
            raise psr.make_syntax_error(f"Extra stuff at the end of {filename}")

    def analyze(self) -> None:
        """Analyze the inputs.

        Raises SystemExit if there is an error.
        """
        self.analyze_macros_and_pseudos()
        self.map_families()
        self.mark_predictions()
        self.check_families()

    def mark_predictions(self) -> None:
        """Mark the instructions that need PREDICTED() labels."""
        # Start with family heads
        for family in self.families.values():
            if family.name in self.instrs:
                self.instrs[family.name].predicted = True
            if family.name in self.macro_instrs:
                self.macro_instrs[family.name].predicted = True
        # Also look for GO_TO_INSTRUCTION() calls
        for instr in self.instrs.values():
            targets: set[str] = set()
            for line in instr.block_text:
                if m := re.match(RE_GO_TO_INSTR, line):
                    targets.add(m.group(1))
            for target in targets:
                if target_instr := self.instrs.get(target):
                    target_instr.predicted = True
                if target_macro := self.macro_instrs.get(target):
                    target_macro.predicted = True
                if not target_instr and not target_macro:
                    self.error(
                        f"Unknown instruction {target!r} predicted in {instr.name!r}",
                        instr.inst,  # TODO: Use better location
                    )

    def map_families(self) -> None:
        """Link instruction names back to their family, if they have one."""
        for family in self.families.values():
            for member in [family.name] + family.members:
                if member_instr := self.instrs.get(member):
                    if (
                        member_instr.family is not family
                        and member_instr.family is not None
                    ):
                        self.error(
                            f"Instruction {member} is a member of multiple families "
                            f"({member_instr.family.name}, {family.name}).",
                            family,
                        )
                    else:
                        member_instr.family = family
                if member_mac := self.macro_instrs.get(member):
                    assert member_mac.family is None, (member, member_mac.family.name)
                    member_mac.family = family
                if not member_instr and not member_mac:
                    self.error(
                        f"Unknown instruction {member!r} referenced in family {family.name!r}",
                        family,
                    )
        # A sanctioned exception:
        # This opcode is a member of the family but it doesn't pass the checks.
        if mac := self.macro_instrs.get("BINARY_OP_INPLACE_ADD_UNICODE"):
            mac.family = self.families.get("BINARY_OP")

    def check_families(self) -> None:
        """Check each family:

        - Must have at least 2 members (including head)
        - Head and all members must be known instructions
        - Head and all members must have the same cache, input and output effects
        """
        for family in self.families.values():
            if family.name not in self.macro_instrs and family.name not in self.instrs:
                self.error(
                    f"Family {family.name!r} has unknown instruction {family.name!r}",
                    family,
                )
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
            expected_effects = self.effect_counts(family.name)
            for member in members:
                member_effects = self.effect_counts(member)
                if member_effects != expected_effects:
                    self.error(
                        f"Family {family.name!r} has inconsistent "
                        f"(cache, input, output) effects:\n"
                        f"  {family.name} = {expected_effects}; "
                        f"{member} = {member_effects}",
                        family,
                    )

    def effect_counts(self, name: str) -> tuple[int, int, int]:
        if mac := self.macro_instrs.get(name):
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
            self.macro_instrs[name] = mac = self.analyze_macro(macro)
            self.check_macro_consistency(mac)
        for name, pseudo in self.pseudos.items():
            self.pseudo_instrs[name] = self.analyze_pseudo(pseudo)

    # TODO: Merge with similar code in stacking.py, write_components()
    def check_macro_consistency(self, mac: MacroInstruction) -> None:
        def get_var_names(instr: Instruction) -> dict[str, StackEffect]:
            vars: dict[str, StackEffect] = {}
            for eff in instr.input_effects + instr.output_effects:
                if eff.name == UNUSED:
                    continue
                if eff.name in vars:
                    if vars[eff.name] != eff:
                        self.error(
                            f"Instruction {instr.name!r} has "
                            f"inconsistent type/cond/size for variable "
                            f"{eff.name!r}: {vars[eff.name]} vs {eff}",
                            instr.inst,
                        )
                else:
                    vars[eff.name] = eff
            return vars

        all_vars: dict[str, StackEffect] = {}
        # print("Checking", mac.name)
        prevop: Instruction | None = None
        for part in mac.parts:
            if not isinstance(part, Component):
                continue
            vars = get_var_names(part.instr)
            # print("    //", part.instr.name, "//", vars)
            for name, eff in vars.items():
                if name in all_vars:
                    if all_vars[name] != eff:
                        self.error(
                            f"Macro {mac.name!r} has "
                            f"inconsistent type/cond/size for variable "
                            f"{name!r}: "
                            f"{all_vars[name]} vs {eff} in {part.instr.name!r}",
                            mac.macro,
                        )
                else:
                    all_vars[name] = eff
            if prevop is not None:
                pushes = list(prevop.output_effects)
                pops = list(reversed(part.instr.input_effects))
                copies: list[tuple[StackEffect, StackEffect]] = []
                while pushes and pops and pushes[-1] == pops[0]:
                    src, dst = pushes.pop(), pops.pop(0)
                    if src.name == dst.name or dst.name == UNUSED:
                        continue
                    copies.append((src, dst))
                reads = set(copy[0].name for copy in copies)
                writes = set(copy[1].name for copy in copies)
                if reads & writes:
                    self.error(
                        f"Macro {mac.name!r} has conflicting copies "
                        f"(source of one copy is destination of another): "
                        f"{reads & writes}",
                        mac.macro,
                    )
            prevop = part.instr

    def analyze_macro(self, macro: parsing.Macro) -> MacroInstruction:
        components = self.check_macro_components(macro)
        parts: MacroParts = []
        flags = InstructionFlags.newEmpty()
        offset = 0
        for component in components:
            match component:
                case parsing.CacheEffect() as ceffect:
                    parts.append(ceffect)
                    offset += ceffect.size
                case Instruction() as instr:
                    part, offset = self.analyze_instruction(instr, offset)
                    parts.append(part)
                    if instr.name != "_SAVE_RETURN_OFFSET":
                        # _SAVE_RETURN_OFFSET's oparg does not transfer
                        flags.add(instr.instr_flags)
                case _:
                    assert_never(component)
        format = "IB" if flags.HAS_ARG_FLAG else "IX"
        if offset:
            format += "C" + "0" * (offset - 1)
        return MacroInstruction(macro.name, format, flags, macro, parts, offset)

    def analyze_pseudo(self, pseudo: parsing.Pseudo) -> PseudoInstruction:
        targets: list[Instruction | MacroInstruction] = []
        for target_name in pseudo.targets:
            if target_name in self.instrs:
                targets.append(self.instrs[target_name])
            else:
                targets.append(self.macro_instrs[target_name])
        assert targets
        ignored_flags = {"HAS_EVAL_BREAK_FLAG", "HAS_DEOPT_FLAG", "HAS_ERROR_FLAG"}
        assert len({t.instr_flags.bitmap(ignore=ignored_flags) for t in targets}) == 1
        return PseudoInstruction(pseudo.name, targets, targets[0].instr_flags)

    def analyze_instruction(
        self, instr: Instruction, offset: int
    ) -> tuple[Component, int]:
        active_effects: list[ActiveCacheEffect] = []
        for ceffect in instr.cache_effects:
            if ceffect.name != UNUSED:
                active_effects.append(ActiveCacheEffect(ceffect, offset))
            offset += ceffect.size
        return (
            Component(instr, active_effects),
            offset,
        )

    def check_macro_components(
        self, macro: parsing.Macro
    ) -> list[InstructionOrCacheEffect]:
        components: list[InstructionOrCacheEffect] = []
        for uop in macro.uops:
            match uop:
                case parsing.OpName(name):
                    if name not in self.instrs:
                        self.error(f"Unknown instruction {name!r}", macro)
                    else:
                        components.append(self.instrs[name])
                case parsing.CacheEffect():
                    components.append(uop)
                case _:
                    assert_never(uop)
        return components

    def report_non_viable_uops(self, jsonfile: str) -> None:
        print("The following ops are not viable uops:")
        skips = {
            "CACHE",
            "RESERVED",
            "INTERPRETER_EXIT",
            "JUMP_BACKWARD",
            "LOAD_FAST_LOAD_FAST",
            "LOAD_CONST_LOAD_FAST",
            "STORE_FAST_STORE_FAST",
            "POP_JUMP_IF_TRUE",
            "POP_JUMP_IF_FALSE",
            "_ITER_JUMP_LIST",
            "_ITER_JUMP_TUPLE",
            "_ITER_JUMP_RANGE",
        }
        try:
            # Secret feature: if bmraw.json exists, print and sort by execution count
            counts = load_execution_counts(jsonfile)
        except FileNotFoundError as err:
            counts = {}
        non_viable = [
            instr
            for instr in self.instrs.values()
            if instr.name not in skips
            and not instr.name.startswith("INSTRUMENTED_")
            and not instr.is_viable_uop()
        ]
        non_viable.sort(key=lambda instr: (-counts.get(instr.name, 0), instr.name))
        for instr in non_viable:
            if instr.name in counts:
                scount = f"{counts[instr.name]:,}"
            else:
                scount = ""
            print(f"    {scount:>15} {instr.name:<35}", end="")
            if instr.name in self.families:
                print("      (unspecialized)", end="")
            elif instr.family is not None:
                print(f" (specialization of {instr.family.name})", end="")
            print()


def load_execution_counts(jsonfile: str) -> dict[str, int]:
    import json

    with open(jsonfile) as f:
        jsondata = json.load(f)

    # Look for keys like "opcode[LOAD_FAST].execution_count"
    prefix = "opcode["
    suffix = "].execution_count"
    res: dict[str, int] = {}
    for key, value in jsondata.items():
        if key.startswith(prefix) and key.endswith(suffix):
            res[key[len(prefix) : -len(suffix)]] = value
    return res
