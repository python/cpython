#include <windows.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

/* This file creates the local getbuildinfo.c file, by
   invoking subwcrev.exe (if found).  This replaces tokens
   int the file with information about the build.
   If this isn't a subversion checkout, or subwcrev isn't
   found, it copies ..\\Modules\\getbuildinfo.c instead.

   Currently, subwcrev.exe is found from the registry entries
   of TortoiseSVN.

   make_buildinfo.exe is called as a pre-build step for pythoncore.
*/

int make_buildinfo2()
{
	struct _stat st;
	HKEY hTortoise;
	char command[500];
	DWORD type, size;
	if (_stat("..\\.svn", &st) < 0)
		return 0;
	/* Allow suppression of subwcrev.exe invocation if a no_subwcrev file is present. */
	if (_stat("no_subwcrev", &st) == 0)
		return 0;
	if (RegOpenKey(HKEY_LOCAL_MACHINE, "Software\\TortoiseSVN", &hTortoise) != ERROR_SUCCESS &&
	    RegOpenKey(HKEY_CURRENT_USER, "Software\\TortoiseSVN", &hTortoise) != ERROR_SUCCESS)
		/* Tortoise not installed */
		return 0;
	command[0] = '"';  /* quote the path to the executable */
	size = sizeof(command) - 1;
	if (RegQueryValueEx(hTortoise, "Directory", 0, &type, command+1, &size) != ERROR_SUCCESS ||
	    type != REG_SZ)
		/* Registry corrupted */
		return 0;
	strcat_s(command, sizeof(command), "bin\\subwcrev.exe");
	if (_stat(command+1, &st) < 0)
		/* subwcrev.exe not part of the release */
		return 0;
	strcat_s(command, sizeof(command), "\" .. ..\\Modules\\getbuildinfo.c getbuildinfo.c");
	puts(command); fflush(stdout);
	if (system(command) < 0)
		return 0;
	return 1;
}

int main(int argc, char*argv[])
{
	char command[500] = "";
	int svn;
	/* Get getbuildinfo.c from svn as getbuildinfo2.c */
	svn = make_buildinfo2();
	if (svn) {
		puts("subcwrev succeeded, generated getbuildinfo.c");
	} else {
		puts("Couldn't run subwcrev.exe on getbuildinfo.c.  Copying it");
		strcat_s(command, sizeof(command), "copy /Y ..\\Modules\\getbuildinfo.c getbuildinfo.c");
		puts(command); fflush(stdout);
		if (system(command) < 0)
			return EXIT_FAILURE;
	}
	return 0;
}