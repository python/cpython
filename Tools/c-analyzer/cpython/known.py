import csv
import os.path

from c_analyzer.parser.declarations import extract_storage
from c_analyzer.variables import known as _common
from c_analyzer.variables.info import Variable

from . import DATA_DIR


# XXX need tests:
# * from_file()
# * look_up_variable()


DATA_FILE = os.path.join(DATA_DIR, 'known.tsv')


def _get_storage(decl, infunc):
    # statics
    if decl.startswith(('Py_LOCAL(', 'Py_LOCAL_INLINE(')):
        return 'static'
    if decl.startswith(('_Py_IDENTIFIER(', '_Py_static_string(')):
        return 'static'
    if decl.startswith('PyDoc_VAR('):
        return 'static'
    if decl.startswith(('SLOT1BINFULL(', 'SLOT1BIN(')):
        return 'static'
    if decl.startswith('WRAP_METHOD('):
        return 'static'
    # public extern
    if decl.startswith('PyAPI_DATA('):
        return 'extern'
    # Fall back to the normal handler.
    return extract_storage(decl, infunc=infunc)


def _handle_var(varid, decl):
#    if varid.name == 'id' and decl == UNKNOWN:
#        # None of these are variables.
#        decl = 'int id';
    storage = _get_storage(decl, varid.funcname)
    return Variable(varid, storage, decl)


def from_file(infile=DATA_FILE, *,
              _from_file=_common.from_file,
              _handle_var=_handle_var,
              ):
    """Return the info for known declarations in the given file."""
    return _from_file(infile, handle_var=_handle_var)


def look_up_variable(varid, knownvars, *,
                     _lookup=_common.look_up_variable,
                     ):
    """Return the known variable matching the given ID.

    "knownvars" is a mapping of ID to Variable.

    "match_files" is used to verify if two filenames point to
    the same file.

    If no match is found then None is returned.
    """
    return _lookup(varid, knownvars)
