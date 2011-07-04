:mod:`xdrlib` --- Encode and decode XDR data
============================================

.. module:: xdrlib
   :synopsis: Encoders and decoders for the External Data Representation (XDR).


.. index::
   single: XDR
   single: External Data Representation

**Source code:** :source:`Lib/xdrlib.py`

--------------

The :mod:`xdrlib` module supports the External Data Representation Standard as
described in :rfc:`1014`, written by Sun Microsystems, Inc. June 1987.  It
supports most of the data types described in the RFC.

The :mod:`xdrlib` module defines two classes, one for packing variables into XDR
representation, and another for unpacking from XDR representation.  There are
also two exception classes.


.. class:: Packer()

   :class:`Packer` is the class for packing data into XDR representation. The
   :class:`Packer` class is instantiated with no arguments.


.. class:: Unpacker(data)

   ``Unpacker`` is the complementary class which unpacks XDR data values from a
   string buffer.  The input buffer is given as *data*.


.. seealso::

   :rfc:`1014` - XDR: External Data Representation Standard
      This RFC defined the encoding of data which was XDR at the time this module was
      originally written.  It has apparently been obsoleted by :rfc:`1832`.

   :rfc:`1832` - XDR: External Data Representation Standard
      Newer RFC that provides a revised definition of XDR.


.. _xdr-packer-objects:

Packer Objects
--------------

:class:`Packer` instances have the following methods:


.. method:: Packer.get_buffer()

   Returns the current pack buffer as a string.


.. method:: Packer.reset()

   Resets the pack buffer to the empty string.

In general, you can pack any of the most common XDR data types by calling the
appropriate ``pack_type()`` method.  Each method takes a single argument, the
value to pack.  The following simple data type packing methods are supported:
:meth:`pack_uint`, :meth:`pack_int`, :meth:`pack_enum`, :meth:`pack_bool`,
:meth:`pack_uhyper`, and :meth:`pack_hyper`.


.. method:: Packer.pack_float(value)

   Packs the single-precision floating point number *value*.


.. method:: Packer.pack_double(value)

   Packs the double-precision floating point number *value*.

The following methods support packing strings, bytes, and opaque data:


.. method:: Packer.pack_fstring(n, s)

   Packs a fixed length string, *s*.  *n* is the length of the string but it is
   *not* packed into the data buffer.  The string is padded with null bytes if
   necessary to guaranteed 4 byte alignment.


.. method:: Packer.pack_fopaque(n, data)

   Packs a fixed length opaque data stream, similarly to :meth:`pack_fstring`.


.. method:: Packer.pack_string(s)

   Packs a variable length string, *s*.  The length of the string is first packed
   as an unsigned integer, then the string data is packed with
   :meth:`pack_fstring`.


.. method:: Packer.pack_opaque(data)

   Packs a variable length opaque data string, similarly to :meth:`pack_string`.


.. method:: Packer.pack_bytes(bytes)

   Packs a variable length byte stream, similarly to :meth:`pack_string`.

The following methods support packing arrays and lists:


.. method:: Packer.pack_list(list, pack_item)

   Packs a *list* of homogeneous items.  This method is useful for lists with an
   indeterminate size; i.e. the size is not available until the entire list has
   been walked.  For each item in the list, an unsigned integer ``1`` is packed
   first, followed by the data value from the list.  *pack_item* is the function
   that is called to pack the individual item.  At the end of the list, an unsigned
   integer ``0`` is packed.

   For example, to pack a list of integers, the code might appear like this::

      import xdrlib
      p = xdrlib.Packer()
      p.pack_list([1, 2, 3], p.pack_int)


.. method:: Packer.pack_farray(n, array, pack_item)

   Packs a fixed length list (*array*) of homogeneous items.  *n* is the length of
   the list; it is *not* packed into the buffer, but a :exc:`ValueError` exception
   is raised if ``len(array)`` is not equal to *n*.  As above, *pack_item* is the
   function used to pack each element.


.. method:: Packer.pack_array(list, pack_item)

   Packs a variable length *list* of homogeneous items.  First, the length of the
   list is packed as an unsigned integer, then each element is packed as in
   :meth:`pack_farray` above.


.. _xdr-unpacker-objects:

Unpacker Objects
----------------

The :class:`Unpacker` class offers the following methods:


.. method:: Unpacker.reset(data)

   Resets the string buffer with the given *data*.


.. method:: Unpacker.get_position()

   Returns the current unpack position in the data buffer.


.. method:: Unpacker.set_position(position)

   Sets the data buffer unpack position to *position*.  You should be careful about
   using :meth:`get_position` and :meth:`set_position`.


.. method:: Unpacker.get_buffer()

   Returns the current unpack data buffer as a string.


.. method:: Unpacker.done()

   Indicates unpack completion.  Raises an :exc:`Error` exception if all of the
   data has not been unpacked.

In addition, every data type that can be packed with a :class:`Packer`, can be
unpacked with an :class:`Unpacker`.  Unpacking methods are of the form
``unpack_type()``, and take no arguments.  They return the unpacked object.


.. method:: Unpacker.unpack_float()

   Unpacks a single-precision floating point number.


.. method:: Unpacker.unpack_double()

   Unpacks a double-precision floating point number, similarly to
   :meth:`unpack_float`.

In addition, the following methods unpack strings, bytes, and opaque data:


.. method:: Unpacker.unpack_fstring(n)

   Unpacks and returns a fixed length string.  *n* is the number of characters
   expected.  Padding with null bytes to guaranteed 4 byte alignment is assumed.


.. method:: Unpacker.unpack_fopaque(n)

   Unpacks and returns a fixed length opaque data stream, similarly to
   :meth:`unpack_fstring`.


.. method:: Unpacker.unpack_string()

   Unpacks and returns a variable length string.  The length of the string is first
   unpacked as an unsigned integer, then the string data is unpacked with
   :meth:`unpack_fstring`.


.. method:: Unpacker.unpack_opaque()

   Unpacks and returns a variable length opaque data string, similarly to
   :meth:`unpack_string`.


.. method:: Unpacker.unpack_bytes()

   Unpacks and returns a variable length byte stream, similarly to
   :meth:`unpack_string`.

The following methods support unpacking arrays and lists:


.. method:: Unpacker.unpack_list(unpack_item)

   Unpacks and returns a list of homogeneous items.  The list is unpacked one
   element at a time by first unpacking an unsigned integer flag.  If the flag is
   ``1``, then the item is unpacked and appended to the list.  A flag of ``0``
   indicates the end of the list.  *unpack_item* is the function that is called to
   unpack the items.


.. method:: Unpacker.unpack_farray(n, unpack_item)

   Unpacks and returns (as a list) a fixed length array of homogeneous items.  *n*
   is number of list elements to expect in the buffer. As above, *unpack_item* is
   the function used to unpack each element.


.. method:: Unpacker.unpack_array(unpack_item)

   Unpacks and returns a variable length *list* of homogeneous items. First, the
   length of the list is unpacked as an unsigned integer, then each element is
   unpacked as in :meth:`unpack_farray` above.


.. _xdr-exceptions:

Exceptions
----------

Exceptions in this module are coded as class instances:


.. exception:: Error

   The base exception class.  :exc:`Error` has a single public attribute
   :attr:`msg` containing the description of the error.


.. exception:: ConversionError

   Class derived from :exc:`Error`.  Contains no additional instance variables.

Here is an example of how you would catch one of these exceptions::

   import xdrlib
   p = xdrlib.Packer()
   try:
       p.pack_double(8.01)
   except xdrlib.ConversionError as instance:
       print('packing the double failed:', instance.msg)

