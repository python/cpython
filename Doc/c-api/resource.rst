.. highlight:: c

PyResource
==========

.. versionadded:: 3.13

API using a callback function to close a resource.


.. c:type:: PyResource

   .. c:member:: void (*close_func) (void *data)

      Callback function called by :c:func:`PyResource_Close`.

   .. c:member:: void *data

      Argument passed to :c:member:`close_func` by :c:func:`PyResource_Close`.


.. c:function:: void PyResource_Close(PyResource *res)

   Close a resource: call :c:member:`PyResource.close_func` with
   :c:member:`PyResource.data`.
