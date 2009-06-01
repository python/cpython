# Copyright 2007 Google Inc.
#  Licensed to PSF under a Contributor Agreement.
#
# See also: http://code.google.com/p/ipaddr-py/

"""An IPv4/IPv6 manipulation library in Python.

This library is used to create/poke/manipulate IPv4 and IPv6 addresses
and prefixes.

"""

__version__ = '1.1.0'

import struct


class Error(Exception):

    """Base class for exceptions."""


class IPTypeError(Error):

    """Tried to perform a v4 action on v6 object or vice versa."""


class IPAddressExclusionError(Error):

    """An Error we should never see occurred in address exclusion."""


class IPv4IpValidationError(Error):

    """Raised when an IPv4 address is invalid."""

    def __init__(self, ip):
        Error.__init__(self)
        self.ip = ip

    def __str__(self):
        return repr(self.ip) + ' is not a valid IPv4 address'


class IPv4NetmaskValidationError(Error):

    """Raised when a netmask is invalid."""

    def __init__(self, netmask):
        Error.__init__(self)
        self.netmask = netmask

    def __str__(self):
        return repr(self.netmask) + ' is not a valid IPv4 netmask'


class IPv6IpValidationError(Error):

    """Raised when an IPv6 address is invalid."""

    def __init__(self, ip):
        Error.__init__(self)
        self.ip = ip

    def __str__(self):
        return repr(self.ip) + ' is not a valid IPv6 address'


class IPv6NetmaskValidationError(Error):

    """Raised when an IPv6 netmask is invalid."""

    def __init__(self, netmask):
        Error.__init__(self)
        self.netmask = netmask

    def __str__(self):
        return repr(self.netmask) + ' is not a valid IPv6 netmask'


class PrefixlenDiffInvalidError(Error):

    """Raised when Sub/Supernets is called with a bad prefixlen_diff."""

    def __init__(self, error_str):
        Error.__init__(self)
        self.error_str = error_str


def IP(ipaddr):
    """Take an IP string/int and return an object of the correct type.

    Args:
        ipaddr: A string or integer, the IP address.  Either IPv4 or
          IPv6 addresses may be supplied; integers less than 2**32 will
          be considered to be IPv4.

    Returns:
        An IPv4 or IPv6 object.

    Raises:
        ValueError: if the string passed isn't either a v4 or a v6
          address.

    """

    try:
        return IPv4(ipaddr)
    except (IPv4IpValidationError, IPv4NetmaskValidationError):
        pass

    try:
        return IPv6(ipaddr)
    except (IPv6IpValidationError, IPv6NetmaskValidationError):
        pass

    raise ValueError('%r does not appear to be an IPv4 or IPv6 address' %
                     ipaddr)


def _collapse_address_list_recursive(addresses):
    """Loops through the addresses, collapsing concurrent netblocks.

    Example:

    >>> ip1 = IPv4('1.1.0.0/24')
    >>> ip2 = IPv4('1.1.1.0/24')
    >>> ip3 = IPv4('1.1.2.0/24')
    >>> ip4 = IPv4('1.1.3.0/24')
    >>> ip5 = IPv4('1.1.4.0/24')
    >>> ip6 = IPv4('1.1.0.1/22')

    >>> _collapse_address_list_recursive([ip1, ip2, ip3, ip4, ip5, ip6])
    [IPv4('1.1.0.0/22'), IPv4('1.1.4.0/24'), IPv4('1.1.0.1/22')]

    Notes:
        This shouldn't be called directly; it is called via
          collapse_address_list([]).

    Args:
        addresses: A list of IPv4 or IPv6 objects.

    Returns:
        A list of IPv4 or IPv6 objects depending on what we were passed.

    """
    ret_array = []
    optimized = False

    for cur_addr in addresses:
        if not ret_array:
            ret_array.append(cur_addr)
            continue
        if cur_addr in ret_array[-1]:
            optimized = True
        elif cur_addr == ret_array[-1].supernet().subnet()[1]:
            ret_array.append(ret_array.pop().supernet())
            optimized = True
        else:
            ret_array.append(cur_addr)

    if optimized:
        return _collapse_address_list_recursive(ret_array)

    return ret_array


def collapse_address_list(addresses):
    """Collapse a list of IP objects.

    Example:

    >>> collapse_address_list([IPv4('1.1.0.0/24'), IPv4('1.1.1.0/24')])
    [IPv4('1.1.0.0/23')]

    Args:
        addresses: A list of IPv4 or IPv6 objects.

    Returns:
        A list of IPv4 or IPv6 objects depending on what we were passed.

    """
    return _collapse_address_list_recursive(
        sorted(addresses, key=BaseIP._get_networks_key))


class BaseIP(object):

    """A generic IP object.

    This IP class contains most of the methods which are used by
    the IPv4 and IPv6 classes.

    """

    def __getitem__(self, n):
        if n >= 0:
            if self.network + n > self.broadcast:
                raise IndexError
            return self._string_from_ip_int(self.network + n)
        else:
            n += 1
            if self.broadcast + n < self.network:
                raise IndexError
            return self._string_from_ip_int(self.broadcast + n)

    def __lt__(self, other):
        try:
            return (self.version < other.version
                    or self.ip < other.ip
                    or self.netmask < other.netmask)
        except AttributeError:
            return NotImplemented

    def __gt__(self, other):
        try:
            return (self.version > other.version
                    or self.ip > other.ip
                    or self.netmask > other.netmask)
        except AttributeError:
            return NotImplemented

    def __eq__(self, other):
        try:
            return (self.version == other.version
                    and self.ip == other.ip
                    and self.netmask == other.netmask)
        except AttributeError:
            return NotImplemented

    def __ne__(self, other):
        eq = self.__eq__(other)
        if eq is NotImplemented:
            return NotImplemented
        return not eq

    def __le__(self, other):
        gt = self.__gt__(other)
        if gt is NotImplemented:
            return NotImplemented
        return not gt

    def __ge__(self, other):
        lt = self.__lt__(other)
        if lt is NotImplemented:
            return NotImplemented
        return not lt

    def __repr__(self):
        return '%s(%r)' % (self.__class__.__name__, str(self))

    def __index__(self):
        return self.ip

    def __int__(self):
        return self.ip

    def address_exclude(self, other):
        """Remove an address from a larger block.

        For example:

        >>> addr1 = IP('10.1.1.0/24')
        >>> addr2 = IP('10.1.1.0/26')
        >>> addr1.address_exclude(addr2)
        [IPv4('10.1.1.64/26'), IPv4('10.1.1.128/25')]

        or IPv6:

        >>> addr1 = IP('::1/32')
        >>> addr2 = IP('::1/128')
        >>> s = addr1.address_exclude(addr2)
        >>> s[:4]
        [IPv6('::/128'), IPv6('::2/127'), IPv6('::4/126'), IPv6('::8/125')]
        >>> s[-1]
        IPv6('0:0:8000::/33')

        Args:
            other: An IP object of the same type.

        Returns:
            A sorted list of IP objects addresses which is self minus
            other.

        Raises:
            IPTypeError: If self and other are of difffering address
              versions.
            IPAddressExclusionError: There was some unknown error in the
              address exclusion process.  This likely points to a bug
              elsewhere in this code.
            ValueError: If other is not completely contained by self.

        """
        if not self.version == other.version:
            raise IPTypeError("%s and %s aren't of the same version" % (
                str(self), str(other)))

        if other not in self:
            raise ValueError('%s not contained in %s' % (str(other),
                                                         str(self)))

        ret_addrs = []

        # Make sure we're comparing the network of other.
        other = IP(other.network_ext + '/' + str(other.prefixlen))

        s1, s2 = self.subnet()
        while s1 != other and s2 != other:
            if other in s1:
                ret_addrs.append(s2)
                s1, s2 = s1.subnet()
            elif other in s2:
                ret_addrs.append(s1)
                s1, s2 = s2.subnet()
            else:
                # If we got here, there's a bug somewhere.
                raise IPAddressExclusionError('Error performing exclusion: '
                                              's1: %s s2: %s other: %s' %
                                              (str(s1), str(s2), str(other)))
        if s1 == other:
            ret_addrs.append(s2)
        elif s2 == other:
            ret_addrs.append(s1)
        else:
            # If we got here, there's a bug somewhere.
            raise IPAddressExclusionError('Error performing exclusion: '
                                          's1: %s s2: %s other: %s' %
                                          (str(s1), str(s2), str(other)))

        return sorted(ret_addrs, key=BaseIP._get_networks_key)

    def compare_networks(self, other):
        """Compare two IP objects.

        This is only concerned about the comparison of the integer
        representation of the network addresses.  This means that the
        host bits aren't considered at all in this method.  If you want
        to compare host bits, you can easily enough do a
        'HostA.ip < HostB.ip'

        Args:
            other: An IP object.

        Returns:
            If the IP versions of self and other are the same, returns:

            -1 if self < other:
              eg: IPv4('1.1.1.0/24') < IPv4('1.1.2.0/24')
              IPv6('1080::200C:417A') < IPv6('1080::200B:417B')
            0 if self == other
              eg: IPv4('1.1.1.1/24') == IPv4('1.1.1.2/24')
              IPv6('1080::200C:417A/96') == IPv6('1080::200C:417B/96')
            1 if self > other
              eg: IPv4('1.1.1.0/24') > IPv4('1.1.0.0/24')
              IPv6('1080::1:200C:417A/112') >
              IPv6('1080::0:200C:417A/112')

            If the IP versions of self and other are different, returns:

            -1 if self.version < other.version
              eg: IPv4('10.0.0.1/24') < IPv6('::1/128')
            1 if self.version > other.version
              eg: IPv6('::1/128') > IPv4('255.255.255.0/24')

        To sort networks with sorted(), min(), max() and other tools with a
        *key* argument, use the operator.attrgetter() function to extract the
        relevant fields:

            >>> from operator import attrgetter
            >>> s = [IPv6('::1/128'), IPv4('255.255.255.0/24')]
            >>> sorted(s, key=attrgetter('version', 'network', 'netmask'))
            [IPv4('255.255.255.0/24'), IPv6('::1/128')]

        """
        if self.version < other.version:
            return -1
        if self.version > other.version:
            return 1
        # self.version == other.version below here:
        if self.network < other.network:
            return -1
        if self.network > other.network:
            return 1
        # self.network == other.network below here:
        if self.netmask < other.netmask:
            return -1
        if self.netmask > other.netmask:
            return 1
        # self.network == other.network and self.netmask == other.netmask
        return 0

    def _get_networks_key(self):
        """Network-only key function.

        Returns an object that identifies this address' network and
        netmask. This function is a suitable "key" argument for sorted()
        and list.sort().

        """
        return (self.version, self.network, self.netmask)

    prefixlen = property(
        fget=lambda self: self._prefixlen,
        fset=lambda self, prefixlen: self._set_prefix(prefixlen))

    def __str__(self):
        return  '%s/%s' % (self._string_from_ip_int(self.ip),
                           str(self.prefixlen))

    def __hash__(self):
        return hash(self.ip ^ self.netmask)

    def __contains__(self, other):
        return self.network <= other.ip and self.broadcast >= other.broadcast

    @property
    def ip_ext(self):
        """Dotted decimal or colon string version of the IP address."""
        return self._string_from_ip_int(self.ip)

    @property
    def ip_ext_full(self):
        """Canonical string version of the IP address."""
        return self.ip_ext

    @property
    def broadcast(self):
        """Integer representation of the broadcast address."""
        return self.ip | self.hostmask

    @property
    def broadcast_ext(self):
        """Dotted decimal or colon string version of the broadcast."""
        return self._string_from_ip_int(self.broadcast)

    @property
    def hostmask(self):
        """Integer representation of the hostmask."""
        return self.netmask ^ self._ALL_ONES

    @property
    def hostmask_ext(self):
        """Dotted decimal or colon string version of the hostmask."""
        return self._string_from_ip_int(self.hostmask)

    @property
    def network(self):
        """Integer representation of the network."""
        return self.ip & self.netmask

    @property
    def network_ext(self):
        """Dotted decimal or colon string version of the network."""
        return self._string_from_ip_int(self.network)

    @property
    def netmask_ext(self):
        """Dotted decimal or colon string version of the netmask."""
        return self._string_from_ip_int(self.netmask)

    @property
    def numhosts(self):
        """Number of hosts in the current subnet."""
        return self.broadcast - self.network + 1

    @property
    def version(self):
        raise NotImplementedError('BaseIP has no version')

    def _ip_int_from_prefix(self, prefixlen=None):
        """Turn the prefix length netmask into a int for comparison.

        Args:
            prefixlen: An integer, the prefix length.

        Returns:
            An integer.

        """
        if not prefixlen and prefixlen != 0:
            prefixlen = self.prefixlen
        return self._ALL_ONES ^ (self._ALL_ONES >> prefixlen)

    def _prefix_from_ip_int(self, ip_int, mask=32):
        """Return prefix length from the decimal netmask.

        Args:
            ip_int: An integer, the IP address.
            mask: The netmask.  Defaults to 32.

        Returns:
            An integer, the prefix length.

        """
        while mask:
            if ip_int & 1 == 1:
                break
            ip_int >>= 1
            mask -= 1

        return mask

    def _ip_string_from_prefix(self, prefixlen=None):
        """Turn a prefix length into a dotted decimal string.

        Args:
            prefixlen: An integer, the netmask prefix length.

        Returns:
            A string, the dotted decimal netmask string.

        """
        if not prefixlen:
            prefixlen = self.prefixlen
        return self._string_from_ip_int(self._ip_int_from_prefix(prefixlen))


class IPv4(BaseIP):

    """This class represents and manipulates 32-bit IPv4 addresses.

    >>> addr = IPv4('1.2.3.4/27')
    >>> for attr in ['ip', 'ip_ext', 'ip_ext_full', 'network', 'network_ext',
    ...              'hostmask', 'hostmask_ext', 'broadcast', 'broadcast_ext',
    ...              'netmask', 'netmask_ext', 'prefixlen']:
    ...     print(attr, '=', getattr(addr, attr))
    ip = 16909060
    ip_ext = 1.2.3.4
    ip_ext_full = 1.2.3.4
    network = 16909056
    network_ext = 1.2.3.0
    hostmask = 31
    hostmask_ext = 0.0.0.31
    broadcast = 16909087
    broadcast_ext = 1.2.3.31
    netmask = 4294967264
    netmask_ext = 255.255.255.224
    prefixlen = 27

    """

    # Equivalent to 255.255.255.255 or 32 bits of 1's.
    _ALL_ONES = 0xffffffff
    _version = 4

    def __init__(self, ipaddr):
        """Instantiate a new IPv4 object.

        Args:
            ipaddr: A string or integer representing the IP [& network].
              '192.168.1.1/32'
              '192.168.1.1/255.255.255.255'
              '192.168.1.1/0.0.0.255'
              '192.168.1.1'
              are all functionally the same in IPv4. That is to say,
              failing to provide a subnetmask will create an object with
              a mask of /32. A netmask of '255.255.255.255' is assumed
              to be /32 and '0.0.0.0' is assumed to be /0, even though
              other netmasks can be expressed both as host- and
              net-masks. (255.0.0.0 == 0.255.255.255)

              Additionally, an integer can be passed, so
                  IPv4('192.168.1.1') == IPv4(3232235777).
              or, more generally
                  IPv4(IPv4('192.168.1.1').ip) == IPv4('192.168.1.1')

        Raises:
            IPv4IpValidationError: If ipaddr isn't a valid IPv4 address.
            IPv4NetmaskValidationError: If the netmask isn't valid for
              an IPv4 address.

        """
        BaseIP.__init__(self)

        # Efficient constructor from integer.
        if isinstance(ipaddr, int):
            self.ip = ipaddr
            self._prefixlen = 32
            self.netmask = self._ALL_ONES
            if ipaddr < 0 or ipaddr > self._ALL_ONES:
                raise IPv4IpValidationError(ipaddr)
            return

        # Constructing from a packed address
        if isinstance(ipaddr, (bytes, bytearray)) and len(ipaddr) == 4:
            self.ip = struct.unpack('!I', ipaddr)[0]
            self._prefixlen = 32
            self.netmask = self._ALL_ONES
            return

        # Assume input argument to be string or any object representation
        # which converts into a formatted IP prefix string.
        addr = str(ipaddr).split('/')

        if len(addr) > 2:
            raise IPv4IpValidationError(ipaddr)

        if not self._is_valid_ip(addr[0]):
            raise IPv4IpValidationError(addr[0])

        self.ip = self._ip_int_from_string(addr[0])

        if len(addr) == 2:
            mask = addr[1].split('.')
            if len(mask) == 4:
                # We have dotted decimal netmask.
                if not self._is_valid_netmask(addr[1]):
                    raise IPv4NetmaskValidationError(addr[1])
                if self._is_hostmask(addr[1]):
                    self.netmask = (
                        self._ip_int_from_string(addr[1]) ^ self._ALL_ONES)
                else:
                    self.netmask = self._ip_int_from_string(addr[1])
                self._prefixlen = self._prefix_from_ip_int(self.netmask)
            else:
                # We have a netmask in prefix length form.
                if not self._is_valid_netmask(addr[1]):
                    raise IPv4NetmaskValidationError(addr[1])
                self._prefixlen = int(addr[1])
                self.netmask = self._ip_int_from_prefix(self._prefixlen)
        else:
            self._prefixlen = 32
            self.netmask = self._ip_int_from_prefix(self._prefixlen)

    def _set_prefix(self, prefixlen):
        """Change the prefix length.

        Args:
            prefixlen: An integer, the new prefix length.

        Raises:
            IPv4NetmaskValidationError: If prefixlen is out of bounds.

        """
        if not 0 <= prefixlen <= 32:
            raise IPv4NetmaskValidationError(prefixlen)
        self._prefixlen = prefixlen
        self.netmask = self._ip_int_from_prefix(self._prefixlen)

    def subnet(self, prefixlen_diff=1):
        """The subnets which join to make the current subnet.

        In the case that self contains only one IP
        (self._prefixlen == 32), return a list with just ourself.

        Args:
            prefixlen_diff: An integer, the amount the prefix length
              should be increased by.  Given a /24 network and a
              prefixlen_diff of 3, for example, 8 subnets of size /27
              will be returned.  The default value of 1 splits the
              current network into two halves.

        Returns:
            A list of IPv4 objects.

        Raises:
            PrefixlenDiffInvalidError: The prefixlen_diff is too small
              or too large.

        """
        if self._prefixlen == 32:
            return [self]

        if prefixlen_diff < 0:
            raise PrefixlenDiffInvalidError('prefix length diff must be > 0')
        new_prefixlen = self.prefixlen + prefixlen_diff

        if not self._is_valid_netmask(str(new_prefixlen)):
            raise PrefixlenDiffInvalidError(
                'prefix length diff %d is invalid for netblock %s' % (
                    new_prefixlen, str(self)))

        first = IPv4(
            self._string_from_ip_int(self.network) + '/' +
            str(self._prefixlen + prefixlen_diff))
        subnets = [first]
        current = first
        while True:
            broadcast = current.broadcast
            if broadcast == self.broadcast:
                break
            current = IPv4(self._string_from_ip_int(broadcast + 1) + '/' +
                           str(new_prefixlen))
            subnets.append(current)

        return subnets

    def supernet(self, prefixlen_diff=1):
        """The supernet containing the current network.

        Args:
            prefixlen_diff: An integer, the amount the prefix length of
              the network should be decreased by.  For example, given a
              /24 network and a prefixlen_diff of 3, a supernet with a
              /21 netmask is returned.

        Returns:
            An IPv4 object.

        Raises:
            PrefixlenDiffInvalidError: If
              self.prefixlen - prefixlen_diff < 0. I.e., you have a
              negative prefix length.

        """
        if self.prefixlen == 0:
            return self
        if self.prefixlen - prefixlen_diff < 0:
            raise PrefixlenDiffInvalidError(
                'current prefixlen is %d, cannot have a prefixlen_diff of %d' %
                (self.prefixlen, prefixlen_diff))
        return IPv4(self.ip_ext + '/' + str(self.prefixlen - prefixlen_diff))

    @property
    def is_private(self):
        """Test if this address is allocated for private networks.

        Returns:
            A boolean, True if the address is reserved per RFC 1918.

        """
        for network in _IPV4_RFC1918_NETWORKS:
            if self in network:
                return True
        return False

    @property
    def is_multicast(self):
        """Test if the address is reserved for multicast use.

        Returns:
            A boolean, True if the address is multicast.
            See RFC 3171 for details.

        """
        return self in _IPV4_RFC3171_MULTICAST

    @property
    def is_loopback(self):
        """Test if the address is a loopback adddress.

        Returns:
            A boolean, True if the address is a loopback per RFC 3330.

        """
        return self in _IPV4_RFC3330_LOOPBACK

    @property
    def is_link_local(self):
        """Test if the address is reserved for link-local.

        Returns:
            A boolean, True if the address is link-local per RFC 3927.

        """
        return self in _IPV4_RFC3927_LINK_LOCAL

    @property
    def version(self):
        return self._version

    @property
    def packed(self):
        """The binary representation of this address."""
        return struct.pack('!I', self.ip)

    def _is_hostmask(self, ip_str):
        """Test if the IP string is a hostmask (rather than a netmask).

        Args:
            ip_str: A string, the potential hostmask.

        Returns:
            A boolean, True if the IP string is a hostmask.

        """
        parts = [int(x) for x in ip_str.split('.')]
        if parts[0] < parts[-1]:
            return True
        return False

    def _ip_int_from_string(self, ip_str):
        """Turn the given IP string into an integer for comparison.

        Args:
            ip_str: A string, the IP address.

        Returns:
            The IP address as an integer.

        """
        packed_ip = 0
        for oc in ip_str.split('.'):
            packed_ip = (packed_ip << 8) | int(oc)
        return packed_ip

    def _string_from_ip_int(self, ip_int):
        """Turns a 32-bit integer into dotted decimal notation.

        Args:
            ip_int: An integer, the IP address.

        Returns:
            The IP address as a string in dotted decimal notation.

        """
        octets = []
        for _ in range(4):
            octets.insert(0, str(ip_int & 0xFF))
            ip_int >>= 8
        return '.'.join(octets)

    def _is_valid_ip(self, ip_str):
        """Validate the dotted decimal notation IP/netmask string.

        Args:
            ip_str: A string, the IP address.

        Returns:
            A boolean, True if the string is a valid dotted decimal IP
            string.

        """
        octets = ip_str.split('.')
        if len(octets) == 1:
            # We have an integer rather than a dotted decimal IP.
            try:
                return int(ip_str) >= 0 and int(ip_str) <= self._ALL_ONES
            except ValueError:
                return False

        if len(octets) != 4:
            return False

        for octet in octets:
            try:
                if not 0 <= int(octet) <= 255:
                    return False
            except ValueError:
                return False
        return True

    def _is_valid_netmask(self, netmask):
        """Verify that the netmask is valid.

        Args:
            netmask: A string, either a prefix or dotted decimal
              netmask.

        Returns:
            A boolean, True if the prefix represents a valid IPv4
            netmask.

        """
        if len(netmask.split('.')) == 4:
            return self._is_valid_ip(netmask)
        try:
            netmask = int(netmask)
        except ValueError:
            return False
        return 0 <= netmask <= 32


class IPv6(BaseIP):

    """This class respresents and manipulates 128-bit IPv6 addresses.

    >>> addr = IPv6('2001:658:22A:CAFE:200::1/64')
    >>> for attr in ['ip', 'ip_ext', 'ip_ext_full', 'network', 'network_ext',
    ...              'hostmask', 'hostmask_ext', 'broadcast', 'broadcast_ext',
    ...              'netmask', 'netmask_ext', 'prefixlen']:
    ...     print(attr, '=', getattr(addr, attr))
    ip = 42540616829182469433547762482097946625
    ip_ext = 2001:658:22a:cafe:200::1
    ip_ext_full = 2001:0658:022a:cafe:0200:0000:0000:0001
    network = 42540616829182469433403647294022090752
    network_ext = 2001:658:22a:cafe::
    hostmask = 18446744073709551615
    hostmask_ext = ::ffff:ffff:ffff:ffff
    broadcast = 42540616829182469451850391367731642367
    broadcast_ext = 2001:658:22a:cafe:ffff:ffff:ffff:ffff
    netmask = 340282366920938463444927863358058659840
    netmask_ext = 64
    prefixlen = 64

    """

    _ALL_ONES = (2**128) - 1
    _version = 6

    def __init__(self, ipaddr):
        """Instantiate a new IPv6 object.

        Args:
            ipaddr: A string or integer representing the IP or the IP
              and prefix/netmask.
              '2001:4860::/128'
              '2001:4860:0000:0000:0000:0000:0000:0000/128'
              '2001:4860::'
              are all functionally the same in IPv6.  That is to say,
              failing to provide a subnetmask will create an object with
              a mask of /128.

              Additionally, an integer can be passed, so
                  IPv6('2001:4860::') ==
                  IPv6(42541956101370907050197289607612071936L).
              or, more generally
                  IPv6(IPv6('2001:4860::').ip) == IPv6('2001:4860::')

        Raises:
            IPv6IpValidationError: If ipaddr isn't a valid IPv6 address.
            IPv6NetmaskValidationError: If the netmask isn't valid for
              an IPv6 address.

        """
        BaseIP.__init__(self)

        # Efficient constructor from integer.
        if isinstance(ipaddr, int):
            self.ip = ipaddr
            self._prefixlen = 128
            self.netmask = self._ALL_ONES
            if ipaddr < 0 or ipaddr > self._ALL_ONES:
                raise IPv6IpValidationError(ipaddr)
            return

        # Constructing from a packed address
        if isinstance(ipaddr, (bytes, bytearray)) and len(ipaddr) == 16:
            tmp = struct.unpack('!QQ', ipaddr)
            self.ip = (tmp[0] << 64) | tmp[1]
            self._prefixlen = 128
            self.netmask = self._ALL_ONES
            return

        # Assume input argument to be string or any object representation
        # which converts into a formatted IP prefix string.
        addr_str = str(ipaddr)
        if not addr_str:
            raise IPv6IpValidationError('')
        addr = addr_str.split('/')
        if len(addr) > 1:
            if self._is_valid_netmask(addr[1]):
                self._prefixlen = int(addr[1])
            else:
                raise IPv6NetmaskValidationError(addr[1])
        else:
            self._prefixlen = 128

        self.netmask = self._ip_int_from_prefix(self._prefixlen)

        if not self._is_valid_ip(addr[0]):
            raise IPv6IpValidationError(addr[0])

        self.ip = self._ip_int_from_string(addr[0])

    @property
    def ip_ext_full(self):
        """Returns the expanded version of the IPv6 string."""
        return self._explode_shorthand_ip_string(self.ip_ext)

    def _set_prefix(self, prefixlen):
        """Change the prefix length.

        Args:
            prefixlen: An integer, the new prefix length.

        Raises:
            IPv6NetmaskValidationError: If prefixlen is out of bounds.

        """
        if not 0 <= prefixlen <= 128:
            raise IPv6NetmaskValidationError(prefixlen)
        self._prefixlen = prefixlen
        self.netmask = self._ip_int_from_prefix(self.prefixlen)

    def subnet(self, prefixlen_diff=1):
        """The subnets which join to make the current subnet.

        In the case that self contains only one IP
        (self._prefixlen == 128), return a list with just ourself.

        Args:
            prefixlen_diff: An integer, the amount the prefix length
              should be increased by.

        Returns:
            A list of IPv6 objects.

        Raises:
            PrefixlenDiffInvalidError: The prefixlen_diff is too small
              or too large.

        """
        # Preserve original functionality (return [self] if
        # self.prefixlen == 128).
        if self.prefixlen == 128:
            return [self]

        if prefixlen_diff < 0:
            raise PrefixlenDiffInvalidError('Prefix length diff must be > 0')
        new_prefixlen = self.prefixlen + prefixlen_diff
        if not self._is_valid_netmask(str(new_prefixlen)):
            raise PrefixlenDiffInvalidError(
                'Prefix length diff %d is invalid for netblock %s' % (
                new_prefixlen, str(self)))
        first = IPv6(
            self._string_from_ip_int(self.network) + '/' +
            str(self._prefixlen + prefixlen_diff))
        subnets = [first]
        current = first
        while True:
            broadcast = current.broadcast
            if current.broadcast == self.broadcast:
                break
            current = IPv6(self._string_from_ip_int(broadcast + 1) + '/' +
                           str(new_prefixlen))
            subnets.append(current)

        return subnets

    def supernet(self, prefixlen_diff=1):
        """The supernet containing the current network.

        Args:
            prefixlen_diff: An integer, the amount the prefix length of the
              network should be decreased by.  For example, given a /96
              network and a prefixlen_diff of 3, a supernet with a /93
              netmask is returned.

        Returns:
            An IPv6 object.

        Raises:
            PrefixlenDiffInvalidError: If
              self._prefixlen - prefixlen_diff < 0. I.e., you have a
              negative prefix length.

        """
        if self.prefixlen == 0:
            return self
        if self.prefixlen - prefixlen_diff < 0:
            raise PrefixlenDiffInvalidError(
                'current prefixlen is %d, cannot have a prefixlen_diff of %d' %
                (self.prefixlen, prefixlen_diff))
        return IPv6(self.ip_ext + '/' + str(self.prefixlen - prefixlen_diff))

    @property
    def is_multicast(self):
        """Test if the address is reserved for multicast use.

        Returns:
            A boolean, True if the address is a multicast address.
            See RFC 2373 2.7 for details.

        """
        return self in _IPV6_RFC2373_MULTICAST

    @property
    def is_unspecified(self):
        """Test if the address is unspecified.

        Returns:
            A boolean, True if this is the unspecified address as defined in
            RFC 2373 2.5.2.

        """
        return self == _IPV6_RFC2373_UNSPECIFIED

    @property
    def is_loopback(self):
        """Test if the address is a loopback adddress.

        Returns:
            A boolean, True if the address is a loopback address as defined in
            RFC 2373 2.5.3.

        """
        return self == _IPV6_RFC2373_LOOPBACK

    @property
    def is_link_local(self):
        """Test if the address is reserved for link-local.

        Returns:
            A boolean, True if the address is reserved per RFC 4291.

        """
        return self in _IPV6_RFC4291_LINK_LOCAL

    @property
    def is_site_local(self):
        """Test if the address is reserved for site-local.

        Note that the site-local address space has been deprecated by RFC 3879.
        Use is_private to test if this address is in the space of unique local
        addresses as defined by RFC 4193.

        Returns:
            A boolean, True if the address is reserved per RFC 3513 2.5.6.

        """
        return self in _IPV6_RFC3513_SITE_LOCAL

    @property
    def is_private(self):
        """Test if this address is allocated for private networks.

        Returns:
            A boolean, True if the address is reserved per RFC 4193.

        """
        return self in _IPV6_RFC4193_PRIVATE

    @property
    def version(self):
        return self._version

    @property
    def packed(self):
        """The binary representation of this address."""
        return struct.pack('!QQ', self.ip >> 64, self.ip & (2**64 - 1))

    def _is_shorthand_ip(self, ip_str=None):
        """Determine if the address is shortened.

        Args:
            ip_str: A string, the IPv6 address.

        Returns:
            A boolean, True if the address is shortened.

        """
        if ip_str.count('::') == 1:
            return True
        return False

    def _explode_shorthand_ip_string(self, ip_str):
        """Expand a shortened IPv6 address.

        Args:
            ip_str: A string, the IPv6 address.

        Returns:
            A string, the expanded IPv6 address.

        """
        if self._is_shorthand_ip(ip_str):
            new_ip = []
            hextet = ip_str.split('::')
            sep = len(hextet[0].split(':')) + len(hextet[1].split(':'))
            new_ip = hextet[0].split(':')

            for _ in range(8 - sep):
                new_ip.append('0000')
            new_ip += hextet[1].split(':')

            # Now need to make sure every hextet is 4 lower case characters.
            # If a hextet is < 4 characters, we've got missing leading 0's.
            ret_ip = []
            for hextet in new_ip:
                ret_ip.append(('0' * (4 - len(hextet)) + hextet).lower())
            return ':'.join(ret_ip)
        # We've already got a longhand ip_str.
        return ip_str

    def _is_valid_ip(self, ip_str=None):
        """Ensure we have a valid IPv6 address.

        Probably not as exhaustive as it should be.

        Args:
            ip_str: A string, the IPv6 address.

        Returns:
            A boolean, True if this is a valid IPv6 address.

        """
        if not ip_str:
            ip_str = self.ip_ext

        # We need to have at least one ':'.
        if ':' not in ip_str:
            return False

        # We can only have one '::' shortener.
        if ip_str.count('::') > 1:
            return False

        # '::' should be encompassed by start, digits or end.
        if ':::' in ip_str:
            return False

        # A single colon can neither start nor end an address.
        if ((ip_str.startswith(':') and not ip_str.startswith('::')) or
                (ip_str.endswith(':') and not ip_str.endswith('::'))):
            return False

        # If we have no concatenation, we need to have 8 fields with 7 ':'.
        if '::' not in ip_str and ip_str.count(':') != 7:
            # We might have an IPv4 mapped address.
            if ip_str.count('.') != 3:
                return False

        ip_str = self._explode_shorthand_ip_string(ip_str)

        # Now that we have that all squared away, let's check that each of the
        # hextets are between 0x0 and 0xFFFF.
        for hextet in ip_str.split(':'):
            if hextet.count('.') == 3:
                # If we have an IPv4 mapped address, the IPv4 portion has to be
                # at the end of the IPv6 portion.
                if not ip_str.split(':')[-1] == hextet:
                    return False
                try:
                    IPv4(hextet)
                except IPv4IpValidationError:
                    return False
            elif int(hextet, 16) < 0x0 or int(hextet, 16) > 0xFFFF:
                return False
        return True

    def _is_valid_netmask(self, prefixlen):
        """Verify that the netmask/prefixlen is valid.

        Args:
            prefixlen: A string, the netmask in prefix length format.

        Returns:
            A boolean, True if the prefix represents a valid IPv6
            netmask.

        """
        try:
            prefixlen = int(prefixlen)
        except ValueError:
            return False
        return 0 <= prefixlen <= 128

    def _ip_int_from_string(self, ip_str=None):
        """Turn an IPv6 address into an integer.

        Args:
            ip_str: A string, the IPv6 address.

        Returns:
            A long, the IPv6 address.

        """
        if not ip_str:
            ip_str = self.ip_ext

        ip_int = 0

        fields = self._explode_shorthand_ip_string(ip_str).split(':')

        # Do we have an IPv4 mapped (::ffff:a.b.c.d) or compact (::a.b.c.d)
        # address?
        if fields[-1].count('.') == 3:
            ipv4_string = fields.pop()
            ipv4_int = IPv4(ipv4_string).ip
            octets = []
            for _ in range(2):
                octets.append(hex(ipv4_int & 0xFFFF).lstrip('0x').rstrip('L'))
                ipv4_int >>= 16
            fields.extend(reversed(octets))

        for field in fields:
            ip_int = (ip_int << 16) + int(field, 16)

        return ip_int

    def _compress_hextets(self, hextets):
        """Compresses a list of hextets.

        Compresses a list of strings, replacing the longest continuous
        sequence of "0" in the list with "" and adding empty strings at
        the beginning or at the end of the string such that subsequently
        calling ":".join(hextets) will produce the compressed version of
        the IPv6 address.

        Args:
            hextets: A list of strings, the hextets to compress.

        Returns:
            A list of strings.

        """
        best_doublecolon_start = -1
        best_doublecolon_len = 0
        doublecolon_start = -1
        doublecolon_len = 0
        for index in range(len(hextets)):
            if hextets[index] == '0':
                doublecolon_len += 1
                if doublecolon_start == -1:
                    # Start of a sequence of zeros.
                    doublecolon_start = index
                if doublecolon_len > best_doublecolon_len:
                    # This is the longest sequence of zeros so far.
                    best_doublecolon_len = doublecolon_len
                    best_doublecolon_start = doublecolon_start
            else:
                doublecolon_len = 0
                doublecolon_start = -1

        if best_doublecolon_len > 1:
            best_doublecolon_end = (best_doublecolon_start +
                                    best_doublecolon_len)
            # For zeros at the end of the address.
            if best_doublecolon_end == len(hextets):
                hextets += ['']
            hextets[best_doublecolon_start:best_doublecolon_end] = ['']
            # For zeros at the beginning of the address.
            if best_doublecolon_start == 0:
                hextets = [''] + hextets

        return hextets

    def _string_from_ip_int(self, ip_int=None):
        """Turns a 128-bit integer into hexadecimal notation.

        Args:
            ip_int: An integer, the IP address.

        Returns:
            A string, the hexadecimal representation of the address.

        Raises:
            ValueError: The address is bigger than 128 bits of all ones.

        """
        if not ip_int and ip_int != 0:
            ip_int = self.ip

        if ip_int > self._ALL_ONES:
            raise ValueError('IPv6 address is too large')

        hex_str = '%032x' % ip_int
        hextets = []
        for x in range(0, 32, 4):
            hextets.append('%x' % int(hex_str[x:x+4], 16))

        hextets = self._compress_hextets(hextets)
        return ':'.join(hextets)

    @property
    def netmask_ext(self):
        """IPv6 extended netmask.

        We don't deal with netmasks in IPv6 like we do in IPv4.  This is
        here strictly for IPv4 compatibility.  We simply return the
        prefix length.

        Returns:
            An integer.

        """
        return self.prefixlen


# IPv4 constants.
_IPV4_RFC1918_NETWORKS = (IPv4('10.0.0.0/8'),
                          IPv4('172.16.0.0/12'),
                          IPv4('192.168.0.0/16'))
_IPV4_RFC3171_MULTICAST = IPv4('224.0.0.0/4')
_IPV4_RFC3330_LOOPBACK = IPv4('127.0.0.0/8')
_IPV4_RFC3927_LINK_LOCAL = IPv4('169.254.0.0/16')

# IPv6 constants.
_IPV6_RFC2373_MULTICAST = IPv6('ff00::/8')
_IPV6_RFC2373_UNSPECIFIED = IPv6('::')
_IPV6_RFC2373_LOOPBACK = IPv6('::1')
_IPV6_RFC4291_LINK_LOCAL = IPv6('fe80::/10')
_IPV6_RFC3513_SITE_LOCAL = IPv6('fec0::/10')  # Deprecated by RFC3879.
_IPV6_RFC4193_PRIVATE = IPv6('fc00::/7')

if __name__ == '__main__':

    import doctest
    print(doctest.testmod())
