from c_analyzer_common import SOURCE_DIRS, PYTHON
from c_analyzer_common.info import UNKNOWN, Variable
from c_symbols import (
        info as s_info,
        find as s_find,
        )


# XXX needs tests:
# * vars_from_source
# * vars_from_preprocessed


def _remove_cached(cache, var):
    if not cache:
        return
    try:
        cached = cache[var.filename]
        cached.remove(var)
    except (KeyError, IndexError):
        pass


def vars_from_binary(binfile=PYTHON, *,
                     known=None,
                     dirnames=SOURCE_DIRS,
                     _iter_vars=s_find.variables,
                     _get_symbol_resolver=s_find.get_resolver,
                     ):
    """Yield a Variable for each found Symbol.

    Details are filled in from the given "known" variables and types.
    """
    cache = {}
    resolve = _get_symbol_resolver(known, dirnames,
                                   perfilecache=cache,
                                   )
    for var, symbol in _iter_vars(binfile, resolve=resolve):
        if var is None:
            var = Variable(symbol.id, UNKNOWN, UNKNOWN)
        yield var
        _remove_cached(cache, var)


def vars_from_source(filenames, *,
                     known=None,
                     ):
    """Yield a Variable for each declaration in the raw source code.

    Details are filled in from the given "known" variables and types.
    """
    raise NotImplementedError


def vars_from_preprocessed(filenames, *,
                           known=None,
                           ):
    """Yield a Variable for each found declaration.

    Details are filled in from the given "known" variables and types.
    """
    raise NotImplementedError
