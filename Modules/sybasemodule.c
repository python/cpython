/*
Subject: Re: Sybase module -- input sought 
From: jredford@lehman.com
To: ags@uncompaghre.informatics.jax.org (Alexander G. Smith)
Cc: python-list@cwi.nl
Date: Tue, 10 May 94 11:53:13 -0400


input sought? how about a complete module? :)

This is a fairly incomplete work.. but I have done things dramatically
differently than sybperl would. Given the nature of the language I
find it is easier to simply get ALL results & then muck with the rows
later as parts of the data. This is a subset of the functionality of a
Modula-3 interface to Sybase that I wrote.. I could send you that if
you are interested in a more complete picture.
*/

#include <stdio.h>

#include <sybfront.h>
#include <sybdb.h>

#include "allobjects.h"
#include "modsupport.h"


static object *SybaseError;	/* exception sybase.error */


typedef struct {
        OB_HEAD
	LOGINREC *login;	/* login record */
	DBPROCESS *dbproc;	/* login record */
} sybdbobject;

extern typeobject SybDbtype; /* Forward */


static sybdbobject *
  newsybdbobject(char *user, char *passwd, char *server)
{
  sybdbobject *s;

  s = NEWOBJ(sybdbobject, &SybDbtype);
  if (s != NULL) {
    s->login = dblogin();
    if (user) {
      (void)DBSETLUSER(s->login, user);
    }
    if (passwd) {
      (void)DBSETLPWD(s->login, passwd);
    }
    if(!(s->dbproc = dbopen(s->login, server))) {
      dbloginfree(s->login);
      DEL(s);
      return (NULL);
    }
  }
  return s;
}

/* OBJECT FUNCTIONS: sybdb */

/* Common code for returning pending results */
static object
  *getresults (DBPROCESS *dbp)
{
  object *results;
  object *list;
  object *tuple;
  object *o;
  int retcode;
  int cols;
  int *fmt;
  int i;

  results = newlistobject(0);
  while ((retcode = dbresults(dbp)) != NO_MORE_RESULTS) {
    if (retcode == SUCCEED && DBROWS(dbp) == SUCCEED) {
      list = newlistobject(0);
      cols = dbnumcols(dbp);
      fmt = (int *)malloc(sizeof(int) * cols);
      for (i = 1; i <= cols; i++) {
	switch(dbcoltype(dbp, i)) {
	case SYBCHAR:
	  fmt[i-1] = SYBCHAR;
	  break;
	case SYBINT1:
	  fmt[i-1] = SYBINT1;
	  break;
	case SYBINT2:
	  fmt[i-1] = SYBINT2;
	  break;
	case SYBINT4:
	  fmt[i-1] = SYBINT4;
	  break;
	case SYBFLT8:
	  fmt[i-1] = SYBFLT8;
	  break;
	}
      }
      while (dbnextrow(dbp) != NO_MORE_ROWS) {
	tuple = newtupleobject(cols);
	for (i = 1; i <= cols; i++) {
	  switch(fmt[i-1]) {
	  case SYBCHAR:
	    o = newsizedstringobject((char *)dbdata(dbp, i), dbdatlen(dbp, i));
	    settupleitem(tuple, i-1, o);
	    break;
	  case SYBINT1:
	    o = newintobject(*((char *)dbdata(dbp, i)));
	    settupleitem(tuple, i-1, o);
	    break;
	  case SYBINT2:
	    o = newintobject(*((short *)dbdata(dbp, i)));
	    settupleitem(tuple, i-1, o);
	    break;
	  case SYBINT4:
	    o = newintobject(*((int *)dbdata(dbp, i)));
	    settupleitem(tuple, i-1, o);
	    break;
	  case SYBFLT8:
	    o = newfloatobject(*((double *)dbdata(dbp, i)));
	    settupleitem(tuple, i-1, o);
	    break;
	  }
	}
	addlistitem(list,tuple);
      }
      free(fmt);
      addlistitem(results,list);
    }
  }
  return (results);
}

static object
  *sybdb_sql (self, args)
object *self;
object *args;
{
  char *sql;
  DBPROCESS *dbp;

  dbp = ((sybdbobject *)self)->dbproc;
  err_clear ();
  if (!getargs (args, "s", &sql)) {
    return NULL;
  }
  dbcancel(dbp);
  dbcmd(dbp, sql);
  dbsqlexec(dbp);
  return getresults(dbp);
}

static object
  *sybdb_sp (self, args)
object *self;
object *args;
{
  char *sp;
  DBPROCESS *dbp;
  object *spargs;
  object *sparg;
  object *results;
  object *r;
  int spargcnt;
  int i;
  int retstatus;

  dbp = ((sybdbobject *)self)->dbproc;
  err_clear ();
  if (!getargs (args, "(sO)", &sp, &spargs)) {
    return NULL;
  }

  dbcancel(dbp);
  dbrpcinit(dbp, sp, 0);

  if (is_tupleobject(spargs)) {
    spargcnt=gettuplesize(spargs);
    for (i=0; i < spargcnt; i++) {
      sparg = gettupleitem(spargs,i);
      if (is_intobject(sparg)) {
	int i;
	i = getintvalue(sparg);
	dbrpcparam(dbp, NULL, 0, SYBINT4, -1, -1, &i);
      } else if (is_floatobject(sparg)) {
	double i;
	i = getfloatvalue(sparg);
	dbrpcparam(dbp, NULL, 0, SYBFLT8, -1, -1, &i);
      } else if (is_stringobject(sparg)) {
	dbrpcparam(dbp, NULL, 0, SYBCHAR, -1, getstringsize(sparg), getstringvalue(sparg));
      } else {
	err_setstr (SybaseError, "Could not handle paramaters to procedure.");
	return NULL;
      }
    }
  } else if (spargs != None) {
    err_setstr (SybaseError, "Could not handle paramaters to procedure.");
    return NULL;
  }
  dbrpcsend(dbp);
  dbsqlok(dbp);

  results = getresults(dbp);
  retstatus = dbretstatus(dbp);

  r = mkvalue("(iO)", retstatus, results);
  DECREF(results);
  return (r);
}


static struct methodlist sybdb_methods[] = {
  {"sql",        sybdb_sql},
  {"sp",         sybdb_sp},
  {NULL,         NULL}		/* sentinel */
};

static void
sybdb_dealloc(s)
     sybdbobject *s;
{
  dbloginfree(s->login);
  dbclose(s->dbproc);
  DEL(s);
}

static object *
sybdb_getattr(s, name)
     sybdbobject *s;
     char *name;
{
  return findmethod(sybdb_methods, (object *) s, name);
}


typeobject SybDbtype = {
        OB_HEAD_INIT(&Typetype)
        0,
        "sybdb",
        sizeof(sybdbobject),
        0,
        sybdb_dealloc,		/*tp_dealloc*/
        0,			/*tp_print*/
        sybdb_getattr,		/*tp_getattr*/
        0,			/*tp_setattr*/
        0,			/*tp_compare*/
        0,			/*tp_repr*/
        0,			/*tp_as_number*/
        0,			/*tp_as_sequence*/
        0,			/*tp_as_mapping*/
};





/* MODULE FUNCTIONS: sybase */

static object
  *sybase_new (self, args)
object *self;		/* Not used */
object *args;
{
  char *user, *passwd, *server;
  object *db;

  err_clear ();
  if (!getargs (args, "(zzz)", &user, &passwd, &server)) {
    return NULL;
  }
  db = (object *) newsybdbobject(user, passwd, server);
  if (!db) {
    /* XXX Should be setting some errstr stuff here based on sybase errors */
    err_setstr (SybaseError, "Could not open connection to server.");
    return NULL;
  }
  return db;
}


/* List of module functions */
static struct methodlist sybase_methods[]=
{
  {"new", sybase_new},
  {NULL, NULL}			/* sentinel */
};

/* Module initialisation */
void initsybase ()
{
  object *m, *d;

  /* Create the module and add the functions */
  m = initmodule ("sybase", sybase_methods);
  /* Add some symbolic constants to the module */
  d = getmoduledict (m);
  SybaseError = newstringobject ("sybase.error");
  if (SybaseError == NULL || dictinsert (d, "error", SybaseError) != 0) {
    fatal ("can't define sybase.error");
  }
  /* Check for errors */
  if (err_occurred ()){
    fatal ("can't initialize module sybase");
  }
}
