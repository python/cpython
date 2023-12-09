#include <stdbool.h>

#include "Python.h"
#include "pycore_code.h"          // write_location_entry_start()
#include "pycore_compile.h"
#include "pycore_opcode.h"        // _PyOpcode_Caches[] and opcode category macros
#include "pycore_pymem.h"         // _PyMem_IsPtrFreed()


#define DEFAULT_CODE_SIZE 128
#define DEFAULT_LNOTAB_SIZE 16
#define DEFAULT_CNOTAB_SIZE 32

#undef SUCCESS
#undef ERROR
#define SUCCESS 0
#define ERROR -1

#define RETURN_IF_ERROR(X)  \
    if ((X) == -1) {        \
        return ERROR;       \
    }

typedef _PyCompilerSrcLocation location;
typedef _PyCompile_Instruction instruction;
typedef _PyCompile_InstructionSequence instr_sequence;

static inline bool
same_location(location a, location b)
{
    return a.lineno == b.lineno &&
           a.end_lineno == b.end_lineno &&
           a.col_offset == b.col_offset &&
           a.end_col_offset == b.end_col_offset;
}

struct assembler {
    PyObject *a_bytecode;  /* bytes containing bytecode */
    int a_offset;              /* offset into bytecode */
    PyObject *a_except_table;  /* bytes containing exception table */
    int a_except_table_off;    /* offset into exception table */
    /* Location Info */
    int a_lineno;          /* lineno of last emitted instruction */
    PyObject* a_linetable; /* bytes containing location info */
    int a_location_off;    /* offset of last written location info frame */
};

static int
assemble_init(struct assembler *a, int firstlineno)
{
    memset(a, 0, sizeof(struct assembler));
    a->a_lineno = firstlineno;
    a->a_linetable = NULL;
    a->a_location_off = 0;
    a->a_except_table = NULL;
    a->a_bytecode = PyBytes_FromStringAndSize(NULL, DEFAULT_CODE_SIZE);
    if (a->a_bytecode == NULL) {
        goto error;
    }
    a->a_linetable = PyBytes_FromStringAndSize(NULL, DEFAULT_CNOTAB_SIZE);
    if (a->a_linetable == NULL) {
        goto error;
    }
    a->a_except_table = PyBytes_FromStringAndSize(NULL, DEFAULT_LNOTAB_SIZE);
    if (a->a_except_table == NULL) {
        goto error;
    }
    return SUCCESS;
error:
    Py_XDECREF(a->a_bytecode);
    Py_XDECREF(a->a_linetable);
    Py_XDECREF(a->a_except_table);
    return ERROR;
}

static void
assemble_free(struct assembler *a)
{
    Py_XDECREF(a->a_bytecode);
    Py_XDECREF(a->a_linetable);
    Py_XDECREF(a->a_except_table);
}

static inline void
write_except_byte(struct assembler *a, int byte) {
    unsigned char *p = (unsigned char *) PyBytes_AS_STRING(a->a_except_table);
    p[a->a_except_table_off++] = byte;
}

#define CONTINUATION_BIT 64

static void
assemble_emit_exception_table_item(struct assembler *a, int value, int msb)
{
    assert ((msb | 128) == 128);
    assert(value >= 0 && value < (1 << 30));
    if (value >= 1 << 24) {
        write_except_byte(a, (value >> 24) | CONTINUATION_BIT | msb);
        msb = 0;
    }
    if (value >= 1 << 18) {
        write_except_byte(a, ((value >> 18)&0x3f) | CONTINUATION_BIT | msb);
        msb = 0;
    }
    if (value >= 1 << 12) {
        write_except_byte(a, ((value >> 12)&0x3f) | CONTINUATION_BIT | msb);
        msb = 0;
    }
    if (value >= 1 << 6) {
        write_except_byte(a, ((value >> 6)&0x3f) | CONTINUATION_BIT | msb);
        msb = 0;
    }
    write_except_byte(a, (value&0x3f) | msb);
}

/* See Objects/exception_handling_notes.txt for details of layout */
#define MAX_SIZE_OF_ENTRY 20

static int
assemble_emit_exception_table_entry(struct assembler *a, int start, int end,
                                    _PyCompile_ExceptHandlerInfo *handler)
{
    Py_ssize_t len = PyBytes_GET_SIZE(a->a_except_table);
    if (a->a_except_table_off + MAX_SIZE_OF_ENTRY >= len) {
        RETURN_IF_ERROR(_PyBytes_Resize(&a->a_except_table, len * 2));
    }
    int size = end-start;
    assert(end > start);
    int target = handler->h_offset;
    int depth = handler->h_startdepth - 1;
    if (handler->h_preserve_lasti > 0) {
        depth -= 1;
    }
    assert(depth >= 0);
    int depth_lasti = (depth<<1) | handler->h_preserve_lasti;
    assemble_emit_exception_table_item(a, start, (1<<7));
    assemble_emit_exception_table_item(a, size, 0);
    assemble_emit_exception_table_item(a, target, 0);
    assemble_emit_exception_table_item(a, depth_lasti, 0);
    return SUCCESS;
}

static int
assemble_exception_table(struct assembler *a, instr_sequence *instrs)
{
    int ioffset = 0;
    _PyCompile_ExceptHandlerInfo handler;
    handler.h_offset = -1;
    handler.h_preserve_lasti = -1;
    int start = -1;
    for (int i = 0; i < instrs->s_used; i++) {
        instruction *instr = &instrs->s_instrs[i];
        if (instr->i_except_handler_info.h_offset != handler.h_offset) {
            if (handler.h_offset >= 0) {
                RETURN_IF_ERROR(
                    assemble_emit_exception_table_entry(a, start, ioffset, &handler));
            }
            start = ioffset;
            handler = instr->i_except_handler_info;
        }
        ioffset += _PyCompile_InstrSize(instr->i_opcode, instr->i_oparg);
    }
    if (handler.h_offset >= 0) {
        RETURN_IF_ERROR(assemble_emit_exception_table_entry(a, start, ioffset, &handler));
    }
    return SUCCESS;
}


/* Code location emitting code. See locations.md for a description of the format. */

#define MSB 0x80

static void
write_location_byte(struct assembler* a, int val)
{
    PyBytes_AS_STRING(a->a_linetable)[a->a_location_off] = val&255;
    a->a_location_off++;
}


static uint8_t *
location_pointer(struct assembler* a)
{
    return (uint8_t *)PyBytes_AS_STRING(a->a_linetable) +
        a->a_location_off;
}

static void
write_location_first_byte(struct assembler* a, int code, int length)
{
    a->a_location_off += write_location_entry_start(
        location_pointer(a), code, length);
}

static void
write_location_varint(struct assembler* a, unsigned int val)
{
    uint8_t *ptr = location_pointer(a);
    a->a_location_off += write_varint(ptr, val);
}


static void
write_location_signed_varint(struct assembler* a, int val)
{
    uint8_t *ptr = location_pointer(a);
    a->a_location_off += write_signed_varint(ptr, val);
}

static void
write_location_info_short_form(struct assembler* a, int length, int column, int end_column)
{
    assert(length > 0 &&  length <= 8);
    int column_low_bits = column & 7;
    int column_group = column >> 3;
    assert(column < 80);
    assert(end_column >= column);
    assert(end_column - column < 16);
    write_location_first_byte(a, PY_CODE_LOCATION_INFO_SHORT0 + column_group, length);
    write_location_byte(a, (column_low_bits << 4) | (end_column - column));
}

static void
write_location_info_oneline_form(struct assembler* a, int length, int line_delta, int column, int end_column)
{
    assert(length > 0 &&  length <= 8);
    assert(line_delta >= 0 && line_delta < 3);
    assert(column < 128);
    assert(end_column < 128);
    write_location_first_byte(a, PY_CODE_LOCATION_INFO_ONE_LINE0 + line_delta, length);
    write_location_byte(a, column);
    write_location_byte(a, end_column);
}

static void
write_location_info_long_form(struct assembler* a, location loc, int length)
{
    assert(length > 0 &&  length <= 8);
    write_location_first_byte(a, PY_CODE_LOCATION_INFO_LONG, length);
    write_location_signed_varint(a, loc.lineno - a->a_lineno);
    assert(loc.end_lineno >= loc.lineno);
    write_location_varint(a, loc.end_lineno - loc.lineno);
    write_location_varint(a, loc.col_offset + 1);
    write_location_varint(a, loc.end_col_offset + 1);
}

static void
write_location_info_none(struct assembler* a, int length)
{
    write_location_first_byte(a, PY_CODE_LOCATION_INFO_NONE, length);
}

static void
write_location_info_no_column(struct assembler* a, int length, int line_delta)
{
    write_location_first_byte(a, PY_CODE_LOCATION_INFO_NO_COLUMNS, length);
    write_location_signed_varint(a, line_delta);
}

#define THEORETICAL_MAX_ENTRY_SIZE 25 /* 1 + 6 + 6 + 6 + 6 */


static int
write_location_info_entry(struct assembler* a, location loc, int isize)
{
    Py_ssize_t len = PyBytes_GET_SIZE(a->a_linetable);
    if (a->a_location_off + THEORETICAL_MAX_ENTRY_SIZE >= len) {
        assert(len > THEORETICAL_MAX_ENTRY_SIZE);
        RETURN_IF_ERROR(_PyBytes_Resize(&a->a_linetable, len*2));
    }
    if (loc.lineno < 0) {
        write_location_info_none(a, isize);
        return SUCCESS;
    }
    int line_delta = loc.lineno - a->a_lineno;
    int column = loc.col_offset;
    int end_column = loc.end_col_offset;
    assert(column >= -1);
    assert(end_column >= -1);
    if (column < 0 || end_column < 0) {
        if (loc.end_lineno == loc.lineno || loc.end_lineno == -1) {
            write_location_info_no_column(a, isize, line_delta);
            a->a_lineno = loc.lineno;
            return SUCCESS;
        }
    }
    else if (loc.end_lineno == loc.lineno) {
        if (line_delta == 0 && column < 80 && end_column - column < 16 && end_column >= column) {
            write_location_info_short_form(a, isize, column, end_column);
            return SUCCESS;
        }
        if (line_delta >= 0 && line_delta < 3 && column < 128 && end_column < 128) {
            write_location_info_oneline_form(a, isize, line_delta, column, end_column);
            a->a_lineno = loc.lineno;
            return SUCCESS;
        }
    }
    write_location_info_long_form(a, loc, isize);
    a->a_lineno = loc.lineno;
    return SUCCESS;
}

static int
assemble_emit_location(struct assembler* a, location loc, int isize)
{
    if (isize == 0) {
        return SUCCESS;
    }
    while (isize > 8) {
        RETURN_IF_ERROR(write_location_info_entry(a, loc, 8));
        isize -= 8;
    }
    return write_location_info_entry(a, loc, isize);
}

static int
assemble_location_info(struct assembler *a, instr_sequence *instrs,
                       int firstlineno)
{
    a->a_lineno = firstlineno;
    location loc = NO_LOCATION;
    int size = 0;
    for (int i = 0; i < instrs->s_used; i++) {
        instruction *instr = &instrs->s_instrs[i];
        if (!same_location(loc, instr->i_loc)) {
                RETURN_IF_ERROR(assemble_emit_location(a, loc, size));
                loc = instr->i_loc;
                size = 0;
        }
        size += _PyCompile_InstrSize(instr->i_opcode, instr->i_oparg);
    }
    RETURN_IF_ERROR(assemble_emit_location(a, loc, size));
    return SUCCESS;
}

static void
write_instr(_Py_CODEUNIT *codestr, instruction *instr, int ilen)
{
    int opcode = instr->i_opcode;
    assert(!IS_PSEUDO_OPCODE(opcode));
    int oparg = instr->i_oparg;
    assert(HAS_ARG(opcode) || oparg == 0);
    int caches = _PyOpcode_Caches[opcode];
    switch (ilen - caches) {
        case 4:
            codestr->op.code = EXTENDED_ARG;
            codestr->op.arg = (oparg >> 24) & 0xFF;
            codestr++;
            /* fall through */
        case 3:
            codestr->op.code = EXTENDED_ARG;
            codestr->op.arg = (oparg >> 16) & 0xFF;
            codestr++;
            /* fall through */
        case 2:
            codestr->op.code = EXTENDED_ARG;
            codestr->op.arg = (oparg >> 8) & 0xFF;
            codestr++;
            /* fall through */
        case 1:
            codestr->op.code = opcode;
            codestr->op.arg = oparg & 0xFF;
            codestr++;
            break;
        default:
            Py_UNREACHABLE();
    }
    while (caches--) {
        codestr->op.code = CACHE;
        codestr->op.arg = 0;
        codestr++;
    }
}

/* assemble_emit_instr()
   Extend the bytecode with a new instruction.
   Update lnotab if necessary.
*/

static int
assemble_emit_instr(struct assembler *a, instruction *instr)
{
    Py_ssize_t len = PyBytes_GET_SIZE(a->a_bytecode);
    _Py_CODEUNIT *code;

    int size = _PyCompile_InstrSize(instr->i_opcode, instr->i_oparg);
    if (a->a_offset + size >= len / (int)sizeof(_Py_CODEUNIT)) {
        if (len > PY_SSIZE_T_MAX / 2) {
            return ERROR;
        }
        RETURN_IF_ERROR(_PyBytes_Resize(&a->a_bytecode, len * 2));
    }
    code = (_Py_CODEUNIT *)PyBytes_AS_STRING(a->a_bytecode) + a->a_offset;
    a->a_offset += size;
    write_instr(code, instr, size);
    return SUCCESS;
}

static int
assemble_emit(struct assembler *a, instr_sequence *instrs,
              int first_lineno, PyObject *const_cache)
{
    RETURN_IF_ERROR(assemble_init(a, first_lineno));

    for (int i = 0; i < instrs->s_used; i++) {
        instruction *instr = &instrs->s_instrs[i];
        RETURN_IF_ERROR(assemble_emit_instr(a, instr));
    }

    RETURN_IF_ERROR(assemble_location_info(a, instrs, a->a_lineno));

    RETURN_IF_ERROR(assemble_exception_table(a, instrs));

    RETURN_IF_ERROR(_PyBytes_Resize(&a->a_except_table, a->a_except_table_off));
    RETURN_IF_ERROR(_PyCompile_ConstCacheMergeOne(const_cache, &a->a_except_table));

    RETURN_IF_ERROR(_PyBytes_Resize(&a->a_linetable, a->a_location_off));
    RETURN_IF_ERROR(_PyCompile_ConstCacheMergeOne(const_cache, &a->a_linetable));

    RETURN_IF_ERROR(_PyBytes_Resize(&a->a_bytecode, a->a_offset * sizeof(_Py_CODEUNIT)));
    RETURN_IF_ERROR(_PyCompile_ConstCacheMergeOne(const_cache, &a->a_bytecode));
    return SUCCESS;
}

static PyObject *
dict_keys_inorder(PyObject *dict, Py_ssize_t offset)
{
    PyObject *tuple, *k, *v;
    Py_ssize_t i, pos = 0, size = PyDict_GET_SIZE(dict);

    tuple = PyTuple_New(size);
    if (tuple == NULL)
        return NULL;
    while (PyDict_Next(dict, &pos, &k, &v)) {
        i = PyLong_AS_LONG(v);
        assert((i - offset) < size);
        assert((i - offset) >= 0);
        PyTuple_SET_ITEM(tuple, i - offset, Py_NewRef(k));
    }
    return tuple;
}

// This is in codeobject.c.
extern void _Py_set_localsplus_info(int, PyObject *, unsigned char,
                                   PyObject *, PyObject *);

static void
compute_localsplus_info(_PyCompile_CodeUnitMetadata *umd, int nlocalsplus,
                        PyObject *names, PyObject *kinds)
{
    PyObject *k, *v;
    Py_ssize_t pos = 0;
    while (PyDict_Next(umd->u_varnames, &pos, &k, &v)) {
        int offset = (int)PyLong_AS_LONG(v);
        assert(offset >= 0);
        assert(offset < nlocalsplus);
        // For now we do not distinguish arg kinds.
        _PyLocals_Kind kind = CO_FAST_LOCAL;
        if (PyDict_Contains(umd->u_fasthidden, k)) {
            kind |= CO_FAST_HIDDEN;
        }
        if (PyDict_GetItem(umd->u_cellvars, k) != NULL) {
            kind |= CO_FAST_CELL;
        }
        _Py_set_localsplus_info(offset, k, kind, names, kinds);
    }
    int nlocals = (int)PyDict_GET_SIZE(umd->u_varnames);

    // This counter mirrors the fix done in fix_cell_offsets().
    int numdropped = 0;
    pos = 0;
    while (PyDict_Next(umd->u_cellvars, &pos, &k, &v)) {
        if (PyDict_GetItem(umd->u_varnames, k) != NULL) {
            // Skip cells that are already covered by locals.
            numdropped += 1;
            continue;
        }
        int offset = (int)PyLong_AS_LONG(v);
        assert(offset >= 0);
        offset += nlocals - numdropped;
        assert(offset < nlocalsplus);
        _Py_set_localsplus_info(offset, k, CO_FAST_CELL, names, kinds);
    }

    pos = 0;
    while (PyDict_Next(umd->u_freevars, &pos, &k, &v)) {
        int offset = (int)PyLong_AS_LONG(v);
        assert(offset >= 0);
        offset += nlocals - numdropped;
        assert(offset < nlocalsplus);
        _Py_set_localsplus_info(offset, k, CO_FAST_FREE, names, kinds);
    }
}

static PyCodeObject *
makecode(_PyCompile_CodeUnitMetadata *umd, struct assembler *a, PyObject *const_cache,
         PyObject *constslist, int maxdepth, int nlocalsplus, int code_flags,
         PyObject *filename)
{
    PyCodeObject *co = NULL;
    PyObject *names = NULL;
    PyObject *consts = NULL;
    PyObject *localsplusnames = NULL;
    PyObject *localspluskinds = NULL;
    names = dict_keys_inorder(umd->u_names, 0);
    if (!names) {
        goto error;
    }
    if (_PyCompile_ConstCacheMergeOne(const_cache, &names) < 0) {
        goto error;
    }

    consts = PyList_AsTuple(constslist); /* PyCode_New requires a tuple */
    if (consts == NULL) {
        goto error;
    }
    if (_PyCompile_ConstCacheMergeOne(const_cache, &consts) < 0) {
        goto error;
    }

    assert(umd->u_posonlyargcount < INT_MAX);
    assert(umd->u_argcount < INT_MAX);
    assert(umd->u_kwonlyargcount < INT_MAX);
    int posonlyargcount = (int)umd->u_posonlyargcount;
    int posorkwargcount = (int)umd->u_argcount;
    assert(INT_MAX - posonlyargcount - posorkwargcount > 0);
    int kwonlyargcount = (int)umd->u_kwonlyargcount;

    localsplusnames = PyTuple_New(nlocalsplus);
    if (localsplusnames == NULL) {
        goto error;
    }
    localspluskinds = PyBytes_FromStringAndSize(NULL, nlocalsplus);
    if (localspluskinds == NULL) {
        goto error;
    }
    compute_localsplus_info(umd, nlocalsplus, localsplusnames, localspluskinds);

    struct _PyCodeConstructor con = {
        .filename = filename,
        .name = umd->u_name,
        .qualname = umd->u_qualname ? umd->u_qualname : umd->u_name,
        .flags = code_flags,

        .code = a->a_bytecode,
        .firstlineno = umd->u_firstlineno,
        .linetable = a->a_linetable,

        .consts = consts,
        .names = names,

        .localsplusnames = localsplusnames,
        .localspluskinds = localspluskinds,

        .argcount = posonlyargcount + posorkwargcount,
        .posonlyargcount = posonlyargcount,
        .kwonlyargcount = kwonlyargcount,

        .stacksize = maxdepth,

        .exceptiontable = a->a_except_table,
    };

   if (_PyCode_Validate(&con) < 0) {
        goto error;
    }

    if (_PyCompile_ConstCacheMergeOne(const_cache, &localsplusnames) < 0) {
        goto error;
    }
    con.localsplusnames = localsplusnames;

    co = _PyCode_New(&con);
    if (co == NULL) {
        goto error;
    }

error:
    Py_XDECREF(names);
    Py_XDECREF(consts);
    Py_XDECREF(localsplusnames);
    Py_XDECREF(localspluskinds);
    return co;
}


PyCodeObject *
_PyAssemble_MakeCodeObject(_PyCompile_CodeUnitMetadata *umd, PyObject *const_cache,
                           PyObject *consts, int maxdepth, instr_sequence *instrs,
                           int nlocalsplus, int code_flags, PyObject *filename)
{
    PyCodeObject *co = NULL;

    struct assembler a;
    int res = assemble_emit(&a, instrs, umd->u_firstlineno, const_cache);
    if (res == SUCCESS) {
        co = makecode(umd, &a, const_cache, consts, maxdepth, nlocalsplus,
                      code_flags, filename);
    }
    assemble_free(&a);
    return co;
}
