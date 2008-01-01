#include <windows.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

/* This file creates the getbuildinfo.o object, by first
   invoking subwcrev.exe (if found), and then invoking cl.exe.
   As a side effect, it might generate PC\VS7.1\getbuildinfo2.c
   also. If this isn't a subversion checkout, or subwcrev isn't
   found, it compiles ..\\..\\Modules\\getbuildinfo.c instead.

   Currently, subwcrev.exe is found from the registry entries
   of TortoiseSVN.

   No attempt is made to place getbuildinfo.o into the proper
   binary directory. This isn't necessary, as this tool is
   invoked as a pre-link step for pythoncore, so that overwrites
   any previous getbuildinfo.o.

*/

int make_buildinfo2()
{
	struct _stat st;
	HKEY hTortoise;
	char command[500];
	DWORD type, size;
	if (_stat(".svn", &st) < 0)
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
	strcat(command, "bin\\subwcrev.exe");
	if (_stat(command+1, &st) < 0)
		/* subwcrev.exe not part of the release */
		return 0;
	strcat(command, "\" ..\\.. ..\\..\\Modules\\getbuildinfo.c getbuildinfo2.c");
	puts(command); fflush(stdout);
	if (system(command) < 0)
		return 0;
	return 1;
}

int main(int argc, char*argv[])
{
	char command[500] = "cl.exe -c -D_WIN32 -DUSE_DL_EXPORT -D_WINDOWS -DWIN32 -D_WINDLL ";
	int do_unlink, result;
	if (argc != 2) {
		fprintf(stderr, "make_buildinfo $(ConfigurationName)\n");
		return EXIT_FAILURE;
	}
	if (strcmp(argv[1], "Release") == 0) {
		strcat(command, "-MD ");
	}
	else if (strcmp(argv[1], "Debug") == 0) {
		strcat(command, "-D_DEBUG -MDd ");
	}
	else if (strcmp(argv[1], "ReleaseItanium") == 0) {
		strcat(command, "-MD /USECL:MS_ITANIUM ");
	}
	else if (strcmp(argv[1], "ReleaseAMD64") == 0) {
		strcat(command, "-MD ");
		strcat(command, "-MD /USECL:MS_OPTERON ");
	}
	else {
		fprintf(stderr, "unsupported configuration %s\n", argv[1]);
		return EXIT_FAILURE;
	}

	if ((do_unlink = make_buildinfo2()))
		strcat(command, "getbuildinfo2.c -DSUBWCREV ");
	else
		strcat(command, "..\\..\\Modules\\getbuildinfo.c");
	strcat(command, " -Fogetbuildinfo.o -I..\\..\\Include -I..\\..\\PC");
	puts(command); fflush(stdout);
	result = system(command);
	if (do_unlink)
		unlink("getbuildinfo2.c");
	if (result < 0)
		return EXIT_FAILURE;
	return 0;
}
