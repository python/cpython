#ifndef Py_RESOURCE_H
#define Py_RESOURCE_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void (*close_func) (void *data);
    void *data;
} PyResource;

PyAPI_FUNC(void) PyResource_Close(PyResource *res);

#ifdef Py_BUILD_CORE
extern void _PyResource_DECREF(void *data);
#endif

#ifdef __cplusplus
}
#endif
#endif  // !Py_RESOURCE_H
