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

/* Berkeley DB interface.
   Author: Michael McLay
   Hacked: Guido van Rossum
   Btree and Recno additions plus sequence methods: David Ely

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
    DB *di_bsddb;
    int di_size;	/* -1 means recompute */
} bsddbobject;

staticforward typeobject Bsddbtype;

#define is_bsddbobject(v) ((v)->ob_type == &Bsddbtype)

static object *BsddbError;

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
    bsddbobject *dp;
    HASHINFO info;

    if ((dp = NEWOBJ(bsddbobject, &Bsddbtype)) == NULL)
	return NULL;

    info.bsize = bsize;
    info.ffactor = ffactor;
    info.nelem = nelem;
    info.cachesize = cachesize;
    info.hash = NULL; /* XXX should derive from hash argument */
    info.lorder = lorder;

    if ((dp->di_bsddb = dbopen(file, flags, mode, DB_HASH, &info)) == NULL) {
	err_errno(BsddbError);
	DECREF(dp);
	return NULL;
    }

    dp->di_size = -1;

    return (object *)dp;
}

static object *
newdbbtobject(file, flags, mode,
		btflags, cachesize, maxkeypage, minkeypage, psize, lorder)
    char *file;
    int flags;
    int mode;
    int btflags;
    int cachesize;
    int maxkeypage;
    int minkeypage;
    int psize;
    int lorder;
{
    bsddbobject *dp;
    BTREEINFO info;

    if ((dp = NEWOBJ(bsddbobject, &Bsddbtype)) == NULL)
	return NULL;

    info.flags = btflags;
    info.cachesize = cachesize;
    info.maxkeypage = maxkeypage;
    info.minkeypage = minkeypage;
    info.psize = psize;
    info.lorder = lorder;
    info.compare = 0;		/* Use default comparison functions, for now..*/
    info.prefix = 0;

    if ((dp->di_bsddb = dbopen(file, flags, mode, DB_BTREE, &info)) == NULL) {
	err_errno(BsddbError);
	DECREF(dp);
	return NULL;
    }

    dp->di_size = -1;

    return (object *)dp;
}

static object *
newdbrnobject(file, flags, mode,
		rnflags, cachesize, psize, lorder, reclen, bval, bfname)
    char *file;
    int flags;
    int mode;
    int rnflags;
    int cachesize;
    int psize;
    int lorder;
    size_t reclen;
    u_char bval;
    char *bfname;
{
    bsddbobject *dp;
    RECNOINFO info;

    if ((dp = NEWOBJ(bsddbobject, &Bsddbtype)) == NULL)
	return NULL;

    info.flags = rnflags;
    info.cachesize = cachesize;
    info.psize = psize;
    info.lorder = lorder;
    info.reclen = reclen;
    info.bval = bval;
    info.bfname = bfname;

    if ((dp->di_bsddb = dbopen(file, flags, mode, DB_RECNO, &info)) == NULL) {
	err_errno(BsddbError);
	DECREF(dp);
	return NULL;
    }

    dp->di_size = -1;

    return (object *)dp;
}


static void
bsddb_dealloc(dp)
    bsddbobject *dp;
{
    if (dp->di_bsddb != NULL) {
	if ((dp->di_bsddb->close)(dp->di_bsddb) != 0)
	    fprintf(stderr,
		    "Python bsddb: close errno %s in dealloc\n", errno);
    }
    DEL(dp);
}

static int
bsddb_length(dp)
    bsddbobject *dp;
{
    if (dp->di_size < 0) {
	DBT krec, drec;
	int status;
	int size = 0;
	for (status = (dp->di_bsddb->seq)(dp->di_bsddb, &krec, &drec,R_FIRST);
	     status == 0;
	     status = (dp->di_bsddb->seq)(dp->di_bsddb, &krec, &drec, R_NEXT))
	    size++;
	if (status < 0) {
	    err_errno(BsddbError);
	    return -1;
	}
	dp->di_size = size;
    }
    return dp->di_size;
}

static object *
bsddb_subscript(dp, key)
    bsddbobject *dp;
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

    status = (dp->di_bsddb->get)(dp->di_bsddb, &krec, &drec, 0);
    if (status != 0) {
	if (status < 0)
	    err_errno(BsddbError);
	else
	    err_setval(KeyError, key);
	return NULL;
    }

    return newsizedstringobject((char *)drec.data, (int)drec.size);
}

static int
bsddb_ass_sub(dp, key, value)
    bsddbobject *dp;
    object *key, *value;
{
    int status;
    DBT krec, drec;
    char *data;
    int size;

    if (!getargs(key, "s#", &data, &size)) {
	err_setstr(TypeError, "bsddb key type must be string");
	return -1;
    }
    krec.data = data;
    krec.size = size;
    dp->di_size = -1;
    if (value == NULL) {
	status = (dp->di_bsddb->del)(dp->di_bsddb, &krec, 0);
    }
    else {
	if (!getargs(value, "s#", &data, &size)) {
	    err_setstr(TypeError, "bsddb value type must be string");
	    return -1;
	}
	drec.data = data;
	drec.size = size;
	status = (dp->di_bsddb->put)(dp->di_bsddb, &krec, &drec, 0);
    }
    if (status != 0) {
	if (status < 0)
	    err_errno(BsddbError);
	else
	    err_setval(KeyError, key);
	return -1;
    }
    return 0;
}

static mapping_methods bsddb_as_mapping = {
    (inquiry)bsddb_length,		/*mp_length*/
    (binaryfunc)bsddb_subscript,	/*mp_subscript*/
    (objobjargproc)bsddb_ass_sub,	/*mp_ass_subscript*/
};

static object *
bsddb_close(dp, args)
    bsddbobject *dp;
    object *args;
{
    if (!getnoarg(args))
	return NULL;
    if (dp->di_bsddb != NULL) {
	if ((dp->di_bsddb->close)(dp->di_bsddb) != 0) {
		dp->di_bsddb = NULL;
	    err_errno(BsddbError);
	    return NULL;
	}
    }
    dp->di_bsddb = NULL;
    INCREF(None);
    return None;
}

static object *
bsddb_keys(dp, args)
    bsddbobject *dp;
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
    for (status = (dp->di_bsddb->seq)(dp->di_bsddb, &krec, &drec, R_FIRST);
	 status == 0;
	 status = (dp->di_bsddb->seq)(dp->di_bsddb, &krec, &drec, R_NEXT)) {
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
	err_errno(BsddbError);
	DECREF(list);
	return NULL;
    }
    if (dp->di_size < 0)
	dp->di_size = getlistsize(list); /* We just did the work for this... */
    return list;
}

static object *
bsddb_has_key(dp, args)
    bsddbobject *dp;
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

    status = (dp->di_bsddb->get)(dp->di_bsddb, &krec, &drec, 0);
    if (status < 0) {
	err_errno(BsddbError);
	return NULL;
    }

    return newintobject(status == 0);
}

static object *
bsddb_set_location(dp, key)
    bsddbobject *dp;
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

    status = (dp->di_bsddb->seq)(dp->di_bsddb, &krec, &drec, R_CURSOR);
    if (status != 0) {
	if (status < 0)
	    err_errno(BsddbError);
	else
	    err_setval(KeyError, key);
	return NULL;
    }

    return mkvalue("s#s#", krec.data, krec.size, drec.data, drec.size);
}

static object *
bsddb_seq(dp, args, sequence_request)
    bsddbobject *dp;
    object *args;
    int sequence_request;
{
    int status;
    DBT krec, drec;

    if (!getnoarg(args))
	return NULL;

    krec.data = 0;
    krec.size = 0;

    status = (dp->di_bsddb->seq)(dp->di_bsddb, &krec, &drec, sequence_request);
    if (status != 0) {
	if (status < 0)
	    err_errno(BsddbError);
	else
	    err_setval(KeyError, args);
	return NULL;
    }

    return mkvalue("s#s#", krec.data, krec.size, drec.data, drec.size);
}

static object *
bsddb_next(dp, key)
    bsddbobject *dp;
    object *key;
{
    return bsddb_seq(dp, key, R_NEXT);
}
static object *
bsddb_previous(dp, key)
    bsddbobject *dp;
    object *key;
{
    return bsddb_seq(dp, key, R_PREV);
}
static object *
bsddb_first(dp, key)
    bsddbobject *dp;
    object *key;
{
    return bsddb_seq(dp, key, R_FIRST);
}
static object *
bsddb_last(dp, key)
    bsddbobject *dp;
    object *key;
{
    return bsddb_seq(dp, key, R_LAST);
}
static object *
bsddb_sync(dp, args)
    bsddbobject *dp;
    object *args;
{
    int status;

    status = (dp->di_bsddb->sync)(dp->di_bsddb, 0);
    if (status != 0) {
	err_errno(BsddbError);
	return NULL;
    }
    return newintobject(status = 0);
}
static struct methodlist bsddb_methods[] = {
    {"close",		(method)bsddb_close},
    {"keys",		(method)bsddb_keys},
    {"has_key",		(method)bsddb_has_key},
    {"set_location",	(method)bsddb_set_location},
    {"next",		(method)bsddb_next},
    {"previous",	(method)bsddb_previous},
    {"first",		(method)bsddb_first},
    {"last",		(method)bsddb_last},
    {"sync",		(method)bsddb_sync},
    {NULL,	       	NULL}		/* sentinel */
};

static object *
bsddb_getattr(dp, name)
    object *dp;
    char *name;
{
    return findmethod(bsddb_methods, dp, name);
}

static typeobject Bsddbtype = {
    OB_HEAD_INIT(&Typetype)
    0,
    "bsddb",
    sizeof(bsddbobject),
    0,
    (destructor)bsddb_dealloc, /*tp_dealloc*/
    0,			/*tp_print*/
    (getattrfunc)bsddb_getattr, /*tp_getattr*/
    0,			/*tp_setattr*/
    0,			/*tp_compare*/
    0,			/*tp_repr*/
    0,			/*tp_as_number*/
    0,			/*tp_as_sequence*/
    &bsddb_as_mapping,	/*tp_as_mapping*/
};

static object *
bsdhashopen(self, args)
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
	    err_setstr(BsddbError,
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
	    err_setstr(BsddbError, "locking not supported on this platform");
	    return NULL;
#endif
	}
    }
    return newdbhashobject(file, flags, mode,
			   bsize, ffactor, nelem, cachesize, hash, lorder);
}

static object *
bsdbtopen(self, args)
    object *self;
    object *args;
{
    char *file;
    char *flag = NULL;
    int flags = O_RDONLY;
    int mode = 0666;
    int cachesize = 0;
    int maxkeypage;
    int minkeypage;
    int btflags;
    unsigned int psize;
    int lorder;

    if (!newgetargs(args, "s|siiiiiii",
		 &file, &flag, &mode,
		 &btflags, &cachesize, &maxkeypage, &minkeypage, &psize, &lorder))
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
	    err_setstr(BsddbError,
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
	    err_setstr(BsddbError, "locking not supported on this platform");
	    return NULL;
#endif
	}
    }
    return newdbbtobject(file, flags, mode,
			   btflags, cachesize, maxkeypage, minkeypage, psize, lorder);
}
static object *
bsdrnopen(self, args)
    object *self;
    object *args;
{
    char *file;
    char *flag = NULL;
    int flags = O_RDONLY;
    int mode = 0666;
    int cachesize = 0;
    int rnflags;
    unsigned int psize;
    int lorder;
    size_t reclen;
    char  *bval;
    char *bfname;

    if (!newgetargs(args, "s|siiiiiiss",
		 &file, &flag, &mode,
		 &rnflags, &cachesize, &psize, &lorder, &reclen, &bval, &bfname))
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
	    err_setstr(BsddbError,
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
	    err_setstr(BsddbError, "locking not supported on this platform");
	    return NULL;
#endif
	}
    }
    return newdbrnobject(file, flags, mode,
			   rnflags, cachesize, psize, lorder, bval[0], bfname);
}

static struct methodlist bsddbmodule_methods[] = {
    {"hashopen",	(method)bsdhashopen, 1},
    {"btopen",		(method)bsdbtopen, 1},
    {"rnopen",		(method)bsdrnopen, 1},
    {0,		0},
};

void
initbsddb() {
    object *m, *d;

    m = initmodule("bsddb", bsddbmodule_methods);
    d = getmoduledict(m);
    BsddbError = newstringobject("bsddb.error");
    if (BsddbError == NULL || dictinsert(d, "error", BsddbError))
	fatal("can't define bsddb.error");
}
