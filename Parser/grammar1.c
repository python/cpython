/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Grammar subroutines needed by parser */

#include "pgenheaders.h"
#include "assert.h"
#include "grammar.h"
#include "token.h"

/* Return the DFA for the given type */

dfa *
PyGrammar_FindDFA(g, type)
	grammar *g;
	register int type;
{
	register dfa *d;
#if 1
	/* Massive speed-up */
	d = &g->g_dfa[type - NT_OFFSET];
	assert(d->d_type == type);
	return d;
#else
	/* Old, slow version */
	register int i;
	
	for (i = g->g_ndfas, d = g->g_dfa; --i >= 0; d++) {
		if (d->d_type == type)
			return d;
	}
	assert(0);
	/* NOTREACHED */
#endif
}

char *
PyGrammar_LabelRepr(lb)
	label *lb;
{
	static char buf[100];
	
	if (lb->lb_type == ENDMARKER)
		return "EMPTY";
	else if (ISNONTERMINAL(lb->lb_type)) {
		if (lb->lb_str == NULL) {
			sprintf(buf, "NT%d", lb->lb_type);
			return buf;
		}
		else
			return lb->lb_str;
	}
	else {
		if (lb->lb_str == NULL)
			return _PyParser_TokenNames[lb->lb_type];
		else {
			sprintf(buf, "%.32s(%.32s)",
				_PyParser_TokenNames[lb->lb_type], lb->lb_str);
			return buf;
		}
	}
}
