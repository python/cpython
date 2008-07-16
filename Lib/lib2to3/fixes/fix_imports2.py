"""Fix incompatible imports and module references that must be fixed after
fix_imports."""
from . import fix_imports


MAPPING = {
            'whichdb': ('dbm', ['whichdb']),
            'anydbm': ('dbm', ['error', 'open']),
          }


class FixImports2(fix_imports.FixImports):
    PATTERN = "|".join((fix_imports.build_pattern(MAPPING)))

    order = "post"

    mapping = MAPPING
