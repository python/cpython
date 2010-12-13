#include <windows.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#define CMD_SIZE 500

/* This file creates the getbuildinfo.o object, by first
   invoking subwcrev.exe (if found), and then invoking cl.exe.
   As a side effect, it might generate PCBuild\getbuildinfo2.c
   also. If this isn't a subversion checkout, or subwcrev isn't
   found, it compiles ..\\Modules\\getbuildinfo.c instead.

   Currently, subwcrev.exe is found from the registry entries
   of TortoiseSVN.

   No attempt is made to place getbuildinfo.o into the proper
   binary directory. This isn't necessary, as this tool is
   invoked as a pre-link step for pythoncore, so that overwrites
   any previous getbuildinfo.o.

   However, if a second argument is provided, this will be used
   as a temporary directory where any getbuildinfo2.c and
   getbuildinfo.o files are put.  This is useful if multiple
   configurations are being built in parallel, to avoid them
   trampling each other's files.

*/

int make_buildinfo2(const char *tmppath)
{
    struct _stat st;
    HKEY hTortoise;
    char command[CMD_SIZE+1];
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
    strcat_s(command, CMD_SIZE, "bin\\subwcrev.exe");
    if (_stat(command+1, &st) < 0)
        /* subwcrev.exe not part of the release */
        return 0;
    strcat_s(command, CMD_SIZE, "\" .. ..\\Modules\\getbuildinfo.c \"");
    strcat_s(command, CMD_SIZE, tmppath); /* quoted tmppath */
    strcat_s(command, CMD_SIZE, "getbuildinfo2.c\"");
    puts(command); fflush(stdout);
    if (system(command) < 0)
        return 0;
    return 1;
}

int main(int argc, char*argv[])
{
    char command[CMD_SIZE] = "cl.exe -c -D_WIN32 -DUSE_DL_EXPORT -D_WINDOWS -DWIN32 -D_WINDLL ";
    char tmppath[CMD_SIZE] = "";
    int do_unlink, result;
    char *tmpdir = NULL;
    if (argc <= 2 || argc > 3) {
        fprintf(stderr, "make_buildinfo $(ConfigurationName) [tmpdir]\n");
        return EXIT_FAILURE;
    }
    if (strcmp(argv[1], "Release") == 0) {
        strcat_s(command, CMD_SIZE, "-MD ");
    }
    else if (strcmp(argv[1], "Debug") == 0) {
        strcat_s(command, CMD_SIZE, "-D_DEBUG -MDd ");
    }
    else if (strcmp(argv[1], "ReleaseItanium") == 0) {
        strcat_s(command, CMD_SIZE, "-MD /USECL:MS_ITANIUM ");
    }
    else if (strcmp(argv[1], "ReleaseAMD64") == 0) {
        strcat_s(command, CMD_SIZE, "-MD ");
        strcat_s(command, CMD_SIZE, "-MD /USECL:MS_OPTERON ");
    }
    else {
        fprintf(stderr, "unsupported configuration %s\n", argv[1]);
        return EXIT_FAILURE;
    }
    if (argc > 2) {
        tmpdir = argv[2];
        strcat_s(tmppath, _countof(tmppath), tmpdir);
        /* Hack fix for bad command line:  If the command is issued like this:
         * $(SolutionDir)make_buildinfo.exe" Debug "$(IntDir)"
         * we will get a trailing quote because IntDir ends with a backslash that then
         * escapes the final ".  To simplify the life for developers, catch that problem
         * here by cutting it off.
         * The proper command line, btw is:
         * $(SolutionDir)make_buildinfo.exe" Debug "$(IntDir)\"
         * Hooray for command line parsing on windows.
         */
        if (strlen(tmppath) > 0 && tmppath[strlen(tmppath)-1] == '"')
            tmppath[strlen(tmppath)-1] = '\0';
        strcat_s(tmppath, _countof(tmppath), "\\");
    }

    if ((do_unlink = make_buildinfo2(tmppath))) {
        strcat_s(command, CMD_SIZE, "\"");
        strcat_s(command, CMD_SIZE, tmppath);
        strcat_s(command, CMD_SIZE, "getbuildinfo2.c\" -DSUBWCREV ");
    } else
        strcat_s(command, CMD_SIZE, "..\\Modules\\getbuildinfo.c");
    strcat_s(command, CMD_SIZE, " -Fo\"");
    strcat_s(command, CMD_SIZE, tmppath);
    strcat_s(command, CMD_SIZE, "getbuildinfo.o\" -I..\\Include -I..\\PC");
    puts(command); fflush(stdout);
    result = system(command);
    if (do_unlink) {
        command[0] = '\0';
        strcat_s(command, CMD_SIZE, "\"");
        strcat_s(command, CMD_SIZE, tmppath);
        strcat_s(command, CMD_SIZE, "getbuildinfo2.c\"");
        _unlink(command);
    }
    if (result < 0)
        return EXIT_FAILURE;
    return 0;
}
