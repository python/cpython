from c_analyzer_common import SOURCE_DIRS
from c_analyzer_common.info import UNKNOWN
from c_symbols import (
        info as s_info,
        binary as b_symbols,
        source as s_symbols,
        resolve,
        )
from c_parser import info, declarations


# XXX needs tests:
# * iter_variables

def globals_from_binary(binfile=b_symbols.PYTHON, *,
                        knownvars=None,
                        dirnames=None,
                        _iter_symbols=b_symbols.iter_symbols,
                        _resolve=resolve.symbols_to_variables,
                        _get_symbol_resolver=resolve.get_resolver,
                        ):
    """Yield a Variable for each found Symbol.

    Details are filled in from the given "known" variables and types.
    """
    symbols = _iter_symbols(binfile, find_local_symbol=None)
    #symbols = list(symbols)
    for variable in _resolve(symbols,
                             resolve=_get_symbol_resolver(knownvars, dirnames),
                             ):
        # Skip each non-global variable (unless we couldn't find it).
        # XXX Drop the "UNKNOWN" condition?
        if not variable.isglobal and variable.vartype != UNKNOWN:
            continue
        yield variable


def globals_from_declarations(dirnames=SOURCE_DIRS, *,
                              known=None,
                              ):
    """Yield a Variable for each found declaration.

    Details are filled in from the given "known" variables and types.
    """
    raise NotImplementedError


def iter_variables(kind='platform', *,
                   known=None,
                   dirnames=None,
                   _resolve_symbols=resolve.symbols_to_variables,
                   _get_symbol_resolver=resolve.get_resolver,
                   _symbols_from_binary=b_symbols.iter_symbols,
                   _symbols_from_source=s_symbols.iter_symbols,
                   _iter_raw=declarations.iter_all,
                   _iter_preprocessed=declarations.iter_preprocessed,
                   ):
    """Yield a Variable for each one found (e.g. in files)."""
    kind = kind or 'platform'

    if kind == 'symbols':
        knownvars = (known or {}).get('variables')
        yield from _resolve_symbols(
                _symbols_from_source(dirnames, known),
                resolve=_get_symbol_resolver(knownvars, dirnames),
                )
    elif kind == 'platform':
        knownvars = (known or {}).get('variables')
        yield from _resolve_symbols(
                _symbols_from_binary(find_local_symbol=None),
                resolve=_get_symbol_resolver(knownvars, dirnames),
                )
    elif kind == 'declarations':
        for decl in _iter_raw(dirnames):
            if not isinstance(decl, info.Variable):
                continue
            yield decl
    elif kind == 'preprocessed':
        for decl in _iter_preprocessed(dirnames):
            if not isinstance(decl, info.Variable):
                continue
            yield decl
    else:
        raise ValueError(f'unsupported kind {kind!r}')


def globals(dirnames, known, *,
            kind=None,  # Use the default.
            _iter_variables=iter_variables,
            ):
    """Return a list of (StaticVar, <supported>) for each found global var."""
    for found in _iter_variables(kind, known=known, dirnames=dirnames):
        if not found.isglobal:
            continue
        yield found
