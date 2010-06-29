
:mod:`StringIO` --- Read and write strings as files
===================================================

.. module:: StringIO
   :synopsis: Read and write strings as if they were files.


This module implements a file-like class, :class:`StringIO`, that reads and
writes a string buffer (also known as *memory files*).  See the description of
file objects for operations (section :ref:`bltin-file-objects`). (For
standard strings, see :class:`str` and :class:`unicode`.)


.. class:: StringIO([buffer])

   When a :class:`StringIO` object is created, it can be initialized to an existing
   string by passing the string to the constructor. If no string is given, the
   :class:`StringIO` will start empty. In both cases, the initial file position
   starts at zero.

   The :class:`StringIO` object can accept either Unicode or 8-bit strings, but
   mixing the two may take some care.  If both are used, 8-bit strings that cannot
   be interpreted as 7-bit ASCII (that use the 8th bit) will cause a
   :exc:`UnicodeError` to be raised when :meth:`getvalue` is called.

The following methods of :class:`StringIO` objects require special mention:


.. method:: StringIO.getvalue()

   Retrieve the entire contents of the "file" at any time before the
   :class:`StringIO` object's :meth:`close` method is called.  See the note above
   for information about mixing Unicode and 8-bit strings; such mixing can cause
   this method to raise :exc:`UnicodeError`.


.. method:: StringIO.close()

   Free the memory buffer.  Attempting to do further operations with a closed
   :class:`StringIO` object will raise a :exc:`ValueError`.

Example usage::

   import StringIO

   output = StringIO.StringIO()
   output.write('First line.\n')
   print >>output, 'Second line.'

   # Retrieve file contents -- this will be
   # 'First line.\nSecond line.\n'
   contents = output.getvalue()

   # Close object and discard memory buffer --
   # .getvalue() will now raise an exception.
   output.close()


:mod:`cStringIO` --- Faster version of :mod:`StringIO`
======================================================

.. module:: cStringIO
   :synopsis: Faster version of StringIO, but not subclassable.
.. moduleauthor:: Jim Fulton <jim@zope.com>
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>


The module :mod:`cStringIO` provides an interface similar to that of the
:mod:`StringIO` module.  Heavy use of :class:`StringIO.StringIO` objects can be
made more efficient by using the function :func:`StringIO` from this module
instead.


.. function:: StringIO([s])

   Return a StringIO-like stream for reading or writing.

   Since this is a factory function which returns objects of built-in types,
   there's no way to build your own version using subclassing.  It's not
   possible to set attributes on it.  Use the original :mod:`StringIO` module in
   those cases.

   Unlike the :mod:`StringIO` module, this module is not able to accept Unicode
   strings that cannot be encoded as plain ASCII strings.  Calling
   :func:`StringIO` with a Unicode string parameter populates the object with
   the buffer representation of the Unicode string instead of encoding the
   string.

   Another difference from the :mod:`StringIO` module is that calling
   :func:`StringIO` with a string parameter creates a read-only object. Unlike an
   object created without a string parameter, it does not have write methods.
   These objects are not generally visible.  They turn up in tracebacks as
   :class:`StringI` and :class:`StringO`.



The following data objects are provided as well:


.. data:: InputType

   The type object of the objects created by calling :func:`StringIO` with a string
   parameter.


.. data:: OutputType

   The type object of the objects returned by calling :func:`StringIO` with no
   parameters.

There is a C API to the module as well; refer to the module source for  more
information.

Example usage::

   import cStringIO

   output = cStringIO.StringIO()
   output.write('First line.\n')
   print >>output, 'Second line.'

   # Retrieve file contents -- this will be
   # 'First line.\nSecond line.\n'
   contents = output.getvalue()

   # Close object and discard memory buffer --
   # .getvalue() will now raise an exception.
   output.close()

