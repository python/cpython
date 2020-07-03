"""Fixer that turns 1L into 1, 0755 into 0o755.
"""
# Copyright 2007 Georg Brandl.
# Licensed to PSF under a Contributor Agreement.

# Local imports
from ..pgen2 import token
from .. import fixer_base
from ..fixer_util import Number


class FixNumliterals(fixer_base.BaseFix):
    # This is so simple that we don't need the pattern compiler.

    _accept_type = token.NUMBER

    def is_long(self, node):
        return node.value[-1] in 'Ll'

    def is_octal(self, node):
        return (
            node.value.startswith("0")
            and node.value.isdigit()
            and len(set(node.value)) > 1
        )

    def match(self, node):
        # Override
        return self.is_long(node) or self.is_octal(node)

    def transform(self, node, results):
        if self.is_long(node):
            return Number(node.value[:-1], prefix=node.prefix)
        elif self.is_octal(node):
            return Number("0o" + node.value[1:], prefix=node.prefix)

        return None
