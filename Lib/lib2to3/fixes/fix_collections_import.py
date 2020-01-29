"""Fixer for abstract base classes which are moved from collections to
collections.abc
"""

import collections.abc as _abc

from lib2to3 import fixer_base
from lib2to3.fixer_util import Comma, FromImport, find_indentation, Newline, syms, token


class FixCollectionsImport(fixer_base.BaseFix):
    BM_compatible = True
    PATTERN = """
              import_from< 'from' package='collections' 'import' imports=any >
              """ %(locals())

    def transform(self, node, results):
        imports = results['imports']

        if imports.type == syms.import_as_name or not imports.children:
            children = [imports]
        else:
            children = imports.children

        abc_children = []

        for child in children[::2]:
            if child.type == token.NAME:
                member = child.value
                name_node = child
            elif child.type == token.STAR:
                return
            else:
                assert child.type == syms.import_as_name
                name_node = child.children[0]

            member_name = name_node.value
            if hasattr(_abc, member_name):
                node.changed()
                if abc_children:
                    abc_children.append(Comma())
                abc_children.append(name_node.clone())
                child.value = None
                child.remove()

        # Make sure the import statement is still sane
        children = imports.children[:] or [imports]
        remove_comma = True
        for child in children:
            if remove_comma and child.type == token.COMMA:
                child.value = None
                child.remove()
            else:
                remove_comma ^= True

        while children and children[-1].type == token.COMMA:
            children.pop().remove()

        if (not (imports.children or getattr(imports, 'value', None)) or
            imports.parent is None) and abc_children:
            p = node.prefix
            node = FromImport('collections.abc', abc_children)
            node.prefix = p
        elif abc_children:
            new_node = FromImport('collections.abc', abc_children)
            new_node.prefix = find_indentation(node)
            node.append_child(Newline())
            node.append_child(new_node)

        return node
