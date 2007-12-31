
:mod:`struct` --- Interpret strings as packed binary data
=========================================================

.. module:: struct
   :synopsis: Interpret strings as packed binary data.

.. index::
   pair: C; structures
   triple: packing; binary; data

This module performs conversions between Python values and C structs represented
as Python strings.  It uses :dfn:`format strings` (explained below) as compact
descriptions of the lay-out of the C structs and the intended conversion to/from
Python values.  This can be used in handling binary data stored in files or from
network connections, among other sources.

The module defines the following exception and functions:


.. exception:: error

   Exception raised on various occasions; argument is a string describing what is
   wrong.


.. function:: pack(fmt, v1, v2, ...)

   Return a string containing the values ``v1, v2, ...`` packed according to the
   given format.  The arguments must match the values required by the format
   exactly.


.. function:: pack_into(fmt, buffer, offset, v1, v2, ...)

   Pack the values ``v1, v2, ...`` according to the given format, write the packed
   bytes into the writable *buffer* starting at *offset*. Note that the offset is
   a required argument.


.. function:: unpack(fmt, string)

   Unpack the string (presumably packed by ``pack(fmt, ...)``) according to the
   given format.  The result is a tuple even if it contains exactly one item.  The
   string must contain exactly the amount of data required by the format
   (``len(string)`` must equal ``calcsize(fmt)``).


.. function:: unpack_from(fmt, buffer[,offset=0])

   Unpack the *buffer* according to tthe given format. The result is a tuple even
   if it contains exactly one item. The *buffer* must contain at least the amount
   of data required by the format (``len(buffer[offset:])`` must be at least
   ``calcsize(fmt)``).


.. function:: calcsize(fmt)

   Return the size of the struct (and hence of the string) corresponding to the
   given format.

Format characters have the following meaning; the conversion between C and
Python values should be obvious given their types:

+--------+-------------------------+--------------------+-------+
| Format | C Type                  | Python             | Notes |
+========+=========================+====================+=======+
| ``x``  | pad byte                | no value           |       |
+--------+-------------------------+--------------------+-------+
| ``c``  | :ctype:`char`           | string of length 1 |       |
+--------+-------------------------+--------------------+-------+
| ``b``  | :ctype:`signed char`    | integer            |       |
+--------+-------------------------+--------------------+-------+
| ``B``  | :ctype:`unsigned char`  | integer            |       |
+--------+-------------------------+--------------------+-------+
| ``t``  | :ctype:`_Bool`          | bool               | \(1)  |
+--------+-------------------------+--------------------+-------+
| ``h``  | :ctype:`short`          | integer            |       |
+--------+-------------------------+--------------------+-------+
| ``H``  | :ctype:`unsigned short` | integer            |       |
+--------+-------------------------+--------------------+-------+
| ``i``  | :ctype:`int`            | integer            |       |
+--------+-------------------------+--------------------+-------+
| ``I``  | :ctype:`unsigned int`   | integer            |       |
+--------+-------------------------+--------------------+-------+
| ``l``  | :ctype:`long`           | integer            |       |
+--------+-------------------------+--------------------+-------+
| ``L``  | :ctype:`unsigned long`  | integer            |       |
+--------+-------------------------+--------------------+-------+
| ``q``  | :ctype:`long long`      | integer            | \(2)  |
+--------+-------------------------+--------------------+-------+
| ``Q``  | :ctype:`unsigned long   | integer            | \(2)  |
|        | long`                   |                    |       |
+--------+-------------------------+--------------------+-------+
| ``f``  | :ctype:`float`          | float              |       |
+--------+-------------------------+--------------------+-------+
| ``d``  | :ctype:`double`         | float              |       |
+--------+-------------------------+--------------------+-------+
| ``s``  | :ctype:`char[]`         | string             |       |
+--------+-------------------------+--------------------+-------+
| ``p``  | :ctype:`char[]`         | string             |       |
+--------+-------------------------+--------------------+-------+
| ``P``  | :ctype:`void \*`        | integer            |       |
+--------+-------------------------+--------------------+-------+

Notes:

(1)
   The ``'t'`` conversion code corresponds to the :ctype:`_Bool` type defined by
   C99. If this type is not available, it is simulated using a :ctype:`char`. In
   standard mode, it is always represented by one byte.

(2)
   The ``'q'`` and ``'Q'`` conversion codes are available in native mode only if
   the platform C compiler supports C :ctype:`long long`, or, on Windows,
   :ctype:`__int64`.  They are always available in standard modes.

A format character may be preceded by an integral repeat count.  For example,
the format string ``'4h'`` means exactly the same as ``'hhhh'``.

Whitespace characters between formats are ignored; a count and its format must
not contain whitespace though.

For the ``'s'`` format character, the count is interpreted as the size of the
string, not a repeat count like for the other format characters; for example,
``'10s'`` means a single 10-byte string, while ``'10c'`` means 10 characters.
For packing, the string is truncated or padded with null bytes as appropriate to
make it fit. For unpacking, the resulting string always has exactly the
specified number of bytes.  As a special case, ``'0s'`` means a single, empty
string (while ``'0c'`` means 0 characters).

The ``'p'`` format character encodes a "Pascal string", meaning a short
variable-length string stored in a fixed number of bytes. The count is the total
number of bytes stored.  The first byte stored is the length of the string, or
255, whichever is smaller.  The bytes of the string follow.  If the string
passed in to :func:`pack` is too long (longer than the count minus 1), only the
leading count-1 bytes of the string are stored.  If the string is shorter than
count-1, it is padded with null bytes so that exactly count bytes in all are
used.  Note that for :func:`unpack`, the ``'p'`` format character consumes count
bytes, but that the string returned can never contain more than 255 characters.



For the ``'t'`` format character, the return value is either :const:`True` or
:const:`False`. When packing, the truth value of the argument object is used.
Either 0 or 1 in the native or standard bool representation will be packed, and
any non-zero value will be True when unpacking.

By default, C numbers are represented in the machine's native format and byte
order, and properly aligned by skipping pad bytes if necessary (according to the
rules used by the C compiler).

Alternatively, the first character of the format string can be used to indicate
the byte order, size and alignment of the packed data, according to the
following table:

+-----------+------------------------+--------------------+
| Character | Byte order             | Size and alignment |
+===========+========================+====================+
| ``@``     | native                 | native             |
+-----------+------------------------+--------------------+
| ``=``     | native                 | standard           |
+-----------+------------------------+--------------------+
| ``<``     | little-endian          | standard           |
+-----------+------------------------+--------------------+
| ``>``     | big-endian             | standard           |
+-----------+------------------------+--------------------+
| ``!``     | network (= big-endian) | standard           |
+-----------+------------------------+--------------------+

If the first character is not one of these, ``'@'`` is assumed.

Native byte order is big-endian or little-endian, depending on the host system.
For example, Motorola and Sun processors are big-endian; Intel and DEC
processors are little-endian.

Native size and alignment are determined using the C compiler's
``sizeof`` expression.  This is always combined with native byte order.

Standard size and alignment are as follows: no alignment is required for any
type (so you have to use pad bytes); :ctype:`short` is 2 bytes; :ctype:`int` and
:ctype:`long` are 4 bytes; :ctype:`long long` (:ctype:`__int64` on Windows) is 8
bytes; :ctype:`float` and :ctype:`double` are 32-bit and 64-bit IEEE floating
point numbers, respectively. :ctype:`_Bool` is 1 byte.

Note the difference between ``'@'`` and ``'='``: both use native byte order, but
the size and alignment of the latter is standardized.

The form ``'!'`` is available for those poor souls who claim they can't remember
whether network byte order is big-endian or little-endian.

There is no way to indicate non-native byte order (force byte-swapping); use the
appropriate choice of ``'<'`` or ``'>'``.

The ``'P'`` format character is only available for the native byte ordering
(selected as the default or with the ``'@'`` byte order character). The byte
order character ``'='`` chooses to use little- or big-endian ordering based on
the host system. The struct module does not interpret this as native ordering,
so the ``'P'`` format is not available.

Examples (all using native byte order, size and alignment, on a big-endian
machine)::

   >>> from struct import *
   >>> pack('hhl', 1, 2, 3)
   '\x00\x01\x00\x02\x00\x00\x00\x03'
   >>> unpack('hhl', '\x00\x01\x00\x02\x00\x00\x00\x03')
   (1, 2, 3)
   >>> calcsize('hhl')
   8

Hint: to align the end of a structure to the alignment requirement of a
particular type, end the format with the code for that type with a repeat count
of zero.  For example, the format ``'llh0l'`` specifies two pad bytes at the
end, assuming longs are aligned on 4-byte boundaries.  This only works when
native size and alignment are in effect; standard size and alignment does not
enforce any alignment.


.. seealso::

   Module :mod:`array`
      Packed binary storage of homogeneous data.

   Module :mod:`xdrlib`
      Packing and unpacking of XDR data.


.. _struct-objects:

Struct Objects
--------------

The :mod:`struct` module also defines the following type:


.. class:: Struct(format)

   Return a new Struct object which writes and reads binary data according to the
   format string *format*.  Creating a Struct object once and calling its methods
   is more efficient than calling the :mod:`struct` functions with the same format
   since the format string only needs to be compiled once.


Compiled Struct objects support the following methods and attributes:

.. method:: Struct.pack(v1, v2, ...)

   Identical to the :func:`pack` function, using the compiled format.
   (``len(result)`` will equal :attr:`self.size`.)


.. method:: Struct.pack_into(buffer, offset, v1, v2, ...)

   Identical to the :func:`pack_into` function, using the compiled format.


.. method:: Struct.unpack(string)

   Identical to the :func:`unpack` function, using the compiled format.
   (``len(string)`` must equal :attr:`self.size`).


.. method:: Struct.unpack_from(buffer[, offset=0])

   Identical to the :func:`unpack_from` function, using the compiled format.
   (``len(buffer[offset:])`` must be at least :attr:`self.size`).


.. attribute:: Struct.format

   The format string used to construct this Struct object.

.. attribute:: Struct.size

   The calculated size of the struct (and hence of the string) corresponding
   to :attr:`format`.

