import os.path
import shutil
import sys

from c_parser import info, util
from . import source


#PYTHON = os.path.join(REPO_ROOT, 'python')
PYTHON = sys.executable


def iter_symbols(dirnames, binary=PYTHON, *,
                 _file_exists=os.path.exists,
                 _find_local_symbol=source.find_symbol,
                 _iter_symbols_nm=(lambda b, fls: _iter_symbols_nm(b, fls)),
                 ):
    """Yield a Symbol for each symbol found in the binary."""
    if not _file_exists(binary):
        raise Exception('executable missing (need to build it first?)')

    cache = {}
    def find_local_symbol(name):
        return _find_local_symbol(name, dirnames, _perfilecache=cache)
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
        'b': info.Symbol.KIND.VARIABLE,  # uninitialized
        'd': info.Symbol.KIND.VARIABLE,  # initialized
        #'g': info.Symbol.KIND.VARIABLE,  # uninitialized
        #'s': info.Symbol.KIND.VARIABLE,  # initialized
        't': info.Symbol.KIND.FUNCTION,
        }


def _iter_symbols_nm(binary, find_local_symbol=None,
                     *,
                     _which=shutil.which,
                     _run=util.run_cmd,
                     ):
    nm = _which('nm')
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
         )= _parse_nm_line(line, _find_local_symbol=find_local_symbol)
        if kind != info.Symbol.KIND.VARIABLE:
            continue
        elif _is_special_symbol(name):
            continue
        elif not filename:
            raise Exception(f'missing filename: {name}')
        yield info.Symbol(
                name=name,
                kind=kind,
                external=external,
                filename=filename,
                funcname=funcname,
                declaration=vartype,
                )


def _parse_nm_line(line, *, _find_local_symbol=None):
    _origline = line
    _, _, line = line.partition(' ')  # strip off the address
    line = line.strip()

    kind, _, line = line.partition(' ')
    line = line.strip()
    external = kind.isupper()
    kind = NM_KINDS.get(kind.lower(), info.Symbol.KIND.OTHER)

    name, _, filename = line.partition('\t')
    name = name.strip()

    funcname = None
    vartype = None
    if filename:
        filename = os.path.relpath(filename.partition(':')[0])
    elif kind != info.Symbol.KIND.VARIABLE:
        pass
    elif _is_special_symbol(name):
        pass
    elif _find_local_symbol is not None:
        orig = name
        name, sep, digits = name.partition('.')
        if not sep or not digits.isdigit():
            print(_origline)
            raise Exception(f'expected local variable, got {orig}')
        filename, funcname, vartype = _find_local_symbol(name)

#    return info.Symbol(name, kind, external, filename or None, funcname)
    return name, kind, external, filename or None, funcname, vartype
