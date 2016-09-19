/* Parser generator */

/* For a description, see the comments at end of this file */

#include "Python.h"
#include "pgenheaders.h"
#include "token.h"
#include "node.h"
#include "grammar.h"
#include "metagrammar.h"
#include "pgen.h"

extern int Py_DebugFlag;
extern int Py_IgnoreEnvironmentFlag; /* needed by Py_GETENV */


/* PART ONE -- CONSTRUCT NFA -- Cf. Algorithm 3.2 from [Aho&Ullman 77] */

typedef struct _nfaarc {
    int         ar_label;
    int         ar_arrow;
} nfaarc;

typedef struct _nfastate {
    int         st_narcs;
    nfaarc      *st_arc;
} nfastate;

typedef struct _nfa {
    int                 nf_type;
    char                *nf_name;
    int                 nf_nstates;
    nfastate            *nf_state;
    int                 nf_start, nf_finish;
} nfa;

/* Forward */
static void compile_rhs(labellist *ll,
                        nfa *nf, node *n, int *pa, int *pb);
static void compile_alt(labellist *ll,
                        nfa *nf, node *n, int *pa, int *pb);
static void compile_item(labellist *ll,
                         nfa *nf, node *n, int *pa, int *pb);
static void compile_atom(labellist *ll,
                         nfa *nf, node *n, int *pa, int *pb);

static int
addnfastate(nfa *nf)
{
    nfastate *st;

    nf->nf_state = (nfastate *)PyObject_REALLOC(nf->nf_state,
                                sizeof(nfastate) * (nf->nf_nstates + 1));
    if (nf->nf_state == NULL)
        Py_FatalError("out of mem");
    st = &nf->nf_state[nf->nf_nstates++];
    st->st_narcs = 0;
    st->st_arc = NULL;
    return st - nf->nf_state;
}

static void
addnfaarc(nfa *nf, int from, int to, int lbl)
{
    nfastate *st;
    nfaarc *ar;

    st = &nf->nf_state[from];
    st->st_arc = (nfaarc *)PyObject_REALLOC(st->st_arc,
                                  sizeof(nfaarc) * (st->st_narcs + 1));
    if (st->st_arc == NULL)
        Py_FatalError("out of mem");
    ar = &st->st_arc[st->st_narcs++];
    ar->ar_label = lbl;
    ar->ar_arrow = to;
}

static nfa *
newnfa(char *name)
{
    nfa *nf;
    static int type = NT_OFFSET; /* All types will be disjunct */

    nf = (nfa *)PyObject_MALLOC(sizeof(nfa));
    if (nf == NULL)
        Py_FatalError("no mem for new nfa");
    nf->nf_type = type++;
    nf->nf_name = name; /* XXX strdup(name) ??? */
    nf->nf_nstates = 0;
    nf->nf_state = NULL;
    nf->nf_start = nf->nf_finish = -1;
    return nf;
}

typedef struct _nfagrammar {
    int                 gr_nnfas;
    nfa                 **gr_nfa;
    labellist           gr_ll;
} nfagrammar;

/* Forward */
static void compile_rule(nfagrammar *gr, node *n);

static nfagrammar *
newnfagrammar(void)
{
    nfagrammar *gr;

    gr = (nfagrammar *)PyObject_MALLOC(sizeof(nfagrammar));
    if (gr == NULL)
        Py_FatalError("no mem for new nfa grammar");
    gr->gr_nnfas = 0;
    gr->gr_nfa = NULL;
    gr->gr_ll.ll_nlabels = 0;
    gr->gr_ll.ll_label = NULL;
    addlabel(&gr->gr_ll, ENDMARKER, "EMPTY");
    return gr;
}

static void
freenfagrammar(nfagrammar *gr)
{
    for (int i = 0; i < gr->gr_nnfas; i++) {
        PyObject_FREE(gr->gr_nfa[i]->nf_state);
    }
    PyObject_FREE(gr->gr_nfa);
    PyObject_FREE(gr);
}

static nfa *
addnfa(nfagrammar *gr, char *name)
{
    nfa *nf;

    nf = newnfa(name);
    gr->gr_nfa = (nfa **)PyObject_REALLOC(gr->gr_nfa,
                                  sizeof(nfa*) * (gr->gr_nnfas + 1));
    if (gr->gr_nfa == NULL)
        Py_FatalError("out of mem");
    gr->gr_nfa[gr->gr_nnfas++] = nf;
    addlabel(&gr->gr_ll, NAME, nf->nf_name);
    return nf;
}

#ifdef Py_DEBUG

static const char REQNFMT[] = "metacompile: less than %d children\n";

#define REQN(i, count) do { \
    if (i < count) { \
        fprintf(stderr, REQNFMT, count); \
        Py_FatalError("REQN"); \
    } \
} while (0)

#else
#define REQN(i, count)  /* empty */
#endif

static nfagrammar *
metacompile(node *n)
{
    nfagrammar *gr;
    int i;

    if (Py_DebugFlag)
        printf("Compiling (meta-) parse tree into NFA grammar\n");
    gr = newnfagrammar();
    REQ(n, MSTART);
    i = n->n_nchildren - 1; /* Last child is ENDMARKER */
    n = n->n_child;
    for (; --i >= 0; n++) {
        if (n->n_type != NEWLINE)
            compile_rule(gr, n);
    }
    return gr;
}

static void
compile_rule(nfagrammar *gr, node *n)
{
    nfa *nf;

    REQ(n, RULE);
    REQN(n->n_nchildren, 4);
    n = n->n_child;
    REQ(n, NAME);
    nf = addnfa(gr, n->n_str);
    n++;
    REQ(n, COLON);
    n++;
    REQ(n, RHS);
    compile_rhs(&gr->gr_ll, nf, n, &nf->nf_start, &nf->nf_finish);
    n++;
    REQ(n, NEWLINE);
}

static void
compile_rhs(labellist *ll, nfa *nf, node *n, int *pa, int *pb)
{
    int i;
    int a, b;

    REQ(n, RHS);
    i = n->n_nchildren;
    REQN(i, 1);
    n = n->n_child;
    REQ(n, ALT);
    compile_alt(ll, nf, n, pa, pb);
    if (--i <= 0)
        return;
    n++;
    a = *pa;
    b = *pb;
    *pa = addnfastate(nf);
    *pb = addnfastate(nf);
    addnfaarc(nf, *pa, a, EMPTY);
    addnfaarc(nf, b, *pb, EMPTY);
    for (; --i >= 0; n++) {
        REQ(n, VBAR);
        REQN(i, 1);
        --i;
        n++;
        REQ(n, ALT);
        compile_alt(ll, nf, n, &a, &b);
        addnfaarc(nf, *pa, a, EMPTY);
        addnfaarc(nf, b, *pb, EMPTY);
    }
}

static void
compile_alt(labellist *ll, nfa *nf, node *n, int *pa, int *pb)
{
    int i;
    int a, b;

    REQ(n, ALT);
    i = n->n_nchildren;
    REQN(i, 1);
    n = n->n_child;
    REQ(n, ITEM);
    compile_item(ll, nf, n, pa, pb);
    --i;
    n++;
    for (; --i >= 0; n++) {
        REQ(n, ITEM);
        compile_item(ll, nf, n, &a, &b);
        addnfaarc(nf, *pb, a, EMPTY);
        *pb = b;
    }
}

static void
compile_item(labellist *ll, nfa *nf, node *n, int *pa, int *pb)
{
    int i;
    int a, b;

    REQ(n, ITEM);
    i = n->n_nchildren;
    REQN(i, 1);
    n = n->n_child;
    if (n->n_type == LSQB) {
        REQN(i, 3);
        n++;
        REQ(n, RHS);
        *pa = addnfastate(nf);
        *pb = addnfastate(nf);
        addnfaarc(nf, *pa, *pb, EMPTY);
        compile_rhs(ll, nf, n, &a, &b);
        addnfaarc(nf, *pa, a, EMPTY);
        addnfaarc(nf, b, *pb, EMPTY);
        REQN(i, 1);
        n++;
        REQ(n, RSQB);
    }
    else {
        compile_atom(ll, nf, n, pa, pb);
        if (--i <= 0)
            return;
        n++;
        addnfaarc(nf, *pb, *pa, EMPTY);
        if (n->n_type == STAR)
            *pb = *pa;
        else
            REQ(n, PLUS);
    }
}

static void
compile_atom(labellist *ll, nfa *nf, node *n, int *pa, int *pb)
{
    int i;

    REQ(n, ATOM);
    i = n->n_nchildren;
    (void)i; /* Don't warn about set but unused */
    REQN(i, 1);
    n = n->n_child;
    if (n->n_type == LPAR) {
        REQN(i, 3);
        n++;
        REQ(n, RHS);
        compile_rhs(ll, nf, n, pa, pb);
        n++;
        REQ(n, RPAR);
    }
    else if (n->n_type == NAME || n->n_type == STRING) {
        *pa = addnfastate(nf);
        *pb = addnfastate(nf);
        addnfaarc(nf, *pa, *pb, addlabel(ll, n->n_type, n->n_str));
    }
    else
        REQ(n, NAME);
}

static void
dumpstate(labellist *ll, nfa *nf, int istate)
{
    nfastate *st;
    int i;
    nfaarc *ar;

    printf("%c%2d%c",
        istate == nf->nf_start ? '*' : ' ',
        istate,
        istate == nf->nf_finish ? '.' : ' ');
    st = &nf->nf_state[istate];
    ar = st->st_arc;
    for (i = 0; i < st->st_narcs; i++) {
        if (i > 0)
            printf("\n    ");
        printf("-> %2d  %s", ar->ar_arrow,
            PyGrammar_LabelRepr(&ll->ll_label[ar->ar_label]));
        ar++;
    }
    printf("\n");
}

static void
dumpnfa(labellist *ll, nfa *nf)
{
    int i;

    printf("NFA '%s' has %d states; start %d, finish %d\n",
        nf->nf_name, nf->nf_nstates, nf->nf_start, nf->nf_finish);
    for (i = 0; i < nf->nf_nstates; i++)
        dumpstate(ll, nf, i);
}


/* PART TWO -- CONSTRUCT DFA -- Algorithm 3.1 from [Aho&Ullman 77] */

static void
addclosure(bitset ss, nfa *nf, int istate)
{
    if (addbit(ss, istate)) {
        nfastate *st = &nf->nf_state[istate];
        nfaarc *ar = st->st_arc;
        int i;

        for (i = st->st_narcs; --i >= 0; ) {
            if (ar->ar_label == EMPTY)
                addclosure(ss, nf, ar->ar_arrow);
            ar++;
        }
    }
}

typedef struct _ss_arc {
    bitset      sa_bitset;
    int         sa_arrow;
    int         sa_label;
} ss_arc;

typedef struct _ss_state {
    bitset      ss_ss;
    int         ss_narcs;
    struct _ss_arc      *ss_arc;
    int         ss_deleted;
    int         ss_finish;
    int         ss_rename;
} ss_state;

typedef struct _ss_dfa {
    int         sd_nstates;
    ss_state *sd_state;
} ss_dfa;

/* Forward */
static void printssdfa(int xx_nstates, ss_state *xx_state, int nbits,
                       labellist *ll, const char *msg);
static void simplify(int xx_nstates, ss_state *xx_state);
static void convert(dfa *d, int xx_nstates, ss_state *xx_state);

static void
makedfa(nfagrammar *gr, nfa *nf, dfa *d)
{
    int nbits = nf->nf_nstates;
    bitset ss;
    int xx_nstates;
    ss_state *xx_state, *yy;
    ss_arc *zz;
    int istate, jstate, iarc, jarc, ibit;
    nfastate *st;
    nfaarc *ar;

    ss = newbitset(nbits);
    addclosure(ss, nf, nf->nf_start);
    xx_state = (ss_state *)PyObject_MALLOC(sizeof(ss_state));
    if (xx_state == NULL)
        Py_FatalError("no mem for xx_state in makedfa");
    xx_nstates = 1;
    yy = &xx_state[0];
    yy->ss_ss = ss;
    yy->ss_narcs = 0;
    yy->ss_arc = NULL;
    yy->ss_deleted = 0;
    yy->ss_finish = testbit(ss, nf->nf_finish);
    if (yy->ss_finish)
        printf("Error: nonterminal '%s' may produce empty.\n",
            nf->nf_name);

    /* This algorithm is from a book written before
       the invention of structured programming... */

    /* For each unmarked state... */
    for (istate = 0; istate < xx_nstates; ++istate) {
        size_t size;
        yy = &xx_state[istate];
        ss = yy->ss_ss;
        /* For all its states... */
        for (ibit = 0; ibit < nf->nf_nstates; ++ibit) {
            if (!testbit(ss, ibit))
                continue;
            st = &nf->nf_state[ibit];
            /* For all non-empty arcs from this state... */
            for (iarc = 0; iarc < st->st_narcs; iarc++) {
                ar = &st->st_arc[iarc];
                if (ar->ar_label == EMPTY)
                    continue;
                /* Look up in list of arcs from this state */
                for (jarc = 0; jarc < yy->ss_narcs; ++jarc) {
                    zz = &yy->ss_arc[jarc];
                    if (ar->ar_label == zz->sa_label)
                        goto found;
                }
                /* Add new arc for this state */
                size = sizeof(ss_arc) * (yy->ss_narcs + 1);
                yy->ss_arc = (ss_arc *)PyObject_REALLOC(
                                            yy->ss_arc, size);
                if (yy->ss_arc == NULL)
                    Py_FatalError("out of mem");
                zz = &yy->ss_arc[yy->ss_narcs++];
                zz->sa_label = ar->ar_label;
                zz->sa_bitset = newbitset(nbits);
                zz->sa_arrow = -1;
             found:             ;
                /* Add destination */
                addclosure(zz->sa_bitset, nf, ar->ar_arrow);
            }
        }
        /* Now look up all the arrow states */
        for (jarc = 0; jarc < xx_state[istate].ss_narcs; jarc++) {
            zz = &xx_state[istate].ss_arc[jarc];
            for (jstate = 0; jstate < xx_nstates; jstate++) {
                if (samebitset(zz->sa_bitset,
                    xx_state[jstate].ss_ss, nbits)) {
                    zz->sa_arrow = jstate;
                    goto done;
                }
            }
            size = sizeof(ss_state) * (xx_nstates + 1);
            xx_state = (ss_state *)PyObject_REALLOC(xx_state,
                                                        size);
            if (xx_state == NULL)
                Py_FatalError("out of mem");
            zz->sa_arrow = xx_nstates;
            yy = &xx_state[xx_nstates++];
            yy->ss_ss = zz->sa_bitset;
            yy->ss_narcs = 0;
            yy->ss_arc = NULL;
            yy->ss_deleted = 0;
            yy->ss_finish = testbit(yy->ss_ss, nf->nf_finish);
         done:          ;
        }
    }

    if (Py_DebugFlag)
        printssdfa(xx_nstates, xx_state, nbits, &gr->gr_ll,
                                        "before minimizing");

    simplify(xx_nstates, xx_state);

    if (Py_DebugFlag)
        printssdfa(xx_nstates, xx_state, nbits, &gr->gr_ll,
                                        "after minimizing");

    convert(d, xx_nstates, xx_state);

    for (int i = 0; i < xx_nstates; i++) {
        for (int j = 0; j < xx_state[i].ss_narcs; j++)
            delbitset(xx_state[i].ss_arc[j].sa_bitset);
        PyObject_FREE(xx_state[i].ss_arc);
    }
    PyObject_FREE(xx_state);
}

static void
printssdfa(int xx_nstates, ss_state *xx_state, int nbits,
           labellist *ll, const char *msg)
{
    int i, ibit, iarc;
    ss_state *yy;
    ss_arc *zz;

    printf("Subset DFA %s\n", msg);
    for (i = 0; i < xx_nstates; i++) {
        yy = &xx_state[i];
        if (yy->ss_deleted)
            continue;
        printf(" Subset %d", i);
        if (yy->ss_finish)
            printf(" (finish)");
        printf(" { ");
        for (ibit = 0; ibit < nbits; ibit++) {
            if (testbit(yy->ss_ss, ibit))
                printf("%d ", ibit);
        }
        printf("}\n");
        for (iarc = 0; iarc < yy->ss_narcs; iarc++) {
            zz = &yy->ss_arc[iarc];
            printf("  Arc to state %d, label %s\n",
                zz->sa_arrow,
                PyGrammar_LabelRepr(
                    &ll->ll_label[zz->sa_label]));
        }
    }
}


/* PART THREE -- SIMPLIFY DFA */

/* Simplify the DFA by repeatedly eliminating states that are
   equivalent to another oner.  This is NOT Algorithm 3.3 from
   [Aho&Ullman 77].  It does not always finds the minimal DFA,
   but it does usually make a much smaller one...  (For an example
   of sub-optimal behavior, try S: x a b+ | y a b+.)
*/

static int
samestate(ss_state *s1, ss_state *s2)
{
    int i;

    if (s1->ss_narcs != s2->ss_narcs || s1->ss_finish != s2->ss_finish)
        return 0;
    for (i = 0; i < s1->ss_narcs; i++) {
        if (s1->ss_arc[i].sa_arrow != s2->ss_arc[i].sa_arrow ||
            s1->ss_arc[i].sa_label != s2->ss_arc[i].sa_label)
            return 0;
    }
    return 1;
}

static void
renamestates(int xx_nstates, ss_state *xx_state, int from, int to)
{
    int i, j;

    if (Py_DebugFlag)
        printf("Rename state %d to %d.\n", from, to);
    for (i = 0; i < xx_nstates; i++) {
        if (xx_state[i].ss_deleted)
            continue;
        for (j = 0; j < xx_state[i].ss_narcs; j++) {
            if (xx_state[i].ss_arc[j].sa_arrow == from)
                xx_state[i].ss_arc[j].sa_arrow = to;
        }
    }
}

static void
simplify(int xx_nstates, ss_state *xx_state)
{
    int changes;
    int i, j;

    do {
        changes = 0;
        for (i = 1; i < xx_nstates; i++) {
            if (xx_state[i].ss_deleted)
                continue;
            for (j = 0; j < i; j++) {
                if (xx_state[j].ss_deleted)
                    continue;
                if (samestate(&xx_state[i], &xx_state[j])) {
                    xx_state[i].ss_deleted++;
                    renamestates(xx_nstates, xx_state,
                                 i, j);
                    changes++;
                    break;
                }
            }
        }
    } while (changes);
}


/* PART FOUR -- GENERATE PARSING TABLES */

/* Convert the DFA into a grammar that can be used by our parser */

static void
convert(dfa *d, int xx_nstates, ss_state *xx_state)
{
    int i, j;
    ss_state *yy;
    ss_arc *zz;

    for (i = 0; i < xx_nstates; i++) {
        yy = &xx_state[i];
        if (yy->ss_deleted)
            continue;
        yy->ss_rename = addstate(d);
    }

    for (i = 0; i < xx_nstates; i++) {
        yy = &xx_state[i];
        if (yy->ss_deleted)
            continue;
        for (j = 0; j < yy->ss_narcs; j++) {
            zz = &yy->ss_arc[j];
            addarc(d, yy->ss_rename,
                xx_state[zz->sa_arrow].ss_rename,
                zz->sa_label);
        }
        if (yy->ss_finish)
            addarc(d, yy->ss_rename, yy->ss_rename, 0);
    }

    d->d_initial = 0;
}


/* PART FIVE -- GLUE IT ALL TOGETHER */

static grammar *
maketables(nfagrammar *gr)
{
    int i;
    nfa *nf;
    dfa *d;
    grammar *g;

    if (gr->gr_nnfas == 0)
        return NULL;
    g = newgrammar(gr->gr_nfa[0]->nf_type);
                    /* XXX first rule must be start rule */
    g->g_ll = gr->gr_ll;

    for (i = 0; i < gr->gr_nnfas; i++) {
        nf = gr->gr_nfa[i];
        if (Py_DebugFlag) {
            printf("Dump of NFA for '%s' ...\n", nf->nf_name);
            dumpnfa(&gr->gr_ll, nf);
            printf("Making DFA for '%s' ...\n", nf->nf_name);
        }
        d = adddfa(g, nf->nf_type, nf->nf_name);
        makedfa(gr, gr->gr_nfa[i], d);
    }

    return g;
}

grammar *
pgen(node *n)
{
    nfagrammar *gr;
    grammar *g;

    gr = metacompile(n);
    g = maketables(gr);
    translatelabels(g);
    addfirstsets(g);
    freenfagrammar(gr);
    return g;
}

grammar *
Py_pgen(node *n)
{
  return pgen(n);
}

/*

Description
-----------

Input is a grammar in extended BNF (using * for repetition, + for
at-least-once repetition, [] for optional parts, | for alternatives and
() for grouping).  This has already been parsed and turned into a parse
tree.

Each rule is considered as a regular expression in its own right.
It is turned into a Non-deterministic Finite Automaton (NFA), which
is then turned into a Deterministic Finite Automaton (DFA), which is then
optimized to reduce the number of states.  See [Aho&Ullman 77] chapter 3,
or similar compiler books (this technique is more often used for lexical
analyzers).

The DFA's are used by the parser as parsing tables in a special way
that's probably unique.  Before they are usable, the FIRST sets of all
non-terminals are computed.

Reference
---------

[Aho&Ullman 77]
    Aho&Ullman, Principles of Compiler Design, Addison-Wesley 1977
    (first edition)

*/
