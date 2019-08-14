"""Fix incompatible imports and module references that get fix_renames after
fix_imports.
"""
from . import fix_imports


MAPPING = {
            'Cookie': 'http.cookies',
          }


class FixImports3(fix_imports.FixImports):

    run_order = 7

    mapping = MAPPING
