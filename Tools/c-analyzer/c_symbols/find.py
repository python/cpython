import os
import os.path
import shutil

from c_analyzer_common import PYTHON

from . import _nm, source

# XXX need tests:
# * iter_symbols


def _get_platform_tool():
    if os.name == 'nt':
        # XXX Support this.
        raise NotImplementedError
    elif nm := shutil.which('nm'):
        return lambda b, fls: _nm.iter_symbols(b, fls, nm=nm)
    else:
        raise NotImplementedError


def iter_symbols(binfile=PYTHON, dirnames=None, *,
                 # Alternately, use look_up_known_symbol()
                 # from c_globals.supported.
                 find_local_symbol=source.find_symbol,
                 _file_exists=os.path.exists,
                 _get_platform_tool=_get_platform_tool,
                 ):
    """Yield a Symbol for each symbol found in the binary."""
    if not _file_exists(binfile):
        raise Exception('executable missing (need to build it first?)')

    if find_local_symbol:
        cache = {}
        def find_local_symbol(name, *, _find=find_local_symbol):
            return _find(name, dirnames, _perfilecache=cache)
    else:
        find_local_symbol = None

    _iter_symbols = _get_platform_tool()
    yield from _iter_symbols(binfile, find_local_symbol)
