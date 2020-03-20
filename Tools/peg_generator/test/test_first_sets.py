from typing import Set, Dict

from pegen.first_sets import FirstSetCalculator
from pegen.grammar import Grammar
from pegen.grammar_parser import GeneratedParser as GrammarParser
from pegen.testutil import parse_string


def calculate_first_sets(grammar_source: str) -> Dict[str, Set[str]]:
    grammar: Grammar = parse_string(grammar_source, GrammarParser)
    return FirstSetCalculator(grammar.rules).calculate()


def test_alternatives() -> None:
    grammar = """
        start: expr NEWLINE? ENDMARKER
        expr: A | B
        A: 'a' | '-'
        B: 'b' | '+'
    """
    assert calculate_first_sets(grammar) == {
        "A": {"'a'", "'-'"},
        "B": {"'+'", "'b'"},
        "expr": {"'+'", "'a'", "'b'", "'-'"},
        "start": {"'+'", "'a'", "'b'", "'-'"},
    }


def test_optionals() -> None:
    grammar = """
        start: expr NEWLINE
        expr: ['a'] ['b'] 'c'
    """
    assert calculate_first_sets(grammar) == {
        "expr": {"'c'", "'a'", "'b'"},
        "start": {"'c'", "'a'", "'b'"},
    }


def test_repeat_with_separator() -> None:
    grammar = """
    start: ','.thing+ NEWLINE
    thing: NUMBER
    """
    assert calculate_first_sets(grammar) == {"thing": {"NUMBER"}, "start": {"NUMBER"}}


def test_optional_operator() -> None:
    grammar = """
    start: sum NEWLINE
    sum: (term)? 'b'
    term: NUMBER
    """
    assert calculate_first_sets(grammar) == {
        "term": {"NUMBER"},
        "sum": {"NUMBER", "'b'"},
        "start": {"'b'", "NUMBER"},
    }


def test_optional_literal() -> None:
    grammar = """
    start: sum NEWLINE
    sum: '+' ? term 
    term: NUMBER
    """
    assert calculate_first_sets(grammar) == {
        "term": {"NUMBER"},
        "sum": {"'+'", "NUMBER"},
        "start": {"'+'", "NUMBER"},
    }


def test_optional_after() -> None:
    grammar = """
    start: term NEWLINE
    term: NUMBER ['+']
    """
    assert calculate_first_sets(grammar) == {"term": {"NUMBER"}, "start": {"NUMBER"}}


def test_optional_before() -> None:
    grammar = """
    start: term NEWLINE
    term: ['+'] NUMBER
    """
    assert calculate_first_sets(grammar) == {"term": {"NUMBER", "'+'"}, "start": {"NUMBER", "'+'"}}


def test_repeat_0() -> None:
    grammar = """
    start: thing* "+" NEWLINE
    thing: NUMBER
    """
    assert calculate_first_sets(grammar) == {"thing": {"NUMBER"}, "start": {'"+"', "NUMBER"}}


def test_repeat_0_with_group() -> None:
    grammar = """
    start: ('+' '-')* term NEWLINE
    term: NUMBER
    """
    assert calculate_first_sets(grammar) == {"term": {"NUMBER"}, "start": {"'+'", "NUMBER"}}


def test_repeat_1() -> None:
    grammar = """
    start: thing+ '-' NEWLINE
    thing: NUMBER
    """
    assert calculate_first_sets(grammar) == {"thing": {"NUMBER"}, "start": {"NUMBER"}}


def test_repeat_1_with_group() -> None:
    grammar = """
    start: ('+' term)+ term NEWLINE
    term: NUMBER
    """
    assert calculate_first_sets(grammar) == {"term": {"NUMBER"}, "start": {"'+'"}}


def test_gather() -> None:
    grammar = """
    start: ','.thing+ NEWLINE
    thing: NUMBER
    """
    assert calculate_first_sets(grammar) == {"thing": {"NUMBER"}, "start": {"NUMBER"}}


def test_positive_lookahead() -> None:
    grammar = """
    start: expr NEWLINE
    expr: &'a' opt
    opt: 'a' | 'b' | 'c'
    """
    assert calculate_first_sets(grammar) == {
        "expr": {"'a'"},
        "start": {"'a'"},
        "opt": {"'b'", "'c'", "'a'"},
    }


def test_negative_lookahead() -> None:
    grammar = """
    start: expr NEWLINE
    expr: !'a' opt
    opt: 'a' | 'b' | 'c'
    """
    assert calculate_first_sets(grammar) == {
        "opt": {"'b'", "'a'", "'c'"},
        "expr": {"'b'", "'c'"},
        "start": {"'b'", "'c'"},
    }


def test_left_recursion() -> None:
    grammar = """
    start: expr NEWLINE
    expr: ('-' term | expr '+' term | term)
    term: NUMBER
    foo: 'foo'
    bar: 'bar'
    baz: 'baz'
    """
    assert calculate_first_sets(grammar) == {
        "expr": {"NUMBER", "'-'"},
        "term": {"NUMBER"},
        "start": {"NUMBER", "'-'"},
        "foo": {"'foo'"},
        "bar": {"'bar'"},
        "baz": {"'baz'"},
    }


def test_advance_left_recursion() -> None:
    grammar = """
    start: NUMBER | sign start
    sign: ['-']
    """
    assert calculate_first_sets(grammar) == {"sign": {"'-'", ""}, "start": {"'-'", "NUMBER"}}


def test_mutual_left_recursion() -> None:
    grammar = """
    start: foo 'E'
    foo: bar 'A' | 'B'
    bar: foo 'C' | 'D'
    """
    assert calculate_first_sets(grammar) == {
        "foo": {"'D'", "'B'"},
        "bar": {"'D'"},
        "start": {"'D'", "'B'"},
    }


def test_nasty_left_recursion() -> None:
    # TODO: Validate this
    grammar = """
    start: target '='
    target: maybe '+' | NAME
    maybe: maybe '-' | target
    """
    assert calculate_first_sets(grammar) == {"maybe": set(), "target": {"NAME"}, "start": {"NAME"}}


def test_nullable_rule() -> None:
    grammar = """
    start: sign thing $
    sign: ['-']
    thing: NUMBER
    """
    assert calculate_first_sets(grammar) == {
        "sign": {"", "'-'"},
        "thing": {"NUMBER"},
        "start": {"NUMBER", "'-'"},
    }


def test_epsilon_production_in_start_rule() -> None:
    grammar = """
    start: ['-'] $
    """
    assert calculate_first_sets(grammar) == {"start": {"ENDMARKER", "'-'"}}


def test_multiple_nullable_rules() -> None:
    grammar = """
    start: sign thing other another $
    sign: ['-']
    thing: ['+']
    other: '*'
    another: '/'
    """
    assert calculate_first_sets(grammar) == {
        "sign": {"", "'-'"},
        "thing": {"'+'", ""},
        "start": {"'+'", "'-'", "'*'"},
        "other": {"'*'"},
        "another": {"'/'"},
    }
