#include "Python.h"

void Rewind_Initialize(void);

void Rewind_Initialize2(void);

void Rewind_Cleanup(void);

void Rewind_PushFrame(PyCodeObject *code, PyFrameObject *frame);

void Rewind_BuildList(PyObject *list);

void Rewind_ListExtend(PyObject *list, PyObject *iterable);

void Rewind_ListAppend(PyObject *list, PyObject *value);

void Rewind_LoadMethod(PyObject *obj, PyObject *name, PyObject *method);

void Rewind_CallMethod(PyObject *method, PyObject **stack_pointer, int level);

void Rewind_StoreSubscript(PyObject *container, PyObject *item, PyObject *value);

void Rewind_DeleteSubscript(PyObject *container, PyObject *item);

void Rewind_StoreName(PyObject *name, PyObject *value);

void Rewind_StoreFast(int index, PyObject *value);

void Rewind_ReturnValue(PyObject *retval);

void Rewind_Dealloc(PyObject *obj);

void Rewind_TrackObject(PyObject *obj);

void Rewind_serializeObject(FILE *file, PyObject *obj);

void printObject(FILE *file, PyObject *obj);

void printStack(FILE *file, PyObject **stack_pointer, int level);

void logOp(char *label, PyObject **stack_pointer, int level, PyFrameObject *frame, int oparg);