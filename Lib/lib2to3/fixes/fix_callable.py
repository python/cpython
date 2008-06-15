# Copyright 2007 Google, Inc. All Rights Reserved.
# Licensed to PSF under a Contributor Agreement.

"""Fixer for callable().

This converts callable(obj) into hasattr(obj, '__call__')."""

# Local imports
from .. import pytree
from .. import fixer_base
from ..fixer_util import Call, Name, String

class FixCallable(fixer_base.BaseFix):

    # Ignore callable(*args) or use of keywords.
    # Either could be a hint that the builtin callable() is not being used.
    PATTERN = """
    power< 'callable'
           trailer< lpar='('
                    ( not(arglist | argument<any '=' any>) func=any
                      | func=arglist<(not argument<any '=' any>) any ','> )
                    rpar=')' >
           after=any*
    >
    """

    def transform(self, node, results):
        func = results["func"]

        args = [func.clone(), String(', '), String("'__call__'")]
        return Call(Name("hasattr"), args, prefix=node.get_prefix())
