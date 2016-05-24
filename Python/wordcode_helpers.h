/* This file contains code shared by the compiler and the peephole
   optimizer.
 */

/* Minimum number of bytes necessary to encode instruction with EXTENDED_ARGs */
static int
instrsize(unsigned int oparg)
{
    return oparg <= 0xff ? 2 :
        oparg <= 0xffff ? 4 :
        oparg <= 0xffffff ? 6 :
        8;
}

/* Spits out op/oparg pair using ilen bytes. codestr should be pointed at the
   desired location of the first EXTENDED_ARG */
static void
write_op_arg(unsigned char *codestr, unsigned char opcode,
    unsigned int oparg, int ilen)
{
    switch (ilen) {
        case 8:
            *codestr++ = EXTENDED_ARG;
            *codestr++ = (oparg >> 24) & 0xff;
        case 6:
            *codestr++ = EXTENDED_ARG;
            *codestr++ = (oparg >> 16) & 0xff;
        case 4:
            *codestr++ = EXTENDED_ARG;
            *codestr++ = (oparg >> 8) & 0xff;
        case 2:
            *codestr++ = opcode;
            *codestr++ = oparg & 0xff;
            break;
        default:
            assert(0);
    }
}
