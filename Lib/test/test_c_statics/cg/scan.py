from . import info, symbols


def iter_statics(dirnames, *,
                 _iter_symbols=symbols.iter_binary,
                 ):
    """Yield a StaticVar for each one found in the file."""
    for symbol in _iter_symbols(dirnames):
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
