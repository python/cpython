import unittest

from test import test_tools

test_tools.skip_if_missing("peg_generator")
with test_tools.imports_under_tool("peg_generator"):
    from pegen.c_generator import consuming_rules
    from pegen.testutil import GrammarParser, parse_string


class ConsumingRuleTests(unittest.TestCase):
    def test_predicates_cuts_and_nullable_repeats(self):
        grammar = parse_string("""
        start: NAME ENDMARKER
        positive: &NAME
        negative: !NAME
        cut: ~ { _PyPegen_dummy_name(p) }
        optional: [NAME]
        empty_repeat: NAME*
        nullable_repeat: optional+
        consuming_repeat: NAME+
        """, GrammarParser)
        self.assertEqual(consuming_rules(grammar.rules), {'start', 'consuming_repeat'})

    def test_fixed_point_and_mixed_alternatives(self):
        grammar = parse_string("""
        start: expression ENDMARKER
        expression: expression '+' term | term
        term: atom
        atom: NAME | '(' expression ')'
        nullable: NAME | &NAME
        """, GrammarParser)
        self.assertEqual(consuming_rules(grammar.rules), {'start', 'expression', 'term', 'atom'})
