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
#include "KeccakSponge.h"
#include "KeccakF-1600-interface.h"
#ifdef KeccakReference
#include "displayIntermediateValues.h"
#endif

static int InitSponge(spongeState *state, unsigned int rate, unsigned int capacity)
{
    if (rate+capacity != 1600)
        return 1;
    if ((rate <= 0) || (rate >= 1600) || ((rate % 64) != 0))
        return 1;
    KeccakInitialize();
    state->rate = rate;
    state->capacity = capacity;
    state->fixedOutputLength = 0;
    KeccakInitializeState(state->state);
    memset(state->dataQueue, 0, KeccakMaximumRateInBytes);
    state->bitsInQueue = 0;
    state->squeezing = 0;
    state->bitsAvailableForSqueezing = 0;

    return 0;
}

static void AbsorbQueue(spongeState *state)
{
    /*  state->bitsInQueue is assumed to be equal to state->rate */
    #ifdef KeccakReference
    displayBytes(1, "Block to be absorbed", state->dataQueue, state->rate/8);
    #endif
#ifdef ProvideFast576
    if (state->rate == 576)
        KeccakAbsorb576bits(state->state, state->dataQueue);
    else 
#endif
#ifdef ProvideFast832
    if (state->rate == 832)
        KeccakAbsorb832bits(state->state, state->dataQueue);
    else 
#endif
#ifdef ProvideFast1024
    if (state->rate == 1024)
        KeccakAbsorb1024bits(state->state, state->dataQueue);
    else 
#endif
#ifdef ProvideFast1088
    if (state->rate == 1088)
        KeccakAbsorb1088bits(state->state, state->dataQueue);
    else
#endif
#ifdef ProvideFast1152
    if (state->rate == 1152)
        KeccakAbsorb1152bits(state->state, state->dataQueue);
    else 
#endif
#ifdef ProvideFast1344
    if (state->rate == 1344)
        KeccakAbsorb1344bits(state->state, state->dataQueue);
    else 
#endif
        KeccakAbsorb(state->state, state->dataQueue, state->rate/64);
    state->bitsInQueue = 0;
}

static int Absorb(spongeState *state, const unsigned char *data, unsigned long long databitlen)
{
    unsigned long long i, j, wholeBlocks;
    unsigned int partialBlock, partialByte;
    const unsigned char *curData;

    if ((state->bitsInQueue % 8) != 0)
        return 1; /*  Only the last call may contain a partial byte */
    if (state->squeezing)
        return 1; /*  Too late for additional input */

    i = 0;
    while(i < databitlen) {
        if ((state->bitsInQueue == 0) && (databitlen >= state->rate) && (i <= (databitlen-state->rate))) {
            wholeBlocks = (databitlen-i)/state->rate;
            curData = data+i/8;
#ifdef ProvideFast576
            if (state->rate == 576) {
                for(j=0; j<wholeBlocks; j++, curData+=576/8) {
                    #ifdef KeccakReference
                    displayBytes(1, "Block to be absorbed", curData, state->rate/8);
                    #endif
                    KeccakAbsorb576bits(state->state, curData);
                }
            }
            else
#endif
#ifdef ProvideFast832
            if (state->rate == 832) {
                for(j=0; j<wholeBlocks; j++, curData+=832/8) {
                    #ifdef KeccakReference
                    displayBytes(1, "Block to be absorbed", curData, state->rate/8);
                    #endif
                    KeccakAbsorb832bits(state->state, curData);
                }
            }
            else
#endif
#ifdef ProvideFast1024
            if (state->rate == 1024) {
                for(j=0; j<wholeBlocks; j++, curData+=1024/8) {
                    #ifdef KeccakReference
                    displayBytes(1, "Block to be absorbed", curData, state->rate/8);
                    #endif
                    KeccakAbsorb1024bits(state->state, curData);
                }
            }
            else
#endif
#ifdef ProvideFast1088
            if (state->rate == 1088) {
                for(j=0; j<wholeBlocks; j++, curData+=1088/8) {
                    #ifdef KeccakReference
                    displayBytes(1, "Block to be absorbed", curData, state->rate/8);
                    #endif
                    KeccakAbsorb1088bits(state->state, curData);
                }
            }
            else
#endif
#ifdef ProvideFast1152
            if (state->rate == 1152) {
                for(j=0; j<wholeBlocks; j++, curData+=1152/8) {
                    #ifdef KeccakReference
                    displayBytes(1, "Block to be absorbed", curData, state->rate/8);
                    #endif
                    KeccakAbsorb1152bits(state->state, curData);
                }
            }
            else
#endif
#ifdef ProvideFast1344
            if (state->rate == 1344) {
                for(j=0; j<wholeBlocks; j++, curData+=1344/8) {
                    #ifdef KeccakReference
                    displayBytes(1, "Block to be absorbed", curData, state->rate/8);
                    #endif
                    KeccakAbsorb1344bits(state->state, curData);
                }
            }
            else
#endif
            {
                for(j=0; j<wholeBlocks; j++, curData+=state->rate/8) {
                    #ifdef KeccakReference
                    displayBytes(1, "Block to be absorbed", curData, state->rate/8);
                    #endif
                    KeccakAbsorb(state->state, curData, state->rate/64);
                }
            }
            i += wholeBlocks*state->rate;
        }
        else {
            partialBlock = (unsigned int)(databitlen - i);
            if (partialBlock+state->bitsInQueue > state->rate)
                partialBlock = state->rate-state->bitsInQueue;
            partialByte = partialBlock % 8;
            partialBlock -= partialByte;
            memcpy(state->dataQueue+state->bitsInQueue/8, data+i/8, partialBlock/8);
            state->bitsInQueue += partialBlock;
            i += partialBlock;
            if (state->bitsInQueue == state->rate)
                AbsorbQueue(state);
            if (partialByte > 0) {
                unsigned char mask = (1 << partialByte)-1;
                state->dataQueue[state->bitsInQueue/8] = data[i/8] & mask;
                state->bitsInQueue += partialByte;
                i += partialByte;
            }
        }
    }
    return 0;
}

static void PadAndSwitchToSqueezingPhase(spongeState *state)
{
    /*  Note: the bits are numbered from 0=LSB to 7=MSB */
    if (state->bitsInQueue + 1 == state->rate) {
        state->dataQueue[state->bitsInQueue/8 ] |= 1 << (state->bitsInQueue % 8);
        AbsorbQueue(state);
        memset(state->dataQueue, 0, state->rate/8);
    }
    else {
        memset(state->dataQueue + (state->bitsInQueue+7)/8, 0, state->rate/8 - (state->bitsInQueue+7)/8);
        state->dataQueue[state->bitsInQueue/8 ] |= 1 << (state->bitsInQueue % 8);
    }
    state->dataQueue[(state->rate-1)/8] |= 1 << ((state->rate-1) % 8);
    AbsorbQueue(state);

    #ifdef KeccakReference
    displayText(1, "--- Switching to squeezing phase ---");
    #endif
#ifdef ProvideFast1024
    if (state->rate == 1024) {
        KeccakExtract1024bits(state->state, state->dataQueue);
        state->bitsAvailableForSqueezing = 1024;
    }
    else
#endif
    {
        KeccakExtract(state->state, state->dataQueue, state->rate/64);
        state->bitsAvailableForSqueezing = state->rate;
    }
    #ifdef KeccakReference
    displayBytes(1, "Block available for squeezing", state->dataQueue, state->bitsAvailableForSqueezing/8);
    #endif
    state->squeezing = 1;
}

static int Squeeze(spongeState *state, unsigned char *output, unsigned long long outputLength)
{
    unsigned long long i;
    unsigned int partialBlock;

    if (!state->squeezing)
        PadAndSwitchToSqueezingPhase(state);
    if ((outputLength % 8) != 0)
        return 1; /*  Only multiple of 8 bits are allowed, truncation can be done at user level */

    i = 0;
    while(i < outputLength) {
        if (state->bitsAvailableForSqueezing == 0) {
            KeccakPermutation(state->state);
#ifdef ProvideFast1024
            if (state->rate == 1024) {
                KeccakExtract1024bits(state->state, state->dataQueue);
                state->bitsAvailableForSqueezing = 1024;
            }
            else
#endif
            {
                KeccakExtract(state->state, state->dataQueue, state->rate/64);
                state->bitsAvailableForSqueezing = state->rate;
            }
            #ifdef KeccakReference
            displayBytes(1, "Block available for squeezing", state->dataQueue, state->bitsAvailableForSqueezing/8);
            #endif
        }
        partialBlock = state->bitsAvailableForSqueezing;
        if ((unsigned long long)partialBlock > outputLength - i)
            partialBlock = (unsigned int)(outputLength - i);
        memcpy(output+i/8, state->dataQueue+(state->rate-state->bitsAvailableForSqueezing)/8, partialBlock/8);
        state->bitsAvailableForSqueezing -= partialBlock;
        i += partialBlock;
    }
    return 0;
}
