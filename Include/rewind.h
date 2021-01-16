#include "Python.h"

void Rewind_Initialize(void);

void Rewind_Initialize2(void);

int Rewind_isSimpleType(PyObject *obj);

void Rewind_Cleanup(void);

void Rewind_PushFrame(PyCodeObject *code, PyFrameObject *frame);

void Rewind_BuildList(PyObject *list);

void Rewind_ListAppend(PyListObject *list, PyObject *value);

void Rewind_ListInsert(PyListObject *list, Py_ssize_t index, PyObject *value);

void Rewind_ListExtend(PyListObject *list, PyObject *iterable);

void Rewind_ListRemove(PyListObject *list, PyObject *item);

void Rewind_ListPop(PyListObject *list, Py_ssize_t index);

void Rewind_ListClear(PyListObject *list);

void Rewind_ListReverse(PyListObject *list);

void Rewind_ListSort(PyListObject *list);

void Rewind_ListStoreSubscript(PyListObject *list, PyObject* item, PyObject* value);

void Rewind_ListStoreItem(PyListObject *list, Py_ssize_t index, PyObject* value);

void Rewind_ListDeleteSubscript(PyListObject *list, PyObject *item);

void Rewind_DictUpdate(PyDictObject *dict, PyObject *otherDict);

void Rewind_DictClear(PyDictObject *dict);

void Rewind_DictPop(PyDictObject *dict, PyObject *key);

void Rewind_SetAdd(PySetObject *set, PyObject *newItem);

void Rewind_SetUpdate(PySetObject *set, PyObject *other);

void Rewind_SetDiscard(PySetObject *set, PyObject *item);

void Rewind_SetRemove(PySetObject *set, PyObject *item);

void Rewind_SetClear(PySetObject *set);

void Rewind_LoadMethod(PyObject *obj, PyObject *name, PyObject *method);

void Rewind_CallMethod(PyObject *method, PyObject **stack_pointer, int level);

void Rewind_CallFunction(PyObject **sp, int oparg);

void Rewind_StoreSubscript(PyObject *container, PyObject *item, PyObject *value);

void Rewind_DeleteSubscript(PyObject *container, PyObject *item);

void Rewind_StoreName(PyObject *name, PyObject *value);

void Rewind_StoreFast(int index, PyObject *value);

void Rewind_ReturnValue(PyObject *retval);

void Rewind_SetAttr(PyObject *obj, PyObject *attr, PyObject *value);

void Rewind_StringInPlaceAdd(PyObject *left, PyObject *right, PyObject *result);

void Rewind_Dealloc(PyObject *obj);

void Rewind_FlushDeallocatedIds(void);

void Rewind_TrackObject(PyObject *obj);

void Rewind_serializeObject(FILE *file, PyObject *obj);

void printObject(FILE *file, PyObject *obj);

void printStack(FILE *file, PyObject **stack_pointer, int level);

void logOp(char *label, PyObject **stack_pointer, int level, PyFrameObject *frame, int oparg);