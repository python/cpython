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

#ifndef HAVE_HYPOT
extern double hypot Py_PROTO((double, double));
#endif


#if defined(USE_MSL) && defined(__MC68K__)
/* CodeWarrior MSL 2.1.1 has weird define overrides that don't work
** when you take the address of math functions. If I interpret the
** ANSI C standard correctly this is illegal, but I haven't been able
** to convince the MetroWerks folks of this...
*/
#undef acos
#undef asin
#undef atan
#undef atan2
#undef ceil
#undef cos
#undef cosh
#undef exp
#undef fabs
#undef floor
#undef fmod
#undef log
#undef log10
#undef pow
#undef sin
#undef sinh
#undef sqrt
#undef tan
#undef tanh
#define acos acosd
#define asin asind
#define atan atand
#define atan2 atan2d
#define ceil ceild
#define cos cosd
#define cosh coshd
#define exp expd
#define fabs fabsd
#define floor floord
#define fmod fmodd
#define log logd
#define log10 log10d
#define pow powd
#define sin sind
#define sinh sinhd
#define sqrt sqrtd
#define tan tand
#define tanh tanhd
#endif 
