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

/* select - Module containing unix select(2) call.
Under Unix, the file descriptors are small integers.
Under Win32, select only exists for sockets, and sockets may
have any value except INVALID_SOCKET.
*/

#include "allobjects.h"
#include "modsupport.h"
#include "ceval.h"

#include <sys/types.h>

#ifdef _MSC_VER
#include <winsock.h>
#else
#include "myselect.h" /* Also includes mytime.h */
#define SOCKET int
#endif

static object *SelectError;

typedef struct {	/* list of Python objects and their file descriptor */
	object *obj;
	SOCKET fd;
} pylist;

static int
list2set(list, set, fd2obj)
    object *list;
    fd_set *set;
    pylist fd2obj[FD_SETSIZE + 3];
{
    int i, len, index, max = -1;
    object *o, *filenomethod, *fno;
    SOCKET v;

    index = 0;
    fd2obj[0].obj = (object*)0;	/* set list to zero size */
    
    FD_ZERO(set);
    len = getlistsize(list);
    for( i=0; i<len; i++ ) {
	o = getlistitem(list, i);
	if ( is_intobject(o) ) {
	    v = getintvalue(o);
	} else if ( (filenomethod = getattr(o, "fileno")) != NULL ) {
	    fno = call_object(filenomethod, NULL);
	    DECREF(filenomethod);
	    if ( fno == NULL )
		return -1;
	    if ( !is_intobject(fno) ) {
		err_badarg();
		DECREF(fno);
		return -1;
	    }
	    v = getintvalue(fno);
	    DECREF(fno);
	} else {
	    err_badarg();
	    return -1;
	}
#ifdef _MSC_VER
	max = 0;	/* not used for Win32 */
#else
	if ( v < 0 || v >= FD_SETSIZE ) {
	    err_setstr(ValueError, "filedescriptor out of range in select()");
	    return -1;
	}
	if ( v > max ) max = v;
#endif
	FD_SET(v, set);
	/* add object and its file descriptor to the list */
	if ( index >= FD_SETSIZE ) {
	    err_setstr(ValueError, "too many file descriptors in select()");
	    return -1;
	}
	fd2obj[index].obj = o;
	fd2obj[index].fd = v;
	fd2obj[++index].obj = (object *)0;	/* sentinel */
    }
    return max+1;
}

static object *
set2list(set, fd2obj)
    fd_set *set;
    pylist fd2obj[FD_SETSIZE + 3];
{
    int j, num=0;
    object *list, *o;
    SOCKET fd;

    for(j=0; fd2obj[j].obj; j++)
      if ( FD_ISSET(fd2obj[j].fd, set) )
	num++;
    list = newlistobject(num);
    num = 0;
    for(j=0; fd2obj[j].obj; j++) {
      fd = fd2obj[j].fd;
      if ( FD_ISSET(fd, set) ) {
#ifndef _MSC_VER
	  if ( fd > FD_SETSIZE ) {
	      err_setstr(SystemError,
			 "filedescriptor out of range returned in select()");
	      return NULL;
	  }
#endif
          o = fd2obj[j].obj;
	  INCREF(o);
	  setlistitem(list, num, o);
	  num++;
      }
    }
    return list;
}
    
static object *
select_select(self, args)
    object *self;
    object *args;
{
    pylist rfd2obj[FD_SETSIZE + 3], wfd2obj[FD_SETSIZE + 3], efd2obj[FD_SETSIZE + 3];
    object *ifdlist, *ofdlist, *efdlist;
    object *ret, *tout;
    fd_set ifdset, ofdset, efdset;
    double timeout;
    struct timeval tv, *tvp;
    int seconds;
    int imax, omax, emax, max;
    int n;


    /* Get args. Looks funny because of optional timeout argument */
    if ( getargs(args, "(OOOO)", &ifdlist, &ofdlist, &efdlist, &tout) ) {
	if (tout == None)
	    tvp = (struct timeval *)0;
	else {
	    if (!getargs(tout, "d;timeout must be float or None", &timeout))
		    return NULL;
	    seconds = (int)timeout;
	    timeout = timeout - (double)seconds;
	    tv.tv_sec = seconds;
	    tv.tv_usec = (int)(timeout*1000000.0);
	    tvp = &tv;
	}
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

    /* Convert lists to fd_sets, and get maximum fd number */
    if( (imax=list2set(ifdlist, &ifdset, rfd2obj)) < 0 )
      return 0;
    if( (omax=list2set(ofdlist, &ofdset, wfd2obj)) < 0 )
      return 0;
    if( (emax=list2set(efdlist, &efdset, efd2obj)) < 0 )
      return 0;
    max = imax;
    if ( omax > max ) max = omax;
    if ( emax > max ) max = emax;

    BGN_SAVE
    n = select(max, &ifdset, &ofdset, &efdset, tvp);
    END_SAVE

    if ( n < 0 ) {
	err_errno(SelectError);
	return 0;
    }

    if ( n == 0 ) { /* Speedup hack */
      ifdlist = newlistobject(0);
      ret = mkvalue("OOO", ifdlist, ifdlist, ifdlist);
      XDECREF(ifdlist);
      return ret;
    }

    ifdlist = set2list(&ifdset, rfd2obj);
    ofdlist = set2list(&ofdset, wfd2obj);
    efdlist = set2list(&efdset, efd2obj);
    ret = mkvalue("OOO", ifdlist, ofdlist, efdlist);
    XDECREF(ifdlist);
    XDECREF(ofdlist);
    XDECREF(efdlist);
    return ret;
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
