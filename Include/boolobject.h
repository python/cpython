/* Boolean object interface */

#ifndef Py_BOOLOBJECT_H
#define Py_BOOLOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif


// PyBool_Type is declared by object.h

#define PyBool_Check(x) Py_IS_TYPE((x), &PyBool_Type)

/* Py_False and Py_True are the only two bools in existence. */

/* Don't use these directly */
PyAPI_DATA(PyLongObject) _Py_FalseStruct;
PyAPI_DATA(PyLongObject) _Py_TrueStruct;

/* Use these macros */
#if defined(Py_LIMITED_API) && Py_LIMITED_API+0 >= 0x030D0000
#  define Py_False Py_GetConstantBorrowed(Py_CONSTANT_FALSE)
#  define Py_True Py_GetConstantBorrowed(Py_CONSTANT_TRUE)
#else
#  define Py_False _PyObject_CAST(&_Py_FalseStruct)
#  define Py_True _PyObject_CAST(&_Py_TrueStruct)
#endif

// Test if an object is the True singleton, the same as "x is True" in Python.
PyAPI_FUNC(int) Py_IsTrue(PyObject *x);
#define Py_IsTrue(x) Py_Is((x), Py_True)

// Test if an object is the False singleton, the same as "x is False" in Python.
PyAPI_FUNC(int) Py_IsFalse(PyObject *x);
#define Py_IsFalse(x) Py_Is((x), Py_False)

/* Macros for returning Py_True or Py_False, respectively.
 * Only treat Py_True and Py_False as immortal in the limited C API 3.12
 * and newer. */
#if defined(Py_LIMITED_API) && Py_LIMITED_API+0 < 0x030c0000
#  define Py_RETURN_TRUE return Py_NewRef(Py_True)
#  define Py_RETURN_FALSE return Py_NewRef(Py_False)
#else
#  define Py_RETURN_TRUE return Py_True
#  define Py_RETURN_FALSE return Py_False
#endif

/* Function to return a bool from a C long */
PyAPI_FUNC(PyObject *) PyBool_FromLong(long);

#ifdef __cplusplus
}
#endif
#endif /* !Py_BOOLOBJECT_H */
