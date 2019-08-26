import csv

from c_analyzer_common.info import ID


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
                      _open=open,
                      ):
    """Yield StaticVar for each ignored var in the file."""
    if isinstance(infile, str):
        with _open(infile) as infile:
            return ignored_from_file(infile, _open=open)

    lines = iter(infile)
    try:
        header = next(lines).strip()
    except StopIteration:
        header = ''
    if header != HEADER:
        raise ValueError(f'bad header {header!r}')

    ignored = {
        'variables': {},
        #'types': {},
        #'constants': {},
        #'macros': {},
        }
    for row in csv.reader(lines, delimiter='\t'):
        filename, funcname, name, kind, reason = row
        id = ID(filename, funcname, name)
        kind = kind.strip()
        if kind == 'variable':
            values = ignored['variables']
        else:
            raise ValueError(f'unsupported kind in row {row}')
        values[id] = reason.strip()
    return ignored
