"""Fixer that turns / into //."""

# Local imports
from .. import pytree
from ..pgen2 import token
from .. import fixer_base


class FixDiv(fixer_base.BaseFix):
    # This is so simple that we don't need the pattern compiler.

    _accept_type = token.SLASH

    def match(self, node):
        # Override
        return node.value == "/"

    def transform(self, node, results):
        new = pytree.Leaf(token.DOUBLESLASH, "//", prefix=node.prefix)
        return new
