/* Macintosh Gestalt interface */

#include "allobjects.h"
#include "modsupport.h"

#include <Types.h>
#include <GestaltEqu.h>

static object *
gestalt_gestalt(self, args)
	object *self;
	object *args;
{
	OSErr iErr;
	char *str;
	int size;
	OSType selector;
	long response;
	if (!getargs(args, "s#", &str, &size))
		return NULL;
	if (size != 4) {
		err_setstr(TypeError, "gestalt arg must be 4-char string");
		return NULL;
	}
	selector = *(OSType*)str;
	iErr = Gestalt ( selector, &response );
	if (iErr != 0) {
		char buf[100];
		sprintf(buf, "Gestalt error code %d", iErr);
		err_setstr(RuntimeError, buf);
		return NULL;
	}
	return newintobject(response);
}

static struct methodlist gestalt_methods[] = {
	{"gestalt", gestalt_gestalt},
	{NULL, NULL} /* Sentinel */
};

void
initgestalt()
{
	initmodule("gestalt", gestalt_methods);
}
