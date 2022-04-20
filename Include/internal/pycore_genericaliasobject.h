#ifndef Py_INTERNAL_GENERICALIASOBJECT_H
#define Py_INTERNAL_GENERICALIASOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

extern PyTypeObject _Py_GenericAliasIterType;

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GENERICALIASOBJECT_H */
