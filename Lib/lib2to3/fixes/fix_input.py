"""Fixer that changes input(...) into eval(input(...))."""
# Author: Andre Roberge

# Local imports
from . import basefix
from .util import Call, Name
from .. import patcomp


context = patcomp.compile_pattern("power< 'eval' trailer< '(' any ')' > >")


class FixInput(basefix.BaseFix):

    PATTERN = """
              power< 'input' args=trailer< '(' [any] ')' > >
              """

    def transform(self, node, results):
        # If we're already wrapped in a eval() call, we're done.
        if context.match(node.parent.parent):
            return

        new = node.clone()
        new.set_prefix("")
        return Call(Name("eval"), [new], prefix=node.get_prefix())
