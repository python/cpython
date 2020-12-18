#include <stdio.h>
#include "Python.h"
#include "pycore_pystate.h"
#include "rewind.h"

int
main(int argc, char **argv)
{
    PyConfig config;
    Rewind_Initialize();
    _PyRuntime_Initialize();
    
    PyConfig_InitPythonConfig(&config);
    Py_InitializeFromConfig(&config);
    
    Rewind_Initialize2();
    
    //PyRun_SimpleStringFlags("print('hello world')", NULL);
    char filename[] = "test.py";
    FILE *file = fopen(filename, "r");
    PyRun_SimpleFileExFlags(file, filename, 0, NULL);
    fclose(file);
    Rewind_Cleanup();
    return 0;
}
