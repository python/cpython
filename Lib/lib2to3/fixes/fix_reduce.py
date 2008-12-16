# Copyright 2008 Armin Ronacher.
# Licensed to PSF under a Contributor Agreement.

"""Fixer for reduce().

Makes sure reduce() is imported from the functools module if reduce is
used in that module.
"""

from .. import pytree
from .. import fixer_base
from ..fixer_util import Name, Attr, touch_import



class FixReduce(fixer_base.BaseFix):

    PATTERN = """
    power< 'reduce'
        trailer< '('
            arglist< (
                (not(argument<any '=' any>) any ','
                 not(argument<any '=' any>) any) |
                (not(argument<any '=' any>) any ','
                 not(argument<any '=' any>) any ','
                 not(argument<any '=' any>) any)
            ) >
        ')' >
    >
    """

    def transform(self, node, results):
        touch_import('functools', 'reduce', node)
