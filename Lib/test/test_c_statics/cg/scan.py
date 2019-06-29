from . import info, symbols


def iter_statics(dirnames, *,
                 _iter_symbols=symbols.iter_binary,
                 ):
    """Yield a StaticVar for each one found in the file."""
    for symbol in _iter_symbols():
        if symbol.kind is not symbols.Symbol.KIND.VARIABLE:
            continue
        if symbol.external:
            continue
        yield info.StaticVar(
                filename=symbol.filename or '<???>',
                funcname='<???>' if not symbol.filename else None,
                name=symbol.name,
                vartype='???',
                )
