/*
The Keccak sponge function, designed by Guido Bertoni, Joan Daemen,
Michaël Peeters and Gilles Van Assche. For more information, feedback or
questions, please refer to our website: http://keccak.noekeon.org/

Implementation by the designers,
hereby denoted as "the implementer".

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#ifndef _KeccakPermutationInterface_h_
#define _KeccakPermutationInterface_h_

#include "KeccakF-1600-int-set.h"

static void KeccakInitialize( void );
static void KeccakInitializeState(unsigned char *state);
static void KeccakPermutation(unsigned char *state);
#ifdef ProvideFast576
static void KeccakAbsorb576bits(unsigned char *state, const unsigned char *data);
#endif
#ifdef ProvideFast832
static void KeccakAbsorb832bits(unsigned char *state, const unsigned char *data);
#endif
#ifdef ProvideFast1024
static void KeccakAbsorb1024bits(unsigned char *state, const unsigned char *data);
#endif
#ifdef ProvideFast1088
static void KeccakAbsorb1088bits(unsigned char *state, const unsigned char *data);
#endif
#ifdef ProvideFast1152
static void KeccakAbsorb1152bits(unsigned char *state, const unsigned char *data);
#endif
#ifdef ProvideFast1344
static void KeccakAbsorb1344bits(unsigned char *state, const unsigned char *data);
#endif
static void KeccakAbsorb(unsigned char *state, const unsigned char *data, unsigned int laneCount);
#ifdef ProvideFast1024
static void KeccakExtract1024bits(const unsigned char *state, unsigned char *data);
#endif
static void KeccakExtract(const unsigned char *state, unsigned char *data, unsigned int laneCount);

#endif
