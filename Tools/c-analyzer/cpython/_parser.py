import os.path
import re

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


def clean_lines(text):
    """Clear out comments, blank lines, and leading/trailing whitespace."""
    lines = (line.strip() for line in text.splitlines())
    lines = (line.partition('#')[0].rstrip()
             for line in lines
             if line and not line.startswith('#'))
    glob_all = f'{GLOB_ALL} '
    lines = (re.sub(r'^[*] ', glob_all, line) for line in lines)
    lines = (_abs(line) for line in lines)
    return list(lines)


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
EXCLUDED = clean_lines('''
# @begin=conf@

# Rather than fixing for this one, we manually make sure it's okay.
Modules/_sha3/kcp/KeccakP-1600-opt64.c

# OSX
#Modules/_ctypes/darwin/*.c
#Modules/_ctypes/libffi_osx/*.c
Modules/_scproxy.c                # SystemConfiguration/SystemConfiguration.h

# Windows
Modules/_winapi.c               # windows.h
Modules/expat/winconfig.h
Modules/overlapped.c            # winsock.h
Python/dynload_win.c            # windows.h
Python/thread_nt.h

# other OS-dependent
Python/dynload_aix.c            # sys/ldr.h
Python/dynload_dl.c             # dl.h
Python/dynload_hpux.c           # dl.h
Python/thread_pthread.h
Python/emscripten_signal.c

# only huge constants (safe but parsing is slow)
Modules/_blake2/impl/blake2-kat.h
Modules/_ssl_data.h
Modules/_ssl_data_31.h
Modules/_ssl_data_300.h
Modules/_ssl_data_111.h
Modules/cjkcodecs/mappings_*.h
Modules/unicodedata_db.h
Modules/unicodename_db.h
Objects/unicodetype_db.h

# generated
Python/deepfreeze/*.c
Python/frozen_modules/*.h
Python/opcode_targets.h
Python/stdlib_module_names.h

# @end=conf@
''')

# XXX Fix the parser.
EXCLUDED += clean_lines('''
# The tool should be able to parse these...

Modules/hashlib.h
Objects/stringlib/codecs.h
Objects/stringlib/count.h
Objects/stringlib/ctype.h
Objects/stringlib/fastsearch.h
Objects/stringlib/find.h
Objects/stringlib/find_max_char.h
Objects/stringlib/partition.h
Objects/stringlib/replace.h
Objects/stringlib/split.h

Modules/_dbmmodule.c
Modules/cjkcodecs/_codecs_*.c
Modules/expat/internal.h
Modules/expat/xmlrole.c
Modules/expat/xmlparse.c
Python/initconfig.c
''')

INCL_DIRS = clean_lines('''
# @begin=tsv@

glob	dirname
*	.
*	./Include
*	./Include/internal

Modules/_tkinter.c	/usr/include/tcl8.6
Modules/tkappinit.c	/usr/include/tcl
Modules/_decimal/**/*.c	Modules/_decimal/libmpdec

# @end=tsv@
''')[1:]

MACROS = clean_lines('''
# @begin=tsv@

glob	name	value

Include/internal/*.h	Py_BUILD_CORE	1
Python/**/*.c	Py_BUILD_CORE	1
Parser/**/*.c	Py_BUILD_CORE	1
Objects/**/*.c	Py_BUILD_CORE	1

Modules/_asynciomodule.c	Py_BUILD_CORE	1
Modules/_collectionsmodule.c	Py_BUILD_CORE	1
Modules/_ctypes/_ctypes.c	Py_BUILD_CORE	1
Modules/_ctypes/cfield.c	Py_BUILD_CORE	1
Modules/_cursesmodule.c	Py_BUILD_CORE	1
Modules/_datetimemodule.c	Py_BUILD_CORE	1
Modules/_functoolsmodule.c	Py_BUILD_CORE	1
Modules/_heapqmodule.c	Py_BUILD_CORE	1
Modules/_io/*.c	Py_BUILD_CORE	1
Modules/_localemodule.c	Py_BUILD_CORE	1
Modules/_operator.c	Py_BUILD_CORE	1
Modules/_posixsubprocess.c	Py_BUILD_CORE	1
Modules/_sre/sre.c	Py_BUILD_CORE	1
Modules/_threadmodule.c	Py_BUILD_CORE	1
Modules/_tracemalloc.c	Py_BUILD_CORE	1
Modules/_weakref.c	Py_BUILD_CORE	1
Modules/_zoneinfo.c	Py_BUILD_CORE	1
Modules/atexitmodule.c	Py_BUILD_CORE	1
Modules/cmathmodule.c	Py_BUILD_CORE	1
Modules/faulthandler.c	Py_BUILD_CORE	1
Modules/gcmodule.c	Py_BUILD_CORE	1
Modules/getpath.c	Py_BUILD_CORE	1
Modules/getpath_noop.c	Py_BUILD_CORE	1
Modules/itertoolsmodule.c	Py_BUILD_CORE	1
Modules/main.c	Py_BUILD_CORE	1
Modules/mathmodule.c	Py_BUILD_CORE	1
Modules/posixmodule.c	Py_BUILD_CORE	1
Modules/sha256module.c	Py_BUILD_CORE	1
Modules/sha512module.c	Py_BUILD_CORE	1
Modules/signalmodule.c	Py_BUILD_CORE	1
Modules/symtablemodule.c	Py_BUILD_CORE	1
Modules/timemodule.c	Py_BUILD_CORE	1
Modules/unicodedata.c	Py_BUILD_CORE	1
Objects/stringlib/codecs.h	Py_BUILD_CORE	1
Objects/stringlib/unicode_format.h	Py_BUILD_CORE	1
Parser/string_parser.h	Py_BUILD_CORE	1
Parser/pegen.h	Py_BUILD_CORE	1
Python/ceval_gil.h	Py_BUILD_CORE	1
Python/condvar.h	Py_BUILD_CORE	1

Modules/_json.c	Py_BUILD_CORE_BUILTIN	1
Modules/_pickle.c	Py_BUILD_CORE_BUILTIN	1
Modules/_testinternalcapi.c	Py_BUILD_CORE_BUILTIN	1

Include/cpython/abstract.h	Py_CPYTHON_ABSTRACTOBJECT_H	1
Include/cpython/bytearrayobject.h	Py_CPYTHON_BYTEARRAYOBJECT_H	1
Include/cpython/bytesobject.h	Py_CPYTHON_BYTESOBJECT_H	1
Include/cpython/ceval.h	Py_CPYTHON_CEVAL_H	1
Include/cpython/code.h	Py_CPYTHON_CODE_H	1
Include/cpython/dictobject.h	Py_CPYTHON_DICTOBJECT_H	1
Include/cpython/fileobject.h	Py_CPYTHON_FILEOBJECT_H	1
Include/cpython/fileutils.h	Py_CPYTHON_FILEUTILS_H	1
Include/cpython/frameobject.h	Py_CPYTHON_FRAMEOBJECT_H	1
Include/cpython/import.h	Py_CPYTHON_IMPORT_H	1
Include/cpython/listobject.h	Py_CPYTHON_LISTOBJECT_H	1
Include/cpython/methodobject.h	Py_CPYTHON_METHODOBJECT_H	1
Include/cpython/object.h	Py_CPYTHON_OBJECT_H	1
Include/cpython/objimpl.h	Py_CPYTHON_OBJIMPL_H	1
Include/cpython/pyerrors.h	Py_CPYTHON_ERRORS_H	1
Include/cpython/pylifecycle.h	Py_CPYTHON_PYLIFECYCLE_H	1
Include/cpython/pymem.h	Py_CPYTHON_PYMEM_H	1
Include/cpython/pystate.h	Py_CPYTHON_PYSTATE_H	1
Include/cpython/sysmodule.h	Py_CPYTHON_SYSMODULE_H	1
Include/cpython/traceback.h	Py_CPYTHON_TRACEBACK_H	1
Include/cpython/tupleobject.h	Py_CPYTHON_TUPLEOBJECT_H	1
Include/cpython/unicodeobject.h	Py_CPYTHON_UNICODEOBJECT_H	1
Include/internal/pycore_code.h	SIZEOF_VOID_P	8

# implied include of pyport.h
Include/**/*.h	PyAPI_DATA(RTYPE)	extern RTYPE
Include/**/*.h	PyAPI_FUNC(RTYPE)	RTYPE
Include/**/*.h	Py_DEPRECATED(VER)	/* */
Include/**/*.h	_Py_NO_RETURN	/* */
Include/**/*.h	PYLONG_BITS_IN_DIGIT	30
Modules/**/*.c	PyMODINIT_FUNC	PyObject*
Objects/unicodeobject.c	PyMODINIT_FUNC	PyObject*
Python/marshal.c	PyMODINIT_FUNC	PyObject*
Python/_warnings.c	PyMODINIT_FUNC	PyObject*
Python/Python-ast.c	PyMODINIT_FUNC	PyObject*
Python/import.c	PyMODINIT_FUNC	PyObject*
Modules/_testcapimodule.c	PyAPI_FUNC(RTYPE)	RTYPE
Python/getargs.c	PyAPI_FUNC(RTYPE)	RTYPE
Objects/stringlib/unicode_format.h	Py_LOCAL_INLINE(type)	static inline type
Include/pymath.h	_Py__has_builtin(x)	0

# implied include of pymacro.h
*/clinic/*.c.h	PyDoc_VAR(name)	static const char name[]
*/clinic/*.c.h	PyDoc_STR(str)	str
*/clinic/*.c.h	PyDoc_STRVAR(name,str)	PyDoc_VAR(name) = PyDoc_STR(str)

# implied include of exports.h
#Modules/_io/bytesio.c	Py_EXPORTED_SYMBOL	/* */

# implied include of object.h
Include/**/*.h	PyObject_HEAD	PyObject ob_base;
Include/**/*.h	PyObject_VAR_HEAD	PyVarObject ob_base;

# implied include of pyconfig.h
Include/**/*.h	SIZEOF_WCHAR_T	4

# implied include of <unistd.h>
Include/**/*.h	_POSIX_THREADS	1
Include/**/*.h	HAVE_PTHREAD_H	1

# from Makefile
Modules/getpath.c	PYTHONPATH	1
Modules/getpath.c	PREFIX	...
Modules/getpath.c	EXEC_PREFIX	...
Modules/getpath.c	VERSION	...
Modules/getpath.c	VPATH	...
Modules/getpath.c	PLATLIBDIR	...

# from Modules/_sha3/sha3module.c
Modules/_sha3/kcp/KeccakP-1600-inplace32BI.c	PLATFORM_BYTE_ORDER	4321  # force big-endian
Modules/_sha3/kcp/*.c	KeccakOpt	64
Modules/_sha3/kcp/*.c	KeccakP200_excluded	1
Modules/_sha3/kcp/*.c	KeccakP400_excluded	1
Modules/_sha3/kcp/*.c	KeccakP800_excluded	1

# See: setup.py
Modules/_decimal/**/*.c	CONFIG_64	1
Modules/_decimal/**/*.c	ASM	1
Modules/expat/xmlparse.c	HAVE_EXPAT_CONFIG_H	1
Modules/expat/xmlparse.c	XML_POOR_ENTROPY	1
Modules/_dbmmodule.c	HAVE_GDBM_DASH_NDBM_H	1

# others
Modules/_sre/sre_lib.h	LOCAL(type)	static inline type
Modules/_sre/sre_lib.h	SRE(F)	sre_ucs2_##F
Objects/stringlib/codecs.h	STRINGLIB_IS_UNICODE	1
Include/internal/pycore_bitutils.h	_Py__has_builtin(B)	0

# @end=tsv@
''')[1:]

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

SAME = [
    './Include/cpython/',
]

MAX_SIZES = {
    _abs('Include/**/*.h'): (5_000, 500),
    _abs('Modules/_ctypes/ctypes.h'): (5_000, 500),
    _abs('Modules/_datetimemodule.c'): (20_000, 300),
    _abs('Modules/posixmodule.c'): (20_000, 500),
    _abs('Modules/termios.c'): (10_000, 800),
    _abs('Modules/_testcapimodule.c'): (20_000, 400),
    _abs('Modules/expat/expat.h'): (10_000, 400),
    _abs('Objects/stringlib/unicode_format.h'): (10_000, 400),
    _abs('Objects/typeobject.c'): (20_000, 200),
    _abs('Python/compile.c'): (20_000, 500),
    _abs('Python/pylifecycle.c'): (500_000, 5000),
    _abs('Python/pystate.c'): (500_000, 5000),
}


def get_preprocessor(*,
                     file_macros=None,
                     file_incldirs=None,
                     file_same=None,
                     **kwargs
                     ):
    macros = tuple(MACROS)
    if file_macros:
        macros += tuple(file_macros)
    incldirs = tuple(INCL_DIRS)
    if file_incldirs:
        incldirs += tuple(file_incldirs)
    return _get_preprocessor(
        file_macros=macros,
        file_incldirs=incldirs,
        file_same=file_same,
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
