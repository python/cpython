#ifndef Py_INTERNAL_PYMEM_H
#define Py_INTERNAL_PYMEM_H
#ifdef __cplusplus
extern "C" {
#endif

/* Set the memory allocator of the specified domain to the default.
   Save the old allocator into *old_alloc if it's non-NULL.
   Return on success, or return -1 if the domain is unknown. */
PyAPI_FUNC(int) _PyMem_SetDefaultAllocator(
    PyMemAllocatorDomain domain,
    PyMemAllocatorEx *old_alloc);

#endif /* !Py_INTERNAL_PYMEM_H */
