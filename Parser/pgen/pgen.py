import collections
import tokenize  # from stdlib

from . import grammar, token


class ParserGenerator(object):

    def __init__(self, grammar_file, token_file, stream=None, verbose=False):
        close_stream = None
        if stream is None:
            stream = open(grammar_file)
            close_stream = stream.close
        with open(token_file) as tok_file:
            token_lines = tok_file.readlines()
        self.tokens = dict(token.generate_tokens(token_lines))
        self.opmap = dict(token.generate_opmap(token_lines))
        # Manually add <> so it does not collide with !=
        self.opmap['<>'] = "NOTEQUAL"
        self.verbose = verbose
        self.filename = grammar_file
        self.stream = stream
        self.generator = tokenize.generate_tokens(stream.readline)
        self.gettoken() # Initialize lookahead
        self.dfas, self.startsymbol = self.parse()
        if close_stream is not None:
            close_stream()
        self.first = {} # map from symbol name to set of tokens
        self.addfirstsets()

    def make_grammar(self):
        c = grammar.Grammar()
        names = list(self.dfas.keys())
        names.remove(self.startsymbol)
        names.insert(0, self.startsymbol)
        for name in names:
            i = 256 + len(c.symbol2number)
            c.symbol2number[name] = i
            c.number2symbol[i] = name
        for name in names:
            self.make_label(c, name)
            dfa = self.dfas[name]
            states = []
            for state in dfa:
                arcs = []
                for label, next in sorted(state.arcs.items()):
                    arcs.append((self.make_label(c, label), dfa.index(next)))
                if state.isfinal:
                    arcs.append((0, dfa.index(state)))
                states.append(arcs)
            c.states.append(states)
            c.dfas[c.symbol2number[name]] = (states, self.make_first(c, name))
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

    def make_first(self, c, name):
        rawfirst = self.first[name]
        first = set()
        for label in sorted(rawfirst):
            ilabel = self.make_label(c, label)
            ##assert ilabel not in first # XXX failed on <> ... !=
            first.add(ilabel)
        return first

    def make_label(self, c, label):
        # XXX Maybe this should be a method on a subclass of converter?
        ilabel = len(c.labels)
        if label[0].isalpha():
            # Either a symbol name or a named token
            if label in c.symbol2number:
                # A symbol name (a non-terminal)
                if label in c.symbol2label:
                    return c.symbol2label[label]
                else:
                    c.labels.append((c.symbol2number[label], None))
                    c.symbol2label[label] = ilabel
                    return ilabel
            else:
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
        else:
            # Either a keyword or an operator
            assert label[0] in ('"', "'"), label
            value = eval(label)
            if value[0].isalpha():
                # A keyword
                if value in c.keywords:
                    return c.keywords[value]
                else:
                    c.labels.append((self.tokens["NAME"], value))
                    c.keywords[value] = ilabel
                    return ilabel
            else:
                # An operator (any non-numeric token)
                tok_name = self.opmap[value] # Fails if unknown token
                itoken = self.tokens[tok_name]
                if itoken in c.tokens:
                    return c.tokens[itoken]
                else:
                    c.labels.append((itoken, None))
                    c.tokens[itoken] = ilabel
                    return ilabel

    def addfirstsets(self):
        names = list(self.dfas.keys())
        for name in names:
            if name not in self.first:
                self.calcfirst(name)

            if self.verbose:
                print("First set for {dfa_name}".format(dfa_name=name))
                for item in self.first[name]:
                    print("    - {terminal}".format(terminal=item))

    def calcfirst(self, name):
        dfa = self.dfas[name]
        self.first[name] = None # dummy to detect left recursion
        state = dfa[0]
        totalset = set()
        overlapcheck = {}
        for label, next in state.arcs.items():
            if label in self.dfas:
                if label in self.first:
                    fset = self.first[label]
                    if fset is None:
                        raise ValueError("recursion for rule %r" % name)
                else:
                    self.calcfirst(label)
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
                    raise ValueError("rule %s is ambiguous; %s is in the"
                                     " first sets of %s as well as %s" %
                                     (name, symbol, label, inverse[symbol]))
                inverse[symbol] = label
        self.first[name] = totalset

    def parse(self):
        dfas = collections.OrderedDict()
        startsymbol = None
        # MSTART: (NEWLINE | RULE)* ENDMARKER
        while self.type != tokenize.ENDMARKER:
            while self.type == tokenize.NEWLINE:
                self.gettoken()
            # RULE: NAME ':' RHS NEWLINE
            name = self.expect(tokenize.NAME)
            if self.verbose:
                print("Processing rule {dfa_name}".format(dfa_name=name))
            self.expect(tokenize.OP, ":")
            a, z = self.parse_rhs()
            self.expect(tokenize.NEWLINE)
            if self.verbose:
                self.dump_nfa(name, a, z)
            dfa = self.make_dfa(a, z)
            if self.verbose:
                self.dump_dfa(name, dfa)
            self.simplify_dfa(dfa)
            dfas[name] = dfa
            if startsymbol is None:
                startsymbol = name
        return dfas, startsymbol

    def make_dfa(self, start, finish):
        # To turn an NFA into a DFA, we define the states of the DFA
        # to correspond to *sets* of states of the NFA.  Then do some
        # state reduction.  Let's represent sets as dicts with 1 for
        # values.
        assert isinstance(start, NFAState)
        assert isinstance(finish, NFAState)
        def closure(state):
            base = set()
            addclosure(state, base)
            return base
        def addclosure(state, base):
            assert isinstance(state, NFAState)
            if state in base:
                return
            base.add(state)
            for label, next in state.arcs:
                if label is None:
                    addclosure(next, base)
        states = [DFAState(closure(start), finish)]
        for state in states: # NB states grows while we're iterating
            arcs = {}
            for nfastate in state.nfaset:
                for label, next in nfastate.arcs:
                    if label is not None:
                        addclosure(next, arcs.setdefault(label, set()))
            for label, nfaset in sorted(arcs.items()):
                for st in states:
                    if st.nfaset == nfaset:
                        break
                else:
                    st = DFAState(nfaset, finish)
                    states.append(st)
                state.addarc(st, label)
        return states # List of DFAState instances; first one is start

    def dump_nfa(self, name, start, finish):
        print("Dump of NFA for", name)
        todo = [start]
        for i, state in enumerate(todo):
            print("  State", i, state is finish and "(final)" or "")
            for label, next in state.arcs:
                if next in todo:
                    j = todo.index(next)
                else:
                    j = len(todo)
                    todo.append(next)
                if label is None:
                    print("    -> %d" % j)
                else:
                    print("    %s -> %d" % (label, j))

    def dump_dfa(self, name, dfa):
        print("Dump of DFA for", name)
        for i, state in enumerate(dfa):
            print("  State", i, state.isfinal and "(final)" or "")
            for label, next in sorted(state.arcs.items()):
                print("    %s -> %d" % (label, dfa.index(next)))

    def simplify_dfa(self, dfa):
        # This is not theoretically optimal, but works well enough.
        # Algorithm: repeatedly look for two states that have the same
        # set of arcs (same labels pointing to the same nodes) and
        # unify them, until things stop changing.

        # dfa is a list of DFAState instances
        changes = True
        while changes:
            changes = False
            for i, state_i in enumerate(dfa):
                for j in range(i+1, len(dfa)):
                    state_j = dfa[j]
                    if state_i == state_j:
                        #print "  unify", i, j
                        del dfa[j]
                        for state in dfa:
                            state.unifystate(state_j, state_i)
                        changes = True
                        break

    def parse_rhs(self):
        # RHS: ALT ('|' ALT)*
        a, z = self.parse_alt()
        if self.value != "|":
            return a, z
        else:
            aa = NFAState()
            zz = NFAState()
            aa.addarc(a)
            z.addarc(zz)
            while self.value == "|":
                self.gettoken()
                a, z = self.parse_alt()
                aa.addarc(a)
                z.addarc(zz)
            return aa, zz

    def parse_alt(self):
        # ALT: ITEM+
        a, b = self.parse_item()
        while (self.value in ("(", "[") or
               self.type in (tokenize.NAME, tokenize.STRING)):
            c, d = self.parse_item()
            b.addarc(c)
            b = d
        return a, b

    def parse_item(self):
        # ITEM: '[' RHS ']' | ATOM ['+' | '*']
        if self.value == "[":
            self.gettoken()
            a, z = self.parse_rhs()
            self.expect(tokenize.OP, "]")
            a.addarc(z)
            return a, z
        else:
            a, z = self.parse_atom()
            value = self.value
            if value not in ("+", "*"):
                return a, z
            self.gettoken()
            z.addarc(a)
            if value == "+":
                return a, z
            else:
                return a, a

    def parse_atom(self):
        # ATOM: '(' RHS ')' | NAME | STRING
        if self.value == "(":
            self.gettoken()
            a, z = self.parse_rhs()
            self.expect(tokenize.OP, ")")
            return a, z
        elif self.type in (tokenize.NAME, tokenize.STRING):
            a = NFAState()
            z = NFAState()
            a.addarc(z, self.value)
            self.gettoken()
            return a, z
        else:
            self.raise_error("expected (...) or NAME or STRING, got %s/%s",
                             self.type, self.value)

    def expect(self, type, value=None):
        if self.type != type or (value is not None and self.value != value):
            self.raise_error("expected %s/%s, got %s/%s",
                             type, value, self.type, self.value)
        value = self.value
        self.gettoken()
        return value

    def gettoken(self):
        tup = next(self.generator)
        while tup[0] in (tokenize.COMMENT, tokenize.NL):
            tup = next(self.generator)
        self.type, self.value, self.begin, self.end, self.line = tup
        # print(getattr(tokenize, 'tok_name')[self.type], repr(self.value))

    def raise_error(self, msg, *args):
        if args:
            try:
                msg = msg % args
            except Exception:
                msg = " ".join([msg] + list(map(str, args)))
        raise SyntaxError(msg, (self.filename, self.end[0],
                                self.end[1], self.line))

class NFAState(object):

    def __init__(self):
        self.arcs = [] # list of (label, NFAState) pairs

    def addarc(self, next, label=None):
        assert label is None or isinstance(label, str)
        assert isinstance(next, NFAState)
        self.arcs.append((label, next))

class DFAState(object):

    def __init__(self, nfaset, final):
        assert isinstance(nfaset, set)
        assert isinstance(next(iter(nfaset)), NFAState)
        assert isinstance(final, NFAState)
        self.nfaset = nfaset
        self.isfinal = final in nfaset
        self.arcs = {} # map from label to DFAState

    def addarc(self, next, label):
        assert isinstance(label, str)
        assert label not in self.arcs
        assert isinstance(next, DFAState)
        self.arcs[label] = next

    def unifystate(self, old, new):
        for label, next in self.arcs.items():
            if next is old:
                self.arcs[label] = new

    def __eq__(self, other):
        # Equality test -- ignore the nfaset instance variable
        assert isinstance(other, DFAState)
        if self.isfinal != other.isfinal:
            return False
        # Can't just return self.arcs == other.arcs, because that
        # would invoke this method recursively, with cycles...
        if len(self.arcs) != len(other.arcs):
            return False
        for label, next in self.arcs.items():
            if next is not other.arcs.get(label):
                return False
        return True

    __hash__ = None # For Py3 compatibility.
