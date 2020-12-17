/*
 * tclWinInit.c --
 *
 *	Contains the Windows-specific interpreter initialization functions.
 *
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 * All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclWinInt.h"
#include <winnt.h>
#include <winbase.h>
#include <lmcons.h>

/*
 * GetUserName() is found in advapi32.dll
 */
#ifdef _MSC_VER
#   pragma comment(lib, "advapi32.lib")
#endif

/*
 * The following declaration is a workaround for some Microsoft brain damage.
 * The SYSTEM_INFO structure is different in various releases, even though the
 * layout is the same. So we overlay our own structure on top of it so we can
 * access the interesting slots in a uniform way.
 */

typedef struct {
    WORD wProcessorArchitecture;
    WORD wReserved;
} OemId;

/*
 * The following macros are missing from some versions of winnt.h.
 */

#ifndef PROCESSOR_ARCHITECTURE_INTEL
#define PROCESSOR_ARCHITECTURE_INTEL		0
#endif
#ifndef PROCESSOR_ARCHITECTURE_MIPS
#define PROCESSOR_ARCHITECTURE_MIPS		1
#endif
#ifndef PROCESSOR_ARCHITECTURE_ALPHA
#define PROCESSOR_ARCHITECTURE_ALPHA		2
#endif
#ifndef PROCESSOR_ARCHITECTURE_PPC
#define PROCESSOR_ARCHITECTURE_PPC		3
#endif
#ifndef PROCESSOR_ARCHITECTURE_SHX
#define PROCESSOR_ARCHITECTURE_SHX		4
#endif
#ifndef PROCESSOR_ARCHITECTURE_ARM
#define PROCESSOR_ARCHITECTURE_ARM		5
#endif
#ifndef PROCESSOR_ARCHITECTURE_IA64
#define PROCESSOR_ARCHITECTURE_IA64		6
#endif
#ifndef PROCESSOR_ARCHITECTURE_ALPHA64
#define PROCESSOR_ARCHITECTURE_ALPHA64		7
#endif
#ifndef PROCESSOR_ARCHITECTURE_MSIL
#define PROCESSOR_ARCHITECTURE_MSIL		8
#endif
#ifndef PROCESSOR_ARCHITECTURE_AMD64
#define PROCESSOR_ARCHITECTURE_AMD64		9
#endif
#ifndef PROCESSOR_ARCHITECTURE_IA32_ON_WIN64
#define PROCESSOR_ARCHITECTURE_IA32_ON_WIN64	10
#endif
#ifndef PROCESSOR_ARCHITECTURE_UNKNOWN
#define PROCESSOR_ARCHITECTURE_UNKNOWN		0xFFFF
#endif


/*
 * Windows version dependend functions
 */
TclWinProcs tclWinProcs;

/*
 * The following arrays contain the human readable strings for the Windows
 * platform and processor values.
 */


#define NUMPLATFORMS 4
static const char *const platforms[NUMPLATFORMS] = {
    "Win32s", "Windows 95", "Windows NT", "Windows CE"
};

#define NUMPROCESSORS 11
static const char *const processors[NUMPROCESSORS] = {
    "intel", "mips", "alpha", "ppc", "shx", "arm", "ia64", "alpha64", "msil",
    "amd64", "ia32_on_win64"
};

/*
 * The default directory in which the init.tcl file is expected to be found.
 */

static TclInitProcessGlobalValueProc	InitializeDefaultLibraryDir;
static ProcessGlobalValue defaultLibraryDir =
	{0, 0, NULL, NULL, InitializeDefaultLibraryDir, NULL, NULL};

static TclInitProcessGlobalValueProc	InitializeSourceLibraryDir;
static ProcessGlobalValue sourceLibraryDir =
	{0, 0, NULL, NULL, InitializeSourceLibraryDir, NULL, NULL};

static void		AppendEnvironment(Tcl_Obj *listPtr, const char *lib);

#if TCL_UTF_MAX < 4
static void		ToUtf(const WCHAR *wSrc, char *dst);
#else
#define ToUtf(wSrc, dst) WideCharToMultiByte(CP_UTF8, 0, wSrc, -1, dst, MAX_PATH * TCL_UTF_MAX, NULL, NULL)
#endif

/*
 *---------------------------------------------------------------------------
 *
 * TclpInitPlatform --
 *
 *	Initialize all the platform-dependant things like signals,
 *	floating-point error handling and sockets.
 *
 *	Called at process initialization time.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

void
TclpInitPlatform(void)
{
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);
    HMODULE handle;

    tclPlatform = TCL_PLATFORM_WINDOWS;

    /*
     * Initialize the winsock library. On Windows XP and higher this
     * can never fail.
     */
    WSAStartup(wVersionRequested, &wsaData);

#ifdef STATIC_BUILD
    /*
     * If we are in a statically linked executable, then we need to explicitly
     * initialize the Windows function tables here since DllMain() will not be
     * invoked.
     */

    TclWinInit(GetModuleHandle(NULL));
#endif

    /*
     * Fill available functions depending on windows version
     */
    handle = GetModuleHandle(TEXT("KERNEL32"));
    tclWinProcs.cancelSynchronousIo =
	    (BOOL (WINAPI *)(HANDLE)) GetProcAddress(handle,
	    "CancelSynchronousIo");
}

/*
 *-------------------------------------------------------------------------
 *
 * TclpInitLibraryPath --
 *
 *	This is the fallback routine that sets the library path if the
 *	application has not set one by the first time it is needed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the library path to an initial value.
 *
 *-------------------------------------------------------------------------
 */

void
TclpInitLibraryPath(
    char **valuePtr,
    int *lengthPtr,
    Tcl_Encoding *encodingPtr)
{
#define LIBRARY_SIZE	    64
    Tcl_Obj *pathPtr;
    char installLib[LIBRARY_SIZE];
    const char *bytes;

    pathPtr = Tcl_NewObj();

    /*
     * Initialize the substring used when locating the script library. The
     * installLib variable computes the script library path relative to the
     * installed DLL.
     */

    sprintf(installLib, "lib/tcl%s", TCL_VERSION);

    /*
     * Look for the library relative to the TCL_LIBRARY env variable. If the
     * last dirname in the TCL_LIBRARY path does not match the last dirname in
     * the installLib variable, use the last dir name of installLib in
     * addition to the orginal TCL_LIBRARY path.
     */

    AppendEnvironment(pathPtr, installLib);

    /*
     * Look for the library in its default location.
     */

    Tcl_ListObjAppendElement(NULL, pathPtr,
	    TclGetProcessGlobalValue(&defaultLibraryDir));

    /*
     * Look for the library in its source checkout location.
     */

    Tcl_ListObjAppendElement(NULL, pathPtr,
	    TclGetProcessGlobalValue(&sourceLibraryDir));

    *encodingPtr = NULL;
    bytes = Tcl_GetStringFromObj(pathPtr, lengthPtr);
    *valuePtr = ckalloc((*lengthPtr) + 1);
    memcpy(*valuePtr, bytes, (size_t)(*lengthPtr)+1);
    Tcl_DecrRefCount(pathPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * AppendEnvironment --
 *
 *	Append the value of the TCL_LIBRARY environment variable onto the path
 *	pointer. If the env variable points to another version of tcl (e.g.
 *	"tcl7.6") also append the path to this version (e.g.,
 *	"tcl7.6/../tcl8.2")
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static void
AppendEnvironment(
    Tcl_Obj *pathPtr,
    const char *lib)
{
    int pathc;
    WCHAR wBuf[MAX_PATH];
    char buf[MAX_PATH * TCL_UTF_MAX];
    Tcl_Obj *objPtr;
    Tcl_DString ds;
    const char **pathv;
    char *shortlib;

    /*
     * The shortlib value needs to be the tail component of the lib path. For
     * example, "lib/tcl8.4" -> "tcl8.4" while "usr/share/tcl8.5" -> "tcl8.5".
     */

    for (shortlib = (char *) &lib[strlen(lib)-1]; shortlib>lib ; shortlib--) {
	if (*shortlib == '/') {
	    if ((unsigned)(shortlib - lib) == strlen(lib) - 1) {
		Tcl_Panic("last character in lib cannot be '/'");
	    }
	    shortlib++;
	    break;
	}
    }
    if (shortlib == lib) {
	Tcl_Panic("no '/' character found in lib");
    }

    /*
     * The "L" preceeding the TCL_LIBRARY string is used to tell VC++ that
     * this is a unicode string.
     */

    if (GetEnvironmentVariableW(L"TCL_LIBRARY", wBuf, MAX_PATH) == 0) {
	buf[0] = '\0';
	GetEnvironmentVariableA("TCL_LIBRARY", buf, MAX_PATH);
    } else {
	ToUtf(wBuf, buf);
    }

    if (buf[0] != '\0') {
	objPtr = Tcl_NewStringObj(buf, -1);
	Tcl_ListObjAppendElement(NULL, pathPtr, objPtr);

	TclWinNoBackslash(buf);
	Tcl_SplitPath(buf, &pathc, &pathv);

	/*
	 * The lstrcmpi() will work even if pathv[pathc-1] is random UTF-8
	 * chars because I know shortlib is ascii.
	 */

	if ((pathc > 0) && (lstrcmpiA(shortlib, pathv[pathc - 1]) != 0)) {
	    /*
	     * TCL_LIBRARY is set but refers to a different tcl installation
	     * than the current version. Try fiddling with the specified
	     * directory to make it refer to this installation by removing the
	     * old "tclX.Y" and substituting the current version string.
	     */

	    pathv[pathc - 1] = shortlib;
	    Tcl_DStringInit(&ds);
	    (void) Tcl_JoinPath(pathc, pathv, &ds);
	    objPtr = TclDStringToObj(&ds);
	} else {
	    objPtr = Tcl_NewStringObj(buf, -1);
	}
	Tcl_ListObjAppendElement(NULL, pathPtr, objPtr);
	ckfree(pathv);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * InitializeDefaultLibraryDir --
 *
 *	Locate the Tcl script library default location relative to the
 *	location of the Tcl DLL.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static void
InitializeDefaultLibraryDir(
    char **valuePtr,
    int *lengthPtr,
    Tcl_Encoding *encodingPtr)
{
    HMODULE hModule = TclWinGetTclInstance();
    WCHAR wName[MAX_PATH + LIBRARY_SIZE];
    char name[(MAX_PATH + LIBRARY_SIZE) * TCL_UTF_MAX];
    char *end, *p;

    if (GetModuleFileNameW(hModule, wName, MAX_PATH) == 0) {
	GetModuleFileNameA(hModule, name, MAX_PATH);
    } else {
	ToUtf(wName, name);
    }

    end = strrchr(name, '\\');
    *end = '\0';
    p = strrchr(name, '\\');
    if (p != NULL) {
	end = p;
    }
    *end = '\\';

    TclWinNoBackslash(name);
    sprintf(end + 1, "lib/tcl%s", TCL_VERSION);
    *lengthPtr = strlen(name);
    *valuePtr = ckalloc(*lengthPtr + 1);
    *encodingPtr = NULL;
    memcpy(*valuePtr, name, (size_t) *lengthPtr + 1);
}

/*
 *---------------------------------------------------------------------------
 *
 * InitializeSourceLibraryDir --
 *
 *	Locate the Tcl script library default location relative to the
 *	location of the Tcl DLL as it exists in the build output directory
 *	associated with the source checkout.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static void
InitializeSourceLibraryDir(
    char **valuePtr,
    int *lengthPtr,
    Tcl_Encoding *encodingPtr)
{
    HMODULE hModule = TclWinGetTclInstance();
    WCHAR wName[MAX_PATH + LIBRARY_SIZE];
    char name[(MAX_PATH + LIBRARY_SIZE) * TCL_UTF_MAX];
    char *end, *p;

    if (GetModuleFileNameW(hModule, wName, MAX_PATH) == 0) {
	GetModuleFileNameA(hModule, name, MAX_PATH);
    } else {
	ToUtf(wName, name);
    }

    end = strrchr(name, '\\');
    *end = '\0';
    p = strrchr(name, '\\');
    if (p != NULL) {
	end = p;
    }
    *end = '\\';

    TclWinNoBackslash(name);
    sprintf(end + 1, "../library");
    *lengthPtr = strlen(name);
    *valuePtr = ckalloc(*lengthPtr + 1);
    *encodingPtr = NULL;
    memcpy(*valuePtr, name, (size_t) *lengthPtr + 1);
}

/*
 *---------------------------------------------------------------------------
 *
 * ToUtf --
 *
 *	Convert a wchar string to a UTF string.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

#if TCL_UTF_MAX < 4
static void
ToUtf(
    const WCHAR *wSrc,
    char *dst)
{
    while (*wSrc != '\0') {
	dst += Tcl_UniCharToUtf(*wSrc, dst);
	wSrc++;
    }
    *dst = '\0';
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 * TclpSetInitialEncodings --
 *
 *	Based on the locale, determine the encoding of the operating system
 *	and the default encoding for newly opened files.
 *
 *	Called at process initialization time, and part way through startup,
 *	we verify that the initial encodings were correctly setup. Depending
 *	on Tcl's environment, there may not have been enough information first
 *	time through (above).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The Tcl library path is converted from native encoding to UTF-8, on
 *	the first call, and the encodings may be changed on first or second
 *	call.
 *
 *---------------------------------------------------------------------------
 */

void
TclpSetInitialEncodings(void)
{
    Tcl_DString encodingName;

    TclpSetInterfaces();
    Tcl_SetSystemEncoding(NULL,
	    Tcl_GetEncodingNameFromEnvironment(&encodingName));
    Tcl_DStringFree(&encodingName);
}

void TclWinSetInterfaces(
    int dummy)			/* Not used. */
{
    TclpSetInterfaces();
}

const char *
Tcl_GetEncodingNameFromEnvironment(
    Tcl_DString *bufPtr)
{
    Tcl_DStringInit(bufPtr);
    Tcl_DStringSetLength(bufPtr, 2+TCL_INTEGER_SPACE);
    wsprintfA(Tcl_DStringValue(bufPtr), "cp%d", GetACP());
    Tcl_DStringSetLength(bufPtr, strlen(Tcl_DStringValue(bufPtr)));
    return Tcl_DStringValue(bufPtr);
}

const char *
TclpGetUserName(
    Tcl_DString *bufferPtr)	/* Uninitialized or free DString filled with
				 * the name of user. */
{
    Tcl_DStringInit(bufferPtr);

    if (TclGetEnv("USERNAME", bufferPtr) == NULL) {
	TCHAR szUserName[UNLEN+1];
	DWORD cchUserNameLen = UNLEN;

	if (!GetUserName(szUserName, &cchUserNameLen)) {
	    return NULL;
	}
	cchUserNameLen--;
	cchUserNameLen *= sizeof(TCHAR);
	Tcl_WinTCharToUtf(szUserName, cchUserNameLen, bufferPtr);
    }
    return Tcl_DStringValue(bufferPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpSetVariables --
 *
 *	Performs platform-specific interpreter initialization related to the
 *	tcl_platform and env variables, and other platform-specific things.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets "tcl_platform", and "env(HOME)" Tcl variables.
 *
 *----------------------------------------------------------------------
 */

void
TclpSetVariables(
    Tcl_Interp *interp)		/* Interp to initialize. */
{
    const char *ptr;
    char buffer[TCL_INTEGER_SPACE * 2];
    union {
	SYSTEM_INFO info;
	OemId oemId;
    } sys;
    static OSVERSIONINFOW osInfo;
    static int osInfoInitialized = 0;
    Tcl_DString ds;

    Tcl_SetVar2Ex(interp, "tclDefaultLibrary", NULL,
	    TclGetProcessGlobalValue(&defaultLibraryDir), TCL_GLOBAL_ONLY);

    if (!osInfoInitialized) {
	HMODULE handle = GetModuleHandle(TEXT("NTDLL"));
	int(__stdcall *getversion)(void *) =
		(int(__stdcall *)(void *)) GetProcAddress(handle, "RtlGetVersion");
	osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
	if (!getversion || getversion(&osInfo)) {
	    GetVersionExW(&osInfo);
	}
	osInfoInitialized = 1;
    }
    GetSystemInfo(&sys.info);

    /*
     * Define the tcl_platform array.
     */

    Tcl_SetVar2(interp, "tcl_platform", "platform", "windows",
	    TCL_GLOBAL_ONLY);
    if (osInfo.dwPlatformId < NUMPLATFORMS) {
	Tcl_SetVar2(interp, "tcl_platform", "os",
		platforms[osInfo.dwPlatformId], TCL_GLOBAL_ONLY);
    }
    wsprintfA(buffer, "%d.%d", osInfo.dwMajorVersion, osInfo.dwMinorVersion);
    Tcl_SetVar2(interp, "tcl_platform", "osVersion", buffer, TCL_GLOBAL_ONLY);
    if (sys.oemId.wProcessorArchitecture < NUMPROCESSORS) {
	Tcl_SetVar2(interp, "tcl_platform", "machine",
		processors[sys.oemId.wProcessorArchitecture],
		TCL_GLOBAL_ONLY);
    }

#ifdef _DEBUG
    /*
     * The existence of the "debug" element of the tcl_platform array
     * indicates that this particular Tcl shell has been compiled with debug
     * information. Using "info exists tcl_platform(debug)" a Tcl script can
     * direct the interpreter to load debug versions of DLLs with the load
     * command.
     */

    Tcl_SetVar2(interp, "tcl_platform", "debug", "1",
	    TCL_GLOBAL_ONLY);
#endif

    /*
     * Set up the HOME environment variable from the HOMEDRIVE & HOMEPATH
     * environment variables, if necessary.
     */

    Tcl_DStringInit(&ds);
    ptr = Tcl_GetVar2(interp, "env", "HOME", TCL_GLOBAL_ONLY);
    if (ptr == NULL) {
	ptr = Tcl_GetVar2(interp, "env", "HOMEDRIVE", TCL_GLOBAL_ONLY);
	if (ptr != NULL) {
	    Tcl_DStringAppend(&ds, ptr, -1);
	}
	ptr = Tcl_GetVar2(interp, "env", "HOMEPATH", TCL_GLOBAL_ONLY);
	if (ptr != NULL) {
	    Tcl_DStringAppend(&ds, ptr, -1);
	}
	if (Tcl_DStringLength(&ds) > 0) {
	    Tcl_SetVar2(interp, "env", "HOME", Tcl_DStringValue(&ds),
		    TCL_GLOBAL_ONLY);
	} else {
	    Tcl_SetVar2(interp, "env", "HOME", "c:\\", TCL_GLOBAL_ONLY);
	}
    }

    /*
     * Initialize the user name from the environment first, since this is much
     * faster than asking the system.
     * Note: cchUserNameLen is number of characters including nul terminator.
     */

    ptr = TclpGetUserName(&ds);
    Tcl_SetVar2(interp, "tcl_platform", "user", ptr ? ptr : "",
	    TCL_GLOBAL_ONLY);
    Tcl_DStringFree(&ds);

    /*
     * Define what the platform PATH separator is. [TIP #315]
     */

    Tcl_SetVar2(interp, "tcl_platform","pathSeparator", ";", TCL_GLOBAL_ONLY);
}

/*
 *----------------------------------------------------------------------
 *
 * TclpFindVariable --
 *
 *	Locate the entry in environ for a given name. On Unix this routine is
 *	case sensitive, on Windows this matches mixed case.
 *
 * Results:
 *	The return value is the index in environ of an entry with the name
 *	"name", or -1 if there is no such entry. The integer at *lengthPtr is
 *	filled in with the length of name (if a matching entry is found) or
 *	the length of the environ array (if no matching entry is found).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclpFindVariable(
    const char *name,		/* Name of desired environment variable
				 * (UTF-8). */
    int *lengthPtr)		/* Used to return length of name (for
				 * successful searches) or number of non-NULL
				 * entries in environ (for unsuccessful
				 * searches). */
{
    int i, length, result = -1;
    register const char *env, *p1, *p2;
    char *envUpper, *nameUpper;
    Tcl_DString envString;

    /*
     * Convert the name to all upper case for the case insensitive comparison.
     */

    length = strlen(name);
    nameUpper = ckalloc(length + 1);
    memcpy(nameUpper, name, (size_t) length+1);
    Tcl_UtfToUpper(nameUpper);

    Tcl_DStringInit(&envString);
    for (i = 0, env = environ[i]; env != NULL; i++, env = environ[i]) {
	/*
	 * Chop the env string off after the equal sign, then Convert the name
	 * to all upper case, so we do not have to convert all the characters
	 * after the equal sign.
	 */

	envUpper = Tcl_ExternalToUtfDString(NULL, env, -1, &envString);
	p1 = strchr(envUpper, '=');
	if (p1 == NULL) {
	    continue;
	}
	length = (int) (p1 - envUpper);
	Tcl_DStringSetLength(&envString, length+1);
	Tcl_UtfToUpper(envUpper);

	p1 = envUpper;
	p2 = nameUpper;
	for (; *p2 == *p1; p1++, p2++) {
	    /* NULL loop body. */
	}
	if ((*p1 == '=') && (*p2 == '\0')) {
	    *lengthPtr = length;
	    result = i;
	    goto done;
	}

	Tcl_DStringFree(&envString);
    }

    *lengthPtr = i;

  done:
    Tcl_DStringFree(&envString);
    ckfree(nameUpper);
    return result;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
