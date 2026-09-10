#ifndef Py_INTERNAL_DESCROBJECT_H
#define Py_INTERNAL_DESCROBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

typedef struct {
    PyObject_HEAD
    PyObject *prop_get;
    PyObject *prop_set;
    PyObject *prop_del;
    PyObject *prop_doc;
    PyObject *prop_name;
    int getter_doc;
} propertyobject;

typedef propertyobject _PyPropertyObject;

extern PyTypeObject _PyMethodWrapper_Type;

extern void *_PyMember_GetOffset(PyObject *, PyMemberDef *);

/* Return a borrowed reference to the mapping wrapped by a mappingproxy.
 * The struct layout matches mappingproxyobject in Objects/descrobject.c.
 */
static inline PyObject *
_PyDictProxy_GetMapping(PyObject *op)
{
    typedef struct {
        PyObject_HEAD
        PyObject *mapping;
    } _PyMappingProxyObject;
    assert(op != NULL);
    assert(PyObject_TypeCheck(op, &PyDictProxy_Type));
    return ((_PyMappingProxyObject *)op)->mapping;
}

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_DESCROBJECT_H */
