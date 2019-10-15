from ..common.info import UNKNOWN, ID

from . import declarations

# XXX need tests:
# * variables
# * variable
# * variable_from_id


def _iter_vars(filenames, preprocessed, *,
               _iter_decls=declarations.iter_all,
               ):
    for filename in filenames or ():
        for kind, funcname, name, decl in _iter_decls(filename,
                                                      preprocessed=preprocessed,
                                                      ):
            if kind != 'variable':
                continue
            yield ID(filename, funcname, name), decl


# XXX Add a "handle_var" arg like we did for get_resolver()?

def variables(*filenames,
              perfilecache=None,
              preprocessed=False,
              _iter_vars=_iter_vars,
              ):
    """Yield (varid, decl) for each variable found in the given files.

    If "preprocessed" is provided (and not False/None) then it is used
    to decide which tool to use to parse the source code after it runs
    through the C preprocessor.  Otherwise the raw
    """
    if len(filenames) == 1 and not (filenames[0], str):
        filenames, = filenames

    if perfilecache is None:
        yield from _iter_vars(filenames, preprocessed)
    else:
        # XXX Cache per-file variables (e.g. `{filename: [(varid, decl)]}`).
        raise NotImplementedError


def variable(name, filenames, *,
             local=False,
             perfilecache=None,
             preprocessed=False,
             _iter_vars=variables,
             ):
    """Return (varid, decl) for the first found variable that matches.

    If "local" is True then the first matching local variable in the
    file will always be returned.  To avoid that, pass perfilecache and
    pop each variable from the cache after using it.
    """
    for varid, decl in _iter_vars(filenames,
                                  perfilecache=perfilecache,
                                  preprocessed=preprocessed,
                                  ):
        if varid.name != name:
            continue
        if local:
            if varid.funcname:
                if varid.funcname == UNKNOWN:
                    raise NotImplementedError
                return varid, decl
        elif not varid.funcname:
            return varid, decl
    else:
       return None, None  # No matching variable was found.


def variable_from_id(id, filenames, *,
                     perfilecache=None,
                     preprocessed=False,
                     _get_var=variable,
                     ):
    """Return (varid, decl) for the first found variable that matches."""
    local = False
    if isinstance(id, str):
        name = id
    else:
        if id.funcname == UNKNOWN:
            local = True
        elif id.funcname:
            raise NotImplementedError

        name = id.name
        if id.filename and id.filename != UNKNOWN:
            filenames = [id.filename]
    return _get_var(id, filenames,
                    local=local,
                    perfilecache=perfilecache,
                    preprocessed=preprocessed,
                    )
