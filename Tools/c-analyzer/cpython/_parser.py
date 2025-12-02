import os.path

from c_parser.preprocessor import (
    get_preprocessor as _get_preprocessor,
)
from c_parser import (
    parse_file as _parse_file,
    parse_files as _parse_files,
)
from . import REPO_ROOT


GLOB_ALL = '**/*'


def _abs(relfile):
    return os.path.join(REPO_ROOT, relfile)


def format_conf_lines(lines):
    """Format conf entries for use in a TSV table."""
    return [_abs(entry) for entry in lines]


def format_tsv_lines(lines):
    """Format entries for use in a TSV table."""
    lines = ((_abs(first), *rest) for first, *rest in lines)
    lines = map('\t'.join, lines)
    return list(map(str.rstrip, lines))


'''
@begin=sh@
./python ../c-parser/cpython.py
    --exclude '+../c-parser/EXCLUDED'
    --macros '+../c-parser/MACROS'
    --incldirs '+../c-parser/INCL_DIRS'
    --same './Include/cpython/'
    Include/*.h
    Include/internal/*.h
    Modules/**/*.c
    Objects/**/*.c
    Parser/**/*.c
    Python/**/*.c
@end=sh@
'''

# XXX Handle these.
EXCLUDED = format_conf_lines([
    # macOS
    'Modules/_scproxy.c',              # SystemConfiguration/SystemConfiguration.h

    # Windows
    'Modules/_winapi.c',               # windows.h
    'Modules/expat/winconfig.h',
    'Modules/overlapped.c',            # winsock.h
    'Python/dynload_win.c',            # windows.h
    'Python/thread_nt.h',

    # other OS-dependent
    'Python/dynload_aix.c',            # sys/ldr.h
    'Python/dynload_dl.c',             # dl.h
    'Python/dynload_hpux.c',           # dl.h
    'Python/emscripten_signal.c',
    'Python/emscripten_syscalls.c',
    'Python/emscripten_trampoline_inner.c',
    'Python/thread_pthread.h',
    'Python/thread_pthread_stubs.h',

    # only huge constants (safe but parsing is slow)
    'Modules/_ssl_data_*.h',
    'Modules/cjkcodecs/mappings_*.h',
    'Modules/unicodedata_db.h',
    'Modules/unicodename_db.h',
    'Objects/unicodetype_db.h',

    # generated
    'Python/deepfreeze/*.c',
    'Python/frozen_modules/*.h',
    'Python/generated_cases.c.h',
    'Python/executor_cases.c.h',
    'Python/optimizer_cases.c.h',
    'Python/opcode_targets.h',
    # XXX: Throws errors if PY_VERSION_HEX is not mocked out
    'Modules/clinic/_testclinic_depr.c.h',

    # not actually source
    'Python/bytecodes.c',
    'Python/optimizer_bytecodes.c',

    # mimalloc
    'Objects/mimalloc/*.c',
    'Include/internal/mimalloc/*.h',
    'Include/internal/mimalloc/mimalloc/*.h',
])

# XXX Fix the parser.
EXCLUDED += format_conf_lines([
    # The tool should be able to parse these...

    # The problem with xmlparse.c is that something
    # has gone wrong where # we handle "maybe inline actual"
    # in Tools/c-analyzer/c_parser/parser/_global.py.
    'Modules/expat/internal.h',
    'Modules/expat/xmlparse.c',
])

INCL_DIRS = format_tsv_lines([
    # (glob, dirname)

    ('*', '.'),
    ('*', './Include'),
    ('*', './Include/internal'),
    ('*', './Include/internal/mimalloc'),

    ('Modules/_decimal/**/*.c', 'Modules/_decimal/libmpdec'),
    ('Modules/_elementtree.c', 'Modules/expat'),
    ('Modules/_hacl/*.c', 'Modules/_hacl/include'),
    ('Modules/_hacl/*.c', 'Modules/_hacl/'),
    ('Modules/_hacl/*.h', 'Modules/_hacl/include'),
    ('Modules/_hacl/*.h', 'Modules/_hacl/'),
    ('Modules/md5module.c', 'Modules/_hacl/include'),
    ('Modules/sha1module.c', 'Modules/_hacl/include'),
    ('Modules/sha2module.c', 'Modules/_hacl/include'),
    ('Modules/sha3module.c', 'Modules/_hacl/include'),
    ('Modules/blake2module.c', 'Modules/_hacl/include'),
    ('Modules/hmacmodule.c', 'Modules/_hacl/include'),
    ('Objects/stringlib/*.h', 'Objects'),

    # possible system-installed headers, just in case
    ('Modules/_tkinter.c', '/usr/include/tcl8.6'),
    ('Modules/_uuidmodule.c', '/usr/include/uuid'),
    ('Modules/tkappinit.c', '/usr/include/tcl'),

])

INCLUDES = format_tsv_lines([
    # (glob, include)

    ('**/*.h', 'Python.h'),
    ('Include/**/*.h', 'object.h'),

    # for Py_HAVE_CONDVAR
    ('Include/internal/pycore_gil.h', 'pycore_condvar.h'),
    ('Python/thread_pthread.h', 'pycore_condvar.h'),

    # other

    ('Objects/stringlib/join.h', 'stringlib/stringdefs.h'),
    ('Objects/stringlib/ctype.h', 'stringlib/stringdefs.h'),
    ('Objects/stringlib/transmogrify.h', 'stringlib/stringdefs.h'),
    # ('Objects/stringlib/fastsearch.h', 'stringlib/stringdefs.h'),
    # ('Objects/stringlib/count.h', 'stringlib/stringdefs.h'),
    # ('Objects/stringlib/find.h', 'stringlib/stringdefs.h'),
    # ('Objects/stringlib/partition.h', 'stringlib/stringdefs.h'),
    # ('Objects/stringlib/split.h', 'stringlib/stringdefs.h'),
    ('Objects/stringlib/fastsearch.h', 'stringlib/ucs1lib.h'),
    ('Objects/stringlib/count.h', 'stringlib/ucs1lib.h'),
    ('Objects/stringlib/find.h', 'stringlib/ucs1lib.h'),
    ('Objects/stringlib/partition.h', 'stringlib/ucs1lib.h'),
    ('Objects/stringlib/split.h', 'stringlib/ucs1lib.h'),
    ('Objects/stringlib/find_max_char.h', 'Objects/stringlib/ucs1lib.h'),
    ('Objects/stringlib/count.h', 'Objects/stringlib/fastsearch.h'),
    ('Objects/stringlib/find.h', 'Objects/stringlib/fastsearch.h'),
    ('Objects/stringlib/partition.h', 'Objects/stringlib/fastsearch.h'),
    ('Objects/stringlib/replace.h', 'Objects/stringlib/fastsearch.h'),
    ('Objects/stringlib/repr.h', 'Objects/stringlib/fastsearch.h'),
    ('Objects/stringlib/split.h', 'Objects/stringlib/fastsearch.h'),
])

MACROS = format_tsv_lines([
    # (glob, name, value)

    ('Include/internal/*.h', 'Py_BUILD_CORE', '1'),
    ('Python/**/*.c', 'Py_BUILD_CORE', '1'),
    ('Python/**/*.h', 'Py_BUILD_CORE', '1'),
    ('Parser/**/*.c', 'Py_BUILD_CORE', '1'),
    ('Parser/**/*.h', 'Py_BUILD_CORE', '1'),
    ('Objects/**/*.c', 'Py_BUILD_CORE', '1'),
    ('Objects/**/*.h', 'Py_BUILD_CORE', '1'),

    ('Modules/_asynciomodule.c', 'Py_BUILD_CORE', '1'),
    ('Modules/_codecsmodule.c', 'Py_BUILD_CORE', '1'),
    ('Modules/_collectionsmodule.c', 'Py_BUILD_CORE', '1'),
    ('Modules/_ctypes/_ctypes.c', 'Py_BUILD_CORE', '1'),
    ('Modules/_ctypes/cfield.c', 'Py_BUILD_CORE', '1'),
    ('Modules/_cursesmodule.c', 'Py_BUILD_CORE', '1'),
    ('Modules/_datetimemodule.c', 'Py_BUILD_CORE', '1'),
    ('Modules/_functoolsmodule.c', 'Py_BUILD_CORE', '1'),
    ('Modules/_heapqmodule.c', 'Py_BUILD_CORE', '1'),
    ('Modules/_io/*.c', 'Py_BUILD_CORE', '1'),
    ('Modules/_io/*.h', 'Py_BUILD_CORE', '1'),
    ('Modules/_localemodule.c', 'Py_BUILD_CORE', '1'),
    ('Modules/_operator.c', 'Py_BUILD_CORE', '1'),
    ('Modules/_posixsubprocess.c', 'Py_BUILD_CORE', '1'),
    ('Modules/_sre/sre.c', 'Py_BUILD_CORE', '1'),
    ('Modules/_threadmodule.c', 'Py_BUILD_CORE', '1'),
    ('Modules/_tracemalloc.c', 'Py_BUILD_CORE', '1'),
    ('Modules/_weakref.c', 'Py_BUILD_CORE', '1'),
    ('Modules/_zoneinfo.c', 'Py_BUILD_CORE', '1'),
    ('Modules/atexitmodule.c', 'Py_BUILD_CORE', '1'),
    ('Modules/cmathmodule.c', 'Py_BUILD_CORE', '1'),
    ('Modules/faulthandler.c', 'Py_BUILD_CORE', '1'),
    ('Modules/gcmodule.c', 'Py_BUILD_CORE', '1'),
    ('Modules/getpath.c', 'Py_BUILD_CORE', '1'),
    ('Modules/getpath_noop.c', 'Py_BUILD_CORE', '1'),
    ('Modules/itertoolsmodule.c', 'Py_BUILD_CORE', '1'),
    ('Modules/main.c', 'Py_BUILD_CORE', '1'),
    ('Modules/mathmodule.c', 'Py_BUILD_CORE', '1'),
    ('Modules/posixmodule.c', 'Py_BUILD_CORE', '1'),
    ('Modules/sha256module.c', 'Py_BUILD_CORE', '1'),
    ('Modules/sha512module.c', 'Py_BUILD_CORE', '1'),
    ('Modules/signalmodule.c', 'Py_BUILD_CORE', '1'),
    ('Modules/symtablemodule.c', 'Py_BUILD_CORE', '1'),
    ('Modules/timemodule.c', 'Py_BUILD_CORE', '1'),
    ('Modules/unicodedata.c', 'Py_BUILD_CORE', '1'),

    ('Modules/_json.c', 'Py_BUILD_CORE_BUILTIN', '1'),
    ('Modules/_pickle.c', 'Py_BUILD_CORE_BUILTIN', '1'),
    ('Modules/_testinternalcapi.c', 'Py_BUILD_CORE_BUILTIN', '1'),

    ('Include/cpython/abstract.h', 'Py_CPYTHON_ABSTRACTOBJECT_H', '1'),
    ('Include/cpython/bytearrayobject.h', 'Py_CPYTHON_BYTEARRAYOBJECT_H', '1'),
    ('Include/cpython/bytesobject.h', 'Py_CPYTHON_BYTESOBJECT_H', '1'),
    ('Include/cpython/ceval.h', 'Py_CPYTHON_CEVAL_H', '1'),
    ('Include/cpython/code.h', 'Py_CPYTHON_CODE_H', '1'),
    ('Include/cpython/dictobject.h', 'Py_CPYTHON_DICTOBJECT_H', '1'),
    ('Include/cpython/fileobject.h', 'Py_CPYTHON_FILEOBJECT_H', '1'),
    ('Include/cpython/fileutils.h', 'Py_CPYTHON_FILEUTILS_H', '1'),
    ('Include/cpython/frameobject.h', 'Py_CPYTHON_FRAMEOBJECT_H', '1'),
    ('Include/cpython/import.h', 'Py_CPYTHON_IMPORT_H', '1'),
    ('Include/cpython/interpreteridobject.h', 'Py_CPYTHON_INTERPRETERIDOBJECT_H', '1'),
    ('Include/cpython/listobject.h', 'Py_CPYTHON_LISTOBJECT_H', '1'),
    ('Include/cpython/methodobject.h', 'Py_CPYTHON_METHODOBJECT_H', '1'),
    ('Include/cpython/object.h', 'Py_CPYTHON_OBJECT_H', '1'),
    ('Include/cpython/objimpl.h', 'Py_CPYTHON_OBJIMPL_H', '1'),
    ('Include/cpython/pyerrors.h', 'Py_CPYTHON_ERRORS_H', '1'),
    ('Include/cpython/pylifecycle.h', 'Py_CPYTHON_PYLIFECYCLE_H', '1'),
    ('Include/cpython/pymem.h', 'Py_CPYTHON_PYMEM_H', '1'),
    ('Include/cpython/pystate.h', 'Py_CPYTHON_PYSTATE_H', '1'),
    ('Include/cpython/sysmodule.h', 'Py_CPYTHON_SYSMODULE_H', '1'),
    ('Include/cpython/traceback.h', 'Py_CPYTHON_TRACEBACK_H', '1'),
    ('Include/cpython/tupleobject.h', 'Py_CPYTHON_TUPLEOBJECT_H', '1'),
    ('Include/cpython/unicodeobject.h', 'Py_CPYTHON_UNICODEOBJECT_H', '1'),

    # implied include of <unistd.h>
    ('Include/**/*.h', '_POSIX_THREADS', '1'),
    ('Include/**/*.h', 'HAVE_PTHREAD_H', '1'),

    # from pyconfig.h
    ('Include/cpython/pthread_stubs.h', 'HAVE_PTHREAD_STUBS', '1'),
    ('Python/thread_pthread_stubs.h', 'HAVE_PTHREAD_STUBS', '1'),

    # from Objects/bytesobject.c
    ('Objects/stringlib/partition.h', 'STRINGLIB_GET_EMPTY()', 'bytes_get_empty()'),
    ('Objects/stringlib/join.h', 'STRINGLIB_MUTABLE', '0'),
    ('Objects/stringlib/partition.h', 'STRINGLIB_MUTABLE', '0'),
    ('Objects/stringlib/split.h', 'STRINGLIB_MUTABLE', '0'),
    ('Objects/stringlib/transmogrify.h', 'STRINGLIB_MUTABLE', '0'),

    # from Makefile
    ('Modules/getpath.c', 'PYTHONPATH', '1'),
    ('Modules/getpath.c', 'PREFIX', '...'),
    ('Modules/getpath.c', 'EXEC_PREFIX', '...'),
    ('Modules/getpath.c', 'VERSION', '...'),
    ('Modules/getpath.c', 'VPATH', '...'),
    ('Modules/getpath.c', 'PLATLIBDIR', '...'),
    # ('Modules/_dbmmodule.c', 'USE_GDBM_COMPAT', '1'),
    ('Modules/_dbmmodule.c', 'USE_NDBM', '1'),
    # ('Modules/_dbmmodule.c', 'USE_BERKDB', '1'),

    # See: setup.py
    ('Modules/_decimal/**/*.c', 'CONFIG_64', '1'),
    ('Modules/_decimal/**/*.c', 'ASM', '1'),
    ('Modules/expat/xmlparse.c', 'HAVE_EXPAT_CONFIG_H', '1'),
    ('Modules/expat/xmlparse.c', 'XML_POOR_ENTROPY', '1'),
    ('Modules/_dbmmodule.c', 'HAVE_GDBM_DASH_NDBM_H', '1'),

    # others
    ('Modules/_sre/sre_lib.h', 'LOCAL(type)', 'static inline type'),
    ('Modules/_sre/sre_lib.h', 'SRE(F)', 'sre_ucs2_##F'),
    ('Objects/stringlib/codecs.h', 'STRINGLIB_IS_UNICODE', '1'),
    ('Include/internal/pycore_crossinterp_data_registry.h', 'Py_CORE_CROSSINTERP_DATA_REGISTRY_H', '1'),
])

# -pthread
# -Wno-unused-result
# -Wsign-compare
# -g
# -Og
# -Wall
# -std=c99
# -Wextra
# -Wno-unused-result -Wno-unused-parameter
# -Wno-missing-field-initializers
# -Werror=implicit-function-declaration

SAME = {
    _abs('Include/*.h'): [_abs('Include/cpython/')],
    _abs('Python/ceval.c'): ['Python/generated_cases.c.h'],
}

MAX_SIZES = {
    # GLOB: (MAXTEXT, MAXLINES),
    # default: (10_000, 200)
    # First match wins.
    _abs('Modules/_ctypes/ctypes.h'): (5_000, 500),
    _abs('Modules/_datetimemodule.c'): (20_000, 300),
    _abs('Modules/_hacl/*.c'): (200_000, 500),
    _abs('Modules/posixmodule.c'): (20_000, 500),
    _abs('Modules/termios.c'): (10_000, 800),
    _abs('Modules/_testcapimodule.c'): (20_000, 400),
    _abs('Modules/expat/expat.h'): (10_000, 400),
    _abs('Objects/stringlib/unicode_format.h'): (10_000, 400),
    _abs('Objects/typeobject.c'): (380_000, 13_000),
    _abs('Python/compile.c'): (20_000, 500),
    _abs('Python/optimizer.c'): (100_000, 5_000),
    _abs('Python/parking_lot.c'): (40_000, 1000),
    _abs('Python/pylifecycle.c'): (750_000, 5000),
    _abs('Python/pystate.c'): (750_000, 5000),
    _abs('Python/initconfig.c'): (50_000, 500),

    # Generated files:
    _abs('Include/internal/pycore_opcode.h'): (10_000, 1000),
    _abs('Include/internal/pycore_global_strings.h'): (5_000, 1000),
    _abs('Include/internal/pycore_runtime_init_generated.h'): (5_000, 1000),
    _abs('Python/deepfreeze/*.c'): (20_000, 500),
    _abs('Python/frozen_modules/*.h'): (20_000, 500),
    _abs('Python/opcode_targets.h'): (10_000, 500),
    _abs('Python/stdlib_module_names.h'): (5_000, 500),

    # These large files are currently ignored (see above).
    _abs('Modules/_ssl_data_31.h'): (80_000, 10_000),
    _abs('Modules/_ssl_data_300.h'): (80_000, 10_000),
    _abs('Modules/_ssl_data_111.h'): (80_000, 10_000),
    _abs('Modules/cjkcodecs/mappings_*.h'): (160_000, 2_000),
    _abs('Modules/unicodedata_db.h'): (180_000, 3_000),
    _abs('Modules/unicodename_db.h'): (1_200_000, 15_000),
    _abs('Objects/unicodetype_db.h'): (240_000, 3_000),

    # Catch-alls:
    _abs('Include/**/*.h'): (5_000, 500),
}


def get_preprocessor(*,
                     file_macros=None,
                     file_includes=None,
                     file_incldirs=None,
                     file_same=None,
                     **kwargs
                     ):
    macros = tuple(MACROS)
    if file_macros:
        macros += tuple(file_macros)
    includes = tuple(INCLUDES)
    if file_includes:
        includes += tuple(file_includes)
    incldirs = tuple(INCL_DIRS)
    if file_incldirs:
        incldirs += tuple(file_incldirs)
    samefiles = dict(SAME)
    if file_same:
        samefiles.update(file_same)
    return _get_preprocessor(
        file_macros=macros,
        file_includes=includes,
        file_incldirs=incldirs,
        file_same=samefiles,
        **kwargs
    )


def parse_file(filename, *,
               match_kind=None,
               ignore_exc=None,
               log_err=None,
               ):
    get_file_preprocessor = get_preprocessor(
        ignore_exc=ignore_exc,
        log_err=log_err,
    )
    yield from _parse_file(
        filename,
        match_kind=match_kind,
        get_file_preprocessor=get_file_preprocessor,
        file_maxsizes=MAX_SIZES,
    )


def parse_files(filenames=None, *,
                match_kind=None,
                ignore_exc=None,
                log_err=None,
                get_file_preprocessor=None,
                **file_kwargs
                ):
    if get_file_preprocessor is None:
        get_file_preprocessor = get_preprocessor(
            ignore_exc=ignore_exc,
            log_err=log_err,
        )
    yield from _parse_files(
        filenames,
        match_kind=match_kind,
        get_file_preprocessor=get_file_preprocessor,
        file_maxsizes=MAX_SIZES,
        **file_kwargs
    )
