/*
 * tclUnixInit.c --
 *
 *	Contains the Unix-specific interpreter initialization functions.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright (c) 1999 by Scriptics Corporation.
 * All rights reserved.
 */

#include "tclInt.h"
#include <stddef.h>
#include <locale.h>
#ifdef HAVE_LANGINFO
#   include <langinfo.h>
#   ifdef __APPLE__
#	if defined(HAVE_WEAK_IMPORT) && MAC_OS_X_VERSION_MIN_REQUIRED < 1030
	    /* Support for weakly importing nl_langinfo on Darwin. */
#	    define WEAK_IMPORT_NL_LANGINFO
	    extern char *nl_langinfo(nl_item) WEAK_IMPORT_ATTRIBUTE;
#	endif
#    endif
#endif
#include <sys/resource.h>
#if defined(__FreeBSD__) && defined(__GNUC__)
#   include <floatingpoint.h>
#endif
#if defined(__bsdi__)
#   include <sys/param.h>
#   if _BSDI_VERSION > 199501
#	include <dlfcn.h>
#   endif
#endif

#ifdef __CYGWIN__
DLLIMPORT extern __stdcall unsigned char GetVersionExW(void *);
DLLIMPORT extern __stdcall void *GetModuleHandleW(const void *);
DLLIMPORT extern __stdcall void FreeLibrary(void *);
DLLIMPORT extern __stdcall void *GetProcAddress(void *, const char *);
DLLIMPORT extern __stdcall void GetSystemInfo(void *);

#define NUMPLATFORMS 4
static const char *const platforms[NUMPLATFORMS] = {
    "Win32s", "Windows 95", "Windows NT", "Windows CE"
};

#define NUMPROCESSORS 11
static const char *const processors[NUMPROCESSORS] = {
    "intel", "mips", "alpha", "ppc", "shx", "arm", "ia64", "alpha64", "msil",
    "amd64", "ia32_on_win64"
};

typedef struct {
  union {
    DWORD  dwOemId;
    struct {
      int wProcessorArchitecture;
      int wReserved;
    };
  };
  DWORD     dwPageSize;
  void *lpMinimumApplicationAddress;
  void *lpMaximumApplicationAddress;
  void *dwActiveProcessorMask;
  DWORD     dwNumberOfProcessors;
  DWORD     dwProcessorType;
  DWORD     dwAllocationGranularity;
  int      wProcessorLevel;
  int      wProcessorRevision;
} SYSTEM_INFO;

typedef struct {
  DWORD dwOSVersionInfoSize;
  DWORD dwMajorVersion;
  DWORD dwMinorVersion;
  DWORD dwBuildNumber;
  DWORD dwPlatformId;
  wchar_t szCSDVersion[128];
} OSVERSIONINFOW;
#endif

#ifdef HAVE_COREFOUNDATION
#include <CoreFoundation/CoreFoundation.h>
#endif

/*
 * Tcl tries to use standard and homebrew methods to guess the right encoding
 * on the platform. However, there is always a final fallback, and this value
 * is it. Make sure it is a real Tcl encoding.
 */

#ifndef TCL_DEFAULT_ENCODING
#define TCL_DEFAULT_ENCODING "iso8859-1"
#endif

/*
 * Default directory in which to look for Tcl library scripts. The symbol is
 * defined by Makefile.
 */

static char defaultLibraryDir[sizeof(TCL_LIBRARY)+200] = TCL_LIBRARY;

/*
 * Directory in which to look for packages (each package is typically
 * installed as a subdirectory of this directory). The symbol is defined by
 * Makefile.
 */

static char pkgPath[sizeof(TCL_PACKAGE_PATH)+200] = TCL_PACKAGE_PATH;

/*
 * The following table is used to map from Unix locale strings to encoding
 * files. If HAVE_LANGINFO is defined, then this is a fallback table when the
 * result from nl_langinfo isn't a recognized encoding. Otherwise this is the
 * first list checked for a mapping from env encoding to Tcl encoding name.
 */

typedef struct LocaleTable {
    const char *lang;
    const char *encoding;
} LocaleTable;

/*
 * The table below is sorted for the sake of doing binary searches on it. The
 * indenting reflects different categories of data. The leftmost data
 * represent the encoding names directly implemented by data files in Tcl's
 * default encoding directory. Indented by one TAB are the encoding names that
 * are common alternative spellings. Indented by two TABs are the accumulated
 * "bug fixes" that have been added to deal with the wide variability seen
 * among existing platforms.
 */

static const LocaleTable localeTable[] = {
	    {"",		"iso8859-1"},
		    {"ansi-1251",	"cp1251"},
	    {"ansi_x3.4-1968",	"iso8859-1"},
    {"ascii",		"ascii"},
    {"big5",		"big5"},
    {"cp1250",		"cp1250"},
    {"cp1251",		"cp1251"},
    {"cp1252",		"cp1252"},
    {"cp1253",		"cp1253"},
    {"cp1254",		"cp1254"},
    {"cp1255",		"cp1255"},
    {"cp1256",		"cp1256"},
    {"cp1257",		"cp1257"},
    {"cp1258",		"cp1258"},
    {"cp437",		"cp437"},
    {"cp737",		"cp737"},
    {"cp775",		"cp775"},
    {"cp850",		"cp850"},
    {"cp852",		"cp852"},
    {"cp855",		"cp855"},
    {"cp857",		"cp857"},
    {"cp860",		"cp860"},
    {"cp861",		"cp861"},
    {"cp862",		"cp862"},
    {"cp863",		"cp863"},
    {"cp864",		"cp864"},
    {"cp865",		"cp865"},
    {"cp866",		"cp866"},
    {"cp869",		"cp869"},
    {"cp874",		"cp874"},
    {"cp932",		"cp932"},
    {"cp936",		"cp936"},
    {"cp949",		"cp949"},
    {"cp950",		"cp950"},
    {"dingbats",	"dingbats"},
    {"ebcdic",		"ebcdic"},
    {"euc-cn",		"euc-cn"},
    {"euc-jp",		"euc-jp"},
    {"euc-kr",		"euc-kr"},
		    {"eucjp",		"euc-jp"},
		    {"euckr",		"euc-kr"},
		    {"euctw",		"euc-cn"},
    {"gb12345",		"gb12345"},
    {"gb1988",		"gb1988"},
    {"gb2312",		"gb2312"},
		    {"gb2312-1980",	"gb2312"},
    {"gb2312-raw",	"gb2312-raw"},
		    {"greek8",		"cp869"},
	    {"ibm1250",		"cp1250"},
	    {"ibm1251",		"cp1251"},
	    {"ibm1252",		"cp1252"},
	    {"ibm1253",		"cp1253"},
	    {"ibm1254",		"cp1254"},
	    {"ibm1255",		"cp1255"},
	    {"ibm1256",		"cp1256"},
	    {"ibm1257",		"cp1257"},
	    {"ibm1258",		"cp1258"},
	    {"ibm437",		"cp437"},
	    {"ibm737",		"cp737"},
	    {"ibm775",		"cp775"},
	    {"ibm850",		"cp850"},
	    {"ibm852",		"cp852"},
	    {"ibm855",		"cp855"},
	    {"ibm857",		"cp857"},
	    {"ibm860",		"cp860"},
	    {"ibm861",		"cp861"},
	    {"ibm862",		"cp862"},
	    {"ibm863",		"cp863"},
	    {"ibm864",		"cp864"},
	    {"ibm865",		"cp865"},
	    {"ibm866",		"cp866"},
	    {"ibm869",		"cp869"},
	    {"ibm874",		"cp874"},
	    {"ibm932",		"cp932"},
	    {"ibm936",		"cp936"},
	    {"ibm949",		"cp949"},
	    {"ibm950",		"cp950"},
	    {"iso-2022",	"iso2022"},
	    {"iso-2022-jp",	"iso2022-jp"},
	    {"iso-2022-kr",	"iso2022-kr"},
	    {"iso-8859-1",	"iso8859-1"},
	    {"iso-8859-10",	"iso8859-10"},
	    {"iso-8859-13",	"iso8859-13"},
	    {"iso-8859-14",	"iso8859-14"},
	    {"iso-8859-15",	"iso8859-15"},
	    {"iso-8859-16",	"iso8859-16"},
	    {"iso-8859-2",	"iso8859-2"},
	    {"iso-8859-3",	"iso8859-3"},
	    {"iso-8859-4",	"iso8859-4"},
	    {"iso-8859-5",	"iso8859-5"},
	    {"iso-8859-6",	"iso8859-6"},
	    {"iso-8859-7",	"iso8859-7"},
	    {"iso-8859-8",	"iso8859-8"},
	    {"iso-8859-9",	"iso8859-9"},
    {"iso2022",		"iso2022"},
    {"iso2022-jp",	"iso2022-jp"},
    {"iso2022-kr",	"iso2022-kr"},
    {"iso8859-1",	"iso8859-1"},
    {"iso8859-10",	"iso8859-10"},
    {"iso8859-13",	"iso8859-13"},
    {"iso8859-14",	"iso8859-14"},
    {"iso8859-15",	"iso8859-15"},
    {"iso8859-16",	"iso8859-16"},
    {"iso8859-2",	"iso8859-2"},
    {"iso8859-3",	"iso8859-3"},
    {"iso8859-4",	"iso8859-4"},
    {"iso8859-5",	"iso8859-5"},
    {"iso8859-6",	"iso8859-6"},
    {"iso8859-7",	"iso8859-7"},
    {"iso8859-8",	"iso8859-8"},
    {"iso8859-9",	"iso8859-9"},
		    {"iso88591",	"iso8859-1"},
		    {"iso885915",	"iso8859-15"},
		    {"iso88592",	"iso8859-2"},
		    {"iso88595",	"iso8859-5"},
		    {"iso88596",	"iso8859-6"},
		    {"iso88597",	"iso8859-7"},
		    {"iso88598",	"iso8859-8"},
		    {"iso88599",	"iso8859-9"},
#ifdef hpux
		    {"ja",		"shiftjis"},
#else
		    {"ja",		"euc-jp"},
#endif
		    {"ja_jp",		"euc-jp"},
		    {"ja_jp.euc",	"euc-jp"},
		    {"ja_jp.eucjp",	"euc-jp"},
		    {"ja_jp.jis",	"iso2022-jp"},
		    {"ja_jp.mscode",	"shiftjis"},
		    {"ja_jp.sjis",	"shiftjis"},
		    {"ja_jp.ujis",	"euc-jp"},
		    {"japan",		"euc-jp"},
#ifdef hpux
		    {"japanese",	"shiftjis"},
#else
		    {"japanese",	"euc-jp"},
#endif
		    {"japanese-sjis",	"shiftjis"},
		    {"japanese-ujis",	"euc-jp"},
		    {"japanese.euc",	"euc-jp"},
		    {"japanese.sjis",	"shiftjis"},
    {"jis0201",		"jis0201"},
    {"jis0208",		"jis0208"},
    {"jis0212",		"jis0212"},
		    {"jp_jp",		"shiftjis"},
		    {"ko",		"euc-kr"},
		    {"ko_kr",		"euc-kr"},
		    {"ko_kr.euc",	"euc-kr"},
		    {"ko_kw.euckw",	"euc-kr"},
    {"koi8-r",		"koi8-r"},
    {"koi8-u",		"koi8-u"},
		    {"korean",		"euc-kr"},
    {"ksc5601",		"ksc5601"},
    {"maccenteuro",	"macCentEuro"},
    {"maccroatian",	"macCroatian"},
    {"maccyrillic",	"macCyrillic"},
    {"macdingbats",	"macDingbats"},
    {"macgreek",	"macGreek"},
    {"maciceland",	"macIceland"},
    {"macjapan",	"macJapan"},
    {"macroman",	"macRoman"},
    {"macromania",	"macRomania"},
    {"macthai",		"macThai"},
    {"macturkish",	"macTurkish"},
    {"macukraine",	"macUkraine"},
		    {"roman8",		"iso8859-1"},
		    {"ru",		"iso8859-5"},
		    {"ru_ru",		"iso8859-5"},
		    {"ru_su",		"iso8859-5"},
    {"shiftjis",	"shiftjis"},
		    {"sjis",		"shiftjis"},
    {"symbol",		"symbol"},
    {"tis-620",		"tis-620"},
		    {"tis620",		"tis-620"},
		    {"turkish8",	"cp857"},
		    {"utf8",		"utf-8"},
		    {"zh",		"cp936"},
		    {"zh_cn.gb2312",	"euc-cn"},
		    {"zh_cn.gbk",	"euc-cn"},
		    {"zh_cz.gb2312",	"euc-cn"},
		    {"zh_tw",		"euc-tw"},
		    {"zh_tw.big5",	"big5"},
};

#ifdef HAVE_COREFOUNDATION
static int		MacOSXGetLibraryPath(Tcl_Interp *interp,
			    int maxPathLen, char *tclLibPath);
#endif /* HAVE_COREFOUNDATION */
#if defined(__APPLE__) && (defined(TCL_LOAD_FROM_MEMORY) || ( \
	defined(MAC_OS_X_VERSION_MIN_REQUIRED) && ( \
	(defined(TCL_THREADS) && MAC_OS_X_VERSION_MIN_REQUIRED < 1030) || \
	(defined(__LP64__) && MAC_OS_X_VERSION_MIN_REQUIRED < 1050) || \
	(defined(HAVE_COREFOUNDATION) && MAC_OS_X_VERSION_MIN_REQUIRED < 1050)\
	)))
/*
 * Need to check Darwin release at runtime in tclUnixFCmd.c and tclLoadDyld.c:
 * initialize release global at startup from uname().
 */
#define GET_DARWIN_RELEASE 1
MODULE_SCOPE long tclMacOSXDarwinRelease;
long tclMacOSXDarwinRelease = 0;
#endif


/*
 *---------------------------------------------------------------------------
 *
 * TclpInitPlatform --
 *
 *	Initialize all the platform-dependent things like signals and
 *	floating-point error handling.
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
#ifdef DJGPP
    tclPlatform = TCL_PLATFORM_WINDOWS;
#else
    tclPlatform = TCL_PLATFORM_UNIX;
#endif

    /*
     * Make sure, that the standard FDs exist. [Bug 772288]
     */

    if (TclOSseek(0, (Tcl_SeekOffset) 0, SEEK_CUR) == -1 && errno == EBADF) {
	open("/dev/null", O_RDONLY);
    }
    if (TclOSseek(1, (Tcl_SeekOffset) 0, SEEK_CUR) == -1 && errno == EBADF) {
	open("/dev/null", O_WRONLY);
    }
    if (TclOSseek(2, (Tcl_SeekOffset) 0, SEEK_CUR) == -1 && errno == EBADF) {
	open("/dev/null", O_WRONLY);
    }

    /*
     * The code below causes SIGPIPE (broken pipe) errors to be ignored. This
     * is needed so that Tcl processes don't die if they create child
     * processes (e.g. using "exec" or "open") that terminate prematurely.
     * The signal handler is only set up when the first interpreter is
     * created; after this the application can override the handler with a
     * different one of its own, if it wants.
     */

#ifdef SIGPIPE
    (void) signal(SIGPIPE, SIG_IGN);
#endif /* SIGPIPE */

#if defined(__FreeBSD__) && defined(__GNUC__)
    /*
     * Adjust the rounding mode to be more conventional. Note that FreeBSD
     * only provides the __fpsetreg() used by the following two for the GNU
     * Compiler. When using, say, Intel's icc they break. (Partially based on
     * patch in BSD ports system from root@celsius.bychok.com)
     */

    fpsetround(FP_RN);
    (void) fpsetmask(0L);
#endif

#if defined(__bsdi__) && (_BSDI_VERSION > 199501)
    /*
     * Find local symbols. Don't report an error if we fail.
     */

    (void) dlopen(NULL, RTLD_NOW);			/* INTL: Native. */
#endif

    /*
     * Initialize the C library's locale subsystem. This is required for input
     * methods to work properly on X11. We only do this for LC_CTYPE because
     * that's the necessary one, and we don't want to affect LC_TIME here.
     * The side effect of setting the default locale should be to load any
     * locale specific modules that are needed by X. [BUG: 5422 3345 4236 2522
     * 2521].
     */

    setlocale(LC_CTYPE, "");

    /*
     * In case the initial locale is not "C", ensure that the numeric
     * processing is done in "C" locale regardless. This is needed because Tcl
     * relies on routines like strtod, but should not have locale dependent
     * behavior.
     */

    setlocale(LC_NUMERIC, "C");

#ifdef GET_DARWIN_RELEASE
    {
	struct utsname name;

	if (!uname(&name)) {
	    tclMacOSXDarwinRelease = strtol(name.release, NULL, 10);
	}
    }
#endif
}

/*
 *---------------------------------------------------------------------------
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
#define LIBRARY_SIZE	    32
    Tcl_Obj *pathPtr, *objPtr;
    const char *str;
    Tcl_DString buffer;

    pathPtr = Tcl_NewObj();

    /*
     * Look for the library relative to the TCL_LIBRARY env variable. If the
     * last dirname in the TCL_LIBRARY path does not match the last dirname in
     * the installLib variable, use the last dir name of installLib in
     * addition to the orginal TCL_LIBRARY path.
     */

    str = getenv("TCL_LIBRARY");			/* INTL: Native. */
    Tcl_ExternalToUtfDString(NULL, str, -1, &buffer);
    str = Tcl_DStringValue(&buffer);

    if ((str != NULL) && (str[0] != '\0')) {
	Tcl_DString ds;
	int pathc;
	const char **pathv;
	char installLib[LIBRARY_SIZE];

	Tcl_DStringInit(&ds);

	/*
	 * Initialize the substrings used when locating an executable. The
	 * installLib variable computes the path as though the executable is
	 * installed.
	 */

	sprintf(installLib, "lib/tcl%s", TCL_VERSION);

	/*
	 * If TCL_LIBRARY is set, search there.
	 */

	Tcl_ListObjAppendElement(NULL, pathPtr, Tcl_NewStringObj(str, -1));

	Tcl_SplitPath(str, &pathc, &pathv);
	if ((pathc > 0) && (strcasecmp(installLib + 4, pathv[pathc-1]) != 0)) {
	    /*
	     * If TCL_LIBRARY is set but refers to a different tcl
	     * installation than the current version, try fiddling with the
	     * specified directory to make it refer to this installation by
	     * removing the old "tclX.Y" and substituting the current version
	     * string.
	     */

	    pathv[pathc - 1] = installLib + 4;
	    str = Tcl_JoinPath(pathc, pathv, &ds);
	    Tcl_ListObjAppendElement(NULL, pathPtr, TclDStringToObj(&ds));
	}
	ckfree(pathv);
    }

    /*
     * Finally, look for the library relative to the compiled-in path. This is
     * needed when users install Tcl with an exec-prefix that is different
     * from the prefix.
     */

    {
#ifdef HAVE_COREFOUNDATION
	char tclLibPath[MAXPATHLEN + 1];

	if (MacOSXGetLibraryPath(NULL, MAXPATHLEN, tclLibPath) == TCL_OK) {
	    str = tclLibPath;
	} else
#endif /* HAVE_COREFOUNDATION */
	{
	    /*
	     * TODO: Pull this value from the TIP 59 table.
	     */

	    str = defaultLibraryDir;
	}
	if (str[0] != '\0') {
	    objPtr = Tcl_NewStringObj(str, -1);
	    Tcl_ListObjAppendElement(NULL, pathPtr, objPtr);
	}
    }
    Tcl_DStringFree(&buffer);

    *encodingPtr = Tcl_GetEncoding(NULL, NULL);
    str = Tcl_GetStringFromObj(pathPtr, lengthPtr);
    *valuePtr = ckalloc((*lengthPtr) + 1);
    memcpy(*valuePtr, str, (size_t)(*lengthPtr)+1);
    Tcl_DecrRefCount(pathPtr);
}

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
    Tcl_SetSystemEncoding(NULL,
	    Tcl_GetEncodingNameFromEnvironment(&encodingName));
    Tcl_DStringFree(&encodingName);
}

void
TclpSetInterfaces(void)
{
    /* do nothing */
}

static const char *
SearchKnownEncodings(
    const char *encoding)
{
    int left = 0;
    int right = sizeof(localeTable)/sizeof(LocaleTable);

    while (left < right) {
	int test = (left + right)/2;
	int code = strcmp(localeTable[test].lang, encoding);

	if (code == 0) {
	    return localeTable[test].encoding;
	}
	if (code < 0) {
	    left = test+1;
	} else {
	    right = test-1;
	}
    }
    return NULL;
}

const char *
Tcl_GetEncodingNameFromEnvironment(
    Tcl_DString *bufPtr)
{
    const char *encoding;
    const char *knownEncoding;

    Tcl_DStringInit(bufPtr);

    /*
     * Determine the current encoding from the LC_* or LANG environment
     * variables. We previously used setlocale() to determine the locale, but
     * this does not work on some systems (e.g. Linux/i386 RH 5.0).
     */

#ifdef HAVE_LANGINFO
    if (
#ifdef WEAK_IMPORT_NL_LANGINFO
	    nl_langinfo != NULL &&
#endif
	    setlocale(LC_CTYPE, "") != NULL) {
	Tcl_DString ds;

	/*
	 * Use a DString so we can modify case.
	 */

	Tcl_DStringInit(&ds);
	encoding = Tcl_DStringAppend(&ds, nl_langinfo(CODESET), -1);
	Tcl_UtfToLower(Tcl_DStringValue(&ds));
	knownEncoding = SearchKnownEncodings(encoding);
	if (knownEncoding != NULL) {
	    Tcl_DStringAppend(bufPtr, knownEncoding, -1);
	} else if (NULL != Tcl_GetEncoding(NULL, encoding)) {
	    Tcl_DStringAppend(bufPtr, encoding, -1);
	}
	Tcl_DStringFree(&ds);
	if (Tcl_DStringLength(bufPtr)) {
	    return Tcl_DStringValue(bufPtr);
	}
    }
#endif /* HAVE_LANGINFO */

    /*
     * Classic fallback check. This tries a homebrew algorithm to determine
     * what encoding should be used based on env vars.
     */

    encoding = getenv("LC_ALL");

    if (encoding == NULL || encoding[0] == '\0') {
	encoding = getenv("LC_CTYPE");
    }
    if (encoding == NULL || encoding[0] == '\0') {
	encoding = getenv("LANG");
    }
    if (encoding == NULL || encoding[0] == '\0') {
	encoding = NULL;
    }

    if (encoding != NULL) {
	const char *p;
	Tcl_DString ds;

	Tcl_DStringInit(&ds);
	p = encoding;
	encoding = Tcl_DStringAppend(&ds, p, -1);
	Tcl_UtfToLower(Tcl_DStringValue(&ds));

	knownEncoding = SearchKnownEncodings(encoding);
	if (knownEncoding != NULL) {
	    Tcl_DStringAppend(bufPtr, knownEncoding, -1);
	} else if (NULL != Tcl_GetEncoding(NULL, encoding)) {
	    Tcl_DStringAppend(bufPtr, encoding, -1);
	}
	if (Tcl_DStringLength(bufPtr)) {
	    Tcl_DStringFree(&ds);
	    return Tcl_DStringValue(bufPtr);
	}

	/*
	 * We didn't recognize the full value as an encoding name. If there is
	 * an encoding subfield, we can try to guess from that.
	 */

	for (p = encoding; *p != '\0'; p++) {
	    if (*p == '.') {
		p++;
		break;
	    }
	}
	if (*p != '\0') {
	    knownEncoding = SearchKnownEncodings(p);
	    if (knownEncoding != NULL) {
		Tcl_DStringAppend(bufPtr, knownEncoding, -1);
	    } else if (NULL != Tcl_GetEncoding(NULL, p)) {
		Tcl_DStringAppend(bufPtr, p, -1);
	    }
	}
	Tcl_DStringFree(&ds);
	if (Tcl_DStringLength(bufPtr)) {
	    return Tcl_DStringValue(bufPtr);
	}
    }
    return Tcl_DStringAppend(bufPtr, TCL_DEFAULT_ENCODING, -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpSetVariables --
 *
 *	Performs platform-specific interpreter initialization related to the
 *	tcl_library and tcl_platform variables, and other platform-specific
 *	things.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets "tclDefaultLibrary", "tcl_pkgPath", and "tcl_platform" Tcl
 *	variables.
 *
 *----------------------------------------------------------------------
 */

#if defined(HAVE_COREFOUNDATION) && MAC_OS_X_VERSION_MAX_ALLOWED > 1020
/*
 * Helper because whether CFLocaleCopyCurrent and CFLocaleGetIdentifier are
 * strongly or weakly bound varies by version of OSX, triggering warnings.
 */

static inline void
InitMacLocaleInfoVar(
    CFLocaleRef (*localeCopyCurrent)(void),
    CFStringRef (*localeGetIdentifier)(CFLocaleRef),
    Tcl_Interp *interp)
{
    CFLocaleRef localeRef;
    CFStringRef locale;
    char loc[256];

    if (localeCopyCurrent == NULL || localeGetIdentifier == NULL) {
	return;
    }

    localeRef = localeCopyCurrent();
    if (!localeRef) {
	return;
    }

    locale = localeGetIdentifier(localeRef);
    if (locale && CFStringGetCString(locale, loc, 256,
	    kCFStringEncodingUTF8)) {
	if (!Tcl_CreateNamespace(interp, "::tcl::mac", NULL, NULL)) {
	    Tcl_ResetResult(interp);
	}
	Tcl_SetVar(interp, "::tcl::mac::locale", loc, TCL_GLOBAL_ONLY);
    }
    CFRelease(localeRef);
}
#endif /*defined(HAVE_COREFOUNDATION) && MAC_OS_X_VERSION_MAX_ALLOWED > 1020*/

void
TclpSetVariables(
    Tcl_Interp *interp)
{
#ifdef __CYGWIN__
    SYSTEM_INFO sysInfo;
    static OSVERSIONINFOW osInfo;
    static int osInfoInitialized = 0;
    char buffer[TCL_INTEGER_SPACE * 2];
#elif !defined(NO_UNAME)
    struct utsname name;
#endif
    int unameOK;
    Tcl_DString ds;

#ifdef HAVE_COREFOUNDATION
    char tclLibPath[MAXPATHLEN + 1];

    /*
     * Set msgcat fallback locale to current CFLocale identifier.
     */

#if MAC_OS_X_VERSION_MAX_ALLOWED > 1020
    InitMacLocaleInfoVar(CFLocaleCopyCurrent, CFLocaleGetIdentifier, interp);
#endif /* MAC_OS_X_VERSION_MAX_ALLOWED > 1020 */

    if (MacOSXGetLibraryPath(interp, MAXPATHLEN, tclLibPath) == TCL_OK) {
	const char *str;
	CFBundleRef bundleRef;

	Tcl_SetVar(interp, "tclDefaultLibrary", tclLibPath, TCL_GLOBAL_ONLY);
	Tcl_SetVar(interp, "tcl_pkgPath", tclLibPath, TCL_GLOBAL_ONLY);
	Tcl_SetVar(interp, "tcl_pkgPath", " ",
		TCL_GLOBAL_ONLY | TCL_APPEND_VALUE);

	str = TclGetEnv("DYLD_FRAMEWORK_PATH", &ds);
	if ((str != NULL) && (str[0] != '\0')) {
	    char *p = Tcl_DStringValue(&ds);

	    /*
	     * Convert DYLD_FRAMEWORK_PATH from colon to space separated.
	     */

	    do {
		if (*p == ':') {
		    *p = ' ';
		}
	    } while (*p++);
	    Tcl_SetVar(interp, "tcl_pkgPath", Tcl_DStringValue(&ds),
		    TCL_GLOBAL_ONLY | TCL_APPEND_VALUE);
	    Tcl_SetVar(interp, "tcl_pkgPath", " ",
		    TCL_GLOBAL_ONLY | TCL_APPEND_VALUE);
	    Tcl_DStringFree(&ds);
	}
	bundleRef = CFBundleGetMainBundle();
	if (bundleRef) {
	    CFURLRef frameworksURL;
	    Tcl_StatBuf statBuf;

	    frameworksURL = CFBundleCopyPrivateFrameworksURL(bundleRef);
	    if (frameworksURL) {
		if (CFURLGetFileSystemRepresentation(frameworksURL, TRUE,
			(unsigned char*) tclLibPath, MAXPATHLEN) &&
			! TclOSstat(tclLibPath, &statBuf) &&
			S_ISDIR(statBuf.st_mode)) {
		    Tcl_SetVar(interp, "tcl_pkgPath", tclLibPath,
			    TCL_GLOBAL_ONLY | TCL_APPEND_VALUE);
		    Tcl_SetVar(interp, "tcl_pkgPath", " ",
			    TCL_GLOBAL_ONLY | TCL_APPEND_VALUE);
		}
		CFRelease(frameworksURL);
	    }
	    frameworksURL = CFBundleCopySharedFrameworksURL(bundleRef);
	    if (frameworksURL) {
		if (CFURLGetFileSystemRepresentation(frameworksURL, TRUE,
			(unsigned char*) tclLibPath, MAXPATHLEN) &&
			! TclOSstat(tclLibPath, &statBuf) &&
			S_ISDIR(statBuf.st_mode)) {
		    Tcl_SetVar(interp, "tcl_pkgPath", tclLibPath,
			    TCL_GLOBAL_ONLY | TCL_APPEND_VALUE);
		    Tcl_SetVar(interp, "tcl_pkgPath", " ",
			    TCL_GLOBAL_ONLY | TCL_APPEND_VALUE);
		}
		CFRelease(frameworksURL);
	    }
	}
	Tcl_SetVar(interp, "tcl_pkgPath", pkgPath,
		TCL_GLOBAL_ONLY | TCL_APPEND_VALUE);
    } else
#endif /* HAVE_COREFOUNDATION */
    {
	Tcl_SetVar(interp, "tcl_pkgPath", pkgPath, TCL_GLOBAL_ONLY);
    }

#ifdef DJGPP
    Tcl_SetVar2(interp, "tcl_platform", "platform", "dos", TCL_GLOBAL_ONLY);
#else
    Tcl_SetVar2(interp, "tcl_platform", "platform", "unix", TCL_GLOBAL_ONLY);
#endif

    unameOK = 0;
#ifdef __CYGWIN__
	unameOK = 1;
    if (!osInfoInitialized) {
	HANDLE handle = GetModuleHandleW(L"NTDLL");
	int(__stdcall *getversion)(void *) =
		(int(__stdcall *)(void *))GetProcAddress(handle, "RtlGetVersion");
	osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
	if (!getversion || getversion(&osInfo)) {
	    GetVersionExW(&osInfo);
	}
	osInfoInitialized = 1;
    }

    GetSystemInfo(&sysInfo);

    if (osInfo.dwPlatformId < NUMPLATFORMS) {
	Tcl_SetVar2(interp, "tcl_platform", "os",
		platforms[osInfo.dwPlatformId], TCL_GLOBAL_ONLY);
    }
    sprintf(buffer, "%d.%d", osInfo.dwMajorVersion, osInfo.dwMinorVersion);
    Tcl_SetVar2(interp, "tcl_platform", "osVersion", buffer, TCL_GLOBAL_ONLY);
    if (sysInfo.wProcessorArchitecture < NUMPROCESSORS) {
	Tcl_SetVar2(interp, "tcl_platform", "machine",
		processors[sysInfo.wProcessorArchitecture],
		TCL_GLOBAL_ONLY);
    }

#elif !defined NO_UNAME
    if (uname(&name) >= 0) {
	const char *native;

	unameOK = 1;

	native = Tcl_ExternalToUtfDString(NULL, name.sysname, -1, &ds);
	Tcl_SetVar2(interp, "tcl_platform", "os", native, TCL_GLOBAL_ONLY);
	Tcl_DStringFree(&ds);

	/*
	 * The following code is a special hack to handle differences in the
	 * way version information is returned by uname. On most systems the
	 * full version number is available in name.release. However, under
	 * AIX the major version number is in name.version and the minor
	 * version number is in name.release.
	 */

	if ((strchr(name.release, '.') != NULL)
		|| !isdigit(UCHAR(name.version[0]))) {	/* INTL: digit */
	    Tcl_SetVar2(interp, "tcl_platform", "osVersion", name.release,
		    TCL_GLOBAL_ONLY);
	} else {
#ifdef DJGPP
	    /*
	     * For some obscure reason DJGPP puts major version into
	     * name.release and minor into name.version. As of DJGPP 2.04 this
	     * is documented in djgpp libc.info file.
	     */

	    Tcl_SetVar2(interp, "tcl_platform", "osVersion", name.release,
		    TCL_GLOBAL_ONLY);
	    Tcl_SetVar2(interp, "tcl_platform", "osVersion", ".",
		    TCL_GLOBAL_ONLY|TCL_APPEND_VALUE);
	    Tcl_SetVar2(interp, "tcl_platform", "osVersion", name.version,
		    TCL_GLOBAL_ONLY|TCL_APPEND_VALUE);
#else
	    Tcl_SetVar2(interp, "tcl_platform", "osVersion", name.version,
		    TCL_GLOBAL_ONLY);
	    Tcl_SetVar2(interp, "tcl_platform", "osVersion", ".",
		    TCL_GLOBAL_ONLY|TCL_APPEND_VALUE);
	    Tcl_SetVar2(interp, "tcl_platform", "osVersion", name.release,
		    TCL_GLOBAL_ONLY|TCL_APPEND_VALUE);

#endif /* DJGPP */
	}
	Tcl_SetVar2(interp, "tcl_platform", "machine", name.machine,
		TCL_GLOBAL_ONLY);
    }
#endif /* !NO_UNAME */
    if (!unameOK) {
	Tcl_SetVar2(interp, "tcl_platform", "os", "", TCL_GLOBAL_ONLY);
	Tcl_SetVar2(interp, "tcl_platform", "osVersion", "", TCL_GLOBAL_ONLY);
	Tcl_SetVar2(interp, "tcl_platform", "machine", "", TCL_GLOBAL_ONLY);
    }

    /*
     * Copy the username of the real user (according to getuid()) into
     * tcl_platform(user).
     */

    {
	struct passwd *pwEnt = TclpGetPwUid(getuid());
	const char *user;

	if (pwEnt == NULL) {
	    user = "";
	    Tcl_DStringInit(&ds);	/* ensure cleanliness */
	} else {
	    user = Tcl_ExternalToUtfDString(NULL, pwEnt->pw_name, -1, &ds);
	}

	Tcl_SetVar2(interp, "tcl_platform", "user", user, TCL_GLOBAL_ONLY);
	Tcl_DStringFree(&ds);
    }

    /*
     * Define what the platform PATH separator is. [TIP #315]
     */

    Tcl_SetVar2(interp, "tcl_platform","pathSeparator", ":", TCL_GLOBAL_ONLY);
}

/*
 *----------------------------------------------------------------------
 *
 * TclpFindVariable --
 *
 *	Locate the entry in environ for a given name. On Unix this routine is
 *	case sensetive, on Windows this matches mixed case.
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
				 * (native). */
    int *lengthPtr)		/* Used to return length of name (for
				 * successful searches) or number of non-NULL
				 * entries in environ (for unsuccessful
				 * searches). */
{
    int i, result = -1;
    register const char *env, *p1, *p2;
    Tcl_DString envString;

    Tcl_DStringInit(&envString);
    for (i = 0, env = environ[i]; env != NULL; i++, env = environ[i]) {
	p1 = Tcl_ExternalToUtfDString(NULL, env, -1, &envString);
	p2 = name;

	for (; *p2 == *p1; p1++, p2++) {
	    /* NULL loop body. */
	}
	if ((*p1 == '=') && (*p2 == '\0')) {
	    *lengthPtr = p2 - name;
	    result = i;
	    goto done;
	}

	Tcl_DStringFree(&envString);
    }

    *lengthPtr = i;

  done:
    Tcl_DStringFree(&envString);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * MacOSXGetLibraryPath --
 *
 *	If we have a bundle structure for the Tcl installation, then check
 *	there first to see if we can find the libraries there.
 *
 * Results:
 *	TCL_OK if we have found the tcl library; TCL_ERROR otherwise.
 *
 * Side effects:
 *	Same as for Tcl_MacOSXOpenVersionedBundleResources.
 *
 *----------------------------------------------------------------------
 */

#ifdef HAVE_COREFOUNDATION
static int
MacOSXGetLibraryPath(
    Tcl_Interp *interp,
    int maxPathLen,
    char *tclLibPath)
{
    int foundInFramework = TCL_ERROR;

#ifdef TCL_FRAMEWORK
    foundInFramework = Tcl_MacOSXOpenVersionedBundleResources(interp,
	    "com.tcltk.tcllibrary", TCL_FRAMEWORK_VERSION, 0, maxPathLen,
	    tclLibPath);
#endif

    return foundInFramework;
}
#endif /* HAVE_COREFOUNDATION */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
