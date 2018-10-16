/* This file contains code shared by the compiler and the peephole
   optimizer.
 */

#ifdef WORDS_BIGENDIAN
#  define PACKOPARG(opcode, oparg) ((_Py_CODEUNIT)(((opcode) << 8) | (oparg)))
#else
#  define PACKOPARG(opcode, oparg) ((_Py_CODEUNIT)(((oparg) << 8) | (opcode)))
#endif

/* Minimum number of code units necessary to encode instruction with
   EXTENDED_ARGs */
static int
instrsize(unsigned int oparg)
{
    return oparg <= 0xff ? 1 :
        oparg <= 0xffff ? 2 :
        oparg <= 0xffffff ? 3 :
        4;
}

/* Spits out op/oparg pair using ilen bytes. codestr should be pointed at the
   desired location of the first EXTENDED_ARG */
static void
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
            assert(0);
    }
}
