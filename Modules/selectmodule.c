/***********************************************************
Copyright 1991, 1992 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

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

/* select - Module containing unix select(2) call */

#include "allobjects.h"
#include "modsupport.h"
#include "compile.h"
#include "ceval.h"

#include "myselect.h"

static object *SelectError;

static
list2set(list, set, fd2obj)
    object *list;
    fd_set *set;
    object *fd2obj[FD_SETSIZE];
{
    int i, len, v, max=-1;
    object *o, *filenomethod, *fno;
    
    FD_ZERO(set);
    len = getlistsize(list);
    for( i=0; i<len; i++ ) {
	o = getlistitem(list, i);
	if ( is_intobject(o) ) {
	    v = getintvalue(o);
	} else if ( (filenomethod = getattr(o, "fileno")) != NULL ) {
	    fno = call_object(filenomethod, NULL);
	    if ( fno == NULL )
		return -1;
	    if ( !is_intobject(fno) ) {
		err_badarg();
		return -1;
	    }
	    v = getintvalue(fno);
	    DECREF(fno);
	} else {
	    err_badarg();
	    return -1;
	}
	if ( v >= FD_SETSIZE ) {
	    err_setstr(SystemError, "FD_SETSIZE too low in select()");
	    return -1;
	}
	if ( v > max ) max = v;
	FD_SET(v, set);
	fd2obj[v] = o;
    }
    return max+1;
}

static object *
set2list(set, max, fd2obj)
    fd_set *set;
    int max;
    object *fd2obj[FD_SETSIZE];
{
    int i, num=0;
    object *list, *o;

    for(i=0; i<max; i++)
      if ( FD_ISSET(i,set) )
	num++;
    list = newlistobject(num);
    num = 0;
    for(i=0; i<max; i++)
      if ( FD_ISSET(i,set) ) {
	  if ( i > FD_SETSIZE ) {
	      err_setstr(SystemError, "FD_SETSIZE too low in select()");
	      return NULL;
	  }
	  o = fd2obj[i];
	  if ( o == 0 ) {
	      err_setstr(SystemError,
			 "Bad filedescriptor returned from select()");
	      return NULL;
	  }
	  INCREF(o);
	  setlistitem(list, num, o);
	  num++;
    }
    return list;
}
    
static object *
select_select(self, args)
    object *self;
    object *args;
{
    object *fd2obj[FD_SETSIZE];
    object *ifdlist, *ofdlist, *efdlist;
    fd_set ifdset, ofdset, efdset;
    double timeout;
    struct timeval tv, *tvp;
    int seconds;
    int imax, omax, emax, max;
    int n;


    /* Get args. Looks funny because of optional timeout argument */
    if ( getargs(args, "(OOOd)", &ifdlist, &ofdlist, &efdlist, &timeout) ) {
	seconds = (int)timeout;
	timeout = timeout - (double)seconds;
	tv.tv_sec = seconds;
	tv.tv_usec = (int)(timeout*1000000.0);
	tvp = &tv;
    } else {
	/* Doesn't have 4 args, that means no timeout */
	err_clear();
	if (!getargs(args, "(OOO)", &ifdlist, &ofdlist, &efdlist) )
	  return 0;
	tvp = (struct timeval *)0;
    }
    if ( !is_listobject(ifdlist) || !is_listobject(ofdlist) ||
	!is_listobject(efdlist) ) {
	err_badarg();
	return 0;
    }

    memset((char *)fd2obj, '\0', sizeof(fd2obj));
    
    /* Convert lists to fd_sets, and get maximum fd number */
    if( (imax=list2set(ifdlist, &ifdset, fd2obj)) < 0 )
      return 0;
    if( (omax=list2set(ofdlist, &ofdset, fd2obj)) < 0 )
      return 0;
    if( (emax=list2set(efdlist, &efdset, fd2obj)) < 0 )
      return 0;
    max = imax;
    if ( omax > max ) max = omax;
    if ( emax > max ) max = emax;

    n = select(max, &ifdset, &ofdset, &efdset, tvp);

    if ( n < 0 ) {
	err_errno(SelectError);
	return 0;
    }

    if ( n == 0 )
      imax = omax = emax = 0; /* Speedup hack */

    ifdlist = set2list(&ifdset, imax, fd2obj);
    ofdlist = set2list(&ofdset, omax, fd2obj);
    efdlist = set2list(&efdset, emax, fd2obj);

    return mkvalue("OOO", ifdlist, ofdlist, efdlist);
}


static struct methodlist select_methods[] = {
    { "select",	select_select },
    { 0,	0 },
};


void
initselect()
{
	object *m, *d;
	m = initmodule("select", select_methods);
	d = getmoduledict(m);
	SelectError = newstringobject("select.error");
	if ( SelectError == NULL || dictinsert(d, "error", SelectError) )
	  fatal("Cannot define select.error");
}
