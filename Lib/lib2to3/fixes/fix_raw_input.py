"""Fixer that changes raw_input(...) into input(...)."""
# Author: Andre Roberge

# Local imports
from .. import fixer_base
from ..fixer_util import Name

class FixRawInput(fixer_base.BaseFix):

    PATTERN = """
              power< name='raw_input' trailer< '(' [any] ')' > any* >
              """

    def transform(self, node, results):
        name = results["name"]
        name.replace(Name("input", prefix=name.get_prefix()))
