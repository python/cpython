import os.path

from c_analyzer.common import files
from c_analyzer.common.info import UNKNOWN, ID
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


def _handle_id(filename, funcname, name, *,
               _relpath=os.path.relpath,
               ):
    filename = _relpath(filename, REPO_ROOT)
    return ID(filename, funcname, name)


def vars_from_binary(*,
                     known=KNOWN_FILE,
                     _known_from_file=known_from_file,
                     _iter_files=files.iter_files_by_suffix,
                     _iter_vars=_common.vars_from_binary,
                     ):
    """Yield a Variable for each found Symbol.

    Details are filled in from the given "known" variables and types.
    """
    if isinstance(known, str):
        known = _known_from_file(known)
    dirnames = SOURCE_DIRS
    suffixes = ('.c',)
    filenames = _iter_files(dirnames, suffixes)
    # XXX For now we only use known variables (no source lookup).
    filenames = None
    yield from _iter_vars(PYTHON,
                          known=known,
                          filenames=filenames,
                          handle_id=_handle_id,
                          check_filename=(lambda n: True),
                          )


def vars_from_source(*,
                     preprocessed=None,
                     known=KNOWN_FILE,
                     _known_from_file=known_from_file,
                     _iter_files=files.iter_files_by_suffix,
                     _iter_vars=_common.vars_from_source,
                     ):
    """Yield a Variable for each declaration in the raw source code.

    Details are filled in from the given "known" variables and types.
    """
    if isinstance(known, str):
        known = _known_from_file(known)
    dirnames = SOURCE_DIRS
    suffixes = ('.c',)
    filenames = _iter_files(dirnames, suffixes)
    yield from _iter_vars(filenames,
                          preprocessed=preprocessed,
                          known=known,
                          handle_id=_handle_id,
                          )


def supported_vars(*,
                   known=KNOWN_FILE,
                   ignored=IGNORED_FILE,
                   skip_objects=False,
                   _known_from_file=known_from_file,
                   _ignored_from_file=ignored_from_file,
                   _iter_vars=vars_from_binary,
                   _is_supported=is_supported,
                   ):
    """Yield (var, is supported) for each found variable."""
    if isinstance(known, str):
        known = _known_from_file(known)
    if isinstance(ignored, str):
        ignored = _ignored_from_file(ignored)

    for var in _iter_vars(known=known):
        if not var.isglobal:
            continue
        elif var.vartype == UNKNOWN:
            yield var, None
        # XXX Support proper filters instead.
        elif skip_objects and _is_object(found.vartype):
            continue
        else:
            yield var, _is_supported(var, ignored, known)
