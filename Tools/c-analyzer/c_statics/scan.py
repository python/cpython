from c_parser import info
from c_symbols import binary as b_symbols, source as s_symbols


# Once we start parsing directly, getting symbols from a binary will
# still be useful for tool validation.

def statics_from_symbols(dirnames, iter_symbols):
    """Yield a StaticVar for each found symbol."""
    for symbol in iter_symbols(dirnames):
        if symbol.kind is not info.Symbol.KIND.VARIABLE:
            continue
        if symbol.external:
            continue
        yield info.StaticVar(
                filename=symbol.filename,
                funcname=symbol.funcname or None,
                name=symbol.name,
                vartype='???',
                )


def iter_statics(dirnames, kind=None, *,
                 _from_symbols=statics_from_symbols,
                 ):
    """Yield a StaticVar for each one found in the files."""
    kind = kind or 'platform'

    if kind == 'symbols':
        return _from_symbols(dirnames, s_symbols.iter_symbols)
    elif kind == 'platform':
        return _from_symbols(dirnames, b_symbols.iter_symbols)
    else:
        raise NotImplementedError
