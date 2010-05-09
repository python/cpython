/* Simple program that repeatedly calls Py_Initialize(), does something, and
   then calls Py_Finalize().  This should help finding leaks related to
   initialization. */

#include "Python.h"

main(int argc, char **argv)
{
    int count = -1;
    char *command;

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: loop <python-command> [count]\n");
        exit(2);
    }
    command = argv[1];

    if (argc == 3) {
        count = atoi(argv[2]);
    }

    Py_SetProgramName(argv[0]);

    /* uncomment this if you don't want to load site.py */
    /* Py_NoSiteFlag = 1; */

    while (count == -1 || --count >= 0 ) {
        Py_Initialize();
        PyRun_SimpleString(command);
        Py_Finalize();
    }
    return 0;
}
