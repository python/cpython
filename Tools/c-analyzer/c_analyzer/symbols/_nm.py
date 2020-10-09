import os.path
import shutil

from c_analyzer.common import util, info

from .info import Symbol


# XXX need tests:
# * iter_symbols

NM_KINDS = {
        'b': Symbol.KIND.VARIABLE,  # uninitialized
        'd': Symbol.KIND.VARIABLE,  # initialized
        #'g': Symbol.KIND.VARIABLE,  # uninitialized
        #'s': Symbol.KIND.VARIABLE,  # initialized
        't': Symbol.KIND.FUNCTION,
        }

SPECIAL_SYMBOLS = {
        # binary format (e.g. ELF)
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


def iter_symbols(binfile, *,
                 nm=None,
                 handle_id=None,
                 _which=shutil.which,
                 _run=util.run_cmd,
                 ):
    """Yield a Symbol for each relevant entry reported by the "nm" command."""
    if nm is None:
        nm = _which('nm')
        if not nm:
            raise NotImplementedError
    if handle_id is None:
        handle_id = info.ID

    argv = [nm,
            '--line-numbers',
            binfile,
            ]
    try:
        output = _run(argv)
    except Exception:
        if nm is None:
            # XXX Use dumpbin.exe /SYMBOLS on Windows.
            raise NotImplementedError
        raise
    for line in output.splitlines():
        (name, kind, external, filename, funcname,
         ) = _parse_nm_line(line)
        if kind != Symbol.KIND.VARIABLE:
            continue
        elif _is_special_symbol(name):
            continue
        yield Symbol(
                id=handle_id(filename, funcname, name),
                kind=kind,
                external=external,
                )


def _parse_nm_line(line):
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

    name, islocal = _parse_nm_name(name, kind)
    funcname = info.UNKNOWN if islocal else None
    return name, kind, external, filename, funcname


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
