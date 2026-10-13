from docutils import nodes
from sphinx.transforms import SphinxTransform

from .asdl_lexer import ASDLLexer
from .peg_lexer import PEGLexer


class StripPEGBlocks(SphinxTransform):
    # Strip PEG blocks like PEGLexer does when highlighting them, so that
    # other builders (gettext in particular) get the text the page shows.
    # Must run before the i18n Locale transform (20).
    default_priority = 10

    def apply(self):
        lexer = PEGLexer()
        for node in self.document.findall(nodes.literal_block):
            if node.get("language") != "peg":
                continue
            tokens = lexer.get_tokens_unprocessed(node.rawsource)
            text = "".join(value for _, _, value in tokens)
            node.rawsource = text
            node[:] = [nodes.Text(text)]


def setup(app):
    # Used for highlighting Parser/Python.asdl in library/ast.rst
    app.add_lexer("asdl", ASDLLexer)
    # Used for highlighting Grammar/python.gram in reference/grammar.rst
    app.add_lexer("peg", PEGLexer)
    app.add_transform(StripPEGBlocks)

    return {
        "version": "1.0",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
