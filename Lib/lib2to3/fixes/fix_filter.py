# Copyright 2007 Google, Inc. All Rights Reserved.
# Licensed to PSF under a Contributor Agreement.

"""Fixer that changes filter(F, X) into list(filter(F, X)).

We avoid the transformation if the filter() call is directly contained
in iter(<>), list(<>), tuple(<>), sorted(<>), ...join(<>), or
for V in <>:.

NOTE: This is still not correct if the original code was depending on
filter(F, X) to return a string if X is a string and a tuple if X is a
tuple.  That would require type inference, which we don't do.  Let
Python 2.6 figure it out.
"""

# Local imports
from ..pgen2 import token
from . import basefix
from .util import Name, Call, ListComp, does_tree_import, in_special_context

class FixFilter(basefix.BaseFix):

    PATTERN = """
    filter_lambda=power<
        'filter'
        trailer<
            '('
            arglist<
                lambdef< 'lambda'
                         (fp=NAME | vfpdef< '(' fp=NAME ')'> ) ':' xp=any
                >
                ','
                it=any
            >
            ')'
        >
    >
    |
    power<
        'filter'
        trailer< '(' arglist< none='None' ',' seq=any > ')' >
    >
    |
    power<
        'filter'
        args=trailer< '(' [any] ')' >
    >
    """

    def start_tree(self, *args):
        super(FixFilter, self).start_tree(*args)
        self._new_filter = None

    def has_new_filter(self, node):
        if self._new_filter is not None:
            return self._new_filter
        self._new_filter = does_tree_import('future_builtins', 'filter', node)
        return self._new_filter

    def transform(self, node, results):
        if self.has_new_filter(node):
            # If filter is imported from future_builtins, we don't want to
            # do anything here.
            return

        if "filter_lambda" in results:
            new = ListComp(results.get("fp").clone(),
                           results.get("fp").clone(),
                           results.get("it").clone(),
                           results.get("xp").clone())

        elif "none" in results:
            new = ListComp(Name("_f"),
                           Name("_f"),
                           results["seq"].clone(),
                           Name("_f"))

        else:
            if in_special_context(node):
                return None
            new = node.clone()
            new.set_prefix("")
            new = Call(Name("list"), [new])
        new.set_prefix(node.get_prefix())
        return new
