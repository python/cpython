/*
The eXtended Keccak Code Package (XKCP)
https://github.com/XKCP/XKCP

The Keccak-p permutations, designed by Guido Bertoni, Joan Daemen, Michaël Peeters and Gilles Van Assche.

Implementation by Gilles Van Assche and Ronny Van Keer, hereby denoted as "the implementer".

For more information, feedback or questions, please refer to the Keccak Team website:
https://keccak.team/

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#if (defined(FullUnrolling))
#define rounds24 \
    prepareTheta \
    thetaRhoPiChiIotaPrepareTheta( 0, A, E) \
    thetaRhoPiChiIotaPrepareTheta( 1, E, A) \
    thetaRhoPiChiIotaPrepareTheta( 2, A, E) \
    thetaRhoPiChiIotaPrepareTheta( 3, E, A) \
    thetaRhoPiChiIotaPrepareTheta( 4, A, E) \
    thetaRhoPiChiIotaPrepareTheta( 5, E, A) \
    thetaRhoPiChiIotaPrepareTheta( 6, A, E) \
    thetaRhoPiChiIotaPrepareTheta( 7, E, A) \
    thetaRhoPiChiIotaPrepareTheta( 8, A, E) \
    thetaRhoPiChiIotaPrepareTheta( 9, E, A) \
    thetaRhoPiChiIotaPrepareTheta(10, A, E) \
    thetaRhoPiChiIotaPrepareTheta(11, E, A) \
    thetaRhoPiChiIotaPrepareTheta(12, A, E) \
    thetaRhoPiChiIotaPrepareTheta(13, E, A) \
    thetaRhoPiChiIotaPrepareTheta(14, A, E) \
    thetaRhoPiChiIotaPrepareTheta(15, E, A) \
    thetaRhoPiChiIotaPrepareTheta(16, A, E) \
    thetaRhoPiChiIotaPrepareTheta(17, E, A) \
    thetaRhoPiChiIotaPrepareTheta(18, A, E) \
    thetaRhoPiChiIotaPrepareTheta(19, E, A) \
    thetaRhoPiChiIotaPrepareTheta(20, A, E) \
    thetaRhoPiChiIotaPrepareTheta(21, E, A) \
    thetaRhoPiChiIotaPrepareTheta(22, A, E) \
    thetaRhoPiChiIota(23, E, A) \

#define rounds12 \
    prepareTheta \
    thetaRhoPiChiIotaPrepareTheta(12, A, E) \
    thetaRhoPiChiIotaPrepareTheta(13, E, A) \
    thetaRhoPiChiIotaPrepareTheta(14, A, E) \
    thetaRhoPiChiIotaPrepareTheta(15, E, A) \
    thetaRhoPiChiIotaPrepareTheta(16, A, E) \
    thetaRhoPiChiIotaPrepareTheta(17, E, A) \
    thetaRhoPiChiIotaPrepareTheta(18, A, E) \
    thetaRhoPiChiIotaPrepareTheta(19, E, A) \
    thetaRhoPiChiIotaPrepareTheta(20, A, E) \
    thetaRhoPiChiIotaPrepareTheta(21, E, A) \
    thetaRhoPiChiIotaPrepareTheta(22, A, E) \
    thetaRhoPiChiIota(23, E, A) \

#define rounds6 \
    prepareTheta \
    thetaRhoPiChiIotaPrepareTheta(18, A, E) \
    thetaRhoPiChiIotaPrepareTheta(19, E, A) \
    thetaRhoPiChiIotaPrepareTheta(20, A, E) \
    thetaRhoPiChiIotaPrepareTheta(21, E, A) \
    thetaRhoPiChiIotaPrepareTheta(22, A, E) \
    thetaRhoPiChiIota(23, E, A) \

#define rounds4 \
    prepareTheta \
    thetaRhoPiChiIotaPrepareTheta(20, A, E) \
    thetaRhoPiChiIotaPrepareTheta(21, E, A) \
    thetaRhoPiChiIotaPrepareTheta(22, A, E) \
    thetaRhoPiChiIota(23, E, A) \

#elif (Unrolling == 12)
#define rounds24 \
    prepareTheta \
    for(i=0; i<24; i+=12) { \
        thetaRhoPiChiIotaPrepareTheta(i   , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+ 1, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+ 2, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+ 3, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+ 4, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+ 5, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+ 6, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+ 7, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+ 8, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+ 9, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+10, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+11, E, A) \
    } \

#define rounds12 \
    prepareTheta \
    thetaRhoPiChiIotaPrepareTheta(12, A, E) \
    thetaRhoPiChiIotaPrepareTheta(13, E, A) \
    thetaRhoPiChiIotaPrepareTheta(14, A, E) \
    thetaRhoPiChiIotaPrepareTheta(15, E, A) \
    thetaRhoPiChiIotaPrepareTheta(16, A, E) \
    thetaRhoPiChiIotaPrepareTheta(17, E, A) \
    thetaRhoPiChiIotaPrepareTheta(18, A, E) \
    thetaRhoPiChiIotaPrepareTheta(19, E, A) \
    thetaRhoPiChiIotaPrepareTheta(20, A, E) \
    thetaRhoPiChiIotaPrepareTheta(21, E, A) \
    thetaRhoPiChiIotaPrepareTheta(22, A, E) \
    thetaRhoPiChiIota(23, E, A) \

#define rounds6 \
    prepareTheta \
    thetaRhoPiChiIotaPrepareTheta(18, A, E) \
    thetaRhoPiChiIotaPrepareTheta(19, E, A) \
    thetaRhoPiChiIotaPrepareTheta(20, A, E) \
    thetaRhoPiChiIotaPrepareTheta(21, E, A) \
    thetaRhoPiChiIotaPrepareTheta(22, A, E) \
    thetaRhoPiChiIota(23, E, A) \

#define rounds4 \
    prepareTheta \
    thetaRhoPiChiIotaPrepareTheta(20, A, E) \
    thetaRhoPiChiIotaPrepareTheta(21, E, A) \
    thetaRhoPiChiIotaPrepareTheta(22, A, E) \
    thetaRhoPiChiIota(23, E, A) \

#elif (Unrolling == 6)
#define rounds24 \
    prepareTheta \
    for(i=0; i<24; i+=6) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+2, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+3, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+4, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+5, E, A) \
    } \

#define rounds12 \
    prepareTheta \
    for(i=12; i<24; i+=6) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+2, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+3, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+4, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+5, E, A) \
    } \

#define rounds6 \
    prepareTheta \
    thetaRhoPiChiIotaPrepareTheta(18, A, E) \
    thetaRhoPiChiIotaPrepareTheta(19, E, A) \
    thetaRhoPiChiIotaPrepareTheta(20, A, E) \
    thetaRhoPiChiIotaPrepareTheta(21, E, A) \
    thetaRhoPiChiIotaPrepareTheta(22, A, E) \
    thetaRhoPiChiIota(23, E, A) \

#define rounds4 \
    prepareTheta \
    thetaRhoPiChiIotaPrepareTheta(20, A, E) \
    thetaRhoPiChiIotaPrepareTheta(21, E, A) \
    thetaRhoPiChiIotaPrepareTheta(22, A, E) \
    thetaRhoPiChiIota(23, E, A) \

#elif (Unrolling == 4)
#define rounds24 \
    prepareTheta \
    for(i=0; i<24; i+=4) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+2, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+3, E, A) \
    } \

#define rounds12 \
    prepareTheta \
    for(i=12; i<24; i+=4) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+2, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+3, E, A) \
    } \

#define rounds6 \
    prepareTheta \
    for(i=18; i<24; i+=2) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
    } \

#define rounds4 \
    prepareTheta \
    thetaRhoPiChiIotaPrepareTheta(20, A, E) \
    thetaRhoPiChiIotaPrepareTheta(21, E, A) \
    thetaRhoPiChiIotaPrepareTheta(22, A, E) \
    thetaRhoPiChiIota(23, E, A) \

#elif (Unrolling == 3)
#define rounds24 \
    prepareTheta \
    for(i=0; i<24; i+=3) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+2, A, E) \
        copyStateVariables(A, E) \
    } \

#define rounds12 \
    prepareTheta \
    for(i=12; i<24; i+=3) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+2, A, E) \
        copyStateVariables(A, E) \
    } \

#define rounds6 \
    prepareTheta \
    for(i=18; i<24; i+=3) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+2, A, E) \
        copyStateVariables(A, E) \
    } \

#define rounds4 \
    prepareTheta \
    for(i=20; i<24; i+=2) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
    } \

#elif (Unrolling == 2)
#define rounds24 \
    prepareTheta \
    for(i=0; i<24; i+=2) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
    } \

#define rounds12 \
    prepareTheta \
    for(i=12; i<24; i+=2) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
    } \

#define rounds6 \
    prepareTheta \
    for(i=18; i<24; i+=2) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
    } \

#define rounds4 \
    prepareTheta \
    for(i=20; i<24; i+=2) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
    } \

#elif (Unrolling == 1)
#define rounds24 \
    prepareTheta \
    for(i=0; i<24; i++) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        copyStateVariables(A, E) \
    } \

#define rounds12 \
    prepareTheta \
    for(i=12; i<24; i++) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        copyStateVariables(A, E) \
    } \

#define rounds6 \
    prepareTheta \
    for(i=18; i<24; i++) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        copyStateVariables(A, E) \
    } \

#define rounds4 \
    prepareTheta \
    for(i=20; i<24; i++) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        copyStateVariables(A, E) \
    } \

#else
#error "Unrolling is not correctly specified!"
#endif

#define roundsN(__nrounds) \
    prepareTheta \
    i = 24 - (__nrounds); \
    if ((i&1) != 0) { \
        thetaRhoPiChiIotaPrepareTheta(i, A, E) \
        copyStateVariables(A, E) \
        ++i; \
    } \
    for( /* empty */; i<24; i+=2) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
    }
