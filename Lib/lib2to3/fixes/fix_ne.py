# Copyright 2006 Google, Inc. All Rights Reserved.
# Licensed to PSF under a Contributor Agreement.

"""Fixer that turns <> into !=."""

# Local imports
from .. import pytree
from ..pgen2 import token
from . import basefix


class FixNe(basefix.BaseFix):
    # This is so simple that we don't need the pattern compiler.

    def match(self, node):
        # Override
        return node.type == token.NOTEQUAL and node.value == "<>"

    def transform(self, node, results):
      new = pytree.Leaf(token.NOTEQUAL, "!=")
      new.set_prefix(node.get_prefix())
      return new
