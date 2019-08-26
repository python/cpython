import csv

from c_parser.info import Variable

from . import info


COLUMNS = ('filename', 'funcname', 'name', 'kind', 'declaration')
HEADER = '\t'.join(COLUMNS)


# XXX need tests:
# * from_file()

def from_file(infile, *,
              _open=open,
              ):
    """Return the info for known declarations in the given file."""
    if isinstance(infile, str):
        with _open(infile) as infile:
            return from_file(infile, _open=open)

    lines = iter(infile)
    try:
        header = next(lines).strip()
    except StopIteration:
        header = ''
    if header != HEADER:
        raise ValueError(f'bad header {header!r}')

    known = {
        'variables': {},
        #'types': {},
        #'constants': {},
        #'macros': {},
        }
    for row in csv.reader(lines, delimiter='\t'):
        filename, funcname, name, kind, declaration = row
        id = info.ID(filename.strip(), funcname.strip(), name.strip())
        kind = kind.strip()
        if kind == 'variable':
            values = known['variables']
            value = Variable(id, declaration.strip())
        else:
            raise ValueError(f'unsupported kind in row {row}')
        value.validate()
        values[id] = value
    return known
