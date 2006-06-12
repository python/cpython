/*****************************************************************
  This file should be kept compatible with Python 2.3, see PEP 291.
 *****************************************************************/

#ifndef _CTYPES_DLFCN_H_
#define _CTYPES_DLFCN_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef MS_WIN32

#include <dlfcn.h>

#ifndef CTYPES_DARWIN_DLFCN

#define ctypes_dlsym dlsym
#define ctypes_dlerror dlerror
#define ctypes_dlopen dlopen
#define ctypes_dlclose dlclose
#define ctypes_dladdr dladdr

#endif /* !CTYPES_DARWIN_DLFCN */

#endif /* !MS_WIN32 */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _CTYPES_DLFCN_H_ */
