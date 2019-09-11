import csv
import os.path

from c_parser.info import Variable

from . import DATA_DIR
from .info import ID, UNKNOWN
from .util import read_tsv


DATA_FILE = os.path.join(DATA_DIR, 'known.tsv')

COLUMNS = ('filename', 'funcname', 'name', 'kind', 'declaration')
HEADER = '\t'.join(COLUMNS)


# XXX need tests:
# * from_file()

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
            value = Variable(id, declaration)
            value._isglobal = _is_global(declaration) or id.funcname is None
        else:
            raise ValueError(f'unsupported kind in row {row}')
        if value.name == 'id' and declaration == UNKNOWN:
            # None of these are variables.
            declaration = 'int id';
        else:
            value.validate()
        values[id] = value
    return known


def _is_global(vartype):
    # statics
    if vartype.startswith('static '):
        return True
    if vartype.startswith(('Py_LOCAL(', 'Py_LOCAL_INLINE(')):
        return True
    if vartype.startswith(('_Py_IDENTIFIER(', '_Py_static_string(')):
        return True
    if vartype.startswith('PyDoc_VAR('):
        return True
    if vartype.startswith(('SLOT1BINFULL(', 'SLOT1BIN(')):
        return True
    if vartype.startswith('WRAP_METHOD('):
        return True
    # public extern
    if vartype.startswith('PyAPI_DATA('):
        return True
    return False
