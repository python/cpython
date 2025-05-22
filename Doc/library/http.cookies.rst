:mod:`!http.cookies` --- HTTP state management
==============================================

.. module:: http.cookies
   :synopsis: Support for HTTP state management (cookies).

.. moduleauthor:: Timothy O'Malley <timo@alum.mit.edu>
.. sectionauthor:: Moshe Zadka <moshez@zadka.site.co.il>

**Source code:** :source:`Lib/http/cookies.py`

--------------

The :mod:`http.cookies` module defines classes for abstracting the concept of
cookies, an HTTP state management mechanism. It supports both simple string-only
cookies, and provides an abstraction for having any serializable data-type as
cookie value.

The module formerly strictly applied the parsing rules described in the
:rfc:`2109` and :rfc:`2068` specifications.  It has since been discovered that
MSIE 3.0x didn't follow the character rules outlined in those specs; many
current-day browsers and servers have also relaxed parsing rules when it comes
to cookie handling.  As a result, this module now uses parsing rules that are a
bit less strict than they once were.

The character set, :data:`string.ascii_letters`, :data:`string.digits` and
``!#$%&'*+-.^_`|~:`` denote the set of valid characters allowed by this module
in a cookie name (as :attr:`~Morsel.key`).

.. versionchanged:: 3.3
   Allowed ':' as a valid cookie name character.


.. note::

   On encountering an invalid cookie, :exc:`CookieError` is raised, so if your
   cookie data comes from a browser you should always prepare for invalid data
   and catch :exc:`CookieError` on parsing.


.. exception:: CookieError

   Exception failing because of :rfc:`2109` invalidity: incorrect attributes,
   incorrect :mailheader:`Set-Cookie` header, etc.


.. class:: BaseCookie([input])

   This class is a dictionary-like object whose keys are strings and whose values
   are :class:`Morsel` instances. Note that upon setting a key to a value, the
   value is first converted to a :class:`Morsel` containing the key and the value.

   If *input* is given, it is passed to the :meth:`load` method.


.. class:: SimpleCookie([input])

   This class derives from :class:`BaseCookie` and overrides :meth:`~BaseCookie.value_decode`
   and :meth:`~BaseCookie.value_encode`. :class:`!SimpleCookie` supports
   strings as cookie values. When setting the value, :class:`!SimpleCookie`
   calls the builtin :func:`str` to convert
   the value to a string. Values received from HTTP are kept as strings.

.. seealso::

   Module :mod:`http.cookiejar`
      HTTP cookie handling for web *clients*.  The :mod:`http.cookiejar` and
      :mod:`http.cookies` modules do not depend on each other.

   :rfc:`2109` - HTTP State Management Mechanism
      This is the state management specification implemented by this module.


.. _cookie-objects:

Cookie Objects
--------------


.. method:: BaseCookie.value_decode(val)

   Return a tuple ``(real_value, coded_value)`` from a string representation.
   ``real_value`` can be any type. This method does no decoding in
   :class:`BaseCookie` --- it exists so it can be overridden.


.. method:: BaseCookie.value_encode(val)

   Return a tuple ``(real_value, coded_value)``. *val* can be any type, but
   ``coded_value`` will always be converted to a string.
   This method does no encoding in :class:`BaseCookie` --- it exists so it can
   be overridden.

   In general, it should be the case that :meth:`value_encode` and
   :meth:`value_decode` are inverses on the range of *value_decode*.


.. method:: BaseCookie.output(attrs=None, header='Set-Cookie:', sep='\r\n')

   Return a string representation suitable to be sent as HTTP headers. *attrs* and
   *header* are sent to each :class:`Morsel`'s :meth:`~Morsel.output` method. *sep* is used
   to join the headers together, and is by default the combination ``'\r\n'``
   (CRLF).


.. method:: BaseCookie.js_output(attrs=None)

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


.. class:: Morsel

   Abstract a key/value pair, which has some :rfc:`2109` attributes.

   Morsels are dictionary-like objects, whose set of keys is constant --- the valid
   :rfc:`2109` attributes, which are:

     .. attribute:: expires
                    path
                    comment
                    domain
                    max-age
                    secure
                    version
                    httponly
                    samesite
                    partitioned

   The attribute :attr:`httponly` specifies that the cookie is only transferred
   in HTTP requests, and is not accessible through JavaScript. This is intended
   to mitigate some forms of cross-site scripting.

   The attribute :attr:`samesite` specifies that the browser is not allowed to
   send the cookie along with cross-site requests. This helps to mitigate CSRF
   attacks. Valid values for this attribute are "Strict" and "Lax".

   The attribute :attr:`partitioned` indicates to user agents that these
   cross-site cookies *should* only be available in the same top-level context
   that the cookie was first set in. For this to be accepted by the user agent,
   you **must** also set ``Secure``.

   In addition, it is recommended to use the ``__Host`` prefix when setting
   partitioned cookies to make them bound to the hostname and not the
   registrable domain. Read
   `CHIPS (Cookies Having Independent Partitioned State)`_
   for full details and examples.

   .. _CHIPS (Cookies Having Independent Partitioned State): https://github.com/privacycg/CHIPS/blob/main/README.md

   The keys are case-insensitive and their default value is ``''``.

   .. versionchanged:: 3.5
      :meth:`!__eq__` now takes :attr:`~Morsel.key` and :attr:`~Morsel.value`
      into account.

   .. versionchanged:: 3.7
      Attributes :attr:`~Morsel.key`, :attr:`~Morsel.value` and
      :attr:`~Morsel.coded_value` are read-only.  Use :meth:`~Morsel.set` for
      setting them.

   .. versionchanged:: 3.8
      Added support for the :attr:`samesite` attribute.

   .. versionchanged:: 3.14
      Added support for the :attr:`partitioned` attribute.


.. attribute:: Morsel.value

   The value of the cookie.


.. attribute:: Morsel.coded_value

   The encoded value of the cookie --- this is what should be sent.


.. attribute:: Morsel.key

   The name of the cookie.


.. method:: Morsel.set(key, value, coded_value)

   Set the *key*, *value* and *coded_value* attributes.


.. method:: Morsel.isReservedKey(K)

   Whether *K* is a member of the set of keys of a :class:`Morsel`.


.. method:: Morsel.output(attrs=None, header='Set-Cookie:')

   Return a string representation of the Morsel, suitable to be sent as an HTTP
   header. By default, all the attributes are included, unless *attrs* is given, in
   which case it should be a list of attributes to use. *header* is by default
   ``"Set-Cookie:"``.


.. method:: Morsel.js_output(attrs=None)

   Return an embeddable JavaScript snippet, which, if run on a browser which
   supports JavaScript, will act the same as if the HTTP header was sent.

   The meaning for *attrs* is the same as in :meth:`output`.


.. method:: Morsel.OutputString(attrs=None)

   Return a string representing the Morsel, without any surrounding HTTP or
   JavaScript.

   The meaning for *attrs* is the same as in :meth:`output`.


.. method:: Morsel.update(values)

   Update the values in the Morsel dictionary with the values in the dictionary
   *values*.  Raise an error if any of the keys in the *values* dict is not a
   valid :rfc:`2109` attribute.

   .. versionchanged:: 3.5
      an error is raised for invalid keys.


.. method:: Morsel.copy(value)

   Return a shallow copy of the Morsel object.

   .. versionchanged:: 3.5
      return a Morsel object instead of a dict.


.. method:: Morsel.setdefault(key, value=None)

   Raise an error if key is not a valid :rfc:`2109` attribute, otherwise
   behave the same as :meth:`dict.setdefault`.


.. _cookie-example:

Example
-------

The following example demonstrates how to use the :mod:`http.cookies` module.

.. doctest::
   :options: +NORMALIZE_WHITESPACE

   >>> from http import cookies
   >>> C = cookies.SimpleCookie()
   >>> C["fig"] = "newton"
   >>> C["sugar"] = "wafer"
   >>> print(C) # generate HTTP headers
   Set-Cookie: fig=newton
   Set-Cookie: sugar=wafer
   >>> print(C.output()) # same thing
   Set-Cookie: fig=newton
   Set-Cookie: sugar=wafer
   >>> C = cookies.SimpleCookie()
   >>> C["rocky"] = "road"
   >>> C["rocky"]["path"] = "/cookie"
   >>> print(C.output(header="Cookie:"))
   Cookie: rocky=road; Path=/cookie
   >>> print(C.output(attrs=[], header="Cookie:"))
   Cookie: rocky=road
   >>> C = cookies.SimpleCookie()
   >>> C.load("chips=ahoy; vienna=finger") # load from a string (HTTP header)
   >>> print(C)
   Set-Cookie: chips=ahoy
   Set-Cookie: vienna=finger
   >>> C = cookies.SimpleCookie()
   >>> C.load('keebler="E=everybody; L=\\"Loves\\"; fudge=\\012;";')
   >>> print(C)
   Set-Cookie: keebler="E=everybody; L=\"Loves\"; fudge=\012;"
   >>> C = cookies.SimpleCookie()
   >>> C["oreo"] = "doublestuff"
   >>> C["oreo"]["path"] = "/"
   >>> print(C)
   Set-Cookie: oreo=doublestuff; Path=/
   >>> C = cookies.SimpleCookie()
   >>> C["twix"] = "none for you"
   >>> C["twix"].value
   'none for you'
   >>> C = cookies.SimpleCookie()
   >>> C["number"] = 7 # equivalent to C["number"] = str(7)
   >>> C["string"] = "seven"
   >>> C["number"].value
   '7'
   >>> C["string"].value
   'seven'
   >>> print(C)
   Set-Cookie: number=7
   Set-Cookie: string=seven
