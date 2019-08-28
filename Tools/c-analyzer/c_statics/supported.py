import csv
import os.path
import re

from c_analyzer_common import DATA_DIR
from c_analyzer_common.info import ID
from c_analyzer_common.util import read_tsv


def is_supported(static, ignored=None, known=None):
    """Return True if the given static variable is okay in CPython."""
    if _is_ignored(static, ignored and ignored.get('variables')):
        return True
    elif _is_vartype_okay(static.vartype, ignored.get('types')):
        return True
    else:
        return False


# XXX Move these to ignored.tsv.
IGNORED = {
        # global
        'PyImport_FrozenModules',
        'M___hello__',
        'inittab_copy',
        'PyHash_Func',
        '_Py_HashSecret_Initialized',
        '_TARGET_LOCALES',
        'runtime_initialized',

        # startup
        'static_arg_parsers',
        'orig_argv',
        'opt_ptr',
        '_preinit_warnoptions',
        '_Py_StandardStreamEncoding',
        '_Py_StandardStreamErrors',

        # should be const
        'tracemalloc_empty_traceback',
        '_empty_bitmap_node',
        'posix_constants_pathconf',
        'posix_constants_confstr',
        'posix_constants_sysconf',

        # signals are main-thread only
        'faulthandler_handlers',
        'user_signals',
        }


def _is_ignored(static, ignoredvars=None):
    if static.name in IGNORED:
        return True

    if ignoredvars and static.id in ignoredvars:
        return True

    # compiler
    if static.filename == 'Python/graminit.c':
        if static.vartype.startswith('static state '):
            return True
    if static.filename == 'Python/symtable.c':
        if static.vartype.startswith('static identifier '):
            return True
    if static.filename == 'Python/Python-ast.c':
        # These should be const.
        if static.name.endswith('_field'):
            return True
        if static.name.endswith('_attribute'):
            return True

    # other
    if static.filename == 'Python/dtoa.c':
        # guarded by lock?
        if static.name in ('p5s', 'freelist'):
            return True
        if static.name in ('private_mem', 'pmem_next'):
            return True

    return False


def _is_vartype_okay(vartype, ignoredtypes=None):
    if _is_object(vartype):
        return False

    # components for TypeObject definitions
    for name in ('PyMethodDef', 'PyGetSetDef', 'PyMemberDef'):
        if name in vartype:
            return True
    for name in ('PyNumberMethods', 'PySequenceMethods', 'PyMappingMethods',
                 'PyBufferProcs', 'PyAsyncMethods'):
        if name in vartype:
            return True
    for name in ('slotdef', 'newfunc'):
        if name in vartype:
            return True

    # structseq
    for name in ('PyStructSequence_Desc', 'PyStructSequence_Field'):
        if name in vartype:
            return True

    # other definiitions
    if 'PyModuleDef' in vartype:
        return True

    # thread-safe
    if '_Py_atomic_int' in vartype:
        return True
    if 'pthread_condattr_t' in vartype:
        return True

    # startup
    if '_Py_PreInitEntry' in vartype:
        return True

    # global
    if 'PyMemAllocatorEx' in vartype:
        return True

    # others
    if 'PyThread_type_lock' in vartype:
        return True
    #if '_Py_hashtable_t' in vartype:
    #    return True  # ???

    # XXX ???
    # _Py_tss_t
    # _Py_hashtable_t
    # stack_t
    # _PyUnicode_Name_CAPI

    # funcsions
    if '(' in vartype and '[' not in vartype:
        return True

    # XXX finish!
    # * allow const values?
    #raise NotImplementedError
    return False


def _is_object(vartype):
    if re.match(r'.*\bPy\w*Object\b', vartype):
        return True
    if '_Py_Identifier' in vartype:
        return True
    if 'traceback_t' in vartype:
        return True
    if 'PyAsyncGenASend' in vartype:
        return True
    if '_PyAsyncGenWrappedValue' in vartype:
        return True
    if 'PyContext' in vartype:
        return True
    if 'method_cache_entry' in vartype:
        return True

    # XXX Add more?

    #for part in vartype.split():
    #    # XXX const is automatic True?
    #    if part == 'PyObject' or part.startswith('PyObject['):
    #        return True
    return False


#############################
# ignored

IGNORED_FILE = os.path.join(DATA_DIR, 'ignored.tsv')

COLUMNS = ('filename', 'funcname', 'name', 'kind', 'reason')
HEADER = '\t'.join(COLUMNS)


def ignored_from_file(infile, *,
                      _read_tsv=read_tsv,
                      ):
    """Yield StaticVar for each ignored var in the file."""
    ignored = {
        'variables': {},
        #'types': {},
        #'constants': {},
        #'macros': {},
        }
    for row in _read_tsv(infile, HEADER):
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
