
:mod:`weakref` --- Weak references
==================================

.. module:: weakref
   :synopsis: Support for weak references and weak dictionaries.
.. moduleauthor:: Fred L. Drake, Jr. <fdrake@acm.org>
.. moduleauthor:: Neil Schemenauer <nas@arctrix.com>
.. moduleauthor:: Martin von Löwis <martin@loewis.home.cs.tu-berlin.de>
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>


.. versionadded:: 2.1

The :mod:`weakref` module allows the Python programmer to create :dfn:`weak
references` to objects.

.. When making changes to the examples in this file, be sure to update
   Lib/test/test_weakref.py::libreftest too!

In the following, the term :dfn:`referent` means the object which is referred to
by a weak reference.

A weak reference to an object is not enough to keep the object alive: when the
only remaining references to a referent are weak references,
:term:`garbage collection` is free to destroy the referent and reuse its memory
for something else.  A primary use for weak references is to implement caches or
mappings holding large objects, where it's desired that a large object not be
kept alive solely because it appears in a cache or mapping.  For example, if you
have a number of large binary image objects, you may wish to associate a name
with each.  If you used a Python dictionary to map names to images, or images to
names, the image objects would remain alive just because they appeared as values
or keys in the dictionaries.  The :class:`WeakKeyDictionary` and
:class:`WeakValueDictionary` classes supplied by the :mod:`weakref` module are
an alternative, using weak references to construct mappings that don't keep
objects alive solely because they appear in the mapping objects.  If, for
example, an image object is a value in a :class:`WeakValueDictionary`, then when
the last remaining references to that image object are the weak references held
by weak mappings, garbage collection can reclaim the object, and its
corresponding entries in weak mappings are simply deleted.

:class:`WeakKeyDictionary` and :class:`WeakValueDictionary` use weak references
in their implementation, setting up callback functions on the weak references
that notify the weak dictionaries when a key or value has been reclaimed by
garbage collection.  Most programs should find that using one of these weak
dictionary types is all they need -- it's not usually necessary to create your
own weak references directly.  The low-level machinery used by the weak
dictionary implementations is exposed by the :mod:`weakref` module for the
benefit of advanced uses.

Not all objects can be weakly referenced; those objects which can include class
instances, functions written in Python (but not in C), methods (both bound and
unbound), sets, frozensets, file objects, :term:`generator`\s, type objects,
:class:`DBcursor` objects from the :mod:`bsddb` module, sockets, arrays, deques,
and regular expression pattern objects.

.. versionchanged:: 2.4
   Added support for files, sockets, arrays, and patterns.

Several builtin types such as :class:`list` and :class:`dict` do not directly
support weak references but can add support through subclassing::

   class Dict(dict):
       pass

   obj = Dict(red=1, green=2, blue=3)   # this object is weak referenceable

Extension types can easily be made to support weak references; see
:ref:`weakref-support`.


.. class:: ref(object[, callback])

   Return a weak reference to *object*.  The original object can be retrieved by
   calling the reference object if the referent is still alive; if the referent is
   no longer alive, calling the reference object will cause :const:`None` to be
   returned.  If *callback* is provided and not :const:`None`, and the returned
   weakref object is still alive, the callback will be called when the object is
   about to be finalized; the weak reference object will be passed as the only
   parameter to the callback; the referent will no longer be available.

   It is allowable for many weak references to be constructed for the same object.
   Callbacks registered for each weak reference will be called from the most
   recently registered callback to the oldest registered callback.

   Exceptions raised by the callback will be noted on the standard error output,
   but cannot be propagated; they are handled in exactly the same way as exceptions
   raised from an object's :meth:`__del__` method.

   Weak references are :term:`hashable` if the *object* is hashable.  They will maintain
   their hash value even after the *object* was deleted.  If :func:`hash` is called
   the first time only after the *object* was deleted, the call will raise
   :exc:`TypeError`.

   Weak references support tests for equality, but not ordering.  If the referents
   are still alive, two references have the same equality relationship as their
   referents (regardless of the *callback*).  If either referent has been deleted,
   the references are equal only if the reference objects are the same object.

   .. versionchanged:: 2.4
      This is now a subclassable type rather than a factory function; it derives from
      :class:`object`.


.. function:: proxy(object[, callback])

   Return a proxy to *object* which uses a weak reference.  This supports use of
   the proxy in most contexts instead of requiring the explicit dereferencing used
   with weak reference objects.  The returned object will have a type of either
   ``ProxyType`` or ``CallableProxyType``, depending on whether *object* is
   callable.  Proxy objects are not :term:`hashable` regardless of the referent; this
   avoids a number of problems related to their fundamentally mutable nature, and
   prevent their use as dictionary keys.  *callback* is the same as the parameter
   of the same name to the :func:`ref` function.


.. function:: getweakrefcount(object)

   Return the number of weak references and proxies which refer to *object*.


.. function:: getweakrefs(object)

   Return a list of all weak reference and proxy objects which refer to *object*.


.. class:: WeakKeyDictionary([dict])

   Mapping class that references keys weakly.  Entries in the dictionary will be
   discarded when there is no longer a strong reference to the key.  This can be
   used to associate additional data with an object owned by other parts of an
   application without adding attributes to those objects.  This can be especially
   useful with objects that override attribute accesses.

   .. note::

      Caution:  Because a :class:`WeakKeyDictionary` is built on top of a Python
      dictionary, it must not change size when iterating over it.  This can be
      difficult to ensure for a :class:`WeakKeyDictionary` because actions performed
      by the program during iteration may cause items in the dictionary to vanish "by
      magic" (as a side effect of garbage collection).

:class:`WeakKeyDictionary` objects have the following additional methods.  These
expose the internal references directly.  The references are not guaranteed to
be "live" at the time they are used, so the result of calling the references
needs to be checked before being used.  This can be used to avoid creating
references that will cause the garbage collector to keep the keys around longer
than needed.


.. method:: WeakKeyDictionary.iterkeyrefs()

   Return an :term:`iterator` that yields the weak references to the keys.

   .. versionadded:: 2.5


.. method:: WeakKeyDictionary.keyrefs()

   Return a list of weak references to the keys.

   .. versionadded:: 2.5


.. class:: WeakValueDictionary([dict])

   Mapping class that references values weakly.  Entries in the dictionary will be
   discarded when no strong reference to the value exists any more.

   .. note::

      Caution:  Because a :class:`WeakValueDictionary` is built on top of a Python
      dictionary, it must not change size when iterating over it.  This can be
      difficult to ensure for a :class:`WeakValueDictionary` because actions performed
      by the program during iteration may cause items in the dictionary to vanish "by
      magic" (as a side effect of garbage collection).

:class:`WeakValueDictionary` objects have the following additional methods.
These method have the same issues as the :meth:`iterkeyrefs` and :meth:`keyrefs`
methods of :class:`WeakKeyDictionary` objects.


.. method:: WeakValueDictionary.itervaluerefs()

   Return an :term:`iterator` that yields the weak references to the values.

   .. versionadded:: 2.5


.. method:: WeakValueDictionary.valuerefs()

   Return a list of weak references to the values.

   .. versionadded:: 2.5


.. data:: ReferenceType

   The type object for weak references objects.


.. data:: ProxyType

   The type object for proxies of objects which are not callable.


.. data:: CallableProxyType

   The type object for proxies of callable objects.


.. data:: ProxyTypes

   Sequence containing all the type objects for proxies.  This can make it simpler
   to test if an object is a proxy without being dependent on naming both proxy
   types.


.. exception:: ReferenceError

   Exception raised when a proxy object is used but the underlying object has been
   collected.  This is the same as the standard :exc:`ReferenceError` exception.


.. seealso::

   :pep:`0205` - Weak References
      The proposal and rationale for this feature, including links to earlier
      implementations and information about similar features in other languages.


.. _weakref-objects:

Weak Reference Objects
----------------------

Weak reference objects have no attributes or methods, but do allow the referent
to be obtained, if it still exists, by calling it::

   >>> import weakref
   >>> class Object:
   ...     pass
   ...
   >>> o = Object()
   >>> r = weakref.ref(o)
   >>> o2 = r()
   >>> o is o2
   True

If the referent no longer exists, calling the reference object returns
:const:`None`::

   >>> del o, o2
   >>> print r()
   None

Testing that a weak reference object is still live should be done using the
expression ``ref() is not None``.  Normally, application code that needs to use
a reference object should follow this pattern::

   # r is a weak reference object
   o = r()
   if o is None:
       # referent has been garbage collected
       print "Object has been deallocated; can't frobnicate."
   else:
       print "Object is still live!"
       o.do_something_useful()

Using a separate test for "liveness" creates race conditions in threaded
applications; another thread can cause a weak reference to become invalidated
before the weak reference is called; the idiom shown above is safe in threaded
applications as well as single-threaded applications.

Specialized versions of :class:`ref` objects can be created through subclassing.
This is used in the implementation of the :class:`WeakValueDictionary` to reduce
the memory overhead for each entry in the mapping.  This may be most useful to
associate additional information with a reference, but could also be used to
insert additional processing on calls to retrieve the referent.

This example shows how a subclass of :class:`ref` can be used to store
additional information about an object and affect the value that's returned when
the referent is accessed::

   import weakref

   class ExtendedRef(weakref.ref):
       def __init__(self, ob, callback=None, **annotations):
           super(ExtendedRef, self).__init__(ob, callback)
           self.__counter = 0
           for k, v in annotations.iteritems():
               setattr(self, k, v)

       def __call__(self):
           """Return a pair containing the referent and the number of
           times the reference has been called.
           """
           ob = super(ExtendedRef, self).__call__()
           if ob is not None:
               self.__counter += 1
               ob = (ob, self.__counter)
           return ob


.. _weakref-example:

Example
-------

This simple example shows how an application can use objects IDs to retrieve
objects that it has seen before.  The IDs of the objects can then be used in
other data structures without forcing the objects to remain alive, but the
objects can still be retrieved by ID if they do.

.. Example contributed by Tim Peters.

::

   import weakref

   _id2obj_dict = weakref.WeakValueDictionary()

   def remember(obj):
       oid = id(obj)
       _id2obj_dict[oid] = obj
       return oid

   def id2obj(oid):
       return _id2obj_dict[oid]

