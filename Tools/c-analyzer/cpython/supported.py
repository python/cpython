import os.path
import re

from c_analyzer.common.info import ID
from c_analyzer.common.util import read_tsv, write_tsv

from . import DATA_DIR

# XXX need tests:
# * generate / script


IGNORED_FILE = os.path.join(DATA_DIR, 'ignored.tsv')

IGNORED_COLUMNS = ('filename', 'funcname', 'name', 'kind', 'reason')
IGNORED_HEADER = '\t'.join(IGNORED_COLUMNS)

# XXX Move these to ignored.tsv.
IGNORED = {
        # global
        'PyImport_FrozenModules': 'process-global',
        'M___hello__': 'process-global',
        'inittab_copy': 'process-global',
        'PyHash_Func': 'process-global',
        '_Py_HashSecret_Initialized': 'process-global',
        '_TARGET_LOCALES': 'process-global',

        # startup (only changed before/during)
        '_PyRuntime': 'runtime startup',
        'runtime_initialized': 'runtime startup',
        'static_arg_parsers': 'runtime startup',
        'orig_argv': 'runtime startup',
        'opt_ptr': 'runtime startup',
        '_preinit_warnoptions': 'runtime startup',
        '_Py_StandardStreamEncoding': 'runtime startup',
        'Py_FileSystemDefaultEncoding': 'runtime startup',
        '_Py_StandardStreamErrors': 'runtime startup',
        'Py_FileSystemDefaultEncodeErrors': 'runtime startup',
        'Py_BytesWarningFlag': 'runtime startup',
        'Py_DebugFlag': 'runtime startup',
        'Py_DontWriteBytecodeFlag': 'runtime startup',
        'Py_FrozenFlag': 'runtime startup',
        'Py_HashRandomizationFlag': 'runtime startup',
        'Py_IgnoreEnvironmentFlag': 'runtime startup',
        'Py_InspectFlag': 'runtime startup',
        'Py_InteractiveFlag': 'runtime startup',
        'Py_IsolatedFlag': 'runtime startup',
        'Py_NoSiteFlag': 'runtime startup',
        'Py_NoUserSiteDirectory': 'runtime startup',
        'Py_OptimizeFlag': 'runtime startup',
        'Py_QuietFlag': 'runtime startup',
        'Py_UTF8Mode': 'runtime startup',
        'Py_UnbufferedStdioFlag': 'runtime startup',
        'Py_VerboseFlag': 'runtime startup',
        '_Py_path_config': 'runtime startup',
        '_PyOS_optarg': 'runtime startup',
        '_PyOS_opterr': 'runtime startup',
        '_PyOS_optind': 'runtime startup',
        '_Py_HashSecret': 'runtime startup',

        # REPL
        '_PyOS_ReadlineLock': 'repl',
        '_PyOS_ReadlineTState': 'repl',

        # effectively const
        'tracemalloc_empty_traceback': 'const',
        '_empty_bitmap_node': 'const',
        'posix_constants_pathconf': 'const',
        'posix_constants_confstr': 'const',
        'posix_constants_sysconf': 'const',
        '_PySys_ImplCacheTag': 'const',
        '_PySys_ImplName': 'const',
        'PyImport_Inittab': 'const',
        '_PyImport_DynLoadFiletab': 'const',
        '_PyParser_Grammar': 'const',
        'Py_hexdigits': 'const',
        '_PyImport_Inittab': 'const',
        '_PyByteArray_empty_string': 'const',
        '_PyLong_DigitValue': 'const',
        '_Py_SwappedOp': 'const',
        'PyStructSequence_UnnamedField': 'const',

        # signals are main-thread only
        'faulthandler_handlers': 'signals are main-thread only',
        'user_signals': 'signals are main-thread only',
        'wakeup': 'signals are main-thread only',

        # hacks
        '_PySet_Dummy': 'only used as a placeholder',
        }

BENIGN = 'races here are benign and unlikely'


def is_supported(variable, ignored=None, known=None, *,
                 _ignored=(lambda *a, **k: _is_ignored(*a, **k)),
                 _vartype_okay=(lambda *a, **k: _is_vartype_okay(*a, **k)),
                 ):
    """Return True if the given global variable is okay in CPython."""
    if _ignored(variable,
                ignored and ignored.get('variables')):
        return True
    elif _vartype_okay(variable.vartype,
                       ignored.get('types')):
        return True
    else:
        return False


def _is_ignored(variable, ignoredvars=None, *,
                _IGNORED=IGNORED,
                ):
    """Return the reason if the variable is a supported global.

    Return None if the variable is not a supported global.
    """
    if ignoredvars and (reason := ignoredvars.get(variable.id)):
        return reason

    if variable.funcname is None:
        if reason := _IGNORED.get(variable.name):
            return reason

    # compiler
    if variable.filename == 'Python/graminit.c':
        if variable.vartype.startswith('static state '):
            return 'compiler'
    if variable.filename == 'Python/symtable.c':
        if variable.vartype.startswith('static identifier '):
            return 'compiler'
    if variable.filename == 'Python/Python-ast.c':
        # These should be const.
        if variable.name.endswith('_field'):
            return 'compiler'
        if variable.name.endswith('_attribute'):
            return 'compiler'

    # other
    if variable.filename == 'Python/dtoa.c':
        # guarded by lock?
        if variable.name in ('p5s', 'freelist'):
            return 'dtoa is thread-safe?'
        if variable.name in ('private_mem', 'pmem_next'):
            return 'dtoa is thread-safe?'
    if variable.filename == 'Python/thread.c':
        # Threads do not become an issue until after these have been set
        # and these never get changed after that.
        if variable.name in ('initialized', 'thread_debug'):
            return 'thread-safe'
    if variable.filename == 'Python/getversion.c':
        if variable.name == 'version':
            # Races are benign here, as well as unlikely.
            return BENIGN
    if variable.filename == 'Python/fileutils.c':
        if variable.name == 'force_ascii':
            return BENIGN
        if variable.name == 'ioctl_works':
            return BENIGN
        if variable.name == '_Py_open_cloexec_works':
            return BENIGN
    if variable.filename == 'Python/codecs.c':
        if variable.name == 'ucnhash_CAPI':
            return BENIGN
    if variable.filename == 'Python/bootstrap_hash.c':
        if variable.name == 'getrandom_works':
            return BENIGN
    if variable.filename == 'Objects/unicodeobject.c':
        if variable.name == 'ucnhash_CAPI':
            return BENIGN
        if variable.name == 'bloom_linebreak':
            # *mostly* benign
            return BENIGN
    if variable.filename == 'Modules/getbuildinfo.c':
        if variable.name == 'buildinfo':
            # The static is used for pre-allocation.
            return BENIGN
    if variable.filename == 'Modules/posixmodule.c':
        if variable.name == 'ticks_per_second':
            return BENIGN
        if variable.name == 'dup3_works':
            return BENIGN
    if variable.filename == 'Modules/timemodule.c':
        if variable.name == 'ticks_per_second':
            return BENIGN
    if variable.filename == 'Objects/longobject.c':
        if variable.name == 'log_base_BASE':
            return BENIGN
        if variable.name == 'convwidth_base':
            return BENIGN
        if variable.name == 'convmultmax_base':
            return BENIGN

    return None


def _is_vartype_okay(vartype, ignoredtypes=None):
    if _is_object(vartype):
        return None

    if vartype.startswith('static const '):
        return 'const'
    if vartype.startswith('const '):
        return 'const'

    # components for TypeObject definitions
    for name in ('PyMethodDef', 'PyGetSetDef', 'PyMemberDef'):
        if name in vartype:
            return 'const'
    for name in ('PyNumberMethods', 'PySequenceMethods', 'PyMappingMethods',
                 'PyBufferProcs', 'PyAsyncMethods'):
        if name in vartype:
            return 'const'
    for name in ('slotdef', 'newfunc'):
        if name in vartype:
            return 'const'

    # structseq
    for name in ('PyStructSequence_Desc', 'PyStructSequence_Field'):
        if name in vartype:
            return 'const'

    # other definiitions
    if 'PyModuleDef' in vartype:
        return 'const'

    # thread-safe
    if '_Py_atomic_int' in vartype:
        return 'thread-safe'
    if 'pthread_condattr_t' in vartype:
        return 'thread-safe'

    # startup
    if '_Py_PreInitEntry' in vartype:
        return 'startup'

    # global
#    if 'PyMemAllocatorEx' in vartype:
#        return True

    # others
#    if 'PyThread_type_lock' in vartype:
#        return True

    # XXX ???
    # _Py_tss_t
    # _Py_hashtable_t
    # stack_t
    # _PyUnicode_Name_CAPI

    # functions
    if '(' in vartype and '[' not in vartype:
        return 'function pointer'

    # XXX finish!
    # * allow const values?
    #raise NotImplementedError
    return None


PYOBJECT_RE = re.compile(r'''
        ^
        (
            # must start with "static "
            static \s+
            (
                identifier
            )
            \b
        ) |
        (
            # may start with "static "
            ( static \s+ )?
            (
                .*
                (
                    PyObject |
                    PyTypeObject |
                    _? Py \w+ Object |
                    _PyArg_Parser |
                    _Py_Identifier |
                    traceback_t |
                    PyAsyncGenASend |
                    _PyAsyncGenWrappedValue |
                    PyContext |
                    method_cache_entry
                )
                \b
            ) |
            (
                (
                    _Py_IDENTIFIER |
                    _Py_static_string
                )
                [(]
            )
        )
        ''', re.VERBOSE)


def _is_object(vartype):
    if 'PyDictKeysObject' in vartype:
        return False
    if PYOBJECT_RE.match(vartype):
        return True
    if vartype.endswith((' _Py_FalseStruct', ' _Py_TrueStruct')):
        return True

    # XXX Add more?

    #for part in vartype.split():
    #    # XXX const is automatic True?
    #    if part == 'PyObject' or part.startswith('PyObject['):
    #        return True
    return False


def ignored_from_file(infile, *,
                      _read_tsv=read_tsv,
                      ):
    """Yield a Variable for each ignored var in the file."""
    ignored = {
        'variables': {},
        #'types': {},
        #'constants': {},
        #'macros': {},
        }
    for row in _read_tsv(infile, IGNORED_HEADER):
        filename, funcname, name, kind, reason = row
        if not funcname or funcname == '-':
            funcname = None
        id = ID(filename, funcname, name)
        if kind == 'variable':
            values = ignored['variables']
        else:
            raise ValueError(f'unsupported kind in row {row}')
        values[id] = reason
    return ignored


##################################
# generate

def _get_row(varid, reason):
    return (
            varid.filename,
            varid.funcname or '-',
            varid.name,
            'variable',
            str(reason),
            )


def _get_rows(variables, ignored=None, *,
              _as_row=_get_row,
              _is_ignored=_is_ignored,
              _vartype_okay=_is_vartype_okay,
              ):
    count = 0
    for variable in variables:
        reason = _is_ignored(variable,
                             ignored and ignored.get('variables'),
                             )
        if not reason:
            reason = _vartype_okay(variable.vartype,
                                   ignored and ignored.get('types'))
        if not reason:
            continue

        print(' ', variable, repr(reason))
        yield _as_row(variable.id, reason)
        count += 1
    print(f'total: {count}')


def _generate_ignored_file(variables, filename=None, *,
                           _generate_rows=_get_rows,
                           _write_tsv=write_tsv,
                           ):
    if not filename:
        filename = IGNORED_FILE + '.new'
    rows = _generate_rows(variables)
    _write_tsv(filename, IGNORED_HEADER, rows)


if __name__ == '__main__':
    from cpython import SOURCE_DIRS
    from cpython.known import (
        from_file as known_from_file,
        DATA_FILE as KNOWN_FILE,
        )
    # XXX This is wrong!
    from . import find
    known = known_from_file(KNOWN_FILE)
    knownvars = (known or {}).get('variables')
    variables = find.globals_from_binary(knownvars=knownvars,
                                         dirnames=SOURCE_DIRS)

    _generate_ignored_file(variables)
