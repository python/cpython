/*
 * This file is for MacOSX only. Dispatch to the right architecture include
 * file based on the current archictecture (instead of relying on a symlink
 * created by configure). This makes is possible to build a univeral binary
 * of ctypes in one go.
 */
#if defined(__i386__)

#ifndef X86_DARWIN
#define X86_DARWIN
#endif
#undef POWERPC_DARWIN

#include "../src/x86/ffitarget.h"

#elif defined(__ppc__)

#ifndef POWERPC_DARWIN
#define POWERPC_DARWIN
#endif
#undef X86_DARWIN

#include "../src/powerpc/ffitarget.h"

#endif
