#include <Types.h>
#include <Files.h>
#include <OSUtils.h>
#include <Resources.h>

#include <string.h>

/* Interface used by tokenizer.c */

guesstabsize(path)
	char *path;
{
	Str255 s;
	int refnum;
	Handle h;
	int tabsize = 0;
	s[0] = strlen(path);
	memcpy(s+1, path, s[0]);
	refnum = OpenResFile(s);
	if (refnum == -1)
		return 0;
	UseResFile(refnum);
	h = GetIndResource('ETAB', 1);
	if (h != 0) {
		tabsize = (*(short**)h)[1];
	}
	CloseResFile(refnum);
	return tabsize;
}
