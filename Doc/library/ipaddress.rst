:mod:`ipaddress` --- IPv4/IPv6 manipulation library
===================================================

.. module:: ipaddress
   :synopsis: IPv4/IPv6 manipulation library.
.. moduleauthor:: Peter Moody

**Source code:** :source:`Lib/ipaddress.py`

--------------

The :mod:`ipaddress` module provides the capabilities to create, manipulate and
operate on IPv4 and IPv6 addresses and networks.

This is the full module API reference - for an overview and introduction,
see :ref:`ipaddress-howto`.

The functions and classes in this module make it straightforward to handle
various tasks related to IP addresses, including checking whether or not two
hosts are on the same subnet, iterating over all hosts in a particular
subnet, as well as checking whether or not a string represents a valid
IP address or network definition.


Convenience factory functions
-----------------------------

The :mod:`ipaddress` module provides factory functions to conveniently create
IP addresses, networks and interfaces:

.. function:: ip_address(address)

   Return an :class:`IPv4Address` or :class:`IPv6Address` object depending on
   the IP address passed as argument.  Either IPv4 or IPv6 addresses may be
   supplied; integers less than 2**32 will be considered to be IPv4 by default.
   A :exc:`ValueError` is raised if *address* does not represent a valid IPv4 or
   IPv6 address.

   >>> ipaddress.ip_address('192.168.0.1')
   IPv4Address('192.168.0.1')
   >>> ipaddress.ip_address('2001:db8::')
   IPv6Address('2001:db8::')


.. function:: ip_network(address, strict=True)

   Return an :class:`IPv4Network` or :class:`IPv6Network` object depending on
   the IP address passed as argument.  *address* is a string or integer
   representing the IP network.  Either IPv4 or IPv6 networks may be supplied;
   integers less than 2**32 will be considered to be IPv4 by default.  *strict*
   is passed to :class:`IPv4Network` or :class:`IPv6Network` constructor.  A
   :exc:`ValueError` is raised if *address* does not represent a valid IPv4 or
   IPv6 address, or if the network has host bits set.

   >>> ipaddress.ip_network('192.168.0.0/28')
   IPv4Network('192.168.0.0/28')


.. function:: ip_interface(address)

   Return an :class:`IPv4Interface` or :class:`IPv6Interface` object depending
   on the IP address passed as argument.  *address* is a string or integer
   representing the IP address.  Either IPv4 or IPv6 addresses may be supplied;
   integers less than 2**32 will be considered to be IPv4 by default.  A
   :exc:`ValueError` is raised if *address* does not represent a valid IPv4 or
   IPv6 address.


Address objects
---------------

The :class:`IPv4Address` and :class:`IPv6Address` objects share a lot of common
attributes.  Some attributes that are only meaningful for IPv6 addresses are
also implemented by :class:`IPv4Address` objects, in order to make it easier to
write code that handles both IP versions correctly.  To avoid duplication, all
common attributes will only be documented for :class:`IPv4Address`.

.. class:: IPv4Address(address)

   Construct an IPv4 address.  An :exc:`AddressValueError` is raised if
   *address* is not a valid IPv4 address.

   The following constitutes a valid IPv4 address:

   1. A string in decimal-dot notation, consisting of four decimal integers in
      the inclusive range 0-255, separated by dots (e.g. ``192.168.0.1``). Each
      integer represents an octet (byte) in the address, big-endian.
   2. An integer that fits into 32 bits.
   3. An integer packed into a :class:`bytes` object of length 4, big-endian.

   >>> ipaddress.IPv4Address('192.168.0.1')
   IPv4Address('192.168.0.1')
   >>> ipaddress.IPv4Address('192.0.2.1') == ipaddress.IPv4Address(3221225985)
   True

   .. attribute:: exploded

   The longhand version of the address as a string. Note: the
   exploded/compressed distinction is meaningful only for IPv6 addresses.
   For IPv4 addresses it is the same.

   .. attribute:: compressed

   The shorthand version of the address as a string.

   .. attribute:: packed

   The binary representation of this address - a :class:`bytes` object.

   .. attribute:: version

   A numeric version number.

   .. attribute:: max_prefixlen

   Maximal length of the prefix (in bits).  The prefix defines the number of
   leading bits in an address that are compared to determine whether or not an
   address is part of a network.

   .. attribute:: is_multicast

   ``True`` if the address is reserved for multicast use.  See :RFC:`3171` (for
   IPv4) or :RFC:`2373` (for IPv6).

   .. attribute:: is_private

   ``True`` if the address is allocated for private networks.  See :RFC:`1918`
   (for IPv4) or :RFC:`4193` (for IPv6).

   .. attribute:: is_unspecified

   ``True`` if the address is unspecified.  See :RFC:`5375` (for IPv4) or
   :RFC:`2373` (for IPv6).

   .. attribute:: is_reserved

   ``True`` if the address is otherwise IETF reserved.

   .. attribute:: is_loopback

   ``True`` if this is a loopback address.  See :RFC:`3330` (for IPv4) or
   :RFC:`2373` (for IPv6).

   .. attribute:: is_link_local

   ``True`` if the address is reserved for link-local.  See :RFC:`3927`.

.. class:: IPv6Address(address)

   Construct an IPv6 address.  An :exc:`AddressValueError` is raised if
   *address* is not a valid IPv6 address.

   The following constitutes a valid IPv6 address:

   1. A string consisting of eight groups of four hexadecimal digits, each
      group representing 16 bits.  The groups are separated by colons.
      This describes an *exploded* (longhand) notation.  The string can
      also be *compressed* (shorthand notation) by various means.  See
      :RFC:`4291` for details.  For example,
      ``"0000:0000:0000:0000:0000:0abc:0007:0def"`` can be compressed to
      ``"::abc:7:def"``.
   2. An integer that fits into 128 bits.
   3. An integer packed into a :class:`bytes` object of length 16, big-endian.

   >>> ipaddress.IPv6Address('2001:db8::1000')
   IPv6Address('2001:db8::1000')

   All the attributes exposed by :class:`IPv4Address` are supported.  In
   addition, the following attributs are exposed only by :class:`IPv6Address`.

   .. attribute:: is_site_local

   ``True`` if the address is reserved for site-local.  Note that the site-local
   address space has been deprecated by :RFC:`3879`.  Use
   :attr:`~IPv4Address.is_private` to test if this address is in the space of
   unique local addresses as defined by :RFC:`4193`.

   .. attribute:: ipv4_mapped

   If this address represents a IPv4 mapped address, return the IPv4 mapped
   address.  Otherwise return ``None``.

   .. attribute:: teredo

   If this address appears to be a teredo address (starts with ``2001::/32``),
   return a tuple of embedded teredo IPs ``(server, client)`` pairs.  Otherwise
   return ``None``.

   .. attribute:: sixtofour

   If this address appears to contain a 6to4 embedded address, return the
   embedded IPv4 address.  Otherwise return ``None``.


Operators
^^^^^^^^^

Address objects support some operators.  Unless stated otherwise, operators can
only be applied between compatible objects (i.e. IPv4 with IPv4, IPv6 with
IPv6).

Logical operators
"""""""""""""""""

Address objects can be compared with the usual set of logical operators.  Some
examples::

   >>> IPv4Address('127.0.0.2') > IPv4Address('127.0.0.1')
   True
   >>> IPv4Address('127.0.0.2') == IPv4Address('127.0.0.1')
   False
   >>> IPv4Address('127.0.0.2') != IPv4Address('127.0.0.1')
   True

Arithmetic operators
""""""""""""""""""""

Integers can be added to or subtracted from address objects.  Some examples::

   >>> IPv4Address('127.0.0.2') + 3
   IPv4Address('127.0.0.5')
   >>> IPv4Address('127.0.0.2') - 3
   IPv4Address('126.255.255.255')
   >>> IPv4Address('255.255.255.255') + 1
   Traceback (most recent call last):
     File "<stdin>", line 1, in <module>
   ipaddress.AddressValueError: 4294967296 (>= 2**32) is not permitted as an IPv4 address


Network objects
---------------

.. class:: IPv4Network(address, strict=True)

   Construct an IPv4 network.  *address* is a string or integer representing the
   IP address (and optionally the network).  An :exc:`AddressValueError` is
   raised if *address* is not a valid IPv4 address.  A :exc:`NetmaskValueError`
   is raised if the netmask is not valid for an IPv4 address.

   If *strict* is ``True`` and host bits are set in the supplied address,
   then :exc:`ValueError` is raised. Otherwise, the host bits are masked out
   to determine the appropriate network address.

   >>> ipaddress.IPv4Network('192.0.2.0/27')
   IPv4Network('192.0.2.0/27')
   >>> ipaddress.IPv4Network('192.0.2.0/27').netmask
   IPv4Address('255.255.255.224')
   >>> ipaddress.IPv4Network('192.0.2.5/27', strict=False)
   IPv4Network('192.0.2.0/27')


.. class:: IPv6Network(address, strict=True)

   Construct an IPv6 network.  *address* is a string or integer representing the
   IP address (and optionally the network).  An :exc:`AddressValueError` is
   raised if *address* is not a valid IPv6 address.  A :exc:`NetmaskValueError`
   is raised if the netmask is not valid for an IPv6 address.

   If *strict* is ``True`` and host bits are set in the supplied address,
   then :exc:`ValueError` is raised. Otherwise, the host bits are masked out
   to determine the appropriate network address.

   >>> ipaddress.IPv6Network('2001:db8::/96')
   IPv6Network('2001:db8::/96')
   >>> ipaddress.IPv6Network('2001:db8::/96').netmask
   IPv6Address('ffff:ffff:ffff:ffff:ffff:ffff::')
   >>> ipaddress.IPv6Network('2001:db8::1000/96', strict=False)
   IPv6Network('2001:db8::/96')


Interface objects
-----------------

.. class:: IPv4Interface(address)

   Construct an IPv4 interface.  *address* is a string or integer representing
   the IP interface.  An :exc:`AddressValueError` is raised if *address* is not
   a valid IPv4 address.

   The network address for the interface is determined by calling
   ``IPv4Network(address, strict=False)``.

   >>> ipaddress.IPv4Interface('192.168.0.0/24')
   IPv4Interface('192.168.0.0/24')
   >>> ipaddress.IPv4Interface('192.168.0.0/24').network
   IPv4Network('192.168.0.0/24')


.. class:: IPv6Interface(address)

   Construct an IPv6 interface.  *address* is a string or integer representing
   the IP interface.  An :exc:`AddressValueError` is raised if *address* is not
   a valid IPv6 address.

   The network address for the interface is determined by calling
   ``IPv6Network(address, strict=False)``.

   >>> ipaddress.IPv6Interface('2001:db8::1000/96')
   IPv6Interface('2001:db8::1000/96')
   >>> ipaddress.IPv6Interface('2001:db8::1000/96').network
   IPv6Network('2001:db8::/96')


Other Module Level Functions
----------------------------

The module also provides the following module level functions:

.. function:: v4_int_to_packed(address)

   Represent an address as 4 packed bytes in network (big-endian) order.
   *address* is an integer representation of an IPv4 IP address.  A
   :exc:`ValueError` is raised if the integer is negative or too large to be an
   IPv4 IP address.

   >>> ipaddress.ip_address(3221225985)
   IPv4Address('192.0.2.1')
   >>> ipaddress.v4_int_to_packed(3221225985)
   b'\xc0\x00\x02\x01'


.. function:: v6_int_to_packed(address)

   Represent an address as 16 packed bytes in network (big-endian) order.
   *address* is an integer representation of an IPv6 IP address.  A
   :exc:`ValueError` is raised if the integer is negative or too large to be an
   IPv6 IP address.


.. function:: summarize_address_range(first, last)

   Return an iterator of the summarized network range given the first and last
   IP addresses.  *first* is the first :class:`IPv4Address` or
   :class:`IPv6Address` in the range and *last* is the last :class:`IPv4Address`
   or :class:`IPv6Address` in the range.  A :exc:`TypeError` is raised if
   *first* or *last* are not IP addresses or are not of the same version.  A
   :exc:`ValueError` is raised if *last* is not greater than *first* or if
   *first* address version is not 4 or 6.

   >>> [ipaddr for ipaddr in ipaddress.summarize_address_range(
   ...    ipaddress.IPv4Address('192.0.2.0'),
   ...    ipaddress.IPv4Address('192.0.2.130'))]
   [IPv4Network('192.0.2.0/25'), IPv4Network('192.0.2.128/31'), IPv4Network('192.0.2.130/32')]


.. function:: collapse_addresses(addresses)

   Return an iterator of the collapsed :class:`IPv4Network` or
   :class:`IPv6Network` objects.  *addresses* is an iterator of
   :class:`IPv4Network` or :class:`IPv6Network` objects.  A :exc:`TypeError` is
   raised if *addresses* contains mixed version objects.

   >>> [ipaddr for ipaddr in
   ... ipaddress.collapse_addresses([ipaddress.IPv4Network('192.0.2.0/25'),
   ... ipaddress.IPv4Network('192.0.2.128/25')])]
   [IPv4Network('192.0.2.0/24')]


.. function:: get_mixed_type_key(obj)

   Return a key suitable for sorting between networks and addresses.  Address
   and Network objects are not sortable by default; they're fundamentally
   different, so the expression::

     IPv4Address('192.0.2.0') <= IPv4Network('192.0.2.0/24')

   doesn't make sense.  There are some times however, where you may wish to
   have :mod:`ipaddress` sort these anyway.  If you need to do this, you can use
   this function as the ``key`` argument to :func:`sorted()`.

   *obj* is either a network or address object.


Custom Exceptions
-----------------

To support more specific error reporting from class constructors, the
module defines the following exceptions:

.. exception:: AddressValueError(ValueError)

   Any value error related to the address.


.. exception:: NetmaskValueError(ValueError)

   Any value error related to the netmask.
