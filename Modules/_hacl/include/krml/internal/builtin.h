/* Copyright (c) INRIA and Microsoft Corporation. All rights reserved.
   Licensed under the Apache 2.0 License. */

#ifndef __KRML_BUILTIN_H
#define __KRML_BUILTIN_H

/* For alloca, when using KaRaMeL's -falloca */
#if (defined(_WIN32) || defined(_WIN64))
#  include <malloc.h>
#endif

/* If some globals need to be initialized before the main, then karamel will
 * generate and try to link last a function with this type: */
void krmlinit_globals(void);

#endif
