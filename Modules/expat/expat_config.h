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

// bpo-30947: Python uses best available entropy sources to
// call XML_SetHashSalt(), expat entropy sources are not needed
#define XML_POOR_ENTROPY 1

#endif /* EXPAT_CONFIG_H */
