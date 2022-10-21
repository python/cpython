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

#define JOIN0(a, b)                     a ## b
#define JOIN(a, b)                      JOIN0(a, b)

#define Sponge                          JOIN(prefix, _Sponge)
#define SpongeInstance                  JOIN(prefix, _SpongeInstance)
#define SpongeInitialize                JOIN(prefix, _SpongeInitialize)
#define SpongeAbsorb                    JOIN(prefix, _SpongeAbsorb)
#define SpongeAbsorbLastFewBits         JOIN(prefix, _SpongeAbsorbLastFewBits)
#define SpongeSqueeze                   JOIN(prefix, _SpongeSqueeze)

#define SnP_stateSizeInBytes            JOIN(SnP, _stateSizeInBytes)
#define SnP_stateAlignment              JOIN(SnP, _stateAlignment)
#define SnP_StaticInitialize            JOIN(SnP, _StaticInitialize)
#define SnP_Initialize                  JOIN(SnP, _Initialize)
#define SnP_AddByte                     JOIN(SnP, _AddByte)
#define SnP_AddBytes                    JOIN(SnP, _AddBytes)
#define SnP_ExtractBytes                JOIN(SnP, _ExtractBytes)

int Sponge(unsigned int rate, unsigned int capacity, const unsigned char *input, size_t inputByteLen, unsigned char suffix, unsigned char *output, size_t outputByteLen)
{
    ALIGN(SnP_stateAlignment) unsigned char state[SnP_stateSizeInBytes];
    unsigned int partialBlock;
    const unsigned char *curInput = input;
    unsigned char *curOutput = output;
    unsigned int rateInBytes = rate/8;

    if (rate+capacity != SnP_width)
        return 1;
    if ((rate <= 0) || (rate > SnP_width) || ((rate % 8) != 0))
        return 1;
    if (suffix == 0)
        return 1;

    /* Initialize the state */

    SnP_StaticInitialize();
    SnP_Initialize(state);

    /* First, absorb whole blocks */

#ifdef SnP_FastLoop_Absorb
    if (((rateInBytes % (SnP_width/200)) == 0) && (inputByteLen >= rateInBytes)) {
        /* fast lane: whole lane rate */

        size_t j;
        j = SnP_FastLoop_Absorb(state, rateInBytes/(SnP_width/200), curInput, inputByteLen);
        curInput += j;
        inputByteLen -= j;
    }
#endif
    while(inputByteLen >= (size_t)rateInBytes) {
        #ifdef KeccakReference
        displayBytes(1, "Block to be absorbed", curInput, rateInBytes);
        #endif
        SnP_AddBytes(state, curInput, 0, rateInBytes);
        SnP_Permute(state);
        curInput += rateInBytes;
        inputByteLen -= rateInBytes;
    }

    /* Then, absorb what remains */

    partialBlock = (unsigned int)inputByteLen;
    #ifdef KeccakReference
    displayBytes(1, "Block to be absorbed (part)", curInput, partialBlock);
    #endif
    SnP_AddBytes(state, curInput, 0, partialBlock);

    /* Finally, absorb the suffix */

    #ifdef KeccakReference
    {
        unsigned char delimitedData1[1];
        delimitedData1[0] = suffix;
        displayBytes(1, "Block to be absorbed (last few bits + first bit of padding)", delimitedData1, 1);
    }
    #endif
    /* Last few bits, whose delimiter coincides with first bit of padding */

    SnP_AddByte(state, suffix, partialBlock);
    /* If the first bit of padding is at position rate-1, we need a whole new block for the second bit of padding */

    if ((suffix >= 0x80) && (partialBlock == (rateInBytes-1)))
        SnP_Permute(state);
    /* Second bit of padding */

    SnP_AddByte(state, 0x80, rateInBytes-1);
    #ifdef KeccakReference
    {
        unsigned char block[SnP_width/8];
        memset(block, 0, SnP_width/8);
        block[rateInBytes-1] = 0x80;
        displayBytes(1, "Second bit of padding", block, rateInBytes);
    }
    #endif
    SnP_Permute(state);
    #ifdef KeccakReference
    displayText(1, "--- Switching to squeezing phase ---");
    #endif

    /* First, output whole blocks */

    while(outputByteLen > (size_t)rateInBytes) {
        SnP_ExtractBytes(state, curOutput, 0, rateInBytes);
        SnP_Permute(state);
        #ifdef KeccakReference
        displayBytes(1, "Squeezed block", curOutput, rateInBytes);
        #endif
        curOutput += rateInBytes;
        outputByteLen -= rateInBytes;
    }

    /* Finally, output what remains */

    partialBlock = (unsigned int)outputByteLen;
    SnP_ExtractBytes(state, curOutput, 0, partialBlock);
    #ifdef KeccakReference
    displayBytes(1, "Squeezed block (part)", curOutput, partialBlock);
    #endif

    return 0;
}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

int SpongeInitialize(SpongeInstance *instance, unsigned int rate, unsigned int capacity)
{
    if (rate+capacity != SnP_width)
        return 1;
    if ((rate <= 0) || (rate > SnP_width) || ((rate % 8) != 0))
        return 1;
    SnP_StaticInitialize();
    SnP_Initialize(instance->state);
    instance->rate = rate;
    instance->byteIOIndex = 0;
    instance->squeezing = 0;

    return 0;
}

/* ---------------------------------------------------------------- */

int SpongeAbsorb(SpongeInstance *instance, const unsigned char *data, size_t dataByteLen)
{
    size_t i, j;
    unsigned int partialBlock;
    const unsigned char *curData;
    unsigned int rateInBytes = instance->rate/8;

    if (instance->squeezing)
        return 1; /* Too late for additional input */


    i = 0;
    curData = data;
    while(i < dataByteLen) {
        if ((instance->byteIOIndex == 0) && (dataByteLen-i >= rateInBytes)) {
#ifdef SnP_FastLoop_Absorb
            /* processing full blocks first */

            if ((rateInBytes % (SnP_width/200)) == 0) {
                /* fast lane: whole lane rate */

                j = SnP_FastLoop_Absorb(instance->state, rateInBytes/(SnP_width/200), curData, dataByteLen - i);
                i += j;
                curData += j;
            }
            else {
#endif
                for(j=dataByteLen-i; j>=rateInBytes; j-=rateInBytes) {
                    #ifdef KeccakReference
                    displayBytes(1, "Block to be absorbed", curData, rateInBytes);
                    #endif
                    SnP_AddBytes(instance->state, curData, 0, rateInBytes);
                    SnP_Permute(instance->state);
                    curData+=rateInBytes;
                }
                i = dataByteLen - j;
#ifdef SnP_FastLoop_Absorb
            }
#endif
        }
        else {
            /* normal lane: using the message queue */
            if (dataByteLen-i > rateInBytes-instance->byteIOIndex)
                partialBlock = rateInBytes-instance->byteIOIndex;
            else
                partialBlock = (unsigned int)(dataByteLen - i);
            #ifdef KeccakReference
            displayBytes(1, "Block to be absorbed (part)", curData, partialBlock);
            #endif
            i += partialBlock;

            SnP_AddBytes(instance->state, curData, instance->byteIOIndex, partialBlock);
            curData += partialBlock;
            instance->byteIOIndex += partialBlock;
            if (instance->byteIOIndex == rateInBytes) {
                SnP_Permute(instance->state);
                instance->byteIOIndex = 0;
            }
        }
    }
    return 0;
}

/* ---------------------------------------------------------------- */

int SpongeAbsorbLastFewBits(SpongeInstance *instance, unsigned char delimitedData)
{
    unsigned int rateInBytes = instance->rate/8;

    if (delimitedData == 0)
        return 1;
    if (instance->squeezing)
        return 1; /* Too late for additional input */


    #ifdef KeccakReference
    {
        unsigned char delimitedData1[1];
        delimitedData1[0] = delimitedData;
        displayBytes(1, "Block to be absorbed (last few bits + first bit of padding)", delimitedData1, 1);
    }
    #endif
    /* Last few bits, whose delimiter coincides with first bit of padding */

    SnP_AddByte(instance->state, delimitedData, instance->byteIOIndex);
    /* If the first bit of padding is at position rate-1, we need a whole new block for the second bit of padding */

    if ((delimitedData >= 0x80) && (instance->byteIOIndex == (rateInBytes-1)))
        SnP_Permute(instance->state);
    /* Second bit of padding */

    SnP_AddByte(instance->state, 0x80, rateInBytes-1);
    #ifdef KeccakReference
    {
        unsigned char block[SnP_width/8];
        memset(block, 0, SnP_width/8);
        block[rateInBytes-1] = 0x80;
        displayBytes(1, "Second bit of padding", block, rateInBytes);
    }
    #endif
    SnP_Permute(instance->state);
    instance->byteIOIndex = 0;
    instance->squeezing = 1;
    #ifdef KeccakReference
    displayText(1, "--- Switching to squeezing phase ---");
    #endif
    return 0;
}

/* ---------------------------------------------------------------- */

int SpongeSqueeze(SpongeInstance *instance, unsigned char *data, size_t dataByteLen)
{
    size_t i, j;
    unsigned int partialBlock;
    unsigned int rateInBytes = instance->rate/8;
    unsigned char *curData;

    if (!instance->squeezing)
        SpongeAbsorbLastFewBits(instance, 0x01);

    i = 0;
    curData = data;
    while(i < dataByteLen) {
        if ((instance->byteIOIndex == rateInBytes) && (dataByteLen-i >= rateInBytes)) {
            for(j=dataByteLen-i; j>=rateInBytes; j-=rateInBytes) {
                SnP_Permute(instance->state);
                SnP_ExtractBytes(instance->state, curData, 0, rateInBytes);
                #ifdef KeccakReference
                displayBytes(1, "Squeezed block", curData, rateInBytes);
                #endif
                curData+=rateInBytes;
            }
            i = dataByteLen - j;
        }
        else {
            /* normal lane: using the message queue */

            if (instance->byteIOIndex == rateInBytes) {
                SnP_Permute(instance->state);
                instance->byteIOIndex = 0;
            }
            if (dataByteLen-i > rateInBytes-instance->byteIOIndex)
                partialBlock = rateInBytes-instance->byteIOIndex;
            else
                partialBlock = (unsigned int)(dataByteLen - i);
            i += partialBlock;

            SnP_ExtractBytes(instance->state, curData, instance->byteIOIndex, partialBlock);
            #ifdef KeccakReference
            displayBytes(1, "Squeezed block (part)", curData, partialBlock);
            #endif
            curData += partialBlock;
            instance->byteIOIndex += partialBlock;
        }
    }
    return 0;
}

/* ---------------------------------------------------------------- */

#undef Sponge
#undef SpongeInstance
#undef SpongeInitialize
#undef SpongeAbsorb
#undef SpongeAbsorbLastFewBits
#undef SpongeSqueeze
#undef SnP_stateSizeInBytes
#undef SnP_stateAlignment
#undef SnP_StaticInitialize
#undef SnP_Initialize
#undef SnP_AddByte
#undef SnP_AddBytes
#undef SnP_ExtractBytes
