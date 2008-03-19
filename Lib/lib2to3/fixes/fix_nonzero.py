"""Fixer for __nonzero__ -> __bool__ methods."""
# Author: Collin Winter

# Local imports
from .import basefix
from .util import Name, syms

class FixNonzero(basefix.BaseFix):
    PATTERN = """
    classdef< 'class' any+ ':'
              suite< any*
                     funcdef< 'def' name='__nonzero__'
                              parameters< '(' NAME ')' > any+ >
                     any* > >
    """

    def transform(self, node, results):
        name = results["name"]
        new = Name("__bool__", prefix=name.get_prefix())
        name.replace(new)
