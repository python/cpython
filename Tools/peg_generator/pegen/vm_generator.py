import ast
import contextlib
import dataclasses
import re
import sys
import token
import tokenize
from collections import defaultdict
from itertools import accumulate
from typing import Any, Dict, Iterator, List, Optional, Tuple, Set, IO, Text, Union

from pegen import grammar
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


@dataclasses.dataclass
class Opcode:
    opcode: str
    oparg: Optional[Union[int, str]] = None

    def __len__(self) -> int:
        return 1 if self.oparg is None else 2


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
        RAISE_SYNTAX_ERROR("invalid opcode");
        return NULL;
    }
}
"""


class RootRule(Rule):
    def __init__(self, name: str, startrulename: str):
        super().__init__(name, "void *", Rhs([]), None)
        self.startrulename = startrulename


class VMCallMakerVisitor(GrammarVisitor):
    def __init__(
        self,
        parser_generator: ParserGenerator,
        exact_tokens: Dict[str, int],
        non_exact_tokens: Set[str],
    ):
        self.gen = parser_generator
        self.exact_tokens = exact_tokens
        self.non_exact_tokens = non_exact_tokens
        self.cache: Dict[Any, Any] = {}
        self.keyword_cache: Dict[str, int] = {}
        self.soft_keyword_cache: List[str] = []

    def keyword_helper(self, keyword: str) -> Tuple[str, str]:
        if keyword not in self.keyword_cache:
            self.keyword_cache[keyword] = self.gen.keyword_type()
        return "OP_TOKEN", str(self.keyword_cache[keyword])

    def soft_keyword_helper(self, keyword: str) -> Tuple[str, str]:
        if keyword not in self.soft_keyword_cache:
            self.soft_keyword_cache.append(keyword)
        return "OP_SOFT_KEYWORD", f"SK_{keyword.upper()}"

    def visit_StringLeaf(self, node: StringLeaf) -> Tuple[str, str]:
        val = ast.literal_eval(node.value)
        if re.match(r"[a-zA-Z_]\w*\Z", val):  # This is a keyword
            if node.value.endswith("'"):
                return self.keyword_helper(val)
            else:
                return self.soft_keyword_helper(val)
        tok_num: int = self.exact_tokens[val]
        return "OP_TOKEN", self.gen.tokens[tok_num]

    def visit_Repeat0(self, node: Repeat0) -> str:
        if node in self.cache:
            return self.cache[node]
        name = self.gen.name_loop(node.node, False)
        self.cache[node] = name
        return name

    def visit_Repeat1(self, node: Repeat1) -> str:
        if node in self.cache:
            return self.cache[node]
        name = self.gen.name_loop(node.node, True)
        self.cache[node] = name
        return name

    def visit_Gather(self, node: Gather) -> str:
        if node in self.cache:
            return self.cache[node]
        name = self.gen.name_gather(node)
        self.cache[node] = name
        return name

    def visit_Group(self, node: Group) -> str:
        return self.visit(node.rhs)

    def visit_Rhs(self, node: Rhs) -> Optional[str]:
        if node in self.cache:
            return self.cache[node]
        if can_we_inline(node):
            return None
        name = self.gen.name_node(node)
        self.cache[node] = name
        return name


def can_we_inline(node: Rhs) -> int:
    if len(node.alts) != 1 or len(node.alts[0].items) != 1:
        return False
    # If the alternative has an action we cannot inline
    if getattr(node.alts[0], "action", None) is not None:
        return False
    return True


class VMParserGenerator(ParserGenerator, GrammarVisitor):
    def __init__(
        self,
        grammar: grammar.Grammar,
        tokens: Dict[str, int],
        exact_tokens: Dict[str, int],
        non_exact_tokens: Set[str],
        file: Optional[IO[Text]],
    ):
        super().__init__(grammar, tokens, file)

        self.opcode_buffer: Optional[List[Opcode]] = None
        self.callmakervisitor: VMCallMakerVisitor = VMCallMakerVisitor(
            self, exact_tokens, non_exact_tokens,
        )

    @contextlib.contextmanager
    def set_opcode_buffer(self, buffer: List[Opcode]) -> Iterator[None]:
        self.opcode_buffer = buffer
        yield
        self.opcode_buffer = None

    def add_opcode(self, opcode: str, oparg: Optional[str] = None) -> None:
        assert not isinstance(oparg, int)
        assert self.opcode_buffer is not None

        self.opcode_buffer.append(Opcode(opcode, oparg))

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

        self.print("static Rule all_rules[] = {")
        while self.todo:
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
        if soft_keywords:
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
            return self.make_typed_var(alt, 0)
        # Sadly, the action is given as a string, so tokenize it back.
        # We must not substitute item names when preceded by '.' or '->'.
        res = []
        prevs = ""
        for stuff in tokenize.generate_tokens(iter([alt.action]).__next__):
            _, s, _, _, _ = stuff
            if prevs not in (".", "->"):
                i = name_to_index.get(s)
                if i is not None:
                    s = self.make_typed_var(alt, i)
            res.append(s)
            prevs = s
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

    def make_typed_var(self, alt: Alt, index: int) -> str:
        var = f"_f->vals[{index}]"
        type = self.get_type_of_indexed_item(alt, index)
        if type:
            var = f"(({type}){var})"
        return var

    def get_type_of_indexed_item(self, alt: Alt, index: int) -> Optional[str]:
        item = alt.items[index].item
        if isinstance(item, NameLeaf):
            if item.value in self.rules:
                return self.rules[item.value].type
            if item.value == "NAME":
                return "expr_ty"
        if isinstance(item, StringLeaf):
            return "Token *"
        return None

    def add_root_rules(self) -> None:
        assert "root" not in self.todo
        assert "root" not in self.rules
        root = RootRule("root", "file")  # TODO: determine start rules dynamically
        self.todo["root"] = root
        self.rules["root"] = root

    def visit_RootRule(self, node: RootRule) -> None:
        self.print(f'{{"{node.name}",')
        self.print(f" R_{node.name.upper()},")
        self.print(f" 0,")
        self.print(f" 0,")

        first_alt = Alt([])
        second_alt = Alt([])

        opcodes_by_alt: Dict[Alt, List[Opcode]] = {first_alt: [], second_alt: []}
        with self.set_opcode_buffer(opcodes_by_alt[first_alt]):
            self.add_opcode("OP_RULE", f"R_{node.startrulename.upper()}")
            self.add_opcode("OP_SUCCESS")
        with self.set_opcode_buffer(opcodes_by_alt[second_alt]):
            self.add_opcode("OP_FAILURE")
        self.generate_opcodes(opcodes_by_alt)
        self.print("},")

    def visit_Rule(self, node: Rule) -> None:
        is_loop = node.is_loop()
        is_loop1 = node.name.startswith("_loop1")
        is_gather = node.is_gather()
        rhs = node.flatten()
        self.print(f'{{"{node.name}",')
        self.print(f" R_{node.name.upper()},")
        self.print(f" {int(node.memo)},  // memo")
        self.print(f" {int(node.left_recursive)},  // leftrec")
        self.current_rule = node  # TODO: make this a context manager
        self.visit(
            rhs,
            is_loop=is_loop,
            is_loop1=is_loop1,
            is_gather=is_gather,
            is_leftrec=node.left_recursive,
        )
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
        elif name in (
            "NEWLINE",
            "DEDENT",
            "INDENT",
            "ENDMARKER",
            "ASYNC",
            "AWAIT",
            "TYPE_COMMENT",
        ):
            self.add_opcode("OP_TOKEN", name)
        else:
            self.add_opcode("OP_RULE", self._get_rule_opcode(name))

    def visit_StringLeaf(self, node: StringLeaf) -> None:
        op_pair = self.callmakervisitor.visit(node)
        self.add_opcode(*op_pair)

    def visit_Cut(self, node: Cut) -> None:
        self.add_opcode("OP_CUT")

    def handle_loop_rhs(
        self, node: Rhs, opcodes_by_alt: Dict[Alt, List[Opcode]], collect_opcode: str,
    ) -> None:
        self.handle_default_rhs(node, opcodes_by_alt, is_loop=True, is_gather=False)
        loop_alt = Alt([])
        with self.set_opcode_buffer(opcodes_by_alt[loop_alt]):
            self.add_opcode(collect_opcode)

    def handle_default_rhs(
        self,
        node: Rhs,
        opcodes_by_alt: Dict[Alt, List[Opcode]],
        is_loop: bool = False,
        is_loop1: bool = False,
        is_gather: bool = False,
        is_leftrec: bool = False,
    ) -> None:
        for index, alt in enumerate(node.alts):
            with self.set_opcode_buffer(opcodes_by_alt[alt]):
                self.visit(alt, is_loop=False, is_loop1=False, is_gather=False)
                assert not (
                    alt.action and (is_loop or is_gather)
                )  # A loop rule can't have actions
                if is_loop or is_gather:
                    if index == 0:
                        self.add_opcode("OP_LOOP_ITERATE")
                    else:
                        self.add_opcode("OP_LOOP_COLLECT_DELIMITED")
                else:
                    opcode = "OP_RETURN"
                    self.add_opcode(opcode, f"A_{self.current_rule.name.upper()}_{index}")

    def visit_Rhs(
        self,
        node: Rhs,
        is_loop: bool = False,
        is_loop1: bool = False,
        is_gather: bool = False,
        is_leftrec: bool = False,
    ) -> None:
        opcodes_by_alt: Dict[Alt, List[Opcode]] = defaultdict(list)
        if is_loop:
            opcode = "OP_LOOP_COLLECT"
            if is_loop1:
                opcode += "_NONEMPTY"
            self.handle_loop_rhs(node, opcodes_by_alt, opcode)
        else:
            self.handle_default_rhs(
                node, opcodes_by_alt, is_loop=is_loop, is_gather=is_gather, is_leftrec=is_leftrec
            )
        self.generate_opcodes(opcodes_by_alt)

    def generate_opcodes(self, opcodes_by_alt: Dict[Alt, List[Opcode]]) -> None:
        *indexes, _ = accumulate(
            map(lambda opcodes: sum(map(len, opcodes)), opcodes_by_alt.values())
        )
        indexes = [0, *indexes, -1]
        self.print(f" {{{', '.join(map(str, indexes))}}},")

        self.print(" {")
        with self.indent():
            for alt, opcodes in opcodes_by_alt.items():
                self.print(f"// {alt if str(alt) else '<Artificial alternative>'}")
                for opcode in opcodes:
                    oparg_text = str(opcode.oparg) + "," if opcode.oparg else ""
                    self.print(f"{opcode.opcode}, {oparg_text}")
                self.print()
        self.print(" },")

    def visit_Alt(self, node: Alt, is_loop: bool, is_loop1: bool, is_gather: bool) -> None:
        for item in node.items:
            self.visit(item)

    def group_helper(self, node: Rhs) -> None:
        name = self.callmakervisitor.visit(node)
        if name:
            self.add_opcode("OP_RULE", self._get_rule_opcode(name))
        else:
            self.visit(node.alts[0].items[0])

    def visit_Opt(self, node: Opt) -> None:
        if isinstance(node.node, Rhs):
            self.group_helper(node.node)
        else:
            self.visit(node.node)
        self.add_opcode("OP_OPTIONAL")

    def visit_Group(self, node: Group) -> None:
        self.group_helper(node.rhs)

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
    from pegen.build import build_parser
    filename = "../../Grammar/python.gram"
    if sys.argv[1:]:
        filename = sys.argv[1]
    grammar, parser, tokenizer = build_parser(filename, False, False)
    p = VMParserGenerator(grammar)
    p.generate("")


if __name__ == "__main__":
    main()
