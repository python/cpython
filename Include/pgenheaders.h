/* Include files and extern declarations used by most of the parser.
   This is a precompiled header for THINK C. */

#include <stdio.h>
#include <string.h>

#ifdef THINK_C
/* #define THINK_C_3_0			/*** TURN THIS ON FOR THINK C 3.0 ****/
#define label label_
#undef label
#endif

#ifdef THINK_C_3_0
#include <proto.h>
#endif

#ifdef THINK_C
#ifndef THINK_C_3_0
#include <stdlib.h>
#endif
#endif

#include "PROTO.h"
#include "malloc.h"

extern void fatal PROTO((char *));
