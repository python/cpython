#ifndef Py_INTERNAL_UNICODECTYPE_H
#define Py_INTERNAL_UNICODECTYPE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

extern int _PyUnicode_ToLowerFull(Py_UCS4 ch, Py_UCS4 *res);
extern int _PyUnicode_ToTitleFull(Py_UCS4 ch, Py_UCS4 *res);
extern int _PyUnicode_ToUpperFull(Py_UCS4 ch, Py_UCS4 *res);
extern int _PyUnicode_ToFoldedFull(Py_UCS4 ch, Py_UCS4 *res);
extern int _PyUnicode_IsCaseIgnorable(Py_UCS4 ch);
extern int _PyUnicode_IsCased(Py_UCS4 ch);

// Export for 'unicodedata' shared extension.
PyAPI_FUNC(int) _PyUnicode_IsXidStart(Py_UCS4 ch);
PyAPI_FUNC(int) _PyUnicode_IsXidContinue(Py_UCS4 ch);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_UNICODECTYPE_H */
