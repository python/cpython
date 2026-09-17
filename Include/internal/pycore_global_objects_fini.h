#ifndef Py_INTERNAL_GLOBAL_OBJECTS_FINI_H
#define Py_INTERNAL_GLOBAL_OBJECTS_FINI_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#ifdef Py_DEBUG

#include "pycore_bytesobject.h"   // _PyBytes_CheckOverflow()
#include "pycore_long.h"          // TAG_FROM_SIGN_AND_SIZE()

static inline void
_PyStaticObject_CheckSingleton(PyObject *obj, PyTypeObject *type)
{
    // Check PyObject.ob_refcnt
    _PyObject_ASSERT(obj, _Py_IsImmortal(obj));

    // Check PyObject.ob_type
    _PyObject_ASSERT(obj, Py_TYPE(obj) == type);
}


static void
_PyStaticObject_CheckLongSingleton(PyObject *obj, long value, int is_bool)
{
    PyTypeObject *type = is_bool ? &PyBool_Type : &PyLong_Type;
    _PyStaticObject_CheckSingleton(obj, type);

    // Check _PyLong_CompactValue()
    Py_ssize_t compact = _PyLong_CompactValue((const PyLongObject *)obj);
    _PyObject_ASSERT(obj, compact == value);

    // Check tv_tag and ob_digit[0]
    _PyLongValue *long_value = &((PyLongObject*)obj)->long_value;
    int sign = (value == 0) ? 0 : ((value < 0) ? -1 : 1);
    uintptr_t lv_tag = TAG_FROM_SIGN_AND_SIZE(sign, (value == 0) ? 0 : 1);
    if (!is_bool) {
        lv_tag |= IMMORTALITY_BIT_MASK;
    }
    _PyObject_ASSERT(obj, long_value->lv_tag == lv_tag);
    _PyObject_ASSERT(obj, long_value->ob_digit[0] == Py_ABS(value));
}


static inline void
_PyStaticObject_CheckBytesSingleton(PyObject *obj,
                                    Py_ssize_t size, unsigned char ch)
{
    _PyStaticObject_CheckSingleton(obj, &PyBytes_Type);
    _PyObject_ASSERT(obj, PyBytes_GET_SIZE(obj) == size);
    const unsigned char *str = (const unsigned char *)PyBytes_AS_STRING(obj);
    _PyObject_ASSERT(obj, str[0] == ch);
    _PyBytes_CheckOverflow(obj, obj, "bytes singleton");
}

static void
_PyStaticObject_CheckUnicode(PyObject *obj, const char *str, Py_ssize_t length)
{
    _PyStaticObject_CheckSingleton(obj, &PyUnicode_Type);
    _PyObject_ASSERT(obj, _PyUnicode_CheckConsistency(obj, 1));
    _PyObject_ASSERT(obj, PyUnicode_GET_LENGTH(obj) == length);
    _PyObject_ASSERT(obj, PyUnicode_KIND(obj) == PyUnicode_1BYTE_KIND);
    const Py_UCS1 *data = PyUnicode_1BYTE_DATA(obj);
    _PyObject_ASSERT(obj, memcmp(data, str, length) == 0);
    _PyObject_ASSERT(obj, data[length] == 0);
}


static void
_PyStaticObject_CheckUnicodeCharSingleton(PyObject *obj, unsigned char ch)
{
    _PyStaticObject_CheckUnicode(obj, (char *)&ch, 1);
    _PyObject_ASSERT(obj, PyUnicode_IS_ASCII(obj) == (ch <= 127));
}


static void
_PyStaticObject_CheckUnicodeSingleton(PyObject *obj,
                                      const char *str, Py_ssize_t length)
{
    _PyStaticObject_CheckUnicode(obj, str, length);
    _PyObject_ASSERT(obj, PyUnicode_IS_ASCII(obj));
}

#endif  // Py_DEBUG

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GLOBAL_OBJECTS_FINI_H */
