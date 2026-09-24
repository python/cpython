"""Discover calls and helper rules, then prepare rules from resolved calls."""

import ast
import re
from collections.abc import Mapping
from dataclasses import replace
from types import MappingProxyType
from typing import TYPE_CHECKING, Any

from pegen.c_generator_model import (
    CAction,
    CAlternative,
    CBindingKind,
    CPrefix,
    CRule,
    CRuleSignature,
    CVariable,
)
from pegen.c_generator_model import (
    FunctionCall as FunctionCall,
)
from pegen.c_generator_model import (
    NodeTypes as NodeTypes,
)
from pegen.grammar import (
    Alt,
    Cut,
    Forced,
    Gather,
    GrammarVisitor,
    Group,
    Item,
    Leaf,
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
    RuleKind,
    StringLeaf,
)

if TYPE_CHECKING:
    from pegen.parser_generator import ParserGenerator


BASE_NODETYPES = {
    "NAME": NodeTypes.NAME_TOKEN,
    "NUMBER": NodeTypes.NUMBER_TOKEN,
    "STRING": NodeTypes.STRING_TOKEN,
    "SOFT_KEYWORD": NodeTypes.SOFT_KEYWORD,
}

_HelperNode = Rhs | Repeat0 | Repeat1 | Gather


def rule_signature(rule: Rule) -> CRuleSignature:
    return_type = rule.type if rule.kind is RuleKind.NORMAL else "asdl_seq *"
    return CRuleSignature(rule.name, rule.kind, return_type)


def bind_call(node: NamedItem, call: FunctionCall) -> FunctionCall:
    if not node.name and not node.type:
        return call
    return replace(
        call,
        assigned_variable=node.name or call.assigned_variable,
        assigned_variable_type=node.type or call.assigned_variable_type,
        binding_kind=CBindingKind.NORMAL if node.name else call.binding_kind,
    )


def consuming_rules(rules: dict[str, Rule]) -> set[str]:
    """Conservatively prove which rules consume a token whenever they succeed."""
    consuming: set[str] = set()

    def consumes(node: Any) -> bool:
        if isinstance(node, NamedItem):
            return consumes(node.item)
        if isinstance(node, NameLeaf):
            return node.value not in rules or node.value in consuming
        if isinstance(node, StringLeaf):
            return True
        if isinstance(node, Group):
            return consumes(node.rhs)
        if isinstance(node, Rhs):
            return bool(node.alts) and all(any(consumes(i) for i in alt.items) for alt in node.alts)
        if isinstance(node, (Forced, Repeat1, Gather)):
            return consumes(node.node)
        # Predicates, cuts, optional items, and zero-or-more items can succeed
        # without consuming. Actions are assumed not to rewrite parser marks.
        return False

    while True:
        added = {name for name, rule in rules.items()
                 if name not in consuming and consumes(rule.rhs)}
        if not added:
            return consuming
        consuming.update(added)


class CCallMakerVisitor(GrammarVisitor):
    def __init__(
        self,
        parser_generator: "ParserGenerator",
        exact_tokens: dict[str, int],
        non_exact_tokens: set[str],
    ):
        self._registry = parser_generator
        self._keywords = parser_generator.keywords
        self._exact_tokens = exact_tokens
        self._non_exact_tokens = non_exact_tokens
        self._helper_cache: dict[tuple[type, str], str] = {}
        self._calls: dict[NamedItem, tuple[Item, FunctionCall]] = {}

    def visit(self, node: Any, *args: Any, **kwargs: Any) -> FunctionCall:
        match node:
            case NamedItem(item=item):
                call = self.visit(item)
                self._calls[node] = (item, call)
                return bind_call(node, call)
            case NameLeaf():
                return self._name_call(node)
            case StringLeaf():
                return self._string_call(node)
            case PositiveLookahead():
                return self._lookahead_call(node, 1)
            case NegativeLookahead():
                return self._lookahead_call(node, 0)
            case Forced():
                return self._forced_call(node)
            case Opt():
                return self._optional_call(node)
            case Rhs(can_be_inlined=True):
                return self.visit(node.alts[0].items[0])
            case Rhs() | Repeat0() | Repeat1() | Gather():
                return self._helper_call(node)
            case Group(rhs=rhs):
                return self.visit(rhs)
            case Cut():
                return FunctionCall(
                    assigned_variable="_cut_var",
                    return_type="int",
                    function="1",
                    nodetype=NodeTypes.CUT_OPERATOR,
                    binding_kind=CBindingKind.CUT,
                )
            case _:
                return self.generic_visit(node, *args, **kwargs)

    def _keyword_call(self, keyword: str) -> FunctionCall:
        return FunctionCall(
            assigned_variable="_keyword",
            function="_PyPegen_expect_token",
            arguments=("p", self._keywords[keyword]),
            return_type="Token *",
            nodetype=NodeTypes.KEYWORD,
            comment=f"token='{keyword}'",
        )

    def _soft_keyword_call(self, value: str) -> FunctionCall:
        return FunctionCall(
            assigned_variable="_keyword",
            function="_PyPegen_expect_soft_keyword",
            arguments=("p", value),
            return_type="expr_ty",
            nodetype=NodeTypes.SOFT_KEYWORD,
            comment=f"soft_keyword='{value}'",
        )

    def _name_call(self, node: NameLeaf) -> FunctionCall:
        name = node.value
        if name in self._non_exact_tokens:
            if name in BASE_NODETYPES:
                return FunctionCall(
                    assigned_variable=f"{name.lower()}_var",
                    function=f"_PyPegen_{name.lower()}_token",
                    arguments=("p",),
                    nodetype=BASE_NODETYPES[name],
                    return_type="expr_ty",
                    comment=name,
                )
            return FunctionCall(
                assigned_variable=f"{name.lower()}_var",
                function="_PyPegen_expect_token",
                arguments=("p", name),
                nodetype=NodeTypes.GENERIC_TOKEN,
                return_type="Token *",
                comment=f"token='{name}'",
            )

        type = None
        if (signature := self._lookup_rule(name)) is not None:
            type = signature.return_type

        return FunctionCall(
            assigned_variable=f"{name}_var",
            function=f"{name}_rule",
            arguments=("p",),
            return_type=type,
            comment=f"{node}",
        )

    def _string_call(self, node: StringLeaf) -> FunctionCall:
        val = ast.literal_eval(node.value)
        if re.match(r"[a-zA-Z_]\w*\Z", val):  # This is a keyword
            if node.value.endswith("'"):
                return self._keyword_call(val)
            else:
                return self._soft_keyword_call(node.value)
        else:
            assert val in self._exact_tokens, f"{node.value} is not a known literal"
            type = self._exact_tokens[val]
            return FunctionCall(
                assigned_variable="_literal",
                function="_PyPegen_expect_token",
                arguments=("p", type),
                nodetype=NodeTypes.GENERIC_TOKEN,
                return_type="Token *",
                comment=f"token='{val}'",
            )

    def _assert_compatible_return_type(
        self, call: FunctionCall, wrapper: str, expected_rtype: str | None,
    ) -> None:
        if call.return_type != expected_rtype:
            raise RuntimeError(
                f"{call.function} return type is incompatible with {wrapper}: "
                f"expect: {expected_rtype}, actual: {call.return_type}"
            )

    def _lookahead_call(self, node: Lookahead, positive: int) -> FunctionCall:
        call = self.visit(node.node)
        comment = None
        match call:
            case FunctionCall(nodetype=NodeTypes.NAME_TOKEN):
                function = "_PyPegen_lookahead_for_expr"
                self._assert_compatible_return_type(call, function, "expr_ty")
            case FunctionCall(nodetype=NodeTypes.STRING_TOKEN):
                # _PyPegen_string_token() returns 'void *' instead of 'Token *';
                # in addition, the overall function call would return 'expr_ty'.
                assert call.function == "_PyPegen_string_token"
                function = "_PyPegen_lookahead"
                self._assert_compatible_return_type(call, function, "expr_ty")
            case FunctionCall(nodetype=NodeTypes.SOFT_KEYWORD):
                function = "_PyPegen_lookahead_with_string"
                self._assert_compatible_return_type(call, function, "expr_ty")
            case FunctionCall(nodetype=NodeTypes.GENERIC_TOKEN | NodeTypes.KEYWORD):
                function = "_PyPegen_lookahead_with_int"
                self._assert_compatible_return_type(call, function, "Token *")
                comment = f"token={node.node}"
            case FunctionCall(return_type="expr_ty"):
                function = "_PyPegen_lookahead_for_expr"
            case FunctionCall(return_type="stmt_ty"):
                function = "_PyPegen_lookahead_for_stmt"
            case _:
                function = "_PyPegen_lookahead"
                self._assert_compatible_return_type(call, function, None)
        return FunctionCall(
            function=function,
            arguments=(positive, call.function, *call.arguments),
            return_type="int",
            comment=comment,
        )

    def _forced_call(self, node: Forced) -> FunctionCall:
        call = self.visit(node.node)
        match node.node:
            case Leaf(value=value):
                val = ast.literal_eval(value)
                assert val in self._exact_tokens, f"{value} is not a known literal"
                return FunctionCall(
                    assigned_variable="_literal",
                    function="_PyPegen_expect_forced_token",
                    arguments=("p", self._exact_tokens[val], f'"{val}"'),
                    nodetype=NodeTypes.GENERIC_TOKEN,
                    return_type="Token *",
                    comment=f"forced_token='{val}'",
                )
            case Group(rhs=rhs):
                return FunctionCall(
                    assigned_variable="_literal",
                    function="_PyPegen_expect_forced_result",
                    arguments=("p", call.expression(), f'"{rhs!s}"'),
                    return_type="void *",
                    comment=f"forced_token=({rhs!s})",
                )
            case _:
                raise NotImplementedError(f"Forced tokens don't work with {node.node} nodes")

    def _optional_call(self, node: Opt) -> FunctionCall:
        call = self.visit(node.node)
        return FunctionCall(
            assigned_variable="_opt_var",
            function=call.function,
            arguments=call.arguments,
            force_true=True,
            comment=f"{node}",
            binding_kind=CBindingKind.OPTIONAL,
        )

    def _helper_call(
        self,
        node: _HelperNode,
    ) -> FunctionCall:
        node_str = f"{node}"
        signature = self._resolve_artificial_rule(node)
        name = signature.name
        return FunctionCall(
            assigned_variable=f"{name}_var",
            function=f"{name}_rule",
            arguments=("p",),
            return_type=signature.return_type,
            comment=node_str,
        )

    def _lookup_rule(self, name: str) -> CRuleSignature | None:
        if (rule := self._registry.all_rules.get(name.lower())) is not None:
            return rule_signature(rule)
        return None

    def _resolve_artificial_rule(self, node: _HelperNode) -> CRuleSignature:
        # Preserve helper reuse and numbering from the fixed-point traversal.
        key = (type(node), str(node))
        if (name := self._helper_cache.get(key)) is None:
            match node:
                case Rhs():
                    name = self._registry.artificial_rule_from_rhs(node)
                case Repeat0(node=child):
                    name = self._registry.artificial_rule_from_repeat(child, is_repeat1=False)
                case Repeat1(node=child):
                    name = self._registry.artificial_rule_from_repeat(child, is_repeat1=True)
                case Gather():
                    name = self._registry.artificial_rule_from_gather(node)
            self._helper_cache[key] = name
        return rule_signature(self._registry.all_rules[name])

    def make_lowerer(self) -> "CCallLowerer":
        return CCallLowerer(
            calls=self._calls,
            rules=self._registry.all_rules,
            original_rules=self._registry.rules,
            signatures={
                name: rule_signature(rule) for name, rule in self._registry.all_rules.items()
            },
        )


class CCallLowerer:
    """Resolve bindings, actions and control flow without registering rules.

    Discovery and lowering operate on the same, unchanged grammar.
    """

    def __init__(
        self,
        *,
        calls: Mapping[NamedItem, tuple[Item, FunctionCall]],
        rules: Mapping[str, Rule],
        original_rules: dict[str, Rule],
        signatures: Mapping[str, CRuleSignature],
    ):
        self._calls = MappingProxyType(dict(calls))
        self._signatures = MappingProxyType(dict(signatures))
        self._prefixes: dict[str, tuple[CPrefix, ...]] = {}
        self._prefix_calls: dict[NamedItem, CPrefix] = {}
        consuming = consuming_rules(original_rules)
        counter = 0

        def candidate(alt: Alt) -> Rule | None:
            if not alt.items or not isinstance(alt.items[0].item, NameLeaf):
                return None
            rule = original_rules.get(alt.items[0].item.value)
            if rule is None or rule.name not in consuming:
                return None
            if ("memo" in rule.flags and not rule.left_recursive) or (
                rule.left_recursive and rule.leader
            ):
                return rule
            return None

        for rule in rules.values():
            if rule.kind in {RuleKind.LOOP0, RuleKind.LOOP1}:
                continue
            # Reuse a consuming prefix only within a consecutive group.
            # Diagnostic calls still invoke the original rule.
            prefixes = []
            alts = rule.flatten().alts
            i = 0
            while i < len(alts):
                prefix_rule = candidate(alts[i])
                j = i + 1
                while prefix_rule is not None and j < len(alts) and candidate(alts[j]) is prefix_rule:
                    j += 1
                if prefix_rule is not None and j - i > 1:
                    prefix = CPrefix(f"_prefix_{counter}", prefix_rule.type or "void *")
                    counter += 1
                    prefixes.append(prefix)
                    for alt in alts[i:j]:
                        self._prefix_calls[alt.items[0]] = prefix
                i = j
            self._prefixes[rule.name] = tuple(prefixes)

    def prepare_rule(self, rule: Rule, *, skip_actions: bool = False) -> CRule:
        if (signature := self._signatures.get(rule.name)) is None:
            raise RuntimeError(f"Rule {rule.name!r} was not discovered")
        rhs = rule.flatten()
        if signature.kind in {RuleKind.LOOP0, RuleKind.LOOP1}:
            assert len(rhs.alts) == 1
        return CRule(
            signature=signature,
            text=str(rule),
            alternatives=tuple(
                self.prepare_alt(alt, kind=signature.kind, skip_actions=skip_actions)
                for alt in rhs.alts
            ),
            left_recursive=rule.left_recursive,
            leader=rule.leader,
            memoize="memo" in rule.flags and not rule.left_recursive,
            disable_invalid_rules=rule.name.endswith("without_invalid"),
            prefixes=self._prefixes.get(rule.name, ()),
        )

    def prepare_alt(
        self,
        node: Alt,
        *,
        kind: RuleKind = RuleKind.NORMAL,
        skip_actions: bool = False,
    ) -> CAlternative:
        calls: list[FunctionCall] = []
        variables: dict[str, CVariable] = {}
        cut_variable = None
        for item in node.items:
            recorded = self._calls.get(item)
            if recorded is None or recorded[0] is not item.item:
                raise RuntimeError(f"Item {item} was not discovered")
            call = bind_call(item, recorded[1])
            if (prefix := self._prefix_calls.get(item)) is not None:
                result, end, valid = prefix.result, prefix.end, prefix.valid
                original = call.expression()
                call = replace(
                    call,
                    function=(
                        f"((!p->call_invalid_rules && {valid}) ? "
                        f"(p->mark = {end}, {result}) : "
                        f"({result} = {original}, {end} = p->mark, {valid} = 1, {result}))"
                    ),
                    arguments=(),
                )
            if original_name := call.assigned_variable:
                name = original_name
                counter = 0
                while name in variables:
                    counter += 1
                    name = f"{original_name}_{counter}"
                if name != original_name:
                    call = replace(call, assigned_variable=name)
                initializer = (
                    "0" if call.binding_kind is CBindingKind.CUT and cut_variable is None
                    else None
                )
                if initializer is not None:
                    cut_variable = name
                variables[name] = CVariable(
                    name=name,
                    type=call.return_type if item.type is None else item.type,
                    initializer=initializer,
                    unused=call.binding_kind is CBindingKind.OPTIONAL,
                )
            calls.append(call)
        return CAlternative(
            text=str(node),
            action=self._prepare_action(node, list(variables), kind, skip_actions),
            calls=tuple(calls),
            variables=tuple(variables.values()),
            cut_variable=cut_variable,
            requires_invalid_rules=self._requires_invalid_rules(node),
            uses_locations=bool(node.action and "EXTRA" in node.action),
        )

    def _requires_invalid_rules(self, node: Alt) -> bool:
        match node.items:
            case [NamedItem(item=item)]:
                pass
            case _:
                return False
        # Preserve the source convention for bare, optional and repeated
        # invalid references, including an invalid gather separator.
        while True:
            match item:
                case Rhs(alts=[Alt(items=[NamedItem(item=child)])]):
                    item = child
                case Gather(separator=separator):
                    item = separator
                case Opt(node=child) | Repeat0(node=child) | Repeat1(node=child):
                    # A compound optional may match empty, so an invalid
                    # reference inside it must not gate the whole alternative.
                    if self._is_compound(child):
                        return False
                    item = child
                case NameLeaf(value=name):
                    return name.startswith("invalid_")
                case _:
                    return False

    def _is_compound(self, item: Item) -> bool:
        match item:
            case Rhs(alts=alts):
                return len(alts) > 1 or any(
                    len(alt.items) > 1 or any(self._is_compound(part.item) for part in alt.items)
                    for alt in alts
                )
            case Group(rhs=rhs):
                return self._is_compound(rhs)
            case Gather(separator=separator, node=child):
                return self._is_compound(separator) or self._is_compound(child)
            case (
                Opt(node=child) | Repeat0(node=child) | Repeat1(node=child)
                | Forced(node=child) | Lookahead(node=child)
            ):
                return self._is_compound(child)
            case StringLeaf(value=value):
                return " " in value
            case _:
                return False

    @staticmethod
    def _prepare_action(
        node: Alt, names: list[str], kind: RuleKind, skip_actions: bool,
    ) -> CAction:
        if skip_actions:
            return CAction("_PyPegen_dummy_name(p)")
        if action := node.action:
            return CAction(
                action, checked=True, debug_message="Hit with action [%d-%d]: %s",
            )
        match names:
            case [first, rest] if kind is RuleKind.GATHER:
                return CAction(f"_PyPegen_seq_insert_in_front(p, {first}, {rest})")
            case [_, _, *_]:
                assert kind is not RuleKind.GATHER
                return CAction(
                    f"_PyPegen_dummy_name(p, {', '.join(names)})",
                    debug_message="Hit without action [%d:%d]: %s",
                )
            case _:
                return CAction(names[0], debug_message="Hit with default action [%d:%d]: %s")
