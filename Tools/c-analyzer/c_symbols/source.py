from c_analyzer_common import files
from c_analyzer_common.info import UNKNOWN
from c_parser import declarations


# XXX need tests:
# * find_symbol()

def find_symbol(name, dirnames, *,
                _perfilecache,
                _iter_files=files.iter_files_by_suffix,
                **kwargs
                ):
    """Return (filename, funcname, vartype) for the matching Symbol."""
    filenames = _iter_files(dirnames, ('.c', '.h'))
    return _find_symbol(name, filenames, _perfilecache, **kwargs)


def _get_symbols(filename, *,
                 _iter_variables=declarations.iter_variables,
                 ):
    """Return the list of Symbols found in the given file."""
    symbols = {}
    for funcname, name, vartype in _iter_variables(filename):
        if not funcname:
            continue
        try:
            instances = symbols[name]
        except KeyError:
            instances = symbols[name] = []
        instances.append((funcname, vartype))
    return symbols


def _find_symbol(name, filenames, _perfilecache, *,
                _get_local_symbols=_get_symbols,
                ):
    for filename in filenames:
        try:
            symbols = _perfilecache[filename]
        except KeyError:
            symbols = _perfilecache[filename] = _get_local_symbols(filename)

        try:
            instances = symbols[name]
        except KeyError:
            continue

        funcname, vartype = instances.pop(0)
        if not instances:
            symbols.pop(name)
        return filename, funcname, vartype
    else:
        return UNKNOWN, UNKNOWN, UNKNOWN


def iter_symbols():
    raise NotImplementedError
