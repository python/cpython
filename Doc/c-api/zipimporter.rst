.. highlightlang:: c

.. zipimporter:

ZipImporter Objects
-------------------

Python for the longest of time had an zipimporter that could not be
subclassed using the C Python API but could in the Python Layer.
Since Python 3.7 users can now subclass it in their C code as now
it is an exported Type in the Python Core.

.. c:var:: PyTypeObject PyZipImporter_Type

   This instance of :c:type:`PyTypeObject` represents the Python zipimporter type;
   it is the same object as :class:`zipimport.zipimporter` in the Python layer.

   .. note::

      This type used to be named ZipImporter_Type and not exported. Since then it
      caused problems with subtyping it in another type forcing them to copy most
      of the implementation from zipimport.c to their extension module which can
      not be a reasonable thing. So this change was needed.


Type check macros
^^^^^^^^^^^^^^^^^

.. c:function:: int PyZipImporter_Check(PyObject *o)

   Return true if the object *o* is a zipimporter type or an instance of a
   subtype of the zipimporter type.
