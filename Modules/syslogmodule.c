/***********************************************************
Copyright 1991, 1992, 1993, 1994 by Stichting Mathematisch Centrum,
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

/* syslog module */
/* (By Lance Ellinghouse) */

#include "allobjects.h"
#include "modsupport.h"

#include <syslog.h>

static object *
syslog_openlog(self, args)
     object *self;
     object *args;
{
  char *ident = "";
  object *ident_o;
  long logopt = LOG_PID;
  long facility = LOG_USER;
  if (!getargs(args, "(Sll);ident string, logoption, facility", &ident_o, &logopt, &facility))
    if (!getargs(args, "(Sl);ident string, logoption", &ident_o, &logopt))
      if (!getargs(args, "S;ident string", &ident_o))
	return NULL;
  INCREF(ident_o); /* This is needed because openlog() does NOT make a copy
		      and syslog() later uses it.. cannot trash it. */
  ident = getstringvalue(ident_o);
  openlog(ident,logopt,facility);
  INCREF(None);
  return None;
}

static object *
syslog_syslog(self, args)
     object *self;
     object *args;
{
  int   priority = LOG_INFO;
  char *message;

  if (!getargs(args,"(is);priority, message string",&priority,&message))
    if (!getargs(args,"s;message string",&message))
      return NULL;
  syslog(priority, message);
  INCREF(None);
  return None;
}

static object *
syslog_closelog(self, args)
     object *self;
     object *args;
{
	if (!getnoarg(args))
		return NULL;
	closelog();
	INCREF(None);
	return None;
}

static object *
syslog_setlogmask(self, args)
     object *self;
     object *args;
{
  long maskpri;
  if (!getargs(args,"l;mask for priority",&maskpri))
    return NULL;
  setlogmask(maskpri);
  INCREF(None);
  return None;
}

static object *
syslog_log_mask(self, args)
     object *self;
     object *args;
{
  long mask;
  long pri;
  if (!getargs(args,"l",&pri))
    return NULL;
  mask = LOG_MASK(pri);
  return newintobject(mask);
}

static object *
syslog_log_upto(self, args)
     object *self;
     object *args;
{
  long mask;
  long pri;
  if (!getargs(args,"l",&pri))
    return NULL;
  mask = LOG_UPTO(pri);
  return newintobject(mask);
}

/* List of functions defined in the module */

static struct methodlist syslog_methods[] = {
	{"openlog",		syslog_openlog},
	{"closelog",		syslog_closelog},
	{"syslog",              syslog_syslog},
	{"setlogmask",          syslog_setlogmask},
	{"LOG_MASK",            syslog_log_mask},
	{"LOG_UPTO",            syslog_log_upto},
	{NULL,		NULL}		/* sentinel */
};

/* Initialization function for the module */

void
initsyslog()
{
	object *m, *d, *x;

	/* Create the module and add the functions */
	m = initmodule("syslog", syslog_methods);

	/* Add some symbolic constants to the module */
	d = getmoduledict(m);
	x = newintobject(LOG_EMERG);
	dictinsert(d, "LOG_EMERG", x);
	x = newintobject(LOG_ALERT);
	dictinsert(d, "LOG_ALERT", x);
	x = newintobject(LOG_CRIT);
	dictinsert(d, "LOG_CRIT", x);
	x = newintobject(LOG_ERR);
	dictinsert(d, "LOG_ERR", x);
	x = newintobject(LOG_WARNING);
	dictinsert(d, "LOG_WARNING", x);
	x = newintobject(LOG_NOTICE);
	dictinsert(d, "LOG_NOTICE", x);
	x = newintobject(LOG_INFO);
	dictinsert(d, "LOG_INFO", x);
	x = newintobject(LOG_DEBUG);
	dictinsert(d, "LOG_DEBUG", x);
	x = newintobject(LOG_PID);
	dictinsert(d, "LOG_PID", x);
	x = newintobject(LOG_CONS);
	dictinsert(d, "LOG_CONS", x);
	x = newintobject(LOG_NDELAY);
	dictinsert(d, "LOG_NDELAY", x);
	x = newintobject(LOG_NOWAIT);
	dictinsert(d, "LOG_NOWAIT", x);
	x = newintobject(LOG_KERN);
	dictinsert(d, "LOG_KERN", x);
	x = newintobject(LOG_USER);
	dictinsert(d, "LOG_USER", x);
	x = newintobject(LOG_MAIL);
	dictinsert(d, "LOG_MAIL", x);
	x = newintobject(LOG_DAEMON);
	dictinsert(d, "LOG_DAEMON", x);
	x = newintobject(LOG_LPR);
	dictinsert(d, "LOG_LPR", x);
	x = newintobject(LOG_LOCAL0);
	dictinsert(d, "LOG_LOCAL0", x);
	x = newintobject(LOG_LOCAL1);
	dictinsert(d, "LOG_LOCAL1", x);
	x = newintobject(LOG_LOCAL2);
	dictinsert(d, "LOG_LOCAL2", x);
	x = newintobject(LOG_LOCAL3);
	dictinsert(d, "LOG_LOCAL3", x);
	x = newintobject(LOG_LOCAL4);
	dictinsert(d, "LOG_LOCAL4", x);
	x = newintobject(LOG_LOCAL5);
	dictinsert(d, "LOG_LOCAL5", x);
	x = newintobject(LOG_LOCAL6);
	dictinsert(d, "LOG_LOCAL6", x);
	x = newintobject(LOG_LOCAL7);
	dictinsert(d, "LOG_LOCAL7", x);

	/* Check for errors */
	if (err_occurred())
		fatal("can't initialize module syslog");
}
