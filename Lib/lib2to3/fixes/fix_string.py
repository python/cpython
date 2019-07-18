"""Fixer for string constants.

Renames `lowercase`, `uppercase`, and `letters` with the prefix `ascii_`.

Caution, some user-defined variables with these names might get renamed also.
"""

# Local imports
from .. import fixer_base
from ..fixer_util import Name, token, find_binding, syms, is_import


class FixString(fixer_base.BaseFix):

    BM_compatible = True
    constants = ["uppercase","lowercase","letters"]
    PATTERN = None

    def compile_pattern(self):
        constants = "('"+("'|'".join(self.constants))+"')"
        self.PATTERN = """
                       power< 'string'
                         trailer< '.' const=%(constants)s > >
                       |
                       const=%(constants)s 
                       """ % dict(constants=constants)
        super(FixString, self).compile_pattern()


    def start_tree(self, tree, filename):
        super(FixString, self).start_tree(tree, filename)

        # create list of names imported from 'string' library
        self.string_defs = self.constants
        self.find_stringdot = False

        # from string import *   => use all constants
        if find_binding(None, tree, "string"):
            return

        # import string    => use all constants and only change string.name
        if find_binding("string", tree, None):
            self.find_stringdot = True
            return

        # from string import ... as ...
        # see comment of test_from_import_as_with_package:
        # 'fail if there is an "from ... import ... as ...'
        # find_bindings won't help, so create a local find_imports()

        def find_imports(node):
            for child in node.children:
                if child.type == syms.simple_stmt:
                    #recursive
                    find_imports(child)
                elif child.type == syms.import_from:

                    # only more testing if it's for string library
                    if not child.children[1].type == token.NAME or \
                      ( child.children[1].type == token.NAME \
                        and not child.children[1].value == 'string' ):
                            continue

                    # from string import name   (one import)
                    if child.children[3].type == token.NAME:
                        name = child.children[3].value
                        if name in self.constants:
                            self.string_defs.add(name)
                        continue

                    # from string import name as foo  (one import as)
                    if child.children[3].type == syms.import_as_name:
                        name = child.children[3].children[0].value
                        if name in self.constants:
                            self.string_defs.add(name)

                    # from string import something, something_else, ...
                    # (multiple imports)
                    if child.children[3].type == syms.import_as_names:
                        for something in child.children[3].children:
                            # ... import name as foo, something_else ...
                            if something.type == syms.import_as_name:
                                name = something.children[0].value
                                if name in self.constants:
                                    self.string_defs.add(name)
                            # ... import name, something_else as bar, ...
                            else:
                                if something.type == token.NAME:
                                    name = something.value
                                    if name in self.constants:
                                        self.string_defs.add(name)

        self.string_defs = set()
        find_imports(tree)


    def transform(self, node, results):
        if not results.get('const', None) \
            or ( self.find_stringdot and not syms.power ):
                return
        const = results['const'][0]
        if const.value in self.string_defs:
            assert const.type == token.NAME
            const.replace(Name(("ascii_" + const.value), prefix=node.prefix))
