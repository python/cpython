#ifndef BEOS_DL_EXPORT_H
#define BEOS_DL_EXPORT_H

/* There are no declarations here, so no #ifdef __cplusplus...
 *
 * This is the nasty declaration decorations required by certain systems
 * (in our case, BeOS) for dynamic object loading.
 *
 * This trivial header is released under the same license as the rest of
 * Python:
 *
 * Copyright (c) 2000, BeOpen.com.
 * Copyright (c) 1995-2000, Corporation for National Research Initiatives.
 * Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
 * All rights reserved.
 * 
 * See the file "Misc/COPYRIGHT" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * - Chris Herborth (chrish@beoscentral.com)
 *   January 11, 1999
 */

#ifndef DL_EXPORT
#  define DL_EXPORT(RTYPE) __declspec(dllexport) RTYPE
#endif
#ifndef DL_IMPORT
#  ifdef USE_DL_EXPORT
#    define DL_IMPORT(RTYPE) __declspec(dllexport) RTYPE
#  else
#    define DL_IMPORT(RTYPE) __declspec(dllimport) RTYPE
#  endif
#endif

#endif
