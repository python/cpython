# Syntax warnings emitted in different parts of the Python compiler.

# Parser/lexer/lexer.c
x = 1or 0  # line 4

# Parser/tokenizer/helpers.c
'\z'  # line 7

# Parser/string_parser.c
'\400'  # line 10

# _PyCompile_Warn() in Python/codegen.c
assert(x, 'message')  # line 13
x is 1  # line 14

# _PyErr_EmitSyntaxWarning() in Python/ast_preprocess.c
def f():
    try:
        pass
    finally:
        return 42  # line 21
