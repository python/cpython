/* Bitset interface */

#define BYTE		char

typedef BYTE *bitset;

bitset newbitset PROTO((int nbits));
void delbitset PROTO((bitset bs));
/* int testbit PROTO((bitset bs, int ibit)); /* Now a macro, see below */
int addbit PROTO((bitset bs, int ibit)); /* Returns 0 if already set */
int samebitset PROTO((bitset bs1, bitset bs2, int nbits));
void mergebitset PROTO((bitset bs1, bitset bs2, int nbits));

#define BITSPERBYTE	(8*sizeof(BYTE))
#define NBYTES(nbits)	(((nbits) + BITSPERBYTE - 1) / BITSPERBYTE)

#define BIT2BYTE(ibit)	((ibit) / BITSPERBYTE)
#define BIT2SHIFT(ibit)	((ibit) % BITSPERBYTE)
#define BIT2MASK(ibit)	(1 << BIT2SHIFT(ibit))
#define BYTE2BIT(ibyte)	((ibyte) * BITSPERBYTE)

#define testbit(ss, ibit) (((ss)[BIT2BYTE(ibit)] & BIT2MASK(ibit)) != 0)
