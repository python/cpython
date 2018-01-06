/* This file contains code shared by the compiler, the peephole
   optimizer and frameobject.
 */

#ifdef WORDS_BIGENDIAN
#  define PACKOPARG(opcode, oparg) ((_Py_CODEUNIT)(((opcode) << 8) | (oparg)))
#else
#  define PACKOPARG(opcode, oparg) ((_Py_CODEUNIT)(((oparg) << 8) | (opcode)))
#endif

#ifdef __GNUC__
#define MAYBE_UNUSED __attribute__ ((unused))
#else
#define MAYBE_UNUSED
#endif

/* Minimum number of code units necessary to encode instruction with
   EXTENDED_ARGs */
MAYBE_UNUSED static int
instrsize(unsigned int oparg)
{
    return oparg <= 0xff ? 1 :
        oparg <= 0xffff ? 2 :
        oparg <= 0xffffff ? 3 :
        4;
}

/* Spits out op/oparg pair using ilen bytes. codestr should be pointed at the
   desired location of the first EXTENDED_ARG */
MAYBE_UNUSED static void
write_op_arg(_Py_CODEUNIT *codestr, unsigned char opcode,
    unsigned int oparg, int ilen)
{
    switch (ilen) {
        case 4:
            *codestr++ = PACKOPARG(EXTENDED_ARG, (oparg >> 24) & 0xff);
            /* fall through */
        case 3:
            *codestr++ = PACKOPARG(EXTENDED_ARG, (oparg >> 16) & 0xff);
            /* fall through */
        case 2:
            *codestr++ = PACKOPARG(EXTENDED_ARG, (oparg >> 8) & 0xff);
            /* fall through */
        case 1:
            *codestr++ = PACKOPARG(opcode, oparg & 0xff);
            break;
        default:
            Py_UNREACHABLE();
    }
}

/* Given the index of the effective opcode,
   scan back to construct the oparg with EXTENDED_ARG */
MAYBE_UNUSED static unsigned int
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
