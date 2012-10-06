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

#ifndef _KeccakSponge_h_
#define _KeccakSponge_h_

#define KeccakPermutationSize 1600
#define KeccakPermutationSizeInBytes (KeccakPermutationSize/8)
#define KeccakMaximumRate 1536
#define KeccakMaximumRateInBytes (KeccakMaximumRate/8)

#if defined(__GNUC__)
#define ALIGN __attribute__ ((aligned(32)))
#elif defined(_MSC_VER)
#define ALIGN __declspec(align(32))
#else
#define ALIGN
#endif

ALIGN typedef struct spongeStateStruct {
    ALIGN unsigned char state[KeccakPermutationSizeInBytes];
    ALIGN unsigned char dataQueue[KeccakMaximumRateInBytes];
    unsigned int rate;
    unsigned int capacity;
    unsigned int bitsInQueue;
    unsigned int fixedOutputLength;
    int squeezing;
    unsigned int bitsAvailableForSqueezing;
} spongeState;

/**
  * Function to initialize the state of the Keccak[r, c] sponge function.
  * The sponge function is set to the absorbing phase.
  * @param  state       Pointer to the state of the sponge function to be initialized.
  * @param  rate        The value of the rate r.
  * @param  capacity    The value of the capacity c.
  * @pre    One must have r+c=1600 and the rate a multiple of 64 bits in this implementation.
  * @return Zero if successful, 1 otherwise.
  */
static int InitSponge(spongeState *state, unsigned int rate, unsigned int capacity);
/**
  * Function to give input data for the sponge function to absorb.
  * @param  state       Pointer to the state of the sponge function initialized by InitSponge().
  * @param  data        Pointer to the input data. 
  *                     When @a databitLen is not a multiple of 8, the last bits of data must be
  *                     in the least significant bits of the last byte.
  * @param  databitLen  The number of input bits provided in the input data.
  * @pre    In the previous call to Absorb(), databitLen was a multiple of 8.
  * @pre    The sponge function must be in the absorbing phase,
  *         i.e., Squeeze() must not have been called before.
  * @return Zero if successful, 1 otherwise.
  */
static int Absorb(spongeState *state, const unsigned char *data, unsigned long long databitlen);
/**
  * Function to squeeze output data from the sponge function.
  * If the sponge function was in the absorbing phase, this function 
  * switches it to the squeezing phase.
  * @param  state       Pointer to the state of the sponge function initialized by InitSponge().
  * @param  output      Pointer to the buffer where to store the output data.
  * @param  outputLength    The number of output bits desired.
  *                     It must be a multiple of 8.
  * @return Zero if successful, 1 otherwise.
  */
static int Squeeze(spongeState *state, unsigned char *output, unsigned long long outputLength);

#endif
