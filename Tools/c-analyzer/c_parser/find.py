from c_analyzer_common.info import Variable, UNKNOWN

from . import declarations

# XXX need tests:
# * variables
# * variable
# * variable_from_id


def _iter_vars(filenames, preprocessed, *,
               _iter_variables=declarations.iter_variables,
               ):
    for filename in filenames or ():
        for funcname, name, decl in _iter_variables(filename,
                                                    preprocessed=preprocessed,
                                                    ):
            yield Variable.from_parts(filename, funcname, name, decl)


def variables(*filenames,
              perfilecache=None,
              preprocessed=False,
              _iter_vars=_iter_vars,
              ):
    """Yield a Variable for each one found in the given files.

    If "preprocessed" is provided (and not False/None) then it is used
    to decide which tool to use to parse the source code after it runs
    through the C preprocessor.  Otherwise the raw
    """
    if perfilecache is None:
        yield from _iter_vars(filenames, preprocessed)
    else:
        # XXX Cache per-file variables (e.g. `{filename: [Variable]}`).
        raise NotImplementedError


def variable(name, filenames, *,
             local=False,
             perfilecache=None,
             preprocessed=False,
             _iter_variables=variables,
             ):
    """Return the first found Variable that matches.

    If "local" is True then the first matching local variable in the
    file will always be returned.  To avoid that, pass perfilecache and
    pop each variable from the cache after using it.
    """
    for var in _iter_variables(filenames,
                               perfilecache=perfilecache,
                               preprocessed=preprocessed,
                               ):
        if var.name != name:
            continue
        if local:
            if var.funcname:
                if var.funcname == UNKNOWN:
                    raise NotImplementedError
                return var
        elif not var.funcname:
            return var
    else:
       return None  # No matching variable was found.


def variable_from_id(id, filenames, *,
                     perfilecache=None,
                     preprocessed=False,
                     _get_var=variable,
                     ):
    """Return the first found Variable that matches."""
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
