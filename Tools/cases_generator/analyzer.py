from dataclasses import dataclass, field
import lexer
import parser
import re
from typing import Optional


@dataclass
class Properties:
    escapes: bool
    error_with_pop: bool
    error_without_pop: bool
    deopts: bool
    oparg: bool
    jumps: bool
    eval_breaker: bool
    ends_with_eval_breaker: bool
    needs_this: bool
    always_exits: bool
    stores_sp: bool
    uses_co_consts: bool
    uses_co_names: bool
    uses_locals: bool
    has_free: bool
    side_exit: bool
    pure: bool
    tier: int | None = None
    oparg_and_1: bool = False
    const_oparg: int = -1

    def dump(self, indent: str) -> None:
        print(indent, end="")
        text = ", ".join([f"{key}: {value}" for (key, value) in self.__dict__.items()])
        print(indent, text, sep="")

    @staticmethod
    def from_list(properties: list["Properties"]) -> "Properties":
        return Properties(
            escapes=any(p.escapes for p in properties),
            error_with_pop=any(p.error_with_pop for p in properties),
            error_without_pop=any(p.error_without_pop for p in properties),
            deopts=any(p.deopts for p in properties),
            oparg=any(p.oparg for p in properties),
            jumps=any(p.jumps for p in properties),
            eval_breaker=any(p.eval_breaker for p in properties),
            ends_with_eval_breaker=any(p.ends_with_eval_breaker for p in properties),
            needs_this=any(p.needs_this for p in properties),
            always_exits=any(p.always_exits for p in properties),
            stores_sp=any(p.stores_sp for p in properties),
            uses_co_consts=any(p.uses_co_consts for p in properties),
            uses_co_names=any(p.uses_co_names for p in properties),
            uses_locals=any(p.uses_locals for p in properties),
            has_free=any(p.has_free for p in properties),
            side_exit=any(p.side_exit for p in properties),
            pure=all(p.pure for p in properties),
        )

    @property
    def infallible(self) -> bool:
        return not self.error_with_pop and not self.error_without_pop



SKIP_PROPERTIES = Properties(
    escapes=False,
    error_with_pop=False,
    error_without_pop=False,
    deopts=False,
    oparg=False,
    jumps=False,
    eval_breaker=False,
    ends_with_eval_breaker=False,
    needs_this=False,
    always_exits=False,
    stores_sp=False,
    uses_co_consts=False,
    uses_co_names=False,
    uses_locals=False,
    has_free=False,
    side_exit=False,
    pure=False,
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
    implicitly_created: bool = False
    replicated = 0
    replicates : "Uop | None" = None

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

    def why_not_viable(self) -> str | None:
        if self.name == "_SAVE_RETURN_OFFSET":
            return None  # Adjusts next_instr, but only in tier 1 code
        if "INSTRUMENTED" in self.name:
            return "is instrumented"
        if "replaced" in self.annotations:
            return "is replaced"
        if self.name in ("INTERPRETER_EXIT", "JUMP_BACKWARD"):
            return "has tier 1 control flow"
        if self.properties.needs_this:
            return "uses the 'this_instr' variable"
        if len([c for c in self.caches if c.name != "unused"]) > 1:
            return "has unused cache entries"
        if self.properties.error_with_pop and self.properties.error_without_pop:
            return "has both popping and not-popping errors"
        if self.properties.eval_breaker:
            if self.properties.error_with_pop or self.properties.error_without_pop:
                return "has error handling and eval-breaker check"
            if self.properties.side_exit:
                return "exits and eval-breaker check"
            if self.properties.deopts:
                return "deopts and eval-breaker check"
        return None

    def is_viable(self) -> bool:
        return self.why_not_viable() is None

    def is_super(self) -> bool:
        for tkn in self.body:
            if tkn.kind == "IDENTIFIER" and tkn.text == "oparg1":
                return True
        return False


Part = Uop | Skip


@dataclass
class Instruction:
    name: str
    parts: list[Part]
    _properties: Properties | None
    is_target: bool = False
    family: Optional["Family"] = None
    opcode: int = -1

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

    def is_super(self) -> bool:
        if len(self.parts) != 1:
            return False
        uop = self.parts[0]
        if isinstance(uop, Uop):
            return uop.is_super()
        else:
            return False


@dataclass
class PseudoInstruction:
    name: str
    targets: list[Instruction]
    flags: list[str]
    opcode: int = -1

    def dump(self, indent: str) -> None:
        print(indent, self.name, "->", " or ".join([t.name for t in self.targets]))

    @property
    def properties(self) -> Properties:
        return Properties.from_list([i.properties for i in self.targets])


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
    opmap: dict[str, int]
    have_arg: int
    min_instrumented: int


def analysis_error(message: str, tkn: lexer.Token) -> SyntaxError:
    # To do -- support file and line output
    # Construct a SyntaxError instance from message and token
    return lexer.make_syntax_error(message, tkn.filename, tkn.line, tkn.column, "")


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


def convert_stack_item(item: parser.StackEffect, replace_op_arg_1: str | None) -> StackItem:
    cond = item.cond
    if replace_op_arg_1 and OPARG_AND_1.match(item.cond):
        cond = replace_op_arg_1
    return StackItem(
        item.name, item.type, cond, (item.size or "1")
    )

def analyze_stack(op: parser.InstDef, replace_op_arg_1: str | None = None) -> StackEffect:
    inputs: list[StackItem] = [
        convert_stack_item(i, replace_op_arg_1) for i in op.inputs if isinstance(i, parser.StackEffect)
    ]
    outputs: list[StackItem] = [convert_stack_item(i, replace_op_arg_1) for i in op.outputs]
    # Mark variables with matching names at the base of the stack as "peek"
    modified = False
    for input, output in zip(inputs, outputs):
        if input.name == output.name and not modified:
            input.peek = output.peek = True
        else:
            modified = True
    return StackEffect(inputs, outputs)


def analyze_caches(inputs: list[parser.InputEffect]) -> list[CacheEntry]:
    caches: list[parser.CacheEffect] = [
        i for i in inputs if isinstance(i, parser.CacheEffect)
    ]
    for cache in caches:
        if cache.name == "unused":
            raise analysis_error(
                "Unused cache entry in op. Move to enclosing macro.", cache.tokens[0]
            )
    return [CacheEntry(i.name, int(i.size)) for i in caches]


def variable_used(node: parser.InstDef, name: str) -> bool:
    """Determine whether a variable with a given name is used in a node."""
    return any(
        token.kind == "IDENTIFIER" and token.text == name for token in node.tokens
    )

def tier_variable(node: parser.InstDef) -> int | None:
    """Determine whether a tier variable is used in a node."""
    for token in node.tokens:
        if token.kind == "ANNOTATION":
            if token.text == "specializing":
                return 1
            if re.fullmatch(r"tier\d", token.text):
                return int(token.text[-1])
    return None

def has_error_with_pop(op: parser.InstDef) -> bool:
    return (
        variable_used(op, "ERROR_IF")
        or variable_used(op, "pop_1_error")
        or variable_used(op, "exception_unwind")
        or variable_used(op, "resume_with_error")
    )

def has_error_without_pop(op: parser.InstDef) -> bool:
    return (
        variable_used(op, "ERROR_NO_POP")
        or variable_used(op, "pop_1_error")
        or variable_used(op, "exception_unwind")
        or variable_used(op, "resume_with_error")
    )


NON_ESCAPING_FUNCTIONS = (
    "Py_INCREF",
    "_PyManagedDictPointer_IsValues",
    "_PyObject_GetManagedDict",
    "_PyObject_ManagedDictPointer",
    "_PyObject_InlineValues",
    "_PyDictValues_AddToInsertionOrder",
    "Py_DECREF",
    "Py_XDECREF",
    "_Py_DECREF_SPECIALIZED",
    "DECREF_INPUTS_AND_REUSE_FLOAT",
    "PyUnicode_Append",
    "_PyLong_IsZero",
    "Py_SIZE",
    "Py_TYPE",
    "PyList_GET_ITEM",
    "PyList_SET_ITEM",
    "PyTuple_GET_ITEM",
    "PyList_GET_SIZE",
    "PyTuple_GET_SIZE",
    "Py_ARRAY_LENGTH",
    "Py_Unicode_GET_LENGTH",
    "PyUnicode_READ_CHAR",
    "_Py_SINGLETON",
    "PyUnicode_GET_LENGTH",
    "_PyLong_IsCompact",
    "_PyLong_IsNonNegativeCompact",
    "_PyLong_CompactValue",
    "_PyLong_DigitCount",
    "_Py_NewRef",
    "_Py_IsImmortal",
    "PyLong_FromLong",
    "_Py_STR",
    "_PyLong_Add",
    "_PyLong_Multiply",
    "_PyLong_Subtract",
    "Py_NewRef",
    "_PyList_ITEMS",
    "_PyTuple_ITEMS",
    "_PyList_AppendTakeRef",
    "_Py_atomic_load_uintptr_relaxed",
    "_PyFrame_GetCode",
    "_PyThreadState_HasStackSpace",
    "_PyUnicode_Equal",
    "_PyFrame_SetStackPointer",
    "_PyType_HasFeature",
    "PyUnicode_Concat",
    "_PyList_FromArraySteal",
    "_PyTuple_FromArraySteal",
    "PySlice_New",
    "_Py_LeaveRecursiveCallPy",
    "CALL_STAT_INC",
    "STAT_INC",
    "maybe_lltrace_resume_frame",
    "_PyUnicode_JoinArray",
    "_PyEval_FrameClearAndPop",
    "_PyFrame_StackPush",
    "PyCell_New",
    "PyFloat_AS_DOUBLE",
    "_PyFrame_PushUnchecked",
    "Py_FatalError",
)

ESCAPING_FUNCTIONS = (
    "import_name",
    "import_from",
)


def makes_escaping_api_call(instr: parser.InstDef) -> bool:
    if "CALL_INTRINSIC" in instr.name:
        return True
    if instr.name == "_BINARY_OP":
        return True
    tkns = iter(instr.tokens)
    for tkn in tkns:
        if tkn.kind != lexer.IDENTIFIER:
            continue
        try:
            next_tkn = next(tkns)
        except StopIteration:
            return False
        if next_tkn.kind != lexer.LPAREN:
            continue
        if tkn.text in ESCAPING_FUNCTIONS:
            return True
        if tkn.text == "tp_vectorcall":
            return True
        if not tkn.text.startswith("Py") and not tkn.text.startswith("_Py"):
            continue
        if tkn.text.endswith("Check"):
            continue
        if tkn.text.startswith("Py_Is"):
            continue
        if tkn.text.endswith("CheckExact"):
            continue
        if tkn.text in NON_ESCAPING_FUNCTIONS:
            continue
        return True
    return False


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


def stack_effect_only_peeks(instr: parser.InstDef) -> bool:
    stack_inputs = [s for s in instr.inputs if not isinstance(s, parser.CacheEffect)]
    if len(stack_inputs) != len(instr.outputs):
        return False
    if len(stack_inputs) == 0:
        return False
    if any(s.cond for s in stack_inputs) or any(s.cond for s in instr.outputs):
        return False
    return all(
        (s.name == other.name and s.type == other.type and s.size == other.size)
        for s, other in zip(stack_inputs, instr.outputs)
    )

OPARG_AND_1 = re.compile("\\(*oparg *& *1")

def effect_depends_on_oparg_1(op: parser.InstDef) -> bool:
    for effect in op.inputs:
        if isinstance(effect, parser.CacheEffect):
            continue
        if not effect.cond:
            continue
        if OPARG_AND_1.match(effect.cond):
            return True
    for effect in op.outputs:
        if not effect.cond:
            continue
        if OPARG_AND_1.match(effect.cond):
            return True
    return False

def compute_properties(op: parser.InstDef) -> Properties:
    has_free = (
        variable_used(op, "PyCell_New")
        or variable_used(op, "PyCell_GetRef")
        or variable_used(op, "PyCell_SetTakeRef")
        or variable_used(op, "PyCell_SwapTakeRef")
    )
    deopts_if = variable_used(op, "DEOPT_IF")
    exits_if = variable_used(op, "EXIT_IF")
    if deopts_if and exits_if:
        tkn = op.tokens[0]
        raise lexer.make_syntax_error(
            "Op cannot contain both EXIT_IF and DEOPT_IF",
            tkn.filename,
            tkn.line,
            tkn.column,
            op.name,
        )
    error_with_pop = has_error_with_pop(op)
    error_without_pop = has_error_without_pop(op)
    return Properties(
        escapes=makes_escaping_api_call(op),
        error_with_pop=error_with_pop,
        error_without_pop=error_without_pop,
        deopts=deopts_if,
        side_exit=exits_if,
        oparg=variable_used(op, "oparg"),
        jumps=variable_used(op, "JUMPBY"),
        eval_breaker=variable_used(op, "CHECK_EVAL_BREAKER"),
        ends_with_eval_breaker=eval_breaker_at_end(op),
        needs_this=variable_used(op, "this_instr"),
        always_exits=always_exits(op),
        stores_sp=variable_used(op, "SYNC_SP"),
        uses_co_consts=variable_used(op, "FRAME_CO_CONSTS"),
        uses_co_names=variable_used(op, "FRAME_CO_NAMES"),
        uses_locals=(variable_used(op, "GETLOCAL") or variable_used(op, "SETLOCAL"))
        and not has_free,
        has_free=has_free,
        pure="pure" in op.annotations,
        tier=tier_variable(op),
    )


def make_uop(name: str, op: parser.InstDef, inputs: list[parser.InputEffect], uops: dict[str, Uop]) -> Uop:
    result = Uop(
        name=name,
        context=op.context,
        annotations=op.annotations,
        stack=analyze_stack(op),
        caches=analyze_caches(inputs),
        body=op.block.tokens,
        properties=compute_properties(op),
    )
    if effect_depends_on_oparg_1(op) and "split" in op.annotations:
        result.properties.oparg_and_1 = True
        for bit in ("0", "1"):
            name_x = name + "_" + bit
            properties = compute_properties(op)
            if properties.oparg:
                # May not need oparg anymore
                properties.oparg = any(token.text == "oparg" for token in op.block.tokens)
            rep = Uop(
                name=name_x,
                context=op.context,
                annotations=op.annotations,
                stack=analyze_stack(op, bit),
                caches=analyze_caches(inputs),
                body=op.block.tokens,
                properties=properties,
            )
            rep.replicates = result
            uops[name_x] = rep
    for anno in op.annotations:
        if anno.startswith("replicate"):
            result.replicated = int(anno[10:-1])
            break
    else:
        return result
    for oparg in range(result.replicated):
        name_x = name + "_" + str(oparg)
        properties = compute_properties(op)
        properties.oparg = False
        properties.const_oparg = oparg
        rep = Uop(
            name=name_x,
            context=op.context,
            annotations=op.annotations,
            stack=analyze_stack(op),
            caches=analyze_caches(inputs),
            body=op.block.tokens,
            properties=properties,
        )
        rep.replicates = result
        uops[name_x] = rep

    return result


def add_op(op: parser.InstDef, uops: dict[str, Uop]) -> None:
    assert op.kind == "op"
    if op.name in uops:
        if "override" not in op.annotations:
            raise override_error(
                op.name, op.context, uops[op.name].context, op.tokens[0]
            )
    uops[op.name] = make_uop(op.name, op, op.inputs, uops)


def add_instruction(
    name: str, parts: list[Part], instructions: dict[str, Instruction]
) -> None:
    instructions[name] = Instruction(name, parts, None)


def desugar_inst(
    inst: parser.InstDef, instructions: dict[str, Instruction], uops: dict[str, Uop]
) -> None:
    assert inst.kind == "inst"
    name = inst.name
    op_inputs: list[parser.InputEffect] = []
    parts: list[Part] = []
    uop_index = -1
    # Move unused cache entries to the Instruction, removing them from the Uop.
    for input in inst.inputs:
        if isinstance(input, parser.CacheEffect) and input.name == "unused":
            parts.append(Skip(input.size))
        else:
            op_inputs.append(input)
            if uop_index < 0:
                uop_index = len(parts)
                # Place holder for the uop.
                parts.append(Skip(0))
    uop = make_uop("_" + inst.name, inst, op_inputs, uops)
    uop.implicitly_created = True
    uops[inst.name] = uop
    if uop_index < 0:
        parts.append(uop)
    else:
        parts[uop_index] = uop
    add_instruction(name, parts, instructions)


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


def assign_opcodes(
    instructions: dict[str, Instruction],
    families: dict[str, Family],
    pseudos: dict[str, PseudoInstruction],
) -> tuple[dict[str, int], int, int]:
    """Assigns opcodes, then returns the opmap,
    have_arg and min_instrumented values"""
    instmap: dict[str, int] = {}

    # 0 is reserved for cache entries. This helps debugging.
    instmap["CACHE"] = 0

    # 17 is reserved as it is the initial value for the specializing counter.
    # This helps catch cases where we attempt to execute a cache.
    instmap["RESERVED"] = 17

    # 149 is RESUME - it is hard coded as such in Tools/build/deepfreeze.py
    instmap["RESUME"] = 149

    # This is an historical oddity.
    instmap["BINARY_OP_INPLACE_ADD_UNICODE"] = 3

    instmap["INSTRUMENTED_LINE"] = 254

    instrumented = [name for name in instructions if name.startswith("INSTRUMENTED")]

    # Special case: this instruction is implemented in ceval.c
    # rather than bytecodes.c, so we need to add it explicitly
    # here (at least until we add something to bytecodes.c to
    # declare external instructions).
    instrumented.append("INSTRUMENTED_LINE")

    specialized: set[str] = set()
    no_arg: list[str] = []
    has_arg: list[str] = []

    for family in families.values():
        specialized.update(inst.name for inst in family.members)

    for inst in instructions.values():
        name = inst.name
        if name in specialized:
            continue
        if name in instrumented:
            continue
        if inst.properties.oparg:
            has_arg.append(name)
        else:
            no_arg.append(name)

    # Specialized ops appear in their own section
    # Instrumented opcodes are at the end of the valid range
    min_internal = 150
    min_instrumented = 254 - (len(instrumented) - 1)
    assert min_internal + len(specialized) < min_instrumented

    next_opcode = 1

    def add_instruction(name: str) -> None:
        nonlocal next_opcode
        if name in instmap:
            return  # Pre-defined name
        while next_opcode in instmap.values():
            next_opcode += 1
        instmap[name] = next_opcode
        next_opcode += 1

    for name in sorted(no_arg):
        add_instruction(name)
    for name in sorted(has_arg):
        add_instruction(name)
    # For compatibility
    next_opcode = min_internal
    for name in sorted(specialized):
        add_instruction(name)
    next_opcode = min_instrumented
    for name in instrumented:
        add_instruction(name)

    for name in instructions:
        instructions[name].opcode = instmap[name]

    for op, name in enumerate(sorted(pseudos), 256):
        instmap[name] = op
        pseudos[name].opcode = op

    return instmap, len(no_arg), min_instrumented


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
    # Special case BINARY_OP_INPLACE_ADD_UNICODE
    # BINARY_OP_INPLACE_ADD_UNICODE is not a normal family member,
    # as it is the wrong size, but we need it to maintain an
    # historical optimization.
    if "BINARY_OP_INPLACE_ADD_UNICODE" in instructions:
        inst = instructions["BINARY_OP_INPLACE_ADD_UNICODE"]
        inst.family = families["BINARY_OP"]
        families["BINARY_OP"].members.append(inst)
    opmap, first_arg, min_instrumented = assign_opcodes(instructions, families, pseudos)
    return Analysis(
        instructions, uops, families, pseudos, opmap, first_arg, min_instrumented
    )


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
