/* Minimal main program -- everything is loaded from the library. */

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "Python.h"

extern int Py_Main();

int WINAPI WinMain(
    HINSTANCE hInstance,      /* handle to current instance */
    HINSTANCE hPrevInstance,  /* handle to previous instance */
    LPSTR lpCmdLine,          /* pointer to command line */
    int nCmdShow              /* show state of window */
)
{
    int null_file;

    /*
     * make sure that the C RTL has valid file descriptors for
     * stdin, stdout, stderr.  Use the NUL device if necessary.
     * This allows popen to work under pythonw.
     *
     * When pythonw.exe starts the C RTL function _ioinit is called
     * first. WinMain is called later hence the need to check for
     * invalid handles.
     *
     * Note: FILE stdin, stdout, stderr do not use the file descriptors
     * setup here. They are already initialised before WinMain was called.
     */

    null_file = open("NUL", _O_RDWR);

    if (_get_osfhandle(0) == -1)
        dup2(null_file, 0);

    if (_get_osfhandle(1) == -1)
        dup2(null_file, 1);

    if (_get_osfhandle(2) == -1)
        dup2(null_file, 2);

    close(null_file);

    return Py_Main(__argc, __argv);
}
