#ifndef Py_PYINTERFACE_H
#define Py_PYINTERFACE_H
#ifdef __cplusplus
extern "C" {
#endif

/* 
The PyInterface_Base struct is the generic type for actual interface data
implementations. The intent is for callers to preallocate the specific struct
and have the PyObject_GetInterface() function fill it in.

An example of direct use of the interface API (definitions and example without
macro helpers below):

    Py_INTERFACE_VAR(PyInterface_GetAttrWChar, attr_data);
    if (PyObject_GetInterface(obj, PyInterface_GetAttrWChar_Name, &attr_data) == 0) {
        result = Py_INTERFACE_CALL(attr_data, getattr, L"attribute_name");
        PyInterface_Release(&attr_data);
    } else {
        char attr_name[128];
        wchar_to_char(attr_name, L"attribute_name"); // hypothetical converter
        result = PyObject_GetAttr(obj, attr_name);
    }

Here are the key points that add value:
 * the PyInterface_GetAttrWChar_Name string literal is embedded in the calling
   module, making the value available when running against earlier releases of
   Python (unlike a function export, which would cause a load failure).
 * if the name is not available, the PyObject_GetInterface call can fail safely.
   Thus, new APIs can be added in later releases, and newer compiled modules can
   be fully binary compatible with older releases.
 * The attr_data value contains all the information required to eventually
   produce the result. It may provide fields for direct access as well as
   function pointers that can calculate/copy/return results as needed.
 * The interface is resolved by the object's type, but does not have to provide
   identical result for every instance. It has independent lifetime from object,
   (though this will often just be a new strong reference to the object).
 * Static/header libraries can be used to wrap up the faster APIs, so that
   extensions can adopt them simply by compiling with the latest release.
   An example is shown at the end of this file.


Without the Py_INTERFACE_* helper macros, it would look like the below. This is
primarily for the benefit of non-C developers trying to use the API without
macros.

    PyInterface_GetAttrWChar attr_data = { sizeof(PyInterface_GetAttrWChar) };
    if (PyObject_GetInterface(obj, PyInterface_GetAttrWChar_Name, &attr_data) == 0) {
        result = (*attr_data.getattr)(&attr_data, L"attribute_name");
        PyInterface_Release(&attr_data);
    } ...
*/

#define Py_INTERFACE_VAR(t, n) t n = { sizeof(t) }
#define Py_INTERFACE_CALL(data, attr, ...) ((data).attr)(&(data), __VA_ARGS__)

typedef struct _interfacedata {
    Py_ssize_t size;
    /* intf is 'struct _interfacedata' but expects the specific struct */
    int (*release)(void *intf);
} PyInterface_Base;


typedef int (*getinterfacefunc)(PyObject *o, const char *intf_name, void *intf);

/* Functions to get interfaces are defined on PyObject and ... TODO */
int PyInterface_Release(void *intf);


/* Some generic/example interface definitions.

The first (PyInterface_GetAttrWChar) shows a hypothetical new API that may be added
in a later release. Rather than modifying the ABI (making newer extensions
incompatible with earlier releases), it is added as a interface. Runtimes
without the interface will return a runtime error, and the caller uses a
fallback (the example above).
*/

#define PyInterface_GetAttrWChar_Name "getattr-wchar"

typedef struct _getattrwcharinterface {
    PyInterface_Base base;
    PyObject *(*getattr)(struct _getattrwcharinterface *, const wchar_t *attr);
    int (*hasattr)(struct _getattrwcharinterface *, const wchar_t *attr);
    // Strong reference to original object, but for private use only.
    PyObject *_obj;
} PyInterface_GetAttrWChar;


/* This example (PyInterface_AsUTF8) shows an optionally-optimised API, where in some
cases the result of the API is readily available, and can be returned to
pre-allocated memory (in this case, the interface data in the stack of the caller).
The caller can then either use a fast path to access it directly, or the more
generic function (pointer) in the struct that will *always* produce a result,
but may take more computation if the fast result was not present.

    Py_INTERFACE_VAR(PyInterface_AsUTF8, intf);
    if (PyObject_GetInterface(o, PyInterface_AsUTF8, &intf) == 0) {
        // This check is optional, as the function calls in the interface should
        // always succeed. However, developers who want to avoid an additional
        // function call/allocation can do the check themselves.
        // This check is defined per-interface struct, so users reference the
        // docs for the specific interfaces they're using to find the check.
        if (intf->s[0]) {
            // use intf->s as the result.
            call_next_api(intf->s);
        } else {
            char *s = Py_INTERFACE_CALL(intf, stralloc);
            call_next_api(s);
            PyMem_Free(s);
        }
        PyInterface_Release(&intf);
    } else { ... }

*/

#define PyInterface_AsUTF8_Name "as-utf8"

typedef struct _asutf8interface {
    PyInterface_Base base;
    // Copy of contents if it was readily available. s[0] will be non-zero if set
    char s[16];
    // Copy the characters into an existing string
    size_t (*strcpy)(struct _asutf8interface *, char *dest, size_t dest_size);
    // Allocate new buffer with PyMem_Malloc (use PyMem_Free to release)
    char * (*stralloc)(struct _asutf8interface *);
    // Optional strong reference to object, if unable to use char array
    PyObject *_obj;
} PyInterface_AsUTF8;

/* Example implementation of PyInterface_AsUTF8.strcpy:

size_t _PyUnicode_AsUTF8Interface_strcpy(PyInterface_AsUTF8 *intf, char *dest, size_t dest_size)
{
    if (intf->_obj) {
        // copy/convert data from _obj...
        _internal_copy((PyUnicodeObject *)intf->_obj, dest, dest_size);
        return chars_copied;
    } else {
        return strcpy_s(dest, dest_size, intf->_reserved);
    }
}

*/

/* API wrappers.

These are inline functions (or in a static import library) to embed all the
implementation into the external module. We can change the implementation over
releases, and by using interfaces to add optional handling, the behaviour
remains compatible with earlier releases, and the sources remain compatible.

Note that additions will usually be at the top of the function, assuming that
newer interfaces are preferred over older ones.

static inline PyObject *PyObject_GetAttrWChar(PyObject *o, wchar_t *attr)
{
    PyObject *result = NULL;

    // Added in version N+1
    Py_INTERFACE_VAR(PyInterface_GetAttrWChar, intf);
    if (PyObject_GetInterface(o, PyInterface_GetAttrWChar_Name, &intf) == 0) {
        result = Py_INTERFACE_CALL(intf, getattr, attr);
        if (PyInterface_Release(&intf) < 0) {
            Py_DecRef(result);
            return NULL;
        }
        return result;
    }
    PyErr_Clear();

    // Original implementation in version N
    PyObject *attro = PyUnicode_FromWideChar(attr, -1);
    if (attro) {
        result = PyObject_GetAttr(o, attro);
        Py_DecRef(attro);
    }
    return result;
}


This may be defined in its own header or statically linked library, provided the
implementation ends up in the external module, not in the Python library.
*/


#ifdef __cplusplus
}
#endif
#endif
