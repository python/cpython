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
                     filenames=None,
                     handle_id=None,
                     check_filename=None,
                     handle_var=Variable.from_id,
                     _iter_vars=s_find.variables,
                     _get_symbol_resolver=s_find.get_resolver,
                     ):
    """Yield a Variable for each found Symbol.

    Details are filled in from the given "known" variables and types.
    """
    cache = {}
    resolve = _get_symbol_resolver(filenames, known,
                                   handle_var=handle_var,
                                   check_filename=check_filename,
                                   perfilecache=cache,
                                   )
    for var, symbol in _iter_vars(binfile,
                                  resolve=resolve,
                                  handle_id=handle_id,
                                  ):
        if var is None:
            var = Variable(symbol.id, UNKNOWN, UNKNOWN)
        yield var
        _remove_cached(cache, var)


def vars_from_source(filenames, *,
                     preprocessed=None,
                     known=None,
                     handle_id=None,
                     handle_var=Variable.from_id,
                     iter_vars=p_find.variables,
                     ):
    """Yield a Variable for each declaration in the raw source code.

    Details are filled in from the given "known" variables and types.
    """
    cache = {}
    for varid, decl in iter_vars(filenames or (),
                                 perfilecache=cache,
                                 preprocessed=preprocessed,
                                 known=known,
                                 handle_id=handle_id,
                                 ):
        var = handle_var(varid, decl)
        yield var
        _remove_cached(cache, var)
