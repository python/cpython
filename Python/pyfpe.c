#include "config.h"
#include "pyfpe.h"
/* 
 * The signal handler for SIGFPE is actually declared in an external
 * module fpectl, or as preferred by the user.  These variable
 * definitions are required in order to compile Python without
 * getting missing externals, but to actually handle SIGFPE requires
 * defining a handler and enabling generation of SIGFPE.
 */

#ifdef WANT_SIGFPE_HANDLER
jmp_buf PyFPE_jbuf;
int PyFPE_counter = 0;
double PyFPE_dummy(void *dummy){ return 1.0; }
#else
#ifdef __MWERKS__
/*
 * Metrowerks fails when compiling an empty file, at least in strict ANSI
 * mode. - [cjh]
 */
static double PyFPE_dummy( void * );
static double PyFPE_dummy( void *dummy ) { return 1.0; }
#endif
#endif
