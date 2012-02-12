# Example custom fixer, derived from fix_raw_input by Andre Roberge

from lib2to3 import fixer_base
from lib2to3.fixer_util import Name


class FixEcho2(fixer_base.BaseFix):

    BM_compatible = True
    PATTERN = """
              power< name='echo2' trailer< '(' [any] ')' > any* >
              """

    def transform(self, node, results):
        name = results['name']
        name.replace(Name('print', prefix=name.prefix))
