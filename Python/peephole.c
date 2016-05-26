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
#define GETJUMPTGT(arr, i) (get_arg(arr, i) + (ABSOLUTE_JUMP(arr[i]) ? 0 : i+2))
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
    assert(codestr[i] == LOAD_CONST); \
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
lastn_const_start(unsigned char *codestr, Py_ssize_t i, Py_ssize_t n)
{
    assert(n > 0 && (i&1) == 0);
    for (;;) {
        i -= 2;
        assert(i >= 0);
        if (codestr[i] == LOAD_CONST) {
            if (!--n) {
                while (i > 0 && codestr[i-2] == EXTENDED_ARG) {
                    i -= 2;
                }
                return i;
            }
        }
        else {
            assert(codestr[i] == NOP || codestr[i] == EXTENDED_ARG);
        }
    }
}

/* Scans through EXTENDED ARGs, seeking the index of the effective opcode */
static Py_ssize_t
find_op(unsigned char *codestr, Py_ssize_t i)
{
    assert((i&1) == 0);
    while (codestr[i] == EXTENDED_ARG) {
        i += 2;
    }
    return i;
}

/* Given the index of the effective opcode,
   scan back to construct the oparg with EXTENDED_ARG */
static unsigned int
get_arg(unsigned char *codestr, Py_ssize_t i)
{
    unsigned int oparg = codestr[i+1];
    assert((i&1) == 0);
    if (i >= 2 && codestr[i-2] == EXTENDED_ARG) {
        oparg |= codestr[i-1] << 8;
        if (i >= 4 && codestr[i-4] == EXTENDED_ARG) {
            oparg |= codestr[i-3] << 16;
            if (i >= 6 && codestr[i-6] == EXTENDED_ARG) {
                oparg |= codestr[i-5] << 24;
            }
        }
    }
    return oparg;
}

/* Given the index of the effective opcode,
   attempt to replace the argument, taking into account EXTENDED_ARG.
   Returns -1 on failure, or the new op index on success */
static Py_ssize_t
set_arg(unsigned char *codestr, Py_ssize_t i, unsigned int oparg)
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

    write_op_arg(codestr + i + 2 - curilen, codestr[i], oparg, newilen);
    memset(codestr + i + 2 - curilen + newilen, NOP, curilen - newilen);
    return i-curilen+newilen;
}

/* Attempt to write op/arg at end of specified region of memory.
   Preceding memory in the region is overwritten with NOPs.
   Returns -1 on failure, op index on success */
static Py_ssize_t
copy_op_arg(unsigned char *codestr, Py_ssize_t i, unsigned char op,
            unsigned int oparg, Py_ssize_t maxi)
{
    int ilen = instrsize(oparg);
    assert((i&1) == 0);
    if (i + ilen > maxi) {
        return -1;
    }
    write_op_arg(codestr + maxi - ilen, op, oparg, ilen);
    memset(codestr + i, NOP, maxi - i - ilen);
    return maxi - 2;
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
fold_tuple_on_constants(unsigned char *codestr, Py_ssize_t c_start,
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
fold_binops_on_constants(unsigned char *codestr, Py_ssize_t c_start,
                         Py_ssize_t opcode_end, unsigned char opcode,
                         PyObject *consts, PyObject **objs)
{
    PyObject *newconst, *v, *w;
    Py_ssize_t len_consts, size;

    /* Pre-conditions */
    assert(PyList_CheckExact(consts));
    len_consts = PyList_GET_SIZE(consts);

    /* Create new constant */
    v = objs[0];
    w = objs[1];
    switch (opcode) {
        case BINARY_POWER:
            newconst = PyNumber_Power(v, w, Py_None);
            break;
        case BINARY_MULTIPLY:
            newconst = PyNumber_Multiply(v, w);
            break;
        case BINARY_TRUE_DIVIDE:
            newconst = PyNumber_TrueDivide(v, w);
            break;
        case BINARY_FLOOR_DIVIDE:
            newconst = PyNumber_FloorDivide(v, w);
            break;
        case BINARY_MODULO:
            newconst = PyNumber_Remainder(v, w);
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
            newconst = PyNumber_Lshift(v, w);
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
    size = PyObject_Size(newconst);
    if (size == -1) {
        if (PyErr_ExceptionMatches(PyExc_KeyboardInterrupt)) {
            return -1;
        }
        PyErr_Clear();
    } else if (size > 20) {
        Py_DECREF(newconst);
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
fold_unaryops_on_constants(unsigned char *codestr, Py_ssize_t c_start,
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
markblocks(unsigned char *code, Py_ssize_t len)
{
    unsigned int *blocks = PyMem_New(unsigned int, len);
    int i, j, opcode, blockcnt = 0;

    if (blocks == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    memset(blocks, 0, len*sizeof(int));

    /* Mark labels in the first pass */
    for (i=0 ; i<len ; i+=2) {
        opcode = code[i];
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
                blocks[j] = 1;
                break;
        }
    }
    /* Build block numbers in the second pass */
    for (i=0 ; i<len ; i+=2) {
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
    unsigned char *codestr = NULL;
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
    assert(codelen % 2 == 0);

    /* Make a modifiable copy of the code string */
    codestr = (unsigned char *)PyMem_Malloc(codelen);
    if (codestr == NULL) {
        PyErr_NoMemory();
        goto exitError;
    }
    codestr = (unsigned char *)memcpy(codestr,
                                      PyBytes_AS_STRING(code), codelen);

    blocks = markblocks(codestr, codelen);
    if (blocks == NULL)
        goto exitError;
    assert(PyList_Check(consts));

    CONST_STACK_CREATE();

    for (i=find_op(codestr, 0) ; i<codelen ; i=nexti) {
        opcode = codestr[i];
        op_start = i;
        while (op_start >= 2 && codestr[op_start-2] == EXTENDED_ARG) {
            op_start -= 2;
        }

        nexti = i + 2;
        while (nexti < codelen && codestr[nexti] == EXTENDED_ARG)
            nexti += 2;
        nextop = nexti < codelen ? codestr[nexti] : 0;

        if (!in_consts) {
            CONST_STACK_RESET();
        }
        in_consts = 0;

        switch (opcode) {
            /* Replace UNARY_NOT POP_JUMP_IF_FALSE
               with    POP_JUMP_IF_TRUE */
            case UNARY_NOT:
                if (nextop != POP_JUMP_IF_FALSE
                    || !ISBASICBLOCK(blocks, op_start, i+2))
                    break;
                memset(codestr + op_start, NOP, i - op_start + 2);
                codestr[nexti] = POP_JUMP_IF_TRUE;
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
                    !ISBASICBLOCK(blocks, op_start, i + 2))
                    break;
                codestr[i+1] = (j^1);
                memset(codestr + i + 2, NOP, nexti - i);
                break;

                /* Skip over LOAD_CONST trueconst
                   POP_JUMP_IF_FALSE xx.  This improves
                   "while 1" performance.  */
            case LOAD_CONST:
                CONST_STACK_PUSH_OP(i);
                if (nextop != POP_JUMP_IF_FALSE  ||
                    !ISBASICBLOCK(blocks, op_start, i + 2)  ||
                    !PyObject_IsTrue(PyList_GET_ITEM(consts, get_arg(codestr, i))))
                    break;
                memset(codestr + op_start, NOP, nexti - op_start + 2);
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
                          (codestr[nexti+1]==6 ||
                           codestr[nexti+1]==7)) ||
                          nextop == GET_ITER) && ISBASICBLOCK(blocks, h, i + 2))) {
                        h = fold_tuple_on_constants(codestr, h, i+2, opcode,
                                                    consts, CONST_STACK_LASTN(j), j);
                        if (h >= 0) {
                            CONST_STACK_POP(j);
                            CONST_STACK_PUSH_OP(h);
                        }
                        break;
                    }
                }
                if (nextop != UNPACK_SEQUENCE  ||
                    !ISBASICBLOCK(blocks, op_start, i + 2) ||
                    j != get_arg(codestr, nexti) ||
                    opcode == BUILD_SET)
                    break;
                if (j < 2) {
                    memset(codestr+op_start, NOP, nexti - op_start + 2);
                } else if (j == 2) {
                    codestr[op_start] = ROT_TWO;
                    codestr[op_start + 1] = 0;
                    memset(codestr + op_start + 2, NOP, nexti - op_start);
                    CONST_STACK_RESET();
                } else if (j == 3) {
                    codestr[op_start] = ROT_THREE;
                    codestr[op_start + 1] = 0;
                    codestr[op_start + 2] = ROT_TWO;
                    codestr[op_start + 3] = 0;
                    memset(codestr + op_start + 4, NOP, nexti - op_start - 2);
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
                    h = fold_binops_on_constants(codestr, h, i+2, opcode,
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
                    h = fold_unaryops_on_constants(codestr, h, i+2, opcode,
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
                      -->  x:POP_JUMP_IF_FALSE y+2
                   where y+2 is the instruction following the second test.
                */
            case JUMP_IF_FALSE_OR_POP:
            case JUMP_IF_TRUE_OR_POP:
                h = get_arg(codestr, i);
                tgt = find_op(codestr, h);

                j = codestr[tgt];
                if (CONDITIONAL_JUMP(j)) {
                    /* NOTE: all possible jumps here are
                       absolute! */
                    if (JUMPS_ON_TRUE(j) == JUMPS_ON_TRUE(opcode)) {
                        /* The second jump will be
                           taken iff the first is.
                           The current opcode inherits
                           its target's stack effect */
                        h = set_arg(codestr, i, get_arg(codestr, tgt));
                    } else {
                        /* The second jump is not taken
                           if the first is (so jump past
                           it), and all conditional
                           jumps pop their argument when
                           they're not taken (so change
                           the first jump to pop its
                           argument when it's taken). */
                        h = set_arg(codestr, i, tgt + 2);
                        j = opcode == JUMP_IF_TRUE_OR_POP ?
                            POP_JUMP_IF_TRUE : POP_JUMP_IF_FALSE;
                    }

                    if (h >= 0) {
                        nexti = h;
                        codestr[nexti] = j;
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
                    codestr[tgt] == RETURN_VALUE) {
                    codestr[op_start] = RETURN_VALUE;
                    codestr[op_start + 1] = 0;
                    memset(codestr + op_start + 2, NOP, i - op_start);
                } else if (UNCONDITIONAL_JUMP(codestr[tgt])) {
                    j = GETJUMPTGT(codestr, tgt);
                    if (opcode == JUMP_FORWARD) { /* JMP_ABS can go backwards */
                        opcode = JUMP_ABSOLUTE;
                    } else if (!ABSOLUTE_JUMP(opcode)) {
                        if ((Py_ssize_t)j < i + 2) {
                            break;           /* No backward relative jumps */
                        }
                        j -= i + 2;          /* Calc relative jump addr */
                    }
                    copy_op_arg(codestr, op_start, opcode, j, i+2);
                }
                break;

                /* Remove unreachable ops after RETURN */
            case RETURN_VALUE:
                h = i + 2;
                while (h + 2 < codelen && ISBASICBLOCK(blocks, i, h + 2)) {
                    h += 2;
                }
                if (h > i + 2) {
                    memset(codestr + i + 2, NOP, h - i);
                    nexti = find_op(codestr, h);
                }
                break;
        }
    }

    /* Fixup lnotab */
    for (i=0, nops=0 ; i<codelen ; i += 2) {
        assert(i - nops <= INT_MAX);
        /* original code offset => new code offset */
        blocks[i] = i - nops;
        if (codestr[i] == NOP)
            nops += 2;
    }
    cum_orig_offset = 0;
    last_offset = 0;
    for (i=0 ; i < tabsiz ; i+=2) {
        unsigned int offset_delta, new_offset;
        cum_orig_offset += lnotab[i];
        assert((cum_orig_offset & 1) == 0);
        new_offset = blocks[cum_orig_offset];
        offset_delta = new_offset - last_offset;
        assert(offset_delta <= 255);
        lnotab[i] = (unsigned char)offset_delta;
        last_offset = new_offset;
    }

    /* Remove NOPs and fixup jump targets */
    for (op_start=0, i=0, h=0 ; i<codelen ; i+=2, op_start=i) {
        j = codestr[i+1];
        while (codestr[i] == EXTENDED_ARG) {
            i += 2;
            j = j<<8 | codestr[i+1];
        }
        opcode = codestr[i];
        switch (opcode) {
            case NOP:continue;

            case JUMP_ABSOLUTE:
            case CONTINUE_LOOP:
            case POP_JUMP_IF_FALSE:
            case POP_JUMP_IF_TRUE:
            case JUMP_IF_FALSE_OR_POP:
            case JUMP_IF_TRUE_OR_POP:
                j = blocks[j];
                break;

            case FOR_ITER:
            case JUMP_FORWARD:
            case SETUP_LOOP:
            case SETUP_EXCEPT:
            case SETUP_FINALLY:
            case SETUP_WITH:
            case SETUP_ASYNC_WITH:
                j = blocks[j + i + 2] - blocks[i] - 2;
                break;
        }
        nexti = i - op_start + 2;
        if (instrsize(j) > nexti)
            goto exitUnchanged;
        /* If instrsize(j) < nexti, we'll emit EXTENDED_ARG 0 */
        write_op_arg(codestr + h, opcode, j, nexti);
        h += nexti;
    }
    assert(h + (Py_ssize_t)nops == codelen);

    CONST_STACK_DELETE();
    PyMem_Free(blocks);
    code = PyBytes_FromStringAndSize((char *)codestr, h);
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
