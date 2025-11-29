:mod:`!xmlrpc.client` --- XML-RPC client access
===============================================

.. module:: xmlrpc.client
   :synopsis: XML-RPC client access.

.. moduleauthor:: Fredrik Lundh <fredrik@pythonware.com>
.. sectionauthor:: Eric S. Raymond <esr@snark.thyrsus.com>

**Source code:** :source:`Lib/xmlrpc/client.py`

.. XXX Not everything is documented yet.  It might be good to describe
   Marshaller, Unmarshaller, getparser and Transport.

--------------

XML-RPC is a Remote Procedure Call method that uses XML passed via HTTP(S) as a
transport.  With it, a client can call methods with parameters on a remote
server (the server is named by a URI) and get back structured data.  This module
supports writing XML-RPC client code; it handles all the details of translating
between conformable Python objects and XML on the wire.


.. warning::

   The :mod:`xmlrpc.client` module is not secure against maliciously
   constructed data.  If you need to parse untrusted or unauthenticated data,
   see :ref:`xml-security`.

.. versionchanged:: 3.5

   For HTTPS URIs, :mod:`xmlrpc.client` now performs all the necessary
   certificate and hostname checks by default.

.. include:: ../includes/wasm-notavail.rst

.. class:: ServerProxy(uri, transport=None, encoding=None, verbose=False, \
                       allow_none=False, use_datetime=False, \
                       use_builtin_types=False, *, headers=(), context=None)

   A :class:`ServerProxy` instance is an object that manages communication with a
   remote XML-RPC server.  The required first argument is a URI (Uniform Resource
   Indicator), and will normally be the URL of the server.  The optional second
   argument is a transport factory instance; by default it is an internal
   :class:`SafeTransport` instance for https: URLs and an internal HTTP
   :class:`Transport` instance otherwise.  The optional third argument is an
   encoding, by default UTF-8. The optional fourth argument is a debugging flag.

   The following parameters govern the use of the returned proxy instance.
   If *allow_none* is true,  the Python constant ``None`` will be translated into
   XML; the default behaviour is for ``None`` to raise a :exc:`TypeError`. This is
   a commonly used extension to the XML-RPC specification, but isn't supported by
   all clients and servers; see `http://ontosys.com/xml-rpc/extensions.php
   <https://web.archive.org/web/20130120074804/http://ontosys.com/xml-rpc/extensions.php>`_
   for a description.
   The *use_builtin_types* flag can be used to cause date/time values
   to be presented as :class:`datetime.datetime` objects and binary data to be
   presented as :class:`bytes` objects; this flag is false by default.
   :class:`datetime.datetime`, :class:`bytes` and :class:`bytearray` objects
   may be passed to calls.
   The *headers* parameter is an optional sequence of HTTP headers to send with
   each request, expressed as a sequence of 2-tuples representing the header
   name and value. (e.g. ``[('Header-Name', 'value')]``).
   If an HTTPS URL is provided, *context* may be :class:`ssl.SSLContext`
   and configures the SSL settings of the underlying HTTPS connection.
   The obsolete *use_datetime* flag is similar to *use_builtin_types* but it
   applies only to date/time values.

   .. versionchanged:: 3.3
      The *use_builtin_types* flag was added.

   .. versionchanged:: 3.8
      The *headers* parameter was added.

   Both the HTTP and HTTPS transports support the URL syntax extension for HTTP
   Basic Authentication: ``http://user:pass@host:port/path``.  The  ``user:pass``
   portion will be base64-encoded as an HTTP 'Authorization' header, and sent to
   the remote server as part of the connection process when invoking an XML-RPC
   method.  You only need to use this if the remote server requires a Basic
   Authentication user and password.

   The returned instance is a proxy object with methods that can be used to invoke
   corresponding RPC calls on the remote server.  If the remote server supports the
   introspection API, the proxy can also be used to query the remote server for the
   methods it supports (service discovery) and fetch other server-associated
   metadata.

   Types that are conformable (e.g. that can be marshalled through XML),
   include the following (and except where noted, they are unmarshalled
   as the same Python type):

   .. tabularcolumns:: |l|L|

   +----------------------+-------------------------------------------------------+
   | XML-RPC type         | Python type                                           |
   +======================+=======================================================+
   | ``boolean``          | :class:`bool`                                         |
   +----------------------+-------------------------------------------------------+
   | ``int``, ``i1``,     | :class:`int` in range from -2147483648 to 2147483647. |
   | ``i2``,  ``i4``,     | Values get the ``<int>`` tag.                         |
   | ``i8`` or            |                                                       |
   | ``biginteger``       |                                                       |
   +----------------------+-------------------------------------------------------+
   | ``double`` or        | :class:`float`.  Values get the ``<double>`` tag.     |
   | ``float``            |                                                       |
   +----------------------+-------------------------------------------------------+
   | ``string``           | :class:`str`                                          |
   +----------------------+-------------------------------------------------------+
   | ``array``            | :class:`list` or :class:`tuple` containing            |
   |                      | conformable elements.  Arrays are returned as         |
   |                      | :class:`lists <list>`.                                |
   +----------------------+-------------------------------------------------------+
   | ``struct``           | :class:`dict`.  Keys must be strings, values may be   |
   |                      | any conformable type.  Objects of user-defined        |
   |                      | classes can be passed in; only their                  |
   |                      | :attr:`~object.__dict__` attribute is transmitted.    |
   +----------------------+-------------------------------------------------------+
   | ``dateTime.iso8601`` | :class:`DateTime` or :class:`datetime.datetime`.      |
   |                      | Returned type depends on values of                    |
   |                      | *use_builtin_types* and *use_datetime* flags.         |
   +----------------------+-------------------------------------------------------+
   | ``base64``           | :class:`Binary`, :class:`bytes` or                    |
   |                      | :class:`bytearray`.  Returned type depends on the     |
   |                      | value of the *use_builtin_types* flag.                |
   +----------------------+-------------------------------------------------------+
   | ``nil``              | The ``None`` constant.  Passing is allowed only if    |
   |                      | *allow_none* is true.                                 |
   +----------------------+-------------------------------------------------------+
   | ``bigdecimal``       | :class:`decimal.Decimal`.  Returned type only.        |
   +----------------------+-------------------------------------------------------+

   This is the full set of data types supported by XML-RPC.  Method calls may also
   raise a special :exc:`Fault` instance, used to signal XML-RPC server errors, or
   :exc:`ProtocolError` used to signal an error in the HTTP/HTTPS transport layer.
   Both :exc:`Fault` and :exc:`ProtocolError` derive from a base class called
   :exc:`Error`.  Note that the xmlrpc client module currently does not marshal
   instances of subclasses of built-in types.

   When passing strings, characters special to XML such as ``<``, ``>``, and ``&``
   will be automatically escaped.  However, it's the caller's responsibility to
   ensure that the string is free of characters that aren't allowed in XML, such as
   the control characters with ASCII values between 0 and 31 (except, of course,
   tab, newline and carriage return); failing to do this will result in an XML-RPC
   request that isn't well-formed XML.  If you have to pass arbitrary bytes
   via XML-RPC, use :class:`bytes` or :class:`bytearray` classes or the
   :class:`Binary` wrapper class described below.

   :class:`Server` is retained as an alias for :class:`ServerProxy` for backwards
   compatibility.  New code should use :class:`ServerProxy`.

   .. versionchanged:: 3.5
      Added the *context* argument.

   .. versionchanged:: 3.6
      Added support of type tags with prefixes (e.g. ``ex:nil``).
      Added support of unmarshalling additional types used by Apache XML-RPC
      implementation for numerics: ``i1``, ``i2``, ``i8``, ``biginteger``,
      ``float`` and ``bigdecimal``.
      See https://ws.apache.org/xmlrpc/types.html for a description.


.. seealso::

   `XML-RPC HOWTO <https://tldp.org/HOWTO/XML-RPC-HOWTO/index.html>`_
      A good description of XML-RPC operation and client software in several languages.
      Contains pretty much everything an XML-RPC client developer needs to know.

   `XML-RPC Introspection <https://xmlrpc-c.sourceforge.io/introspection.html>`_
      Describes the XML-RPC protocol extension for introspection.

   `XML-RPC Specification <http://xmlrpc.scripting.com/spec.html>`_
      The official specification.

.. _serverproxy-objects:

ServerProxy Objects
-------------------

A :class:`ServerProxy` instance has a method corresponding to each remote
procedure call accepted by the XML-RPC server.  Calling the method performs an
RPC, dispatched by both name and argument signature (e.g. the same method name
can be overloaded with multiple argument signatures).  The RPC finishes either
by returning data in a conformant type or by raising a :class:`Fault` or
:class:`ProtocolError` exception indicating an error.

Servers that support the XML introspection API support some common methods
grouped under the reserved :attr:`~ServerProxy.system` attribute:


.. method:: ServerProxy.system.listMethods()

   This method returns a list of strings, one for each (non-system) method
   supported by the XML-RPC server.


.. method:: ServerProxy.system.methodSignature(name)

   This method takes one parameter, the name of a method implemented by the XML-RPC
   server. It returns an array of possible signatures for this method. A signature
   is an array of types. The first of these types is the return type of the method,
   the rest are parameters.

   Because multiple signatures (ie. overloading) is permitted, this method returns
   a list of signatures rather than a singleton.

   Signatures themselves are restricted to the top level parameters expected by a
   method. For instance if a method expects one array of structs as a parameter,
   and it returns a string, its signature is simply "string, array". If it expects
   three integers and returns a string, its signature is "string, int, int, int".

   If no signature is defined for the method, a non-array value is returned. In
   Python this means that the type of the returned  value will be something other
   than list.


.. method:: ServerProxy.system.methodHelp(name)

   This method takes one parameter, the name of a method implemented by the XML-RPC
   server.  It returns a documentation string describing the use of that method. If
   no such string is available, an empty string is returned. The documentation
   string may contain HTML markup.

.. versionchanged:: 3.5

   Instances of :class:`ServerProxy` support the :term:`context manager` protocol
   for closing the underlying transport.


A working example follows. The server code::

   from xmlrpc.server import SimpleXMLRPCServer

   def is_even(n):
       return n % 2 == 0

   server = SimpleXMLRPCServer(("localhost", 8000))
   print("Listening on port 8000...")
   server.register_function(is_even, "is_even")
   server.serve_forever()

The client code for the preceding server::

   import xmlrpc.client

   with xmlrpc.client.ServerProxy("http://localhost:8000/") as proxy:
       print("3 is even: %s" % str(proxy.is_even(3)))
       print("100 is even: %s" % str(proxy.is_even(100)))

.. _datetime-objects:

DateTime Objects
----------------

.. class:: DateTime

   This class may be initialized with seconds since the epoch, a time
   tuple, an ISO 8601 time/date string, or a :class:`datetime.datetime`
   instance.  It has the following methods, supported mainly for internal
   use by the marshalling/unmarshalling code:


   .. method:: decode(string)

      Accept a string as the instance's new time value.


   .. method:: encode(out)

      Write the XML-RPC encoding of this :class:`DateTime` item to the *out* stream
      object.

   It also supports certain of Python's built-in operators through
   :meth:`rich comparison <object.__lt__>` and :meth:`~object.__repr__`
   methods.

A working example follows. The server code::

   import datetime
   from xmlrpc.server import SimpleXMLRPCServer
   import xmlrpc.client

   def today():
       today = datetime.datetime.today()
       return xmlrpc.client.DateTime(today)

   server = SimpleXMLRPCServer(("localhost", 8000))
   print("Listening on port 8000...")
   server.register_function(today, "today")
   server.serve_forever()

The client code for the preceding server::

   import xmlrpc.client
   import datetime

   proxy = xmlrpc.client.ServerProxy("http://localhost:8000/")

   today = proxy.today()
   # convert the ISO8601 string to a datetime object
   converted = datetime.datetime.strptime(today.value, "%Y%m%dT%H:%M:%S")
   print("Today: %s" % converted.strftime("%d.%m.%Y, %H:%M"))

.. _binary-objects:

Binary Objects
--------------

.. class:: Binary

   This class may be initialized from bytes data (which may include NULs). The
   primary access to the content of a :class:`Binary` object is provided by an
   attribute:


   .. attribute:: data

      The binary data encapsulated by the :class:`Binary` instance.  The data is
      provided as a :class:`bytes` object.

   :class:`Binary` objects have the following methods, supported mainly for
   internal use by the marshalling/unmarshalling code:


   .. method:: decode(bytes)

      Accept a base64 :class:`bytes` object and decode it as the instance's new data.


   .. method:: encode(out)

      Write the XML-RPC base 64 encoding of this binary item to the *out* stream object.

      The encoded data will have newlines every 76 characters as per
      :rfc:`RFC 2045 section 6.8 <2045#section-6.8>`,
      which was the de facto standard base64 specification when the
      XML-RPC spec was written.

   It also supports certain of Python's built-in operators through
   :meth:`~object.__eq__` and :meth:`~object.__ne__` methods.

Example usage of the binary objects.  We're going to transfer an image over
XMLRPC::

   from xmlrpc.server import SimpleXMLRPCServer
   import xmlrpc.client

   def python_logo():
       with open("python_logo.jpg", "rb") as handle:
           return xmlrpc.client.Binary(handle.read())

   server = SimpleXMLRPCServer(("localhost", 8000))
   print("Listening on port 8000...")
   server.register_function(python_logo, 'python_logo')

   server.serve_forever()

The client gets the image and saves it to a file::

   import xmlrpc.client

   proxy = xmlrpc.client.ServerProxy("http://localhost:8000/")
   with open("fetched_python_logo.jpg", "wb") as handle:
       handle.write(proxy.python_logo().data)

.. _fault-objects:

Fault Objects
-------------

.. class:: Fault

   A :class:`Fault` object encapsulates the content of an XML-RPC fault tag. Fault
   objects have the following attributes:


   .. attribute:: faultCode

      An int indicating the fault type.


   .. attribute:: faultString

      A string containing a diagnostic message associated with the fault.

In the following example we're going to intentionally cause a :exc:`Fault` by
returning a complex type object.  The server code::

   from xmlrpc.server import SimpleXMLRPCServer

   # A marshalling error is going to occur because we're returning a
   # complex number
   def add(x, y):
       return x+y+0j

   server = SimpleXMLRPCServer(("localhost", 8000))
   print("Listening on port 8000...")
   server.register_function(add, 'add')

   server.serve_forever()

The client code for the preceding server::

   import xmlrpc.client

   proxy = xmlrpc.client.ServerProxy("http://localhost:8000/")
   try:
       proxy.add(2, 5)
   except xmlrpc.client.Fault as err:
       print("A fault occurred")
       print("Fault code: %d" % err.faultCode)
       print("Fault string: %s" % err.faultString)



.. _protocol-error-objects:

ProtocolError Objects
---------------------

.. class:: ProtocolError

   A :class:`ProtocolError` object describes a protocol error in the underlying
   transport layer (such as a 404 'not found' error if the server named by the URI
   does not exist).  It has the following attributes:


   .. attribute:: url

      The URI or URL that triggered the error.


   .. attribute:: errcode

      The error code.


   .. attribute:: errmsg

      The error message or diagnostic string.


   .. attribute:: headers

      A dict containing the headers of the HTTP/HTTPS request that triggered the
      error.

In the following example we're going to intentionally cause a :exc:`ProtocolError`
by providing an invalid URI::

   import xmlrpc.client

   # create a ServerProxy with a URI that doesn't respond to XMLRPC requests
   proxy = xmlrpc.client.ServerProxy("http://google.com/")

   try:
       proxy.some_method()
   except xmlrpc.client.ProtocolError as err:
       print("A protocol error occurred")
       print("URL: %s" % err.url)
       print("HTTP/HTTPS headers: %s" % err.headers)
       print("Error code: %d" % err.errcode)
       print("Error message: %s" % err.errmsg)

MultiCall Objects
-----------------

The :class:`MultiCall` object provides a way to encapsulate multiple calls to a
remote server into a single request [#]_.


.. class:: MultiCall(server)

   Create an object used to boxcar method calls. *server* is the eventual target of
   the call. Calls can be made to the result object, but they will immediately
   return ``None``, and only store the call name and arguments in the
   :class:`MultiCall` object. Calling the object itself causes all stored calls to
   be transmitted as a single ``system.multicall`` request. The result of this call
   is a :term:`generator`; iterating over this generator yields the individual
   results.

A usage example of this class follows.  The server code::

   from xmlrpc.server import SimpleXMLRPCServer

   def add(x, y):
       return x + y

   def subtract(x, y):
       return x - y

   def multiply(x, y):
       return x * y

   def divide(x, y):
       return x // y

   # A simple server with simple arithmetic functions
   server = SimpleXMLRPCServer(("localhost", 8000))
   print("Listening on port 8000...")
   server.register_multicall_functions()
   server.register_function(add, 'add')
   server.register_function(subtract, 'subtract')
   server.register_function(multiply, 'multiply')
   server.register_function(divide, 'divide')
   server.serve_forever()

The client code for the preceding server::

   import xmlrpc.client

   proxy = xmlrpc.client.ServerProxy("http://localhost:8000/")
   multicall = xmlrpc.client.MultiCall(proxy)
   multicall.add(7, 3)
   multicall.subtract(7, 3)
   multicall.multiply(7, 3)
   multicall.divide(7, 3)
   result = multicall()

   print("7+3=%d, 7-3=%d, 7*3=%d, 7//3=%d" % tuple(result))


Convenience Functions
---------------------

.. function:: dumps(params, methodname=None, methodresponse=None, encoding=None, allow_none=False)

   Convert *params* into an XML-RPC request, or into a response if *methodresponse*
   is true. *params* can be either a tuple of arguments or an instance of the
   :exc:`Fault` exception class.  If *methodresponse* is true, only a single value
   can be returned, meaning that *params* must be of length 1. *encoding*, if
   supplied, is the encoding to use in the generated XML; the default is UTF-8.
   Python's :const:`None` value cannot be used in standard XML-RPC; to allow using
   it via an extension,  provide a true value for *allow_none*.


.. function:: loads(data, use_datetime=False, use_builtin_types=False)

   Convert an XML-RPC request or response into Python objects, a ``(params,
   methodname)``.  *params* is a tuple of argument; *methodname* is a string, or
   ``None`` if no method name is present in the packet. If the XML-RPC packet
   represents a fault condition, this function will raise a :exc:`Fault` exception.
   The *use_builtin_types* flag can be used to cause date/time values to be
   presented as :class:`datetime.datetime` objects and binary data to be
   presented as :class:`bytes` objects; this flag is false by default.

   The obsolete *use_datetime* flag is similar to *use_builtin_types* but it
   applies only to date/time values.

   .. versionchanged:: 3.3
      The *use_builtin_types* flag was added.


.. _xmlrpc-client-example:

Example of Client Usage
-----------------------

::

   # simple test program (from the XML-RPC specification)
   from xmlrpc.client import ServerProxy, Error

   # server = ServerProxy("http://localhost:8000") # local server
   with ServerProxy("http://betty.userland.com") as proxy:

       print(proxy)

       try:
           print(proxy.examples.getStateName(41))
       except Error as v:
           print("ERROR", v)

To access an XML-RPC server through a HTTP proxy, you need to define a custom
transport.  The following example shows how::

   import http.client
   import xmlrpc.client

   class ProxiedTransport(xmlrpc.client.Transport):

       def set_proxy(self, host, port=None, headers=None):
           self.proxy = host, port
           self.proxy_headers = headers

       def make_connection(self, host):
           connection = http.client.HTTPConnection(*self.proxy)
           connection.set_tunnel(host, headers=self.proxy_headers)
           self._connection = host, connection
           return connection

   transport = ProxiedTransport()
   transport.set_proxy('proxy-server', 8080)
   server = xmlrpc.client.ServerProxy('http://betty.userland.com', transport=transport)
   print(server.examples.getStateName(41))


Example of Client and Server Usage
----------------------------------

See :ref:`simplexmlrpcserver-example`.


.. rubric:: Footnotes

.. [#] This approach has been first presented in `a discussion on xmlrpc.com
   <https://web.archive.org/web/20060624230303/http://www.xmlrpc.com/discuss/msgReader$1208?mode=topic>`_.
.. the link now points to webarchive since the one at
.. http://www.xmlrpc.com/discuss/msgReader%241208 is broken (and webadmin
.. doesn't reply)
