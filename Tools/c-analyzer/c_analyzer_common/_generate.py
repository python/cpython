# The code here consists of hacks for pre-populating the known.tsv file.

import contextlib
import glob
import os.path
import re

from c_statics import KNOWN_FILE, SOURCE_DIRS, REPO_ROOT

from . import known
from .info import UNKNOWN, ID
from .util import run_cmd, write_tsv


@contextlib.contextmanager
def get_srclines(filename, srclines=None, *,
                 cache=None,
                 _open=open,
                 ):
    if srclines is None:
        if cache is None:
            with _open(filename) as srcfile:
                srclines = list(srcfile)
        else:
            try:
                srclines = cache[filename]
            except KeyError:
                with open(filename) as srcfile:
                    srclines = [line.rstrip() for line in srcfile]
                cache[filename] = srclines
    yield srclines


def find_matching_variable(varid, filename, srclines=None, *,
                           cache=None,
                           _used=set(),
                           _get_srclines=get_srclines,
                           ):
    # XXX Store found variables in the cache instead of the src lines?
    # Then we would pop from cache instead of using "_used".
    name = varid.name
    funcname = varid.funcname
    with _get_srclines(filename, srclines, cache=cache) as srclines:
        for line in srclines:
            # XXX remember current funcname
            if not re.match(rf'.*\b{name}\b', line):
                continue
            if funcname:
                if not line.startswith('    '):
                    continue
                line = line.strip()
            if not line.startswith('static '):
                continue
            filename = os.path.relpath(filename, REPO_ROOT)
            newid = ID(filename, funcname, name)
            if newid in _used:
                continue
            _used.add(newid)

            decl = line.split('=')[0].strip()
            decl = decl.strip(';').strip()
            return newid, decl
    return varid, UNKNOWN


def known_rows(symbols, *,
               dirnames=SOURCE_DIRS,
               _find_match=find_matching_variable,
               ):
    cache = {}
    for symbol in symbols:
        if symbol.filename and symbol.filename != UNKNOWN:
            filenames = [symbol.filename]
        else:
            filenames = []
            for suffix in ('.c', '.h'):
                for dirname in SOURCE_DIRS:
                    filenames.extend(glob.glob(f'{dirname}/*{suffix}'))
                    filenames.extend(glob.glob(f'{dirname}/**/*{suffix}'))

        if symbol.funcname == UNKNOWN:
            print(symbol)

        if not filenames:
            raise ValueError(f'no files found for dirnames {dirnames}')
        for filename in filenames:
            found, decl = _find_match(symbol.id, filename, cache=cache)
            if decl != UNKNOWN:
                break
        # else we will have made at least one pass, so found will be
        # symbol.id and decl will be UNKNOWN.

        yield (
            found.filename,
            found.funcname or '-',
            found.name,
            'variable',
            decl,
            )


def known_file(symbols, filename=None, *,
               _generate_rows=known_rows,
               ):
    if not filename:
        filename = KNOWN_FILE + '.new'

    rows = _generate_rows(symbols)
    write_tsv(filename, known.HEADER, rows)
