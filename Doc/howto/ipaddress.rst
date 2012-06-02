.. _ipaddress-howto:

***************
Ipaddress Howto
***************

:author: Peter Moody

.. topic:: Abstract

   This document is a gentle introduction to :mod:`ipaddress` module.


Creating Address/Network/Interface objects
==========================================

Since :mod:`ipaddress` is a module for inspecting and manipulating IP address,
the first thing you'll want to do is create some objects.  You can use
:mod:`ipaddress` to create objects from strings and integers.


A Note on IP Versions
---------------------

For readers that aren't particularly familiar with IP addressing, it's
important to know that the Internet Protocol is currently in the process
of moving from version 4 of the protocol to version 6. This transition is
occurring largely because version 4 of the protocol doesn't provide enough
addresses to handle the needs of the whole world, especially given the
increasing number of devices with direct connections to the internet.

Explaining the details of the differences between the two versions of the
protocol is beyond the scope of this introduction, but readers need to at
least be aware that these two versions exist, and it will sometimes be
necessary to force the use of one version or the other.


IP Host Addresses
-----------------

Addresses, often referred to as "host addresses" are the most basic unit
when working with IP addressing. The simplest way to create addresses is
to use the ``ip_address`` factory function, which automatically determines
whether to create an IPv4 or IPv6 address based on the passed in value::

   >>> ipaddress.ip_address('192.0.2.1')
   IPv4Address('192.0.2.1')
   >>> ipaddress.ip_address('2001:DB8::1')
   IPv6Address('2001:db8::1')

Addresses can also be created directly from integers. Values that will
fit within 32 bits are assumed to be IPv4 addresses::

   >>> ipaddress.ip_address(3221225985)
   IPv4Address('192.0.2.1')
   >>> ipaddress.ip_address(42540766411282592856903984951653826561)
   IPv6Address('2001:db8::1')

To force the use of IPv4 or IPv6 addresses, the relevant classes can be
invoked directly. This is particularly useful to force creation of IPv6
addresses for small integers::

   >>> ipaddress.ip_address(1)
   IPv4Address('0.0.0.1')
   >>> ipaddress.IPv4Address(1)
   IPv4Address('0.0.0.1')
   >>> ipaddress.IPv6Address(1)
   IPv6Address('::1')


Defining Networks
-----------------

Host addresses are usually grouped together into IP networks, so
:mod:`ipaddress` provides a way to create, inspect and manipulate network
definitions. IP network objects are constructed from strings that define the
range of host addresses that are part of that network. The simplest form
for that information is a "network address/network prefix" pair, where the
prefix defines the number of leading bits that are compared to determine
whether or not an address is part of the network and the network address
defines the expected value of those bits.

As for addresses, a factory function is provided that determines the correct
IP version automatically::

   >>> ipaddress.ip_network('192.0.2.0/24')
   IPv4Network('192.0.2.0/24')
   >>> ipaddress.ip_network('2001:db8::0/96')
   IPv6Network('2001:db8::/96')

Network objects cannot have any host bits set.  The practical effect of this
is that ``192.0.2.1/24`` does not describe a network.  Such definitions are
referred to as interface objects since the ip-on-a-network notation is
commonly used to describe network interfaces of a computer on a given network
and are described further in the next section.

By default, attempting to create a network object with host bits set will
result in :exc:`ValueError` being raised. To request that the
additional bits instead be coerced to zero, the flag ``strict=False`` can
be passed to the constructor::

   >>> ipaddress.ip_network('192.0.2.1/24')
   Traceback (most recent call last):
      ...
   ValueError: 192.0.2.1/24 has host bits set
   >>> ipaddress.ip_network('192.0.2.1/24', strict=False)
   IPv4Network('192.0.2.0/24')

While the string form offers significantly more flexibility, networks can
also be defined with integers, just like host addresses. In this case, the
network is considered to contain only the single address identified by the
integer, so the network prefix includes the entire network address::

   >>> ipaddress.ip_network(3221225984)
   IPv4Network('192.0.2.0/32')
   >>> ipaddress.ip_network(42540766411282592856903984951653826560L)
   IPv6Network('2001:db8::/128')

Creation of a particular kind of network can be forced by calling the
class constructor directly instead of using the factory function.


Host Interfaces
---------------

As mentioned just above, if you need to describe an address on a particular
network, neither the address nor the network classes are sufficient.
Notation like ``192.0.2.1/24`` is commonly used network engineers and the
people who write tools for firewalls and routers as shorthand for "the host
``192.0.2.1`` on the network ``192.0.2.0/24``", Accordingly, :mod:`ipaddress`
provides a set of hybrid classes that associate an address with a particular
network. The interface for creation is identical to that for defining network
objects, except that the address portion isn't constrained to being a network
address.

   >>> ipaddress.ip_interface('192.0.2.1/24')
   IPv4Interface('192.0.2.1/24')
   >>> ipaddress.ip_network('2001:db8::1/96')
   IPv6Interface('2001:db8::1/96')

Integer inputs are accepted (as with networks), and use of a particular IP
version can be forced by calling the relevant constructor directly.


Inspecting Address/Network/Interface Objects
============================================

You've gone to the trouble of creating an IPv(4|6)(Address|Network|Interface)
object, so you probably want to get information about it.  :mod:`ipaddress`
tries to make doing this easy and intuitive.

Extracting the IP version::

   >>> addr4 = ipaddress.ip_address('192.0.2.1')
   >>> addr6 = ipaddress.ip_address('2001:db8::1')
   >>> addr6.version
   6
   >>> addr4.version
   4

Obtaining the network from an interface::

   >>> host4 = ipaddress.ip_interface('192.0.2.1/24')
   >>> host4.network
   IPv4Network('192.0.2.0/24')
   >>> host6 = ipaddress.ip_interface('2001:db8::1/96')
   >>> host6.network
   IPv6Network('2001:db8::/96')

Finding out how many individual addresses are in a network::

   >>> net4 = ipaddress.ip_network('192.0.2.0/24')
   >>> net4.numhosts
   256
   >>> net6 = ipaddress.ip_network('2001:db8::0/96')
   >>> net6.numhosts
   4294967296

Iterating through the 'usable' addresses on a network::

   >>> net4 = ipaddress.ip_network('192.0.2.0/24')
   >>> for x in net4.iterhosts():
          print(x)
   192.0.2.1
   192.0.2.2
   192.0.2.3
   192.0.2.4
   <snip>
   192.0.2.252
   192.0.2.253
   192.0.2.254


Obtaining the netmask (i.e. set bits corresponding to the network prefix) or
the hostmask (any bits that are not part of the netmask):

   >>> net4 = ipaddress.ip_network('192.0.2.0/24')
   >>> net4.netmask
   IPv4Address('255.255.255.0')
   >>> net4.hostmask
   IPv4Address('0.0.0.255')
   >>> net6 = ipaddress.ip_network('2001:db8::0/96')
   >>> net6.netmask
   IPv6Address('ffff:ffff:ffff:ffff:ffff:ffff::')
   >>> net6.hostmask
   IPv6Address('::ffff:ffff')


Exploding or compressing the address::

   >>> net6.exploded
   '2001:0000:0000:0000:0000:0000:0000:0000/96'
   >>> addr6.exploded
   '2001:0000:0000:0000:0000:0000:0000:0001'


Networks as lists of Addresses
==============================

It's sometimes useful to treat networks as lists.  This means it is possible
to index them like this::

   >>> net4[1]
   IPv4Address('192.0.2.1')
   >>> net4[-1]
   IPv4Address('192.0.2.255')
   >>> net6[1]
   IPv6Address('2001::1')
   >>> net6[-1]
   IPv6Address('2001::ffff:ffff')


It also means that network objects lend themselves to using the list
membership test syntax like this::

   if address in network:
       # do something

Containment testing is done efficiently based on the network prefix::

   >>> addr4 = ipaddress.ip_address('192.0.2.1')
   >>> addr4 in ipaddress.ip_network('192.0.2.0/24')
   True
   >>> addr4 in ipaddress.ip_network('192.0.3.0/24')
   False


Comparisons
===========

:mod:`ipaddress` provides some simple, hopefully intuitive ways to compare
objects, where it makes sense::

   >>> ipaddress.ip_address('192.0.2.1') < ipaddress.ip_address('192.0.2.2')
   True

A :exc:`TypeError` exception is raised if you try to compare objects of
different versions or different types.


Using IP Addresses with other modules
=====================================

Other modules that use IP addresses (such as :mod:`socket`) usually won't
accept objects from this module directly. Instead, they must be coerced to
an integer or string that the other module will accept::

   >>> addr4 = ipaddress.ip_address('192.0.2.1')
   >>> str(addr4)
   '192.0.2.1'
   >>> int(addr4)
   3221225985


Exceptions raised by :mod:`ipaddress`
=====================================

If you try to create an address/network/interface object with an invalid value
for either the address or netmask, :mod:`ipaddress` will raise an
:exc:`AddressValueError` or :exc:`NetmaskValueError` respectively. However,
this applies only when calling the class constructors directly. The factory
functions and other module level functions will just raise :exc:`ValueError`.

Both of the module specific exceptions have :exc:`ValueError` as their
parent class, so if you're not concerned with the particular type of error,
you can still do the following::

   try:
       ipaddress.IPv4Address(address)
   except ValueError:
       print('address/netmask is invalid:', address)
