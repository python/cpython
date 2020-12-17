/*
 * NFA utilities.
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
 * One or two things that technically ought to be in here are actually in
 * color.c, thanks to some incestuous relationships in the color chains.
 */

#define	NISERR()	VISERR(nfa->v)
#define	NERR(e)		VERR(nfa->v, (e))
#define STACK_TOO_DEEP(x) (0)
#define CANCEL_REQUESTED(x) (0)
#define REG_CANCEL 777

/*
 - newnfa - set up an NFA
 ^ static struct nfa *newnfa(struct vars *, struct colormap *, struct nfa *);
 */
static struct nfa *		/* the NFA, or NULL */
newnfa(
    struct vars *v,
    struct colormap *cm,
    struct nfa *parent)		/* NULL if primary NFA */
{
    struct nfa *nfa;

    nfa = (struct nfa *) MALLOC(sizeof(struct nfa));
    if (nfa == NULL) {
	ERR(REG_ESPACE);
	return NULL;
    }

    nfa->states = NULL;
    nfa->slast = NULL;
    nfa->free = NULL;
    nfa->nstates = 0;
    nfa->cm = cm;
    nfa->v = v;
    nfa->bos[0] = nfa->bos[1] = COLORLESS;
    nfa->eos[0] = nfa->eos[1] = COLORLESS;
    nfa->parent = parent;	/* Precedes newfstate so parent is valid. */
    nfa->post = newfstate(nfa, '@');	/* number 0 */
    nfa->pre = newfstate(nfa, '>');	/* number 1 */

    nfa->init = newstate(nfa);	/* May become invalid later. */
    nfa->final = newstate(nfa);
    if (ISERR()) {
	freenfa(nfa);
	return NULL;
    }
    rainbow(nfa, nfa->cm, PLAIN, COLORLESS, nfa->pre, nfa->init);
    newarc(nfa, '^', 1, nfa->pre, nfa->init);
    newarc(nfa, '^', 0, nfa->pre, nfa->init);
    rainbow(nfa, nfa->cm, PLAIN, COLORLESS, nfa->final, nfa->post);
    newarc(nfa, '$', 1, nfa->final, nfa->post);
    newarc(nfa, '$', 0, nfa->final, nfa->post);

    if (ISERR()) {
	freenfa(nfa);
	return NULL;
    }
    return nfa;
}

/*
 - freenfa - free an entire NFA
 ^ static void freenfa(struct nfa *);
 */
static void
freenfa(
    struct nfa *nfa)
{
    struct state *s;

    while ((s = nfa->states) != NULL) {
	s->nins = s->nouts = 0;	/* don't worry about arcs */
	freestate(nfa, s);
    }
    while ((s = nfa->free) != NULL) {
	nfa->free = s->next;
	destroystate(nfa, s);
    }

    nfa->slast = NULL;
    nfa->nstates = -1;
    nfa->pre = NULL;
    nfa->post = NULL;
    FREE(nfa);
}

/*
 - newstate - allocate an NFA state, with zero flag value
 ^ static struct state *newstate(struct nfa *);
 */
static struct state *		/* NULL on error */
newstate(
    struct nfa *nfa)
{
    struct state *s;

    if (nfa->free != NULL) {
	s = nfa->free;
	nfa->free = s->next;
    } else {
	if (nfa->v->spaceused >= REG_MAX_COMPILE_SPACE) {
	    NERR(REG_ETOOBIG);
	    return NULL;
	}
	s = (struct state *) MALLOC(sizeof(struct state));
	if (s == NULL) {
	    NERR(REG_ESPACE);
	    return NULL;
	}
	nfa->v->spaceused += sizeof(struct state);
	s->oas.next = NULL;
	s->free = NULL;
	s->noas = 0;
    }

    assert(nfa->nstates >= 0);
    s->no = nfa->nstates++;
    s->flag = 0;
    if (nfa->states == NULL) {
	nfa->states = s;
    }
    s->nins = 0;
    s->ins = NULL;
    s->nouts = 0;
    s->outs = NULL;
    s->tmp = NULL;
    s->next = NULL;
    if (nfa->slast != NULL) {
	assert(nfa->slast->next == NULL);
	nfa->slast->next = s;
    }
    s->prev = nfa->slast;
    nfa->slast = s;
    return s;
}

/*
 - newfstate - allocate an NFA state with a specified flag value
 ^ static struct state *newfstate(struct nfa *, int flag);
 */
static struct state *		/* NULL on error */
newfstate(
    struct nfa *nfa,
    int flag)
{
    struct state *s;

    s = newstate(nfa);
    if (s != NULL) {
	s->flag = (char) flag;
    }
    return s;
}

/*
 - dropstate - delete a state's inarcs and outarcs and free it
 ^ static void dropstate(struct nfa *, struct state *);
 */
static void
dropstate(
    struct nfa *nfa,
    struct state *s)
{
    struct arc *a;

    while ((a = s->ins) != NULL) {
	freearc(nfa, a);
    }
    while ((a = s->outs) != NULL) {
	freearc(nfa, a);
    }
    freestate(nfa, s);
}

/*
 - freestate - free a state, which has no in-arcs or out-arcs
 ^ static void freestate(struct nfa *, struct state *);
 */
static void
freestate(
    struct nfa *nfa,
    struct state *s)
{
    assert(s != NULL);
    assert(s->nins == 0 && s->nouts == 0);

    s->no = FREESTATE;
    s->flag = 0;
    if (s->next != NULL) {
	s->next->prev = s->prev;
    } else {
	assert(s == nfa->slast);
	nfa->slast = s->prev;
    }
    if (s->prev != NULL) {
	s->prev->next = s->next;
    } else {
	assert(s == nfa->states);
	nfa->states = s->next;
    }
    s->prev = NULL;
    s->next = nfa->free;	/* don't delete it, put it on the free list */
    nfa->free = s;
}

/*
 - destroystate - really get rid of an already-freed state
 ^ static void destroystate(struct nfa *, struct state *);
 */
static void
destroystate(
    struct nfa *nfa,
    struct state *s)
{
    struct arcbatch *ab;
    struct arcbatch *abnext;

    assert(s->no == FREESTATE);
    for (ab=s->oas.next ; ab!=NULL ; ab=abnext) {
	abnext = ab->next;
	FREE(ab);
	nfa->v->spaceused -= sizeof(struct arcbatch);
    }
    s->ins = NULL;
    s->outs = NULL;
    s->next = NULL;
    FREE(s);
    nfa->v->spaceused -= sizeof(struct state);
}

/*
 - newarc - set up a new arc within an NFA
 ^ static void newarc(struct nfa *, int, pcolor, struct state *,
 ^	struct state *);
 */
/*
 * This function checks to make sure that no duplicate arcs are created.
 * In general we never want duplicates.
 */
static void
newarc(
    struct nfa *nfa,
    int t,
    pcolor co,
    struct state *from,
    struct state *to)
{
    struct arc *a;

    assert(from != NULL && to != NULL);

    /* check for duplicate arc, using whichever chain is shorter */
    if (from->nouts <= to->nins) {
	for (a = from->outs; a != NULL; a = a->outchain) {
	    if (a->to == to && a->co == co && a->type == t) {
		return;
	    }
	}
    } else {
	for (a = to->ins; a != NULL; a = a->inchain) {
	    if (a->from == from && a->co == co && a->type == t) {
		return;
	    }
	}
    }

    /* no dup, so create the arc */
    createarc(nfa, t, co, from, to);
}

/*
 * createarc - create a new arc within an NFA
 *
 * This function must *only* be used after verifying that there is no existing
 * identical arc (same type/color/from/to).
 */
static void
createarc(
    struct nfa * nfa,
    int t,
    pcolor co,
    struct state * from,
    struct state * to)
{
    struct arc *a;

    /* the arc is physically allocated within its from-state */
    a = allocarc(nfa, from);
    if (NISERR()) {
	return;
    }
    assert(a != NULL);

    a->type = t;
    a->co = (color) co;
    a->to = to;
    a->from = from;

    /*
     * Put the new arc on the beginning, not the end, of the chains; it's
     * simpler here, and freearc() is the same cost either way.  See also the
     * logic in moveins() and its cohorts, as well as fixempties().
     */
    a->inchain = to->ins;
    a->inchainRev = NULL;
    if (to->ins) {
	to->ins->inchainRev = a;
    }
    to->ins = a;
    a->outchain = from->outs;
    a->outchainRev = NULL;
    if (from->outs) {
	from->outs->outchainRev = a;
    }
    from->outs = a;

    from->nouts++;
    to->nins++;

    if (COLORED(a) && nfa->parent == NULL) {
	colorchain(nfa->cm, a);
    }
}

/*
 - allocarc - allocate a new out-arc within a state
 ^ static struct arc *allocarc(struct nfa *, struct state *);
 */
static struct arc *		/* NULL for failure */
allocarc(
    struct nfa *nfa,
    struct state *s)
{
    struct arc *a;

    /*
     * Shortcut
     */

    if (s->free == NULL && s->noas < ABSIZE) {
	a = &s->oas.a[s->noas];
	s->noas++;
	return a;
    }

    /*
     * if none at hand, get more
     */

    if (s->free == NULL) {
	struct arcbatch *newAb;
	int i;

	if (nfa->v->spaceused >= REG_MAX_COMPILE_SPACE) {
	    NERR(REG_ETOOBIG);
	    return NULL;
	}
	newAb = (struct arcbatch *) MALLOC(sizeof(struct arcbatch));
	if (newAb == NULL) {
	    NERR(REG_ESPACE);
	    return NULL;
	}
	nfa->v->spaceused += sizeof(struct arcbatch);
	newAb->next = s->oas.next;
	s->oas.next = newAb;

	for (i=0 ; i<ABSIZE ; i++) {
	    newAb->a[i].type = 0;
	    newAb->a[i].freechain = &newAb->a[i+1];
	}
	newAb->a[ABSIZE-1].freechain = NULL;
	s->free = &newAb->a[0];
    }
    assert(s->free != NULL);

    a = s->free;
    s->free = a->freechain;
    return a;
}

/*
 - freearc - free an arc
 ^ static void freearc(struct nfa *, struct arc *);
 */
static void
freearc(
    struct nfa *nfa,
    struct arc *victim)
{
    struct state *from = victim->from;
    struct state *to = victim->to;
    struct arc *predecessor;

    assert(victim->type != 0);

    /*
     * Take it off color chain if necessary.
     */

    if (COLORED(victim) && nfa->parent == NULL) {
	uncolorchain(nfa->cm, victim);
    }

    /*
     * Take it off source's out-chain.
     */

    assert(from != NULL);
    predecessor = victim->outchainRev;
    if (predecessor == NULL) {
	assert(from->outs == victim);
	from->outs = victim->outchain;
    } else {
	assert(predecessor->outchain == victim);
	predecessor->outchain = victim->outchain;
    }
    if (victim->outchain != NULL) {
	assert(victim->outchain->outchainRev == victim);
	victim->outchain->outchainRev = predecessor;
    }
    from->nouts--;

    /*
     * Take it off target's in-chain.
     */

    assert(to != NULL);
    predecessor = victim->inchainRev;
    if (predecessor == NULL) {
	assert(to->ins == victim);
	to->ins = victim->inchain;
    } else {
	assert(predecessor->inchain == victim);
	predecessor->inchain = victim->inchain;
    }
    if (victim->inchain != NULL) {
	assert(victim->inchain->inchainRev == victim);
	victim->inchain->inchainRev = predecessor;
    }
    to->nins--;

    /*
     * Clean up and place on from-state's free list.
     */

    victim->type = 0;
    victim->from = NULL;	/* precautions... */
    victim->to = NULL;
    victim->inchain = NULL;
    victim->inchainRev = NULL;
    victim->outchain = NULL;
    victim->outchainRev = NULL;
    victim->freechain = from->free;
    from->free = victim;
}

/*
 * changearctarget - flip an arc to have a different to state
 *
 * Caller must have verified that there is no pre-existing duplicate arc.
 *
 * Note that because we store arcs in their from state, we can't easily have
 * a similar changearcsource function.
 */
static void
changearctarget(struct arc * a, struct state * newto)
{
    struct state *oldto = a->to;
    struct arc *predecessor;

    assert(oldto != newto);

    /* take it off old target's in-chain */
    assert(oldto != NULL);
    predecessor = a->inchainRev;
    if (predecessor == NULL) {
	assert(oldto->ins == a);
	oldto->ins = a->inchain;
    } else {
	assert(predecessor->inchain == a);
	predecessor->inchain = a->inchain;
    }
    if (a->inchain != NULL) {
	assert(a->inchain->inchainRev == a);
	a->inchain->inchainRev = predecessor;
    }
    oldto->nins--;

    a->to = newto;

    /* prepend it to new target's in-chain */
    a->inchain = newto->ins;
    a->inchainRev = NULL;
    if (newto->ins) {
	newto->ins->inchainRev = a;
    }
    newto->ins = a;
    newto->nins++;
}

/*
 - hasnonemptyout - Does state have a non-EMPTY out arc?
 ^ static int hasnonemptyout(struct state *);
 */
static int
hasnonemptyout(
    struct state *s)
{
    struct arc *a;

    for (a = s->outs; a != NULL; a = a->outchain) {
	if (a->type != EMPTY) {
	    return 1;
	}
    }
    return 0;
}

/*
 - findarc - find arc, if any, from given source with given type and color
 * If there is more than one such arc, the result is random.
 ^ static struct arc *findarc(struct state *, int, pcolor);
 */
static struct arc *
findarc(
    struct state *s,
    int type,
    pcolor co)
{
    struct arc *a;

    for (a=s->outs ; a!=NULL ; a=a->outchain) {
	if (a->type == type && a->co == co) {
	    return a;
	}
    }
    return NULL;
}

/*
 - cparc - allocate a new arc within an NFA, copying details from old one
 ^ static void cparc(struct nfa *, struct arc *, struct state *,
 ^ 	struct state *);
 */
static void
cparc(
    struct nfa *nfa,
    struct arc *oa,
    struct state *from,
    struct state *to)
{
    newarc(nfa, oa->type, oa->co, from, to);
}

/*
 * sortins - sort the in arcs of a state by from/color/type
 */
static void
sortins(
    struct nfa * nfa,
    struct state * s)
{
    struct arc **sortarray;
    struct arc *a;
    int n = s->nins;
    int i;

    if (n <= 1) {
	return;		/* nothing to do */
    }
    /* make an array of arc pointers ... */
    sortarray = (struct arc **) MALLOC(n * sizeof(struct arc *));
    if (sortarray == NULL) {
	NERR(REG_ESPACE);
	return;
    }
    i = 0;
    for (a = s->ins; a != NULL; a = a->inchain) {
	sortarray[i++] = a;
    }
    assert(i == n);
    /* ... sort the array */
    qsort(sortarray, n, sizeof(struct arc *), sortins_cmp);
    /* ... and rebuild arc list in order */
    /* it seems worth special-casing first and last items to simplify loop */
    a = sortarray[0];
    s->ins = a;
    a->inchain = sortarray[1];
    a->inchainRev = NULL;
    for (i = 1; i < n - 1; i++) {
	a = sortarray[i];
	a->inchain = sortarray[i + 1];
	a->inchainRev = sortarray[i - 1];
    }
    a = sortarray[i];
    a->inchain = NULL;
    a->inchainRev = sortarray[i - 1];
    FREE(sortarray);
}

static int
sortins_cmp(
    const void *a,
    const void *b)
{
    const struct arc *aa = *((const struct arc * const *) a);
    const struct arc *bb = *((const struct arc * const *) b);

    /* we check the fields in the order they are most likely to be different */
    if (aa->from->no < bb->from->no) {
	return -1;
    }
    if (aa->from->no > bb->from->no) {
 	return 1;
    }
    if (aa->co < bb->co) {
 	return -1;
    }
    if (aa->co > bb->co) {
 	return 1;
    }
    if (aa->type < bb->type) {
 	return -1;
    }
    if (aa->type > bb->type) {
 	return 1;
    }
    return 0;
}

/*
 * sortouts - sort the out arcs of a state by to/color/type
 */
static void
sortouts(
    struct nfa * nfa,
    struct state * s)
{
    struct arc **sortarray;
    struct arc *a;
    int	n = s->nouts;
    int	i;

    if (n <= 1) {
	return;					/* nothing to do */
    }
    /* make an array of arc pointers ... */
    sortarray = (struct arc **) MALLOC(n * sizeof(struct arc *));
    if (sortarray == NULL) {
	NERR(REG_ESPACE);
	return;
    }
    i = 0;
    for (a = s->outs; a != NULL; a = a->outchain) {
	sortarray[i++] = a;
    }
    assert(i == n);
    /* ... sort the array */
    qsort(sortarray, n, sizeof(struct arc *), sortouts_cmp);
    /* ... and rebuild arc list in order */
    /* it seems worth special-casing first and last items to simplify loop */
    a = sortarray[0];
    s->outs = a;
    a->outchain = sortarray[1];
    a->outchainRev = NULL;
    for (i = 1; i < n - 1; i++) {
	a = sortarray[i];
	a->outchain = sortarray[i + 1];
	a->outchainRev = sortarray[i - 1];
    }
    a = sortarray[i];
    a->outchain = NULL;
    a->outchainRev = sortarray[i - 1];
    FREE(sortarray);
}

static int
sortouts_cmp(
    const void *a,
    const void *b)
{
    const struct arc *aa = *((const struct arc * const *) a);
    const struct arc *bb = *((const struct arc * const *) b);

    /* we check the fields in the order they are most likely to be different */
    if (aa->to->no < bb->to->no) {
	return -1;
    }
    if (aa->to->no > bb->to->no) {
	return 1;
    }
    if (aa->co < bb->co) {
	return -1;
    }
    if (aa->co > bb->co) {
	return 1;
    }
    if (aa->type < bb->type) {
	return -1;
    }
    if (aa->type > bb->type) {
	return 1;
    }
    return 0;
}

/*
 * Common decision logic about whether to use arc-by-arc operations or
 * sort/merge.  If there's just a few source arcs we cannot recoup the
 * cost of sorting the destination arc list, no matter how large it is.
 * Otherwise, limit the number of arc-by-arc comparisons to about 1000
 * (a somewhat arbitrary choice, but the breakeven point would probably
 * be machine dependent anyway).
 */
#define BULK_ARC_OP_USE_SORT(nsrcarcs, ndestarcs) \
	((nsrcarcs) < 4 ? 0 : ((nsrcarcs) > 32 || (ndestarcs) > 32))

/*
 - moveins - move all in arcs of a state to another state
 * You might think this could be done better by just updating the
 * existing arcs, and you would be right if it weren't for the need
 * for duplicate suppression, which makes it easier to just make new
 * ones to exploit the suppression built into newarc.
 *
 * However, if we have a whole lot of arcs to deal with, retail duplicate
 * checks become too slow.  In that case we proceed by sorting and merging
 * the arc lists, and then we can indeed just update the arcs in-place.
 *
 ^ static void moveins(struct nfa *, struct state *, struct state *);
 */
static void
moveins(
    struct nfa *nfa,
    struct state *oldState,
    struct state *newState)
{
    assert(oldState != newState);

    if (!BULK_ARC_OP_USE_SORT(oldState->nins, newState->nins)) {
	/* With not too many arcs, just do them one at a time */
	struct arc *a;

	while ((a = oldState->ins) != NULL) {
	    cparc(nfa, a, a->from, newState);
	    freearc(nfa, a);
	}
    } else {
	/*
	 * With many arcs, use a sort-merge approach.  Note changearctarget()
	 * will put the arc onto the front of newState's chain, so it does not
	 * break our walk through the sorted part of the chain.
	 */
	struct arc *oa;
	struct arc *na;

	/*
	 * Because we bypass newarc() in this code path, we'd better include a
	 * cancel check.
	 */
	if (CANCEL_REQUESTED(nfa->v->re)) {
	    NERR(REG_CANCEL);
	    return;
	}

	sortins(nfa, oldState);
	sortins(nfa, newState);
	if (NISERR()) {
	    return;		/* might have failed to sort */
	}
	oa = oldState->ins;
	na = newState->ins;
	while (oa != NULL && na != NULL) {
	    struct arc *a = oa;

	    switch (sortins_cmp(&oa, &na)) {
		case -1:
		    /* newState does not have anything matching oa */
		    oa = oa->inchain;

		    /*
		     * Rather than doing createarc+freearc, we can just unlink
		     * and relink the existing arc struct.
		     */
		    changearctarget(a, newState);
		    break;
		case 0:
		    /* match, advance in both lists */
		    oa = oa->inchain;
		    na = na->inchain;
		    /* ... and drop duplicate arc from oldState */
		    freearc(nfa, a);
		    break;
		case +1:
		    /* advance only na; oa might have a match later */
		    na = na->inchain;
		    break;
		default:
		    assert(NOTREACHED);
	    }
	}
	while (oa != NULL) {
	    /* newState does not have anything matching oa */
	    struct arc *a = oa;

	    oa = oa->inchain;
	    changearctarget(a, newState);
	}
    }

    assert(oldState->nins == 0);
    assert(oldState->ins == NULL);
}

/*
 - copyins - copy in arcs of a state to another state
 ^ static VOID copyins(struct nfa *, struct state *, struct state *, int);
 */
static void
copyins(
    struct nfa *nfa,
    struct state *oldState,
    struct state *newState)
{
    assert(oldState != newState);

    if (!BULK_ARC_OP_USE_SORT(oldState->nins, newState->nins)) {
	/* With not too many arcs, just do them one at a time */
	struct arc *a;

	for (a = oldState->ins; a != NULL; a = a->inchain) {
	    cparc(nfa, a, a->from, newState);
	}
    } else {
	/*
	 * With many arcs, use a sort-merge approach.  Note that createarc()
	 * will put new arcs onto the front of newState's chain, so it does
	 * not break our walk through the sorted part of the chain.
	 */
	struct arc *oa;
	struct arc *na;

	/*
	 * Because we bypass newarc() in this code path, we'd better include a
	 * cancel check.
	 */
	if (CANCEL_REQUESTED(nfa->v->re)) {
	    NERR(REG_CANCEL);
	    return;
	}

	sortins(nfa, oldState);
	sortins(nfa, newState);
	if (NISERR()) {
	    return;		/* might have failed to sort */
	}
	oa = oldState->ins;
	na = newState->ins;
	while (oa != NULL && na != NULL) {
	    struct arc *a = oa;

	    switch (sortins_cmp(&oa, &na)) {
		case -1:
		    /* newState does not have anything matching oa */
		    oa = oa->inchain;
		    createarc(nfa, a->type, a->co, a->from, newState);
		    break;
		case 0:
		    /* match, advance in both lists */
		    oa = oa->inchain;
		    na = na->inchain;
		    break;
		case +1:
		    /* advance only na; oa might have a match later */
		    na = na->inchain;
		    break;
		default:
		    assert(NOTREACHED);
	    }
	}
	while (oa != NULL) {
	    /* newState does not have anything matching oa */
	    struct arc *a = oa;

	    oa = oa->inchain;
	    createarc(nfa, a->type, a->co, a->from, newState);
	}
    }
}

/*
 * mergeins - merge a list of inarcs into a state
 *
 * This is much like copyins, but the source arcs are listed in an array,
 * and are not guaranteed unique.  It's okay to clobber the array contents.
 */
static void
mergeins(
    struct nfa * nfa,
    struct state * s,
    struct arc ** arcarray,
    int arccount)
{
    struct arc *na;
    int	i;
    int	j;

    if (arccount <= 0) {
	return;
    }

    /*
     * Because we bypass newarc() in this code path, we'd better include a
     * cancel check.
     */
    if (CANCEL_REQUESTED(nfa->v->re)) {
	NERR(REG_CANCEL);
	return;
    }

    /* Sort existing inarcs as well as proposed new ones */
    sortins(nfa, s);
    if (NISERR()) {
	return;			/* might have failed to sort */
    }

    qsort(arcarray, arccount, sizeof(struct arc *), sortins_cmp);

    /*
     * arcarray very likely includes dups, so we must eliminate them.  (This
     * could be folded into the next loop, but it's not worth the trouble.)
     */
    j = 0;
    for (i = 1; i < arccount; i++) {
	switch (sortins_cmp(&arcarray[j], &arcarray[i])) {
	    case -1:
		/* non-dup */
		arcarray[++j] = arcarray[i];
		break;
	    case 0:
		/* dup */
		break;
	    default:
		/* trouble */
		assert(NOTREACHED);
	}
    }
    arccount = j + 1;

    /*
     * Now merge into s' inchain.  Note that createarc() will put new arcs
     * onto the front of s's chain, so it does not break our walk through the
     * sorted part of the chain.
     */
    i = 0;
    na = s->ins;
    while (i < arccount && na != NULL) {
	struct arc *a = arcarray[i];

	switch (sortins_cmp(&a, &na)) {
	    case -1:
		/* s does not have anything matching a */
		createarc(nfa, a->type, a->co, a->from, s);
		i++;
		break;
	    case 0:
		/* match, advance in both lists */
		i++;
		na = na->inchain;
		break;
	    case +1:
		/* advance only na; array might have a match later */
		na = na->inchain;
		break;
	    default:
		assert(NOTREACHED);
	}
    }
    while (i < arccount) {
	/* s does not have anything matching a */
	struct arc *a = arcarray[i];

	createarc(nfa, a->type, a->co, a->from, s);
	i++;
    }
}

/*
 - moveouts - move all out arcs of a state to another state
 ^ static void moveouts(struct nfa *, struct state *, struct state *);
 */
static void
moveouts(
    struct nfa *nfa,
    struct state *oldState,
    struct state *newState)
{
    assert(oldState != newState);

    if (!BULK_ARC_OP_USE_SORT(oldState->nouts, newState->nouts)) {
	/* With not too many arcs, just do them one at a time */
	struct arc *a;

	while ((a = oldState->outs) != NULL) {
	    cparc(nfa, a, newState, a->to);
	    freearc(nfa, a);
	}
    } else {
	/*
	 * With many arcs, use a sort-merge approach.  Note that createarc()
	 * will put new arcs onto the front of newState's chain, so it does
	 * not break our walk through the sorted part of the chain.
	 */
	struct arc *oa;
	struct arc *na;

	/*
	 * Because we bypass newarc() in this code path, we'd better include a
	 * cancel check.
	 */
	if (CANCEL_REQUESTED(nfa->v->re)) {
	    NERR(REG_CANCEL);
	    return;
	}

	sortouts(nfa, oldState);
	sortouts(nfa, newState);
	if (NISERR()) {
	    return;	/* might have failed to sort */
	}
	oa = oldState->outs;
	na = newState->outs;
	while (oa != NULL && na != NULL) {
	    struct arc *a = oa;

	    switch (sortouts_cmp(&oa, &na)) {
		case -1:
		    /* newState does not have anything matching oa */
		    oa = oa->outchain;
		    createarc(nfa, a->type, a->co, newState, a->to);
		    freearc(nfa, a);
		    break;
		case 0:
		    /* match, advance in both lists */
		    oa = oa->outchain;
		    na = na->outchain;
		    /* ... and drop duplicate arc from oldState */
		    freearc(nfa, a);
		    break;
		case +1:
		    /* advance only na; oa might have a match later */
		    na = na->outchain;
		    break;
		default:
		    assert(NOTREACHED);
	    }
	}
	while (oa != NULL) {
	    /* newState does not have anything matching oa */
	    struct arc *a = oa;

	    oa = oa->outchain;
	    createarc(nfa, a->type, a->co, newState, a->to);
	    freearc(nfa, a);
	}
    }

    assert(oldState->nouts == 0);
    assert(oldState->outs == NULL);
}

/*
 - copyouts - copy out arcs of a state to another state
 ^ static VOID copyouts(struct nfa *, struct state *, struct state *, int);
 */
static void
copyouts(
    struct nfa *nfa,
    struct state *oldState,
    struct state *newState)
{
    assert(oldState != newState);

    if (!BULK_ARC_OP_USE_SORT(oldState->nouts, newState->nouts)) {
	/* With not too many arcs, just do them one at a time */
	struct arc *a;

	for (a = oldState->outs; a != NULL; a = a->outchain) {
	    cparc(nfa, a, newState, a->to);
	}
    } else {
 	/*
	 * With many arcs, use a sort-merge approach.  Note that createarc()
	 * will put new arcs onto the front of newState's chain, so it does
	 * not break our walk through the sorted part of the chain.
	 */
	struct arc *oa;
	struct arc *na;

	/*
	 * Because we bypass newarc() in this code path, we'd better include a
	 * cancel check.
	 */
	if (CANCEL_REQUESTED(nfa->v->re)) {
	    NERR(REG_CANCEL);
	    return;
	}

	sortouts(nfa, oldState);
	sortouts(nfa, newState);
	if (NISERR()) {
	    return;		/* might have failed to sort */
	}
	oa = oldState->outs;
	na = newState->outs;
	while (oa != NULL && na != NULL) {
	    struct arc *a = oa;

	    switch (sortouts_cmp(&oa, &na)) {
		case -1:
		    /* newState does not have anything matching oa */
		    oa = oa->outchain;
		    createarc(nfa, a->type, a->co, newState, a->to);
		    break;
		case 0:
		    /* match, advance in both lists */
		    oa = oa->outchain;
		    na = na->outchain;
		    break;
		case +1:
		    /* advance only na; oa might have a match later */
		    na = na->outchain;
		    break;
		default:
		    assert(NOTREACHED);
	    }
	}
	while (oa != NULL) {
	    /* newState does not have anything matching oa */
	    struct arc *a = oa;

	    oa = oa->outchain;
	    createarc(nfa, a->type, a->co, newState, a->to);
	}
    }
}

/*
 - cloneouts - copy out arcs of a state to another state pair, modifying type
 ^ static void cloneouts(struct nfa *, struct state *, struct state *,
 ^ 	struct state *, int);
 */
static void
cloneouts(
    struct nfa *nfa,
    struct state *old,
    struct state *from,
    struct state *to,
    int type)
{
    struct arc *a;

    assert(old != from);

    for (a=old->outs ; a!=NULL ; a=a->outchain) {
	newarc(nfa, type, a->co, from, to);
    }
}

/*
 - delsub - delete a sub-NFA, updating subre pointers if necessary
 * This uses a recursive traversal of the sub-NFA, marking already-seen
 * states using their tmp pointer.
 ^ static void delsub(struct nfa *, struct state *, struct state *);
 */
static void
delsub(
    struct nfa *nfa,
    struct state *lp,		/* the sub-NFA goes from here... */
    struct state *rp)		/* ...to here, *not* inclusive */
{
    assert(lp != rp);

    rp->tmp = rp;		/* mark end */

    deltraverse(nfa, lp, lp);
    assert(lp->nouts == 0 && rp->nins == 0);	/* did the job */
    assert(lp->no != FREESTATE && rp->no != FREESTATE);	/* no more */

    rp->tmp = NULL;		/* unmark end */
    lp->tmp = NULL;		/* and begin, marked by deltraverse */
}

/*
 - deltraverse - the recursive heart of delsub
 * This routine's basic job is to destroy all out-arcs of the state.
 ^ static void deltraverse(struct nfa *, struct state *, struct state *);
 */
static void
deltraverse(
    struct nfa *nfa,
    struct state *leftend,
    struct state *s)
{
    struct arc *a;
    struct state *to;

    if (s->nouts == 0) {
	return;			/* nothing to do */
    }
    if (s->tmp != NULL) {
	return;			/* already in progress */
    }

    s->tmp = s;			/* mark as in progress */

    while ((a = s->outs) != NULL) {
	to = a->to;
	deltraverse(nfa, leftend, to);
	assert(to->nouts == 0 || to->tmp != NULL);
	freearc(nfa, a);
	if (to->nins == 0 && to->tmp == NULL) {
	    assert(to->nouts == 0);
	    freestate(nfa, to);
	}
    }

    assert(s->no != FREESTATE);	/* we're still here */
    assert(s == leftend || s->nins != 0);	/* and still reachable */
    assert(s->nouts == 0);	/* but have no outarcs */

    s->tmp = NULL;		/* we're done here */
}

/*
 - dupnfa - duplicate sub-NFA
 * Another recursive traversal, this time using tmp to point to duplicates as
 * well as mark already-seen states. (You knew there was a reason why it's a
 * state pointer, didn't you? :-))
 ^ static void dupnfa(struct nfa *, struct state *, struct state *,
 ^ 	struct state *, struct state *);
 */
static void
dupnfa(
    struct nfa *nfa,
    struct state *start,	/* duplicate of subNFA starting here */
    struct state *stop,		/* and stopping here */
    struct state *from,		/* stringing duplicate from here */
    struct state *to)		/* to here */
{
    if (start == stop) {
	newarc(nfa, EMPTY, 0, from, to);
	return;
    }

    stop->tmp = to;
    duptraverse(nfa, start, from, 0);
    /* done, except for clearing out the tmp pointers */

    stop->tmp = NULL;
    cleartraverse(nfa, start);
}

/*
 - duptraverse - recursive heart of dupnfa
 ^ static void duptraverse(struct nfa *, struct state *, struct state *);
 */
static void
duptraverse(
    struct nfa *nfa,
    struct state *s,
    struct state *stmp,		/* s's duplicate, or NULL */
    int depth)
{
    struct arc *a;

    if (s->tmp != NULL) {
	return;			/* already done */
    }

    s->tmp = (stmp == NULL) ? newstate(nfa) : stmp;
    if (s->tmp == NULL) {
	assert(NISERR());
	return;
    }

    /*
     * Arbitrary depth limit. Needs tuning, but this value is sufficient to
     * make all normal tests (not reg-33.14) pass.
     */
#ifndef DUPTRAVERSE_MAX_DEPTH
#define DUPTRAVERSE_MAX_DEPTH 15000
#endif

    if (depth++ > DUPTRAVERSE_MAX_DEPTH) {
	NERR(REG_ESPACE);
    }

    for (a=s->outs ; a!=NULL && !NISERR() ; a=a->outchain) {
	duptraverse(nfa, a->to, NULL, depth);
	if (NISERR()) {
	    break;
	}
	assert(a->to->tmp != NULL);
	cparc(nfa, a, s->tmp, a->to->tmp);
    }
}

/*
 - cleartraverse - recursive cleanup for algorithms that leave tmp ptrs set
 ^ static void cleartraverse(struct nfa *, struct state *);
 */
static void
cleartraverse(
    struct nfa *nfa,
    struct state *s)
{
    struct arc *a;

    if (s->tmp == NULL) {
	return;
    }
    s->tmp = NULL;

    for (a=s->outs ; a!=NULL ; a=a->outchain) {
	cleartraverse(nfa, a->to);
    }
}

/*
 - specialcolors - fill in special colors for an NFA
 ^ static void specialcolors(struct nfa *);
 */
static void
specialcolors(
    struct nfa *nfa)
{
    /*
     * False colors for BOS, BOL, EOS, EOL
     */

    if (nfa->parent == NULL) {
	nfa->bos[0] = pseudocolor(nfa->cm);
	nfa->bos[1] = pseudocolor(nfa->cm);
	nfa->eos[0] = pseudocolor(nfa->cm);
	nfa->eos[1] = pseudocolor(nfa->cm);
    } else {
	assert(nfa->parent->bos[0] != COLORLESS);
	nfa->bos[0] = nfa->parent->bos[0];
	assert(nfa->parent->bos[1] != COLORLESS);
	nfa->bos[1] = nfa->parent->bos[1];
	assert(nfa->parent->eos[0] != COLORLESS);
	nfa->eos[0] = nfa->parent->eos[0];
	assert(nfa->parent->eos[1] != COLORLESS);
	nfa->eos[1] = nfa->parent->eos[1];
    }
}

/*
 - optimize - optimize an NFA
 ^ static long optimize(struct nfa *, FILE *);
 */

 /*
  * The main goal of this function is not so much "optimization" (though it
  * does try to get rid of useless NFA states) as reducing the NFA to a form
  * the regex executor can handle.  The executor, and indeed the cNFA format
  * that is its input, can only handle PLAIN and LACON arcs.  The output of
  * the regex parser also includes EMPTY (do-nothing) arcs, as well as
  * ^, $, AHEAD, and BEHIND constraint arcs, which we must get rid of here.
  * We first get rid of EMPTY arcs and then deal with the constraint arcs.
  * The hardest part of either job is to get rid of circular loops of the
  * target arc type.  We would have to do that in any case, though, as such a
  * loop would otherwise allow the executor to cycle through the loop endlessly
  * without making any progress in the input string.
  */
static long			/* re_info bits */
optimize(
    struct nfa *nfa,
    FILE *f)			/* for debug output; NULL none */
{
    int verbose = (f != NULL) ? 1 : 0;

    if (verbose) {
	fprintf(f, "\ninitial cleanup:\n");
    }
    cleanup(nfa);		/* may simplify situation */
    if (verbose) {
	dumpnfa(nfa, f);
    }
    if (verbose) {
	fprintf(f, "\nempties:\n");
    }
    fixempties(nfa, f);		/* get rid of EMPTY arcs */
    if (verbose) {
	fprintf(f, "\nconstraints:\n");
    }
    fixconstraintloops(nfa, f);	/* get rid of constraint loops */
    pullback(nfa, f);		/* pull back constraints backward */
    pushfwd(nfa, f);		/* push fwd constraints forward */
    if (verbose) {
	fprintf(f, "\nfinal cleanup:\n");
    }
    cleanup(nfa);		/* final tidying */
#ifdef REG_DEBUG
    if (verbose) {
	dumpnfa(nfa, f);
    }
#endif
    return analyze(nfa);	/* and analysis */
}

/*
 - pullback - pull back constraints backward to eliminate them
 ^ static void pullback(struct nfa *, FILE *);
 */
static void
pullback(
    struct nfa *nfa,
    FILE *f)			/* for debug output; NULL none */
{
    struct state *s;
    struct state *nexts;
    struct arc *a;
    struct arc *nexta;
    struct state *intermediates;
    int progress;

    /*
     * Find and pull until there are no more.
     */

    do {
	progress = 0;
	for (s=nfa->states ; s!=NULL && !NISERR() ; s=nexts) {
	    nexts = s->next;
	    intermediates = NULL;
	    for (a=s->outs ; a!=NULL && !NISERR() ; a=nexta) {
		nexta = a->outchain;
		if (a->type == '^' || a->type == BEHIND) {
		    if (pull(nfa, a, &intermediates)) {
			progress = 1;
		    }
		}
		assert(nexta == NULL || s->no != FREESTATE);
	    }
	    /* clear tmp fields of intermediate states created here */
	    while (intermediates != NULL) {
		struct state *ns = intermediates->tmp;

		intermediates->tmp = NULL;
		intermediates = ns;
	    }
	    /* if s is now useless, get rid of it */
	    if ((s->nins == 0 || s->nouts == 0) && !s->flag) {
		dropstate(nfa, s);
	    }
	}
	if (progress && f != NULL) {
	    dumpnfa(nfa, f);
	}
    } while (progress && !NISERR());
    if (NISERR()) {
	return;
    }

    /*
     * Any ^ constraints we were able to pull to the start state can now be
     * replaced by PLAIN arcs referencing the BOS or BOL colors.  There should
     * be no other ^ or BEHIND arcs left in the NFA, though we do not check
     * that here (compact() will fail if so).
     */
    for (a=nfa->pre->outs ; a!=NULL ; a=nexta) {
	nexta = a->outchain;
	if (a->type == '^') {
	    assert(a->co == 0 || a->co == 1);
	    newarc(nfa, PLAIN, nfa->bos[a->co], a->from, a->to);
	    freearc(nfa, a);
	}
    }
}

/*
 - pull - pull a back constraint backward past its source state
 *
 * Returns 1 if successful (which it always is unless the source is the
 * start state or we have an internal error), 0 if nothing happened.
 *
 * A significant property of this function is that it deletes no pre-existing
 * states, and no outarcs of the constraint's from state other than the given
 * constraint arc.  This makes the loops in pullback() safe, at the cost that
 * we may leave useless states behind.  Therefore, we leave it to pullback()
 * to delete such states.
 *
 * If the from state has multiple back-constraint outarcs, and/or multiple
 * compatible constraint inarcs, we only need to create one new intermediate
 * state per combination of predecessor and successor states.  *intermediates
 * points to a list of such intermediate states for this from state (chained
 * through their tmp fields).
 ^ static int pull(struct nfa *, struct arc *);
 */
static int
pull(
    struct nfa *nfa,
    struct arc *con,
    struct state **intermediates)
{
    struct state *from = con->from;
    struct state *to = con->to;
    struct arc *a;
    struct arc *nexta;
    struct state *s;

    assert(from != to);		/* should have gotten rid of this earlier */
    if (from->flag) {		/* can't pull back beyond start */
	return 0;
    }
    if (from->nins == 0) {	/* unreachable */
	freearc(nfa, con);
	return 1;
    }

    /*
     * First, clone from state if necessary to avoid other outarcs.  This may
     * seem wasteful, but it simplifies the logic, and we'll get rid of the
     * clone state again at the bottom.
     */

    if (from->nouts > 1) {
	s = newstate(nfa);
	if (NISERR()) {
	    return 0;
	}
	copyins(nfa, from, s);	/* duplicate inarcs */
	cparc(nfa, con, s, to);		/* move constraint arc */
	freearc(nfa, con);
	if (NISERR()) {
	    return 0;
	}
	from = s;
	con = from->outs;
    }
    assert(from->nouts == 1);

    /*
     * Propagate the constraint into the from state's inarcs.
     */

    for (a=from->ins ; a!=NULL && !NISERR(); a=nexta) {
	nexta = a->inchain;
	switch (combine(con, a)) {
	case INCOMPATIBLE:	/* destroy the arc */
	    freearc(nfa, a);
	    break;
	case SATISFIED:		/* no action needed */
	    break;
	case COMPATIBLE:	/* swap the two arcs, more or less */
	    /* need an intermediate state, but might have one already */
	    for (s = *intermediates; s != NULL; s = s->tmp) {
		assert(s->nins > 0 && s->nouts > 0);
		if (s->ins->from == a->from && s->outs->to == to) {
		    break;
		}
	    }
	    if (s == NULL) {
		s = newstate(nfa);
		if (NISERR()) {
		    return 0;
		}
		s->tmp = *intermediates;
		*intermediates = s;
	    }
  	    cparc(nfa, con, a->from, s);
	    cparc(nfa, a, s, to);
 	    freearc(nfa, a);
  	    break;
	default:
	    assert(NOTREACHED);
	    break;
	}
    }

    /*
     * Remaining inarcs, if any, incorporate the constraint.
     */

    moveins(nfa, from, to);
    freearc(nfa, con);
    /* from state is now useless, but we leave it to pullback() to clean up */
    return 1;
}

/*
 - pushfwd - push forward constraints forward to eliminate them
 ^ static void pushfwd(struct nfa *, FILE *);
 */
static void
pushfwd(
    struct nfa *nfa,
    FILE *f)			/* for debug output; NULL none */
{
    struct state *s;
    struct state *nexts;
    struct arc *a;
    struct arc *nexta;
    struct state *intermediates;
    int progress;

    /*
     * Find and push until there are no more.
     */

    do {
	progress = 0;
	for (s=nfa->states ; s!=NULL && !NISERR() ; s=nexts) {
	    nexts = s->next;
	    intermediates = NULL;
	    for (a = s->ins; a != NULL && !NISERR(); a = nexta) {
		nexta = a->inchain;
		if (a->type == '$' || a->type == AHEAD) {
		    if (push(nfa, a, &intermediates)) {
			progress = 1;
		    }
		}
	    }
	    /* clear tmp fields of intermediate states created here */
	    while (intermediates != NULL) {
		struct state *ns = intermediates->tmp;

		intermediates->tmp = NULL;
		intermediates = ns;
	    }
	    /* if s is now useless, get rid of it */
	    if ((s->nins == 0 || s->nouts == 0) && !s->flag) {
		dropstate(nfa, s);
	    }
	}
	if (progress && f != NULL) {
	    dumpnfa(nfa, f);
	}
    } while (progress && !NISERR());
    if (NISERR()) {
	return;
    }

    /*
     * Any $ constraints we were able to push to the post state can now be
     * replaced by PLAIN arcs referencing the EOS or EOL colors.  There should
     * be no other $ or AHEAD arcs left in the NFA, though we do not check
     * that here (compact() will fail if so).
     */
    for (a = nfa->post->ins; a != NULL; a = nexta) {
	nexta = a->inchain;
	if (a->type == '$') {
	    assert(a->co == 0 || a->co == 1);
	    newarc(nfa, PLAIN, nfa->eos[a->co], a->from, a->to);
	    freearc(nfa, a);
	}
    }
}

/*
 - push - push a forward constraint forward past its destination state
 *
 * Returns 1 if successful (which it always is unless the destination is the
 * post state or we have an internal error), 0 if nothing happened.
 *
 * A significant property of this function is that it deletes no pre-existing
 * states, and no inarcs of the constraint's to state other than the given
 * constraint arc.  This makes the loops in pushfwd() safe, at the cost that
 * we may leave useless states behind.  Therefore, we leave it to pushfwd()
 * to delete such states.
 *
 * If the to state has multiple forward-constraint inarcs, and/or multiple
 * compatible constraint outarcs, we only need to create one new intermediate
 * state per combination of predecessor and successor states.  *intermediates
 * points to a list of such intermediate states for this to state (chained
 * through their tmp fields).
 ^ static int push(struct nfa *, struct arc *);
 */
static int
push(
    struct nfa *nfa,
    struct arc *con,
    struct state **intermediates)
{
    struct state *from = con->from;
    struct state *to = con->to;
    struct arc *a;
    struct arc *nexta;
    struct state *s;

    assert(to != from);		/* should have gotten rid of this earlier */
    if (to->flag) {		/* can't push forward beyond end */
	return 0;
    }
    if (to->nouts == 0) {	/* dead end */
	freearc(nfa, con);
	return 1;
    }

    /*
     * First, clone to state if necessary to avoid other inarcs.  This may
     * seem wasteful, but it simplifies the logic, and we'll get rid of the
     * clone state again at the bottom.
     */

    if (to->nins > 1) {
	s = newstate(nfa);
	if (NISERR()) {
	    return 0;
	}
	copyouts(nfa, to, s);		/* duplicate outarcs */
	cparc(nfa, con, from, s);	/* move constraint arc */
	freearc(nfa, con);
	if (NISERR()) {
	    return 0;
	}
	to = s;
	con = to->ins;
    }
    assert(to->nins == 1);

    /*
     * Propagate the constraint into the to state's outarcs.
     */

    for (a = to->outs; a != NULL && !NISERR(); a = nexta) {
	nexta = a->outchain;
	switch (combine(con, a)) {
	case INCOMPATIBLE:	/* destroy the arc */
	    freearc(nfa, a);
	    break;
	case SATISFIED:		/* no action needed */
	    break;
	case COMPATIBLE:	/* swap the two arcs, more or less */
	    /* need an intermediate state, but might have one already */
	    for (s = *intermediates; s != NULL; s = s->tmp) {
		assert(s->nins > 0 && s->nouts > 0);
		if (s->ins->from == from && s->outs->to == a->to) {
		    break;
		}
	    }
	    if (s == NULL) {
		s = newstate(nfa);
		if (NISERR()) {
		    return 0;
		}
		s->tmp = *intermediates;
		*intermediates = s;
	    }
	    cparc(nfa, con, s, a->to);
  	    cparc(nfa, a, from, s);
  	    freearc(nfa, a);
  	    break;
	default:
	    assert(NOTREACHED);
	    break;
	}
    }

    /*
     * Remaining outarcs, if any, incorporate the constraint.
     */

    moveouts(nfa, to, from);
    freearc(nfa, con);
    /* to state is now useless, but we leave it to pushfwd() to clean up */
    return 1;
}

/*
 - combine - constraint lands on an arc, what happens?
 ^ #def	INCOMPATIBLE	1	// destroys arc
 ^ #def	SATISFIED	2	// constraint satisfied
 ^ #def	COMPATIBLE	3	// compatible but not satisfied yet
 ^ static int combine(struct arc *, struct arc *);
 */
static int
combine(
    struct arc *con,
    struct arc *a)
{
#define CA(ct,at)	(((ct)<<CHAR_BIT) | (at))

    switch (CA(con->type, a->type)) {
    case CA('^', PLAIN):	/* newlines are handled separately */
    case CA('$', PLAIN):
	return INCOMPATIBLE;
	break;
    case CA(AHEAD, PLAIN):	/* color constraints meet colors */
    case CA(BEHIND, PLAIN):
	if (con->co == a->co) {
	    return SATISFIED;
	}
	return INCOMPATIBLE;
	break;
    case CA('^', '^'):		/* collision, similar constraints */
    case CA('$', '$'):
    case CA(AHEAD, AHEAD):
    case CA(BEHIND, BEHIND):
	if (con->co == a->co) {	/* true duplication */
	    return SATISFIED;
	}
	return INCOMPATIBLE;
	break;
    case CA('^', BEHIND):	/* collision, dissimilar constraints */
    case CA(BEHIND, '^'):
    case CA('$', AHEAD):
    case CA(AHEAD, '$'):
	return INCOMPATIBLE;
	break;
    case CA('^', '$'):		/* constraints passing each other */
    case CA('^', AHEAD):
    case CA(BEHIND, '$'):
    case CA(BEHIND, AHEAD):
    case CA('$', '^'):
    case CA('$', BEHIND):
    case CA(AHEAD, '^'):
    case CA(AHEAD, BEHIND):
    case CA('^', LACON):
    case CA(BEHIND, LACON):
    case CA('$', LACON):
    case CA(AHEAD, LACON):
	return COMPATIBLE;
	break;
    }
    assert(NOTREACHED);
    return INCOMPATIBLE;	/* for benefit of blind compilers */
}

/*
 - fixempties - get rid of EMPTY arcs
 ^ static void fixempties(struct nfa *, FILE *);
 */
static void
fixempties(
    struct nfa *nfa,
    FILE *f)			/* for debug output; NULL none */
{
    struct state *s;
    struct state *s2;
    struct state *nexts;
    struct arc *a;
    struct arc *nexta;
    int totalinarcs;
    struct arc **inarcsorig;
    struct arc **arcarray;
    int arccount;
    int prevnins;
    int nskip;

    /*
     * First, get rid of any states whose sole out-arc is an EMPTY,
     * since they're basically just aliases for their successor.  The
     * parsing algorithm creates enough of these that it's worth
     * special-casing this.
     */
    for (s = nfa->states; s != NULL && !NISERR(); s = nexts) {
	nexts = s->next;
	if (s->flag || s->nouts != 1) {
	    continue;
	}
	a = s->outs;
	assert(a != NULL && a->outchain == NULL);
	if (a->type != EMPTY) {
	    continue;
	}
	if (s != a->to) {
	    moveins(nfa, s, a->to);
	}
	dropstate(nfa, s);
    }

    /*
     * Similarly, get rid of any state with a single EMPTY in-arc, by
     * folding it into its predecessor.
     */
    for (s = nfa->states; s != NULL && !NISERR(); s = nexts) {
	nexts = s->next;
	/* Ensure tmp fields are clear for next step */
	assert(s->tmp == NULL);
	if (s->flag || s->nins != 1) {
	    continue;
	}
	a = s->ins;
	assert(a != NULL && a->inchain == NULL);
	if (a->type != EMPTY) {
	    continue;
	}
	if (s != a->from) {
	    moveouts(nfa, s, a->from);
	}
	dropstate(nfa, s);
    }

    if (NISERR()) {
	return;
    }

    /*
     * For each remaining NFA state, find all other states from which it is
     * reachable by a chain of one or more EMPTY arcs.  Then generate new arcs
     * that eliminate the need for each such chain.
     *
     * We could replace a chain of EMPTY arcs that leads from a "from" state
     * to a "to" state either by pushing non-EMPTY arcs forward (linking
     * directly from "from"'s predecessors to "to") or by pulling them back
     * (linking directly from "from" to "to"'s successors).  We choose to
     * always do the former; this choice is somewhat arbitrary, but the
     * approach below requires that we uniformly do one or the other.
     *
     * Suppose we have a chain of N successive EMPTY arcs (where N can easily
     * approach the size of the NFA).  All of the intermediate states must
     * have additional inarcs and outarcs, else they'd have been removed by
     * the steps above.  Assuming their inarcs are mostly not empties, we will
     * add O(N^2) arcs to the NFA, since a non-EMPTY inarc leading to any one
     * state in the chain must be duplicated to lead to all its successor
     * states as well.  So there is no hope of doing less than O(N^2) work;
     * however, we should endeavor to keep the big-O cost from being even
     * worse than that, which it can easily become without care.  In
     * particular, suppose we were to copy all S1's inarcs forward to S2, and
     * then also to S3, and then later we consider pushing S2's inarcs forward
     * to S3.  If we include the arcs already copied from S1 in that, we'd be
     * doing O(N^3) work.  (The duplicate-arc elimination built into newarc()
     * and its cohorts would get rid of the extra arcs, but not without cost.)
     *
     * We can avoid this cost by treating only arcs that existed at the start
     * of this phase as candidates to be pushed forward.  To identify those,
     * we remember the first inarc each state had to start with.  We rely on
     * the fact that newarc() and friends put new arcs on the front of their
     * to-states' inchains, and that this phase never deletes arcs, so that
     * the original arcs must be the last arcs in their to-states' inchains.
     *
     * So the process here is that, for each state in the NFA, we gather up
     * all non-EMPTY inarcs of states that can reach the target state via
     * EMPTY arcs.  We then sort, de-duplicate, and merge these arcs into the
     * target state's inchain.  (We can safely use sort-merge for this as long
     * as we update each state's original-arcs pointer after we add arcs to
     * it; the sort step of mergeins probably changed the order of the old
     * arcs.)
     *
     * Another refinement worth making is that, because we only add non-EMPTY
     * arcs during this phase, and all added arcs have the same from-state as
     * the non-EMPTY arc they were cloned from, we know ahead of time that any
     * states having only EMPTY outarcs will be useless for lack of outarcs
     * after we drop the EMPTY arcs.  (They cannot gain non-EMPTY outarcs if
     * they had none to start with.)  So we need not bother to update the
     * inchains of such states at all.
     */

    /* Remember the states' first original inarcs */
    /* ... and while at it, count how many old inarcs there are altogether */
    inarcsorig = (struct arc **) MALLOC(nfa->nstates * sizeof(struct arc *));
    if (inarcsorig == NULL) {
	NERR(REG_ESPACE);
	return;
    }
    totalinarcs = 0;
    for (s = nfa->states; s != NULL; s = s->next) {
	inarcsorig[s->no] = s->ins;
	totalinarcs += s->nins;
    }

    /*
     * Create a workspace for accumulating the inarcs to be added to the
     * current target state.  totalinarcs is probably a considerable
     * overestimate of the space needed, but the NFA is unlikely to be large
     * enough at this point to make it worth being smarter.
     */
    arcarray = (struct arc **) MALLOC(totalinarcs * sizeof(struct arc *));
    if (arcarray == NULL) {
	NERR(REG_ESPACE);
	FREE(inarcsorig);
	return;
    }

    /* And iterate over the target states */
    for (s = nfa->states; s != NULL && !NISERR(); s = s->next) {
	/* Ignore target states without non-EMPTY outarcs, per note above */
	if (!s->flag && !hasnonemptyout(s)) {
	    continue;
	}

	/* Find predecessor states and accumulate their original inarcs */
	arccount = 0;
	for (s2 = emptyreachable(nfa, s, s, inarcsorig); s2 != s; s2 = nexts) {
	    /* Add s2's original inarcs to arcarray[], but ignore empties */
	    for (a = inarcsorig[s2->no]; a != NULL; a = a->inchain) {
		if (a->type != EMPTY) {
		    arcarray[arccount++] = a;
		}
	    }

  	    /* Reset the tmp fields as we walk back */
  	    nexts = s2->tmp;
  	    s2->tmp = NULL;
  	}
  	s->tmp = NULL;
	assert(arccount <= totalinarcs);

	/* Remember how many original inarcs this state has */
	prevnins = s->nins;

	/* Add non-duplicate inarcs to target state */
	mergeins(nfa, s, arcarray, arccount);

	/* Now we must update the state's inarcsorig pointer */
	nskip = s->nins - prevnins;
	a = s->ins;
	while (nskip-- > 0) {
	    a = a->inchain;
	}
	inarcsorig[s->no] = a;
    }

    FREE(arcarray);
    FREE(inarcsorig);

    if (NISERR()) {
	return;
    }

    /*
     * Remove all the EMPTY arcs, since we don't need them anymore.
     */
    for (s = nfa->states; s != NULL; s = s->next) {
	for (a = s->outs; a != NULL; a = nexta) {
	    nexta = a->outchain;
	    if (a->type == EMPTY) {
		freearc(nfa, a);
	    }
	}
    }

    /*
     * And remove any states that have become useless.  (This cleanup is
     * not very thorough, and would be even less so if we tried to
     * combine it with the previous step; but cleanup() will take care
     * of anything we miss.)
     */
    for (s = nfa->states; s != NULL; s = nexts) {
	nexts = s->next;
	if ((s->nins == 0 || s->nouts == 0) && !s->flag) {
	    dropstate(nfa, s);
	}
    }

    if (f != NULL) {
	dumpnfa(nfa, f);
    }
}

/*
 - emptyreachable - recursively find all states that can reach s by EMPTY arcs
 * The return value is the last such state found.  Its tmp field links back
 * to the next-to-last such state, and so on back to s, so that all these
 * states can be located without searching the whole NFA.
 *
 * Since this is only used in fixempties(), we pass in the inarcsorig[] array
 * maintained by that function.  This lets us skip over all new inarcs, which
 * are certainly not EMPTY arcs.
 *
 * The maximum recursion depth here is equal to the length of the longest
 * loop-free chain of EMPTY arcs, which is surely no more than the size of
 * the NFA, and in practice will be less than that.
 ^ static struct state *emptyreachable(struct state *, struct state *);
 */
static struct state *
emptyreachable(
    struct nfa *nfa,
    struct state *s,
    struct state *lastfound,
    struct arc **inarcsorig)
{
    struct arc *a;

    s->tmp = lastfound;
    lastfound = s;
    for (a = inarcsorig[s->no]; a != NULL; a = a->inchain) {
	if (a->type == EMPTY && a->from->tmp == NULL) {
	    lastfound = emptyreachable(nfa, a->from, lastfound, inarcsorig);
	}
    }
    return lastfound;
}

/*
 * isconstraintarc - detect whether an arc is of a constraint type
 */
static inline int
isconstraintarc(struct arc * a)
{
    switch (a->type)
    {
	case '^':
	case '$':
	case BEHIND:
	case AHEAD:
	case LACON:
	    return 1;
    }
    return 0;
}

/*
 * hasconstraintout - does state have a constraint out arc?
 */
static int
hasconstraintout(struct state * s)
{
    struct arc *a;

    for (a = s->outs; a != NULL; a = a->outchain) {
	if (isconstraintarc(a)) {
	    return 1;
	}
    }
    return 0;
}

/*
 * fixconstraintloops - get rid of loops containing only constraint arcs
 *
 * A loop of states that contains only constraint arcs is useless, since
 * passing around the loop represents no forward progress.  Moreover, it
 * would cause infinite looping in pullback/pushfwd, so we need to get rid
 * of such loops before doing that.
 */
static void
fixconstraintloops(
    struct nfa * nfa,
    FILE *f)		/* for debug output; NULL none */
{
    struct state *s;
    struct state *nexts;
    struct arc *a;
    struct arc *nexta;
    int hasconstraints;

    /*
     * In the trivial case of a state that loops to itself, we can just drop
     * the constraint arc altogether.  This is worth special-casing because
     * such loops are far more common than loops containing multiple states.
     * While we're at it, note whether any constraint arcs survive.
     */
    hasconstraints = 0;
    for (s = nfa->states; s != NULL && !NISERR(); s = nexts) {
	nexts = s->next;
	/* while we're at it, ensure tmp fields are clear for next step */
	assert(s->tmp == NULL);
	for (a = s->outs; a != NULL && !NISERR(); a = nexta) {
	    nexta = a->outchain;
	    if (isconstraintarc(a)) {
		if (a->to == s) {
		    freearc(nfa, a);
		} else {
		    hasconstraints = 1;
 		}
	    }
	}
 	/* If we removed all the outarcs, the state is useless. */
 	if (s->nouts == 0 && !s->flag) {
 	    dropstate(nfa, s);
	}
    }

    /* Nothing to do if no remaining constraint arcs */
    if (NISERR() || !hasconstraints) {
	return;
    }

    /*
     * Starting from each remaining NFA state, search outwards for a
     * constraint loop.  If we find a loop, break the loop, then start the
     * search over.  (We could possibly retain some state from the first scan,
     * but it would complicate things greatly, and multi-state constraint
     * loops are rare enough that it's not worth optimizing the case.)
     */
  restart:
    for (s = nfa->states; s != NULL && !NISERR(); s = s->next) {
	if (findconstraintloop(nfa, s)) {
	    goto restart;
	}
    }

    if (NISERR()) {
	return;
    }

    /*
     * Now remove any states that have become useless.  (This cleanup is not
     * very thorough, and would be even less so if we tried to combine it with
     * the previous step; but cleanup() will take care of anything we miss.)
     *
     * Because findconstraintloop intentionally doesn't reset all tmp fields,
     * we have to clear them after it's done.  This is a convenient place to
     * do that, too.
     */
    for (s = nfa->states; s != NULL; s = nexts) {
	nexts = s->next;
	s->tmp = NULL;
	if ((s->nins == 0 || s->nouts == 0) && !s->flag) {
	    dropstate(nfa, s);
	}
    }

    if (f != NULL) {
 	dumpnfa(nfa, f);
    }
}

/*
 * findconstraintloop - recursively find a loop of constraint arcs
 *
 * If we find a loop, break it by calling breakconstraintloop(), then
 * return 1; otherwise return 0.
 *
 * State tmp fields are guaranteed all NULL on a success return, because
 * breakconstraintloop does that.  After a failure return, any state that
 * is known not to be part of a loop is marked with s->tmp == s; this allows
 * us not to have to re-prove that fact on later calls.  (This convention is
 * workable because we already eliminated single-state loops.)
 *
 * Note that the found loop doesn't necessarily include the first state we
 * are called on.  Any loop reachable from that state will do.
 *
 * The maximum recursion depth here is one more than the length of the longest
 * loop-free chain of constraint arcs, which is surely no more than the size
 * of the NFA, and in practice will be a lot less than that.
 */
static int
findconstraintloop(struct nfa * nfa, struct state * s)
{
    struct arc *a;

    /* Since this is recursive, it could be driven to stack overflow */
    if (STACK_TOO_DEEP(nfa->v->re)) {
	NERR(REG_ETOOBIG);
	return 1;		/* to exit as quickly as possible */
    }

    if (s->tmp != NULL) {
	/* Already proven uninteresting? */
	if (s->tmp == s) {
	    return 0;
	}
	/* Found a loop involving s */
	breakconstraintloop(nfa, s);
	/* The tmp fields have been cleaned up by breakconstraintloop */
	return 1;
    }
    for (a = s->outs; a != NULL; a = a->outchain) {
	if (isconstraintarc(a)) {
	    struct state *sto = a->to;

	    assert(sto != s);
	    s->tmp = sto;
	    if (findconstraintloop(nfa, sto)) {
		return 1;
	    }
	}
    }

    /*
     * If we get here, no constraint loop exists leading out from s.  Mark it
     * with s->tmp == s so we need not rediscover that fact again later.
     */
    s->tmp = s;
    return 0;
}

/*
 * breakconstraintloop - break a loop of constraint arcs
 *
 * sinitial is any one member state of the loop.  Each loop member's tmp
 * field links to its successor within the loop.  (Note that this function
 * will reset all the tmp fields to NULL.)
 *
 * We can break the loop by, for any one state S1 in the loop, cloning its
 * loop successor state S2 (and possibly following states), and then moving
 * all S1->S2 constraint arcs to point to the cloned S2.  The cloned S2 should
 * copy any non-constraint outarcs of S2.  Constraint outarcs should be
 * dropped if they point back to S1, else they need to be copied as arcs to
 * similarly cloned states S3, S4, etc.  In general, each cloned state copies
 * non-constraint outarcs, drops constraint outarcs that would lead to itself
 * or any earlier cloned state, and sends other constraint outarcs to newly
 * cloned states.  No cloned state will have any inarcs that aren't constraint
 * arcs or do not lead from S1 or earlier-cloned states.  It's okay to drop
 * constraint back-arcs since they would not take us to any state we've not
 * already been in; therefore, no new constraint loop is created.  In this way
 * we generate a modified NFA that can still represent every useful state
 * sequence, but not sequences that represent state loops with no consumption
 * of input data.  Note that the set of cloned states will certainly include
 * all of the loop member states other than S1, and it may also include
 * non-loop states that are reachable from S2 via constraint arcs.  This is
 * important because there is no guarantee that findconstraintloop found a
 * maximal loop (and searching for one would be NP-hard, so don't try).
 * Frequently the "non-loop states" are actually part of a larger loop that
 * we didn't notice, and indeed there may be several overlapping loops.
 * This technique ensures convergence in such cases, while considering only
 * the originally-found loop does not.
 *
 * If there is only one S1->S2 constraint arc, then that constraint is
 * certainly satisfied when we enter any of the clone states.  This means that
 * in the common case where many of the constraint arcs are identically
 * labeled, we can merge together clone states linked by a similarly-labeled
 * constraint: if we can get to the first one we can certainly get to the
 * second, so there's no need to distinguish.  This greatly reduces the number
 * of new states needed, so we preferentially break the given loop at a state
 * pair where this is true.
 *
 * Furthermore, it's fairly common to find that a cloned successor state has
 * no outarcs, especially if we're a bit aggressive about removing unnecessary
 * outarcs.  If that happens, then there is simply not any interesting state
 * that can be reached through the predecessor's loop arcs, which means we can
 * break the loop just by removing those loop arcs, with no new states added.
 */
static void
breakconstraintloop(struct nfa * nfa, struct state * sinitial)
{
    struct state *s;
    struct state *shead;
    struct state *stail;
    struct state *sclone;
    struct state *nexts;
    struct arc *refarc;
    struct arc *a;
    struct arc *nexta;

    /*
     * Start by identifying which loop step we want to break at.
     * Preferentially this is one with only one constraint arc.  (XXX are
     * there any other secondary heuristics we want to use here?)  Set refarc
     * to point to the selected lone constraint arc, if there is one.
     */
    refarc = NULL;
    s = sinitial;
    do {
	nexts = s->tmp;
	assert(nexts != s);	/* should not see any one-element loops */
	if (refarc == NULL) {
	    int narcs = 0;

	    for (a = s->outs; a != NULL; a = a->outchain) {
		if (a->to == nexts && isconstraintarc(a)) {
		    refarc = a;
		    narcs++;
		}
	    }
	    assert(narcs > 0);
	    if (narcs > 1) {
		refarc = NULL;	/* multiple constraint arcs here, no good */
	    }
	}
	s = nexts;
    } while (s != sinitial);

    if (refarc) {
	/* break at the refarc */
	shead = refarc->from;
	stail = refarc->to;
	assert(stail == shead->tmp);
    } else {
	/* for lack of a better idea, break after sinitial */
	shead = sinitial;
	stail = sinitial->tmp;
    }

    /*
     * Reset the tmp fields so that we can use them for local storage in
     * clonesuccessorstates.  (findconstraintloop won't mind, since it's just
     * going to abandon its search anyway.)
     */
    for (s = nfa->states; s != NULL; s = s->next) {
	s->tmp = NULL;
    }

    /*
     * Recursively build clone state(s) as needed.
     */
    sclone = newstate(nfa);
    if (sclone == NULL) {
	assert(NISERR());
	return;
    }

    clonesuccessorstates(nfa, stail, sclone, shead, refarc,
	    NULL, NULL, nfa->nstates);

    if (NISERR()) {
	return;
    }

    /*
     * It's possible that sclone has no outarcs at all, in which case it's
     * useless.  (We don't try extremely hard to get rid of useless states
     * here, but this is an easy and fairly common case.)
     */
    if (sclone->nouts == 0) {
	freestate(nfa, sclone);
	sclone = NULL;
    }

    /*
     * Move shead's constraint-loop arcs to point to sclone, or just drop them
     * if we discovered we don't need sclone.
     */
    for (a = shead->outs; a != NULL; a = nexta) {
	nexta = a->outchain;
	if (a->to == stail && isconstraintarc(a)) {
	    if (sclone) {
		cparc(nfa, a, shead, sclone);
	    }
	    freearc(nfa, a);
	    if (NISERR()) {
		break;
	    }
	}
    }
}

/*
 * clonesuccessorstates - create a tree of constraint-arc successor states
 *
 * ssource is the state to be cloned, and sclone is the state to copy its
 * outarcs into.  sclone's inarcs, if any, should already be set up.
 *
 * spredecessor is the original predecessor state that we are trying to build
 * successors for (it may not be the immediate predecessor of ssource).
 * refarc, if not NULL, is the original constraint arc that is known to have
 * been traversed out of spredecessor to reach the successor(s).
 *
 * For each cloned successor state, we transiently create a "donemap" that is
 * a boolean array showing which source states we've already visited for this
 * clone state.  This prevents infinite recursion as well as useless repeat
 * visits to the same state subtree (which can add up fast, since typical NFAs
 * have multiple redundant arc pathways).  Each donemap is a char array
 * indexed by state number.  The donemaps are all of the same size "nstates",
 * which is nfa->nstates as of the start of the recursion.  This is enough to
 * have entries for all pre-existing states, but *not* entries for clone
 * states created during the recursion.  That's okay since we have no need to
 * mark those.
 *
 * curdonemap is NULL when recursing to a new sclone state, or sclone's
 * donemap when we are recursing without having created a new state (which we
 * do when we decide we can merge a successor state into the current clone
 * state).  outerdonemap is NULL at the top level and otherwise the parent
 * clone state's donemap.
 *
 * The successor states we create and fill here form a strict tree structure,
 * with each state having exactly one predecessor, except that the toplevel
 * state has no inarcs as yet (breakconstraintloop will add its inarcs from
 * spredecessor after we're done).  Thus, we can examine sclone's inarcs back
 * to the root, plus refarc if any, to identify the set of constraints already
 * known valid at the current point.  This allows us to avoid generating extra
 * successor states.
 */
static void
clonesuccessorstates(
    struct nfa * nfa,
    struct state * ssource,
    struct state * sclone,
    struct state * spredecessor,
    struct arc * refarc,
    char *curdonemap,
    char *outerdonemap,
    int nstates)
{
    char *donemap;
    struct arc *a;

    /* Since this is recursive, it could be driven to stack overflow */
    if (STACK_TOO_DEEP(nfa->v->re)) {
	NERR(REG_ETOOBIG);
	return;
    }

    /* If this state hasn't already got a donemap, create one */
    donemap = curdonemap;
    if (donemap == NULL) {
	donemap = (char *) MALLOC(nstates * sizeof(char));
	if (donemap == NULL) {
	    NERR(REG_ESPACE);
	    return;
	}

	if (outerdonemap != NULL) {
	    /*
	     * Not at outermost recursion level, so copy the outer level's
	     * donemap; this ensures that we see states in process of being
	     * visited at outer levels, or already merged into predecessor
	     * states, as ones we shouldn't traverse back to.
	     */
	    memcpy(donemap, outerdonemap, nstates * sizeof(char));
	} else {
	    /* At outermost level, only spredecessor is off-limits */
	    memset(donemap, 0, nstates * sizeof(char));
	    assert(spredecessor->no < nstates);
	    donemap[spredecessor->no] = 1;
	}
    }

    /* Mark ssource as visited in the donemap */
    assert(ssource->no < nstates);
    assert(donemap[ssource->no] == 0);
    donemap[ssource->no] = 1;

    /*
     * We proceed by first cloning all of ssource's outarcs, creating new
     * clone states as needed but not doing more with them than that.  Then in
     * a second pass, recurse to process the child clone states.  This allows
     * us to have only one child clone state per reachable source state, even
     * when there are multiple outarcs leading to the same state.  Also, when
     * we do visit a child state, its set of inarcs is known exactly, which
     * makes it safe to apply the constraint-is-already-checked optimization.
     * Also, this ensures that we've merged all the states we can into the
     * current clone before we recurse to any children, thus possibly saving
     * them from making extra images of those states.
     *
     * While this function runs, child clone states of the current state are
     * marked by setting their tmp fields to point to the original state they
     * were cloned from.  This makes it possible to detect multiple outarcs
     * leading to the same state, and also makes it easy to distinguish clone
     * states from original states (which will have tmp == NULL).
     */
    for (a = ssource->outs; a != NULL && !NISERR(); a = a->outchain) {
	struct state *sto = a->to;

	/*
	 * We do not consider cloning successor states that have no constraint
	 * outarcs; just link to them as-is.  They cannot be part of a
	 * constraint loop so there is no need to make copies.  In particular,
	 * this rule keeps us from trying to clone the post state, which would
	 * be a bad idea.
	 */
	if (isconstraintarc(a) && hasconstraintout(sto)) {
	    struct state *prevclone;
	    int canmerge;
	    struct arc *a2;

	    /*
	     * Back-link constraint arcs must not be followed.  Nor is there a
	     * need to revisit states previously merged into this clone.
	     */
	    assert(sto->no < nstates);
	    if (donemap[sto->no] != 0) {
		continue;
	    }

	    /*
	     * Check whether we already have a child clone state for this
	     * source state.
	     */
	    prevclone = NULL;
	    for (a2 = sclone->outs; a2 != NULL; a2 = a2->outchain) {
		if (a2->to->tmp == sto) {
		    prevclone = a2->to;
		    break;
		}
	    }

	    /*
	     * If this arc is labeled the same as refarc, or the same as any
	     * arc we must have traversed to get to sclone, then no additional
	     * constraints need to be met to get to sto, so we should just
	     * merge its outarcs into sclone.
	     */
	    if (refarc && a->type == refarc->type && a->co == refarc->co) {
		canmerge = 1;
	    } else {
		struct state *s;

		canmerge = 0;
		for (s = sclone; s->ins; s = s->ins->from) {
		    if (s->nins == 1 &&
			    a->type == s->ins->type && a->co == s->ins->co) {
			canmerge = 1;
			break;
		    }
		}
	    }

	    if (canmerge) {
		/*
		 * We can merge into sclone.  If we previously made a child
		 * clone state, drop it; there's no need to visit it.  (This
		 * can happen if ssource has multiple pathways to sto, and we
		 * only just now found one that is provably a no-op.)
		 */
		if (prevclone) {
		    dropstate(nfa, prevclone);	/* kills our outarc, too */
		}

		/* Recurse to merge sto's outarcs into sclone */
		clonesuccessorstates(nfa, sto, sclone, spredecessor, refarc,
			donemap, outerdonemap, nstates);
		/* sto should now be marked as previously visited */
		assert(NISERR() || donemap[sto->no] == 1);
	    } else if (prevclone) {
		/*
		 * We already have a clone state for this successor, so just
		 * make another arc to it.
		 */
		cparc(nfa, a, sclone, prevclone);
	    } else {
		/*
		 * We need to create a new successor clone state.
		 */
		struct state *stoclone;

		stoclone = newstate(nfa);
		if (stoclone == NULL) {
		    assert(NISERR());
		    break;
		}
		/* Mark it as to what it's a clone of */
		stoclone->tmp = sto;
		/* ... and add the outarc leading to it */
		cparc(nfa, a, sclone, stoclone);
	    }
	} else {
	    /*
	     * Non-constraint outarcs just get copied to sclone, as do outarcs
	     * leading to states with no constraint outarc.
	     */
	    cparc(nfa, a, sclone, sto);
	}
    }

    /*
     * If we are at outer level for this clone state, recurse to all its child
     * clone states, clearing their tmp fields as we go.  (If we're not
     * outermost for sclone, leave this to be done by the outer call level.)
     * Note that if we have multiple outarcs leading to the same clone state,
     * it will only be recursed-to once.
     */
    if (curdonemap == NULL) {
	for (a = sclone->outs; a != NULL && !NISERR(); a = a->outchain) {
	    struct state *stoclone = a->to;
	    struct state *sto = stoclone->tmp;

	    if (sto != NULL) {
		stoclone->tmp = NULL;
		clonesuccessorstates(nfa, sto, stoclone, spredecessor, refarc,
			NULL, donemap, nstates);
	    }
	}

	/* Don't forget to free sclone's donemap when done with it */
	FREE(donemap);
    }
}

/*
 - cleanup - clean up NFA after optimizations
 ^ static void cleanup(struct nfa *);
 */
static void
cleanup(
    struct nfa *nfa)
{
    struct state *s;
    struct state *nexts;
    int n;

    /*
     * Clear out unreachable or dead-end states. Use pre to mark reachable,
     * then post to mark can-reach-post.
     */

    markreachable(nfa, nfa->pre, NULL, nfa->pre);
    markcanreach(nfa, nfa->post, nfa->pre, nfa->post);
    for (s = nfa->states; s != NULL; s = nexts) {
	nexts = s->next;
	if (s->tmp != nfa->post && !s->flag) {
	    dropstate(nfa, s);
	}
    }
    assert(nfa->post->nins == 0 || nfa->post->tmp == nfa->post);
    cleartraverse(nfa, nfa->pre);
    assert(nfa->post->nins == 0 || nfa->post->tmp == NULL);
    /* the nins==0 (final unreachable) case will be caught later */

    /*
     * Renumber surviving states.
     */

    n = 0;
    for (s = nfa->states; s != NULL; s = s->next) {
	s->no = n++;
    }
    nfa->nstates = n;
}

/*
 - markreachable - recursive marking of reachable states
 ^ static void markreachable(struct nfa *, struct state *, struct state *,
 ^ 	struct state *);
 */
static void
markreachable(
    struct nfa *nfa,
    struct state *s,
    struct state *okay,		/* consider only states with this mark */
    struct state *mark)		/* the value to mark with */
{
    struct arc *a;

    if (s->tmp != okay) {
	return;
    }
    s->tmp = mark;

    for (a = s->outs; a != NULL; a = a->outchain) {
	markreachable(nfa, a->to, okay, mark);
    }
}

/*
 - markcanreach - recursive marking of states which can reach here
 ^ static void markcanreach(struct nfa *, struct state *, struct state *,
 ^ 	struct state *);
 */
static void
markcanreach(
    struct nfa *nfa,
    struct state *s,
    struct state *okay,		/* consider only states with this mark */
    struct state *mark)		/* the value to mark with */
{
    struct arc *a;

    if (s->tmp != okay) {
	return;
    }
    s->tmp = mark;

    for (a = s->ins; a != NULL; a = a->inchain) {
	markcanreach(nfa, a->from, okay, mark);
    }
}

/*
 - analyze - ascertain potentially-useful facts about an optimized NFA
 ^ static long analyze(struct nfa *);
 */
static long			/* re_info bits to be ORed in */
analyze(
    struct nfa *nfa)
{
    struct arc *a;
    struct arc *aa;

    if (nfa->pre->outs == NULL) {
	return REG_UIMPOSSIBLE;
    }
    for (a = nfa->pre->outs; a != NULL; a = a->outchain) {
	for (aa = a->to->outs; aa != NULL; aa = aa->outchain) {
	    if (aa->to == nfa->post) {
		return REG_UEMPTYMATCH;
	    }
	}
    }
    return 0;
}

/*
 - compact - construct the compact representation of an NFA
 ^ static void compact(struct nfa *, struct cnfa *);
 */
static void
compact(
    struct nfa *nfa,
    struct cnfa *cnfa)
{
    struct state *s;
    struct arc *a;
    size_t nstates;
    size_t narcs;
    struct carc *ca;
    struct carc *first;

    assert(!NISERR());

    nstates = 0;
    narcs = 0;
    for (s = nfa->states; s != NULL; s = s->next) {
	nstates++;
	narcs += s->nouts + 1;	/* need one extra for endmarker */
    }

    cnfa->stflags = (char *) MALLOC(nstates * sizeof(char));
    cnfa->states = (struct carc **) MALLOC(nstates * sizeof(struct carc *));
    cnfa->arcs = (struct carc *) MALLOC(narcs * sizeof(struct carc));
    if (cnfa->stflags == NULL || cnfa->states == NULL || cnfa->arcs == NULL) {
	if (cnfa->stflags != NULL) {
	    FREE(cnfa->stflags);
	}
	if (cnfa->states != NULL) {
	    FREE(cnfa->states);
	}
	if (cnfa->arcs != NULL) {
	    FREE(cnfa->arcs);
	}
	NERR(REG_ESPACE);
	return;
    }
    cnfa->nstates = nstates;
    cnfa->pre = nfa->pre->no;
    cnfa->post = nfa->post->no;
    cnfa->bos[0] = nfa->bos[0];
    cnfa->bos[1] = nfa->bos[1];
    cnfa->eos[0] = nfa->eos[0];
    cnfa->eos[1] = nfa->eos[1];
    cnfa->ncolors = maxcolor(nfa->cm) + 1;
    cnfa->flags = 0;

    ca = cnfa->arcs;
    for (s = nfa->states; s != NULL; s = s->next) {
	assert((size_t) s->no < nstates);
	cnfa->stflags[s->no] = 0;
	cnfa->states[s->no] = ca;
	first = ca;
	for (a = s->outs; a != NULL; a = a->outchain) {
	    switch (a->type) {
	    case PLAIN:
		ca->co = a->co;
		ca->to = a->to->no;
		ca++;
		break;
	    case LACON:
		assert(s->no != cnfa->pre);
		ca->co = (color) (cnfa->ncolors + a->co);
		ca->to = a->to->no;
		ca++;
		cnfa->flags |= HASLACONS;
		break;
	    default:
		NERR(REG_ASSERT);
		break;
	    }
	}
	carcsort(first, ca - first);
	ca->co = COLORLESS;
	ca->to = 0;
	ca++;
    }
    assert(ca == &cnfa->arcs[narcs]);
    assert(cnfa->nstates != 0);

    /*
     * Mark no-progress states.
     */

    for (a = nfa->pre->outs; a != NULL; a = a->outchain) {
	cnfa->stflags[a->to->no] = CNFA_NOPROGRESS;
    }
    cnfa->stflags[nfa->pre->no] = CNFA_NOPROGRESS;
}

/*
 - carcsort - sort compacted-NFA arcs by color
 ^ static void carcsort(struct carc *, struct carc *);
 */
static void
carcsort(
    struct carc *first,
    size_t n)
{
    if (n > 1) {
	qsort(first, n, sizeof(struct carc), carc_cmp);
    }
}

static int
carc_cmp(
    const void *a,
    const void *b)
{
    const struct carc *aa = (const struct carc *) a;
    const struct carc *bb = (const struct carc *) b;

    if (aa->co < bb->co) {
	return -1;
    }
    if (aa->co > bb->co) {
	return +1;
    }
    if (aa->to < bb->to) {
	return -1;
    }
    if (aa->to > bb->to) {
	return +1;
    }
    return 0;
}

/*
 - freecnfa - free a compacted NFA
 ^ static void freecnfa(struct cnfa *);
 */
static void
freecnfa(
    struct cnfa *cnfa)
{
    assert(cnfa->nstates != 0);	/* not empty already */
    cnfa->nstates = 0;
    FREE(cnfa->stflags);
    FREE(cnfa->states);
    FREE(cnfa->arcs);
}

/*
 - dumpnfa - dump an NFA in human-readable form
 ^ static void dumpnfa(struct nfa *, FILE *);
 */
static void
dumpnfa(
    struct nfa *nfa,
    FILE *f)
{
#ifdef REG_DEBUG
    struct state *s;
    int nstates = 0;
    int narcs = 0;

    fprintf(f, "pre %d, post %d", nfa->pre->no, nfa->post->no);
    if (nfa->bos[0] != COLORLESS) {
	fprintf(f, ", bos [%ld]", (long) nfa->bos[0]);
    }
    if (nfa->bos[1] != COLORLESS) {
	fprintf(f, ", bol [%ld]", (long) nfa->bos[1]);
    }
    if (nfa->eos[0] != COLORLESS) {
	fprintf(f, ", eos [%ld]", (long) nfa->eos[0]);
    }
    if (nfa->eos[1] != COLORLESS) {
	fprintf(f, ", eol [%ld]", (long) nfa->eos[1]);
    }
    fprintf(f, "\n");
    for (s = nfa->states; s != NULL; s = s->next) {
	dumpstate(s, f);
	nstates++;
	narcs += s->nouts;
    }
    fprintf(f, "total of %d states, %d arcs\n", nstates, narcs);
    if (nfa->parent == NULL) {
	dumpcolors(nfa->cm, f);
    }
    fflush(f);
#endif
}

#ifdef REG_DEBUG		/* subordinates of dumpnfa */
/*
 ^ #ifdef REG_DEBUG
 */

/*
 - dumpstate - dump an NFA state in human-readable form
 ^ static void dumpstate(struct state *, FILE *);
 */
static void
dumpstate(
    struct state *s,
    FILE *f)
{
    struct arc *a;

    fprintf(f, "%d%s%c", s->no, (s->tmp != NULL) ? "T" : "",
	    (s->flag) ? s->flag : '.');
    if (s->prev != NULL && s->prev->next != s) {
	fprintf(f, "\tstate chain bad\n");
    }
    if (s->nouts == 0) {
	fprintf(f, "\tno out arcs\n");
    } else {
	dumparcs(s, f);
    }
    fflush(f);
    for (a = s->ins; a != NULL; a = a->inchain) {
	if (a->to != s) {
	    fprintf(f, "\tlink from %d to %d on %d's in-chain\n",
		    a->from->no, a->to->no, s->no);
	}
    }
}

/*
 - dumparcs - dump out-arcs in human-readable form
 ^ static void dumparcs(struct state *, FILE *);
 */
static void
dumparcs(
    struct state *s,
    FILE *f)
{
    int pos;
    struct arc *a;

    /* printing oldest arcs first is usually clearer */
    a = s->outs;
    assert(a != NULL);
    while (a->outchain != NULL) {
	a = a->outchain;
    }
    pos = 1;
    do {
	dumparc(a, s, f);
	if (pos == 5) {
	    fprintf(f, "\n");
	    pos = 1;
	} else {
	    pos++;
	}
	a = a->outchainRev;
    } while (a != NULL);
    if (pos != 1) {
	fprintf(f, "\n");
    }
}

/*
 - dumparc - dump one outarc in readable form, including prefixing tab
 ^ static void dumparc(struct arc *, struct state *, FILE *);
 */
static void
dumparc(
    struct arc *a,
    struct state *s,
    FILE *f)
{
    struct arc *aa;
    struct arcbatch *ab;

    fprintf(f, "\t");
    switch (a->type) {
    case PLAIN:
	fprintf(f, "[%ld]", (long) a->co);
	break;
    case AHEAD:
	fprintf(f, ">%ld>", (long) a->co);
	break;
    case BEHIND:
	fprintf(f, "<%ld<", (long) a->co);
	break;
    case LACON:
	fprintf(f, ":%ld:", (long) a->co);
	break;
    case '^':
    case '$':
	fprintf(f, "%c%d", a->type, (int) a->co);
	break;
    case EMPTY:
	break;
    default:
	fprintf(f, "0x%x/0%lo", a->type, (long) a->co);
	break;
    }
    if (a->from != s) {
	fprintf(f, "?%d?", a->from->no);
    }
    for (ab = &a->from->oas; ab != NULL; ab = ab->next) {
	for (aa = &ab->a[0]; aa < &ab->a[ABSIZE]; aa++) {
	    if (aa == a) {
		break;		/* NOTE BREAK OUT */
	    }
	}
	if (aa < &ab->a[ABSIZE]) {	/* propagate break */
	    break;		/* NOTE BREAK OUT */
	}
    }
    if (ab == NULL) {
	fprintf(f, "?!?");	/* not in allocated space */
    }
    fprintf(f, "->");
    if (a->to == NULL) {
	fprintf(f, "NULL");
	return;
    }
    fprintf(f, "%d", a->to->no);
    for (aa = a->to->ins; aa != NULL; aa = aa->inchain) {
	if (aa == a) {
	    break;		/* NOTE BREAK OUT */
	}
    }
    if (aa == NULL) {
	fprintf(f, "?!?");	/* missing from in-chain */
    }
}

/*
 ^ #endif
 */
#endif				/* ifdef REG_DEBUG */

/*
 - dumpcnfa - dump a compacted NFA in human-readable form
 ^ static void dumpcnfa(struct cnfa *, FILE *);
 */
static void
dumpcnfa(
    struct cnfa *cnfa,
    FILE *f)
{
#ifdef REG_DEBUG
    int st;

    fprintf(f, "pre %d, post %d", cnfa->pre, cnfa->post);
    if (cnfa->bos[0] != COLORLESS) {
	fprintf(f, ", bos [%ld]", (long) cnfa->bos[0]);
    }
    if (cnfa->bos[1] != COLORLESS) {
	fprintf(f, ", bol [%ld]", (long) cnfa->bos[1]);
    }
    if (cnfa->eos[0] != COLORLESS) {
	fprintf(f, ", eos [%ld]", (long) cnfa->eos[0]);
    }
    if (cnfa->eos[1] != COLORLESS) {
	fprintf(f, ", eol [%ld]", (long) cnfa->eos[1]);
    }
    if (cnfa->flags&HASLACONS) {
	fprintf(f, ", haslacons");
    }
    fprintf(f, "\n");
    for (st = 0; st < cnfa->nstates; st++) {
	dumpcstate(st, cnfa, f);
    }
    fflush(f);
#endif
}

#ifdef REG_DEBUG		/* subordinates of dumpcnfa */
/*
 ^ #ifdef REG_DEBUG
 */

/*
 - dumpcstate - dump a compacted-NFA state in human-readable form
 ^ static void dumpcstate(int, struct cnfa *, FILE *);
 */
static void
dumpcstate(
    int st,
    struct cnfa *cnfa,
    FILE *f)
{
    struct carc *ca;
    int pos;

    fprintf(f, "%d%s", st, (cnfa->stflags[st] & CNFA_NOPROGRESS) ? ":" : ".");
    pos = 1;
    for (ca = cnfa->states[st]; ca->co != COLORLESS; ca++) {
	if (ca->co < cnfa->ncolors) {
	    fprintf(f, "\t[%ld]->%d", (long) ca->co, ca->to);
	} else {
	    fprintf(f, "\t:%ld:->%d", (long) (ca->co - cnfa->ncolors), ca->to);
	}
	if (pos == 5) {
	    fprintf(f, "\n");
	    pos = 1;
	} else {
	    pos++;
	}
    }
    if (ca == cnfa->states[st] || pos != 1) {
	fprintf(f, "\n");
    }
    fflush(f);
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
