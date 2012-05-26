/*
 * This wrapper program executes a python executable hidden inside an
 * application bundle inside the Python framework. This is needed to run
 * GUI code: some GUI API's don't work unless the program is inside an
 * application bundle.
 *
 * This program uses posix_spawn rather than plain execv because we need
 * slightly more control over how the "real" interpreter is executed.
 *
 * On OSX 10.4 (and earlier) this falls back to using exec because the
 * posix_spawnv functions aren't available there.
 */

#pragma weak_import posix_spawnattr_init
#pragma weak_import posix_spawnattr_setbinpref_np
#pragma weak_import posix_spawnattr_setflags
#pragma weak_import posix_spawn

#include <Python.h>
#include <unistd.h>
#ifdef HAVE_SPAWN_H
#include <spawn.h>
#endif
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <err.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <Python.h>


extern char** environ;

/*
 * Locate the python framework by looking for the
 * library that contains Py_Initialize.
 *
 * In a regular framework the structure is:
 *
 *    Python.framework/Versions/2.7
 *              /Python
 *              /Resources/Python.app/Contents/MacOS/Python
 *
 * In a virtualenv style structure the expected
 * structure is:
 *
 *    ROOT
 *       /bin/pythonw
 *       /.Python   <- the dylib
 *       /.Resources/Python.app/Contents/MacOS/Python
 *
 * NOTE: virtualenv's are not an officially supported
 * feature, support for that structure is provided as
 * a convenience.
 */
static char* get_python_path(void)
{
    size_t len;
    Dl_info info;
    char* end;
    char* g_path;

    if (dladdr(Py_Initialize, &info) == 0) {
        return NULL;
    }

    len = strlen(info.dli_fname);

    g_path = malloc(len+60);
    if (g_path == NULL) {
        return NULL;
    }

    strcpy(g_path, info.dli_fname);
    end = g_path + len - 1;
    while (end != g_path && *end != '/') {
        end --;
    }
    end++;
    if (*end == '.') {
        end++;
    }
    strcpy(end, "Resources/Python.app/Contents/MacOS/" PYTHONFRAMEWORK);

    return g_path;
}

#ifdef HAVE_SPAWN_H
static void
setup_spawnattr(posix_spawnattr_t* spawnattr)
{
    size_t ocount;
    size_t count;
    cpu_type_t cpu_types[1];
    short flags = 0;
#ifdef __LP64__
    int   ch;
#endif

    if ((errno = posix_spawnattr_init(spawnattr)) != 0) {
        err(2, "posix_spawnattr_int");
        /* NOTREACHTED */
    }

    count = 1;

    /* Run the real python executable using the same architecture as this
     * executable, this allows users to control the architecture using
     * "arch -ppc python"
     */

#if defined(__ppc64__)
    cpu_types[0] = CPU_TYPE_POWERPC64;

#elif defined(__x86_64__)
    cpu_types[0] = CPU_TYPE_X86_64;

#elif defined(__ppc__)
    cpu_types[0] = CPU_TYPE_POWERPC;
#elif defined(__i386__)
    cpu_types[0] = CPU_TYPE_X86;
#else
#       error "Unknown CPU"
#endif

    if (posix_spawnattr_setbinpref_np(spawnattr, count,
                            cpu_types, &ocount) == -1) {
        err(1, "posix_spawnattr_setbinpref");
        /* NOTREACHTED */
    }
    if (count != ocount) {
        fprintf(stderr, "posix_spawnattr_setbinpref failed to copy\n");
        exit(1);
        /* NOTREACHTED */
    }


    /*
     * Set flag that causes posix_spawn to behave like execv
     */
    flags |= POSIX_SPAWN_SETEXEC;
    if ((errno = posix_spawnattr_setflags(spawnattr, flags)) != 0) {
        err(1, "posix_spawnattr_setflags");
        /* NOTREACHTED */
    }
}
#endif

int
main(int argc, char **argv) {
    char* exec_path = get_python_path();
    static char path[PATH_MAX * 2];
    static char real_path[PATH_MAX * 2];
    int status;
    uint32_t size = PATH_MAX * 2;

    /* Set the original executable path in the environment. */
    status = _NSGetExecutablePath(path, &size);
    if (status == 0) {
        if (realpath(path, real_path) != NULL) {
            setenv("__PYTHONV_LAUNCHER__", real_path, 1);
        }
    }

    /*
     * Let argv[0] refer to the new interpreter. This is needed to
     * get the effect we want on OSX 10.5 or earlier. That is, without
     * changing argv[0] the real interpreter won't have access to
     * the Window Server.
     */
    argv[0] = exec_path;

#ifdef HAVE_SPAWN_H
    /* We're weak-linking to posix-spawnv to ensure that
     * an executable build on 10.5 can work on 10.4.
     */
    if (posix_spawn != NULL) {
        posix_spawnattr_t spawnattr = NULL;

        setup_spawnattr(&spawnattr);
        posix_spawn(NULL, exec_path, NULL,
            &spawnattr, argv, environ);
        err(1, "posix_spawn: %s", exec_path);
    }
#endif
    execve(exec_path, argv, environ);
    err(1, "execve: %s", argv[0]);
    /* NOTREACHED */
}
