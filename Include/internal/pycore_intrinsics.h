

typedef PyObject *(*instrinsic_func1)(PyThreadState* tstate, PyObject *value);


#define INTRINSIC_PRINT 1
#define INTRINSIC_IMPORT_STAR 2
#define INTRINSIC_STOPITERATION_ERROR 3

extern instrinsic_func1
_PyIntrinsics_UnaryFunctions[];
