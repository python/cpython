#include "Python.h"
#include "frameobject.h"
#include "rewind.h"

static FILE *rewindLog;
static char rewindActive = 0;
static int lastLine = -1;
static PyObject *knownObjectIds;

inline int equalstr(PyObject *obj, char *string) {
    return PyObject_RichCompareBool(obj, PyUnicode_FromString(string), Py_EQ);
}

void Rewind_Activate() {
    rewindLog = fopen("rewind.log", "w");
    knownObjectIds = PySet_New(NULL);
    rewindActive = 1;
}

void Rewind_Deactivate() {
    rewindActive = 0;
    fclose(rewindLog);
}

void Rewind_PushFrame(PyCodeObject *code, PyFrameObject *frame) {
    if (!rewindActive) return;

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
    PyObject_Print(code->co_filename, rewindLog, 0);
    fprintf(rewindLog, ", ");
    PyObject_Print(code->co_name, rewindLog, 0);
    
    PyTupleObject *varnames = (PyTupleObject *)code->co_varnames;
    Py_ssize_t numVarnames = Py_SIZE(varnames);
    fprintf(rewindLog, ", %lu", numVarnames);

    for (int i = 0; i < numVarnames; i++) {
        PyObject *item = varnames->ob_item[i];
        fprintf(rewindLog, ", ");
        PyObject_Print(item, rewindLog, 0);
    }
    
    PyObject **p = frame->f_localsplus;
    while (p < valuestack) {
        fprintf(rewindLog, ", ");
        PyObject *obj = *p;
        if (obj == NULL) {
            fprintf(rewindLog, "None");
        } else {
            
            Rewind_serializeObject(rewindLog, obj);
        }
        p++;
    }
    fprintf(rewindLog, ")");
    fprintf(rewindLog, "\n");
}

void Rewind_ListAppend(PyListObject *list, PyObject *value) {
    if (!rewindActive) return;

    Rewind_TrackObject((PyObject *)list);
    Rewind_TrackObject(value);
    fprintf(rewindLog, "LIST_APPEND(%lu, ", (unsigned long)list);
    Rewind_serializeObject(rewindLog, value);
    fprintf(rewindLog, ")\n");
}

void Rewind_ListInsert(PyListObject *list, Py_ssize_t index, PyObject *value) {
    if (!rewindActive) return;

    Rewind_TrackObject((PyObject *)list);
    Rewind_TrackObject(value);
    fprintf(rewindLog, "LIST_INSERT(%lu, %lu", (unsigned long)list, index);
    fprintf(rewindLog, ", ");
    Rewind_serializeObject(rewindLog, value);
    fprintf(rewindLog, ")\n");
}

void Rewind_ListExtend(PyListObject *list, PyObject *iterable) {
    if (!rewindActive) return;

    Rewind_TrackObject((PyObject *)list);
    Rewind_TrackObject(iterable);
    fprintf(rewindLog, "LIST_EXTEND(%lu, ", (unsigned long)list);
    Rewind_serializeObject(rewindLog, iterable);
    fprintf(rewindLog, ")\n");
}

void Rewind_ListRemove(PyListObject *list, PyObject *item) {
    if (!rewindActive) return;

    Rewind_TrackObject((PyObject *)list);
    Rewind_TrackObject(item);
    fprintf(rewindLog, "LIST_REMOVE(%lu, ", (unsigned long)list);
    Rewind_serializeObject(rewindLog, item);
    fprintf(rewindLog, ")\n");
}

void Rewind_ListPop(PyListObject *list, Py_ssize_t index) {
    if (!rewindActive) return;

    Rewind_TrackObject((PyObject *)list);
    fprintf(rewindLog, "LIST_POP(%lu, %lu)\n", (unsigned long)list, index);
}

void Rewind_ListClear(PyListObject *list) {
    if (!rewindActive) return;

    Rewind_TrackObject((PyObject *)list);
    fprintf(rewindLog, "LIST_CLEAR(%lu)\n", (unsigned long)list);
}

void Rewind_ListReverse(PyListObject *list) {
    if (!rewindActive) return;

    Rewind_TrackObject((PyObject *)list);
    fprintf(rewindLog, "LIST_REVERSE(%lu)\n", (unsigned long)list);
}

void Rewind_ListSort(PyListObject *list) {
    if (!rewindActive) return;

    Rewind_TrackObject((PyObject *)list);
    fprintf(rewindLog, "LIST_SORT(%lu, ", (unsigned long)list);
    for (int i = 0; i < Py_SIZE(list); ++i) {
        PyObject *item = list->ob_item[i];
        if (i != 0) {
            fprintf(rewindLog, ", ");
        }
        Rewind_serializeObject(rewindLog, item);
    }
    fprintf(rewindLog, ")\n");
}

void Rewind_ListStoreSubscript(PyListObject *list, PyObject* item, PyObject* value) {
    if (!rewindActive) return;

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
    if (!rewindActive) return;

    Rewind_TrackObject((PyObject *)list);
    Rewind_TrackObject(item);
    fprintf(rewindLog, "LIST_DELETE_SUBSCRIPT(%lu, ", (unsigned long)list);
    Rewind_serializeObject(rewindLog, item);
    fprintf(rewindLog, ")\n");
}

void Rewind_DictStoreSubscript(PyDictObject *dict, PyObject *key, PyObject *value) {
    if (!rewindActive) return;

    Rewind_TrackObject((PyObject *)dict);
    Rewind_TrackObject(key);
    Rewind_TrackObject(value);
    fprintf(rewindLog, "DICT_STORE_SUBSCRIPT(%lu, ", (unsigned long)dict);
    Rewind_serializeObject(rewindLog, key);
    fprintf(rewindLog, ", ");
    Rewind_serializeObject(rewindLog, value);
    fprintf(rewindLog, ")\n");
}

void Rewind_DictDeleteSubscript(PyDictObject *dict, PyObject *item) {
    if (!rewindActive) return;

    Rewind_TrackObject((PyObject *)dict);
    Rewind_TrackObject(item);
    fprintf(rewindLog, "DICT_DELETE_SUBSCRIPT(%lu, ", (unsigned long)dict);
    Rewind_serializeObject(rewindLog, item);
    fprintf(rewindLog, ")\n");
}

void Rewind_DictUpdate(PyDictObject *dict, PyObject *otherDict) {
    if (!rewindActive) return;

    Rewind_TrackObject((PyObject *)dict);
    Rewind_TrackObject(otherDict);
    fprintf(rewindLog, "DICT_UPDATE(%lu, ", (unsigned long)dict);
    Rewind_serializeObject(rewindLog, otherDict);
    fprintf(rewindLog, ")\n");
}

void Rewind_DictClear(PyDictObject *dict) {
    if (!rewindActive) return;

    Rewind_TrackObject((PyObject *)dict);
    fprintf(rewindLog, "DICT_CLEAR(%lu)\n", (unsigned long)dict);
}

void Rewind_DictPop(PyDictObject *dict, PyObject *key) {
    if (!rewindActive) return;

    Rewind_TrackObject((PyObject *)dict);
    Rewind_TrackObject(key);
    fprintf(rewindLog, "DICT_POP(%lu, ", (unsigned long)dict);
    Rewind_serializeObject(rewindLog, key);
    fprintf(rewindLog, ")\n");
}

void Rewind_DictPopItem(PyDictObject *dict, PyObject *key) {
    if (!rewindActive) return;

    Rewind_TrackObject((PyObject *)dict);
    Rewind_TrackObject(key);
    fprintf(rewindLog, "DICT_POP_ITEM(%lu, ", (unsigned long)dict);
    Rewind_serializeObject(rewindLog, key);
    fprintf(rewindLog, ")\n");
}

void Rewind_SetAdd(PySetObject *set, PyObject *newItem) {
    if (!rewindActive) return;

    Rewind_TrackObject((PyObject *)set);
    Rewind_TrackObject(newItem);
    fprintf(rewindLog, "SET_ADD(%lu, ", (unsigned long)set);
    Rewind_serializeObject(rewindLog, newItem);
    fprintf(rewindLog, ")\n");
}

void Rewind_SetDiscard(PySetObject *set, PyObject *item) {
    if (!rewindActive) return;

    Rewind_TrackObject((PyObject *)set);
    Rewind_TrackObject(item);
    fprintf(rewindLog, "SET_DISCARD(%lu, ", (unsigned long)set);
    Rewind_serializeObject(rewindLog, item);
    fprintf(rewindLog, ")\n");
}

void Rewind_SetClear(PySetObject *set) {
    if (!rewindActive) return;

    Rewind_TrackObject((PyObject *)set);
    fprintf(rewindLog, "SET_CLEAR(%lu)\n", (unsigned long)set);
}

void Rewind_SetPrintItems(PySetObject *set) {
    Py_ssize_t pos;
    PyObject *key;
    Py_hash_t hash;
    pos = 0;
    while (_PySet_NextEntry((PyObject *)set, &pos, &key, &hash)) {
        fprintf(rewindLog, ", ");
        Rewind_serializeObject(rewindLog, key);
    }
}

void Rewind_SetUpdate(PySetObject *set) {
    Rewind_TrackObject((PyObject *)set);
    fprintf(rewindLog, "SET_UPDATE(%lu", (unsigned long)set);
    Rewind_SetPrintItems(set);
    fprintf(rewindLog, ")\n");
}

void Rewind_StoreName(PyObject *name, PyObject *value) {
    if (!rewindActive) return;

    Rewind_TrackObject(value);
    fprintf(rewindLog, "STORE_NAME(");
    PyObject_Print(name, rewindLog, 0);
    fprintf(rewindLog, ", ");
    
    Rewind_serializeObject(rewindLog, value);
    fprintf(rewindLog, ")\n");
}

void Rewind_StoreFast(int index, PyObject *value) {
    if (!rewindActive) return;

    Rewind_TrackObject(value);
    fprintf(rewindLog, "STORE_FAST(%d, ", index);
    Rewind_serializeObject(rewindLog, value);
    fprintf(rewindLog, ")\n");
}

void Rewind_ReturnValue(PyObject *retval) {
    if (!rewindActive) return;

    Rewind_TrackObject(retval);
    fprintf(rewindLog, "RETURN_VALUE(");
    Rewind_serializeObject(rewindLog, retval);
    fprintf(rewindLog, ")\n");
}

void Rewind_SetAttr(PyObject *obj, PyObject *attr, PyObject *value) {
    if (!rewindActive) return;

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
    if (!rewindActive) return;
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
    //Rewind_FlushDeallocatedIds();
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
        fprintf(rewindLog, "NEW_LIST(%lu", (unsigned long)obj);
        for (int i = 0; i < Py_SIZE(list); ++i) {
            PyObject *item = list->ob_item[i];
            fprintf(rewindLog, ", ");
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
        Py_ssize_t pos;
        PyObject *key;
        Py_hash_t hash;
        pos = 0;

        while (_PySet_NextEntry(obj, &pos, &key, &hash)) {
            Rewind_TrackObject(key);
        }

        fprintf(rewindLog, "NEW_SET(%lu", (unsigned long)obj);
        
        pos = 0;
        while (_PySet_NextEntry(obj, &pos, &key, &hash)) {
            fprintf(rewindLog, ", ");
            Rewind_serializeObject(rewindLog, key);
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
    if (!rewindActive) return;

    PyObject *id = PyLong_FromLong((long)obj);
    if (PySet_Contains(knownObjectIds, id)) {
        PySet_Discard(knownObjectIds, id);
        fprintf(rewindLog, "DEALLOC(%lu)\n", (unsigned long)obj);
    }
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
    if (!rewindActive) return;

    int lineNo = PyFrame_GetLineNumber(frame);
    if (rewindLog) {
        if (lastLine != lineNo) {
            fprintf(rewindLog, "VISIT(%d)\n", lineNo);
        }
        lastLine = lineNo;
    }
    // printStack(rewindLog, stack_pointer, level);
    fprintf(rewindLog, "-- %s(%d) on #%d\n", label, oparg, lineNo);
}
