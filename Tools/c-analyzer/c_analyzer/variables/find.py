from ..common import files
from ..common.info import UNKNOWN
from ..parser import (
        find as p_find,
        )
from ..symbols import (
        info as s_info,
        find as s_find,
        )
from .info import Variable

# XXX need tests:
# * vars_from_source


def _remove_cached(cache, var):
    if not cache:
        return
    try:
        cached = cache[var.filename]
        cached.remove(var)
    except (KeyError, IndexError):
        pass


def vars_from_binary(binfile, *,
                     known=None,
                     dirnames=None,
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


def vars_from_source(filenames=None, *,
                     iter_vars=p_find.variables,
                     preprocessed=None,
                     known=None,  # for types
                     dirnames=None,
                     _iter_files=files.iter_files_by_suffix,
                     ):
    """Yield a Variable for each declaration in the raw source code.

    Details are filled in from the given "known" variables and types.
    """
    if filenames is None:
        if not dirnames:
            return
        filenames = _iter_files(dirnames, ('.c', '.h'))
    cache = {}
    for var in _iter_vars(filenames,
                          perfilecache=cache,
                          preprocessed=preprocessed,
                          ):
        yield var
        _remove_cached(cache, var)
