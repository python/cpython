/***********************************************************
Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
******************************************************************/

/* List a node on a file */

#include "pgenheaders.h"
#include "token.h"
#include "node.h"

/* Forward */
static void list1node(FILE *, node *);
static void listnode(FILE *, node *);

void
PyNode_ListTree(n)
	node *n;
{
	listnode(stdout, n);
}

static int level, atbol;

static void
listnode(fp, n)
	FILE *fp;
	node *n;
{
	level = 0;
	atbol = 1;
	list1node(fp, n);
}

static void
list1node(fp, n)
	FILE *fp;
	node *n;
{
	if (n == 0)
		return;
	if (ISNONTERMINAL(TYPE(n))) {
		int i;
		for (i = 0; i < NCH(n); i++)
			list1node(fp, CHILD(n, i));
	}
	else if (ISTERMINAL(TYPE(n))) {
		switch (TYPE(n)) {
		case INDENT:
			++level;
			break;
		case DEDENT:
			--level;
			break;
		default:
			if (atbol) {
				int i;
				for (i = 0; i < level; ++i)
					fprintf(fp, "\t");
				atbol = 0;
			}
			if (TYPE(n) == NEWLINE) {
				if (STR(n) != NULL)
					fprintf(fp, "%s", STR(n));
				fprintf(fp, "\n");
				atbol = 1;
			}
			else
				fprintf(fp, "%s ", STR(n));
			break;
		}
	}
	else
		fprintf(fp, "? ");
}
