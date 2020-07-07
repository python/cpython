import csv

from ..common.info import ID, UNKNOWN
from ..common.util import read_tsv
from .info import Variable


# XXX need tests:
# * read_file()
# * look_up_variable()


COLUMNS = ('filename', 'funcname', 'name', 'kind', 'declaration')
HEADER = '\t'.join(COLUMNS)


def read_file(infile, *,
              _read_tsv=read_tsv,
              ):
    """Yield (kind, id, decl) for each row in the data file.

    The caller is responsible for validating each row.
    """
    for row in _read_tsv(infile, HEADER):
        filename, funcname, name, kind, declaration = row
        if not funcname or funcname == '-':
            funcname = None
        id = ID(filename, funcname, name)
        yield kind, id, declaration


def from_file(infile, *,
              handle_var=Variable.from_id,
              _read_file=read_file,
              ):
    """Return the info for known declarations in the given file."""
    known = {
        'variables': {},
        #'types': {},
        #'constants': {},
        #'macros': {},
        }
    for kind, id, decl in _read_file(infile):
        if kind == 'variable':
            values = known['variables']
            value = handle_var(id, decl)
        else:
            raise ValueError(f'unsupported kind in row {row}')
        value.validate()
        values[id] = value
    return known


def look_up_variable(varid, knownvars, *,
                     match_files=(lambda f1, f2: f1 == f2),
                     ):
    """Return the known Variable matching the given ID.

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
