import csv

from c_parser.info import Variable

from .info import ID
from .util import read_tsv


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
        id = ID(filename.strip(), funcname.strip(), name.strip())
        kind = kind.strip()
        if kind == 'variable':
            values = known['variables']
            value = Variable(id, declaration.strip())
        else:
            raise ValueError(f'unsupported kind in row {row}')
        value.validate()
        values[id] = value
    return known
