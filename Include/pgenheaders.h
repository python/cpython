/* Include files and extern declarations used by most of the parser.
   This is a precompiled header for THINK C. */

#include <stdio.h>

#ifdef THINK_C
#define label label_
#include <proto.h>
#undef label
#endif

#include "PROTO.h"
#include "malloc.h"

extern void fatal PROTO((char *));
