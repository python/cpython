from .asdl_lexer import ASDLLexer
from .peg_lexer import PEGLexer


def setup(app):
    # Used for highlighting Parser/Python.asdl in library/ast.rst
    app.add_lexer("asdl", ASDLLexer)
    # Used for highlighting Grammar/python.gram in reference/grammar.rst
    app.add_lexer("peg", PEGLexer)

    return {
        "version": "1.0",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
