"""Parser for the Python metagrammar"""

import io
import tokenize  # from stdlib

from .automata import NFA, NFAState


class GrammarParser:
    """Parser for Python grammar files."""

    _translation_table = {
        tokenize.NAME: "NAME",
        tokenize.STRING: "STRING",
        tokenize.NEWLINE: "NEWLINE",
        tokenize.NL: "NL",
        tokenize.OP: "OP",
        tokenize.ENDMARKER: "ENDMARKER",
        tokenize.COMMENT: "COMMENT",
    }

    def __init__(self, grammar):
        self.grammar = grammar
        grammar_adaptor = io.StringIO(grammar)
        self.generator = tokenize.generate_tokens(grammar_adaptor.readline)
        self._gettoken()  # Initialize lookahead
        self._current_rule_name = None

    def parse(self):
        """Turn the grammar into a collection of NFAs"""
        # grammar: (NEWLINE | rule)* ENDMARKER
        while self.type != tokenize.ENDMARKER:
            while self.type == tokenize.NEWLINE:
                self._gettoken()
            # rule: NAME ':' rhs NEWLINE
            self._current_rule_name = self._expect(tokenize.NAME)
            self._expect(tokenize.OP, ":")
            a, z = self._parse_rhs()
            self._expect(tokenize.NEWLINE)

            yield NFA(a, z)

    def _parse_rhs(self):
        # rhs: items ('|' items)*
        a, z = self._parse_items()
        if self.value != "|":
            return a, z
        else:
            aa = NFAState(self._current_rule_name)
            zz = NFAState(self._current_rule_name)
            while True:
                # Allow to transit directly to the previous state and connect the end of the
                # previous state to the end of the current one, effectively allowing to skip
                # the current state.
                aa.add_arc(a)
                z.add_arc(zz)
                if self.value != "|":
                    break

                self._gettoken()
                a, z = self._parse_items()
            return aa, zz

    def _parse_items(self):
        # items: item+
        a, b = self._parse_item()
        while self.type in (tokenize.NAME, tokenize.STRING) or self.value in ("(", "["):
            c, d = self._parse_item()
            # Allow a transition between the end of the previous state
            # and the beginning of the new one, connecting all the items
            # together. In this way we can only reach the end if we visit
            # all the items.
            b.add_arc(c)
            b = d
        return a, b

    def _parse_item(self):
        # item: '[' rhs ']' | atom ['+' | '*']
        if self.value == "[":
            self._gettoken()
            a, z = self._parse_rhs()
            self._expect(tokenize.OP, "]")
            # Make a transition from the beginning to the end so it is possible to
            # advance for free to the next state of this item # without consuming
            # anything from the rhs.
            a.add_arc(z)
            return a, z
        else:
            a, z = self._parse_atom()
            value = self.value
            if value not in ("+", "*"):
                return a, z
            self._gettoken()
            z.add_arc(a)
            if value == "+":
                # Create a cycle to the beginning so we go back to the old state in this
                # item and repeat.
                return a, z
            else:
                # The end state is the same as the beginning, so we can cycle arbitrarily
                # and end in the beginning if necessary.
                return a, a

    def _parse_atom(self):
        # atom: '(' rhs ')' | NAME | STRING
        if self.value == "(":
            self._gettoken()
            a, z = self._parse_rhs()
            self._expect(tokenize.OP, ")")
            return a, z
        elif self.type in (tokenize.NAME, tokenize.STRING):
            a = NFAState(self._current_rule_name)
            z = NFAState(self._current_rule_name)
            # We can transit to the next state only if we consume the value.
            a.add_arc(z, self.value)
            self._gettoken()
            return a, z
        else:
            self._raise_error(
                "expected (...) or NAME or STRING, got {} ({})",
                self._translation_table.get(self.type, self.type),
                self.value,
            )

    def _expect(self, type_, value=None):
        if self.type != type_:
            self._raise_error(
                "expected {}, got {} ({})",
                self._translation_table.get(type_, type_),
                self._translation_table.get(self.type, self.type),
                self.value,
            )
        if value is not None and self.value != value:
            self._raise_error("expected {}, got {}", value, self.value)
        value = self.value
        self._gettoken()
        return value

    def _gettoken(self):
        tup = next(self.generator)
        while tup[0] in (tokenize.COMMENT, tokenize.NL):
            tup = next(self.generator)
        self.type, self.value, self.begin, self.end, self.line = tup

    def _raise_error(self, msg, *args):
        if args:
            try:
                msg = msg.format(*args)
            except Exception:
                msg = " ".join([msg] + list(map(str, args)))
        line = self.grammar.splitlines()[self.begin[0] - 1]
        raise SyntaxError(msg, ("<grammar>", self.begin[0], self.begin[1], line))
