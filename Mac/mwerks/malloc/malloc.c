/*
 * Attempt at a memory allocator for the Mac, Jack Jansen, CWI, 1995.
 *
 * Code adapted from BSD malloc, which is:
 *
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
/*static char *sccsid = "from: @(#)malloc.c	5.11 (Berkeley) 2/23/91";*/
static char *rcsid = "$Id$";
#endif /* LIBC_SCCS and not lint */

/*
 * malloc.c (Caltech) 2/21/82
 * Chris Kingsley, kingsley@cit-20. Modified by Jack Jansen, CWI.
 *
 * This is a very fast storage allocator.  It allocates blocks of a small 
 * number of different sizes, and keeps free lists of each size.  Blocks that
 * don't exactly fit are passed up to the next larger size.
 *
 * Blocks over a certain size are directly allocated by calling NewPtr.
 * 
 */
 

#define DEBUG
#define DEBUG2
#define MSTATS
#define RCHECK

typedef unsigned char u_char;
typedef unsigned long u_long;
typedef unsigned int u_int;
typedef unsigned short u_short;
typedef u_long caddr_t;

#include <Memory.h>
#include <stdlib.h>
#include <string.h>

#define	NULL 0

static void morecore();

/*
 * The overhead on a block is at least 4 bytes.  When free, this space
 * contains a pointer to the next free block, and the bottom two bits must
 * be zero.  When in use, the first byte is set to MAGIC, and the second
 * byte is the size index.  The remaining bytes are for alignment.
 * If range checking is enabled then a second word holds the size of the
 * requested block, less 1, rounded up to a multiple of sizeof(RMAGIC).
 * The order of elements is critical: ov_magic must overlay the low order
 * bits of ov_next, and ov_magic can not be a valid ov_next bit pattern.
 */
union	overhead {
	union	overhead *ov_next;	/* when free */
	struct {
		u_char	ovu_magic0;	/* magic number */
		u_char	ovu_index;	/* bucket # */
		u_char	ovu_unused;	/* unused */
		u_char	ovu_magic1;	/* other magic number */
#ifdef RCHECK
		u_short	ovu_rmagic;	/* range magic number */
		u_long	ovu_size;	/* actual block size */
#endif
	} ovu;
#define	ov_magic0	ovu.ovu_magic0
#define	ov_magic1	ovu.ovu_magic1
#define	ov_index	ovu.ovu_index
#define	ov_rmagic	ovu.ovu_rmagic
#define	ov_size		ovu.ovu_size
};

#define	MAGIC		0xef		/* magic # on accounting info */
#define RMAGIC		0x5555		/* magic # on range info */

#ifdef RCHECK
#define	RSLOP		sizeof (u_short)
#else
#define	RSLOP		0
#endif

#define OVERHEAD (sizeof(union overhead) + RSLOP)

/*
 * nextf[i] is the pointer to the next free block of size 2^(i+3).  The
 * smallest allocatable block is 8 bytes.  The overhead information
 * precedes the data area returned to the user.
 */
#define	NBUCKETS 11
#define MAXMALLOC (1<<(NBUCKETS+2))
static	union overhead *nextf[NBUCKETS];

#ifdef MSTATS
/*
 * nmalloc[i] is the difference between the number of mallocs and frees
 * for a given block size.
 */
static	u_int nmalloc[NBUCKETS+1];
#include <stdio.h>
#endif

#if defined(DEBUG) || defined(RCHECK) || defined(DEBUG2)
#define	ASSERT(p)   if (!(p)) botch(# p)
#include <stdio.h>
static
botch(s)
	char *s;
{
	fprintf(stderr, "\r\nmalloc assertion botched: %s\r\n", s);
 	(void) fflush(stderr);		/* just in case user buffered it */
	abort();
}
#else
#define	ASSERT(p)
#endif

void *
malloc(nbytes)
	size_t nbytes;
{
  	register union overhead *op;
  	register long bucket, n;
	register unsigned amt;

	/*
	** First the simple case: we simple allocate big blocks directly
	*/
	if ( nbytes + OVERHEAD >= MAXMALLOC ) {
		op = (union overhead *)NewPtr(nbytes+OVERHEAD);
		if ( op == NULL )
			return NULL;
		op->ov_magic0 = op->ov_magic1 = MAGIC;
		op->ov_index = 0xff;
#ifdef MSTATS
 	 	nmalloc[NBUCKETS]++;
#endif
#ifdef RCHECK
		/*
		 * Record allocated size of block and
		 * bound space with magic numbers.
		 */
		op->ov_size = (nbytes + RSLOP - 1) & ~(RSLOP - 1);
		op->ov_rmagic = RMAGIC;
  		*(u_short *)((caddr_t)(op + 1) + op->ov_size) = RMAGIC;
#endif
		return (void *)(op+1);
	}
	/*
	 * Convert amount of memory requested into closest block size
	 * stored in hash buckets which satisfies request.
	 * Account for space used per block for accounting.
	 */
#ifndef RCHECK
	amt = 8;	/* size of first bucket */
	bucket = 0;
#else
	amt = 16;	/* size of first bucket */
	bucket = 1;
#endif
	while (nbytes + OVERHEAD > amt) {
		amt <<= 1;
		if (amt == 0)
			return (NULL);
		bucket++;
	}
#ifdef DEBUG2
	ASSERT( bucket < NBUCKETS );
#endif
	/*
	 * If nothing in hash bucket right now,
	 * request more memory from the system.
	 */
  	if ((op = nextf[bucket]) == NULL) {
  		morecore(bucket);
  		if ((op = nextf[bucket]) == NULL)
  			return (NULL);
	}
	/* remove from linked list */
  	nextf[bucket] = op->ov_next;
	op->ov_magic0 = op->ov_magic1 = MAGIC;
	op->ov_index = bucket;
#ifdef MSTATS
  	nmalloc[bucket]++;
#endif
#ifdef RCHECK
	/*
	 * Record allocated size of block and
	 * bound space with magic numbers.
	 */
	op->ov_size = (nbytes + RSLOP - 1) & ~(RSLOP - 1);
	op->ov_rmagic = RMAGIC;
  	*(u_short *)((caddr_t)(op + 1) + op->ov_size) = RMAGIC;
#endif
  	return ((char *)(op + 1));
}

/*
 * Allocate more memory to the indicated bucket.
 */
static void
morecore(bucket)
	int bucket;
{
  	register union overhead *op;
	register long sz;		/* size of desired block */
  	long amt;			/* amount to allocate */
  	int nblks;			/* how many blocks we get */

	/*
	 * sbrk_size <= 0 only for big, FLUFFY, requests (about
	 * 2^30 bytes on a VAX, I think) or for a negative arg.
	 */
	sz = 1 << (bucket + 3);
#ifdef DEBUG2
	ASSERT(sz > 0);
#endif
	amt = MAXMALLOC;
	nblks = amt / sz;
#ifdef DEBUG2
	ASSERT(nblks*sz == amt);
#endif
	op = (union overhead *)NewPtr(amt);
	/* no more room! */
  	if (op == NULL)
  		return;
	/*
	 * Add new memory allocated to that on
	 * free list for this hash bucket.
	 */
  	nextf[bucket] = op;
  	while (--nblks > 0) {
		op->ov_next = (union overhead *)((caddr_t)op + sz);
		op = (union overhead *)((caddr_t)op + sz);
  	}
  	op->ov_next = (union overhead *)NULL;
}

void
free(cp)
	void *cp;
{   
  	register long size;
	register union overhead *op;

  	if (cp == NULL)
  		return;
	op = (union overhead *)((caddr_t)cp - sizeof (union overhead));
#ifdef DEBUG
  	ASSERT(op->ov_magic0 == MAGIC);		/* make sure it was in use */
  	ASSERT(op->ov_magic1 == MAGIC);
#else
	if (op->ov_magic0 != MAGIC || op->ov_magic1 != MAGIC)
		return;				/* sanity */
#endif
#ifdef RCHECK
  	ASSERT(op->ov_rmagic == RMAGIC);
	ASSERT(*(u_short *)((caddr_t)(op + 1) + op->ov_size) == RMAGIC);
#endif
  	size = op->ov_index;
  	if ( size == 0xff ) {
#ifdef MSTATS
		nmalloc[NBUCKETS]--;
#endif
  		DisposPtr((Ptr)op);
  		return;
  	}
  	ASSERT(size < NBUCKETS);
	op->ov_next = nextf[size];	/* also clobbers ov_magic */
  	nextf[size] = op;
#ifdef MSTATS
  	nmalloc[size]--;
#endif
}

void *
realloc(cp, nbytes)
	void *cp; 
	size_t nbytes;
{   
	int i;
	union overhead *op;
	int expensive;
	u_long maxsize;

  	if (cp == NULL)
  		return (malloc(nbytes));
	op = (union overhead *)((caddr_t)cp - sizeof (union overhead));
#ifdef DEBUG
  	ASSERT(op->ov_magic0 == MAGIC);		/* make sure it was in use */
  	ASSERT(op->ov_magic1 == MAGIC);
#else
	if (op->ov_magic0 != MAGIC || op->ov_magic1 != MAGIC)
		return NULL;				/* sanity */
#endif
#ifdef RCHECK
  	ASSERT(op->ov_rmagic == RMAGIC);
	ASSERT(*(u_short *)((caddr_t)(op + 1) + op->ov_size) == RMAGIC);
#endif
	i = op->ov_index;
	/*
	** First the malloc/copy cases
	*/
	expensive = 0;
	if ( i == 0xff ) {
		/* Big block. See if it has to stay big */
		if (nbytes+OVERHEAD > MAXMALLOC) {
			/* Yup, try to resize it first */
			SetPtrSize((Ptr)op, nbytes+OVERHEAD);
			if ( MemError() == 0 ) {
#ifdef RCHECK
				op->ov_size = (nbytes + RSLOP - 1) & ~(RSLOP - 1);
				*(u_short *)((caddr_t)(op + 1) + op->ov_size) = RMAGIC;
#endif
				return cp;
			}
			/* Nope, failed. Take the long way */
		}
		maxsize = GetPtrSize((Ptr)op);
		expensive = 1;
	} else {
		maxsize = 1 << (i+3);
		if ( nbytes + OVERHEAD > maxsize )
			expensive = 1;
		else if ( i > 0 && nbytes + OVERHEAD < (maxsize/2) )
			expensive = 1;
	}
	if ( expensive ) {
		void *newp;
		
		newp = malloc(nbytes);
		if ( newp == NULL ) {
			return NULL;
		}
		maxsize -= OVERHEAD;
		if ( maxsize < nbytes )
			nbytes = maxsize;
		memcpy(newp, cp, nbytes);
		free(cp);
		return newp;
	}
	/*
	** Ok, we don't have to copy, it fits as-is
	*/
#ifdef RCHECK
	op->ov_size = (nbytes + RSLOP - 1) & ~(RSLOP - 1);
	*(u_short *)((caddr_t)(op + 1) + op->ov_size) = RMAGIC;
#endif
	return(cp);
}


#ifdef MSTATS
/*
 * mstats - print out statistics about malloc
 * 
 * Prints two lines of numbers, one showing the length of the free list
 * for each size category, the second showing the number of mallocs -
 * frees for each size category.
 */
mstats(s)
	char *s;
{
  	register int i, j;
  	register union overhead *p;
  	int totfree = 0,
  	totused = 0;

  	fprintf(stderr, "Memory allocation statistics %s\nfree:\t", s);
  	for (i = 0; i < NBUCKETS; i++) {
  		for (j = 0, p = nextf[i]; p; p = p->ov_next, j++)
  			;
  		fprintf(stderr, " %d", j);
  		totfree += j * (1 << (i + 3));
  	}
  	fprintf(stderr, "\nused:\t");
  	for (i = 0; i < NBUCKETS; i++) {
  		fprintf(stderr, " %d", nmalloc[i]);
  		totused += nmalloc[i] * (1 << (i + 3));
  	}
  	fprintf(stderr, "\n\tTotal small in use: %d, total free: %d\n",
	    totused, totfree);
	fprintf(stderr, "\n\tNumber of big (>%d) blocks in use: %d\n", MAXMALLOC, nmalloc[NBUCKETS]);
}
#endif
