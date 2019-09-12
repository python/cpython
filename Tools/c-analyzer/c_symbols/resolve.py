import os.path

from c_analyzer_common import files
from c_analyzer_common.info import UNKNOWN
from c_parser import declarations, info
from .info import Symbol
from .source import _find_symbol


# XXX need tests:
# * look_up_known_symbol()
# * symbol_from_source()
# * get_resolver()
# * symbols_to_variables()

def look_up_known_symbol(symbol, knownvars, *,
                         match_files=(lambda f1, f2: f1 == f2),
                         ):
    """Return the known variable matching the given symbol.

    "knownvars" is a mapping of common.ID to parser.Variable.

    "match_files" is used to verify if two filenames point to
    the same file.
    """
    if not knownvars:
        return None

    if symbol.funcname == UNKNOWN:
        if not symbol.filename or symbol.filename == UNKNOWN:
            for varid in knownvars:
                if not varid.funcname:
                    continue
                if varid.name == symbol.name:
                    return knownvars[varid]
            else:
                return None
        else:
            for varid in knownvars:
                if not varid.funcname:
                    continue
                if not match_files(varid.filename, symbol.filename):
                    continue
                if varid.name == symbol.name:
                    return knownvars[varid]
            else:
                return None
    elif not symbol.filename or symbol.filename == UNKNOWN:
        raise NotImplementedError
    else:
        return knownvars.get(symbol.id)


def find_in_source(symbol, dirnames, *,
                   _perfilecache={},
                   _find_symbol=_find_symbol,
                   _iter_files=files.iter_files_by_suffix,
                   ):
    """Return the Variable matching the given Symbol.

    If there is no match then return None.
    """
    if symbol.filename and symbol.filename != UNKNOWN:
        filenames = [symbol.filename]
    else:
        filenames = _iter_files(dirnames, ('.c', '.h'))

    if symbol.funcname and symbol.funcname != UNKNOWN:
        raise NotImplementedError

    (filename, funcname, vartype
     ) = _find_symbol(symbol.name, filenames, _perfilecache)
    if filename == UNKNOWN:
        return None
    return info.Variable(
            id=(filename, funcname, symbol.name),
            vartype=vartype,
            )


def get_resolver(knownvars=None, dirnames=None, *,
                 _look_up_known=look_up_known_symbol,
                 _from_source=find_in_source,
                 ):
    """Return a "resolver" func for the given known vars and dirnames.

    The func takes a single Symbol and returns a corresponding Variable.
    If the symbol was located then the variable will be valid, populated
    with the corresponding information.  Otherwise None is returned.
    """
    if knownvars:
        knownvars = dict(knownvars)  # a copy
        def resolve_known(symbol):
            found = _look_up_known(symbol, knownvars)
            if found is None:
                return None
            elif symbol.funcname == UNKNOWN:
                knownvars.pop(found.id)
            elif not symbol.filename or symbol.filename == UNKNOWN:
                knownvars.pop(found.id)
            return found
        if dirnames:
            def resolve(symbol):
                found = resolve_known(symbol)
                if found is None:
                    return None
                    #return _from_source(symbol, dirnames)
                else:
                    for dirname in dirnames:
                        if not dirname.endswith(os.path.sep):
                            dirname += os.path.sep
                        if found.filename.startswith(dirname):
                            break
                    else:
                        return None
                    return found
        else:
            resolve = resolve_known
    elif dirnames:
        def resolve(symbol):
            return _from_source(symbol, dirnames)
    else:
        def resolve(symbol):
            return None
    return resolve


def symbols_to_variables(symbols, *,
                         resolve=(lambda s: look_up_known_symbol(s, None)),
                         ):
    """Yield the variable the matches each given symbol.

    Use get_resolver() for a "resolve" func to use.
    """
    for symbol in symbols:
        if isinstance(symbol, info.Variable):
            # XXX validate?
            yield symbol
            continue
        if symbol.kind != Symbol.KIND.VARIABLE:
            continue
        resolved = resolve(symbol)
        if resolved is None:
            #raise NotImplementedError(symbol)
            resolved = info.Variable(
                    id=symbol.id,
                    vartype=UNKNOWN,
                    )
        yield resolved
