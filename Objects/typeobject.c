/* Type object implementation */

#include "allobjects.h"

/* Type object implementation */

static void
type_print(v, fp, flags)
	typeobject *v;
	FILE *fp;
	int flags;
{
	fprintf(fp, "<type '%s'>", v->tp_name);
}

static object *
type_repr(v)
	typeobject *v;
{
	char buf[100];
	sprintf(buf, "<type '%.80s'>", v->tp_name);
	return newstringobject(buf);
}

typeobject Typetype = {
	OB_HEAD_INIT(&Typetype)
	0,			/* Number of items for varobject */
	"type",			/* Name of this type */
	sizeof(typeobject),	/* Basic object size */
	0,			/* Item size for varobject */
	0,			/*tp_dealloc*/
	type_print,		/*tp_print*/
	0,			/*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	type_repr,		/*tp_repr*/
};
