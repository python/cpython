import re

from ._regexes import (
    _ind,
    STRING_LITERAL,
    VAR_DECL as _VAR_DECL,
)


def log_match(group, m):
    from . import _logger
    _logger.debug(f'matched <{group}> ({m.group(0)})')


#############################
# regex utils

def set_capture_group(pattern, group, *, strict=True):
    old = f'(?:  # <{group}>'
    if strict and f'(?:  # <{group}>' not in pattern:
        raise ValueError(f'{old!r} not found in pattern')
    return pattern.replace(old, f'(  # <{group}>', 1)


def set_capture_groups(pattern, groups, *, strict=True):
    for group in groups:
        pattern = set_capture_group(pattern, group, strict=strict)
    return pattern


#############################
# syntax-related utils

_PAREN_RE = re.compile(rf'''
    (?:
        (?:
            [^'"()]*
            {_ind(STRING_LITERAL, 3)}
         )*
        [^'"()]*
        (?:
            ( [(] )
            |
            ( [)] )
         )
     )
    ''', re.VERBOSE)


def match_paren(text, depth=0):
    pos = 0
    while (m := _PAREN_RE.match(text, pos)):
        pos = m.end()
        _open, _close = m.groups()
        if _open:
            depth += 1
        else:  # _close
            depth -= 1
            if depth == 0:
                return pos
    else:
        raise ValueError(f'could not find matching parens for {text!r}')


VAR_DECL = set_capture_groups(_VAR_DECL, (
    'STORAGE',
    'TYPE_QUAL',
    'TYPE_SPEC',
    'DECLARATOR',
    'IDENTIFIER',
    'WRAPPED_IDENTIFIER',
    'FUNC_IDENTIFIER',
))


def parse_var_decl(decl):
    m = re.match(VAR_DECL, decl, re.VERBOSE)
    (storage, typequal, typespec, declarator,
     name,
     wrappedname,
     funcptrname,
     ) = m.groups()
    if name:
        kind = 'simple'
    elif wrappedname:
        kind = 'wrapped'
        name = wrappedname
    elif funcptrname:
        kind = 'funcptr'
        name = funcptrname
    else:
        raise NotImplementedError
    abstract = declarator.replace(name, '')
    vartype = {
        'storage': storage,
        'typequal': typequal,
        'typespec': typespec,
        'abstract': abstract,
    }
    return (kind, name, vartype)


#############################
# parser state utils

# XXX Drop this or use it!
def iter_results(results):
    if not results:
        return
    if callable(results):
        results = results()

    for result, text in results():
        if result:
            yield result, text
