/*
 *
 * File: $Id$
 *
 * Author: George V. Neville-Neil
 *
 * Update History: $Log$
 * Update History: Revision 2.2  1994/01/14 16:55:50  guido
 * Update History: Make more robust against Minix and Mac
 * Update History:
 * Revision 2.1  1994/01/02  23:22:21  guido
 * Added George Neville-Neil's timing module
 *
 * Revision 1.1  93/12/28  13:14:39  gnn
 * Initial revision
 * 
 * 
 *
 *
 */

#ifndef lint
#ifndef THINK_C
static char rcsid [] = "$Header$" ;
#endif /* THINK_C */
#endif /* lint */

#include "allobjects.h"
#include "import.h"
#include "modsupport.h"
#include "ceval.h"

/* Our stuff... */
#include "timing.h"

static object *
start_timing(self, args)
    object *self;
    object *args;
{
    if (!getargs(args, ""))
	return NULL;

    INCREF(None);
    BEGINTIMING;
    return None;
}

static object *
finish_timing(self, args)
    object *self;
    object *args;
{
    if (!getargs(args, ""))
	return NULL;

    ENDTIMING    
    INCREF(None);
    return None;
}

static object *
seconds(self, args)
    object *self;
    object *args;
{
    if (!getargs(args, ""))
	return NULL;

    return newintobject(TIMINGS);

}

static object *
milli(self, args)
    object *self;
    object *args;
{
    if (!getargs(args, ""))
	return NULL;

    return newintobject(TIMINGMS);

}
static object *
micro(self, args)
    object *self;
    object *args;
{
    if (!getargs(args, ""))
	return NULL;

    return newintobject(TIMINGUS);

}


static struct methodlist timing_methods[] = {
   {"start", start_timing},
   {"finish", finish_timing},
   {"seconds", seconds},
   {"milli", milli},
   {"micro", micro},
   {NULL, NULL}
};


void inittiming()
{
    object *m;

    m = initmodule("timing", timing_methods);
   
}
