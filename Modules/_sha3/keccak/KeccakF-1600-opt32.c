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

#include <string.h>
/* #include "brg_endian.h" */
#include "KeccakF-1600-opt32-settings.h"
#include "KeccakF-1600-interface.h"

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
/* typedef unsigned long long int UINT64; */

#ifdef UseInterleaveTables
static int interleaveTablesBuilt = 0;
static UINT16 interleaveTable[65536];
static UINT16 deinterleaveTable[65536];

static void buildInterleaveTables()
{
    UINT32 i, j;
    UINT16 x;

    if (!interleaveTablesBuilt) {
        for(i=0; i<65536; i++) {
            x = 0;
            for(j=0; j<16; j++) {
                if (i & (1 << j))
                    x |= (1 << (j/2 + 8*(j%2)));
            }
            interleaveTable[i] = x;
            deinterleaveTable[x] = (UINT16)i;
        }
        interleaveTablesBuilt = 1;
    }
}

#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)

#define xor2bytesIntoInterleavedWords(even, odd, source, j) \
    i##j = interleaveTable[((const UINT16*)source)[j]]; \
    ((UINT8*)even)[j] ^= i##j & 0xFF; \
    ((UINT8*)odd)[j] ^= i##j >> 8;

#define setInterleavedWordsInto2bytes(dest, even, odd, j) \
    d##j = deinterleaveTable[((even >> (j*8)) & 0xFF) ^ (((odd >> (j*8)) & 0xFF) << 8)]; \
    ((UINT16*)dest)[j] = d##j;

#else /*  (PLATFORM_BYTE_ORDER == IS_BIG_ENDIAN) */

#define xor2bytesIntoInterleavedWords(even, odd, source, j) \
    i##j = interleaveTable[source[2*j] ^ ((UINT16)source[2*j+1] << 8)]; \
    *even ^= (i##j & 0xFF) << (j*8); \
    *odd ^= ((i##j >> 8) & 0xFF) << (j*8);

#define setInterleavedWordsInto2bytes(dest, even, odd, j) \
    d##j = deinterleaveTable[((even >> (j*8)) & 0xFF) ^ (((odd >> (j*8)) & 0xFF) << 8)]; \
    dest[2*j] = d##j & 0xFF; \
    dest[2*j+1] = d##j >> 8;

#endif /*  Endianness */

static void xor8bytesIntoInterleavedWords(UINT32 *even, UINT32 *odd, const UINT8* source)
{
    UINT16 i0, i1, i2, i3;

    xor2bytesIntoInterleavedWords(even, odd, source, 0)
    xor2bytesIntoInterleavedWords(even, odd, source, 1)
    xor2bytesIntoInterleavedWords(even, odd, source, 2)
    xor2bytesIntoInterleavedWords(even, odd, source, 3)
}

#define xorLanesIntoState(laneCount, state, input) \
    { \
        int i; \
        for(i=0; i<(laneCount); i++) \
            xor8bytesIntoInterleavedWords(state+i*2, state+i*2+1, input+i*8); \
    }

static void setInterleavedWordsInto8bytes(UINT8* dest, UINT32 even, UINT32 odd)
{
    UINT16 d0, d1, d2, d3;

    setInterleavedWordsInto2bytes(dest, even, odd, 0)
    setInterleavedWordsInto2bytes(dest, even, odd, 1)
    setInterleavedWordsInto2bytes(dest, even, odd, 2)
    setInterleavedWordsInto2bytes(dest, even, odd, 3)
}

#define extractLanes(laneCount, state, data) \
    { \
        int i; \
        for(i=0; i<(laneCount); i++) \
            setInterleavedWordsInto8bytes(data+i*8, ((UINT32*)state)[i*2], ((UINT32*)state)[i*2+1]); \
    }

#else /*  No interleaving tables */

#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)

/*  Credit: Henry S. Warren, Hacker's Delight, Addison-Wesley, 2002 */
#define xorInterleavedLE(rateInLanes, state, input) \
	{ \
		const UINT32 * pI = (const UINT32 *)input; \
		UINT32 * pS = state; \
		UINT32 t, x0, x1; \
	    int i; \
	    for (i = (rateInLanes)-1; i >= 0; --i) \
		{ \
			x0 = *(pI++); \
			t = (x0 ^ (x0 >>  1)) & 0x22222222UL;  x0 = x0 ^ t ^ (t <<  1); \
			t = (x0 ^ (x0 >>  2)) & 0x0C0C0C0CUL;  x0 = x0 ^ t ^ (t <<  2); \
			t = (x0 ^ (x0 >>  4)) & 0x00F000F0UL;  x0 = x0 ^ t ^ (t <<  4); \
			t = (x0 ^ (x0 >>  8)) & 0x0000FF00UL;  x0 = x0 ^ t ^ (t <<  8); \
 			x1 = *(pI++); \
			t = (x1 ^ (x1 >>  1)) & 0x22222222UL;  x1 = x1 ^ t ^ (t <<  1); \
			t = (x1 ^ (x1 >>  2)) & 0x0C0C0C0CUL;  x1 = x1 ^ t ^ (t <<  2); \
			t = (x1 ^ (x1 >>  4)) & 0x00F000F0UL;  x1 = x1 ^ t ^ (t <<  4); \
			t = (x1 ^ (x1 >>  8)) & 0x0000FF00UL;  x1 = x1 ^ t ^ (t <<  8); \
			*(pS++) ^= (UINT16)x0 | (x1 << 16); \
			*(pS++) ^= (x0 >> 16) | (x1 & 0xFFFF0000); \
		} \
	}

#define xorLanesIntoState(laneCount, state, input) \
    xorInterleavedLE(laneCount, state, input)

#else /*  (PLATFORM_BYTE_ORDER == IS_BIG_ENDIAN) */

/*  Credit: Henry S. Warren, Hacker's Delight, Addison-Wesley, 2002 */
UINT64 toInterleaving(UINT64 x) 
{
   UINT64 t;

   t = (x ^ (x >>  1)) & 0x2222222222222222ULL;  x = x ^ t ^ (t <<  1);
   t = (x ^ (x >>  2)) & 0x0C0C0C0C0C0C0C0CULL;  x = x ^ t ^ (t <<  2);
   t = (x ^ (x >>  4)) & 0x00F000F000F000F0ULL;  x = x ^ t ^ (t <<  4);
   t = (x ^ (x >>  8)) & 0x0000FF000000FF00ULL;  x = x ^ t ^ (t <<  8);
   t = (x ^ (x >> 16)) & 0x00000000FFFF0000ULL;  x = x ^ t ^ (t << 16);

   return x;
}

static void xor8bytesIntoInterleavedWords(UINT32* evenAndOdd, const UINT8* source)
{
    /*  This can be optimized */
    UINT64 sourceWord =
        (UINT64)source[0]
        ^ (((UINT64)source[1]) <<  8)
        ^ (((UINT64)source[2]) << 16)
        ^ (((UINT64)source[3]) << 24)
        ^ (((UINT64)source[4]) << 32)
        ^ (((UINT64)source[5]) << 40)
        ^ (((UINT64)source[6]) << 48)
        ^ (((UINT64)source[7]) << 56);
    UINT64 evenAndOddWord = toInterleaving(sourceWord);
    evenAndOdd[0] ^= (UINT32)evenAndOddWord;
    evenAndOdd[1] ^= (UINT32)(evenAndOddWord >> 32);
}

#define xorLanesIntoState(laneCount, state, input) \
    { \
        int i; \
        for(i=0; i<(laneCount); i++) \
            xor8bytesIntoInterleavedWords(state+i*2, input+i*8); \
    }

#endif /*  Endianness */

/*  Credit: Henry S. Warren, Hacker's Delight, Addison-Wesley, 2002 */
UINT64 fromInterleaving(UINT64 x)
{
   UINT64 t;

   t = (x ^ (x >> 16)) & 0x00000000FFFF0000ULL;  x = x ^ t ^ (t << 16);
   t = (x ^ (x >>  8)) & 0x0000FF000000FF00ULL;  x = x ^ t ^ (t <<  8);
   t = (x ^ (x >>  4)) & 0x00F000F000F000F0ULL;  x = x ^ t ^ (t <<  4);
   t = (x ^ (x >>  2)) & 0x0C0C0C0C0C0C0C0CULL;  x = x ^ t ^ (t <<  2);
   t = (x ^ (x >>  1)) & 0x2222222222222222ULL;  x = x ^ t ^ (t <<  1);

   return x;
}

static void setInterleavedWordsInto8bytes(UINT8* dest, UINT32* evenAndOdd)
{
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    ((UINT64*)dest)[0] = fromInterleaving(*(UINT64*)evenAndOdd);
#else /*  (PLATFORM_BYTE_ORDER == IS_BIG_ENDIAN) */
    /*  This can be optimized */
    UINT64 evenAndOddWord = (UINT64)evenAndOdd[0] ^ ((UINT64)evenAndOdd[1] << 32);
    UINT64 destWord = fromInterleaving(evenAndOddWord);
    dest[0] = destWord & 0xFF;
    dest[1] = (destWord >> 8) & 0xFF;
    dest[2] = (destWord >> 16) & 0xFF;
    dest[3] = (destWord >> 24) & 0xFF;
    dest[4] = (destWord >> 32) & 0xFF;
    dest[5] = (destWord >> 40) & 0xFF;
    dest[6] = (destWord >> 48) & 0xFF;
    dest[7] = (destWord >> 56) & 0xFF;
#endif /*  Endianness */
}

#define extractLanes(laneCount, state, data) \
    { \
        int i; \
        for(i=0; i<(laneCount); i++) \
            setInterleavedWordsInto8bytes(data+i*8, (UINT32*)state+i*2); \
    }

#endif /*  With or without interleaving tables */

#if defined(_MSC_VER)
#define ROL32(a, offset) _rotl(a, offset)
#elif (defined (__arm__) && defined(__ARMCC_VERSION))
#define ROL32(a, offset) __ror(a, 32-(offset))
#else
#define ROL32(a, offset) ((((UINT32)a) << (offset)) ^ (((UINT32)a) >> (32-(offset))))
#endif

#include "KeccakF-1600-unrolling.macros"
#include "KeccakF-1600-32.macros"

#if (UseSchedule == 3)

#ifdef UseBebigokimisa
#error "No lane complementing with schedule 3."
#endif

#if (Unrolling != 2)
#error "Only unrolling 2 is supported by schedule 3."
#endif

static void KeccakPermutationOnWords(UINT32 *state)
{
    rounds
}

static void KeccakPermutationOnWordsAfterXoring(UINT32 *state, const UINT8 *input, unsigned int laneCount)
{
    xorLanesIntoState(laneCount, state, input)
    rounds
}

#ifdef ProvideFast576
static void KeccakPermutationOnWordsAfterXoring576bits(UINT32 *state, const UINT8 *input)
{
    xorLanesIntoState(9, state, input)
    rounds
}
#endif

#ifdef ProvideFast832
static void KeccakPermutationOnWordsAfterXoring832bits(UINT32 *state, const UINT8 *input)
{
    xorLanesIntoState(13, state, input)
    rounds
}
#endif

#ifdef ProvideFast1024
static void KeccakPermutationOnWordsAfterXoring1024bits(UINT32 *state, const UINT8 *input)
{
    xorLanesIntoState(16, state, input)
    rounds
}
#endif

#ifdef ProvideFast1088
static void KeccakPermutationOnWordsAfterXoring1088bits(UINT32 *state, const UINT8 *input)
{
    xorLanesIntoState(17, state, input)
    rounds
}
#endif

#ifdef ProvideFast1152
static void KeccakPermutationOnWordsAfterXoring1152bits(UINT32 *state, const UINT8 *input)
{
    xorLanesIntoState(18, state, input)
    rounds
}
#endif

#ifdef ProvideFast1344
static void KeccakPermutationOnWordsAfterXoring1344bits(UINT32 *state, const UINT8 *input)
{
    xorLanesIntoState(21, state, input)
    rounds
}
#endif

#else /*  (Schedule != 3) */

static void KeccakPermutationOnWords(UINT32 *state)
{
    declareABCDE
#if (Unrolling != 24)
    unsigned int i;
#endif

    copyFromState(A, state)
    rounds
}

static void KeccakPermutationOnWordsAfterXoring(UINT32 *state, const UINT8 *input, unsigned int laneCount)
{
    declareABCDE
    unsigned int i;

    xorLanesIntoState(laneCount, state, input)
    copyFromState(A, state)
    rounds
}

#ifdef ProvideFast576
static void KeccakPermutationOnWordsAfterXoring576bits(UINT32 *state, const UINT8 *input)
{
    declareABCDE
    unsigned int i;

    xorLanesIntoState(9, state, input)
    copyFromState(A, state)
    rounds
}
#endif

#ifdef ProvideFast832
static void KeccakPermutationOnWordsAfterXoring832bits(UINT32 *state, const UINT8 *input)
{
    declareABCDE
    unsigned int i;

    xorLanesIntoState(13, state, input)
    copyFromState(A, state)
    rounds
}
#endif

#ifdef ProvideFast1024
static void KeccakPermutationOnWordsAfterXoring1024bits(UINT32 *state, const UINT8 *input)
{
    declareABCDE
    unsigned int i;

    xorLanesIntoState(16, state, input)
    copyFromState(A, state)
    rounds
}
#endif

#ifdef ProvideFast1088
static void KeccakPermutationOnWordsAfterXoring1088bits(UINT32 *state, const UINT8 *input)
{
    declareABCDE
    unsigned int i;

    xorLanesIntoState(17, state, input)
    copyFromState(A, state)
    rounds
}
#endif

#ifdef ProvideFast1152
static void KeccakPermutationOnWordsAfterXoring1152bits(UINT32 *state, const UINT8 *input)
{
    declareABCDE
    unsigned int i;

    xorLanesIntoState(18, state, input)
    copyFromState(A, state)
    rounds
}
#endif

#ifdef ProvideFast1344
static void KeccakPermutationOnWordsAfterXoring1344bits(UINT32 *state, const UINT8 *input)
{
    declareABCDE
    unsigned int i;

    xorLanesIntoState(21, state, input)
    copyFromState(A, state)
    rounds
}
#endif

#endif

static void KeccakInitialize()
{
#ifdef UseInterleaveTables
    buildInterleaveTables();
#endif
}

static void KeccakInitializeState(unsigned char *state)
{
    memset(state, 0, 200);
#ifdef UseBebigokimisa
    ((UINT32*)state)[ 2] = ~(UINT32)0;
    ((UINT32*)state)[ 3] = ~(UINT32)0;
    ((UINT32*)state)[ 4] = ~(UINT32)0;
    ((UINT32*)state)[ 5] = ~(UINT32)0;
    ((UINT32*)state)[16] = ~(UINT32)0;
    ((UINT32*)state)[17] = ~(UINT32)0;
    ((UINT32*)state)[24] = ~(UINT32)0;
    ((UINT32*)state)[25] = ~(UINT32)0;
    ((UINT32*)state)[34] = ~(UINT32)0;
    ((UINT32*)state)[35] = ~(UINT32)0;
    ((UINT32*)state)[40] = ~(UINT32)0;
    ((UINT32*)state)[41] = ~(UINT32)0;
#endif
}

static void KeccakPermutation(unsigned char *state)
{
    /*  We assume the state is always stored as interleaved 32-bit words */
    KeccakPermutationOnWords((UINT32*)state);
}

#ifdef ProvideFast576
static void KeccakAbsorb576bits(unsigned char *state, const unsigned char *data)
{
    KeccakPermutationOnWordsAfterXoring576bits((UINT32*)state, data);
}
#endif

#ifdef ProvideFast832
static void KeccakAbsorb832bits(unsigned char *state, const unsigned char *data)
{
    KeccakPermutationOnWordsAfterXoring832bits((UINT32*)state, data);
}
#endif

#ifdef ProvideFast1024
static void KeccakAbsorb1024bits(unsigned char *state, const unsigned char *data)
{
    KeccakPermutationOnWordsAfterXoring1024bits((UINT32*)state, data);
}
#endif

#ifdef ProvideFast1088
static void KeccakAbsorb1088bits(unsigned char *state, const unsigned char *data)
{
    KeccakPermutationOnWordsAfterXoring1088bits((UINT32*)state, data);
}
#endif

#ifdef ProvideFast1152
static void KeccakAbsorb1152bits(unsigned char *state, const unsigned char *data)
{
    KeccakPermutationOnWordsAfterXoring1152bits((UINT32*)state, data);
}
#endif

#ifdef ProvideFast1344
static void KeccakAbsorb1344bits(unsigned char *state, const unsigned char *data)
{
    KeccakPermutationOnWordsAfterXoring1344bits((UINT32*)state, data);
}
#endif

static void KeccakAbsorb(unsigned char *state, const unsigned char *data, unsigned int laneCount)
{
    KeccakPermutationOnWordsAfterXoring((UINT32*)state, data, laneCount);
}

#ifdef ProvideFast1024
static void KeccakExtract1024bits(const unsigned char *state, unsigned char *data)
{
    extractLanes(16, state, data)
#ifdef UseBebigokimisa
    ((UINT32*)data)[ 2] = ~((UINT32*)data)[ 2];
    ((UINT32*)data)[ 3] = ~((UINT32*)data)[ 3];
    ((UINT32*)data)[ 4] = ~((UINT32*)data)[ 4];
    ((UINT32*)data)[ 5] = ~((UINT32*)data)[ 5];
    ((UINT32*)data)[16] = ~((UINT32*)data)[16];
    ((UINT32*)data)[17] = ~((UINT32*)data)[17];
    ((UINT32*)data)[24] = ~((UINT32*)data)[24];
    ((UINT32*)data)[25] = ~((UINT32*)data)[25];
#endif
}
#endif

static void KeccakExtract(const unsigned char *state, unsigned char *data, unsigned int laneCount)
{
    extractLanes(laneCount, state, data)
#ifdef UseBebigokimisa
    if (laneCount > 1) {
        ((UINT32*)data)[ 2] = ~((UINT32*)data)[ 2];
        ((UINT32*)data)[ 3] = ~((UINT32*)data)[ 3];
        if (laneCount > 2) {
            ((UINT32*)data)[ 4] = ~((UINT32*)data)[ 4];
            ((UINT32*)data)[ 5] = ~((UINT32*)data)[ 5];
            if (laneCount > 8) {
                ((UINT32*)data)[16] = ~((UINT32*)data)[16];
                ((UINT32*)data)[17] = ~((UINT32*)data)[17];
                if (laneCount > 12) {
                    ((UINT32*)data)[24] = ~((UINT32*)data)[24];
                    ((UINT32*)data)[25] = ~((UINT32*)data)[25];
                    if (laneCount > 17) {
                        ((UINT32*)data)[34] = ~((UINT32*)data)[34];
                        ((UINT32*)data)[35] = ~((UINT32*)data)[35];
                        if (laneCount > 20) {
                            ((UINT32*)data)[40] = ~((UINT32*)data)[40];
                            ((UINT32*)data)[41] = ~((UINT32*)data)[41];
                        }
                    }
                }
            }
        }
    }
#endif
}
