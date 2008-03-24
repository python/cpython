""" Fixer for imports of itertools.(imap|ifilter|izip|ifilterfalse) """

# Local imports
from . import basefix
from .util import BlankLine

class FixItertoolsImports(basefix.BaseFix):
    PATTERN = """
              import_from< 'from' 'itertools' 'import' imports=any >
              """ %(locals())

    def transform(self, node, results):
        imports = results['imports']
        children = imports.children[:] or [imports]
        for child in children:
            if not hasattr(child, 'value'):
                # Handle 'import ... as ...'
                continue
            if child.value in ('imap', 'izip', 'ifilter'):
                # The value must be set to none in case child == import,
                # so that the test for empty imports will work out
                child.value = None
                child.remove()
            elif child.value == 'ifilterfalse':
                node.changed()
                child.value = 'filterfalse'

        # Make sure the import statement is still sane
        children = imports.children[:] or [imports]
        remove_comma = True
        for child in children:
            if remove_comma and getattr(child, 'value', None) == ',':
                child.remove()
            else:
                remove_comma ^= True

        if str(children[-1]) == ',':
            children[-1].remove()

        # If there are no imports left, just get rid of the entire statement
        if not (imports.children or getattr(imports, 'value', None)):
            p = node.get_prefix()
            node = BlankLine()
            node.prefix = p
        return node
