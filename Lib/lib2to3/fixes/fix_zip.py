"""
Fixer that changes zip(seq0, seq1, ...) into list(zip(seq0, seq1, ...)
unless there exists a 'from future_builtins import zip' statement in the
top-level namespace.

We avoid the transformation if the zip() call is directly contained in
iter(<>), list(<>), tuple(<>), sorted(<>), ...join(<>), or for V in <>:.
"""

# Local imports
from . import basefix
from .util import Name, Call, does_tree_import, in_special_context

class FixZip(basefix.BaseFix):

    PATTERN = """
    power< 'zip' args=trailer< '(' [any] ')' >
    >
    """

    def start_tree(self, *args):
        super(FixZip, self).start_tree(*args)
        self._future_zip_found = None

    def has_future_zip(self, node):
        if self._future_zip_found is not None:
            return self._future_zip_found
        self._future_zip_found = does_tree_import('future_builtins', 'zip', node)
        return self._future_zip_found

    def transform(self, node, results):
        if self.has_future_zip(node):
            # If a future zip has been imported for this file, we won't
            # be making any modifications
            return

        if in_special_context(node):
            return None
        new = node.clone()
        new.set_prefix("")
        new = Call(Name("list"), [new])
        new.set_prefix(node.get_prefix())
        return new
