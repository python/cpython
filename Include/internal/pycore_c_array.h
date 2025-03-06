#ifndef Py_INTERNAL_C_ARRAY_H
#define Py_INTERNAL_C_ARRAY_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


/* Utility for a number of growing arrays
 *
 * idx: index we need to write to
 * array: pointer to the array
 * alloc: pointer to array capacity
 * delta: array growth
 * item_size: size of each array entry
 *
 * If *array is NULL, allocate default_alloc entries.
 * Otherwise, grow the array by default_alloc entries.
 *
 * return 0 if successful and -1 (with exception set) otherwise.
 */
int _Py_EnsureArrayLargeEnough(
        int idx,
        void **array,
        int *alloc,
        int delta,
        size_t item_size);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_C_ARRAY_H */
