/* GDBM module, hacked from the still-breathing corpse of the
   DBM module by anthony.baxter@aaii.oz.au. Original copyright
   follows:
*/
/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

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
#include "gdbm.h"

typedef struct {
	OB_HEAD
	int di_size;	/* -1 means recompute */
	GDBM_FILE di_dbm;
} dbmobject;

staticforward typeobject Dbmtype;

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
	errno = 0;
	if ( (dp->di_dbm = gdbm_open(file, 0, flags, mode, NULL)) == 0 ) {
	    if (errno != 0)
	        err_errno(DbmError);
	    else
	        err_setstr(DbmError, (char *) gdbm_strerror(gdbm_errno));
	    DECREF(dp);
	    return NULL;
	}
	return (object *)dp;
}

/* Methods */

static void
dbm_dealloc(dp)
	register dbmobject *dp;
{
        if ( dp->di_dbm )
	  gdbm_close(dp->di_dbm);
	DEL(dp);
}

static int
dbm_length(dp)
	dbmobject *dp;
{
        if ( dp->di_size < 0 ) {
	    datum key,okey;
	    int size;
    	    okey.dsize=0;

	    size = 0;
	    for ( key=gdbm_firstkey(dp->di_dbm); key.dptr;
				   key = gdbm_nextkey(dp->di_dbm,okey)) {
		 size++;
    	    	 if(okey.dsize) free(okey.dptr);
    	    	 okey=key;
    	    }
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
	
	drec = gdbm_fetch(dp->di_dbm, krec);
	if ( drec.dptr == 0 ) {
	    err_setstr(KeyError, GETSTRINGVALUE((stringobject *)key));
	    return NULL;
	}
	v = newsizedstringobject(drec.dptr, drec.dsize);
	free(drec.dptr);
	return v;
}

static int
dbm_ass_sub(dp, v, w)
	dbmobject *dp;
	object *v, *w;
{
        datum krec, drec;
	
        if ( !getargs(v, "s#", &krec.dptr, &krec.dsize) ) {
	    err_setstr(TypeError, "gdbm mappings have string indices only");
	    return -1;
	}
	dp->di_size = -1;
	if (w == NULL) {
	    if ( gdbm_delete(dp->di_dbm, krec) < 0 ) {
		err_setstr(KeyError, GETSTRINGVALUE((stringobject *)v));
		return -1;
	    }
	} else {
	    if ( !getargs(w, "s#", &drec.dptr, &drec.dsize) ) {
		err_setstr(TypeError,
			   "gdbm mappings have string elements only");
		return -1;
	    }
	    errno = 0;
	    if ( gdbm_store(dp->di_dbm, krec, drec, GDBM_REPLACE) < 0 ) {
	        if (errno != 0)
	            err_errno(DbmError);
	        else
	            err_setstr(DbmError, (char *) gdbm_strerror(gdbm_errno));
		return -1;
	    }
	}
	return 0;
}

static mapping_methods dbm_as_mapping = {
	(inquiry)dbm_length,		/*mp_length*/
	(binaryfunc)dbm_subscript,	/*mp_subscript*/
	(objobjargproc)dbm_ass_sub,	/*mp_ass_subscript*/
};

static object *
dbm_close(dp, args)
	register dbmobject *dp;
	object *args;
{
	if ( !getnoarg(args) )
		return NULL;
        if ( dp->di_dbm )
		gdbm_close(dp->di_dbm);
	dp->di_dbm = NULL;
	DEL(dp);
	INCREF(None);
	return None;
}

static object *
dbm_keys(dp, args)
	register dbmobject *dp;
	object *args;
{
	register object *v, *item;
	datum key, okey={ (char *)NULL, 0};

	if (dp == NULL || !is_dbmobject(dp)) {
		err_badcall();
		return NULL;
	}
	if (!getnoarg(args))
		return NULL;
	v = newlistobject(0);
	if (v == NULL)
		return NULL;
	for (key = gdbm_firstkey(dp->di_dbm); key.dptr;
				key = gdbm_nextkey(dp->di_dbm,okey) ) {
	    item = newsizedstringobject(key.dptr, key.dsize);
	    if ( item == 0 )
	      return NULL;
	    addlistitem(v, item);
    	    if(okey.dsize) free(okey.dptr);
    	    okey=key;
	}
	return v;
}


static object *
dbm_has_key(dp, args)
	register dbmobject *dp;
	object *args;
{
	datum key;
	
	if (!getargs(args, "s#", &key.dptr, &key.dsize))
		return NULL;
	return newintobject((long) gdbm_exists(dp->di_dbm, key));
}

static object *
dbm_firstkey(dp, args)
	register dbmobject *dp;
	object *args;
{
	register object *v;
	datum key;

	if (!getnoarg(args))
		return NULL;
	key = gdbm_firstkey(dp->di_dbm);
	if (key.dptr) {
	    v = newsizedstringobject(key.dptr, key.dsize);
	    free(key.dptr);
	    return v;
	} else {
	    INCREF(None);
	    return None;
	}
}

static object *
dbm_nextkey(dp, args)
	register dbmobject *dp;
	object *args;
{
	register object *v;
	datum key, nextkey;

	if (!getargs(args, "s#", &key.dptr, &key.dsize))
		return NULL;
	nextkey = gdbm_nextkey(dp->di_dbm, key);
	if (nextkey.dptr) {
	    v = newsizedstringobject(nextkey.dptr, nextkey.dsize);
	    free(nextkey.dptr);
	    return v;
	} else {
	    INCREF(None);
	    return None;
	}
}

static object *
dbm_reorganize(dp, args)
	register dbmobject *dp;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	errno = 0;
	if (gdbm_reorganize(dp->di_dbm) < 0) {
	    if (errno != 0)
		err_errno(DbmError);
	    else
		err_setstr(DbmError, (char *) gdbm_strerror(gdbm_errno));
	    return NULL;
	}
	INCREF(None);
	return None;
}

static struct methodlist dbm_methods[] = {
	{"close",	(method)dbm_close},
	{"keys",	(method)dbm_keys},
	{"has_key",	(method)dbm_has_key},
	{"firstkey",	(method)dbm_firstkey},
	{"nextkey",	(method)dbm_nextkey},
	{"reorganize",	(method)dbm_reorganize},
	{NULL,		NULL}		/* sentinel */
};

static object *
dbm_getattr(dp, name)
	dbmobject *dp;
	char *name;
{
	return findmethod(dbm_methods, (object *)dp, name);
}

static typeobject Dbmtype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"gdbm",
	sizeof(dbmobject),
	0,
	(destructor)dbm_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)dbm_getattr, /*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	&dbm_as_mapping,	/*tp_as_mapping*/
};

/* ----------------------------------------------------------------- */

static object *
dbmopen(self, args)
    object *self;
    object *args;
{
    char *name;
    char *flags = "r";
    int iflags;
   int mode = 0666;

/* XXXX add other flags */
        if ( !newgetargs(args, "s|si", &name, &flags, &mode) )
	  return NULL;
	if ( strcmp(flags, "r") == 0 )
	  iflags = GDBM_READER;
	else if ( strcmp(flags, "w") == 0 )
	  iflags = GDBM_WRITER;
	else if ( strcmp(flags, "c") == 0 )
	  iflags = GDBM_WRCREAT;
	else if ( strcmp(flags, "n") == 0 )
	  iflags = GDBM_NEWDB;
	else {
	    err_setstr(DbmError,
		       "Flags should be one of 'r', 'w', 'c' or 'n'");
	    return NULL;
	}
        return newdbmobject(name, iflags, mode);
}

static struct methodlist dbmmodule_methods[] = {
    { "open", (method)dbmopen, 1 },
    { 0, 0 },
};

void
initgdbm() {
    object *m, *d;

    m = initmodule("gdbm", dbmmodule_methods);
    d = getmoduledict(m);
    DbmError = newstringobject("gdbm.error");
    if ( DbmError == NULL || dictinsert(d, "error", DbmError) )
      fatal("can't define gdbm.error");
}
