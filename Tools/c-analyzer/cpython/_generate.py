# The code here consists of hacks for pre-populating the known.tsv file.

from c_analyzer.parser.preprocessor import _iter_clean_lines
from c_analyzer.parser.naive import (
        iter_variables, parse_variable_declaration, find_variables,
        )
from c_analyzer.common.known import HEADER as KNOWN_HEADER
from c_analyzer.common.info import UNKNOWN, ID
from c_analyzer.variables import Variable
from c_analyzer.util import write_tsv

from . import SOURCE_DIRS, REPO_ROOT
from .known import DATA_FILE as KNOWN_FILE
from .files import iter_cpython_files


POTS = ('char ', 'wchar_t ', 'int ', 'Py_ssize_t ')
POTS += tuple('const ' + v for v in POTS)
STRUCTS = ('PyTypeObject', 'PyObject', 'PyMethodDef', 'PyModuleDef', 'grammar')


def _parse_global(line, funcname=None):
    line = line.strip()
    if line.startswith('static '):
        if '(' in line and '[' not in line and ' = ' not in line:
            return None, None
        name, decl = parse_variable_declaration(line)
    elif line.startswith(('Py_LOCAL(', 'Py_LOCAL_INLINE(')):
        name, decl = parse_variable_declaration(line)
    elif line.startswith('_Py_static_string('):
        decl = line.strip(';').strip()
        name = line.split('(')[1].split(',')[0].strip()
    elif line.startswith('_Py_IDENTIFIER('):
        decl = line.strip(';').strip()
        name = 'PyId_' + line.split('(')[1].split(')')[0].strip()
    elif funcname:
        return None, None

    # global-only
    elif line.startswith('PyAPI_DATA('):  # only in .h files
        name, decl = parse_variable_declaration(line)
    elif line.startswith('extern '):  # only in .h files
        name, decl = parse_variable_declaration(line)
    elif line.startswith('PyDoc_VAR('):
        decl = line.strip(';').strip()
        name = line.split('(')[1].split(')')[0].strip()
    elif line.startswith(POTS):  # implied static
        if '(' in line and '[' not in line and ' = ' not in line:
            return None, None
        name, decl = parse_variable_declaration(line)
    elif line.startswith(STRUCTS) and line.endswith(' = {'):  # implied static
        name, decl = parse_variable_declaration(line)
    elif line.startswith(STRUCTS) and line.endswith(' = NULL;'):  # implied static
        name, decl = parse_variable_declaration(line)
    elif line.startswith('struct '):
        if not line.endswith(' = {'):
            return None, None
        if not line.partition(' ')[2].startswith(STRUCTS):
            return None, None
        # implied static
        name, decl = parse_variable_declaration(line)

    # file-specific
    elif line.startswith(('SLOT1BINFULL(', 'SLOT1BIN(')):
        # Objects/typeobject.c
        funcname = line.split('(')[1].split(',')[0]
        return [
                ('op_id', funcname, '_Py_static_string(op_id, OPSTR)'),
                ('rop_id', funcname, '_Py_static_string(op_id, OPSTR)'),
                ]
    elif line.startswith('WRAP_METHOD('):
        # Objects/weakrefobject.c
        funcname, name = (v.strip() for v in line.split('(')[1].split(')')[0].split(','))
        return [
                ('PyId_' + name, funcname, f'_Py_IDENTIFIER({name})'),
                ]

    else:
        return None, None
    return name, decl


def _pop_cached(varcache, filename, funcname, name, *,
                _iter_variables=iter_variables,
                ):
    # Look for the file.
    try:
        cached = varcache[filename]
    except KeyError:
        cached = varcache[filename] = {}
        for variable in _iter_variables(filename,
                                        parse_variable=_parse_global,
                                        ):
            variable._isglobal = True
            cached[variable.id] = variable
        for var in cached:
            print(' ', var)

    # Look for the variable.
    if funcname == UNKNOWN:
        for varid in cached:
            if varid.name == name:
                break
        else:
            return None
        return cached.pop(varid)
    else:
        return cached.pop((filename, funcname, name), None)


def find_matching_variable(varid, varcache, allfilenames, *,
                           _pop_cached=_pop_cached,
                           ):
    if varid.filename and varid.filename != UNKNOWN:
        filenames = [varid.filename]
    else:
        filenames = allfilenames
    for filename in filenames:
        variable = _pop_cached(varcache, filename, varid.funcname, varid.name)
        if variable is not None:
            return variable
    else:
        if varid.filename and varid.filename != UNKNOWN and varid.funcname is None:
            for filename in allfilenames:
                if not filename.endswith('.h'):
                    continue
                variable = _pop_cached(varcache, filename, None, varid.name)
                if variable is not None:
                    return variable
        return None


MULTILINE = {
    # Python/Python-ast.c
    'Load_singleton': 'PyObject *',
    'Store_singleton': 'PyObject *',
    'Del_singleton': 'PyObject *',
    'AugLoad_singleton': 'PyObject *',
    'AugStore_singleton': 'PyObject *',
    'Param_singleton': 'PyObject *',
    'And_singleton': 'PyObject *',
    'Or_singleton': 'PyObject *',
    'Add_singleton': 'static PyObject *',
    'Sub_singleton': 'static PyObject *',
    'Mult_singleton': 'static PyObject *',
    'MatMult_singleton': 'static PyObject *',
    'Div_singleton': 'static PyObject *',
    'Mod_singleton': 'static PyObject *',
    'Pow_singleton': 'static PyObject *',
    'LShift_singleton': 'static PyObject *',
    'RShift_singleton': 'static PyObject *',
    'BitOr_singleton': 'static PyObject *',
    'BitXor_singleton': 'static PyObject *',
    'BitAnd_singleton': 'static PyObject *',
    'FloorDiv_singleton': 'static PyObject *',
    'Invert_singleton': 'static PyObject *',
    'Not_singleton': 'static PyObject *',
    'UAdd_singleton': 'static PyObject *',
    'USub_singleton': 'static PyObject *',
    'Eq_singleton': 'static PyObject *',
    'NotEq_singleton': 'static PyObject *',
    'Lt_singleton': 'static PyObject *',
    'LtE_singleton': 'static PyObject *',
    'Gt_singleton': 'static PyObject *',
    'GtE_singleton': 'static PyObject *',
    'Is_singleton': 'static PyObject *',
    'IsNot_singleton': 'static PyObject *',
    'In_singleton': 'static PyObject *',
    'NotIn_singleton': 'static PyObject *',
    # Python/symtable.c
    'top': 'static identifier ',
    'lambda': 'static identifier ',
    'genexpr': 'static identifier ',
    'listcomp': 'static identifier ',
    'setcomp': 'static identifier ',
    'dictcomp': 'static identifier ',
    '__class__': 'static identifier ',
    # Python/compile.c
    '__doc__': 'static PyObject *',
    '__annotations__': 'static PyObject *',
    # Objects/floatobject.c
    'double_format': 'static float_format_type ',
    'float_format': 'static float_format_type ',
    'detected_double_format': 'static float_format_type ',
    'detected_float_format': 'static float_format_type ',
    # Parser/listnode.c
    'level': 'static int ',
    'atbol': 'static int ',
    # Python/dtoa.c
    'private_mem': 'static double private_mem[PRIVATE_mem]',
    'pmem_next': 'static double *',
    # Modules/_weakref.c
    'weakref_functions': 'static PyMethodDef ',
}
INLINE = {
    # Modules/_tracemalloc.c
    'allocators': 'static struct { PyMemAllocatorEx mem; PyMemAllocatorEx raw; PyMemAllocatorEx obj; } ',
    # Modules/faulthandler.c
    'fatal_error': 'static struct { int enabled; PyObject *file; int fd; int all_threads; PyInterpreterState *interp; void *exc_handler; } ',
    'thread': 'static struct { PyObject *file; int fd; PY_TIMEOUT_T timeout_us; int repeat; PyInterpreterState *interp; int exit; char *header; size_t header_len; PyThread_type_lock cancel_event; PyThread_type_lock running; } ',
    # Modules/signalmodule.c
    'Handlers': 'static volatile struct { _Py_atomic_int tripped; PyObject *func; } Handlers[NSIG]',
    'wakeup': 'static volatile struct { SOCKET_T fd; int warn_on_full_buffer; int use_send; } ',
    # Python/dynload_shlib.c
    'handles': 'static struct { dev_t dev; ino_t ino; void *handle; } handles[128]',
    # Objects/obmalloc.c
    '_PyMem_Debug': 'static struct { debug_alloc_api_t raw; debug_alloc_api_t mem; debug_alloc_api_t obj; } ',
    # Python/bootstrap_hash.c
    'urandom_cache': 'static struct { int fd; dev_t st_dev; ino_t st_ino; } ',
    }
FUNC = {
    # Objects/object.c
    '_Py_abstract_hack': 'Py_ssize_t (*_Py_abstract_hack)(PyObject *)',
    # Parser/myreadline.c
    'PyOS_InputHook': 'int (*PyOS_InputHook)(void)',
    # Python/pylifecycle.c
    '_PyOS_mystrnicmp_hack': 'int (*_PyOS_mystrnicmp_hack)(const char *, const char *, Py_ssize_t)',
    # Parser/myreadline.c
    'PyOS_ReadlineFunctionPointer': 'char *(*PyOS_ReadlineFunctionPointer)(FILE *, FILE *, const char *)',
    }
IMPLIED = {
    # Objects/boolobject.c
    '_Py_FalseStruct': 'static struct _longobject ',
    '_Py_TrueStruct': 'static struct _longobject ',
    # Modules/config.c
    '_PyImport_Inittab': 'struct _inittab _PyImport_Inittab[]',
    }
GLOBALS = {}
GLOBALS.update(MULTILINE)
GLOBALS.update(INLINE)
GLOBALS.update(FUNC)
GLOBALS.update(IMPLIED)

LOCALS = {
    'buildinfo': ('Modules/getbuildinfo.c',
                  'Py_GetBuildInfo',
                  'static char buildinfo[50 + sizeof(GITVERSION) + ((sizeof(GITTAG) > sizeof(GITBRANCH)) ?  sizeof(GITTAG) : sizeof(GITBRANCH))]'),
    'methods': ('Python/codecs.c',
                '_PyCodecRegistry_Init',
                'static struct { char *name; PyMethodDef def; } methods[]'),
    }


def _known(symbol):
    if symbol.funcname:
        if symbol.funcname != UNKNOWN or symbol.filename != UNKNOWN:
            raise KeyError(symbol.name)
        filename, funcname, decl = LOCALS[symbol.name]
        varid = ID(filename, funcname, symbol.name)
    elif not symbol.filename or symbol.filename == UNKNOWN:
        raise KeyError(symbol.name)
    else:
        varid = symbol.id
        try:
            decl = GLOBALS[symbol.name]
        except KeyError:

            if symbol.name.endswith('_methods'):
                decl = 'static PyMethodDef '
            elif symbol.filename == 'Objects/exceptions.c' and symbol.name.startswith(('PyExc_', '_PyExc_')):
                decl = 'static PyTypeObject '
            else:
                raise
    if symbol.name not in decl:
        decl = decl + symbol.name
    return Variable(varid, 'static', decl)


def known_row(varid, decl):
    return (
            varid.filename,
            varid.funcname or '-',
            varid.name,
            'variable',
            decl,
            )


def known_rows(symbols, *,
               cached=True,
               _get_filenames=iter_cpython_files,
               _find_match=find_matching_variable,
               _find_symbols=find_variables,
               _as_known=known_row,
               ):
    filenames = list(_get_filenames())
    cache = {}
    if cached:
        for symbol in symbols:
            try:
                found = _known(symbol)
            except KeyError:
                found = _find_match(symbol, cache, filenames)
                if found is None:
                    found = Variable(symbol.id, UNKNOWN, UNKNOWN)
            yield _as_known(found.id, found.vartype)
    else:
        raise NotImplementedError  # XXX incorporate KNOWN
        for variable in _find_symbols(symbols, filenames,
                                      srccache=cache,
                                      parse_variable=_parse_global,
                                      ):
            #variable = variable._replace(
            #    filename=os.path.relpath(variable.filename, REPO_ROOT))
            if variable.funcname == UNKNOWN:
                print(variable)
            if variable.vartype== UNKNOWN:
                print(variable)
            yield _as_known(variable.id, variable.vartype)


def generate(symbols, filename=None, *,
             _generate_rows=known_rows,
             _write_tsv=write_tsv,
             ):
    if not filename:
        filename = KNOWN_FILE + '.new'

    rows = _generate_rows(symbols)
    _write_tsv(filename, KNOWN_HEADER, rows)


if __name__ == '__main__':
    from c_symbols import binary
    symbols = binary.iter_symbols(
            binary.PYTHON,
            find_local_symbol=None,
            )
    generate(symbols)
