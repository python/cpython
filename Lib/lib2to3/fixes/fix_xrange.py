# Copyright 2007 Google, Inc. All Rights Reserved.
# Licensed to PSF under a Contributor Agreement.

"""Fixer that changes xrange(...) into range(...)."""

# Local imports
from .. import fixer_base
from ..fixer_util import Name, Call, consuming_calls
from .. import patcomp


class FixXrange(fixer_base.BaseFix):

    PATTERN = """
              power<
                 (name='range'|name='xrange') trailer< '(' args=any ')' >
              rest=any* >
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
            range_call = Call(Name("range"), [results["args"].clone()])
            # Encase the range call in list().
            list_call = Call(Name("list"), [range_call],
                             prefix=node.get_prefix())
            # Put things that were after the range() call after the list call.
            for n in results["rest"]:
                list_call.append_child(n)
            return list_call
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
