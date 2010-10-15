:mod:`json` --- JSON encoder and decoder
========================================

.. module:: json
   :synopsis: Encode and decode the JSON format.
.. moduleauthor:: Bob Ippolito <bob@redivi.com>
.. sectionauthor:: Bob Ippolito <bob@redivi.com>

JSON (JavaScript Object Notation) <http://json.org> is a subset of JavaScript
syntax (ECMA-262 3rd edition) used as a lightweight data interchange format.

:mod:`json` exposes an API familiar to users of the standard library
:mod:`marshal` and :mod:`pickle` modules.

Encoding basic Python object hierarchies::

    >>> import json
    >>> json.dumps(['foo', {'bar': ('baz', None, 1.0, 2)}])
    '["foo", {"bar": ["baz", null, 1.0, 2]}]'
    >>> print(json.dumps("\"foo\bar"))
    "\"foo\bar"
    >>> print(json.dumps('\u1234'))
    "\u1234"
    >>> print(json.dumps('\\'))
    "\\"
    >>> print(json.dumps({"c": 0, "b": 0, "a": 0}, sort_keys=True))
    {"a": 0, "b": 0, "c": 0}
    >>> from io import StringIO
    >>> io = StringIO()
    >>> json.dump(['streaming API'], io)
    >>> io.getvalue()
    '["streaming API"]'

Compact encoding::

    >>> import json
    >>> json.dumps([1,2,3,{'4': 5, '6': 7}], separators=(',',':'))
    '[1,2,3,{"4":5,"6":7}]'

Pretty printing::

    >>> import json
    >>> print(json.dumps({'4': 5, '6': 7}, sort_keys=True, indent=4))
    {
        "4": 5,
        "6": 7
    }

Decoding JSON::

    >>> import json
    >>> json.loads('["foo", {"bar":["baz", null, 1.0, 2]}]')
    ['foo', {'bar': ['baz', None, 1.0, 2]}]
    >>> json.loads('"\\"foo\\bar"')
    '"foo\x08ar'
    >>> from io import StringIO
    >>> io = StringIO('["streaming API"]')
    >>> json.load(io)
    ['streaming API']

Specializing JSON object decoding::

    >>> import json
    >>> def as_complex(dct):
    ...     if '__complex__' in dct:
    ...         return complex(dct['real'], dct['imag'])
    ...     return dct
    ...
    >>> json.loads('{"__complex__": true, "real": 1, "imag": 2}',
    ...     object_hook=as_complex)
    (1+2j)
    >>> import decimal
    >>> json.loads('1.1', parse_float=decimal.Decimal)
    Decimal('1.1')

Extending :class:`JSONEncoder`::

    >>> import json
    >>> class ComplexEncoder(json.JSONEncoder):
    ...     def default(self, obj):
    ...         if isinstance(obj, complex):
    ...             return [obj.real, obj.imag]
    ...         return json.JSONEncoder.default(self, obj)
    ...
    >>> json.dumps(2 + 1j, cls=ComplexEncoder)
    '[2.0, 1.0]'
    >>> ComplexEncoder().encode(2 + 1j)
    '[2.0, 1.0]'
    >>> list(ComplexEncoder().iterencode(2 + 1j))
    ['[2.0', ', 1.0', ']']


.. highlight:: none

Using json.tool from the shell to validate and pretty-print::

    $ echo '{"json":"obj"}' | python -mjson.tool
    {
        "json": "obj"
    }
    $ echo '{ 1.2:3.4}' | python -mjson.tool
    Expecting property name: line 1 column 2 (char 2)

.. highlight:: python

.. note::

   The JSON produced by this module's default settings is a subset of
   YAML, so it may be used as a serializer for that as well.


Basic Usage
-----------

.. function:: dump(obj, fp, skipkeys=False, ensure_ascii=True, check_circular=True, allow_nan=True, cls=None, indent=None, separators=None, default=None, **kw)

   Serialize *obj* as a JSON formatted stream to *fp* (a ``.write()``-supporting
   file-like object).

   If *skipkeys* is ``True`` (default: ``False``), then dict keys that are not
   of a basic type (:class:`bytes`, :class:`str`, :class:`int`,
   :class:`float`, :class:`bool`, ``None``) will be skipped instead of raising a
   :exc:`TypeError`.

   The :mod:`json` module always produces :class:`str` objects, not
   :class:`bytes` objects. Therefore, ``fp.write()`` must support :class:`str`
   input.

   If *check_circular* is ``False`` (default: ``True``), then the circular
   reference check for container types will be skipped and a circular reference
   will result in an :exc:`OverflowError` (or worse).

   If *allow_nan* is ``False`` (default: ``True``), then it will be a
   :exc:`ValueError` to serialize out of range :class:`float` values (``nan``,
   ``inf``, ``-inf``) in strict compliance of the JSON specification, instead of
   using the JavaScript equivalents (``NaN``, ``Infinity``, ``-Infinity``).

   If *indent* is a non-negative integer, then JSON array elements and object
   members will be pretty-printed with that indent level.  An indent level of 0
   will only insert newlines.  ``None`` (the default) selects the most compact
   representation.

   If *separators* is an ``(item_separator, dict_separator)`` tuple, then it
   will be used instead of the default ``(', ', ': ')`` separators.  ``(',',
   ':')`` is the most compact JSON representation.

   *default(obj)* is a function that should return a serializable version of
   *obj* or raise :exc:`TypeError`.  The default simply raises :exc:`TypeError`.

   To use a custom :class:`JSONEncoder` subclass (e.g. one that overrides the
   :meth:`default` method to serialize additional types), specify it with the
   *cls* kwarg; otherwise :class:`JSONEncoder` is used.


.. function:: dumps(obj, skipkeys=False, ensure_ascii=True, check_circular=True, allow_nan=True, cls=None, indent=None, separators=None, default=None, **kw)

   Serialize *obj* to a JSON formatted :class:`str`.  The arguments have the
   same meaning as in :func:`dump`.


.. function:: load(fp, cls=None, object_hook=None, parse_float=None, parse_int=None, parse_constant=None, object_pairs_hook=None, **kw)

   Deserialize *fp* (a ``.read()``-supporting file-like object containing a JSON
   document) to a Python object.

   *object_hook* is an optional function that will be called with the result of
   any object literal decoded (a :class:`dict`).  The return value of
   *object_hook* will be used instead of the :class:`dict`.  This feature can be used
   to implement custom decoders (e.g. JSON-RPC class hinting).

   *object_pairs_hook* is an optional function that will be called with the
   result of any object literal decoded with an ordered list of pairs.  The
   return value of *object_pairs_hook* will be used instead of the
   :class:`dict`.  This feature can be used to implement custom decoders that
   rely on the order that the key and value pairs are decoded (for example,
   :func:`collections.OrderedDict` will remember the order of insertion). If
   *object_hook* is also defined, the *object_pairs_hook* takes priority.

   .. versionchanged:: 3.1
      Added support for *object_pairs_hook*.

   *parse_float*, if specified, will be called with the string of every JSON
   float to be decoded.  By default, this is equivalent to ``float(num_str)``.
   This can be used to use another datatype or parser for JSON floats
   (e.g. :class:`decimal.Decimal`).

   *parse_int*, if specified, will be called with the string of every JSON int
   to be decoded.  By default, this is equivalent to ``int(num_str)``.  This can
   be used to use another datatype or parser for JSON integers
   (e.g. :class:`float`).

   *parse_constant*, if specified, will be called with one of the following
   strings: ``'-Infinity'``, ``'Infinity'``, ``'NaN'``, ``'null'``, ``'true'``,
   ``'false'``.  This can be used to raise an exception if invalid JSON numbers
   are encountered.

   To use a custom :class:`JSONDecoder` subclass, specify it with the ``cls``
   kwarg; otherwise :class:`JSONDecoder` is used.  Additional keyword arguments
   will be passed to the constructor of the class.


.. function:: loads(s, encoding=None, cls=None, object_hook=None, parse_float=None, parse_int=None, parse_constant=None, object_pairs_hook=None, **kw)

   Deserialize *s* (a :class:`bytes` or :class:`str` instance containing a JSON
   document) to a Python object.

   If *s* is a :class:`bytes` instance and is encoded with an ASCII based encoding
   other than UTF-8 (e.g. latin-1), then an appropriate *encoding* name must be
   specified.  Encodings that are not ASCII based (such as UCS-2) are not
   allowed and should be decoded to :class:`str` first.

   The other arguments have the same meaning as in :func:`load`.


Encoders and decoders
---------------------

.. class:: JSONDecoder(object_hook=None, parse_float=None, parse_int=None, parse_constant=None, strict=True, object_pairs_hook=None)

   Simple JSON decoder.

   Performs the following translations in decoding by default:

   +---------------+-------------------+
   | JSON          | Python            |
   +===============+===================+
   | object        | dict              |
   +---------------+-------------------+
   | array         | list              |
   +---------------+-------------------+
   | string        | str               |
   +---------------+-------------------+
   | number (int)  | int               |
   +---------------+-------------------+
   | number (real) | float             |
   +---------------+-------------------+
   | true          | True              |
   +---------------+-------------------+
   | false         | False             |
   +---------------+-------------------+
   | null          | None              |
   +---------------+-------------------+

   It also understands ``NaN``, ``Infinity``, and ``-Infinity`` as their
   corresponding ``float`` values, which is outside the JSON spec.

   *object_hook*, if specified, will be called with the result of every JSON
   object decoded and its return value will be used in place of the given
   :class:`dict`.  This can be used to provide custom deserializations (e.g. to
   support JSON-RPC class hinting).

   *object_pairs_hook*, if specified will be called with the result of every
   JSON object decoded with an ordered list of pairs.  The return value of
   *object_pairs_hook* will be used instead of the :class:`dict`.  This
   feature can be used to implement custom decoders that rely on the order
   that the key and value pairs are decoded (for example,
   :func:`collections.OrderedDict` will remember the order of insertion). If
   *object_hook* is also defined, the *object_pairs_hook* takes priority.

   .. versionchanged:: 3.1
      Added support for *object_pairs_hook*.

   *parse_float*, if specified, will be called with the string of every JSON
   float to be decoded.  By default, this is equivalent to ``float(num_str)``.
   This can be used to use another datatype or parser for JSON floats
   (e.g. :class:`decimal.Decimal`).

   *parse_int*, if specified, will be called with the string of every JSON int
   to be decoded.  By default, this is equivalent to ``int(num_str)``.  This can
   be used to use another datatype or parser for JSON integers
   (e.g. :class:`float`).

   *parse_constant*, if specified, will be called with one of the following
   strings: ``'-Infinity'``, ``'Infinity'``, ``'NaN'``, ``'null'``, ``'true'``,
   ``'false'``.  This can be used to raise an exception if invalid JSON numbers
   are encountered.

   If *strict* is ``False`` (``True`` is the default), then control characters
   will be allowed inside strings.  Control characters in this context are
   those with character codes in the 0-31 range, including ``'\t'`` (tab),
   ``'\n'``, ``'\r'`` and ``'\0'``.


   .. method:: decode(s)

      Return the Python representation of *s* (a :class:`str` instance
      containing a JSON document)

   .. method:: raw_decode(s)

      Decode a JSON document from *s* (a :class:`str` beginning with a
      JSON document) and return a 2-tuple of the Python representation
      and the index in *s* where the document ended.

      This can be used to decode a JSON document from a string that may have
      extraneous data at the end.


.. class:: JSONEncoder(skipkeys=False, ensure_ascii=True, check_circular=True, allow_nan=True, sort_keys=False, indent=None, separators=None, default=None)

   Extensible JSON encoder for Python data structures.

   Supports the following objects and types by default:

   +-------------------+---------------+
   | Python            | JSON          |
   +===================+===============+
   | dict              | object        |
   +-------------------+---------------+
   | list, tuple       | array         |
   +-------------------+---------------+
   | str               | string        |
   +-------------------+---------------+
   | int, float        | number        |
   +-------------------+---------------+
   | True              | true          |
   +-------------------+---------------+
   | False             | false         |
   +-------------------+---------------+
   | None              | null          |
   +-------------------+---------------+

   To extend this to recognize other objects, subclass and implement a
   :meth:`default` method with another method that returns a serializable object
   for ``o`` if possible, otherwise it should call the superclass implementation
   (to raise :exc:`TypeError`).

   If *skipkeys* is ``False`` (the default), then it is a :exc:`TypeError` to
   attempt encoding of keys that are not str, int, float or None.  If
   *skipkeys* is ``True``, such items are simply skipped.

   If *ensure_ascii* is ``True`` (the default), the output is guaranteed to
   have all incoming non-ASCII characters escaped.  If *ensure_ascii* is
   ``False``, these characters will be output as-is.

   If *check_circular* is ``True`` (the default), then lists, dicts, and custom
   encoded objects will be checked for circular references during encoding to
   prevent an infinite recursion (which would cause an :exc:`OverflowError`).
   Otherwise, no such check takes place.

   If *allow_nan* is ``True`` (the default), then ``NaN``, ``Infinity``, and
   ``-Infinity`` will be encoded as such.  This behavior is not JSON
   specification compliant, but is consistent with most JavaScript based
   encoders and decoders.  Otherwise, it will be a :exc:`ValueError` to encode
   such floats.

   If *sort_keys* is ``True`` (default ``False``), then the output of dictionaries
   will be sorted by key; this is useful for regression tests to ensure that
   JSON serializations can be compared on a day-to-day basis.

   If *indent* is a non-negative integer (it is ``None`` by default), then JSON
   array elements and object members will be pretty-printed with that indent
   level.  An indent level of 0 will only insert newlines.  ``None`` is the most
   compact representation.

   If specified, *separators* should be an ``(item_separator, key_separator)``
   tuple.  The default is ``(', ', ': ')``.  To get the most compact JSON
   representation, you should specify ``(',', ':')`` to eliminate whitespace.

   If specified, *default* is a function that gets called for objects that can't
   otherwise be serialized.  It should return a JSON encodable version of the
   object or raise a :exc:`TypeError`.


   .. method:: default(o)

      Implement this method in a subclass such that it returns a serializable
      object for *o*, or calls the base implementation (to raise a
      :exc:`TypeError`).

      For example, to support arbitrary iterators, you could implement default
      like this::

         def default(self, o):
            try:
                iterable = iter(o)
            except TypeError:
                pass
            else:
                return list(iterable)
            return json.JSONEncoder.default(self, o)


   .. method:: encode(o)

      Return a JSON string representation of a Python data structure, *o*.  For
      example::

        >>> json.JSONEncoder().encode({"foo": ["bar", "baz"]})
        '{"foo": ["bar", "baz"]}'


   .. method:: iterencode(o)

      Encode the given object, *o*, and yield each string representation as
      available.  For example::

            for chunk in json.JSONEncoder().iterencode(bigobject):
                mysocket.write(chunk)
