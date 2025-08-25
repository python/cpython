/* Minimal embedding example: start Python interpreter from C */

#include "Python.h"

#ifdef _WIN32   /* Windows entrypoint */
int wmain(int argc, wchar_t **argv)
{
    /* Call into the main Python interpreter loop */
    return Py_Main(argc, argv);
}
#else           /* Linux/macOS entrypoint */
int main(int argc, char **argv)
{
    /* Py_BytesMain handles UTF-8 encoded argv */
    return Py_BytesMain(argc, argv);
}
#endif
