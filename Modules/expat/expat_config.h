/*
 * Expat configuration for python. This file is not part of the expat
 * distribution.
 */
#ifndef EXPAT_CONFIG_H
#define EXPAT_CONFIG_H 1

#include <pyconfig.h>
#ifdef WORDS_BIGENDIAN
#define BYTEORDER 4321
#else
#define BYTEORDER 1234
#endif

#define HAVE_MEMMOVE 1

#define XML_NS 1
#define XML_DTD 1
#define XML_GE 1
#define XML_CONTEXT_BYTES 1024

#undef HAVE_ARC4RANDOM
#undef HAVE_ARC4RANDOM_BUF
#undef HAVE_GETENTROPY
#undef HAVE_GETRANDOM
#undef HAVE_SYSCALL_GETRANDOM

#endif /* EXPAT_CONFIG_H */
