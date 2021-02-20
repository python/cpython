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

#include "KeccakSponge.h"

#ifdef KeccakReference
    #include "displayIntermediateValues.h"
#endif

#ifdef XKCP_has_KeccakP200
    #include "KeccakP-200-SnP.h"

    #define prefix KeccakWidth200
    #define SnP KeccakP200
    #define SnP_width 200
    #define SnP_Permute KeccakP200_Permute_18rounds
    #if defined(KeccakF200_FastLoop_supported)
        #define SnP_FastLoop_Absorb KeccakF200_FastLoop_Absorb
    #endif
        #include "KeccakSponge.inc"
    #undef prefix
    #undef SnP
    #undef SnP_width
    #undef SnP_Permute
    #undef SnP_FastLoop_Absorb
#endif

#ifdef XKCP_has_KeccakP400
    #include "KeccakP-400-SnP.h"

    #define prefix KeccakWidth400
    #define SnP KeccakP400
    #define SnP_width 400
    #define SnP_Permute KeccakP400_Permute_20rounds
    #if defined(KeccakF400_FastLoop_supported)
        #define SnP_FastLoop_Absorb KeccakF400_FastLoop_Absorb
    #endif
        #include "KeccakSponge.inc"
    #undef prefix
    #undef SnP
    #undef SnP_width
    #undef SnP_Permute
    #undef SnP_FastLoop_Absorb
#endif

#ifdef XKCP_has_KeccakP800
    #include "KeccakP-800-SnP.h"

    #define prefix KeccakWidth800
    #define SnP KeccakP800
    #define SnP_width 800
    #define SnP_Permute KeccakP800_Permute_22rounds
    #if defined(KeccakF800_FastLoop_supported)
        #define SnP_FastLoop_Absorb KeccakF800_FastLoop_Absorb
    #endif
        #include "KeccakSponge.inc"
    #undef prefix
    #undef SnP
    #undef SnP_width
    #undef SnP_Permute
    #undef SnP_FastLoop_Absorb
#endif

#ifdef XKCP_has_KeccakP1600
    #include "KeccakP-1600-SnP.h"

    #define prefix KeccakWidth1600
    #define SnP KeccakP1600
    #define SnP_width 1600
    #define SnP_Permute KeccakP1600_Permute_24rounds
    #if defined(KeccakF1600_FastLoop_supported)
        #define SnP_FastLoop_Absorb KeccakF1600_FastLoop_Absorb
    #endif
        #include "KeccakSponge.inc"
    #undef prefix
    #undef SnP
    #undef SnP_width
    #undef SnP_Permute
    #undef SnP_FastLoop_Absorb
#endif

#ifdef XKCP_has_KeccakP1600
    #include "KeccakP-1600-SnP.h"

    #define prefix KeccakWidth1600_12rounds
    #define SnP KeccakP1600
    #define SnP_width 1600
    #define SnP_Permute KeccakP1600_Permute_12rounds
    #if defined(KeccakP1600_12rounds_FastLoop_supported)
        #define SnP_FastLoop_Absorb KeccakP1600_12rounds_FastLoop_Absorb
    #endif
        #include "KeccakSponge.inc"
    #undef prefix
    #undef SnP
    #undef SnP_width
    #undef SnP_Permute
    #undef SnP_FastLoop_Absorb
#endif
