/*	Dispatch to the right ffitarget file. This file is PyObjC specific; in a
	normal build, the build environment copies the file to the right location or
	sets up the right include flags. We want to do neither because that would
	make building fat binaries harder.
*/

#if defined(__i386__) || defined(__x86_64__)
#include "x86-ffitarget.h"
#elif defined(__ppc__) || defined(__ppc64__)
#include "ppc-ffitarget.h"
#else
#error "Unsupported CPU type"
#endif