/***********************************************************
Copyright 1991 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Bitset primitives used by the parser generator */

#include "pgenheaders.h"
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
