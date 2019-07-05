from collections import namedtuple
import os.path
import shutil
import subprocess

from . import REPO_ROOT, files, info, parse


PYTHON = os.path.join(REPO_ROOT, 'python')


def iter_binary(dirnames, binary=PYTHON, *,
                _file_exists=os.path.exists,
                _find_local_symbol=(lambda *a, **k: _find_local_symbol(*a, **k)),
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
# C scanning (move to scan.py)

def _get_local_symbols(filename, *,
                       _iter_variables=parse.iter_variables,
                       ):
    symbols = {}
    for funcname, name, vartype in _iter_variables(filename):
        if not funcname:
            continue
        try:
            instances = symbols[name]
        except KeyError:
            instances = symbols[name] = []
        instances.append((funcname, vartype))
    return symbols


def _find_local_symbol(name, dirnames, *,
                       _perfilecache,
                       _get_local_symbols=_get_local_symbols,
                       _iter_files=files.iter_files,
                       ):
    for filename in _iter_files(dirnames, ('.c', '.h')):
        try:
            symbols = _perfilecache[filename]
        except KeyError:
            symbols = _perfilecache[filename] = _get_local_symbols(filename)

        try:
            instances = symbols[name]
        except KeyError:
            continue

        funcname, vartype = instances.pop(0)
        if not instances:
            symbols.pop(name)
        return filename, funcname, vartype


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
                     _run=subprocess.run,
                     ):
    nm = shutil.which('nm')
    argv = [nm,
            '--line-numbers',
            binary,
            ]
    try:
        proc = _run(argv,
                    capture_output=True,
                    text=True,
                    check=True,
                    )
    except Exception:
        if nm is None:
            # XXX Use dumpbin.exe /SYMBOLS on Windows.
            raise NotImplementedError
        raise
    for line in proc.stdout.splitlines():
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
