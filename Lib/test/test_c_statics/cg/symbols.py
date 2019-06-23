from collections import namedtuple
import os.path
import shutil
import subprocess

from . import REPO_ROOT


PYTHON = os.path.join(REPO_ROOT, 'python')


class Symbol(namedtuple('Symbol', 'name kind external filename')):
    """Info for a single symbol compiled into a binary executable."""

    class KIND:
        VARIABLE = 'variable'
        FUNCTION = 'function'
        OTHER = 'other'


def iter_binary(binary=PYTHON):
    """Yield a Symbol for each symbol found in the binary."""
    if not os.path.exists(binary):
        raise Exception('executable missing (need to build it first?)')
    yield from _iter_symbols_nm(binary)


#############################
# "nm"

NM_KINDS = {
        'b': Symbol.KIND.VARIABLE,  # uninitialized
        'd': Symbol.KIND.VARIABLE,  # initialized
        #'g': Symbol.KIND.VARIABLE,  # uninitialized
        #'s': Symbol.KIND.VARIABLE,  # initialized
        't': Symbol.KIND.FUNCTION,
        }


def _iter_symbols_nm(binary):
    nm = shutil.which('nm')
    try:
        out = subprocess.check_output([nm,
                '--line-numbers',
                binary])
    except Exception:
        if nm is None:
            # XXX Use dumpbin.exe /SYMBOLS on Windows.
            raise NotImplementedError
        raise
    for line in out.decode('utf-8').splitlines():
        yield _parse_nm_line(line)


def _parse_nm_line(line):
    _, _, line = line.partition(' ')  # strip off the address
    line = line.strip()

    kind, _, line = line.partition(' ')
    line = line.strip()
    external = kind.isupper()
    kind = NM_KINDS.get(kind.lower(), Symbol.KIND.OTHER)

    name, _, filename = line.partition('\t')
    name = name.strip()

    if filename:
        filename = os.path.relpath(filename.partition(':')[0])

    return Symbol(name, kind, external, filename or None)
