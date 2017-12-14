/* Peephole optimizations for bytecode compiler. */

#include "Python.h"

#include "Python-ast.h"
#include "node.h"
#include "ast.h"
#include "code.h"
#include "symtable.h"
#include "opcode.h"
#include "wordcode_helpers.h"

#define UNCONDITIONAL_JUMP(op)  (op==JUMP_ABSOLUTE || op==JUMP_FORWARD)
#define CONDITIONAL_JUMP(op) (op==POP_JUMP_IF_FALSE || op==POP_JUMP_IF_TRUE \
    || op==JUMP_IF_FALSE_OR_POP || op==JUMP_IF_TRUE_OR_POP)
#define ABSOLUTE_JUMP(op) (op==JUMP_ABSOLUTE || op==CONTINUE_LOOP \
    || op==POP_JUMP_IF_FALSE || op==POP_JUMP_IF_TRUE \
    || op==JUMP_IF_FALSE_OR_POP || op==JUMP_IF_TRUE_OR_POP)
#define JUMPS_ON_TRUE(op) (op==POP_JUMP_IF_TRUE || op==JUMP_IF_TRUE_OR_POP)
#define GETJUMPTGT(arr, i) (get_arg(arr, i) / sizeof(_Py_CODEUNIT) + \
        (ABSOLUTE_JUMP(_Py_OPCODE(arr[i])) ? 0 : i+1))
#define ISBASICBLOCK(blocks, start, end) \
    (blocks[start]==blocks[end])


#define CONST_STACK_CREATE() { \
    const_stack_size = 256; \
    const_stack = PyMem_New(PyObject *, const_stack_size); \
    if (!const_stack) { \
        PyErr_NoMemory(); \
        goto exitError; \
    } \
    }

#define CONST_STACK_DELETE() do { \
    if (const_stack) \
        PyMem_Free(const_stack); \
    } while(0)

#define CONST_STACK_LEN() ((unsigned)(const_stack_top + 1))

#define CONST_STACK_PUSH_OP(i) do { \
    PyObject *_x; \
    assert(_Py_OPCODE(codestr[i]) == LOAD_CONST); \
    assert(PyList_GET_SIZE(consts) > (Py_ssize_t)get_arg(codestr, i)); \
    _x = PyList_GET_ITEM(consts, get_arg(codestr, i)); \
    if (++const_stack_top >= const_stack_size) { \
        const_stack_size *= 2; \
        PyMem_Resize(const_stack, PyObject *, const_stack_size); \
        if (!const_stack) { \
            PyErr_NoMemory(); \
            goto exitError; \
        } \
    } \
    const_stack[const_stack_top] = _x; \
    in_consts = 1; \
    } while(0)

#define CONST_STACK_RESET() do { \
    const_stack_top = -1; \
    } while(0)

#define CONST_STACK_LASTN(i) \
    &const_stack[CONST_STACK_LEN() - i]

#define CONST_STACK_POP(i) do { \
    assert(CONST_STACK_LEN() >= i); \
    const_stack_top -= i; \
    } while(0)

/* Scans back N consecutive LOAD_CONST instructions, skipping NOPs,
   returns index of the Nth last's LOAD_CONST's EXTENDED_ARG prefix.
   Callers are responsible to check CONST_STACK_LEN beforehand.
*/
static Py_ssize_t
lastn_const_start(const _Py_CODEUNIT *codestr, Py_ssize_t i, Py_ssize_t n)
{
    assert(n > 0);
    for (;;) {
        i--;
        assert(i >= 0);
        if (_Py_OPCODE(codestr[i]) == LOAD_CONST) {
            if (!--n) {
                while (i > 0 && _Py_OPCODE(codestr[i-1]) == EXTENDED_ARG) {
                    i--;
                }
                return i;
            }
        }
        else {
            assert(_Py_OPCODE(codestr[i]) == NOP ||
                   _Py_OPCODE(codestr[i]) == EXTENDED_ARG);
        }
    }
}

/* Scans through EXTENDED ARGs, seeking the index of the effective opcode */
static Py_ssize_t
find_op(const _Py_CODEUNIT *codestr, Py_ssize_t i)
{
    while (_Py_OPCODE(codestr[i]) == EXTENDED_ARG) {
        i++;
    }
    return i;
}

/* Given the index of the effective opcode,
   scan back to construct the oparg with EXTENDED_ARG */
static unsigned int
get_arg(const _Py_CODEUNIT *codestr, Py_ssize_t i)
{
    _Py_CODEUNIT word;
    unsigned int oparg = _Py_OPARG(codestr[i]);
    if (i >= 1 && _Py_OPCODE(word = codestr[i-1]) == EXTENDED_ARG) {
        oparg |= _Py_OPARG(word) << 8;
        if (i >= 2 && _Py_OPCODE(word = codestr[i-2]) == EXTENDED_ARG) {
            oparg |= _Py_OPARG(word) << 16;
            if (i >= 3 && _Py_OPCODE(word = codestr[i-3]) == EXTENDED_ARG) {
                oparg |= _Py_OPARG(word) << 24;
            }
        }
    }
    return oparg;
}

/* Fill the region with NOPs. */
static void
fill_nops(_Py_CODEUNIT *codestr, Py_ssize_t start, Py_ssize_t end)
{
    memset(codestr + start, NOP, (end - start) * sizeof(_Py_CODEUNIT));
}

/* Given the index of the effective opcode,
   attempt to replace the argument, taking into account EXTENDED_ARG.
   Returns -1 on failure, or the new op index on success */
static Py_ssize_t
set_arg(_Py_CODEUNIT *codestr, Py_ssize_t i, unsigned int oparg)
{
    unsigned int curarg = get_arg(codestr, i);
    int curilen, newilen;
    if (curarg == oparg)
        return i;
    curilen = instrsize(curarg);
    newilen = instrsize(oparg);
    if (curilen < newilen) {
        return -1;
    }

    write_op_arg(codestr + i + 1 - curilen, _Py_OPCODE(codestr[i]), oparg, newilen);
    fill_nops(codestr, i + 1 - curilen + newilen, i + 1);
    return i-curilen+newilen;
}

/* Attempt to write op/arg at end of specified region of memory.
   Preceding memory in the region is overwritten with NOPs.
   Returns -1 on failure, op index on success */
static Py_ssize_t
copy_op_arg(_Py_CODEUNIT *codestr, Py_ssize_t i, unsigned char op,
            unsigned int oparg, Py_ssize_t maxi)
{
    int ilen = instrsize(oparg);
    if (i + ilen > maxi) {
        return -1;
    }
    write_op_arg(codestr + maxi - ilen, op, oparg, ilen);
    fill_nops(codestr, i, maxi - ilen);
    return maxi - 1;
}

/* Check whether a collection doesn't containing too much items (including
   subcollections).  This protects from creating a constant that needs
   too much time for calculating a hash.
   "limit" is the maximal number of items.
   Returns the negative number if the total number of items exceeds the
   limit.  Otherwise returns the limit minus the total number of items.
*/

static Py_ssize_t
check_complexity(PyObject *obj, Py_ssize_t limit)
{
    if (PyTuple_Check(obj)) {
        Py_ssize_t i;
        limit -= PyTuple_GET_SIZE(obj);
        for (i = 0; limit >= 0 && i < PyTuple_GET_SIZE(obj); i++) {
            limit = check_complexity(PyTuple_GET_ITEM(obj, i), limit);
        }
        return limit;
    }
    else if (PyFrozenSet_Check(obj)) {
        Py_ssize_t i = 0;
        PyObject *item;
        Py_hash_t hash;
        limit -= PySet_GET_SIZE(obj);
        while (limit >= 0 && _PySet_NextEntry(obj, &i, &item, &hash)) {
            limit = check_complexity(item, limit);
        }
    }
    return limit;
}

/* Replace LOAD_CONST c1, LOAD_CONST c2 ... LOAD_CONST cn, BUILD_TUPLE n
   with    LOAD_CONST (c1, c2, ... cn).
   The consts table must still be in list form so that the
   new constant (c1, c2, ... cn) can be appended.
   Called with codestr pointing to the first LOAD_CONST.
   Bails out with no change if one or more of the LOAD_CONSTs is missing.
   Also works for BUILD_LIST and BUILT_SET when followed by an "in" or "not in"
   test; for BUILD_SET it assembles a frozenset rather than a tuple.
*/
static Py_ssize_t
fold_tuple_on_constants(_Py_CODEUNIT *codestr, Py_ssize_t c_start,
                        Py_ssize_t opcode_end, unsigned char opcode,
                        PyObject *consts, PyObject **objs, int n)
{
    PyObject *newconst, *constant;
    Py_ssize_t i, len_consts;

    /* Pre-conditions */
    assert(PyList_CheckExact(consts));

    /* Buildup new tuple of constants */
    newconst = PyTuple_New(n);
    if (newconst == NULL) {
        return -1;
    }
    for (i=0 ; i<n ; i++) {
        constant = objs[i];
        Py_INCREF(constant);
        PyTuple_SET_ITEM(newconst, i, constant);
    }

    /* If it's a BUILD_SET, use the PyTuple we just built to create a
       PyFrozenSet, and use that as the constant instead: */
    if (opcode == BUILD_SET) {
        Py_SETREF(newconst, PyFrozenSet_New(newconst));
        if (newconst == NULL) {
            return -1;
        }
    }

    /* Append folded constant onto consts */
    len_consts = PyList_GET_SIZE(consts);
    if (PyList_Append(consts, newconst)) {
        Py_DECREF(newconst);
        return -1;
    }
    Py_DECREF(newconst);

    return copy_op_arg(codestr, c_start, LOAD_CONST, len_consts, opcode_end);
}

#define MAX_INT_SIZE           128  /* bits */
#define MAX_COLLECTION_SIZE     20  /* items */
#define MAX_STR_SIZE            20  /* characters */
#define MAX_TOTAL_ITEMS       1024  /* including nested collections */

static PyObject *
safe_multiply(PyObject *v, PyObject *w)
{
    if (PyLong_Check(v) && PyLong_Check(w) && Py_SIZE(v) && Py_SIZE(w)) {
        size_t vbits = _PyLong_NumBits(v);
        size_t wbits = _PyLong_NumBits(w);
        if (vbits == (size_t)-1 || wbits == (size_t)-1) {
            return NULL;
        }
        if (vbits + wbits > MAX_INT_SIZE) {
            return NULL;
        }
    }
    else if (PyLong_Check(v) && (PyTuple_Check(w) || PyFrozenSet_Check(w))) {
        Py_ssize_t size = PyTuple_Check(w) ? PyTuple_GET_SIZE(w) :
                                             PySet_GET_SIZE(w);
        if (size) {
            long n = PyLong_AsLong(v);
            if (n < 0 || n > MAX_COLLECTION_SIZE / size) {
                return NULL;
            }
            if (n && check_complexity(w, MAX_TOTAL_ITEMS / n) < 0) {
                return NULL;
            }
        }
    }
    else if (PyLong_Check(v) && (PyUnicode_Check(w) || PyBytes_Check(w))) {
        Py_ssize_t size = PyUnicode_Check(w) ? PyUnicode_GET_LENGTH(w) :
                                               PyBytes_GET_SIZE(w);
        if (size) {
            long n = PyLong_AsLong(v);
            if (n < 0 || n > MAX_STR_SIZE / size) {
                return NULL;
            }
        }
    }
    else if (PyLong_Check(w) &&
             (PyTuple_Check(v) || PyFrozenSet_Check(v) ||
              PyUnicode_Check(v) || PyBytes_Check(v)))
    {
        return safe_multiply(w, v);
    }

    return PyNumber_Multiply(v, w);
}

static PyObject *
safe_power(PyObject *v, PyObject *w)
{
    if (PyLong_Check(v) && PyLong_Check(w) && Py_SIZE(v) && Py_SIZE(w) > 0) {
        size_t vbits = _PyLong_NumBits(v);
        size_t wbits = PyLong_AsSize_t(w);
        if (vbits == (size_t)-1 || wbits == (size_t)-1) {
            return NULL;
        }
        if (vbits > MAX_INT_SIZE / wbits) {
            return NULL;
        }
    }

    return PyNumber_Power(v, w, Py_None);
}

static PyObject *
safe_lshift(PyObject *v, PyObject *w)
{
    if (PyLong_Check(v) && PyLong_Check(w) && Py_SIZE(v) && Py_SIZE(w)) {
        size_t vbits = _PyLong_NumBits(v);
        size_t wbits = PyLong_AsSize_t(w);
        if (vbits == (size_t)-1 || wbits == (size_t)-1) {
            return NULL;
        }
        if (wbits > MAX_INT_SIZE || vbits > MAX_INT_SIZE - wbits) {
            return NULL;
        }
    }

    return PyNumber_Lshift(v, w);
}

static PyObject *
safe_mod(PyObject *v, PyObject *w)
{
    if (PyUnicode_Check(v) || PyBytes_Check(v)) {
        return NULL;
    }

    return PyNumber_Remainder(v, w);
}

/* Replace LOAD_CONST c1, LOAD_CONST c2, BINOP
   with    LOAD_CONST binop(c1,c2)
   The consts table must still be in list form so that the
   new constant can be appended.
   Called with codestr pointing to the BINOP.
   Abandons the transformation if the folding fails (i.e.  1+'a').
   If the new constant is a sequence, only folds when the size
   is below a threshold value.  That keeps pyc files from
   becoming large in the presence of code like:  (None,)*1000.
*/
static Py_ssize_t
fold_binops_on_constants(_Py_CODEUNIT *codestr, Py_ssize_t c_start,
                         Py_ssize_t opcode_end, unsigned char opcode,
                         PyObject *consts, PyObject **objs)
{
    PyObject *newconst, *v, *w;
    Py_ssize_t len_consts;

    /* Pre-conditions */
    assert(PyList_CheckExact(consts));
    len_consts = PyList_GET_SIZE(consts);

    /* Create new constant */
    v = objs[0];
    w = objs[1];
    switch (opcode) {
        case BINARY_POWER:
            newconst = safe_power(v, w);
            break;
        case BINARY_MULTIPLY:
            newconst = safe_multiply(v, w);
            break;
        case BINARY_TRUE_DIVIDE:
            newconst = PyNumber_TrueDivide(v, w);
            break;
        case BINARY_FLOOR_DIVIDE:
            newconst = PyNumber_FloorDivide(v, w);
            break;
        case BINARY_MODULO:
            newconst = safe_mod(v, w);
            break;
        case BINARY_ADD:
            newconst = PyNumber_Add(v, w);
            break;
        case BINARY_SUBTRACT:
            newconst = PyNumber_Subtract(v, w);
            break;
        case BINARY_SUBSCR:
            newconst = PyObject_GetItem(v, w);
            break;
        case BINARY_LSHIFT:
            newconst = safe_lshift(v, w);
            break;
        case BINARY_RSHIFT:
            newconst = PyNumber_Rshift(v, w);
            break;
        case BINARY_AND:
            newconst = PyNumber_And(v, w);
            break;
        case BINARY_XOR:
            newconst = PyNumber_Xor(v, w);
            break;
        case BINARY_OR:
            newconst = PyNumber_Or(v, w);
            break;
        default:
            /* Called with an unknown opcode */
            PyErr_Format(PyExc_SystemError,
                 "unexpected binary operation %d on a constant",
                     opcode);
            return -1;
    }
    if (newconst == NULL) {
        if(!PyErr_ExceptionMatches(PyExc_KeyboardInterrupt)) {
            PyErr_Clear();
        }
        return -1;
    }

    /* Append folded constant into consts table */
    if (PyList_Append(consts, newconst)) {
        Py_DECREF(newconst);
        return -1;
    }
    Py_DECREF(newconst);

    return copy_op_arg(codestr, c_start, LOAD_CONST, len_consts, opcode_end);
}

static Py_ssize_t
fold_unaryops_on_constants(_Py_CODEUNIT *codestr, Py_ssize_t c_start,
                           Py_ssize_t opcode_end, unsigned char opcode,
                           PyObject *consts, PyObject *v)
{
    PyObject *newconst;
    Py_ssize_t len_consts;

    /* Pre-conditions */
    assert(PyList_CheckExact(consts));
    len_consts = PyList_GET_SIZE(consts);

    /* Create new constant */
    switch (opcode) {
        case UNARY_NEGATIVE:
            newconst = PyNumber_Negative(v);
            break;
        case UNARY_INVERT:
            newconst = PyNumber_Invert(v);
            break;
        case UNARY_POSITIVE:
            newconst = PyNumber_Positive(v);
            break;
        default:
            /* Called with an unknown opcode */
            PyErr_Format(PyExc_SystemError,
                 "unexpected unary operation %d on a constant",
                     opcode);
            return -1;
    }
    if (newconst == NULL) {
        if(!PyErr_ExceptionMatches(PyExc_KeyboardInterrupt)) {
            PyErr_Clear();
        }
        return -1;
    }

    /* Append folded constant into consts table */
    if (PyList_Append(consts, newconst)) {
        Py_DECREF(newconst);
        PyErr_Clear();
        return -1;
    }
    Py_DECREF(newconst);

    return copy_op_arg(codestr, c_start, LOAD_CONST, len_consts, opcode_end);
}

static unsigned int *
markblocks(_Py_CODEUNIT *code, Py_ssize_t len)
{
    unsigned int *blocks = PyMem_New(unsigned int, len);
    int i, j, opcode, blockcnt = 0;

    if (blocks == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    memset(blocks, 0, len*sizeof(int));

    /* Mark labels in the first pass */
    for (i = 0; i < len; i++) {
        opcode = _Py_OPCODE(code[i]);
        switch (opcode) {
            case FOR_ITER:
            case JUMP_FORWARD:
            case JUMP_IF_FALSE_OR_POP:
            case JUMP_IF_TRUE_OR_POP:
            case POP_JUMP_IF_FALSE:
            case POP_JUMP_IF_TRUE:
            case JUMP_ABSOLUTE:
            case CONTINUE_LOOP:
            case SETUP_LOOP:
            case SETUP_EXCEPT:
            case SETUP_FINALLY:
            case SETUP_WITH:
            case SETUP_ASYNC_WITH:
                j = GETJUMPTGT(code, i);
                assert(j < len);
                blocks[j] = 1;
                break;
        }
    }
    /* Build block numbers in the second pass */
    for (i = 0; i < len; i++) {
        blockcnt += blocks[i];          /* increment blockcnt over labels */
        blocks[i] = blockcnt;
    }
    return blocks;
}

/* Perform basic peephole optimizations to components of a code object.
   The consts object should still be in list form to allow new constants
   to be appended.

   To keep the optimizer simple, it bails when the lineno table has complex
   encoding for gaps >= 255.

   Optimizations are restricted to simple transformations occurring within a
   single basic block.  All transformations keep the code size the same or
   smaller.  For those that reduce size, the gaps are initially filled with
   NOPs.  Later those NOPs are removed and the jump addresses retargeted in
   a single pass. */

PyObject *
PyCode_Optimize(PyObject *code, PyObject* consts, PyObject *names,
                PyObject *lnotab_obj)
{
    Py_ssize_t h, i, nexti, op_start, codelen, tgt;
    unsigned int j, nops;
    unsigned char opcode, nextop;
    _Py_CODEUNIT *codestr = NULL;
    unsigned char *lnotab;
    unsigned int cum_orig_offset, last_offset;
    Py_ssize_t tabsiz;
    PyObject **const_stack = NULL;
    Py_ssize_t const_stack_top = -1;
    Py_ssize_t const_stack_size = 0;
    int in_consts = 0;  /* whether we are in a LOAD_CONST sequence */
    unsigned int *blocks = NULL;

    /* Bail out if an exception is set */
    if (PyErr_Occurred())
        goto exitError;

    /* Bypass optimization when the lnotab table is too complex */
    assert(PyBytes_Check(lnotab_obj));
    lnotab = (unsigned char*)PyBytes_AS_STRING(lnotab_obj);
    tabsiz = PyBytes_GET_SIZE(lnotab_obj);
    assert(tabsiz == 0 || Py_REFCNT(lnotab_obj) == 1);
    if (memchr(lnotab, 255, tabsiz) != NULL) {
        /* 255 value are used for multibyte bytecode instructions */
        goto exitUnchanged;
    }
    /* Note: -128 and 127 special values for line number delta are ok,
       the peephole optimizer doesn't modify line numbers. */

    assert(PyBytes_Check(code));
    codelen = PyBytes_GET_SIZE(code);
    assert(codelen % sizeof(_Py_CODEUNIT) == 0);

    /* Make a modifiable copy of the code string */
    codestr = (_Py_CODEUNIT *)PyMem_Malloc(codelen);
    if (codestr == NULL) {
        PyErr_NoMemory();
        goto exitError;
    }
    memcpy(codestr, PyBytes_AS_STRING(code), codelen);
    codelen /= sizeof(_Py_CODEUNIT);

    blocks = markblocks(codestr, codelen);
    if (blocks == NULL)
        goto exitError;
    assert(PyList_Check(consts));

    CONST_STACK_CREATE();

    for (i=find_op(codestr, 0) ; i<codelen ; i=nexti) {
        opcode = _Py_OPCODE(codestr[i]);
        op_start = i;
        while (op_start >= 1 && _Py_OPCODE(codestr[op_start-1]) == EXTENDED_ARG) {
            op_start--;
        }

        nexti = i + 1;
        while (nexti < codelen && _Py_OPCODE(codestr[nexti]) == EXTENDED_ARG)
            nexti++;
        nextop = nexti < codelen ? _Py_OPCODE(codestr[nexti]) : 0;

        if (!in_consts) {
            CONST_STACK_RESET();
        }
        in_consts = 0;

        switch (opcode) {
            /* Replace UNARY_NOT POP_JUMP_IF_FALSE
               with    POP_JUMP_IF_TRUE */
            case UNARY_NOT:
                if (nextop != POP_JUMP_IF_FALSE
                    || !ISBASICBLOCK(blocks, op_start, i + 1))
                    break;
                fill_nops(codestr, op_start, i + 1);
                codestr[nexti] = PACKOPARG(POP_JUMP_IF_TRUE, _Py_OPARG(codestr[nexti]));
                break;

                /* not a is b -->  a is not b
                   not a in b -->  a not in b
                   not a is not b -->  a is b
                   not a not in b -->  a in b
                */
            case COMPARE_OP:
                j = get_arg(codestr, i);
                if (j < 6 || j > 9 ||
                    nextop != UNARY_NOT ||
                    !ISBASICBLOCK(blocks, op_start, i + 1))
                    break;
                codestr[i] = PACKOPARG(opcode, j^1);
                fill_nops(codestr, i + 1, nexti + 1);
                break;

                /* Skip over LOAD_CONST trueconst
                   POP_JUMP_IF_FALSE xx.  This improves
                   "while 1" performance.  */
            case LOAD_CONST:
                CONST_STACK_PUSH_OP(i);
                if (nextop != POP_JUMP_IF_FALSE  ||
                    !ISBASICBLOCK(blocks, op_start, i + 1)  ||
                    !PyObject_IsTrue(PyList_GET_ITEM(consts, get_arg(codestr, i))))
                    break;
                fill_nops(codestr, op_start, nexti + 1);
                CONST_STACK_POP(1);
                break;

                /* Try to fold tuples of constants (includes a case for lists
                   and sets which are only used for "in" and "not in" tests).
                   Skip over BUILD_SEQN 1 UNPACK_SEQN 1.
                   Replace BUILD_SEQN 2 UNPACK_SEQN 2 with ROT2.
                   Replace BUILD_SEQN 3 UNPACK_SEQN 3 with ROT3 ROT2. */
            case BUILD_TUPLE:
            case BUILD_LIST:
            case BUILD_SET:
                j = get_arg(codestr, i);
                if (j > 0 && CONST_STACK_LEN() >= j) {
                    h = lastn_const_start(codestr, op_start, j);
                    if ((opcode == BUILD_TUPLE &&
                          ISBASICBLOCK(blocks, h, op_start)) ||
                         ((opcode == BUILD_LIST || opcode == BUILD_SET) &&
                          ((nextop==COMPARE_OP &&
                          (_Py_OPARG(codestr[nexti]) == PyCmp_IN ||
                           _Py_OPARG(codestr[nexti]) == PyCmp_NOT_IN)) ||
                          nextop == GET_ITER) && ISBASICBLOCK(blocks, h, i + 1))) {
                        h = fold_tuple_on_constants(codestr, h, i + 1, opcode,
                                                    consts, CONST_STACK_LASTN(j), j);
                        if (h >= 0) {
                            CONST_STACK_POP(j);
                            CONST_STACK_PUSH_OP(h);
                        }
                        break;
                    }
                }
                if (nextop != UNPACK_SEQUENCE  ||
                    !ISBASICBLOCK(blocks, op_start, i + 1) ||
                    j != get_arg(codestr, nexti) ||
                    opcode == BUILD_SET)
                    break;
                if (j < 2) {
                    fill_nops(codestr, op_start, nexti + 1);
                } else if (j == 2) {
                    codestr[op_start] = PACKOPARG(ROT_TWO, 0);
                    fill_nops(codestr, op_start + 1, nexti + 1);
                    CONST_STACK_RESET();
                } else if (j == 3) {
                    codestr[op_start] = PACKOPARG(ROT_THREE, 0);
                    codestr[op_start + 1] = PACKOPARG(ROT_TWO, 0);
                    fill_nops(codestr, op_start + 2, nexti + 1);
                    CONST_STACK_RESET();
                }
                break;

                /* Fold binary ops on constants.
                   LOAD_CONST c1 LOAD_CONST c2 BINOP --> LOAD_CONST binop(c1,c2) */
            case BINARY_POWER:
            case BINARY_MULTIPLY:
            case BINARY_TRUE_DIVIDE:
            case BINARY_FLOOR_DIVIDE:
            case BINARY_MODULO:
            case BINARY_ADD:
            case BINARY_SUBTRACT:
            case BINARY_SUBSCR:
            case BINARY_LSHIFT:
            case BINARY_RSHIFT:
            case BINARY_AND:
            case BINARY_XOR:
            case BINARY_OR:
                if (CONST_STACK_LEN() < 2)
                    break;
                h = lastn_const_start(codestr, op_start, 2);
                if (ISBASICBLOCK(blocks, h, op_start)) {
                    h = fold_binops_on_constants(codestr, h, i + 1, opcode,
                                                 consts, CONST_STACK_LASTN(2));
                    if (h >= 0) {
                        CONST_STACK_POP(2);
                        CONST_STACK_PUSH_OP(h);
                    }
                }
                break;

                /* Fold unary ops on constants.
                   LOAD_CONST c1  UNARY_OP --> LOAD_CONST unary_op(c) */
            case UNARY_NEGATIVE:
            case UNARY_INVERT:
            case UNARY_POSITIVE:
                if (CONST_STACK_LEN() < 1)
                    break;
                h = lastn_const_start(codestr, op_start, 1);
                if (ISBASICBLOCK(blocks, h, op_start)) {
                    h = fold_unaryops_on_constants(codestr, h, i + 1, opcode,
                                                   consts, *CONST_STACK_LASTN(1));
                    if (h >= 0) {
                        CONST_STACK_POP(1);
                        CONST_STACK_PUSH_OP(h);
                    }
                }
                break;

                /* Simplify conditional jump to conditional jump where the
                   result of the first test implies the success of a similar
                   test or the failure of the opposite test.
                   Arises in code like:
                   "if a and b:"
                   "if a or b:"
                   "a and b or c"
                   "(a and b) and c"
                   x:JUMP_IF_FALSE_OR_POP y   y:JUMP_IF_FALSE_OR_POP z
                      -->  x:JUMP_IF_FALSE_OR_POP z
                   x:JUMP_IF_FALSE_OR_POP y   y:JUMP_IF_TRUE_OR_POP z
                      -->  x:POP_JUMP_IF_FALSE y+1
                   where y+1 is the instruction following the second test.
                */
            case JUMP_IF_FALSE_OR_POP:
            case JUMP_IF_TRUE_OR_POP:
                h = get_arg(codestr, i) / sizeof(_Py_CODEUNIT);
                tgt = find_op(codestr, h);

                j = _Py_OPCODE(codestr[tgt]);
                if (CONDITIONAL_JUMP(j)) {
                    /* NOTE: all possible jumps here are absolute. */
                    if (JUMPS_ON_TRUE(j) == JUMPS_ON_TRUE(opcode)) {
                        /* The second jump will be taken iff the first is.
                           The current opcode inherits its target's
                           stack effect */
                        h = set_arg(codestr, i, get_arg(codestr, tgt));
                    } else {
                        /* The second jump is not taken if the first is (so
                           jump past it), and all conditional jumps pop their
                           argument when they're not taken (so change the
                           first jump to pop its argument when it's taken). */
                        h = set_arg(codestr, i, (tgt + 1) * sizeof(_Py_CODEUNIT));
                        j = opcode == JUMP_IF_TRUE_OR_POP ?
                            POP_JUMP_IF_TRUE : POP_JUMP_IF_FALSE;
                    }

                    if (h >= 0) {
                        nexti = h;
                        codestr[nexti] = PACKOPARG(j, _Py_OPARG(codestr[nexti]));
                        break;
                    }
                }
                /* Intentional fallthrough */

                /* Replace jumps to unconditional jumps */
            case POP_JUMP_IF_FALSE:
            case POP_JUMP_IF_TRUE:
            case FOR_ITER:
            case JUMP_FORWARD:
            case JUMP_ABSOLUTE:
            case CONTINUE_LOOP:
            case SETUP_LOOP:
            case SETUP_EXCEPT:
            case SETUP_FINALLY:
            case SETUP_WITH:
            case SETUP_ASYNC_WITH:
                h = GETJUMPTGT(codestr, i);
                tgt = find_op(codestr, h);
                /* Replace JUMP_* to a RETURN into just a RETURN */
                if (UNCONDITIONAL_JUMP(opcode) &&
                    _Py_OPCODE(codestr[tgt]) == RETURN_VALUE) {
                    codestr[op_start] = PACKOPARG(RETURN_VALUE, 0);
                    fill_nops(codestr, op_start + 1, i + 1);
                } else if (UNCONDITIONAL_JUMP(_Py_OPCODE(codestr[tgt]))) {
                    j = GETJUMPTGT(codestr, tgt);
                    if (opcode == JUMP_FORWARD) { /* JMP_ABS can go backwards */
                        opcode = JUMP_ABSOLUTE;
                    } else if (!ABSOLUTE_JUMP(opcode)) {
                        if ((Py_ssize_t)j < i + 1) {
                            break;           /* No backward relative jumps */
                        }
                        j -= i + 1;          /* Calc relative jump addr */
                    }
                    j *= sizeof(_Py_CODEUNIT);
                    copy_op_arg(codestr, op_start, opcode, j, i + 1);
                }
                break;

                /* Remove unreachable ops after RETURN */
            case RETURN_VALUE:
                h = i + 1;
                while (h < codelen && ISBASICBLOCK(blocks, i, h)) {
                    h++;
                }
                if (h > i + 1) {
                    fill_nops(codestr, i + 1, h);
                    nexti = find_op(codestr, h);
                }
                break;
        }
    }

    /* Fixup lnotab */
    for (i = 0, nops = 0; i < codelen; i++) {
        assert(i - nops <= INT_MAX);
        /* original code offset => new code offset */
        blocks[i] = i - nops;
        if (_Py_OPCODE(codestr[i]) == NOP)
            nops++;
    }
    cum_orig_offset = 0;
    last_offset = 0;
    for (i=0 ; i < tabsiz ; i+=2) {
        unsigned int offset_delta, new_offset;
        cum_orig_offset += lnotab[i];
        assert(cum_orig_offset % sizeof(_Py_CODEUNIT) == 0);
        new_offset = blocks[cum_orig_offset / sizeof(_Py_CODEUNIT)] *
                sizeof(_Py_CODEUNIT);
        offset_delta = new_offset - last_offset;
        assert(offset_delta <= 255);
        lnotab[i] = (unsigned char)offset_delta;
        last_offset = new_offset;
    }

    /* Remove NOPs and fixup jump targets */
    for (op_start = i = h = 0; i < codelen; i++, op_start = i) {
        j = _Py_OPARG(codestr[i]);
        while (_Py_OPCODE(codestr[i]) == EXTENDED_ARG) {
            i++;
            j = j<<8 | _Py_OPARG(codestr[i]);
        }
        opcode = _Py_OPCODE(codestr[i]);
        switch (opcode) {
            case NOP:continue;

            case JUMP_ABSOLUTE:
            case CONTINUE_LOOP:
            case POP_JUMP_IF_FALSE:
            case POP_JUMP_IF_TRUE:
            case JUMP_IF_FALSE_OR_POP:
            case JUMP_IF_TRUE_OR_POP:
                j = blocks[j / sizeof(_Py_CODEUNIT)] * sizeof(_Py_CODEUNIT);
                break;

            case FOR_ITER:
            case JUMP_FORWARD:
            case SETUP_LOOP:
            case SETUP_EXCEPT:
            case SETUP_FINALLY:
            case SETUP_WITH:
            case SETUP_ASYNC_WITH:
                j = blocks[j / sizeof(_Py_CODEUNIT) + i + 1] - blocks[i] - 1;
                j *= sizeof(_Py_CODEUNIT);
                break;
        }
        nexti = i - op_start + 1;
        if (instrsize(j) > nexti)
            goto exitUnchanged;
        /* If instrsize(j) < nexti, we'll emit EXTENDED_ARG 0 */
        write_op_arg(codestr + h, opcode, j, nexti);
        h += nexti;
    }
    assert(h + (Py_ssize_t)nops == codelen);

    CONST_STACK_DELETE();
    PyMem_Free(blocks);
    code = PyBytes_FromStringAndSize((char *)codestr, h * sizeof(_Py_CODEUNIT));
    PyMem_Free(codestr);
    return code;

 exitError:
    code = NULL;

 exitUnchanged:
    Py_XINCREF(code);
    CONST_STACK_DELETE();
    PyMem_Free(blocks);
    PyMem_Free(codestr);
    return code;
}
