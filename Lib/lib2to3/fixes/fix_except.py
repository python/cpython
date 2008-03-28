"""Fixer for except statements with named exceptions.

The following cases will be converted:

- "except E, T:" where T is a name:

    except E as T:

- "except E, T:" where T is not a name, tuple or list:

        except E as t:
            T = t

    This is done because the target of an "except" clause must be a
    name.

- "except E, T:" where T is a tuple or list literal:

        except E as t:
            T = t.args
"""
# Author: Collin Winter

# Local imports
from .. import pytree
from ..pgen2 import token
from . import basefix
from .util import Assign, Attr, Name, is_tuple, is_list, reversed

def find_excepts(nodes):
    for i, n in enumerate(nodes):
        if isinstance(n, pytree.Node):
            if n.children[0].value == 'except':
                yield (n, nodes[i+2])

class FixExcept(basefix.BaseFix):

    PATTERN = """
    try_stmt< 'try' ':' suite
                  cleanup=(except_clause ':' suite)+
                  tail=(['except' ':' suite]
                        ['else' ':' suite]
                        ['finally' ':' suite]) >
    """

    def transform(self, node, results):
        syms = self.syms

        tail = [n.clone() for n in results["tail"]]

        try_cleanup = [ch.clone() for ch in results["cleanup"]]
        for except_clause, e_suite in find_excepts(try_cleanup):
            if len(except_clause.children) == 4:
                (E, comma, N) = except_clause.children[1:4]
                comma.replace(Name("as", prefix=" "))

                if N.type != token.NAME:
                    # Generate a new N for the except clause
                    new_N = Name(self.new_name(), prefix=" ")
                    target = N.clone()
                    target.set_prefix("")
                    N.replace(new_N)
                    new_N = new_N.clone()

                    # Insert "old_N = new_N" as the first statement in
                    #  the except body. This loop skips leading whitespace
                    #  and indents
                    #TODO(cwinter) suite-cleanup
                    suite_stmts = e_suite.children
                    for i, stmt in enumerate(suite_stmts):
                        if isinstance(stmt, pytree.Node):
                            break

                    # The assignment is different if old_N is a tuple or list
                    # In that case, the assignment is old_N = new_N.args
                    if is_tuple(N) or is_list(N):
                        assign = Assign(target, Attr(new_N, Name('args')))
                    else:
                        assign = Assign(target, new_N)

                    #TODO(cwinter) stopgap until children becomes a smart list
                    for child in reversed(suite_stmts[:i]):
                        e_suite.insert_child(0, child)
                    e_suite.insert_child(i, assign)
                elif N.get_prefix() == "":
                    # No space after a comma is legal; no space after "as",
                    # not so much.
                    N.set_prefix(" ")

        #TODO(cwinter) fix this when children becomes a smart list
        children = [c.clone() for c in node.children[:3]] + try_cleanup + tail
        return pytree.Node(node.type, children)
