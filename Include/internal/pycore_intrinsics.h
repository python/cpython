#ifndef Py_INTERNAL_INTRINSIC_H
#define Py_INTERNAL_INTRINSIC_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

/* Unary Functions: */
#define INTRINSIC_1_INVALID                      0
#define INTRINSIC_PRINT                          1
#define INTRINSIC_IMPORT_STAR                    2
#define INTRINSIC_STOPITERATION_ERROR            3
#define INTRINSIC_ASYNC_GEN_WRAP                 4
#define INTRINSIC_UNARY_POSITIVE                 5
#define INTRINSIC_LIST_TO_TUPLE                  6
#define INTRINSIC_TYPEVAR                        7
#define INTRINSIC_PARAMSPEC                      8
#define INTRINSIC_TYPEVARTUPLE                   9
#define INTRINSIC_SUBSCRIPT_GENERIC             10
#define INTRINSIC_TYPEALIAS                     11

#define MAX_INTRINSIC_1                         11


/* Binary Functions: */
#define INTRINSIC_2_INVALID                      0
#define INTRINSIC_PREP_RERAISE_STAR              1
#define INTRINSIC_TYPEVAR_WITH_BOUND             2
#define INTRINSIC_TYPEVAR_WITH_CONSTRAINTS       3
#define INTRINSIC_SET_FUNCTION_TYPE_PARAMS       4

#define MAX_INTRINSIC_2                          4

typedef PyObject *(*intrinsic_func1)(PyThreadState* tstate, PyObject *value);
typedef PyObject *(*intrinsic_func2)(PyThreadState* tstate, PyObject *value1, PyObject *value2);

typedef struct {
    intrinsic_func1 func;
    const char *name;
} intrinsic_func1_info;

typedef struct {
    intrinsic_func2 func;
    const char *name;
} intrinsic_func2_info;

extern const intrinsic_func1_info _PyIntrinsics_UnaryFunctions[];
extern const intrinsic_func2_info _PyIntrinsics_BinaryFunctions[];

#endif  // !Py_INTERNAL_INTRINSIC_H
