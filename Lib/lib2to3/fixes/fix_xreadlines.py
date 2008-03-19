"""Fix "for x in f.xreadlines()" -> "for x in f".

This fixer will also convert g(f.xreadlines) into g(f.__iter__)."""
# Author: Collin Winter

# Local imports
from .import basefix
from .util import Name


class FixXreadlines(basefix.BaseFix):
    PATTERN = """
    power< call=any+ trailer< '.' 'xreadlines' > trailer< '(' ')' > >
    |
    power< any+ trailer< '.' no_call='xreadlines' > >
    """

    def transform(self, node, results):
        no_call = results.get("no_call")

        if no_call:
            no_call.replace(Name("__iter__", prefix=no_call.get_prefix()))
        else:
            node.replace([x.clone() for x in results["call"]])
