/* On the 68K Mac, when using CFM (Code Fragment Manager),
   <math.h> requires special treatment -- we need to surround it with
   #pragma lib_export off / on...
   This is because MathLib.o is a static library, and exporting its
   symbols doesn't quite work...
   XXX Not sure now...  Seems to be something else going on as well... */

#ifdef SYMANTEC__CFM68K__
#pragma lib_export off
#endif

#include <math.h>

#ifdef SYMANTEC__CFM68K__
#pragma lib_export on
#endif

#if defined(HAVE_HYPOT)
/* Defined in <math.h> */
#else
extern double hypot PROTO((double, double)); /* defined in mathmodule.c */
#endif
