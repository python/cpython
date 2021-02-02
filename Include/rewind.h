#include "Python.h"

void Rewind_Activate(const wchar_t * filename);

void Rewind_Deactivate(void);

int Rewind_isSimpleType(PyObject *obj);

void Rewind_Cleanup(void);

void Rewind_PushFrame(PyFrameObject *frame);

void Rewind_PopFrame(PyFrameObject *frame);

void Rewind_StoreDeref(PyObject *cell, PyObject *value);

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

void Rewind_DictStoreSubscript(PyDictObject *dict, PyObject *key, PyObject *value);

void Rewind_DictDeleteSubscript(PyDictObject *dict, PyObject *item);

void Rewind_DictUpdate(PyDictObject *dict, PyObject *otherDict);

void Rewind_DictClear(PyDictObject *dict);

void Rewind_DictPop(PyDictObject *dict, PyObject *key);

void Rewind_DictPopItem(PyDictObject *dict, PyObject *key);

void Rewind_DictSetDefault(PyDictObject *dict, PyObject *key, PyObject *value);

void Rewind_SetAdd(PySetObject *set, PyObject *newItem);

void Rewind_SetDiscard(PySetObject *set, PyObject *item);

void Rewind_SetClear(PySetObject *set);

void Rewind_SetUpdate(PySetObject *set);

void Rewind_YieldValue(PyObject *retval);

void Rewind_StoreName(PyObject *ns, PyObject *name, PyObject *value);

void Rewind_StoreFast(int index, PyObject *value);

void Rewind_StoreGlobal(PyObject *ns, PyObject *name, PyObject *value);

void Rewind_DeleteGlobal(PyObject *ns, PyObject *name);

void Rewind_ReturnValue(PyObject *retval);

void Rewind_SetAttr(PyObject *obj, PyObject *attr, PyObject *value);

void Rewind_StringInPlaceAdd(PyObject *left, PyObject *right, PyObject *result);

void Rewind_Dealloc(PyObject *obj);

void Rewind_FlushDeallocatedIds(void);

void Rewind_TrackObject(PyObject *obj);

void Rewind_serializeObject(FILE *file, PyObject *obj);

void Rewind_Log(char *message);

void printObject(FILE *file, PyObject *obj);

void printStack(FILE *file, PyObject **stack_pointer, int level);

void logOp(char *label, PyObject **stack_pointer, int level, PyFrameObject *frame, int oparg);