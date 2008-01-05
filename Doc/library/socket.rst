
:mod:`socket` --- Low-level networking interface
================================================

.. module:: socket
   :synopsis: Low-level networking interface.


This module provides access to the BSD *socket* interface. It is available on
all modern Unix systems, Windows, Mac OS X, BeOS, OS/2, and probably additional
platforms.

.. note::

   Some behavior may be platform dependent, since calls are made to the operating
   system socket APIs.

For an introduction to socket programming (in C), see the following papers: An
Introductory 4.3BSD Interprocess Communication Tutorial, by Stuart Sechrest and
An Advanced 4.3BSD Interprocess Communication Tutorial, by Samuel J.  Leffler et
al, both in the UNIX Programmer's Manual, Supplementary Documents 1 (sections
PS1:7 and PS1:8).  The platform-specific reference material for the various
socket-related system calls are also a valuable source of information on the
details of socket semantics.  For Unix, refer to the manual pages; for Windows,
see the WinSock (or Winsock 2) specification. For IPv6-ready APIs, readers may
want to refer to :rfc:`2553` titled Basic Socket Interface Extensions for IPv6.

.. index:: object: socket

The Python interface is a straightforward transliteration of the Unix system
call and library interface for sockets to Python's object-oriented style: the
:func:`socket` function returns a :dfn:`socket object` whose methods implement
the various socket system calls.  Parameter types are somewhat higher-level than
in the C interface: as with :meth:`read` and :meth:`write` operations on Python
files, buffer allocation on receive operations is automatic, and buffer length
is implicit on send operations.

Socket addresses are represented as follows: A single string is used for the
:const:`AF_UNIX` address family. A pair ``(host, port)`` is used for the
:const:`AF_INET` address family, where *host* is a string representing either a
hostname in Internet domain notation like ``'daring.cwi.nl'`` or an IPv4 address
like ``'100.50.200.5'``, and *port* is an integral port number. For
:const:`AF_INET6` address family, a four-tuple ``(host, port, flowinfo,
scopeid)`` is used, where *flowinfo* and *scopeid* represents ``sin6_flowinfo``
and ``sin6_scope_id`` member in :const:`struct sockaddr_in6` in C. For
:mod:`socket` module methods, *flowinfo* and *scopeid* can be omitted just for
backward compatibility. Note, however, omission of *scopeid* can cause problems
in manipulating scoped IPv6 addresses. Other address families are currently not
supported. The address format required by a particular socket object is
automatically selected based on the address family specified when the socket
object was created.

For IPv4 addresses, two special forms are accepted instead of a host address:
the empty string represents :const:`INADDR_ANY`, and the string
``'<broadcast>'`` represents :const:`INADDR_BROADCAST`. The behavior is not
available for IPv6 for backward compatibility, therefore, you may want to avoid
these if you intend to support IPv6 with your Python programs.

If you use a hostname in the *host* portion of IPv4/v6 socket address, the
program may show a nondeterministic behavior, as Python uses the first address
returned from the DNS resolution.  The socket address will be resolved
differently into an actual IPv4/v6 address, depending on the results from DNS
resolution and/or the host configuration.  For deterministic behavior use a
numeric address in *host* portion.

.. versionadded:: 2.5
   AF_NETLINK sockets are represented as  pairs ``pid, groups``.

All errors raise exceptions.  The normal exceptions for invalid argument types
and out-of-memory conditions can be raised; errors related to socket or address
semantics raise the error :exc:`socket.error`.

Non-blocking mode is supported through :meth:`setblocking`.  A generalization of
this based on timeouts is supported through :meth:`settimeout`.

The module :mod:`socket` exports the following constants and functions:


.. exception:: error

   .. index:: module: errno

   This exception is raised for socket-related errors. The accompanying value is
   either a string telling what went wrong or a pair ``(errno, string)``
   representing an error returned by a system call, similar to the value
   accompanying :exc:`os.error`. See the module :mod:`errno`, which contains names
   for the error codes defined by the underlying operating system.

   .. versionchanged:: 2.6
      :exc:`socket.error` is now a child class of :exc:`IOError`.


.. exception:: herror

   This exception is raised for address-related errors, i.e. for functions that use
   *h_errno* in the C API, including :func:`gethostbyname_ex` and
   :func:`gethostbyaddr`.

   The accompanying value is a pair ``(h_errno, string)`` representing an error
   returned by a library call. *string* represents the description of *h_errno*, as
   returned by the :cfunc:`hstrerror` C function.


.. exception:: gaierror

   This exception is raised for address-related errors, for :func:`getaddrinfo` and
   :func:`getnameinfo`. The accompanying value is a pair ``(error, string)``
   representing an error returned by a library call. *string* represents the
   description of *error*, as returned by the :cfunc:`gai_strerror` C function. The
   *error* value will match one of the :const:`EAI_\*` constants defined in this
   module.


.. exception:: timeout

   This exception is raised when a timeout occurs on a socket which has had
   timeouts enabled via a prior call to :meth:`settimeout`.  The accompanying value
   is a string whose value is currently always "timed out".

   .. versionadded:: 2.3


.. data:: AF_UNIX
          AF_INET
          AF_INET6

   These constants represent the address (and protocol) families, used for the
   first argument to :func:`socket`.  If the :const:`AF_UNIX` constant is not
   defined then this protocol is unsupported.


.. data:: SOCK_STREAM
          SOCK_DGRAM
          SOCK_RAW
          SOCK_RDM
          SOCK_SEQPACKET

   These constants represent the socket types, used for the second argument to
   :func:`socket`. (Only :const:`SOCK_STREAM` and :const:`SOCK_DGRAM` appear to be
   generally useful.)


.. data:: SO_*
          SOMAXCONN
          MSG_*
          SOL_*
          IPPROTO_*
          IPPORT_*
          INADDR_*
          IP_*
          IPV6_*
          EAI_*
          AI_*
          NI_*
          TCP_*

   Many constants of these forms, documented in the Unix documentation on sockets
   and/or the IP protocol, are also defined in the socket module. They are
   generally used in arguments to the :meth:`setsockopt` and :meth:`getsockopt`
   methods of socket objects.  In most cases, only those symbols that are defined
   in the Unix header files are defined; for a few symbols, default values are
   provided.

.. data:: SIO_*
          RCVALL_*
          
   Constants for Windows' WSAIoctl(). The constants are used as arguments to the
   :meth:`ioctl` method of socket objects.
   
   .. versionadded:: 2.6


.. data:: has_ipv6

   This constant contains a boolean value which indicates if IPv6 is supported on
   this platform.

   .. versionadded:: 2.3


.. function:: create_connection(address[, timeout])

   Connects to the *address* received (as usual, a ``(host, port)`` pair), with an
   optional timeout for the connection.  Especially useful for higher-level
   protocols, it is not normally used directly from application-level code.
   Passing the optional *timeout* parameter will set the timeout on the socket
   instance (if it is not given or ``None``, the global default timeout setting is
   used).

   .. versionadded:: 2.6


.. function:: getaddrinfo(host, port[, family[, socktype[, proto[, flags]]]])

   Resolves the *host*/*port* argument, into a sequence of 5-tuples that contain
   all the necessary argument for the sockets manipulation. *host* is a domain
   name, a string representation of IPv4/v6 address or ``None``. *port* is a string
   service name (like ``'http'``), a numeric port number or ``None``.

   The rest of the arguments are optional and must be numeric if specified.  For
   *host* and *port*, by passing either an empty string or ``None``, you can pass
   ``NULL`` to the C API.  The :func:`getaddrinfo` function returns a list of
   5-tuples with the following structure:

   ``(family, socktype, proto, canonname, sockaddr)``

   *family*, *socktype*, *proto* are all integer and are meant to be passed to the
   :func:`socket` function. *canonname* is a string representing the canonical name
   of the *host*. It can be a numeric IPv4/v6 address when :const:`AI_CANONNAME` is
   specified for a numeric *host*. *sockaddr* is a tuple describing a socket
   address, as described above. See the source for :mod:`socket` and other
   library modules for a typical usage of the function.

   .. versionadded:: 2.2


.. function:: getfqdn([name])

   Return a fully qualified domain name for *name*. If *name* is omitted or empty,
   it is interpreted as the local host.  To find the fully qualified name, the
   hostname returned by :func:`gethostbyaddr` is checked, then aliases for the
   host, if available.  The first name which includes a period is selected.  In
   case no fully qualified domain name is available, the hostname as returned by
   :func:`gethostname` is returned.

   .. versionadded:: 2.0


.. function:: gethostbyname(hostname)

   Translate a host name to IPv4 address format.  The IPv4 address is returned as a
   string, such as  ``'100.50.200.5'``.  If the host name is an IPv4 address itself
   it is returned unchanged.  See :func:`gethostbyname_ex` for a more complete
   interface. :func:`gethostbyname` does not support IPv6 name resolution, and
   :func:`getaddrinfo` should be used instead for IPv4/v6 dual stack support.


.. function:: gethostbyname_ex(hostname)

   Translate a host name to IPv4 address format, extended interface. Return a
   triple ``(hostname, aliaslist, ipaddrlist)`` where *hostname* is the primary
   host name responding to the given *ip_address*, *aliaslist* is a (possibly
   empty) list of alternative host names for the same address, and *ipaddrlist* is
   a list of IPv4 addresses for the same interface on the same host (often but not
   always a single address). :func:`gethostbyname_ex` does not support IPv6 name
   resolution, and :func:`getaddrinfo` should be used instead for IPv4/v6 dual
   stack support.


.. function:: gethostname()

   Return a string containing the hostname of the machine where  the Python
   interpreter is currently executing. If you want to know the current machine's IP
   address, you may want to use ``gethostbyname(gethostname())``. This operation
   assumes that there is a valid address-to-host mapping for the host, and the
   assumption does not always hold. Note: :func:`gethostname` doesn't always return
   the fully qualified domain name; use ``getfqdn()`` (see above).


.. function:: gethostbyaddr(ip_address)

   Return a triple ``(hostname, aliaslist, ipaddrlist)`` where *hostname* is the
   primary host name responding to the given *ip_address*, *aliaslist* is a
   (possibly empty) list of alternative host names for the same address, and
   *ipaddrlist* is a list of IPv4/v6 addresses for the same interface on the same
   host (most likely containing only a single address). To find the fully qualified
   domain name, use the function :func:`getfqdn`. :func:`gethostbyaddr` supports
   both IPv4 and IPv6.


.. function:: getnameinfo(sockaddr, flags)

   Translate a socket address *sockaddr* into a 2-tuple ``(host, port)``. Depending
   on the settings of *flags*, the result can contain a fully-qualified domain name
   or numeric address representation in *host*.  Similarly, *port* can contain a
   string port name or a numeric port number.

   .. versionadded:: 2.2


.. function:: getprotobyname(protocolname)

   Translate an Internet protocol name (for example, ``'icmp'``) to a constant
   suitable for passing as the (optional) third argument to the :func:`socket`
   function.  This is usually only needed for sockets opened in "raw" mode
   (:const:`SOCK_RAW`); for the normal socket modes, the correct protocol is chosen
   automatically if the protocol is omitted or zero.


.. function:: getservbyname(servicename[, protocolname])

   Translate an Internet service name and protocol name to a port number for that
   service.  The optional protocol name, if given, should be ``'tcp'`` or
   ``'udp'``, otherwise any protocol will match.


.. function:: getservbyport(port[, protocolname])

   Translate an Internet port number and protocol name to a service name for that
   service.  The optional protocol name, if given, should be ``'tcp'`` or
   ``'udp'``, otherwise any protocol will match.


.. function:: socket([family[, type[, proto]]])

   Create a new socket using the given address family, socket type and protocol
   number.  The address family should be :const:`AF_INET` (the default),
   :const:`AF_INET6` or :const:`AF_UNIX`.  The socket type should be
   :const:`SOCK_STREAM` (the default), :const:`SOCK_DGRAM` or perhaps one of the
   other ``SOCK_`` constants.  The protocol number is usually zero and may be
   omitted in that case.


.. function:: socketpair([family[, type[, proto]]])

   Build a pair of connected socket objects using the given address family, socket
   type, and protocol number.  Address family, socket type, and protocol number are
   as for the :func:`socket` function above. The default family is :const:`AF_UNIX`
   if defined on the platform; otherwise, the default is :const:`AF_INET`.
   Availability: Unix.

   .. versionadded:: 2.4


.. function:: fromfd(fd, family, type[, proto])

   Duplicate the file descriptor *fd* (an integer as returned by a file object's
   :meth:`fileno` method) and build a socket object from the result.  Address
   family, socket type and protocol number are as for the :func:`socket` function
   above. The file descriptor should refer to a socket, but this is not checked ---
   subsequent operations on the object may fail if the file descriptor is invalid.
   This function is rarely needed, but can be used to get or set socket options on
   a socket passed to a program as standard input or output (such as a server
   started by the Unix inet daemon).  The socket is assumed to be in blocking mode.
   Availability: Unix.


.. function:: ntohl(x)

   Convert 32-bit positive integers from network to host byte order.  On machines
   where the host byte order is the same as network byte order, this is a no-op;
   otherwise, it performs a 4-byte swap operation.


.. function:: ntohs(x)

   Convert 16-bit positive integers from network to host byte order.  On machines
   where the host byte order is the same as network byte order, this is a no-op;
   otherwise, it performs a 2-byte swap operation.


.. function:: htonl(x)

   Convert 32-bit positive integers from host to network byte order.  On machines
   where the host byte order is the same as network byte order, this is a no-op;
   otherwise, it performs a 4-byte swap operation.


.. function:: htons(x)

   Convert 16-bit positive integers from host to network byte order.  On machines
   where the host byte order is the same as network byte order, this is a no-op;
   otherwise, it performs a 2-byte swap operation.


.. function:: inet_aton(ip_string)

   Convert an IPv4 address from dotted-quad string format (for example,
   '123.45.67.89') to 32-bit packed binary format, as a string four characters in
   length.  This is useful when conversing with a program that uses the standard C
   library and needs objects of type :ctype:`struct in_addr`, which is the C type
   for the 32-bit packed binary this function returns.

   If the IPv4 address string passed to this function is invalid,
   :exc:`socket.error` will be raised. Note that exactly what is valid depends on
   the underlying C implementation of :cfunc:`inet_aton`.

   :func:`inet_aton` does not support IPv6, and :func:`getnameinfo` should be used
   instead for IPv4/v6 dual stack support.


.. function:: inet_ntoa(packed_ip)

   Convert a 32-bit packed IPv4 address (a string four characters in length) to its
   standard dotted-quad string representation (for example, '123.45.67.89').  This
   is useful when conversing with a program that uses the standard C library and
   needs objects of type :ctype:`struct in_addr`, which is the C type for the
   32-bit packed binary data this function takes as an argument.

   If the string passed to this function is not exactly 4 bytes in length,
   :exc:`socket.error` will be raised. :func:`inet_ntoa` does not support IPv6, and
   :func:`getnameinfo` should be used instead for IPv4/v6 dual stack support.


.. function:: inet_pton(address_family, ip_string)

   Convert an IP address from its family-specific string format to a packed, binary
   format. :func:`inet_pton` is useful when a library or network protocol calls for
   an object of type :ctype:`struct in_addr` (similar to :func:`inet_aton`) or
   :ctype:`struct in6_addr`.

   Supported values for *address_family* are currently :const:`AF_INET` and
   :const:`AF_INET6`. If the IP address string *ip_string* is invalid,
   :exc:`socket.error` will be raised. Note that exactly what is valid depends on
   both the value of *address_family* and the underlying implementation of
   :cfunc:`inet_pton`.

   Availability: Unix (maybe not all platforms).

   .. versionadded:: 2.3


.. function:: inet_ntop(address_family, packed_ip)

   Convert a packed IP address (a string of some number of characters) to its
   standard, family-specific string representation (for example, ``'7.10.0.5'`` or
   ``'5aef:2b::8'``) :func:`inet_ntop` is useful when a library or network protocol
   returns an object of type :ctype:`struct in_addr` (similar to :func:`inet_ntoa`)
   or :ctype:`struct in6_addr`.

   Supported values for *address_family* are currently :const:`AF_INET` and
   :const:`AF_INET6`. If the string *packed_ip* is not the correct length for the
   specified address family, :exc:`ValueError` will be raised.  A
   :exc:`socket.error` is raised for errors from the call to :func:`inet_ntop`.

   Availability: Unix (maybe not all platforms).

   .. versionadded:: 2.3


.. function:: getdefaulttimeout()

   Return the default timeout in floating seconds for new socket objects. A value
   of ``None`` indicates that new socket objects have no timeout. When the socket
   module is first imported, the default is ``None``.

   .. versionadded:: 2.3


.. function:: setdefaulttimeout(timeout)

   Set the default timeout in floating seconds for new socket objects. A value of
   ``None`` indicates that new socket objects have no timeout. When the socket
   module is first imported, the default is ``None``.

   .. versionadded:: 2.3


.. data:: SocketType

   This is a Python type object that represents the socket object type. It is the
   same as ``type(socket(...))``.


.. seealso::

   Module :mod:`SocketServer`
      Classes that simplify writing network servers.


.. _socket-objects:

Socket Objects
--------------

Socket objects have the following methods.  Except for :meth:`makefile` these
correspond to Unix system calls applicable to sockets.


.. method:: socket.accept()

   Accept a connection. The socket must be bound to an address and listening for
   connections. The return value is a pair ``(conn, address)`` where *conn* is a
   *new* socket object usable to send and receive data on the connection, and
   *address* is the address bound to the socket on the other end of the connection.


.. method:: socket.bind(address)

   Bind the socket to *address*.  The socket must not already be bound. (The format
   of *address* depends on the address family --- see above.)

   .. note::

      This method has historically accepted a pair of parameters for :const:`AF_INET`
      addresses instead of only a tuple.  This was never intentional and is no longer
      available in Python 2.0 and later.


.. method:: socket.close()

   Close the socket.  All future operations on the socket object will fail. The
   remote end will receive no more data (after queued data is flushed). Sockets are
   automatically closed when they are garbage-collected.


.. method:: socket.connect(address)

   Connect to a remote socket at *address*. (The format of *address* depends on the
   address family --- see above.)

   .. note::

      This method has historically accepted a pair of parameters for :const:`AF_INET`
      addresses instead of only a tuple.  This was never intentional and is no longer
      available in Python 2.0 and later.


.. method:: socket.connect_ex(address)

   Like ``connect(address)``, but return an error indicator instead of raising an
   exception for errors returned by the C-level :cfunc:`connect` call (other
   problems, such as "host not found," can still raise exceptions).  The error
   indicator is ``0`` if the operation succeeded, otherwise the value of the
   :cdata:`errno` variable.  This is useful to support, for example, asynchronous
   connects.

   .. note::

      This method has historically accepted a pair of parameters for :const:`AF_INET`
      addresses instead of only a tuple. This was never intentional and is no longer
      available in Python 2.0 and later.


.. method:: socket.fileno()

   Return the socket's file descriptor (a small integer).  This is useful with
   :func:`select.select`.

   Under Windows the small integer returned by this method cannot be used where a
   file descriptor can be used (such as :func:`os.fdopen`).  Unix does not have
   this limitation.


.. method:: socket.getpeername()

   Return the remote address to which the socket is connected.  This is useful to
   find out the port number of a remote IPv4/v6 socket, for instance. (The format
   of the address returned depends on the address family --- see above.)  On some
   systems this function is not supported.


.. method:: socket.getsockname()

   Return the socket's own address.  This is useful to find out the port number of
   an IPv4/v6 socket, for instance. (The format of the address returned depends on
   the address family --- see above.)


.. method:: socket.getsockopt(level, optname[, buflen])

   Return the value of the given socket option (see the Unix man page
   :manpage:`getsockopt(2)`).  The needed symbolic constants (:const:`SO_\*` etc.)
   are defined in this module.  If *buflen* is absent, an integer option is assumed
   and its integer value is returned by the function.  If *buflen* is present, it
   specifies the maximum length of the buffer used to receive the option in, and
   this buffer is returned as a string.  It is up to the caller to decode the
   contents of the buffer (see the optional built-in module :mod:`struct` for a way
   to decode C structures encoded as strings).

   
.. method:: socket.ioctl(control, option)

   :platform: Windows 
   
   The `meth:ioctl` method is a limited interface to the WSAIoctl system
   interface. Please refer to the MSDN documentation for more information.
   
   .. versionadded:: 2.6


.. method:: socket.listen(backlog)

   Listen for connections made to the socket.  The *backlog* argument specifies the
   maximum number of queued connections and should be at least 1; the maximum value
   is system-dependent (usually 5).


.. method:: socket.makefile([mode[, bufsize]])

   .. index:: single: I/O control; buffering

   Return a :dfn:`file object` associated with the socket.  (File objects are
   described in :ref:`bltin-file-objects`.) The file object
   references a :cfunc:`dup`\ ped version of the socket file descriptor, so the
   file object and socket object may be closed or garbage-collected independently.
   The socket must be in blocking mode (it can not have a timeout). The optional
   *mode* and *bufsize* arguments are interpreted the same way as by the built-in
   :func:`file` function.


.. method:: socket.recv(bufsize[, flags])

   Receive data from the socket.  The return value is a string representing the
   data received.  The maximum amount of data to be received at once is specified
   by *bufsize*.  See the Unix manual page :manpage:`recv(2)` for the meaning of
   the optional argument *flags*; it defaults to zero.

   .. note::

      For best match with hardware and network realities, the value of  *bufsize*
      should be a relatively small power of 2, for example, 4096.


.. method:: socket.recvfrom(bufsize[, flags])

   Receive data from the socket.  The return value is a pair ``(string, address)``
   where *string* is a string representing the data received and *address* is the
   address of the socket sending the data.  See the Unix manual page
   :manpage:`recv(2)` for the meaning of the optional argument *flags*; it defaults
   to zero. (The format of *address* depends on the address family --- see above.)


.. method:: socket.recvfrom_into(buffer[, nbytes[, flags]])

   Receive data from the socket, writing it into *buffer* instead of  creating a
   new string.  The return value is a pair ``(nbytes, address)`` where *nbytes* is
   the number of bytes received and *address* is the address of the socket sending
   the data.  See the Unix manual page :manpage:`recv(2)` for the meaning of the
   optional argument *flags*; it defaults to zero.  (The format of *address*
   depends on the address family --- see above.)

   .. versionadded:: 2.5


.. method:: socket.recv_into(buffer[, nbytes[, flags]])

   Receive up to *nbytes* bytes from the socket, storing the data into a buffer
   rather than creating a new string.     If *nbytes* is not specified (or 0),
   receive up to the size available in the given buffer. See the Unix manual page
   :manpage:`recv(2)` for the meaning of the optional argument *flags*; it defaults
   to zero.

   .. versionadded:: 2.5


.. method:: socket.send(string[, flags])

   Send data to the socket.  The socket must be connected to a remote socket.  The
   optional *flags* argument has the same meaning as for :meth:`recv` above.
   Returns the number of bytes sent. Applications are responsible for checking that
   all data has been sent; if only some of the data was transmitted, the
   application needs to attempt delivery of the remaining data.


.. method:: socket.sendall(string[, flags])

   Send data to the socket.  The socket must be connected to a remote socket.  The
   optional *flags* argument has the same meaning as for :meth:`recv` above.
   Unlike :meth:`send`, this method continues to send data from *string* until
   either all data has been sent or an error occurs.  ``None`` is returned on
   success.  On error, an exception is raised, and there is no way to determine how
   much data, if any, was successfully sent.


.. method:: socket.sendto(string[, flags], address)

   Send data to the socket.  The socket should not be connected to a remote socket,
   since the destination socket is specified by *address*.  The optional *flags*
   argument has the same meaning as for :meth:`recv` above.  Return the number of
   bytes sent. (The format of *address* depends on the address family --- see
   above.)


.. method:: socket.setblocking(flag)

   Set blocking or non-blocking mode of the socket: if *flag* is 0, the socket is
   set to non-blocking, else to blocking mode.  Initially all sockets are in
   blocking mode.  In non-blocking mode, if a :meth:`recv` call doesn't find any
   data, or if a :meth:`send` call can't immediately dispose of the data, a
   :exc:`error` exception is raised; in blocking mode, the calls block until they
   can proceed. ``s.setblocking(0)`` is equivalent to ``s.settimeout(0)``;
   ``s.setblocking(1)`` is equivalent to ``s.settimeout(None)``.


.. method:: socket.settimeout(value)

   Set a timeout on blocking socket operations.  The *value* argument can be a
   nonnegative float expressing seconds, or ``None``. If a float is given,
   subsequent socket operations will raise an :exc:`timeout` exception if the
   timeout period *value* has elapsed before the operation has completed.  Setting
   a timeout of ``None`` disables timeouts on socket operations.
   ``s.settimeout(0.0)`` is equivalent to ``s.setblocking(0)``;
   ``s.settimeout(None)`` is equivalent to ``s.setblocking(1)``.

   .. versionadded:: 2.3


.. method:: socket.gettimeout()

   Return the timeout in floating seconds associated with socket operations, or
   ``None`` if no timeout is set.  This reflects the last call to
   :meth:`setblocking` or :meth:`settimeout`.

   .. versionadded:: 2.3

Some notes on socket blocking and timeouts: A socket object can be in one of
three modes: blocking, non-blocking, or timeout.  Sockets are always created in
blocking mode.  In blocking mode, operations block until complete.  In
non-blocking mode, operations fail (with an error that is unfortunately
system-dependent) if they cannot be completed immediately.  In timeout mode,
operations fail if they cannot be completed within the timeout specified for the
socket.  The :meth:`setblocking` method is simply a shorthand for certain
:meth:`settimeout` calls.

Timeout mode internally sets the socket in non-blocking mode.  The blocking and
timeout modes are shared between file descriptors and socket objects that refer
to the same network endpoint.  A consequence of this is that file objects
returned by the :meth:`makefile` method must only be used when the socket is in
blocking mode; in timeout or non-blocking mode file operations that cannot be
completed immediately will fail.

Note that the :meth:`connect` operation is subject to the timeout setting, and
in general it is recommended to call :meth:`settimeout` before calling
:meth:`connect`.


.. method:: socket.setsockopt(level, optname, value)

   .. index:: module: struct

   Set the value of the given socket option (see the Unix manual page
   :manpage:`setsockopt(2)`).  The needed symbolic constants are defined in the
   :mod:`socket` module (:const:`SO_\*` etc.).  The value can be an integer or a
   string representing a buffer.  In the latter case it is up to the caller to
   ensure that the string contains the proper bits (see the optional built-in
   module :mod:`struct` for a way to encode C structures as strings).


.. method:: socket.shutdown(how)

   Shut down one or both halves of the connection.  If *how* is :const:`SHUT_RD`,
   further receives are disallowed.  If *how* is :const:`SHUT_WR`, further sends
   are disallowed.  If *how* is :const:`SHUT_RDWR`, further sends and receives are
   disallowed.

Note that there are no methods :meth:`read` or :meth:`write`; use :meth:`recv`
and :meth:`send` without *flags* argument instead.

Socket objects also have these (read-only) attributes that correspond to the
values given to the :class:`socket` constructor.


.. attribute:: socket.family

   The socket family.

   .. versionadded:: 2.5


.. attribute:: socket.type

   The socket type.

   .. versionadded:: 2.5


.. attribute:: socket.proto

   The socket protocol.

   .. versionadded:: 2.5


.. _socket-example:

Example
-------

Here are four minimal example programs using the TCP/IP protocol: a server that
echoes all data that it receives back (servicing only one client), and a client
using it.  Note that a server must perform the sequence :func:`socket`,
:meth:`bind`, :meth:`listen`, :meth:`accept` (possibly repeating the
:meth:`accept` to service more than one client), while a client only needs the
sequence :func:`socket`, :meth:`connect`.  Also note that the server does not
:meth:`send`/:meth:`recv` on the  socket it is listening on but on the new
socket returned by :meth:`accept`.

The first two examples support IPv4 only. ::

   # Echo server program
   import socket

   HOST = ''                 # Symbolic name meaning the local host
   PORT = 50007              # Arbitrary non-privileged port
   s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
   s.bind((HOST, PORT))
   s.listen(1)
   conn, addr = s.accept()
   print 'Connected by', addr
   while 1:
       data = conn.recv(1024)
       if not data: break
       conn.send(data)
   conn.close()

::

   # Echo client program
   import socket

   HOST = 'daring.cwi.nl'    # The remote host
   PORT = 50007              # The same port as used by the server
   s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
   s.connect((HOST, PORT))
   s.send('Hello, world')
   data = s.recv(1024)
   s.close()
   print 'Received', repr(data)

The next two examples are identical to the above two, but support both IPv4 and
IPv6. The server side will listen to the first address family available (it
should listen to both instead). On most of IPv6-ready systems, IPv6 will take
precedence and the server may not accept IPv4 traffic. The client side will try
to connect to the all addresses returned as a result of the name resolution, and
sends traffic to the first one connected successfully. ::

   # Echo server program
   import socket
   import sys

   HOST = ''                 # Symbolic name meaning the local host
   PORT = 50007              # Arbitrary non-privileged port
   s = None
   for res in socket.getaddrinfo(HOST, PORT, socket.AF_UNSPEC, socket.SOCK_STREAM, 0, socket.AI_PASSIVE):
       af, socktype, proto, canonname, sa = res
       try:
   	s = socket.socket(af, socktype, proto)
       except socket.error, msg:
   	s = None
   	continue
       try:
   	s.bind(sa)
   	s.listen(1)
       except socket.error, msg:
   	s.close()
   	s = None
   	continue
       break
   if s is None:
       print 'could not open socket'
       sys.exit(1)
   conn, addr = s.accept()
   print 'Connected by', addr
   while 1:
       data = conn.recv(1024)
       if not data: break
       conn.send(data)
   conn.close()

::

   # Echo client program
   import socket
   import sys

   HOST = 'daring.cwi.nl'    # The remote host
   PORT = 50007              # The same port as used by the server
   s = None
   for res in socket.getaddrinfo(HOST, PORT, socket.AF_UNSPEC, socket.SOCK_STREAM):
       af, socktype, proto, canonname, sa = res
       try:
   	s = socket.socket(af, socktype, proto)
       except socket.error, msg:
   	s = None
   	continue
       try:
   	s.connect(sa)
       except socket.error, msg:
   	s.close()
   	s = None
   	continue
       break
   if s is None:
       print 'could not open socket'
       sys.exit(1)
   s.send('Hello, world')
   data = s.recv(1024)
   s.close()
   print 'Received', repr(data)

   
The last example shows how to write a very simple network sniffer with raw
sockets on Windows. The example requires administrator priviliges to modify
the interface::

   import socket

   # the public network interface
   HOST = socket.gethostbyname(socket.gethostname())
   
   # create a raw socket and bind it to the public interface
   s = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_IP)
   s.bind((HOST, 0))
   
   # Include IP headers
   s.setsockopt(socket.IPPROTO_IP, socket.IP_HDRINCL, 1)
   
   # receive all packages
   s.ioctl(socket.SIO_RCVALL, socket.RCVALL_ON)
   
   # receive a package
   print s.recvfrom(65565)
   
   # disabled promiscous mode
   s.ioctl(socket.SIO_RCVALL, socket.RCVALL_OFF)
