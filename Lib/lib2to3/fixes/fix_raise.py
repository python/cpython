"""Fixer for 'raise E, V, T'

raise         -> raise
raise E       -> raise E
raise E, V    -> raise E(V)
raise E, V, T -> raise E(V).with_traceback(T)

raise (((E, E'), E''), E'''), V -> raise E(V)
raise "foo", V, T               -> warns about string exceptions


CAVEATS:
1) "raise E, V" will be incorrectly translated if V is an exception
   instance. The correct Python 3 idiom is

        raise E from V

   but since we can't detect instance-hood by syntax alone and since
   any client code would have to be changed as well, we don't automate
   this.
"""
# Author: Collin Winter

# Local imports
from .. import pytree
from ..pgen2 import token
from .. import fixer_base
from ..fixer_util import Name, Call, Attr, ArgList, is_tuple

class FixRaise(fixer_base.BaseFix):

    PATTERN = """
    raise_stmt< 'raise' exc=any [',' val=any [',' tb=any]] >
    """

    def transform(self, node, results):
        syms = self.syms

        exc = results["exc"].clone()
        if exc.type is token.STRING:
            self.cannot_convert(node, "Python 3 does not support string exceptions")
            return

        # Python 2 supports
        #  raise ((((E1, E2), E3), E4), E5), V
        # as a synonym for
        #  raise E1, V
        # Since Python 3 will not support this, we recurse down any tuple
        # literals, always taking the first element.
        if is_tuple(exc):
            while is_tuple(exc):
                # exc.children[1:-1] is the unparenthesized tuple
                # exc.children[1].children[0] is the first element of the tuple
                exc = exc.children[1].children[0].clone()
            exc.set_prefix(" ")

        if "val" not in results:
            # One-argument raise
            new = pytree.Node(syms.raise_stmt, [Name("raise"), exc])
            new.set_prefix(node.get_prefix())
            return new

        val = results["val"].clone()
        if is_tuple(val):
            args = [c.clone() for c in val.children[1:-1]]
        else:
            val.set_prefix("")
            args = [val]

        if "tb" in results:
            tb = results["tb"].clone()
            tb.set_prefix("")

            e = Call(exc, args)
            with_tb = Attr(e, Name('with_traceback')) + [ArgList([tb])]
            new = pytree.Node(syms.simple_stmt, [Name("raise")] + with_tb)
            new.set_prefix(node.get_prefix())
            return new
        else:
            return pytree.Node(syms.raise_stmt,
                               [Name("raise"), Call(exc, args)],
                               prefix=node.get_prefix())
