# Copyright 2007 Google, Inc. All Rights Reserved.
# Licensed to PSF under a Contributor Agreement.

"""Fixer that changes xrange(...) into range(...)."""

# Local imports
from .import basefix
from .util import Name, Call, consuming_calls
from .. import patcomp


class FixXrange(basefix.BaseFix):

    PATTERN = """
              power< (name='range'|name='xrange') trailer< '(' [any] ')' > any* >
              """

    def transform(self, node, results):
        name = results["name"]
        if name.value == "xrange":
            return self.transform_xrange(node, results)
        elif name.value == "range":
            return self.transform_range(node, results)
        else:
            raise ValueError(repr(name))

    def transform_xrange(self, node, results):
        name = results["name"]
        name.replace(Name("range", prefix=name.get_prefix()))

    def transform_range(self, node, results):
        if not self.in_special_context(node):
            arg = node.clone()
            arg.set_prefix("")
            call = Call(Name("list"), [arg])
            call.set_prefix(node.get_prefix())
            return call
        return node

    P1 = "power< func=NAME trailer< '(' node=any ')' > any* >"
    p1 = patcomp.compile_pattern(P1)

    P2 = """for_stmt< 'for' any 'in' node=any ':' any* >
            | comp_for< 'for' any 'in' node=any any* >
            | comparison< any 'in' node=any any*>
         """
    p2 = patcomp.compile_pattern(P2)

    def in_special_context(self, node):
        if node.parent is None:
            return False
        results = {}
        if (node.parent.parent is not None and
               self.p1.match(node.parent.parent, results) and
               results["node"] is node):
            # list(d.keys()) -> list(d.keys()), etc.
            return results["func"].value in consuming_calls
        # for ... in d.iterkeys() -> for ... in d.keys(), etc.
        return self.p2.match(node.parent, results) and results["node"] is node
