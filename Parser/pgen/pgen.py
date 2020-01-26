"""Python parser generator


This parser generator transforms a Python grammar file into parsing tables
that can be consumed by Python's LL(1) parser written in C.

Concepts
--------

* An LL(1) parser (Left-to-right, Leftmost derivation, 1 token-lookahead) is a
  top-down parser for a subset of context-free languages. It parses the input
  from Left to right, performing Leftmost derivation of the sentence, and can
  only use 1 token of lookahead when parsing a sentence.

* A parsing table is a collection of data that a generic implementation of the
  LL(1) parser consumes to know how to parse a given context-free grammar. In
  this case the collection of data involves Deterministic Finite Automatons,
  calculated first sets, keywords and transition labels.

* A grammar is defined by production rules (or just 'productions') that specify
  which symbols may replace which other symbols; these rules may be used to
  generate strings, or to parse them. Each such rule has a head, or left-hand
  side, which consists of the string that may be replaced, and a body, or
  right-hand side, which consists of a string that may replace it. In the
  Python grammar, rules are written in the form

  rule_name: rule_description;

  meaning the rule 'a: b' specifies that a can be replaced by b. A context-free
  grammar is a grammar in which the left-hand side of each production rule
  consists of only a single nonterminal symbol. Context-free grammars can
  always be recognized by a Non-Deterministic Automatons.

* Terminal symbols are literal symbols which may appear in the outputs of the
  production rules of the grammar and which cannot be changed using the rules
  of the grammar. Applying the rules recursively to a source string of symbols
  will usually terminate in a final output string consisting only of terminal
  symbols.

* Nonterminal symbols are those symbols which can be replaced. The grammar
  includes a start symbol a designated member of the set of nonterminals from
  which all the strings in the language may be derived by successive
  applications of the production rules.

* The language defined by the grammar is defined as the set of terminal strings
  that can be derived using the production rules.

* The first sets of a rule (FIRST(rule)) are defined to be the set of terminals
  that can appear in the first position of any string derived from the rule.
  This is useful for LL(1) parsers as the parser is only allowed to look at the
  next token in the input to know which rule needs to parse. For example, given
  this grammar:

  start: '(' A | B ')'
  A: 'a' '<'
  B: 'b' '<'

  and the input '(b<)' the parser can only look at 'b' to know if it needs
  to parse A o B. Because FIRST(A) = {'a'} and FIRST(B) = {'b'} it knows
  that needs to continue parsing rule B because only that rule can start
  with 'b'.

Description
-----------

The input for the parser generator is a grammar in extended BNF form (using *
for repetition, + for at-least-once repetition, [] for optional parts, | for
alternatives and () for grouping).

Each rule in the grammar file is considered as a regular expression in its
own right. It is turned into a Non-deterministic Finite Automaton (NFA),
which is then turned into a Deterministic Finite Automaton (DFA), which is
then optimized to reduce the number of states. See [Aho&Ullman 77] chapter 3,
or similar compiler books (this technique is more often used for lexical
analyzers).

The DFA's are used by the parser as parsing tables in a special way that's
probably unique. Before they are usable, the FIRST sets of all non-terminals
are computed so the LL(1) parser consuming the parsing tables can distinguish
between different transitions.
Reference
---------

[Aho&Ullman 77]
    Aho&Ullman, Principles of Compiler Design, Addison-Wesley 1977
    (first edition)
"""

from ast import literal_eval
import collections

from . import grammar, token
from .automata import DFA
from .metaparser import GrammarParser

import enum


class LabelType(enum.Enum):
    NONTERMINAL = 0
    NAMED_TOKEN = 1
    KEYWORD = 2
    OPERATOR = 3
    NONE = 4


class Label(str):
    def __init__(self, value):
        self.type = self._get_type()

    def _get_type(self):
        if self[0].isalpha():
            if self.upper() == self:
                # NAMED tokens (ASYNC, NAME...) are all uppercase by convention
                return LabelType.NAMED_TOKEN
            else:
                # If is not uppercase it must be a non terminal.
                return LabelType.NONTERMINAL
        else:
            # Keywords and operators are wrapped in quotes
            assert self[0] == self[-1] in ('"', "'"), self
            value = literal_eval(self)
            if value[0].isalpha():
                return LabelType.KEYWORD
            else:
                return LabelType.OPERATOR

    def __repr__(self):
        return "{}({})".format(self.type, super().__repr__())


class ParserGenerator(object):
    def __init__(self, grammar_file, token_file, verbose=False, graph_file=None):
        with open(grammar_file) as f:
            self.grammar = f.read()
        with open(token_file) as tok_file:
            token_lines = tok_file.readlines()
        self.tokens = dict(token.generate_tokens(token_lines))
        self.opmap = dict(token.generate_opmap(token_lines))
        # Manually add <> so it does not collide with !=
        self.opmap["<>"] = "NOTEQUAL"
        self.verbose = verbose
        self.filename = grammar_file
        self.graph_file = graph_file
        self.dfas, self.startsymbol = self.create_dfas()
        self.first = {}  # map from symbol name to set of tokens
        self.calculate_first_sets()

    def create_dfas(self):
        rule_to_dfas = collections.OrderedDict()
        start_nonterminal = None
        for nfa in GrammarParser(self.grammar).parse():
            if self.verbose:
                print("Dump of NFA for", nfa.name)
                nfa.dump()
            if self.graph_file is not None:
                nfa.dump_graph(self.graph_file.write)
            dfa = DFA.from_nfa(nfa)
            if self.verbose:
                print("Dump of DFA for", dfa.name)
                dfa.dump()
            dfa.simplify()
            if self.graph_file is not None:
                dfa.dump_graph(self.graph_file.write)
            rule_to_dfas[dfa.name] = dfa

            if start_nonterminal is None:
                start_nonterminal = dfa.name

        return rule_to_dfas, start_nonterminal

    def make_grammar(self):
        c = grammar.Grammar()
        c.all_labels = set()
        names = list(self.dfas.keys())
        names.remove(self.startsymbol)
        names.insert(0, self.startsymbol)
        for name in names:
            i = 256 + len(c.symbol2number)
            c.symbol2number[Label(name)] = i
            c.number2symbol[i] = Label(name)
            c.all_labels.add(name)
        for name in names:
            self.make_label(c, name)
            dfa = self.dfas[name]
            states = []
            for state in dfa:
                arcs = []
                for label, next in sorted(state.arcs.items()):
                    c.all_labels.add(label)
                    arcs.append((self.make_label(c, label), dfa.states.index(next)))
                if state.is_final:
                    arcs.append((0, dfa.states.index(state)))
                states.append(arcs)
            c.states.append(states)
            c.dfas[c.symbol2number[name]] = (states, self.make_first_sets(c, name))
        c.start = c.symbol2number[self.startsymbol]

        if self.verbose:
            print("")
            print("Grammar summary")
            print("===============")

            print("- {n_labels} labels".format(n_labels=len(c.labels)))
            print("- {n_dfas} dfas".format(n_dfas=len(c.dfas)))
            print("- {n_tokens} tokens".format(n_tokens=len(c.tokens)))
            print("- {n_keywords} keywords".format(n_keywords=len(c.keywords)))
            print(
                "- Start symbol: {start_symbol}".format(
                    start_symbol=c.number2symbol[c.start]
                )
            )
        return c

    def make_first_sets(self, c, name):
        rawfirst = self.first[name]
        first = set()
        for label in sorted(rawfirst):
            ilabel = self.make_label(c, label)
            ##assert ilabel not in first # XXX failed on <> ... !=
            first.add(ilabel)
        return first

    def make_label(self, c, label):
        label = Label(label)
        ilabel = len(c.labels)

        if label.type == LabelType.NONTERMINAL:
            if label in c.symbol2label:
                return c.symbol2label[label]
            else:
                c.labels.append((c.symbol2number[label], None))
                c.symbol2label[label] = ilabel
                return ilabel
        elif label.type == LabelType.NAMED_TOKEN:
            # A named token (NAME, NUMBER, STRING)
            itoken = self.tokens.get(label, None)
            assert isinstance(itoken, int), label
            assert itoken in self.tokens.values(), label
            if itoken in c.tokens:
                return c.tokens[itoken]
            else:
                c.labels.append((itoken, None))
                c.tokens[itoken] = ilabel
                return ilabel
        elif label.type == LabelType.KEYWORD:
            # A keyword
            value = literal_eval(label)
            if value in c.keywords:
                return c.keywords[value]
            else:
                c.labels.append((self.tokens["NAME"], value))
                c.keywords[value] = ilabel
                return ilabel
        elif label.type == LabelType.OPERATOR:
            # An operator (any non-numeric token)
            value = literal_eval(label)
            tok_name = self.opmap[value]  # Fails if unknown token
            itoken = self.tokens[tok_name]
            if itoken in c.tokens:
                return c.tokens[itoken]
            else:
                c.labels.append((itoken, None))
                c.tokens[itoken] = ilabel
                return ilabel
        else:
            raise ValueError("Cannot categorize label {}".format(label))

    def calculate_first_sets(self):
        names = list(self.dfas.keys())
        for name in names:
            if name not in self.first:
                self.calculate_first_sets_for_rule(name)

            if self.verbose:
                print("First set for {dfa_name}".format(dfa_name=name))
                for item in self.first[name]:
                    print("    - {terminal}".format(terminal=item))

    def calculate_first_sets_for_rule(self, name):
        dfa = self.dfas[name]
        self.first[name] = None  # dummy to detect left recursion
        state = dfa.states[0]
        totalset = set()
        overlapcheck = {}
        for label, next in state.arcs.items():
            if label in self.dfas:
                if label in self.first:
                    fset = self.first[label]
                    if fset is None:
                        raise ValueError("recursion for rule %r" % name)
                else:
                    self.calculate_first_sets_for_rule(label)
                    fset = self.first[label]
                totalset.update(fset)
                overlapcheck[label] = fset
            else:
                totalset.add(label)
                overlapcheck[label] = {label}
        inverse = {}
        for label, itsfirst in overlapcheck.items():
            for symbol in itsfirst:
                if symbol in inverse:
                    raise ValueError(
                        "rule %s is ambiguous; %s is in the"
                        " first sets of %s as well as %s"
                        % (name, symbol, label, inverse[symbol])
                    )
                inverse[symbol] = label
        self.first[name] = totalset
