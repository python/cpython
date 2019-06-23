#import os
#import os.path

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


#def _iter_files(dirnames):
#    for dirname in dirnames:
#        for parent, _, names in os.walk(dirname):
#            for name in names:
#                yield os.path.join(parent, name)
