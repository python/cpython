
from dataclasses import dataclass
import lexer
import parser
from _typing_backports import assert_never

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

    def dump(self, indent):
        print(indent,
              "escapes:", self.escapes,
              ", infallible:", self.infallible,
              ", deopts:", self.deopts,
              ", oparg:", self.oparg,
              ", jumps:", self.jumps,
              ", eval_breaker:", self.ends_with_eval_breaker,
              ", needs_this:", self.needs_this,
              ", always_exits:", self.always_exits,
              ", stores_sp:", self.stores_sp,
              sep=""
        )


SKIP_PROPERTIES = Properties(
    False,
    True,
    False,
    False,
    False,
    False,
    False,
    False,
    False,
)

@dataclass
class Skip:
   "Unused cache entry"
   size: int

   @property
   def name(self):
       return f"unused/{self.size}"

   @property
   def properties(self):
       return SKIP_PROPERTIES

@dataclass
class StackItem:
    name: str
    type: str | None
    condition: str | None
    size: str
    peek: bool = False

    def __str__(self):
        cond = " if (" + self.condition + ")" if self.condition else ""
        size = "[" + self.size + "]" if self.size != "1" else ""
        type = "" if self.type is None else self.type + " "
        return f"{type}{self.name}{size}{cond} {self.peek}"

    def is_array(self):
        return self.type == "PyObject **"

@dataclass
class StackEffect:
    inputs: list[StackItem]
    outputs: list[StackItem]

    def __str__(self):
        return f"({', '.join([str(i) for i in self.inputs])} -- {', '.join([str(i) for i in self.outputs])})"

@dataclass
class CacheEntry:
    name: str
    size: int

    def __str__(self):
        return f"{self.name}/{self.size}"

@dataclass
class Uop:
    name: str
    context: parser.Context
    annotations: list[str]
    stack: StackEffect
    caches: list[CacheEntry]
    body: list[lexer.Token]
    properties: Properties
    _size: int = -1

    def dump(self, indent):
        print(indent, self.name, ", ".join(self.annotations) if self.annotations else "")
        print(indent, self.stack, ", ".join([str(c) for c in self.caches]))
        self.properties.dump("    " + indent)

    @property
    def size(self):
        if self._size < 0:
            self._size = sum(c.size for c in self.caches)
        return self._size

Part = Uop | Skip

@dataclass
class Instruction:
    name: str
    uops: list[Part]
    _properties : Properties | None
    is_target: bool = False
    family: "Family" = None

    @property
    def properties(self) -> Properties:
        if self._properties is None:
            self._properties = self._compute_properties()
        return self._properties

    def _compute_properties(self) -> Properties:
        return Properties(
            any(part.properties.escapes for part in self.uops),
            all(part.properties.infallible for part in self.uops),
            any(part.properties.deopts for part in self.uops),
            any(part.properties.oparg for part in self.uops),
            any(part.properties.jumps for part in self.uops),
            any(part.properties.ends_with_eval_breaker for part in self.uops),
            any(part.properties.needs_this for part in self.uops),
            any(part.properties.always_exits for part in self.uops),
            any(part.properties.stores_sp for part in self.uops),
        )

    def dump(self, indent):
        print(indent, self.name, "=", ", ".join([op.name for op in self.uops]))
        self.properties.dump("    " + indent)

    @property
    def size(self):
        return 1 + sum(uop.size for uop in self.uops)

@dataclass
class PseudoInstruction:
    name: str
    targets: list[Instruction]

    def dump(self, indent):
        print(indent, self.name, "->", " or ".join([t.name for t in self.targets]))


@dataclass
class Family:
    name: str
    members: list[Instruction]

    def dump(self, indent):
        print(indent, self.name, "= ", ", ".join([m.name for m in self.members]))


@dataclass
class Analysis:
    instructions: dict[str, Instruction]
    uops: dict[str, Uop]
    families: dict[str, Family]
    pseudos: dict[str, PseudoInstruction]


def analysis_error(message: str, tkn: lexer.Token) -> SyntaxError:
    #To do -- support file and line output
    # Construct a SyntaxError instance from message and token
    return lexer.make_syntax_error(
        message, "", tkn.line, tkn.column, ""
    )

def override_error(name, context, prev_context, token) -> SyntaxError:
    return analysis_error(
        f"Duplicate definition of '{name}' @ {context} "
        f"previous definition @ {prev_context}",
        token
    )

def convert_stack_item(item: parser.StackEffect) -> StackItem:
    return StackItem(item.name, item.type,
                     item.cond, (item.size if item.size else "1"))


def analyze_stack(op: parser.InstDef) -> StackEffect:
    inputs: list[parser.StackItem] = [convert_stack_item(i) for i in op.inputs if isinstance(i, parser.StackEffect)]
    outputs: list[parser.StackItem] = [convert_stack_item(i) for i in op.outputs]
    for (input, output) in zip(inputs, outputs):
        if input.name == output.name:
            input.peek = output.peek = True
    return StackEffect(inputs, outputs)


def analyze_caches(op: parser.InstDef) -> list[CacheEntry]:
    caches: list[parser.CacheEffect] = [i for i in op.inputs if isinstance(i, parser.CacheEffect)]
    return [ CacheEntry(i.name, int(i.size)) for i in caches ]


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

EXITS = set([
    "DISPATCH",
    "GO_TO_INSTRUCTION",
    "Py_UNREACHABLE",
    "DISPATCH_INLINED",
    "DISPATCH_GOTO",
])

def eval_breaker_at_end(op: parser.InstDef):
    return op.tokens[-5].text == "CHECK_EVAL_BREAKER"

def always_exits(op: parser.InstDef):
    depth = 0
    for tkn in op.tokens:
        if tkn.kind == "LBRACE":
            depth += 1
        elif tkn.kind == "RBRACE":
            depth -= 1
        elif tkn.kind == "GOTO" or tkn.kind == "RETURN":
            if depth <= 1:
                return True
        elif tkn.kind == "KEYWORD" or tkn.kind == "IDENTIFIER":
            if depth <= 1 and tkn.text in EXITS:
                return True
    return False

def compute_properties(op: parser.InstDef) -> Properties:
    return Properties(
        makes_escaping_api_call(op),
        is_infallible(op),
        variable_used(op, "DEOPT_IF"),
        variable_used(op, "oparg"),
        variable_used(op, "JUMPBY"),
        eval_breaker_at_end(op),
        variable_used(op, "this_instr"),
        always_exits(op),
        variable_used(op, "STORE_SP"),
    )

def make_uop(name, op: parser.InstDef) -> Uop:
    return Uop(
        name,
        op.context,
        op.annotations,
        analyze_stack(op),
        analyze_caches(op),
        op.block.tokens,
        compute_properties(op),
    )

def add_op(op: parser.InstDef, uops: dict[str, Uop]) -> None:
    assert op.kind == "op"
    if op.name in uops:
        if "override" not in op.annotations:
            raise override_error(op.name, op.context, uops[op.name].context,op.tokens[0])
    uops[op.name] = make_uop(op.name, op)

def add_instruction(name: str, parts: list[Part], instructions: dict[str, Instruction]):
    instructions[name] = Instruction(name, parts, None)

def desugar_inst(inst: parser.InstDef, instructions: dict[str, Instruction], uops: dict[str, Uop]):
    assert inst.kind == "inst"
    name = inst.name
    uop = make_uop("_" + inst.name, inst)
    uops[inst.name] = uop
    add_instruction(name, [uop], instructions)

def add_macro(macro: parser.Macro, instructions: dict[str, Instruction], uops: dict[str, Uop]):
    parts = []
    for part in macro.uops:
        match part:
            case parser.OpName():
                if part.name not in uops:
                    analysis_error(f"No Uop named {part.name}", macro.context, macro.tokens[0])
                parts.append(uops[part.name])
            case parser.CacheEffect():
                parts.append(Skip(part.size))
            case _:
                assert_never(part)
    assert(parts)
    add_instruction(macro.name, parts, instructions)

def add_family(family: parser.Family, instructions: dict[str, Instruction], families: dict[str, Family]):
    family = Family(
        family.name,
        [ instructions[member] for member in family.members ]
    )
    for member in family.members:
        member.family = family
    #The head of the family is an implicit jump target for DEOPTs
    instructions[family.name].is_target = True
    families[family.name] = family

def add_pseudo(pseudo: parser.Pseudo, instructions: dict[str, Instruction], pseudos: dict[str, PseudoInstruction]):
    pseudos[pseudo.name] = PseudoInstruction(
        pseudo.name,
        [ instructions[target] for target in pseudo.targets ]
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
                assert_never(node)
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
    #Hack
    instructions["BINARY_OP_INPLACE_ADD_UNICODE"].family = families["BINARY_OP"]
    return Analysis(instructions, uops, families, pseudos)

def analyze_file(filename: str) -> Analysis:
    return analyze_forest(parser.parse_file(filename))

def dump_analysis(analysis: Analysis) -> None:
    print("Uops:")
    for i in analysis.uops.values():
        i.dump("    ")
    print("Instructions:")
    for i in analysis.instructions.values():
        i.dump("    ")
    print("Families:")
    for i in analysis.families.values():
        i.dump("    ")
    print("Pseudos:")
    for i in analysis.pseudos.values():
        i.dump("    ")


if __name__ == "__main__":
    import sys
    if len(sys.argv) < 2:
        print("No input")
    else:
        filename = sys.argv[1]
        dump_analysis(analyze_file(filename))













