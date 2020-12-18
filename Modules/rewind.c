#include "Python.h"
#include "frameobject.h"
#include "rewind.h"

static FILE *rewindLog;
static char rewindInitialized = 0;
static int lastLine = -1;
static PyObject *knownObjectIds;
static PyObject *currentMethodName;
static PyObject *currentMethodSelf;
static PyObject *currentMethodObject;

void Rewind_Initialize() {
    rewindLog = fopen("rewind.log", "w");
}

void Rewind_Initialize2() {
    knownObjectIds = PySet_New(NULL);
    rewindInitialized = 1;
}

void Rewind_Cleanup() {
    fclose(rewindLog);
}

void Rewind_PushFrame(PyCodeObject *code, PyFrameObject *frame) {
    if (!rewindInitialized) return;

    lastLine = -1;
    fprintf(rewindLog, "PUSH_FRAME(");
    fprintf(rewindLog, "filename: ");
    PyObject_Print(code->co_filename, rewindLog, Py_PRINT_RAW);
    fprintf(rewindLog, ", name: ");
    PyObject_Print(code->co_name, rewindLog, Py_PRINT_RAW);
    /*
    fprintf(rewindLog, ", names: ");
    PyObject_Print(code->co_names, rewindLog, 0);
    fprintf(rewindLog, ", consts: ");
    PyObject_Print(code->co_consts, rewindLog, 0);
    */
    fprintf(rewindLog, ", locals: (");
    PyObject **valuestack = frame->f_valuestack;
    for (PyObject **p = frame->f_localsplus; p < valuestack; p++) {
        if (p != frame->f_localsplus) {
            fprintf(rewindLog, ", ");
        }
        PyObject *obj = *p;
        if (obj != NULL) {
            Rewind_serializeObject(rewindLog, obj);
        }
    }
    fprintf(rewindLog, "))");
    fprintf(rewindLog, "\n");
}

void Rewind_BuildList(PyObject *list) {
    if (!rewindInitialized) return;

    Rewind_TrackObject(list);
}

void Rewind_ListExtend(PyObject *list, PyObject *iterable) {
    if (!rewindInitialized) return;

    PyObject *iterator = PySequence_Fast(iterable, "argument must be iterable");
    Py_ssize_t n = PySequence_Fast_GET_SIZE(iterator);
    PyObject **items = PySequence_Fast_ITEMS(iterator);
    for (int i = 0; i < n; i++) {
        Rewind_TrackObject(items[i]);
    }

    fprintf(rewindLog, "LIST_EXTEND(%lu, ", (unsigned long)list);
    iterator = PySequence_Fast(iterable, "argument must be iterable");
    n = PySequence_Fast_GET_SIZE(iterator);
    items = PySequence_Fast_ITEMS(iterator);
    for (int i = 0; i < n; i++) {
        if (i != 0) {
            fprintf(rewindLog, ", ");
        }
        Rewind_serializeObject(rewindLog, items[i]);
    }
    fprintf(rewindLog, ")\n");
}

void Rewind_ListAppend(PyObject *list, PyObject *value) {
    if (!rewindInitialized) return;

    Rewind_TrackObject(value);
    fprintf(rewindLog, "LIST_APPEND(%lu, ", (unsigned long)list);
    Rewind_serializeObject(rewindLog, value);
    fprintf(rewindLog, ")\n");
}

inline int equalstr(PyObject *obj, char *string) {
    return PyObject_RichCompareBool(obj, PyUnicode_FromString(string), Py_EQ);
}

void Rewind_LoadMethod(PyObject *obj, PyObject *name, PyObject *method) {
    if (!rewindInitialized) return;
    
    currentMethodSelf = obj;
    currentMethodName = name;
    currentMethodObject = method;
}

void Rewind_CallMethod(PyObject *method, PyObject **stack_pointer, int level) {
    if (!rewindInitialized) return;

    if (currentMethodSelf && currentMethodSelf) {
        if (Py_IS_TYPE(currentMethodSelf, &PyList_Type)) {
            if (equalstr(currentMethodName, "append")) {
                PyObject *newItem = stack_pointer[-1];
                Rewind_TrackObject(newItem);
                fprintf(rewindLog, "LIST_APPEND(%lu, ", (unsigned long)currentMethodSelf);
                Rewind_serializeObject(rewindLog, newItem);
                fprintf(rewindLog, ")\n");
                //printStack(rewindLog, stack_pointer, level);
            } else if (equalstr(currentMethodName, "remove")) {
                PyObject *item = stack_pointer[-1];
                fprintf(rewindLog, "LIST_REMOVE(%lu, ", (unsigned long)currentMethodSelf);
                Rewind_serializeObject(rewindLog, item);
                fprintf(rewindLog, ")\n");
            } else if (equalstr(currentMethodName, "extend")) {
                PyObject *iterable = stack_pointer[-1];
                Rewind_TrackObject(iterable);
                fprintf(rewindLog, "LIST_EXTEND(%lu, ", (unsigned long)currentMethodSelf);
                Rewind_serializeObject(rewindLog, iterable);
                fprintf(rewindLog, ")\n");
            } else if (equalstr(currentMethodName, "insert")) {
                PyObject *value = stack_pointer[-1];
                PyObject *index = stack_pointer[-2];
                Rewind_TrackObject(value);
                fprintf(rewindLog, "LIST_INSERT(%lu, ", (unsigned long)currentMethodSelf);
                Rewind_serializeObject(rewindLog, index);
                fprintf(rewindLog, ", ");
                Rewind_serializeObject(rewindLog, value);
                fprintf(rewindLog, ")\n");
            } else if (equalstr(currentMethodName, "pop")) {
                fprintf(rewindLog, "LIST_POP(%lu)\n", (unsigned long)currentMethodSelf);
            } else if (equalstr(currentMethodName, "clear")) {
                fprintf(rewindLog, "LIST_CLEAR(%lu)\n", (unsigned long)currentMethodSelf);
            } else if (equalstr(currentMethodName, "reverse")) {
                fprintf(rewindLog, "LIST_REVERSE(%lu)\n", (unsigned long)currentMethodSelf);
            }
        }
    }
    currentMethodSelf = NULL;
    currentMethodName = NULL;
    currentMethodObject = NULL;
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
    Rewind_serializeObject(rewindLog, name);
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

    fprintf(rewindLog, "RETURN_VALUE(");
    Rewind_serializeObject(rewindLog, retval);
    fprintf(rewindLog, ")\n");
}

void Rewind_TrackObject(PyObject *obj) {
    if (!Py_IS_TYPE(obj, &PyList_Type)) {
        return;
    }
    
    PyObject *id = (PyObject *)PyLong_FromLong((long)obj);
    if (PySet_Contains(knownObjectIds, id)) {
        Py_DECREF(id);
        return;
    }

    PySet_Add(knownObjectIds, id);
    Py_DECREF(id);
    if (Py_IS_TYPE(obj, &PyList_Type)) {
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
        
    }
}

void Rewind_serializeObject(FILE *file, PyObject *obj) {
    PyObject *type = PyObject_Type(obj);
    if (type == (PyObject *)&PyUnicode_Type || 
        type == (PyObject *)&_PyNone_Type ||
        type == (PyObject *)&PyLong_Type || 
        type == (PyObject *)&PyBool_Type) {
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
    fprintf(file, "  Stack(%d): [", level);
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
    printStack(rewindLog, stack_pointer, level);
    fprintf(rewindLog, "-- %s(%d) on #%d\n", label, oparg, lineNo);
}