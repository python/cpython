
/* Unary Functions: */

#define INTRINSIC_PRINT 1
#define INTRINSIC_IMPORT_STAR 2
#define INTRINSIC_STOPITERATION_ERROR 3
#define INTRINSIC_ASYNC_GEN_WRAP 4
#define INTRINSIC_UNARY_POSITIVE 5
#define INTRINSIC_LIST_TO_TUPLE 6
#define INTRINSIC_TYPEVAR 7
#define INTRINSIC_PARAMSPEC 8
#define INTRINSIC_TYPEVARTUPLE 9

#define MAX_INTRINSIC_1 9


/* Binary Functions: */

#define INTRINSIC_PREP_RERAISE_STAR 1
#define INTRINSIC_TYPEVAR_WITH_BOUND 2

#define MAX_INTRINSIC_2 2


typedef PyObject *(*instrinsic_func1)(PyThreadState* tstate, PyObject *value);
typedef PyObject *(*instrinsic_func2)(PyThreadState* tstate, PyObject *value1, PyObject *value2);

extern const instrinsic_func1 _PyIntrinsics_UnaryFunctions[];
extern const instrinsic_func2 _PyIntrinsics_BinaryFunctions[];

