"""Fixer that changes unicode to str, unichr to chr, and u"..." into "...".

"""

import re
from ..pgen2 import token
from .import basefix

class FixUnicode(basefix.BaseFix):

    PATTERN = "STRING | NAME<'unicode' | 'unichr'>"

    def transform(self, node, results):
        if node.type == token.NAME:
            if node.value == "unicode":
                new = node.clone()
                new.value = "str"
                return new
            if node.value == "unichr":
                new = node.clone()
                new.value = "chr"
                return new
            # XXX Warn when __unicode__ found?
        elif node.type == token.STRING:
            if re.match(r"[uU][rR]?[\'\"]", node.value):
                new = node.clone()
                new.value = new.value[1:]
                return new
