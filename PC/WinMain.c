/* Minimal main program -- everything is loaded from the library. */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fcntl.h>

#include "Python.h"

#ifdef LAUNCHER
/* Q105305 suggests this routine to adjust the handles. */
static void adjust_file(DWORD handle, FILE* f, char* mode)
{
    int hCrt;
    FILE *hf;
    hCrt = _open_osfhandle((intptr_t)GetStdHandle(handle), _O_TEXT);
    hf = _fdopen(hCrt, mode);
    *f = *hf;
    setvbuf(f, NULL, _IONBF, 0);
    /* Alternatively, we could use __set_app_type and _set_osfhnd, 
       but that appears to be undocumented. */
}
#endif

int WINAPI WinMain(
    HINSTANCE hInstance,      /* handle to current instance */
    HINSTANCE hPrevInstance,  /* handle to previous instance */
    LPSTR lpCmdLine,          /* pointer to command line */
    int nCmdShow              /* show state of window */
)
{
#ifdef LAUNCHER
    int i;
    if (__argc > 1 && strcmp(__argv[1], "-console") == 0) {
	/* Allocate a console, and remove the -console argument. */
	AllocConsole();
	for (i = 2; i < __argc; i++)
	    __argv[i-1] = __argv[i];
        __argc--;
	/* Make stdin, stdout, stderr use the newly allocated OS handles. */
	adjust_file(STD_INPUT_HANDLE, stdin, "r");
	adjust_file(STD_OUTPUT_HANDLE, stdout, "w");
	adjust_file(STD_ERROR_HANDLE, stderr, "w");
    }
#endif
    return Py_Main(__argc, __argv);
}
