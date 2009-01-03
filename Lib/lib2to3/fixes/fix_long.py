# Copyright 2006 Google, Inc. All Rights Reserved.
# Licensed to PSF under a Contributor Agreement.

"""Fixer that turns 'long' into 'int' everywhere.
"""

# Local imports
from .. import fixer_base
from ..fixer_util import Name, Number, is_probably_builtin


class FixLong(fixer_base.BaseFix):

    PATTERN = "'long'"

    static_int = Name("int")

    def transform(self, node, results):
        if is_probably_builtin(node):
            new = self.static_int.clone()
            new.set_prefix(node.get_prefix())
            return new
