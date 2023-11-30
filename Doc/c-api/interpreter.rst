.. highlight:: c

.. _interpreters:

Interpreters
============

Interpreter ID API
------------------

.. c:var:: PyTypeObject PyInterpreterID_Type

   This instance of :c:type:`PyTypeObject` represents the Python interpreter ID type.

.. c:function:: PyObject* PyInterpreterID_New(int64_t id)

   Returns a new interpreter ID object with the given *id*.

.. c:function:: PyInterpreterState* PyInterpreterID_LookUp(PyObject *requested_id)

   Returns a :term:`borrowed reference` to the :c:type:`PyInterpreterState` interpreter state for the given interpreter ID object *requested_id*.
   Fails with :c:data:`PyExc_RuntimeError` if the interpreter is not found.

.. c:function:: PyObject* PyInterpreterState_GetIDObject(PyInterpreterState *interp)

   Returns a new interpreter ID object for the given interpreter state *interp*.
