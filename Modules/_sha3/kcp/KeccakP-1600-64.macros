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

#define declareABCDE \
    UINT64 Aba, Abe, Abi, Abo, Abu; \
    UINT64 Aga, Age, Agi, Ago, Agu; \
    UINT64 Aka, Ake, Aki, Ako, Aku; \
    UINT64 Ama, Ame, Ami, Amo, Amu; \
    UINT64 Asa, Ase, Asi, Aso, Asu; \
    UINT64 Bba, Bbe, Bbi, Bbo, Bbu; \
    UINT64 Bga, Bge, Bgi, Bgo, Bgu; \
    UINT64 Bka, Bke, Bki, Bko, Bku; \
    UINT64 Bma, Bme, Bmi, Bmo, Bmu; \
    UINT64 Bsa, Bse, Bsi, Bso, Bsu; \
    UINT64 Ca, Ce, Ci, Co, Cu; \
    UINT64 Da, De, Di, Do, Du; \
    UINT64 Eba, Ebe, Ebi, Ebo, Ebu; \
    UINT64 Ega, Ege, Egi, Ego, Egu; \
    UINT64 Eka, Eke, Eki, Eko, Eku; \
    UINT64 Ema, Eme, Emi, Emo, Emu; \
    UINT64 Esa, Ese, Esi, Eso, Esu; \

#define prepareTheta \
    Ca = Aba^Aga^Aka^Ama^Asa; \
    Ce = Abe^Age^Ake^Ame^Ase; \
    Ci = Abi^Agi^Aki^Ami^Asi; \
    Co = Abo^Ago^Ako^Amo^Aso; \
    Cu = Abu^Agu^Aku^Amu^Asu; \

#ifdef UseBebigokimisa
/* --- Code for round, with prepare-theta (lane complementing pattern 'bebigokimisa') */

/* --- 64-bit lanes mapped to 64-bit words */

#define thetaRhoPiChiIotaPrepareTheta(i, A, E) \
    Da = Cu^ROL64(Ce, 1); \
    De = Ca^ROL64(Ci, 1); \
    Di = Ce^ROL64(Co, 1); \
    Do = Ci^ROL64(Cu, 1); \
    Du = Co^ROL64(Ca, 1); \
\
    A##ba ^= Da; \
    Bba = A##ba; \
    A##ge ^= De; \
    Bbe = ROL64(A##ge, 44); \
    A##ki ^= Di; \
    Bbi = ROL64(A##ki, 43); \
    A##mo ^= Do; \
    Bbo = ROL64(A##mo, 21); \
    A##su ^= Du; \
    Bbu = ROL64(A##su, 14); \
    E##ba =   Bba ^(  Bbe |  Bbi ); \
    E##ba ^= KeccakF1600RoundConstants[i]; \
    Ca = E##ba; \
    E##be =   Bbe ^((~Bbi)|  Bbo ); \
    Ce = E##be; \
    E##bi =   Bbi ^(  Bbo &  Bbu ); \
    Ci = E##bi; \
    E##bo =   Bbo ^(  Bbu |  Bba ); \
    Co = E##bo; \
    E##bu =   Bbu ^(  Bba &  Bbe ); \
    Cu = E##bu; \
\
    A##bo ^= Do; \
    Bga = ROL64(A##bo, 28); \
    A##gu ^= Du; \
    Bge = ROL64(A##gu, 20); \
    A##ka ^= Da; \
    Bgi = ROL64(A##ka, 3); \
    A##me ^= De; \
    Bgo = ROL64(A##me, 45); \
    A##si ^= Di; \
    Bgu = ROL64(A##si, 61); \
    E##ga =   Bga ^(  Bge |  Bgi ); \
    Ca ^= E##ga; \
    E##ge =   Bge ^(  Bgi &  Bgo ); \
    Ce ^= E##ge; \
    E##gi =   Bgi ^(  Bgo |(~Bgu)); \
    Ci ^= E##gi; \
    E##go =   Bgo ^(  Bgu |  Bga ); \
    Co ^= E##go; \
    E##gu =   Bgu ^(  Bga &  Bge ); \
    Cu ^= E##gu; \
\
    A##be ^= De; \
    Bka = ROL64(A##be, 1); \
    A##gi ^= Di; \
    Bke = ROL64(A##gi, 6); \
    A##ko ^= Do; \
    Bki = ROL64(A##ko, 25); \
    A##mu ^= Du; \
    Bko = ROL64(A##mu, 8); \
    A##sa ^= Da; \
    Bku = ROL64(A##sa, 18); \
    E##ka =   Bka ^(  Bke |  Bki ); \
    Ca ^= E##ka; \
    E##ke =   Bke ^(  Bki &  Bko ); \
    Ce ^= E##ke; \
    E##ki =   Bki ^((~Bko)&  Bku ); \
    Ci ^= E##ki; \
    E##ko = (~Bko)^(  Bku |  Bka ); \
    Co ^= E##ko; \
    E##ku =   Bku ^(  Bka &  Bke ); \
    Cu ^= E##ku; \
\
    A##bu ^= Du; \
    Bma = ROL64(A##bu, 27); \
    A##ga ^= Da; \
    Bme = ROL64(A##ga, 36); \
    A##ke ^= De; \
    Bmi = ROL64(A##ke, 10); \
    A##mi ^= Di; \
    Bmo = ROL64(A##mi, 15); \
    A##so ^= Do; \
    Bmu = ROL64(A##so, 56); \
    E##ma =   Bma ^(  Bme &  Bmi ); \
    Ca ^= E##ma; \
    E##me =   Bme ^(  Bmi |  Bmo ); \
    Ce ^= E##me; \
    E##mi =   Bmi ^((~Bmo)|  Bmu ); \
    Ci ^= E##mi; \
    E##mo = (~Bmo)^(  Bmu &  Bma ); \
    Co ^= E##mo; \
    E##mu =   Bmu ^(  Bma |  Bme ); \
    Cu ^= E##mu; \
\
    A##bi ^= Di; \
    Bsa = ROL64(A##bi, 62); \
    A##go ^= Do; \
    Bse = ROL64(A##go, 55); \
    A##ku ^= Du; \
    Bsi = ROL64(A##ku, 39); \
    A##ma ^= Da; \
    Bso = ROL64(A##ma, 41); \
    A##se ^= De; \
    Bsu = ROL64(A##se, 2); \
    E##sa =   Bsa ^((~Bse)&  Bsi ); \
    Ca ^= E##sa; \
    E##se = (~Bse)^(  Bsi |  Bso ); \
    Ce ^= E##se; \
    E##si =   Bsi ^(  Bso &  Bsu ); \
    Ci ^= E##si; \
    E##so =   Bso ^(  Bsu |  Bsa ); \
    Co ^= E##so; \
    E##su =   Bsu ^(  Bsa &  Bse ); \
    Cu ^= E##su; \
\

/* --- Code for round (lane complementing pattern 'bebigokimisa') */

/* --- 64-bit lanes mapped to 64-bit words */

#define thetaRhoPiChiIota(i, A, E) \
    Da = Cu^ROL64(Ce, 1); \
    De = Ca^ROL64(Ci, 1); \
    Di = Ce^ROL64(Co, 1); \
    Do = Ci^ROL64(Cu, 1); \
    Du = Co^ROL64(Ca, 1); \
\
    A##ba ^= Da; \
    Bba = A##ba; \
    A##ge ^= De; \
    Bbe = ROL64(A##ge, 44); \
    A##ki ^= Di; \
    Bbi = ROL64(A##ki, 43); \
    A##mo ^= Do; \
    Bbo = ROL64(A##mo, 21); \
    A##su ^= Du; \
    Bbu = ROL64(A##su, 14); \
    E##ba =   Bba ^(  Bbe |  Bbi ); \
    E##ba ^= KeccakF1600RoundConstants[i]; \
    E##be =   Bbe ^((~Bbi)|  Bbo ); \
    E##bi =   Bbi ^(  Bbo &  Bbu ); \
    E##bo =   Bbo ^(  Bbu |  Bba ); \
    E##bu =   Bbu ^(  Bba &  Bbe ); \
\
    A##bo ^= Do; \
    Bga = ROL64(A##bo, 28); \
    A##gu ^= Du; \
    Bge = ROL64(A##gu, 20); \
    A##ka ^= Da; \
    Bgi = ROL64(A##ka, 3); \
    A##me ^= De; \
    Bgo = ROL64(A##me, 45); \
    A##si ^= Di; \
    Bgu = ROL64(A##si, 61); \
    E##ga =   Bga ^(  Bge |  Bgi ); \
    E##ge =   Bge ^(  Bgi &  Bgo ); \
    E##gi =   Bgi ^(  Bgo |(~Bgu)); \
    E##go =   Bgo ^(  Bgu |  Bga ); \
    E##gu =   Bgu ^(  Bga &  Bge ); \
\
    A##be ^= De; \
    Bka = ROL64(A##be, 1); \
    A##gi ^= Di; \
    Bke = ROL64(A##gi, 6); \
    A##ko ^= Do; \
    Bki = ROL64(A##ko, 25); \
    A##mu ^= Du; \
    Bko = ROL64(A##mu, 8); \
    A##sa ^= Da; \
    Bku = ROL64(A##sa, 18); \
    E##ka =   Bka ^(  Bke |  Bki ); \
    E##ke =   Bke ^(  Bki &  Bko ); \
    E##ki =   Bki ^((~Bko)&  Bku ); \
    E##ko = (~Bko)^(  Bku |  Bka ); \
    E##ku =   Bku ^(  Bka &  Bke ); \
\
    A##bu ^= Du; \
    Bma = ROL64(A##bu, 27); \
    A##ga ^= Da; \
    Bme = ROL64(A##ga, 36); \
    A##ke ^= De; \
    Bmi = ROL64(A##ke, 10); \
    A##mi ^= Di; \
    Bmo = ROL64(A##mi, 15); \
    A##so ^= Do; \
    Bmu = ROL64(A##so, 56); \
    E##ma =   Bma ^(  Bme &  Bmi ); \
    E##me =   Bme ^(  Bmi |  Bmo ); \
    E##mi =   Bmi ^((~Bmo)|  Bmu ); \
    E##mo = (~Bmo)^(  Bmu &  Bma ); \
    E##mu =   Bmu ^(  Bma |  Bme ); \
\
    A##bi ^= Di; \
    Bsa = ROL64(A##bi, 62); \
    A##go ^= Do; \
    Bse = ROL64(A##go, 55); \
    A##ku ^= Du; \
    Bsi = ROL64(A##ku, 39); \
    A##ma ^= Da; \
    Bso = ROL64(A##ma, 41); \
    A##se ^= De; \
    Bsu = ROL64(A##se, 2); \
    E##sa =   Bsa ^((~Bse)&  Bsi ); \
    E##se = (~Bse)^(  Bsi |  Bso ); \
    E##si =   Bsi ^(  Bso &  Bsu ); \
    E##so =   Bso ^(  Bsu |  Bsa ); \
    E##su =   Bsu ^(  Bsa &  Bse ); \
\

#else /* UseBebigokimisa */

/* --- Code for round, with prepare-theta */

/* --- 64-bit lanes mapped to 64-bit words */

#define thetaRhoPiChiIotaPrepareTheta(i, A, E) \
    Da = Cu^ROL64(Ce, 1); \
    De = Ca^ROL64(Ci, 1); \
    Di = Ce^ROL64(Co, 1); \
    Do = Ci^ROL64(Cu, 1); \
    Du = Co^ROL64(Ca, 1); \
\
    A##ba ^= Da; \
    Bba = A##ba; \
    A##ge ^= De; \
    Bbe = ROL64(A##ge, 44); \
    A##ki ^= Di; \
    Bbi = ROL64(A##ki, 43); \
    A##mo ^= Do; \
    Bbo = ROL64(A##mo, 21); \
    A##su ^= Du; \
    Bbu = ROL64(A##su, 14); \
    E##ba =   Bba ^((~Bbe)&  Bbi ); \
    E##ba ^= KeccakF1600RoundConstants[i]; \
    Ca = E##ba; \
    E##be =   Bbe ^((~Bbi)&  Bbo ); \
    Ce = E##be; \
    E##bi =   Bbi ^((~Bbo)&  Bbu ); \
    Ci = E##bi; \
    E##bo =   Bbo ^((~Bbu)&  Bba ); \
    Co = E##bo; \
    E##bu =   Bbu ^((~Bba)&  Bbe ); \
    Cu = E##bu; \
\
    A##bo ^= Do; \
    Bga = ROL64(A##bo, 28); \
    A##gu ^= Du; \
    Bge = ROL64(A##gu, 20); \
    A##ka ^= Da; \
    Bgi = ROL64(A##ka, 3); \
    A##me ^= De; \
    Bgo = ROL64(A##me, 45); \
    A##si ^= Di; \
    Bgu = ROL64(A##si, 61); \
    E##ga =   Bga ^((~Bge)&  Bgi ); \
    Ca ^= E##ga; \
    E##ge =   Bge ^((~Bgi)&  Bgo ); \
    Ce ^= E##ge; \
    E##gi =   Bgi ^((~Bgo)&  Bgu ); \
    Ci ^= E##gi; \
    E##go =   Bgo ^((~Bgu)&  Bga ); \
    Co ^= E##go; \
    E##gu =   Bgu ^((~Bga)&  Bge ); \
    Cu ^= E##gu; \
\
    A##be ^= De; \
    Bka = ROL64(A##be, 1); \
    A##gi ^= Di; \
    Bke = ROL64(A##gi, 6); \
    A##ko ^= Do; \
    Bki = ROL64(A##ko, 25); \
    A##mu ^= Du; \
    Bko = ROL64(A##mu, 8); \
    A##sa ^= Da; \
    Bku = ROL64(A##sa, 18); \
    E##ka =   Bka ^((~Bke)&  Bki ); \
    Ca ^= E##ka; \
    E##ke =   Bke ^((~Bki)&  Bko ); \
    Ce ^= E##ke; \
    E##ki =   Bki ^((~Bko)&  Bku ); \
    Ci ^= E##ki; \
    E##ko =   Bko ^((~Bku)&  Bka ); \
    Co ^= E##ko; \
    E##ku =   Bku ^((~Bka)&  Bke ); \
    Cu ^= E##ku; \
\
    A##bu ^= Du; \
    Bma = ROL64(A##bu, 27); \
    A##ga ^= Da; \
    Bme = ROL64(A##ga, 36); \
    A##ke ^= De; \
    Bmi = ROL64(A##ke, 10); \
    A##mi ^= Di; \
    Bmo = ROL64(A##mi, 15); \
    A##so ^= Do; \
    Bmu = ROL64(A##so, 56); \
    E##ma =   Bma ^((~Bme)&  Bmi ); \
    Ca ^= E##ma; \
    E##me =   Bme ^((~Bmi)&  Bmo ); \
    Ce ^= E##me; \
    E##mi =   Bmi ^((~Bmo)&  Bmu ); \
    Ci ^= E##mi; \
    E##mo =   Bmo ^((~Bmu)&  Bma ); \
    Co ^= E##mo; \
    E##mu =   Bmu ^((~Bma)&  Bme ); \
    Cu ^= E##mu; \
\
    A##bi ^= Di; \
    Bsa = ROL64(A##bi, 62); \
    A##go ^= Do; \
    Bse = ROL64(A##go, 55); \
    A##ku ^= Du; \
    Bsi = ROL64(A##ku, 39); \
    A##ma ^= Da; \
    Bso = ROL64(A##ma, 41); \
    A##se ^= De; \
    Bsu = ROL64(A##se, 2); \
    E##sa =   Bsa ^((~Bse)&  Bsi ); \
    Ca ^= E##sa; \
    E##se =   Bse ^((~Bsi)&  Bso ); \
    Ce ^= E##se; \
    E##si =   Bsi ^((~Bso)&  Bsu ); \
    Ci ^= E##si; \
    E##so =   Bso ^((~Bsu)&  Bsa ); \
    Co ^= E##so; \
    E##su =   Bsu ^((~Bsa)&  Bse ); \
    Cu ^= E##su; \
\

/* --- Code for round */

/* --- 64-bit lanes mapped to 64-bit words */

#define thetaRhoPiChiIota(i, A, E) \
    Da = Cu^ROL64(Ce, 1); \
    De = Ca^ROL64(Ci, 1); \
    Di = Ce^ROL64(Co, 1); \
    Do = Ci^ROL64(Cu, 1); \
    Du = Co^ROL64(Ca, 1); \
\
    A##ba ^= Da; \
    Bba = A##ba; \
    A##ge ^= De; \
    Bbe = ROL64(A##ge, 44); \
    A##ki ^= Di; \
    Bbi = ROL64(A##ki, 43); \
    A##mo ^= Do; \
    Bbo = ROL64(A##mo, 21); \
    A##su ^= Du; \
    Bbu = ROL64(A##su, 14); \
    E##ba =   Bba ^((~Bbe)&  Bbi ); \
    E##ba ^= KeccakF1600RoundConstants[i]; \
    E##be =   Bbe ^((~Bbi)&  Bbo ); \
    E##bi =   Bbi ^((~Bbo)&  Bbu ); \
    E##bo =   Bbo ^((~Bbu)&  Bba ); \
    E##bu =   Bbu ^((~Bba)&  Bbe ); \
\
    A##bo ^= Do; \
    Bga = ROL64(A##bo, 28); \
    A##gu ^= Du; \
    Bge = ROL64(A##gu, 20); \
    A##ka ^= Da; \
    Bgi = ROL64(A##ka, 3); \
    A##me ^= De; \
    Bgo = ROL64(A##me, 45); \
    A##si ^= Di; \
    Bgu = ROL64(A##si, 61); \
    E##ga =   Bga ^((~Bge)&  Bgi ); \
    E##ge =   Bge ^((~Bgi)&  Bgo ); \
    E##gi =   Bgi ^((~Bgo)&  Bgu ); \
    E##go =   Bgo ^((~Bgu)&  Bga ); \
    E##gu =   Bgu ^((~Bga)&  Bge ); \
\
    A##be ^= De; \
    Bka = ROL64(A##be, 1); \
    A##gi ^= Di; \
    Bke = ROL64(A##gi, 6); \
    A##ko ^= Do; \
    Bki = ROL64(A##ko, 25); \
    A##mu ^= Du; \
    Bko = ROL64(A##mu, 8); \
    A##sa ^= Da; \
    Bku = ROL64(A##sa, 18); \
    E##ka =   Bka ^((~Bke)&  Bki ); \
    E##ke =   Bke ^((~Bki)&  Bko ); \
    E##ki =   Bki ^((~Bko)&  Bku ); \
    E##ko =   Bko ^((~Bku)&  Bka ); \
    E##ku =   Bku ^((~Bka)&  Bke ); \
\
    A##bu ^= Du; \
    Bma = ROL64(A##bu, 27); \
    A##ga ^= Da; \
    Bme = ROL64(A##ga, 36); \
    A##ke ^= De; \
    Bmi = ROL64(A##ke, 10); \
    A##mi ^= Di; \
    Bmo = ROL64(A##mi, 15); \
    A##so ^= Do; \
    Bmu = ROL64(A##so, 56); \
    E##ma =   Bma ^((~Bme)&  Bmi ); \
    E##me =   Bme ^((~Bmi)&  Bmo ); \
    E##mi =   Bmi ^((~Bmo)&  Bmu ); \
    E##mo =   Bmo ^((~Bmu)&  Bma ); \
    E##mu =   Bmu ^((~Bma)&  Bme ); \
\
    A##bi ^= Di; \
    Bsa = ROL64(A##bi, 62); \
    A##go ^= Do; \
    Bse = ROL64(A##go, 55); \
    A##ku ^= Du; \
    Bsi = ROL64(A##ku, 39); \
    A##ma ^= Da; \
    Bso = ROL64(A##ma, 41); \
    A##se ^= De; \
    Bsu = ROL64(A##se, 2); \
    E##sa =   Bsa ^((~Bse)&  Bsi ); \
    E##se =   Bse ^((~Bsi)&  Bso ); \
    E##si =   Bsi ^((~Bso)&  Bsu ); \
    E##so =   Bso ^((~Bsu)&  Bsa ); \
    E##su =   Bsu ^((~Bsa)&  Bse ); \
\

#endif /* UseBebigokimisa */


#define copyFromState(X, state) \
    X##ba = state[ 0]; \
    X##be = state[ 1]; \
    X##bi = state[ 2]; \
    X##bo = state[ 3]; \
    X##bu = state[ 4]; \
    X##ga = state[ 5]; \
    X##ge = state[ 6]; \
    X##gi = state[ 7]; \
    X##go = state[ 8]; \
    X##gu = state[ 9]; \
    X##ka = state[10]; \
    X##ke = state[11]; \
    X##ki = state[12]; \
    X##ko = state[13]; \
    X##ku = state[14]; \
    X##ma = state[15]; \
    X##me = state[16]; \
    X##mi = state[17]; \
    X##mo = state[18]; \
    X##mu = state[19]; \
    X##sa = state[20]; \
    X##se = state[21]; \
    X##si = state[22]; \
    X##so = state[23]; \
    X##su = state[24]; \

#define copyToState(state, X) \
    state[ 0] = X##ba; \
    state[ 1] = X##be; \
    state[ 2] = X##bi; \
    state[ 3] = X##bo; \
    state[ 4] = X##bu; \
    state[ 5] = X##ga; \
    state[ 6] = X##ge; \
    state[ 7] = X##gi; \
    state[ 8] = X##go; \
    state[ 9] = X##gu; \
    state[10] = X##ka; \
    state[11] = X##ke; \
    state[12] = X##ki; \
    state[13] = X##ko; \
    state[14] = X##ku; \
    state[15] = X##ma; \
    state[16] = X##me; \
    state[17] = X##mi; \
    state[18] = X##mo; \
    state[19] = X##mu; \
    state[20] = X##sa; \
    state[21] = X##se; \
    state[22] = X##si; \
    state[23] = X##so; \
    state[24] = X##su; \

#define copyStateVariables(X, Y) \
    X##ba = Y##ba; \
    X##be = Y##be; \
    X##bi = Y##bi; \
    X##bo = Y##bo; \
    X##bu = Y##bu; \
    X##ga = Y##ga; \
    X##ge = Y##ge; \
    X##gi = Y##gi; \
    X##go = Y##go; \
    X##gu = Y##gu; \
    X##ka = Y##ka; \
    X##ke = Y##ke; \
    X##ki = Y##ki; \
    X##ko = Y##ko; \
    X##ku = Y##ku; \
    X##ma = Y##ma; \
    X##me = Y##me; \
    X##mi = Y##mi; \
    X##mo = Y##mo; \
    X##mu = Y##mu; \
    X##sa = Y##sa; \
    X##se = Y##se; \
    X##si = Y##si; \
    X##so = Y##so; \
    X##su = Y##su; \

#define copyFromStateAndAdd(X, state, input, laneCount) \
    if (laneCount < 16) { \
        if (laneCount < 8) { \
            if (laneCount < 4) { \
                if (laneCount < 2) { \
                    if (laneCount < 1) { \
                        X##ba = state[ 0]; \
                    } \
                    else { \
                        X##ba = state[ 0]^input[ 0]; \
                    } \
                    X##be = state[ 1]; \
                    X##bi = state[ 2]; \
                } \
                else { \
                    X##ba = state[ 0]^input[ 0]; \
                    X##be = state[ 1]^input[ 1]; \
                    if (laneCount < 3) { \
                        X##bi = state[ 2]; \
                    } \
                    else { \
                        X##bi = state[ 2]^input[ 2]; \
                    } \
                } \
                X##bo = state[ 3]; \
                X##bu = state[ 4]; \
                X##ga = state[ 5]; \
                X##ge = state[ 6]; \
            } \
            else { \
                X##ba = state[ 0]^input[ 0]; \
                X##be = state[ 1]^input[ 1]; \
                X##bi = state[ 2]^input[ 2]; \
                X##bo = state[ 3]^input[ 3]; \
                if (laneCount < 6) { \
                    if (laneCount < 5) { \
                        X##bu = state[ 4]; \
                    } \
                    else { \
                        X##bu = state[ 4]^input[ 4]; \
                    } \
                    X##ga = state[ 5]; \
                    X##ge = state[ 6]; \
                } \
                else { \
                    X##bu = state[ 4]^input[ 4]; \
                    X##ga = state[ 5]^input[ 5]; \
                    if (laneCount < 7) { \
                        X##ge = state[ 6]; \
                    } \
                    else { \
                        X##ge = state[ 6]^input[ 6]; \
                    } \
                } \
            } \
            X##gi = state[ 7]; \
            X##go = state[ 8]; \
            X##gu = state[ 9]; \
            X##ka = state[10]; \
            X##ke = state[11]; \
            X##ki = state[12]; \
            X##ko = state[13]; \
            X##ku = state[14]; \
        } \
        else { \
            X##ba = state[ 0]^input[ 0]; \
            X##be = state[ 1]^input[ 1]; \
            X##bi = state[ 2]^input[ 2]; \
            X##bo = state[ 3]^input[ 3]; \
            X##bu = state[ 4]^input[ 4]; \
            X##ga = state[ 5]^input[ 5]; \
            X##ge = state[ 6]^input[ 6]; \
            X##gi = state[ 7]^input[ 7]; \
            if (laneCount < 12) { \
                if (laneCount < 10) { \
                    if (laneCount < 9) { \
                        X##go = state[ 8]; \
                    } \
                    else { \
                        X##go = state[ 8]^input[ 8]; \
                    } \
                    X##gu = state[ 9]; \
                    X##ka = state[10]; \
                } \
                else { \
                    X##go = state[ 8]^input[ 8]; \
                    X##gu = state[ 9]^input[ 9]; \
                    if (laneCount < 11) { \
                        X##ka = state[10]; \
                    } \
                    else { \
                        X##ka = state[10]^input[10]; \
                    } \
                } \
                X##ke = state[11]; \
                X##ki = state[12]; \
                X##ko = state[13]; \
                X##ku = state[14]; \
            } \
            else { \
                X##go = state[ 8]^input[ 8]; \
                X##gu = state[ 9]^input[ 9]; \
                X##ka = state[10]^input[10]; \
                X##ke = state[11]^input[11]; \
                if (laneCount < 14) { \
                    if (laneCount < 13) { \
                        X##ki = state[12]; \
                    } \
                    else { \
                        X##ki = state[12]^input[12]; \
                    } \
                    X##ko = state[13]; \
                    X##ku = state[14]; \
                } \
                else { \
                    X##ki = state[12]^input[12]; \
                    X##ko = state[13]^input[13]; \
                    if (laneCount < 15) { \
                        X##ku = state[14]; \
                    } \
                    else { \
                        X##ku = state[14]^input[14]; \
                    } \
                } \
            } \
        } \
        X##ma = state[15]; \
        X##me = state[16]; \
        X##mi = state[17]; \
        X##mo = state[18]; \
        X##mu = state[19]; \
        X##sa = state[20]; \
        X##se = state[21]; \
        X##si = state[22]; \
        X##so = state[23]; \
        X##su = state[24]; \
    } \
    else { \
        X##ba = state[ 0]^input[ 0]; \
        X##be = state[ 1]^input[ 1]; \
        X##bi = state[ 2]^input[ 2]; \
        X##bo = state[ 3]^input[ 3]; \
        X##bu = state[ 4]^input[ 4]; \
        X##ga = state[ 5]^input[ 5]; \
        X##ge = state[ 6]^input[ 6]; \
        X##gi = state[ 7]^input[ 7]; \
        X##go = state[ 8]^input[ 8]; \
        X##gu = state[ 9]^input[ 9]; \
        X##ka = state[10]^input[10]; \
        X##ke = state[11]^input[11]; \
        X##ki = state[12]^input[12]; \
        X##ko = state[13]^input[13]; \
        X##ku = state[14]^input[14]; \
        X##ma = state[15]^input[15]; \
        if (laneCount < 24) { \
            if (laneCount < 20) { \
                if (laneCount < 18) { \
                    if (laneCount < 17) { \
                        X##me = state[16]; \
                    } \
                    else { \
                        X##me = state[16]^input[16]; \
                    } \
                    X##mi = state[17]; \
                    X##mo = state[18]; \
                } \
                else { \
                    X##me = state[16]^input[16]; \
                    X##mi = state[17]^input[17]; \
                    if (laneCount < 19) { \
                        X##mo = state[18]; \
                    } \
                    else { \
                        X##mo = state[18]^input[18]; \
                    } \
                } \
                X##mu = state[19]; \
                X##sa = state[20]; \
                X##se = state[21]; \
                X##si = state[22]; \
            } \
            else { \
                X##me = state[16]^input[16]; \
                X##mi = state[17]^input[17]; \
                X##mo = state[18]^input[18]; \
                X##mu = state[19]^input[19]; \
                if (laneCount < 22) { \
                    if (laneCount < 21) { \
                        X##sa = state[20]; \
                    } \
                    else { \
                        X##sa = state[20]^input[20]; \
                    } \
                    X##se = state[21]; \
                    X##si = state[22]; \
                } \
                else { \
                    X##sa = state[20]^input[20]; \
                    X##se = state[21]^input[21]; \
                    if (laneCount < 23) { \
                        X##si = state[22]; \
                    } \
                    else { \
                        X##si = state[22]^input[22]; \
                    } \
                } \
            } \
            X##so = state[23]; \
            X##su = state[24]; \
        } \
        else { \
            X##me = state[16]^input[16]; \
            X##mi = state[17]^input[17]; \
            X##mo = state[18]^input[18]; \
            X##mu = state[19]^input[19]; \
            X##sa = state[20]^input[20]; \
            X##se = state[21]^input[21]; \
            X##si = state[22]^input[22]; \
            X##so = state[23]^input[23]; \
            if (laneCount < 25) { \
                X##su = state[24]; \
            } \
            else { \
                X##su = state[24]^input[24]; \
            } \
        } \
    }

#define addInput(X, input, laneCount) \
    if (laneCount == 21) { \
        X##ba ^= input[ 0]; \
        X##be ^= input[ 1]; \
        X##bi ^= input[ 2]; \
        X##bo ^= input[ 3]; \
        X##bu ^= input[ 4]; \
        X##ga ^= input[ 5]; \
        X##ge ^= input[ 6]; \
        X##gi ^= input[ 7]; \
        X##go ^= input[ 8]; \
        X##gu ^= input[ 9]; \
        X##ka ^= input[10]; \
        X##ke ^= input[11]; \
        X##ki ^= input[12]; \
        X##ko ^= input[13]; \
        X##ku ^= input[14]; \
        X##ma ^= input[15]; \
        X##me ^= input[16]; \
        X##mi ^= input[17]; \
        X##mo ^= input[18]; \
        X##mu ^= input[19]; \
        X##sa ^= input[20]; \
    } \
    else if (laneCount < 16) { \
        if (laneCount < 8) { \
            if (laneCount < 4) { \
                if (laneCount < 2) { \
                    if (laneCount < 1) { \
                    } \
                    else { \
                        X##ba ^= input[ 0]; \
                    } \
                } \
                else { \
                    X##ba ^= input[ 0]; \
                    X##be ^= input[ 1]; \
                    if (laneCount < 3) { \
                    } \
                    else { \
                        X##bi ^= input[ 2]; \
                    } \
                } \
            } \
            else { \
                X##ba ^= input[ 0]; \
                X##be ^= input[ 1]; \
                X##bi ^= input[ 2]; \
                X##bo ^= input[ 3]; \
                if (laneCount < 6) { \
                    if (laneCount < 5) { \
                    } \
                    else { \
                        X##bu ^= input[ 4]; \
                    } \
                } \
                else { \
                    X##bu ^= input[ 4]; \
                    X##ga ^= input[ 5]; \
                    if (laneCount < 7) { \
                    } \
                    else { \
                        X##ge ^= input[ 6]; \
                    } \
                } \
            } \
        } \
        else { \
            X##ba ^= input[ 0]; \
            X##be ^= input[ 1]; \
            X##bi ^= input[ 2]; \
            X##bo ^= input[ 3]; \
            X##bu ^= input[ 4]; \
            X##ga ^= input[ 5]; \
            X##ge ^= input[ 6]; \
            X##gi ^= input[ 7]; \
            if (laneCount < 12) { \
                if (laneCount < 10) { \
                    if (laneCount < 9) { \
                    } \
                    else { \
                        X##go ^= input[ 8]; \
                    } \
                } \
                else { \
                    X##go ^= input[ 8]; \
                    X##gu ^= input[ 9]; \
                    if (laneCount < 11) { \
                    } \
                    else { \
                        X##ka ^= input[10]; \
                    } \
                } \
            } \
            else { \
                X##go ^= input[ 8]; \
                X##gu ^= input[ 9]; \
                X##ka ^= input[10]; \
                X##ke ^= input[11]; \
                if (laneCount < 14) { \
                    if (laneCount < 13) { \
                    } \
                    else { \
                        X##ki ^= input[12]; \
                    } \
                } \
                else { \
                    X##ki ^= input[12]; \
                    X##ko ^= input[13]; \
                    if (laneCount < 15) { \
                    } \
                    else { \
                        X##ku ^= input[14]; \
                    } \
                } \
            } \
        } \
    } \
    else { \
        X##ba ^= input[ 0]; \
        X##be ^= input[ 1]; \
        X##bi ^= input[ 2]; \
        X##bo ^= input[ 3]; \
        X##bu ^= input[ 4]; \
        X##ga ^= input[ 5]; \
        X##ge ^= input[ 6]; \
        X##gi ^= input[ 7]; \
        X##go ^= input[ 8]; \
        X##gu ^= input[ 9]; \
        X##ka ^= input[10]; \
        X##ke ^= input[11]; \
        X##ki ^= input[12]; \
        X##ko ^= input[13]; \
        X##ku ^= input[14]; \
        X##ma ^= input[15]; \
        if (laneCount < 24) { \
            if (laneCount < 20) { \
                if (laneCount < 18) { \
                    if (laneCount < 17) { \
                    } \
                    else { \
                        X##me ^= input[16]; \
                    } \
                } \
                else { \
                    X##me ^= input[16]; \
                    X##mi ^= input[17]; \
                    if (laneCount < 19) { \
                    } \
                    else { \
                        X##mo ^= input[18]; \
                    } \
                } \
            } \
            else { \
                X##me ^= input[16]; \
                X##mi ^= input[17]; \
                X##mo ^= input[18]; \
                X##mu ^= input[19]; \
                if (laneCount < 22) { \
                    if (laneCount < 21) { \
                    } \
                    else { \
                        X##sa ^= input[20]; \
                    } \
                } \
                else { \
                    X##sa ^= input[20]; \
                    X##se ^= input[21]; \
                    if (laneCount < 23) { \
                    } \
                    else { \
                        X##si ^= input[22]; \
                    } \
                } \
            } \
        } \
        else { \
            X##me ^= input[16]; \
            X##mi ^= input[17]; \
            X##mo ^= input[18]; \
            X##mu ^= input[19]; \
            X##sa ^= input[20]; \
            X##se ^= input[21]; \
            X##si ^= input[22]; \
            X##so ^= input[23]; \
            if (laneCount < 25) { \
            } \
            else { \
                X##su ^= input[24]; \
            } \
        } \
    }

#ifdef UseBebigokimisa

#define copyToStateAndOutput(X, state, output, laneCount) \
    if (laneCount < 16) { \
        if (laneCount < 8) { \
            if (laneCount < 4) { \
                if (laneCount < 2) { \
                    state[ 0] = X##ba; \
                    if (laneCount >= 1) { \
                        output[ 0] = X##ba; \
                    } \
                    state[ 1] = X##be; \
                    state[ 2] = X##bi; \
                } \
                else { \
                    state[ 0] = X##ba; \
                    output[ 0] = X##ba; \
                    state[ 1] = X##be; \
                    output[ 1] = ~X##be; \
                    state[ 2] = X##bi; \
                    if (laneCount >= 3) { \
                        output[ 2] = ~X##bi; \
                    } \
                } \
                state[ 3] = X##bo; \
                state[ 4] = X##bu; \
                state[ 5] = X##ga; \
                state[ 6] = X##ge; \
            } \
            else { \
                state[ 0] = X##ba; \
                output[ 0] = X##ba; \
                state[ 1] = X##be; \
                output[ 1] = ~X##be; \
                state[ 2] = X##bi; \
                output[ 2] = ~X##bi; \
                state[ 3] = X##bo; \
                output[ 3] = X##bo; \
                if (laneCount < 6) { \
                    state[ 4] = X##bu; \
                    if (laneCount >= 5) { \
                        output[ 4] = X##bu; \
                    } \
                    state[ 5] = X##ga; \
                    state[ 6] = X##ge; \
                } \
                else { \
                    state[ 4] = X##bu; \
                    output[ 4] = X##bu; \
                    state[ 5] = X##ga; \
                    output[ 5] = X##ga; \
                    state[ 6] = X##ge; \
                    if (laneCount >= 7) { \
                        output[ 6] = X##ge; \
                    } \
                } \
            } \
            state[ 7] = X##gi; \
            state[ 8] = X##go; \
            state[ 9] = X##gu; \
            state[10] = X##ka; \
            state[11] = X##ke; \
            state[12] = X##ki; \
            state[13] = X##ko; \
            state[14] = X##ku; \
        } \
        else { \
            state[ 0] = X##ba; \
            output[ 0] = X##ba; \
            state[ 1] = X##be; \
            output[ 1] = ~X##be; \
            state[ 2] = X##bi; \
            output[ 2] = ~X##bi; \
            state[ 3] = X##bo; \
            output[ 3] = X##bo; \
            state[ 4] = X##bu; \
            output[ 4] = X##bu; \
            state[ 5] = X##ga; \
            output[ 5] = X##ga; \
            state[ 6] = X##ge; \
            output[ 6] = X##ge; \
            state[ 7] = X##gi; \
            output[ 7] = X##gi; \
            if (laneCount < 12) { \
                if (laneCount < 10) { \
                    state[ 8] = X##go; \
                    if (laneCount >= 9) { \
                        output[ 8] = ~X##go; \
                    } \
                    state[ 9] = X##gu; \
                    state[10] = X##ka; \
                } \
                else { \
                    state[ 8] = X##go; \
                    output[ 8] = ~X##go; \
                    state[ 9] = X##gu; \
                    output[ 9] = X##gu; \
                    state[10] = X##ka; \
                    if (laneCount >= 11) { \
                        output[10] = X##ka; \
                    } \
                } \
                state[11] = X##ke; \
                state[12] = X##ki; \
                state[13] = X##ko; \
                state[14] = X##ku; \
            } \
            else { \
                state[ 8] = X##go; \
                output[ 8] = ~X##go; \
                state[ 9] = X##gu; \
                output[ 9] = X##gu; \
                state[10] = X##ka; \
                output[10] = X##ka; \
                state[11] = X##ke; \
                output[11] = X##ke; \
                if (laneCount < 14) { \
                    state[12] = X##ki; \
                    if (laneCount >= 13) { \
                        output[12] = ~X##ki; \
                    } \
                    state[13] = X##ko; \
                    state[14] = X##ku; \
                } \
                else { \
                    state[12] = X##ki; \
                    output[12] = ~X##ki; \
                    state[13] = X##ko; \
                    output[13] = X##ko; \
                    state[14] = X##ku; \
                    if (laneCount >= 15) { \
                        output[14] = X##ku; \
                    } \
                } \
            } \
        } \
        state[15] = X##ma; \
        state[16] = X##me; \
        state[17] = X##mi; \
        state[18] = X##mo; \
        state[19] = X##mu; \
        state[20] = X##sa; \
        state[21] = X##se; \
        state[22] = X##si; \
        state[23] = X##so; \
        state[24] = X##su; \
    } \
    else { \
        state[ 0] = X##ba; \
        output[ 0] = X##ba; \
        state[ 1] = X##be; \
        output[ 1] = ~X##be; \
        state[ 2] = X##bi; \
        output[ 2] = ~X##bi; \
        state[ 3] = X##bo; \
        output[ 3] = X##bo; \
        state[ 4] = X##bu; \
        output[ 4] = X##bu; \
        state[ 5] = X##ga; \
        output[ 5] = X##ga; \
        state[ 6] = X##ge; \
        output[ 6] = X##ge; \
        state[ 7] = X##gi; \
        output[ 7] = X##gi; \
        state[ 8] = X##go; \
        output[ 8] = ~X##go; \
        state[ 9] = X##gu; \
        output[ 9] = X##gu; \
        state[10] = X##ka; \
        output[10] = X##ka; \
        state[11] = X##ke; \
        output[11] = X##ke; \
        state[12] = X##ki; \
        output[12] = ~X##ki; \
        state[13] = X##ko; \
        output[13] = X##ko; \
        state[14] = X##ku; \
        output[14] = X##ku; \
        state[15] = X##ma; \
        output[15] = X##ma; \
        if (laneCount < 24) { \
            if (laneCount < 20) { \
                if (laneCount < 18) { \
                    state[16] = X##me; \
                    if (laneCount >= 17) { \
                        output[16] = X##me; \
                    } \
                    state[17] = X##mi; \
                    state[18] = X##mo; \
                } \
                else { \
                    state[16] = X##me; \
                    output[16] = X##me; \
                    state[17] = X##mi; \
                    output[17] = ~X##mi; \
                    state[18] = X##mo; \
                    if (laneCount >= 19) { \
                        output[18] = X##mo; \
                    } \
                } \
                state[19] = X##mu; \
                state[20] = X##sa; \
                state[21] = X##se; \
                state[22] = X##si; \
            } \
            else { \
                state[16] = X##me; \
                output[16] = X##me; \
                state[17] = X##mi; \
                output[17] = ~X##mi; \
                state[18] = X##mo; \
                output[18] = X##mo; \
                state[19] = X##mu; \
                output[19] = X##mu; \
                if (laneCount < 22) { \
                    state[20] = X##sa; \
                    if (laneCount >= 21) { \
                        output[20] = ~X##sa; \
                    } \
                    state[21] = X##se; \
                    state[22] = X##si; \
                } \
                else { \
                    state[20] = X##sa; \
                    output[20] = ~X##sa; \
                    state[21] = X##se; \
                    output[21] = X##se; \
                    state[22] = X##si; \
                    if (laneCount >= 23) { \
                        output[22] = X##si; \
                    } \
                } \
            } \
            state[23] = X##so; \
            state[24] = X##su; \
        } \
        else { \
            state[16] = X##me; \
            output[16] = X##me; \
            state[17] = X##mi; \
            output[17] = ~X##mi; \
            state[18] = X##mo; \
            output[18] = X##mo; \
            state[19] = X##mu; \
            output[19] = X##mu; \
            state[20] = X##sa; \
            output[20] = ~X##sa; \
            state[21] = X##se; \
            output[21] = X##se; \
            state[22] = X##si; \
            output[22] = X##si; \
            state[23] = X##so; \
            output[23] = X##so; \
            state[24] = X##su; \
            if (laneCount >= 25) { \
                output[24] = X##su; \
            } \
        } \
    }

#define output(X, output, laneCount) \
    if (laneCount < 16) { \
        if (laneCount < 8) { \
            if (laneCount < 4) { \
                if (laneCount < 2) { \
                    if (laneCount >= 1) { \
                        output[ 0] = X##ba; \
                    } \
                } \
                else { \
                    output[ 0] = X##ba; \
                    output[ 1] = ~X##be; \
                    if (laneCount >= 3) { \
                        output[ 2] = ~X##bi; \
                    } \
                } \
            } \
            else { \
                output[ 0] = X##ba; \
                output[ 1] = ~X##be; \
                output[ 2] = ~X##bi; \
                output[ 3] = X##bo; \
                if (laneCount < 6) { \
                    if (laneCount >= 5) { \
                        output[ 4] = X##bu; \
                    } \
                } \
                else { \
                    output[ 4] = X##bu; \
                    output[ 5] = X##ga; \
                    if (laneCount >= 7) { \
                        output[ 6] = X##ge; \
                    } \
                } \
            } \
        } \
        else { \
            output[ 0] = X##ba; \
            output[ 1] = ~X##be; \
            output[ 2] = ~X##bi; \
            output[ 3] = X##bo; \
            output[ 4] = X##bu; \
            output[ 5] = X##ga; \
            output[ 6] = X##ge; \
            output[ 7] = X##gi; \
            if (laneCount < 12) { \
                if (laneCount < 10) { \
                    if (laneCount >= 9) { \
                        output[ 8] = ~X##go; \
                    } \
                } \
                else { \
                    output[ 8] = ~X##go; \
                    output[ 9] = X##gu; \
                    if (laneCount >= 11) { \
                        output[10] = X##ka; \
                    } \
                } \
            } \
            else { \
                output[ 8] = ~X##go; \
                output[ 9] = X##gu; \
                output[10] = X##ka; \
                output[11] = X##ke; \
                if (laneCount < 14) { \
                    if (laneCount >= 13) { \
                        output[12] = ~X##ki; \
                    } \
                } \
                else { \
                    output[12] = ~X##ki; \
                    output[13] = X##ko; \
                    if (laneCount >= 15) { \
                        output[14] = X##ku; \
                    } \
                } \
            } \
        } \
    } \
    else { \
        output[ 0] = X##ba; \
        output[ 1] = ~X##be; \
        output[ 2] = ~X##bi; \
        output[ 3] = X##bo; \
        output[ 4] = X##bu; \
        output[ 5] = X##ga; \
        output[ 6] = X##ge; \
        output[ 7] = X##gi; \
        output[ 8] = ~X##go; \
        output[ 9] = X##gu; \
        output[10] = X##ka; \
        output[11] = X##ke; \
        output[12] = ~X##ki; \
        output[13] = X##ko; \
        output[14] = X##ku; \
        output[15] = X##ma; \
        if (laneCount < 24) { \
            if (laneCount < 20) { \
                if (laneCount < 18) { \
                    if (laneCount >= 17) { \
                        output[16] = X##me; \
                    } \
                } \
                else { \
                    output[16] = X##me; \
                    output[17] = ~X##mi; \
                    if (laneCount >= 19) { \
                        output[18] = X##mo; \
                    } \
                } \
            } \
            else { \
                output[16] = X##me; \
                output[17] = ~X##mi; \
                output[18] = X##mo; \
                output[19] = X##mu; \
                if (laneCount < 22) { \
                    if (laneCount >= 21) { \
                        output[20] = ~X##sa; \
                    } \
                } \
                else { \
                    output[20] = ~X##sa; \
                    output[21] = X##se; \
                    if (laneCount >= 23) { \
                        output[22] = X##si; \
                    } \
                } \
            } \
        } \
        else { \
            output[16] = X##me; \
            output[17] = ~X##mi; \
            output[18] = X##mo; \
            output[19] = X##mu; \
            output[20] = ~X##sa; \
            output[21] = X##se; \
            output[22] = X##si; \
            output[23] = X##so; \
            if (laneCount >= 25) { \
                output[24] = X##su; \
            } \
        } \
    }

#define wrapOne(X, input, output, index, name) \
    X##name ^= input[index]; \
    output[index] = X##name;

#define wrapOneInvert(X, input, output, index, name) \
    X##name ^= input[index]; \
    output[index] = ~X##name;

#define unwrapOne(X, input, output, index, name) \
    output[index] = input[index] ^ X##name; \
    X##name ^= output[index];

#define unwrapOneInvert(X, input, output, index, name) \
    output[index] = ~(input[index] ^ X##name); \
    X##name ^= output[index]; \

#else /* UseBebigokimisa */


#define copyToStateAndOutput(X, state, output, laneCount) \
    if (laneCount < 16) { \
        if (laneCount < 8) { \
            if (laneCount < 4) { \
                if (laneCount < 2) { \
                    state[ 0] = X##ba; \
                    if (laneCount >= 1) { \
                        output[ 0] = X##ba; \
                    } \
                    state[ 1] = X##be; \
                    state[ 2] = X##bi; \
                } \
                else { \
                    state[ 0] = X##ba; \
                    output[ 0] = X##ba; \
                    state[ 1] = X##be; \
                    output[ 1] = X##be; \
                    state[ 2] = X##bi; \
                    if (laneCount >= 3) { \
                        output[ 2] = X##bi; \
                    } \
                } \
                state[ 3] = X##bo; \
                state[ 4] = X##bu; \
                state[ 5] = X##ga; \
                state[ 6] = X##ge; \
            } \
            else { \
                state[ 0] = X##ba; \
                output[ 0] = X##ba; \
                state[ 1] = X##be; \
                output[ 1] = X##be; \
                state[ 2] = X##bi; \
                output[ 2] = X##bi; \
                state[ 3] = X##bo; \
                output[ 3] = X##bo; \
                if (laneCount < 6) { \
                    state[ 4] = X##bu; \
                    if (laneCount >= 5) { \
                        output[ 4] = X##bu; \
                    } \
                    state[ 5] = X##ga; \
                    state[ 6] = X##ge; \
                } \
                else { \
                    state[ 4] = X##bu; \
                    output[ 4] = X##bu; \
                    state[ 5] = X##ga; \
                    output[ 5] = X##ga; \
                    state[ 6] = X##ge; \
                    if (laneCount >= 7) { \
                        output[ 6] = X##ge; \
                    } \
                } \
            } \
            state[ 7] = X##gi; \
            state[ 8] = X##go; \
            state[ 9] = X##gu; \
            state[10] = X##ka; \
            state[11] = X##ke; \
            state[12] = X##ki; \
            state[13] = X##ko; \
            state[14] = X##ku; \
        } \
        else { \
            state[ 0] = X##ba; \
            output[ 0] = X##ba; \
            state[ 1] = X##be; \
            output[ 1] = X##be; \
            state[ 2] = X##bi; \
            output[ 2] = X##bi; \
            state[ 3] = X##bo; \
            output[ 3] = X##bo; \
            state[ 4] = X##bu; \
            output[ 4] = X##bu; \
            state[ 5] = X##ga; \
            output[ 5] = X##ga; \
            state[ 6] = X##ge; \
            output[ 6] = X##ge; \
            state[ 7] = X##gi; \
            output[ 7] = X##gi; \
            if (laneCount < 12) { \
                if (laneCount < 10) { \
                    state[ 8] = X##go; \
                    if (laneCount >= 9) { \
                        output[ 8] = X##go; \
                    } \
                    state[ 9] = X##gu; \
                    state[10] = X##ka; \
                } \
                else { \
                    state[ 8] = X##go; \
                    output[ 8] = X##go; \
                    state[ 9] = X##gu; \
                    output[ 9] = X##gu; \
                    state[10] = X##ka; \
                    if (laneCount >= 11) { \
                        output[10] = X##ka; \
                    } \
                } \
                state[11] = X##ke; \
                state[12] = X##ki; \
                state[13] = X##ko; \
                state[14] = X##ku; \
            } \
            else { \
                state[ 8] = X##go; \
                output[ 8] = X##go; \
                state[ 9] = X##gu; \
                output[ 9] = X##gu; \
                state[10] = X##ka; \
                output[10] = X##ka; \
                state[11] = X##ke; \
                output[11] = X##ke; \
                if (laneCount < 14) { \
                    state[12] = X##ki; \
                    if (laneCount >= 13) { \
                        output[12]= X##ki; \
                    } \
                    state[13] = X##ko; \
                    state[14] = X##ku; \
                } \
                else { \
                    state[12] = X##ki; \
                    output[12]= X##ki; \
                    state[13] = X##ko; \
                    output[13] = X##ko; \
                    state[14] = X##ku; \
                    if (laneCount >= 15) { \
                        output[14] = X##ku; \
                    } \
                } \
            } \
        } \
        state[15] = X##ma; \
        state[16] = X##me; \
        state[17] = X##mi; \
        state[18] = X##mo; \
        state[19] = X##mu; \
        state[20] = X##sa; \
        state[21] = X##se; \
        state[22] = X##si; \
        state[23] = X##so; \
        state[24] = X##su; \
    } \
    else { \
        state[ 0] = X##ba; \
        output[ 0] = X##ba; \
        state[ 1] = X##be; \
        output[ 1] = X##be; \
        state[ 2] = X##bi; \
        output[ 2] = X##bi; \
        state[ 3] = X##bo; \
        output[ 3] = X##bo; \
        state[ 4] = X##bu; \
        output[ 4] = X##bu; \
        state[ 5] = X##ga; \
        output[ 5] = X##ga; \
        state[ 6] = X##ge; \
        output[ 6] = X##ge; \
        state[ 7] = X##gi; \
        output[ 7] = X##gi; \
        state[ 8] = X##go; \
        output[ 8] = X##go; \
        state[ 9] = X##gu; \
        output[ 9] = X##gu; \
        state[10] = X##ka; \
        output[10] = X##ka; \
        state[11] = X##ke; \
        output[11] = X##ke; \
        state[12] = X##ki; \
        output[12]= X##ki; \
        state[13] = X##ko; \
        output[13] = X##ko; \
        state[14] = X##ku; \
        output[14] = X##ku; \
        state[15] = X##ma; \
        output[15] = X##ma; \
        if (laneCount < 24) { \
            if (laneCount < 20) { \
                if (laneCount < 18) { \
                    state[16] = X##me; \
                    if (laneCount >= 17) { \
                        output[16] = X##me; \
                    } \
                    state[17] = X##mi; \
                    state[18] = X##mo; \
                } \
                else { \
                    state[16] = X##me; \
                    output[16] = X##me; \
                    state[17] = X##mi; \
                    output[17] = X##mi; \
                    state[18] = X##mo; \
                    if (laneCount >= 19) { \
                        output[18] = X##mo; \
                    } \
                } \
                state[19] = X##mu; \
                state[20] = X##sa; \
                state[21] = X##se; \
                state[22] = X##si; \
            } \
            else { \
                state[16] = X##me; \
                output[16] = X##me; \
                state[17] = X##mi; \
                output[17] = X##mi; \
                state[18] = X##mo; \
                output[18] = X##mo; \
                state[19] = X##mu; \
                output[19] = X##mu; \
                if (laneCount < 22) { \
                    state[20] = X##sa; \
                    if (laneCount >= 21) { \
                        output[20] = X##sa; \
                    } \
                    state[21] = X##se; \
                    state[22] = X##si; \
                } \
                else { \
                    state[20] = X##sa; \
                    output[20] = X##sa; \
                    state[21] = X##se; \
                    output[21] = X##se; \
                    state[22] = X##si; \
                    if (laneCount >= 23) { \
                        output[22] = X##si; \
                    } \
                } \
            } \
            state[23] = X##so; \
            state[24] = X##su; \
        } \
        else { \
            state[16] = X##me; \
            output[16] = X##me; \
            state[17] = X##mi; \
            output[17] = X##mi; \
            state[18] = X##mo; \
            output[18] = X##mo; \
            state[19] = X##mu; \
            output[19] = X##mu; \
            state[20] = X##sa; \
            output[20] = X##sa; \
            state[21] = X##se; \
            output[21] = X##se; \
            state[22] = X##si; \
            output[22] = X##si; \
            state[23] = X##so; \
            output[23] = X##so; \
            state[24] = X##su; \
            if (laneCount >= 25) { \
                output[24] = X##su; \
            } \
        } \
    }

#define output(X, output, laneCount) \
    if (laneCount < 16) { \
        if (laneCount < 8) { \
            if (laneCount < 4) { \
                if (laneCount < 2) { \
                    if (laneCount >= 1) { \
                        output[ 0] = X##ba; \
                    } \
                } \
                else { \
                    output[ 0] = X##ba; \
                    output[ 1] = X##be; \
                    if (laneCount >= 3) { \
                        output[ 2] = X##bi; \
                    } \
                } \
            } \
            else { \
                output[ 0] = X##ba; \
                output[ 1] = X##be; \
                output[ 2] = X##bi; \
                output[ 3] = X##bo; \
                if (laneCount < 6) { \
                    if (laneCount >= 5) { \
                        output[ 4] = X##bu; \
                    } \
                } \
                else { \
                    output[ 4] = X##bu; \
                    output[ 5] = X##ga; \
                    if (laneCount >= 7) { \
                        output[ 6] = X##ge; \
                    } \
                } \
            } \
        } \
        else { \
            output[ 0] = X##ba; \
            output[ 1] = X##be; \
            output[ 2] = X##bi; \
            output[ 3] = X##bo; \
            output[ 4] = X##bu; \
            output[ 5] = X##ga; \
            output[ 6] = X##ge; \
            output[ 7] = X##gi; \
            if (laneCount < 12) { \
                if (laneCount < 10) { \
                    if (laneCount >= 9) { \
                        output[ 8] = X##go; \
                    } \
                } \
                else { \
                    output[ 8] = X##go; \
                    output[ 9] = X##gu; \
                    if (laneCount >= 11) { \
                        output[10] = X##ka; \
                    } \
                } \
            } \
            else { \
                output[ 8] = X##go; \
                output[ 9] = X##gu; \
                output[10] = X##ka; \
                output[11] = X##ke; \
                if (laneCount < 14) { \
                    if (laneCount >= 13) { \
                        output[12] = X##ki; \
                    } \
                } \
                else { \
                    output[12] = X##ki; \
                    output[13] = X##ko; \
                    if (laneCount >= 15) { \
                        output[14] = X##ku; \
                    } \
                } \
            } \
        } \
    } \
    else { \
        output[ 0] = X##ba; \
        output[ 1] = X##be; \
        output[ 2] = X##bi; \
        output[ 3] = X##bo; \
        output[ 4] = X##bu; \
        output[ 5] = X##ga; \
        output[ 6] = X##ge; \
        output[ 7] = X##gi; \
        output[ 8] = X##go; \
        output[ 9] = X##gu; \
        output[10] = X##ka; \
        output[11] = X##ke; \
        output[12] = X##ki; \
        output[13] = X##ko; \
        output[14] = X##ku; \
        output[15] = X##ma; \
        if (laneCount < 24) { \
            if (laneCount < 20) { \
                if (laneCount < 18) { \
                    if (laneCount >= 17) { \
                        output[16] = X##me; \
                    } \
                } \
                else { \
                    output[16] = X##me; \
                    output[17] = X##mi; \
                    if (laneCount >= 19) { \
                        output[18] = X##mo; \
                    } \
                } \
            } \
            else { \
                output[16] = X##me; \
                output[17] = X##mi; \
                output[18] = X##mo; \
                output[19] = X##mu; \
                if (laneCount < 22) { \
                    if (laneCount >= 21) { \
                        output[20] = X##sa; \
                    } \
                } \
                else { \
                    output[20] = X##sa; \
                    output[21] = X##se; \
                    if (laneCount >= 23) { \
                        output[22] = X##si; \
                    } \
                } \
            } \
        } \
        else { \
            output[16] = X##me; \
            output[17] = X##mi; \
            output[18] = X##mo; \
            output[19] = X##mu; \
            output[20] = X##sa; \
            output[21] = X##se; \
            output[22] = X##si; \
            output[23] = X##so; \
            if (laneCount >= 25) { \
                output[24] = X##su; \
            } \
        } \
    }

#define wrapOne(X, input, output, index, name) \
    X##name ^= input[index]; \
    output[index] = X##name;

#define wrapOneInvert(X, input, output, index, name) \
    X##name ^= input[index]; \
    output[index] = X##name;

#define unwrapOne(X, input, output, index, name) \
    output[index] = input[index] ^ X##name; \
    X##name ^= output[index];

#define unwrapOneInvert(X, input, output, index, name) \
    output[index] = input[index] ^ X##name; \
    X##name ^= output[index];

#endif

#define wrap(X, input, output, laneCount, trailingBits) \
    if (laneCount < 16) { \
        if (laneCount < 8) { \
            if (laneCount < 4) { \
                if (laneCount < 2) { \
                    if (laneCount < 1) { \
                        X##ba ^= trailingBits; \
                    } \
                    else { \
                        wrapOne(X, input, output, 0, ba) \
                        X##be ^= trailingBits; \
                    } \
                } \
                else { \
                    wrapOne(X, input, output, 0, ba) \
                    wrapOneInvert(X, input, output, 1, be) \
                    if (laneCount < 3) { \
                        X##bi ^= trailingBits; \
                    } \
                    else { \
                        wrapOneInvert(X, input, output, 2, bi) \
                        X##bo ^= trailingBits; \
                    } \
                } \
            } \
            else { \
                wrapOne(X, input, output, 0, ba) \
                wrapOneInvert(X, input, output, 1, be) \
                wrapOneInvert(X, input, output, 2, bi) \
                wrapOne(X, input, output, 3, bo) \
                if (laneCount < 6) { \
                    if (laneCount < 5) { \
                        X##bu ^= trailingBits; \
                    } \
                    else { \
                        wrapOne(X, input, output, 4, bu) \
                        X##ga ^= trailingBits; \
                    } \
                } \
                else { \
                    wrapOne(X, input, output, 4, bu) \
                    wrapOne(X, input, output, 5, ga) \
                    if (laneCount < 7) { \
                        X##ge ^= trailingBits; \
                    } \
                    else { \
                        wrapOne(X, input, output, 6, ge) \
                        X##gi ^= trailingBits; \
                    } \
                } \
            } \
        } \
        else { \
            wrapOne(X, input, output, 0, ba) \
            wrapOneInvert(X, input, output, 1, be) \
            wrapOneInvert(X, input, output, 2, bi) \
            wrapOne(X, input, output, 3, bo) \
            wrapOne(X, input, output, 4, bu) \
            wrapOne(X, input, output, 5, ga) \
            wrapOne(X, input, output, 6, ge) \
            wrapOne(X, input, output, 7, gi) \
            if (laneCount < 12) { \
                if (laneCount < 10) { \
                    if (laneCount < 9) { \
                        X##go ^= trailingBits; \
                    } \
                    else { \
                        wrapOneInvert(X, input, output, 8, go) \
                        X##gu ^= trailingBits; \
                    } \
                } \
                else { \
                    wrapOneInvert(X, input, output, 8, go) \
                    wrapOne(X, input, output, 9, gu) \
                    if (laneCount < 11) { \
                        X##ka ^= trailingBits; \
                    } \
                    else { \
                        wrapOne(X, input, output, 10, ka) \
                        X##ke ^= trailingBits; \
                    } \
                } \
            } \
            else { \
                wrapOneInvert(X, input, output, 8, go) \
                wrapOne(X, input, output, 9, gu) \
                wrapOne(X, input, output, 10, ka) \
                wrapOne(X, input, output, 11, ke) \
                if (laneCount < 14) { \
                    if (laneCount < 13) { \
                        X##ki ^= trailingBits; \
                    } \
                    else { \
                        wrapOneInvert(X, input, output, 12, ki) \
                        X##ko ^= trailingBits; \
                    } \
                } \
                else { \
                    wrapOneInvert(X, input, output, 12, ki) \
                    wrapOne(X, input, output, 13, ko) \
                    if (laneCount < 15) { \
                        X##ku ^= trailingBits; \
                    } \
                    else { \
                        wrapOne(X, input, output, 14, ku) \
                        X##ma ^= trailingBits; \
                    } \
                } \
            } \
        } \
    } \
    else { \
        wrapOne(X, input, output, 0, ba) \
        wrapOneInvert(X, input, output, 1, be) \
        wrapOneInvert(X, input, output, 2, bi) \
        wrapOne(X, input, output, 3, bo) \
        wrapOne(X, input, output, 4, bu) \
        wrapOne(X, input, output, 5, ga) \
        wrapOne(X, input, output, 6, ge) \
        wrapOne(X, input, output, 7, gi) \
        wrapOneInvert(X, input, output, 8, go) \
        wrapOne(X, input, output, 9, gu) \
        wrapOne(X, input, output, 10, ka) \
        wrapOne(X, input, output, 11, ke) \
        wrapOneInvert(X, input, output, 12, ki) \
        wrapOne(X, input, output, 13, ko) \
        wrapOne(X, input, output, 14, ku) \
        wrapOne(X, input, output, 15, ma) \
        if (laneCount < 24) { \
            if (laneCount < 20) { \
                if (laneCount < 18) { \
                    if (laneCount < 17) { \
                        X##me ^= trailingBits; \
                    } \
                    else { \
                        wrapOne(X, input, output, 16, me) \
                        X##mi ^= trailingBits; \
                    } \
                } \
                else { \
                    wrapOne(X, input, output, 16, me) \
                    wrapOneInvert(X, input, output, 17, mi) \
                    if (laneCount < 19) { \
                        X##mo ^= trailingBits; \
                    } \
                    else { \
                        wrapOne(X, input, output, 18, mo) \
                        X##mu ^= trailingBits; \
                    } \
                } \
            } \
            else { \
                wrapOne(X, input, output, 16, me) \
                wrapOneInvert(X, input, output, 17, mi) \
                wrapOne(X, input, output, 18, mo) \
                wrapOne(X, input, output, 19, mu) \
                if (laneCount < 22) { \
                    if (laneCount < 21) { \
                        X##sa ^= trailingBits; \
                    } \
                    else { \
                        wrapOneInvert(X, input, output, 20, sa) \
                        X##se ^= trailingBits; \
                    } \
                } \
                else { \
                    wrapOneInvert(X, input, output, 20, sa) \
                    wrapOne(X, input, output, 21, se) \
                    if (laneCount < 23) { \
                        X##si ^= trailingBits; \
                    } \
                    else { \
                        wrapOne(X, input, output, 22, si) \
                        X##so ^= trailingBits; \
                    } \
                } \
            } \
        } \
        else { \
            wrapOne(X, input, output, 16, me) \
            wrapOneInvert(X, input, output, 17, mi) \
            wrapOne(X, input, output, 18, mo) \
            wrapOne(X, input, output, 19, mu) \
            wrapOneInvert(X, input, output, 20, sa) \
            wrapOne(X, input, output, 21, se) \
            wrapOne(X, input, output, 22, si) \
            wrapOne(X, input, output, 23, so) \
            if (laneCount < 25) { \
                X##su ^= trailingBits; \
            } \
            else { \
                wrapOne(X, input, output, 24, su) \
            } \
        } \
    }

#define unwrap(X, input, output, laneCount, trailingBits) \
    if (laneCount < 16) { \
        if (laneCount < 8) { \
            if (laneCount < 4) { \
                if (laneCount < 2) { \
                    if (laneCount < 1) { \
                        X##ba ^= trailingBits; \
                    } \
                    else { \
                        unwrapOne(X, input, output, 0, ba) \
                        X##be ^= trailingBits; \
                    } \
                } \
                else { \
                    unwrapOne(X, input, output, 0, ba) \
                    unwrapOneInvert(X, input, output, 1, be) \
                    if (laneCount < 3) { \
                        X##bi ^= trailingBits; \
                    } \
                    else { \
                        unwrapOneInvert(X, input, output, 2, bi) \
                        X##bo ^= trailingBits; \
                    } \
                } \
            } \
            else { \
                unwrapOne(X, input, output, 0, ba) \
                unwrapOneInvert(X, input, output, 1, be) \
                unwrapOneInvert(X, input, output, 2, bi) \
                unwrapOne(X, input, output, 3, bo) \
                if (laneCount < 6) { \
                    if (laneCount < 5) { \
                        X##bu ^= trailingBits; \
                    } \
                    else { \
                        unwrapOne(X, input, output, 4, bu) \
                        X##ga ^= trailingBits; \
                    } \
                } \
                else { \
                    unwrapOne(X, input, output, 4, bu) \
                    unwrapOne(X, input, output, 5, ga) \
                    if (laneCount < 7) { \
                        X##ge ^= trailingBits; \
                    } \
                    else { \
                        unwrapOne(X, input, output, 6, ge) \
                        X##gi ^= trailingBits; \
                    } \
                } \
            } \
        } \
        else { \
            unwrapOne(X, input, output, 0, ba) \
            unwrapOneInvert(X, input, output, 1, be) \
            unwrapOneInvert(X, input, output, 2, bi) \
            unwrapOne(X, input, output, 3, bo) \
            unwrapOne(X, input, output, 4, bu) \
            unwrapOne(X, input, output, 5, ga) \
            unwrapOne(X, input, output, 6, ge) \
            unwrapOne(X, input, output, 7, gi) \
            if (laneCount < 12) { \
                if (laneCount < 10) { \
                    if (laneCount < 9) { \
                        X##go ^= trailingBits; \
                    } \
                    else { \
                        unwrapOneInvert(X, input, output, 8, go) \
                        X##gu ^= trailingBits; \
                    } \
                } \
                else { \
                    unwrapOneInvert(X, input, output, 8, go) \
                    unwrapOne(X, input, output, 9, gu) \
                    if (laneCount < 11) { \
                        X##ka ^= trailingBits; \
                    } \
                    else { \
                        unwrapOne(X, input, output, 10, ka) \
                        X##ke ^= trailingBits; \
                    } \
                } \
            } \
            else { \
                unwrapOneInvert(X, input, output, 8, go) \
                unwrapOne(X, input, output, 9, gu) \
                unwrapOne(X, input, output, 10, ka) \
                unwrapOne(X, input, output, 11, ke) \
                if (laneCount < 14) { \
                    if (laneCount < 13) { \
                        X##ki ^= trailingBits; \
                    } \
                    else { \
                        unwrapOneInvert(X, input, output, 12, ki) \
                        X##ko ^= trailingBits; \
                    } \
                } \
                else { \
                    unwrapOneInvert(X, input, output, 12, ki) \
                    unwrapOne(X, input, output, 13, ko) \
                    if (laneCount < 15) { \
                        X##ku ^= trailingBits; \
                    } \
                    else { \
                        unwrapOne(X, input, output, 14, ku) \
                        X##ma ^= trailingBits; \
                    } \
                } \
            } \
        } \
    } \
    else { \
        unwrapOne(X, input, output, 0, ba) \
        unwrapOneInvert(X, input, output, 1, be) \
        unwrapOneInvert(X, input, output, 2, bi) \
        unwrapOne(X, input, output, 3, bo) \
        unwrapOne(X, input, output, 4, bu) \
        unwrapOne(X, input, output, 5, ga) \
        unwrapOne(X, input, output, 6, ge) \
        unwrapOne(X, input, output, 7, gi) \
        unwrapOneInvert(X, input, output, 8, go) \
        unwrapOne(X, input, output, 9, gu) \
        unwrapOne(X, input, output, 10, ka) \
        unwrapOne(X, input, output, 11, ke) \
        unwrapOneInvert(X, input, output, 12, ki) \
        unwrapOne(X, input, output, 13, ko) \
        unwrapOne(X, input, output, 14, ku) \
        unwrapOne(X, input, output, 15, ma) \
        if (laneCount < 24) { \
            if (laneCount < 20) { \
                if (laneCount < 18) { \
                    if (laneCount < 17) { \
                        X##me ^= trailingBits; \
                    } \
                    else { \
                        unwrapOne(X, input, output, 16, me) \
                        X##mi ^= trailingBits; \
                    } \
                } \
                else { \
                    unwrapOne(X, input, output, 16, me) \
                    unwrapOneInvert(X, input, output, 17, mi) \
                    if (laneCount < 19) { \
                        X##mo ^= trailingBits; \
                    } \
                    else { \
                        unwrapOne(X, input, output, 18, mo) \
                        X##mu ^= trailingBits; \
                    } \
                } \
            } \
            else { \
                unwrapOne(X, input, output, 16, me) \
                unwrapOneInvert(X, input, output, 17, mi) \
                unwrapOne(X, input, output, 18, mo) \
                unwrapOne(X, input, output, 19, mu) \
                if (laneCount < 22) { \
                    if (laneCount < 21) { \
                        X##sa ^= trailingBits; \
                    } \
                    else { \
                        unwrapOneInvert(X, input, output, 20, sa) \
                        X##se ^= trailingBits; \
                    } \
                } \
                else { \
                    unwrapOneInvert(X, input, output, 20, sa) \
                    unwrapOne(X, input, output, 21, se) \
                    if (laneCount < 23) { \
                        X##si ^= trailingBits; \
                    } \
                    else { \
                        unwrapOne(X, input, output, 22, si) \
                        X##so ^= trailingBits; \
                    } \
                } \
            } \
        } \
        else { \
            unwrapOne(X, input, output, 16, me) \
            unwrapOneInvert(X, input, output, 17, mi) \
            unwrapOne(X, input, output, 18, mo) \
            unwrapOne(X, input, output, 19, mu) \
            unwrapOneInvert(X, input, output, 20, sa) \
            unwrapOne(X, input, output, 21, se) \
            unwrapOne(X, input, output, 22, si) \
            unwrapOne(X, input, output, 23, so) \
            if (laneCount < 25) { \
                X##su ^= trailingBits; \
            } \
            else { \
                unwrapOne(X, input, output, 24, su) \
            } \
        } \
    }
