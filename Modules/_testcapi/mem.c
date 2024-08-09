#include "parts.h"

#include <stddef.h>


typedef struct {
    PyMemAllocatorEx alloc;

    size_t malloc_size;
    size_t calloc_nelem;
    size_t calloc_elsize;
    void *realloc_ptr;
    size_t realloc_new_size;
    void *free_ptr;
    void *ctx;
} alloc_hook_t;

static void *
hook_malloc(void *ctx, size_t size)
{
    alloc_hook_t *hook = (alloc_hook_t *)ctx;
    hook->ctx = ctx;
    hook->malloc_size = size;
    return hook->alloc.malloc(hook->alloc.ctx, size);
}

static void *
hook_calloc(void *ctx, size_t nelem, size_t elsize)
{
    alloc_hook_t *hook = (alloc_hook_t *)ctx;
    hook->ctx = ctx;
    hook->calloc_nelem = nelem;
    hook->calloc_elsize = elsize;
    return hook->alloc.calloc(hook->alloc.ctx, nelem, elsize);
}

static void *
hook_realloc(void *ctx, void *ptr, size_t new_size)
{
    alloc_hook_t *hook = (alloc_hook_t *)ctx;
    hook->ctx = ctx;
    hook->realloc_ptr = ptr;
    hook->realloc_new_size = new_size;
    return hook->alloc.realloc(hook->alloc.ctx, ptr, new_size);
}

static void
hook_free(void *ctx, void *ptr)
{
    alloc_hook_t *hook = (alloc_hook_t *)ctx;
    hook->ctx = ctx;
    hook->free_ptr = ptr;
    hook->alloc.free(hook->alloc.ctx, ptr);
}

/* Most part of the following code is inherited from the pyfailmalloc project
 * written by Victor Stinner. */
static struct {
    int installed;
    PyMemAllocatorEx raw;
    PyMemAllocatorEx mem;
    PyMemAllocatorEx obj;
} FmHook;

static struct {
    int start;
    int stop;
    Py_ssize_t count;
} FmData;

static int
fm_nomemory(void)
{
    FmData.count++;
    if (FmData.count > FmData.start &&
        (FmData.stop <= 0 || FmData.count <= FmData.stop))
    {
        return 1;
    }
    return 0;
}

static void *
hook_fmalloc(void *ctx, size_t size)
{
    PyMemAllocatorEx *alloc = (PyMemAllocatorEx *)ctx;
    if (fm_nomemory()) {
        return NULL;
    }
    return alloc->malloc(alloc->ctx, size);
}

static void *
hook_fcalloc(void *ctx, size_t nelem, size_t elsize)
{
    PyMemAllocatorEx *alloc = (PyMemAllocatorEx *)ctx;
    if (fm_nomemory()) {
        return NULL;
    }
    return alloc->calloc(alloc->ctx, nelem, elsize);
}

static void *
hook_frealloc(void *ctx, void *ptr, size_t new_size)
{
    PyMemAllocatorEx *alloc = (PyMemAllocatorEx *)ctx;
    if (fm_nomemory()) {
        return NULL;
    }
    return alloc->realloc(alloc->ctx, ptr, new_size);
}

static void
hook_ffree(void *ctx, void *ptr)
{
    PyMemAllocatorEx *alloc = (PyMemAllocatorEx *)ctx;
    alloc->free(alloc->ctx, ptr);
}

static void
fm_setup_hooks(void)
{
    if (FmHook.installed) {
        return;
    }
    FmHook.installed = 1;

    PyMemAllocatorEx alloc;
    alloc.malloc = hook_fmalloc;
    alloc.calloc = hook_fcalloc;
    alloc.realloc = hook_frealloc;
    alloc.free = hook_ffree;
    PyMem_GetAllocator(PYMEM_DOMAIN_RAW, &FmHook.raw);
    PyMem_GetAllocator(PYMEM_DOMAIN_MEM, &FmHook.mem);
    PyMem_GetAllocator(PYMEM_DOMAIN_OBJ, &FmHook.obj);

    alloc.ctx = &FmHook.raw;
    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &alloc);

    alloc.ctx = &FmHook.mem;
    PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &alloc);

    alloc.ctx = &FmHook.obj;
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &alloc);
}

static void
fm_remove_hooks(void)
{
    if (FmHook.installed) {
        FmHook.installed = 0;
        PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &FmHook.raw);
        PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &FmHook.mem);
        PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &FmHook.obj);
    }
}

static PyObject *
set_nomemory(PyObject *self, PyObject *args)
{
    /* Memory allocation fails after 'start' allocation requests, and until
     * 'stop' allocation requests except when 'stop' is negative or equal
     * to 0 (default) in which case allocation failures never stop. */
    FmData.count = 0;
    FmData.stop = 0;
    if (!PyArg_ParseTuple(args, "i|i", &FmData.start, &FmData.stop)) {
        return NULL;
    }
    fm_setup_hooks();
    Py_RETURN_NONE;
}

static PyObject *
remove_mem_hooks(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    fm_remove_hooks();
    Py_RETURN_NONE;
}

static PyObject *
test_setallocators(PyMemAllocatorDomain domain)
{
    PyObject *res = NULL;
    const char *error_msg;
    alloc_hook_t hook;

    memset(&hook, 0, sizeof(hook));

    PyMemAllocatorEx alloc;
    alloc.ctx = &hook;
    alloc.malloc = &hook_malloc;
    alloc.calloc = &hook_calloc;
    alloc.realloc = &hook_realloc;
    alloc.free = &hook_free;
    PyMem_GetAllocator(domain, &hook.alloc);
    PyMem_SetAllocator(domain, &alloc);

    /* malloc, realloc, free */
    size_t size = 42;
    hook.ctx = NULL;
    void *ptr;
    switch(domain) {
        case PYMEM_DOMAIN_RAW:
            ptr = PyMem_RawMalloc(size);
            break;
        case PYMEM_DOMAIN_MEM:
            ptr = PyMem_Malloc(size);
            break;
        case PYMEM_DOMAIN_OBJ:
            ptr = PyObject_Malloc(size);
            break;
        default:
            ptr = NULL;
            break;
    }

#define CHECK_CTX(FUNC)                     \
    if (hook.ctx != &hook) {                \
        error_msg = FUNC " wrong context";  \
        goto fail;                          \
    }                                       \
    hook.ctx = NULL;  /* reset for next check */

    if (ptr == NULL) {
        error_msg = "malloc failed";
        goto fail;
    }
    CHECK_CTX("malloc");
    if (hook.malloc_size != size) {
        error_msg = "malloc invalid size";
        goto fail;
    }

    size_t size2 = 200;
    void *ptr2;
    switch(domain) {
        case PYMEM_DOMAIN_RAW:
            ptr2 = PyMem_RawRealloc(ptr, size2);
            break;
        case PYMEM_DOMAIN_MEM:
            ptr2 = PyMem_Realloc(ptr, size2);
            break;
        case PYMEM_DOMAIN_OBJ:
            ptr2 = PyObject_Realloc(ptr, size2);
            break;
        default:
            ptr2 = NULL;
            break;
    }

    if (ptr2 == NULL) {
        error_msg = "realloc failed";
        goto fail;
    }
    CHECK_CTX("realloc");
    if (hook.realloc_ptr != ptr || hook.realloc_new_size != size2) {
        error_msg = "realloc invalid parameters";
        goto fail;
    }

    switch(domain) {
        case PYMEM_DOMAIN_RAW:
            PyMem_RawFree(ptr2);
            break;
        case PYMEM_DOMAIN_MEM:
            PyMem_Free(ptr2);
            break;
        case PYMEM_DOMAIN_OBJ:
            PyObject_Free(ptr2);
            break;
    }

    CHECK_CTX("free");
    if (hook.free_ptr != ptr2) {
        error_msg = "free invalid pointer";
        goto fail;
    }

    /* calloc, free */
    size_t nelem = 2;
    size_t elsize = 5;
    switch(domain) {
        case PYMEM_DOMAIN_RAW:
            ptr = PyMem_RawCalloc(nelem, elsize);
            break;
        case PYMEM_DOMAIN_MEM:
            ptr = PyMem_Calloc(nelem, elsize);
            break;
        case PYMEM_DOMAIN_OBJ:
            ptr = PyObject_Calloc(nelem, elsize);
            break;
        default:
            ptr = NULL;
            break;
    }

    if (ptr == NULL) {
        error_msg = "calloc failed";
        goto fail;
    }
    CHECK_CTX("calloc");
    if (hook.calloc_nelem != nelem || hook.calloc_elsize != elsize) {
        error_msg = "calloc invalid nelem or elsize";
        goto fail;
    }

    hook.free_ptr = NULL;
    switch(domain) {
        case PYMEM_DOMAIN_RAW:
            PyMem_RawFree(ptr);
            break;
        case PYMEM_DOMAIN_MEM:
            PyMem_Free(ptr);
            break;
        case PYMEM_DOMAIN_OBJ:
            PyObject_Free(ptr);
            break;
    }

    CHECK_CTX("calloc free");
    if (hook.free_ptr != ptr) {
        error_msg = "calloc free invalid pointer";
        goto fail;
    }

    res = Py_NewRef(Py_None);
    goto finally;

fail:
    PyErr_SetString(PyExc_RuntimeError, error_msg);

finally:
    PyMem_SetAllocator(domain, &hook.alloc);
    return res;

#undef CHECK_CTX
}

static PyObject *
test_pyobject_setallocators(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return test_setallocators(PYMEM_DOMAIN_OBJ);
}

static PyObject *
test_pyobject_new(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *obj;
    PyTypeObject *type = &PyBaseObject_Type;
    PyTypeObject *var_type = &PyBytes_Type;

    // PyObject_New()
    obj = PyObject_New(PyObject, type);
    if (obj == NULL) {
        goto alloc_failed;
    }
    Py_DECREF(obj);

    // PyObject_NEW()
    obj = PyObject_NEW(PyObject, type);
    if (obj == NULL) {
        goto alloc_failed;
    }
    Py_DECREF(obj);

    // PyObject_NewVar()
    obj = PyObject_NewVar(PyObject, var_type, 3);
    if (obj == NULL) {
        goto alloc_failed;
    }
    Py_DECREF(obj);

    // PyObject_NEW_VAR()
    obj = PyObject_NEW_VAR(PyObject, var_type, 3);
    if (obj == NULL) {
        goto alloc_failed;
    }
    Py_DECREF(obj);

    Py_RETURN_NONE;

alloc_failed:
    PyErr_NoMemory();
    return NULL;
}

static PyObject *
test_pymem_alloc0(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    void *ptr;

    ptr = PyMem_RawMalloc(0);
    if (ptr == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "PyMem_RawMalloc(0) returns NULL");
        return NULL;
    }
    PyMem_RawFree(ptr);

    ptr = PyMem_RawCalloc(0, 0);
    if (ptr == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "PyMem_RawCalloc(0, 0) returns NULL");
        return NULL;
    }
    PyMem_RawFree(ptr);

    ptr = PyMem_Malloc(0);
    if (ptr == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "PyMem_Malloc(0) returns NULL");
        return NULL;
    }
    PyMem_Free(ptr);

    ptr = PyMem_Calloc(0, 0);
    if (ptr == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "PyMem_Calloc(0, 0) returns NULL");
        return NULL;
    }
    PyMem_Free(ptr);

    ptr = PyObject_Malloc(0);
    if (ptr == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "PyObject_Malloc(0) returns NULL");
        return NULL;
    }
    PyObject_Free(ptr);

    ptr = PyObject_Calloc(0, 0);
    if (ptr == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "PyObject_Calloc(0, 0) returns NULL");
        return NULL;
    }
    PyObject_Free(ptr);

    Py_RETURN_NONE;
}

static PyObject *
test_pymem_getallocatorsname(PyObject *self, PyObject *args)
{
    const char *name = _PyMem_GetCurrentAllocatorName();
    if (name == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "cannot get allocators name");
        return NULL;
    }
    return PyUnicode_FromString(name);
}

static PyObject *
test_pymem_setrawallocators(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return test_setallocators(PYMEM_DOMAIN_RAW);
}

static PyObject *
test_pymem_setallocators(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return test_setallocators(PYMEM_DOMAIN_MEM);
}

static PyObject *
pyobject_malloc_without_gil(PyObject *self, PyObject *args)
{
    char *buffer;

    /* Deliberate bug to test debug hooks on Python memory allocators:
       call PyObject_Malloc() without holding the GIL */
    Py_BEGIN_ALLOW_THREADS
    buffer = PyObject_Malloc(10);
    Py_END_ALLOW_THREADS

    PyObject_Free(buffer);

    Py_RETURN_NONE;
}

static PyObject *
pymem_buffer_overflow(PyObject *self, PyObject *args)
{
    char *buffer;

    /* Deliberate buffer overflow to check that PyMem_Free() detects
       the overflow when debug hooks are installed. */
    buffer = PyMem_Malloc(16);
    if (buffer == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    buffer[16] = 'x';
    PyMem_Free(buffer);

    Py_RETURN_NONE;
}

static PyObject *
pymem_api_misuse(PyObject *self, PyObject *args)
{
    char *buffer;

    /* Deliberate misusage of Python allocators:
       allococate with PyMem but release with PyMem_Raw. */
    buffer = PyMem_Malloc(16);
    PyMem_RawFree(buffer);

    Py_RETURN_NONE;
}

static PyObject *
pymem_malloc_without_gil(PyObject *self, PyObject *args)
{
    char *buffer;

    /* Deliberate bug to test debug hooks on Python memory allocators:
       call PyMem_Malloc() without holding the GIL */
    Py_BEGIN_ALLOW_THREADS
    buffer = PyMem_Malloc(10);
    Py_END_ALLOW_THREADS

    PyMem_Free(buffer);

    Py_RETURN_NONE;
}

static PyObject *
test_pyobject_is_freed(const char *test_name, PyObject *op)
{
    if (!_PyObject_IsFreed(op)) {
        PyErr_SetString(PyExc_AssertionError,
                        "object is not seen as freed");
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
check_pyobject_null_is_freed(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyObject *op = NULL;
    return test_pyobject_is_freed("check_pyobject_null_is_freed", op);
}


static PyObject *
check_pyobject_uninitialized_is_freed(PyObject *self,
                                      PyObject *Py_UNUSED(args))
{
    PyObject *op = (PyObject *)PyObject_Malloc(sizeof(PyObject));
    if (op == NULL) {
        return NULL;
    }
    /* Initialize reference count to avoid early crash in ceval or GC */
    Py_SET_REFCNT(op, 1);
    /* object fields like ob_type are uninitialized! */
    return test_pyobject_is_freed("check_pyobject_uninitialized_is_freed", op);
}


static PyObject *
check_pyobject_forbidden_bytes_is_freed(PyObject *self,
                                        PyObject *Py_UNUSED(args))
{
    /* Allocate an incomplete PyObject structure: truncate 'ob_type' field */
    PyObject *op = (PyObject *)PyObject_Malloc(offsetof(PyObject, ob_type));
    if (op == NULL) {
        return NULL;
    }
    /* Initialize reference count to avoid early crash in ceval or GC */
    Py_SET_REFCNT(op, 1);
    /* ob_type field is after the memory block: part of "forbidden bytes"
       when using debug hooks on memory allocators! */
    return test_pyobject_is_freed("check_pyobject_forbidden_bytes_is_freed", op);
}


static PyObject *
check_pyobject_freed_is_freed(PyObject *self, PyObject *Py_UNUSED(args))
{
    /* ASan or TSan would report an use-after-free error */
#if defined(_Py_ADDRESS_SANITIZER) || defined(_Py_THREAD_SANITIZER)
    Py_RETURN_NONE;
#else
    PyObject *op = PyObject_CallNoArgs((PyObject *)&PyBaseObject_Type);
    if (op == NULL) {
        return NULL;
    }
    Py_TYPE(op)->tp_dealloc(op);
    /* Reset reference count to avoid early crash in ceval or GC */
    Py_SET_REFCNT(op, 1);
    /* object memory is freed! */
    return test_pyobject_is_freed("check_pyobject_freed_is_freed", op);
#endif
}

// Tracemalloc tests
static PyObject *
tracemalloc_track(PyObject *self, PyObject *args)
{
    unsigned int domain;
    PyObject *ptr_obj;
    Py_ssize_t size;
    int release_gil = 0;

    if (!PyArg_ParseTuple(args, "IOn|i",
                          &domain, &ptr_obj, &size, &release_gil))
    {
        return NULL;
    }
    void *ptr = PyLong_AsVoidPtr(ptr_obj);
    if (PyErr_Occurred()) {
        return NULL;
    }

    int res;
    if (release_gil) {
        Py_BEGIN_ALLOW_THREADS
        res = PyTraceMalloc_Track(domain, (uintptr_t)ptr, size);
        Py_END_ALLOW_THREADS
    }
    else {
        res = PyTraceMalloc_Track(domain, (uintptr_t)ptr, size);
    }
    if (res < 0) {
        PyErr_SetString(PyExc_RuntimeError, "PyTraceMalloc_Track error");
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *
tracemalloc_untrack(PyObject *self, PyObject *args)
{
    unsigned int domain;
    PyObject *ptr_obj;

    if (!PyArg_ParseTuple(args, "IO", &domain, &ptr_obj)) {
        return NULL;
    }
    void *ptr = PyLong_AsVoidPtr(ptr_obj);
    if (PyErr_Occurred()) {
        return NULL;
    }

    int res = PyTraceMalloc_Untrack(domain, (uintptr_t)ptr);
    if (res < 0) {
        PyErr_SetString(PyExc_RuntimeError, "PyTraceMalloc_Untrack error");
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *
tracemalloc_get_traceback(PyObject *self, PyObject *args)
{
    unsigned int domain;
    PyObject *ptr_obj;

    if (!PyArg_ParseTuple(args, "IO", &domain, &ptr_obj)) {
        return NULL;
    }
    void *ptr = PyLong_AsVoidPtr(ptr_obj);
    if (PyErr_Occurred()) {
        return NULL;
    }

    return _PyTraceMalloc_GetTraceback(domain, (uintptr_t)ptr);
}

static PyMethodDef test_methods[] = {
    {"check_pyobject_forbidden_bytes_is_freed",
                            check_pyobject_forbidden_bytes_is_freed, METH_NOARGS},
    {"check_pyobject_freed_is_freed", check_pyobject_freed_is_freed, METH_NOARGS},
    {"check_pyobject_null_is_freed",  check_pyobject_null_is_freed,  METH_NOARGS},
    {"check_pyobject_uninitialized_is_freed",
                              check_pyobject_uninitialized_is_freed, METH_NOARGS},
    {"pymem_api_misuse",              pymem_api_misuse,              METH_NOARGS},
    {"pymem_buffer_overflow",         pymem_buffer_overflow,         METH_NOARGS},
    {"pymem_getallocatorsname",       test_pymem_getallocatorsname,  METH_NOARGS},
    {"pymem_malloc_without_gil",      pymem_malloc_without_gil,      METH_NOARGS},
    {"pyobject_malloc_without_gil",   pyobject_malloc_without_gil,   METH_NOARGS},
    {"remove_mem_hooks",              remove_mem_hooks,              METH_NOARGS,
        PyDoc_STR("Remove memory hooks.")},
    {"set_nomemory",                  (PyCFunction)set_nomemory,     METH_VARARGS,
        PyDoc_STR("set_nomemory(start:int, stop:int = 0)")},
    {"test_pymem_alloc0",             test_pymem_alloc0,             METH_NOARGS},
    {"test_pymem_setallocators",      test_pymem_setallocators,      METH_NOARGS},
    {"test_pymem_setrawallocators",   test_pymem_setrawallocators,   METH_NOARGS},
    {"test_pyobject_new",             test_pyobject_new,             METH_NOARGS},
    {"test_pyobject_setallocators",   test_pyobject_setallocators,   METH_NOARGS},

    // Tracemalloc tests
    {"tracemalloc_track",             tracemalloc_track,             METH_VARARGS},
    {"tracemalloc_untrack",           tracemalloc_untrack,           METH_VARARGS},
    {"tracemalloc_get_traceback",     tracemalloc_get_traceback,     METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_Mem(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }

    PyObject *v;
#ifdef WITH_PYMALLOC
    v = Py_NewRef(Py_True);
#else
    v = Py_NewRef(Py_False);
#endif
    int rc = PyModule_AddObjectRef(mod, "WITH_PYMALLOC", v);
    Py_DECREF(v);
    if (rc < 0) {
        return -1;
    }

    return 0;
}
