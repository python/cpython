/* Boolean object interface */

#ifndef Py_BOOLOBJECT_H
#define Py_BOOLOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif


PyAPI_DATA(PyTypeObject) PyBool_Type;

#define PyBool_Check(x) Py_IS_TYPE(x, &PyBool_Type)

/* Py_False and Py_True are the only two bools in existence.
Don't forget to apply Py_INCREF() when returning either!!! */

/* Don't use these directly */
PyAPI_DATA(PyLongObject) _Py_FalseStruct;
PyAPI_DATA(PyLongObject) _Py_TrueStruct;

/* Use these macros */
#define Py_False ((PyObject *) &_Py_FalseStruct)
#define Py_True ((PyObject *) &_Py_TrueStruct)

// Return a new reference to the False singleton.
// The function cannot return NULL.
static inline PyObject *Py_RefFalse(void)
{
    return Py_NewRef(Py_False);
}

// Return a new reference to the True singleton.
// The function cannot return NULL.
static inline PyObject *Py_RefTrue(void)
{
    return Py_NewRef(Py_True);
}

// Test if an object is the True singleton, the same as "x is True" in Python.
PyAPI_FUNC(int) Py_IsTrue(PyObject *x);
#define Py_IsTrue(x) Py_Is((x), Py_True)

// Test if an object is the False singleton, the same as "x is False" in Python.
PyAPI_FUNC(int) Py_IsFalse(PyObject *x);
#define Py_IsFalse(x) Py_Is((x), Py_False)

/* Macros for returning Py_True or Py_False, respectively */
#define Py_RETURN_TRUE return Py_RefTrue()
#define Py_RETURN_FALSE return Py_RefFalse()

// Function to return a bool from a C long
// The function cannot return NULL.
PyAPI_FUNC(PyObject *) PyBool_FromLong(long);

static inline PyObject *_PyBool_FromLong(long ok)
{
    return Py_NewRef(ok ? Py_True : Py_False);
}
#define PyBool_FromLong(ok) _PyBool_FromLong(ok)

#ifdef __cplusplus
}
#endif
#endif /* !Py_BOOLOBJECT_H */
