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

/* Berkeley DB hash interface.
   Author: Michael McLay
   Hacked: Guido van Rossum

   XXX To do:
   - provide interface to the B-tree and record libraries too
   - provide a way to access the various hash functions
   - support more open flags
 */

#include "allobjects.h"
#include "modsupport.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <db.h>
/* Please don't include internal header files of the Berkeley db package
   (it messes up the info required in the Setup file) */

typedef struct {
    OB_HEAD
    DB *di_dbhash;
    int di_size;	/* -1 means recompute */
} dbhashobject;

staticforward typeobject Dbhashtype;

#define is_dbhashobject(v) ((v)->ob_type == &Dbhashtype)

static object *DbhashError;

static object *
newdbhashobject(file, flags, mode,
		bsize, ffactor, nelem, cachesize, hash, lorder)
    char *file;
    int flags;
    int mode;
    int bsize;
    int ffactor;
    int nelem;
    int cachesize;
    int hash; /* XXX ignored */
    int lorder;
{
    dbhashobject *dp;
    HASHINFO info;

    if ((dp = NEWOBJ(dbhashobject, &Dbhashtype)) == NULL)
	return NULL;

    info.bsize = bsize;
    info.ffactor = ffactor;
    info.nelem = nelem;
    info.cachesize = cachesize;
    info.hash = NULL; /* XXX should derive from hash argument */
    info.lorder = lorder;

    if ((dp->di_dbhash = dbopen(file, flags, mode, DB_HASH, &info)) == NULL) {
	err_errno(DbhashError);
	DECREF(dp);
	return NULL;
    }

    dp->di_size = -1;

    return (object *)dp;
}

static void
dbhash_dealloc(dp)
    dbhashobject *dp;
{
    if (dp->di_dbhash != NULL) {
	if ((dp->di_dbhash->close)(dp->di_dbhash) != 0)
	    fprintf(stderr,
		    "Python dbhash: close errno %s in dealloc\n", errno);
    }
    DEL(dp);
}

static int
dbhash_length(dp)
    dbhashobject *dp;
{
    if (dp->di_size < 0) {
	DBT krec, drec;
	int status;
	int size = 0;
	for (status = (dp->di_dbhash->seq)(dp->di_dbhash, &krec, &drec,R_FIRST);
	     status == 0;
	     status = (dp->di_dbhash->seq)(dp->di_dbhash, &krec, &drec, R_NEXT))
	    size++;
	if (status < 0) {
	    err_errno(DbhashError);
	    return -1;
	}
	dp->di_size = size;
    }
    return dp->di_size;
}

static object *
dbhash_subscript(dp, key)
    dbhashobject *dp;
    object *key;
{
    int status;
    object *v;
    DBT krec, drec;
    char *data;
    int size;

    if (!getargs(key, "s#", &data, &size))
	return NULL;
    krec.data = data;
    krec.size = size;

    status = (dp->di_dbhash->get)(dp->di_dbhash, &krec, &drec, 0);
    if (status != 0) {
	if (status < 0)
	    err_errno(DbhashError);
	else
	    err_setval(KeyError, key);
	return NULL;
    }

    return newsizedstringobject((char *)drec.data, (int)drec.size);
}

static int
dbhash_ass_sub(dp, key, value)
    dbhashobject *dp;
    object *key, *value;
{
    int status;
    DBT krec, drec;
    char *data;
    int size;

    if (!getargs(key, "s#", &data, &size)) {
	err_setstr(TypeError, "dbhash key type must be string");
	return -1;
    }
    krec.data = data;
    krec.size = size;
    dp->di_size = -1;
    if (value == NULL) {
	status = (dp->di_dbhash->del)(dp->di_dbhash, &krec, 0);
    }
    else {
	if (!getargs(value, "s#", &data, &size)) {
	    err_setstr(TypeError, "dbhash value type must be string");
	    return -1;
	}
	drec.data = data;
	drec.size = size;
	status = (dp->di_dbhash->put)(dp->di_dbhash, &krec, &drec, 0);
    }
    if (status != 0) {
	if (status < 0)
	    err_errno(DbhashError);
	else
	    err_setval(KeyError, key);
	return -1;
    }
    return 0;
}

static mapping_methods dbhash_as_mapping = {
    (inquiry)dbhash_length,		/*mp_length*/
    (binaryfunc)dbhash_subscript,	/*mp_subscript*/
    (objobjargproc)dbhash_ass_sub,	/*mp_ass_subscript*/
};

static object *
dbhash_close(dp, args)
    dbhashobject *dp;
    object *args;
{
    if (!getnoarg(args))
	return NULL;
    if (dp->di_dbhash != NULL) {
	if ((dp->di_dbhash->close)(dp->di_dbhash) != 0) {
		dp->di_dbhash = NULL;
	    err_errno(DbhashError);
	    return NULL;
	}
    }
    dp->di_dbhash = NULL;
    INCREF(None);
    return None;
}

static object *
dbhash_keys(dp, args)
    dbhashobject *dp;
    object *args;
{
    object *list, *item;
    DBT krec, drec;
    int status;
    int err;

    if (!getnoarg(args))
	return NULL;
    list = newlistobject(0);
    if (list == NULL)
	return NULL;
    for (status = (dp->di_dbhash->seq)(dp->di_dbhash, &krec, &drec, R_FIRST);
	 status == 0;
	 status = (dp->di_dbhash->seq)(dp->di_dbhash, &krec, &drec, R_NEXT)) {
	item = newsizedstringobject((char *)krec.data, (int)krec.size);
	if (item == NULL) {
	    DECREF(list);
	    return NULL;
	}
	err = addlistitem(list, item);
	DECREF(item);
	if (err != 0) {
	    DECREF(list);
	    return NULL;
	}
    }
    if (status < 0) {
	err_errno(DbhashError);
	DECREF(list);
	return NULL;
    }
    if (dp->di_size < 0)
	dp->di_size = getlistsize(list); /* We just did the work for this... */
    return list;
}

static object *
dbhash_has_key(dp, args)
    dbhashobject *dp;
    object *args;
{
    DBT krec, drec;
    int status;
    char *data;
    int size;

    if (!getargs(args, "s#", &data, &size))
	return NULL;
    krec.data = data;
    krec.size = size;

    status = (dp->di_dbhash->get)(dp->di_dbhash, &krec, &drec, 0);
    if (status < 0) {
	err_errno(DbhashError);
	return NULL;
    }

    return newintobject(status == 0);
}

static struct methodlist dbhash_methods[] = {
    {"close",		(method)dbhash_close},
    {"keys",		(method)dbhash_keys},
    {"has_key",		(method)dbhash_has_key},
    {NULL,	       	NULL}		/* sentinel */
};

static object *
dbhash_getattr(dp, name)
    object *dp;
    char *name;
{
    return findmethod(dbhash_methods, dp, name);
}

static typeobject Dbhashtype = {
    OB_HEAD_INIT(&Typetype)
    0,
    "dbhash",
    sizeof(dbhashobject),
    0,
    (destructor)dbhash_dealloc, /*tp_dealloc*/
    0,			/*tp_print*/
    (getattrfunc)dbhash_getattr, /*tp_getattr*/
    0,			/*tp_setattr*/
    0,			/*tp_compare*/
    0,			/*tp_repr*/
    0,			/*tp_as_number*/
    0,			/*tp_as_sequence*/
    &dbhash_as_mapping,	/*tp_as_mapping*/
};

static object *
dbhashopen(self, args)
    object *self;
    object *args;
{
    char *file;
    char *flag = NULL;
    int flags = O_RDONLY;
    int mode = 0666;
    int bsize = 0;
    int ffactor = 0;
    int nelem = 0;
    int cachesize = 0;
    int hash = 0; /* XXX currently ignored */
    int lorder = 0;

    if (!newgetargs(args, "s|siiiiiii",
		 &file, &flag, &mode,
		 &bsize, &ffactor, &nelem, &cachesize, &hash, &lorder))
	  return NULL;
    if (flag != NULL) {
	/* XXX need a way to pass O_EXCL, O_EXLOCK, O_NONBLOCK, O_SHLOCK */
	if (flag[0] == 'r')
	    flags = O_RDONLY;
	else if (flag[0] == 'w')
	    flags = O_RDWR;
	else if (flag[0] == 'c')
	    flags = O_RDWR|O_CREAT;
	else if (flag[0] == 'n')
	    flags = O_RDWR|O_CREAT|O_TRUNC;
	else {
	    err_setstr(DbhashError,
		       "Flag should begin with 'r', 'w', 'c' or 'n'");
	    return NULL;
	}
	if (flag[1] == 'l') {
#if defined(O_EXLOCK) && defined(O_SHLOCK)
	    if (flag[0] == 'r')
	        flags |= O_SHLOCK;
	    else
		flags |= O_EXLOCK;
#else
	    err_setstr(DbhashError, "locking not supported on this platform");
	    return NULL;
#endif
	}
    }
    return newdbhashobject(file, flags, mode,
			   bsize, ffactor, nelem, cachesize, hash, lorder);
}

static struct methodlist dbhashmodule_methods[] = {
    {"open",	(method)dbhashopen, 1},
    {0,		0},
};

void
initdbhash() {
    object *m, *d;

    m = initmodule("dbhash", dbhashmodule_methods);
    d = getmoduledict(m);
    DbhashError = newstringobject("dbhash.error");
    if (DbhashError == NULL || dictinsert(d, "error", DbhashError))
	fatal("can't define dbhash.error");
}
