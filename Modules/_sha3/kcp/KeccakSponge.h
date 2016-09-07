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

#ifndef _KeccakSponge_h_
#define _KeccakSponge_h_

/** General information
  *
  * The following type and functions are not actually implemented. Their
  * documentation is generic, with the prefix Prefix replaced by
  * - KeccakWidth200 for a sponge function based on Keccak-f[200]
  * - KeccakWidth400 for a sponge function based on Keccak-f[400]
  * - KeccakWidth800 for a sponge function based on Keccak-f[800]
  * - KeccakWidth1600 for a sponge function based on Keccak-f[1600]
  *
  * In all these functions, the rate and capacity must sum to the width of the
  * chosen permutation. For instance, to use the sponge function
  * Keccak[r=1344, c=256], one must use KeccakWidth1600_Sponge() or a combination
  * of KeccakWidth1600_SpongeInitialize(), KeccakWidth1600_SpongeAbsorb(),
  * KeccakWidth1600_SpongeAbsorbLastFewBits() and
  * KeccakWidth1600_SpongeSqueeze().
  *
  * The Prefix_SpongeInstance contains the sponge instance attributes for use
  * with the Prefix_Sponge* functions.
  * It gathers the state processed by the permutation as well as the rate,
  * the position of input/output bytes in the state and the phase
  * (absorbing or squeezing).
  */

#ifdef DontReallyInclude_DocumentationOnly
/** Function to evaluate the sponge function Keccak[r, c] in a single call.
  * @param  rate        The value of the rate r.
  * @param  capacity    The value of the capacity c.
  * @param  input           Pointer to the input message (before the suffix).
  * @param  inputByteLen    The length of the input message in bytes.
  * @param  suffix          Byte containing from 0 to 7 suffix bits
  *                         that must be absorbed after @a input.
  *                         These <i>n</i> bits must be in the least significant bit positions.
  *                         These bits must be delimited with a bit 1 at position <i>n</i>
  *                         (counting from 0=LSB to 7=MSB) and followed by bits 0
  *                         from position <i>n</i>+1 to position 7.
  *                         Some examples:
  *                             - If no bits are to be absorbed, then @a suffix must be 0x01.
  *                             - If the 2-bit sequence 0,0 is to be absorbed, @a suffix must be 0x04.
  *                             - If the 5-bit sequence 0,1,0,0,1 is to be absorbed, @a suffix must be 0x32.
  *                             - If the 7-bit sequence 1,1,0,1,0,0,0 is to be absorbed, @a suffix must be 0x8B.
  *                         .
  * @param  output          Pointer to the output buffer.
  * @param  outputByteLen   The desired number of output bytes.
  * @pre    One must have r+c equal to the supported width of this implementation
  *         and the rate a multiple of 8 bits (one byte) in this implementation.
  * @pre    @a suffix ≠ 0x00
  * @return Zero if successful, 1 otherwise.
  */
int Prefix_Sponge(unsigned int rate, unsigned int capacity, const unsigned char *input, size_t inputByteLen, unsigned char suffix, unsigned char *output, size_t outputByteLen);

/**
  * Function to initialize the state of the Keccak[r, c] sponge function.
  * The phase of the sponge function is set to absorbing.
  * @param  spongeInstance  Pointer to the sponge instance to be initialized.
  * @param  rate        The value of the rate r.
  * @param  capacity    The value of the capacity c.
  * @pre    One must have r+c equal to the supported width of this implementation
  *         and the rate a multiple of 8 bits (one byte) in this implementation.
  * @return Zero if successful, 1 otherwise.
  */
int Prefix_SpongeInitialize(Prefix_SpongeInstance *spongeInstance, unsigned int rate, unsigned int capacity);

/**
  * Function to give input data bytes for the sponge function to absorb.
  * @param  spongeInstance  Pointer to the sponge instance initialized by Prefix_SpongeInitialize().
  * @param  data        Pointer to the input data.
  * @param  dataByteLen  The number of input bytes provided in the input data.
  * @pre    The sponge function must be in the absorbing phase,
  *         i.e., Prefix_SpongeSqueeze() or Prefix_SpongeAbsorbLastFewBits()
  *         must not have been called before.
  * @return Zero if successful, 1 otherwise.
  */
int Prefix_SpongeAbsorb(Prefix_SpongeInstance *spongeInstance, const unsigned char *data, size_t dataByteLen);

/**
  * Function to give input data bits for the sponge function to absorb
  * and then to switch to the squeezing phase.
  * @param  spongeInstance  Pointer to the sponge instance initialized by Prefix_SpongeInitialize().
  * @param  delimitedData   Byte containing from 0 to 7 trailing bits
  *                     that must be absorbed.
  *                     These <i>n</i> bits must be in the least significant bit positions.
  *                     These bits must be delimited with a bit 1 at position <i>n</i>
  *                     (counting from 0=LSB to 7=MSB) and followed by bits 0
  *                     from position <i>n</i>+1 to position 7.
  *                     Some examples:
  *                         - If no bits are to be absorbed, then @a delimitedData must be 0x01.
  *                         - If the 2-bit sequence 0,0 is to be absorbed, @a delimitedData must be 0x04.
  *                         - If the 5-bit sequence 0,1,0,0,1 is to be absorbed, @a delimitedData must be 0x32.
  *                         - If the 7-bit sequence 1,1,0,1,0,0,0 is to be absorbed, @a delimitedData must be 0x8B.
  *                     .
  * @pre    The sponge function must be in the absorbing phase,
  *         i.e., Prefix_SpongeSqueeze() or Prefix_SpongeAbsorbLastFewBits()
  *         must not have been called before.
  * @pre    @a delimitedData ≠ 0x00
  * @return Zero if successful, 1 otherwise.
  */
int Prefix_SpongeAbsorbLastFewBits(Prefix_SpongeInstance *spongeInstance, unsigned char delimitedData);

/**
  * Function to squeeze output data from the sponge function.
  * If the sponge function was in the absorbing phase, this function
  * switches it to the squeezing phase
  * as if Prefix_SpongeAbsorbLastFewBits(spongeInstance, 0x01) was called.
  * @param  spongeInstance  Pointer to the sponge instance initialized by Prefix_SpongeInitialize().
  * @param  data        Pointer to the buffer where to store the output data.
  * @param  dataByteLen The number of output bytes desired.
  * @return Zero if successful, 1 otherwise.
  */
int Prefix_SpongeSqueeze(Prefix_SpongeInstance *spongeInstance, unsigned char *data, size_t dataByteLen);
#endif

#include <string.h>
#include "align.h"

#define KCP_DeclareSpongeStructure(prefix, size, alignment) \
    ALIGN(alignment) typedef struct prefix##_SpongeInstanceStruct { \
        unsigned char state[size]; \
        unsigned int rate; \
        unsigned int byteIOIndex; \
        int squeezing; \
    } prefix##_SpongeInstance;

#define KCP_DeclareSpongeFunctions(prefix) \
    int prefix##_Sponge(unsigned int rate, unsigned int capacity, const unsigned char *input, size_t inputByteLen, unsigned char suffix, unsigned char *output, size_t outputByteLen); \
    int prefix##_SpongeInitialize(prefix##_SpongeInstance *spongeInstance, unsigned int rate, unsigned int capacity); \
    int prefix##_SpongeAbsorb(prefix##_SpongeInstance *spongeInstance, const unsigned char *data, size_t dataByteLen); \
    int prefix##_SpongeAbsorbLastFewBits(prefix##_SpongeInstance *spongeInstance, unsigned char delimitedData); \
    int prefix##_SpongeSqueeze(prefix##_SpongeInstance *spongeInstance, unsigned char *data, size_t dataByteLen);

#ifndef KeccakP200_excluded
    #include "KeccakP-200-SnP.h"
    KCP_DeclareSpongeStructure(KeccakWidth200, KeccakP200_stateSizeInBytes, KeccakP200_stateAlignment)
    KCP_DeclareSpongeFunctions(KeccakWidth200)
#endif

#ifndef KeccakP400_excluded
    #include "KeccakP-400-SnP.h"
    KCP_DeclareSpongeStructure(KeccakWidth400, KeccakP400_stateSizeInBytes, KeccakP400_stateAlignment)
    KCP_DeclareSpongeFunctions(KeccakWidth400)
#endif

#ifndef KeccakP800_excluded
    #include "KeccakP-800-SnP.h"
    KCP_DeclareSpongeStructure(KeccakWidth800, KeccakP800_stateSizeInBytes, KeccakP800_stateAlignment)
    KCP_DeclareSpongeFunctions(KeccakWidth800)
#endif

#ifndef KeccakP1600_excluded
    #include "KeccakP-1600-SnP.h"
    KCP_DeclareSpongeStructure(KeccakWidth1600, KeccakP1600_stateSizeInBytes, KeccakP1600_stateAlignment)
    KCP_DeclareSpongeFunctions(KeccakWidth1600)
#endif

#endif
