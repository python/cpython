/*
 * Author: George V. Neville-Neil
 */

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
