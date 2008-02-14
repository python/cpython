.. highlightlang:: c

.. _unicodeobjects:

Unicode Objects and Codecs
--------------------------

.. sectionauthor:: Marc-Andre Lemburg <mal@lemburg.com>

Unicode Objects
^^^^^^^^^^^^^^^


These are the basic Unicode object types used for the Unicode implementation in
Python:

.. % --- Unicode Type -------------------------------------------------------


.. ctype:: Py_UNICODE

   This type represents the storage type which is used by Python internally as
   basis for holding Unicode ordinals.  Python's default builds use a 16-bit type
   for :ctype:`Py_UNICODE` and store Unicode values internally as UCS2. It is also
   possible to build a UCS4 version of Python (most recent Linux distributions come
   with UCS4 builds of Python). These builds then use a 32-bit type for
   :ctype:`Py_UNICODE` and store Unicode data internally as UCS4. On platforms
   where :ctype:`wchar_t` is available and compatible with the chosen Python
   Unicode build variant, :ctype:`Py_UNICODE` is a typedef alias for
   :ctype:`wchar_t` to enhance native platform compatibility. On all other
   platforms, :ctype:`Py_UNICODE` is a typedef alias for either :ctype:`unsigned
   short` (UCS2) or :ctype:`unsigned long` (UCS4).

Note that UCS2 and UCS4 Python builds are not binary compatible. Please keep
this in mind when writing extensions or interfaces.


.. ctype:: PyUnicodeObject

   This subtype of :ctype:`PyObject` represents a Python Unicode object.


.. cvar:: PyTypeObject PyUnicode_Type

   This instance of :ctype:`PyTypeObject` represents the Python Unicode type.  It
   is exposed to Python code as ``unicode`` and ``types.UnicodeType``.

The following APIs are really C macros and can be used to do fast checks and to
access internal read-only data of Unicode objects:


.. cfunction:: int PyUnicode_Check(PyObject *o)

   Return true if the object *o* is a Unicode object or an instance of a Unicode
   subtype.

   .. versionchanged:: 2.2
      Allowed subtypes to be accepted.


.. cfunction:: int PyUnicode_CheckExact(PyObject *o)

   Return true if the object *o* is a Unicode object, but not an instance of a
   subtype.

   .. versionadded:: 2.2


.. cfunction:: Py_ssize_t PyUnicode_GET_SIZE(PyObject *o)

   Return the size of the object.  *o* has to be a :ctype:`PyUnicodeObject` (not
   checked).


.. cfunction:: Py_ssize_t PyUnicode_GET_DATA_SIZE(PyObject *o)

   Return the size of the object's internal buffer in bytes.  *o* has to be a
   :ctype:`PyUnicodeObject` (not checked).


.. cfunction:: Py_UNICODE* PyUnicode_AS_UNICODE(PyObject *o)

   Return a pointer to the internal :ctype:`Py_UNICODE` buffer of the object.  *o*
   has to be a :ctype:`PyUnicodeObject` (not checked).


.. cfunction:: const char* PyUnicode_AS_DATA(PyObject *o)

   Return a pointer to the internal buffer of the object. *o* has to be a
   :ctype:`PyUnicodeObject` (not checked).


.. cfunction:: int PyUnicode_ClearFreeList(void)

   Clear the free list. Return the total number of freed items.

   .. versionadded:: 2.6

Unicode provides many different character properties. The most often needed ones
are available through these macros which are mapped to C functions depending on
the Python configuration.

.. % --- Unicode character properties ---------------------------------------


.. cfunction:: int Py_UNICODE_ISSPACE(Py_UNICODE ch)

   Return 1 or 0 depending on whether *ch* is a whitespace character.


.. cfunction:: int Py_UNICODE_ISLOWER(Py_UNICODE ch)

   Return 1 or 0 depending on whether *ch* is a lowercase character.


.. cfunction:: int Py_UNICODE_ISUPPER(Py_UNICODE ch)

   Return 1 or 0 depending on whether *ch* is an uppercase character.


.. cfunction:: int Py_UNICODE_ISTITLE(Py_UNICODE ch)

   Return 1 or 0 depending on whether *ch* is a titlecase character.


.. cfunction:: int Py_UNICODE_ISLINEBREAK(Py_UNICODE ch)

   Return 1 or 0 depending on whether *ch* is a linebreak character.


.. cfunction:: int Py_UNICODE_ISDECIMAL(Py_UNICODE ch)

   Return 1 or 0 depending on whether *ch* is a decimal character.


.. cfunction:: int Py_UNICODE_ISDIGIT(Py_UNICODE ch)

   Return 1 or 0 depending on whether *ch* is a digit character.


.. cfunction:: int Py_UNICODE_ISNUMERIC(Py_UNICODE ch)

   Return 1 or 0 depending on whether *ch* is a numeric character.


.. cfunction:: int Py_UNICODE_ISALPHA(Py_UNICODE ch)

   Return 1 or 0 depending on whether *ch* is an alphabetic character.


.. cfunction:: int Py_UNICODE_ISALNUM(Py_UNICODE ch)

   Return 1 or 0 depending on whether *ch* is an alphanumeric character.

These APIs can be used for fast direct character conversions:


.. cfunction:: Py_UNICODE Py_UNICODE_TOLOWER(Py_UNICODE ch)

   Return the character *ch* converted to lower case.


.. cfunction:: Py_UNICODE Py_UNICODE_TOUPPER(Py_UNICODE ch)

   Return the character *ch* converted to upper case.


.. cfunction:: Py_UNICODE Py_UNICODE_TOTITLE(Py_UNICODE ch)

   Return the character *ch* converted to title case.


.. cfunction:: int Py_UNICODE_TODECIMAL(Py_UNICODE ch)

   Return the character *ch* converted to a decimal positive integer.  Return
   ``-1`` if this is not possible.  This macro does not raise exceptions.


.. cfunction:: int Py_UNICODE_TODIGIT(Py_UNICODE ch)

   Return the character *ch* converted to a single digit integer. Return ``-1`` if
   this is not possible.  This macro does not raise exceptions.


.. cfunction:: double Py_UNICODE_TONUMERIC(Py_UNICODE ch)

   Return the character *ch* converted to a double. Return ``-1.0`` if this is not
   possible.  This macro does not raise exceptions.

To create Unicode objects and access their basic sequence properties, use these
APIs:

.. % --- Plain Py_UNICODE ---------------------------------------------------


.. cfunction:: PyObject* PyUnicode_FromUnicode(const Py_UNICODE *u, Py_ssize_t size)

   Create a Unicode Object from the Py_UNICODE buffer *u* of the given size. *u*
   may be *NULL* which causes the contents to be undefined. It is the user's
   responsibility to fill in the needed data.  The buffer is copied into the new
   object. If the buffer is not *NULL*, the return value might be a shared object.
   Therefore, modification of the resulting Unicode object is only allowed when *u*
   is *NULL*.


.. cfunction:: Py_UNICODE* PyUnicode_AsUnicode(PyObject *unicode)

   Return a read-only pointer to the Unicode object's internal :ctype:`Py_UNICODE`
   buffer, *NULL* if *unicode* is not a Unicode object.


.. cfunction:: Py_ssize_t PyUnicode_GetSize(PyObject *unicode)

   Return the length of the Unicode object.


.. cfunction:: PyObject* PyUnicode_FromEncodedObject(PyObject *obj, const char *encoding, const char *errors)

   Coerce an encoded object *obj* to an Unicode object and return a reference with
   incremented refcount.

   String and other char buffer compatible objects are decoded according to the
   given encoding and using the error handling defined by errors.  Both can be
   *NULL* to have the interface use the default values (see the next section for
   details).

   All other objects, including Unicode objects, cause a :exc:`TypeError` to be
   set.

   The API returns *NULL* if there was an error.  The caller is responsible for
   decref'ing the returned objects.


.. cfunction:: PyObject* PyUnicode_FromObject(PyObject *obj)

   Shortcut for ``PyUnicode_FromEncodedObject(obj, NULL, "strict")`` which is used
   throughout the interpreter whenever coercion to Unicode is needed.

If the platform supports :ctype:`wchar_t` and provides a header file wchar.h,
Python can interface directly to this type using the following functions.
Support is optimized if Python's own :ctype:`Py_UNICODE` type is identical to
the system's :ctype:`wchar_t`.

.. % --- wchar_t support for platforms which support it ---------------------


.. cfunction:: PyObject* PyUnicode_FromWideChar(const wchar_t *w, Py_ssize_t size)

   Create a Unicode object from the :ctype:`wchar_t` buffer *w* of the given size.
   Return *NULL* on failure.


.. cfunction:: Py_ssize_t PyUnicode_AsWideChar(PyUnicodeObject *unicode, wchar_t *w, Py_ssize_t size)

   Copy the Unicode object contents into the :ctype:`wchar_t` buffer *w*.  At most
   *size* :ctype:`wchar_t` characters are copied (excluding a possibly trailing
   0-termination character).  Return the number of :ctype:`wchar_t` characters
   copied or -1 in case of an error.  Note that the resulting :ctype:`wchar_t`
   string may or may not be 0-terminated.  It is the responsibility of the caller
   to make sure that the :ctype:`wchar_t` string is 0-terminated in case this is
   required by the application.


.. _builtincodecs:

Built-in Codecs
^^^^^^^^^^^^^^^

Python provides a set of builtin codecs which are written in C for speed. All of
these codecs are directly usable via the following functions.

Many of the following APIs take two arguments encoding and errors. These
parameters encoding and errors have the same semantics as the ones of the
builtin unicode() Unicode object constructor.

Setting encoding to *NULL* causes the default encoding to be used which is
ASCII.  The file system calls should use :cdata:`Py_FileSystemDefaultEncoding`
as the encoding for file names. This variable should be treated as read-only: On
some systems, it will be a pointer to a static string, on others, it will change
at run-time (such as when the application invokes setlocale).

Error handling is set by errors which may also be set to *NULL* meaning to use
the default handling defined for the codec.  Default error handling for all
builtin codecs is "strict" (:exc:`ValueError` is raised).

The codecs all use a similar interface.  Only deviation from the following
generic ones are documented for simplicity.

These are the generic codec APIs:

.. % --- Generic Codecs -----------------------------------------------------


.. cfunction:: PyObject* PyUnicode_Decode(const char *s, Py_ssize_t size, const char *encoding, const char *errors)

   Create a Unicode object by decoding *size* bytes of the encoded string *s*.
   *encoding* and *errors* have the same meaning as the parameters of the same name
   in the :func:`unicode` builtin function.  The codec to be used is looked up
   using the Python codec registry.  Return *NULL* if an exception was raised by
   the codec.


.. cfunction:: PyObject* PyUnicode_Encode(const Py_UNICODE *s, Py_ssize_t size, const char *encoding, const char *errors)

   Encode the :ctype:`Py_UNICODE` buffer of the given size and return a Python
   string object.  *encoding* and *errors* have the same meaning as the parameters
   of the same name in the Unicode :meth:`encode` method.  The codec to be used is
   looked up using the Python codec registry.  Return *NULL* if an exception was
   raised by the codec.


.. cfunction:: PyObject* PyUnicode_AsEncodedString(PyObject *unicode, const char *encoding, const char *errors)

   Encode a Unicode object and return the result as Python string object.
   *encoding* and *errors* have the same meaning as the parameters of the same name
   in the Unicode :meth:`encode` method. The codec to be used is looked up using
   the Python codec registry. Return *NULL* if an exception was raised by the
   codec.

These are the UTF-8 codec APIs:

.. % --- UTF-8 Codecs -------------------------------------------------------


.. cfunction:: PyObject* PyUnicode_DecodeUTF8(const char *s, Py_ssize_t size, const char *errors)

   Create a Unicode object by decoding *size* bytes of the UTF-8 encoded string
   *s*. Return *NULL* if an exception was raised by the codec.


.. cfunction:: PyObject* PyUnicode_DecodeUTF8Stateful(const char *s, Py_ssize_t size, const char *errors, Py_ssize_t *consumed)

   If *consumed* is *NULL*, behave like :cfunc:`PyUnicode_DecodeUTF8`. If
   *consumed* is not *NULL*, trailing incomplete UTF-8 byte sequences will not be
   treated as an error. Those bytes will not be decoded and the number of bytes
   that have been decoded will be stored in *consumed*.

   .. versionadded:: 2.4


.. cfunction:: PyObject* PyUnicode_EncodeUTF8(const Py_UNICODE *s, Py_ssize_t size, const char *errors)

   Encode the :ctype:`Py_UNICODE` buffer of the given size using UTF-8 and return a
   Python string object.  Return *NULL* if an exception was raised by the codec.


.. cfunction:: PyObject* PyUnicode_AsUTF8String(PyObject *unicode)

   Encode a Unicode object using UTF-8 and return the result as Python string
   object.  Error handling is "strict".  Return *NULL* if an exception was raised
   by the codec.

These are the UTF-32 codec APIs:

.. % --- UTF-32 Codecs ------------------------------------------------------ */


.. cfunction:: PyObject* PyUnicode_DecodeUTF32(const char *s, Py_ssize_t size, const char *errors, int *byteorder)

   Decode *length* bytes from a UTF-32 encoded buffer string and return the
   corresponding Unicode object.  *errors* (if non-*NULL*) defines the error
   handling. It defaults to "strict".

   If *byteorder* is non-*NULL*, the decoder starts decoding using the given byte
   order::

      *byteorder == -1: little endian
      *byteorder == 0:  native order
      *byteorder == 1:  big endian

   and then switches if the first four bytes of the input data are a byte order mark
   (BOM) and the specified byte order is native order.  This BOM is not copied into
   the resulting Unicode string.  After completion, *\*byteorder* is set to the
   current byte order at the end of input data.

   In a narrow build codepoints outside the BMP will be decoded as surrogate pairs.

   If *byteorder* is *NULL*, the codec starts in native order mode.

   Return *NULL* if an exception was raised by the codec.

   .. versionadded:: 2.6


.. cfunction:: PyObject* PyUnicode_DecodeUTF32Stateful(const char *s, Py_ssize_t size, const char *errors, int *byteorder, Py_ssize_t *consumed)

   If *consumed* is *NULL*, behave like :cfunc:`PyUnicode_DecodeUTF32`. If
   *consumed* is not *NULL*, :cfunc:`PyUnicode_DecodeUTF32Stateful` will not treat
   trailing incomplete UTF-32 byte sequences (such as a number of bytes not divisible
   by four) as an error. Those bytes will not be decoded and the number of bytes
   that have been decoded will be stored in *consumed*.

   .. versionadded:: 2.6


.. cfunction:: PyObject* PyUnicode_EncodeUTF32(const Py_UNICODE *s, Py_ssize_t size, const char *errors, int byteorder)

   Return a Python bytes object holding the UTF-32 encoded value of the Unicode
   data in *s*.  If *byteorder* is not ``0``, output is written according to the
   following byte order::

      byteorder == -1: little endian
      byteorder == 0:  native byte order (writes a BOM mark)
      byteorder == 1:  big endian

   If byteorder is ``0``, the output string will always start with the Unicode BOM
   mark (U+FEFF). In the other two modes, no BOM mark is prepended.

   If *Py_UNICODE_WIDE* is not defined, surrogate pairs will be output
   as a single codepoint.

   Return *NULL* if an exception was raised by the codec.

   .. versionadded:: 2.6


.. cfunction:: PyObject* PyUnicode_AsUTF32String(PyObject *unicode)

   Return a Python string using the UTF-32 encoding in native byte order. The
   string always starts with a BOM mark.  Error handling is "strict".  Return
   *NULL* if an exception was raised by the codec.

   .. versionadded:: 2.6


These are the UTF-16 codec APIs:

.. % --- UTF-16 Codecs ------------------------------------------------------ */


.. cfunction:: PyObject* PyUnicode_DecodeUTF16(const char *s, Py_ssize_t size, const char *errors, int *byteorder)

   Decode *length* bytes from a UTF-16 encoded buffer string and return the
   corresponding Unicode object.  *errors* (if non-*NULL*) defines the error
   handling. It defaults to "strict".

   If *byteorder* is non-*NULL*, the decoder starts decoding using the given byte
   order::

      *byteorder == -1: little endian
      *byteorder == 0:  native order
      *byteorder == 1:  big endian

   and then switches if the first two bytes of the input data are a byte order mark
   (BOM) and the specified byte order is native order.  This BOM is not copied into
   the resulting Unicode string.  After completion, *\*byteorder* is set to the
   current byte order at the.

   If *byteorder* is *NULL*, the codec starts in native order mode.

   Return *NULL* if an exception was raised by the codec.


.. cfunction:: PyObject* PyUnicode_DecodeUTF16Stateful(const char *s, Py_ssize_t size, const char *errors, int *byteorder, Py_ssize_t *consumed)

   If *consumed* is *NULL*, behave like :cfunc:`PyUnicode_DecodeUTF16`. If
   *consumed* is not *NULL*, :cfunc:`PyUnicode_DecodeUTF16Stateful` will not treat
   trailing incomplete UTF-16 byte sequences (such as an odd number of bytes or a
   split surrogate pair) as an error. Those bytes will not be decoded and the
   number of bytes that have been decoded will be stored in *consumed*.

   .. versionadded:: 2.4


.. cfunction:: PyObject* PyUnicode_EncodeUTF16(const Py_UNICODE *s, Py_ssize_t size, const char *errors, int byteorder)

   Return a Python string object holding the UTF-16 encoded value of the Unicode
   data in *s*.  If *byteorder* is not ``0``, output is written according to the
   following byte order::

      byteorder == -1: little endian
      byteorder == 0:  native byte order (writes a BOM mark)
      byteorder == 1:  big endian

   If byteorder is ``0``, the output string will always start with the Unicode BOM
   mark (U+FEFF). In the other two modes, no BOM mark is prepended.

   If *Py_UNICODE_WIDE* is defined, a single :ctype:`Py_UNICODE` value may get
   represented as a surrogate pair. If it is not defined, each :ctype:`Py_UNICODE`
   values is interpreted as an UCS-2 character.

   Return *NULL* if an exception was raised by the codec.


.. cfunction:: PyObject* PyUnicode_AsUTF16String(PyObject *unicode)

   Return a Python string using the UTF-16 encoding in native byte order. The
   string always starts with a BOM mark.  Error handling is "strict".  Return
   *NULL* if an exception was raised by the codec.

These are the "Unicode Escape" codec APIs:

.. % --- Unicode-Escape Codecs ----------------------------------------------


.. cfunction:: PyObject* PyUnicode_DecodeUnicodeEscape(const char *s, Py_ssize_t size, const char *errors)

   Create a Unicode object by decoding *size* bytes of the Unicode-Escape encoded
   string *s*.  Return *NULL* if an exception was raised by the codec.


.. cfunction:: PyObject* PyUnicode_EncodeUnicodeEscape(const Py_UNICODE *s, Py_ssize_t size)

   Encode the :ctype:`Py_UNICODE` buffer of the given size using Unicode-Escape and
   return a Python string object.  Return *NULL* if an exception was raised by the
   codec.


.. cfunction:: PyObject* PyUnicode_AsUnicodeEscapeString(PyObject *unicode)

   Encode a Unicode object using Unicode-Escape and return the result as Python
   string object.  Error handling is "strict". Return *NULL* if an exception was
   raised by the codec.

These are the "Raw Unicode Escape" codec APIs:

.. % --- Raw-Unicode-Escape Codecs ------------------------------------------


.. cfunction:: PyObject* PyUnicode_DecodeRawUnicodeEscape(const char *s, Py_ssize_t size, const char *errors)

   Create a Unicode object by decoding *size* bytes of the Raw-Unicode-Escape
   encoded string *s*.  Return *NULL* if an exception was raised by the codec.


.. cfunction:: PyObject* PyUnicode_EncodeRawUnicodeEscape(const Py_UNICODE *s, Py_ssize_t size, const char *errors)

   Encode the :ctype:`Py_UNICODE` buffer of the given size using Raw-Unicode-Escape
   and return a Python string object.  Return *NULL* if an exception was raised by
   the codec.


.. cfunction:: PyObject* PyUnicode_AsRawUnicodeEscapeString(PyObject *unicode)

   Encode a Unicode object using Raw-Unicode-Escape and return the result as
   Python string object. Error handling is "strict". Return *NULL* if an exception
   was raised by the codec.

These are the Latin-1 codec APIs: Latin-1 corresponds to the first 256 Unicode
ordinals and only these are accepted by the codecs during encoding.

.. % --- Latin-1 Codecs -----------------------------------------------------


.. cfunction:: PyObject* PyUnicode_DecodeLatin1(const char *s, Py_ssize_t size, const char *errors)

   Create a Unicode object by decoding *size* bytes of the Latin-1 encoded string
   *s*.  Return *NULL* if an exception was raised by the codec.


.. cfunction:: PyObject* PyUnicode_EncodeLatin1(const Py_UNICODE *s, Py_ssize_t size, const char *errors)

   Encode the :ctype:`Py_UNICODE` buffer of the given size using Latin-1 and return
   a Python string object.  Return *NULL* if an exception was raised by the codec.


.. cfunction:: PyObject* PyUnicode_AsLatin1String(PyObject *unicode)

   Encode a Unicode object using Latin-1 and return the result as Python string
   object.  Error handling is "strict".  Return *NULL* if an exception was raised
   by the codec.

These are the ASCII codec APIs.  Only 7-bit ASCII data is accepted. All other
codes generate errors.

.. % --- ASCII Codecs -------------------------------------------------------


.. cfunction:: PyObject* PyUnicode_DecodeASCII(const char *s, Py_ssize_t size, const char *errors)

   Create a Unicode object by decoding *size* bytes of the ASCII encoded string
   *s*.  Return *NULL* if an exception was raised by the codec.


.. cfunction:: PyObject* PyUnicode_EncodeASCII(const Py_UNICODE *s, Py_ssize_t size, const char *errors)

   Encode the :ctype:`Py_UNICODE` buffer of the given size using ASCII and return a
   Python string object.  Return *NULL* if an exception was raised by the codec.


.. cfunction:: PyObject* PyUnicode_AsASCIIString(PyObject *unicode)

   Encode a Unicode object using ASCII and return the result as Python string
   object.  Error handling is "strict".  Return *NULL* if an exception was raised
   by the codec.

These are the mapping codec APIs:

.. % --- Character Map Codecs -----------------------------------------------

This codec is special in that it can be used to implement many different codecs
(and this is in fact what was done to obtain most of the standard codecs
included in the :mod:`encodings` package). The codec uses mapping to encode and
decode characters.

Decoding mappings must map single string characters to single Unicode
characters, integers (which are then interpreted as Unicode ordinals) or None
(meaning "undefined mapping" and causing an error).

Encoding mappings must map single Unicode characters to single string
characters, integers (which are then interpreted as Latin-1 ordinals) or None
(meaning "undefined mapping" and causing an error).

The mapping objects provided must only support the __getitem__ mapping
interface.

If a character lookup fails with a LookupError, the character is copied as-is
meaning that its ordinal value will be interpreted as Unicode or Latin-1 ordinal
resp. Because of this, mappings only need to contain those mappings which map
characters to different code points.


.. cfunction:: PyObject* PyUnicode_DecodeCharmap(const char *s, Py_ssize_t size, PyObject *mapping, const char *errors)

   Create a Unicode object by decoding *size* bytes of the encoded string *s* using
   the given *mapping* object.  Return *NULL* if an exception was raised by the
   codec. If *mapping* is *NULL* latin-1 decoding will be done. Else it can be a
   dictionary mapping byte or a unicode string, which is treated as a lookup table.
   Byte values greater that the length of the string and U+FFFE "characters" are
   treated as "undefined mapping".

   .. versionchanged:: 2.4
      Allowed unicode string as mapping argument.


.. cfunction:: PyObject* PyUnicode_EncodeCharmap(const Py_UNICODE *s, Py_ssize_t size, PyObject *mapping, const char *errors)

   Encode the :ctype:`Py_UNICODE` buffer of the given size using the given
   *mapping* object and return a Python string object. Return *NULL* if an
   exception was raised by the codec.


.. cfunction:: PyObject* PyUnicode_AsCharmapString(PyObject *unicode, PyObject *mapping)

   Encode a Unicode object using the given *mapping* object and return the result
   as Python string object.  Error handling is "strict".  Return *NULL* if an
   exception was raised by the codec.

The following codec API is special in that maps Unicode to Unicode.


.. cfunction:: PyObject* PyUnicode_TranslateCharmap(const Py_UNICODE *s, Py_ssize_t size, PyObject *table, const char *errors)

   Translate a :ctype:`Py_UNICODE` buffer of the given length by applying a
   character mapping *table* to it and return the resulting Unicode object.  Return
   *NULL* when an exception was raised by the codec.

   The *mapping* table must map Unicode ordinal integers to Unicode ordinal
   integers or None (causing deletion of the character).

   Mapping tables need only provide the :meth:`__getitem__` interface; dictionaries
   and sequences work well.  Unmapped character ordinals (ones which cause a
   :exc:`LookupError`) are left untouched and are copied as-is.

These are the MBCS codec APIs. They are currently only available on Windows and
use the Win32 MBCS converters to implement the conversions.  Note that MBCS (or
DBCS) is a class of encodings, not just one.  The target encoding is defined by
the user settings on the machine running the codec.

.. % --- MBCS codecs for Windows --------------------------------------------


.. cfunction:: PyObject* PyUnicode_DecodeMBCS(const char *s, Py_ssize_t size, const char *errors)

   Create a Unicode object by decoding *size* bytes of the MBCS encoded string *s*.
   Return *NULL* if an exception was raised by the codec.


.. cfunction:: PyObject* PyUnicode_DecodeMBCSStateful(const char *s, int size, const char *errors, int *consumed)

   If *consumed* is *NULL*, behave like :cfunc:`PyUnicode_DecodeMBCS`. If
   *consumed* is not *NULL*, :cfunc:`PyUnicode_DecodeMBCSStateful` will not decode
   trailing lead byte and the number of bytes that have been decoded will be stored
   in *consumed*.

   .. versionadded:: 2.5


.. cfunction:: PyObject* PyUnicode_EncodeMBCS(const Py_UNICODE *s, Py_ssize_t size, const char *errors)

   Encode the :ctype:`Py_UNICODE` buffer of the given size using MBCS and return a
   Python string object.  Return *NULL* if an exception was raised by the codec.


.. cfunction:: PyObject* PyUnicode_AsMBCSString(PyObject *unicode)

   Encode a Unicode object using MBCS and return the result as Python string
   object.  Error handling is "strict".  Return *NULL* if an exception was raised
   by the codec.

.. % --- Methods & Slots ----------------------------------------------------


.. _unicodemethodsandslots:

Methods and Slot Functions
^^^^^^^^^^^^^^^^^^^^^^^^^^

The following APIs are capable of handling Unicode objects and strings on input
(we refer to them as strings in the descriptions) and return Unicode objects or
integers as appropriate.

They all return *NULL* or ``-1`` if an exception occurs.


.. cfunction:: PyObject* PyUnicode_Concat(PyObject *left, PyObject *right)

   Concat two strings giving a new Unicode string.


.. cfunction:: PyObject* PyUnicode_Split(PyObject *s, PyObject *sep, Py_ssize_t maxsplit)

   Split a string giving a list of Unicode strings.  If sep is *NULL*, splitting
   will be done at all whitespace substrings.  Otherwise, splits occur at the given
   separator.  At most *maxsplit* splits will be done.  If negative, no limit is
   set.  Separators are not included in the resulting list.


.. cfunction:: PyObject* PyUnicode_Splitlines(PyObject *s, int keepend)

   Split a Unicode string at line breaks, returning a list of Unicode strings.
   CRLF is considered to be one line break.  If *keepend* is 0, the Line break
   characters are not included in the resulting strings.


.. cfunction:: PyObject* PyUnicode_Translate(PyObject *str, PyObject *table, const char *errors)

   Translate a string by applying a character mapping table to it and return the
   resulting Unicode object.

   The mapping table must map Unicode ordinal integers to Unicode ordinal integers
   or None (causing deletion of the character).

   Mapping tables need only provide the :meth:`__getitem__` interface; dictionaries
   and sequences work well.  Unmapped character ordinals (ones which cause a
   :exc:`LookupError`) are left untouched and are copied as-is.

   *errors* has the usual meaning for codecs. It may be *NULL* which indicates to
   use the default error handling.


.. cfunction:: PyObject* PyUnicode_Join(PyObject *separator, PyObject *seq)

   Join a sequence of strings using the given separator and return the resulting
   Unicode string.


.. cfunction:: int PyUnicode_Tailmatch(PyObject *str, PyObject *substr, Py_ssize_t start, Py_ssize_t end, int direction)

   Return 1 if *substr* matches *str*[*start*:*end*] at the given tail end
   (*direction* == -1 means to do a prefix match, *direction* == 1 a suffix match),
   0 otherwise. Return ``-1`` if an error occurred.


.. cfunction:: Py_ssize_t PyUnicode_Find(PyObject *str, PyObject *substr, Py_ssize_t start, Py_ssize_t end, int direction)

   Return the first position of *substr* in *str*[*start*:*end*] using the given
   *direction* (*direction* == 1 means to do a forward search, *direction* == -1 a
   backward search).  The return value is the index of the first match; a value of
   ``-1`` indicates that no match was found, and ``-2`` indicates that an error
   occurred and an exception has been set.


.. cfunction:: Py_ssize_t PyUnicode_Count(PyObject *str, PyObject *substr, Py_ssize_t start, Py_ssize_t end)

   Return the number of non-overlapping occurrences of *substr* in
   ``str[start:end]``.  Return ``-1`` if an error occurred.


.. cfunction:: PyObject* PyUnicode_Replace(PyObject *str, PyObject *substr, PyObject *replstr, Py_ssize_t maxcount)

   Replace at most *maxcount* occurrences of *substr* in *str* with *replstr* and
   return the resulting Unicode object. *maxcount* == -1 means replace all
   occurrences.


.. cfunction:: int PyUnicode_Compare(PyObject *left, PyObject *right)

   Compare two strings and return -1, 0, 1 for less than, equal, and greater than,
   respectively.


.. cfunction:: int PyUnicode_RichCompare(PyObject *left,  PyObject *right,  int op)

   Rich compare two unicode strings and return one of the following:

   * ``NULL`` in case an exception was raised
   * :const:`Py_True` or :const:`Py_False` for successful comparisons
   * :const:`Py_NotImplemented` in case the type combination is unknown

   Note that :const:`Py_EQ` and :const:`Py_NE` comparisons can cause a
   :exc:`UnicodeWarning` in case the conversion of the arguments to Unicode fails
   with a :exc:`UnicodeDecodeError`.

   Possible values for *op* are :const:`Py_GT`, :const:`Py_GE`, :const:`Py_EQ`,
   :const:`Py_NE`, :const:`Py_LT`, and :const:`Py_LE`.


.. cfunction:: PyObject* PyUnicode_Format(PyObject *format, PyObject *args)

   Return a new string object from *format* and *args*; this is analogous to
   ``format % args``.  The *args* argument must be a tuple.


.. cfunction:: int PyUnicode_Contains(PyObject *container, PyObject *element)

   Check whether *element* is contained in *container* and return true or false
   accordingly.

   *element* has to coerce to a one element Unicode string. ``-1`` is returned if
   there was an error.
