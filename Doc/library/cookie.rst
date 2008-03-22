
:mod:`Cookie` --- HTTP state management
=======================================

.. module:: Cookie
   :synopsis: Support for HTTP state management (cookies).
.. moduleauthor:: Timothy O'Malley <timo@alum.mit.edu>
.. sectionauthor:: Moshe Zadka <moshez@zadka.site.co.il>


The :mod:`Cookie` module defines classes for abstracting the concept of
cookies, an HTTP state management mechanism. It supports both simple string-only
cookies, and provides an abstraction for having any serializable data-type as
cookie value.

The module formerly strictly applied the parsing rules described in the
:rfc:`2109` and :rfc:`2068` specifications.  It has since been discovered that
MSIE 3.0x doesn't follow the character rules outlined in those specs.  As a
result, the parsing rules used are a bit less strict.


.. exception:: CookieError

   Exception failing because of :rfc:`2109` invalidity: incorrect attributes,
   incorrect :mailheader:`Set-Cookie` header, etc.


.. class:: BaseCookie([input])

   This class is a dictionary-like object whose keys are strings and whose values
   are :class:`Morsel` instances. Note that upon setting a key to a value, the
   value is first converted to a :class:`Morsel` containing the key and the value.

   If *input* is given, it is passed to the :meth:`load` method.


.. class:: SimpleCookie([input])

   This class derives from :class:`BaseCookie` and overrides :meth:`value_decode`
   and :meth:`value_encode` to be the identity and :func:`str` respectively.


.. class:: SerialCookie([input])

   This class derives from :class:`BaseCookie` and overrides :meth:`value_decode`
   and :meth:`value_encode` to be the :func:`pickle.loads` and
   :func:`pickle.dumps`.

   .. deprecated:: 2.3
      Reading pickled values from untrusted cookie data is a huge security hole, as
      pickle strings can be crafted to cause arbitrary code to execute on your server.
      It is supported for backwards compatibility only, and may eventually go away.


.. class:: SmartCookie([input])

   This class derives from :class:`BaseCookie`. It overrides :meth:`value_decode`
   to be :func:`pickle.loads` if it is a valid pickle, and otherwise the value
   itself. It overrides :meth:`value_encode` to be :func:`pickle.dumps` unless it
   is a string, in which case it returns the value itself.

   .. deprecated:: 2.3
      The same security warning from :class:`SerialCookie` applies here.

A further security note is warranted.  For backwards compatibility, the
:mod:`Cookie` module exports a class named :class:`Cookie` which is just an
alias for :class:`SmartCookie`.  This is probably a mistake and will likely be
removed in a future version.  You should not use the :class:`Cookie` class in
your applications, for the same reason why you should not use the
:class:`SerialCookie` class.


.. seealso::

   Module :mod:`cookielib`
      HTTP cookie handling for web *clients*.  The :mod:`cookielib` and :mod:`Cookie`
      modules do not depend on each other.

   :rfc:`2109` - HTTP State Management Mechanism
      This is the state management specification implemented by this module.


.. _cookie-objects:

Cookie Objects
--------------


.. method:: BaseCookie.value_decode(val)

   Return a decoded value from a string representation. Return value can be any
   type. This method does nothing in :class:`BaseCookie` --- it exists so it can be
   overridden.


.. method:: BaseCookie.value_encode(val)

   Return an encoded value. *val* can be any type, but return value must be a
   string. This method does nothing in :class:`BaseCookie` --- it exists so it can
   be overridden

   In general, it should be the case that :meth:`value_encode` and
   :meth:`value_decode` are inverses on the range of *value_decode*.


.. method:: BaseCookie.output([attrs[, header[, sep]]])

   Return a string representation suitable to be sent as HTTP headers. *attrs* and
   *header* are sent to each :class:`Morsel`'s :meth:`output` method. *sep* is used
   to join the headers together, and is by default the combination ``'\r\n'``
   (CRLF).

   .. versionchanged:: 2.5
      The default separator has been changed from ``'\n'`` to match the cookie
      specification.


.. method:: BaseCookie.js_output([attrs])

   Return an embeddable JavaScript snippet, which, if run on a browser which
   supports JavaScript, will act the same as if the HTTP headers was sent.

   The meaning for *attrs* is the same as in :meth:`output`.


.. method:: BaseCookie.load(rawdata)

   If *rawdata* is a string, parse it as an ``HTTP_COOKIE`` and add the values
   found there as :class:`Morsel`\ s. If it is a dictionary, it is equivalent to::

      for k, v in rawdata.items():
          cookie[k] = v


.. _morsel-objects:

Morsel Objects
--------------


.. class:: Morsel()

   Abstract a key/value pair, which has some :rfc:`2109` attributes.

   Morsels are dictionary-like objects, whose set of keys is constant --- the valid
   :rfc:`2109` attributes, which are

   * ``expires``
   * ``path``
   * ``comment``
   * ``domain``
   * ``max-age``
   * ``secure``
   * ``version``

   The keys are case-insensitive.


.. attribute:: Morsel.value

   The value of the cookie.


.. attribute:: Morsel.coded_value

   The encoded value of the cookie --- this is what should be sent.


.. attribute:: Morsel.key

   The name of the cookie.


.. method:: Morsel.set(key, value, coded_value)

   Set the *key*, *value* and *coded_value* members.


.. method:: Morsel.isReservedKey(K)

   Whether *K* is a member of the set of keys of a :class:`Morsel`.


.. method:: Morsel.output([attrs[, header]])

   Return a string representation of the Morsel, suitable to be sent as an HTTP
   header. By default, all the attributes are included, unless *attrs* is given, in
   which case it should be a list of attributes to use. *header* is by default
   ``"Set-Cookie:"``.


.. method:: Morsel.js_output([attrs])

   Return an embeddable JavaScript snippet, which, if run on a browser which
   supports JavaScript, will act the same as if the HTTP header was sent.

   The meaning for *attrs* is the same as in :meth:`output`.


.. method:: Morsel.OutputString([attrs])

   Return a string representing the Morsel, without any surrounding HTTP or
   JavaScript.

   The meaning for *attrs* is the same as in :meth:`output`.


.. _cookie-example:

Example
-------

The following example demonstrates how to use the :mod:`Cookie` module.

.. doctest::
   :options: +NORMALIZE_WHITESPACE

   >>> import Cookie
   >>> C = Cookie.SimpleCookie()
   >>> C = Cookie.SerialCookie()
   >>> C = Cookie.SmartCookie()
   >>> C["fig"] = "newton"
   >>> C["sugar"] = "wafer"
   >>> print C # generate HTTP headers
   Set-Cookie: fig=newton
   Set-Cookie: sugar=wafer
   >>> print C.output() # same thing
   Set-Cookie: fig=newton
   Set-Cookie: sugar=wafer
   >>> C = Cookie.SmartCookie()
   >>> C["rocky"] = "road"
   >>> C["rocky"]["path"] = "/cookie"
   >>> print C.output(header="Cookie:")
   Cookie: rocky=road; Path=/cookie
   >>> print C.output(attrs=[], header="Cookie:")
   Cookie: rocky=road
   >>> C = Cookie.SmartCookie()
   >>> C.load("chips=ahoy; vienna=finger") # load from a string (HTTP header)
   >>> print C
   Set-Cookie: chips=ahoy
   Set-Cookie: vienna=finger
   >>> C = Cookie.SmartCookie()
   >>> C.load('keebler="E=everybody; L=\\"Loves\\"; fudge=\\012;";')
   >>> print C
   Set-Cookie: keebler="E=everybody; L=\"Loves\"; fudge=\012;"
   >>> C = Cookie.SmartCookie()
   >>> C["oreo"] = "doublestuff"
   >>> C["oreo"]["path"] = "/"
   >>> print C
   Set-Cookie: oreo=doublestuff; Path=/
   >>> C = Cookie.SmartCookie()
   >>> C["twix"] = "none for you"
   >>> C["twix"].value
   'none for you'
   >>> C = Cookie.SimpleCookie()
   >>> C["number"] = 7 # equivalent to C["number"] = str(7)
   >>> C["string"] = "seven"
   >>> C["number"].value
   '7'
   >>> C["string"].value
   'seven'
   >>> print C
   Set-Cookie: number=7
   Set-Cookie: string=seven
   >>> C = Cookie.SerialCookie()
   >>> C["number"] = 7
   >>> C["string"] = "seven"
   >>> C["number"].value
   7
   >>> C["string"].value
   'seven'
   >>> print C
   Set-Cookie: number="I7\012."
   Set-Cookie: string="S'seven'\012p1\012."
   >>> C = Cookie.SmartCookie()
   >>> C["number"] = 7
   >>> C["string"] = "seven"
   >>> C["number"].value
   7
   >>> C["string"].value
   'seven'
   >>> print C
   Set-Cookie: number="I7\012."
   Set-Cookie: string=seven

