/* String Literals: _Py_Identifier API */

#ifndef Py_INTERNAL_IDENTIFIER_H
#define Py_INTERNAL_IDENTIFIER_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

/* This structure helps managing static strings. The basic usage goes like this:
   Instead of doing

       r = PyObject_CallMethod(o, "foo", "args", ...);

   do

       _Py_IDENTIFIER(foo);
       ...
       r = _PyObject_CallMethodId(o, &PyId_foo, "args", ...);

   PyId_foo is a static variable, either on block level or file level. On first
   usage, the string "foo" is interned, and the structures are linked. On interpreter
   shutdown, all strings are released.

   Alternatively, _Py_static_string allows choosing the variable name.
   _PyUnicode_FromId returns a borrowed reference to the interned string.
   _PyObject_{Get,Set,Has}AttrId are __getattr__ versions using _Py_Identifier*.
*/
typedef struct _Py_Identifier {
    const char* string;
    // Index in PyInterpreterState.unicode.ids.array. It is process-wide
    // unique and must be initialized to -1.
    Py_ssize_t index;
} _Py_Identifier;

// For now we are keeping _Py_IDENTIFIER for continued use
// in non-builtin extensions (and naughty PyPI modules).

#define _Py_static_string_init(value) { .string = (value), .index = -1 }
#define _Py_static_string(varname, value)  static _Py_Identifier varname = _Py_static_string_init(value)
#define _Py_IDENTIFIER(varname) _Py_static_string(PyId_##varname, #varname)

extern PyObject* _PyType_LookupId(PyTypeObject *, _Py_Identifier *);
extern PyObject* _PyObject_LookupSpecialId(PyObject *, _Py_Identifier *);
extern PyObject* _PyObject_GetAttrId(PyObject *, _Py_Identifier *);
extern int _PyObject_SetAttrId(PyObject *, _Py_Identifier *, PyObject *);

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_IDENTIFIER_H
