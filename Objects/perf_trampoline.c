#include "Python.h"
#include "pycore_ceval.h"
#include "pycore_frame.h"
#include "pycore_interp.h"

#ifdef HAVE_PERF_TRAMPOLINE

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

typedef struct {
    char *start_addr;
    char *current_addr;
    size_t size;
    size_t size_left;
    size_t code_size;
} code_arena_t;

static Py_ssize_t extra_code_index = -1;
static code_arena_t code_arena;

static int
new_code_arena()
{
    size_t page_size = sysconf(_SC_PAGESIZE);
    char *memory = mmap(NULL,  // address
                        page_size, PROT_READ | PROT_WRITE | PROT_EXEC,
                        MAP_PRIVATE | MAP_ANONYMOUS,
                        -1,  // fd (not used here)
                        0);  // offset (not used here)
    if (!memory) {
        Py_FatalError("Failed to allocate new code arena");
        return -1;
    }
    void *start = &_Py_trampoline_func_start;
    void *end = &_Py_trampoline_func_end;
    size_t code_size = end - start;

    long n_copies = page_size / code_size;
    for (int i = 0; i < n_copies; i++) {
        memcpy(memory + i * code_size, start, code_size * sizeof(char));
    }

    mprotect(memory, page_size, PROT_READ | PROT_EXEC);

    code_arena.start_addr = memory;
    code_arena.current_addr = memory;
    code_arena.size = page_size;
    code_arena.size_left = page_size;
    code_arena.code_size = code_size;
    return 0;
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
    if (code_arena.size_left <= code_arena.code_size) {
        if (new_code_arena() < 0) {
            return NULL;
        }
    }

    assert(code_arena.size_left <= code_arena.size);
    return code_arena_new_code(&code_arena);
}

static inline FILE *
perf_map_open(pid_t pid)
{
    char filename[100];
    snprintf(filename, sizeof(filename), "/tmp/perf-%d.map", pid);
    FILE *res = fopen(filename, "a");
    if (!res) {
        _Py_FatalErrorFormat(__func__, "Couldn't open %s: errno(%d)", filename, errno);
        return NULL;
    }
    return res;
}

static inline int
perf_map_close(FILE *fp)
{
    if (fp) {
        return fclose(fp);
    }
    return 0;
}

static void
perf_map_write_entry(FILE *method_file, const void *code_addr,
                     unsigned int code_size, const char *entry,
                     const char *file)
{
    fprintf(method_file, "%lx %x py::%s:%s\n", (unsigned long)code_addr,
            code_size, entry, file);
}

static PyObject *
py_trampoline_evaluator(PyThreadState *ts, _PyInterpreterFrame *frame,
                        int throw)
{
    PyCodeObject *co = frame->f_code;
    py_trampoline f = NULL;
    _PyCode_GetExtra((PyObject *)co, extra_code_index, (void **)&f);
    if (f == NULL) {
        if (extra_code_index == -1) {
            extra_code_index = _PyEval_RequestCodeExtraIndex(NULL);
        }
        py_trampoline new_trampoline = compile_trampoline();
        if (new_trampoline == NULL) {
            return NULL;
        }
        FILE *pfile = perf_map_open(getpid());
        if (pfile == NULL) {
            return NULL;
        }
        perf_map_write_entry(pfile, new_trampoline, code_arena.code_size,
                             PyUnicode_AsUTF8(co->co_qualname),
                             PyUnicode_AsUTF8(co->co_filename));
        perf_map_close(pfile);
        _PyCode_SetExtra((PyObject *)co, extra_code_index,
                         (void *)new_trampoline);
        f = new_trampoline;
    }
    assert(f != NULL);
    return f(_PyEval_EvalFrameDefault, ts, frame, throw);
}
#endif

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
