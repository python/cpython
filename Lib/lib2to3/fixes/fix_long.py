# Copyright 2006 Google, Inc. All Rights Reserved.
# Licensed to PSF under a Contributor Agreement.

"""Fixer that turns 'long' into 'int' everywhere.

This also strips the trailing 'L' or 'l' from long loterals.
"""

# Local imports
from .. import pytree
from . import basefix
from .util import Name, Number


class FixLong(basefix.BaseFix):

    PATTERN = """
    (long_type = 'long' | number = NUMBER)
    """

    static_long = Name("long")
    static_int = Name("int")

    def transform(self, node, results):
        long_type = results.get("long_type")
        number = results.get("number")
        new = None
        if long_type:
            assert node == self.static_long, node
            new = self.static_int.clone()
        if number and node.value[-1] in ("l", "L"):
            new = Number(node.value[:-1])
        if new is not None:
            new.set_prefix(node.get_prefix())
            return new
