"""Adjust some old Python 2 idioms to their modern counterparts.

* Change some type comparisons to isinstance() calls:
    type(x) == T -> isinstance(x, T)
    type(x) is T -> isinstance(x, T)
    type(x) != T -> not isinstance(x, T)
    type(x) is not T -> not isinstance(x, T)

* Change "while 1:" into "while True:".

* Change both

    v = list(EXPR)
    v.sort()
    foo(v)

and the more general

    v = EXPR
    v.sort()
    foo(v)

into

    v = sorted(EXPR)
    foo(v)
"""
# Author: Jacques Frechet, Collin Winter

# Local imports
from lib2to3 import fixer_base
from lib2to3.fixer_util import Call, Comma, Name, Node, syms

CMP = "(n='!=' | '==' | 'is' | n=comp_op< 'is' 'not' >)"
TYPE = "power< 'type' trailer< '(' x=any ')' > >"

class FixIdioms(fixer_base.BaseFix):

    explicit = False # The user must ask for this fixer

    PATTERN = r"""
        isinstance=comparison< %s %s T=any >
        |
        isinstance=comparison< T=any %s %s >
        |
        while_stmt< 'while' while='1' ':' any+ >
        |
        sorted=any<
            any*
            simple_stmt<
              expr_stmt< id1=any '='
                         power< list='list' trailer< '(' (not arglist<any+>) any ')' > >
              >
              '\n'
            >
            sort=
            simple_stmt<
              power< id2=any
                     trailer< '.' 'sort' > trailer< '(' ')' >
              >
              '\n'
            >
            next=any*
        >
        |
        sorted=any<
            any*
            simple_stmt< expr_stmt< id1=any '=' expr=any > '\n' >
            sort=
            simple_stmt<
              power< id2=any
                     trailer< '.' 'sort' > trailer< '(' ')' >
              >
              '\n'
            >
            next=any*
        >
    """ % (TYPE, CMP, CMP, TYPE)

    def match(self, node):
        r = super(FixIdioms, self).match(node)
        # If we've matched one of the sort/sorted subpatterns above, we
        # want to reject matches where the initial assignment and the
        # subsequent .sort() call involve different identifiers.
        if r and "sorted" in r:
            if r["id1"] == r["id2"]:
                return r
            return None
        return r

    def transform(self, node, results):
        if "isinstance" in results:
            return self.transform_isinstance(node, results)
        elif "while" in results:
            return self.transform_while(node, results)
        elif "sorted" in results:
            return self.transform_sort(node, results)
        else:
            raise RuntimeError("Invalid match")

    def transform_isinstance(self, node, results):
        x = results["x"].clone() # The thing inside of type()
        T = results["T"].clone() # The type being compared against
        x.prefix = ""
        T.prefix = " "
        test = Call(Name("isinstance"), [x, Comma(), T])
        if "n" in results:
            test.prefix = " "
            test = Node(syms.not_test, [Name("not"), test])
        test.prefix = node.prefix
        return test

    def transform_while(self, node, results):
        one = results["while"]
        one.replace(Name("True", prefix=one.prefix))

    def transform_sort(self, node, results):
        sort_stmt = results["sort"]
        next_stmt = results["next"]
        list_call = results.get("list")
        simple_expr = results.get("expr")

        if list_call:
            list_call.replace(Name("sorted", prefix=list_call.prefix))
        elif simple_expr:
            new = simple_expr.clone()
            new.prefix = ""
            simple_expr.replace(Call(Name("sorted"), [new],
                                     prefix=simple_expr.prefix))
        else:
            raise RuntimeError("should not have reached here")
        sort_stmt.remove()
        if next_stmt:
            next_stmt[0].prefix = sort_stmt._prefix
