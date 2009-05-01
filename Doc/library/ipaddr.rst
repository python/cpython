:mod:`ipaddr` --- IP address manipulation library
=================================================

.. module:: ipaddr
   :synopsis: IPv4 and IPv6 network address manipulation classes.
.. moduleauthor:: Google, Inc.
.. sectionauthor:: Gregory P. Smith <greg@krypto.org>


.. versionadded:: 2.7

.. index::
   single: IP address, IPv4, IPv6, netmask

This module implements classes for working with IP host and network addresses,
both IPv4 and IPv6.


.. function:: IP(ipaddr)

   Take an IP string or int and return an object of the correct type.  Returns
   an :class:`IPv4` or :class:`IPv6` object.

   The *ipaddr* parameter must be a string or integer representing the IP
   address.  Either IPv4 or IPv6 addresses may be supplied.  Integers less than
   2**32 will be considered to be IPv4.

   Raises :exc:`ValueError` if the *ipaddr* passed is not either an IPv4 or an
   IPv6 address.


.. function:: collapse_address_list(addresses)

   Collapse a sequence of :class:`IPv4` or :class:`IPv6` objects into the most
   concise representation.  Returns a list of :class:`IPv4` or :class:`IPv6`
   objects.

   Example usage::

      >>> collapse_address_list([IPv4('1.1.0.0/24'), IPv4('1.1.1.0/24')])
      [IPv4('1.1.0.0/23')]


.. class:: BaseIP()

   A generic IP address object.  This base class defines the API and contains
   common code.  Most authors should either use the :func:`IP` function or
   create :class:`IPv4` or :class:`IPv6` objects directly rather than using this
   base class.

   IP address objects support the following python operators:
   ``=``, ``!=``, ``<``, ``>``, ``<=``, ``>=``, and ``in``.

   An IP address object may be used as a sequence index or as a hash key and can
   be converted back to an integer representation using :func:`int`.  It may
   also be used as a sequence that yields the string representation of every IP
   address within the object's subnet.

   The following properties are available on all IP address objects:

   .. attr:: broadcast

      Integer representation of the broadcast address.  Read only.

   .. attr:: broadcast_ext

      Dotted decimal or colon string version of the broadcast address.  Read
      only.

   .. attr:: hostmask

      Integer representation of the hostmask.  Read only.

   .. attr:: hostmask_ext

      Dotted decimal or colon string version of the hostmask.  Read only.

   .. attr:: ip

      Integer representation of the IP address.  Read only.

   .. attr:: ip_ext

      Dotted decimal or colon string version of the IP address.  Read only.

   .. attr:: ip_ext_full

      Canonical string version of the IP address.  Read only.

   .. attr:: is_loopback

      True if the address is a loopback address as defined in IPv4 :rfc:`3330`
      or IPv6 :rfc:`2373` section 2.5.3.

   .. attr:: is_link_local

      True if the address is a link-local address as defined in IPv4 :rfc:`3927`
      or IPv6 :rfc:`4291`.

   .. attr:: is_multicast

      True if the address is reserved for multicast use.  See IPv4 :rfc:`3171`
      or IPv6 :rfc:`2373` section 2.7 for details.

   .. attr:: is_private

      True if the address is reserved for private networks as defined in IPv4
      :rfc:`1918` or IPv6 :rfc:`4193`.

   .. attr:: netmask

      Integer representation of the netmask.  Read only.

   .. attr:: netmask_ext

      Dotted decimal or colon string version of the netmask.  Read only.

   .. attr:: network

      Integer representation of the network.  Read only.

   .. attr:: network_ext

      Dotted decimal or colon string version of the network.  Read only.

   .. attr:: numhosts

      Number of hosts in the current subnet.  Read only.

   .. attr:: packed

      The packed network byte order representation of this network address.
      Read only.

   .. attr:: prefixlen

      A property to get and set the prefix length.  Readable and writeable.

   .. attr:: version

      Integer IP version number.  Read only.


   The following methods are available on all IP address objects:

   .. method:: address_exclude(other)

      Remove an address from within a larger block.  Returns a sorted list of IP
      address objects representing networks.

      Examples::

         >>> addr1 = IP('10.1.1.0/24')
         >>> addr2 = IP('10.1.1.0/26')
         >>> addr1.address_exclude(addr2)
         [IP('10.1.1.64/26'), IP('10.1.1.128/25')]

         >>> addr1 = IP('::1/32')
         >>> addr2 = IP('::1/128')
         >>> addr1.address_exclude(addr2)
         [IP('::0/128'), IP('::2/127'), IP('::4/126'), IP('::8/125'),
          ... IP('0:0:8000::/33')]

      Raises :exc:`ValueError` if *other* is not completely contained by *self*.


   .. method:: compare_networks(other)

      Compare this IP object's network to another IP network.
      Returns -1, 0 or 1.

      This compares the integer representation of the network addresses.  The
      host bits are not considered by this method.  If you want to compare host
      bits, you can use ``host_a.ip < host_b.ip``.

      If the IP versions of self and other are the same, returns:

      -1 if self < other
        eg: IPv4('1.1.1.0/24') < IPv4('1.1.2.0/24')

        IPv6('1080::200C:417A') < IPv6('1080::200B:417B')

      0 if self == other
        eg: IPv4('1.1.1.1/24') == IPv4('1.1.1.2/24')

        IPv6('1080::200C:417A/96') == IPv6('1080::200C:417B/96')

      1 if self > other
        eg: IPv4('1.1.1.0/24') > IPv4('1.1.0.0/24')

        IPv6('1080::1:200C:417A/112') > IPv6('1080::0:200C:417A/112')

      If the IP versions of self and other are different, returns:

      -1 if self.version < other.version
        eg: IPv4('10.0.0.1/24') < IPv6('::1/128')

      1 if self.version > other.version
        eg: IPv6('::1/128') > IPv4('255.255.255.0/24')


   .. method:: subnet(prefixlen_diff=1)

      Returns a list of subnets which when joined make up the current subnet.

      The optional *prefixlen_diff* argument specifies how many bits the prefix
      length should be increased by.  Given a /24 network and
      ``prefixlen_diff=3``, for example, 8 subnets of size /27 will be returned.

      If called on a host IP address rather than a network, a list containing
      the host itself will be returned.

      Raises :exc:`PrefixlenDiffInvalidError` if the *prefixlen_diff* is out of
      range.


   .. method:: supernet(prefixlen_diff=1)

      Returns a single IP object representing the supernet containing the
      current network.

      The optional *prefixlen_diff* argument specifies how many bits the prefix
      length should be decreased by.  Given a /24 network and
      ``prefixlen_diff=3``, for example, a supernet with a 21 bit netmask is
      returned.

      Raises :exc:`PrefixlenDiffInvalidError` if the prefixlen_diff is out of
      range.


.. class:: IPv4()

   This class represents and manipulates 32-bit IPv4 addresses.

   Attributes::

      # These examples for IPv4('1.2.3.4/27')
      .ip: 16909060
      .ip_ext: '1.2.3.4'
      .ip_ext_full: '1.2.3.4'
      .network: 16909056
      .network_ext: '1.2.3.0'
      .hostmask: 31 (0x1F)
      .hostmask_ext: '0.0.0.31'
      .broadcast: 16909087 (0x102031F)
      .broadcast_ext: '1.2.3.31'
      .netmask: 4294967040 (0xFFFFFFE0)
      .netmask_ext: '255.255.255.224'
      .prefixlen: 27


.. class:: IPv6()

   This class respresents and manipulates 128-bit IPv6 addresses.

   Attributes::

      # These examples are for IPv6('2001:658:22A:CAFE:200::1/64')
      .ip: 42540616829182469433547762482097946625
      .ip_ext: '2001:658:22a:cafe:200::1'
      .ip_ext_full: '2001:0658:022a:cafe:0200:0000:0000:0001'
      .network: 42540616829182469433403647294022090752
      .network_ext: '2001:658:22a:cafe::'
      .hostmask: 18446744073709551615
      .hostmask_ext: '::ffff:ffff:ffff:ffff'
      .broadcast: 42540616829182469451850391367731642367
      .broadcast_ext: '2001:658:22a:cafe:ffff:ffff:ffff:ffff'
      .netmask: 340282366920938463444927863358058659840
      .netmask_ext: 64
      .prefixlen: 64

   .. attr:: is_site_local

      True if the address was reserved as site-local in :rfc:`3513` section
      2.5.6.

      .. note::

         The IPv6 site-local address space has been deprecated by :rfc:`3879`.
         Use :data:`is_private` to test if this address is in the space of
         unique local addresses as defined by :rfc:`4193`.

   .. attr:: is_unspecified

      True if this is the unspecified address as defined in :rfc:`2373` section
      2.5.2.


The following exceptions are defined by this module:

.. exception:: Error

   Base class for all exceptions defined in this module.

.. exception:: IPTypeError

   Tried to perform a v4 action on v6 object or vice versa.

.. exception:: IPAddressExclusionError

   An Error we should never see occurred in address exclusion.

.. exception:: IPv4IpValidationError

   Raised when an IPv4 address is invalid.

.. exception:: IPv4NetmaskValidationError

   Raised when a netmask is invalid.

.. exception:: IPv6IpValidationError

   Raised when an IPv6 address is invalid.

.. exception:: IPv6NetmaskValidationError

   Raised when an IPv6 netmask is invalid.

.. exception:: PrefixlenDiffInvalidError

   Raised when :meth:`BaseIP.subnet` or :meth:`BaseIP.supernet` is called with a
   bad ``prefixlen_diff``.


.. seealso::

   http://code.google.com/p/ipaddr-py/
      The original source of this module and a place to download it as a package
      for use on earlier versions of Python.
