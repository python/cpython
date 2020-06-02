import ast
import contextlib
import sys
import re
import token
import tokenize
from collections import defaultdict
from itertools import accumulate
import sys
from typing import Any, Dict, Iterator, List, Optional, Tuple, Union

from pegen import grammar
from pegen.build import build_parser
from pegen.grammar import (
    Alt,
    Cut,
    Gather,
    GrammarVisitor,
    Group,
    Leaf,
    Lookahead,
    NamedItem,
    NameLeaf,
    NegativeLookahead,
    Opt,
    Plain,
    PositiveLookahead,
    Repeat,
    Repeat0,
    Repeat1,
    Rhs,
    Rule,
    StringLeaf,
)
from pegen.parser_generator import ParserGenerator

CALL_ACTION_HEAD = """
static void *
call_action(Parser *p, Frame *_f, int _iaction)
{
    assert(p->mark > 0);
    Token *_t = p->tokens[_f->mark];
    int _start_lineno = _t->lineno;
    int _start_col_offset = _t->col_offset;
    _t = p->tokens[p->mark - 1];
    int _end_lineno = _t->end_lineno;
    int _end_col_offset = _t->end_col_offset;

    switch (_iaction) {
"""

CALL_ACTION_TAIL = """\
    default:
        assert(0);
    }
}
"""

class RootRule(Rule):
    def __init__(self, name: str, startrulename: str):
        super().__init__(name, "void *", Rhs([]), None)
        self.startrulename = startrulename


class VMCallMakerVisitor(GrammarVisitor):
    def __init__(
        self, parser_generator: ParserGenerator,
    ):
        self.gen = parser_generator
        self.cache: Dict[Any, Any] = {}
        self.keyword_cache: Dict[str, int] = {}
        self.soft_keyword_cache: List[str] = []

    def keyword_helper(self, keyword: str) -> Tuple[str, int]:
        if keyword not in self.keyword_cache:
            self.keyword_cache[keyword] = self.gen.keyword_type()
        return "OP_TOKEN", self.keyword_cache[keyword]

    def soft_keyword_helper(self, keyword: str) -> Tuple[str, str]:
        if keyword not in self.soft_keyword_cache:
            self.soft_keyword_cache.append(keyword)
        return "OP_SOFT_KEYWORD", f"SK_{keyword.upper()}"

    def visit_StringLeaf(self, node: StringLeaf) -> Tuple[str, Union[str, int]]:
        val = ast.literal_eval(node.value)
        if re.match(r"[a-zA-Z_]\w*\Z", val):  # This is a keyword
            if node.value.endswith("'"):
                return self.keyword_helper(val)
            else:
                return self.soft_keyword_helper(val)
        tok_num = token.EXACT_TOKEN_TYPES[val]
        return "OP_TOKEN", token.tok_name[tok_num]

    def visit_Repeat0(self, node: Repeat0) -> None:
        if node in self.cache:
            return self.cache[node]
        name = self.gen.name_loop(node.node, False)
        self.cache[node] = name

    def visit_Repeat1(self, node: Repeat1) -> None:
        if node in self.cache:
            return self.cache[node]
        name = self.gen.name_loop(node.node, True)
        self.cache[node] = name

    def visit_Gather(self, node: Gather) -> None:
        if node in self.cache:
            return self.cache[node]
        name = self.gen.name_gather(node)
        self.cache[node] = name


class VMParserGenerator(ParserGenerator, GrammarVisitor):
    def __init__(
        self, grammar: grammar.Grammar,
    ):
        super().__init__(grammar, token.tok_name, sys.stdout)

        self.opcode_buffer: Optional[List[str]] = None
        self.callmakervisitor: VMCallMakerVisitor = VMCallMakerVisitor(self)

    @contextlib.contextmanager
    def set_opcode_buffer(self, buffer: List[str]) -> Iterator[None]:
        self.opcode_buffer = buffer
        yield
        self.opcode_buffer = None

    def add_opcode(self, opcode: str, oparg: Optional[Union[int, str]] = None) -> None:
        assert self.opcode_buffer is not None
        self.opcode_buffer.append(opcode)
        if oparg is not None:
            self.opcode_buffer.append(str(oparg))

    def name_gather(self, node: Gather) -> str:
        self.counter += 1
        name = f"_gather_{self.counter}"
        alt0 = Alt([NamedItem(None, node.node), NamedItem(None, node.separator)])
        alt1 = Alt([NamedItem(None, node.node)])
        self.todo[name] = Rule(name, None, Rhs([alt0, alt1]))
        return name

    def generate(self, filename: str) -> None:
        self.add_root_rules()
        self.collect_todo()
        self.gather_actions()
        self._setup_keywords()
        self._setup_soft_keywords()

        self.print("enum {")
        with self.indent():
            for rulename in self.todo:
                self.print(f"R_{rulename.upper()},")
        self.print("};")
        self.print()

        self.print("enum {")
        with self.indent():
            for actionname, action in self.actions.items():
                self.print(f"{actionname},")
        self.print("};")
        self.print()

        while self.todo:
            self.print("static Rule all_rules[] = {")
            for rulename, rule in list(self.todo.items()):
                del self.todo[rulename]
                with self.indent():
                    self.visit(rule)
            self.print("};")

        self.printblock(CALL_ACTION_HEAD)
        with self.indent():
            self.print_action_cases()
        self.printblock(CALL_ACTION_TAIL)

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
        self.print()

    def _setup_soft_keywords(self) -> None:
        soft_keywords = self.callmakervisitor.soft_keyword_cache
        if not soft_keywords:
            return

        self.print("enum {")
        with self.indent():
            for soft_keyword in soft_keywords:
                self.print(f"SK_{soft_keyword.upper()},")
        self.print("};")
        self.print()
        self.print("static const char *soft_keywords[] = {")
        with self.indent():
            for soft_keyword in soft_keywords:
                self.print(f'"{soft_keyword}",')
        self.print("};")
        self.print()

    def print_action_cases(self) -> None:
        unique_actions: Dict[str, List[str]] = defaultdict(list)
        for actionname, action in self.actions.items():
            unique_actions[action].append(actionname)
        for action, actionnames in unique_actions.items():
            for actionname in actionnames:
                self.print(f"case {actionname}:")
            self.print(f"    return {action};")

    def gather_actions(self) -> None:
        self.actions: Dict[str, str] = {}
        for rulename, rule in self.todo.items():
            if not rule.is_loop():
                for index, alt in enumerate(rule.rhs.alts):
                    actionname = f"A_{rulename.upper()}_{index}"
                    self.actions[actionname] = self.translate_action(alt)

    def translate_action(self, alt: Alt) -> str:
        # Given an alternative like this:
        #     | a=NAME '=' b=expr { foo(p, a, b) }
        # return a string like this:
        #     "foo(p, _f->vals[0], _f->vals[2])"
        # As a special case, if there's no action, return _f->vals[0].
        name_to_index, index = self.map_alt_names_to_vals_index(alt)
        if not alt.action:
            # TODO: Restore the assert, but expect index == 2 in Gather
            ##assert index == 1, "Alternative with >1 item must have an action"
            return "_f->vals[0]"
        # Sadly, the action is given as a string, so tokenize it back.
        # We must not substitute item names when preceded by '.' or '->'.
        # Note that Python tokenizes '->' as two separate tokens.
        res = []
        prevs = ""
        for stuff in tokenize.generate_tokens(iter([alt.action]).__next__):
            _, s, _, _, _ = stuff
            if prevs not in (".", "->"):
                i = name_to_index.get(s)
                if i is not None:
                    s = f"_f->vals[{i}]"
            if prevs == "-" and s == ">":
                prevs = "->"
            else:
                prevs = s
            res.append(s)
        return " ".join(res).strip()

    def map_alt_names_to_vals_index(self, alt: Alt) -> Tuple[Dict[str, int], int]:
        index = 0
        map: Dict[str, int] = {}
        for nameditem in alt.items:
            if nameditem.name:
                map[nameditem.name] = index
            if isinstance(nameditem.item, (Leaf, Group, Opt, Repeat)):
                index += 1
        return map, index

    def add_root_rules(self) -> None:
        assert "root" not in self.todo
        assert "root" not in self.rules
        root = RootRule("root", "start")  # TODO: determine start rules dynamically
        self.todo["root"] = root
        self.rules["root"] = root

    def visit_RootRule(self, node: RootRule) -> None:
        # TODO: Refactor visit_Rule() so we can share code.
        self.print(f'{{"{node.name}",')
        self.print(f" R_{node.name.upper()},")

        opcodes_by_alt: Dict[int, List[str]] = {0: [], 1: []}
        with self.set_opcode_buffer(opcodes_by_alt[0]):
            self.add_opcode("OP_RULE", f"R_{node.startrulename.upper()}")
            self.add_opcode("OP_SUCCESS")
        with self.set_opcode_buffer(opcodes_by_alt[1]):
            self.add_opcode("OP_FAILURE")

        indexes = [0, len(opcodes_by_alt[0]), -1]

        self.print(f" {{{', '.join(map(str, indexes))}}},")

        self.print(" {")
        with self.indent():
            for index, opcodes in opcodes_by_alt.items():
                self.print(", ".join(opcodes) + ",")
        self.print(" },")

        self.print("},")

    def visit_Rule(self, node: Rule) -> None:
        is_loop = node.is_loop()
        is_loop1 = node.name.startswith("_loop1")
        is_gather = node.is_gather()
        rhs = node.flatten()
        self.print(f'{{"{node.name}",')
        self.print(f" R_{node.name.upper()},")
        self.current_rule = node  # TODO: make this a context manager
        self.visit(rhs, is_loop=is_loop, is_loop1=is_loop1, is_gather=is_gather, is_leftrec=node.left_recursive)
        self.print("},")

    def visit_NamedItem(self, node: NamedItem) -> None:
        self.visit(node.item)

    def visit_PositiveLookahead(self, node: PositiveLookahead) -> None:
        self.add_opcode("OP_SAVE_MARK")
        self.visit(node.node)
        self.add_opcode("OP_POS_LOOKAHEAD")

    def visit_NegativeLookahead(self, node: PositiveLookahead) -> None:
        self.add_opcode("OP_SAVE_MARK")
        self.visit(node.node)
        self.add_opcode("OP_NEG_LOOKAHEAD")

    def _get_rule_opcode(self, name: str) -> str:
        return f"R_{name.upper()}"

    def visit_NameLeaf(self, node: NameLeaf) -> None:
        name = node.value
        if name in ("NAME", "NUMBER", "STRING"):
            self.add_opcode(f"OP_{name}")
        elif name in ("NEWLINE", "DEDENT", "INDENT", "ENDMARKER", "ASYNC", "AWAIT"):
            self.add_opcode("OP_TOKEN", name)
        else:
            self.add_opcode("OP_RULE", self._get_rule_opcode(name))
    def visit_StringLeaf(self, node: StringLeaf) -> None:
        op_pair = self.callmakervisitor.visit(node)
        self.add_opcode(*op_pair)

    def handle_loop_rhs(
        self, node: Rhs, opcodes_by_alt: Dict[int, List[str]], collect_opcode: str,
    ) -> None:
        self.handle_default_rhs(node, opcodes_by_alt, is_loop=True, is_gather=False)
        with self.set_opcode_buffer(opcodes_by_alt[len(node.alts)]):
            self.add_opcode(collect_opcode)

    def handle_default_rhs(
        self,
        node: Rhs,
        opcodes_by_alt: Dict[int, List[str]],
        is_loop: bool = False,
        is_loop1: bool = False,
        is_gather: bool = False,
        is_leftrec: bool = False,
    ) -> None:
        for index, alt in enumerate(node.alts):
            with self.set_opcode_buffer(opcodes_by_alt[index]):
                if is_leftrec and index == 0:
                    self.add_opcode("OP_SETUP_LEFT_REC")
                self.visit(alt, is_loop=False, is_loop1=False, is_gather=False)
                assert not (alt.action and (is_loop or is_gather))  # A loop rule can't have actions
                if is_loop or is_gather:
                    if index == 0:
                        self.add_opcode("OP_LOOP_ITERATE")
                    else:
                        self.add_opcode("OP_LOOP_COLLECT_DELIMITED")
                else:
                    opcode = "OP_RETURN"
                    if is_leftrec:
                        opcode += "_LEFT_REC"
                    self.add_opcode(opcode, f"A_{self.current_rule.name.upper()}_{index}")

    def visit_Rhs(
        self, node: Rhs, is_loop: bool = False, is_loop1: bool = False, is_gather: bool = False,
        is_leftrec: bool = False,
    ) -> None:
        opcodes_by_alt: Dict[int, List[str]] = defaultdict(list)
        if is_loop:
            opcode = "OP_LOOP_COLLECT"
            if is_loop1:
                opcode += "_NONEMPTY"
            self.handle_loop_rhs(node, opcodes_by_alt, opcode)
        else:
            self.handle_default_rhs(node, opcodes_by_alt, is_loop=is_loop, is_gather=is_gather,
                                    is_leftrec=is_leftrec)
        *indexes, _ = accumulate(map(len, opcodes_by_alt.values()))
        indexes = [0, *indexes, -1]
        self.print(f" {{{', '.join(map(str, indexes))}}},")

        self.print(" {")
        with self.indent():
            for index, opcodes in opcodes_by_alt.items():
                self.print(", ".join(opcodes) + ",")
        self.print(" },")

    def visit_Alt(self, node: Alt, is_loop: bool, is_loop1: bool, is_gather: bool) -> None:
        for item in node.items:
            self.visit(item)

    def visit_Repeat0(self, node: Repeat0) -> None:
        name = self.callmakervisitor.visit(node)
        self.add_opcode("OP_RULE", self._get_rule_opcode(name))

    def visit_Repeat1(self, node: Repeat1) -> None:
        name = self.callmakervisitor.visit(node)
        self.add_opcode("OP_RULE", self._get_rule_opcode(name))

    def visit_Gather(self, node: Gather) -> None:
        name = self.callmakervisitor.visit(node)
        self.add_opcode("OP_RULE", self._get_rule_opcode(name))


def main() -> None:
    filename = "./data/simple.gram"
    if sys.argv[1:]:
        filename = sys.argv[1]
    grammar, parser, tokenizer = build_parser(filename, False, False)
    p = VMParserGenerator(grammar)
    p.generate("")


if __name__ == "__main__":
    main()
