import csv

from c_analyzer_common.info import ID
from c_analyzer_common.util import read_tsv


def is_supported(static, ignored=None, known=None):
    """Return True if the given static variable is okay in CPython."""
    # XXX finish!
    raise NotImplementedError
    if static in ignored:
        return True
    if static in known:
        return True
    for part in static.vartype.split():
        # XXX const is automatic True?
        if part == 'PyObject' or part.startswith('PyObject['):
            return False
    return True


#############################
# ignored

COLUMNS = ('filename', 'funcname', 'name', 'kind', 'reason')
HEADER = '\t'.join(COLUMNS)


def ignored_from_file(infile, *,
                      _read_tsv=read_tsv,
                      ):
    """Yield StaticVar for each ignored var in the file."""
    ignored = {
        'variables': {},
        #'types': {},
        #'constants': {},
        #'macros': {},
        }
    for row in _read_tsv(infile, HEADER):
        filename, funcname, name, kind, reason = row
        if not funcname or funcname == '-':
            funcname = None
        id = ID(filename, funcname, name)
        if kind == 'variable':
            values = ignored['variables']
        else:
            raise ValueError(f'unsupported kind in row {row}')
        values[id] = reason
    return ignored
