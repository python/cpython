/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()

static Py_hash_t
weakref_hash_impl(PyWeakReference *self);

static Py_hash_t
weakref_hash(PyObject *self)
{
    Py_hash_t return_value = -1;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = weakref_hash_impl((PyWeakReference *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}
/*[clinic end generated code: output=40453fff9335a3f7 input=a9049054013a1b77]*/
