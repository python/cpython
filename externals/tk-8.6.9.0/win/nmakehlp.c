/*
 * ----------------------------------------------------------------------------
 * nmakehlp.c --
 *
 *	This is used to fix limitations within nmake and the environment.
 *
 * Copyright (c) 2002 by David Gravereaux.
 * Copyright (c) 2006 by Pat Thoyts
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ----------------------------------------------------------------------------
 */

#define _CRT_SECURE_NO_DEPRECATE
#include <windows.h>
#define NO_SHLWAPI_GDI
#define NO_SHLWAPI_STREAM
#define NO_SHLWAPI_REG
#include <shlwapi.h>
#pragma comment (lib, "user32.lib")
#pragma comment (lib, "kernel32.lib")
#pragma comment (lib, "shlwapi.lib")
#include <stdio.h>
#include <math.h>

/*
 * This library is required for x64 builds with _some_ versions of MSVC
 */
#if defined(_M_IA64) || defined(_M_AMD64)
#if _MSC_VER >= 1400 && _MSC_VER < 1500
#pragma comment(lib, "bufferoverflowU")
#endif
#endif

/* ISO hack for dumb VC++ */
#ifdef _MSC_VER
#define   snprintf	_snprintf
#endif


/* protos */

static int CheckForCompilerFeature(const char *option);
static int CheckForLinkerFeature(const char **options, int count);
static int IsIn(const char *string, const char *substring);
static int SubstituteFile(const char *substs, const char *filename);
static int QualifyPath(const char *path);
static int LocateDependency(const char *keyfile);
static const char *GetVersionFromFile(const char *filename, const char *match, int numdots);
static DWORD WINAPI ReadFromPipe(LPVOID args);

/* globals */

#define CHUNK	25
#define STATICBUFFERSIZE    1000
typedef struct {
    HANDLE pipe;
    char buffer[STATICBUFFERSIZE];
} pipeinfo;

pipeinfo Out = {INVALID_HANDLE_VALUE, '\0'};
pipeinfo Err = {INVALID_HANDLE_VALUE, '\0'};

/*
 * exitcodes: 0 == no, 1 == yes, 2 == error
 */

int
main(
    int argc,
    char *argv[])
{
    char msg[300];
    DWORD dwWritten;
    int chars;
    char *s;

    /*
     * Make sure children (cl.exe and link.exe) are kept quiet.
     */

    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    /*
     * Make sure the compiler and linker aren't effected by the outside world.
     */

    SetEnvironmentVariable("CL", "");
    SetEnvironmentVariable("LINK", "");

    if (argc > 1 && *argv[1] == '-') {
	switch (*(argv[1]+1)) {
	case 'c':
	    if (argc != 3) {
		chars = snprintf(msg, sizeof(msg) - 1,
		        "usage: %s -c <compiler option>\n"
			"Tests for whether cl.exe supports an option\n"
			"exitcodes: 0 == no, 1 == yes, 2 == error\n", argv[0]);
		WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, chars,
			&dwWritten, NULL);
		return 2;
	    }
	    return CheckForCompilerFeature(argv[2]);
	case 'l':
	    if (argc < 3) {
		chars = snprintf(msg, sizeof(msg) - 1,
	       		"usage: %s -l <linker option> ?<mandatory option> ...?\n"
			"Tests for whether link.exe supports an option\n"
			"exitcodes: 0 == no, 1 == yes, 2 == error\n", argv[0]);
		WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, chars,
			&dwWritten, NULL);
		return 2;
	    }
	    return CheckForLinkerFeature(&argv[2], argc-2);
	case 'f':
	    if (argc == 2) {
		chars = snprintf(msg, sizeof(msg) - 1,
			"usage: %s -f <string> <substring>\n"
			"Find a substring within another\n"
			"exitcodes: 0 == no, 1 == yes, 2 == error\n", argv[0]);
		WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, chars,
			&dwWritten, NULL);
		return 2;
	    } else if (argc == 3) {
		/*
		 * If the string is blank, there is no match.
		 */

		return 0;
	    } else {
		return IsIn(argv[2], argv[3]);
	    }
	case 's':
	    if (argc == 2) {
		chars = snprintf(msg, sizeof(msg) - 1,
			"usage: %s -s <substitutions file> <file>\n"
			"Perform a set of string map type substutitions on a file\n"
			"exitcodes: 0\n",
			argv[0]);
		WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, chars,
			&dwWritten, NULL);
		return 2;
	    }
	    return SubstituteFile(argv[2], argv[3]);
	case 'V':
	    if (argc != 4) {
		chars = snprintf(msg, sizeof(msg) - 1,
		    "usage: %s -V filename matchstring\n"
		    "Extract a version from a file:\n"
		    "eg: pkgIndex.tcl \"package ifneeded http\"",
		    argv[0]);
		WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, chars,
		    &dwWritten, NULL);
		return 0;
	    }
	    s = GetVersionFromFile(argv[2], argv[3], *(argv[1]+2) - '0');
	    if (s && *s) {
		printf("%s\n", s);
		return 0;
	    } else
		return 1; /* Version not found. Return non-0 exit code */

	case 'Q':
	    if (argc != 3) {
		chars = snprintf(msg, sizeof(msg) - 1,
		    "usage: %s -Q path\n"
		    "Emit the fully qualified path\n"
		    "exitcodes: 0 == no, 1 == yes, 2 == error\n", argv[0]);
		WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, chars,
		    &dwWritten, NULL);
		return 2;
	    }
	    return QualifyPath(argv[2]);

	case 'L':
	    if (argc != 3) {
		chars = snprintf(msg, sizeof(msg) - 1,
		    "usage: %s -L keypath\n"
		    "Emit the fully qualified path of directory containing keypath\n"
		    "exitcodes: 0 == success, 1 == not found, 2 == error\n", argv[0]);
		WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, chars,
		    &dwWritten, NULL);
		return 2;
	    }
	    return LocateDependency(argv[2]);
	}
    }
    chars = snprintf(msg, sizeof(msg) - 1,
	    "usage: %s -c|-f|-l|-Q|-s|-V ...\n"
	    "This is a little helper app to equalize shell differences between WinNT and\n"
	    "Win9x and get nmake.exe to accomplish its job.\n",
	    argv[0]);
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, chars, &dwWritten, NULL);
    return 2;
}

static int
CheckForCompilerFeature(
    const char *option)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;
    DWORD threadID;
    char msg[300];
    BOOL ok;
    HANDLE hProcess, h, pipeThreads[2];
    char cmdline[100];

    hProcess = GetCurrentProcess();

    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags   = STARTF_USESTDHANDLES;
    si.hStdInput = INVALID_HANDLE_VALUE;

    ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = FALSE;

    /*
     * Create a non-inheritible pipe.
     */

    CreatePipe(&Out.pipe, &h, &sa, 0);

    /*
     * Dupe the write side, make it inheritible, and close the original.
     */

    DuplicateHandle(hProcess, h, hProcess, &si.hStdOutput, 0, TRUE,
	    DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);

    /*
     * Same as above, but for the error side.
     */

    CreatePipe(&Err.pipe, &h, &sa, 0);
    DuplicateHandle(hProcess, h, hProcess, &si.hStdError, 0, TRUE,
	    DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);

    /*
     * Base command line.
     */

    lstrcpy(cmdline, "cl.exe -nologo -c -TC -Zs -X -Fp.\\_junk.pch ");

    /*
     * Append our option for testing
     */

    lstrcat(cmdline, option);

    /*
     * Filename to compile, which exists, but is nothing and empty.
     */

    lstrcat(cmdline, " .\\nul");

    ok = CreateProcess(
	    NULL,	    /* Module name. */
	    cmdline,	    /* Command line. */
	    NULL,	    /* Process handle not inheritable. */
	    NULL,	    /* Thread handle not inheritable. */
	    TRUE,	    /* yes, inherit handles. */
	    DETACHED_PROCESS, /* No console for you. */
	    NULL,	    /* Use parent's environment block. */
	    NULL,	    /* Use parent's starting directory. */
	    &si,	    /* Pointer to STARTUPINFO structure. */
	    &pi);	    /* Pointer to PROCESS_INFORMATION structure. */

    if (!ok) {
	DWORD err = GetLastError();
	int chars = snprintf(msg, sizeof(msg) - 1,
		"Tried to launch: \"%s\", but got error [%u]: ", cmdline, err);

	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS|
		FORMAT_MESSAGE_MAX_WIDTH_MASK, 0L, err, 0, (LPVOID)&msg[chars],
		(300-chars), 0);
	WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, lstrlen(msg), &err,NULL);
	return 2;
    }

    /*
     * Close our references to the write handles that have now been inherited.
     */

    CloseHandle(si.hStdOutput);
    CloseHandle(si.hStdError);

    WaitForInputIdle(pi.hProcess, 5000);
    CloseHandle(pi.hThread);

    /*
     * Start the pipe reader threads.
     */

    pipeThreads[0] = CreateThread(NULL, 0, ReadFromPipe, &Out, 0, &threadID);
    pipeThreads[1] = CreateThread(NULL, 0, ReadFromPipe, &Err, 0, &threadID);

    /*
     * Block waiting for the process to end.
     */

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);

    /*
     * Wait for our pipe to get done reading, should it be a little slow.
     */

    WaitForMultipleObjects(2, pipeThreads, TRUE, 500);
    CloseHandle(pipeThreads[0]);
    CloseHandle(pipeThreads[1]);

    /*
     * Look for the commandline warning code in both streams.
     *  - in MSVC 6 & 7 we get D4002, in MSVC 8 we get D9002.
     */

    return !(strstr(Out.buffer, "D4002") != NULL
             || strstr(Err.buffer, "D4002") != NULL
             || strstr(Out.buffer, "D9002") != NULL
             || strstr(Err.buffer, "D9002") != NULL
             || strstr(Out.buffer, "D2021") != NULL
             || strstr(Err.buffer, "D2021") != NULL);
}

static int
CheckForLinkerFeature(
    const char **options,
    int count)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;
    DWORD threadID;
    char msg[300];
    BOOL ok;
    HANDLE hProcess, h, pipeThreads[2];
    int i;
    char cmdline[255];

    hProcess = GetCurrentProcess();

    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags   = STARTF_USESTDHANDLES;
    si.hStdInput = INVALID_HANDLE_VALUE;

    ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    /*
     * Create a non-inheritible pipe.
     */

    CreatePipe(&Out.pipe, &h, &sa, 0);

    /*
     * Dupe the write side, make it inheritible, and close the original.
     */

    DuplicateHandle(hProcess, h, hProcess, &si.hStdOutput, 0, TRUE,
	    DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);

    /*
     * Same as above, but for the error side.
     */

    CreatePipe(&Err.pipe, &h, &sa, 0);
    DuplicateHandle(hProcess, h, hProcess, &si.hStdError, 0, TRUE,
	    DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);

    /*
     * Base command line.
     */

    lstrcpy(cmdline, "link.exe -nologo ");

    /*
     * Append our option for testing.
     */

    for (i = 0; i < count; i++) {
	lstrcat(cmdline, " \"");
	lstrcat(cmdline, options[i]);
	lstrcat(cmdline, "\"");
    }

    ok = CreateProcess(
	    NULL,	    /* Module name. */
	    cmdline,	    /* Command line. */
	    NULL,	    /* Process handle not inheritable. */
	    NULL,	    /* Thread handle not inheritable. */
	    TRUE,	    /* yes, inherit handles. */
	    DETACHED_PROCESS, /* No console for you. */
	    NULL,	    /* Use parent's environment block. */
	    NULL,	    /* Use parent's starting directory. */
	    &si,	    /* Pointer to STARTUPINFO structure. */
	    &pi);	    /* Pointer to PROCESS_INFORMATION structure. */

    if (!ok) {
	DWORD err = GetLastError();
	int chars = snprintf(msg, sizeof(msg) - 1,
		"Tried to launch: \"%s\", but got error [%u]: ", cmdline, err);

	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS|
		FORMAT_MESSAGE_MAX_WIDTH_MASK, 0L, err, 0, (LPVOID)&msg[chars],
		(300-chars), 0);
	WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, lstrlen(msg), &err,NULL);
	return 2;
    }

    /*
     * Close our references to the write handles that have now been inherited.
     */

    CloseHandle(si.hStdOutput);
    CloseHandle(si.hStdError);

    WaitForInputIdle(pi.hProcess, 5000);
    CloseHandle(pi.hThread);

    /*
     * Start the pipe reader threads.
     */

    pipeThreads[0] = CreateThread(NULL, 0, ReadFromPipe, &Out, 0, &threadID);
    pipeThreads[1] = CreateThread(NULL, 0, ReadFromPipe, &Err, 0, &threadID);

    /*
     * Block waiting for the process to end.
     */

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);

    /*
     * Wait for our pipe to get done reading, should it be a little slow.
     */

    WaitForMultipleObjects(2, pipeThreads, TRUE, 500);
    CloseHandle(pipeThreads[0]);
    CloseHandle(pipeThreads[1]);

    /*
     * Look for the commandline warning code in the stderr stream.
     */

    return !(strstr(Out.buffer, "LNK1117") != NULL ||
	    strstr(Err.buffer, "LNK1117") != NULL ||
	    strstr(Out.buffer, "LNK4044") != NULL ||
	    strstr(Err.buffer, "LNK4044") != NULL ||
	    strstr(Out.buffer, "LNK4224") != NULL ||
	    strstr(Err.buffer, "LNK4224") != NULL);
}

static DWORD WINAPI
ReadFromPipe(
    LPVOID args)
{
    pipeinfo *pi = (pipeinfo *) args;
    char *lastBuf = pi->buffer;
    DWORD dwRead;
    BOOL ok;

  again:
    if (lastBuf - pi->buffer + CHUNK > STATICBUFFERSIZE) {
	CloseHandle(pi->pipe);
	return (DWORD)-1;
    }
    ok = ReadFile(pi->pipe, lastBuf, CHUNK, &dwRead, 0L);
    if (!ok || dwRead == 0) {
	CloseHandle(pi->pipe);
	return 0;
    }
    lastBuf += dwRead;
    goto again;

    return 0;  /* makes the compiler happy */
}

static int
IsIn(
    const char *string,
    const char *substring)
{
    return (strstr(string, substring) != NULL);
}

/*
 * GetVersionFromFile --
 * 	Looks for a match string in a file and then returns the version
 * 	following the match where a version is anything acceptable to
 * 	package provide or package ifneeded.
 */

static const char *
GetVersionFromFile(
    const char *filename,
    const char *match,
    int numdots)
{
    size_t cbBuffer = 100;
    static char szBuffer[100];
    char *szResult = NULL;
    FILE *fp = fopen(filename, "rt");

    if (fp != NULL) {
	/*
	 * Read data until we see our match string.
	 */

	while (fgets(szBuffer, cbBuffer, fp) != NULL) {
	    LPSTR p, q;

	    p = strstr(szBuffer, match);
	    if (p != NULL) {
		/*
		 * Skip to first digit after the match.
		 */

		p += strlen(match);
		while (*p && !isdigit(*p)) {
		    ++p;
		}

		/*
		 * Find ending whitespace.
		 */

		q = p;
		while (*q && (strchr("0123456789.ab", *q)) && ((!strchr(".ab", *q)
			    && (!strchr("ab", q[-1])) || --numdots))) {
		    ++q;
		}

		memcpy(szBuffer, p, q - p);
		szBuffer[q-p] = 0;
		szResult = szBuffer;
		break;
	    }
	}
	fclose(fp);
    }
    return szResult;
}

/*
 * List helpers for the SubstituteFile function
 */

typedef struct list_item_t {
    struct list_item_t *nextPtr;
    char * key;
    char * value;
} list_item_t;

/* insert a list item into the list (list may be null) */
static list_item_t *
list_insert(list_item_t **listPtrPtr, const char *key, const char *value)
{
    list_item_t *itemPtr = malloc(sizeof(list_item_t));
    if (itemPtr) {
	itemPtr->key = strdup(key);
	itemPtr->value = strdup(value);
	itemPtr->nextPtr = NULL;

	while(*listPtrPtr) {
	    listPtrPtr = &(*listPtrPtr)->nextPtr;
	}
	*listPtrPtr = itemPtr;
    }
    return itemPtr;
}

static void
list_free(list_item_t **listPtrPtr)
{
    list_item_t *tmpPtr, *listPtr = *listPtrPtr;
    while (listPtr) {
	tmpPtr = listPtr;
	listPtr = listPtr->nextPtr;
	free(tmpPtr->key);
	free(tmpPtr->value);
	free(tmpPtr);
    }
}

/*
 * SubstituteFile --
 *	As windows doesn't provide anything useful like sed and it's unreliable
 *	to use the tclsh you are building against (consider x-platform builds -
 *	eg compiling AMD64 target from IX86) we provide a simple substitution
 *	option here to handle autoconf style substitutions.
 *	The substitution file is whitespace and line delimited. The file should
 *	consist of lines matching the regular expression:
 *	  \s*\S+\s+\S*$
 *
 *	Usage is something like:
 *	  nmakehlp -S << $** > $@
 *        @PACKAGE_NAME@ $(PACKAGE_NAME)
 *        @PACKAGE_VERSION@ $(PACKAGE_VERSION)
 *        <<
 */

static int
SubstituteFile(
    const char *substitutions,
    const char *filename)
{
    size_t cbBuffer = 1024;
    static char szBuffer[1024], szCopy[1024];
    char *szResult = NULL;
    list_item_t *substPtr = NULL;
    FILE *fp, *sp;

    fp = fopen(filename, "rt");
    if (fp != NULL) {

	/*
	 * Build a list of substutitions from the first filename
	 */

	sp = fopen(substitutions, "rt");
	if (sp != NULL) {
	    while (fgets(szBuffer, cbBuffer, sp) != NULL) {
		unsigned char *ks, *ke, *vs, *ve;
		ks = (unsigned char*)szBuffer;
		while (ks && *ks && isspace(*ks)) ++ks;
		ke = ks;
		while (ke && *ke && !isspace(*ke)) ++ke;
		vs = ke;
		while (vs && *vs && isspace(*vs)) ++vs;
		ve = vs;
		while (ve && *ve && !(*ve == '\r' || *ve == '\n')) ++ve;
		*ke = 0, *ve = 0;
		list_insert(&substPtr, (char*)ks, (char*)vs);
	    }
	    fclose(sp);
	}

	/* debug: dump the list */
#ifdef _DEBUG
	{
	    int n = 0;
	    list_item_t *p = NULL;
	    for (p = substPtr; p != NULL; p = p->nextPtr, ++n) {
		fprintf(stderr, "% 3d '%s' => '%s'\n", n, p->key, p->value);
	    }
	}
#endif

	/*
	 * Run the substitutions over each line of the input
	 */

	while (fgets(szBuffer, cbBuffer, fp) != NULL) {
	    list_item_t *p = NULL;
	    for (p = substPtr; p != NULL; p = p->nextPtr) {
		char *m = strstr(szBuffer, p->key);
		if (m) {
		    char *cp, *op, *sp;
		    cp = szCopy;
		    op = szBuffer;
		    while (op != m) *cp++ = *op++;
		    sp = p->value;
		    while (sp && *sp) *cp++ = *sp++;
		    op += strlen(p->key);
		    while (*op) *cp++ = *op++;
		    *cp = 0;
		    memcpy(szBuffer, szCopy, sizeof(szCopy));
		}
	    }
	    printf(szBuffer);
	}

	list_free(&substPtr);
    }
    fclose(fp);
    return 0;
}

/*
 * QualifyPath --
 *
 *	This composes the current working directory with a provided path
 *	and returns the fully qualified and normalized path.
 *	Mostly needed to setup paths for testing.
 */

static int
QualifyPath(
    const char *szPath)
{
    char szCwd[MAX_PATH + 1];
    char szTmp[MAX_PATH + 1];
    char *p;
    GetCurrentDirectory(MAX_PATH, szCwd);
    while ((p = strchr(szPath, '/')) && *p)
	*p = '\\';
    PathCombine(szTmp, szCwd, szPath);
    PathCanonicalize(szCwd, szTmp);
    printf("%s\n", szCwd);
    return 0;
}

/*
 * Implements LocateDependency for a single directory. See that command
 * for an explanation.
 * Returns 0 if found after printing the directory.
 * Returns 1 if not found but no errors.
 * Returns 2 on any kind of error
 * Basically, these are used as exit codes for the process.
 */
static int LocateDependencyHelper(const char *dir, const char *keypath)
{
    HANDLE hSearch;
    char path[MAX_PATH+1];
    int dirlen, keylen, ret;
    WIN32_FIND_DATA finfo;

    if (dir == NULL || keypath == NULL)
	return 2; /* Have no real error reporting mechanism into nmake */
    dirlen = strlen(dir);
    if ((dirlen + 3) > sizeof(path))
	return 2;
    strncpy(path, dir, dirlen);
    strncpy(path+dirlen, "\\*", 3);	/* Including terminating \0 */
    keylen = strlen(keypath);

#if 0 /* This function is not available in Visual C++ 6 */
    /*
     * Use numerics 0 -> FindExInfoStandard,
     * 1 -> FindExSearchLimitToDirectories,
     * as these are not defined in Visual C++ 6
     */
    hSearch = FindFirstFileEx(path, 0, &finfo, 1, NULL, 0);
#else
    hSearch = FindFirstFile(path, &finfo);
#endif
    if (hSearch == INVALID_HANDLE_VALUE)
	return 1; /* Not found */

    /* Loop through all subdirs checking if the keypath is under there */
    ret = 1; /* Assume not found */
    do {
	int sublen;
	/*
	 * We need to check it is a directory despite the
	 * FindExSearchLimitToDirectories in the above call. See SDK docs
	 */
	if ((finfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
	    continue;
	sublen = strlen(finfo.cFileName);
	if ((dirlen+1+sublen+1+keylen+1) > sizeof(path))
	    continue;		/* Path does not fit, assume not matched */
	strncpy(path+dirlen+1, finfo.cFileName, sublen);
	path[dirlen+1+sublen] = '\\';
	strncpy(path+dirlen+1+sublen+1, keypath, keylen+1);
	if (PathFileExists(path)) {
	    /* Found a match, print to stdout */
	    path[dirlen+1+sublen] = '\0';
	    QualifyPath(path);
	    ret = 0;
	    break;
	}
    } while (FindNextFile(hSearch, &finfo));
    FindClose(hSearch);
    return ret;
}

/*
 * LocateDependency --
 *
 *	Locates a dependency for a package.
 *        keypath - a relative path within the package directory
 *          that is used to confirm it is the correct directory.
 *	The search path for the package directory is currently only
 *      the parent and grandparent of the current working directory.
 *      If found, the command prints
 *         name_DIRPATH=<full path of located directory>
 *      and returns 0. If not found, does not print anything and returns 1.
 */
static int LocateDependency(const char *keypath)
{
    int i, ret;
    static char *paths[] = {"..", "..\\..", "..\\..\\.."};

    for (i = 0; i < (sizeof(paths)/sizeof(paths[0])); ++i) {
	ret = LocateDependencyHelper(paths[i], keypath);
	if (ret == 0)
	    return ret;
    }
    return ret;
}


/*
 * Local variables:
 *   mode: c
 *   c-basic-offset: 4
 *   fill-column: 78
 *   indent-tabs-mode: t
 *   tab-width: 8
 * End:
 */
