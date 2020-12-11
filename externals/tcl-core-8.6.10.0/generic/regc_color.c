/*
 * colorings of characters
 * This file is #included by regcomp.c.
 *
 * Copyright (c) 1998, 1999 Henry Spencer. All rights reserved.
 *
 * Development of this software was funded, in part, by Cray Research Inc.,
 * UUNET Communications Services Inc., Sun Microsystems Inc., and Scriptics
 * Corporation, none of whom are responsible for the results. The author
 * thanks all of them.
 *
 * Redistribution and use in source and binary forms -- with or without
 * modification -- are permitted for any purpose, provided that
 * redistributions in source form retain this entire copyright notice and
 * indicate the origin and nature of any modifications.
 *
 * I'd appreciate being given credit for this package in the documentation of
 * software which uses it, but that is not a requirement.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * HENRY SPENCER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Note that there are some incestuous relationships between this code and NFA
 * arc maintenance, which perhaps ought to be cleaned up sometime.
 */

#define	CISERR()	VISERR(cm->v)
#define	CERR(e)		VERR(cm->v, (e))

/*
 - initcm - set up new colormap
 ^ static void initcm(struct vars *, struct colormap *);
 */
static void
initcm(
    struct vars *v,
    struct colormap *cm)
{
    int i;
    int j;
    union tree *t;
    union tree *nextt;
    struct colordesc *cd;

    cm->magic = CMMAGIC;
    cm->v = v;

    cm->ncds = NINLINECDS;
    cm->cd = cm->cdspace;
    cm->max = 0;
    cm->free = 0;

    cd = cm->cd;		/* cm->cd[WHITE] */
    cd->sub = NOSUB;
    cd->arcs = NULL;
    cd->flags = 0;
    cd->nchrs = CHR_MAX - CHR_MIN + 1;

    /*
     * Upper levels of tree.
     */

    for (t=&cm->tree[0], j=NBYTS-1 ; j>0 ; t=nextt, j--) {
	nextt = t + 1;
	for (i=BYTTAB-1 ; i>=0 ; i--) {
	    t->tptr[i] = nextt;
	}
    }

    /*
     * Bottom level is solid white.
     */

    t = &cm->tree[NBYTS-1];
    for (i=BYTTAB-1 ; i>=0 ; i--) {
	t->tcolor[i] = WHITE;
    }
    cd->block = t;
}

/*
 - freecm - free dynamically-allocated things in a colormap
 ^ static void freecm(struct colormap *);
 */
static void
freecm(
    struct colormap *cm)
{
    size_t i;
    union tree *cb;

    cm->magic = 0;
    if (NBYTS > 1) {
	cmtreefree(cm, cm->tree, 0);
    }
    for (i=1 ; i<=cm->max ; i++) {	/* skip WHITE */
	if (!UNUSEDCOLOR(&cm->cd[i])) {
	    cb = cm->cd[i].block;
	    if (cb != NULL) {
		FREE(cb);
	    }
	}
    }
    if (cm->cd != cm->cdspace) {
	FREE(cm->cd);
    }
}

/*
 - cmtreefree - free a non-terminal part of a colormap tree
 ^ static void cmtreefree(struct colormap *, union tree *, int);
 */
static void
cmtreefree(
    struct colormap *cm,
    union tree *tree,
    int level)			/* level number (top == 0) of this block */
{
    int i;
    union tree *t;
    union tree *fillt = &cm->tree[level+1];
    union tree *cb;

    assert(level < NBYTS-1);	/* this level has pointers */
    for (i=BYTTAB-1 ; i>=0 ; i--) {
	t = tree->tptr[i];
	assert(t != NULL);
	if (t != fillt) {
	    if (level < NBYTS-2) {	/* more pointer blocks below */
		cmtreefree(cm, t, level+1);
		FREE(t);
	    } else {		/* color block below */
		cb = cm->cd[t->tcolor[0]].block;
		if (t != cb) {	/* not a solid block */
		    FREE(t);
		}
	    }
	}
    }
}

/*
 - setcolor - set the color of a character in a colormap
 ^ static color setcolor(struct colormap *, pchr, pcolor);
 */
static color			/* previous color */
setcolor(
    struct colormap *cm,
    pchr c,
    pcolor co)
{
    uchr uc = c;
    int shift;
    int level;
    int b;
    int bottom;
    union tree *t;
    union tree *newt;
    union tree *fillt;
    union tree *lastt;
    union tree *cb;
    color prev;

    assert(cm->magic == CMMAGIC);
    if (CISERR() || co == COLORLESS) {
	return COLORLESS;
    }

    t = cm->tree;
    for (level=0, shift=BYTBITS*(NBYTS-1) ; shift>0; level++, shift-=BYTBITS){
	b = (uc >> shift) & BYTMASK;
	lastt = t;
	t = lastt->tptr[b];
	assert(t != NULL);
	fillt = &cm->tree[level+1];
	bottom = (shift <= BYTBITS) ? 1 : 0;
	cb = (bottom) ? cm->cd[t->tcolor[0]].block : fillt;
	if (t == fillt || t == cb) {	/* must allocate a new block */
	    newt = (union tree *) MALLOC((bottom) ?
		    sizeof(struct colors) : sizeof(struct ptrs));
	    if (newt == NULL) {
		CERR(REG_ESPACE);
		return COLORLESS;
	    }
	    if (bottom) {
		memcpy(newt->tcolor, t->tcolor, BYTTAB*sizeof(color));
	    } else {
		memcpy(newt->tptr, t->tptr, BYTTAB*sizeof(union tree *));
	    }
	    t = newt;
	    lastt->tptr[b] = t;
	}
    }

    b = uc & BYTMASK;
    prev = t->tcolor[b];
    t->tcolor[b] = (color) co;
    return prev;
}

/*
 - maxcolor - report largest color number in use
 ^ static color maxcolor(struct colormap *);
 */
static color
maxcolor(
    struct colormap *cm)
{
    if (CISERR()) {
	return COLORLESS;
    }

    return (color) cm->max;
}

/*
 - newcolor - find a new color (must be subject of setcolor at once)
 * Beware: may relocate the colordescs.
 ^ static color newcolor(struct colormap *);
 */
static color			/* COLORLESS for error */
newcolor(
    struct colormap *cm)
{
    struct colordesc *cd;
    size_t n;

    if (CISERR()) {
	return COLORLESS;
    }

    if (cm->free != 0) {
	assert(cm->free > 0);
	assert((size_t) cm->free < cm->ncds);
	cd = &cm->cd[cm->free];
	assert(UNUSEDCOLOR(cd));
	assert(cd->arcs == NULL);
	cm->free = cd->sub;
    } else if (cm->max < cm->ncds - 1) {
	cm->max++;
	cd = &cm->cd[cm->max];
    } else {
	struct colordesc *newCd;

	/*
	 * Oops, must allocate more.
	 */

	if (cm->max == MAX_COLOR) {
	    CERR(REG_ECOLORS);
	    return COLORLESS;		/* too many colors */
	}
	n = cm->ncds * 2;
	if (n > MAX_COLOR + 1) {
	    n = MAX_COLOR + 1;
	}
	if (cm->cd == cm->cdspace) {
	    newCd = (struct colordesc *) MALLOC(n * sizeof(struct colordesc));
	    if (newCd != NULL) {
		memcpy(newCd, cm->cdspace,
			cm->ncds * sizeof(struct colordesc));
	    }
	} else {
	    newCd = (struct colordesc *)
		    REALLOC(cm->cd, n * sizeof(struct colordesc));
	}
	if (newCd == NULL) {
	    CERR(REG_ESPACE);
	    return COLORLESS;
	}
	cm->cd = newCd;
	cm->ncds = n;
	assert(cm->max < cm->ncds - 1);
	cm->max++;
	cd = &cm->cd[cm->max];
    }

    cd->nchrs = 0;
    cd->sub = NOSUB;
    cd->arcs = NULL;
    cd->flags = 0;
    cd->block = NULL;

    return (color) (cd - cm->cd);
}

/*
 - freecolor - free a color (must have no arcs or subcolor)
 ^ static void freecolor(struct colormap *, pcolor);
 */
static void
freecolor(
    struct colormap *cm,
    pcolor co)
{
    struct colordesc *cd = &cm->cd[co];
    color pco, nco;		/* for freelist scan */

    assert(co >= 0);
    if (co == WHITE) {
	return;
    }

    assert(cd->arcs == NULL);
    assert(cd->sub == NOSUB);
    assert(cd->nchrs == 0);
    cd->flags = FREECOL;
    if (cd->block != NULL) {
	FREE(cd->block);
	cd->block = NULL;	/* just paranoia */
    }

    if ((size_t) co == cm->max) {
	while (cm->max > WHITE && UNUSEDCOLOR(&cm->cd[cm->max])) {
	    cm->max--;
	}
	assert(cm->free >= 0);
	while ((size_t) cm->free > cm->max) {
	    cm->free = cm->cd[cm->free].sub;
	}
	if (cm->free > 0) {
	    assert((size_t)cm->free < cm->max);
	    pco = cm->free;
	    nco = cm->cd[pco].sub;
	    while (nco > 0) {
		if ((size_t) nco > cm->max) {
		    /*
		     * Take this one out of freelist.
		     */

		    nco = cm->cd[nco].sub;
		    cm->cd[pco].sub = nco;
		} else {
		    assert((size_t)nco < cm->max);
		    pco = nco;
		    nco = cm->cd[pco].sub;
		}
	    }
	}
    } else {
	cd->sub = cm->free;
	cm->free = (color) (cd - cm->cd);
    }
}

/*
 - pseudocolor - allocate a false color, to be managed by other means
 ^ static color pseudocolor(struct colormap *);
 */
static color
pseudocolor(
    struct colormap *cm)
{
    color co;

    co = newcolor(cm);
    if (CISERR()) {
	return COLORLESS;
    }
    cm->cd[co].nchrs = 1;
    cm->cd[co].flags = PSEUDO;
    return co;
}

/*
 - subcolor - allocate a new subcolor (if necessary) to this chr
 ^ static color subcolor(struct colormap *, pchr c);
 */
static color
subcolor(
    struct colormap *cm,
    pchr c)
{
    color co;			/* current color of c */
    color sco;			/* new subcolor */

    co = GETCOLOR(cm, c);
    sco = newsub(cm, co);
    if (CISERR()) {
	return COLORLESS;
    }
    assert(sco != COLORLESS);

    if (co == sco) {		/* already in an open subcolor */
	return co;		/* rest is redundant */
    }
    cm->cd[co].nchrs--;
    cm->cd[sco].nchrs++;
    setcolor(cm, c, sco);
    return sco;
}

/*
 - newsub - allocate a new subcolor (if necessary) for a color
 ^ static color newsub(struct colormap *, pcolor);
 */
static color
newsub(
    struct colormap *cm,
    pcolor co)
{
    color sco;			/* new subcolor */

    sco = cm->cd[co].sub;
    if (sco == NOSUB) {		/* color has no open subcolor */
	if (cm->cd[co].nchrs == 1) {	/* optimization */
	    return co;
	}
	sco = newcolor(cm);	/* must create subcolor */
	if (sco == COLORLESS) {
	    assert(CISERR());
	    return COLORLESS;
	}
	cm->cd[co].sub = sco;
	cm->cd[sco].sub = sco;	/* open subcolor points to self */
    }
    assert(sco != NOSUB);

    return sco;
}

/*
 - subrange - allocate new subcolors to this range of chrs, fill in arcs
 ^ static void subrange(struct vars *, pchr, pchr, struct state *,
 ^ 	struct state *);
 */
static void
subrange(
    struct vars *v,
    pchr from,
    pchr to,
    struct state *lp,
    struct state *rp)
{
    uchr uf;
    int i;

    assert(from <= to);

    /*
     * First, align "from" on a tree-block boundary
     */

    uf = (uchr) from;
    i = (int) (((uf + BYTTAB - 1) & (uchr) ~BYTMASK) - uf);
    for (; from<=to && i>0; i--, from++) {
	newarc(v->nfa, PLAIN, subcolor(v->cm, from), lp, rp);
    }
    if (from > to) {		/* didn't reach a boundary */
	return;
    }

    /*
     * Deal with whole blocks.
     */

    for (; to-from>=BYTTAB ; from+=BYTTAB) {
	subblock(v, from, lp, rp);
    }

    /*
     * Clean up any remaining partial table.
     */

    for (; from<=to ; from++) {
	newarc(v->nfa, PLAIN, subcolor(v->cm, from), lp, rp);
    }
}

/*
 - subblock - allocate new subcolors for one tree block of chrs, fill in arcs
 ^ static void subblock(struct vars *, pchr, struct state *, struct state *);
 */
static void
subblock(
    struct vars *v,
    pchr start,			/* first of BYTTAB chrs */
    struct state *lp,
    struct state *rp)
{
    uchr uc = start;
    struct colormap *cm = v->cm;
    int shift;
    int level;
    int i;
    int b;
    union tree *t;
    union tree *cb;
    union tree *fillt;
    union tree *lastt;
    int previ;
    int ndone;
    color co;
    color sco;

    assert((uc % BYTTAB) == 0);

    /*
     * Find its color block, making new pointer blocks as needed.
     */

    t = cm->tree;
    fillt = NULL;
    for (level=0, shift=BYTBITS*(NBYTS-1); shift>0; level++, shift-=BYTBITS) {
	b = (uc >> shift) & BYTMASK;
	lastt = t;
	t = lastt->tptr[b];
	assert(t != NULL);
	fillt = &cm->tree[level+1];
	if (t == fillt && shift > BYTBITS) {	/* need new ptr block */
	    t = (union tree *) MALLOC(sizeof(struct ptrs));
	    if (t == NULL) {
		CERR(REG_ESPACE);
		return;
	    }
	    memcpy(t->tptr, fillt->tptr, BYTTAB*sizeof(union tree *));
	    lastt->tptr[b] = t;
	}
    }

    /*
     * Special cases: fill block or solid block.
     */
    co = t->tcolor[0];
    cb = cm->cd[co].block;
    if (t == fillt || t == cb) {
	/*
	 * Either way, we want a subcolor solid block.
	 */

	sco = newsub(cm, co);
	t = cm->cd[sco].block;
	if (t == NULL) {	/* must set it up */
	    t = (union tree *) MALLOC(sizeof(struct colors));
	    if (t == NULL) {
		CERR(REG_ESPACE);
		return;
	    }
	    for (i=0 ; i<BYTTAB ; i++) {
		t->tcolor[i] = sco;
	    }
	    cm->cd[sco].block = t;
	}

	/*
	 * Find loop must have run at least once.
	 */

	lastt->tptr[b] = t;
	newarc(v->nfa, PLAIN, sco, lp, rp);
	cm->cd[co].nchrs -= BYTTAB;
	cm->cd[sco].nchrs += BYTTAB;
	return;
    }

    /*
     * General case, a mixed block to be altered.
     */

    i = 0;
    while (i < BYTTAB) {
	co = t->tcolor[i];
	sco = newsub(cm, co);
	newarc(v->nfa, PLAIN, sco, lp, rp);
	previ = i;
	do {
	    t->tcolor[i++] = sco;
	} while (i < BYTTAB && t->tcolor[i] == co);
	ndone = i - previ;
	cm->cd[co].nchrs -= ndone;
	cm->cd[sco].nchrs += ndone;
    }
}

/*
 - okcolors - promote subcolors to full colors
 ^ static void okcolors(struct nfa *, struct colormap *);
 */
static void
okcolors(
    struct nfa *nfa,
    struct colormap *cm)
{
    struct colordesc *cd;
    struct colordesc *end = CDEND(cm);
    struct colordesc *scd;
    struct arc *a;
    color co;
    color sco;

    for (cd=cm->cd, co=0 ; cd<end ; cd++, co++) {
	sco = cd->sub;
	if (UNUSEDCOLOR(cd) || sco == NOSUB) {
	    /*
	     * Has no subcolor, no further action.
	     */
	} else if (sco == co) {
	    /*
	     * Is subcolor, let parent deal with it.
	     */
	} else if (cd->nchrs == 0) {
	    /*
	     * Parent empty, its arcs change color to subcolor.
	     */

	    cd->sub = NOSUB;
	    scd = &cm->cd[sco];
	    assert(scd->nchrs > 0);
	    assert(scd->sub == sco);
	    scd->sub = NOSUB;
	    while ((a = cd->arcs) != NULL) {
		assert(a->co == co);
		uncolorchain(cm, a);
		a->co = sco;
		colorchain(cm, a);
	    }
	    freecolor(cm, co);
	} else {
	    /*
	     * Parent's arcs must gain parallel subcolor arcs.
	     */

	    cd->sub = NOSUB;
	    scd = &cm->cd[sco];
	    assert(scd->nchrs > 0);
	    assert(scd->sub == sco);
	    scd->sub = NOSUB;
	    for (a=cd->arcs ; a!=NULL ; a=a->colorchain) {
		assert(a->co == co);
		newarc(nfa, a->type, sco, a->from, a->to);
	    }
	}
    }
}

/*
 - colorchain - add this arc to the color chain of its color
 ^ static void colorchain(struct colormap *, struct arc *);
 */
static void
colorchain(
    struct colormap *cm,
    struct arc *a)
{
    struct colordesc *cd = &cm->cd[a->co];

    if (cd->arcs != NULL) {
	cd->arcs->colorchainRev = a;
    }
    a->colorchain = cd->arcs;
    a->colorchainRev = NULL;
    cd->arcs = a;
}

/*
 - uncolorchain - delete this arc from the color chain of its color
 ^ static void uncolorchain(struct colormap *, struct arc *);
 */
static void
uncolorchain(
    struct colormap *cm,
    struct arc *a)
{
    struct colordesc *cd = &cm->cd[a->co];
    struct arc *aa = a->colorchainRev;

    if (aa == NULL) {
	assert(cd->arcs == a);
	cd->arcs = a->colorchain;
    } else {
	assert(aa->colorchain == a);
	aa->colorchain = a->colorchain;
    }
    if (a->colorchain != NULL) {
	a->colorchain->colorchainRev = aa;
    }
    a->colorchain = NULL;	/* paranoia */
    a->colorchainRev = NULL;
}

/*
 - rainbow - add arcs of all full colors (but one) between specified states
 ^ static void rainbow(struct nfa *, struct colormap *, int, pcolor,
 ^ 	struct state *, struct state *);
 */
static void
rainbow(
    struct nfa *nfa,
    struct colormap *cm,
    int type,
    pcolor but,			/* COLORLESS if no exceptions */
    struct state *from,
    struct state *to)
{
    struct colordesc *cd;
    struct colordesc *end = CDEND(cm);
    color co;

    for (cd=cm->cd, co=0 ; cd<end && !CISERR(); cd++, co++) {
	if (!UNUSEDCOLOR(cd) && (cd->sub != co) && (co != but)
		&& !(cd->flags&PSEUDO)) {
	    newarc(nfa, type, co, from, to);
	}
    }
}

/*
 - colorcomplement - add arcs of complementary colors
 * The calling sequence ought to be reconciled with cloneouts().
 ^ static void colorcomplement(struct nfa *, struct colormap *, int,
 ^ 	struct state *, struct state *, struct state *);
 */
static void
colorcomplement(
    struct nfa *nfa,
    struct colormap *cm,
    int type,
    struct state *of,		/* complements of this guy's PLAIN outarcs */
    struct state *from,
    struct state *to)
{
    struct colordesc *cd;
    struct colordesc *end = CDEND(cm);
    color co;

    assert(of != from);
    for (cd=cm->cd, co=0 ; cd<end && !CISERR() ; cd++, co++) {
	if (!UNUSEDCOLOR(cd) && !(cd->flags&PSEUDO)) {
	    if (findarc(of, PLAIN, co) == NULL) {
		newarc(nfa, type, co, from, to);
	    }
	}
    }
}

#ifdef REG_DEBUG
/*
 ^ #ifdef REG_DEBUG
 */

/*
 - dumpcolors - debugging output
 ^ static void dumpcolors(struct colormap *, FILE *);
 */
static void
dumpcolors(
    struct colormap *cm,
    FILE *f)
{
    struct colordesc *cd;
    struct colordesc *end;
    color co;
    chr c;
    char *has;

    fprintf(f, "max %ld\n", (long) cm->max);
    if (NBYTS > 1) {
	fillcheck(cm, cm->tree, 0, f);
    }
    end = CDEND(cm);
    for (cd=cm->cd+1, co=1 ; cd<end ; cd++, co++) {	/* skip 0 */
	if (!UNUSEDCOLOR(cd)) {
	    assert(cd->nchrs > 0);
	    has = (cd->block != NULL) ? "#" : "";
	    if (cd->flags&PSEUDO) {
		fprintf(f, "#%2ld%s(ps): ", (long) co, has);
	    } else {
		fprintf(f, "#%2ld%s(%2d): ", (long) co, has, cd->nchrs);
	    }

	    /*
	     * Unfortunately, it's hard to do this next bit more efficiently.
	     *
	     * Spencer's original coding has the loop iterating from CHR_MIN
	     * to CHR_MAX, but that's utterly unusable for 32-bit chr, or
	     * even 16-bit.  For debugging purposes it seems fine to print
	     * only chr codes up to 1000 or so.
	     */

	    for (c=CHR_MIN ; c<1000 ; c++) {
		if (GETCOLOR(cm, c) == co) {
		    dumpchr(c, f);
		}
	    }
	    fprintf(f, "\n");
	}
    }
}

/*
 - fillcheck - check proper filling of a tree
 ^ static void fillcheck(struct colormap *, union tree *, int, FILE *);
 */
static void
fillcheck(
    struct colormap *cm,
    union tree *tree,
    int level,			/* level number (top == 0) of this block */
    FILE *f)
{
    int i;
    union tree *t;
    union tree *fillt = &cm->tree[level+1];

    assert(level < NBYTS-1);	/* this level has pointers */
    for (i=BYTTAB-1 ; i>=0 ; i--) {
	t = tree->tptr[i];
	if (t == NULL) {
	    fprintf(f, "NULL found in filled tree!\n");
	} else if (t == fillt) {
	    /* empty body */
	} else if (level < NBYTS-2) {	/* more pointer blocks below */
	    fillcheck(cm, t, level+1, f);
	}
    }
}

/*
 - dumpchr - print a chr
 * Kind of char-centric but works well enough for debug use.
 ^ static void dumpchr(pchr, FILE *);
 */
static void
dumpchr(
    pchr c,
    FILE *f)
{
    if (c == '\\') {
	fprintf(f, "\\\\");
    } else if (c > ' ' && c <= '~') {
	putc((char) c, f);
    } else {
	fprintf(f, "\\u%04lx", (long) c);
    }
}

/*
 ^ #endif
 */
#endif				/* ifdef REG_DEBUG */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
