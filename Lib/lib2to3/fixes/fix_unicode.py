"""Fixer that changes unicode to str, unichr to chr, and u"..." into "...".

"""

import re
from ..pgen2 import token
from .. import fixer_base

_mapping = {"unichr" : "chr", "unicode" : "str"}
_literal_re = re.compile(r"[uU][rR]?[\'\"]")

class FixUnicode(fixer_base.BaseFix):
    BM_compatible = True
    PATTERN = "STRING | 'unicode' | 'unichr'"

    def transform(self, node, results):
        if node.type == token.NAME:
            new = node.clone()
            new.value = _mapping[node.value]
            return new
        elif node.type == token.STRING:
            if _literal_re.match(node.value):
                new = node.clone()
                new.value = new.value[1:]
                return new
