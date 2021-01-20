#include <stdio.h>
#include "Python.h"
#include "pycore_pystate.h"
#include "rewind.h"

int
main(int argc, char **argv)
{
    PyConfig config;
    _PyRuntime_Initialize();
    
    PyConfig_InitPythonConfig(&config);
    Py_InitializeFromConfig(&config);
    
    Rewind_Activate();
    
    //PyRun_SimpleStringFlags("print('hello world')", NULL);
    char filename[] = "two_sum.py";
    FILE *file = fopen(filename, "r");
    PyRun_SimpleFileExFlags(file, filename, 0, NULL);
    fclose(file);
    Rewind_Deactivate();
    return 0;
}
