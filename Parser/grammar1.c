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

/* Grammar subroutines needed by parser */

#include "pgenheaders.h"
#include "assert.h"
#include "grammar.h"
#include "token.h"

/* Return the DFA for the given type */

dfa *
finddfa(g, type)
	grammar *g;
	register int type;
{
	register int i;
	register dfa *d;
	
	for (i = g->g_ndfas, d = g->g_dfa; --i >= 0; d++) {
		if (d->d_type == type)
			return d;
	}
	assert(0);
	/* NOTREACHED */
}

char *
labelrepr(lb)
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
			return tok_name[lb->lb_type];
		else {
			sprintf(buf, "%.32s(%.32s)",
				tok_name[lb->lb_type], lb->lb_str);
			return buf;
		}
	}
}
