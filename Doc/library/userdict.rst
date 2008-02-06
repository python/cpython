

:mod:`UserList` --- Class wrapper for list objects
==================================================

.. module:: UserList
   :synopsis: Class wrapper for list objects.


.. note::

   This module is available for backward compatibility only.  If you are writing
   code that does not need to work with versions of Python earlier than Python 2.2,
   please consider subclassing directly from the built-in :class:`list` type.

This module defines a class that acts as a wrapper around list objects.  It is a
useful base class for your own list-like classes, which can inherit from them
and override existing methods or add new ones.  In this way one can add new
behaviors to lists.

The :mod:`UserList` module defines the :class:`UserList` class:


.. class:: UserList([list])

   Class that simulates a list.  The instance's contents are kept in a regular
   list, which is accessible via the :attr:`data` attribute of
   :class:`UserList`
   instances.  The instance's contents are initially set to a copy of *list*,
   defaulting to the empty list ``[]``.  *list* can be any iterable, for
   example a real Python list or a :class:`UserList` object.

In addition to supporting the methods and operations of mutable sequences (see
section :ref:`typesseq`), :class:`UserList` instances provide the following
attribute:


.. attribute:: UserList.data

   A real Python list object used to store the contents of the :class:`UserList`
   class.

**Subclassing requirements:** Subclasses of :class:`UserList` are expect to
offer a constructor which can be called with either no arguments or one
argument.  List operations which return a new sequence attempt to create an
instance of the actual implementation class.  To do so, it assumes that the
constructor can be called with a single parameter, which is a sequence object
used as a data source.

If a derived class does not wish to comply with this requirement, all of the
special methods supported by this class will need to be overridden; please
consult the sources for information about the methods which need to be provided
in that case.


:mod:`UserString` --- Class wrapper for string objects
======================================================

.. module:: UserString
   :synopsis: Class wrapper for string objects.
.. moduleauthor:: Peter Funk <pf@artcom-gmbh.de>
.. sectionauthor:: Peter Funk <pf@artcom-gmbh.de>


.. note::

   This :class:`UserString` class from this module is available for backward
   compatibility only.  If you are writing code that does not need to work with
   versions of Python earlier than Python 2.2, please consider subclassing directly
   from the built-in :class:`str` type instead of using :class:`UserString` (there
   is no built-in equivalent to :class:`MutableString`).

This module defines a class that acts as a wrapper around string objects.  It is
a useful base class for your own string-like classes, which can inherit from
them and override existing methods or add new ones.  In this way one can add new
behaviors to strings.

It should be noted that these classes are highly inefficient compared to real
string or bytes objects; this is especially the case for
:class:`MutableString`.

The :mod:`UserString` module defines the following classes:


.. class:: UserString([sequence])

   Class that simulates a string or a Unicode string object.  The instance's
   content is kept in a regular string or Unicode string object, which is
   accessible via the :attr:`data` attribute of :class:`UserString` instances.  The
   instance's contents are initially set to a copy of *sequence*.  *sequence* can
   be an instance of :class:`bytes`, :class:`str`, :class:`UserString` (or a
   subclass) or an arbitrary sequence which can be converted into a string using
   the built-in :func:`str` function.


.. class:: MutableString([sequence])

   This class is derived from the :class:`UserString` above and redefines strings
   to be *mutable*.  Mutable strings can't be used as dictionary keys, because
   dictionaries require *immutable* objects as keys.  The main intention of this
   class is to serve as an educational example for inheritance and necessity to
   remove (override) the :meth:`__hash__` method in order to trap attempts to use a
   mutable object as dictionary key, which would be otherwise very error prone and
   hard to track down.

In addition to supporting the methods and operations of bytes and string
objects (see section :ref:`string-methods`), :class:`UserString` instances
provide the following attribute:


.. attribute:: MutableString.data

   A real Python string or bytes object used to store the content of the
   :class:`UserString` class.

