#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <errno.h>

/* Extract the mapping of Win32 error codes to errno */

int main()
{
    int i;
    _setmode(fileno(stdout), O_BINARY);
    printf("/* Generated file. Do not edit. */\n");
    printf("int winerror_to_errno(int winerror)\n");
    printf("{\n    switch(winerror) {\n");
    for(i=1; i < 65000; i++) {
        _dosmaperr(i);
        if (errno == EINVAL) {
            /* Issue #12802 */
            if (i == ERROR_DIRECTORY)
                errno = ENOTDIR;
            /* Issue #13063 */
            else if (i == ERROR_NO_DATA)
                errno = EPIPE;
            else
                continue;
        }
        printf("        case %d: return %d;\n", i, errno);
    }
    printf("        default: return EINVAL;\n");
    printf("    }\n}\n");
}
