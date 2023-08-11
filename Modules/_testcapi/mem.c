#include "parts.h"
#include "clinic/mem.c.h"

#include <stddef.h>

/*[clinic input]
module _testcapi
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=6361033e795369fc]*/

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

/*[clinic input]
_testcapi.set_nomemory
    start: int
    end as stop: int = 0
    /
[clinic start generated code]*/

static PyObject *
_testcapi_set_nomemory_impl(PyObject *module, int start, int stop)
/*[clinic end generated code: output=63269cdbe9b8bc74 input=5eb9a6321d41801e]*/
{
    /* Memory allocation fails after 'start' allocation requests, and until
     * 'stop' allocation requests except when 'stop' is negative or equal
     * to 0 (default) in which case allocation failures never stop. */
    FmData.count = 0;
    FmData.start = start;
    FmData.stop = stop;

    fm_setup_hooks();
    Py_RETURN_NONE;
}

/*[clinic input]
_testcapi.remove_mem_hooks
[clinic start generated code]*/

static PyObject *
_testcapi_remove_mem_hooks_impl(PyObject *module)
/*[clinic end generated code: output=c9ef1a5cbc9f111e input=be328dd3aaa05fcb]*/
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

/*[clinic input]
_testcapi.test_pyobject_setallocators
[clinic start generated code]*/

static PyObject *
_testcapi_test_pyobject_setallocators_impl(PyObject *module)
/*[clinic end generated code: output=fa7b2959ec484b73 input=761913abd861dc85]*/
{
    return test_setallocators(PYMEM_DOMAIN_OBJ);
}

/*[clinic input]
_testcapi.test_pyobject_new
[clinic start generated code]*/

static PyObject *
_testcapi_test_pyobject_new_impl(PyObject *module)
/*[clinic end generated code: output=1c824f0a4c415f65 input=e96a497b57a96c04]*/
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

/*[clinic input]
_testcapi.test_pymem_alloc0
[clinic start generated code]*/

static PyObject *
_testcapi_test_pymem_alloc0_impl(PyObject *module)
/*[clinic end generated code: output=5db0c9ea6b668a0d input=ab5befac960ec333]*/
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

/*[clinic input]
_testcapi.test_pymem_setrawallocators
[clinic start generated code]*/

static PyObject *
_testcapi_test_pymem_setrawallocators_impl(PyObject *module)
/*[clinic end generated code: output=a5f4483ab8bee291 input=0f7c60917d941582]*/
{
    return test_setallocators(PYMEM_DOMAIN_RAW);
}

/*[clinic input]
_testcapi.test_pymem_setallocators
[clinic start generated code]*/

static PyObject *
_testcapi_test_pymem_setallocators_impl(PyObject *module)
/*[clinic end generated code: output=93cfc7a5e184b19e input=a24e4766ebf66e13]*/
{
    return test_setallocators(PYMEM_DOMAIN_MEM);
}

/*[clinic input]
_testcapi.pyobject_malloc_without_gil
[clinic start generated code]*/

static PyObject *
_testcapi_pyobject_malloc_without_gil_impl(PyObject *module)
/*[clinic end generated code: output=9e990721ce72f6a4 input=4974132190035e68]*/
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

/*[clinic input]
_testcapi.pymem_buffer_overflow
[clinic start generated code]*/

static PyObject *
_testcapi_pymem_buffer_overflow_impl(PyObject *module)
/*[clinic end generated code: output=0c142a3a0177340f input=e374bb743c6721d3]*/
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

/*[clinic input]
_testcapi.pymem_api_misuse
[clinic start generated code]*/

static PyObject *
_testcapi_pymem_api_misuse_impl(PyObject *module)
/*[clinic end generated code: output=e96e27933186f2bc input=69317f9a91181283]*/
{
    char *buffer;

    /* Deliberate misusage of Python allocators:
       allococate with PyMem but release with PyMem_Raw. */
    buffer = PyMem_Malloc(16);
    PyMem_RawFree(buffer);

    Py_RETURN_NONE;
}

/*[clinic input]
_testcapi.pymem_malloc_without_gil
[clinic start generated code]*/

static PyObject *
_testcapi_pymem_malloc_without_gil_impl(PyObject *module)
/*[clinic end generated code: output=4700cb04720fc5f7 input=29f55895e29cbd10]*/
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

/*[clinic input]
_testcapi.tracemalloc_track
    domain: unsigned_int(bitwise=True)
    ptr_obj: object
    size: Py_ssize_t
    release_gil: int = 0
    /
[clinic start generated code]*/

static PyObject *
_testcapi_tracemalloc_track_impl(PyObject *module, unsigned int domain,
                                 PyObject *ptr_obj, Py_ssize_t size,
                                 int release_gil)
/*[clinic end generated code: output=9f945935ede4349c input=a913a86c49b28acc]*/

// Tracemalloc tests
{
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

/*[clinic input]
_testcapi.tracemalloc_untrack
    domain: int
    ptr_obj: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_tracemalloc_untrack_impl(PyObject *module, int domain,
                                   PyObject *ptr_obj)
/*[clinic end generated code: output=4f5d256b1feb9b63 input=fc0a2852ba54ce82]*/
{
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

static PyMethodDef test_methods[] = {
    _TESTCAPI_PYMEM_API_MISUSE_METHODDEF
    _TESTCAPI_PYMEM_BUFFER_OVERFLOW_METHODDEF
    _TESTCAPI_PYMEM_MALLOC_WITHOUT_GIL_METHODDEF
    _TESTCAPI_PYOBJECT_MALLOC_WITHOUT_GIL_METHODDEF
    _TESTCAPI_REMOVE_MEM_HOOKS_METHODDEF
    _TESTCAPI_SET_NOMEMORY_METHODDEF
    _TESTCAPI_TEST_PYMEM_ALLOC0_METHODDEF
    _TESTCAPI_TEST_PYOBJECT_SETALLOCATORS_METHODDEF
    _TESTCAPI_TEST_PYMEM_SETRAWALLOCATORS_METHODDEF
    _TESTCAPI_TEST_PYOBJECT_NEW_METHODDEF
    _TESTCAPI_TEST_PYMEM_SETALLOCATORS_METHODDEF

    // Tracemalloc tests
    _TESTCAPI_TRACEMALLOC_TRACK_METHODDEF
    _TESTCAPI_TRACEMALLOC_UNTRACK_METHODDEF
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
    v = Py_True;
#else
    v = Py_False;
#endif
    if (PyModule_AddObjectRef(mod, "WITH_PYMALLOC", v) < 0) {
        return -1;
    }

    return 0;
}
