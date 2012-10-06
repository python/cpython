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

#if (Unrolling == 24)
#define rounds \
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
    copyToState(state, A)
#elif (Unrolling == 12)
#define rounds \
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
    copyToState(state, A)
#elif (Unrolling == 8)
#define rounds \
    prepareTheta \
    for(i=0; i<24; i+=8) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+2, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+3, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+4, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+5, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+6, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+7, E, A) \
    } \
    copyToState(state, A)
#elif (Unrolling == 6)
#define rounds \
    prepareTheta \
    for(i=0; i<24; i+=6) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+2, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+3, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+4, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+5, E, A) \
    } \
    copyToState(state, A)
#elif (Unrolling == 4)
#define rounds \
    prepareTheta \
    for(i=0; i<24; i+=4) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+2, A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+3, E, A) \
    } \
    copyToState(state, A)
#elif (Unrolling == 3)
#define rounds \
    prepareTheta \
    for(i=0; i<24; i+=3) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
        thetaRhoPiChiIotaPrepareTheta(i+2, A, E) \
        copyStateVariables(A, E) \
    } \
    copyToState(state, A)
#elif (Unrolling == 2)
#define rounds \
    prepareTheta \
    for(i=0; i<24; i+=2) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        thetaRhoPiChiIotaPrepareTheta(i+1, E, A) \
    } \
    copyToState(state, A)
#elif (Unrolling == 1)
#define rounds \
    prepareTheta \
    for(i=0; i<24; i++) { \
        thetaRhoPiChiIotaPrepareTheta(i  , A, E) \
        copyStateVariables(A, E) \
    } \
    copyToState(state, A)
#else
#error "Unrolling is not correctly specified!"
#endif
