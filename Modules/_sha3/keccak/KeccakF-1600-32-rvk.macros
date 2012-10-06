/*
The Keccak sponge function, designed by Guido Bertoni, Joan Daemen,
Michaël Peeters and Gilles Van Assche. For more information, feedback or
questions, please refer to our website: http://keccak.noekeon.org/

Implementation by Ronny Van Keer,
hereby denoted as "the implementer".

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

static const UINT32 KeccakF1600RoundConstants_int2[2*24] =
{
    0x00000001UL,    0x00000000UL,
    0x00000000UL,    0x00000089UL,
    0x00000000UL,    0x8000008bUL,
    0x00000000UL,    0x80008080UL,
    0x00000001UL,    0x0000008bUL,
    0x00000001UL,    0x00008000UL,
    0x00000001UL,    0x80008088UL,
    0x00000001UL,    0x80000082UL,
    0x00000000UL,    0x0000000bUL,
    0x00000000UL,    0x0000000aUL,
    0x00000001UL,    0x00008082UL,
    0x00000000UL,    0x00008003UL,
    0x00000001UL,    0x0000808bUL,
    0x00000001UL,    0x8000000bUL,
    0x00000001UL,    0x8000008aUL,
    0x00000001UL,    0x80000081UL,
    0x00000000UL,    0x80000081UL,
    0x00000000UL,    0x80000008UL,
    0x00000000UL,    0x00000083UL,
    0x00000000UL,    0x80008003UL,
    0x00000001UL,    0x80008088UL,
    0x00000000UL,    0x80000088UL,
    0x00000001UL,    0x00008000UL,
    0x00000000UL,    0x80008082UL
};

#undef rounds

#define rounds \
{ \
    UINT32 Da0, De0, Di0, Do0, Du0; \
    UINT32 Da1, De1, Di1, Do1, Du1; \
    UINT32 Ba, Be, Bi, Bo, Bu; \
    UINT32 Aba0, Abe0, Abi0, Abo0, Abu0; \
    UINT32 Aba1, Abe1, Abi1, Abo1, Abu1; \
    UINT32 Aga0, Age0, Agi0, Ago0, Agu0; \
    UINT32 Aga1, Age1, Agi1, Ago1, Agu1; \
    UINT32 Aka0, Ake0, Aki0, Ako0, Aku0; \
    UINT32 Aka1, Ake1, Aki1, Ako1, Aku1; \
    UINT32 Ama0, Ame0, Ami0, Amo0, Amu0; \
    UINT32 Ama1, Ame1, Ami1, Amo1, Amu1; \
    UINT32 Asa0, Ase0, Asi0, Aso0, Asu0; \
    UINT32 Asa1, Ase1, Asi1, Aso1, Asu1; \
    UINT32 Cw, Cx, Cy, Cz; \
    UINT32 Eba0, Ebe0, Ebi0, Ebo0, Ebu0; \
    UINT32 Eba1, Ebe1, Ebi1, Ebo1, Ebu1; \
    UINT32 Ega0, Ege0, Egi0, Ego0, Egu0; \
    UINT32 Ega1, Ege1, Egi1, Ego1, Egu1; \
    UINT32 Eka0, Eke0, Eki0, Eko0, Eku0; \
    UINT32 Eka1, Eke1, Eki1, Eko1, Eku1; \
    UINT32 Ema0, Eme0, Emi0, Emo0, Emu0; \
    UINT32 Ema1, Eme1, Emi1, Emo1, Emu1; \
    UINT32 Esa0, Ese0, Esi0, Eso0, Esu0; \
    UINT32 Esa1, Ese1, Esi1, Eso1, Esu1; \
	const UINT32 * pRoundConstants = KeccakF1600RoundConstants_int2; \
    UINT32 i; \
\
    copyFromState(A, state) \
\
    for( i = 12; i != 0; --i ) { \
	    Cx = Abu0^Agu0^Aku0^Amu0^Asu0; \
	    Du1 = Abe1^Age1^Ake1^Ame1^Ase1; \
	    Da0 = Cx^ROL32(Du1, 1); \
	    Cz = Abu1^Agu1^Aku1^Amu1^Asu1; \
	    Du0 = Abe0^Age0^Ake0^Ame0^Ase0; \
	    Da1 = Cz^Du0; \
\
	    Cw = Abi0^Agi0^Aki0^Ami0^Asi0; \
	    Do0 = Cw^ROL32(Cz, 1); \
	    Cy = Abi1^Agi1^Aki1^Ami1^Asi1; \
	    Do1 = Cy^Cx; \
\
	    Cx = Aba0^Aga0^Aka0^Ama0^Asa0; \
	    De0 = Cx^ROL32(Cy, 1); \
	    Cz = Aba1^Aga1^Aka1^Ama1^Asa1; \
	    De1 = Cz^Cw; \
\
	    Cy = Abo1^Ago1^Ako1^Amo1^Aso1; \
	    Di0 = Du0^ROL32(Cy, 1); \
	    Cw = Abo0^Ago0^Ako0^Amo0^Aso0; \
	    Di1 = Du1^Cw; \
\
	    Du0 = Cw^ROL32(Cz, 1); \
	    Du1 = Cy^Cx; \
\
	    Aba0 ^= Da0; \
	    Ba = Aba0; \
	    Age0 ^= De0; \
	    Be = ROL32(Age0, 22); \
	    Aki1 ^= Di1; \
	    Bi = ROL32(Aki1, 22); \
	    Amo1 ^= Do1; \
	    Bo = ROL32(Amo1, 11); \
	    Asu0 ^= Du0; \
	    Bu = ROL32(Asu0, 7); \
	    Eba0 =   Ba ^((~Be)&  Bi ) ^ *(pRoundConstants++); \
	    Ebe0 =   Be ^((~Bi)&  Bo ); \
	    Ebi0 =   Bi ^((~Bo)&  Bu ); \
	    Ebo0 =   Bo ^((~Bu)&  Ba ); \
	    Ebu0 =   Bu ^((~Ba)&  Be ); \
\
	    Abo0 ^= Do0; \
	    Ba = ROL32(Abo0, 14); \
	    Agu0 ^= Du0; \
	    Be = ROL32(Agu0, 10); \
	    Aka1 ^= Da1; \
	    Bi = ROL32(Aka1, 2); \
	    Ame1 ^= De1; \
	    Bo = ROL32(Ame1, 23); \
	    Asi1 ^= Di1; \
	    Bu = ROL32(Asi1, 31); \
	    Ega0 =   Ba ^((~Be)&  Bi ); \
	    Ege0 =   Be ^((~Bi)&  Bo ); \
	    Egi0 =   Bi ^((~Bo)&  Bu ); \
	    Ego0 =   Bo ^((~Bu)&  Ba ); \
	    Egu0 =   Bu ^((~Ba)&  Be ); \
\
	    Abe1 ^= De1; \
	    Ba = ROL32(Abe1, 1); \
	    Agi0 ^= Di0; \
	    Be = ROL32(Agi0, 3); \
	    Ako1 ^= Do1; \
	    Bi = ROL32(Ako1, 13); \
	    Amu0 ^= Du0; \
	    Bo = ROL32(Amu0, 4); \
	    Asa0 ^= Da0; \
	    Bu = ROL32(Asa0, 9); \
	    Eka0 =   Ba ^((~Be)&  Bi ); \
	    Eke0 =   Be ^((~Bi)&  Bo ); \
	    Eki0 =   Bi ^((~Bo)&  Bu ); \
	    Eko0 =   Bo ^((~Bu)&  Ba ); \
	    Eku0 =   Bu ^((~Ba)&  Be ); \
\
	    Abu1 ^= Du1; \
	    Ba = ROL32(Abu1, 14); \
	    Aga0 ^= Da0; \
	    Be = ROL32(Aga0, 18); \
	    Ake0 ^= De0; \
	    Bi = ROL32(Ake0, 5); \
	    Ami1 ^= Di1; \
	    Bo = ROL32(Ami1, 8); \
	    Aso0 ^= Do0; \
	    Bu = ROL32(Aso0, 28); \
	    Ema0 =   Ba ^((~Be)&  Bi ); \
	    Eme0 =   Be ^((~Bi)&  Bo ); \
	    Emi0 =   Bi ^((~Bo)&  Bu ); \
	    Emo0 =   Bo ^((~Bu)&  Ba ); \
	    Emu0 =   Bu ^((~Ba)&  Be ); \
\
	    Abi0 ^= Di0; \
	    Ba = ROL32(Abi0, 31); \
	    Ago1 ^= Do1; \
	    Be = ROL32(Ago1, 28); \
	    Aku1 ^= Du1; \
	    Bi = ROL32(Aku1, 20); \
	    Ama1 ^= Da1; \
	    Bo = ROL32(Ama1, 21); \
	    Ase0 ^= De0; \
	    Bu = ROL32(Ase0, 1); \
	    Esa0 =   Ba ^((~Be)&  Bi ); \
	    Ese0 =   Be ^((~Bi)&  Bo ); \
	    Esi0 =   Bi ^((~Bo)&  Bu ); \
	    Eso0 =   Bo ^((~Bu)&  Ba ); \
	    Esu0 =   Bu ^((~Ba)&  Be ); \
\
	    Aba1 ^= Da1; \
	    Ba = Aba1; \
	    Age1 ^= De1; \
	    Be = ROL32(Age1, 22); \
	    Aki0 ^= Di0; \
	    Bi = ROL32(Aki0, 21); \
	    Amo0 ^= Do0; \
	    Bo = ROL32(Amo0, 10); \
	    Asu1 ^= Du1; \
	    Bu = ROL32(Asu1, 7); \
	    Eba1 =   Ba ^((~Be)&  Bi ); \
	    Eba1 ^= *(pRoundConstants++); \
	    Ebe1 =   Be ^((~Bi)&  Bo ); \
	    Ebi1 =   Bi ^((~Bo)&  Bu ); \
	    Ebo1 =   Bo ^((~Bu)&  Ba ); \
	    Ebu1 =   Bu ^((~Ba)&  Be ); \
\
	    Abo1 ^= Do1; \
	    Ba = ROL32(Abo1, 14); \
	    Agu1 ^= Du1; \
	    Be = ROL32(Agu1, 10); \
	    Aka0 ^= Da0; \
	    Bi = ROL32(Aka0, 1); \
	    Ame0 ^= De0; \
	    Bo = ROL32(Ame0, 22); \
	    Asi0 ^= Di0; \
	    Bu = ROL32(Asi0, 30); \
	    Ega1 =   Ba ^((~Be)&  Bi ); \
	    Ege1 =   Be ^((~Bi)&  Bo ); \
	    Egi1 =   Bi ^((~Bo)&  Bu ); \
	    Ego1 =   Bo ^((~Bu)&  Ba ); \
	    Egu1 =   Bu ^((~Ba)&  Be ); \
\
	    Abe0 ^= De0; \
	    Ba = Abe0; \
	    Agi1 ^= Di1; \
	    Be = ROL32(Agi1, 3); \
	    Ako0 ^= Do0; \
	    Bi = ROL32(Ako0, 12); \
	    Amu1 ^= Du1; \
	    Bo = ROL32(Amu1, 4); \
	    Asa1 ^= Da1; \
	    Bu = ROL32(Asa1, 9); \
	    Eka1 =   Ba ^((~Be)&  Bi ); \
	    Eke1 =   Be ^((~Bi)&  Bo ); \
	    Eki1 =   Bi ^((~Bo)&  Bu ); \
	    Eko1 =   Bo ^((~Bu)&  Ba ); \
	    Eku1 =   Bu ^((~Ba)&  Be ); \
\
	    Abu0 ^= Du0; \
	    Ba = ROL32(Abu0, 13); \
	    Aga1 ^= Da1; \
	    Be = ROL32(Aga1, 18); \
	    Ake1 ^= De1; \
	    Bi = ROL32(Ake1, 5); \
	    Ami0 ^= Di0; \
	    Bo = ROL32(Ami0, 7); \
	    Aso1 ^= Do1; \
	    Bu = ROL32(Aso1, 28); \
	    Ema1 =   Ba ^((~Be)&  Bi ); \
	    Eme1 =   Be ^((~Bi)&  Bo ); \
	    Emi1 =   Bi ^((~Bo)&  Bu ); \
	    Emo1 =   Bo ^((~Bu)&  Ba ); \
	    Emu1 =   Bu ^((~Ba)&  Be ); \
\
	    Abi1 ^= Di1; \
	    Ba = ROL32(Abi1, 31); \
	    Ago0 ^= Do0; \
	    Be = ROL32(Ago0, 27); \
	    Aku0 ^= Du0; \
	    Bi = ROL32(Aku0, 19); \
	    Ama0 ^= Da0; \
	    Bo = ROL32(Ama0, 20); \
	    Ase1 ^= De1; \
	    Bu = ROL32(Ase1, 1); \
	    Esa1 =   Ba ^((~Be)&  Bi ); \
	    Ese1 =   Be ^((~Bi)&  Bo ); \
	    Esi1 =   Bi ^((~Bo)&  Bu ); \
	    Eso1 =   Bo ^((~Bu)&  Ba ); \
	    Esu1 =   Bu ^((~Ba)&  Be ); \
\
	    Cx = Ebu0^Egu0^Eku0^Emu0^Esu0; \
	    Du1 = Ebe1^Ege1^Eke1^Eme1^Ese1; \
	    Da0 = Cx^ROL32(Du1, 1); \
	    Cz = Ebu1^Egu1^Eku1^Emu1^Esu1; \
	    Du0 = Ebe0^Ege0^Eke0^Eme0^Ese0; \
	    Da1 = Cz^Du0; \
\
	    Cw = Ebi0^Egi0^Eki0^Emi0^Esi0; \
	    Do0 = Cw^ROL32(Cz, 1); \
	    Cy = Ebi1^Egi1^Eki1^Emi1^Esi1; \
	    Do1 = Cy^Cx; \
\
	    Cx = Eba0^Ega0^Eka0^Ema0^Esa0; \
	    De0 = Cx^ROL32(Cy, 1); \
	    Cz = Eba1^Ega1^Eka1^Ema1^Esa1; \
	    De1 = Cz^Cw; \
\
	    Cy = Ebo1^Ego1^Eko1^Emo1^Eso1; \
	    Di0 = Du0^ROL32(Cy, 1); \
	    Cw = Ebo0^Ego0^Eko0^Emo0^Eso0; \
	    Di1 = Du1^Cw; \
\
	    Du0 = Cw^ROL32(Cz, 1); \
	    Du1 = Cy^Cx; \
\
	    Eba0 ^= Da0; \
	    Ba = Eba0; \
	    Ege0 ^= De0; \
	    Be = ROL32(Ege0, 22); \
	    Eki1 ^= Di1; \
	    Bi = ROL32(Eki1, 22); \
	    Emo1 ^= Do1; \
	    Bo = ROL32(Emo1, 11); \
	    Esu0 ^= Du0; \
	    Bu = ROL32(Esu0, 7); \
	    Aba0 =   Ba ^((~Be)&  Bi ); \
	    Aba0 ^= *(pRoundConstants++); \
	    Abe0 =   Be ^((~Bi)&  Bo ); \
	    Abi0 =   Bi ^((~Bo)&  Bu ); \
	    Abo0 =   Bo ^((~Bu)&  Ba ); \
	    Abu0 =   Bu ^((~Ba)&  Be ); \
\
	    Ebo0 ^= Do0; \
	    Ba = ROL32(Ebo0, 14); \
	    Egu0 ^= Du0; \
	    Be = ROL32(Egu0, 10); \
	    Eka1 ^= Da1; \
	    Bi = ROL32(Eka1, 2); \
	    Eme1 ^= De1; \
	    Bo = ROL32(Eme1, 23); \
	    Esi1 ^= Di1; \
	    Bu = ROL32(Esi1, 31); \
	    Aga0 =   Ba ^((~Be)&  Bi ); \
	    Age0 =   Be ^((~Bi)&  Bo ); \
	    Agi0 =   Bi ^((~Bo)&  Bu ); \
	    Ago0 =   Bo ^((~Bu)&  Ba ); \
	    Agu0 =   Bu ^((~Ba)&  Be ); \
\
	    Ebe1 ^= De1; \
	    Ba = ROL32(Ebe1, 1); \
	    Egi0 ^= Di0; \
	    Be = ROL32(Egi0, 3); \
	    Eko1 ^= Do1; \
	    Bi = ROL32(Eko1, 13); \
	    Emu0 ^= Du0; \
	    Bo = ROL32(Emu0, 4); \
	    Esa0 ^= Da0; \
	    Bu = ROL32(Esa0, 9); \
	    Aka0 =   Ba ^((~Be)&  Bi ); \
	    Ake0 =   Be ^((~Bi)&  Bo ); \
	    Aki0 =   Bi ^((~Bo)&  Bu ); \
	    Ako0 =   Bo ^((~Bu)&  Ba ); \
	    Aku0 =   Bu ^((~Ba)&  Be ); \
\
	    Ebu1 ^= Du1; \
	    Ba = ROL32(Ebu1, 14); \
	    Ega0 ^= Da0; \
	    Be = ROL32(Ega0, 18); \
	    Eke0 ^= De0; \
	    Bi = ROL32(Eke0, 5); \
	    Emi1 ^= Di1; \
	    Bo = ROL32(Emi1, 8); \
	    Eso0 ^= Do0; \
	    Bu = ROL32(Eso0, 28); \
	    Ama0 =   Ba ^((~Be)&  Bi ); \
	    Ame0 =   Be ^((~Bi)&  Bo ); \
	    Ami0 =   Bi ^((~Bo)&  Bu ); \
	    Amo0 =   Bo ^((~Bu)&  Ba ); \
	    Amu0 =   Bu ^((~Ba)&  Be ); \
\
	    Ebi0 ^= Di0; \
	    Ba = ROL32(Ebi0, 31); \
	    Ego1 ^= Do1; \
	    Be = ROL32(Ego1, 28); \
	    Eku1 ^= Du1; \
	    Bi = ROL32(Eku1, 20); \
	    Ema1 ^= Da1; \
	    Bo = ROL32(Ema1, 21); \
	    Ese0 ^= De0; \
	    Bu = ROL32(Ese0, 1); \
	    Asa0 =   Ba ^((~Be)&  Bi ); \
	    Ase0 =   Be ^((~Bi)&  Bo ); \
	    Asi0 =   Bi ^((~Bo)&  Bu ); \
	    Aso0 =   Bo ^((~Bu)&  Ba ); \
	    Asu0 =   Bu ^((~Ba)&  Be ); \
\
	    Eba1 ^= Da1; \
	    Ba = Eba1; \
	    Ege1 ^= De1; \
	    Be = ROL32(Ege1, 22); \
	    Eki0 ^= Di0; \
	    Bi = ROL32(Eki0, 21); \
	    Emo0 ^= Do0; \
	    Bo = ROL32(Emo0, 10); \
	    Esu1 ^= Du1; \
	    Bu = ROL32(Esu1, 7); \
	    Aba1 =   Ba ^((~Be)&  Bi ); \
	    Aba1 ^= *(pRoundConstants++); \
	    Abe1 =   Be ^((~Bi)&  Bo ); \
	    Abi1 =   Bi ^((~Bo)&  Bu ); \
	    Abo1 =   Bo ^((~Bu)&  Ba ); \
	    Abu1 =   Bu ^((~Ba)&  Be ); \
\
	    Ebo1 ^= Do1; \
	    Ba = ROL32(Ebo1, 14); \
	    Egu1 ^= Du1; \
	    Be = ROL32(Egu1, 10); \
	    Eka0 ^= Da0; \
	    Bi = ROL32(Eka0, 1); \
	    Eme0 ^= De0; \
	    Bo = ROL32(Eme0, 22); \
	    Esi0 ^= Di0; \
	    Bu = ROL32(Esi0, 30); \
	    Aga1 =   Ba ^((~Be)&  Bi ); \
	    Age1 =   Be ^((~Bi)&  Bo ); \
	    Agi1 =   Bi ^((~Bo)&  Bu ); \
	    Ago1 =   Bo ^((~Bu)&  Ba ); \
	    Agu1 =   Bu ^((~Ba)&  Be ); \
\
	    Ebe0 ^= De0; \
	    Ba = Ebe0; \
	    Egi1 ^= Di1; \
	    Be = ROL32(Egi1, 3); \
	    Eko0 ^= Do0; \
	    Bi = ROL32(Eko0, 12); \
	    Emu1 ^= Du1; \
	    Bo = ROL32(Emu1, 4); \
	    Esa1 ^= Da1; \
	    Bu = ROL32(Esa1, 9); \
	    Aka1 =   Ba ^((~Be)&  Bi ); \
	    Ake1 =   Be ^((~Bi)&  Bo ); \
	    Aki1 =   Bi ^((~Bo)&  Bu ); \
	    Ako1 =   Bo ^((~Bu)&  Ba ); \
	    Aku1 =   Bu ^((~Ba)&  Be ); \
\
	    Ebu0 ^= Du0; \
	    Ba = ROL32(Ebu0, 13); \
	    Ega1 ^= Da1; \
	    Be = ROL32(Ega1, 18); \
	    Eke1 ^= De1; \
	    Bi = ROL32(Eke1, 5); \
	    Emi0 ^= Di0; \
	    Bo = ROL32(Emi0, 7); \
	    Eso1 ^= Do1; \
	    Bu = ROL32(Eso1, 28); \
	    Ama1 =   Ba ^((~Be)&  Bi ); \
	    Ame1 =   Be ^((~Bi)&  Bo ); \
	    Ami1 =   Bi ^((~Bo)&  Bu ); \
	    Amo1 =   Bo ^((~Bu)&  Ba ); \
	    Amu1 =   Bu ^((~Ba)&  Be ); \
\
	    Ebi1 ^= Di1; \
	    Ba = ROL32(Ebi1, 31); \
	    Ego0 ^= Do0; \
	    Be = ROL32(Ego0, 27); \
	    Eku0 ^= Du0; \
	    Bi = ROL32(Eku0, 19); \
	    Ema0 ^= Da0; \
	    Bo = ROL32(Ema0, 20); \
	    Ese1 ^= De1; \
	    Bu = ROL32(Ese1, 1); \
	    Asa1 =   Ba ^((~Be)&  Bi ); \
	    Ase1 =   Be ^((~Bi)&  Bo ); \
	    Asi1 =   Bi ^((~Bo)&  Bu ); \
	    Aso1 =   Bo ^((~Bu)&  Ba ); \
	    Asu1 =   Bu ^((~Ba)&  Be ); \
    } \
    copyToState(state, A) \
}

#define copyFromState(X, state) \
    X##ba0 = state[ 0]; \
    X##ba1 = state[ 1]; \
    X##be0 = state[ 2]; \
    X##be1 = state[ 3]; \
    X##bi0 = state[ 4]; \
    X##bi1 = state[ 5]; \
    X##bo0 = state[ 6]; \
    X##bo1 = state[ 7]; \
    X##bu0 = state[ 8]; \
    X##bu1 = state[ 9]; \
    X##ga0 = state[10]; \
    X##ga1 = state[11]; \
    X##ge0 = state[12]; \
    X##ge1 = state[13]; \
    X##gi0 = state[14]; \
    X##gi1 = state[15]; \
    X##go0 = state[16]; \
    X##go1 = state[17]; \
    X##gu0 = state[18]; \
    X##gu1 = state[19]; \
    X##ka0 = state[20]; \
    X##ka1 = state[21]; \
    X##ke0 = state[22]; \
    X##ke1 = state[23]; \
    X##ki0 = state[24]; \
    X##ki1 = state[25]; \
    X##ko0 = state[26]; \
    X##ko1 = state[27]; \
    X##ku0 = state[28]; \
    X##ku1 = state[29]; \
    X##ma0 = state[30]; \
    X##ma1 = state[31]; \
    X##me0 = state[32]; \
    X##me1 = state[33]; \
    X##mi0 = state[34]; \
    X##mi1 = state[35]; \
    X##mo0 = state[36]; \
    X##mo1 = state[37]; \
    X##mu0 = state[38]; \
    X##mu1 = state[39]; \
    X##sa0 = state[40]; \
    X##sa1 = state[41]; \
    X##se0 = state[42]; \
    X##se1 = state[43]; \
    X##si0 = state[44]; \
    X##si1 = state[45]; \
    X##so0 = state[46]; \
    X##so1 = state[47]; \
    X##su0 = state[48]; \
    X##su1 = state[49]; \

#define copyToState(state, X) \
    state[ 0] = X##ba0; \
    state[ 1] = X##ba1; \
    state[ 2] = X##be0; \
    state[ 3] = X##be1; \
    state[ 4] = X##bi0; \
    state[ 5] = X##bi1; \
    state[ 6] = X##bo0; \
    state[ 7] = X##bo1; \
    state[ 8] = X##bu0; \
    state[ 9] = X##bu1; \
    state[10] = X##ga0; \
    state[11] = X##ga1; \
    state[12] = X##ge0; \
    state[13] = X##ge1; \
    state[14] = X##gi0; \
    state[15] = X##gi1; \
    state[16] = X##go0; \
    state[17] = X##go1; \
    state[18] = X##gu0; \
    state[19] = X##gu1; \
    state[20] = X##ka0; \
    state[21] = X##ka1; \
    state[22] = X##ke0; \
    state[23] = X##ke1; \
    state[24] = X##ki0; \
    state[25] = X##ki1; \
    state[26] = X##ko0; \
    state[27] = X##ko1; \
    state[28] = X##ku0; \
    state[29] = X##ku1; \
    state[30] = X##ma0; \
    state[31] = X##ma1; \
    state[32] = X##me0; \
    state[33] = X##me1; \
    state[34] = X##mi0; \
    state[35] = X##mi1; \
    state[36] = X##mo0; \
    state[37] = X##mo1; \
    state[38] = X##mu0; \
    state[39] = X##mu1; \
    state[40] = X##sa0; \
    state[41] = X##sa1; \
    state[42] = X##se0; \
    state[43] = X##se1; \
    state[44] = X##si0; \
    state[45] = X##si1; \
    state[46] = X##so0; \
    state[47] = X##so1; \
    state[48] = X##su0; \
    state[49] = X##su1; \

