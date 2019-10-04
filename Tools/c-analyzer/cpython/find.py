import os.path

from c_analyzer.common.info import UNKNOWN
from c_analyzer.variables import find as _common

from . import SOURCE_DIRS, PYTHON, REPO_ROOT
from .known import (
    from_file as known_from_file,
    DATA_FILE as KNOWN_FILE,
    )
from .supported import (
        ignored_from_file, IGNORED_FILE, is_supported, _is_object,
        )

# XXX need tests:
# * vars_from_binary()
# * vars_from_source()
# * supported_vars()


def vars_from_binary(*,
                     dirnames=SOURCE_DIRS,
                     known=None,
                     _iter_vars=_common.vars_from_binary,
                     ):
    """Yield a Variable for each found Symbol.

    Details are filled in from the given "known" variables and types.
    """
    yield from _iter_vars(PYTHON, known=known, dirnames=dirnames)


def vars_from_source(*,
                     preprocessed=None,
                     known=None,  # for types
                     _iter_vars=_common.vars_from_source,
                     ):
    """Yield a Variable for each declaration in the raw source code.

    Details are filled in from the given "known" variables and types.
    """
    yield from _iter_vars(
            dirnames=SOURCE_DIRS,
            preprocessed=preprocessed,
            known=known,
            )


def supported_vars(dirnames=SOURCE_DIRS, *,
                   known=KNOWN_FILE,
                   ignored=IGNORED_FILE,
                   skip_objects=False,
                   _known_from_file=known_from_file,
                   _ignored_from_file=ignored_from_file,
                   _relpath=os.path.relpath,
                   _iter_vars=vars_from_binary,
                   _is_supported=is_supported,
                   ):
    """Yield (var, is supported) for each found variable."""
    if isinstance(known, str):
        known = _known_from_file(known)
    if isinstance(ignored, str):
        ignored = _ignored_from_file(ignored)

    if dirnames == SOURCE_DIRS:
        dirnames = [_relpath(d, REPO_ROOT) for d in dirnames]
    # XXX For now we only use known variables (no source lookup).
    dirnames = None

    knownvars = (known or {}).get('variables')
    for var in _iter_vars(known=known, dirnames=dirnames):
        if not var.isglobal:
            continue
        elif var.vartype == UNKNOWN:
            yield var, None
        # XXX Support proper filters instead.
        elif skip_objects and _is_object(found.vartype):
            continue
        else:
            yield var, _is_supported(var, ignored, known)
