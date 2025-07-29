#ifndef Py_INTERNAL_C_ARRAY_H
#define Py_INTERNAL_C_ARRAY_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


/* Utility for a number of growing arrays */

typedef struct {
    void *array;              /* pointer to the array */
    int allocated_entries;    /* pointer to the capacity of the array */
    size_t item_size;         /* size of each element */
    int initial_num_entries;  /* initial allocation size */
} _Py_c_array_t;


int _Py_CArray_Init(_Py_c_array_t* array, int item_size, int initial_num_entries);
void _Py_CArray_Fini(_Py_c_array_t* array);

/* If idx is out of bounds:
 * If arr->array is NULL, allocate arr->initial_num_entries slots.
 * Otherwise, double its size.
 *
 * Return 0 if successful and -1 (with exception set) otherwise.
 */
int _Py_CArray_EnsureCapacity(_Py_c_array_t *c_array, int idx);


#ifdef __cplusplus
}
#endif

#endif /* !Py_INTERNAL_C_ARRAY_H */
