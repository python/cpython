from dataclasses import dataclass, field
import lexer
import parser
import re
from typing import Optional


TEMPLATE_SIZES = [
    (651, "_INIT_CALL_PY_EXACT_ARGS"),
    (411, "_CALL_ISINSTANCE"),
    (399, "_CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS"),
    (383, "_CALL_BUILTIN_FAST_WITH_KEYWORDS"),
    (383, "_CALL_BUILTIN_FAST"),
    (383, "_CALL_BUILTIN_CLASS"),
    (380, "_CALL_METHOD_DESCRIPTOR_O"),
    (379, "_CALL_METHOD_DESCRIPTOR_FAST"),
    (372, "_CALL_LEN"),
    (371, "_BINARY_SUBSCR_STR_INT"),
    (369, "_CALL_METHOD_DESCRIPTOR_NOARGS"),
    (358, "_LOAD_SUPER_ATTR_METHOD"),
    (358, "_COLD_EXIT"),
    (349, "_CALL_BUILTIN_O"),
    (327, "_CHECK_EG_MATCH"),
    (313, "_LOAD_GLOBAL"),
    (311, "_BUILD_SLICE"),
    (303, "_INIT_CALL_PY_EXACT_ARGS_4"),
    (294, "_INIT_CALL_PY_EXACT_ARGS_3"),
    (292, "_STORE_SLICE"),
    (287, "_GET_AITER"),
    (285, "_INIT_CALL_PY_EXACT_ARGS_2"),
    (283, "_GET_ANEXT"),
    (282, "_LOAD_SUPER_ATTR_ATTR"),
    (281, "_BUILD_CONST_KEY_MAP"),
    (278, "_LOAD_ATTR_WITH_HINT"),
    (277, "_COMPARE_OP_INT"),
    (276, "_INIT_CALL_PY_EXACT_ARGS_1"),
    (275, "_GET_AWAITABLE"),
    (258, "_LOAD_FROM_DICT_OR_GLOBALS"),
    (258, "_COMPARE_OP"),
    (257, "_INIT_CALL_PY_EXACT_ARGS_0"),
    (256, "_LOAD_ATTR"),
    (251, "_LIST_EXTEND"),
    (245, "_BUILD_MAP"),
    (244, "_CONTAINS_OP_SET"),
    (240, "_BINARY_SUBSCR_DICT"),
    (238, "_MATCH_CLASS"),
    (235, "_CONTAINS_OP_DICT"),
    (233, "_SETUP_ANNOTATIONS"),
    (231, "_BUILD_STRING"),
    (231, "_BINARY_OP_SUBTRACT_FLOAT"),
    (231, "_BINARY_OP_MULTIPLY_FLOAT"),
    (231, "_BINARY_OP_ADD_FLOAT"),
    (228, "_STORE_NAME"),
    (225, "_STORE_SUBSCR_LIST_INT"),
    (221, "_DICT_UPDATE"),
    (219, "_COMPARE_OP_FLOAT"),
    (212, "_WITH_EXCEPT_START"),
    (208, "_BINARY_SUBSCR_LIST_INT"),
    (204, "_BINARY_SUBSCR_TUPLE_INT"),
    (203, "_LOAD_FROM_DICT_OR_DEREF"),
    (203, "_FOR_ITER_TIER_TWO"),
    (202, "_STORE_SUBSCR"),
    (195, "_LOAD_ATTR_MODULE"),
    (194, "_CALL_INTRINSIC_2"),
    (194, "_BINARY_SLICE"),
    (192, "_CONTAINS_OP"),
    (190, "_CHECK_EXC_MATCH"),
    (188, "_DICT_MERGE"),
    (187, "_BINARY_OP"),
    (185, "_GET_YIELD_FROM_ITER"),
    (183, "_UNPACK_SEQUENCE_LIST"),
    (183, "_STORE_ATTR"),
    (176, "_FORMAT_WITH_SPEC"),
    (176, "_BINARY_SUBSCR"),
    (176, "_BINARY_OP_SUBTRACT_INT"),
    (176, "_BINARY_OP_MULTIPLY_INT"),
    (176, "_BINARY_OP_ADD_UNICODE"),
    (176, "_BINARY_OP_ADD_INT"),
    (175, "_DELETE_FAST"),
    (174, "_COPY_FREE_VARS"),
    (169, "_UNPACK_SEQUENCE_TWO_TUPLE"),
    (166, "_DELETE_NAME"),
    (165, "_UNPACK_SEQUENCE_TUPLE"),
    (165, "_CALL_TUPLE_1"),
    (165, "_CALL_STR_1"),
    (164, "_INIT_CALL_BOUND_METHOD_EXACT_ARGS"),
    (164, "_DELETE_SUBSCR"),
    (163, "_COMPARE_OP_STR"),
    (155, "_STORE_SUBSCR_DICT"),
    (153, "_LOAD_FAST_CHECK"),
    (151, "_UNPACK_EX"),
    (150, "_UNPACK_SEQUENCE"),
    (150, "_STORE_ATTR_INSTANCE_VALUE"),
    (147, "_IS_OP"),
    (144, "_FORMAT_SIMPLE"),
    (142, "_LOAD_ATTR_INSTANCE_VALUE_1"),
    (140, "_MAKE_FUNCTION"),
    (140, "_LOAD_BUILD_CLASS"),
    (140, "_CALL_INTRINSIC_1"),
    (138, "_TO_BOOL_STR"),
    (138, "_DELETE_GLOBAL"),
    (137, "_LOAD_ATTR_SLOT_1"),
    (135, "_TO_BOOL_LIST"),
    (135, "_SET_UPDATE"),
    (134, "_TO_BOOL"),
    (133, "_STORE_GLOBAL"),
    (133, "_CONVERT_VALUE"),
    (132, "_TO_BOOL_INT"),
    (131, "_LOAD_ATTR_INSTANCE_VALUE_0"),
    (130, "_STORE_ATTR_SLOT"),
    (129, "_SET_ADD"),
    (129, "_DELETE_ATTR"),
    (129, "_CALL_TYPE_1"),
    (126, "_LOAD_ATTR_SLOT_0"),
    (123, "_DELETE_DEREF"),
    (122, "_UNARY_NEGATIVE"),
    (122, "_UNARY_INVERT"),
    (122, "_GET_ITER"),
    (120, "_MAKE_CELL"),
    (120, "_BUILD_TUPLE"),
    (120, "_BUILD_LIST"),
    (118, "_LIST_APPEND"),
    (114, "_SET_FUNCTION_ATTRIBUTE"),
    (109, "_LOAD_DEREF"),
    (104, "_LOAD_LOCALS"),
    (100, "_LOAD_GLOBAL_MODULE"),
    (100, "_LOAD_GLOBAL_BUILTINS"),
    (98, "_EXIT_INIT_CHECK"),
    (96, "_LOAD_ATTR_CLASS_1"),
    (95, "_ITER_NEXT_RANGE"),
    (93, "_LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES"),
    (93, "_LOAD_ATTR_NONDESCRIPTOR_NO_DICT"),
    (92, "_POP_FRAME"),
    (92, "_JUMP_TO_TOP"),
    (92, "_GUARD_IS_NOT_NONE_POP"),
    (91, "_MAP_ADD"),
    (91, "_IS_NONE"),
    (90, "_GET_LEN"),
    (88, "_CHECK_FUNCTION_EXACT_ARGS"),
    (86, "_GUARD_IS_NONE_POP"),
    (86, "_CHECK_PERIODIC"),
    (85, "_POP_EXCEPT"),
    (85, "_LOAD_ATTR_CLASS_0"),
    (83, "_MATCH_KEYS"),
    (82, "_STORE_DEREF"),
    (79, "_STORE_FAST"),
    (78, "_GUARD_DORV_VALUES_INST_ATTR_FROM_DICT"),
    (78, "_CHECK_MANAGED_OBJECT_HAS_VALUES"),
    (76, "_STORE_FAST_7"),
    (74, "_END_SEND"),
    (74, "_CHECK_STACK_SPACE"),
    (70, "_STORE_FAST_6"),
    (70, "_STORE_FAST_5"),
    (70, "_STORE_FAST_4"),
    (70, "_STORE_FAST_3"),
    (70, "_STORE_FAST_2"),
    (70, "_STORE_FAST_1"),
    (70, "_STORE_FAST_0"),
    (69, "_SIDE_EXIT"),
    (67, "_START_EXECUTOR"),
    (64, "_REPLACE_WITH_TRUE"),
    (64, "_POP_TOP_LOAD_CONST_INLINE_BORROW"),
    (62, "_CHECK_CALL_BOUND_METHOD_EXACT_ARGS"),
    (57, "_POP_TOP"),
    (55, "_PUSH_FRAME"),
    (51, "_MATCH_SEQUENCE"),
    (51, "_MATCH_MAPPING"),
    (48, "_PUSH_EXC_INFO"),
    (48, "_DEOPT"),
    (48, "_CHECK_ATTR_MODULE"),
    (45, "_GUARD_GLOBALS_VERSION"),
    (45, "_GUARD_BUILTINS_VERSION"),
    (44, "_ERROR_POP_N"),
    (43, "_ITER_NEXT_LIST"),
    (43, "_CHECK_ATTR_CLASS"),
    (40, "_ITER_NEXT_TUPLE"),
    (40, "_GUARD_BOTH_UNICODE"),
    (40, "_GUARD_BOTH_INT"),
    (40, "_GUARD_BOTH_FLOAT"),
    (38, "_GUARD_KEYS_VERSION"),
    (36, "_CHECK_VALIDITY_AND_SET_IP"),
    (35, "_SWAP"),
    (35, "_GUARD_NOT_EXHAUSTED_TUPLE"),
    (35, "_GUARD_NOT_EXHAUSTED_LIST"),
    (35, "_COPY"),
    (34, "_LOAD_CONST"),
    (34, "_GUARD_TYPE_VERSION"),
    (33, "_LOAD_ATTR_METHOD_WITH_VALUES"),
    (33, "_LOAD_ATTR_METHOD_NO_DICT"),
    (33, "_LOAD_ATTR_METHOD_LAZY_DICT"),
    (30, "_TO_BOOL_NONE"),
    (30, "_LOAD_CONST_INLINE_WITH_NULL"),
    (30, "_CHECK_FUNCTION"),
    (29, "_CHECK_ATTR_WITH_HINT"),
    (28, "_RESUME_CHECK"),
    (28, "_LOAD_FAST_AND_CLEAR"),
    (28, "_CHECK_ATTR_METHOD_LAZY_DICT"),
    (27, "_LOAD_FAST"),
    (25, "_LOAD_ASSERTION_ERROR"),
    (25, "_CHECK_VALIDITY"),
    (24, "_UNARY_NOT"),
    (23, "_TO_BOOL_BOOL"),
    (23, "_ITER_CHECK_TUPLE"),
    (23, "_ITER_CHECK_RANGE"),
    (23, "_ITER_CHECK_LIST"),
    (23, "_GUARD_NOT_EXHAUSTED_RANGE"),
    (23, "_GUARD_IS_TRUE_POP"),
    (23, "_GUARD_IS_FALSE_POP"),
    (22, "_LOAD_FAST_7"),
    (22, "_LOAD_CONST_INLINE_BORROW_WITH_NULL"),
    (22, "_LOAD_CONST_INLINE"),
    (21, "_FATAL_ERROR"),
    (20, "_CHECK_PEP_523"),
    (19, "_LOAD_FAST_6"),
    (19, "_LOAD_FAST_5"),
    (19, "_LOAD_FAST_4"),
    (19, "_LOAD_FAST_3"),
    (19, "_LOAD_FAST_2"),
    (19, "_LOAD_FAST_1"),
    (19, "_LOAD_FAST_0"),
    (16, "_GUARD_DORV_VALUES"),
    (14, "_LOAD_CONST_INLINE_BORROW"),
    (12, "_INTERNAL_INCREMENT_OPT_COUNTER"),
    (11, "_SET_IP"),
    (11, "_SAVE_RETURN_OFFSET"),
    (11, "_PUSH_NULL"),
    (6, "_EXIT_TRACE"),
    (0, "_NOP"),
]
TEMPLATE_SIZES = {x[1]: x[0] for x in TEMPLATE_SIZES}


BROKEN = {
    "_BINARY_OP_MULTIPLY_FLOAT",
    "_LOAD_ATTR_WITH_HINT",
    "CALL_LEN",
    "CALL_ISINSTANCE",
    "_COLD_EXIT",
    "_BINARY_OP_ADD_FLOAT",
    "_BINARY_OP_SUBTRACT_FLOAT",
}


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
    passthrough: bool
    tier: int | None = None
    oparg_and_1: bool = False
    const_oparg: int = -1
    stub: bool = False

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
            passthrough=all(p.passthrough for p in properties),
            stub=all(p.stub for p in properties),
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
    passthrough=False,
    stub=False,
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
    type_prop: None | tuple[str, None | str] = field(
        default_factory=lambda: None, init=True, compare=False, hash=False
    )

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
    replicates: "Uop | None" = None

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


def convert_stack_item(
    item: parser.StackEffect, replace_op_arg_1: str | None
) -> StackItem:
    cond = item.cond
    if replace_op_arg_1 and OPARG_AND_1.match(item.cond):
        cond = replace_op_arg_1
    return StackItem(item.name, item.type, cond, (item.size or "1"))


def analyze_stack(
    op: parser.InstDef, replace_op_arg_1: str | None = None
) -> StackEffect:
    inputs: list[StackItem] = [
        convert_stack_item(i, replace_op_arg_1)
        for i in op.inputs
        if isinstance(i, parser.StackEffect)
    ]
    outputs: list[StackItem] = [
        convert_stack_item(i, replace_op_arg_1) for i in op.outputs
    ]
    for input, output in zip(inputs, outputs):
        if input.name == output.name:
            input.peek = output.peek = True
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
    "_PyDictOrValues_IsValues",
    "_PyObject_DictOrValuesPointer",
    "_PyDictOrValues_GetValues",
    "_PyDictValues_AddToInsertionOrder",
    "_PyObject_MakeInstanceAttributesFromDict",
    "Py_DECREF",
    "_Py_DECREF_SPECIALIZED",
    "DECREF_INPUTS_AND_REUSE_FLOAT",
    "PyUnicode_Append",
    "_PyLong_IsZero",
    "Py_SIZE",
    "Py_TYPE",
    "PyList_GET_ITEM",
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
    "maybe_lltrace_resume_frame",
    "_PyUnicode_JoinArray",
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
        or variable_used(op, "PyCell_GET")
        or variable_used(op, "PyCell_SET")
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
    infallible = not error_with_pop and not error_without_pop
    passthrough = stack_effect_only_peeks(op) and infallible
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
        passthrough=passthrough,
        tier=tier_variable(op),
        stub=((TEMPLATE_SIZES.get(op.name, 0) > 200 or TEMPLATE_SIZES.get("_" + op.name, 0) > 200) and op.name not in BROKEN),
    )


def make_uop(
    name: str,
    op: parser.InstDef,
    inputs: list[parser.InputEffect],
    uops: dict[str, Uop],
) -> Uop:
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
                properties.oparg = any(
                    token.text == "oparg" for token in op.block.tokens
                )
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
