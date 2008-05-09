/* Minimal main program -- everything is loaded from the library. */

#include "Python.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int WINAPI wWinMain(
    HINSTANCE hInstance,      /* handle to current instance */
    HINSTANCE hPrevInstance,  /* handle to previous instance */
    LPWSTR lpCmdLine,         /* pointer to command line */
    int nCmdShow              /* show state of window */
)
{
    return Py_Main(__argc, __wargv);
}
