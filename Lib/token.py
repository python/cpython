"""Token constants."""

__all__ = ['tok_name', 'ISTERMINAL', 'ISNONTERMINAL', 'ISEOF']

tokens = [
    'ENDMARKER',
    'NAME',
    'NUMBER',
    'STRING',
    'NEWLINE',
    'INDENT',
    'DEDENT',

    ('LPAR', '('),
    ('RPAR', ')'),
    ('LSQB', '['),
    ('RSQB', ']'),
    ('COLON', ':'),
    ('COMMA', ','),
    ('SEMI', ';'),
    ('PLUS', '+'),
    ('MINUS', '-'),
    ('STAR', '*'),
    ('SLASH', '/'),
    ('VBAR', '|'),
    ('AMPER', '&'),
    ('LESS', '<'),
    ('GREATER', '>'),
    ('EQUAL', '='),
    ('DOT', '.'),
    ('PERCENT', '%'),
    ('LBRACE', '{'),
    ('RBRACE', '}'),
    ('EQEQUAL', '=='),
    ('NOTEQUAL', '!='),
    ('LESSEQUAL', '<='),
    ('GREATEREQUAL', '>='),
    ('TILDE', '~'),
    ('CIRCUMFLEX', '^'),
    ('LEFTSHIFT', '<<'),
    ('RIGHTSHIFT', '>>'),
    ('DOUBLESTAR', '**'),
    ('PLUSEQUAL', '+='),
    ('MINEQUAL', '-='),
    ('STAREQUAL', '*='),
    ('SLASHEQUAL', '/='),
    ('PERCENTEQUAL', '%='),
    ('AMPEREQUAL', '&='),
    ('VBAREQUAL', '|='),
    ('CIRCUMFLEXEQUAL', '^='),
    ('LEFTSHIFTEQUAL', '<<='),
    ('RIGHTSHIFTEQUAL', '>>='),
    ('DOUBLESTAREQUAL', '**='),
    ('DOUBLESLASH', '//'),
    ('DOUBLESLASHEQUAL', '//='),
    ('AT', '@'),
    ('ATEQUAL', '@='),
    ('RARROW', '->'),
    ('ELLIPSIS', '...'),

    'OP',
    'AWAIT',
    'ASYNC',
    'ERRORTOKEN',
    'COMMENT',
    'NL',
    'ENCODING',

    'N_TOKENS',
]

tok_name = {i: v[0] if isinstance(v, tuple) else v
            for i, v in enumerate(tokens)}
EXACT_TOKEN_TYPES = {x: i
                     for i, v in enumerate(tokens)
                     if isinstance(v, tuple)
                     for x in v[1:]}
del tokens

# Special definitions for cooperation with parser
tok_name[256] = 'NT_OFFSET'

__all__.extend(tok_name.values())
globals().update({name: value for value, name in tok_name.items()})

def ISTERMINAL(x):
    return x < NT_OFFSET

def ISNONTERMINAL(x):
    return x >= NT_OFFSET

def ISEOF(x):
    return x == ENDMARKER
