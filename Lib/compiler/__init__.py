"""Package for parsing and compiling Python source code

There are several functions defined at the top level that are imported
from modules contained in the package.

parse(buf) -> AST
    Donverts a string containing Python source code to an abstract
    syntax tree (AST).  The AST is defined in compiler.ast.

parseFile(path) -> AST
    The same as parse(open(path))

walk(ast, visitor, verbose=None)
    Does a pre-order walk over the ast using the visitor instance.
    See compiler.visitor for details.

compile(filename)
    Generates a .pyc file by compilining filename.
"""

from transformer import parse, parseFile
from visitor import walk
from pycodegen import compile

