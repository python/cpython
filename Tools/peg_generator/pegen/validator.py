from pegen import grammar
from pegen.grammar import Alt, GrammarVisitor, Rhs, Rule


class ValidationError(Exception):
    pass


class GrammarValidator(GrammarVisitor):
    def __init__(self, grammar: grammar.Grammar) -> None:
        self.grammar = grammar
        self.rulename: str | None = None

    def validate_rule(self, rulename: str, node: Rule) -> None:
        self.rulename = rulename
        self.visit(node)
        self.rulename = None


class SubRuleValidator(GrammarValidator):
    def visit_Rhs(self, node: Rhs) -> None:
        for index, alt in enumerate(node.alts):
            alts_to_consider = node.alts[index + 1 :]
            for other_alt in alts_to_consider:
                self.check_intersection(alt, other_alt)

    def check_intersection(self, first_alt: Alt, second_alt: Alt) -> None:
        if str(second_alt).startswith(str(first_alt)):
            raise ValidationError(
                f"In {self.rulename} there is an alternative that will "
                f"never be visited:\n{second_alt}"
            )


class RaiseRuleValidator(GrammarValidator):
    def visit_Alt(self, node: Alt) -> None:
        if self.rulename and self.rulename.startswith('invalid'):
            # raising is allowed in invalid rules
            return
        if node.action and 'RAISE_SYNTAX_ERROR' in node.action:
            raise ValidationError(
                f"In {self.rulename!r} there is an alternative that contains "
                f"RAISE_SYNTAX_ERROR; this is only allowed in invalid_ rules"
            )


def validate_grammar(the_grammar: grammar.Grammar) -> None:
    for validator_cls in GrammarValidator.__subclasses__():
        validator = validator_cls(the_grammar)
        for rule_name, rule in the_grammar.rules.items():
            validator.validate_rule(rule_name, rule)
