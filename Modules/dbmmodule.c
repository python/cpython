/***********************************************************
Copyright 1991, 1992, 1993 by Stichting Mathematisch Centrum,
Amsterdam, The Netherlands.

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

/* DBM module using dictionary interface */


#include "allobjects.h"
#include "modsupport.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ndbm.h>

typedef struct {
	OB_HEAD
	int di_size;	/* -1 means recompute */
	DBM *di_dbm;
} dbmobject;

extern typeobject Dbmtype;	/* Really forward */

#define is_dbmobject(v) ((v)->ob_type == &Dbmtype)

static object *DbmError;

static object *
newdbmobject(file, flags, mode)
    char *file;
    int flags;
    int mode;
{
        dbmobject *dp;

	dp = NEWOBJ(dbmobject, &Dbmtype);
	if (dp == NULL)
		return NULL;
	dp->di_size = -1;
	if ( (dp->di_dbm = dbm_open(file, flags, mode)) == 0 ) {
	    err_errno(DbmError);
	    DECREF(dp);
	    return 0;
	}
	return (object *)dp;
}

/* Methods */

static void
dbm_dealloc(dp)
	register dbmobject *dp;
{
        if ( dp->di_dbm )
	  dbm_close(dp->di_dbm);
	DEL(dp);
}

static int
dbm_length(dp)
	dbmobject *dp;
{
        if ( dp->di_size < 0 ) {
	    datum key;
	    int size;

	    size = 0;
	    for ( key=dbm_firstkey(dp->di_dbm); key.dptr;
				   key = dbm_nextkey(dp->di_dbm))
		 size++;
	    dp->di_size = size;
	}
	return dp->di_size;
}

static object *
dbm_subscript(dp, key)
	dbmobject *dp;
	register object *key;
{
	object *v;
	datum drec, krec;
	
	if (!getargs(key, "s#", &krec.dptr, &krec.dsize) )
		return NULL;
	
	drec = dbm_fetch(dp->di_dbm, krec);
	if ( drec.dptr == 0 ) {
	    err_setstr(KeyError, GETSTRINGVALUE((stringobject *)key));
	    return 0;
	}
	if ( dbm_error(dp->di_dbm) ) {
	    dbm_clearerr(dp->di_dbm);
	    err_setstr(DbmError, "");
	    return 0;
	}
	return newsizedstringobject(drec.dptr, drec.dsize);
}

static int
dbm_ass_sub(dp, v, w)
	dbmobject *dp;
	object *v, *w;
{
        datum krec, drec;
	
        if ( !getargs(v, "s#", &krec.dptr, &krec.dsize) ) {
	    err_setstr(TypeError, "dbm mappings have string indices only");
	    return -1;
	}
	dp->di_size = -1;
	if (w == NULL) {
	    if ( dbm_delete(dp->di_dbm, krec) < 0 ) {
		err_setstr(KeyError, GETSTRINGVALUE((stringobject *)v));
		return -1;
	    }
	} else {
	    if ( !getargs(w, "s#", &drec.dptr, &drec.dsize) ) {
		err_setstr(TypeError,
			   "dbm mappings have string elements only");
		return -1;
	    }
	    if ( dbm_store(dp->di_dbm, krec, drec, DBM_REPLACE) < 0 ) {
		err_setstr(DbmError, "Cannot add item to database");
		return -1;
	    }
	}
	if ( dbm_error(dp->di_dbm) ) {
	    dbm_clearerr(dp->di_dbm);
	    err_setstr(DbmError, "");
	    return -1;
	}
	return 0;
}

static mapping_methods dbm_as_mapping = {
	dbm_length,	/*mp_length*/
	dbm_subscript,	/*mp_subscript*/
	dbm_ass_sub,	/*mp_ass_subscript*/
};

static object *
dbm_keys(dp, args)
	register dbmobject *dp;
	object *args;
{
	register object *v, *item;
	datum key;

	if (dp == NULL || !is_dbmobject(dp)) {
		err_badcall();
		return NULL;
	}
	if (!getnoarg(args))
		return NULL;
	v = newlistobject(0);
	if (v == NULL)
		return NULL;
	for (key = dbm_firstkey(dp->di_dbm); key.dptr;
				key = dbm_nextkey(dp->di_dbm) ) {
	    item = newsizedstringobject(key.dptr, key.dsize);
	    if ( item == 0 )
	      return NULL;
	    addlistitem(v, item);
	}
	return v;
}


static object *
dbm_has_key(dp, args)
	register dbmobject *dp;
	object *args;
{
	datum key, val;
	
	if (!getargs(args, "s#", &key.dptr, &key.dsize))
		return NULL;
	val = dbm_fetch(dp->di_dbm, key);
	return newintobject(val.dptr != NULL);
}

static struct methodlist dbm_methods[] = {
	{"keys",	dbm_keys},
	{"has_key",	dbm_has_key},
	{NULL,		NULL}		/* sentinel */
};

static object *
dbm_getattr(dp, name)
	dbmobject *dp;
	char *name;
{
	return findmethod(dbm_methods, (object *)dp, name);
}

typeobject Dbmtype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"Dbm_dictionary",
	sizeof(dbmobject),
	0,
	dbm_dealloc,	/*tp_dealloc*/
	0,		/*tp_print*/
	dbm_getattr,	/*tp_getattr*/
	0,		/*tp_setattr*/
	0,		/*tp_compare*/
	0,		/*tp_repr*/
	0,		/*tp_as_number*/
	0,		/*tp_as_sequence*/
	&dbm_as_mapping,	/*tp_as_mapping*/
};

/* ----------------------------------------------------------------- */

static object *
dbmopen(self, args)
    object *self;
    object *args;
{
    char *name, *flags;
    int iflags, mode;

        if ( !getargs(args, "(ssi)", &name, &flags, &mode) )
	  return 0;
	if ( strcmp(flags, "r") == 0 )
	  iflags = O_RDONLY;
	else if ( strcmp(flags, "w") == 0 )
	  iflags = O_WRONLY|O_CREAT;
	else if ( strcmp(flags, "rw") == 0 )
	  iflags = O_RDWR|O_CREAT;
	else {
	    err_setstr(DbmError,
		       "Flags should be one of 'r', 'w' or 'rw'");
	    return 0;
	}
        return newdbmobject(name, iflags, mode);
}

static struct methodlist dbmmodule_methods[] = {
    { "open", dbmopen },
    { 0, 0 },
};

void
initdbm() {
    object *m, *d;

    m = initmodule("dbm", dbmmodule_methods);
    d = getmoduledict(m);
    DbmError = newstringobject("dbm.error");
    if ( DbmError == NULL || dictinsert(d, "error", DbmError) )
      fatal("can't define dbm.error");
}

	
