/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Return the initial module search path. */

#include "Python.h"
#include "osdefs.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <strings.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

/* Search in some common locations for the associated Python libraries.
 *
 * Two directories must be found, the platform independent directory
 * (prefix), containing the common .py and .pyc files, and the platform
 * dependent directory (exec_prefix), containing the shared library
 * modules.  Note that prefix and exec_prefix can be the same directory,
 * but for some installations, they are different.
 *
 * Py_GetPath() carries out separate searches for prefix and exec_prefix.
 * Each search tries a number of different locations until a ``landmark''
 * file or directory is found.  If no prefix or exec_prefix is found, a
 * warning message is issued and the preprocessor defined PREFIX and
 * EXEC_PREFIX are used (even though they will not work); python carries on
 * as best as is possible, but most imports will fail.
 *
 * Before any searches are done, the location of the executable is
 * determined.  If argv[0] has one or more slashs in it, it is used
 * unchanged.  Otherwise, it must have been invoked from the shell's path,
 * so we search $PATH for the named executable and use that.  If the
 * executable was not found on $PATH (or there was no $PATH environment
 * variable), the original argv[0] string is used.
 *
 * Next, the executable location is examined to see if it is a symbolic
 * link.  If so, the link is chased (correctly interpreting a relative
 * pathname if one is found) and the directory of the link target is used.
 *
 * Finally, argv0_path is set to the directory containing the executable
 * (i.e. the last component is stripped).
 *
 * With argv0_path in hand, we perform a number of steps.  The same steps
 * are performed for prefix and for exec_prefix, but with a different
 * landmark.
 *
 * Step 1. Are we running python out of the build directory?  This is
 * checked by looking for a different kind of landmark relative to
 * argv0_path.  For prefix, the landmark's path is derived from the VPATH
 * preprocessor variable (taking into account that its value is almost, but
 * not quite, what we need).  For exec_prefix, the landmark is
 * Modules/Setup.  If the landmark is found, we're done.
 *
 * For the remaining steps, the prefix landmark will always be
 * lib/python$VERSION/string.py and the exec_prefix will always be
 * lib/python$VERSION/sharedmodules, where $VERSION is Python's version
 * number as supplied by the Makefile.  Note that this means that no more
 * build directory checking is performed; if the first step did not find
 * the landmarks, the assumption is that python is running from an
 * installed setup.
 *
 * Step 2. See if the $PYTHONHOME environment variable points to the
 * installed location of the Python libraries.  If $PYTHONHOME is set, then
 * it points to prefix and exec_prefix.  $PYTHONHOME can be a single
 * directory, which is used for both, or the prefix and exec_prefix
 * directories separated by a colon.
 *
 * Step 3. Try to find prefix and exec_prefix relative to argv0_path,
 * backtracking up the path until it is exhausted.  This is the most common
 * step to succeed.  Note that if prefix and exec_prefix are different,
 * exec_prefix is more likely to be found; however if exec_prefix is a
 * subdirectory of prefix, both will be found.
 *
 * Step 4. Search the directories pointed to by the preprocessor variables
 * PREFIX and EXEC_PREFIX.  These are supplied by the Makefile but can be
 * passed in as options to the configure script.
 *
 * Step 5. Search some `standard' directories, namely: /usr/local, /usr,
 * then finally /.
 *
 * That's it!
 *
 * Well, almost.  Once we have determined prefix and exec_prefix, the
 * preprocesor variable PYTHONPATH is used to construct a path.  Each
 * relative path on PYTHONPATH is prefixed with prefix.  Then the directory
 * containing the shared library modules is appended.  The environment
 * variable $PYTHONPATH is inserted in front of it all.  Finally, the
 * prefix and exec_prefix globals are tweaked so they reflect the values
 * expected by other code, by stripping the "lib/python$VERSION/..." stuff
 * off.  If either points to the build directory, the globals are reset to
 * the corresponding preprocessor variables (so sys.prefix will reflect the
 * installation location, even though sys.path points into the build
 * directory).  This seems to make more sense given that currently the only
 * known use of sys.prefix and sys.exec_prefix is for the ILU installation
 * process to find the installed Python tree.
 */

#ifndef VERSION
#define VERSION "1.5"
#endif

#ifndef VPATH
#define VPATH "."
#endif

#ifndef PREFIX
#define PREFIX "/usr/local"
#endif

#ifndef EXEC_PREFIX
#define EXEC_PREFIX PREFIX
#endif

#ifndef PYTHONPATH
/* I know this isn't K&R C, but the Makefile specifies it anyway */
#define PYTHONPATH PREFIX "/lib/python" VERSION ":" \
	           PREFIX "/lib/python" VERSION "/test" ":" \
	      EXEC_PREFIX "/lib/python" VERSION "/sharedmodules"
#endif

#ifndef LANDMARK
#define LANDMARK "string.py"
#endif

static char *std_dirs[] = {"/usr/local/", "/usr/", "/", NULL};

static char prefix[MAXPATHLEN+1];
static char exec_prefix[MAXPATHLEN+1];
static char *module_search_path = NULL;
static char lib_python[20]; /* Dynamically set to "lib/python" VERSION */

static void
reduce(dir)
	char *dir;
{
	int i = strlen(dir);
	while (i > 0 && dir[i] != SEP)
		--i;
	dir[i] = '\0';
}
	

static int
exists(filename)
	char *filename;
{
	struct stat buf;
	return stat(filename, &buf) == 0;
}


static void
join(buffer, stuff)
	char *buffer;
	char *stuff;
{
	int n, k;
	if (stuff[0] == SEP)
		n = 0;
	else {
		n = strlen(buffer);
		if (n > 0 && buffer[n-1] != SEP && n < MAXPATHLEN)
			buffer[n++] = SEP;
	}
	k = strlen(stuff);
	if (n + k > MAXPATHLEN)
		k = MAXPATHLEN - n;
	strncpy(buffer+n, stuff, k);
	buffer[n+k] = '\0';
}


static int
search_for_prefix(argv0_path, home)
	char *argv0_path;
	char *home;
{
	int i, n;
	char *vpath;

	/* Check to see if argv[0] is in the build directory */
	strcpy(prefix, argv0_path);
	join(prefix, "Modules/Setup");
	if (exists(prefix)) {
		/* Check VPATH to see if argv0_path is in the build directory.
		 * Complication: the VPATH passed in is relative to the
		 * Modules build directory and points to the Modules source
		 * directory; we need it relative to the build tree and
		 * pointing to the source tree.  Solution: chop off a leading
		 * ".." (but only if it's there -- it could be an absolute
		 * path) and chop off the final component (assuming it's
		 * "Modules").
		 */
		vpath = VPATH;
		if (vpath[0] == '.' && vpath[1] == '.' && vpath[2] == '/')
			vpath += 3;
		strcpy(prefix, argv0_path);
		join(prefix, vpath);
		reduce(prefix);
		join(prefix, "Lib");
		join(prefix, LANDMARK);
		if (exists(prefix))
			return -1;
	}

	if (home) {
		/* Check $PYTHONHOME */
		char *delim;
		strcpy(prefix, home);
		delim = strchr(prefix, DELIM);
		if (delim)
			*delim = '\0';
		join(prefix, lib_python);
		join(prefix, LANDMARK);
		if (exists(prefix))
			return 1;
	}

	/* Search from argv0_path, until root is found */
	strcpy(prefix, argv0_path);
	do {
		n = strlen(prefix);
		join(prefix, lib_python);
		join(prefix, LANDMARK);
		if (exists(prefix))
			return 1;
		prefix[n] = '\0';
		reduce(prefix);
	} while (prefix[0]);

	/* Look at configure's PREFIX */
	strcpy(prefix, PREFIX);
	join(prefix, lib_python);
	join(prefix, LANDMARK);
	if (exists(prefix))
		return 1;

	/* Look at `standard' directories */
	for (i = 0; std_dirs[i]; i++) {
		strcpy(prefix, std_dirs[i]);
		join(prefix, lib_python);
		join(prefix, LANDMARK);
		if (exists(prefix))
			return 1;
	}
	return 0;
}


static int
search_for_exec_prefix(argv0_path, home)
	char *argv0_path;
	char *home;
{
	int i, n;

	/* Check to see if argv[0] is in the build directory */
	strcpy(exec_prefix, argv0_path);
	join(exec_prefix, "Modules/Setup");
	if (exists(exec_prefix)) {
		reduce(exec_prefix);
		return -1;
	}

	if (home) {
		/* Check $PYTHONHOME */
		char *delim;
		delim = strchr(home, DELIM);
		if (delim)
			strcpy(exec_prefix, delim+1);
		else
			strcpy(exec_prefix, home);
		join(exec_prefix, lib_python);
		join(exec_prefix, "sharedmodules");
		if (exists(exec_prefix))
			return 1;
	}

	/* Search from argv0_path, until root is found */
	strcpy(exec_prefix, argv0_path);
	do {
		n = strlen(exec_prefix);
		join(exec_prefix, lib_python);
		join(exec_prefix, "sharedmodules");
		if (exists(exec_prefix))
			return 1;
		exec_prefix[n] = '\0';
		reduce(exec_prefix);
	} while (exec_prefix[0]);

	/* Look at configure's EXEC_PREFIX */
	strcpy(exec_prefix, EXEC_PREFIX);
	join(exec_prefix, lib_python);
	join(exec_prefix, "sharedmodules");
	if (exists(exec_prefix))
		return 1;

	/* Look at `standard' directories */
	for (i = 0; std_dirs[i]; i++) {
		strcpy(exec_prefix, std_dirs[i]);
		join(exec_prefix, lib_python);
		join(exec_prefix, "sharedmodules");
		if (exists(exec_prefix))
			return 1;
	}
	return 0;
}


static void
calculate_path()
{
	extern char *Py_GetProgramName();

	char delimiter[2] = {DELIM, '\0'};
	char separator[2] = {SEP, '\0'};
	char *pythonpath = PYTHONPATH;
	char *rtpypath = getenv("PYTHONPATH");
	char *home = getenv("PYTHONHOME");
	char *path = getenv("PATH");
	char *prog = Py_GetProgramName();
	char argv0_path[MAXPATHLEN+1];
	char progpath[MAXPATHLEN+1];
	int pfound, efound; /* 1 if found; -1 if found build directory */
	char *buf;
	int bufsz;
	int prefixsz;
	char *defpath = pythonpath;

	/* Initialize this dynamically for K&R C */
	sprintf(lib_python, "lib/python%s", VERSION);

	/* If there is no slash in the argv0 path, then we have to
	 * assume python is on the user's $PATH, since there's no
	 * other way to find a directory to start the search from.  If
	 * $PATH isn't exported, you lose.
	 */
	if (strchr(prog, SEP))
		strcpy(progpath, prog);
	else if (path) {
		while (1) {
			char *delim = strchr(path, DELIM);

			if (delim) {
				int len = delim - path;
				strncpy(progpath, path, len);
				*(progpath + len) = '\0';
			}
			else
				strcpy(progpath, path);

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

	strcpy(argv0_path, progpath);
	
#if HAVE_READLINK
	{
		char tmpbuffer[MAXPATHLEN+1];
		int linklen = readlink(progpath, tmpbuffer, MAXPATHLEN);
		if (linklen != -1) {
			/* It's not null terminated! */
			tmpbuffer[linklen] = '\0';
			if (tmpbuffer[0] == SEP)
				strcpy(argv0_path, tmpbuffer);
			else {
				/* Interpret relative to progpath */
				reduce(argv0_path);
				join(argv0_path, tmpbuffer);
			}
		}
	}
#endif /* HAVE_READLINK */

	reduce(argv0_path);

	if (!(pfound = search_for_prefix(argv0_path, home))) {
		fprintf(stderr,
		   "Could not find platform independent libraries <prefix>\n");
		strcpy(prefix, PREFIX);
		join(prefix, lib_python);
	}
	else
		reduce(prefix);
	
	if (!(efound = search_for_exec_prefix(argv0_path, home))) {
		fprintf(stderr,
		"Could not find platform dependent libraries <exec_prefix>\n");
		strcpy(exec_prefix, EXEC_PREFIX);
		join(exec_prefix, "lib/sharedmodules");
	}
	/* If we found EXEC_PREFIX do *not* reduce it!  (Yet.) */

	if (!pfound || !efound)
		fprintf(stderr,
		 "Consider setting $PYTHONHOME to <prefix>[:<exec_prefix>]\n");

	/* Calculate size of return buffer.
	 */
	bufsz = 0;

	if (rtpypath)
		bufsz += strlen(rtpypath) + 1;

	prefixsz = strlen(prefix) + 1;

	while (1) {
		char *delim = strchr(defpath, DELIM);

		if (defpath[0] != SEP)
			/* Paths are relative to prefix */
			bufsz += prefixsz;

		if (delim)
			bufsz += delim - defpath + 1;
		else {
			bufsz += strlen(defpath) + 1;
			break;
		}
		defpath = delim + 1;
	}

	bufsz += strlen(exec_prefix) + 1;

	/* This is the only malloc call in this file */
	buf = malloc(bufsz);

	if (buf == NULL) {
		/* We can't exit, so print a warning and limp along */
		fprintf(stderr, "Not enough memory for dynamic PYTHONPATH.\n");
		fprintf(stderr, "Using default static PYTHONPATH.\n");
		module_search_path = PYTHONPATH;
	}
	else {
		/* Run-time value of $PYTHONPATH goes first */
		if (rtpypath) {
			strcpy(buf, rtpypath);
			strcat(buf, delimiter);
		}
		else
			buf[0] = '\0';

		/* Next goes merge of compile-time $PYTHONPATH with
		 * dynamically located prefix.
		 */
		defpath = pythonpath;
		while (1) {
			char *delim = strchr(defpath, DELIM);

			if (defpath[0] != SEP) {
				strcat(buf, prefix);
				strcat(buf, separator);
			}

			if (delim) {
				int len = delim - defpath + 1;
				int end = strlen(buf) + len;
				strncat(buf, defpath, len);
				*(buf + end) = '\0';
			}
			else {
				strcat(buf, defpath);
				break;
			}
			defpath = delim + 1;
		}
		strcat(buf, delimiter);

		/* Finally, on goes the directory for dynamic-load modules */
		strcat(buf, exec_prefix);

		/* And publish the results */
		module_search_path = buf;
	}

	/* Reduce prefix and exec_prefix to their essence,
	 * e.g. /usr/local/lib/python1.5 is reduced to /usr/local.
	 * If we're loading relative to the build directory,
	 * return the compiled-in defaults instead.
	 */
	if (pfound > 0) {
		reduce(prefix);
		reduce(prefix);
	}
	else
		strcpy(prefix, PREFIX);

	if (efound > 0) {
		reduce(exec_prefix);
		reduce(exec_prefix);
		reduce(exec_prefix);
	}
	else
		strcpy(exec_prefix, EXEC_PREFIX);
}


/* External interface */

char *
Py_GetPath()
{
	if (!module_search_path)
		calculate_path();
	return module_search_path;
}

char *
Py_GetPrefix()
{
	if (!module_search_path)
		calculate_path();
	return prefix;
}

char *
Py_GetExecPrefix()
{
	if (!module_search_path)
		calculate_path();
	return exec_prefix;
}
