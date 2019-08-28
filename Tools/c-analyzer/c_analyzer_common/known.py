import csv
import os.path

from c_analyzer_common import DATA_DIR
from c_parser.info import Variable

from .info import ID
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
        else:
            raise ValueError(f'unsupported kind in row {row}')
        value.validate()
        values[id] = value
    return known
