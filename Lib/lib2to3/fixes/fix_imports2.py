"""Fix incompatible imports and module references that must be fixed after
fix_imports."""
from . import fix_imports


MAPPING = {
            'whichdb': 'dbm',
            'anydbm': 'dbm',
          }


class FixImports2(fix_imports.FixImports):
    PATTERN = "|".join((fix_imports.build_pattern(MAPPING)))

    order = "post"

    mapping = MAPPING
