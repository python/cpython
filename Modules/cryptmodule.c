/* cryptmodule.c - by Steve Majewski
 */

#include "allobjects.h"
#include "modsupport.h"

#include <sys/types.h>


/* Module crypt */


static object *crypt_crypt(self, args)
	object *self, *args;
{
	char *word, *salt; 
	extern char * crypt();

	struct passwd *p;
	if (!getargs(args, "(ss)", &word, &salt)) {
		return NULL;
	}
	return newstringobject( crypt( word, salt ) );

}

static struct methodlist crypt_methods[] = {
	{"crypt",	crypt_crypt},
	{NULL,		NULL}		/* sentinel */
};

void
initcrypt()
{
	initmodule("crypt", crypt_methods);
}
