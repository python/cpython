/******************************************************************************
 * Remote Debugging Module - Subprocess Enumeration
 *
 * This file contains platform-specific functions for enumerating child
 * processes of a given PID.
 ******************************************************************************/

#include "_remote_debugging.h"

#ifndef MS_WINDOWS
#include <unistd.h>
#include <dirent.h>
#endif

#ifdef MS_WINDOWS
#include <tlhelp32.h>
#endif

/* ============================================================================
 * INTERNAL DATA STRUCTURES
 * ============================================================================ */

/* Simple dynamic array for collecting PIDs */
typedef struct {
    pid_t *pids;
    size_t count;
    size_t capacity;
} pid_array_t;

static int
pid_array_init(pid_array_t *arr)
{
    arr->capacity = 64;
    arr->count = 0;
    arr->pids = (pid_t *)PyMem_Malloc(arr->capacity * sizeof(pid_t));
    if (arr->pids == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    return 0;
}

static void
pid_array_cleanup(pid_array_t *arr)
{
    if (arr->pids != NULL) {
        PyMem_Free(arr->pids);
        arr->pids = NULL;
    }
    arr->count = 0;
    arr->capacity = 0;
}

static int
pid_array_append(pid_array_t *arr, pid_t pid)
{
    if (arr->count >= arr->capacity) {
        /* Check for overflow before multiplication */
        if (arr->capacity > SIZE_MAX / 2) {
            PyErr_SetString(PyExc_OverflowError, "PID array capacity overflow");
            return -1;
        }
        size_t new_capacity = arr->capacity * 2;
        /* Check allocation size won't overflow */
        if (new_capacity > SIZE_MAX / sizeof(pid_t)) {
            PyErr_SetString(PyExc_OverflowError, "PID array size overflow");
            return -1;
        }
        pid_t *new_pids = (pid_t *)PyMem_Realloc(arr->pids, new_capacity * sizeof(pid_t));
        if (new_pids == NULL) {
            PyErr_NoMemory();
            return -1;
        }
        arr->pids = new_pids;
        arr->capacity = new_capacity;
    }
    arr->pids[arr->count++] = pid;
    return 0;
}

static int
pid_array_contains(pid_array_t *arr, pid_t pid)
{
    for (size_t i = 0; i < arr->count; i++) {
        if (arr->pids[i] == pid) {
            return 1;
        }
    }
    return 0;
}

/* ============================================================================
 * SHARED BFS HELPER
 * ============================================================================ */

/* Find child PIDs using BFS traversal of the pid->ppid mapping.
 * all_pids and ppids must have the same count (parallel arrays).
 * Returns 0 on success, -1 on error. */
static int
find_children_bfs(pid_t target_pid, int recursive,
                  pid_t *all_pids, pid_t *ppids, size_t pid_count,
                  pid_array_t *result)
{
    int retval = -1;
    pid_array_t to_process = {0};

    if (pid_array_init(&to_process) < 0) {
        goto done;
    }
    if (pid_array_append(&to_process, target_pid) < 0) {
        goto done;
    }

    size_t process_idx = 0;
    while (process_idx < to_process.count) {
        pid_t current_pid = to_process.pids[process_idx++];

        for (size_t i = 0; i < pid_count; i++) {
            if (ppids[i] != current_pid) {
                continue;
            }
            pid_t child_pid = all_pids[i];
            if (pid_array_contains(result, child_pid)) {
                continue;
            }
            if (pid_array_append(result, child_pid) < 0) {
                goto done;
            }
            if (recursive && pid_array_append(&to_process, child_pid) < 0) {
                goto done;
            }
        }

        if (!recursive) {
            break;
        }
    }

    retval = 0;

done:
    pid_array_cleanup(&to_process);
    return retval;
}

/* ============================================================================
 * LINUX IMPLEMENTATION
 * ============================================================================ */

#if defined(__linux__)

/* Parse /proc/{pid}/stat to get parent PID */
static pid_t
get_ppid_linux(pid_t pid)
{
    char stat_path[64];
    char buffer[2048];

    snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", (int)pid);

    int fd = open(stat_path, O_RDONLY);
    if (fd == -1) {
        return -1;
    }

    ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);

    if (n <= 0) {
        return -1;
    }
    buffer[n] = '\0';

    /* Find closing paren of comm field - stat format: pid (comm) state ppid ... */
    char *p = strrchr(buffer, ')');
    if (!p) {
        return -1;
    }

    /* Skip ") " with bounds checking */
    char *end = buffer + n;
    p += 2;
    if (p >= end) {
        return -1;
    }
    if (*p == ' ') {
        p++;
        if (p >= end) {
            return -1;
        }
    }

    /* Parse: state ppid */
    char state;
    int ppid;
    if (sscanf(p, "%c %d", &state, &ppid) != 2) {
        return -1;
    }

    return (pid_t)ppid;
}

static int
get_child_pids_platform(pid_t target_pid, int recursive, pid_array_t *result)
{
    int retval = -1;
    pid_array_t all_pids = {0};
    pid_array_t ppids = {0};
    DIR *proc_dir = NULL;

    if (pid_array_init(&all_pids) < 0) {
        goto done;
    }

    if (pid_array_init(&ppids) < 0) {
        goto done;
    }

    proc_dir = opendir("/proc");
    if (!proc_dir) {
        PyErr_SetFromErrnoWithFilename(PyExc_OSError, "/proc");
        goto done;
    }

    /* Single pass: collect PIDs and their PPIDs together */
    struct dirent *entry;
    while ((entry = readdir(proc_dir)) != NULL) {
        /* Skip non-numeric entries (also skips . and ..) */
        if (entry->d_name[0] < '1' || entry->d_name[0] > '9') {
            continue;
        }
        char *endptr;
        long pid_long = strtol(entry->d_name, &endptr, 10);
        if (*endptr != '\0' || pid_long <= 0) {
            continue;
        }
        pid_t pid = (pid_t)pid_long;
        pid_t ppid = get_ppid_linux(pid);
        if (ppid < 0) {
            continue;
        }
        if (pid_array_append(&all_pids, pid) < 0 ||
            pid_array_append(&ppids, ppid) < 0) {
            goto done;
        }
    }

    closedir(proc_dir);
    proc_dir = NULL;

    if (find_children_bfs(target_pid, recursive,
                          all_pids.pids, ppids.pids, all_pids.count,
                          result) < 0) {
        goto done;
    }

    retval = 0;

done:
    if (proc_dir) {
        closedir(proc_dir);
    }
    pid_array_cleanup(&all_pids);
    pid_array_cleanup(&ppids);
    return retval;
}

#endif /* __linux__ */

/* ============================================================================
 * MACOS IMPLEMENTATION
 * ============================================================================ */

#if defined(__APPLE__) && TARGET_OS_OSX

#include <libproc.h>
#include <sys/proc_info.h>

static int
get_child_pids_platform(pid_t target_pid, int recursive, pid_array_t *result)
{
    int retval = -1;
    pid_t *pid_list = NULL;
    pid_t *ppids = NULL;

    /* Get count of all PIDs */
    int n_pids = proc_listpids(PROC_ALL_PIDS, 0, NULL, 0);
    if (n_pids <= 0) {
        PyErr_SetString(PyExc_OSError, "Failed to get process count");
        goto done;
    }

    /* Allocate buffer for PIDs (add some slack for new processes) */
    int buffer_size = n_pids + 64;
    pid_list = (pid_t *)PyMem_Malloc(buffer_size * sizeof(pid_t));
    if (!pid_list) {
        PyErr_NoMemory();
        goto done;
    }

    /* Get actual PIDs */
    int actual = proc_listpids(PROC_ALL_PIDS, 0, pid_list, buffer_size * sizeof(pid_t));
    if (actual <= 0) {
        PyErr_SetString(PyExc_OSError, "Failed to list PIDs");
        goto done;
    }

    /* Build pid -> ppid mapping */
    ppids = (pid_t *)PyMem_Malloc(actual * sizeof(pid_t));
    if (!ppids) {
        PyErr_NoMemory();
        goto done;
    }

    /* Get parent PIDs for each process */
    int valid_count = 0;
    for (int i = 0; i < actual; i++) {
        struct proc_bsdinfo proc_info;
        int ret = proc_pidinfo(pid_list[i], PROC_PIDTBSDINFO, 0,
                              &proc_info, sizeof(proc_info));
        if (ret != sizeof(proc_info)) {
            continue;
        }
        pid_list[valid_count] = pid_list[i];
        ppids[valid_count] = proc_info.pbi_ppid;
        valid_count++;
    }

    if (find_children_bfs(target_pid, recursive,
                          pid_list, ppids, valid_count,
                          result) < 0) {
        goto done;
    }

    retval = 0;

done:
    PyMem_Free(pid_list);
    PyMem_Free(ppids);
    return retval;
}

#endif /* __APPLE__ && TARGET_OS_OSX */

/* ============================================================================
 * WINDOWS IMPLEMENTATION
 * ============================================================================ */

#ifdef MS_WINDOWS

static int
get_child_pids_platform(pid_t target_pid, int recursive, pid_array_t *result)
{
    int retval = -1;
    pid_array_t all_pids = {0};
    pid_array_t ppids = {0};
    HANDLE snapshot = INVALID_HANDLE_VALUE;

    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        PyErr_SetFromWindowsErr(0);
        goto done;
    }

    if (pid_array_init(&all_pids) < 0) {
        goto done;
    }

    if (pid_array_init(&ppids) < 0) {
        goto done;
    }

    /* Single pass: collect PIDs and PPIDs together */
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(snapshot, &pe)) {
        do {
            if (pid_array_append(&all_pids, (pid_t)pe.th32ProcessID) < 0 ||
                pid_array_append(&ppids, (pid_t)pe.th32ParentProcessID) < 0) {
                goto done;
            }
        } while (Process32Next(snapshot, &pe));
    }

    CloseHandle(snapshot);
    snapshot = INVALID_HANDLE_VALUE;

    if (find_children_bfs(target_pid, recursive,
                          all_pids.pids, ppids.pids, all_pids.count,
                          result) < 0) {
        goto done;
    }

    retval = 0;

done:
    if (snapshot != INVALID_HANDLE_VALUE) {
        CloseHandle(snapshot);
    }
    pid_array_cleanup(&all_pids);
    pid_array_cleanup(&ppids);
    return retval;
}

#endif /* MS_WINDOWS */

/* ============================================================================
 * UNSUPPORTED PLATFORM STUB
 * ============================================================================ */

#if !defined(__linux__) && !(defined(__APPLE__) && TARGET_OS_OSX) && !defined(MS_WINDOWS)

static int
get_child_pids_platform(pid_t target_pid, int recursive, pid_array_t *result)
{
    PyErr_SetString(PyExc_NotImplementedError,
                   "Subprocess enumeration not supported on this platform");
    return -1;
}

#endif

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

PyObject *
enumerate_child_pids(pid_t target_pid, int recursive)
{
    pid_array_t result;

    if (pid_array_init(&result) < 0) {
        return NULL;
    }

    if (get_child_pids_platform(target_pid, recursive, &result) < 0) {
        pid_array_cleanup(&result);
        return NULL;
    }

    /* Convert to Python list */
    PyObject *list = PyList_New(result.count);
    if (list == NULL) {
        pid_array_cleanup(&result);
        return NULL;
    }

    for (size_t i = 0; i < result.count; i++) {
        PyObject *pid_obj = PyLong_FromLong((long)result.pids[i]);
        if (pid_obj == NULL) {
            Py_DECREF(list);
            pid_array_cleanup(&result);
            return NULL;
        }
        PyList_SET_ITEM(list, i, pid_obj);
    }

    pid_array_cleanup(&result);
    return list;
}
