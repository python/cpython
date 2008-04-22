:mod:`xmlrpclib` --- XML-RPC client access
==========================================

.. module:: xmlrpclib
   :synopsis: XML-RPC client access.
.. moduleauthor:: Fredrik Lundh <fredrik@pythonware.com>
.. sectionauthor:: Eric S. Raymond <esr@snark.thyrsus.com>


.. XXX Not everything is documented yet.  It might be good to describe
   Marshaller, Unmarshaller, getparser, dumps, loads, and Transport.

.. versionadded:: 2.2

XML-RPC is a Remote Procedure Call method that uses XML passed via HTTP as a
transport.  With it, a client can call methods with parameters on a remote
server (the server is named by a URI) and get back structured data.  This module
supports writing XML-RPC client code; it handles all the details of translating
between conformable Python objects and XML on the wire.


.. class:: ServerProxy(uri[, transport[, encoding[, verbose[,  allow_none[, use_datetime]]]]])

   A :class:`ServerProxy` instance is an object that manages communication with a
   remote XML-RPC server.  The required first argument is a URI (Uniform Resource
   Indicator), and will normally be the URL of the server.  The optional second
   argument is a transport factory instance; by default it is an internal
   :class:`SafeTransport` instance for https: URLs and an internal HTTP
   :class:`Transport` instance otherwise.  The optional third argument is an
   encoding, by default UTF-8. The optional fourth argument is a debugging flag.
   If *allow_none* is true,  the Python constant ``None`` will be translated into
   XML; the default behaviour is for ``None`` to raise a :exc:`TypeError`. This is
   a commonly-used extension to the XML-RPC specification, but isn't supported by
   all clients and servers; see http://ontosys.com/xml-rpc/extensions.php for a
   description.  The *use_datetime* flag can be used to cause date/time values to
   be presented as :class:`datetime.datetime` objects; this is false by default.
   :class:`datetime.datetime` objects may be passed to calls.

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

   :class:`ServerProxy` instance methods take Python basic types and objects as
   arguments and return Python basic types and classes.  Types that are conformable
   (e.g. that can be marshalled through XML), include the following (and except
   where noted, they are unmarshalled as the same Python type):

   +---------------------------------+---------------------------------------------+
   | Name                            | Meaning                                     |
   +=================================+=============================================+
   | :const:`boolean`                | The :const:`True` and :const:`False`        |
   |                                 | constants                                   |
   +---------------------------------+---------------------------------------------+
   | :const:`integers`               | Pass in directly                            |
   +---------------------------------+---------------------------------------------+
   | :const:`floating-point numbers` | Pass in directly                            |
   +---------------------------------+---------------------------------------------+
   | :const:`strings`                | Pass in directly                            |
   +---------------------------------+---------------------------------------------+
   | :const:`arrays`                 | Any Python sequence type containing         |
   |                                 | conformable elements. Arrays are returned   |
   |                                 | as lists                                    |
   +---------------------------------+---------------------------------------------+
   | :const:`structures`             | A Python dictionary. Keys must be strings,  |
   |                                 | values may be any conformable type. Objects |
   |                                 | of user-defined classes can be passed in;   |
   |                                 | only their *__dict__* attribute is          |
   |                                 | transmitted.                                |
   +---------------------------------+---------------------------------------------+
   | :const:`dates`                  | in seconds since the epoch (pass in an      |
   |                                 | instance of the :class:`DateTime` class) or |
   |                                 | a :class:`datetime.datetime` instance.      |
   +---------------------------------+---------------------------------------------+
   | :const:`binary data`            | pass in an instance of the :class:`Binary`  |
   |                                 | wrapper class                               |
   +---------------------------------+---------------------------------------------+

   This is the full set of data types supported by XML-RPC.  Method calls may also
   raise a special :exc:`Fault` instance, used to signal XML-RPC server errors, or
   :exc:`ProtocolError` used to signal an error in the HTTP/HTTPS transport layer.
   Both :exc:`Fault` and :exc:`ProtocolError` derive from a base class called
   :exc:`Error`.  Note that even though starting with Python 2.2 you can subclass
   builtin types, the xmlrpclib module currently does not marshal instances of such
   subclasses.

   When passing strings, characters special to XML such as ``<``, ``>``, and ``&``
   will be automatically escaped.  However, it's the caller's responsibility to
   ensure that the string is free of characters that aren't allowed in XML, such as
   the control characters with ASCII values between 0 and 31 (except, of course,
   tab, newline and carriage return); failing to do this will result in an XML-RPC
   request that isn't well-formed XML.  If you have to pass arbitrary strings via
   XML-RPC, use the :class:`Binary` wrapper class described below.

   :class:`Server` is retained as an alias for :class:`ServerProxy` for backwards
   compatibility.  New code should use :class:`ServerProxy`.

   .. versionchanged:: 2.5
      The *use_datetime* flag was added.

   .. versionchanged:: 2.6
      Instances of :term:`new-style class`\es can be passed in if they have an
      *__dict__* attribute and don't have a base class that is marshalled in a
      special way.


.. seealso::

   `XML-RPC HOWTO <http://www.tldp.org/HOWTO/XML-RPC-HOWTO/index.html>`_
      A good description of XML-RPC operation and client software in several languages.
      Contains pretty much everything an XML-RPC client developer needs to know.

   `XML-RPC Introspection <http://xmlrpc-c.sourceforge.net/introspection.html>`_
      Describes the XML-RPC protocol extension for introspection.

   `XML-RPC Specification <http://www.xmlrpc.com/spec>`_
      The official specification.

   `Unofficial XML-RPC Errata <http://effbot.org/zone/xmlrpc-errata.htm>`_
      Fredrik Lundh's "unofficial errata, intended to clarify certain
      details in the XML-RPC specification, as well as hint at
      'best practices' to use when designing your own XML-RPC
      implementations."

.. _serverproxy-objects:

ServerProxy Objects
-------------------

A :class:`ServerProxy` instance has a method corresponding to each remote
procedure call accepted by the XML-RPC server.  Calling the method performs an
RPC, dispatched by both name and argument signature (e.g. the same method name
can be overloaded with multiple argument signatures).  The RPC finishes by
returning a value, which may be either returned data in a conformant type or a
:class:`Fault` or :class:`ProtocolError` object indicating an error.

Servers that support the XML introspection API support some common methods
grouped under the reserved :attr:`system` member:


.. method:: ServerProxy.system.listMethods()

   This method returns a list of strings, one for each (non-system) method
   supported by the XML-RPC server.


.. method:: ServerProxy.system.methodSignature(name)

   This method takes one parameter, the name of a method implemented by the XML-RPC
   server.It returns an array of possible signatures for this method. A signature
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
   that list.


.. method:: ServerProxy.system.methodHelp(name)

   This method takes one parameter, the name of a method implemented by the XML-RPC
   server.  It returns a documentation string describing the use of that method. If
   no such string is available, an empty string is returned. The documentation
   string may contain HTML markup.


.. _boolean-objects:

Boolean Objects
---------------

This class may be initialized from any Python value; the instance returned
depends only on its truth value.  It supports various Python operators through
:meth:`__cmp__`, :meth:`__repr__`, :meth:`__int__`, and :meth:`__nonzero__`
methods, all implemented in the obvious ways.

It also has the following method, supported mainly for internal use by the
unmarshalling code:


.. method:: Boolean.encode(out)

   Write the XML-RPC encoding of this Boolean item to the out stream object.

A working example follows. The server code::

   import xmlrpclib
   from SimpleXMLRPCServer import SimpleXMLRPCServer

   def is_even(n):
       return n%2 == 0

   server = SimpleXMLRPCServer(("localhost", 8000))
   print "Listening on port 8000..."
   server.register_function(is_even, "is_even")
   server.serve_forever()

The client code for the preceding server::

   import xmlrpclib

   proxy = xmlrpclib.ServerProxy("http://localhost:8000/")
   print "3 is even: %s" % str(proxy.is_even(3))
   print "100 is even: %s" % str(proxy.is_even(100))

.. _datetime-objects:

DateTime Objects
----------------

This class may be initialized with seconds since the epoch, a time
tuple, an ISO 8601 time/date string, or a :class:`datetime.datetime`
instance.  It has the following methods, supported mainly for internal
use by the marshalling/unmarshalling code:


.. method:: DateTime.decode(string)

   Accept a string as the instance's new time value.


.. method:: DateTime.encode(out)

   Write the XML-RPC encoding of this :class:`DateTime` item to the *out* stream
   object.

It also supports certain of Python's built-in operators through  :meth:`__cmp__`
and :meth:`__repr__` methods.

A working example follows. The server code::

   import datetime
   from SimpleXMLRPCServer import SimpleXMLRPCServer
   import xmlrpclib

   def today():
       today = datetime.datetime.today()
       return xmlrpclib.DateTime(today)

   server = SimpleXMLRPCServer(("localhost", 8000))
   print "Listening on port 8000..."
   server.register_function(today, "today")
   server.serve_forever()

The client code for the preceding server::

   import xmlrpclib
   import datetime

   proxy = xmlrpclib.ServerProxy("http://localhost:8000/")

   today = proxy.today()
   # convert the ISO8601 string to a datetime object
   converted = datetime.datetime.strptime(today.value, "%Y%m%dT%H:%M:%S")
   print "Today: %s" % converted.strftime("%d.%m.%Y, %H:%M")

.. _binary-objects:

Binary Objects
--------------

This class may be initialized from string data (which may include NULs). The
primary access to the content of a :class:`Binary` object is provided by an
attribute:


.. attribute:: Binary.data

   The binary data encapsulated by the :class:`Binary` instance.  The data is
   provided as an 8-bit string.

:class:`Binary` objects have the following methods, supported mainly for
internal use by the marshalling/unmarshalling code:


.. method:: Binary.decode(string)

   Accept a base64 string and decode it as the instance's new data.


.. method:: Binary.encode(out)

   Write the XML-RPC base 64 encoding of this binary item to the out stream object.

   The encoded data will have newlines every 76 characters as per
   `RFC 2045 section 6.8 <http://tools.ietf.org/html/rfc2045#section-6.8>`_,
   which was the de facto standard base64 specification when the
   XML-RPC spec was written.

It also supports certain of Python's built-in operators through a
:meth:`__cmp__` method.

Example usage of the binary objects.  We're going to transfer an image over
XMLRPC::

   from SimpleXMLRPCServer import SimpleXMLRPCServer
   import xmlrpclib

   def python_logo():
        handle = open("python_logo.jpg")
        return xmlrpclib.Binary(handle.read())
        handle.close()

   server = SimpleXMLRPCServer(("localhost", 8000))
   print "Listening on port 8000..."
   server.register_function(python_logo, 'python_logo')

   server.serve_forever()

The client gets the image and saves it to a file::

   import xmlrpclib

   proxy = xmlrpclib.ServerProxy("http://localhost:8000/")
   handle = open("fetched_python_logo.jpg", "w")
   handle.write(proxy.python_logo().data)
   handle.close()

.. _fault-objects:

Fault Objects
-------------

A :class:`Fault` object encapsulates the content of an XML-RPC fault tag. Fault
objects have the following members:


.. attribute:: Fault.faultCode

   A string indicating the fault type.


.. attribute:: Fault.faultString

   A string containing a diagnostic message associated with the fault.

In the following example we're going to intentionally cause a :exc:`Fault` by
returning a complex type object.  The server code::

   from SimpleXMLRPCServer import SimpleXMLRPCServer

   # A marshalling error is going to occur because we're returning a
   # complex number
   def add(x,y):
       return x+y+0j

   server = SimpleXMLRPCServer(("localhost", 8000))
   print "Listening on port 8000..."
   server.register_function(add, 'add')

   server.serve_forever()

The client code for the preceding server::

   import xmlrpclib

   proxy = xmlrpclib.ServerProxy("http://localhost:8000/")
   try:
       proxy.add(2, 5)
   except xmlrpclib.Fault, err:
       print "A fault occured"
       print "Fault code: %d" % err.faultCode
       print "Fault string: %s" % err.faultString



.. _protocol-error-objects:

ProtocolError Objects
---------------------

A :class:`ProtocolError` object describes a protocol error in the underlying
transport layer (such as a 404 'not found' error if the server named by the URI
does not exist).  It has the following members:


.. attribute:: ProtocolError.url

   The URI or URL that triggered the error.


.. attribute:: ProtocolError.errcode

   The error code.


.. attribute:: ProtocolError.errmsg

   The error message or diagnostic string.


.. attribute:: ProtocolError.headers

   A string containing the headers of the HTTP/HTTPS request that triggered the
   error.

In the following example we're going to intentionally cause a :exc:`ProtocolError`
by providing an invalid URI::

   import xmlrpclib

   # create a ServerProxy with an invalid URI
   proxy = xmlrpclib.ServerProxy("http://invalidaddress/")

   try:
       proxy.some_method()
   except xmlrpclib.ProtocolError, err:
       print "A protocol error occured"
       print "URL: %s" % err.url
       print "HTTP/HTTPS headers: %s" % err.headers
       print "Error code: %d" % err.errcode
       print "Error message: %s" % err.errmsg

MultiCall Objects
-----------------

.. versionadded:: 2.4

In http://www.xmlrpc.com/discuss/msgReader%241208, an approach is presented to
encapsulate multiple calls to a remote server into a single request.


.. class:: MultiCall(server)

   Create an object used to boxcar method calls. *server* is the eventual target of
   the call. Calls can be made to the result object, but they will immediately
   return ``None``, and only store the call name and parameters in the
   :class:`MultiCall` object. Calling the object itself causes all stored calls to
   be transmitted as a single ``system.multicall`` request. The result of this call
   is a :term:`generator`; iterating over this generator yields the individual
   results.

A usage example of this class follows.  The server code ::

   from SimpleXMLRPCServer import SimpleXMLRPCServer

   def add(x,y):
       return x+y

   def subtract(x, y):
       return x-y

   def multiply(x, y):
       return x*y

   def divide(x, y):
       return x/y

   # A simple server with simple arithmetic functions
   server = SimpleXMLRPCServer(("localhost", 8000))
   print "Listening on port 8000..."
   server.register_multicall_functions()
   server.register_function(add, 'add')
   server.register_function(subtract, 'subtract')
   server.register_function(multiply, 'multiply')
   server.register_function(divide, 'divide')
   server.serve_forever()

The client code for the preceding server::

   import xmlrpclib

   proxy = xmlrpclib.ServerProxy("http://localhost:8000/")
   multicall = xmlrpclib.MultiCall(proxy)
   multicall.add(7,3)
   multicall.subtract(7,3)
   multicall.multiply(7,3)
   multicall.divide(7,3)
   result = multicall()

   print "7+3=%d, 7-3=%d, 7*3=%d, 7/3=%d" % tuple(result)


Convenience Functions
---------------------


.. function:: boolean(value)

   Convert any Python value to one of the XML-RPC Boolean constants, ``True`` or
   ``False``.


.. function:: dumps(params[, methodname[,  methodresponse[, encoding[, allow_none]]]])

   Convert *params* into an XML-RPC request. or into a response if *methodresponse*
   is true. *params* can be either a tuple of arguments or an instance of the
   :exc:`Fault` exception class.  If *methodresponse* is true, only a single value
   can be returned, meaning that *params* must be of length 1. *encoding*, if
   supplied, is the encoding to use in the generated XML; the default is UTF-8.
   Python's :const:`None` value cannot be used in standard XML-RPC; to allow using
   it via an extension,  provide a true value for *allow_none*.


.. function:: loads(data[, use_datetime])

   Convert an XML-RPC request or response into Python objects, a ``(params,
   methodname)``.  *params* is a tuple of argument; *methodname* is a string, or
   ``None`` if no method name is present in the packet. If the XML-RPC packet
   represents a fault condition, this function will raise a :exc:`Fault` exception.
   The *use_datetime* flag can be used to cause date/time values to be presented as
   :class:`datetime.datetime` objects; this is false by default.

   .. versionchanged:: 2.5
      The *use_datetime* flag was added.


.. _xmlrpc-client-example:

Example of Client Usage
-----------------------

::

   # simple test program (from the XML-RPC specification)
   from xmlrpclib import ServerProxy, Error

   # server = ServerProxy("http://localhost:8000") # local server
   server = ServerProxy("http://betty.userland.com")

   print server

   try:
       print server.examples.getStateName(41)
   except Error, v:
       print "ERROR", v

To access an XML-RPC server through a proxy, you need to define  a custom
transport.  The following example shows how:

.. Example taken from http://lowlife.jp/nobonobo/wiki/xmlrpcwithproxy.html

::

   import xmlrpclib, httplib

   class ProxiedTransport(xmlrpclib.Transport):
       def set_proxy(self, proxy):
           self.proxy = proxy
       def make_connection(self, host):
           self.realhost = host
   	h = httplib.HTTP(self.proxy)
   	return h
       def send_request(self, connection, handler, request_body):
           connection.putrequest("POST", 'http://%s%s' % (self.realhost, handler))
       def send_host(self, connection, host):
           connection.putheader('Host', self.realhost)

   p = ProxiedTransport()
   p.set_proxy('proxy-server:8080')
   server = xmlrpclib.Server('http://time.xmlrpc.com/RPC2', transport=p)
   print server.currentTime.getCurrentTime()


Example of Client and Server Usage
----------------------------------

See :ref:`simplexmlrpcserver-example`.


