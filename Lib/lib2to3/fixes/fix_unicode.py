"""Fixer that changes unicode to str, unichr to chr, and u"..." into "...".

"""

import re
from ..pgen2 import token
from .. import fixer_base

class FixUnicode(fixer_base.BaseFix):

    PATTERN = "STRING | NAME<'unicode' | 'unichr'>"

    def transform(self, node, results):
        if node.type == token.NAME:
            if node.value == u"unicode":
                new = node.clone()
                new.value = u"str"
                return new
            if node.value == u"unichr":
                new = node.clone()
                new.value = u"chr"
                return new
            # XXX Warn when __unicode__ found?
        elif node.type == token.STRING:
            if re.match(ur"[uU][rR]?[\'\"]", node.value):
                new = node.clone()
                new.value = new.value[1:]
                return new
