#include "Python.h"
#include "frameobject.h"
#include "rewind.h"

static FILE *rewindLog;
static char rewindInitialized = 0;
static int lastLine = -1;
static PyObject *knownObjectIds;
static PyObject *deallocatedObjectIds;
// static PyObject *currentMethodName;
// static PyObject *currentMethodSelf;
// static PyObject *currentMethodObject;

inline int equalstr(PyObject *obj, char *string) {
    return PyObject_RichCompareBool(obj, PyUnicode_FromString(string), Py_EQ);
}

void Rewind_Initialize() {
    rewindLog = fopen("rewind.log", "w");
}

void Rewind_Initialize2() {
    knownObjectIds = PySet_New(NULL);
    deallocatedObjectIds = PySet_New(NULL);
    rewindInitialized = 1;
}

void Rewind_Cleanup() {
    fclose(rewindLog);
}

void Rewind_PushFrame(PyCodeObject *code, PyFrameObject *frame) {
    if (!rewindInitialized) return;

    if (equalstr(code->co_name, "__init__")) {
        Rewind_TrackObject(*frame->f_localsplus);
    }
    PyObject **valuestack = frame->f_valuestack;
    for (PyObject **p = frame->f_localsplus; p < valuestack; p++) {
        PyObject *obj = *p;
        if (obj != NULL) {
            Rewind_TrackObject(obj);
        }
    }

    lastLine = -1;
    fprintf(rewindLog, "PUSH_FRAME(");
    fprintf(rewindLog, "filename: ");
    PyObject_Print(code->co_filename, rewindLog, Py_PRINT_RAW);
    fprintf(rewindLog, ", name: ");
    PyObject_Print(code->co_name, rewindLog, Py_PRINT_RAW);
    fprintf(rewindLog, ", varnames: ");
    PyObject_Print(code->co_varnames, rewindLog, 0);
    fprintf(rewindLog, ", locals: (");
    
    for (PyObject **p = frame->f_localsplus; p < valuestack; p++) {
        if (p != frame->f_localsplus) {
            fprintf(rewindLog, ", ");
        }
        PyObject *obj = *p;
        if (obj == NULL) {
            fprintf(rewindLog, "None");
        } else {
            
            Rewind_serializeObject(rewindLog, obj);
        }
    }
    fprintf(rewindLog, "))");
    fprintf(rewindLog, "\n");
}

void Rewind_ListAppend(PyListObject *list, PyObject *value) {
    if (!rewindInitialized) return;

    Rewind_TrackObject((PyObject *)list);
    Rewind_TrackObject(value);
    fprintf(rewindLog, "LIST_APPEND(%lu, ", (unsigned long)list);
    Rewind_serializeObject(rewindLog, value);
    fprintf(rewindLog, ")\n");
}

void Rewind_ListInsert(PyListObject *list, Py_ssize_t index, PyObject *value) {
    if (!rewindInitialized) return;

    Rewind_TrackObject((PyObject *)list);
    Rewind_TrackObject(value);
    fprintf(rewindLog, "LIST_INSERT(%lu, %lu", (unsigned long)list, index);
    fprintf(rewindLog, ", ");
    Rewind_serializeObject(rewindLog, value);
    fprintf(rewindLog, ")\n");
}

void Rewind_ListExtend(PyListObject *list, PyObject *iterable) {
    if (!rewindInitialized) return;

    Rewind_TrackObject((PyObject *)list);
    Rewind_TrackObject(iterable);
    fprintf(rewindLog, "LIST_EXTEND(%lu, ", (unsigned long)list);
    Rewind_serializeObject(rewindLog, iterable);
    fprintf(rewindLog, ")\n");
}

void Rewind_ListRemove(PyListObject *list, PyObject *item) {
    if (!rewindInitialized) return;

    Rewind_TrackObject((PyObject *)list);
    Rewind_TrackObject(item);
    fprintf(rewindLog, "LIST_REMOVE(%lu, ", (unsigned long)list);
    Rewind_serializeObject(rewindLog, item);
    fprintf(rewindLog, ")\n");
}

void Rewind_ListPop(PyListObject *list, Py_ssize_t index) {
    if (!rewindInitialized) return;

    Rewind_TrackObject((PyObject *)list);
    fprintf(rewindLog, "LIST_POP(%lu)\n", (unsigned long)list);
}

void Rewind_ListClear(PyListObject *list) {
    if (!rewindInitialized) return;

    Rewind_TrackObject((PyObject *)list);
    fprintf(rewindLog, "LIST_CLEAR(%lu)\n", (unsigned long)list);
}

void Rewind_ListReverse(PyListObject *list) {
    if (!rewindInitialized) return;

    Rewind_TrackObject((PyObject *)list);
    fprintf(rewindLog, "LIST_REVERSE(%lu)\n", (unsigned long)list);
}

void Rewind_ListStoreSubscript(PyListObject *list, PyObject* item, PyObject* value) {
    if (!rewindInitialized) return;

    Rewind_TrackObject((PyObject *)list);
    Rewind_TrackObject(item);
    Rewind_TrackObject(value);
    fprintf(rewindLog, "LIST_STORE_SUBSCRIPT(%lu, ", (unsigned long)list);
    Rewind_serializeObject(rewindLog, item);
    fprintf(rewindLog, ", ");
    Rewind_serializeObject(rewindLog, value);
    fprintf(rewindLog, ")\n");
}

void Rewind_ListDeleteSubscript(PyListObject *list, PyObject *item) {
    if (!rewindInitialized) return;

    Rewind_TrackObject((PyObject *)list);
    Rewind_TrackObject(item);
    fprintf(rewindLog, "LIST_DELETE_SUBSCRIPT(%lu, ", (unsigned long)list);
    Rewind_serializeObject(rewindLog, item);
    fprintf(rewindLog, ")\n");
}

void Rewind_DictUpdate(PyDictObject *dict, PyObject *otherDict) {
    if (!rewindInitialized) return;

    Rewind_TrackObject((PyObject *)dict);
    Rewind_TrackObject(otherDict);
    fprintf(rewindLog, "DICT_UPDATE(%lu, ", (unsigned long)dict);
    Rewind_serializeObject(rewindLog, otherDict);
    fprintf(rewindLog, ")\n");
}

void Rewind_DictClear(PyDictObject *dict) {
    if (!rewindInitialized) return;

    Rewind_TrackObject((PyObject *)dict);
    fprintf(rewindLog, "DICT_CLEAR(%lu)", (unsigned long)PyDict_Copy);
}

void Rewind_DictPop(PyDictObject *dict, PyObject *key) {
    if (!rewindInitialized) return;

    Rewind_TrackObject((PyObject *)dict);
    Rewind_TrackObject(key);
    fprintf(rewindLog, "DICT_POP(%lu, ", (unsigned long)dict);
    Rewind_serializeObject(rewindLog, key);
    fprintf(rewindLog, ")\n");
}

void Rewind_SetAdd(PySetObject *set, PyObject *newItem) {
    if (!rewindInitialized) return;

    Rewind_TrackObject((PyObject *)set);
    Rewind_TrackObject(newItem);
    fprintf(rewindLog, "SET_ADD(%lu, ", (unsigned long)set);
    Rewind_serializeObject(rewindLog, newItem);
    fprintf(rewindLog, ")\n");
}

void Rewind_SetUpdate(PySetObject *set, PyObject *other) {
    if (!rewindInitialized) return;

    Rewind_TrackObject((PyObject *)set);
    Rewind_TrackObject(other);
    fprintf(rewindLog, "SET_UPDATE(%lu, ", (unsigned long)set);
    Rewind_serializeObject(rewindLog, other);
    fprintf(rewindLog, ")\n");
}

void Rewind_SetDiscard(PySetObject *set, PyObject *item) {
    if (!rewindInitialized) return;

    Rewind_TrackObject((PyObject *)set);
    Rewind_TrackObject(item);
    fprintf(rewindLog, "SET_DISCARD(%lu, ", (unsigned long)set);
    Rewind_serializeObject(rewindLog, item);
    fprintf(rewindLog, ")\n");
}

void Rewind_SetRemove(PySetObject *set, PyObject *item) {
    if (!rewindInitialized) return;

    Rewind_TrackObject((PyObject *)set);
    Rewind_TrackObject(item);
    fprintf(rewindLog, "SET_REMOVE(%lu, ", (unsigned long)set);
    Rewind_serializeObject(rewindLog, item);
    fprintf(rewindLog, ")\n");
}

void Rewind_SetClear(PySetObject *set) {
    if (!rewindInitialized) return;

    Rewind_TrackObject((PyObject *)set);
    fprintf(rewindLog, "SET_CLEAR(%lu)\n", (unsigned long)set);
}

void Rewind_StoreSubscript(PyObject *container, PyObject *item, PyObject *value) {
    if (!rewindInitialized) return;

    Rewind_TrackObject(item);
    Rewind_TrackObject(value);
    fprintf(rewindLog, "STORE_SUBSCRIPT(%lu, ", (unsigned long)container);
    Rewind_serializeObject(rewindLog, item);
    fprintf(rewindLog, ", ");
    Rewind_serializeObject(rewindLog, value);
    fprintf(rewindLog, ")\n");
}

void Rewind_DeleteSubscript(PyObject *container, PyObject *item) {
    if (!rewindInitialized) return;

    fprintf(rewindLog, "DELETE_SUBSCRIPT(%lu, ", (unsigned long)container);
    Rewind_serializeObject(rewindLog, item);
    fprintf(rewindLog, ")\n");
}

void Rewind_StoreName(PyObject *name, PyObject *value) {
    if (!rewindInitialized) return;

    Rewind_TrackObject(value);
    fprintf(rewindLog, "STORE_NAME(");
    PyObject_Print(name, rewindLog, 0);
    fprintf(rewindLog, ", ");
    
    Rewind_serializeObject(rewindLog, value);
    fprintf(rewindLog, ")\n");
}

void Rewind_StoreFast(int index, PyObject *value) {
    if (!rewindInitialized) return;

    Rewind_TrackObject(value);
    fprintf(rewindLog, "STORE_FAST(%d, ", index);
    Rewind_serializeObject(rewindLog, value);
    fprintf(rewindLog, ")\n");
}

void Rewind_ReturnValue(PyObject *retval) {
    if (!rewindInitialized) return;

    Rewind_TrackObject(retval);
    fprintf(rewindLog, "RETURN_VALUE(");
    Rewind_serializeObject(rewindLog, retval);
    fprintf(rewindLog, ")\n");
}

void Rewind_SetAttr(PyObject *obj, PyObject *attr, PyObject *value) {
    if (!rewindInitialized) return;

    Rewind_TrackObject(obj);
    Rewind_TrackObject(attr);
    Rewind_TrackObject(value);
    fprintf(rewindLog, "STORE_ATTR(%lu, ", (unsigned long)obj);
    Rewind_serializeObject(rewindLog, attr);
    fprintf(rewindLog, ", ");
    Rewind_serializeObject(rewindLog, value);
    fprintf(rewindLog, ")\n");
}

void Rewind_StringInPlaceAdd(PyObject *left, PyObject *right, PyObject *result) {
    if (!rewindInitialized) return;
    if (left == result || right == result) {
        Rewind_TrackObject(result);
        fprintf(rewindLog, "STRING_INPLACE_ADD_RESULT(%lu, ", (unsigned long)result);
        PyObject_Print(result, rewindLog, 0);
        fprintf(rewindLog, ")\n");
    }
}

void Rewind_TrackObject(PyObject *obj) {
    if (obj == NULL) {
        return;
    }
    // Rewind_FlushDeallocatedIds();
    if (Rewind_isSimpleType(obj)) {
        return;
    }

    PyObject *id = (PyObject *)PyLong_FromLong((long)obj);
    if (PySet_Contains(knownObjectIds, id)) {
        Py_DECREF(id);
        return;
    }

    if (Py_IS_TYPE(obj, &PyList_Type)) {
        PySet_Add(knownObjectIds, id);
        Py_DECREF(id);
    
        PyListObject *list = (PyListObject *)obj;
        for (int i = 0; i < Py_SIZE(list); ++i) {
            PyObject *item = list->ob_item[i];
            Rewind_TrackObject(item);
        }
        fprintf(rewindLog, "NEW_LIST(%lu, ", (unsigned long)obj);
        for (int i = 0; i < Py_SIZE(list); ++i) {
            PyObject *item = list->ob_item[i];
            if (i != 0) {
                fprintf(rewindLog, ", ");
            }
            Rewind_serializeObject(rewindLog, item);
        }
        fprintf(rewindLog, ")\n");
    } else if (Py_IS_TYPE(obj, &PyUnicode_Type)) {
        PySet_Add(knownObjectIds, id);
        Py_DECREF(id);
    
        fprintf(rewindLog, "NEW_STRING(%lu, ", (unsigned long)obj);
        PyObject_Print(obj, rewindLog, 0);
        fprintf(rewindLog, ")\n");
    } else if (Py_IS_TYPE(obj, &PyDict_Type)) {
        PySet_Add(knownObjectIds, id);
        Py_DECREF(id);

        PyObject *items = PyDict_Items(obj);
        PyObject *iterator = PySequence_Fast(items, "argument must be iterable");
        Py_ssize_t n = PySequence_Fast_GET_SIZE(iterator);
        PyObject **iteratorItems = PySequence_Fast_ITEMS(iterator);
        for (int i = 0; i < n; i++) {
            PyObject *item = iteratorItems[i];
            PyObject *key = PyObject_GetItem(item, PyLong_FromLong(0));
            Rewind_TrackObject(key);
            PyObject *value = PyObject_GetItem(item, PyLong_FromLong(1));
            Rewind_TrackObject(value);
        }
        Py_DECREF(iterator);

        fprintf(rewindLog, "NEW_DICT(%lu, ", (unsigned long)obj);
        iterator = PySequence_Fast(items, "argument must be iterable");
        n = PySequence_Fast_GET_SIZE(iterator);
        iteratorItems = PySequence_Fast_ITEMS(iterator);
        for (int i = 0; i < n; i++) {
            if (i != 0) {
                fprintf(rewindLog, ", ");
            }
            PyObject *item = iteratorItems[i];
            PyObject *key = PyObject_GetItem(item, PyLong_FromLong(0));
            PyObject *value = PyObject_GetItem(item, PyLong_FromLong(1));
            Rewind_serializeObject(rewindLog, key);
            fprintf(rewindLog, ", ");
            Rewind_serializeObject(rewindLog, value);
        }
        Py_DECREF(iterator);
        Py_DECREF(items);
        fprintf(rewindLog, ")\n");
    } else if (Py_IS_TYPE(obj, &PySet_Type)) {
        PySet_Add(knownObjectIds, id);
        Py_DECREF(id);

        PyObject *iterator = PySequence_Fast(obj, "argument must be iterable");
        Py_ssize_t n = PySequence_Fast_GET_SIZE(iterator);
        PyObject **iteratorItems = PySequence_Fast_ITEMS(iterator);
        for (int i = 0; i < n; i++) {
            PyObject *item = iteratorItems[i];
            Rewind_TrackObject(item);
        }
        Py_DECREF(iterator);

        fprintf(rewindLog, "NEW_SET(%lu, ", (unsigned long)obj);
        iterator = PySequence_Fast(obj, "argument must be iterable");
        n = PySequence_Fast_GET_SIZE(iterator);
        iteratorItems = PySequence_Fast_ITEMS(iterator);
        for (int i = 0; i < n; i++) {
            if (i != 0) {
                fprintf(rewindLog, ", ");
            }
            PyObject *item = iteratorItems[i];
            Rewind_serializeObject(rewindLog, item);
        }
        fprintf(rewindLog, ")\n");
    } else if (Py_IS_TYPE(obj, &PyTuple_Type)) {
        PySet_Add(knownObjectIds, id);
        Py_DECREF(id);
        PyObject *iterator = PySequence_Fast(obj, "argument must be iterable");
        Py_ssize_t n = PySequence_Fast_GET_SIZE(iterator);
        PyObject **iteratorItems = PySequence_Fast_ITEMS(iterator);
        for (int i = 0; i < n; i++) {
            PyObject *item = iteratorItems[i];
            Rewind_TrackObject(item);
        }
        Py_DECREF(iterator);

        fprintf(rewindLog, "NEW_TUPLE(%lu, ", (unsigned long)obj);
        iterator = PySequence_Fast(obj, "argument must be iterable");
        n = PySequence_Fast_GET_SIZE(iterator);
        iteratorItems = PySequence_Fast_ITEMS(iterator);
        for (int i = 0; i < n; i++) {
            if (i != 0) {
                fprintf(rewindLog, ", ");
            }
            PyObject *item = iteratorItems[i];
            Rewind_serializeObject(rewindLog, item);
        }
        fprintf(rewindLog, ")\n");
    } else {
        PySet_Add(knownObjectIds, id);
        Py_DECREF(id);

        fprintf(rewindLog, "NEW_OBJECT(%lu, ", (unsigned long)obj);
        PyObject *type = PyObject_Type(obj);
        PyObject *typeName = PyObject_GetAttr(type, PyUnicode_FromString("__qualname__"));
        PyObject_Print(typeName, rewindLog, 0);
        fprintf(rewindLog, ", ");
        Rewind_serializeObject(rewindLog, type);
        fprintf(rewindLog, ")\n");
    }
}

inline int Rewind_isSimpleType(PyObject *obj) {
    return (Py_IS_TYPE(obj, &_PyNone_Type) ||
        Py_IS_TYPE(obj, &PyLong_Type) || 
        Py_IS_TYPE(obj, &PyBool_Type));
}

void Rewind_serializeObject(FILE *file, PyObject *obj) {
    if (obj == NULL) {
        fprintf(file, "None");
        return;
    }
    if (Rewind_isSimpleType(obj)) {
        PyObject_Print(obj, file, 0);
    } else {
        fprintf(file, "*%lu", (unsigned long)obj);
    }
}

void Rewind_Dealloc(PyObject *obj) {
    if (!rewindInitialized) return;

    PyObject *id = PyLong_FromLong((long)obj);
    if (PySet_Contains(knownObjectIds, id)) {
        PySet_Discard(knownObjectIds, id);
    }
    PySet_Add(deallocatedObjectIds, id);
}

void Rewind_FlushDeallocatedIds() {
    if (!rewindInitialized) return;

    Py_ssize_t size = PySet_Size(deallocatedObjectIds);
    if (size == 0) {
        return;
    }
    fprintf(rewindLog, "DEALLOC_OBJECT(");
    PyObject *deallocatedObjectIdsList = PyList_New(0);
    _PyList_Extend((PyListObject *)deallocatedObjectIdsList, deallocatedObjectIds);
    PyObject *iterator = PySequence_Fast(deallocatedObjectIdsList, "argument must be iterable");
    Py_ssize_t n = PySequence_Fast_GET_SIZE(iterator);
    PyObject **iteratorItems = PySequence_Fast_ITEMS(iterator);
    for (int i = 0; i < n; i++) {
        PyObject *id = iteratorItems[i];
        if (i != 0) {
            fprintf(rewindLog, ", ");
        }
        PyObject_Print(id, rewindLog, Py_PRINT_RAW);
    }
    fprintf(rewindLog, ")\n");
    Py_DECREF(iterator);
    Py_DECREF(deallocatedObjectIdsList);
    PySet_Clear(deallocatedObjectIds);
}

void printObject(FILE *file, PyObject *obj) {
    PyObject *type = PyObject_Type(obj);
    if (type == (PyObject *)&PyUnicode_Type || 
        type == (PyObject *)&_PyNone_Type ||
        type == (PyObject *)&PyLong_Type || 
        type == (PyObject *)&PyBool_Type) {
        PyObject_Print(obj, file, 0);
    } else {
        PyObject *typeName = PyObject_GetAttr(type, PyUnicode_FromString("__name__"));
        if (PyObject_RichCompareBool(typeName, PyUnicode_FromString("builtin_function_or_method"), Py_EQ) ||
            PyObject_RichCompareBool(typeName, PyUnicode_FromString("function"), Py_EQ) ||
            PyObject_RichCompareBool(typeName, PyUnicode_FromString("method_descriptor"), Py_EQ)) {
            PyObject *methodName = PyObject_GetAttr(obj, PyUnicode_FromString("__qualname__"));
            fprintf(file, "<");
            PyObject_Print(typeName, file, Py_PRINT_RAW);
            fprintf(file, " ");
            PyObject_Print(methodName, file, Py_PRINT_RAW);
            fprintf(file, "()");
            fprintf(file, ">");
        } else {
            fprintf(file, "<object ");
            PyObject_Print(typeName, file, Py_PRINT_RAW);
            fprintf(file, "(%lu)>", (unsigned long)obj);
        }
    }
}

void printStack(FILE *file, PyObject **stack_pointer, int level) {
    fprintf(file, "-- Stack(%d): [", level);
    for (int i = 1; i <= level; i++) {
        PyObject *obj = stack_pointer[-i];
        if (i != 1) {
            fprintf(file, ", ");
        }
        if (obj) {
            printObject(file, obj);
        }
    }
    fprintf(file, "]\n");
}

void logOp(char *label, PyObject **stack_pointer, int level, PyFrameObject *frame, int oparg) {
    if (!rewindInitialized) return;

    int lineNo = PyFrame_GetLineNumber(frame);
    if (rewindLog) {
        if (lastLine != lineNo) {
            fprintf(rewindLog, "VISIT #%d\n", lineNo);
        }
        lastLine = lineNo;
    }
    // printStack(rewindLog, stack_pointer, level);
    // fprintf(rewindLog, "-- %s(%d) on #%d\n", label, oparg, lineNo);
}

void Rewind_LoadMethod(PyObject *obj, PyObject *name, PyObject *method) {
    // if (!rewindInitialized) return;
    
    // currentMethodSelf = obj;
    // currentMethodName = name;
    // currentMethodObject = method;
}

void Rewind_CallMethod(PyObject *method, PyObject **stack_pointer, int level) {
    // if (!rewindInitialized) return;

    // if (currentMethodSelf && currentMethodSelf) {
    //     if (Py_IS_TYPE(currentMethodSelf, &PyList_Type)) {
    //         if (equalstr(currentMethodName, "append")) {
    //             // PyObject *newItem = stack_pointer[-1];
    //             // Rewind_ListAppend(currentMethodSelf, newItem);
    //         } else if (equalstr(currentMethodName, "remove")) {
    //             // PyObject *item = stack_pointer[-1];
    //             // fprintf(rewindLog, "LIST_REMOVE(%lu, ", (unsigned long)currentMethodSelf);
    //             // Rewind_serializeObject(rewindLog, item);
    //             // fprintf(rewindLog, ")\n");
    //         } else if (equalstr(currentMethodName, "extend")) {
    //             // PyObject *iterable = stack_pointer[-1];
    //             // Rewind_ListExtend(currentMethodSelf, iterable);
    //         } else if (equalstr(currentMethodName, "insert")) {
    //             // PyObject *value = stack_pointer[-1];
    //             // PyObject *index = stack_pointer[-2];
    //             // Rewind_TrackObject(value);
    //             // fprintf(rewindLog, "LIST_INSERT(%lu, ", (unsigned long)currentMethodSelf);
    //             // Rewind_serializeObject(rewindLog, index);
    //             // fprintf(rewindLog, ", ");
    //             // Rewind_serializeObject(rewindLog, value);
    //             // fprintf(rewindLog, ")\n");
    //         } else if (equalstr(currentMethodName, "pop")) {
    //             // fprintf(rewindLog, "LIST_POP(%lu)\n", (unsigned long)currentMethodSelf);
    //         } else if (equalstr(currentMethodName, "clear")) {
    //             // fprintf(rewindLog, "LIST_CLEAR(%lu)\n", (unsigned long)currentMethodSelf);
    //         } else if (equalstr(currentMethodName, "reverse")) {
    //             // fprintf(rewindLog, "LIST_REVERSE(%lu)\n", (unsigned long)currentMethodSelf);
    //         }
    //     } else if (Py_IS_TYPE(currentMethodSelf, &PyDict_Type)) {
    //         if (equalstr(currentMethodName, "update")) {
    //             // PyObject *otherDict = stack_pointer[-1];
    //             // Rewind_TrackObject(otherDict);
    //             // fprintf(rewindLog, "DICT_UPDATE(%lu, ", (unsigned long)currentMethodSelf);
    //             // Rewind_serializeObject(rewindLog, otherDict);
    //             // fprintf(rewindLog, ")\n");
    //         } else if (equalstr(currentMethodName, "clear")) {
    //             // fprintf(rewindLog, "DICT_CLEAR(%lu)", (unsigned long)currentMethodSelf);
    //         } else if (equalstr(currentMethodName, "pop")) {
    //             // PyObject *key = stack_pointer[-1];
    //             // Rewind_TrackObject(key);
    //             // fprintf(rewindLog, "DICT_POP(%lu, ", (unsigned long)currentMethodSelf);
    //             // Rewind_serializeObject(rewindLog, key);
    //             // fprintf(rewindLog, ")\n");
    //         }
    //     } else if (Py_IS_TYPE(currentMethodSelf, &PySet_Type)) {
    //         if (equalstr(currentMethodName, "add")) {
    //             // PyObject *newItem = stack_pointer[-1];
    //             // Rewind_TrackObject(newItem);
    //             // fprintf(rewindLog, "SET_ADD(%lu, ", (unsigned long)currentMethodSelf);
    //             // Rewind_serializeObject(rewindLog, newItem);
    //             // fprintf(rewindLog, ")\n");
    //         } else if (equalstr(currentMethodName, "update")) {
    //             // PyObject *other = stack_pointer[-1];
    //             // Rewind_TrackObject(other);
    //             // fprintf(rewindLog, "SET_UPDATE(%lu, ", (unsigned long)currentMethodSelf);
    //             // Rewind_serializeObject(rewindLog, other);
    //             // fprintf(rewindLog, ")\n");
    //         } else if (equalstr(currentMethodName, "discard")) {
    //             // PyObject *item = stack_pointer[-1];
    //             // Rewind_TrackObject(item);
    //             // fprintf(rewindLog, "SET_DISCARD(%lu, ", (unsigned long)currentMethodSelf);
    //             // Rewind_serializeObject(rewindLog, item);
    //             // fprintf(rewindLog, ")\n");
    //         } else if (equalstr(currentMethodName, "remove")) {
    //             // PyObject *item = stack_pointer[-1];
    //             // Rewind_TrackObject(item);
    //             // fprintf(rewindLog, "SET_REMOVE(%lu, ", (unsigned long)currentMethodSelf);
    //             // Rewind_serializeObject(rewindLog, item);
    //             // fprintf(rewindLog, ")\n");
    //         } else if (equalstr(currentMethodName, "clear")) {
    //             // fprintf(rewindLog, "SET_CLEAR(%lu)\n", (unsigned long)currentMethodSelf);
    //         }
    //     }
    // }
    // currentMethodSelf = NULL;
    // currentMethodName = NULL;
    // currentMethodObject = NULL;
}

void Rewind_CallFunction(PyObject **sp, int oparg) {
    // if (!rewindInitialized) return;
    
    // PyObject *func = *(sp - oparg - 1);
    // PyObject *qualNameStr = PyUnicode_FromString("__qualname__");
    // if (PyObject_HasAttr(func, qualNameStr)) {
    //     PyObject *name = PyObject_GetAttr(func, qualNameStr);
    //     PyObject *type = PyObject_Type(func);
    //     PyObject *typeName = PyObject_GetAttr(type, PyUnicode_FromString("__name__"));
    //     if (equalstr(typeName, "builtin_function_or_method")) {
    //         fprintf(rewindLog, "CALL_BUILT_IN_FUNCTION(");
    //         PyObject_Print(name, rewindLog, 0);
    //         fprintf(rewindLog, ")\n");
    //     }
    // }
}
