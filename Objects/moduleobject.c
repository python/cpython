/* Module object implementation */

#include <stdio.h>

#include "PROTO.h"
#include "object.h"
#include "stringobject.h"
#include "dictobject.h"
#include "moduleobject.h"
#include "objimpl.h"

typedef struct {
	OB_HEAD
	object *md_name;
	object *md_dict;
} moduleobject;

object *
newmoduleobject(name)
	char *name;
{
	moduleobject *m = NEWOBJ(moduleobject, &Moduletype);
	if (m == NULL)
		return NULL;
	m->md_name = newstringobject(name);
	m->md_dict = newdictobject();
	if (m->md_name == NULL || m->md_dict == NULL) {
		DECREF(m);
		return NULL;
	}
	return (object *)m;
}

object *
getmoduledict(m)
	object *m;
{
	if (!is_moduleobject(m)) {
		errno = EBADF;
		return NULL;
	}
	return ((moduleobject *)m) -> md_dict;
}

int
setmoduledict(m, v)
	object *m;
	object *v;
{
	if (!is_moduleobject(m))
		return errno = EBADF;
	if (!is_dictobject(v))
		return errno = EINVAL;
	DECREF(((moduleobject *)m) -> md_dict);
	INCREF(v);
	((moduleobject *)m) -> md_dict = v;
	return 0;
}

/* Methods */

static void
moduledealloc(m)
	moduleobject *m;
{
	if (m->md_name != NULL)
		DECREF(m->md_name);
	if (m->md_dict != NULL)
		DECREF(m->md_dict);
	free((char *)m);
}

static void
moduleprint(m, fp, flags)
	moduleobject *m;
	FILE *fp;
	int flags;
{
	fprintf(fp, "<module %s>", getstringvalue(m->md_name));
}

static object *
modulerepr(m)
	moduleobject *m;
{
	char buf[100];
	sprintf(buf, "<module %.80s>", getstringvalue(m->md_name));
	return newstringobject(buf);
}

static object *
modulegetattr(m, name)
	moduleobject *m;
	char *name;
{
	object *res = dictlookup(m->md_dict, name);
	if (res == NULL) {
		if (errno == ENOENT)
			errno = ESRCH;
	}
	else
		INCREF(res);
	return res;
}

static int
modulesetattr(m, name, v)
	moduleobject *m;
	char *name;
	object *v;
{
	if (v == NULL)
		return dictremove(m->md_dict, name);
	else
		return dictinsert(m->md_dict, name, v);
}

typeobject Moduletype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"module",		/*tp_name*/
	sizeof(moduleobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	moduledealloc,	/*tp_dealloc*/
	moduleprint,	/*tp_print*/
	modulegetattr,	/*tp_getattr*/
	modulesetattr,	/*tp_setattr*/
	0,		/*tp_compare*/
	modulerepr,	/*tp_repr*/
};
