""" Fixer for imports of itertools.(imap|ifilter|izip|ifilterfalse) """

# Local imports
from lib2to3 import fixer_base
from lib2to3.fixer_util import BlankLine, syms, token


class FixItertoolsImports(fixer_base.BaseFix):
    PATTERN = """
              import_from< 'from' 'itertools' 'import' imports=any >
              """ %(locals())

    def transform(self, node, results):
        imports = results['imports']
        if imports.type == syms.import_as_name or not imports.children:
            children = [imports]
        else:
            children = imports.children
        for child in children[::2]:
            if child.type == token.NAME:
                member = child.value
                name_node = child
            else:
                assert child.type == syms.import_as_name
                name_node = child.children[0]
            member_name = name_node.value
            if member_name in (u'imap', u'izip', u'ifilter'):
                child.value = None
                child.remove()
            elif member_name == u'ifilterfalse':
                node.changed()
                name_node.value = u'filterfalse'

        # Make sure the import statement is still sane
        children = imports.children[:] or [imports]
        remove_comma = True
        for child in children:
            if remove_comma and child.type == token.COMMA:
                child.remove()
            else:
                remove_comma ^= True

        if children[-1].type == token.COMMA:
            children[-1].remove()

        # If there are no imports left, just get rid of the entire statement
        if not (imports.children or getattr(imports, 'value', None)) or \
                imports.parent is None:
            p = node.prefix
            node = BlankLine()
            node.prefix = p
            return node
