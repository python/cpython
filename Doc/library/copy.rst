
:mod:`copy` --- Shallow and deep copy operations
================================================

.. module:: copy
   :synopsis: Shallow and deep copy operations.


.. index::
   single: copy() (in copy)
   single: deepcopy() (in copy)

This module provides generic (shallow and deep) copying operations.

Interface summary::

   import copy

   x = copy.copy(y)        # make a shallow copy of y
   x = copy.deepcopy(y)    # make a deep copy of y

For module specific errors, :exc:`copy.error` is raised.

.. % 

The difference between shallow and deep copying is only relevant for compound
objects (objects that contain other objects, like lists or class instances):

* A *shallow copy* constructs a new compound object and then (to the extent
  possible) inserts *references* into it to the objects found in the original.

* A *deep copy* constructs a new compound object and then, recursively, inserts
  *copies* into it of the objects found in the original.

Two problems often exist with deep copy operations that don't exist with shallow
copy operations:

* Recursive objects (compound objects that, directly or indirectly, contain a
  reference to themselves) may cause a recursive loop.

* Because deep copy copies *everything* it may copy too much, e.g.,
  administrative data structures that should be shared even between copies.

The :func:`deepcopy` function avoids these problems by:

* keeping a "memo" dictionary of objects already copied during the current
  copying pass; and

* letting user-defined classes override the copying operation or the set of
  components copied.

This module does not copy types like module, method, stack trace, stack frame,
file, socket, window, array, or any similar types.  It does "copy" functions and
classes (shallow and deeply), by returning the original object unchanged; this
is compatible with the way these are treated by the :mod:`pickle` module.

Shallow copies of dictionaries can be made using :meth:`dict.copy`, and
of lists by assigning a slice of the entire list, for example,
``copied_list = original_list[:]``.

.. versionchanged:: 2.5
   Added copying functions.

.. index:: module: pickle

Classes can use the same interfaces to control copying that they use to control
pickling.  See the description of module :mod:`pickle` for information on these
methods.  The :mod:`copy` module does not use the :mod:`copy_reg` registration
module.

.. index::
   single: __copy__() (copy protocol)
   single: __deepcopy__() (copy protocol)

In order for a class to define its own copy implementation, it can define
special methods :meth:`__copy__` and :meth:`__deepcopy__`.  The former is called
to implement the shallow copy operation; no additional arguments are passed.
The latter is called to implement the deep copy operation; it is passed one
argument, the memo dictionary.  If the :meth:`__deepcopy__` implementation needs
to make a deep copy of a component, it should call the :func:`deepcopy` function
with the component as first argument and the memo dictionary as second argument.


.. seealso::

   Module :mod:`pickle`
      Discussion of the special methods used to support object state retrieval and
      restoration.

