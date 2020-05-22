import ast
from dataclasses import field, dataclass
import re
from typing import Any, Dict, IO, Optional, List, Text, Tuple, Set
from enum import Enum

from pegen import grammar
from pegen.grammar import (
    Alt,
    Cut,
    Gather,
    GrammarVisitor,
    Group,
    Lookahead,
    NamedItem,
    NameLeaf,
    NegativeLookahead,
    Opt,
    PositiveLookahead,
    Repeat0,
    Repeat1,
    Rhs,
    Rule,
    StringLeaf,
)
from pegen.parser_generator import ParserGenerator


EXTENSION_PREFIX = """\
#include "pegen.h"

"""


EXTENSION_SUFFIX = """
void *
_PyPegen_parse(Parser *p)
{
    // Initialize keywords
    p->keywords = reserved_keywords;
    p->n_keyword_lists = n_keyword_lists;

    return start_rule(p);
}
"""


class NodeTypes(Enum):
    NAME_TOKEN = 0
    NUMBER_TOKEN = 1
    STRING_TOKEN = 2
    GENERIC_TOKEN = 3
    KEYWORD = 4
    CUT_OPERATOR = 5


BASE_NODETYPES = {
    "NAME": NodeTypes.NAME_TOKEN,
    "NUMBER": NodeTypes.NUMBER_TOKEN,
    "STRING": NodeTypes.STRING_TOKEN,
}


@dataclass
class FunctionCall:
    function: str
    arguments: List[Any] = field(default_factory=list)
    assigned_variable: Optional[str] = None
    return_type: Optional[str] = None
    nodetype: Optional[NodeTypes] = None
    force_true: bool = False
    comment: Optional[str] = None

    def __str__(self) -> str:
        parts = []
        parts.append(self.function)
        if self.arguments:
            parts.append(f"({', '.join(map(str, self.arguments))})")
        if self.force_true:
            parts.append(", 1")
        if self.assigned_variable:
            parts = ["(", self.assigned_variable, " = ", *parts, ")"]
        if self.comment:
            parts.append(f"  // {self.comment}")
        return "".join(parts)


class CCallMakerVisitor(GrammarVisitor):
    def __init__(
        self,
        parser_generator: ParserGenerator,
        exact_tokens: Dict[str, int],
        non_exact_tokens: Set[str],
    ):
        self.gen = parser_generator
        self.exact_tokens = exact_tokens
        self.non_exact_tokens = non_exact_tokens
        self.cache: Dict[Any, FunctionCall] = {}
        self.keyword_cache: Dict[str, int] = {}

    def keyword_helper(self, keyword: str) -> FunctionCall:
        if keyword not in self.keyword_cache:
            self.keyword_cache[keyword] = self.gen.keyword_type()
        return FunctionCall(
            assigned_variable="_keyword",
            function="_PyPegen_expect_token",
            arguments=["p", self.keyword_cache[keyword]],
            return_type="Token *",
            nodetype=NodeTypes.KEYWORD,
            comment=f"token='{keyword}'",
        )

    def visit_NameLeaf(self, node: NameLeaf) -> FunctionCall:
        name = node.value
        if name in self.non_exact_tokens:
            if name in BASE_NODETYPES:
                return FunctionCall(
                    assigned_variable=f"{name.lower()}_var",
                    function=f"_PyPegen_{name.lower()}_token",
                    arguments=["p"],
                    nodetype=BASE_NODETYPES[name],
                    return_type="expr_ty",
                    comment=name,
                )
            return FunctionCall(
                assigned_variable=f"{name.lower()}_var",
                function=f"_PyPegen_expect_token",
                arguments=["p", name],
                nodetype=NodeTypes.GENERIC_TOKEN,
                return_type="Token *",
                comment=f"token='{name}'",
            )

        type = None
        rule = self.gen.all_rules.get(name.lower())
        if rule is not None:
            type = "asdl_seq *" if rule.is_loop() or rule.is_gather() else rule.type

        return FunctionCall(
            assigned_variable=f"{name}_var",
            function=f"{name}_rule",
            arguments=["p"],
            return_type=type,
            comment=f"{node}",
        )

    def visit_StringLeaf(self, node: StringLeaf) -> FunctionCall:
        val = ast.literal_eval(node.value)
        if re.match(r"[a-zA-Z_]\w*\Z", val):  # This is a keyword
            return self.keyword_helper(val)
        else:
            assert val in self.exact_tokens, f"{node.value} is not a known literal"
            type = self.exact_tokens[val]
            return FunctionCall(
                assigned_variable="_literal",
                function=f"_PyPegen_expect_token",
                arguments=["p", type],
                nodetype=NodeTypes.GENERIC_TOKEN,
                return_type="Token *",
                comment=f"token='{val}'",
            )

    def visit_Rhs(self, node: Rhs) -> FunctionCall:
        def can_we_inline(node: Rhs) -> int:
            if len(node.alts) != 1 or len(node.alts[0].items) != 1:
                return False
            # If the alternative has an action we cannot inline
            if getattr(node.alts[0], "action", None) is not None:
                return False
            return True

        if node in self.cache:
            return self.cache[node]
        if can_we_inline(node):
            self.cache[node] = self.generate_call(node.alts[0].items[0])
        else:
            name = self.gen.name_node(node)
            self.cache[node] = FunctionCall(
                assigned_variable=f"{name}_var",
                function=f"{name}_rule",
                arguments=["p"],
                comment=f"{node}",
            )
        return self.cache[node]

    def visit_NamedItem(self, node: NamedItem) -> FunctionCall:
        call = self.generate_call(node.item)
        if node.name:
            call.assigned_variable = node.name
        return call

    def lookahead_call_helper(self, node: Lookahead, positive: int) -> FunctionCall:
        call = self.generate_call(node.node)
        if call.nodetype == NodeTypes.NAME_TOKEN:
            return FunctionCall(
                function=f"_PyPegen_lookahead_with_name",
                arguments=[positive, call.function, *call.arguments],
                return_type="int",
            )
        elif call.nodetype in {NodeTypes.GENERIC_TOKEN, NodeTypes.KEYWORD}:
            return FunctionCall(
                function=f"_PyPegen_lookahead_with_int",
                arguments=[positive, call.function, *call.arguments],
                return_type="int",
                comment=f"token={node.node}",
            )
        else:
            return FunctionCall(
                function=f"_PyPegen_lookahead",
                arguments=[positive, call.function, *call.arguments],
                return_type="int",
            )

    def visit_PositiveLookahead(self, node: PositiveLookahead) -> FunctionCall:
        return self.lookahead_call_helper(node, 1)

    def visit_NegativeLookahead(self, node: NegativeLookahead) -> FunctionCall:
        return self.lookahead_call_helper(node, 0)

    def visit_Opt(self, node: Opt) -> FunctionCall:
        call = self.generate_call(node.node)
        return FunctionCall(
            assigned_variable="_opt_var",
            function=call.function,
            arguments=call.arguments,
            force_true=True,
            comment=f"{node}",
        )

    def visit_Repeat0(self, node: Repeat0) -> FunctionCall:
        if node in self.cache:
            return self.cache[node]
        name = self.gen.name_loop(node.node, False)
        self.cache[node] = FunctionCall(
            assigned_variable=f"{name}_var",
            function=f"{name}_rule",
            arguments=["p"],
            return_type="asdl_seq *",
            comment=f"{node}",
        )
        return self.cache[node]

    def visit_Repeat1(self, node: Repeat1) -> FunctionCall:
        if node in self.cache:
            return self.cache[node]
        name = self.gen.name_loop(node.node, True)
        self.cache[node] = FunctionCall(
            assigned_variable=f"{name}_var",
            function=f"{name}_rule",
            arguments=["p"],
            return_type="asdl_seq *",
            comment=f"{node}",
        )
        return self.cache[node]

    def visit_Gather(self, node: Gather) -> FunctionCall:
        if node in self.cache:
            return self.cache[node]
        name = self.gen.name_gather(node)
        self.cache[node] = FunctionCall(
            assigned_variable=f"{name}_var",
            function=f"{name}_rule",
            arguments=["p"],
            return_type="asdl_seq *",
            comment=f"{node}",
        )
        return self.cache[node]

    def visit_Group(self, node: Group) -> FunctionCall:
        return self.generate_call(node.rhs)

    def visit_Cut(self, node: Cut) -> FunctionCall:
        return FunctionCall(
            assigned_variable="_cut_var",
            return_type="int",
            function="1",
            nodetype=NodeTypes.CUT_OPERATOR,
        )

    def generate_call(self, node: Any) -> FunctionCall:
        return super().visit(node)


class CParserGenerator(ParserGenerator, GrammarVisitor):
    def __init__(
        self,
        grammar: grammar.Grammar,
        tokens: Dict[int, str],
        exact_tokens: Dict[str, int],
        non_exact_tokens: Set[str],
        file: Optional[IO[Text]],
        debug: bool = False,
        skip_actions: bool = False,
    ):
        super().__init__(grammar, tokens, file)
        self.callmakervisitor: CCallMakerVisitor = CCallMakerVisitor(
            self, exact_tokens, non_exact_tokens
        )
        self._varname_counter = 0
        self.debug = debug
        self.skip_actions = skip_actions

    def unique_varname(self, name: str = "tmpvar") -> str:
        new_var = name + "_" + str(self._varname_counter)
        self._varname_counter += 1
        return new_var

    def call_with_errorcheck_return(self, call_text: str, returnval: str) -> None:
        error_var = self.unique_varname()
        self.print(f"int {error_var} = {call_text};")
        self.print(f"if ({error_var}) {{")
        with self.indent():
            self.print(f"return {returnval};")
        self.print(f"}}")

    def call_with_errorcheck_goto(self, call_text: str, goto_target: str) -> None:
        error_var = self.unique_varname()
        self.print(f"int {error_var} = {call_text};")
        self.print(f"if ({error_var}) {{")
        with self.indent():
            self.print(f"goto {goto_target};")
        self.print(f"}}")

    def out_of_memory_return(self, expr: str, cleanup_code: Optional[str] = None,) -> None:
        self.print(f"if ({expr}) {{")
        with self.indent():
            if cleanup_code is not None:
                self.print(cleanup_code)
            self.print("p->error_indicator = 1;")
            self.print("PyErr_NoMemory();")
            self.print("return NULL;")
        self.print(f"}}")

    def out_of_memory_goto(self, expr: str, goto_target: str) -> None:
        self.print(f"if ({expr}) {{")
        with self.indent():
            self.print("PyErr_NoMemory();")
            self.print(f"goto {goto_target};")
        self.print(f"}}")

    def generate(self, filename: str) -> None:
        self.collect_todo()
        self.print(f"// @generated by pegen.py from {filename}")
        header = self.grammar.metas.get("header", EXTENSION_PREFIX)
        if header:
            self.print(header.rstrip("\n"))
        subheader = self.grammar.metas.get("subheader", "")
        if subheader:
            self.print(subheader)
        self._setup_keywords()
        for i, (rulename, rule) in enumerate(self.todo.items(), 1000):
            comment = "  // Left-recursive" if rule.left_recursive else ""
            self.print(f"#define {rulename}_type {i}{comment}")
        self.print()
        for rulename, rule in self.todo.items():
            if rule.is_loop() or rule.is_gather():
                type = "asdl_seq *"
            elif rule.type:
                type = rule.type + " "
            else:
                type = "void *"
            self.print(f"static {type}{rulename}_rule(Parser *p);")
        self.print()
        while self.todo:
            for rulename, rule in list(self.todo.items()):
                del self.todo[rulename]
                self.print()
                if rule.left_recursive:
                    self.print("// Left-recursive")
                self.visit(rule)
        if self.skip_actions:
            mode = 0
        else:
            mode = int(self.rules["start"].type == "mod_ty") if "start" in self.rules else 1
            if mode == 1 and self.grammar.metas.get("bytecode"):
                mode += 1
        modulename = self.grammar.metas.get("modulename", "parse")
        trailer = self.grammar.metas.get("trailer", EXTENSION_SUFFIX)
        if trailer:
            self.print(trailer.rstrip("\n") % dict(mode=mode, modulename=modulename))

    def _group_keywords_by_length(self) -> Dict[int, List[Tuple[str, int]]]:
        groups: Dict[int, List[Tuple[str, int]]] = {}
        for keyword_str, keyword_type in self.callmakervisitor.keyword_cache.items():
            length = len(keyword_str)
            if length in groups:
                groups[length].append((keyword_str, keyword_type))
            else:
                groups[length] = [(keyword_str, keyword_type)]
        return groups

    def _setup_keywords(self) -> None:
        keyword_cache = self.callmakervisitor.keyword_cache
        n_keyword_lists = (
            len(max(keyword_cache.keys(), key=len)) + 1 if len(keyword_cache) > 0 else 0
        )
        self.print(f"static const int n_keyword_lists = {n_keyword_lists};")
        groups = self._group_keywords_by_length()
        self.print("static KeywordToken *reserved_keywords[] = {")
        with self.indent():
            num_groups = max(groups) + 1 if groups else 1
            for keywords_length in range(num_groups):
                if keywords_length not in groups.keys():
                    self.print("NULL,")
                else:
                    self.print("(KeywordToken[]) {")
                    with self.indent():
                        for keyword_str, keyword_type in groups[keywords_length]:
                            self.print(f'{{"{keyword_str}", {keyword_type}}},')
                        self.print("{NULL, -1},")
                    self.print("},")
        self.print("};")

    def _set_up_token_start_metadata_extraction(self) -> None:
        self.print("if (p->mark == p->fill && _PyPegen_fill_token(p) < 0) {")
        with self.indent():
            self.print("p->error_indicator = 1;")
            self.print("return NULL;")
        self.print("}")
        self.print("int _start_lineno = p->tokens[_mark]->lineno;")
        self.print("UNUSED(_start_lineno); // Only used by EXTRA macro")
        self.print("int _start_col_offset = p->tokens[_mark]->col_offset;")
        self.print("UNUSED(_start_col_offset); // Only used by EXTRA macro")

    def _set_up_token_end_metadata_extraction(self) -> None:
        self.print("Token *_token = _PyPegen_get_last_nonnwhitespace_token(p);")
        self.print("if (_token == NULL) {")
        with self.indent():
            self.print("return NULL;")
        self.print("}")
        self.print("int _end_lineno = _token->end_lineno;")
        self.print("UNUSED(_end_lineno); // Only used by EXTRA macro")
        self.print("int _end_col_offset = _token->end_col_offset;")
        self.print("UNUSED(_end_col_offset); // Only used by EXTRA macro")

    def _check_for_errors(self) -> None:
        self.print("if (p->error_indicator) {")
        with self.indent():
            self.print("return NULL;")
        self.print("}")

    def _set_up_rule_memoization(self, node: Rule, result_type: str) -> None:
        self.print("{")
        with self.indent():
            self.print(f"{result_type} _res = NULL;")
            self.print(f"if (_PyPegen_is_memoized(p, {node.name}_type, &_res))")
            with self.indent():
                self.print("return _res;")
            self.print("int _mark = p->mark;")
            self.print("int _resmark = p->mark;")
            self.print("while (1) {")
            with self.indent():
                self.call_with_errorcheck_return(
                    f"_PyPegen_update_memo(p, _mark, {node.name}_type, _res)", "_res"
                )
                self.print("p->mark = _mark;")
                self.print(f"void *_raw = {node.name}_raw(p);")
                self.print("if (_raw == NULL || p->mark <= _resmark)")
                with self.indent():
                    self.print("break;")
                self.print(f"_resmark = p->mark;")
                self.print("_res = _raw;")
            self.print("}")
            self.print(f"p->mark = _resmark;")
            self.print("return _res;")
        self.print("}")
        self.print(f"static {result_type}")
        self.print(f"{node.name}_raw(Parser *p)")

    def _should_memoize(self, node: Rule) -> bool:
        return node.memo and not node.left_recursive

    def _handle_default_rule_body(self, node: Rule, rhs: Rhs, result_type: str) -> None:
        memoize = self._should_memoize(node)

        with self.indent():
            self._check_for_errors()
            self.print(f"{result_type} _res = NULL;")
            if memoize:
                self.print(f"if (_PyPegen_is_memoized(p, {node.name}_type, &_res))")
                with self.indent():
                    self.print("return _res;")
            self.print("int _mark = p->mark;")
            if any(alt.action and "EXTRA" in alt.action for alt in rhs.alts):
                self._set_up_token_start_metadata_extraction()
            self.visit(
                rhs, is_loop=False, is_gather=node.is_gather(), rulename=node.name,
            )
            if self.debug:
                self.print(f'fprintf(stderr, "Fail at %d: {node.name}\\n", p->mark);')
            self.print("_res = NULL;")
        self.print("  done:")
        with self.indent():
            if memoize:
                self.print(f"_PyPegen_insert_memo(p, _mark, {node.name}_type, _res);")
            self.print("return _res;")

    def _handle_loop_rule_body(self, node: Rule, rhs: Rhs) -> None:
        memoize = self._should_memoize(node)
        is_repeat1 = node.name.startswith("_loop1")

        with self.indent():
            self._check_for_errors()
            self.print("void *_res = NULL;")
            if memoize:
                self.print(f"if (_PyPegen_is_memoized(p, {node.name}_type, &_res))")
                with self.indent():
                    self.print("return _res;")
            self.print("int _mark = p->mark;")
            self.print("int _start_mark = p->mark;")
            self.print("void **_children = PyMem_Malloc(sizeof(void *));")
            self.out_of_memory_return(f"!_children")
            self.print("ssize_t _children_capacity = 1;")
            self.print("ssize_t _n = 0;")
            if any(alt.action and "EXTRA" in alt.action for alt in rhs.alts):
                self._set_up_token_start_metadata_extraction()
            self.visit(
                rhs, is_loop=True, is_gather=node.is_gather(), rulename=node.name,
            )
            if is_repeat1:
                self.print("if (_n == 0 || p->error_indicator) {")
                with self.indent():
                    self.print("PyMem_Free(_children);")
                    self.print("return NULL;")
                self.print("}")
            self.print("asdl_seq *_seq = _Py_asdl_seq_new(_n, p->arena);")
            self.out_of_memory_return(f"!_seq", cleanup_code="PyMem_Free(_children);")
            self.print("for (int i = 0; i < _n; i++) asdl_seq_SET(_seq, i, _children[i]);")
            self.print("PyMem_Free(_children);")
            if node.name:
                self.print(f"_PyPegen_insert_memo(p, _start_mark, {node.name}_type, _seq);")
            self.print("return _seq;")

    def visit_Rule(self, node: Rule) -> None:
        is_loop = node.is_loop()
        is_gather = node.is_gather()
        rhs = node.flatten()
        if is_loop or is_gather:
            result_type = "asdl_seq *"
        elif node.type:
            result_type = node.type
        else:
            result_type = "void *"

        for line in str(node).splitlines():
            self.print(f"// {line}")
        if node.left_recursive and node.leader:
            self.print(f"static {result_type} {node.name}_raw(Parser *);")

        self.print(f"static {result_type}")
        self.print(f"{node.name}_rule(Parser *p)")

        if node.left_recursive and node.leader:
            self._set_up_rule_memoization(node, result_type)

        self.print("{")
        if is_loop:
            self._handle_loop_rule_body(node, rhs)
        else:
            self._handle_default_rule_body(node, rhs, result_type)
        self.print("}")

    def visit_NamedItem(self, node: NamedItem) -> None:
        call = self.callmakervisitor.generate_call(node)
        if call.assigned_variable:
            call.assigned_variable = self.dedupe(call.assigned_variable)
        self.print(call)

    def visit_Rhs(
        self, node: Rhs, is_loop: bool, is_gather: bool, rulename: Optional[str]
    ) -> None:
        if is_loop:
            assert len(node.alts) == 1
        for alt in node.alts:
            self.visit(alt, is_loop=is_loop, is_gather=is_gather, rulename=rulename)

    def join_conditions(self, keyword: str, node: Any) -> None:
        self.print(f"{keyword} (")
        with self.indent():
            first = True
            for item in node.items:
                if first:
                    first = False
                else:
                    self.print("&&")
                self.visit(item)
        self.print(")")

    def emit_action(self, node: Alt, cleanup_code: Optional[str] = None) -> None:
        self.print(f"_res = {node.action};")

        self.print("if (_res == NULL && PyErr_Occurred()) {")
        with self.indent():
            self.print("p->error_indicator = 1;")
            if cleanup_code:
                self.print(cleanup_code)
            self.print("return NULL;")
        self.print("}")

        if self.debug:
            self.print(
                f'fprintf(stderr, "Hit with action [%d-%d]: %s\\n", _mark, p->mark, "{node}");'
            )

    def emit_default_action(self, is_gather: bool, node: Alt) -> None:
        if len(self.local_variable_names) > 1:
            if is_gather:
                assert len(self.local_variable_names) == 2
                self.print(
                    f"_res = _PyPegen_seq_insert_in_front(p, "
                    f"{self.local_variable_names[0]}, {self.local_variable_names[1]});"
                )
            else:
                if self.debug:
                    self.print(
                        f'fprintf(stderr, "Hit without action [%d:%d]: %s\\n", _mark, p->mark, "{node}");'
                    )
                self.print(
                    f"_res = _PyPegen_dummy_name(p, {', '.join(self.local_variable_names)});"
                )
        else:
            if self.debug:
                self.print(
                    f'fprintf(stderr, "Hit with default action [%d:%d]: %s\\n", _mark, p->mark, "{node}");'
                )
            self.print(f"_res = {self.local_variable_names[0]};")

    def emit_dummy_action(self) -> None:
        self.print("_res = _PyPegen_dummy_name(p);")

    def handle_alt_normal(self, node: Alt, is_gather: bool) -> None:
        self.join_conditions(keyword="if", node=node)
        self.print("{")
        # We have parsed successfully all the conditions for the option.
        with self.indent():
            # Prepare to emmit the rule action and do so
            if node.action and "EXTRA" in node.action:
                self._set_up_token_end_metadata_extraction()
            if self.skip_actions:
                self.emit_dummy_action()
            elif node.action:
                self.emit_action(node)
            else:
                self.emit_default_action(is_gather, node)

            # As the current option has parsed correctly, do not continue with the rest.
            self.print(f"goto done;")
        self.print("}")

    def handle_alt_loop(self, node: Alt, is_gather: bool, rulename: Optional[str]) -> None:
        # Condition of the main body of the alternative
        self.join_conditions(keyword="while", node=node)
        self.print("{")
        # We have parsed successfully one item!
        with self.indent():
            # Prepare to emit the rule action and do so
            if node.action and "EXTRA" in node.action:
                self._set_up_token_end_metadata_extraction()
            if self.skip_actions:
                self.emit_dummy_action()
            elif node.action:
                self.emit_action(node, cleanup_code="PyMem_Free(_children);")
            else:
                self.emit_default_action(is_gather, node)

            # Add the result of rule to the temporary buffer of children. This buffer
            # will populate later an asdl_seq with all elements to return.
            self.print("if (_n == _children_capacity) {")
            with self.indent():
                self.print("_children_capacity *= 2;")
                self.print(
                    "void **_new_children = PyMem_Realloc(_children, _children_capacity*sizeof(void *));"
                )
                self.out_of_memory_return(f"!_new_children")
                self.print("_children = _new_children;")
            self.print("}")
            self.print("_children[_n++] = _res;")
            self.print("_mark = p->mark;")
        self.print("}")

    def visit_Alt(
        self, node: Alt, is_loop: bool, is_gather: bool, rulename: Optional[str]
    ) -> None:
        self.print(f"{{ // {node}")
        with self.indent():
            self._check_for_errors()
            # Prepare variable declarations for the alternative
            vars = self.collect_vars(node)
            for v, var_type in sorted(item for item in vars.items() if item[0] is not None):
                if not var_type:
                    var_type = "void *"
                else:
                    var_type += " "
                if v == "_cut_var":
                    v += " = 0"  # cut_var must be initialized
                self.print(f"{var_type}{v};")
                if v == "_opt_var":
                    self.print("UNUSED(_opt_var); // Silence compiler warnings")

            with self.local_variable_context():
                if is_loop:
                    self.handle_alt_loop(node, is_gather, rulename)
                else:
                    self.handle_alt_normal(node, is_gather)

            self.print("p->mark = _mark;")
            if "_cut_var" in vars:
                self.print("if (_cut_var) return NULL;")
        self.print("}")

    def collect_vars(self, node: Alt) -> Dict[Optional[str], Optional[str]]:
        types = {}
        with self.local_variable_context():
            for item in node.items:
                name, type = self.add_var(item)
                types[name] = type
        return types

    def add_var(self, node: NamedItem) -> Tuple[Optional[str], Optional[str]]:
        call = self.callmakervisitor.generate_call(node.item)
        name = node.name if node.name else call.assigned_variable
        if name is not None:
            name = self.dedupe(name)
        return name, call.return_type
