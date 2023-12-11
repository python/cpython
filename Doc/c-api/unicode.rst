.. highlight:: c

.. _unicodeobjects:

Unicode Objects and Codecs
--------------------------

.. sectionauthor:: Marc-André Lemburg <mal@lemburg.com>
.. sectionauthor:: Georg Brandl <georg@python.org>

Unicode Objects
^^^^^^^^^^^^^^^

Since the implementation of :pep:`393` in Python 3.3, Unicode objects internally
use a variety of representations, in order to allow handling the complete range
of Unicode characters while staying memory efficient.  There are special cases
for strings where all code points are below 128, 256, or 65536; otherwise, code
points must be below 1114112 (which is the full Unicode range).

UTF-8 representation is created on demand and cached in the Unicode object.

.. note::
   The :c:type:`Py_UNICODE` representation has been removed since Python 3.12
   with deprecated APIs.
   See :pep:`623` for more information.


Unicode Type
""""""""""""

These are the basic Unicode object types used for the Unicode implementation in
Python:

.. c:type:: Py_UCS4
            Py_UCS2
            Py_UCS1

   These types are typedefs for unsigned integer types wide enough to contain
   characters of 32 bits, 16 bits and 8 bits, respectively.  When dealing with
   single Unicode characters, use :c:type:`Py_UCS4`.

   .. versionadded:: 3.3


.. c:type:: Py_UNICODE

   This is a typedef of :c:type:`wchar_t`, which is a 16-bit type or 32-bit type
   depending on the platform.

   .. versionchanged:: 3.3
      In previous versions, this was a 16-bit type or a 32-bit type depending on
      whether you selected a "narrow" or "wide" Unicode version of Python at
      build time.

   .. deprecated-removed:: 3.13 3.15


.. c:type:: PyASCIIObject
            PyCompactUnicodeObject
            PyUnicodeObject

   These subtypes of :c:type:`PyObject` represent a Python Unicode object.  In
   almost all cases, they shouldn't be used directly, since all API functions
   that deal with Unicode objects take and return :c:type:`PyObject` pointers.

   .. versionadded:: 3.3


.. c:var:: PyTypeObject PyUnicode_Type

   This instance of :c:type:`PyTypeObject` represents the Python Unicode type.  It
   is exposed to Python code as ``str``.


The following APIs are C macros and static inlined functions for fast checks and
access to internal read-only data of Unicode objects:

.. c:function:: int PyUnicode_Check(PyObject *obj)

   Return true if the object *obj* is a Unicode object or an instance of a Unicode
   subtype.  This function always succeeds.


.. c:function:: int PyUnicode_CheckExact(PyObject *obj)

   Return true if the object *obj* is a Unicode object, but not an instance of a
   subtype.  This function always succeeds.


.. c:function:: int PyUnicode_READY(PyObject *unicode)

   Returns ``0``. This API is kept only for backward compatibility.

   .. versionadded:: 3.3

   .. deprecated:: 3.10
      This API does nothing since Python 3.12.


.. c:function:: Py_ssize_t PyUnicode_GET_LENGTH(PyObject *unicode)

   Return the length of the Unicode string, in code points.  *unicode* has to be a
   Unicode object in the "canonical" representation (not checked).

   .. versionadded:: 3.3


.. c:function:: Py_UCS1* PyUnicode_1BYTE_DATA(PyObject *unicode)
                Py_UCS2* PyUnicode_2BYTE_DATA(PyObject *unicode)
                Py_UCS4* PyUnicode_4BYTE_DATA(PyObject *unicode)

   Return a pointer to the canonical representation cast to UCS1, UCS2 or UCS4
   integer types for direct character access.  No checks are performed if the
   canonical representation has the correct character size; use
   :c:func:`PyUnicode_KIND` to select the right function.

   .. versionadded:: 3.3


.. c:macro:: PyUnicode_1BYTE_KIND
             PyUnicode_2BYTE_KIND
             PyUnicode_4BYTE_KIND

   Return values of the :c:func:`PyUnicode_KIND` macro.

   .. versionadded:: 3.3

   .. versionchanged:: 3.12
      ``PyUnicode_WCHAR_KIND`` has been removed.


.. c:function:: int PyUnicode_KIND(PyObject *unicode)

   Return one of the PyUnicode kind constants (see above) that indicate how many
   bytes per character this Unicode object uses to store its data.  *unicode* has to
   be a Unicode object in the "canonical" representation (not checked).

   .. versionadded:: 3.3


.. c:function:: void* PyUnicode_DATA(PyObject *unicode)

   Return a void pointer to the raw Unicode buffer.  *unicode* has to be a Unicode
   object in the "canonical" representation (not checked).

   .. versionadded:: 3.3


.. c:function:: void PyUnicode_WRITE(int kind, void *data, \
                                     Py_ssize_t index, Py_UCS4 value)

   Write into a canonical representation *data* (as obtained with
   :c:func:`PyUnicode_DATA`).  This function performs no sanity checks, and is
   intended for usage in loops.  The caller should cache the *kind* value and
   *data* pointer as obtained from other calls.  *index* is the index in
   the string (starts at 0) and *value* is the new code point value which should
   be written to that location.

   .. versionadded:: 3.3


.. c:function:: Py_UCS4 PyUnicode_READ(int kind, void *data, \
                                       Py_ssize_t index)

   Read a code point from a canonical representation *data* (as obtained with
   :c:func:`PyUnicode_DATA`).  No checks or ready calls are performed.

   .. versionadded:: 3.3


.. c:function:: Py_UCS4 PyUnicode_READ_CHAR(PyObject *unicode, Py_ssize_t index)

   Read a character from a Unicode object *unicode*, which must be in the "canonical"
   representation.  This is less efficient than :c:func:`PyUnicode_READ` if you
   do multiple consecutive reads.

   .. versionadded:: 3.3


.. c:function:: Py_UCS4 PyUnicode_MAX_CHAR_VALUE(PyObject *unicode)

   Return the maximum code point that is suitable for creating another string
   based on *unicode*, which must be in the "canonical" representation.  This is
   always an approximation but more efficient than iterating over the string.

   .. versionadded:: 3.3


.. c:function:: int PyUnicode_IsIdentifier(PyObject *unicode)

   Return ``1`` if the string is a valid identifier according to the language
   definition, section :ref:`identifiers`. Return ``0`` otherwise.

   .. versionchanged:: 3.9
      The function does not call :c:func:`Py_FatalError` anymore if the string
      is not ready.


Unicode Character Properties
""""""""""""""""""""""""""""

Unicode provides many different character properties. The most often needed ones
are available through these macros which are mapped to C functions depending on
the Python configuration.


.. c:function:: int Py_UNICODE_ISSPACE(Py_UCS4 ch)

   Return ``1`` or ``0`` depending on whether *ch* is a whitespace character.


.. c:function:: int Py_UNICODE_ISLOWER(Py_UCS4 ch)

   Return ``1`` or ``0`` depending on whether *ch* is a lowercase character.


.. c:function:: int Py_UNICODE_ISUPPER(Py_UCS4 ch)

   Return ``1`` or ``0`` depending on whether *ch* is an uppercase character.


.. c:function:: int Py_UNICODE_ISTITLE(Py_UCS4 ch)

   Return ``1`` or ``0`` depending on whether *ch* is a titlecase character.


.. c:function:: int Py_UNICODE_ISLINEBREAK(Py_UCS4 ch)

   Return ``1`` or ``0`` depending on whether *ch* is a linebreak character.


.. c:function:: int Py_UNICODE_ISDECIMAL(Py_UCS4 ch)

   Return ``1`` or ``0`` depending on whether *ch* is a decimal character.


.. c:function:: int Py_UNICODE_ISDIGIT(Py_UCS4 ch)

   Return ``1`` or ``0`` depending on whether *ch* is a digit character.


.. c:function:: int Py_UNICODE_ISNUMERIC(Py_UCS4 ch)

   Return ``1`` or ``0`` depending on whether *ch* is a numeric character.


.. c:function:: int Py_UNICODE_ISALPHA(Py_UCS4 ch)

   Return ``1`` or ``0`` depending on whether *ch* is an alphabetic character.


.. c:function:: int Py_UNICODE_ISALNUM(Py_UCS4 ch)

   Return ``1`` or ``0`` depending on whether *ch* is an alphanumeric character.


.. c:function:: int Py_UNICODE_ISPRINTABLE(Py_UCS4 ch)

   Return ``1`` or ``0`` depending on whether *ch* is a printable character.
   Nonprintable characters are those characters defined in the Unicode character
   database as "Other" or "Separator", excepting the ASCII space (0x20) which is
   considered printable.  (Note that printable characters in this context are
   those which should not be escaped when :func:`repr` is invoked on a string.
   It has no bearing on the handling of strings written to :data:`sys.stdout` or
   :data:`sys.stderr`.)


These APIs can be used for fast direct character conversions:


.. c:function:: Py_UCS4 Py_UNICODE_TOLOWER(Py_UCS4 ch)

   Return the character *ch* converted to lower case.


.. c:function:: Py_UCS4 Py_UNICODE_TOUPPER(Py_UCS4 ch)

   Return the character *ch* converted to upper case.


.. c:function:: Py_UCS4 Py_UNICODE_TOTITLE(Py_UCS4 ch)

   Return the character *ch* converted to title case.


.. c:function:: int Py_UNICODE_TODECIMAL(Py_UCS4 ch)

   Return the character *ch* converted to a decimal positive integer.  Return
   ``-1`` if this is not possible.  This function does not raise exceptions.


.. c:function:: int Py_UNICODE_TODIGIT(Py_UCS4 ch)

   Return the character *ch* converted to a single digit integer. Return ``-1`` if
   this is not possible.  This function does not raise exceptions.


.. c:function:: double Py_UNICODE_TONUMERIC(Py_UCS4 ch)

   Return the character *ch* converted to a double. Return ``-1.0`` if this is not
   possible.  This function does not raise exceptions.


These APIs can be used to work with surrogates:

.. c:function:: int Py_UNICODE_IS_SURROGATE(Py_UCS4 ch)

   Check if *ch* is a surrogate (``0xD800 <= ch <= 0xDFFF``).

.. c:function:: int Py_UNICODE_IS_HIGH_SURROGATE(Py_UCS4 ch)

   Check if *ch* is a high surrogate (``0xD800 <= ch <= 0xDBFF``).

.. c:function:: int Py_UNICODE_IS_LOW_SURROGATE(Py_UCS4 ch)

   Check if *ch* is a low surrogate (``0xDC00 <= ch <= 0xDFFF``).

.. c:function:: Py_UCS4 Py_UNICODE_JOIN_SURROGATES(Py_UCS4 high, Py_UCS4 low)

   Join two surrogate characters and return a single :c:type:`Py_UCS4` value.
   *high* and *low* are respectively the leading and trailing surrogates in a
   surrogate pair. *high* must be in the range [0xD800; 0xDBFF] and *low* must
   be in the range [0xDC00; 0xDFFF].


Creating and accessing Unicode strings
""""""""""""""""""""""""""""""""""""""

To create Unicode objects and access their basic sequence properties, use these
APIs:

.. c:function:: PyObject* PyUnicode_New(Py_ssize_t size, Py_UCS4 maxchar)

   Create a new Unicode object.  *maxchar* should be the true maximum code point
   to be placed in the string.  As an approximation, it can be rounded up to the
   nearest value in the sequence 127, 255, 65535, 1114111.

   This is the recommended way to allocate a new Unicode object.  Objects
   created using this function are not resizable.

   .. versionadded:: 3.3


.. c:function:: PyObject* PyUnicode_FromKindAndData(int kind, const void *buffer, \
                                                    Py_ssize_t size)

   Create a new Unicode object with the given *kind* (possible values are
   :c:macro:`PyUnicode_1BYTE_KIND` etc., as returned by
   :c:func:`PyUnicode_KIND`).  The *buffer* must point to an array of *size*
   units of 1, 2 or 4 bytes per character, as given by the kind.

   If necessary, the input *buffer* is copied and transformed into the
   canonical representation.  For example, if the *buffer* is a UCS4 string
   (:c:macro:`PyUnicode_4BYTE_KIND`) and it consists only of codepoints in
   the UCS1 range, it will be transformed into UCS1
   (:c:macro:`PyUnicode_1BYTE_KIND`).

   .. versionadded:: 3.3


.. c:function:: PyObject* PyUnicode_FromStringAndSize(const char *str, Py_ssize_t size)

   Create a Unicode object from the char buffer *str*.  The bytes will be
   interpreted as being UTF-8 encoded.  The buffer is copied into the new
   object.
   The return value might be a shared object, i.e. modification of the data is
   not allowed.

   This function raises :exc:`SystemError` when:

   * *size* < 0,
   * *str* is ``NULL`` and *size* > 0

   .. versionchanged:: 3.12
      *str* == ``NULL`` with *size* > 0 is not allowed anymore.


.. c:function:: PyObject *PyUnicode_FromString(const char *str)

   Create a Unicode object from a UTF-8 encoded null-terminated char buffer
   *str*.


.. c:function:: PyObject* PyUnicode_FromFormat(const char *format, ...)

   Take a C :c:func:`printf`\ -style *format* string and a variable number of
   arguments, calculate the size of the resulting Python Unicode string and return
   a string with the values formatted into it.  The variable arguments must be C
   types and must correspond exactly to the format characters in the *format*
   ASCII-encoded string.

   A conversion specifier contains two or more characters and has the following
   components, which must occur in this order:

   #. The ``'%'`` character, which marks the start of the specifier.

   #. Conversion flags (optional), which affect the result of some conversion
      types.

   #. Minimum field width (optional).
      If specified as an ``'*'`` (asterisk), the actual width is given in the
      next argument, which must be of type :c:expr:`int`, and the object to
      convert comes after the minimum field width and optional precision.

   #. Precision (optional), given as a ``'.'`` (dot) followed by the precision.
      If specified as ``'*'`` (an asterisk), the actual precision is given in
      the next argument, which must be of type :c:expr:`int`, and the value to
      convert comes after the precision.

   #. Length modifier (optional).

   #. Conversion type.

   The conversion flag characters are:

   .. tabularcolumns:: |l|L|

   +-------+-------------------------------------------------------------+
   | Flag  | Meaning                                                     |
   +=======+=============================================================+
   | ``0`` | The conversion will be zero padded for numeric values.      |
   +-------+-------------------------------------------------------------+
   | ``-`` | The converted value is left adjusted (overrides the ``0``   |
   |       | flag if both are given).                                    |
   +-------+-------------------------------------------------------------+

   The length modifiers for following integer conversions (``d``, ``i``,
   ``o``, ``u``, ``x``, or ``X``) specify the type of the argument
   (:c:expr:`int` by default):

   .. tabularcolumns:: |l|L|

   +----------+-----------------------------------------------------+
   | Modifier | Types                                               |
   +==========+=====================================================+
   | ``l``    | :c:expr:`long` or :c:expr:`unsigned long`           |
   +----------+-----------------------------------------------------+
   | ``ll``   | :c:expr:`long long` or :c:expr:`unsigned long long` |
   +----------+-----------------------------------------------------+
   | ``j``    | :c:type:`intmax_t` or :c:type:`uintmax_t`           |
   +----------+-----------------------------------------------------+
   | ``z``    | :c:type:`size_t` or :c:type:`ssize_t`               |
   +----------+-----------------------------------------------------+
   | ``t``    | :c:type:`ptrdiff_t`                                 |
   +----------+-----------------------------------------------------+

   The length modifier ``l`` for following conversions ``s`` or ``V`` specify
   that the type of the argument is :c:expr:`const wchar_t*`.

   The conversion specifiers are:

   .. list-table::
      :widths: auto
      :header-rows: 1

      * - Conversion Specifier
        - Type
        - Comment

      * - ``%``
        - *n/a*
        - The literal ``%`` character.

      * - ``d``, ``i``
        - Specified by the length modifier
        - The decimal representation of a signed C integer.

      * - ``u``
        - Specified by the length modifier
        - The decimal representation of an unsigned C integer.

      * - ``o``
        - Specified by the length modifier
        - The octal representation of an unsigned C integer.

      * - ``x``
        - Specified by the length modifier
        - The hexadecimal representation of an unsigned C integer (lowercase).

      * - ``X``
        - Specified by the length modifier
        - The hexadecimal representation of an unsigned C integer (uppercase).

      * - ``c``
        - :c:expr:`int`
        - A single character.

      * - ``s``
        - :c:expr:`const char*` or :c:expr:`const wchar_t*`
        - A null-terminated C character array.

      * - ``p``
        - :c:expr:`const void*`
        - The hex representation of a C  pointer.
          Mostly equivalent to ``printf("%p")`` except that it is guaranteed to
          start with the literal ``0x`` regardless of what the platform's
          ``printf`` yields.

      * - ``A``
        - :c:expr:`PyObject*`
        - The result of calling :func:`ascii`.

      * - ``U``
        - :c:expr:`PyObject*`
        - A Unicode object.

      * - ``V``
        - :c:expr:`PyObject*`, :c:expr:`const char*` or :c:expr:`const wchar_t*`
        - A Unicode object (which may be ``NULL``) and a null-terminated
          C character array as a second parameter (which will be used,
          if the first parameter is ``NULL``).

      * - ``S``
        - :c:expr:`PyObject*`
        - The result of calling :c:func:`PyObject_Str`.

      * - ``R``
        - :c:expr:`PyObject*`
        - The result of calling :c:func:`PyObject_Repr`.

   .. note::
      The width formatter unit is number of characters rather than bytes.
      The precision formatter unit is number of bytes or :c:type:`wchar_t`
      items (if the length modifier ``l`` is used) for ``"%s"`` and
      ``"%V"`` (if the ``PyObject*`` argument is ``NULL``), and a number of
      characters for ``"%A"``, ``"%U"``, ``"%S"``, ``"%R"`` and ``"%V"``
      (if the ``PyObject*`` argument is not ``NULL``).

   .. note::
      Unlike to C :c:func:`printf` the ``0`` flag has effect even when
      a precision is given for integer conversions (``d``, ``i``, ``u``, ``o``,
      ``x``, or ``X``).

   .. versionchanged:: 3.2
      Support for ``"%lld"`` and ``"%llu"`` added.

   .. versionchanged:: 3.3
      Support for ``"%li"``, ``"%lli"`` and ``"%zi"`` added.

   .. versionchanged:: 3.4
      Support width and precision formatter for ``"%s"``, ``"%A"``, ``"%U"``,
      ``"%V"``, ``"%S"``, ``"%R"`` added.

   .. versionchanged:: 3.12
      Support for conversion specifiers ``o`` and ``X``.
      Support for length modifiers ``j`` and ``t``.
      Length modifiers are now applied to all integer conversions.
      Length modifier ``l`` is now applied to conversion specifiers ``s`` and ``V``.
      Support for variable width and precision ``*``.
      Support for flag ``-``.

      An unrecognized format character now sets a :exc:`SystemError`.
      In previous versions it caused all the rest of the format string to be
      copied as-is to the result string, and any extra arguments discarded.


.. c:function:: PyObject* PyUnicode_FromFormatV(const char *format, va_list vargs)

   Identical to :c:func:`PyUnicode_FromFormat` except that it takes exactly two
   arguments.


.. c:function:: PyObject* PyUnicode_FromObject(PyObject *obj)

   Copy an instance of a Unicode subtype to a new true Unicode object if
   necessary. If *obj* is already a true Unicode object (not a subtype),
   return a new :term:`strong reference` to the object.

   Objects other than Unicode or its subtypes will cause a :exc:`TypeError`.


.. c:function:: PyObject* PyUnicode_FromEncodedObject(PyObject *obj, \
                               const char *encoding, const char *errors)

   Decode an encoded object *obj* to a Unicode object.

   :class:`bytes`, :class:`bytearray` and other
   :term:`bytes-like objects <bytes-like object>`
   are decoded according to the given *encoding* and using the error handling
   defined by *errors*. Both can be ``NULL`` to have the interface use the default
   values (see :ref:`builtincodecs` for details).

   All other objects, including Unicode objects, cause a :exc:`TypeError` to be
   set.

   The API returns ``NULL`` if there was an error.  The caller is responsible for
   decref'ing the returned objects.


.. c:function:: Py_ssize_t PyUnicode_GetLength(PyObject *unicode)

   Return the length of the Unicode object, in code points.

   .. versionadded:: 3.3


.. c:function:: Py_ssize_t PyUnicode_CopyCharacters(PyObject *to, \
                                                    Py_ssize_t to_start, \
                                                    PyObject *from, \
                                                    Py_ssize_t from_start, \
                                                    Py_ssize_t how_many)

   Copy characters from one Unicode object into another.  This function performs
   character conversion when necessary and falls back to :c:func:`!memcpy` if
   possible.  Returns ``-1`` and sets an exception on error, otherwise returns
   the number of copied characters.

   .. versionadded:: 3.3


.. c:function:: Py_ssize_t PyUnicode_Fill(PyObject *unicode, Py_ssize_t start, \
                        Py_ssize_t length, Py_UCS4 fill_char)

   Fill a string with a character: write *fill_char* into
   ``unicode[start:start+length]``.

   Fail if *fill_char* is bigger than the string maximum character, or if the
   string has more than 1 reference.

   Return the number of written character, or return ``-1`` and raise an
   exception on error.

   .. versionadded:: 3.3


.. c:function:: int PyUnicode_WriteChar(PyObject *unicode, Py_ssize_t index, \
                                        Py_UCS4 character)

   Write a character to a string.  The string must have been created through
   :c:func:`PyUnicode_New`.  Since Unicode strings are supposed to be immutable,
   the string must not be shared, or have been hashed yet.

   This function checks that *unicode* is a Unicode object, that the index is
   not out of bounds, and that the object can be modified safely (i.e. that it
   its reference count is one).

   .. versionadded:: 3.3


.. c:function:: Py_UCS4 PyUnicode_ReadChar(PyObject *unicode, Py_ssize_t index)

   Read a character from a string.  This function checks that *unicode* is a
   Unicode object and the index is not out of bounds, in contrast to
   :c:func:`PyUnicode_READ_CHAR`, which performs no error checking.

   .. versionadded:: 3.3


.. c:function:: PyObject* PyUnicode_Substring(PyObject *unicode, Py_ssize_t start, \
                                              Py_ssize_t end)

   Return a substring of *unicode*, from character index *start* (included) to
   character index *end* (excluded).  Negative indices are not supported.

   .. versionadded:: 3.3


.. c:function:: Py_UCS4* PyUnicode_AsUCS4(PyObject *unicode, Py_UCS4 *buffer, \
                                          Py_ssize_t buflen, int copy_null)

   Copy the string *unicode* into a UCS4 buffer, including a null character, if
   *copy_null* is set.  Returns ``NULL`` and sets an exception on error (in
   particular, a :exc:`SystemError` if *buflen* is smaller than the length of
   *unicode*).  *buffer* is returned on success.

   .. versionadded:: 3.3


.. c:function:: Py_UCS4* PyUnicode_AsUCS4Copy(PyObject *unicode)

   Copy the string *unicode* into a new UCS4 buffer that is allocated using
   :c:func:`PyMem_Malloc`.  If this fails, ``NULL`` is returned with a
   :exc:`MemoryError` set.  The returned buffer always has an extra
   null code point appended.

   .. versionadded:: 3.3


Locale Encoding
"""""""""""""""

The current locale encoding can be used to decode text from the operating
system.

.. c:function:: PyObject* PyUnicode_DecodeLocaleAndSize(const char *str, \
                                                        Py_ssize_t length, \
                                                        const char *errors)

   Decode a string from UTF-8 on Android and VxWorks, or from the current
   locale encoding on other platforms. The supported
   error handlers are ``"strict"`` and ``"surrogateescape"``
   (:pep:`383`). The decoder uses ``"strict"`` error handler if
   *errors* is ``NULL``.  *str* must end with a null character but
   cannot contain embedded null characters.

   Use :c:func:`PyUnicode_DecodeFSDefaultAndSize` to decode a string from
   the :term:`filesystem encoding and error handler`.

   This function ignores the :ref:`Python UTF-8 Mode <utf8-mode>`.

   .. seealso::

      The :c:func:`Py_DecodeLocale` function.

   .. versionadded:: 3.3

   .. versionchanged:: 3.7
      The function now also uses the current locale encoding for the
      ``surrogateescape`` error handler, except on Android. Previously, :c:func:`Py_DecodeLocale`
      was used for the ``surrogateescape``, and the current locale encoding was
      used for ``strict``.


.. c:function:: PyObject* PyUnicode_DecodeLocale(const char *str, const char *errors)

   Similar to :c:func:`PyUnicode_DecodeLocaleAndSize`, but compute the string
   length using :c:func:`!strlen`.

   .. versionadded:: 3.3


.. c:function:: PyObject* PyUnicode_EncodeLocale(PyObject *unicode, const char *errors)

   Encode a Unicode object to UTF-8 on Android and VxWorks, or to the current
   locale encoding on other platforms. The
   supported error handlers are ``"strict"`` and ``"surrogateescape"``
   (:pep:`383`). The encoder uses ``"strict"`` error handler if
   *errors* is ``NULL``. Return a :class:`bytes` object. *unicode* cannot
   contain embedded null characters.

   Use :c:func:`PyUnicode_EncodeFSDefault` to encode a string to the
   :term:`filesystem encoding and error handler`.

   This function ignores the :ref:`Python UTF-8 Mode <utf8-mode>`.

   .. seealso::

      The :c:func:`Py_EncodeLocale` function.

   .. versionadded:: 3.3

   .. versionchanged:: 3.7
      The function now also uses the current locale encoding for the
      ``surrogateescape`` error handler, except on Android. Previously,
      :c:func:`Py_EncodeLocale`
      was used for the ``surrogateescape``, and the current locale encoding was
      used for ``strict``.


File System Encoding
""""""""""""""""""""

Functions encoding to and decoding from the :term:`filesystem encoding and
error handler` (:pep:`383` and :pep:`529`).

To encode file names to :class:`bytes` during argument parsing, the ``"O&"``
converter should be used, passing :c:func:`PyUnicode_FSConverter` as the
conversion function:

.. c:function:: int PyUnicode_FSConverter(PyObject* obj, void* result)

   ParseTuple converter: encode :class:`str` objects -- obtained directly or
   through the :class:`os.PathLike` interface -- to :class:`bytes` using
   :c:func:`PyUnicode_EncodeFSDefault`; :class:`bytes` objects are output as-is.
   *result* must be a :c:expr:`PyBytesObject*` which must be released when it is
   no longer used.

   .. versionadded:: 3.1

   .. versionchanged:: 3.6
      Accepts a :term:`path-like object`.

To decode file names to :class:`str` during argument parsing, the ``"O&"``
converter should be used, passing :c:func:`PyUnicode_FSDecoder` as the
conversion function:

.. c:function:: int PyUnicode_FSDecoder(PyObject* obj, void* result)

   ParseTuple converter: decode :class:`bytes` objects -- obtained either
   directly or indirectly through the :class:`os.PathLike` interface -- to
   :class:`str` using :c:func:`PyUnicode_DecodeFSDefaultAndSize`; :class:`str`
   objects are output as-is. *result* must be a :c:expr:`PyUnicodeObject*` which
   must be released when it is no longer used.

   .. versionadded:: 3.2

   .. versionchanged:: 3.6
      Accepts a :term:`path-like object`.


.. c:function:: PyObject* PyUnicode_DecodeFSDefaultAndSize(const char *str, Py_ssize_t size)

   Decode a string from the :term:`filesystem encoding and error handler`.

   If you need to decode a string from the current locale encoding, use
   :c:func:`PyUnicode_DecodeLocaleAndSize`.

   .. seealso::

      The :c:func:`Py_DecodeLocale` function.

   .. versionchanged:: 3.6
      The :term:`filesystem error handler <filesystem encoding and error
      handler>` is now used.


.. c:function:: PyObject* PyUnicode_DecodeFSDefault(const char *str)

   Decode a null-terminated string from the :term:`filesystem encoding and
   error handler`.

   If the string length is known, use
   :c:func:`PyUnicode_DecodeFSDefaultAndSize`.

   .. versionchanged:: 3.6
      The :term:`filesystem error handler <filesystem encoding and error
      handler>` is now used.


.. c:function:: PyObject* PyUnicode_EncodeFSDefault(PyObject *unicode)

   Encode a Unicode object to the :term:`filesystem encoding and error
   handler`, and return :class:`bytes`. Note that the resulting :class:`bytes`
   object can contain null bytes.

   If you need to encode a string to the current locale encoding, use
   :c:func:`PyUnicode_EncodeLocale`.

   .. seealso::

      The :c:func:`Py_EncodeLocale` function.

   .. versionadded:: 3.2

   .. versionchanged:: 3.6
      The :term:`filesystem error handler <filesystem encoding and error
      handler>` is now used.

wchar_t Support
"""""""""""""""

:c:type:`wchar_t` support for platforms which support it:

.. c:function:: PyObject* PyUnicode_FromWideChar(const wchar_t *wstr, Py_ssize_t size)

   Create a Unicode object from the :c:type:`wchar_t` buffer *wstr* of the given *size*.
   Passing ``-1`` as the *size* indicates that the function must itself compute the length,
   using :c:func:`!wcslen`.
   Return ``NULL`` on failure.


.. c:function:: Py_ssize_t PyUnicode_AsWideChar(PyObject *unicode, wchar_t *wstr, Py_ssize_t size)

   Copy the Unicode object contents into the :c:type:`wchar_t` buffer *wstr*.  At most
   *size* :c:type:`wchar_t` characters are copied (excluding a possibly trailing
   null termination character).  Return the number of :c:type:`wchar_t` characters
   copied or ``-1`` in case of an error.  Note that the resulting :c:expr:`wchar_t*`
   string may or may not be null-terminated.  It is the responsibility of the caller
   to make sure that the :c:expr:`wchar_t*` string is null-terminated in case this is
   required by the application. Also, note that the :c:expr:`wchar_t*` string
   might contain null characters, which would cause the string to be truncated
   when used with most C functions.


.. c:function:: wchar_t* PyUnicode_AsWideCharString(PyObject *unicode, Py_ssize_t *size)

   Convert the Unicode object to a wide character string. The output string
   always ends with a null character. If *size* is not ``NULL``, write the number
   of wide characters (excluding the trailing null termination character) into
   *\*size*. Note that the resulting :c:type:`wchar_t` string might contain
   null characters, which would cause the string to be truncated when used with
   most C functions. If *size* is ``NULL`` and the :c:expr:`wchar_t*` string
   contains null characters a :exc:`ValueError` is raised.

   Returns a buffer allocated by :c:macro:`PyMem_New` (use
   :c:func:`PyMem_Free` to free it) on success. On error, returns ``NULL``
   and *\*size* is undefined. Raises a :exc:`MemoryError` if memory allocation
   is failed.

   .. versionadded:: 3.2

   .. versionchanged:: 3.7
      Raises a :exc:`ValueError` if *size* is ``NULL`` and the :c:expr:`wchar_t*`
      string contains null characters.


.. _builtincodecs:

Built-in Codecs
^^^^^^^^^^^^^^^

Python provides a set of built-in codecs which are written in C for speed. All of
these codecs are directly usable via the following functions.

Many of the following APIs take two arguments encoding and errors, and they
have the same semantics as the ones of the built-in :func:`str` string object
constructor.

Setting encoding to ``NULL`` causes the default encoding to be used
which is UTF-8.  The file system calls should use
:c:func:`PyUnicode_FSConverter` for encoding file names. This uses the
:term:`filesystem encoding and error handler` internally.

Error handling is set by errors which may also be set to ``NULL`` meaning to use
the default handling defined for the codec.  Default error handling for all
built-in codecs is "strict" (:exc:`ValueError` is raised).

The codecs all use a similar interface.  Only deviations from the following
generic ones are documented for simplicity.


Generic Codecs
""""""""""""""

These are the generic codec APIs:


.. c:function:: PyObject* PyUnicode_Decode(const char *str, Py_ssize_t size, \
                              const char *encoding, const char *errors)

   Create a Unicode object by decoding *size* bytes of the encoded string *str*.
   *encoding* and *errors* have the same meaning as the parameters of the same name
   in the :func:`str` built-in function.  The codec to be used is looked up
   using the Python codec registry.  Return ``NULL`` if an exception was raised by
   the codec.


.. c:function:: PyObject* PyUnicode_AsEncodedString(PyObject *unicode, \
                              const char *encoding, const char *errors)

   Encode a Unicode object and return the result as Python bytes object.
   *encoding* and *errors* have the same meaning as the parameters of the same
   name in the Unicode :meth:`~str.encode` method. The codec to be used is looked up
   using the Python codec registry. Return ``NULL`` if an exception was raised by
   the codec.


UTF-8 Codecs
""""""""""""

These are the UTF-8 codec APIs:


.. c:function:: PyObject* PyUnicode_DecodeUTF8(const char *str, Py_ssize_t size, const char *errors)

   Create a Unicode object by decoding *size* bytes of the UTF-8 encoded string
   *str*. Return ``NULL`` if an exception was raised by the codec.


.. c:function:: PyObject* PyUnicode_DecodeUTF8Stateful(const char *str, Py_ssize_t size, \
                              const char *errors, Py_ssize_t *consumed)

   If *consumed* is ``NULL``, behave like :c:func:`PyUnicode_DecodeUTF8`. If
   *consumed* is not ``NULL``, trailing incomplete UTF-8 byte sequences will not be
   treated as an error. Those bytes will not be decoded and the number of bytes
   that have been decoded will be stored in *consumed*.


.. c:function:: PyObject* PyUnicode_AsUTF8String(PyObject *unicode)

   Encode a Unicode object using UTF-8 and return the result as Python bytes
   object.  Error handling is "strict".  Return ``NULL`` if an exception was
   raised by the codec.


.. c:function:: const char* PyUnicode_AsUTF8AndSize(PyObject *unicode, Py_ssize_t *size)

   Return a pointer to the UTF-8 encoding of the Unicode object, and
   store the size of the encoded representation (in bytes) in *size*.  The
   *size* argument can be ``NULL``; in this case no size will be stored.  The
   returned buffer always has an extra null byte appended (not included in
   *size*), regardless of whether there are any other null code points.

   On error, set an exception, set *size* to ``-1`` (if it's not NULL) and
   return ``NULL``.

   This caches the UTF-8 representation of the string in the Unicode object, and
   subsequent calls will return a pointer to the same buffer.  The caller is not
   responsible for deallocating the buffer. The buffer is deallocated and
   pointers to it become invalid when the Unicode object is garbage collected.

   .. versionadded:: 3.3

   .. versionchanged:: 3.7
      The return type is now ``const char *`` rather of ``char *``.

   .. versionchanged:: 3.10
      This function is a part of the :ref:`limited API <limited-c-api>`.


.. c:function:: const char* PyUnicode_AsUTF8(PyObject *unicode)

   As :c:func:`PyUnicode_AsUTF8AndSize`, but does not store the size.

   .. versionadded:: 3.3

   .. versionchanged:: 3.7
      The return type is now ``const char *`` rather of ``char *``.


UTF-32 Codecs
"""""""""""""

These are the UTF-32 codec APIs:


.. c:function:: PyObject* PyUnicode_DecodeUTF32(const char *str, Py_ssize_t size, \
                              const char *errors, int *byteorder)

   Decode *size* bytes from a UTF-32 encoded buffer string and return the
   corresponding Unicode object.  *errors* (if non-``NULL``) defines the error
   handling. It defaults to "strict".

   If *byteorder* is non-``NULL``, the decoder starts decoding using the given byte
   order::

      *byteorder == -1: little endian
      *byteorder == 0:  native order
      *byteorder == 1:  big endian

   If ``*byteorder`` is zero, and the first four bytes of the input data are a
   byte order mark (BOM), the decoder switches to this byte order and the BOM is
   not copied into the resulting Unicode string.  If ``*byteorder`` is ``-1`` or
   ``1``, any byte order mark is copied to the output.

   After completion, *\*byteorder* is set to the current byte order at the end
   of input data.

   If *byteorder* is ``NULL``, the codec starts in native order mode.

   Return ``NULL`` if an exception was raised by the codec.


.. c:function:: PyObject* PyUnicode_DecodeUTF32Stateful(const char *str, Py_ssize_t size, \
                              const char *errors, int *byteorder, Py_ssize_t *consumed)

   If *consumed* is ``NULL``, behave like :c:func:`PyUnicode_DecodeUTF32`. If
   *consumed* is not ``NULL``, :c:func:`PyUnicode_DecodeUTF32Stateful` will not treat
   trailing incomplete UTF-32 byte sequences (such as a number of bytes not divisible
   by four) as an error. Those bytes will not be decoded and the number of bytes
   that have been decoded will be stored in *consumed*.


.. c:function:: PyObject* PyUnicode_AsUTF32String(PyObject *unicode)

   Return a Python byte string using the UTF-32 encoding in native byte
   order. The string always starts with a BOM mark.  Error handling is "strict".
   Return ``NULL`` if an exception was raised by the codec.


UTF-16 Codecs
"""""""""""""

These are the UTF-16 codec APIs:


.. c:function:: PyObject* PyUnicode_DecodeUTF16(const char *str, Py_ssize_t size, \
                              const char *errors, int *byteorder)

   Decode *size* bytes from a UTF-16 encoded buffer string and return the
   corresponding Unicode object.  *errors* (if non-``NULL``) defines the error
   handling. It defaults to "strict".

   If *byteorder* is non-``NULL``, the decoder starts decoding using the given byte
   order::

      *byteorder == -1: little endian
      *byteorder == 0:  native order
      *byteorder == 1:  big endian

   If ``*byteorder`` is zero, and the first two bytes of the input data are a
   byte order mark (BOM), the decoder switches to this byte order and the BOM is
   not copied into the resulting Unicode string.  If ``*byteorder`` is ``-1`` or
   ``1``, any byte order mark is copied to the output (where it will result in
   either a ``\ufeff`` or a ``\ufffe`` character).

   After completion, ``*byteorder`` is set to the current byte order at the end
   of input data.

   If *byteorder* is ``NULL``, the codec starts in native order mode.

   Return ``NULL`` if an exception was raised by the codec.


.. c:function:: PyObject* PyUnicode_DecodeUTF16Stateful(const char *str, Py_ssize_t size, \
                              const char *errors, int *byteorder, Py_ssize_t *consumed)

   If *consumed* is ``NULL``, behave like :c:func:`PyUnicode_DecodeUTF16`. If
   *consumed* is not ``NULL``, :c:func:`PyUnicode_DecodeUTF16Stateful` will not treat
   trailing incomplete UTF-16 byte sequences (such as an odd number of bytes or a
   split surrogate pair) as an error. Those bytes will not be decoded and the
   number of bytes that have been decoded will be stored in *consumed*.


.. c:function:: PyObject* PyUnicode_AsUTF16String(PyObject *unicode)

   Return a Python byte string using the UTF-16 encoding in native byte
   order. The string always starts with a BOM mark.  Error handling is "strict".
   Return ``NULL`` if an exception was raised by the codec.


UTF-7 Codecs
""""""""""""

These are the UTF-7 codec APIs:


.. c:function:: PyObject* PyUnicode_DecodeUTF7(const char *str, Py_ssize_t size, const char *errors)

   Create a Unicode object by decoding *size* bytes of the UTF-7 encoded string
   *str*.  Return ``NULL`` if an exception was raised by the codec.


.. c:function:: PyObject* PyUnicode_DecodeUTF7Stateful(const char *str, Py_ssize_t size, \
                              const char *errors, Py_ssize_t *consumed)

   If *consumed* is ``NULL``, behave like :c:func:`PyUnicode_DecodeUTF7`.  If
   *consumed* is not ``NULL``, trailing incomplete UTF-7 base-64 sections will not
   be treated as an error.  Those bytes will not be decoded and the number of
   bytes that have been decoded will be stored in *consumed*.


Unicode-Escape Codecs
"""""""""""""""""""""

These are the "Unicode Escape" codec APIs:


.. c:function:: PyObject* PyUnicode_DecodeUnicodeEscape(const char *str, \
                              Py_ssize_t size, const char *errors)

   Create a Unicode object by decoding *size* bytes of the Unicode-Escape encoded
   string *str*.  Return ``NULL`` if an exception was raised by the codec.


.. c:function:: PyObject* PyUnicode_AsUnicodeEscapeString(PyObject *unicode)

   Encode a Unicode object using Unicode-Escape and return the result as a
   bytes object.  Error handling is "strict".  Return ``NULL`` if an exception was
   raised by the codec.


Raw-Unicode-Escape Codecs
"""""""""""""""""""""""""

These are the "Raw Unicode Escape" codec APIs:


.. c:function:: PyObject* PyUnicode_DecodeRawUnicodeEscape(const char *str, \
                              Py_ssize_t size, const char *errors)

   Create a Unicode object by decoding *size* bytes of the Raw-Unicode-Escape
   encoded string *str*.  Return ``NULL`` if an exception was raised by the codec.


.. c:function:: PyObject* PyUnicode_AsRawUnicodeEscapeString(PyObject *unicode)

   Encode a Unicode object using Raw-Unicode-Escape and return the result as
   a bytes object.  Error handling is "strict".  Return ``NULL`` if an exception
   was raised by the codec.


Latin-1 Codecs
""""""""""""""

These are the Latin-1 codec APIs: Latin-1 corresponds to the first 256 Unicode
ordinals and only these are accepted by the codecs during encoding.


.. c:function:: PyObject* PyUnicode_DecodeLatin1(const char *str, Py_ssize_t size, const char *errors)

   Create a Unicode object by decoding *size* bytes of the Latin-1 encoded string
   *str*.  Return ``NULL`` if an exception was raised by the codec.


.. c:function:: PyObject* PyUnicode_AsLatin1String(PyObject *unicode)

   Encode a Unicode object using Latin-1 and return the result as Python bytes
   object.  Error handling is "strict".  Return ``NULL`` if an exception was
   raised by the codec.


ASCII Codecs
""""""""""""

These are the ASCII codec APIs.  Only 7-bit ASCII data is accepted. All other
codes generate errors.


.. c:function:: PyObject* PyUnicode_DecodeASCII(const char *str, Py_ssize_t size, const char *errors)

   Create a Unicode object by decoding *size* bytes of the ASCII encoded string
   *str*.  Return ``NULL`` if an exception was raised by the codec.


.. c:function:: PyObject* PyUnicode_AsASCIIString(PyObject *unicode)

   Encode a Unicode object using ASCII and return the result as Python bytes
   object.  Error handling is "strict".  Return ``NULL`` if an exception was
   raised by the codec.


Character Map Codecs
""""""""""""""""""""

This codec is special in that it can be used to implement many different codecs
(and this is in fact what was done to obtain most of the standard codecs
included in the :mod:`!encodings` package). The codec uses mappings to encode and
decode characters.  The mapping objects provided must support the
:meth:`~object.__getitem__` mapping interface; dictionaries and sequences work well.

These are the mapping codec APIs:

.. c:function:: PyObject* PyUnicode_DecodeCharmap(const char *str, Py_ssize_t length, \
                              PyObject *mapping, const char *errors)

   Create a Unicode object by decoding *size* bytes of the encoded string *str*
   using the given *mapping* object.  Return ``NULL`` if an exception was raised
   by the codec.

   If *mapping* is ``NULL``, Latin-1 decoding will be applied.  Else
   *mapping* must map bytes ordinals (integers in the range from 0 to 255)
   to Unicode strings, integers (which are then interpreted as Unicode
   ordinals) or ``None``.  Unmapped data bytes -- ones which cause a
   :exc:`LookupError`, as well as ones which get mapped to ``None``,
   ``0xFFFE`` or ``'\ufffe'``, are treated as undefined mappings and cause
   an error.


.. c:function:: PyObject* PyUnicode_AsCharmapString(PyObject *unicode, PyObject *mapping)

   Encode a Unicode object using the given *mapping* object and return the
   result as a bytes object.  Error handling is "strict".  Return ``NULL`` if an
   exception was raised by the codec.

   The *mapping* object must map Unicode ordinal integers to bytes objects,
   integers in the range from 0 to 255 or ``None``.  Unmapped character
   ordinals (ones which cause a :exc:`LookupError`) as well as mapped to
   ``None`` are treated as "undefined mapping" and cause an error.


The following codec API is special in that maps Unicode to Unicode.

.. c:function:: PyObject* PyUnicode_Translate(PyObject *unicode, PyObject *table, const char *errors)

   Translate a string by applying a character mapping table to it and return the
   resulting Unicode object. Return ``NULL`` if an exception was raised by the
   codec.

   The mapping table must map Unicode ordinal integers to Unicode ordinal integers
   or ``None`` (causing deletion of the character).

   Mapping tables need only provide the :meth:`~object.__getitem__` interface; dictionaries
   and sequences work well.  Unmapped character ordinals (ones which cause a
   :exc:`LookupError`) are left untouched and are copied as-is.

   *errors* has the usual meaning for codecs. It may be ``NULL`` which indicates to
   use the default error handling.


MBCS codecs for Windows
"""""""""""""""""""""""

These are the MBCS codec APIs. They are currently only available on Windows and
use the Win32 MBCS converters to implement the conversions.  Note that MBCS (or
DBCS) is a class of encodings, not just one.  The target encoding is defined by
the user settings on the machine running the codec.

.. c:function:: PyObject* PyUnicode_DecodeMBCS(const char *str, Py_ssize_t size, const char *errors)

   Create a Unicode object by decoding *size* bytes of the MBCS encoded string *str*.
   Return ``NULL`` if an exception was raised by the codec.


.. c:function:: PyObject* PyUnicode_DecodeMBCSStateful(const char *str, Py_ssize_t size, \
                              const char *errors, Py_ssize_t *consumed)

   If *consumed* is ``NULL``, behave like :c:func:`PyUnicode_DecodeMBCS`. If
   *consumed* is not ``NULL``, :c:func:`PyUnicode_DecodeMBCSStateful` will not decode
   trailing lead byte and the number of bytes that have been decoded will be stored
   in *consumed*.


.. c:function:: PyObject* PyUnicode_AsMBCSString(PyObject *unicode)

   Encode a Unicode object using MBCS and return the result as Python bytes
   object.  Error handling is "strict".  Return ``NULL`` if an exception was
   raised by the codec.


.. c:function:: PyObject* PyUnicode_EncodeCodePage(int code_page, PyObject *unicode, const char *errors)

   Encode the Unicode object using the specified code page and return a Python
   bytes object.  Return ``NULL`` if an exception was raised by the codec. Use
   :c:macro:`!CP_ACP` code page to get the MBCS encoder.

   .. versionadded:: 3.3


Methods & Slots
"""""""""""""""


.. _unicodemethodsandslots:

Methods and Slot Functions
^^^^^^^^^^^^^^^^^^^^^^^^^^

The following APIs are capable of handling Unicode objects and strings on input
(we refer to them as strings in the descriptions) and return Unicode objects or
integers as appropriate.

They all return ``NULL`` or ``-1`` if an exception occurs.


.. c:function:: PyObject* PyUnicode_Concat(PyObject *left, PyObject *right)

   Concat two strings giving a new Unicode string.


.. c:function:: PyObject* PyUnicode_Split(PyObject *unicode, PyObject *sep, Py_ssize_t maxsplit)

   Split a string giving a list of Unicode strings.  If *sep* is ``NULL``, splitting
   will be done at all whitespace substrings.  Otherwise, splits occur at the given
   separator.  At most *maxsplit* splits will be done.  If negative, no limit is
   set.  Separators are not included in the resulting list.


.. c:function:: PyObject* PyUnicode_Splitlines(PyObject *unicode, int keepends)

   Split a Unicode string at line breaks, returning a list of Unicode strings.
   CRLF is considered to be one line break.  If *keepends* is ``0``, the Line break
   characters are not included in the resulting strings.


.. c:function:: PyObject* PyUnicode_Join(PyObject *separator, PyObject *seq)

   Join a sequence of strings using the given *separator* and return the resulting
   Unicode string.


.. c:function:: Py_ssize_t PyUnicode_Tailmatch(PyObject *unicode, PyObject *substr, \
                        Py_ssize_t start, Py_ssize_t end, int direction)

   Return ``1`` if *substr* matches ``unicode[start:end]`` at the given tail end
   (*direction* == ``-1`` means to do a prefix match, *direction* == ``1`` a suffix match),
   ``0`` otherwise. Return ``-1`` if an error occurred.


.. c:function:: Py_ssize_t PyUnicode_Find(PyObject *unicode, PyObject *substr, \
                               Py_ssize_t start, Py_ssize_t end, int direction)

   Return the first position of *substr* in ``unicode[start:end]`` using the given
   *direction* (*direction* == ``1`` means to do a forward search, *direction* == ``-1`` a
   backward search).  The return value is the index of the first match; a value of
   ``-1`` indicates that no match was found, and ``-2`` indicates that an error
   occurred and an exception has been set.


.. c:function:: Py_ssize_t PyUnicode_FindChar(PyObject *unicode, Py_UCS4 ch, \
                               Py_ssize_t start, Py_ssize_t end, int direction)

   Return the first position of the character *ch* in ``unicode[start:end]`` using
   the given *direction* (*direction* == ``1`` means to do a forward search,
   *direction* == ``-1`` a backward search).  The return value is the index of the
   first match; a value of ``-1`` indicates that no match was found, and ``-2``
   indicates that an error occurred and an exception has been set.

   .. versionadded:: 3.3

   .. versionchanged:: 3.7
      *start* and *end* are now adjusted to behave like ``unicode[start:end]``.


.. c:function:: Py_ssize_t PyUnicode_Count(PyObject *unicode, PyObject *substr, \
                               Py_ssize_t start, Py_ssize_t end)

   Return the number of non-overlapping occurrences of *substr* in
   ``unicode[start:end]``.  Return ``-1`` if an error occurred.


.. c:function:: PyObject* PyUnicode_Replace(PyObject *unicode, PyObject *substr, \
                              PyObject *replstr, Py_ssize_t maxcount)

   Replace at most *maxcount* occurrences of *substr* in *unicode* with *replstr* and
   return the resulting Unicode object. *maxcount* == ``-1`` means replace all
   occurrences.


.. c:function:: int PyUnicode_Compare(PyObject *left, PyObject *right)

   Compare two strings and return ``-1``, ``0``, ``1`` for less than, equal, and greater than,
   respectively.

   This function returns ``-1`` upon failure, so one should call
   :c:func:`PyErr_Occurred` to check for errors.


.. c:function:: int PyUnicode_EqualToUTF8AndSize(PyObject *unicode, const char *string, Py_ssize_t size)

   Compare a Unicode object with a char buffer which is interpreted as
   being UTF-8 or ASCII encoded and return true (``1``) if they are equal,
   or false (``0``) otherwise.
   If the Unicode object contains surrogate characters or
   the C string is not valid UTF-8, false (``0``) is returned.

   This function does not raise exceptions.

   .. versionadded:: 3.13


.. c:function:: int PyUnicode_EqualToUTF8(PyObject *unicode, const char *string)

   Similar to :c:func:`PyUnicode_EqualToUTF8AndSize`, but compute *string*
   length using :c:func:`!strlen`.
   If the Unicode object contains null characters, false (``0``) is returned.

   .. versionadded:: 3.13


.. c:function:: int PyUnicode_CompareWithASCIIString(PyObject *unicode, const char *string)

   Compare a Unicode object, *unicode*, with *string* and return ``-1``, ``0``, ``1`` for less
   than, equal, and greater than, respectively. It is best to pass only
   ASCII-encoded strings, but the function interprets the input string as
   ISO-8859-1 if it contains non-ASCII characters.

   This function does not raise exceptions.


.. c:function:: PyObject* PyUnicode_RichCompare(PyObject *left,  PyObject *right, int op)

   Rich compare two Unicode strings and return one of the following:

   * ``NULL`` in case an exception was raised
   * :c:data:`Py_True` or :c:data:`Py_False` for successful comparisons
   * :c:data:`Py_NotImplemented` in case the type combination is unknown

   Possible values for *op* are :c:macro:`Py_GT`, :c:macro:`Py_GE`, :c:macro:`Py_EQ`,
   :c:macro:`Py_NE`, :c:macro:`Py_LT`, and :c:macro:`Py_LE`.


.. c:function:: PyObject* PyUnicode_Format(PyObject *format, PyObject *args)

   Return a new string object from *format* and *args*; this is analogous to
   ``format % args``.


.. c:function:: int PyUnicode_Contains(PyObject *unicode, PyObject *substr)

   Check whether *substr* is contained in *unicode* and return true or false
   accordingly.

   *substr* has to coerce to a one element Unicode string. ``-1`` is returned
   if there was an error.


.. c:function:: void PyUnicode_InternInPlace(PyObject **p_unicode)

   Intern the argument :c:expr:`*p_unicode` in place.  The argument must be the address of a
   pointer variable pointing to a Python Unicode string object.  If there is an
   existing interned string that is the same as :c:expr:`*p_unicode`, it sets :c:expr:`*p_unicode` to
   it (releasing the reference to the old string object and creating a new
   :term:`strong reference` to the interned string object), otherwise it leaves
   :c:expr:`*p_unicode` alone and interns it (creating a new :term:`strong reference`).
   (Clarification: even though there is a lot of talk about references, think
   of this function as reference-neutral; you own the object after the call
   if and only if you owned it before the call.)


.. c:function:: PyObject* PyUnicode_InternFromString(const char *str)

   A combination of :c:func:`PyUnicode_FromString` and
   :c:func:`PyUnicode_InternInPlace`, returning either a new Unicode string
   object that has been interned, or a new ("owned") reference to an earlier
   interned string object with the same value.
