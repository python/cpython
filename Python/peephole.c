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
#define ABSOLUTE_JUMP(op) (op==JUMP_ABSOLUTE \
    || op==POP_JUMP_IF_FALSE || op==POP_JUMP_IF_TRUE \
    || op==JUMP_IF_FALSE_OR_POP || op==JUMP_IF_TRUE_OR_POP || op==JUMP_IF_NOT_EXC_MATCH)
#define JUMPS_ON_TRUE(op) (op==POP_JUMP_IF_TRUE || op==JUMP_IF_TRUE_OR_POP)
#define GETJUMPTGT(arr, i) (get_arg(arr, i) / sizeof(_Py_CODEUNIT) + \
        (ABSOLUTE_JUMP(_Py_OPCODE(arr[i])) ? 0 : i+1))
#define ISBASICBLOCK(blocks, start, end) \
    (blocks[start]==blocks[end])


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
            assert(_Py_OPCODE(codestr[i]) == EXTENDED_ARG);
        }
    }
}

/* Scans through EXTENDED ARGs, seeking the index of the effective opcode */
static Py_ssize_t
find_op(const _Py_CODEUNIT *codestr, Py_ssize_t codelen, Py_ssize_t i)
{
    while (i < codelen && _Py_OPCODE(codestr[i]) == EXTENDED_ARG) {
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

/* Replace LOAD_CONST c1, LOAD_CONST c2 ... LOAD_CONST cn, BUILD_TUPLE n
   with    LOAD_CONST (c1, c2, ... cn).
   The consts table must still be in list form so that the
   new constant (c1, c2, ... cn) can be appended.
   Called with codestr pointing to the first LOAD_CONST.
*/
static Py_ssize_t
fold_tuple_on_constants(_Py_CODEUNIT *codestr, Py_ssize_t codelen,
                        Py_ssize_t c_start, Py_ssize_t opcode_end,
                        PyObject *consts, int n)
{
    /* Pre-conditions */
    assert(PyList_CheckExact(consts));

    /* Buildup new tuple of constants */
    PyObject *newconst = PyTuple_New(n);
    if (newconst == NULL) {
        return -1;
    }

    for (Py_ssize_t i = 0, pos = c_start; i < n; i++, pos++) {
        assert(pos < opcode_end);
        pos = find_op(codestr, codelen, pos);
        assert(_Py_OPCODE(codestr[pos]) == LOAD_CONST);

        unsigned int arg = get_arg(codestr, pos);
        PyObject *constant = PyList_GET_ITEM(consts, arg);
        Py_INCREF(constant);
        PyTuple_SET_ITEM(newconst, i, constant);
    }

    Py_ssize_t index = PyList_GET_SIZE(consts);
#if SIZEOF_SIZE_T > SIZEOF_INT
    if ((size_t)index >= UINT_MAX - 1) {
        Py_DECREF(newconst);
        PyErr_SetString(PyExc_OverflowError, "too many constants");
        return -1;
    }
#endif

    /* Append folded constant onto consts */
    if (PyList_Append(consts, newconst)) {
        Py_DECREF(newconst);
        return -1;
    }
    Py_DECREF(newconst);

    return copy_op_arg(codestr, c_start, LOAD_CONST,
                       (unsigned int)index, opcode_end);
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
            case JUMP_IF_NOT_EXC_MATCH:
            case JUMP_ABSOLUTE:
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
    Py_ssize_t h, i, nexti, op_start, tgt;
    unsigned int j, nops;
    unsigned char opcode, nextop;
    _Py_CODEUNIT *codestr = NULL;
    unsigned char *lnotab;
    unsigned int cum_orig_offset, last_offset;
    Py_ssize_t tabsiz;
    // Count runs of consecutive LOAD_CONSTs
    unsigned int cumlc = 0, lastlc = 0;
    unsigned int *blocks = NULL;

    /* Bail out if an exception is set */
    if (PyErr_Occurred())
        goto exitError;

    /* Bypass optimization when the lnotab table is too complex */
    assert(PyBytes_Check(lnotab_obj));
    lnotab = (unsigned char*)PyBytes_AS_STRING(lnotab_obj);
    tabsiz = PyBytes_GET_SIZE(lnotab_obj);
    assert(tabsiz == 0 || Py_REFCNT(lnotab_obj) == 1);

    /* Don't optimize if lnotab contains instruction pointer delta larger
       than +255 (encoded as multiple bytes), just to keep the peephole optimizer
       simple. The optimizer leaves line number deltas unchanged. */

    for (i = 0; i < tabsiz; i += 2) {
        if (lnotab[i] == 255) {
            goto exitUnchanged;
        }
    }

    assert(PyBytes_Check(code));
    Py_ssize_t codesize = PyBytes_GET_SIZE(code);
    assert(codesize % sizeof(_Py_CODEUNIT) == 0);
    Py_ssize_t codelen = codesize / sizeof(_Py_CODEUNIT);
    if (codelen > INT_MAX) {
        /* Python assembler is limited to INT_MAX: see assembler.a_offset in
           compile.c. */
        goto exitUnchanged;
    }

    /* Make a modifiable copy of the code string */
    codestr = (_Py_CODEUNIT *)PyMem_Malloc(codesize);
    if (codestr == NULL) {
        PyErr_NoMemory();
        goto exitError;
    }
    memcpy(codestr, PyBytes_AS_STRING(code), codesize);

    blocks = markblocks(codestr, codelen);
    if (blocks == NULL)
        goto exitError;
    assert(PyList_Check(consts));

    for (i=find_op(codestr, codelen, 0) ; i<codelen ; i=nexti) {
        opcode = _Py_OPCODE(codestr[i]);
        op_start = i;
        while (op_start >= 1 && _Py_OPCODE(codestr[op_start-1]) == EXTENDED_ARG) {
            op_start--;
        }

        nexti = i + 1;
        while (nexti < codelen && _Py_OPCODE(codestr[nexti]) == EXTENDED_ARG)
            nexti++;
        nextop = nexti < codelen ? _Py_OPCODE(codestr[nexti]) : 0;

        lastlc = cumlc;
        cumlc = 0;

        switch (opcode) {
                /* Skip over LOAD_CONST trueconst
                   POP_JUMP_IF_FALSE xx.  This improves
                   "while 1" performance.  */
            case LOAD_CONST:
                cumlc = lastlc + 1;
                if (nextop != POP_JUMP_IF_FALSE  ||
                    !ISBASICBLOCK(blocks, op_start, i + 1)) {
                    break;
                }
                PyObject* cnt = PyList_GET_ITEM(consts, get_arg(codestr, i));
                int is_true = PyObject_IsTrue(cnt);
                if (is_true == -1) {
                    goto exitError;
                }
                if (is_true == 1) {
                    fill_nops(codestr, op_start, nexti + 1);
                    cumlc = 0;
                }
                break;

                /* Try to fold tuples of constants.
                   Skip over BUILD_SEQN 1 UNPACK_SEQN 1.
                   Replace BUILD_SEQN 2 UNPACK_SEQN 2 with ROT2.
                   Replace BUILD_SEQN 3 UNPACK_SEQN 3 with ROT3 ROT2. */
            case BUILD_TUPLE:
                j = get_arg(codestr, i);
                if (j > 0 && lastlc >= j) {
                    h = lastn_const_start(codestr, op_start, j);
                    if (ISBASICBLOCK(blocks, h, op_start)) {
                        h = fold_tuple_on_constants(codestr, codelen,
                                                    h, i+1, consts, j);
                        break;
                    }
                }
                if (nextop != UNPACK_SEQUENCE  ||
                    !ISBASICBLOCK(blocks, op_start, i + 1) ||
                    j != get_arg(codestr, nexti))
                    break;
                if (j < 2) {
                    fill_nops(codestr, op_start, nexti + 1);
                } else if (j == 2) {
                    codestr[op_start] = PACKOPARG(ROT_TWO, 0);
                    fill_nops(codestr, op_start + 1, nexti + 1);
                } else if (j == 3) {
                    codestr[op_start] = PACKOPARG(ROT_THREE, 0);
                    codestr[op_start + 1] = PACKOPARG(ROT_TWO, 0);
                    fill_nops(codestr, op_start + 2, nexti + 1);
                }
                break;

                /* Simplify conditional jump to conditional jump where the
                   result of the first test implies the success of a similar
                   test or the failure of the opposite test.
                   Arises in code like:
                   "a and b or c"
                   "(a and b) and c"
                   "(a or b) or c"
                   "(a or b) and c"
                   x:JUMP_IF_FALSE_OR_POP y   y:JUMP_IF_FALSE_OR_POP z
                      -->  x:JUMP_IF_FALSE_OR_POP z
                   x:JUMP_IF_FALSE_OR_POP y   y:JUMP_IF_TRUE_OR_POP z
                      -->  x:POP_JUMP_IF_FALSE y+1
                   where y+1 is the instruction following the second test.
                */
            case JUMP_IF_FALSE_OR_POP:
            case JUMP_IF_TRUE_OR_POP:
                h = get_arg(codestr, i) / sizeof(_Py_CODEUNIT);
                tgt = find_op(codestr, codelen, h);

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
                        Py_ssize_t arg = (tgt + 1);
                        /* cannot overflow: codelen <= INT_MAX */
                        assert((size_t)arg <= UINT_MAX / sizeof(_Py_CODEUNIT));
                        arg *= sizeof(_Py_CODEUNIT);
                        h = set_arg(codestr, i, (unsigned int)arg);
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
            case JUMP_FORWARD:
            case JUMP_ABSOLUTE:
                h = GETJUMPTGT(codestr, i);
                tgt = find_op(codestr, codelen, h);
                /* Replace JUMP_* to a RETURN into just a RETURN */
                if (UNCONDITIONAL_JUMP(opcode) &&
                    _Py_OPCODE(codestr[tgt]) == RETURN_VALUE) {
                    codestr[op_start] = PACKOPARG(RETURN_VALUE, 0);
                    fill_nops(codestr, op_start + 1, i + 1);
                } else if (UNCONDITIONAL_JUMP(_Py_OPCODE(codestr[tgt]))) {
                    size_t arg = GETJUMPTGT(codestr, tgt);
                    if (opcode == JUMP_FORWARD) { /* JMP_ABS can go backwards */
                        opcode = JUMP_ABSOLUTE;
                    } else if (!ABSOLUTE_JUMP(opcode)) {
                        if (arg < (size_t)(i + 1)) {
                            break;           /* No backward relative jumps */
                        }
                        arg -= i + 1;          /* Calc relative jump addr */
                    }
                    /* cannot overflow: codelen <= INT_MAX */
                    assert(arg <= (UINT_MAX / sizeof(_Py_CODEUNIT)));
                    arg *= sizeof(_Py_CODEUNIT);
                    copy_op_arg(codestr, op_start, opcode,
                                (unsigned int)arg, i + 1);
                }
                break;

                /* Remove unreachable ops after RETURN */
            case RETURN_VALUE:
                h = i + 1;
                while (h < codelen && ISBASICBLOCK(blocks, i, h))
                {
                    /* Leave SETUP_FINALLY and RERAISE in place to help find block limits. */
                    if (_Py_OPCODE(codestr[h]) == SETUP_FINALLY || _Py_OPCODE(codestr[h]) == RERAISE) {
                        while (h > i + 1 &&
                               _Py_OPCODE(codestr[h - 1]) == EXTENDED_ARG)
                        {
                            h--;
                        }
                        break;
                    }
                    h++;
                }
                if (h > i + 1) {
                    fill_nops(codestr, i + 1, h);
                    nexti = find_op(codestr, codelen, h);
                }
                break;
        }
    }

    /* Fixup lnotab */
    for (i = 0, nops = 0; i < codelen; i++) {
        size_t block = (size_t)i - nops;
        /* cannot overflow: codelen <= INT_MAX */
        assert(block <= UINT_MAX);
        /* original code offset => new code offset */
        blocks[i] = (unsigned int)block;
        if (_Py_OPCODE(codestr[i]) == NOP) {
            nops++;
        }
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
            case POP_JUMP_IF_FALSE:
            case POP_JUMP_IF_TRUE:
            case JUMP_IF_FALSE_OR_POP:
            case JUMP_IF_TRUE_OR_POP:
            case JUMP_IF_NOT_EXC_MATCH:
                j = blocks[j / sizeof(_Py_CODEUNIT)] * sizeof(_Py_CODEUNIT);
                break;

            case FOR_ITER:
            case JUMP_FORWARD:
            case SETUP_FINALLY:
            case SETUP_WITH:
            case SETUP_ASYNC_WITH:
                j = blocks[j / sizeof(_Py_CODEUNIT) + i + 1] - blocks[i] - 1;
                j *= sizeof(_Py_CODEUNIT);
                break;
        }
        Py_ssize_t ilen = i - op_start + 1;
        if (instrsize(j) > ilen) {
            goto exitUnchanged;
        }
        /* If instrsize(j) < ilen, we'll emit EXTENDED_ARG 0 */
        if (ilen > 4) {
            /* Can only happen when PyCode_Optimize() is called with
               malformed bytecode. */
            goto exitUnchanged;
        }
        write_op_arg(codestr + h, opcode, j, (int)ilen);
        h += ilen;
    }
    assert(h + (Py_ssize_t)nops == codelen);

    PyMem_Free(blocks);
    code = PyBytes_FromStringAndSize((char *)codestr, h * sizeof(_Py_CODEUNIT));
    PyMem_Free(codestr);
    return code;

 exitError:
    code = NULL;

 exitUnchanged:
    Py_XINCREF(code);
    PyMem_Free(blocks);
    PyMem_Free(codestr);
    return code;
}
