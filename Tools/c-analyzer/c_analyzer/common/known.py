import csv
import os.path

from . import DATA_DIR
from .info import ID, UNKNOWN, Variable
from .util import read_tsv

# XXX need tests:
# * from_file()
# * look_up_variable()


DATA_FILE = os.path.join(DATA_DIR, 'known.tsv')

COLUMNS = ('filename', 'funcname', 'name', 'kind', 'declaration')
HEADER = '\t'.join(COLUMNS)


def from_file(infile, *,
              _read_tsv=read_tsv,
              ):
    """Return the info for known declarations in the given file."""
    known = {
        'variables': {},
        #'types': {},
        #'constants': {},
        #'macros': {},
        }
    for row in _read_tsv(infile, HEADER):
        filename, funcname, name, kind, declaration = row
        if not funcname or funcname == '-':
            funcname = None
        id = ID(filename, funcname, name)
        if kind == 'variable':
            values = known['variables']
            if funcname:
                storage = _get_storage(declaration) or 'local'
            else:
                storage = _get_storage(declaration) or 'implicit'
            value = Variable(id, storage, declaration)
        else:
            raise ValueError(f'unsupported kind in row {row}')
        value.validate()
#        if value.name == 'id' and declaration == UNKNOWN:
#            # None of these are variables.
#            declaration = 'int id';
#        else:
#            value.validate()
        values[id] = value
    return known


def _get_storage(decl):
    # statics
    if decl.startswith('static '):
        return 'static'
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
    if decl.startswith('extern '):
        return 'extern'
    if decl.startswith('PyAPI_DATA('):
        return 'extern'
    # implicit or local
    return None


def look_up_variable(varid, knownvars, *,
                     match_files=(lambda f1, f2: f1 == f2),
                     ):
    """Return the known variable matching the given ID.

    "knownvars" is a mapping of ID to Variable.

    "match_files" is used to verify if two filenames point to
    the same file.

    If no match is found then None is returned.
    """
    if not knownvars:
        return None

    if varid.funcname == UNKNOWN:
        if not varid.filename or varid.filename == UNKNOWN:
            for varid in knownvars:
                if not varid.funcname:
                    continue
                if varid.name == varid.name:
                    return knownvars[varid]
            else:
                return None
        else:
            for varid in knownvars:
                if not varid.funcname:
                    continue
                if not match_files(varid.filename, varid.filename):
                    continue
                if varid.name == varid.name:
                    return knownvars[varid]
            else:
                return None
    elif not varid.filename or varid.filename == UNKNOWN:
        raise NotImplementedError
    else:
        return knownvars.get(varid.id)
