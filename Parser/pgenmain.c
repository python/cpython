/* Parser generator main program */

#include <stdio.h>

#include "PROTO.h"
#include "grammar.h"
#include "node.h"
#include "parsetok.h"
#include "pgen.h"

int debugging;

#ifdef THINK_C
char *
askfile()
{
	char buf[256];
	static char name[256];
	printf("Input file name: ");
	if (fgets(buf, sizeof buf, stdin) == NULL) {
		printf("EOF\n");
		exit(1);
	}
	if (sscanf(buf, " %s ", name) != 1) {
		printf("No file\n");
		exit(1);
	}
	return name;
}
#endif

grammar *
getgrammar(filename)
	char *filename;
{
	FILE *fp;
	node *n;
	grammar *g0, *g;
	
	fp = fopen(filename, "r");
	if (fp == NULL) {
		perror(filename);
		exit(1);
	}
	g0 = meta_grammar();
	n = NULL;
	parsefile(fp, g0, g0->g_start, (char *)NULL, (char *)NULL, &n);
	fclose(fp);
	if (n == NULL) {
		fprintf(stderr, "Parsing error.\n");
		exit(1);
	}
	g = pgen(n);
	if (g == NULL) {
		printf("Bad grammar.\n");
		exit(1);
	}
	return g;
}

main(argc, argv)
	int argc;
	char **argv;
{
	grammar *g;
	node *n;
	FILE *fp;
	char *filename;
	
#ifdef THINK_C
	filename = askfile();
#else
	if (argc != 2) {
		fprintf(stderr, "usage: %s grammar\n", argv[0]);
		exit(2);
	}
	filename = argv[1];
#endif
	g = getgrammar(filename);
	fp = fopen("graminit.c", "w");
	if (fp == NULL) {
		perror("graminit.c");
		exit(1);
	}
	printf("Writing graminit.c ...\n");
	printgrammar(g, fp);
	fclose(fp);
	fp = fopen("graminit.h", "w");
	if (fp == NULL) {
		perror("graminit.h");
		exit(1);
	}
	printf("Writing graminit.h ...\n");
	printnonterminals(g, fp);
	fclose(fp);
	exit(0);
}

void
fatal(msg)
	char *msg;
{
	fprintf(stderr, "pgen: FATAL ERROR: %s\n", msg);
	exit(1);
}

/*	TO DO:

	- improve user interface
	- check for duplicate definitions of names (instead of fatal err)
*/
