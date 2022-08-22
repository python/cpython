#include "Python.h"
#include "pycore_ceval.h"
#include "pycore_frame.h"
#include "pycore_interp.h"

#ifdef HAVE_PERF_TRAMPOLINE

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

typedef PyObject *(*py_evaluator)(PyThreadState *, _PyInterpreterFrame *,
                                  int throwflag);
typedef PyObject *(*py_trampoline)(py_evaluator, PyThreadState *,
                                   _PyInterpreterFrame *, int throwflag);
extern void *_Py_trampoline_func_start;
extern void *_Py_trampoline_func_end;

struct code_arena_st {
    char *start_addr;
    char *current_addr;
    size_t size;
    size_t size_left;
    size_t code_size;
    struct code_arena_st *prev;
};

typedef struct code_arena_st code_arena_t;

typedef enum {
    PERF_STATUS_FAILED = -1,
    PERF_STATUS_NO_INIT = 0,
} perf_status_t;

static perf_status_t perf_status = PERF_STATUS_NO_INIT;
static Py_ssize_t extra_code_index = -1;
static code_arena_t *code_arena;
static FILE *perf_map_file;

static int
new_code_arena(void)
{
    // non-trivial programs typically need 64 to 256 kiB.
    size_t mem_size = 4096 * 16;
    assert(mem_size % sysconf(_SC_PAGESIZE) == 0);
    char *memory = mmap(NULL,  // address
                        mem_size,
                        PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS,
                        -1,  // fd (not used here)
                        0);  // offset (not used here)
    if (!memory) {
        PyErr_SetFromErrno(PyExc_OSError);
        _PyErr_WriteUnraisableMsg(
            "Failed to create new mmap for perf trampoline", NULL);
        perf_status = PERF_STATUS_FAILED;
        return -1;
    }
    void *start = &_Py_trampoline_func_start;
    void *end = &_Py_trampoline_func_end;
    size_t code_size = end - start;

    size_t n_copies = mem_size / code_size;
    for (size_t i = 0; i < n_copies; i++) {
        memcpy(memory + i * code_size, start, code_size * sizeof(char));
    }
    // Some systems may prevent us from creating executable code on the fly.
    int res = mprotect(memory, mem_size, PROT_READ | PROT_EXEC);
    if (res == -1) {
        PyErr_SetFromErrno(PyExc_OSError);
        munmap(memory, mem_size);
        _PyErr_WriteUnraisableMsg(
            "Failed to set mmap for perf trampoline to PROT_READ | PROT_EXEC", NULL);
    }

    code_arena_t *new_arena = PyMem_RawCalloc(1, sizeof(code_arena_t));
    if (new_arena == NULL) {
        PyErr_NoMemory();
        munmap(memory, mem_size);
        _PyErr_WriteUnraisableMsg(
            "Failed to allocate new code arena struct", NULL);
        return -1;
    }

    new_arena->start_addr = memory;
    new_arena->current_addr = memory;
    new_arena->size = mem_size;
    new_arena->size_left = mem_size;
    new_arena->code_size = code_size;
    new_arena->prev = code_arena;
    code_arena = new_arena;
    return 0;
}

static void
free_code_arenas(void)
{
    code_arena_t *cur = code_arena;
    code_arena_t *prev;
    code_arena = NULL; // invalid static pointer
    while(cur) {
        munmap(cur->start_addr, cur->size);
        prev = cur->prev;
        PyMem_RawFree(cur);
        cur = prev;
    }
}

static inline py_trampoline
code_arena_new_code(code_arena_t *code_arena)
{
    py_trampoline trampoline = (py_trampoline)code_arena->current_addr;
    code_arena->size_left -= code_arena->code_size;
    code_arena->current_addr += code_arena->code_size;
    return trampoline;
}

static inline py_trampoline
compile_trampoline(void)
{
    if ((code_arena == NULL) || (code_arena->size_left <= code_arena->code_size)) {
        if (new_code_arena() < 0) {
            return NULL;
        }
    }
    assert(code_arena->size_left <= code_arena->size);
    return code_arena_new_code(code_arena);
}

static inline FILE *
perf_map_get_file(void)
{
    if (perf_map_file) {
        return perf_map_file;
    }
    char filename[100];
    pid_t pid = getpid();
    // Location and file name of perf map is hard-coded in perf tool.
    // Use exclusive create flag wit nofollow to prevent symlink attacks.
    int flags = O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC;
    snprintf(filename, sizeof(filename)-1, "/tmp/perf-%jd.map", (intmax_t)pid);
    int fd = open(filename, flags, 0600);
    if (fd == -1) {
        perf_status = PERF_STATUS_FAILED;
        PyErr_SetFromErrnoWithFilename(PyExc_OSError, filename);
        _PyErr_WriteUnraisableMsg("Failed to create perf map file", NULL);
        return NULL;
    }
    perf_map_file = fdopen(fd, "w");
    if (!perf_map_file) {
        perf_status = PERF_STATUS_FAILED;
        PyErr_SetFromErrnoWithFilename(PyExc_OSError, filename);
        close(fd);
        _PyErr_WriteUnraisableMsg("Failed to create perf map file handle", NULL);
        return NULL;
    }
    return perf_map_file;
}

static inline int
perf_map_close(FILE *fp)
{
    if (fp) {
        return fclose(fp);
    }
    perf_status = PERF_STATUS_NO_INIT;
    return 0;
}

static void
perf_map_write_entry(FILE *method_file, const void *code_addr,
                     unsigned int code_size, const char *entry,
                     const char *file)
{
    assert(entry != NULL && file != NULL);
    fprintf(method_file, "%p %x py::%s:%s\n", code_addr,
            code_size, entry, file);
    fflush(method_file);
}

static PyObject *
py_trampoline_evaluator(PyThreadState *ts, _PyInterpreterFrame *frame,
                        int throw)
{
    if (perf_status == PERF_STATUS_FAILED) {
        return _PyEval_EvalFrameDefault(ts, frame, throw);
    }
    PyCodeObject *co = frame->f_code;
    py_trampoline f = NULL;
    _PyCode_GetExtra((PyObject *)co, extra_code_index, (void **)&f);
    if (f == NULL) {
        FILE *pfile = perf_map_get_file();
        if (pfile == NULL) {
            return _PyEval_EvalFrameDefault(ts, frame, throw);
        }
        if (extra_code_index == -1) {
            extra_code_index = _PyEval_RequestCodeExtraIndex(NULL);
        }
        py_trampoline new_trampoline = compile_trampoline();
        if (new_trampoline == NULL) {
            return _PyEval_EvalFrameDefault(ts, frame, throw);
        }
        perf_map_write_entry(pfile, new_trampoline, code_arena->code_size,
                             PyUnicode_AsUTF8(co->co_qualname),
                             PyUnicode_AsUTF8(co->co_filename));
        _PyCode_SetExtra((PyObject *)co, extra_code_index,
                         (void *)new_trampoline);
        f = new_trampoline;
    }
    assert(f != NULL);
    return f(_PyEval_EvalFrameDefault, ts, frame, throw);
}
#endif // HAVE_PERF_TRAMPOLINE

int
_PyIsPerfTrampolineActive(void)
{
#ifdef HAVE_PERF_TRAMPOLINE
    PyThreadState *tstate = _PyThreadState_GET();
    return tstate->interp->eval_frame == py_trampoline_evaluator;
#endif
    return 0;
}



int
_PyPerfTrampoline_Init(int activate)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (!activate) {
        tstate->interp->eval_frame = NULL;
    }
    else {
#ifdef HAVE_PERF_TRAMPOLINE
        tstate->interp->eval_frame = py_trampoline_evaluator;
        if (new_code_arena() < 0) {
            return -1;
        }
#endif
    }
    return 0;
}

int
_PyPerfTrampoline_Fini(void)
{
#ifdef HAVE_PERF_TRAMPOLINE
    free_code_arenas();
    perf_map_close(perf_map_file);
#endif
    return 0;
}

PyStatus
_PyPerfTrampoline_AfterFork_Child(void)
{
#ifdef HAVE_PERF_TRAMPOLINE
    // close file in child.
    perf_map_close(perf_map_file);
    perf_map_file = NULL;
#endif
    return PyStatus_Ok();
}
