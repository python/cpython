.. highlight:: c

SimpleNamespace Objects
-----------------------

.. c:function:: PyObject* PySimpleNamespace_New(PyObject *attrs)

   Create a new simple namespace object.

   *attrs* must be a mapping object with non-empty string keys.
   It can be ``NULL``.

   Return ``NULL`` and raise an exception on failure.

   .. versionadded:: 3.11
