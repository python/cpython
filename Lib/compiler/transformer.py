"""Parse tree transformation module.

Transforms Python source code into an abstract syntax tree (AST)
defined in the ast module.

The simplest ways to invoke this module are via parse and parseFile.
parse(buf) -> AST
parseFile(path) -> AST
"""

# Original version written by Greg Stein (gstein@lyra.org)
#                         and Bill Tutt (rassilon@lima.mudlib.org)
# February 1997.
#
# Modifications and improvements for Python 2.0 by Jeremy Hylton and
# Mark Hammond

# Portions of this file are:
# Copyright (C) 1997-1998 Greg Stein. All Rights Reserved.
#
# This module is provided under a BSD-ish license. See
#   http://www.opensource.org/licenses/bsd-license.html
# and replace OWNER, ORGANIZATION, and YEAR as appropriate.

from ast import *
import parser
# Care must be taken to use only symbols and tokens defined in Python
# 1.5.2 for code branches executed in 1.5.2
import symbol
import token
import sys

error = 'walker.error'

from consts import CO_VARARGS, CO_VARKEYWORDS
from consts import OP_ASSIGN, OP_DELETE, OP_APPLY

def parseFile(path):
    f = open(path)
    # XXX The parser API tolerates files without a trailing newline,
    # but not strings without a trailing newline.  Always add an extra
    # newline to the file contents, since we're going through the string
    # version of the API.
    src = f.read() + "\n"
    f.close()
    return parse(src)

def parse(buf, mode="exec"):
    if mode == "exec" or mode == "single":
        return Transformer().parsesuite(buf)
    elif mode == "eval":
        return Transformer().parseexpr(buf)
    else:
        raise ValueError("compile() arg 3 must be"
                         " 'exec' or 'eval' or 'single'")

def asList(nodes):
    l = []
    for item in nodes:
        if hasattr(item, "asList"):
            l.append(item.asList())
        else:
            if type(item) is type( (None, None) ):
                l.append(tuple(asList(item)))
            elif type(item) is type( [] ):
                l.append(asList(item))
            else:
                l.append(item)
    return l

def Node(*args):
    kind = args[0]
    if nodes.has_key(kind):
        try:
            return nodes[kind](*args[1:])
        except TypeError:
            print nodes[kind], len(args), args
            raise
    else:
        raise error, "Can't find appropriate Node type: %s" % str(args)
        #return apply(ast.Node, args)

class Transformer:
    """Utility object for transforming Python parse trees.

    Exposes the following methods:
        tree = transform(ast_tree)
        tree = parsesuite(text)
        tree = parseexpr(text)
        tree = parsefile(fileob | filename)
    """

    def __init__(self):
        self._dispatch = {}
        for value, name in symbol.sym_name.items():
            if hasattr(self, name):
                self._dispatch[value] = getattr(self, name)
        self._dispatch[token.NEWLINE] = self.com_NEWLINE
        self._atom_dispatch = {token.LPAR: self.atom_lpar,
                               token.LSQB: self.atom_lsqb,
                               token.LBRACE: self.atom_lbrace,
                               token.BACKQUOTE: self.atom_backquote,
                               token.NUMBER: self.atom_number,
                               token.STRING: self.atom_string,
                               token.NAME: self.atom_name,
                               }
        self.encoding = None

    def transform(self, tree):
        """Transform an AST into a modified parse tree."""
        if type(tree) != type(()) and type(tree) != type([]):
            tree = parser.ast2tuple(tree, line_info=1)
        return self.compile_node(tree)

    def parsesuite(self, text):
        """Return a modified parse tree for the given suite text."""
        # Hack for handling non-native line endings on non-DOS like OSs.
        # this can go now we have universal newlines?
        text = text.replace('\x0d', '')
        return self.transform(parser.suite(text))

    def parseexpr(self, text):
        """Return a modified parse tree for the given expression text."""
        return self.transform(parser.expr(text))

    def parsefile(self, file):
        """Return a modified parse tree for the contents of the given file."""
        if type(file) == type(''):
            file = open(file)
        return self.parsesuite(file.read())

    # --------------------------------------------------------------
    #
    # PRIVATE METHODS
    #

    def compile_node(self, node):
        ### emit a line-number node?
        n = node[0]

        if n == symbol.encoding_decl:
            self.encoding = node[2]
            node = node[1]
            n = node[0]

        if n == symbol.single_input:
            return self.single_input(node[1:])
        if n == symbol.file_input:
            return self.file_input(node[1:])
        if n == symbol.eval_input:
            return self.eval_input(node[1:])
        if n == symbol.lambdef:
            return self.lambdef(node[1:])
        if n == symbol.funcdef:
            return self.funcdef(node[1:])
        if n == symbol.classdef:
            return self.classdef(node[1:])

        raise error, ('unexpected node type', n)

    def single_input(self, node):
        ### do we want to do anything about being "interactive" ?

        # NEWLINE | simple_stmt | compound_stmt NEWLINE
        n = node[0][0]
        if n != token.NEWLINE:
            return self.com_stmt(node[0])

        return Pass()

    def file_input(self, nodelist):
        doc = self.get_docstring(nodelist, symbol.file_input)
        if doc is not None:
            i = 1
        else:
            i = 0
        stmts = []
        for node in nodelist[i:]:
            if node[0] != token.ENDMARKER and node[0] != token.NEWLINE:
                self.com_append_stmt(stmts, node)
        return Module(doc, Stmt(stmts))

    def eval_input(self, nodelist):
        # from the built-in function input()
        ### is this sufficient?
        return Expression(self.com_node(nodelist[0]))

    def funcdef(self, nodelist):
        # funcdef: 'def' NAME parameters ':' suite
        # parameters: '(' [varargslist] ')'

        lineno = nodelist[1][2]
        name = nodelist[1][1]
        args = nodelist[2][2]

        if args[0] == symbol.varargslist:
            names, defaults, flags = self.com_arglist(args[1:])
        else:
            names = defaults = ()
            flags = 0
        doc = self.get_docstring(nodelist[4])

        # code for function
        code = self.com_node(nodelist[4])

        if doc is not None:
            assert isinstance(code, Stmt)
            assert isinstance(code.nodes[0], Discard)
            del code.nodes[0]
        n = Function(name, names, defaults, flags, doc, code)
        n.lineno = lineno
        return n

    def lambdef(self, nodelist):
        # lambdef: 'lambda' [varargslist] ':' test
        if nodelist[2][0] == symbol.varargslist:
            names, defaults, flags = self.com_arglist(nodelist[2][1:])
        else:
            names = defaults = ()
            flags = 0

        # code for lambda
        code = self.com_node(nodelist[-1])

        n = Lambda(names, defaults, flags, code)
        n.lineno = nodelist[1][2]
        return n

    def classdef(self, nodelist):
        # classdef: 'class' NAME ['(' testlist ')'] ':' suite

        name = nodelist[1][1]
        doc = self.get_docstring(nodelist[-1])
        if nodelist[2][0] == token.COLON:
            bases = []
        else:
            bases = self.com_bases(nodelist[3])

        # code for class
        code = self.com_node(nodelist[-1])

        if doc is not None:
            assert isinstance(code, Stmt)
            assert isinstance(code.nodes[0], Discard)
            del code.nodes[0]

        n = Class(name, bases, doc, code)
        n.lineno = nodelist[1][2]
        return n

    def stmt(self, nodelist):
        return self.com_stmt(nodelist[0])

    small_stmt = stmt
    flow_stmt = stmt
    compound_stmt = stmt

    def simple_stmt(self, nodelist):
        # small_stmt (';' small_stmt)* [';'] NEWLINE
        stmts = []
        for i in range(0, len(nodelist), 2):
            self.com_append_stmt(stmts, nodelist[i])
        return Stmt(stmts)

    def parameters(self, nodelist):
        raise error

    def varargslist(self, nodelist):
        raise error

    def fpdef(self, nodelist):
        raise error

    def fplist(self, nodelist):
        raise error

    def dotted_name(self, nodelist):
        raise error

    def comp_op(self, nodelist):
        raise error

    def trailer(self, nodelist):
        raise error

    def sliceop(self, nodelist):
        raise error

    def argument(self, nodelist):
        raise error

    # --------------------------------------------------------------
    #
    # STATEMENT NODES  (invoked by com_node())
    #

    def expr_stmt(self, nodelist):
        # augassign testlist | testlist ('=' testlist)*
        en = nodelist[-1]
        exprNode = self.lookup_node(en)(en[1:])
        if len(nodelist) == 1:
            n = Discard(exprNode)
            n.lineno = exprNode.lineno
            return n
        if nodelist[1][0] == token.EQUAL:
            nodesl = []
            for i in range(0, len(nodelist) - 2, 2):
                nodesl.append(self.com_assign(nodelist[i], OP_ASSIGN))
            n = Assign(nodesl, exprNode)
            n.lineno = nodelist[1][2]
        else:
            lval = self.com_augassign(nodelist[0])
            op = self.com_augassign_op(nodelist[1])
            n = AugAssign(lval, op[1], exprNode)
            n.lineno = op[2]
        return n

    def print_stmt(self, nodelist):
        # print ([ test (',' test)* [','] ] | '>>' test [ (',' test)+ [','] ])
        items = []
        if len(nodelist) == 1:
            start = 1
            dest = None
        elif nodelist[1][0] == token.RIGHTSHIFT:
            assert len(nodelist) == 3 \
                   or nodelist[3][0] == token.COMMA
            dest = self.com_node(nodelist[2])
            start = 4
        else:
            dest = None
            start = 1
        for i in range(start, len(nodelist), 2):
            items.append(self.com_node(nodelist[i]))
        if nodelist[-1][0] == token.COMMA:
            n = Print(items, dest)
            n.lineno = nodelist[0][2]
            return n
        n = Printnl(items, dest)
        n.lineno = nodelist[0][2]
        return n

    def del_stmt(self, nodelist):
        return self.com_assign(nodelist[1], OP_DELETE)

    def pass_stmt(self, nodelist):
        n = Pass()
        n.lineno = nodelist[0][2]
        return n

    def break_stmt(self, nodelist):
        n = Break()
        n.lineno = nodelist[0][2]
        return n

    def continue_stmt(self, nodelist):
        n = Continue()
        n.lineno = nodelist[0][2]
        return n

    def return_stmt(self, nodelist):
        # return: [testlist]
        if len(nodelist) < 2:
            n = Return(Const(None))
            n.lineno = nodelist[0][2]
            return n
        n = Return(self.com_node(nodelist[1]))
        n.lineno = nodelist[0][2]
        return n

    def yield_stmt(self, nodelist):
        n = Yield(self.com_node(nodelist[1]))
        n.lineno = nodelist[0][2]
        return n

    def raise_stmt(self, nodelist):
        # raise: [test [',' test [',' test]]]
        if len(nodelist) > 5:
            expr3 = self.com_node(nodelist[5])
        else:
            expr3 = None
        if len(nodelist) > 3:
            expr2 = self.com_node(nodelist[3])
        else:
            expr2 = None
        if len(nodelist) > 1:
            expr1 = self.com_node(nodelist[1])
        else:
            expr1 = None
        n = Raise(expr1, expr2, expr3)
        n.lineno = nodelist[0][2]
        return n

    def import_stmt(self, nodelist):
        # import_stmt: 'import' dotted_as_name (',' dotted_as_name)* |
        # from: 'from' dotted_name 'import'
        #                        ('*' | import_as_name (',' import_as_name)*)
        if nodelist[0][1] == 'from':
            names = []
            if nodelist[3][0] == token.NAME:
                for i in range(3, len(nodelist), 2):
                    names.append((nodelist[i][1], None))
            else:
                for i in range(3, len(nodelist), 2):
                    names.append(self.com_import_as_name(nodelist[i]))
            n = From(self.com_dotted_name(nodelist[1]), names)
            n.lineno = nodelist[0][2]
            return n

        if nodelist[1][0] == symbol.dotted_name:
            names = [(self.com_dotted_name(nodelist[1][1:]), None)]
        else:
            names = []
            for i in range(1, len(nodelist), 2):
                names.append(self.com_dotted_as_name(nodelist[i]))
        n = Import(names)
        n.lineno = nodelist[0][2]
        return n

    def global_stmt(self, nodelist):
        # global: NAME (',' NAME)*
        names = []
        for i in range(1, len(nodelist), 2):
            names.append(nodelist[i][1])
        n = Global(names)
        n.lineno = nodelist[0][2]
        return n

    def exec_stmt(self, nodelist):
        # exec_stmt: 'exec' expr ['in' expr [',' expr]]
        expr1 = self.com_node(nodelist[1])
        if len(nodelist) >= 4:
            expr2 = self.com_node(nodelist[3])
            if len(nodelist) >= 6:
                expr3 = self.com_node(nodelist[5])
            else:
                expr3 = None
        else:
            expr2 = expr3 = None

        n = Exec(expr1, expr2, expr3)
        n.lineno = nodelist[0][2]
        return n

    def assert_stmt(self, nodelist):
        # 'assert': test, [',' test]
        expr1 = self.com_node(nodelist[1])
        if (len(nodelist) == 4):
            expr2 = self.com_node(nodelist[3])
        else:
            expr2 = None
        n = Assert(expr1, expr2)
        n.lineno = nodelist[0][2]
        return n

    def if_stmt(self, nodelist):
        # if: test ':' suite ('elif' test ':' suite)* ['else' ':' suite]
        tests = []
        for i in range(0, len(nodelist) - 3, 4):
            testNode = self.com_node(nodelist[i + 1])
            suiteNode = self.com_node(nodelist[i + 3])
            tests.append((testNode, suiteNode))

        if len(nodelist) % 4 == 3:
            elseNode = self.com_node(nodelist[-1])
##      elseNode.lineno = nodelist[-1][1][2]
        else:
            elseNode = None
        n = If(tests, elseNode)
        n.lineno = nodelist[0][2]
        return n

    def while_stmt(self, nodelist):
        # 'while' test ':' suite ['else' ':' suite]

        testNode = self.com_node(nodelist[1])
        bodyNode = self.com_node(nodelist[3])

        if len(nodelist) > 4:
            elseNode = self.com_node(nodelist[6])
        else:
            elseNode = None

        n = While(testNode, bodyNode, elseNode)
        n.lineno = nodelist[0][2]
        return n

    def for_stmt(self, nodelist):
        # 'for' exprlist 'in' exprlist ':' suite ['else' ':' suite]

        assignNode = self.com_assign(nodelist[1], OP_ASSIGN)
        listNode = self.com_node(nodelist[3])
        bodyNode = self.com_node(nodelist[5])

        if len(nodelist) > 8:
            elseNode = self.com_node(nodelist[8])
        else:
            elseNode = None

        n = For(assignNode, listNode, bodyNode, elseNode)
        n.lineno = nodelist[0][2]
        return n

    def try_stmt(self, nodelist):
        # 'try' ':' suite (except_clause ':' suite)+ ['else' ':' suite]
        # | 'try' ':' suite 'finally' ':' suite
        if nodelist[3][0] != symbol.except_clause:
            return self.com_try_finally(nodelist)

        return self.com_try_except(nodelist)

    def suite(self, nodelist):
        # simple_stmt | NEWLINE INDENT NEWLINE* (stmt NEWLINE*)+ DEDENT
        if len(nodelist) == 1:
            return self.com_stmt(nodelist[0])

        stmts = []
        for node in nodelist:
            if node[0] == symbol.stmt:
                self.com_append_stmt(stmts, node)
        return Stmt(stmts)

    # --------------------------------------------------------------
    #
    # EXPRESSION NODES  (invoked by com_node())
    #

    def testlist(self, nodelist):
        # testlist: expr (',' expr)* [',']
        # testlist_safe: test [(',' test)+ [',']]
        # exprlist: expr (',' expr)* [',']
        return self.com_binary(Tuple, nodelist)

    testlist_safe = testlist # XXX
    testlist1 = testlist
    exprlist = testlist

    def test(self, nodelist):
        # and_test ('or' and_test)* | lambdef
        if len(nodelist) == 1 and nodelist[0][0] == symbol.lambdef:
            return self.lambdef(nodelist[0])
        return self.com_binary(Or, nodelist)

    def and_test(self, nodelist):
        # not_test ('and' not_test)*
        return self.com_binary(And, nodelist)

    def not_test(self, nodelist):
        # 'not' not_test | comparison
        result = self.com_node(nodelist[-1])
        if len(nodelist) == 2:
            n = Not(result)
            n.lineno = nodelist[0][2]
            return n
        return result

    def comparison(self, nodelist):
        # comparison: expr (comp_op expr)*
        node = self.com_node(nodelist[0])
        if len(nodelist) == 1:
            return node

        results = []
        for i in range(2, len(nodelist), 2):
            nl = nodelist[i-1]

            # comp_op: '<' | '>' | '=' | '>=' | '<=' | '<>' | '!=' | '=='
            #          | 'in' | 'not' 'in' | 'is' | 'is' 'not'
            n = nl[1]
            if n[0] == token.NAME:
                type = n[1]
                if len(nl) == 3:
                    if type == 'not':
                        type = 'not in'
                    else:
                        type = 'is not'
            else:
                type = _cmp_types[n[0]]

            lineno = nl[1][2]
            results.append((type, self.com_node(nodelist[i])))

        # we need a special "compare" node so that we can distinguish
        #   3 < x < 5   from    (3 < x) < 5
        # the two have very different semantics and results (note that the
        # latter form is always true)

        n = Compare(node, results)
        n.lineno = lineno
        return n

    def expr(self, nodelist):
        # xor_expr ('|' xor_expr)*
        return self.com_binary(Bitor, nodelist)

    def xor_expr(self, nodelist):
        # xor_expr ('^' xor_expr)*
        return self.com_binary(Bitxor, nodelist)

    def and_expr(self, nodelist):
        # xor_expr ('&' xor_expr)*
        return self.com_binary(Bitand, nodelist)

    def shift_expr(self, nodelist):
        # shift_expr ('<<'|'>>' shift_expr)*
        node = self.com_node(nodelist[0])
        for i in range(2, len(nodelist), 2):
            right = self.com_node(nodelist[i])
            if nodelist[i-1][0] == token.LEFTSHIFT:
                node = LeftShift([node, right])
                node.lineno = nodelist[1][2]
            elif nodelist[i-1][0] == token.RIGHTSHIFT:
                node = RightShift([node, right])
                node.lineno = nodelist[1][2]
            else:
                raise ValueError, "unexpected token: %s" % nodelist[i-1][0]
        return node

    def arith_expr(self, nodelist):
        node = self.com_node(nodelist[0])
        for i in range(2, len(nodelist), 2):
            right = self.com_node(nodelist[i])
            if nodelist[i-1][0] == token.PLUS:
                node = Add([node, right])
                node.lineno = nodelist[1][2]
            elif nodelist[i-1][0] == token.MINUS:
                node = Sub([node, right])
                node.lineno = nodelist[1][2]
            else:
                raise ValueError, "unexpected token: %s" % nodelist[i-1][0]
        return node

    def term(self, nodelist):
        node = self.com_node(nodelist[0])
        for i in range(2, len(nodelist), 2):
            right = self.com_node(nodelist[i])
            t = nodelist[i-1][0]
            if t == token.STAR:
                node = Mul([node, right])
            elif t == token.SLASH:
                node = Div([node, right])
            elif t == token.PERCENT:
                node = Mod([node, right])
            elif t == token.DOUBLESLASH:
                node = FloorDiv([node, right])
            else:
                raise ValueError, "unexpected token: %s" % t
            node.lineno = nodelist[1][2]
        return node

    def factor(self, nodelist):
        elt = nodelist[0]
        t = elt[0]
        node = self.com_node(nodelist[-1])
        # need to handle (unary op)constant here...
        if t == token.PLUS:
            node = UnaryAdd(node)
            node.lineno = elt[2]
        elif t == token.MINUS:
            node = UnarySub(node)
            node.lineno = elt[2]
        elif t == token.TILDE:
            node = Invert(node)
            node.lineno = elt[2]
        return node

    def power(self, nodelist):
        # power: atom trailer* ('**' factor)*
        node = self.com_node(nodelist[0])
        for i in range(1, len(nodelist)):
            elt = nodelist[i]
            if elt[0] == token.DOUBLESTAR:
                n = Power([node, self.com_node(nodelist[i+1])])
                n.lineno = elt[2]
                return n

            node = self.com_apply_trailer(node, elt)

        return node

    def atom(self, nodelist):
        n = self._atom_dispatch[nodelist[0][0]](nodelist)
        n.lineno = nodelist[0][2]
        return n

    def atom_lpar(self, nodelist):
        if nodelist[1][0] == token.RPAR:
            n = Tuple(())
            n.lineno = nodelist[0][2]
            return n
        return self.com_node(nodelist[1])

    def atom_lsqb(self, nodelist):
        if nodelist[1][0] == token.RSQB:
            n = List(())
            n.lineno = nodelist[0][2]
            return n
        return self.com_list_constructor(nodelist[1])

    def atom_lbrace(self, nodelist):
        if nodelist[1][0] == token.RBRACE:
            return Dict(())
        return self.com_dictmaker(nodelist[1])

    def atom_backquote(self, nodelist):
        n = Backquote(self.com_node(nodelist[1]))
        n.lineno = nodelist[0][2]
        return n

    def atom_number(self, nodelist):
        ### need to verify this matches compile.c
        k = eval(nodelist[0][1])
        n = Const(k)
        n.lineno = nodelist[0][2]
        return n

    def decode_literal(self, lit):
        if self.encoding:
            # this is particularly fragile & a bit of a
            # hack... changes in compile.c:parsestr and
            # tokenizer.c must be reflected here.
            if self.encoding not in ['utf-8', 'iso-8859-1']:
                lit = unicode(lit, 'utf-8').encode(self.encoding)
            return eval("# coding: %s\n%s" % (self.encoding, lit))
        else:
            return eval(lit)

    def atom_string(self, nodelist):
        k = ''
        for node in nodelist:
            k += self.decode_literal(node[1])
        n = Const(k)
        n.lineno = nodelist[0][2]
        return n

    def atom_name(self, nodelist):
        ### any processing to do?
        n = Name(nodelist[0][1])
        n.lineno = nodelist[0][2]
        return n

    # --------------------------------------------------------------
    #
    # INTERNAL PARSING UTILITIES
    #

    # The use of com_node() introduces a lot of extra stack frames,
    # enough to cause a stack overflow compiling test.test_parser with
    # the standard interpreter recursionlimit.  The com_node() is a
    # convenience function that hides the dispatch details, but comes
    # at a very high cost.  It is more efficient to dispatch directly
    # in the callers.  In these cases, use lookup_node() and call the
    # dispatched node directly.

    def lookup_node(self, node):
        return self._dispatch[node[0]]

    def com_node(self, node):
        # Note: compile.c has handling in com_node for del_stmt, pass_stmt,
        #       break_stmt, stmt, small_stmt, flow_stmt, simple_stmt,
        #       and compound_stmt.
        #       We'll just dispatch them.
        return self._dispatch[node[0]](node[1:])

    def com_NEWLINE(self, *args):
        # A ';' at the end of a line can make a NEWLINE token appear
        # here, Render it harmless. (genc discards ('discard',
        # ('const', xxxx)) Nodes)
        return Discard(Const(None))

    def com_arglist(self, nodelist):
        # varargslist:
        #     (fpdef ['=' test] ',')* ('*' NAME [',' '**' NAME] | '**' NAME)
        #   | fpdef ['=' test] (',' fpdef ['=' test])* [',']
        # fpdef: NAME | '(' fplist ')'
        # fplist: fpdef (',' fpdef)* [',']
        names = []
        defaults = []
        flags = 0

        i = 0
        while i < len(nodelist):
            node = nodelist[i]
            if node[0] == token.STAR or node[0] == token.DOUBLESTAR:
                if node[0] == token.STAR:
                    node = nodelist[i+1]
                    if node[0] == token.NAME:
                        names.append(node[1])
                        flags = flags | CO_VARARGS
                        i = i + 3

                if i < len(nodelist):
                    # should be DOUBLESTAR
                    t = nodelist[i][0]
                    if t == token.DOUBLESTAR:
                        node = nodelist[i+1]
                    else:
                        raise ValueError, "unexpected token: %s" % t
                    names.append(node[1])
                    flags = flags | CO_VARKEYWORDS

                break

            # fpdef: NAME | '(' fplist ')'
            names.append(self.com_fpdef(node))

            i = i + 1
            if i >= len(nodelist):
                break

            if nodelist[i][0] == token.EQUAL:
                defaults.append(self.com_node(nodelist[i + 1]))
                i = i + 2
            elif len(defaults):
                # Treat "(a=1, b)" as "(a=1, b=None)"
                defaults.append(Const(None))

            i = i + 1

        return names, defaults, flags

    def com_fpdef(self, node):
        # fpdef: NAME | '(' fplist ')'
        if node[1][0] == token.LPAR:
            return self.com_fplist(node[2])
        return node[1][1]

    def com_fplist(self, node):
        # fplist: fpdef (',' fpdef)* [',']
        if len(node) == 2:
            return self.com_fpdef(node[1])
        list = []
        for i in range(1, len(node), 2):
            list.append(self.com_fpdef(node[i]))
        return tuple(list)

    def com_dotted_name(self, node):
        # String together the dotted names and return the string
        name = ""
        for n in node:
            if type(n) == type(()) and n[0] == 1:
                name = name + n[1] + '.'
        return name[:-1]

    def com_dotted_as_name(self, node):
        dot = self.com_dotted_name(node[1])
        if len(node) <= 2:
            return dot, None
        if node[0] == symbol.dotted_name:
            pass
        else:
            assert node[2][1] == 'as'
            assert node[3][0] == token.NAME
            return dot, node[3][1]

    def com_import_as_name(self, node):
        if node[0] == token.STAR:
            return '*', None
        assert node[0] == symbol.import_as_name
        node = node[1:]
        if len(node) == 1:
            assert node[0][0] == token.NAME
            return node[0][1], None

        assert node[1][1] == 'as', node
        assert node[2][0] == token.NAME
        return node[0][1], node[2][1]

    def com_bases(self, node):
        bases = []
        for i in range(1, len(node), 2):
            bases.append(self.com_node(node[i]))
        return bases

    def com_try_finally(self, nodelist):
        # try_fin_stmt: "try" ":" suite "finally" ":" suite
        n = TryFinally(self.com_node(nodelist[2]),
                       self.com_node(nodelist[5]))
        n.lineno = nodelist[0][2]
        return n

    def com_try_except(self, nodelist):
        # try_except: 'try' ':' suite (except_clause ':' suite)* ['else' suite]
        #tryexcept:  [TryNode, [except_clauses], elseNode)]
        stmt = self.com_node(nodelist[2])
        clauses = []
        elseNode = None
        for i in range(3, len(nodelist), 3):
            node = nodelist[i]
            if node[0] == symbol.except_clause:
                # except_clause: 'except' [expr [',' expr]] */
                if len(node) > 2:
                    expr1 = self.com_node(node[2])
                    if len(node) > 4:
                        expr2 = self.com_assign(node[4], OP_ASSIGN)
                    else:
                        expr2 = None
                else:
                    expr1 = expr2 = None
                clauses.append((expr1, expr2, self.com_node(nodelist[i+2])))

            if node[0] == token.NAME:
                elseNode = self.com_node(nodelist[i+2])
        n = TryExcept(self.com_node(nodelist[2]), clauses, elseNode)
        n.lineno = nodelist[0][2]
        return n

    def com_augassign_op(self, node):
        assert node[0] == symbol.augassign
        return node[1]

    def com_augassign(self, node):
        """Return node suitable for lvalue of augmented assignment

        Names, slices, and attributes are the only allowable nodes.
        """
        l = self.com_node(node)
        if l.__class__ in (Name, Slice, Subscript, Getattr):
            return l
        raise SyntaxError, "can't assign to %s" % l.__class__.__name__

    def com_assign(self, node, assigning):
        # return a node suitable for use as an "lvalue"
        # loop to avoid trivial recursion
        while 1:
            t = node[0]
            if t == symbol.exprlist or t == symbol.testlist:
                if len(node) > 2:
                    return self.com_assign_tuple(node, assigning)
                node = node[1]
            elif t in _assign_types:
                if len(node) > 2:
                    raise SyntaxError, "can't assign to operator"
                node = node[1]
            elif t == symbol.power:
                if node[1][0] != symbol.atom:
                    raise SyntaxError, "can't assign to operator"
                if len(node) > 2:
                    primary = self.com_node(node[1])
                    for i in range(2, len(node)-1):
                        ch = node[i]
                        if ch[0] == token.DOUBLESTAR:
                            raise SyntaxError, "can't assign to operator"
                        primary = self.com_apply_trailer(primary, ch)
                    return self.com_assign_trailer(primary, node[-1],
                                                   assigning)
                node = node[1]
            elif t == symbol.atom:
                t = node[1][0]
                if t == token.LPAR:
                    node = node[2]
                    if node[0] == token.RPAR:
                        raise SyntaxError, "can't assign to ()"
                elif t == token.LSQB:
                    node = node[2]
                    if node[0] == token.RSQB:
                        raise SyntaxError, "can't assign to []"
                    return self.com_assign_list(node, assigning)
                elif t == token.NAME:
                    return self.com_assign_name(node[1], assigning)
                else:
                    raise SyntaxError, "can't assign to literal"
            else:
                raise SyntaxError, "bad assignment"

    def com_assign_tuple(self, node, assigning):
        assigns = []
        for i in range(1, len(node), 2):
            assigns.append(self.com_assign(node[i], assigning))
        return AssTuple(assigns)

    def com_assign_list(self, node, assigning):
        assigns = []
        for i in range(1, len(node), 2):
            if i + 1 < len(node):
                if node[i + 1][0] == symbol.list_for:
                    raise SyntaxError, "can't assign to list comprehension"
                assert node[i + 1][0] == token.COMMA, node[i + 1]
            assigns.append(self.com_assign(node[i], assigning))
        return AssList(assigns)

    def com_assign_name(self, node, assigning):
        n = AssName(node[1], assigning)
        n.lineno = node[2]
        return n

    def com_assign_trailer(self, primary, node, assigning):
        t = node[1][0]
        if t == token.DOT:
            return self.com_assign_attr(primary, node[2], assigning)
        if t == token.LSQB:
            return self.com_subscriptlist(primary, node[2], assigning)
        if t == token.LPAR:
            raise SyntaxError, "can't assign to function call"
        raise SyntaxError, "unknown trailer type: %s" % t

    def com_assign_attr(self, primary, node, assigning):
        return AssAttr(primary, node[1], assigning)

    def com_binary(self, constructor, nodelist):
        "Compile 'NODE (OP NODE)*' into (type, [ node1, ..., nodeN ])."
        l = len(nodelist)
        if l == 1:
            n = nodelist[0]
            return self.lookup_node(n)(n[1:])
        items = []
        for i in range(0, l, 2):
            n = nodelist[i]
            items.append(self.lookup_node(n)(n[1:]))
        return constructor(items)

    def com_stmt(self, node):
        result = self.lookup_node(node)(node[1:])
        assert result is not None
        if isinstance(result, Stmt):
            return result
        return Stmt([result])

    def com_append_stmt(self, stmts, node):
        result = self.com_node(node)
        assert result is not None
        if isinstance(result, Stmt):
            stmts.extend(result.nodes)
        else:
            stmts.append(result)

    if hasattr(symbol, 'list_for'):
        def com_list_constructor(self, nodelist):
            # listmaker: test ( list_for | (',' test)* [','] )
            values = []
            for i in range(1, len(nodelist)):
                if nodelist[i][0] == symbol.list_for:
                    assert len(nodelist[i:]) == 1
                    return self.com_list_comprehension(values[0],
                                                       nodelist[i])
                elif nodelist[i][0] == token.COMMA:
                    continue
                values.append(self.com_node(nodelist[i]))
            return List(values)

        def com_list_comprehension(self, expr, node):
            # list_iter: list_for | list_if
            # list_for: 'for' exprlist 'in' testlist [list_iter]
            # list_if: 'if' test [list_iter]

            # XXX should raise SyntaxError for assignment

            lineno = node[1][2]
            fors = []
            while node:
                t = node[1][1]
                if t == 'for':
                    assignNode = self.com_assign(node[2], OP_ASSIGN)
                    listNode = self.com_node(node[4])
                    newfor = ListCompFor(assignNode, listNode, [])
                    newfor.lineno = node[1][2]
                    fors.append(newfor)
                    if len(node) == 5:
                        node = None
                    else:
                        node = self.com_list_iter(node[5])
                elif t == 'if':
                    test = self.com_node(node[2])
                    newif = ListCompIf(test)
                    newif.lineno = node[1][2]
                    newfor.ifs.append(newif)
                    if len(node) == 3:
                        node = None
                    else:
                        node = self.com_list_iter(node[3])
                else:
                    raise SyntaxError, \
                          ("unexpected list comprehension element: %s %d"
                           % (node, lineno))
            n = ListComp(expr, fors)
            n.lineno = lineno
            return n

        def com_list_iter(self, node):
            assert node[0] == symbol.list_iter
            return node[1]
    else:
        def com_list_constructor(self, nodelist):
            values = []
            for i in range(1, len(nodelist), 2):
                values.append(self.com_node(nodelist[i]))
            return List(values)

    def com_dictmaker(self, nodelist):
        # dictmaker: test ':' test (',' test ':' value)* [',']
        items = []
        for i in range(1, len(nodelist), 4):
            items.append((self.com_node(nodelist[i]),
                          self.com_node(nodelist[i+2])))
        return Dict(items)

    def com_apply_trailer(self, primaryNode, nodelist):
        t = nodelist[1][0]
        if t == token.LPAR:
            return self.com_call_function(primaryNode, nodelist[2])
        if t == token.DOT:
            return self.com_select_member(primaryNode, nodelist[2])
        if t == token.LSQB:
            return self.com_subscriptlist(primaryNode, nodelist[2], OP_APPLY)

        raise SyntaxError, 'unknown node type: %s' % t

    def com_select_member(self, primaryNode, nodelist):
        if nodelist[0] != token.NAME:
            raise SyntaxError, "member must be a name"
        n = Getattr(primaryNode, nodelist[1])
        n.lineno = nodelist[2]
        return n

    def com_call_function(self, primaryNode, nodelist):
        if nodelist[0] == token.RPAR:
            return CallFunc(primaryNode, [])
        args = []
        kw = 0
        len_nodelist = len(nodelist)
        for i in range(1, len_nodelist, 2):
            node = nodelist[i]
            if node[0] == token.STAR or node[0] == token.DOUBLESTAR:
                break
            kw, result = self.com_argument(node, kw)
            args.append(result)
        else:
            # No broken by star arg, so skip the last one we processed.
            i = i + 1
        if i < len_nodelist and nodelist[i][0] == token.COMMA:
            # need to accept an application that looks like "f(a, b,)"
            i = i + 1
        star_node = dstar_node = None
        while i < len_nodelist:
            tok = nodelist[i]
            ch = nodelist[i+1]
            i = i + 3
            if tok[0]==token.STAR:
                if star_node is not None:
                    raise SyntaxError, 'already have the varargs indentifier'
                star_node = self.com_node(ch)
            elif tok[0]==token.DOUBLESTAR:
                if dstar_node is not None:
                    raise SyntaxError, 'already have the kwargs indentifier'
                dstar_node = self.com_node(ch)
            else:
                raise SyntaxError, 'unknown node type: %s' % tok

        return CallFunc(primaryNode, args, star_node, dstar_node)

    def com_argument(self, nodelist, kw):
        if len(nodelist) == 2:
            if kw:
                raise SyntaxError, "non-keyword arg after keyword arg"
            return 0, self.com_node(nodelist[1])
        result = self.com_node(nodelist[3])
        n = nodelist[1]
        while len(n) == 2 and n[0] != token.NAME:
            n = n[1]
        if n[0] != token.NAME:
            raise SyntaxError, "keyword can't be an expression (%s)"%n[0]
        node = Keyword(n[1], result)
        node.lineno = n[2]
        return 1, node

    def com_subscriptlist(self, primary, nodelist, assigning):
        # slicing:      simple_slicing | extended_slicing
        # simple_slicing:   primary "[" short_slice "]"
        # extended_slicing: primary "[" slice_list "]"
        # slice_list:   slice_item ("," slice_item)* [","]

        # backwards compat slice for '[i:j]'
        if len(nodelist) == 2:
            sub = nodelist[1]
            if (sub[1][0] == token.COLON or \
                            (len(sub) > 2 and sub[2][0] == token.COLON)) and \
                            sub[-1][0] != symbol.sliceop:
                return self.com_slice(primary, sub, assigning)

        subscripts = []
        for i in range(1, len(nodelist), 2):
            subscripts.append(self.com_subscript(nodelist[i]))

        return Subscript(primary, assigning, subscripts)

    def com_subscript(self, node):
        # slice_item: expression | proper_slice | ellipsis
        ch = node[1]
        t = ch[0]
        if t == token.DOT and node[2][0] == token.DOT:
            return Ellipsis()
        if t == token.COLON or len(node) > 2:
            return self.com_sliceobj(node)
        return self.com_node(ch)

    def com_sliceobj(self, node):
        # proper_slice: short_slice | long_slice
        # short_slice:  [lower_bound] ":" [upper_bound]
        # long_slice:   short_slice ":" [stride]
        # lower_bound:  expression
        # upper_bound:  expression
        # stride:       expression
        #
        # Note: a stride may be further slicing...

        items = []

        if node[1][0] == token.COLON:
            items.append(Const(None))
            i = 2
        else:
            items.append(self.com_node(node[1]))
            # i == 2 is a COLON
            i = 3

        if i < len(node) and node[i][0] == symbol.test:
            items.append(self.com_node(node[i]))
            i = i + 1
        else:
            items.append(Const(None))

        # a short_slice has been built. look for long_slice now by looking
        # for strides...
        for j in range(i, len(node)):
            ch = node[j]
            if len(ch) == 2:
                items.append(Const(None))
            else:
                items.append(self.com_node(ch[2]))

        return Sliceobj(items)

    def com_slice(self, primary, node, assigning):
        # short_slice:  [lower_bound] ":" [upper_bound]
        lower = upper = None
        if len(node) == 3:
            if node[1][0] == token.COLON:
                upper = self.com_node(node[2])
            else:
                lower = self.com_node(node[1])
        elif len(node) == 4:
            lower = self.com_node(node[1])
            upper = self.com_node(node[3])
        return Slice(primary, assigning, lower, upper)

    def get_docstring(self, node, n=None):
        if n is None:
            n = node[0]
            node = node[1:]
        if n == symbol.suite:
            if len(node) == 1:
                return self.get_docstring(node[0])
            for sub in node:
                if sub[0] == symbol.stmt:
                    return self.get_docstring(sub)
            return None
        if n == symbol.file_input:
            for sub in node:
                if sub[0] == symbol.stmt:
                    return self.get_docstring(sub)
            return None
        if n == symbol.atom:
            if node[0][0] == token.STRING:
                s = ''
                for t in node:
                    s = s + eval(t[1])
                return s
            return None
        if n == symbol.stmt or n == symbol.simple_stmt \
           or n == symbol.small_stmt:
            return self.get_docstring(node[0])
        if n in _doc_nodes and len(node) == 1:
            return self.get_docstring(node[0])
        return None


_doc_nodes = [
    symbol.expr_stmt,
    symbol.testlist,
    symbol.testlist_safe,
    symbol.test,
    symbol.and_test,
    symbol.not_test,
    symbol.comparison,
    symbol.expr,
    symbol.xor_expr,
    symbol.and_expr,
    symbol.shift_expr,
    symbol.arith_expr,
    symbol.term,
    symbol.factor,
    symbol.power,
    ]

# comp_op: '<' | '>' | '=' | '>=' | '<=' | '<>' | '!=' | '=='
#             | 'in' | 'not' 'in' | 'is' | 'is' 'not'
_cmp_types = {
    token.LESS : '<',
    token.GREATER : '>',
    token.EQEQUAL : '==',
    token.EQUAL : '==',
    token.LESSEQUAL : '<=',
    token.GREATEREQUAL : '>=',
    token.NOTEQUAL : '!=',
    }

_legal_node_types = [
    symbol.funcdef,
    symbol.classdef,
    symbol.stmt,
    symbol.small_stmt,
    symbol.flow_stmt,
    symbol.simple_stmt,
    symbol.compound_stmt,
    symbol.expr_stmt,
    symbol.print_stmt,
    symbol.del_stmt,
    symbol.pass_stmt,
    symbol.break_stmt,
    symbol.continue_stmt,
    symbol.return_stmt,
    symbol.raise_stmt,
    symbol.import_stmt,
    symbol.global_stmt,
    symbol.exec_stmt,
    symbol.assert_stmt,
    symbol.if_stmt,
    symbol.while_stmt,
    symbol.for_stmt,
    symbol.try_stmt,
    symbol.suite,
    symbol.testlist,
    symbol.testlist_safe,
    symbol.test,
    symbol.and_test,
    symbol.not_test,
    symbol.comparison,
    symbol.exprlist,
    symbol.expr,
    symbol.xor_expr,
    symbol.and_expr,
    symbol.shift_expr,
    symbol.arith_expr,
    symbol.term,
    symbol.factor,
    symbol.power,
    symbol.atom,
    ]

if hasattr(symbol, 'yield_stmt'):
    _legal_node_types.append(symbol.yield_stmt)

_assign_types = [
    symbol.test,
    symbol.and_test,
    symbol.not_test,
    symbol.comparison,
    symbol.expr,
    symbol.xor_expr,
    symbol.and_expr,
    symbol.shift_expr,
    symbol.arith_expr,
    symbol.term,
    symbol.factor,
    ]

import types
_names = {}
for k, v in symbol.sym_name.items():
    _names[k] = v
for k, v in token.tok_name.items():
    _names[k] = v

def debug_tree(tree):
    l = []
    for elt in tree:
        if type(elt) == types.IntType:
            l.append(_names.get(elt, elt))
        elif type(elt) == types.StringType:
            l.append(elt)
        else:
            l.append(debug_tree(elt))
    return l
