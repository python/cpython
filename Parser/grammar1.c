
/* Grammar subroutines needed by parser */

#include "Python.h"
#include "pgenheaders.h"
#include "grammar.h"
#include "token.h"

/* Return the DFA for the given type */

dfa *
PyGrammar_FindDFA(grammar *g, int type)
{
    dfa *d;
#if 1
    /* Massive speed-up */
    d = &g->g_dfa[type - NT_OFFSET];
    assert(d->d_type == type);
    return d;
#else
    /* Old, slow version */
    int i;

    for (i = g->g_ndfas, d = g->g_dfa; --i >= 0; d++) {
        if (d->d_type == type)
            return d;
    }
    Py_UNREACHABLE();
#endif
}

const char *
PyGrammar_LabelRepr(label *lb)
{
    static char buf[100];

    if (lb->lb_type == ENDMARKER)
        return "EMPTY";
    else if (ISNONTERMINAL(lb->lb_type)) {
        if (lb->lb_str == NULL) {
            PyOS_snprintf(buf, sizeof(buf), "NT%d", lb->lb_type);
            return buf;
        }
        else
            return lb->lb_str;
    }
    else if (lb->lb_type < N_TOKENS) {
        if (lb->lb_str == NULL)
            return _PyParser_TokenNames[lb->lb_type];
        else {
            PyOS_snprintf(buf, sizeof(buf), "%.32s(%.32s)",
                _PyParser_TokenNames[lb->lb_type], lb->lb_str);
            return buf;
        }
    }
    else {
        Py_FatalError("invalid label");
        return NULL;
    }
}
