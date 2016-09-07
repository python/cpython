/*
Implementation by the Keccak, Keyak and Ketje Teams, namely, Guido Bertoni,
Joan Daemen, Michaël Peeters, Gilles Van Assche and Ronny Van Keer, hereby
denoted as "the implementer".

For more information, feedback or questions, please refer to our websites:
http://keccak.noekeon.org/
http://keyak.noekeon.org/
http://ketje.noekeon.org/

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#include    <string.h>
/* #include "brg_endian.h" */
#include "KeccakP-1600-SnP.h"
#include "SnP-Relaned.h"

typedef unsigned char UINT8;
typedef unsigned int UINT32;
/* WARNING: on 8-bit and 16-bit platforms, this should be replaced by: */

/*typedef unsigned long       UINT32; */


#define ROL32(a, offset) ((((UINT32)a) << (offset)) ^ (((UINT32)a) >> (32-(offset))))

/* Credit to Henry S. Warren, Hacker's Delight, Addison-Wesley, 2002 */

#define prepareToBitInterleaving(low, high, temp, temp0, temp1) \
        temp0 = (low); \
        temp = (temp0 ^ (temp0 >>  1)) & 0x22222222UL;  temp0 = temp0 ^ temp ^ (temp <<  1); \
        temp = (temp0 ^ (temp0 >>  2)) & 0x0C0C0C0CUL;  temp0 = temp0 ^ temp ^ (temp <<  2); \
        temp = (temp0 ^ (temp0 >>  4)) & 0x00F000F0UL;  temp0 = temp0 ^ temp ^ (temp <<  4); \
        temp = (temp0 ^ (temp0 >>  8)) & 0x0000FF00UL;  temp0 = temp0 ^ temp ^ (temp <<  8); \
        temp1 = (high); \
        temp = (temp1 ^ (temp1 >>  1)) & 0x22222222UL;  temp1 = temp1 ^ temp ^ (temp <<  1); \
        temp = (temp1 ^ (temp1 >>  2)) & 0x0C0C0C0CUL;  temp1 = temp1 ^ temp ^ (temp <<  2); \
        temp = (temp1 ^ (temp1 >>  4)) & 0x00F000F0UL;  temp1 = temp1 ^ temp ^ (temp <<  4); \
        temp = (temp1 ^ (temp1 >>  8)) & 0x0000FF00UL;  temp1 = temp1 ^ temp ^ (temp <<  8);

#define toBitInterleavingAndXOR(low, high, even, odd, temp, temp0, temp1) \
        prepareToBitInterleaving(low, high, temp, temp0, temp1) \
        even ^= (temp0 & 0x0000FFFF) | (temp1 << 16); \
        odd ^= (temp0 >> 16) | (temp1 & 0xFFFF0000);

#define toBitInterleavingAndAND(low, high, even, odd, temp, temp0, temp1) \
        prepareToBitInterleaving(low, high, temp, temp0, temp1) \
        even &= (temp0 & 0x0000FFFF) | (temp1 << 16); \
        odd &= (temp0 >> 16) | (temp1 & 0xFFFF0000);

#define toBitInterleavingAndSet(low, high, even, odd, temp, temp0, temp1) \
        prepareToBitInterleaving(low, high, temp, temp0, temp1) \
        even = (temp0 & 0x0000FFFF) | (temp1 << 16); \
        odd = (temp0 >> 16) | (temp1 & 0xFFFF0000);

/* Credit to Henry S. Warren, Hacker's Delight, Addison-Wesley, 2002 */

#define prepareFromBitInterleaving(even, odd, temp, temp0, temp1) \
        temp0 = (even); \
        temp1 = (odd); \
        temp = (temp0 & 0x0000FFFF) | (temp1 << 16); \
        temp1 = (temp0 >> 16) | (temp1 & 0xFFFF0000); \
        temp0 = temp; \
        temp = (temp0 ^ (temp0 >>  8)) & 0x0000FF00UL;  temp0 = temp0 ^ temp ^ (temp <<  8); \
        temp = (temp0 ^ (temp0 >>  4)) & 0x00F000F0UL;  temp0 = temp0 ^ temp ^ (temp <<  4); \
        temp = (temp0 ^ (temp0 >>  2)) & 0x0C0C0C0CUL;  temp0 = temp0 ^ temp ^ (temp <<  2); \
        temp = (temp0 ^ (temp0 >>  1)) & 0x22222222UL;  temp0 = temp0 ^ temp ^ (temp <<  1); \
        temp = (temp1 ^ (temp1 >>  8)) & 0x0000FF00UL;  temp1 = temp1 ^ temp ^ (temp <<  8); \
        temp = (temp1 ^ (temp1 >>  4)) & 0x00F000F0UL;  temp1 = temp1 ^ temp ^ (temp <<  4); \
        temp = (temp1 ^ (temp1 >>  2)) & 0x0C0C0C0CUL;  temp1 = temp1 ^ temp ^ (temp <<  2); \
        temp = (temp1 ^ (temp1 >>  1)) & 0x22222222UL;  temp1 = temp1 ^ temp ^ (temp <<  1);

#define fromBitInterleaving(even, odd, low, high, temp, temp0, temp1) \
        prepareFromBitInterleaving(even, odd, temp, temp0, temp1) \
        low = temp0; \
        high = temp1;

#define fromBitInterleavingAndXOR(even, odd, lowIn, highIn, lowOut, highOut, temp, temp0, temp1) \
        prepareFromBitInterleaving(even, odd, temp, temp0, temp1) \
        lowOut = lowIn ^ temp0; \
        highOut = highIn ^ temp1;

void KeccakP1600_SetBytesInLaneToZero(void *state, unsigned int lanePosition, unsigned int offset, unsigned int length)
{
    UINT8 laneAsBytes[8];
    UINT32 low, high;
    UINT32 temp, temp0, temp1;
    UINT32 *stateAsHalfLanes = (UINT32*)state;

    memset(laneAsBytes, 0xFF, offset);
    memset(laneAsBytes+offset, 0x00, length);
    memset(laneAsBytes+offset+length, 0xFF, 8-offset-length);
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    low = *((UINT32*)(laneAsBytes+0));
    high = *((UINT32*)(laneAsBytes+4));
#else
    low = laneAsBytes[0]
        | ((UINT32)(laneAsBytes[1]) << 8)
        | ((UINT32)(laneAsBytes[2]) << 16)
        | ((UINT32)(laneAsBytes[3]) << 24);
    high = laneAsBytes[4]
        | ((UINT32)(laneAsBytes[5]) << 8)
        | ((UINT32)(laneAsBytes[6]) << 16)
        | ((UINT32)(laneAsBytes[7]) << 24);
#endif
    toBitInterleavingAndAND(low, high, stateAsHalfLanes[lanePosition*2+0], stateAsHalfLanes[lanePosition*2+1], temp, temp0, temp1);
}

/* ---------------------------------------------------------------- */

void KeccakP1600_Initialize(void *state)
{
    memset(state, 0, 200);
}

/* ---------------------------------------------------------------- */

void KeccakP1600_AddByte(void *state, unsigned char byte, unsigned int offset)
{
    unsigned int lanePosition = offset/8;
    unsigned int offsetInLane = offset%8;
    UINT32 low, high;
    UINT32 temp, temp0, temp1;
    UINT32 *stateAsHalfLanes = (UINT32*)state;

    if (offsetInLane < 4) {
        low = (UINT32)byte << (offsetInLane*8);
        high = 0;
    }
    else {
        low = 0;
        high = (UINT32)byte << ((offsetInLane-4)*8);
    }
    toBitInterleavingAndXOR(low, high, stateAsHalfLanes[lanePosition*2+0], stateAsHalfLanes[lanePosition*2+1], temp, temp0, temp1);
}

/* ---------------------------------------------------------------- */

void KeccakP1600_AddBytesInLane(void *state, unsigned int lanePosition, const unsigned char *data, unsigned int offset, unsigned int length)
{
    UINT8 laneAsBytes[8];
    UINT32 low, high;
    UINT32 temp, temp0, temp1;
    UINT32 *stateAsHalfLanes = (UINT32*)state;

    memset(laneAsBytes, 0, 8);
    memcpy(laneAsBytes+offset, data, length);
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    low = *((UINT32*)(laneAsBytes+0));
    high = *((UINT32*)(laneAsBytes+4));
#else
    low = laneAsBytes[0]
        | ((UINT32)(laneAsBytes[1]) << 8)
        | ((UINT32)(laneAsBytes[2]) << 16)
        | ((UINT32)(laneAsBytes[3]) << 24);
    high = laneAsBytes[4]
        | ((UINT32)(laneAsBytes[5]) << 8)
        | ((UINT32)(laneAsBytes[6]) << 16)
        | ((UINT32)(laneAsBytes[7]) << 24);
#endif
    toBitInterleavingAndXOR(low, high, stateAsHalfLanes[lanePosition*2+0], stateAsHalfLanes[lanePosition*2+1], temp, temp0, temp1);
}

/* ---------------------------------------------------------------- */

void KeccakP1600_AddLanes(void *state, const unsigned char *data, unsigned int laneCount)
{
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    const UINT32 * pI = (const UINT32 *)data;
    UINT32 * pS = (UINT32*)state;
    UINT32 t, x0, x1;
    int i;
    for (i = laneCount-1; i >= 0; --i) {
#ifdef NO_MISALIGNED_ACCESSES
        UINT32 low;
        UINT32 high;
        memcpy(&low, pI++, 4);
        memcpy(&high, pI++, 4);
        toBitInterleavingAndXOR(low, high, *(pS++), *(pS++), t, x0, x1);
#else
        toBitInterleavingAndXOR(*(pI++), *(pI++), *(pS++), *(pS++), t, x0, x1)
#endif
    }
#else
    unsigned int lanePosition;
    for(lanePosition=0; lanePosition<laneCount; lanePosition++) {
        UINT8 laneAsBytes[8];
        UINT32 low, high, temp, temp0, temp1;
        UINT32 *stateAsHalfLanes;
        memcpy(laneAsBytes, data+lanePosition*8, 8);
        low = laneAsBytes[0]
            | ((UINT32)(laneAsBytes[1]) << 8)
            | ((UINT32)(laneAsBytes[2]) << 16)
            | ((UINT32)(laneAsBytes[3]) << 24);
        high = laneAsBytes[4]
            | ((UINT32)(laneAsBytes[5]) << 8)
            | ((UINT32)(laneAsBytes[6]) << 16)
            | ((UINT32)(laneAsBytes[7]) << 24);
        stateAsHalfLanes = (UINT32*)state;
        toBitInterleavingAndXOR(low, high, stateAsHalfLanes[lanePosition*2+0], stateAsHalfLanes[lanePosition*2+1], temp, temp0, temp1);
    }
#endif
}

/* ---------------------------------------------------------------- */

void KeccakP1600_AddBytes(void *state, const unsigned char *data, unsigned int offset, unsigned int length)
{
    SnP_AddBytes(state, data, offset, length, KeccakP1600_AddLanes, KeccakP1600_AddBytesInLane, 8);
}

/* ---------------------------------------------------------------- */

void KeccakP1600_OverwriteBytesInLane(void *state, unsigned int lanePosition, const unsigned char *data, unsigned int offset, unsigned int length)
{
    KeccakP1600_SetBytesInLaneToZero(state, lanePosition, offset, length);
    KeccakP1600_AddBytesInLane(state, lanePosition, data, offset, length);
}

/* ---------------------------------------------------------------- */

void KeccakP1600_OverwriteLanes(void *state, const unsigned char *data, unsigned int laneCount)
{
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    const UINT32 * pI = (const UINT32 *)data;
    UINT32 * pS = (UINT32 *)state;
    UINT32 t, x0, x1;
    int i;
    for (i = laneCount-1; i >= 0; --i) {
#ifdef NO_MISALIGNED_ACCESSES
        UINT32 low;
        UINT32 high;
        memcpy(&low, pI++, 4);
        memcpy(&high, pI++, 4);
        toBitInterleavingAndSet(low, high, *(pS++), *(pS++), t, x0, x1);
#else
        toBitInterleavingAndSet(*(pI++), *(pI++), *(pS++), *(pS++), t, x0, x1)
#endif
    }
#else
    unsigned int lanePosition;
    for(lanePosition=0; lanePosition<laneCount; lanePosition++) {
        UINT8 laneAsBytes[8];
        UINT32 low, high, temp, temp0, temp1;
        UINT32 *stateAsHalfLanes;
        memcpy(laneAsBytes, data+lanePosition*8, 8);
        low = laneAsBytes[0]
            | ((UINT32)(laneAsBytes[1]) << 8)
            | ((UINT32)(laneAsBytes[2]) << 16)
            | ((UINT32)(laneAsBytes[3]) << 24);
        high = laneAsBytes[4]
            | ((UINT32)(laneAsBytes[5]) << 8)
            | ((UINT32)(laneAsBytes[6]) << 16)
            | ((UINT32)(laneAsBytes[7]) << 24);
        stateAsHalfLanes = (UINT32*)state;
        toBitInterleavingAndSet(low, high, stateAsHalfLanes[lanePosition*2+0], stateAsHalfLanes[lanePosition*2+1], temp, temp0, temp1);
    }
#endif
}

/* ---------------------------------------------------------------- */

void KeccakP1600_OverwriteBytes(void *state, const unsigned char *data, unsigned int offset, unsigned int length)
{
    SnP_OverwriteBytes(state, data, offset, length, KeccakP1600_OverwriteLanes, KeccakP1600_OverwriteBytesInLane, 8);
}

/* ---------------------------------------------------------------- */

void KeccakP1600_OverwriteWithZeroes(void *state, unsigned int byteCount)
{
    UINT32 *stateAsHalfLanes = (UINT32*)state;
    unsigned int i;

    for(i=0; i<byteCount/8; i++) {
        stateAsHalfLanes[i*2+0] = 0;
        stateAsHalfLanes[i*2+1] = 0;
    }
    if (byteCount%8 != 0)
        KeccakP1600_SetBytesInLaneToZero(state, byteCount/8, 0, byteCount%8);
}

/* ---------------------------------------------------------------- */

void KeccakP1600_ExtractBytesInLane(const void *state, unsigned int lanePosition, unsigned char *data, unsigned int offset, unsigned int length)
{
    UINT32 *stateAsHalfLanes = (UINT32*)state;
    UINT32 low, high, temp, temp0, temp1;
    UINT8 laneAsBytes[8];

    fromBitInterleaving(stateAsHalfLanes[lanePosition*2], stateAsHalfLanes[lanePosition*2+1], low, high, temp, temp0, temp1);
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    *((UINT32*)(laneAsBytes+0)) = low;
    *((UINT32*)(laneAsBytes+4)) = high;
#else
    laneAsBytes[0] = low & 0xFF;
    laneAsBytes[1] = (low >> 8) & 0xFF;
    laneAsBytes[2] = (low >> 16) & 0xFF;
    laneAsBytes[3] = (low >> 24) & 0xFF;
    laneAsBytes[4] = high & 0xFF;
    laneAsBytes[5] = (high >> 8) & 0xFF;
    laneAsBytes[6] = (high >> 16) & 0xFF;
    laneAsBytes[7] = (high >> 24) & 0xFF;
#endif
    memcpy(data, laneAsBytes+offset, length);
}

/* ---------------------------------------------------------------- */

void KeccakP1600_ExtractLanes(const void *state, unsigned char *data, unsigned int laneCount)
{
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    UINT32 * pI = (UINT32 *)data;
    const UINT32 * pS = ( const UINT32 *)state;
    UINT32 t, x0, x1;
    int i;
    for (i = laneCount-1; i >= 0; --i) {
#ifdef NO_MISALIGNED_ACCESSES
        UINT32 low;
        UINT32 high;
        fromBitInterleaving(*(pS++), *(pS++), low, high, t, x0, x1);
        memcpy(pI++, &low, 4);
        memcpy(pI++, &high, 4);
#else
        fromBitInterleaving(*(pS++), *(pS++), *(pI++), *(pI++), t, x0, x1)
#endif
    }
#else
    unsigned int lanePosition;
    for(lanePosition=0; lanePosition<laneCount; lanePosition++) {
        UINT32 *stateAsHalfLanes = (UINT32*)state;
        UINT32 low, high, temp, temp0, temp1;
        UINT8 laneAsBytes[8];
        fromBitInterleaving(stateAsHalfLanes[lanePosition*2], stateAsHalfLanes[lanePosition*2+1], low, high, temp, temp0, temp1);
        laneAsBytes[0] = low & 0xFF;
        laneAsBytes[1] = (low >> 8) & 0xFF;
        laneAsBytes[2] = (low >> 16) & 0xFF;
        laneAsBytes[3] = (low >> 24) & 0xFF;
        laneAsBytes[4] = high & 0xFF;
        laneAsBytes[5] = (high >> 8) & 0xFF;
        laneAsBytes[6] = (high >> 16) & 0xFF;
        laneAsBytes[7] = (high >> 24) & 0xFF;
        memcpy(data+lanePosition*8, laneAsBytes, 8);
    }
#endif
}

/* ---------------------------------------------------------------- */

void KeccakP1600_ExtractBytes(const void *state, unsigned char *data, unsigned int offset, unsigned int length)
{
    SnP_ExtractBytes(state, data, offset, length, KeccakP1600_ExtractLanes, KeccakP1600_ExtractBytesInLane, 8);
}

/* ---------------------------------------------------------------- */

void KeccakP1600_ExtractAndAddBytesInLane(const void *state, unsigned int lanePosition, const unsigned char *input, unsigned char *output, unsigned int offset, unsigned int length)
{
    UINT32 *stateAsHalfLanes = (UINT32*)state;
    UINT32 low, high, temp, temp0, temp1;
    UINT8 laneAsBytes[8];
    unsigned int i;

    fromBitInterleaving(stateAsHalfLanes[lanePosition*2], stateAsHalfLanes[lanePosition*2+1], low, high, temp, temp0, temp1);
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    *((UINT32*)(laneAsBytes+0)) = low;
    *((UINT32*)(laneAsBytes+4)) = high;
#else
    laneAsBytes[0] = low & 0xFF;
    laneAsBytes[1] = (low >> 8) & 0xFF;
    laneAsBytes[2] = (low >> 16) & 0xFF;
    laneAsBytes[3] = (low >> 24) & 0xFF;
    laneAsBytes[4] = high & 0xFF;
    laneAsBytes[5] = (high >> 8) & 0xFF;
    laneAsBytes[6] = (high >> 16) & 0xFF;
    laneAsBytes[7] = (high >> 24) & 0xFF;
#endif
    for(i=0; i<length; i++)
        output[i] = input[i] ^ laneAsBytes[offset+i];
}

/* ---------------------------------------------------------------- */

void KeccakP1600_ExtractAndAddLanes(const void *state, const unsigned char *input, unsigned char *output, unsigned int laneCount)
{
#if (PLATFORM_BYTE_ORDER == IS_LITTLE_ENDIAN)
    const UINT32 * pI = (const UINT32 *)input;
    UINT32 * pO = (UINT32 *)output;
    const UINT32 * pS = (const UINT32 *)state;
    UINT32 t, x0, x1;
    int i;
    for (i = laneCount-1; i >= 0; --i) {
#ifdef NO_MISALIGNED_ACCESSES
        UINT32 low;
        UINT32 high;
        fromBitInterleaving(*(pS++), *(pS++), low, high, t, x0, x1);
        *(pO++) = *(pI++) ^ low;
        *(pO++) = *(pI++) ^ high;
#else
        fromBitInterleavingAndXOR(*(pS++), *(pS++), *(pI++), *(pI++), *(pO++), *(pO++), t, x0, x1)
#endif
    }
#else
    unsigned int lanePosition;
    for(lanePosition=0; lanePosition<laneCount; lanePosition++) {
        UINT32 *stateAsHalfLanes = (UINT32*)state;
        UINT32 low, high, temp, temp0, temp1;
        UINT8 laneAsBytes[8];
        fromBitInterleaving(stateAsHalfLanes[lanePosition*2], stateAsHalfLanes[lanePosition*2+1], low, high, temp, temp0, temp1);
        laneAsBytes[0] = low & 0xFF;
        laneAsBytes[1] = (low >> 8) & 0xFF;
        laneAsBytes[2] = (low >> 16) & 0xFF;
        laneAsBytes[3] = (low >> 24) & 0xFF;
        laneAsBytes[4] = high & 0xFF;
        laneAsBytes[5] = (high >> 8) & 0xFF;
        laneAsBytes[6] = (high >> 16) & 0xFF;
        laneAsBytes[7] = (high >> 24) & 0xFF;
        ((UINT32*)(output+lanePosition*8))[0] = ((UINT32*)(input+lanePosition*8))[0] ^ (*(const UINT32*)(laneAsBytes+0));
        ((UINT32*)(output+lanePosition*8))[1] = ((UINT32*)(input+lanePosition*8))[0] ^ (*(const UINT32*)(laneAsBytes+4));
    }
#endif
}
/* ---------------------------------------------------------------- */

void KeccakP1600_ExtractAndAddBytes(const void *state, const unsigned char *input, unsigned char *output, unsigned int offset, unsigned int length)
{
    SnP_ExtractAndAddBytes(state, input, output, offset, length, KeccakP1600_ExtractAndAddLanes, KeccakP1600_ExtractAndAddBytesInLane, 8);
}

/* ---------------------------------------------------------------- */

static const UINT32 KeccakF1600RoundConstants_int2[2*24+1] =
{
    0x00000001UL,    0x00000000UL,
    0x00000000UL,    0x00000089UL,
    0x00000000UL,    0x8000008bUL,
    0x00000000UL,    0x80008080UL,
    0x00000001UL,    0x0000008bUL,
    0x00000001UL,    0x00008000UL,
    0x00000001UL,    0x80008088UL,
    0x00000001UL,    0x80000082UL,
    0x00000000UL,    0x0000000bUL,
    0x00000000UL,    0x0000000aUL,
    0x00000001UL,    0x00008082UL,
    0x00000000UL,    0x00008003UL,
    0x00000001UL,    0x0000808bUL,
    0x00000001UL,    0x8000000bUL,
    0x00000001UL,    0x8000008aUL,
    0x00000001UL,    0x80000081UL,
    0x00000000UL,    0x80000081UL,
    0x00000000UL,    0x80000008UL,
    0x00000000UL,    0x00000083UL,
    0x00000000UL,    0x80008003UL,
    0x00000001UL,    0x80008088UL,
    0x00000000UL,    0x80000088UL,
    0x00000001UL,    0x00008000UL,
    0x00000000UL,    0x80008082UL,
    0x000000FFUL
};

#define KeccakAtoD_round0() \
        Cx = Abu0^Agu0^Aku0^Amu0^Asu0; \
        Du1 = Abe1^Age1^Ake1^Ame1^Ase1; \
        Da0 = Cx^ROL32(Du1, 1); \
        Cz = Abu1^Agu1^Aku1^Amu1^Asu1; \
        Du0 = Abe0^Age0^Ake0^Ame0^Ase0; \
        Da1 = Cz^Du0; \
\
        Cw = Abi0^Agi0^Aki0^Ami0^Asi0; \
        Do0 = Cw^ROL32(Cz, 1); \
        Cy = Abi1^Agi1^Aki1^Ami1^Asi1; \
        Do1 = Cy^Cx; \
\
        Cx = Aba0^Aga0^Aka0^Ama0^Asa0; \
        De0 = Cx^ROL32(Cy, 1); \
        Cz = Aba1^Aga1^Aka1^Ama1^Asa1; \
        De1 = Cz^Cw; \
\
        Cy = Abo1^Ago1^Ako1^Amo1^Aso1; \
        Di0 = Du0^ROL32(Cy, 1); \
        Cw = Abo0^Ago0^Ako0^Amo0^Aso0; \
        Di1 = Du1^Cw; \
\
        Du0 = Cw^ROL32(Cz, 1); \
        Du1 = Cy^Cx; \

#define KeccakAtoD_round1() \
        Cx = Asu0^Agu0^Amu0^Abu1^Aku1; \
        Du1 = Age1^Ame0^Abe0^Ake1^Ase1; \
        Da0 = Cx^ROL32(Du1, 1); \
        Cz = Asu1^Agu1^Amu1^Abu0^Aku0; \
        Du0 = Age0^Ame1^Abe1^Ake0^Ase0; \
        Da1 = Cz^Du0; \
\
        Cw = Aki1^Asi1^Agi0^Ami1^Abi0; \
        Do0 = Cw^ROL32(Cz, 1); \
        Cy = Aki0^Asi0^Agi1^Ami0^Abi1; \
        Do1 = Cy^Cx; \
\
        Cx = Aba0^Aka1^Asa0^Aga0^Ama1; \
        De0 = Cx^ROL32(Cy, 1); \
        Cz = Aba1^Aka0^Asa1^Aga1^Ama0; \
        De1 = Cz^Cw; \
\
        Cy = Amo0^Abo1^Ako0^Aso1^Ago0; \
        Di0 = Du0^ROL32(Cy, 1); \
        Cw = Amo1^Abo0^Ako1^Aso0^Ago1; \
        Di1 = Du1^Cw; \
\
        Du0 = Cw^ROL32(Cz, 1); \
        Du1 = Cy^Cx; \

#define KeccakAtoD_round2() \
        Cx = Aku1^Agu0^Abu1^Asu1^Amu1; \
        Du1 = Ame0^Ake0^Age0^Abe0^Ase1; \
        Da0 = Cx^ROL32(Du1, 1); \
        Cz = Aku0^Agu1^Abu0^Asu0^Amu0; \
        Du0 = Ame1^Ake1^Age1^Abe1^Ase0; \
        Da1 = Cz^Du0; \
\
        Cw = Agi1^Abi1^Asi1^Ami0^Aki1; \
        Do0 = Cw^ROL32(Cz, 1); \
        Cy = Agi0^Abi0^Asi0^Ami1^Aki0; \
        Do1 = Cy^Cx; \
\
        Cx = Aba0^Asa1^Ama1^Aka1^Aga1; \
        De0 = Cx^ROL32(Cy, 1); \
        Cz = Aba1^Asa0^Ama0^Aka0^Aga0; \
        De1 = Cz^Cw; \
\
        Cy = Aso0^Amo0^Ako1^Ago0^Abo0; \
        Di0 = Du0^ROL32(Cy, 1); \
        Cw = Aso1^Amo1^Ako0^Ago1^Abo1; \
        Di1 = Du1^Cw; \
\
        Du0 = Cw^ROL32(Cz, 1); \
        Du1 = Cy^Cx; \

#define KeccakAtoD_round3() \
        Cx = Amu1^Agu0^Asu1^Aku0^Abu0; \
        Du1 = Ake0^Abe1^Ame1^Age0^Ase1; \
        Da0 = Cx^ROL32(Du1, 1); \
        Cz = Amu0^Agu1^Asu0^Aku1^Abu1; \
        Du0 = Ake1^Abe0^Ame0^Age1^Ase0; \
        Da1 = Cz^Du0; \
\
        Cw = Asi0^Aki0^Abi1^Ami1^Agi1; \
        Do0 = Cw^ROL32(Cz, 1); \
        Cy = Asi1^Aki1^Abi0^Ami0^Agi0; \
        Do1 = Cy^Cx; \
\
        Cx = Aba0^Ama0^Aga1^Asa1^Aka0; \
        De0 = Cx^ROL32(Cy, 1); \
        Cz = Aba1^Ama1^Aga0^Asa0^Aka1; \
        De1 = Cz^Cw; \
\
        Cy = Ago1^Aso0^Ako0^Abo0^Amo1; \
        Di0 = Du0^ROL32(Cy, 1); \
        Cw = Ago0^Aso1^Ako1^Abo1^Amo0; \
        Di1 = Du1^Cw; \
\
        Du0 = Cw^ROL32(Cz, 1); \
        Du1 = Cy^Cx; \

void KeccakP1600_Permute_Nrounds(void *state, unsigned int nRounds)
{
    {
        UINT32 Da0, De0, Di0, Do0, Du0;
        UINT32 Da1, De1, Di1, Do1, Du1;
        UINT32 Ca0, Ce0, Ci0, Co0, Cu0;
        UINT32 Cx, Cy, Cz, Cw;
        #define Ba Ca0
        #define Be Ce0
        #define Bi Ci0
        #define Bo Co0
        #define Bu Cu0
        const UINT32 *pRoundConstants = KeccakF1600RoundConstants_int2+(24-nRounds)*2;
        UINT32 *stateAsHalfLanes = (UINT32*)state;
        #define Aba0 stateAsHalfLanes[ 0]
        #define Aba1 stateAsHalfLanes[ 1]
        #define Abe0 stateAsHalfLanes[ 2]
        #define Abe1 stateAsHalfLanes[ 3]
        #define Abi0 stateAsHalfLanes[ 4]
        #define Abi1 stateAsHalfLanes[ 5]
        #define Abo0 stateAsHalfLanes[ 6]
        #define Abo1 stateAsHalfLanes[ 7]
        #define Abu0 stateAsHalfLanes[ 8]
        #define Abu1 stateAsHalfLanes[ 9]
        #define Aga0 stateAsHalfLanes[10]
        #define Aga1 stateAsHalfLanes[11]
        #define Age0 stateAsHalfLanes[12]
        #define Age1 stateAsHalfLanes[13]
        #define Agi0 stateAsHalfLanes[14]
        #define Agi1 stateAsHalfLanes[15]
        #define Ago0 stateAsHalfLanes[16]
        #define Ago1 stateAsHalfLanes[17]
        #define Agu0 stateAsHalfLanes[18]
        #define Agu1 stateAsHalfLanes[19]
        #define Aka0 stateAsHalfLanes[20]
        #define Aka1 stateAsHalfLanes[21]
        #define Ake0 stateAsHalfLanes[22]
        #define Ake1 stateAsHalfLanes[23]
        #define Aki0 stateAsHalfLanes[24]
        #define Aki1 stateAsHalfLanes[25]
        #define Ako0 stateAsHalfLanes[26]
        #define Ako1 stateAsHalfLanes[27]
        #define Aku0 stateAsHalfLanes[28]
        #define Aku1 stateAsHalfLanes[29]
        #define Ama0 stateAsHalfLanes[30]
        #define Ama1 stateAsHalfLanes[31]
        #define Ame0 stateAsHalfLanes[32]
        #define Ame1 stateAsHalfLanes[33]
        #define Ami0 stateAsHalfLanes[34]
        #define Ami1 stateAsHalfLanes[35]
        #define Amo0 stateAsHalfLanes[36]
        #define Amo1 stateAsHalfLanes[37]
        #define Amu0 stateAsHalfLanes[38]
        #define Amu1 stateAsHalfLanes[39]
        #define Asa0 stateAsHalfLanes[40]
        #define Asa1 stateAsHalfLanes[41]
        #define Ase0 stateAsHalfLanes[42]
        #define Ase1 stateAsHalfLanes[43]
        #define Asi0 stateAsHalfLanes[44]
        #define Asi1 stateAsHalfLanes[45]
        #define Aso0 stateAsHalfLanes[46]
        #define Aso1 stateAsHalfLanes[47]
        #define Asu0 stateAsHalfLanes[48]
        #define Asu1 stateAsHalfLanes[49]

        do
        {
            /* --- Code for 4 rounds */

            /* --- using factor 2 interleaving, 64-bit lanes mapped to 32-bit words */

            KeccakAtoD_round0();

            Ba = (Aba0^Da0);
            Be = ROL32((Age0^De0), 22);
            Bi = ROL32((Aki1^Di1), 22);
            Bo = ROL32((Amo1^Do1), 11);
            Bu = ROL32((Asu0^Du0), 7);
            Aba0 =   Ba ^((~Be)&  Bi );
            Aba0 ^= *(pRoundConstants++);
            Age0 =   Be ^((~Bi)&  Bo );
            Aki1 =   Bi ^((~Bo)&  Bu );
            Amo1 =   Bo ^((~Bu)&  Ba );
            Asu0 =   Bu ^((~Ba)&  Be );

            Ba = (Aba1^Da1);
            Be = ROL32((Age1^De1), 22);
            Bi = ROL32((Aki0^Di0), 21);
            Bo = ROL32((Amo0^Do0), 10);
            Bu = ROL32((Asu1^Du1), 7);
            Aba1 =   Ba ^((~Be)&  Bi );
            Aba1 ^= *(pRoundConstants++);
            Age1 =   Be ^((~Bi)&  Bo );
            Aki0 =   Bi ^((~Bo)&  Bu );
            Amo0 =   Bo ^((~Bu)&  Ba );
            Asu1 =   Bu ^((~Ba)&  Be );

            Bi = ROL32((Aka1^Da1), 2);
            Bo = ROL32((Ame1^De1), 23);
            Bu = ROL32((Asi1^Di1), 31);
            Ba = ROL32((Abo0^Do0), 14);
            Be = ROL32((Agu0^Du0), 10);
            Aka1 =   Ba ^((~Be)&  Bi );
            Ame1 =   Be ^((~Bi)&  Bo );
            Asi1 =   Bi ^((~Bo)&  Bu );
            Abo0 =   Bo ^((~Bu)&  Ba );
            Agu0 =   Bu ^((~Ba)&  Be );

            Bi = ROL32((Aka0^Da0), 1);
            Bo = ROL32((Ame0^De0), 22);
            Bu = ROL32((Asi0^Di0), 30);
            Ba = ROL32((Abo1^Do1), 14);
            Be = ROL32((Agu1^Du1), 10);
            Aka0 =   Ba ^((~Be)&  Bi );
            Ame0 =   Be ^((~Bi)&  Bo );
            Asi0 =   Bi ^((~Bo)&  Bu );
            Abo1 =   Bo ^((~Bu)&  Ba );
            Agu1 =   Bu ^((~Ba)&  Be );

            Bu = ROL32((Asa0^Da0), 9);
            Ba = ROL32((Abe1^De1), 1);
            Be = ROL32((Agi0^Di0), 3);
            Bi = ROL32((Ako1^Do1), 13);
            Bo = ROL32((Amu0^Du0), 4);
            Asa0 =   Ba ^((~Be)&  Bi );
            Abe1 =   Be ^((~Bi)&  Bo );
            Agi0 =   Bi ^((~Bo)&  Bu );
            Ako1 =   Bo ^((~Bu)&  Ba );
            Amu0 =   Bu ^((~Ba)&  Be );

            Bu = ROL32((Asa1^Da1), 9);
            Ba = (Abe0^De0);
            Be = ROL32((Agi1^Di1), 3);
            Bi = ROL32((Ako0^Do0), 12);
            Bo = ROL32((Amu1^Du1), 4);
            Asa1 =   Ba ^((~Be)&  Bi );
            Abe0 =   Be ^((~Bi)&  Bo );
            Agi1 =   Bi ^((~Bo)&  Bu );
            Ako0 =   Bo ^((~Bu)&  Ba );
            Amu1 =   Bu ^((~Ba)&  Be );

            Be = ROL32((Aga0^Da0), 18);
            Bi = ROL32((Ake0^De0), 5);
            Bo = ROL32((Ami1^Di1), 8);
            Bu = ROL32((Aso0^Do0), 28);
            Ba = ROL32((Abu1^Du1), 14);
            Aga0 =   Ba ^((~Be)&  Bi );
            Ake0 =   Be ^((~Bi)&  Bo );
            Ami1 =   Bi ^((~Bo)&  Bu );
            Aso0 =   Bo ^((~Bu)&  Ba );
            Abu1 =   Bu ^((~Ba)&  Be );

            Be = ROL32((Aga1^Da1), 18);
            Bi = ROL32((Ake1^De1), 5);
            Bo = ROL32((Ami0^Di0), 7);
            Bu = ROL32((Aso1^Do1), 28);
            Ba = ROL32((Abu0^Du0), 13);
            Aga1 =   Ba ^((~Be)&  Bi );
            Ake1 =   Be ^((~Bi)&  Bo );
            Ami0 =   Bi ^((~Bo)&  Bu );
            Aso1 =   Bo ^((~Bu)&  Ba );
            Abu0 =   Bu ^((~Ba)&  Be );

            Bo = ROL32((Ama1^Da1), 21);
            Bu = ROL32((Ase0^De0), 1);
            Ba = ROL32((Abi0^Di0), 31);
            Be = ROL32((Ago1^Do1), 28);
            Bi = ROL32((Aku1^Du1), 20);
            Ama1 =   Ba ^((~Be)&  Bi );
            Ase0 =   Be ^((~Bi)&  Bo );
            Abi0 =   Bi ^((~Bo)&  Bu );
            Ago1 =   Bo ^((~Bu)&  Ba );
            Aku1 =   Bu ^((~Ba)&  Be );

            Bo = ROL32((Ama0^Da0), 20);
            Bu = ROL32((Ase1^De1), 1);
            Ba = ROL32((Abi1^Di1), 31);
            Be = ROL32((Ago0^Do0), 27);
            Bi = ROL32((Aku0^Du0), 19);
            Ama0 =   Ba ^((~Be)&  Bi );
            Ase1 =   Be ^((~Bi)&  Bo );
            Abi1 =   Bi ^((~Bo)&  Bu );
            Ago0 =   Bo ^((~Bu)&  Ba );
            Aku0 =   Bu ^((~Ba)&  Be );

            KeccakAtoD_round1();

            Ba = (Aba0^Da0);
            Be = ROL32((Ame1^De0), 22);
            Bi = ROL32((Agi1^Di1), 22);
            Bo = ROL32((Aso1^Do1), 11);
            Bu = ROL32((Aku1^Du0), 7);
            Aba0 =   Ba ^((~Be)&  Bi );
            Aba0 ^= *(pRoundConstants++);
            Ame1 =   Be ^((~Bi)&  Bo );
            Agi1 =   Bi ^((~Bo)&  Bu );
            Aso1 =   Bo ^((~Bu)&  Ba );
            Aku1 =   Bu ^((~Ba)&  Be );

            Ba = (Aba1^Da1);
            Be = ROL32((Ame0^De1), 22);
            Bi = ROL32((Agi0^Di0), 21);
            Bo = ROL32((Aso0^Do0), 10);
            Bu = ROL32((Aku0^Du1), 7);
            Aba1 =   Ba ^((~Be)&  Bi );
            Aba1 ^= *(pRoundConstants++);
            Ame0 =   Be ^((~Bi)&  Bo );
            Agi0 =   Bi ^((~Bo)&  Bu );
            Aso0 =   Bo ^((~Bu)&  Ba );
            Aku0 =   Bu ^((~Ba)&  Be );

            Bi = ROL32((Asa1^Da1), 2);
            Bo = ROL32((Ake1^De1), 23);
            Bu = ROL32((Abi1^Di1), 31);
            Ba = ROL32((Amo1^Do0), 14);
            Be = ROL32((Agu0^Du0), 10);
            Asa1 =   Ba ^((~Be)&  Bi );
            Ake1 =   Be ^((~Bi)&  Bo );
            Abi1 =   Bi ^((~Bo)&  Bu );
            Amo1 =   Bo ^((~Bu)&  Ba );
            Agu0 =   Bu ^((~Ba)&  Be );

            Bi = ROL32((Asa0^Da0), 1);
            Bo = ROL32((Ake0^De0), 22);
            Bu = ROL32((Abi0^Di0), 30);
            Ba = ROL32((Amo0^Do1), 14);
            Be = ROL32((Agu1^Du1), 10);
            Asa0 =   Ba ^((~Be)&  Bi );
            Ake0 =   Be ^((~Bi)&  Bo );
            Abi0 =   Bi ^((~Bo)&  Bu );
            Amo0 =   Bo ^((~Bu)&  Ba );
            Agu1 =   Bu ^((~Ba)&  Be );

            Bu = ROL32((Ama1^Da0), 9);
            Ba = ROL32((Age1^De1), 1);
            Be = ROL32((Asi1^Di0), 3);
            Bi = ROL32((Ako0^Do1), 13);
            Bo = ROL32((Abu1^Du0), 4);
            Ama1 =   Ba ^((~Be)&  Bi );
            Age1 =   Be ^((~Bi)&  Bo );
            Asi1 =   Bi ^((~Bo)&  Bu );
            Ako0 =   Bo ^((~Bu)&  Ba );
            Abu1 =   Bu ^((~Ba)&  Be );

            Bu = ROL32((Ama0^Da1), 9);
            Ba = (Age0^De0);
            Be = ROL32((Asi0^Di1), 3);
            Bi = ROL32((Ako1^Do0), 12);
            Bo = ROL32((Abu0^Du1), 4);
            Ama0 =   Ba ^((~Be)&  Bi );
            Age0 =   Be ^((~Bi)&  Bo );
            Asi0 =   Bi ^((~Bo)&  Bu );
            Ako1 =   Bo ^((~Bu)&  Ba );
            Abu0 =   Bu ^((~Ba)&  Be );

            Be = ROL32((Aka1^Da0), 18);
            Bi = ROL32((Abe1^De0), 5);
            Bo = ROL32((Ami0^Di1), 8);
            Bu = ROL32((Ago1^Do0), 28);
            Ba = ROL32((Asu1^Du1), 14);
            Aka1 =   Ba ^((~Be)&  Bi );
            Abe1 =   Be ^((~Bi)&  Bo );
            Ami0 =   Bi ^((~Bo)&  Bu );
            Ago1 =   Bo ^((~Bu)&  Ba );
            Asu1 =   Bu ^((~Ba)&  Be );

            Be = ROL32((Aka0^Da1), 18);
            Bi = ROL32((Abe0^De1), 5);
            Bo = ROL32((Ami1^Di0), 7);
            Bu = ROL32((Ago0^Do1), 28);
            Ba = ROL32((Asu0^Du0), 13);
            Aka0 =   Ba ^((~Be)&  Bi );
            Abe0 =   Be ^((~Bi)&  Bo );
            Ami1 =   Bi ^((~Bo)&  Bu );
            Ago0 =   Bo ^((~Bu)&  Ba );
            Asu0 =   Bu ^((~Ba)&  Be );

            Bo = ROL32((Aga1^Da1), 21);
            Bu = ROL32((Ase0^De0), 1);
            Ba = ROL32((Aki1^Di0), 31);
            Be = ROL32((Abo1^Do1), 28);
            Bi = ROL32((Amu1^Du1), 20);
            Aga1 =   Ba ^((~Be)&  Bi );
            Ase0 =   Be ^((~Bi)&  Bo );
            Aki1 =   Bi ^((~Bo)&  Bu );
            Abo1 =   Bo ^((~Bu)&  Ba );
            Amu1 =   Bu ^((~Ba)&  Be );

            Bo = ROL32((Aga0^Da0), 20);
            Bu = ROL32((Ase1^De1), 1);
            Ba = ROL32((Aki0^Di1), 31);
            Be = ROL32((Abo0^Do0), 27);
            Bi = ROL32((Amu0^Du0), 19);
            Aga0 =   Ba ^((~Be)&  Bi );
            Ase1 =   Be ^((~Bi)&  Bo );
            Aki0 =   Bi ^((~Bo)&  Bu );
            Abo0 =   Bo ^((~Bu)&  Ba );
            Amu0 =   Bu ^((~Ba)&  Be );

            KeccakAtoD_round2();

            Ba = (Aba0^Da0);
            Be = ROL32((Ake1^De0), 22);
            Bi = ROL32((Asi0^Di1), 22);
            Bo = ROL32((Ago0^Do1), 11);
            Bu = ROL32((Amu1^Du0), 7);
            Aba0 =   Ba ^((~Be)&  Bi );
            Aba0 ^= *(pRoundConstants++);
            Ake1 =   Be ^((~Bi)&  Bo );
            Asi0 =   Bi ^((~Bo)&  Bu );
            Ago0 =   Bo ^((~Bu)&  Ba );
            Amu1 =   Bu ^((~Ba)&  Be );

            Ba = (Aba1^Da1);
            Be = ROL32((Ake0^De1), 22);
            Bi = ROL32((Asi1^Di0), 21);
            Bo = ROL32((Ago1^Do0), 10);
            Bu = ROL32((Amu0^Du1), 7);
            Aba1 =   Ba ^((~Be)&  Bi );
            Aba1 ^= *(pRoundConstants++);
            Ake0 =   Be ^((~Bi)&  Bo );
            Asi1 =   Bi ^((~Bo)&  Bu );
            Ago1 =   Bo ^((~Bu)&  Ba );
            Amu0 =   Bu ^((~Ba)&  Be );

            Bi = ROL32((Ama0^Da1), 2);
            Bo = ROL32((Abe0^De1), 23);
            Bu = ROL32((Aki0^Di1), 31);
            Ba = ROL32((Aso1^Do0), 14);
            Be = ROL32((Agu0^Du0), 10);
            Ama0 =   Ba ^((~Be)&  Bi );
            Abe0 =   Be ^((~Bi)&  Bo );
            Aki0 =   Bi ^((~Bo)&  Bu );
            Aso1 =   Bo ^((~Bu)&  Ba );
            Agu0 =   Bu ^((~Ba)&  Be );

            Bi = ROL32((Ama1^Da0), 1);
            Bo = ROL32((Abe1^De0), 22);
            Bu = ROL32((Aki1^Di0), 30);
            Ba = ROL32((Aso0^Do1), 14);
            Be = ROL32((Agu1^Du1), 10);
            Ama1 =   Ba ^((~Be)&  Bi );
            Abe1 =   Be ^((~Bi)&  Bo );
            Aki1 =   Bi ^((~Bo)&  Bu );
            Aso0 =   Bo ^((~Bu)&  Ba );
            Agu1 =   Bu ^((~Ba)&  Be );

            Bu = ROL32((Aga1^Da0), 9);
            Ba = ROL32((Ame0^De1), 1);
            Be = ROL32((Abi1^Di0), 3);
            Bi = ROL32((Ako1^Do1), 13);
            Bo = ROL32((Asu1^Du0), 4);
            Aga1 =   Ba ^((~Be)&  Bi );
            Ame0 =   Be ^((~Bi)&  Bo );
            Abi1 =   Bi ^((~Bo)&  Bu );
            Ako1 =   Bo ^((~Bu)&  Ba );
            Asu1 =   Bu ^((~Ba)&  Be );

            Bu = ROL32((Aga0^Da1), 9);
            Ba = (Ame1^De0);
            Be = ROL32((Abi0^Di1), 3);
            Bi = ROL32((Ako0^Do0), 12);
            Bo = ROL32((Asu0^Du1), 4);
            Aga0 =   Ba ^((~Be)&  Bi );
            Ame1 =   Be ^((~Bi)&  Bo );
            Abi0 =   Bi ^((~Bo)&  Bu );
            Ako0 =   Bo ^((~Bu)&  Ba );
            Asu0 =   Bu ^((~Ba)&  Be );

            Be = ROL32((Asa1^Da0), 18);
            Bi = ROL32((Age1^De0), 5);
            Bo = ROL32((Ami1^Di1), 8);
            Bu = ROL32((Abo1^Do0), 28);
            Ba = ROL32((Aku0^Du1), 14);
            Asa1 =   Ba ^((~Be)&  Bi );
            Age1 =   Be ^((~Bi)&  Bo );
            Ami1 =   Bi ^((~Bo)&  Bu );
            Abo1 =   Bo ^((~Bu)&  Ba );
            Aku0 =   Bu ^((~Ba)&  Be );

            Be = ROL32((Asa0^Da1), 18);
            Bi = ROL32((Age0^De1), 5);
            Bo = ROL32((Ami0^Di0), 7);
            Bu = ROL32((Abo0^Do1), 28);
            Ba = ROL32((Aku1^Du0), 13);
            Asa0 =   Ba ^((~Be)&  Bi );
            Age0 =   Be ^((~Bi)&  Bo );
            Ami0 =   Bi ^((~Bo)&  Bu );
            Abo0 =   Bo ^((~Bu)&  Ba );
            Aku1 =   Bu ^((~Ba)&  Be );

            Bo = ROL32((Aka0^Da1), 21);
            Bu = ROL32((Ase0^De0), 1);
            Ba = ROL32((Agi1^Di0), 31);
            Be = ROL32((Amo0^Do1), 28);
            Bi = ROL32((Abu0^Du1), 20);
            Aka0 =   Ba ^((~Be)&  Bi );
            Ase0 =   Be ^((~Bi)&  Bo );
            Agi1 =   Bi ^((~Bo)&  Bu );
            Amo0 =   Bo ^((~Bu)&  Ba );
            Abu0 =   Bu ^((~Ba)&  Be );

            Bo = ROL32((Aka1^Da0), 20);
            Bu = ROL32((Ase1^De1), 1);
            Ba = ROL32((Agi0^Di1), 31);
            Be = ROL32((Amo1^Do0), 27);
            Bi = ROL32((Abu1^Du0), 19);
            Aka1 =   Ba ^((~Be)&  Bi );
            Ase1 =   Be ^((~Bi)&  Bo );
            Agi0 =   Bi ^((~Bo)&  Bu );
            Amo1 =   Bo ^((~Bu)&  Ba );
            Abu1 =   Bu ^((~Ba)&  Be );

            KeccakAtoD_round3();

            Ba = (Aba0^Da0);
            Be = ROL32((Abe0^De0), 22);
            Bi = ROL32((Abi0^Di1), 22);
            Bo = ROL32((Abo0^Do1), 11);
            Bu = ROL32((Abu0^Du0), 7);
            Aba0 =   Ba ^((~Be)&  Bi );
            Aba0 ^= *(pRoundConstants++);
            Abe0 =   Be ^((~Bi)&  Bo );
            Abi0 =   Bi ^((~Bo)&  Bu );
            Abo0 =   Bo ^((~Bu)&  Ba );
            Abu0 =   Bu ^((~Ba)&  Be );

            Ba = (Aba1^Da1);
            Be = ROL32((Abe1^De1), 22);
            Bi = ROL32((Abi1^Di0), 21);
            Bo = ROL32((Abo1^Do0), 10);
            Bu = ROL32((Abu1^Du1), 7);
            Aba1 =   Ba ^((~Be)&  Bi );
            Aba1 ^= *(pRoundConstants++);
            Abe1 =   Be ^((~Bi)&  Bo );
            Abi1 =   Bi ^((~Bo)&  Bu );
            Abo1 =   Bo ^((~Bu)&  Ba );
            Abu1 =   Bu ^((~Ba)&  Be );

            Bi = ROL32((Aga0^Da1), 2);
            Bo = ROL32((Age0^De1), 23);
            Bu = ROL32((Agi0^Di1), 31);
            Ba = ROL32((Ago0^Do0), 14);
            Be = ROL32((Agu0^Du0), 10);
            Aga0 =   Ba ^((~Be)&  Bi );
            Age0 =   Be ^((~Bi)&  Bo );
            Agi0 =   Bi ^((~Bo)&  Bu );
            Ago0 =   Bo ^((~Bu)&  Ba );
            Agu0 =   Bu ^((~Ba)&  Be );

            Bi = ROL32((Aga1^Da0), 1);
            Bo = ROL32((Age1^De0), 22);
            Bu = ROL32((Agi1^Di0), 30);
            Ba = ROL32((Ago1^Do1), 14);
            Be = ROL32((Agu1^Du1), 10);
            Aga1 =   Ba ^((~Be)&  Bi );
            Age1 =   Be ^((~Bi)&  Bo );
            Agi1 =   Bi ^((~Bo)&  Bu );
            Ago1 =   Bo ^((~Bu)&  Ba );
            Agu1 =   Bu ^((~Ba)&  Be );

            Bu = ROL32((Aka0^Da0), 9);
            Ba = ROL32((Ake0^De1), 1);
            Be = ROL32((Aki0^Di0), 3);
            Bi = ROL32((Ako0^Do1), 13);
            Bo = ROL32((Aku0^Du0), 4);
            Aka0 =   Ba ^((~Be)&  Bi );
            Ake0 =   Be ^((~Bi)&  Bo );
            Aki0 =   Bi ^((~Bo)&  Bu );
            Ako0 =   Bo ^((~Bu)&  Ba );
            Aku0 =   Bu ^((~Ba)&  Be );

            Bu = ROL32((Aka1^Da1), 9);
            Ba = (Ake1^De0);
            Be = ROL32((Aki1^Di1), 3);
            Bi = ROL32((Ako1^Do0), 12);
            Bo = ROL32((Aku1^Du1), 4);
            Aka1 =   Ba ^((~Be)&  Bi );
            Ake1 =   Be ^((~Bi)&  Bo );
            Aki1 =   Bi ^((~Bo)&  Bu );
            Ako1 =   Bo ^((~Bu)&  Ba );
            Aku1 =   Bu ^((~Ba)&  Be );

            Be = ROL32((Ama0^Da0), 18);
            Bi = ROL32((Ame0^De0), 5);
            Bo = ROL32((Ami0^Di1), 8);
            Bu = ROL32((Amo0^Do0), 28);
            Ba = ROL32((Amu0^Du1), 14);
            Ama0 =   Ba ^((~Be)&  Bi );
            Ame0 =   Be ^((~Bi)&  Bo );
            Ami0 =   Bi ^((~Bo)&  Bu );
            Amo0 =   Bo ^((~Bu)&  Ba );
            Amu0 =   Bu ^((~Ba)&  Be );

            Be = ROL32((Ama1^Da1), 18);
            Bi = ROL32((Ame1^De1), 5);
            Bo = ROL32((Ami1^Di0), 7);
            Bu = ROL32((Amo1^Do1), 28);
            Ba = ROL32((Amu1^Du0), 13);
            Ama1 =   Ba ^((~Be)&  Bi );
            Ame1 =   Be ^((~Bi)&  Bo );
            Ami1 =   Bi ^((~Bo)&  Bu );
            Amo1 =   Bo ^((~Bu)&  Ba );
            Amu1 =   Bu ^((~Ba)&  Be );

            Bo = ROL32((Asa0^Da1), 21);
            Bu = ROL32((Ase0^De0), 1);
            Ba = ROL32((Asi0^Di0), 31);
            Be = ROL32((Aso0^Do1), 28);
            Bi = ROL32((Asu0^Du1), 20);
            Asa0 =   Ba ^((~Be)&  Bi );
            Ase0 =   Be ^((~Bi)&  Bo );
            Asi0 =   Bi ^((~Bo)&  Bu );
            Aso0 =   Bo ^((~Bu)&  Ba );
            Asu0 =   Bu ^((~Ba)&  Be );

            Bo = ROL32((Asa1^Da0), 20);
            Bu = ROL32((Ase1^De1), 1);
            Ba = ROL32((Asi1^Di1), 31);
            Be = ROL32((Aso1^Do0), 27);
            Bi = ROL32((Asu1^Du0), 19);
            Asa1 =   Ba ^((~Be)&  Bi );
            Ase1 =   Be ^((~Bi)&  Bo );
            Asi1 =   Bi ^((~Bo)&  Bu );
            Aso1 =   Bo ^((~Bu)&  Ba );
            Asu1 =   Bu ^((~Ba)&  Be );
        }
        while ( *pRoundConstants != 0xFF );

        #undef Aba0
        #undef Aba1
        #undef Abe0
        #undef Abe1
        #undef Abi0
        #undef Abi1
        #undef Abo0
        #undef Abo1
        #undef Abu0
        #undef Abu1
        #undef Aga0
        #undef Aga1
        #undef Age0
        #undef Age1
        #undef Agi0
        #undef Agi1
        #undef Ago0
        #undef Ago1
        #undef Agu0
        #undef Agu1
        #undef Aka0
        #undef Aka1
        #undef Ake0
        #undef Ake1
        #undef Aki0
        #undef Aki1
        #undef Ako0
        #undef Ako1
        #undef Aku0
        #undef Aku1
        #undef Ama0
        #undef Ama1
        #undef Ame0
        #undef Ame1
        #undef Ami0
        #undef Ami1
        #undef Amo0
        #undef Amo1
        #undef Amu0
        #undef Amu1
        #undef Asa0
        #undef Asa1
        #undef Ase0
        #undef Ase1
        #undef Asi0
        #undef Asi1
        #undef Aso0
        #undef Aso1
        #undef Asu0
        #undef Asu1
    }
}

/* ---------------------------------------------------------------- */

void KeccakP1600_Permute_12rounds(void *state)
{
     KeccakP1600_Permute_Nrounds(state, 12);
}

/* ---------------------------------------------------------------- */

void KeccakP1600_Permute_24rounds(void *state)
{
     KeccakP1600_Permute_Nrounds(state, 24);
}
