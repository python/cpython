#ifndef Py_CPYTHON_ABSTRACTOBJECT_H
#  error "this header file must not be included directly"
#endif

/* === Object Protocol ================================================== */

/* Like PyObject_CallMethod(), but expect a _Py_Identifier*
   as the method name. */
PyAPI_FUNC(PyObject*) _PyObject_CallMethodId(
    PyObject *obj,
    _Py_Identifier *name,
    const char *format, ...);

/* Convert keyword arguments from the FASTCALL (stack: C array, kwnames: tuple)
   format to a Python dictionary ("kwargs" dict).

   The type of kwnames keys is not checked. The final function getting
   arguments is responsible to check if all keys are strings, for example using
   PyArg_ParseTupleAndKeywords() or PyArg_ValidateKeywordArguments().

   Duplicate keys are merged using the last value. If duplicate keys must raise
   an exception, the caller is responsible to implement an explicit keys on
   kwnames. */
PyAPI_FUNC(PyObject*) _PyStack_AsDict(PyObject *const *values, PyObject *kwnames);


/* === Vectorcall protocol (PEP 590) ============================= */

// PyVectorcall_NARGS() is exported as a function for the stable ABI.
// Here (when we are not using the stable ABI), the name is overridden to
// call a static inline function for best performance.
static inline Py_ssize_t
_PyVectorcall_NARGS(size_t n)
{
    return n & ~PY_VECTORCALL_ARGUMENTS_OFFSET;
}
#define PyVectorcall_NARGS(n) _PyVectorcall_NARGS(n)

PyAPI_FUNC(vectorcallfunc) PyVectorcall_Function(PyObject *callable);

// Backwards compatibility aliases (PEP 590) for API that was provisional
// in Python 3.8
#define _PyObject_Vectorcall PyObject_Vectorcall
#define _PyObject_VectorcallMethod PyObject_VectorcallMethod
#define _PyObject_FastCallDict PyObject_VectorcallDict
#define _PyVectorcall_Function PyVectorcall_Function
#define _PyObject_CallOneArg PyObject_CallOneArg
#define _PyObject_CallMethodNoArgs PyObject_CallMethodNoArgs
#define _PyObject_CallMethodOneArg PyObject_CallMethodOneArg

/* Same as PyObject_Vectorcall except that keyword arguments are passed as
   dict, which may be NULL if there are no keyword arguments. */
PyAPI_FUNC(PyObject *) PyObject_VectorcallDict(
    PyObject *callable,
    PyObject *const *args,
    size_t nargsf,
    PyObject *kwargs);

PyAPI_FUNC(PyObject *) PyObject_CallOneArg(PyObject *func, PyObject *arg);

static inline PyObject *
PyObject_CallMethodNoArgs(PyObject *self, PyObject *name)
{
    size_t nargsf = 1 | PY_VECTORCALL_ARGUMENTS_OFFSET;
    return PyObject_VectorcallMethod(name, &self, nargsf, _Py_NULL);
}

static inline PyObject *
PyObject_CallMethodOneArg(PyObject *self, PyObject *name, PyObject *arg)
{
    PyObject *args[2] = {self, arg};
    size_t nargsf = 2 | PY_VECTORCALL_ARGUMENTS_OFFSET;
    assert(arg != NULL);
    return PyObject_VectorcallMethod(name, args, nargsf, _Py_NULL);
}

/* Guess the size of object 'o' using len(o) or o.__length_hint__().
   If neither of those return a non-negative value, then return the default
   value.  If one of the calls fails, this function returns -1. */
PyAPI_FUNC(Py_ssize_t) PyObject_LengthHint(PyObject *o, Py_ssize_t);

/* === Sequence protocol ================================================ */

/* Assume tp_as_sequence and sq_item exist and that 'i' does not
   need to be corrected for a negative index. */
#define PySequence_ITEM(o, i)\
    ( Py_TYPE(o)->tp_as_sequence->sq_item((o), (i)) )
