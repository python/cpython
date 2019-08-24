from c_parser import info, declarations
from c_symbols import (
        info as s_info,
        binary as b_symbols,
        source as s_symbols,
        )


# Once we start parsing directly, getting symbols from a binary will
# still be useful for tool validation.

def statics_from_symbols(dirnames, iter_symbols):
    """Yield a Variable for each found symbol (static only)."""
    for symbol in iter_symbols(dirnames):
        if symbol.kind is not s_info.Symbol.KIND.VARIABLE:
            continue
        if symbol.external:
            continue
        yield info.Variable.from_parts(
                filename=symbol.filename,
                funcname=symbol.funcname or None,
                name=symbol.name,
                vartype='???',
                isstatic=True,
                )


def statics_from_declarations(dirnames, iter_declarations):
    """Yield a Variable for each static variable declaration."""
    for decl in iter_declarations(dirnames):
        if not isinstance(decl, info.Variable):
            continue
        if decl.isstatic:
            continue
        yield decl


def iter_statics(dirnames, kind=None, *,
                 _from_symbols=statics_from_symbols,
                 _from_declarations=statics_from_declarations,
                 ):
    """Yield a Variable for each one found in the files."""
    kind = kind or 'platform'

    if kind == 'symbols':
        return _from_symbols(dirnames, s_symbols.iter_symbols)
    elif kind == 'platform':
        return _from_symbols(dirnames, b_symbols.iter_symbols)
    elif kind == 'declarations':
        return _from_declarations(dirnames, declarations.iter_all)
    elif kind == 'preprocessed':
        return _from_declarations(dirnames, declarations.iter_preprocessed)
    else:
        raise ValueError(f'unsupported kind {kind!r}')
