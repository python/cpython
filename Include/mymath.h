/* On the 68K Mac, when using CFM (Code Fragment Manager),
   <math.h> requires special treatment -- we need to surround it with
   #pragma lib_export off / on...
   This is because MathLib.o is a static library, and exporting its
   symbols doesn't quite work...
   XXX Not sure now...  Seems to be something else going on as well... */

#ifdef __CFM68K__
#pragma lib_export off
#endif

#include <math.h>

#ifdef __CFM68K__
#pragma lib_export on
#endif
