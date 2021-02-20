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

#ifndef _KeccakSponge_h_
#define _KeccakSponge_h_

/* For the documentation, please follow the link: */
/* #include "KeccakSponge-documentation.h" */

#include <string.h>
#include "align.h"
/* #include "config.h" */

#define XKCP_DeclareSpongeStructure(prefix, size, alignment) \
    ALIGN(alignment) typedef struct prefix##_SpongeInstanceStruct { \
        unsigned char state[size]; \
        unsigned int rate; \
        unsigned int byteIOIndex; \
        int squeezing; \
    } prefix##_SpongeInstance;

#define XKCP_DeclareSpongeFunctions(prefix) \
    int prefix##_Sponge(unsigned int rate, unsigned int capacity, const unsigned char *input, size_t inputByteLen, unsigned char suffix, unsigned char *output, size_t outputByteLen); \
    int prefix##_SpongeInitialize(prefix##_SpongeInstance *spongeInstance, unsigned int rate, unsigned int capacity); \
    int prefix##_SpongeAbsorb(prefix##_SpongeInstance *spongeInstance, const unsigned char *data, size_t dataByteLen); \
    int prefix##_SpongeAbsorbLastFewBits(prefix##_SpongeInstance *spongeInstance, unsigned char delimitedData); \
    int prefix##_SpongeSqueeze(prefix##_SpongeInstance *spongeInstance, unsigned char *data, size_t dataByteLen);

#ifdef XKCP_has_KeccakP200
    #include "KeccakP-200-SnP.h"
    XKCP_DeclareSpongeStructure(KeccakWidth200, KeccakP200_stateSizeInBytes, KeccakP200_stateAlignment)
    XKCP_DeclareSpongeFunctions(KeccakWidth200)
    #define XKCP_has_Sponge_Keccak_width200
#endif

#ifdef XKCP_has_KeccakP400
    #include "KeccakP-400-SnP.h"
    XKCP_DeclareSpongeStructure(KeccakWidth400, KeccakP400_stateSizeInBytes, KeccakP400_stateAlignment)
    XKCP_DeclareSpongeFunctions(KeccakWidth400)
    #define XKCP_has_Sponge_Keccak_width400
#endif

#ifdef XKCP_has_KeccakP800
    #include "KeccakP-800-SnP.h"
    XKCP_DeclareSpongeStructure(KeccakWidth800, KeccakP800_stateSizeInBytes, KeccakP800_stateAlignment)
    XKCP_DeclareSpongeFunctions(KeccakWidth800)
    #define XKCP_has_Sponge_Keccak_width800
#endif

#ifdef XKCP_has_KeccakP1600
    #include "KeccakP-1600-SnP.h"
    XKCP_DeclareSpongeStructure(KeccakWidth1600, KeccakP1600_stateSizeInBytes, KeccakP1600_stateAlignment)
    XKCP_DeclareSpongeFunctions(KeccakWidth1600)
    #define XKCP_has_Sponge_Keccak_width1600
#endif

#ifdef XKCP_has_KeccakP1600
    #include "KeccakP-1600-SnP.h"
    XKCP_DeclareSpongeStructure(KeccakWidth1600_12rounds, KeccakP1600_stateSizeInBytes, KeccakP1600_stateAlignment)
    XKCP_DeclareSpongeFunctions(KeccakWidth1600_12rounds)
#endif

#endif
