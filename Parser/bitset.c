/* Bitset primitives */

#include "PROTO.h"
#include "malloc.h"
#include "bitset.h"

bitset
newbitset(nbits)
	int nbits;
{
	int nbytes = NBYTES(nbits);
	bitset ss = NEW(BYTE, nbytes);
	
	if (ss == NULL)
		fatal("no mem for bitset");
	
	ss += nbytes;
	while (--nbytes >= 0)
		*--ss = 0;
	return ss;
}

void
delbitset(ss)
	bitset ss;
{
	DEL(ss);
}

int
addbit(ss, ibit)
	bitset ss;
	int ibit;
{
	int ibyte = BIT2BYTE(ibit);
	BYTE mask = BIT2MASK(ibit);
	
	if (ss[ibyte] & mask)
		return 0; /* Bit already set */
	ss[ibyte] |= mask;
	return 1;
}

#if 0 /* Now a macro */
int
testbit(ss, ibit)
	bitset ss;
	int ibit;
{
	return (ss[BIT2BYTE(ibit)] & BIT2MASK(ibit)) != 0;
}
#endif

int
samebitset(ss1, ss2, nbits)
	bitset ss1, ss2;
	int nbits;
{
	int i;
	
	for (i = NBYTES(nbits); --i >= 0; )
		if (*ss1++ != *ss2++)
			return 0;
	return 1;
}

void
mergebitset(ss1, ss2, nbits)
	bitset ss1, ss2;
	int nbits;
{
	int i;
	
	for (i = NBYTES(nbits); --i >= 0; )
		*ss1++ |= *ss2++;
}
