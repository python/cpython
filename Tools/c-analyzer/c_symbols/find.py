import os
import os.path
import shutil

from c_analyzer_common import PYTHON, known, files
from c_analyzer_common.info import UNKNOWN, Variable
from c_parser import find as p_find

from . import _nm
from .info import Symbol

# XXX need tests:
# * get_resolver
# * symbol
# * symbols
# * variables


def _resolve_known(symbol, knownvars):
    for varid in knownvars:
        if symbol.match(varid):
            break
    else:
        return None
    return knownvars.pop(varid)


def get_resolver(known=None, dirnames=(), *,
                 filenames=None,
                 perfilecache=None,
                 preprocessed=False,
                 _iter_files=files.iter_files_by_suffix,
                 _from_source=p_find.variable_from_id,
                 ):
    """Return a "resolver" func for the given known vars/types and filenames.

    The returned func takes a single Symbol and returns a corresponding
    Variable.  If the symbol was located then the variable will be
    valid, populated with the corresponding information.  Otherwise None
    is returned.
    """
    knownvars = (known or {}).get('variables')
    if filenames:
        filenames = list(filenames)
        def check_filename(filename):
            return filename in filenames
    elif dirnames:
        dirnames = [d if d.endswith(os.path.sep) else d + os.path.sep
                    for d in dirnames]
        filenames = list(_iter_files(dirnames, ('.c', '.h')))
        def check_filename(filename):
            for dirname in dirnames:
                if filename.startswith(dirname):
                    return True
            else:
                return False

    if knownvars:
        knownvars = dict(knownvars)  # a copy
        if filenames:
            def resolve(symbol):
                # XXX Check "found" instead?
                if not check_filename(symbol.filename):
                    return None
                found = _resolve_known(symbol, knownvars)
                if found is None:
                    #return None
                    found = _from_source(symbol, filenames,
                                         perfilecache=perfilecache,
                                         preprocessed=preprocessed,
                                         )
                return found
        else:
            def resolve(symbol):
                return _resolve_known(symbol, knownvars)
    elif filenames:
        def resolve(symbol):
            return _from_source(symbol, filenames,
                                perfilecache=perfilecache,
                                preprocessed=preprocessed,
                                )
    else:
        def resolve(symbol):
            return None
    return resolve


def symbol(symbol, filenames, known=None, *,
           perfilecache=None,
           preprocessed=False,
           _get_resolver=get_resolver,
           ):
    """Return the Variable matching the given symbol.
    
    "symbol" can be one of several objects:

    * Symbol - use the contained info
    * name (str) - look for a global variable with that name
    * (filename, name) - look for named global in file
    * (filename, funcname, name) - look for named local in file

    A name is always required.  If the filename is None, "", or
    "UNKNOWN" then all files will be searched.  If the funcname is
    "" or "UNKNOWN" then only local variables will be searched for.
    """
    resolve = _get_resolver(known, filenames,
                            perfilecache=perfilecache,
                            preprocessed=preprocessed,
                            )
    return resolve(symbol)


def _get_platform_tool():
    if os.name == 'nt':
        # XXX Support this.
        raise NotImplementedError
    elif nm := shutil.which('nm'):
        return lambda b: _nm.iter_symbols(b, nm=nm)
    else:
        raise NotImplementedError


def symbols(binfile=PYTHON, *,
            _file_exists=os.path.exists,
            _get_platform_tool=_get_platform_tool,
            ):
    """Yield a Symbol for each one found in the binary."""
    if not _file_exists(binfile):
        raise Exception('executable missing (need to build it first?)')

    _iter_symbols = _get_platform_tool()
    yield from _iter_symbols(binfile)


def variables(binfile=PYTHON, *,
              resolve=(lambda s: symbol(s, ())),
              _iter_symbols=symbols,
              ):
    """Yield (Variable, Symbol) for each found symbol."""
    for symbol in _iter_symbols(binfile):
        if symbol.kind != Symbol.KIND.VARIABLE:
            continue
        var = resolve(symbol)
        #if var is None:
        #    var = Variable(symbol.id, UNKNOWN)
        yield var, symbol
