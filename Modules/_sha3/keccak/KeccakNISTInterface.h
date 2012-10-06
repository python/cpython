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

#ifndef _KeccakNISTInterface_h_
#define _KeccakNISTInterface_h_

#include "KeccakSponge.h"

typedef unsigned char BitSequence;
typedef unsigned long long DataLength;
typedef enum { SUCCESS = 0, FAIL = 1, BAD_HASHLEN = 2 } HashReturn;

typedef spongeState hashState;

/**
  * Function to initialize the state of the Keccak[r, c] sponge function.
  * The rate r and capacity c values are determined from @a hashbitlen.
  * @param  state       Pointer to the state of the sponge function to be initialized.
  * @param  hashbitlen  The desired number of output bits, 
  *                     or 0 for Keccak[] with default parameters
  *                     and arbitrarily-long output.
  * @pre    The value of hashbitlen must be one of 0, 224, 256, 384 and 512.
  * @return SUCCESS if successful, BAD_HASHLEN if the value of hashbitlen is incorrect.
  */
static HashReturn Init(hashState *state, int hashbitlen);
/**
  * Function to give input data for the sponge function to absorb.
  * @param  state       Pointer to the state of the sponge function initialized by Init().
  * @param  data        Pointer to the input data. 
  *                     When @a databitLen is not a multiple of 8, the last bits of data must be
  *                     in the most significant bits of the last byte.
  * @param  databitLen  The number of input bits provided in the input data.
  * @pre    In the previous call to Absorb(), databitLen was a multiple of 8.
  * @return SUCCESS if successful, FAIL otherwise.
  */
static HashReturn Update(hashState *state, const BitSequence *data, DataLength databitlen);
/**
  * Function to squeeze output data from the sponge function.
  * If @a hashbitlen was not 0 in the call to Init(), the number of output bits is equal to @a hashbitlen.
  * If @a hashbitlen was 0 in the call to Init(), the output bits must be extracted using the Squeeze() function.
  * @param  state       Pointer to the state of the sponge function initialized by Init().
  * @param  hashval     Pointer to the buffer where to store the output data.
  * @return SUCCESS if successful, FAIL otherwise.
  */
static HashReturn Final(hashState *state, BitSequence *hashval);
/**
  * Function to compute a hash using the Keccak[r, c] sponge function.
  * The rate r and capacity c values are determined from @a hashbitlen.
  * @param  hashbitlen  The desired number of output bits.
  * @param  data        Pointer to the input data. 
  *                     When @a databitLen is not a multiple of 8, the last bits of data must be
  *                     in the most significant bits of the last byte.
  * @param  databitLen  The number of input bits provided in the input data.
  * @param  hashval     Pointer to the buffer where to store the output data.
  * @pre    The value of hashbitlen must be one of 224, 256, 384 and 512.
  * @return SUCCESS if successful, BAD_HASHLEN if the value of hashbitlen is incorrect.
  */
/*
static HashReturn Hash(int hashbitlen, const BitSequence *data, DataLength databitlen, BitSequence *hashval);
*/

#endif
