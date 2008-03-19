"""Fix incompatible imports and module references.

Fixes:
  * StringIO -> io
  * cStringIO -> io
  * md5 -> hashlib
"""
# Author: Collin Winter

# Local imports
from . import basefix
from .util import Name, attr_chain, any, set
import __builtin__
builtin_names = [name for name in dir(__builtin__)
                 if name not in ("__name__", "__doc__")]

MAPPING = {"StringIO":  ("io", ["StringIO"]),
           "cStringIO": ("io", ["StringIO"]),
           "__builtin__" : ("builtins", builtin_names), 
          }


def alternates(members):
    return "(" + "|".join(map(repr, members)) + ")"


def build_pattern():
    bare = set()
    for old_module, (new_module, members) in MAPPING.items():
        bare.add(old_module)
        bare.update(members)
        members = alternates(members)
        yield """import_name< 'import' (module=%r
                              | dotted_as_names< any* module=%r any* >) >
              """ % (old_module, old_module)
        yield """import_from< 'from' module_name=%r 'import'
                   ( %s | import_as_name< %s 'as' any >) >
              """ % (old_module, members, members)
        yield """import_from< 'from' module_name=%r 'import' star='*' >
              """ % old_module
        yield """import_name< 'import'
                              dotted_as_name< module_name=%r 'as' any > >
              """ % old_module
        yield """power< module_name=%r trailer< '.' %s > any* >
              """ % (old_module, members)
    yield """bare_name=%s""" % alternates(bare)


class FixImports(basefix.BaseFix):
    PATTERN = "|".join(build_pattern())

    order = "pre" # Pre-order tree traversal

    # Don't match the node if it's within another match
    def match(self, node):
        match = super(FixImports, self).match
        results = match(node)
        if results:
            if any([match(obj) for obj in attr_chain(node, "parent")]):
                return False
            return results
        return False

    def start_tree(self, tree, filename):
        super(FixImports, self).start_tree(tree, filename)
        self.replace = {}

    def transform(self, node, results):
        import_mod = results.get("module")
        mod_name = results.get("module_name")
        bare_name = results.get("bare_name")
        star = results.get("star")

        if import_mod or mod_name:
            new_name, members = MAPPING[(import_mod or mod_name).value]

        if import_mod:
            self.replace[import_mod.value] = new_name
            import_mod.replace(Name(new_name, prefix=import_mod.get_prefix()))
        elif mod_name:
            if star:
                self.cannot_convert(node, "Cannot handle star imports.")
            else:
                mod_name.replace(Name(new_name, prefix=mod_name.get_prefix()))
        elif bare_name:
            bare_name = bare_name[0]
            new_name = self.replace.get(bare_name.value)
            if new_name:
              bare_name.replace(Name(new_name, prefix=bare_name.get_prefix()))
