/*
Copyright (c) 2002 Peter O'Gorman <ogorman@users.sourceforge.net>

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


/* Just to prove that it isn't that hard to add Mac calls to your code :)
   This works with pretty much everything, including kde3 xemacs and the gimp,
   I'd guess that it'd work in at least 95% of cases, use this as your starting
   point, rather than the mess that is dlfcn.c, assuming that your code does not
   require ref counting or symbol lookups in dependent libraries
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <limits.h>
#include <mach-o/dyld.h>
#include <AvailabilityMacros.h>
#include "dlfcn.h"

#ifdef CTYPES_DARWIN_DLFCN

#define ERR_STR_LEN 256

#ifndef MAC_OS_X_VERSION_10_3
#define MAC_OS_X_VERSION_10_3 1030
#endif

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_3
#define DARWIN_HAS_DLOPEN
extern void * dlopen(const char *path, int mode) __attribute__((weak_import));
extern void * dlsym(void * handle, const char *symbol) __attribute__((weak_import));
extern const char * dlerror(void) __attribute__((weak_import));
extern int dlclose(void * handle) __attribute__((weak_import));
extern int dladdr(const void *, Dl_info *) __attribute__((weak_import));
#endif /* MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_3 */

#ifndef DARWIN_HAS_DLOPEN
#define dlopen darwin_dlopen
#define dlsym darwin_dlsym
#define dlerror darwin_dlerror
#define dlclose darwin_dlclose
#define dladdr darwin_dladdr
#endif

void * (*ctypes_dlopen)(const char *path, int mode);
void * (*ctypes_dlsym)(void * handle, const char *symbol);
const char * (*ctypes_dlerror)(void);
int (*ctypes_dlclose)(void * handle);
int (*ctypes_dladdr)(const void *, Dl_info *);

#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_3
/* Mac OS X 10.3+ has dlopen, so strip all this dead code to avoid warnings */

static void *dlsymIntern(void *handle, const char *symbol);

static const char *error(int setget, const char *str, ...);

/* Set and get the error string for use by dlerror */
static const char *error(int setget, const char *str, ...)
{
    static char errstr[ERR_STR_LEN];
    static int err_filled = 0;
    const char *retval;
    va_list arg;
    if (setget == 0)
    {
        va_start(arg, str);
        strncpy(errstr, "dlcompat: ", ERR_STR_LEN);
        vsnprintf(errstr + 10, ERR_STR_LEN - 10, str, arg);
        va_end(arg);
        err_filled = 1;
        retval = NULL;
    }
    else
    {
        if (!err_filled)
            retval = NULL;
        else
            retval = errstr;
        err_filled = 0;
    }
    return retval;
}

/* darwin_dlopen */
static void *darwin_dlopen(const char *path, int mode)
{
    void *module = 0;
    NSObjectFileImage ofi = 0;
    NSObjectFileImageReturnCode ofirc;

    /* If we got no path, the app wants the global namespace, use -1 as the marker
       in this case */
    if (!path)
        return (void *)-1;

    /* Create the object file image, works for things linked with the -bundle arg to ld */
    ofirc = NSCreateObjectFileImageFromFile(path, &ofi);
    switch (ofirc)
    {
        case NSObjectFileImageSuccess:
            /* It was okay, so use NSLinkModule to link in the image */
            module = NSLinkModule(ofi, path,
                                                      NSLINKMODULE_OPTION_RETURN_ON_ERROR
                                                      | (mode & RTLD_GLOBAL) ? 0 : NSLINKMODULE_OPTION_PRIVATE
                                                      | (mode & RTLD_LAZY) ? 0 : NSLINKMODULE_OPTION_BINDNOW);
            NSDestroyObjectFileImage(ofi);
            break;
        case NSObjectFileImageInappropriateFile:
            /* It may have been a dynamic library rather than a bundle, try to load it */
            module = (void *)NSAddImage(path, NSADDIMAGE_OPTION_RETURN_ON_ERROR);
            break;
        default:
            /* God knows what we got */
            error(0, "Can not open \"%s\"", path);
            return 0;
    }
    if (!module)
        error(0, "Can not open \"%s\"", path);
    return module;

}

/* dlsymIntern is used by dlsym to find the symbol */
static void *dlsymIntern(void *handle, const char *symbol)
{
    NSSymbol nssym = 0;
    /* If the handle is -1, if is the app global context */
    if (handle == (void *)-1)
    {
        /* Global context, use NSLookupAndBindSymbol */
        if (NSIsSymbolNameDefined(symbol))
        {
            nssym = NSLookupAndBindSymbol(symbol);
        }

    }
    /* Now see if the handle is a struch mach_header* or not, use NSLookupSymbol in image
       for libraries, and NSLookupSymbolInModule for bundles */
    else
    {
        /* Check for both possible magic numbers depending on x86/ppc byte order */
        if ((((struct mach_header *)handle)->magic == MH_MAGIC) ||
            (((struct mach_header *)handle)->magic == MH_CIGAM))
        {
            if (NSIsSymbolNameDefinedInImage((struct mach_header *)handle, symbol))
            {
                nssym = NSLookupSymbolInImage((struct mach_header *)handle,
                                                                          symbol,
                                                                          NSLOOKUPSYMBOLINIMAGE_OPTION_BIND
                                                                          | NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
            }

        }
        else
        {
            nssym = NSLookupSymbolInModule(handle, symbol);
        }
    }
    if (!nssym)
    {
        error(0, "Symbol \"%s\" Not found", symbol);
        return NULL;
    }
    return NSAddressOfSymbol(nssym);
}

static const char *darwin_dlerror(void)
{
    return error(1, (char *)NULL);
}

static int darwin_dlclose(void *handle)
{
    if ((((struct mach_header *)handle)->magic == MH_MAGIC) ||
        (((struct mach_header *)handle)->magic == MH_CIGAM))
    {
        error(0, "Can't remove dynamic libraries on darwin");
        return 0;
    }
    if (!NSUnLinkModule(handle, 0))
    {
        error(0, "unable to unlink module %s", NSNameOfModule(handle));
        return 1;
    }
    return 0;
}


/* dlsym, prepend the underscore and call dlsymIntern */
static void *darwin_dlsym(void *handle, const char *symbol)
{
    static char undersym[257];          /* Saves calls to malloc(3) */
    int sym_len = strlen(symbol);
    void *value = NULL;
    char *malloc_sym = NULL;

    if (sym_len < 256)
    {
        snprintf(undersym, 256, "_%s", symbol);
        value = dlsymIntern(handle, undersym);
    }
    else
    {
        malloc_sym = malloc(sym_len + 2);
        if (malloc_sym)
        {
            sprintf(malloc_sym, "_%s", symbol);
            value = dlsymIntern(handle, malloc_sym);
            free(malloc_sym);
        }
        else
        {
            error(0, "Unable to allocate memory");
        }
    }
    return value;
}

static int darwin_dladdr(const void *handle, Dl_info *info) {
    return 0;
}
#endif /* MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_3 */

#if __GNUC__ < 4
#pragma CALL_ON_LOAD ctypes_dlfcn_init
#else
static void __attribute__ ((constructor)) ctypes_dlfcn_init(void);
static
#endif
void ctypes_dlfcn_init(void) {
    if (dlopen != NULL) {
        ctypes_dlsym = dlsym;
        ctypes_dlopen = dlopen;
        ctypes_dlerror = dlerror;
        ctypes_dlclose = dlclose;
        ctypes_dladdr = dladdr;
    } else {
#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_3
        ctypes_dlsym = darwin_dlsym;
        ctypes_dlopen = darwin_dlopen;
        ctypes_dlerror = darwin_dlerror;
        ctypes_dlclose = darwin_dlclose;
        ctypes_dladdr = darwin_dladdr;
#endif /* MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_3 */
    }
}

#endif /* CTYPES_DARWIN_DLFCN */
