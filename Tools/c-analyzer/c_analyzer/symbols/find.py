import os
import os.path
import shutil

from ..common import files
from ..common.info import UNKNOWN, ID
from ..parser import find as p_find

from . import _nm
from .info import Symbol

# XXX need tests:
# * get_resolver()
# * get_resolver_from_dirs()
# * symbol()
# * symbols()
# * variables()


def _resolve_known(symbol, knownvars):
    for varid in knownvars:
        if symbol.match(varid):
            break
    else:
        return None
    return knownvars.pop(varid)


def get_resolver(filenames=None, known=None, *,
                 handle_var,
                 check_filename=None,
                 perfilecache=None,
                 preprocessed=False,
                 _from_source=p_find.variable_from_id,
                 ):
    """Return a "resolver" func for the given known vars/types and filenames.

    "handle_var" is a callable that takes (ID, decl) and returns a
    Variable.  Variable.from_id is a suitable callable.

    The returned func takes a single Symbol and returns a corresponding
    Variable.  If the symbol was located then the variable will be
    valid, populated with the corresponding information.  Otherwise None
    is returned.
    """
    knownvars = (known or {}).get('variables')
    if knownvars:
        knownvars = dict(knownvars)  # a copy
        if filenames:
            if check_filename is None:
                filenames = list(filenames)
                def check_filename(filename):
                    return filename in filenames
            def resolve(symbol):
                # XXX Check "found" instead?
                if not check_filename(symbol.filename):
                    return None
                found = _resolve_known(symbol, knownvars)
                if found is None:
                    #return None
                    varid, decl = _from_source(symbol, filenames,
                                               perfilecache=perfilecache,
                                               preprocessed=preprocessed,
                                               )
                    found = handle_var(varid, decl)
                return found
        else:
            def resolve(symbol):
                return _resolve_known(symbol, knownvars)
    elif filenames:
        def resolve(symbol):
            varid, decl = _from_source(symbol, filenames,
                                       perfilecache=perfilecache,
                                       preprocessed=preprocessed,
                                       )
            return handle_var(varid, decl)
    else:
        def resolve(symbol):
            return None
    return resolve


def get_resolver_from_dirs(dirnames, known=None, *,
                           handle_var,
                           suffixes=('.c',),
                           perfilecache=None,
                           preprocessed=False,
                           _iter_files=files.iter_files_by_suffix,
                           _get_resolver=get_resolver,
                           ):
    """Return a "resolver" func for the given known vars/types and filenames.

    "dirnames" should be absolute paths.  If not then they will be
    resolved relative to CWD.

    See get_resolver().
    """
    dirnames = [d if d.endswith(os.path.sep) else d + os.path.sep
                for d in dirnames]
    filenames = _iter_files(dirnames, suffixes)
    def check_filename(filename):
        for dirname in dirnames:
            if filename.startswith(dirname):
                return True
        else:
            return False
    return _get_resolver(filenames, known,
                         handle_var=handle_var,
                         check_filename=check_filename,
                         perfilecache=perfilecache,
                         preprocessed=preprocessed,
                         )


def symbol(symbol, filenames, known=None, *,
           perfilecache=None,
           preprocessed=False,
           handle_id=None,
           _get_resolver=get_resolver,
           ):
    """Return a Variable for the one matching the given symbol.

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
                            handle_id=handle_id,
                            perfilecache=perfilecache,
                            preprocessed=preprocessed,
                            )
    return resolve(symbol)


def _get_platform_tool():
    if os.name == 'nt':
        # XXX Support this.
        raise NotImplementedError
    elif nm := shutil.which('nm'):
        return lambda b, hi: _nm.iter_symbols(b, nm=nm, handle_id=hi)
    else:
        raise NotImplementedError


def symbols(binfile, *,
            handle_id=None,
            _file_exists=os.path.exists,
            _get_platform_tool=_get_platform_tool,
            ):
    """Yield a Symbol for each one found in the binary."""
    if not _file_exists(binfile):
        raise Exception('executable missing (need to build it first?)')

    _iter_symbols = _get_platform_tool()
    yield from _iter_symbols(binfile, handle_id)


def variables(binfile, *,
              resolve,
              handle_id=None,
              _iter_symbols=symbols,
              ):
    """Yield (Variable, Symbol) for each found symbol."""
    for symbol in _iter_symbols(binfile, handle_id=handle_id):
        if symbol.kind != Symbol.KIND.VARIABLE:
            continue
        var = resolve(symbol) or None
        yield var, symbol
