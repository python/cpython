
/* Return the initial module search path. */
/* Used by DOS, OS/2, Windows 3.1, Windows 95/98, Windows NT. */

/* ----------------------------------------------------------------
   PATH RULES FOR WINDOWS:
   This describes how sys.path is formed on Windows.  It describes the 
   functionality, not the implementation (ie, the order in which these 
   are actually fetched is different)

   * Python always adds an empty entry at the start, which corresponds
     to the current directory.

   * If the PYTHONPATH env. var. exists, its entries are added next.

   * We look in the registry for "application paths" - that is, sub-keys
     under the main PythonPath registry key.  These are added next (the
     order of sub-key processing is undefined).
     HKEY_CURRENT_USER is searched and added first.
     HKEY_LOCAL_MACHINE is searched and added next.
     (Note that all known installers only use HKLM, so HKCU is typically
     empty)

   * We attempt to locate the "Python Home" - if the PYTHONHOME env var
     is set, we believe it.  Otherwise, we use the path of our host .EXE's
     to try and locate our "landmark" (lib\\os.py) and deduce our home.
     - If we DO have a Python Home: The relevant sub-directories (Lib, 
       plat-win, lib-tk, etc) are based on the Python Home
     - If we DO NOT have a Python Home, the core Python Path is
       loaded from the registry.  This is the main PythonPath key, 
       and both HKLM and HKCU are combined to form the path)

   * Iff - we can not locate the Python Home, have not had a PYTHONPATH
     specified, and can't locate any Registry entries (ie, we have _nothing_
     we can assume is a good path), a default path with relative entries is 
     used (eg. .\Lib;.\plat-win, etc)


  The end result of all this is:
  * When running python.exe, or any other .exe in the main Python directory
    (either an installed version, or directly from the PCbuild directory),
    the core path is deduced, and the core paths in the registry are
    ignored.  Other "application paths" in the registry are always read.

  * When Python is hosted in another exe (different directory, embedded via 
    COM, etc), the Python Home will not be deduced, so the core path from
    the registry is used.  Other "application paths" in the registry are 
    always read.

  * If Python can't find its home and there is no registry (eg, frozen
    exe, some very strange installation setup) you get a path with
    some default, but relative, paths.

   ---------------------------------------------------------------- */


#include "Python.h"
#include "osdefs.h"

#ifdef MS_WINDOWS
#include <windows.h>
#include <tchar.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */

#include <string.h>

/* Search in some common locations for the associated Python libraries.
 *
 * Py_GetPath() tries to return a sensible Python module search path.
 *
 * The approach is an adaptation for Windows of the strategy used in
 * ../Modules/getpath.c; it uses the Windows Registry as one of its
 * information sources.
 */

#ifndef LANDMARK
#define LANDMARK "lib\\os.py"
#endif

static char prefix[MAXPATHLEN+1];
static char progpath[MAXPATHLEN+1];
static char dllpath[MAXPATHLEN+1];
static char *module_search_path = NULL;


static int
is_sep(char ch)	/* determine if "ch" is a separator character */
{
#ifdef ALTSEP
	return ch == SEP || ch == ALTSEP;
#else
	return ch == SEP;
#endif
}

/* assumes 'dir' null terminated in bounds.  Never writes
   beyond existing terminator.
*/
static void
reduce(char *dir)
{
	size_t i = strlen(dir);
	while (i > 0 && !is_sep(dir[i]))
		--i;
	dir[i] = '\0';
}
	

static int
exists(char *filename)
{
	struct stat buf;
	return stat(filename, &buf) == 0;
}

/* Assumes 'filename' MAXPATHLEN+1 bytes long - 
   may extend 'filename' by one character.
*/
static int
ismodule(char *filename)	/* Is module -- check for .pyc/.pyo too */
{
	if (exists(filename))
		return 1;

	/* Check for the compiled version of prefix. */
	if (strlen(filename) < MAXPATHLEN) {
		strcat(filename, Py_OptimizeFlag ? "o" : "c");
		if (exists(filename))
			return 1;
	}
	return 0;
}

/* Add a path component, by appending stuff to buffer.
   buffer must have at least MAXPATHLEN + 1 bytes allocated, and contain a
   NUL-terminated string with no more than MAXPATHLEN characters (not counting
   the trailing NUL).  It's a fatal error if it contains a string longer than
   that (callers must be careful!).  If these requirements are met, it's
   guaranteed that buffer will still be a NUL-terminated string with no more
   than MAXPATHLEN characters at exit.  If stuff is too long, only as much of
   stuff as fits will be appended.
*/
static void
join(char *buffer, char *stuff)
{
	size_t n, k;
	if (is_sep(stuff[0]))
		n = 0;
	else {
		n = strlen(buffer);
		if (n > 0 && !is_sep(buffer[n-1]) && n < MAXPATHLEN)
			buffer[n++] = SEP;
	}
	if (n > MAXPATHLEN)
		Py_FatalError("buffer overflow in getpathp.c's joinpath()");
	k = strlen(stuff);
	if (n + k > MAXPATHLEN)
		k = MAXPATHLEN - n;
	strncpy(buffer+n, stuff, k);
	buffer[n+k] = '\0';
}

/* gotlandmark only called by search_for_prefix, which ensures
   'prefix' is null terminated in bounds.  join() ensures
   'landmark' can not overflow prefix if too long.
*/
static int
gotlandmark(char *landmark)
{
	int ok;
	Py_ssize_t n;

	n = strlen(prefix);
	join(prefix, landmark);
	ok = ismodule(prefix);
	prefix[n] = '\0';
	return ok;
}

/* assumes argv0_path is MAXPATHLEN+1 bytes long, already \0 term'd. 
   assumption provided by only caller, calculate_path() */
static int
search_for_prefix(char *argv0_path, char *landmark)
{
	/* Search from argv0_path, until landmark is found */
	strcpy(prefix, argv0_path);
	do {
		if (gotlandmark(landmark))
			return 1;
		reduce(prefix);
	} while (prefix[0]);
	return 0;
}

#ifdef MS_WINDOWS
#ifdef Py_ENABLE_SHARED

/* a string loaded from the DLL at startup.*/
extern const char *PyWin_DLLVersionString;


/* Load a PYTHONPATH value from the registry.
   Load from either HKEY_LOCAL_MACHINE or HKEY_CURRENT_USER.

   Works in both Unicode and 8bit environments.  Only uses the
   Ex family of functions so it also works with Windows CE.

   Returns NULL, or a pointer that should be freed.

   XXX - this code is pretty strange, as it used to also
   work on Win16, where the buffer sizes werent available
   in advance.  It could be simplied now Win16/Win32s is dead!
*/

static char *
getpythonregpath(HKEY keyBase, int skipcore)
{
	HKEY newKey = 0;
	DWORD dataSize = 0;
	DWORD numKeys = 0;
	LONG rc;
	char *retval = NULL;
	TCHAR *dataBuf = NULL;
	static const TCHAR keyPrefix[] = _T("Software\\Python\\PythonCore\\");
	static const TCHAR keySuffix[] = _T("\\PythonPath");
	size_t versionLen;
	DWORD index;
	TCHAR *keyBuf = NULL;
	TCHAR *keyBufPtr;
	TCHAR **ppPaths = NULL;

	/* Tried to use sysget("winver") but here is too early :-( */
	versionLen = _tcslen(PyWin_DLLVersionString);
	/* Space for all the chars, plus one \0 */
	keyBuf = keyBufPtr = malloc(sizeof(keyPrefix) + 
		                    sizeof(TCHAR)*(versionLen-1) + 
				    sizeof(keySuffix));
	if (keyBuf==NULL) goto done;

	memcpy(keyBufPtr, keyPrefix, sizeof(keyPrefix)-sizeof(TCHAR));
	keyBufPtr += sizeof(keyPrefix)/sizeof(TCHAR) - 1;
	memcpy(keyBufPtr, PyWin_DLLVersionString, versionLen * sizeof(TCHAR));
	keyBufPtr += versionLen;
	/* NULL comes with this one! */
	memcpy(keyBufPtr, keySuffix, sizeof(keySuffix));
	/* Open the root Python key */
	rc=RegOpenKeyEx(keyBase,
	                keyBuf, /* subkey */
	                0, /* reserved */
	                KEY_READ,
	                &newKey);
	if (rc!=ERROR_SUCCESS) goto done;
	/* Find out how big our core buffer is, and how many subkeys we have */
	rc = RegQueryInfoKey(newKey, NULL, NULL, NULL, &numKeys, NULL, NULL, 
	                NULL, NULL, &dataSize, NULL, NULL);
	if (rc!=ERROR_SUCCESS) goto done;
	if (skipcore) dataSize = 0; /* Only count core ones if we want them! */
	/* Allocate a temp array of char buffers, so we only need to loop 
	   reading the registry once
	*/
	ppPaths = malloc( sizeof(TCHAR *) * numKeys );
	if (ppPaths==NULL) goto done;
	memset(ppPaths, 0, sizeof(TCHAR *) * numKeys);
	/* Loop over all subkeys, allocating a temp sub-buffer. */
	for(index=0;index<numKeys;index++) {
		TCHAR keyBuf[MAX_PATH+1];
		HKEY subKey = 0;
		DWORD reqdSize = MAX_PATH+1;
		/* Get the sub-key name */
		DWORD rc = RegEnumKeyEx(newKey, index, keyBuf, &reqdSize,
		                        NULL, NULL, NULL, NULL );
		if (rc!=ERROR_SUCCESS) goto done;
		/* Open the sub-key */
		rc=RegOpenKeyEx(newKey,
						keyBuf, /* subkey */
						0, /* reserved */
						KEY_READ,
						&subKey);
		if (rc!=ERROR_SUCCESS) goto done;
		/* Find the value of the buffer size, malloc, then read it */
		RegQueryValueEx(subKey, NULL, 0, NULL, NULL, &reqdSize);
		if (reqdSize) {
			ppPaths[index] = malloc(reqdSize);
			if (ppPaths[index]) {
				RegQueryValueEx(subKey, NULL, 0, NULL, 
				                (LPBYTE)ppPaths[index], 
				                &reqdSize);
				dataSize += reqdSize + 1; /* 1 for the ";" */
			}
		}
		RegCloseKey(subKey);
	}

	/* return null if no path to return */
	if (dataSize == 0) goto done;

	/* original datasize from RegQueryInfo doesn't include the \0 */
	dataBuf = malloc((dataSize+1) * sizeof(TCHAR));
	if (dataBuf) {
		TCHAR *szCur = dataBuf;
		DWORD reqdSize = dataSize;
		/* Copy our collected strings */
		for (index=0;index<numKeys;index++) {
			if (index > 0) {
				*(szCur++) = _T(';');
				dataSize--;
			}
			if (ppPaths[index]) {
				Py_ssize_t len = _tcslen(ppPaths[index]);
				_tcsncpy(szCur, ppPaths[index], len);
				szCur += len;
				assert(dataSize > (DWORD)len);
				dataSize -= (DWORD)len;
			}
		}
		if (skipcore)
			*szCur = '\0';
		else {
			/* If we have no values, we dont need a ';' */
			if (numKeys) {
				*(szCur++) = _T(';');
				dataSize--;
			}
			/* Now append the core path entries - 
			   this will include the NULL 
			*/
			rc = RegQueryValueEx(newKey, NULL, 0, NULL, 
			                     (LPBYTE)szCur, &dataSize);
		}
		/* And set the result - caller must free 
		   If MBCS, it is fine as is.  If Unicode, allocate new
		   buffer and convert.
		*/
#ifdef UNICODE
		retval = (char *)malloc(reqdSize+1);
		if (retval)
			WideCharToMultiByte(CP_ACP, 0, 
					dataBuf, -1, /* source */ 
					retval, reqdSize+1, /* dest */
					NULL, NULL);
		free(dataBuf);
#else
		retval = dataBuf;
#endif
	}
done:
	/* Loop freeing my temp buffers */
	if (ppPaths) {
		for(index=0;index<numKeys;index++)
			if (ppPaths[index]) free(ppPaths[index]);
		free(ppPaths);
	}
	if (newKey)
		RegCloseKey(newKey);
	if (keyBuf)
		free(keyBuf);
	return retval;
}
#endif /* Py_ENABLE_SHARED */
#endif /* MS_WINDOWS */

static void
get_progpath(void)
{
	extern char *Py_GetProgramName(void);
	char *path = getenv("PATH");
	char *prog = Py_GetProgramName();

#ifdef MS_WINDOWS
	extern HANDLE PyWin_DLLhModule;
#ifdef UNICODE
	WCHAR wprogpath[MAXPATHLEN+1];
	/* Windows documents that GetModuleFileName() will "truncate",
	   but makes no mention of the null terminator.  Play it safe.
	   PLUS Windows itself defines MAX_PATH as the same, but anyway...
	*/
#ifdef Py_ENABLE_SHARED
	wprogpath[MAXPATHLEN]=_T('\0');
	if (PyWin_DLLhModule &&
	    GetModuleFileName(PyWin_DLLhModule, wprogpath, MAXPATHLEN)) {
		WideCharToMultiByte(CP_ACP, 0, 
		                    wprogpath, -1, 
		                    dllpath, MAXPATHLEN+1, 
		                    NULL, NULL);
	}
#else
	dllpath[0] = 0;
#endif
	wprogpath[MAXPATHLEN]=_T('\0');
	if (GetModuleFileName(NULL, wprogpath, MAXPATHLEN)) {
		WideCharToMultiByte(CP_ACP, 0, 
		                    wprogpath, -1, 
		                    progpath, MAXPATHLEN+1, 
		                    NULL, NULL);
		return;
	}
#else
	/* static init of progpath ensures final char remains \0 */
#ifdef Py_ENABLE_SHARED
	if (PyWin_DLLhModule)
		if (!GetModuleFileName(PyWin_DLLhModule, dllpath, MAXPATHLEN))
			dllpath[0] = 0;
#else
	dllpath[0] = 0;
#endif
	if (GetModuleFileName(NULL, progpath, MAXPATHLEN))
		return;
#endif
#endif
	if (prog == NULL || *prog == '\0')
		prog = "python";

	/* If there is no slash in the argv0 path, then we have to
	 * assume python is on the user's $PATH, since there's no
	 * other way to find a directory to start the search from.  If
	 * $PATH isn't exported, you lose.
	 */
#ifdef ALTSEP
	if (strchr(prog, SEP) || strchr(prog, ALTSEP))
#else
	if (strchr(prog, SEP))
#endif
		strncpy(progpath, prog, MAXPATHLEN);
	else if (path) {
		while (1) {
			char *delim = strchr(path, DELIM);

			if (delim) {
				size_t len = delim - path;
				/* ensure we can't overwrite buffer */
				len = min(MAXPATHLEN,len);
				strncpy(progpath, path, len);
				*(progpath + len) = '\0';
			}
			else
				strncpy(progpath, path, MAXPATHLEN);

			/* join() is safe for MAXPATHLEN+1 size buffer */
			join(progpath, prog);
			if (exists(progpath))
				break;

			if (!delim) {
				progpath[0] = '\0';
				break;
			}
			path = delim + 1;
		}
	}
	else
		progpath[0] = '\0';
}

static void
calculate_path(void)
{
	char argv0_path[MAXPATHLEN+1];
	char *buf;
	size_t bufsz;
	char *pythonhome = Py_GetPythonHome();
	char *envpath = Py_GETENV("PYTHONPATH");

#ifdef MS_WINDOWS
	int skiphome, skipdefault;
	char *machinepath = NULL;
	char *userpath = NULL;
	char zip_path[MAXPATHLEN+1];
	size_t len;
#endif

	get_progpath();
	/* progpath guaranteed \0 terminated in MAXPATH+1 bytes. */
	strcpy(argv0_path, progpath);
	reduce(argv0_path);
	if (pythonhome == NULL || *pythonhome == '\0') {
		if (search_for_prefix(argv0_path, LANDMARK))
			pythonhome = prefix;
		else
			pythonhome = NULL;
	}
	else
		strncpy(prefix, pythonhome, MAXPATHLEN);

	if (envpath && *envpath == '\0')
		envpath = NULL;


#ifdef MS_WINDOWS
	/* Calculate zip archive path */
	if (dllpath[0])		/* use name of python DLL */
		strncpy(zip_path, dllpath, MAXPATHLEN);
	else			/* use name of executable program */
		strncpy(zip_path, progpath, MAXPATHLEN);
	zip_path[MAXPATHLEN] = '\0';
	len = strlen(zip_path);
	if (len > 4) {
		zip_path[len-3] = 'z';	/* change ending to "zip" */
		zip_path[len-2] = 'i';
		zip_path[len-1] = 'p';
	}
	else {
		zip_path[0] = 0;
	}
 
	skiphome = pythonhome==NULL ? 0 : 1;
#ifdef Py_ENABLE_SHARED
	machinepath = getpythonregpath(HKEY_LOCAL_MACHINE, skiphome);
	userpath = getpythonregpath(HKEY_CURRENT_USER, skiphome);
#endif
	/* We only use the default relative PYTHONPATH if we havent
	   anything better to use! */
	skipdefault = envpath!=NULL || pythonhome!=NULL || \
		      machinepath!=NULL || userpath!=NULL;
#endif

	/* We need to construct a path from the following parts.
	   (1) the PYTHONPATH environment variable, if set;
	   (2) for Win32, the zip archive file path;
	   (3) for Win32, the machinepath and userpath, if set;
	   (4) the PYTHONPATH config macro, with the leading "."
	       of each component replaced with pythonhome, if set;
	   (5) the directory containing the executable (argv0_path).
	   The length calculation calculates #4 first.
	   Extra rules:
	   - If PYTHONHOME is set (in any way) item (3) is ignored.
	   - If registry values are used, (4) and (5) are ignored.
	*/

	/* Calculate size of return buffer */
	if (pythonhome != NULL) {
		char *p;
		bufsz = 1;	
		for (p = PYTHONPATH; *p; p++) {
			if (*p == DELIM)
				bufsz++; /* number of DELIM plus one */
		}
		bufsz *= strlen(pythonhome);
	}
	else
		bufsz = 0;
	bufsz += strlen(PYTHONPATH) + 1;
	bufsz += strlen(argv0_path) + 1;
#ifdef MS_WINDOWS
	if (userpath)
		bufsz += strlen(userpath) + 1;
	if (machinepath)
		bufsz += strlen(machinepath) + 1;
	bufsz += strlen(zip_path) + 1;
#endif
	if (envpath != NULL)
		bufsz += strlen(envpath) + 1;

	module_search_path = buf = malloc(bufsz);
	if (buf == NULL) {
		/* We can't exit, so print a warning and limp along */
		fprintf(stderr, "Can't malloc dynamic PYTHONPATH.\n");
		if (envpath) {
			fprintf(stderr, "Using environment $PYTHONPATH.\n");
			module_search_path = envpath;
		}
		else {
			fprintf(stderr, "Using default static path.\n");
			module_search_path = PYTHONPATH;
		}
#ifdef MS_WINDOWS
		if (machinepath)
			free(machinepath);
		if (userpath)
			free(userpath);
#endif /* MS_WINDOWS */
		return;
	}

	if (envpath) {
		strcpy(buf, envpath);
		buf = strchr(buf, '\0');
		*buf++ = DELIM;
	}
#ifdef MS_WINDOWS
	if (zip_path[0]) {
		strcpy(buf, zip_path);
		buf = strchr(buf, '\0');
		*buf++ = DELIM;
	}
	if (userpath) {
		strcpy(buf, userpath);
		buf = strchr(buf, '\0');
		*buf++ = DELIM;
		free(userpath);
	}
	if (machinepath) {
		strcpy(buf, machinepath);
		buf = strchr(buf, '\0');
		*buf++ = DELIM;
		free(machinepath);
	}
	if (pythonhome == NULL) {
		if (!skipdefault) {
			strcpy(buf, PYTHONPATH);
			buf = strchr(buf, '\0');
		}
	}
#else
	if (pythonhome == NULL) {
		strcpy(buf, PYTHONPATH);
		buf = strchr(buf, '\0');
	}
#endif /* MS_WINDOWS */
	else {
		char *p = PYTHONPATH;
		char *q;
		size_t n;
		for (;;) {
			q = strchr(p, DELIM);
			if (q == NULL)
				n = strlen(p);
			else
				n = q-p;
			if (p[0] == '.' && is_sep(p[1])) {
				strcpy(buf, pythonhome);
				buf = strchr(buf, '\0');
				p++;
				n--;
			}
			strncpy(buf, p, n);
			buf += n;
			if (q == NULL)
				break;
			*buf++ = DELIM;
			p = q+1;
		}
	}
	if (argv0_path) {
		*buf++ = DELIM;
		strcpy(buf, argv0_path);
		buf = strchr(buf, '\0');
	}
	*buf = '\0';
	/* Now to pull one last hack/trick.  If sys.prefix is
	   empty, then try and find it somewhere on the paths
	   we calculated.  We scan backwards, as our general policy
	   is that Python core directories are at the *end* of
	   sys.path.  We assume that our "lib" directory is
	   on the path, and that our 'prefix' directory is
	   the parent of that.
	*/
	if (*prefix=='\0') {
		char lookBuf[MAXPATHLEN+1];
		char *look = buf - 1; /* 'buf' is at the end of the buffer */
		while (1) {
			Py_ssize_t nchars;
			char *lookEnd = look;
			/* 'look' will end up one character before the
			   start of the path in question - even if this
			   is one character before the start of the buffer
			*/
			while (look >= module_search_path && *look != DELIM)
				look--;
			nchars = lookEnd-look;
			strncpy(lookBuf, look+1, nchars);
			lookBuf[nchars] = '\0';
			/* Up one level to the parent */
			reduce(lookBuf);
			if (search_for_prefix(lookBuf, LANDMARK)) {
				break;
			}
			/* If we are out of paths to search - give up */
			if (look < module_search_path)
				break;
			look--;
		}
	}
}


/* External interface */

char *
Py_GetPath(void)
{
	if (!module_search_path)
		calculate_path();
	return module_search_path;
}

char *
Py_GetPrefix(void)
{
	if (!module_search_path)
		calculate_path();
	return prefix;
}

char *
Py_GetExecPrefix(void)
{
	return Py_GetPrefix();
}

char *
Py_GetProgramFullPath(void)
{
	if (!module_search_path)
		calculate_path();
	return progpath;
}
