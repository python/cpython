import unittest

from test import test_tools
from typing import Dict, Set

test_tools.skip_if_missing('peg_generator')
with test_tools.imports_under_tool('peg_generator'):
    from pegen.grammar_parser import GeneratedParser as GrammarParser
    from pegen.testutil import parse_string
    from pegen.first_sets import FirstSetCalculator
    from pegen.grammar import Grammar


class TestFirstSets(unittest.TestCase):
    def calculate_first_sets(self, grammar_source: str) -> Dict[str, Set[str]]:
        grammar: Grammar = parse_string(grammar_source, GrammarParser)
        return FirstSetCalculator(grammar.rules).calculate()

    def test_alternatives(self) -> None:
        grammar = """
            start: expr NEWLINE? ENDMARKER
            expr: A | B
            A: 'a' | '-'
            B: 'b' | '+'
        """
        self.assertEqual(self.calculate_first_sets(grammar), {
            "A": {"'a'", "'-'"},
            "B": {"'+'", "'b'"},
            "expr": {"'+'", "'a'", "'b'", "'-'"},
            "start": {"'+'", "'a'", "'b'", "'-'"},
        })

    def test_optionals(self) -> None:
        grammar = """
            start: expr NEWLINE
            expr: ['a'] ['b'] 'c'
        """
        self.assertEqual(self.calculate_first_sets(grammar), {
            "expr": {"'c'", "'a'", "'b'"},
            "start": {"'c'", "'a'", "'b'"},
        })

    def test_repeat_with_separator(self) -> None:
        grammar = """
        start: ','.thing+ NEWLINE
        thing: NUMBER
        """
        self.assertEqual(self.calculate_first_sets(grammar), {"thing": {"NUMBER"}, "start": {"NUMBER"}})

    def test_optional_operator(self) -> None:
        grammar = """
        start: sum NEWLINE
        sum: (term)? 'b'
        term: NUMBER
        """
        self.assertEqual(self.calculate_first_sets(grammar), {
            "term": {"NUMBER"},
            "sum": {"NUMBER", "'b'"},
            "start": {"'b'", "NUMBER"},
        })

    def test_optional_literal(self) -> None:
        grammar = """
        start: sum NEWLINE
        sum: '+' ? term
        term: NUMBER
        """
        self.assertEqual(self.calculate_first_sets(grammar), {
            "term": {"NUMBER"},
            "sum": {"'+'", "NUMBER"},
            "start": {"'+'", "NUMBER"},
        })

    def test_optional_after(self) -> None:
        grammar = """
        start: term NEWLINE
        term: NUMBER ['+']
        """
        self.assertEqual(self.calculate_first_sets(grammar), {"term": {"NUMBER"}, "start": {"NUMBER"}})

    def test_optional_before(self) -> None:
        grammar = """
        start: term NEWLINE
        term: ['+'] NUMBER
        """
        self.assertEqual(self.calculate_first_sets(grammar), {"term": {"NUMBER", "'+'"}, "start": {"NUMBER", "'+'"}})

    def test_repeat_0(self) -> None:
        grammar = """
        start: thing* "+" NEWLINE
        thing: NUMBER
        """
        self.assertEqual(self.calculate_first_sets(grammar), {"thing": {"NUMBER"}, "start": {'"+"', "NUMBER"}})

    def test_repeat_0_with_group(self) -> None:
        grammar = """
        start: ('+' '-')* term NEWLINE
        term: NUMBER
        """
        self.assertEqual(self.calculate_first_sets(grammar), {"term": {"NUMBER"}, "start": {"'+'", "NUMBER"}})

    def test_repeat_1(self) -> None:
        grammar = """
        start: thing+ '-' NEWLINE
        thing: NUMBER
        """
        self.assertEqual(self.calculate_first_sets(grammar), {"thing": {"NUMBER"}, "start": {"NUMBER"}})

    def test_repeat_1_with_group(self) -> None:
        grammar = """
        start: ('+' term)+ term NEWLINE
        term: NUMBER
        """
        self.assertEqual(self.calculate_first_sets(grammar), {"term": {"NUMBER"}, "start": {"'+'"}})

    def test_gather(self) -> None:
        grammar = """
        start: ','.thing+ NEWLINE
        thing: NUMBER
        """
        self.assertEqual(self.calculate_first_sets(grammar), {"thing": {"NUMBER"}, "start": {"NUMBER"}})

    def test_positive_lookahead(self) -> None:
        grammar = """
        start: expr NEWLINE
        expr: &'a' opt
        opt: 'a' | 'b' | 'c'
        """
        self.assertEqual(self.calculate_first_sets(grammar), {
            "expr": {"'a'"},
            "start": {"'a'"},
            "opt": {"'b'", "'c'", "'a'"},
        })

    def test_negative_lookahead(self) -> None:
        grammar = """
        start: expr NEWLINE
        expr: !'a' opt
        opt: 'a' | 'b' | 'c'
        """
        self.assertEqual(self.calculate_first_sets(grammar), {
            "opt": {"'b'", "'a'", "'c'"},
            "expr": {"'b'", "'c'"},
            "start": {"'b'", "'c'"},
        })

    def test_left_recursion(self) -> None:
        grammar = """
        start: expr NEWLINE
        expr: ('-' term | expr '+' term | term)
        term: NUMBER
        foo: 'foo'
        bar: 'bar'
        baz: 'baz'
        """
        self.assertEqual(self.calculate_first_sets(grammar), {
            "expr": {"NUMBER", "'-'"},
            "term": {"NUMBER"},
            "start": {"NUMBER", "'-'"},
            "foo": {"'foo'"},
            "bar": {"'bar'"},
            "baz": {"'baz'"},
        })

    def test_advance_left_recursion(self) -> None:
        grammar = """
        start: NUMBER | sign start
        sign: ['-']
        """
        self.assertEqual(self.calculate_first_sets(grammar), {"sign": {"'-'", ""}, "start": {"'-'", "NUMBER"}})

    def test_mutual_left_recursion(self) -> None:
        grammar = """
        start: foo 'E'
        foo: bar 'A' | 'B'
        bar: foo 'C' | 'D'
        """
        self.assertEqual(self.calculate_first_sets(grammar), {
            "foo": {"'D'", "'B'"},
            "bar": {"'D'"},
            "start": {"'D'", "'B'"},
        })

    def test_nasty_left_recursion(self) -> None:
        # TODO: Validate this
        grammar = """
        start: target '='
        target: maybe '+' | NAME
        maybe: maybe '-' | target
        """
        self.assertEqual(self.calculate_first_sets(grammar), {"maybe": set(), "target": {"NAME"}, "start": {"NAME"}})

    def test_nullable_rule(self) -> None:
        grammar = """
        start: sign thing $
        sign: ['-']
        thing: NUMBER
        """
        self.assertEqual(self.calculate_first_sets(grammar), {
            "sign": {"", "'-'"},
            "thing": {"NUMBER"},
            "start": {"NUMBER", "'-'"},
        })

    def test_epsilon_production_in_start_rule(self) -> None:
        grammar = """
        start: ['-'] $
        """
        self.assertEqual(self.calculate_first_sets(grammar), {"start": {"ENDMARKER", "'-'"}})

    def test_multiple_nullable_rules(self) -> None:
        grammar = """
        start: sign thing other another $
        sign: ['-']
        thing: ['+']
        other: '*'
        another: '/'
        """
        self.assertEqual(self.calculate_first_sets(grammar), {
            "sign": {"", "'-'"},
            "thing": {"'+'", ""},
            "start": {"'+'", "'-'", "'*'"},
            "other": {"'*'"},
            "another": {"'/'"},
        })
