/* Peephole optimizations for bytecode compiler. */

#include "Python.h"

#include "Python-ast.h"
#include "node.h"
#include "ast.h"
#include "code.h"
#include "symtable.h"
#include "opcode.h"

#define GETARG(arr, i) ((int)((arr[i+2]<<8) + arr[i+1]))
#define UNCONDITIONAL_JUMP(op)  (op==JUMP_ABSOLUTE || op==JUMP_FORWARD)
#define CONDITIONAL_JUMP(op) (op==POP_JUMP_IF_FALSE || op==POP_JUMP_IF_TRUE \
    || op==JUMP_IF_FALSE_OR_POP || op==JUMP_IF_TRUE_OR_POP)
#define ABSOLUTE_JUMP(op) (op==JUMP_ABSOLUTE || op==CONTINUE_LOOP \
    || op==POP_JUMP_IF_FALSE || op==POP_JUMP_IF_TRUE \
    || op==JUMP_IF_FALSE_OR_POP || op==JUMP_IF_TRUE_OR_POP)
#define JUMPS_ON_TRUE(op) (op==POP_JUMP_IF_TRUE || op==JUMP_IF_TRUE_OR_POP)
#define GETJUMPTGT(arr, i) (GETARG(arr,i) + (ABSOLUTE_JUMP(arr[i]) ? 0 : i+3))
#define SETARG(arr, i, val) do {                            \
    assert(0 <= val && val <= 0xffff);                      \
    arr[i+2] = (unsigned char)(((unsigned int)val)>>8);     \
    arr[i+1] = (unsigned char)(((unsigned int)val) & 255);  \
} while(0)
#define CODESIZE(op)  (HAS_ARG(op) ? 3 : 1)
#define ISBASICBLOCK(blocks, start, bytes) \
    (blocks[start]==blocks[start+bytes-1])


#define CONST_STACK_CREATE() { \
    const_stack_size = 256; \
    const_stack = PyMem_New(PyObject *, const_stack_size); \
    load_const_stack = PyMem_New(Py_ssize_t, const_stack_size); \
    if (!const_stack || !load_const_stack) { \
        PyErr_NoMemory(); \
        goto exitError; \
    } \
    }

#define CONST_STACK_DELETE() do { \
    if (const_stack) \
        PyMem_Free(const_stack); \
    if (load_const_stack) \
        PyMem_Free(load_const_stack); \
    } while(0)

#define CONST_STACK_LEN() (const_stack_top + 1)

#define CONST_STACK_PUSH_OP(i) do { \
    PyObject *_x; \
    assert(codestr[i] == LOAD_CONST); \
    assert(PyList_GET_SIZE(consts) > GETARG(codestr, i)); \
    _x = PyList_GET_ITEM(consts, GETARG(codestr, i)); \
    if (++const_stack_top >= const_stack_size) { \
        const_stack_size *= 2; \
        PyMem_Resize(const_stack, PyObject *, const_stack_size); \
        PyMem_Resize(load_const_stack, Py_ssize_t, const_stack_size); \
        if (!const_stack || !load_const_stack) { \
            PyErr_NoMemory(); \
            goto exitError; \
        } \
    } \
    load_const_stack[const_stack_top] = i; \
    const_stack[const_stack_top] = _x; \
    in_consts = 1; \
    } while(0)

#define CONST_STACK_RESET() do { \
    const_stack_top = -1; \
    } while(0)

#define CONST_STACK_TOP() \
    const_stack[const_stack_top]

#define CONST_STACK_LASTN(i) \
    &const_stack[const_stack_top - i + 1]

#define CONST_STACK_POP(i) do { \
    assert(const_stack_top + 1 >= i); \
    const_stack_top -= i; \
    } while(0)

#define CONST_STACK_OP_LASTN(i) \
    ((const_stack_top >= i - 1) ? load_const_stack[const_stack_top - i + 1] : -1)


/* Replace LOAD_CONST c1. LOAD_CONST c2 ... LOAD_CONST cn BUILD_TUPLE n
   with    LOAD_CONST (c1, c2, ... cn).
   The consts table must still be in list form so that the
   new constant (c1, c2, ... cn) can be appended.
   Called with codestr pointing to the first LOAD_CONST.
   Bails out with no change if one or more of the LOAD_CONSTs is missing.
   Also works for BUILD_LIST and BUILT_SET when followed by an "in" or "not in"
   test; for BUILD_SET it assembles a frozenset rather than a tuple.
*/
static int
tuple_of_constants(unsigned char *codestr, Py_ssize_t n,
                   PyObject *consts, PyObject **objs)
{
    PyObject *newconst, *constant;
    Py_ssize_t i, len_consts;

    /* Pre-conditions */
    assert(PyList_CheckExact(consts));

    /* Buildup new tuple of constants */
    newconst = PyTuple_New(n);
    if (newconst == NULL)
        return 0;
    len_consts = PyList_GET_SIZE(consts);
    for (i=0 ; i<n ; i++) {
        constant = objs[i];
        Py_INCREF(constant);
        PyTuple_SET_ITEM(newconst, i, constant);
    }

    /* If it's a BUILD_SET, use the PyTuple we just built to create a
      PyFrozenSet, and use that as the constant instead: */
    if (codestr[0] == BUILD_SET) {
        PyObject *tuple = newconst;
        newconst = PyFrozenSet_New(tuple);
        Py_DECREF(tuple);
        if (newconst == NULL)
            return 0;
    }

    /* Append folded constant onto consts */
    if (PyList_Append(consts, newconst)) {
        Py_DECREF(newconst);
        return 0;
    }
    Py_DECREF(newconst);

    /* Write NOPs over old LOAD_CONSTS and
       add a new LOAD_CONST newconst on top of the BUILD_TUPLE n */
    codestr[0] = LOAD_CONST;
    SETARG(codestr, 0, len_consts);
    return 1;
}

/* Replace LOAD_CONST c1. LOAD_CONST c2 BINOP
   with    LOAD_CONST binop(c1,c2)
   The consts table must still be in list form so that the
   new constant can be appended.
   Called with codestr pointing to the BINOP.
   Abandons the transformation if the folding fails (i.e.  1+'a').
   If the new constant is a sequence, only folds when the size
   is below a threshold value.  That keeps pyc files from
   becoming large in the presence of code like:  (None,)*1000.
*/
static int
fold_binops_on_constants(unsigned char *codestr, PyObject *consts, PyObject **objs)
{
    PyObject *newconst, *v, *w;
    Py_ssize_t len_consts, size;
    int opcode;

    /* Pre-conditions */
    assert(PyList_CheckExact(consts));

    /* Create new constant */
    v = objs[0];
    w = objs[1];
    opcode = codestr[0];
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
            return 0;
    }
    if (newconst == NULL) {
        if(!PyErr_ExceptionMatches(PyExc_KeyboardInterrupt))
            PyErr_Clear();
        return 0;
    }
    size = PyObject_Size(newconst);
    if (size == -1) {
        if (PyErr_ExceptionMatches(PyExc_KeyboardInterrupt))
            return 0;
        PyErr_Clear();
    } else if (size > 20) {
        Py_DECREF(newconst);
        return 0;
    }

    /* Append folded constant into consts table */
    len_consts = PyList_GET_SIZE(consts);
    if (PyList_Append(consts, newconst)) {
        Py_DECREF(newconst);
        return 0;
    }
    Py_DECREF(newconst);

    /* Write NOP NOP NOP NOP LOAD_CONST newconst */
    codestr[-2] = LOAD_CONST;
    SETARG(codestr, -2, len_consts);
    return 1;
}

static int
fold_unaryops_on_constants(unsigned char *codestr, PyObject *consts, PyObject *v)
{
    PyObject *newconst;
    Py_ssize_t len_consts;
    int opcode;

    /* Pre-conditions */
    assert(PyList_CheckExact(consts));
    assert(codestr[0] == LOAD_CONST);

    /* Create new constant */
    opcode = codestr[3];
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
            return 0;
    }
    if (newconst == NULL) {
        if(!PyErr_ExceptionMatches(PyExc_KeyboardInterrupt))
            PyErr_Clear();
        return 0;
    }

    /* Append folded constant into consts table */
    len_consts = PyList_GET_SIZE(consts);
    if (PyList_Append(consts, newconst)) {
        Py_DECREF(newconst);
        PyErr_Clear();
        return 0;
    }
    Py_DECREF(newconst);

    /* Write NOP LOAD_CONST newconst */
    codestr[0] = NOP;
    codestr[1] = LOAD_CONST;
    SETARG(codestr, 1, len_consts);
    return 1;
}

static unsigned int *
markblocks(unsigned char *code, Py_ssize_t len)
{
    unsigned int *blocks = PyMem_New(unsigned int, len);
    int i,j, opcode, blockcnt = 0;

    if (blocks == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    memset(blocks, 0, len*sizeof(int));

    /* Mark labels in the first pass */
    for (i=0 ; i<len ; i+=CODESIZE(opcode)) {
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
    for (i=0 ; i<len ; i++) {
        blockcnt += blocks[i];          /* increment blockcnt over labels */
        blocks[i] = blockcnt;
    }
    return blocks;
}

/* Perform basic peephole optimizations to components of a code object.
   The consts object should still be in list form to allow new constants
   to be appended.

   To keep the optimizer simple, it bails out (does nothing) for code that
   has a length over 32,700, and does not calculate extended arguments.
   That allows us to avoid overflow and sign issues. Likewise, it bails when
   the lineno table has complex encoding for gaps >= 255. EXTENDED_ARG can
   appear before MAKE_FUNCTION; in this case both opcodes are skipped.
   EXTENDED_ARG preceding any other opcode causes the optimizer to bail.

   Optimizations are restricted to simple transformations occuring within a
   single basic block.  All transformations keep the code size the same or
   smaller.  For those that reduce size, the gaps are initially filled with
   NOPs.  Later those NOPs are removed and the jump addresses retargeted in
   a single pass.  Line numbering is adjusted accordingly. */

PyObject *
PyCode_Optimize(PyObject *code, PyObject* consts, PyObject *names,
                PyObject *lineno_obj)
{
    Py_ssize_t i, j, codelen;
    int nops, h, adj;
    int tgt, tgttgt, opcode;
    unsigned char *codestr = NULL;
    unsigned char *lineno;
    int *addrmap = NULL;
    int new_line, cum_orig_line, last_line;
    Py_ssize_t tabsiz;
    PyObject **const_stack = NULL;
    Py_ssize_t *load_const_stack = NULL;
    Py_ssize_t const_stack_top = -1;
    Py_ssize_t const_stack_size = 0;
    int in_consts = 0;  /* whether we are in a LOAD_CONST sequence */
    unsigned int *blocks = NULL;

    /* Bail out if an exception is set */
    if (PyErr_Occurred())
        goto exitError;

    /* Bypass optimization when the lineno table is too complex */
    assert(PyBytes_Check(lineno_obj));
    lineno = (unsigned char*)PyBytes_AS_STRING(lineno_obj);
    tabsiz = PyBytes_GET_SIZE(lineno_obj);
    if (memchr(lineno, 255, tabsiz) != NULL)
        goto exitUnchanged;

    /* Avoid situations where jump retargeting could overflow */
    assert(PyBytes_Check(code));
    codelen = PyBytes_GET_SIZE(code);
    if (codelen > 32700)
        goto exitUnchanged;

    /* Make a modifiable copy of the code string */
    codestr = (unsigned char *)PyMem_Malloc(codelen);
    if (codestr == NULL) {
        PyErr_NoMemory();
        goto exitError;
    }
    codestr = (unsigned char *)memcpy(codestr,
                                      PyBytes_AS_STRING(code), codelen);

    /* Verify that RETURN_VALUE terminates the codestring.      This allows
       the various transformation patterns to look ahead several
       instructions without additional checks to make sure they are not
       looking beyond the end of the code string.
    */
    if (codestr[codelen-1] != RETURN_VALUE)
        goto exitUnchanged;

    /* Mapping to new jump targets after NOPs are removed */
    addrmap = PyMem_New(int, codelen);
    if (addrmap == NULL) {
        PyErr_NoMemory();
        goto exitError;
    }

    blocks = markblocks(codestr, codelen);
    if (blocks == NULL)
        goto exitError;
    assert(PyList_Check(consts));

    CONST_STACK_CREATE();

    for (i=0 ; i<codelen ; i += CODESIZE(codestr[i])) {
      reoptimize_current:
        opcode = codestr[i];

        if (!in_consts) {
            CONST_STACK_RESET();
        }
        in_consts = 0;

        switch (opcode) {
            /* Replace UNARY_NOT POP_JUMP_IF_FALSE
               with    POP_JUMP_IF_TRUE */
            case UNARY_NOT:
                if (codestr[i+1] != POP_JUMP_IF_FALSE
                    || !ISBASICBLOCK(blocks,i,4))
                    continue;
                j = GETARG(codestr, i+1);
                codestr[i] = POP_JUMP_IF_TRUE;
                SETARG(codestr, i, j);
                codestr[i+3] = NOP;
                goto reoptimize_current;

                /* not a is b -->  a is not b
                   not a in b -->  a not in b
                   not a is not b -->  a is b
                   not a not in b -->  a in b
                */
            case COMPARE_OP:
                j = GETARG(codestr, i);
                if (j < 6  ||  j > 9  ||
                    codestr[i+3] != UNARY_NOT  ||
                    !ISBASICBLOCK(blocks,i,4))
                    continue;
                SETARG(codestr, i, (j^1));
                codestr[i+3] = NOP;
                break;

                /* Skip over LOAD_CONST trueconst
                   POP_JUMP_IF_FALSE xx. This improves
                   "while 1" performance. */
            case LOAD_CONST:
                CONST_STACK_PUSH_OP(i);
                j = GETARG(codestr, i);
                if (codestr[i+3] != POP_JUMP_IF_FALSE  ||
                    !ISBASICBLOCK(blocks,i,6)  ||
                    !PyObject_IsTrue(PyList_GET_ITEM(consts, j)))
                    continue;
                memset(codestr+i, NOP, 6);
                CONST_STACK_RESET();
                break;

                /* Try to fold tuples of constants (includes a case for lists and sets
                   which are only used for "in" and "not in" tests).
                   Skip over BUILD_SEQN 1 UNPACK_SEQN 1.
                   Replace BUILD_SEQN 2 UNPACK_SEQN 2 with ROT2.
                   Replace BUILD_SEQN 3 UNPACK_SEQN 3 with ROT3 ROT2. */
            case BUILD_TUPLE:
            case BUILD_LIST:
            case BUILD_SET:
                j = GETARG(codestr, i);
                if (j == 0)
                    break;
                h = CONST_STACK_OP_LASTN(j);
                assert((h >= 0 || CONST_STACK_LEN() < j));
                if (h >= 0 && j > 0 && j <= CONST_STACK_LEN() &&
                    ((opcode == BUILD_TUPLE &&
                      ISBASICBLOCK(blocks, h, i-h+3)) ||
                     ((opcode == BUILD_LIST || opcode == BUILD_SET) &&
                      codestr[i+3]==COMPARE_OP &&
                      ISBASICBLOCK(blocks, h, i-h+6) &&
                      (GETARG(codestr,i+3)==6 ||
                       GETARG(codestr,i+3)==7))) &&
                    tuple_of_constants(&codestr[i], j, consts, CONST_STACK_LASTN(j))) {
                    assert(codestr[i] == LOAD_CONST);
                    memset(&codestr[h], NOP, i - h);
                    CONST_STACK_POP(j);
                    CONST_STACK_PUSH_OP(i);
                    break;
                }
                if (codestr[i+3] != UNPACK_SEQUENCE  ||
                    !ISBASICBLOCK(blocks,i,6) ||
                    j != GETARG(codestr, i+3) ||
                    opcode == BUILD_SET)
                    continue;
                if (j == 1) {
                    memset(codestr+i, NOP, 6);
                } else if (j == 2) {
                    codestr[i] = ROT_TWO;
                    memset(codestr+i+1, NOP, 5);
                    CONST_STACK_RESET();
                } else if (j == 3) {
                    codestr[i] = ROT_THREE;
                    codestr[i+1] = ROT_TWO;
                    memset(codestr+i+2, NOP, 4);
                    CONST_STACK_RESET();
                }
                break;

                /* Fold binary ops on constants.
                   LOAD_CONST c1 LOAD_CONST c2 BINOP -->  LOAD_CONST binop(c1,c2) */
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
                /* NOTE: LOAD_CONST is saved at `i-2` since it has an arg
                   while BINOP hasn't */
                h = CONST_STACK_OP_LASTN(2);
                assert((h >= 0 || CONST_STACK_LEN() < 2));
                if (h >= 0 &&
                    ISBASICBLOCK(blocks, h, i-h+1)  &&
                    fold_binops_on_constants(&codestr[i], consts, CONST_STACK_LASTN(2))) {
                    i -= 2;
                    memset(&codestr[h], NOP, i - h);
                    assert(codestr[i] == LOAD_CONST);
                    CONST_STACK_POP(2);
                    CONST_STACK_PUSH_OP(i);
                }
                break;

                /* Fold unary ops on constants.
                   LOAD_CONST c1  UNARY_OP -->                  LOAD_CONST unary_op(c) */
            case UNARY_NEGATIVE:
            case UNARY_INVERT:
            case UNARY_POSITIVE:
                h = CONST_STACK_OP_LASTN(1);
                assert((h >= 0 || CONST_STACK_LEN() < 1));
                if (h >= 0 &&
                    ISBASICBLOCK(blocks, h, i-h+1)  &&
                    fold_unaryops_on_constants(&codestr[i-3], consts, CONST_STACK_TOP())) {
                    i -= 2;
                    assert(codestr[i] == LOAD_CONST);
                    CONST_STACK_POP(1);
                    CONST_STACK_PUSH_OP(i);
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
                      -->  x:POP_JUMP_IF_FALSE y+3
                   where y+3 is the instruction following the second test.
                */
            case JUMP_IF_FALSE_OR_POP:
            case JUMP_IF_TRUE_OR_POP:
                tgt = GETJUMPTGT(codestr, i);
                j = codestr[tgt];
                if (CONDITIONAL_JUMP(j)) {
                    /* NOTE: all possible jumps here are
                       absolute! */
                    if (JUMPS_ON_TRUE(j) == JUMPS_ON_TRUE(opcode)) {
                        /* The second jump will be
                           taken iff the first is. */
                        tgttgt = GETJUMPTGT(codestr, tgt);
                        /* The current opcode inherits
                           its target's stack behaviour */
                        codestr[i] = j;
                        SETARG(codestr, i, tgttgt);
                        goto reoptimize_current;
                    } else {
                        /* The second jump is not taken
                           if the first is (so jump past
                           it), and all conditional
                           jumps pop their argument when
                           they're not taken (so change
                           the first jump to pop its
                           argument when it's taken). */
                        if (JUMPS_ON_TRUE(opcode))
                            codestr[i] = POP_JUMP_IF_TRUE;
                        else
                            codestr[i] = POP_JUMP_IF_FALSE;
                        SETARG(codestr, i, (tgt + 3));
                        goto reoptimize_current;
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
                tgt = GETJUMPTGT(codestr, i);
                /* Replace JUMP_* to a RETURN into just a RETURN */
                if (UNCONDITIONAL_JUMP(opcode) &&
                    codestr[tgt] == RETURN_VALUE) {
                    codestr[i] = RETURN_VALUE;
                    memset(codestr+i+1, NOP, 2);
                    continue;
                }
                if (!UNCONDITIONAL_JUMP(codestr[tgt]))
                    continue;
                tgttgt = GETJUMPTGT(codestr, tgt);
                if (opcode == JUMP_FORWARD) /* JMP_ABS can go backwards */
                    opcode = JUMP_ABSOLUTE;
                if (!ABSOLUTE_JUMP(opcode))
                    tgttgt -= i + 3;     /* Calc relative jump addr */
                if (tgttgt < 0)                           /* No backward relative jumps */
                    continue;
                codestr[i] = opcode;
                SETARG(codestr, i, tgttgt);
                break;

            case EXTENDED_ARG:
                if (codestr[i+3] != MAKE_FUNCTION)
                    goto exitUnchanged;
                /* don't visit MAKE_FUNCTION as GETARG will be wrong */
                i += 3;
                break;

                /* Replace RETURN LOAD_CONST None RETURN with just RETURN */
                /* Remove unreachable JUMPs after RETURN */
            case RETURN_VALUE:
                if (i+4 >= codelen)
                    continue;
                if (codestr[i+4] == RETURN_VALUE &&
                    ISBASICBLOCK(blocks,i,5))
                    memset(codestr+i+1, NOP, 4);
                else if (UNCONDITIONAL_JUMP(codestr[i+1]) &&
                         ISBASICBLOCK(blocks,i,4))
                    memset(codestr+i+1, NOP, 3);
                break;
        }
    }

    /* Fixup linenotab */
    for (i=0, nops=0 ; i<codelen ; i += CODESIZE(codestr[i])) {
        assert(i - nops <= INT_MAX);
        addrmap[i] = (int)(i - nops);
        if (codestr[i] == NOP)
            nops++;
    }
    cum_orig_line = 0;
    last_line = 0;
    for (i=0 ; i < tabsiz ; i+=2) {
        cum_orig_line += lineno[i];
        new_line = addrmap[cum_orig_line];
        assert (new_line - last_line < 255);
        lineno[i] =((unsigned char)(new_line - last_line));
        last_line = new_line;
    }

    /* Remove NOPs and fixup jump targets */
    for (i=0, h=0 ; i<codelen ; ) {
        opcode = codestr[i];
        switch (opcode) {
            case NOP:
                i++;
                continue;

            case JUMP_ABSOLUTE:
            case CONTINUE_LOOP:
            case POP_JUMP_IF_FALSE:
            case POP_JUMP_IF_TRUE:
            case JUMP_IF_FALSE_OR_POP:
            case JUMP_IF_TRUE_OR_POP:
                j = addrmap[GETARG(codestr, i)];
                SETARG(codestr, i, j);
                break;

            case FOR_ITER:
            case JUMP_FORWARD:
            case SETUP_LOOP:
            case SETUP_EXCEPT:
            case SETUP_FINALLY:
            case SETUP_WITH:
            case SETUP_ASYNC_WITH:
                j = addrmap[GETARG(codestr, i) + i + 3] - addrmap[i] - 3;
                SETARG(codestr, i, j);
                break;
        }
        adj = CODESIZE(opcode);
        while (adj--)
            codestr[h++] = codestr[i++];
    }
    assert(h + nops == codelen);

    code = PyBytes_FromStringAndSize((char *)codestr, h);
    CONST_STACK_DELETE();
    PyMem_Free(addrmap);
    PyMem_Free(codestr);
    PyMem_Free(blocks);
    return code;

 exitError:
    code = NULL;

 exitUnchanged:
    CONST_STACK_DELETE();
    if (blocks != NULL)
        PyMem_Free(blocks);
    if (addrmap != NULL)
        PyMem_Free(addrmap);
    if (codestr != NULL)
        PyMem_Free(codestr);
    Py_XINCREF(code);
    return code;
}
