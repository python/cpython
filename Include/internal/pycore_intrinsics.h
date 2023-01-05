
#define INTRINSIC_PRINT 1
#define INTRINSIC_IMPORT_STAR 2
#define INTRINSIC_STOPITERATION_ERROR 3

#define MAX_INTRINSIC_1 3

typedef PyObject *(*instrinsic_func1)(PyThreadState* tstate, PyObject *value);

extern instrinsic_func1 _PyIntrinsics_UnaryFunctions[];
