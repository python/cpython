#include <Types.h>
#include <Files.h>
#include <OSUtils.h>
#include <Resources.h>

#include <string.h>

/* Interface used by tokenizer.c */

guesstabsize(path)
	char *path;
{
	char s[256];
	int refnum;
	Handle h;
	int tabsize = 0;
	s[0] = strlen(path);
	strncpy(s+1, path, s[0]);
	refnum = OpenResFile(s);
/* printf("%s --> refnum=%d\n", path, refnum); */
	if (refnum == -1)
		return 0;
	UseResFile(refnum);
	h = GetIndResource('ETAB', 1);
	if (h != 0) {
		tabsize = (*(short**)h)[1];
/* printf("tabsize=%d\n", tabsize); */
	}
	CloseResFile(refnum);
	return tabsize;
}
