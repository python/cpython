# Copyright 2006 Google, Inc. All Rights Reserved.
# Licensed to PSF under a Contributor Agreement.

"""Fixer that turns 'long' into 'int' everywhere.
"""

# Local imports
from .. import pytree
from .. import fixer_base
from ..fixer_util import Name, Number


class FixLong(fixer_base.BaseFix):

    PATTERN = "'long'"

    static_long = Name("long")
    static_int = Name("int")

    def transform(self, node, results):
        assert node == self.static_long, node
        new = self.static_int.clone()
        new.set_prefix(node.get_prefix())
        return new
