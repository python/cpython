/* Minimal main program -- everything is loaded from the library. */

#include <windows.h>
#include "Python.h"

extern int Py_Main();

int WINAPI WinMain(
    HINSTANCE hInstance,      // handle to current instance
    HINSTANCE hPrevInstance,  // handle to previous instance
    LPSTR lpCmdLine,          // pointer to command line
    int nCmdShow              // show state of window
)
{
	return Py_Main(__argc, __argv);
}
