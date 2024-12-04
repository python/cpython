/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()

#if defined(BaseException___traceback___HAS_DOCSTR)
#  define BaseException___traceback___DOCSTR BaseException___traceback____doc__
#else
#  define BaseException___traceback___DOCSTR NULL
#endif
#if defined(BASEEXCEPTION___TRACEBACK___GETSETDEF)
#  undef BASEEXCEPTION___TRACEBACK___GETSETDEF
#  define BASEEXCEPTION___TRACEBACK___GETSETDEF {"__traceback__", (getter)BaseException___traceback___get, (setter)BaseException___traceback___set, BaseException___traceback___DOCSTR},
#else
#  define BASEEXCEPTION___TRACEBACK___GETSETDEF {"__traceback__", (getter)BaseException___traceback___get, NULL, BaseException___traceback___DOCSTR},
#endif

static PyObject *
BaseException___traceback___get_impl(PyBaseExceptionObject *self);

static PyObject *
BaseException___traceback___get(PyBaseExceptionObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException___traceback___get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if defined(BASEEXCEPTION___TRACEBACK___HAS_DOCSTR)
#  define BaseException___traceback___DOCSTR BaseException___traceback____doc__
#else
#  define BaseException___traceback___DOCSTR NULL
#endif
#if defined(BASEEXCEPTION___TRACEBACK___GETSETDEF)
#  undef BASEEXCEPTION___TRACEBACK___GETSETDEF
#  define BASEEXCEPTION___TRACEBACK___GETSETDEF {"__traceback__", (getter)BaseException___traceback___get, (setter)BaseException___traceback___set, BaseException___traceback___DOCSTR},
#else
#  define BASEEXCEPTION___TRACEBACK___GETSETDEF {"__traceback__", NULL, (setter)BaseException___traceback___set, NULL},
#endif

static int
BaseException___traceback___set_impl(PyBaseExceptionObject *self,
                                     PyObject *value);

static int
BaseException___traceback___set(PyBaseExceptionObject *self, PyObject *value, void *Py_UNUSED(context))
{
    int return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException___traceback___set_impl(self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}
/*[clinic end generated code: output=4821907e4f70fde6 input=a9049054013a1b77]*/
