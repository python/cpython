"""Fixer for reload().

reload(s) -> imp.reload(s)"""

# Local imports
from .. import fixer_base
from ..fixer_util import ImportAndCall, touch_import


class FixReload(fixer_base.BaseFix):
    BM_compatible = True
    order = "pre"

    PATTERN = """
    power< 'reload'
           trailer< lpar='('
                    ( not(arglist | argument<any '=' any>) obj=any
                      | obj=arglist<(not argument<any '=' any>) any ','> )
                    rpar=')' >
           after=any*
    >
    """

    def transform(self, node, results):
        names = ('imp', 'reload')
        new = ImportAndCall(node, results, names)
        touch_import(None, 'imp', node)
        return new
