import os
import os.path
import shutil
import sys

from c_analyzer_common import util, info
from . import source
from .info import Symbol


#PYTHON = os.path.join(REPO_ROOT, 'python')
PYTHON = sys.executable


def iter_symbols(binary=PYTHON, dirnames=None, *,
                 # Alternately, use look_up_known_symbol()
                 # from c_globals.supported.
                 find_local_symbol=source.find_symbol,
                 _file_exists=os.path.exists,
                 _iter_symbols_nm=(lambda b, *a: _iter_symbols_nm(b, *a)),
                 ):
    """Yield a Symbol for each symbol found in the binary."""
    if not _file_exists(binary):
        raise Exception('executable missing (need to build it first?)')

    if find_local_symbol:
        cache = {}
        def find_local_symbol(name, *, _find=find_local_symbol):
            return _find(name, dirnames, _perfilecache=cache)
    else:
        find_local_symbol = None

    if os.name == 'nt':
        # XXX Support this.
        raise NotImplementedError
    else:
        yield from _iter_symbols_nm(binary, find_local_symbol)


#############################
# binary format (e.g. ELF)

SPECIAL_SYMBOLS = {
        '__bss_start',
        '__data_start',
        '__dso_handle',
        '_DYNAMIC',
        '_edata',
        '_end',
        '__environ@@GLIBC_2.2.5',
        '_GLOBAL_OFFSET_TABLE_',
        '__JCR_END__',
        '__JCR_LIST__',
        '__TMC_END__',
        }


def _is_special_symbol(name):
    if name in SPECIAL_SYMBOLS:
        return True
    if '@@GLIBC' in name:
        return True
    return False


#############################
# "nm"

NM_KINDS = {
        'b': Symbol.KIND.VARIABLE,  # uninitialized
        'd': Symbol.KIND.VARIABLE,  # initialized
        #'g': Symbol.KIND.VARIABLE,  # uninitialized
        #'s': Symbol.KIND.VARIABLE,  # initialized
        't': Symbol.KIND.FUNCTION,
        }


def _iter_symbols_nm(binary, find_local_symbol=None,
                     *,
                     _which=shutil.which,
                     _run=util.run_cmd,
                     ):
    nm = _which('nm')
    if not nm:
        raise NotImplementedError
    argv = [nm,
            '--line-numbers',
            binary,
            ]
    try:
        output = _run(argv)
    except Exception:
        if nm is None:
            # XXX Use dumpbin.exe /SYMBOLS on Windows.
            raise NotImplementedError
        raise
    for line in output.splitlines():
        (name, kind, external, filename, funcname, vartype,
         ) = _parse_nm_line(line,
                            _find_local_symbol=find_local_symbol,
                            )
        if kind != Symbol.KIND.VARIABLE:
            continue
        elif _is_special_symbol(name):
            continue
        assert vartype is None
        yield Symbol(
                id=(filename, funcname, name),
                kind=kind,
                external=external,
                )


def _parse_nm_line(line, *, _find_local_symbol=None):
    _origline = line
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
    else:
        filename = info.UNKNOWN

    vartype = None
    name, islocal = _parse_nm_name(name, kind)
    if islocal:
        funcname = info.UNKNOWN
        if _find_local_symbol is not None:
            filename, funcname, vartype = _find_local_symbol(name)
            filename = filename or info.UNKNOWN
            funcname = funcname or info.UNKNOWN
    else:
        funcname = None
        # XXX fine filename and vartype?
    return name, kind, external, filename, funcname, vartype


def _parse_nm_name(name, kind):
    if kind != Symbol.KIND.VARIABLE:
        return name, None
    if _is_special_symbol(name):
        return name, None

    actual, sep, digits = name.partition('.')
    if not sep:
        return name, False

    if not digits.isdigit():
        raise Exception(f'got bogus name {name}')
    return actual, True
