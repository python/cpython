"""Fixer for basestring -> str."""
# Author: Christian Heimes

# Local imports
from . import basefix
from .util import Name

class FixBasestring(basefix.BaseFix):

    PATTERN = "'basestring'"

    def transform(self, node, results):
        return Name("str", prefix=node.get_prefix())
