#  Copyright (c) 1998-2002 John Aycock
#
#  Permission is hereby granted, free of charge, to any person obtaining
#  a copy of this software and associated documentation files (the
#  "Software"), to deal in the Software without restriction, including
#  without limitation the rights to use, copy, modify, merge, publish,
#  distribute, sublicense, and/or sell copies of the Software, and to
#  permit persons to whom the Software is furnished to do so, subject to
#  the following conditions:
#
#  The above copyright notice and this permission notice shall be
#  included in all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
#  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
#  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

__version__ = 'SPARK-0.7 (pre-alpha-5)'

import re
import string

def _namelist(instance):
    namelist, namedict, classlist = [], {}, [instance.__class__]
    for c in classlist:
        for b in c.__bases__:
            classlist.append(b)
        for name in c.__dict__.keys():
            if not namedict.has_key(name):
                namelist.append(name)
                namedict[name] = 1
    return namelist

class GenericScanner:
    def __init__(self, flags=0):
        pattern = self.reflect()
        self.re = re.compile(pattern, re.VERBOSE|flags)

        self.index2func = {}
        for name, number in self.re.groupindex.items():
            self.index2func[number-1] = getattr(self, 't_' + name)

    def makeRE(self, name):
        doc = getattr(self, name).__doc__
        rv = '(?P<%s>%s)' % (name[2:], doc)
        return rv

    def reflect(self):
        rv = []
        for name in _namelist(self):
            if name[:2] == 't_' and name != 't_default':
                rv.append(self.makeRE(name))

        rv.append(self.makeRE('t_default'))
        return string.join(rv, '|')

    def error(self, s, pos):
        print "Lexical error at position %s" % pos
        raise SystemExit

    def tokenize(self, s):
        pos = 0
        n = len(s)
        while pos < n:
            m = self.re.match(s, pos)
            if m is None:
                self.error(s, pos)

            groups = m.groups()
            for i in range(len(groups)):
                if groups[i] and self.index2func.has_key(i):
                    self.index2func[i](groups[i])
            pos = m.end()

    def t_default(self, s):
        r'( . | \n )+'
        print "Specification error: unmatched input"
        raise SystemExit

#
#  Extracted from GenericParser and made global so that [un]picking works.
#
class _State:
    def __init__(self, stateno, items):
        self.T, self.complete, self.items = [], [], items
        self.stateno = stateno

class GenericParser:
    #
    #  An Earley parser, as per J. Earley, "An Efficient Context-Free
    #  Parsing Algorithm", CACM 13(2), pp. 94-102.  Also J. C. Earley,
    #  "An Efficient Context-Free Parsing Algorithm", Ph.D. thesis,
    #  Carnegie-Mellon University, August 1968.  New formulation of
    #  the parser according to J. Aycock, "Practical Earley Parsing
    #  and the SPARK Toolkit", Ph.D. thesis, University of Victoria,
    #  2001, and J. Aycock and R. N. Horspool, "Practical Earley
    #  Parsing", unpublished paper, 2001.
    #

    def __init__(self, start):
        self.rules = {}
        self.rule2func = {}
        self.rule2name = {}
        self.collectRules()
        self.augment(start)
        self.ruleschanged = 1

    _NULLABLE = '\e_'
    _START = 'START'
    _BOF = '|-'

    #
    #  When pickling, take the time to generate the full state machine;
    #  some information is then extraneous, too.  Unfortunately we
    #  can't save the rule2func map.
    #
    def __getstate__(self):
        if self.ruleschanged:
            #
            #  XXX - duplicated from parse()
            #
            self.computeNull()
            self.newrules = {}
            self.new2old = {}
            self.makeNewRules()
            self.ruleschanged = 0
            self.edges, self.cores = {}, {}
            self.states = { 0: self.makeState0() }
            self.makeState(0, self._BOF)
        #
        #  XXX - should find a better way to do this..
        #
        changes = 1
        while changes:
            changes = 0
            for k, v in self.edges.items():
                if v is None:
                    state, sym = k
                    if self.states.has_key(state):
                        self.goto(state, sym)
                        changes = 1
        rv = self.__dict__.copy()
        for s in self.states.values():
            del s.items
        del rv['rule2func']
        del rv['nullable']
        del rv['cores']
        return rv

    def __setstate__(self, D):
        self.rules = {}
        self.rule2func = {}
        self.rule2name = {}
        self.collectRules()
        start = D['rules'][self._START][0][1][1]        # Blech.
        self.augment(start)
        D['rule2func'] = self.rule2func
        D['makeSet'] = self.makeSet_fast
        self.__dict__ = D

    #
    #  A hook for GenericASTBuilder and GenericASTMatcher.  Mess
    #  thee not with this; nor shall thee toucheth the _preprocess
    #  argument to addRule.
    #
    def preprocess(self, rule, func):       return rule, func

    def addRule(self, doc, func, _preprocess=1):
        fn = func
        rules = string.split(doc)

        index = []
        for i in range(len(rules)):
            if rules[i] == '::=':
                index.append(i-1)
        index.append(len(rules))

        for i in range(len(index)-1):
            lhs = rules[index[i]]
            rhs = rules[index[i]+2:index[i+1]]
            rule = (lhs, tuple(rhs))

            if _preprocess:
                rule, fn = self.preprocess(rule, func)

            if self.rules.has_key(lhs):
                self.rules[lhs].append(rule)
            else:
                self.rules[lhs] = [ rule ]
            self.rule2func[rule] = fn
            self.rule2name[rule] = func.__name__[2:]
        self.ruleschanged = 1

    def collectRules(self):
        for name in _namelist(self):
            if name[:2] == 'p_':
                func = getattr(self, name)
                doc = func.__doc__
                self.addRule(doc, func)

    def augment(self, start):
        rule = '%s ::= %s %s' % (self._START, self._BOF, start)
        self.addRule(rule, lambda args: args[1], 0)

    def computeNull(self):
        self.nullable = {}
        tbd = []

        for rulelist in self.rules.values():
            lhs = rulelist[0][0]
            self.nullable[lhs] = 0
            for rule in rulelist:
                rhs = rule[1]
                if len(rhs) == 0:
                    self.nullable[lhs] = 1
                    continue
                #
                #  We only need to consider rules which
                #  consist entirely of nonterminal symbols.
                #  This should be a savings on typical
                #  grammars.
                #
                for sym in rhs:
                    if not self.rules.has_key(sym):
                        break
                else:
                    tbd.append(rule)
        changes = 1
        while changes:
            changes = 0
            for lhs, rhs in tbd:
                if self.nullable[lhs]:
                    continue
                for sym in rhs:
                    if not self.nullable[sym]:
                        break
                else:
                    self.nullable[lhs] = 1
                    changes = 1

    def makeState0(self):
        s0 = _State(0, [])
        for rule in self.newrules[self._START]:
            s0.items.append((rule, 0))
        return s0

    def finalState(self, tokens):
        #
        #  Yuck.
        #
        if len(self.newrules[self._START]) == 2 and len(tokens) == 0:
            return 1
        start = self.rules[self._START][0][1][1]
        return self.goto(1, start)

    def makeNewRules(self):
        worklist = []
        for rulelist in self.rules.values():
            for rule in rulelist:
                worklist.append((rule, 0, 1, rule))

        for rule, i, candidate, oldrule in worklist:
            lhs, rhs = rule
            n = len(rhs)
            while i < n:
                sym = rhs[i]
                if not self.rules.has_key(sym) or \
                   not self.nullable[sym]:
                    candidate = 0
                    i = i + 1
                    continue

                newrhs = list(rhs)
                newrhs[i] = self._NULLABLE+sym
                newrule = (lhs, tuple(newrhs))
                worklist.append((newrule, i+1,
                                 candidate, oldrule))
                candidate = 0
                i = i + 1
            else:
                if candidate:
                    lhs = self._NULLABLE+lhs
                    rule = (lhs, rhs)
                if self.newrules.has_key(lhs):
                    self.newrules[lhs].append(rule)
                else:
                    self.newrules[lhs] = [ rule ]
                self.new2old[rule] = oldrule

    def typestring(self, token):
        return None

    def error(self, token):
        print "Syntax error at or near `%s' token" % token
        raise SystemExit

    def parse(self, tokens):
        sets = [ [(1,0), (2,0)] ]
        self.links = {}

        if self.ruleschanged:
            self.computeNull()
            self.newrules = {}
            self.new2old = {}
            self.makeNewRules()
            self.ruleschanged = 0
            self.edges, self.cores = {}, {}
            self.states = { 0: self.makeState0() }
            self.makeState(0, self._BOF)

        for i in xrange(len(tokens)):
            sets.append([])

            if sets[i] == []:
                break
            self.makeSet(tokens[i], sets, i)
        else:
            sets.append([])
            self.makeSet(None, sets, len(tokens))

        #_dump(tokens, sets, self.states)

        finalitem = (self.finalState(tokens), 0)
        if finalitem not in sets[-2]:
            if len(tokens) > 0:
                self.error(tokens[i-1])
            else:
                self.error(None)

        return self.buildTree(self._START, finalitem,
                              tokens, len(sets)-2)

    def isnullable(self, sym):
        #
        #  For symbols in G_e only.  If we weren't supporting 1.5,
        #  could just use sym.startswith().
        #
        return self._NULLABLE == sym[0:len(self._NULLABLE)]

    def skip(self, (lhs, rhs), pos=0):
        n = len(rhs)
        while pos < n:
            if not self.isnullable(rhs[pos]):
                break
            pos = pos + 1
        return pos

    def makeState(self, state, sym):
        assert sym is not None
        #
        #  Compute \epsilon-kernel state's core and see if
        #  it exists already.
        #
        kitems = []
        for rule, pos in self.states[state].items:
            lhs, rhs = rule
            if rhs[pos:pos+1] == (sym,):
                kitems.append((rule, self.skip(rule, pos+1)))
        core = kitems

        core.sort()
        tcore = tuple(core)
        if self.cores.has_key(tcore):
            return self.cores[tcore]
        #
        #  Nope, doesn't exist.  Compute it and the associated
        #  \epsilon-nonkernel state together; we'll need it right away.
        #
        k = self.cores[tcore] = len(self.states)
        K, NK = _State(k, kitems), _State(k+1, [])
        self.states[k] = K
        predicted = {}

        edges = self.edges
        rules = self.newrules
        for X in K, NK:
            worklist = X.items
            for item in worklist:
                rule, pos = item
                lhs, rhs = rule
                if pos == len(rhs):
                    X.complete.append(rule)
                    continue

                nextSym = rhs[pos]
                key = (X.stateno, nextSym)
                if not rules.has_key(nextSym):
                    if not edges.has_key(key):
                        edges[key] = None
                        X.T.append(nextSym)
                else:
                    edges[key] = None
                    if not predicted.has_key(nextSym):
                        predicted[nextSym] = 1
                        for prule in rules[nextSym]:
                            ppos = self.skip(prule)
                            new = (prule, ppos)
                            NK.items.append(new)
            #
            #  Problem: we know K needs generating, but we
            #  don't yet know about NK.  Can't commit anything
            #  regarding NK to self.edges until we're sure.  Should
            #  we delay committing on both K and NK to avoid this
            #  hacky code?  This creates other problems..
            #
            if X is K:
                edges = {}

        if NK.items == []:
            return k

        #
        #  Check for \epsilon-nonkernel's core.  Unfortunately we
        #  need to know the entire set of predicted nonterminals
        #  to do this without accidentally duplicating states.
        #
        core = predicted.keys()
        core.sort()
        tcore = tuple(core)
        if self.cores.has_key(tcore):
            self.edges[(k, None)] = self.cores[tcore]
            return k

        nk = self.cores[tcore] = self.edges[(k, None)] = NK.stateno
        self.edges.update(edges)
        self.states[nk] = NK
        return k

    def goto(self, state, sym):
        key = (state, sym)
        if not self.edges.has_key(key):
            #
            #  No transitions from state on sym.
            #
            return None

        rv = self.edges[key]
        if rv is None:
            #
            #  Target state isn't generated yet.  Remedy this.
            #
            rv = self.makeState(state, sym)
            self.edges[key] = rv
        return rv

    def gotoT(self, state, t):
        return [self.goto(state, t)]

    def gotoST(self, state, st):
        rv = []
        for t in self.states[state].T:
            if st == t:
                rv.append(self.goto(state, t))
        return rv

    def add(self, set, item, i=None, predecessor=None, causal=None):
        if predecessor is None:
            if item not in set:
                set.append(item)
        else:
            key = (item, i)
            if item not in set:
                self.links[key] = []
                set.append(item)
            self.links[key].append((predecessor, causal))

    def makeSet(self, token, sets, i):
        cur, next = sets[i], sets[i+1]

        ttype = token is not None and self.typestring(token) or None
        if ttype is not None:
            fn, arg = self.gotoT, ttype
        else:
            fn, arg = self.gotoST, token

        for item in cur:
            ptr = (item, i)
            state, parent = item
            add = fn(state, arg)
            for k in add:
                if k is not None:
                    self.add(next, (k, parent), i+1, ptr)
                    nk = self.goto(k, None)
                    if nk is not None:
                        self.add(next, (nk, i+1))

            if parent == i:
                continue

            for rule in self.states[state].complete:
                lhs, rhs = rule
                for pitem in sets[parent]:
                    pstate, pparent = pitem
                    k = self.goto(pstate, lhs)
                    if k is not None:
                        why = (item, i, rule)
                        pptr = (pitem, parent)
                        self.add(cur, (k, pparent),
                                 i, pptr, why)
                        nk = self.goto(k, None)
                        if nk is not None:
                            self.add(cur, (nk, i))

    def makeSet_fast(self, token, sets, i):
        #
        #  Call *only* when the entire state machine has been built!
        #  It relies on self.edges being filled in completely, and
        #  then duplicates and inlines code to boost speed at the
        #  cost of extreme ugliness.
        #
        cur, next = sets[i], sets[i+1]
        ttype = token is not None and self.typestring(token) or None

        for item in cur:
            ptr = (item, i)
            state, parent = item
            if ttype is not None:
                k = self.edges.get((state, ttype), None)
                if k is not None:
                    #self.add(next, (k, parent), i+1, ptr)
                    #INLINED --v
                    new = (k, parent)
                    key = (new, i+1)
                    if new not in next:
                        self.links[key] = []
                        next.append(new)
                    self.links[key].append((ptr, None))
                    #INLINED --^
                    #nk = self.goto(k, None)
                    nk = self.edges.get((k, None), None)
                    if nk is not None:
                        #self.add(next, (nk, i+1))
                        #INLINED --v
                        new = (nk, i+1)
                        if new not in next:
                            next.append(new)
                        #INLINED --^
            else:
                add = self.gotoST(state, token)
                for k in add:
                    if k is not None:
                        self.add(next, (k, parent), i+1, ptr)
                        #nk = self.goto(k, None)
                        nk = self.edges.get((k, None), None)
                        if nk is not None:
                            self.add(next, (nk, i+1))

            if parent == i:
                continue

            for rule in self.states[state].complete:
                lhs, rhs = rule
                for pitem in sets[parent]:
                    pstate, pparent = pitem
                    #k = self.goto(pstate, lhs)
                    k = self.edges.get((pstate, lhs), None)
                    if k is not None:
                        why = (item, i, rule)
                        pptr = (pitem, parent)
                        #self.add(cur, (k, pparent),
                        #        i, pptr, why)
                        #INLINED --v
                        new = (k, pparent)
                        key = (new, i)
                        if new not in cur:
                            self.links[key] = []
                            cur.append(new)
                        self.links[key].append((pptr, why))
                        #INLINED --^
                        #nk = self.goto(k, None)
                        nk = self.edges.get((k, None), None)
                        if nk is not None:
                            #self.add(cur, (nk, i))
                            #INLINED --v
                            new = (nk, i)
                            if new not in cur:
                                cur.append(new)
                            #INLINED --^

    def predecessor(self, key, causal):
        for p, c in self.links[key]:
            if c == causal:
                return p
        assert 0

    def causal(self, key):
        links = self.links[key]
        if len(links) == 1:
            return links[0][1]
        choices = []
        rule2cause = {}
        for p, c in links:
            rule = c[2]
            choices.append(rule)
            rule2cause[rule] = c
        return rule2cause[self.ambiguity(choices)]

    def deriveEpsilon(self, nt):
        if len(self.newrules[nt]) > 1:
            rule = self.ambiguity(self.newrules[nt])
        else:
            rule = self.newrules[nt][0]
        #print rule

        rhs = rule[1]
        attr = [None] * len(rhs)

        for i in range(len(rhs)-1, -1, -1):
            attr[i] = self.deriveEpsilon(rhs[i])
        return self.rule2func[self.new2old[rule]](attr)

    def buildTree(self, nt, item, tokens, k):
        state, parent = item

        choices = []
        for rule in self.states[state].complete:
            if rule[0] == nt:
                choices.append(rule)
        rule = choices[0]
        if len(choices) > 1:
            rule = self.ambiguity(choices)
        #print rule

        rhs = rule[1]
        attr = [None] * len(rhs)

        for i in range(len(rhs)-1, -1, -1):
            sym = rhs[i]
            if not self.newrules.has_key(sym):
                if sym != self._BOF:
                    attr[i] = tokens[k-1]
                    key = (item, k)
                    item, k = self.predecessor(key, None)
            #elif self.isnullable(sym):
            elif self._NULLABLE == sym[0:len(self._NULLABLE)]:
                attr[i] = self.deriveEpsilon(sym)
            else:
                key = (item, k)
                why = self.causal(key)
                attr[i] = self.buildTree(sym, why[0],
                                         tokens, why[1])
                item, k = self.predecessor(key, why)
        return self.rule2func[self.new2old[rule]](attr)

    def ambiguity(self, rules):
        #
        #  XXX - problem here and in collectRules() if the same rule
        #        appears in >1 method.  Also undefined results if rules
        #        causing the ambiguity appear in the same method.
        #
        sortlist = []
        name2index = {}
        for i in range(len(rules)):
            lhs, rhs = rule = rules[i]
            name = self.rule2name[self.new2old[rule]]
            sortlist.append((len(rhs), name))
            name2index[name] = i
        sortlist.sort()
        list = map(lambda (a,b): b, sortlist)
        return rules[name2index[self.resolve(list)]]

    def resolve(self, list):
        #
        #  Resolve ambiguity in favor of the shortest RHS.
        #  Since we walk the tree from the top down, this
        #  should effectively resolve in favor of a "shift".
        #
        return list[0]

#
#  GenericASTBuilder automagically constructs a concrete/abstract syntax tree
#  for a given input.  The extra argument is a class (not an instance!)
#  which supports the "__setslice__" and "__len__" methods.
#
#  XXX - silently overrides any user code in methods.
#

class GenericASTBuilder(GenericParser):
    def __init__(self, AST, start):
        GenericParser.__init__(self, start)
        self.AST = AST

    def preprocess(self, rule, func):
        rebind = lambda lhs, self=self: \
                        lambda args, lhs=lhs, self=self: \
                                self.buildASTNode(args, lhs)
        lhs, rhs = rule
        return rule, rebind(lhs)

    def buildASTNode(self, args, lhs):
        children = []
        for arg in args:
            if isinstance(arg, self.AST):
                children.append(arg)
            else:
                children.append(self.terminal(arg))
        return self.nonterminal(lhs, children)

    def terminal(self, token):      return token

    def nonterminal(self, type, args):
        rv = self.AST(type)
        rv[:len(args)] = args
        return rv

#
#  GenericASTTraversal is a Visitor pattern according to Design Patterns.  For
#  each node it attempts to invoke the method n_<node type>, falling
#  back onto the default() method if the n_* can't be found.  The preorder
#  traversal also looks for an exit hook named n_<node type>_exit (no default
#  routine is called if it's not found).  To prematurely halt traversal
#  of a subtree, call the prune() method -- this only makes sense for a
#  preorder traversal.  Node type is determined via the typestring() method.
#

class GenericASTTraversalPruningException:
    pass

class GenericASTTraversal:
    def __init__(self, ast):
        self.ast = ast

    def typestring(self, node):
        return node.type

    def prune(self):
        raise GenericASTTraversalPruningException

    def preorder(self, node=None):
        if node is None:
            node = self.ast

        try:
            name = 'n_' + self.typestring(node)
            if hasattr(self, name):
                func = getattr(self, name)
                func(node)
            else:
                self.default(node)
        except GenericASTTraversalPruningException:
            return

        for kid in node:
            self.preorder(kid)

        name = name + '_exit'
        if hasattr(self, name):
            func = getattr(self, name)
            func(node)

    def postorder(self, node=None):
        if node is None:
            node = self.ast

        for kid in node:
            self.postorder(kid)

        name = 'n_' + self.typestring(node)
        if hasattr(self, name):
            func = getattr(self, name)
            func(node)
        else:
            self.default(node)


    def default(self, node):
        pass

#
#  GenericASTMatcher.  AST nodes must have "__getitem__" and "__cmp__"
#  implemented.
#
#  XXX - makes assumptions about how GenericParser walks the parse tree.
#

class GenericASTMatcher(GenericParser):
    def __init__(self, start, ast):
        GenericParser.__init__(self, start)
        self.ast = ast

    def preprocess(self, rule, func):
        rebind = lambda func, self=self: \
                        lambda args, func=func, self=self: \
                                self.foundMatch(args, func)
        lhs, rhs = rule
        rhslist = list(rhs)
        rhslist.reverse()

        return (lhs, tuple(rhslist)), rebind(func)

    def foundMatch(self, args, func):
        func(args[-1])
        return args[-1]

    def match_r(self, node):
        self.input.insert(0, node)
        children = 0

        for child in node:
            if children == 0:
                self.input.insert(0, '(')
            children = children + 1
            self.match_r(child)

        if children > 0:
            self.input.insert(0, ')')

    def match(self, ast=None):
        if ast is None:
            ast = self.ast
        self.input = []

        self.match_r(ast)
        self.parse(self.input)

    def resolve(self, list):
        #
        #  Resolve ambiguity in favor of the longest RHS.
        #
        return list[-1]

def _dump(tokens, sets, states):
    for i in range(len(sets)):
        print 'set', i
        for item in sets[i]:
            print '\t', item
            for (lhs, rhs), pos in states[item[0]].items:
                print '\t\t', lhs, '::=',
                print string.join(rhs[:pos]),
                print '.',
                print string.join(rhs[pos:])
        if i < len(tokens):
            print
            print 'token', str(tokens[i])
            print
