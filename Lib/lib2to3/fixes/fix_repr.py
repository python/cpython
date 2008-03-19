# Copyright 2006 Google, Inc. All Rights Reserved.
# Licensed to PSF under a Contributor Agreement.

"""Fixer that transforms `xyzzy` into repr(xyzzy)."""

# Local imports
from .import basefix
from .util import Call, Name


class FixRepr(basefix.BaseFix):

    PATTERN = """
              atom < '`' expr=any '`' >
              """

    def transform(self, node, results):
      expr = results["expr"].clone()

      if expr.type == self.syms.testlist1:
          expr = self.parenthesize(expr)
      return Call(Name("repr"), [expr], prefix=node.get_prefix())
