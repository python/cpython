/*
The eXtended Keccak Code Package (XKCP)
https://github.com/XKCP/XKCP

Keccak, designed by Guido Bertoni, Joan Daemen, Michaël Peeters and Gilles Van Assche.

Implementation by the designers, hereby denoted as "the implementer".

For more information, feedback or questions, please refer to the Keccak Team website:
https://keccak.team/

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#ifndef _KeccakHashInterface_h_
#define _KeccakHashInterface_h_

/* #include "config.h" */
#ifdef XKCP_has_KeccakP1600

#include <stdint.h>
#include <string.h>
#include "KeccakSponge.h"

#ifndef _Keccak_BitTypes_
#define _Keccak_BitTypes_
typedef uint8_t BitSequence;

typedef size_t BitLength;
#endif

typedef enum { KECCAK_SUCCESS = 0, KECCAK_FAIL = 1, KECCAK_BAD_HASHLEN = 2 } HashReturn;

typedef struct {
    KeccakWidth1600_SpongeInstance sponge;
    unsigned int fixedOutputLength;
    unsigned char delimitedSuffix;
} Keccak_HashInstance;

/**
  * Function to initialize the Keccak[r, c] sponge function instance used in sequential hashing mode.
  * @param  hashInstance    Pointer to the hash instance to be initialized.
  * @param  rate        The value of the rate r.
  * @param  capacity    The value of the capacity c.
  * @param  hashbitlen  The desired number of output bits,
  *                     or 0 for an arbitrarily-long output.
  * @param  delimitedSuffix Bits that will be automatically appended to the end
  *                         of the input message, as in domain separation.
  *                         This is a byte containing from 0 to 7 bits
  *                         formatted like the @a delimitedData parameter of
  *                         the Keccak_SpongeAbsorbLastFewBits() function.
  * @pre    One must have r+c=1600 and the rate a multiple of 8 bits in this implementation.
  * @return KECCAK_SUCCESS if successful, KECCAK_FAIL otherwise.
  */
HashReturn Keccak_HashInitialize(Keccak_HashInstance *hashInstance, unsigned int rate, unsigned int capacity, unsigned int hashbitlen, unsigned char delimitedSuffix);

/** Macro to initialize a SHAKE128 instance as specified in the FIPS 202 standard.
  */
#define Keccak_HashInitialize_SHAKE128(hashInstance)        Keccak_HashInitialize(hashInstance, 1344,  256,   0, 0x1F)

/** Macro to initialize a SHAKE256 instance as specified in the FIPS 202 standard.
  */
#define Keccak_HashInitialize_SHAKE256(hashInstance)        Keccak_HashInitialize(hashInstance, 1088,  512,   0, 0x1F)

/** Macro to initialize a SHA3-224 instance as specified in the FIPS 202 standard.
  */
#define Keccak_HashInitialize_SHA3_224(hashInstance)        Keccak_HashInitialize(hashInstance, 1152,  448, 224, 0x06)

/** Macro to initialize a SHA3-256 instance as specified in the FIPS 202 standard.
  */
#define Keccak_HashInitialize_SHA3_256(hashInstance)        Keccak_HashInitialize(hashInstance, 1088,  512, 256, 0x06)

/** Macro to initialize a SHA3-384 instance as specified in the FIPS 202 standard.
  */
#define Keccak_HashInitialize_SHA3_384(hashInstance)        Keccak_HashInitialize(hashInstance,  832,  768, 384, 0x06)

/** Macro to initialize a SHA3-512 instance as specified in the FIPS 202 standard.
  */
#define Keccak_HashInitialize_SHA3_512(hashInstance)        Keccak_HashInitialize(hashInstance,  576, 1024, 512, 0x06)

/**
  * Function to give input data to be absorbed.
  * @param  hashInstance    Pointer to the hash instance initialized by Keccak_HashInitialize().
  * @param  data        Pointer to the input data.
  *                     When @a databitLen is not a multiple of 8, the last bits of data must be
  *                     in the least significant bits of the last byte (little-endian convention).
  *                     In this case, the (8 - @a databitLen mod 8) most significant bits
  *                     of the last byte are ignored.
  * @param  databitLen  The number of input bits provided in the input data.
  * @pre    In the previous call to Keccak_HashUpdate(), databitlen was a multiple of 8.
  * @return KECCAK_SUCCESS if successful, KECCAK_FAIL otherwise.
  */
HashReturn Keccak_HashUpdate(Keccak_HashInstance *hashInstance, const BitSequence *data, BitLength databitlen);

/**
  * Function to call after all input blocks have been input and to get
  * output bits if the length was specified when calling Keccak_HashInitialize().
  * @param  hashInstance    Pointer to the hash instance initialized by Keccak_HashInitialize().
  * If @a hashbitlen was not 0 in the call to Keccak_HashInitialize(), the number of
  *     output bits is equal to @a hashbitlen.
  * If @a hashbitlen was 0 in the call to Keccak_HashInitialize(), the output bits
  *     must be extracted using the Keccak_HashSqueeze() function.
  * @param  hashval     Pointer to the buffer where to store the output data.
  * @return KECCAK_SUCCESS if successful, KECCAK_FAIL otherwise.
  */
HashReturn Keccak_HashFinal(Keccak_HashInstance *hashInstance, BitSequence *hashval);

 /**
  * Function to squeeze output data.
  * @param  hashInstance    Pointer to the hash instance initialized by Keccak_HashInitialize().
  * @param  data        Pointer to the buffer where to store the output data.
  * @param  databitlen  The number of output bits desired (must be a multiple of 8).
  * @pre    Keccak_HashFinal() must have been already called.
  * @pre    @a databitlen is a multiple of 8.
  * @return KECCAK_SUCCESS if successful, KECCAK_FAIL otherwise.
  */
HashReturn Keccak_HashSqueeze(Keccak_HashInstance *hashInstance, BitSequence *data, BitLength databitlen);

#else
#error This requires an implementation of Keccak-p[1600]
#endif

#endif
