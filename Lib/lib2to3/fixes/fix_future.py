"""Remove __future__ imports

from __future__ import foo is replaced with an empty line.
"""
# Author: Christian Heimes

# Local imports
from . import basefix
from .util import BlankLine

class FixFuture(basefix.BaseFix):
    PATTERN = """import_from< 'from' module_name="__future__" 'import' any >"""

    def transform(self, node, results):
        return BlankLine()
