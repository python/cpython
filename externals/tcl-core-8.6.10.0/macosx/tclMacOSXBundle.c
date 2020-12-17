/*
 * tclMacOSXBundle.c --
 *
 *	This file implements functions that inspect CFBundle structures on
 *	MacOS X.
 *
 * Copyright 2001-2009, Apple Inc.
 * Copyright (c) 2003-2009 Daniel A. Steffen <das@users.sourceforge.net>
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclPort.h"

#ifdef HAVE_COREFOUNDATION
#include <CoreFoundation/CoreFoundation.h>

#ifndef TCL_DYLD_USE_DLFCN
/*
 * Use preferred dlfcn API on 10.4 and later
 */
#   if !defined(NO_DLFCN_H) && MAC_OS_X_VERSION_MAX_ALLOWED >= 1040
#	define TCL_DYLD_USE_DLFCN 1
#   else
#	define TCL_DYLD_USE_DLFCN 0
#   endif
#endif /* TCL_DYLD_USE_DLFCN */

#ifndef TCL_DYLD_USE_NSMODULE
/*
 * Use deprecated NSModule API only to support 10.3 and earlier:
 */
#   if MAC_OS_X_VERSION_MIN_REQUIRED < 1040
#	define TCL_DYLD_USE_NSMODULE 1
#   else
#	define TCL_DYLD_USE_NSMODULE 0
#   endif
#endif /* TCL_DYLD_USE_NSMODULE */

#if TCL_DYLD_USE_DLFCN
#include <dlfcn.h>
#if defined(HAVE_WEAK_IMPORT) && MAC_OS_X_VERSION_MIN_REQUIRED < 1040
/*
 * Support for weakly importing dlfcn API.
 */
extern void *		dlsym(void *handle, const char *symbol)
			    WEAK_IMPORT_ATTRIBUTE;
extern char *		dlerror(void) WEAK_IMPORT_ATTRIBUTE;
#endif
#endif /* TCL_DYLD_USE_DLFCN */

#if TCL_DYLD_USE_NSMODULE
#include <mach-o/dyld.h>
#endif

#if (TCL_DYLD_USE_DLFCN && MAC_OS_X_VERSION_MIN_REQUIRED < 1040) || \
	(MAC_OS_X_VERSION_MIN_REQUIRED < 1050)
MODULE_SCOPE long	tclMacOSXDarwinRelease;
#endif

#ifdef TCL_DEBUG_LOAD
#define TclLoadDbgMsg(m, ...) \
    do {								\
	fprintf(stderr, "%s:%d: %s(): " m ".\n",			\
		strrchr(__FILE__, '/')+1, __LINE__, __func__,		\
		##__VA_ARGS__);						\
    } while (0)
#else
#define TclLoadDbgMsg(m, ...)
#endif /* TCL_DEBUG_LOAD */

/*
 * Forward declaration of functions defined in this file:
 */

static short		OpenResourceMap(CFBundleRef bundleRef);

#endif /* HAVE_COREFOUNDATION */

/*
 *----------------------------------------------------------------------
 *
 * OpenResourceMap --
 *
 *	Wrapper that dynamically acquires the address for the function
 *	CFBundleOpenBundleResourceMap before calling it, since it is only
 *	present in full CoreFoundation on Mac OS X and not in CFLite on pure
 *	Darwin. Factored out because it is moderately ugly code.
 *
 *----------------------------------------------------------------------
 */

#ifdef HAVE_COREFOUNDATION

static short
OpenResourceMap(
    CFBundleRef bundleRef)
{
    static int initialized = FALSE;
    static short (*openresourcemap)(CFBundleRef) = NULL;

    if (!initialized) {
#if TCL_DYLD_USE_DLFCN
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1040
	if (tclMacOSXDarwinRelease >= 8)
#endif
	{
	    openresourcemap = dlsym(RTLD_NEXT,
		    "CFBundleOpenBundleResourceMap");
#ifdef TCL_DEBUG_LOAD
	    if (!openresourcemap) {
		const char *errMsg = dlerror();

		TclLoadDbgMsg("dlsym() failed: %s", errMsg);
	    }
#endif /* TCL_DEBUG_LOAD */
	}
	if (!openresourcemap)
#endif /* TCL_DYLD_USE_DLFCN */
	{
#if TCL_DYLD_USE_NSMODULE
	    if (NSIsSymbolNameDefinedWithHint(
		    "_CFBundleOpenBundleResourceMap", "CoreFoundation")) {
		NSSymbol nsSymbol = NSLookupAndBindSymbolWithHint(
			"_CFBundleOpenBundleResourceMap", "CoreFoundation");

		if (nsSymbol) {
		    openresourcemap = NSAddressOfSymbol(nsSymbol);
		}
	    }
#endif /* TCL_DYLD_USE_NSMODULE */
	}
	initialized = TRUE;
    }

    if (openresourcemap) {
	return openresourcemap(bundleRef);
    }
    return -1;
}

#endif /* HAVE_COREFOUNDATION */

/*
 *----------------------------------------------------------------------
 *
 * Tcl_MacOSXOpenBundleResources --
 *
 *	Given the bundle name for a shared library, this routine sets
 *	libraryPath to the Resources/Scripts directory in the framework
 *	package. If hasResourceFile is true, it will also open the main
 *	resource file for the bundle.
 *
 * Results:
 *	TCL_OK if the bundle could be opened, and the Scripts folder found.
 *	TCL_ERROR otherwise.
 *
 * Side effects:
 *	libraryVariableName may be set, and the resource file opened.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_MacOSXOpenBundleResources(
    Tcl_Interp *interp,
    const char *bundleName,
    int hasResourceFile,
    int maxPathLen,
    char *libraryPath)
{
    return Tcl_MacOSXOpenVersionedBundleResources(interp, bundleName, NULL,
	    hasResourceFile, maxPathLen, libraryPath);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_MacOSXOpenVersionedBundleResources --
 *
 *	Given the bundle and version name for a shared library (version name
 *	can be NULL to indicate latest version), this routine sets libraryPath
 *	to the Resources/Scripts directory in the framework package. If
 *	hasResourceFile is true, it will also open the main resource file for
 *	the bundle.
 *
 * Results:
 *	TCL_OK if the bundle could be opened, and the Scripts folder found.
 *	TCL_ERROR otherwise.
 *
 * Side effects:
 *	libraryVariableName may be set, and the resource file opened.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_MacOSXOpenVersionedBundleResources(
    Tcl_Interp *interp,
    const char *bundleName,
    const char *bundleVersion,
    int hasResourceFile,
    int maxPathLen,
    char *libraryPath)
{
#ifdef HAVE_COREFOUNDATION
    CFBundleRef bundleRef, versionedBundleRef = NULL;
    CFStringRef bundleNameRef;
    CFURLRef libURL;

    libraryPath[0] = '\0';

    bundleNameRef = CFStringCreateWithCString(NULL, bundleName,
	    kCFStringEncodingUTF8);

    bundleRef = CFBundleGetBundleWithIdentifier(bundleNameRef);
    CFRelease(bundleNameRef);

    if (bundleVersion && bundleRef) {
	/*
	 * Create bundle from bundleVersion subdirectory of 'Versions'.
	 */

	CFURLRef bundleURL = CFBundleCopyBundleURL(bundleRef);

	if (bundleURL) {
	    CFStringRef bundleVersionRef = CFStringCreateWithCString(NULL,
		    bundleVersion, kCFStringEncodingUTF8);

	    if (bundleVersionRef) {
		CFComparisonResult versionComparison = kCFCompareLessThan;
		CFStringRef bundleTailRef = CFURLCopyLastPathComponent(
			bundleURL);

		if (bundleTailRef) {
		    versionComparison = CFStringCompare(bundleTailRef,
			    bundleVersionRef, 0);
		    CFRelease(bundleTailRef);
		}
		if (versionComparison != kCFCompareEqualTo) {
		    CFURLRef versURL = CFURLCreateCopyAppendingPathComponent(
			    NULL, bundleURL, CFSTR("Versions"), TRUE);

		    if (versURL) {
			CFURLRef versionedBundleURL =
				CFURLCreateCopyAppendingPathComponent(
				NULL, versURL, bundleVersionRef, TRUE);

			if (versionedBundleURL) {
			    versionedBundleRef = CFBundleCreate(NULL,
				    versionedBundleURL);
			    if (versionedBundleRef) {
				bundleRef = versionedBundleRef;
			    }
			    CFRelease(versionedBundleURL);
			}
			CFRelease(versURL);
		    }
		}
		CFRelease(bundleVersionRef);
	    }
	    CFRelease(bundleURL);
	}
    }

    if (bundleRef) {
	if (hasResourceFile) {
	    (void) OpenResourceMap(bundleRef);
	}

	libURL = CFBundleCopyResourceURL(bundleRef, CFSTR("Scripts"),
		NULL, NULL);

	if (libURL) {
	    /*
	     * FIXME: This is a quick fix, it is probably not right for
	     * internationalization.
	     */

	    CFURLGetFileSystemRepresentation(libURL, TRUE,
		    (unsigned char *) libraryPath, maxPathLen);
	    CFRelease(libURL);
	}
	if (versionedBundleRef) {
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1050
	    /*
	     * Workaround CFBundle bug in Tiger and earlier. [Bug 2569449]
	     */

	    if (tclMacOSXDarwinRelease >= 9)
#endif
	    {
		CFRelease(versionedBundleRef);
	    }
	}
    }

    if (libraryPath[0]) {
	return TCL_OK;
    }
#endif /* HAVE_COREFOUNDATION */
    return TCL_ERROR;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
