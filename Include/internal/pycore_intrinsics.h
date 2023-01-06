
#define INTRINSIC_PRINT 1
#define INTRINSIC_IMPORT_STAR 2
#define INTRINSIC_STOPITERATION_ERROR 3
#define INTRINSIC_ASYNC_GEN_WRAP 4
#define INTRINSIC_UNARY_POSITIVE 5
#define INTRINSIC_LIST_TO_TUPLE 6

#define MAX_INTRINSIC_1 6

typedef PyObject *(*instrinsic_func1)(PyThreadState* tstate, PyObject *value);

extern instrinsic_func1 _PyIntrinsics_UnaryFunctions[];
