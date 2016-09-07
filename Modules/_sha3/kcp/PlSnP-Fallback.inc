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

/* expect PlSnP_baseParallelism, PlSnP_targetParallelism */

/* expect SnP_stateSizeInBytes, SnP_stateAlignment */

/* expect prefix */

/* expect SnP_* */


#define JOIN0(a, b)                     a ## b
#define JOIN(a, b)                      JOIN0(a, b)

#define PlSnP_StaticInitialize          JOIN(prefix, _StaticInitialize)
#define PlSnP_InitializeAll             JOIN(prefix, _InitializeAll)
#define PlSnP_AddByte                   JOIN(prefix, _AddByte)
#define PlSnP_AddBytes                  JOIN(prefix, _AddBytes)
#define PlSnP_AddLanesAll               JOIN(prefix, _AddLanesAll)
#define PlSnP_OverwriteBytes            JOIN(prefix, _OverwriteBytes)
#define PlSnP_OverwriteLanesAll         JOIN(prefix, _OverwriteLanesAll)
#define PlSnP_OverwriteWithZeroes       JOIN(prefix, _OverwriteWithZeroes)
#define PlSnP_ExtractBytes              JOIN(prefix, _ExtractBytes)
#define PlSnP_ExtractLanesAll           JOIN(prefix, _ExtractLanesAll)
#define PlSnP_ExtractAndAddBytes        JOIN(prefix, _ExtractAndAddBytes)
#define PlSnP_ExtractAndAddLanesAll     JOIN(prefix, _ExtractAndAddLanesAll)

#if (PlSnP_baseParallelism == 1)
    #define SnP_stateSizeInBytes            JOIN(SnP, _stateSizeInBytes)
    #define SnP_stateAlignment              JOIN(SnP, _stateAlignment)
#else
    #define SnP_stateSizeInBytes            JOIN(SnP, _statesSizeInBytes)
    #define SnP_stateAlignment              JOIN(SnP, _statesAlignment)
#endif
#define PlSnP_factor ((PlSnP_targetParallelism)/(PlSnP_baseParallelism))
#define SnP_stateOffset (((SnP_stateSizeInBytes+(SnP_stateAlignment-1))/SnP_stateAlignment)*SnP_stateAlignment)
#define stateWithIndex(i) ((unsigned char *)states+((i)*SnP_stateOffset))

#define SnP_StaticInitialize            JOIN(SnP, _StaticInitialize)
#define SnP_Initialize                  JOIN(SnP, _Initialize)
#define SnP_InitializeAll               JOIN(SnP, _InitializeAll)
#define SnP_AddByte                     JOIN(SnP, _AddByte)
#define SnP_AddBytes                    JOIN(SnP, _AddBytes)
#define SnP_AddLanesAll                 JOIN(SnP, _AddLanesAll)
#define SnP_OverwriteBytes              JOIN(SnP, _OverwriteBytes)
#define SnP_OverwriteLanesAll           JOIN(SnP, _OverwriteLanesAll)
#define SnP_OverwriteWithZeroes         JOIN(SnP, _OverwriteWithZeroes)
#define SnP_ExtractBytes                JOIN(SnP, _ExtractBytes)
#define SnP_ExtractLanesAll             JOIN(SnP, _ExtractLanesAll)
#define SnP_ExtractAndAddBytes          JOIN(SnP, _ExtractAndAddBytes)
#define SnP_ExtractAndAddLanesAll       JOIN(SnP, _ExtractAndAddLanesAll)

void PlSnP_StaticInitialize( void )
{
    SnP_StaticInitialize();
}

void PlSnP_InitializeAll(void *states)
{
    unsigned int i;

    for(i=0; i<PlSnP_factor; i++)
    #if (PlSnP_baseParallelism == 1)
        SnP_Initialize(stateWithIndex(i));
    #else
        SnP_InitializeAll(stateWithIndex(i));
    #endif
}

void PlSnP_AddByte(void *states, unsigned int instanceIndex, unsigned char byte, unsigned int offset)
{
    #if (PlSnP_baseParallelism == 1)
        SnP_AddByte(stateWithIndex(instanceIndex), byte, offset);
    #else
        SnP_AddByte(stateWithIndex(instanceIndex/PlSnP_baseParallelism), instanceIndex%PlSnP_baseParallelism, byte, offset);
    #endif
}

void PlSnP_AddBytes(void *states, unsigned int instanceIndex, const unsigned char *data, unsigned int offset, unsigned int length)
{
    #if (PlSnP_baseParallelism == 1)
        SnP_AddBytes(stateWithIndex(instanceIndex), data, offset, length);
    #else
        SnP_AddBytes(stateWithIndex(instanceIndex/PlSnP_baseParallelism), instanceIndex%PlSnP_baseParallelism, data, offset, length);
    #endif
}

void PlSnP_AddLanesAll(void *states, const unsigned char *data, unsigned int laneCount, unsigned int laneOffset)
{
    unsigned int i;

    for(i=0; i<PlSnP_factor; i++) {
        #if (PlSnP_baseParallelism == 1)
            SnP_AddBytes(stateWithIndex(i), data, 0, laneCount*SnP_laneLengthInBytes);
        #else
            SnP_AddLanesAll(stateWithIndex(i), data, laneCount, laneOffset);
        #endif
        data += PlSnP_baseParallelism*laneOffset*SnP_laneLengthInBytes;
    }
}

void PlSnP_OverwriteBytes(void *states, unsigned int instanceIndex, const unsigned char *data, unsigned int offset, unsigned int length)
{
    #if (PlSnP_baseParallelism == 1)
        SnP_OverwriteBytes(stateWithIndex(instanceIndex), data, offset, length);
    #else
        SnP_OverwriteBytes(stateWithIndex(instanceIndex/PlSnP_baseParallelism), instanceIndex%PlSnP_baseParallelism, data, offset, length);
    #endif
}

void PlSnP_OverwriteLanesAll(void *states, const unsigned char *data, unsigned int laneCount, unsigned int laneOffset)
{
    unsigned int i;

    for(i=0; i<PlSnP_factor; i++) {
        #if (PlSnP_baseParallelism == 1)
            SnP_OverwriteBytes(stateWithIndex(i), data, 0, laneCount*SnP_laneLengthInBytes);
        #else
            SnP_OverwriteLanesAll(stateWithIndex(i), data, laneCount, laneOffset);
        #endif
        data += PlSnP_baseParallelism*laneOffset*SnP_laneLengthInBytes;
    }
}

void PlSnP_OverwriteWithZeroes(void *states, unsigned int instanceIndex, unsigned int byteCount)
{
    #if (PlSnP_baseParallelism == 1)
        SnP_OverwriteWithZeroes(stateWithIndex(instanceIndex), byteCount);
    #else
        SnP_OverwriteWithZeroes(stateWithIndex(instanceIndex/PlSnP_baseParallelism), instanceIndex%PlSnP_baseParallelism, byteCount);
    #endif
}

void PlSnP_PermuteAll(void *states)
{
    unsigned int i;

    for(i=0; i<PlSnP_factor; i++) {
        #if (PlSnP_baseParallelism == 1)
            SnP_Permute(stateWithIndex(i));
        #else
            SnP_PermuteAll(stateWithIndex(i));
        #endif
    }
}

#if (defined(SnP_Permute_12rounds) || defined(SnP_PermuteAll_12rounds))
void PlSnP_PermuteAll_12rounds(void *states)
{
    unsigned int i;

    for(i=0; i<PlSnP_factor; i++) {
        #if (PlSnP_baseParallelism == 1)
            SnP_Permute_12rounds(stateWithIndex(i));
        #else
            SnP_PermuteAll_12rounds(stateWithIndex(i));
        #endif
    }
}
#endif

void PlSnP_ExtractBytes(void *states, unsigned int instanceIndex, unsigned char *data, unsigned int offset, unsigned int length)
{
    #if (PlSnP_baseParallelism == 1)
        SnP_ExtractBytes(stateWithIndex(instanceIndex), data, offset, length);
    #else
        SnP_ExtractBytes(stateWithIndex(instanceIndex/PlSnP_baseParallelism), instanceIndex%PlSnP_baseParallelism, data, offset, length);
    #endif
}

void PlSnP_ExtractLanesAll(const void *states, unsigned char *data, unsigned int laneCount, unsigned int laneOffset)
{
    unsigned int i;

    for(i=0; i<PlSnP_factor; i++) {
        #if (PlSnP_baseParallelism == 1)
            SnP_ExtractBytes(stateWithIndex(i), data, 0, laneCount*SnP_laneLengthInBytes);
        #else
            SnP_ExtractLanesAll(stateWithIndex(i), data, laneCount, laneOffset);
        #endif
        data += laneOffset*SnP_laneLengthInBytes*PlSnP_baseParallelism;
    }
}

void PlSnP_ExtractAndAddBytes(void *states, unsigned int instanceIndex, const unsigned char *input, unsigned char *output, unsigned int offset, unsigned int length)
{
    #if (PlSnP_baseParallelism == 1)
        SnP_ExtractAndAddBytes(stateWithIndex(instanceIndex), input, output, offset, length);
    #else
        SnP_ExtractAndAddBytes(stateWithIndex(instanceIndex/PlSnP_baseParallelism), instanceIndex%PlSnP_baseParallelism, input, output, offset, length);
    #endif
}

void PlSnP_ExtractAndAddLanesAll(const void *states, const unsigned char *input, unsigned char *output, unsigned int laneCount, unsigned int laneOffset)
{
    unsigned int i;

    for(i=0; i<PlSnP_factor; i++) {
        #if (PlSnP_baseParallelism == 1)
            SnP_ExtractAndAddBytes(stateWithIndex(i), input, output, 0, laneCount*SnP_laneLengthInBytes);
        #else
            SnP_ExtractAndAddLanesAll(stateWithIndex(i), input, output, laneCount, laneOffset);
        #endif
        input += laneOffset*SnP_laneLengthInBytes*PlSnP_baseParallelism;
        output += laneOffset*SnP_laneLengthInBytes*PlSnP_baseParallelism;
    }
}

#undef PlSnP_factor
#undef SnP_stateOffset
#undef stateWithIndex
#undef JOIN0
#undef JOIN
#undef PlSnP_StaticInitialize
#undef PlSnP_InitializeAll
#undef PlSnP_AddByte
#undef PlSnP_AddBytes
#undef PlSnP_AddLanesAll
#undef PlSnP_OverwriteBytes
#undef PlSnP_OverwriteLanesAll
#undef PlSnP_OverwriteWithZeroes
#undef PlSnP_PermuteAll
#undef PlSnP_ExtractBytes
#undef PlSnP_ExtractLanesAll
#undef PlSnP_ExtractAndAddBytes
#undef PlSnP_ExtractAndAddLanesAll
#undef SnP_stateAlignment
#undef SnP_stateSizeInBytes
#undef PlSnP_factor
#undef SnP_stateOffset
#undef stateWithIndex
#undef SnP_StaticInitialize
#undef SnP_Initialize
#undef SnP_InitializeAll
#undef SnP_AddByte
#undef SnP_AddBytes
#undef SnP_AddLanesAll
#undef SnP_OverwriteBytes
#undef SnP_OverwriteWithZeroes
#undef SnP_OverwriteLanesAll
#undef SnP_ExtractBytes
#undef SnP_ExtractLanesAll
#undef SnP_ExtractAndAddBytes
#undef SnP_ExtractAndAddLanesAll
