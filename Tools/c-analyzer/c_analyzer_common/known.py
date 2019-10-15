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
