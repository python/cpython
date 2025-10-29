#ifndef Py_INTERNAL_UNICODECTYPE_H
#define Py_INTERNAL_UNICODECTYPE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

PyAPI_FUNC(int) _PyUnicode_IsXidStart(Py_UCS4 ch);
PyAPI_FUNC(int) _PyUnicode_IsXidContinue(Py_UCS4 ch);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_UNICODECTYPE_H */
