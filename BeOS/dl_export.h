#ifndef BEOS_DL_EXPORT_H
#define BEOS_DL_EXPORT_H

/* There are no declarations here, so no #ifdef __cplusplus...
 *
 * This is the nasty declaration decorations required by certain systems
 * (in our case, BeOS) for dynamic object loading.
 *
 * This trivial header is released under the same license as the rest of
 * Python.
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
