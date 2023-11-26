from dataclasses import dataclass
import lexer
import parser
from typing import Optional


@dataclass
class Properties:
    escapes: bool
    infallible: bool
    deopts: bool
    oparg: bool
    jumps: bool
    ends_with_eval_breaker: bool
    needs_this: bool
    always_exits: bool
    stores_sp: bool

    def dump(self, indent: str) -> None:
        print(indent, end="")
        text = ", ".join([f"{key}: {value}" for (key, value) in self.__dict__.items()])
        print(indent, text, sep="")

    @staticmethod
    def from_list(properties: list["Properties"]) -> "Properties":
        return Properties(
            escapes=any(p.escapes for p in properties),
            infallible=all(p.infallible for p in properties),
            deopts=any(p.deopts for p in properties),
            oparg=any(p.oparg for p in properties),
            jumps=any(p.jumps for p in properties),
            ends_with_eval_breaker=any(p.ends_with_eval_breaker for p in properties),
            needs_this=any(p.needs_this for p in properties),
            always_exits=any(p.always_exits for p in properties),
            stores_sp=any(p.stores_sp for p in properties),
        )


SKIP_PROPERTIES = Properties(
    escapes=False,
    infallible=True,
    deopts=False,
    oparg=False,
    jumps=False,
    ends_with_eval_breaker=False,
    needs_this=False,
    always_exits=False,
    stores_sp=False,
)


@dataclass
class Skip:
    "Unused cache entry"
    size: int

    @property
    def name(self) -> str:
        return f"unused/{self.size}"

    @property
    def properties(self) -> Properties:
        return SKIP_PROPERTIES


@dataclass
class StackItem:
    name: str
    type: str | None
    condition: str | None
    size: str
    peek: bool = False

    def __str__(self) -> str:
        cond = f" if ({self.condition})" if self.condition else ""
        size = f"[{self.size}]" if self.size != "1" else ""
        type = "" if self.type is None else f"{self.type} "
        return f"{type}{self.name}{size}{cond} {self.peek}"

    def is_array(self) -> bool:
        return self.type == "PyObject **"


@dataclass
class StackEffect:
    inputs: list[StackItem]
    outputs: list[StackItem]

    def __str__(self) -> str:
        return f"({', '.join([str(i) for i in self.inputs])} -- {', '.join([str(i) for i in self.outputs])})"


@dataclass
class CacheEntry:
    name: str
    size: int

    def __str__(self) -> str:
        return f"{self.name}/{self.size}"


@dataclass
class Uop:
    name: str
    context: parser.Context | None
    annotations: list[str]
    stack: StackEffect
    caches: list[CacheEntry]
    body: list[lexer.Token]
    properties: Properties
    _size: int = -1

    def dump(self, indent: str) -> None:
        print(
            indent, self.name, ", ".join(self.annotations) if self.annotations else ""
        )
        print(indent, self.stack, ", ".join([str(c) for c in self.caches]))
        self.properties.dump("    " + indent)

    @property
    def size(self) -> int:
        if self._size < 0:
            self._size = sum(c.size for c in self.caches)
        return self._size


Part = Uop | Skip


@dataclass
class Instruction:
    name: str
    parts: list[Part]
    _properties: Properties | None
    is_target: bool = False
    family: Optional["Family"] = None

    @property
    def properties(self) -> Properties:
        if self._properties is None:
            self._properties = self._compute_properties()
        return self._properties

    def _compute_properties(self) -> Properties:
        return Properties.from_list([part.properties for part in self.parts])

    def dump(self, indent: str) -> None:
        print(indent, self.name, "=", ", ".join([part.name for part in self.parts]))
        self.properties.dump("    " + indent)

    @property
    def size(self) -> int:
        return 1 + sum(part.size for part in self.parts)


@dataclass
class PseudoInstruction:
    name: str
    targets: list[Instruction]
    flags: list[str]

    def dump(self, indent: str) -> None:
        print(indent, self.name, "->", " or ".join([t.name for t in self.targets]))


@dataclass
class Family:
    name: str
    size: str
    members: list[Instruction]

    def dump(self, indent: str) -> None:
        print(indent, self.name, "= ", ", ".join([m.name for m in self.members]))


@dataclass
class Analysis:
    instructions: dict[str, Instruction]
    uops: dict[str, Uop]
    families: dict[str, Family]
    pseudos: dict[str, PseudoInstruction]


def analysis_error(message: str, tkn: lexer.Token) -> SyntaxError:
    # To do -- support file and line output
    # Construct a SyntaxError instance from message and token
    return lexer.make_syntax_error(message, "", tkn.line, tkn.column, "")


def override_error(
    name: str,
    context: parser.Context | None,
    prev_context: parser.Context | None,
    token: lexer.Token,
) -> SyntaxError:
    return analysis_error(
        f"Duplicate definition of '{name}' @ {context} "
        f"previous definition @ {prev_context}",
        token,
    )


def convert_stack_item(item: parser.StackEffect) -> StackItem:
    return StackItem(item.name, item.type, item.cond, (item.size or "1"))


def analyze_stack(op: parser.InstDef) -> StackEffect:
    inputs: list[StackItem] = [
        convert_stack_item(i) for i in op.inputs if isinstance(i, parser.StackEffect)
    ]
    outputs: list[StackItem] = [convert_stack_item(i) for i in op.outputs]
    for input, output in zip(inputs, outputs):
        if input.name == output.name:
            input.peek = output.peek = True
    return StackEffect(inputs, outputs)


def analyze_caches(op: parser.InstDef) -> list[CacheEntry]:
    caches: list[parser.CacheEffect] = [
        i for i in op.inputs if isinstance(i, parser.CacheEffect)
    ]
    return [CacheEntry(i.name, int(i.size)) for i in caches]


def variable_used(node: parser.InstDef, name: str) -> bool:
    """Determine whether a variable with a given name is used in a node."""
    return any(
        token.kind == "IDENTIFIER" and token.text == name for token in node.tokens
    )


def is_infallible(op: parser.InstDef) -> bool:
    return not (
        variable_used(op, "ERROR_IF")
        or variable_used(op, "error")
        or variable_used(op, "pop_1_error")
        or variable_used(op, "exception_unwind")
        or variable_used(op, "resume_with_error")
    )


from flags import makes_escaping_api_call

EXITS = {
    "DISPATCH",
    "GO_TO_INSTRUCTION",
    "Py_UNREACHABLE",
    "DISPATCH_INLINED",
    "DISPATCH_GOTO",
}


def eval_breaker_at_end(op: parser.InstDef) -> bool:
    return op.tokens[-5].text == "CHECK_EVAL_BREAKER"


def always_exits(op: parser.InstDef) -> bool:
    depth = 0
    tkn_iter = iter(op.tokens)
    for tkn in tkn_iter:
        if tkn.kind == "LBRACE":
            depth += 1
        elif tkn.kind == "RBRACE":
            depth -= 1
        elif depth > 1:
            continue
        elif tkn.kind == "GOTO" or tkn.kind == "RETURN":
            return True
        elif tkn.kind == "KEYWORD":
            if tkn.text in EXITS:
                return True
        elif tkn.kind == "IDENTIFIER":
            if tkn.text in EXITS:
                return True
            if tkn.text == "DEOPT_IF" or tkn.text == "ERROR_IF":
                next(tkn_iter)  # '('
                t = next(tkn_iter)
                if t.text == "true":
                    return True
    return False


def compute_properties(op: parser.InstDef) -> Properties:
    return Properties(
        escapes=makes_escaping_api_call(op),
        infallible=is_infallible(op),
        deopts=variable_used(op, "DEOPT_IF"),
        oparg=variable_used(op, "oparg"),
        jumps=variable_used(op, "JUMPBY"),
        ends_with_eval_breaker=eval_breaker_at_end(op),
        needs_this=variable_used(op, "this_instr"),
        always_exits=always_exits(op),
        stores_sp=variable_used(op, "STORE_SP"),
    )


def make_uop(name: str, op: parser.InstDef) -> Uop:
    return Uop(
        name=name,
        context=op.context,
        annotations=op.annotations,
        stack=analyze_stack(op),
        caches=analyze_caches(op),
        body=op.block.tokens,
        properties=compute_properties(op),
    )


def add_op(op: parser.InstDef, uops: dict[str, Uop]) -> None:
    assert op.kind == "op"
    if op.name in uops:
        if "override" not in op.annotations:
            raise override_error(
                op.name, op.context, uops[op.name].context, op.tokens[0]
            )
    uops[op.name] = make_uop(op.name, op)


def add_instruction(
    name: str, parts: list[Part], instructions: dict[str, Instruction]
) -> None:
    instructions[name] = Instruction(name, parts, None)


def desugar_inst(
    inst: parser.InstDef, instructions: dict[str, Instruction], uops: dict[str, Uop]
) -> None:
    assert inst.kind == "inst"
    name = inst.name
    uop = make_uop("_" + inst.name, inst)
    uops[inst.name] = uop
    add_instruction(name, [uop], instructions)


def add_macro(
    macro: parser.Macro, instructions: dict[str, Instruction], uops: dict[str, Uop]
) -> None:
    parts: list[Uop | Skip] = []
    for part in macro.uops:
        match part:
            case parser.OpName():
                if part.name not in uops:
                    analysis_error(f"No Uop named {part.name}", macro.tokens[0])
                parts.append(uops[part.name])
            case parser.CacheEffect():
                parts.append(Skip(part.size))
            case _:
                assert False
    assert parts
    add_instruction(macro.name, parts, instructions)


def add_family(
    pfamily: parser.Family,
    instructions: dict[str, Instruction],
    families: dict[str, Family],
) -> None:
    family = Family(
        pfamily.name,
        pfamily.size,
        [instructions[member_name] for member_name in pfamily.members],
    )
    for member in family.members:
        member.family = family
    # The head of the family is an implicit jump target for DEOPTs
    instructions[family.name].is_target = True
    families[family.name] = family


def add_pseudo(
    pseudo: parser.Pseudo,
    instructions: dict[str, Instruction],
    pseudos: dict[str, PseudoInstruction],
) -> None:
    pseudos[pseudo.name] = PseudoInstruction(
        pseudo.name,
        [instructions[target] for target in pseudo.targets],
        pseudo.flags,
    )


def analyze_forest(forest: list[parser.AstNode]) -> Analysis:
    instructions: dict[str, Instruction] = {}
    uops: dict[str, Uop] = {}
    families: dict[str, Family] = {}
    pseudos: dict[str, PseudoInstruction] = {}
    for node in forest:
        match node:
            case parser.InstDef(name):
                if node.kind == "inst":
                    desugar_inst(node, instructions, uops)
                else:
                    assert node.kind == "op"
                    add_op(node, uops)
            case parser.Macro():
                pass
            case parser.Family():
                pass
            case parser.Pseudo():
                pass
            case _:
                assert False
    for node in forest:
        if isinstance(node, parser.Macro):
            add_macro(node, instructions, uops)
    for node in forest:
        match node:
            case parser.Family():
                add_family(node, instructions, families)
            case parser.Pseudo():
                add_pseudo(node, instructions, pseudos)
            case _:
                pass
    for uop in uops.values():
        tkn_iter = iter(uop.body)
        for tkn in tkn_iter:
            if tkn.kind == "IDENTIFIER" and tkn.text == "GO_TO_INSTRUCTION":
                if next(tkn_iter).kind != "LPAREN":
                    continue
                target = next(tkn_iter)
                if target.kind != "IDENTIFIER":
                    continue
                if target.text in instructions:
                    instructions[target.text].is_target = True
    # Hack
    instructions["BINARY_OP_INPLACE_ADD_UNICODE"].family = families["BINARY_OP"]
    return Analysis(instructions, uops, families, pseudos)


def analyze_files(filenames: list[str]) -> Analysis:
    return analyze_forest(parser.parse_files(filenames))


def dump_analysis(analysis: Analysis) -> None:
    print("Uops:")
    for u in analysis.uops.values():
        u.dump("    ")
    print("Instructions:")
    for i in analysis.instructions.values():
        i.dump("    ")
    print("Families:")
    for f in analysis.families.values():
        f.dump("    ")
    print("Pseudos:")
    for p in analysis.pseudos.values():
        p.dump("    ")


if __name__ == "__main__":
    import sys

    if len(sys.argv) < 2:
        print("No input")
    else:
        filenames = sys.argv[1:]
        dump_analysis(analyze_files(filenames))
