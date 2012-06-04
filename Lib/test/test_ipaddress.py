# Copyright 2007 Google Inc.
#  Licensed to PSF under a Contributor Agreement.

"""Unittest for ipaddress module."""


import unittest
import time
import ipaddress

# Compatibility function to cast str to bytes objects
_cb = lambda bytestr: bytes(bytestr, 'charmap')

class IpaddrUnitTest(unittest.TestCase):

    def setUp(self):
        self.ipv4_address = ipaddress.IPv4Address('1.2.3.4')
        self.ipv4_interface = ipaddress.IPv4Interface('1.2.3.4/24')
        self.ipv4_network = ipaddress.IPv4Network('1.2.3.0/24')
        #self.ipv4_hostmask = ipaddress.IPv4Interface('10.0.0.1/0.255.255.255')
        self.ipv6_address = ipaddress.IPv6Interface(
            '2001:658:22a:cafe:200:0:0:1')
        self.ipv6_interface = ipaddress.IPv6Interface(
            '2001:658:22a:cafe:200:0:0:1/64')
        self.ipv6_network = ipaddress.IPv6Network('2001:658:22a:cafe::/64')

    def testRepr(self):
        self.assertEqual("IPv4Interface('1.2.3.4/32')",
                         repr(ipaddress.IPv4Interface('1.2.3.4')))
        self.assertEqual("IPv6Interface('::1/128')",
                         repr(ipaddress.IPv6Interface('::1')))

    # issue57
    def testAddressIntMath(self):
        self.assertEqual(ipaddress.IPv4Address('1.1.1.1') + 255,
                         ipaddress.IPv4Address('1.1.2.0'))
        self.assertEqual(ipaddress.IPv4Address('1.1.1.1') - 256,
                         ipaddress.IPv4Address('1.1.0.1'))
        self.assertEqual(ipaddress.IPv6Address('::1') + (2**16 - 2),
                         ipaddress.IPv6Address('::ffff'))
        self.assertEqual(ipaddress.IPv6Address('::ffff') - (2**16 - 2),
                         ipaddress.IPv6Address('::1'))

    def testInvalidStrings(self):
        def AssertInvalidIP(ip_str):
            self.assertRaises(ValueError, ipaddress.ip_address, ip_str)
        AssertInvalidIP("")
        AssertInvalidIP("016.016.016.016")
        AssertInvalidIP("016.016.016")
        AssertInvalidIP("016.016")
        AssertInvalidIP("016")
        AssertInvalidIP("000.000.000.000")
        AssertInvalidIP("000")
        AssertInvalidIP("0x0a.0x0a.0x0a.0x0a")
        AssertInvalidIP("0x0a.0x0a.0x0a")
        AssertInvalidIP("0x0a.0x0a")
        AssertInvalidIP("0x0a")
        AssertInvalidIP("42.42.42.42.42")
        AssertInvalidIP("42.42.42")
        AssertInvalidIP("42.42")
        AssertInvalidIP("42")
        AssertInvalidIP("42..42.42")
        AssertInvalidIP("42..42.42.42")
        AssertInvalidIP("42.42.42.42.")
        AssertInvalidIP("42.42.42.42...")
        AssertInvalidIP(".42.42.42.42")
        AssertInvalidIP("...42.42.42.42")
        AssertInvalidIP("42.42.42.-0")
        AssertInvalidIP("42.42.42.+0")
        AssertInvalidIP(".")
        AssertInvalidIP("...")
        AssertInvalidIP("bogus")
        AssertInvalidIP("bogus.com")
        AssertInvalidIP("192.168.0.1.com")
        AssertInvalidIP("12345.67899.-54321.-98765")
        AssertInvalidIP("257.0.0.0")
        AssertInvalidIP("42.42.42.-42")
        AssertInvalidIP("3ffe::1.net")
        AssertInvalidIP("3ffe::1::1")
        AssertInvalidIP("1::2::3::4:5")
        AssertInvalidIP("::7:6:5:4:3:2:")
        AssertInvalidIP(":6:5:4:3:2:1::")
        AssertInvalidIP("2001::db:::1")
        AssertInvalidIP("FEDC:9878")
        AssertInvalidIP("+1.+2.+3.4")
        AssertInvalidIP("1.2.3.4e0")
        AssertInvalidIP("::7:6:5:4:3:2:1:0")
        AssertInvalidIP("7:6:5:4:3:2:1:0::")
        AssertInvalidIP("9:8:7:6:5:4:3::2:1")
        AssertInvalidIP("0:1:2:3::4:5:6:7")
        AssertInvalidIP("3ffe:0:0:0:0:0:0:0:1")
        AssertInvalidIP("3ffe::10000")
        AssertInvalidIP("3ffe::goog")
        AssertInvalidIP("3ffe::-0")
        AssertInvalidIP("3ffe::+0")
        AssertInvalidIP("3ffe::-1")
        AssertInvalidIP(":")
        AssertInvalidIP(":::")
        AssertInvalidIP("::1.2.3")
        AssertInvalidIP("::1.2.3.4.5")
        AssertInvalidIP("::1.2.3.4:")
        AssertInvalidIP("1.2.3.4::")
        AssertInvalidIP("2001:db8::1:")
        AssertInvalidIP(":2001:db8::1")
        AssertInvalidIP(":1:2:3:4:5:6:7")
        AssertInvalidIP("1:2:3:4:5:6:7:")
        AssertInvalidIP(":1:2:3:4:5:6:")

        self.assertRaises(ipaddress.AddressValueError,
                          ipaddress.IPv4Interface, '')
        self.assertRaises(ipaddress.AddressValueError, ipaddress.IPv4Interface,
                          'google.com')
        self.assertRaises(ipaddress.AddressValueError, ipaddress.IPv4Interface,
                          '::1.2.3.4')
        self.assertRaises(ipaddress.AddressValueError,
                          ipaddress.IPv6Interface, '')
        self.assertRaises(ipaddress.AddressValueError, ipaddress.IPv6Interface,
                          'google.com')
        self.assertRaises(ipaddress.AddressValueError, ipaddress.IPv6Interface,
                          '1.2.3.4')
        self.assertRaises(ipaddress.AddressValueError, ipaddress.IPv6Interface,
                          'cafe:cafe::/128/190')
        self.assertRaises(ipaddress.AddressValueError, ipaddress.IPv6Interface,
                          '1234:axy::b')
        self.assertRaises(ipaddress.AddressValueError, ipaddress.IPv6Address,
                          '1234:axy::b')
        self.assertRaises(ipaddress.AddressValueError, ipaddress.IPv6Address,
                          '2001:db8:::1')
        self.assertRaises(ipaddress.AddressValueError, ipaddress.IPv6Address,
                          '2001:888888::1')
        self.assertRaises(ipaddress.AddressValueError,
                          ipaddress.IPv4Address(1)._ip_int_from_string,
                          '1.a.2.3')
        self.assertEqual(False, ipaddress.IPv4Interface(1)._is_hostmask(
                '1.a.2.3'))

    def testGetNetwork(self):
        self.assertEqual(int(self.ipv4_network.network_address), 16909056)
        self.assertEqual(str(self.ipv4_network.network_address), '1.2.3.0')

        self.assertEqual(int(self.ipv6_network.network_address),
                         42540616829182469433403647294022090752)
        self.assertEqual(str(self.ipv6_network.network_address),
                         '2001:658:22a:cafe::')
        self.assertEqual(str(self.ipv6_network.hostmask),
                         '::ffff:ffff:ffff:ffff')

    def testBadVersionComparison(self):
        # These should always raise TypeError
        v4addr = ipaddress.ip_address('1.1.1.1')
        v4net = ipaddress.ip_network('1.1.1.1')
        v6addr = ipaddress.ip_address('::1')
        v6net = ipaddress.ip_address('::1')

        self.assertRaises(TypeError, v4addr.__lt__, v6addr)
        self.assertRaises(TypeError, v4addr.__gt__, v6addr)
        self.assertRaises(TypeError, v4net.__lt__, v6net)
        self.assertRaises(TypeError, v4net.__gt__, v6net)

        self.assertRaises(TypeError, v6addr.__lt__, v4addr)
        self.assertRaises(TypeError, v6addr.__gt__, v4addr)
        self.assertRaises(TypeError, v6net.__lt__, v4net)
        self.assertRaises(TypeError, v6net.__gt__, v4net)

    def testMixedTypeComparison(self):
        v4addr = ipaddress.ip_address('1.1.1.1')
        v4net = ipaddress.ip_network('1.1.1.1/32')
        v6addr = ipaddress.ip_address('::1')
        v6net = ipaddress.ip_network('::1/128')

        self.assertFalse(v4net.__contains__(v6net))
        self.assertFalse(v6net.__contains__(v4net))

        self.assertRaises(TypeError, lambda: v4addr < v4net)
        self.assertRaises(TypeError, lambda: v4addr > v4net)
        self.assertRaises(TypeError, lambda: v4net < v4addr)
        self.assertRaises(TypeError, lambda: v4net > v4addr)

        self.assertRaises(TypeError, lambda: v6addr < v6net)
        self.assertRaises(TypeError, lambda: v6addr > v6net)
        self.assertRaises(TypeError, lambda: v6net < v6addr)
        self.assertRaises(TypeError, lambda: v6net > v6addr)

        # with get_mixed_type_key, you can sort addresses and network.
        self.assertEqual([v4addr, v4net],
                         sorted([v4net, v4addr],
                                key=ipaddress.get_mixed_type_key))
        self.assertEqual([v6addr, v6net],
                         sorted([v6net, v6addr],
                                key=ipaddress.get_mixed_type_key))

    def testIpFromInt(self):
        self.assertEqual(self.ipv4_interface._ip,
                         ipaddress.IPv4Interface(16909060)._ip)
        self.assertRaises(ipaddress.AddressValueError,
                          ipaddress.IPv4Interface, 2**32)
        self.assertRaises(ipaddress.AddressValueError,
                          ipaddress.IPv4Interface, -1)

        ipv4 = ipaddress.ip_network('1.2.3.4')
        ipv6 = ipaddress.ip_network('2001:658:22a:cafe:200:0:0:1')
        self.assertEqual(ipv4, ipaddress.ip_network(int(ipv4)))
        self.assertEqual(ipv6, ipaddress.ip_network(int(ipv6)))

        v6_int = 42540616829182469433547762482097946625
        self.assertEqual(self.ipv6_interface._ip,
                         ipaddress.IPv6Interface(v6_int)._ip)
        self.assertRaises(ipaddress.AddressValueError,
                          ipaddress.IPv6Interface, 2**128)
        self.assertRaises(ipaddress.AddressValueError,
                          ipaddress.IPv6Interface, -1)

        self.assertEqual(ipaddress.ip_network(self.ipv4_address._ip).version, 4)
        self.assertEqual(ipaddress.ip_network(self.ipv6_address._ip).version, 6)

    def testIpFromPacked(self):
        ip = ipaddress.ip_network

        self.assertEqual(self.ipv4_interface._ip,
                         ipaddress.ip_interface(_cb('\x01\x02\x03\x04'))._ip)
        self.assertEqual(ip('255.254.253.252'),
                         ip(_cb('\xff\xfe\xfd\xfc')))
        self.assertRaises(ValueError, ipaddress.ip_network, _cb('\x00' * 3))
        self.assertRaises(ValueError, ipaddress.ip_network, _cb('\x00' * 5))
        self.assertEqual(self.ipv6_interface.ip,
                         ipaddress.ip_interface(
                _cb('\x20\x01\x06\x58\x02\x2a\xca\xfe'
                    '\x02\x00\x00\x00\x00\x00\x00\x01')).ip)
        self.assertEqual(ip('ffff:2:3:4:ffff::'),
                         ip(_cb('\xff\xff\x00\x02\x00\x03\x00\x04' +
                                '\xff\xff' + '\x00' * 6)))
        self.assertEqual(ip('::'),
                         ip(_cb('\x00' * 16)))
        self.assertRaises(ValueError, ip, _cb('\x00' * 15))
        self.assertRaises(ValueError, ip, _cb('\x00' * 17))

    def testGetIp(self):
        self.assertEqual(int(self.ipv4_interface.ip), 16909060)
        self.assertEqual(str(self.ipv4_interface.ip), '1.2.3.4')

        self.assertEqual(int(self.ipv6_interface.ip),
                         42540616829182469433547762482097946625)
        self.assertEqual(str(self.ipv6_interface.ip),
                         '2001:658:22a:cafe:200::1')

    def testGetNetmask(self):
        self.assertEqual(int(self.ipv4_network.netmask), 4294967040)
        self.assertEqual(str(self.ipv4_network.netmask), '255.255.255.0')
        self.assertEqual(int(self.ipv6_network.netmask),
                         340282366920938463444927863358058659840)
        self.assertEqual(self.ipv6_network.prefixlen, 64)

    def testZeroNetmask(self):
        ipv4_zero_netmask = ipaddress.IPv4Interface('1.2.3.4/0')
        self.assertEqual(int(ipv4_zero_netmask.network.netmask), 0)
        self.assertTrue(ipv4_zero_netmask.network._is_valid_netmask(
                str(0)))

        ipv6_zero_netmask = ipaddress.IPv6Interface('::1/0')
        self.assertEqual(int(ipv6_zero_netmask.network.netmask), 0)
        self.assertTrue(ipv6_zero_netmask.network._is_valid_netmask(
                str(0)))

    def testGetBroadcast(self):
        self.assertEqual(int(self.ipv4_network.broadcast_address), 16909311)
        self.assertEqual(str(self.ipv4_network.broadcast_address), '1.2.3.255')

        self.assertEqual(int(self.ipv6_network.broadcast_address),
                         42540616829182469451850391367731642367)
        self.assertEqual(str(self.ipv6_network.broadcast_address),
                         '2001:658:22a:cafe:ffff:ffff:ffff:ffff')

    def testGetPrefixlen(self):
        self.assertEqual(self.ipv4_interface.prefixlen, 24)
        self.assertEqual(self.ipv6_interface.prefixlen, 64)

    def testGetSupernet(self):
        self.assertEqual(self.ipv4_network.supernet().prefixlen, 23)
        self.assertEqual(str(self.ipv4_network.supernet().network_address),
                         '1.2.2.0')
        self.assertEqual(
            ipaddress.IPv4Interface('0.0.0.0/0').network.supernet(),
            ipaddress.IPv4Network('0.0.0.0/0'))

        self.assertEqual(self.ipv6_network.supernet().prefixlen, 63)
        self.assertEqual(str(self.ipv6_network.supernet().network_address),
                         '2001:658:22a:cafe::')
        self.assertEqual(ipaddress.IPv6Interface('::0/0').network.supernet(),
                         ipaddress.IPv6Network('::0/0'))

    def testGetSupernet3(self):
        self.assertEqual(self.ipv4_network.supernet(3).prefixlen, 21)
        self.assertEqual(str(self.ipv4_network.supernet(3).network_address),
                         '1.2.0.0')

        self.assertEqual(self.ipv6_network.supernet(3).prefixlen, 61)
        self.assertEqual(str(self.ipv6_network.supernet(3).network_address),
                         '2001:658:22a:caf8::')

    def testGetSupernet4(self):
        self.assertRaises(ValueError, self.ipv4_network.supernet,
                          prefixlen_diff=2, new_prefix=1)
        self.assertRaises(ValueError, self.ipv4_network.supernet, new_prefix=25)
        self.assertEqual(self.ipv4_network.supernet(prefixlen_diff=2),
                         self.ipv4_network.supernet(new_prefix=22))

        self.assertRaises(ValueError, self.ipv6_network.supernet,
                          prefixlen_diff=2, new_prefix=1)
        self.assertRaises(ValueError, self.ipv6_network.supernet, new_prefix=65)
        self.assertEqual(self.ipv6_network.supernet(prefixlen_diff=2),
                         self.ipv6_network.supernet(new_prefix=62))

    def testHosts(self):
        self.assertEqual([ipaddress.IPv4Address('2.0.0.0'),
                          ipaddress.IPv4Address('2.0.0.1')],
                         list(ipaddress.ip_network('2.0.0.0/31').hosts()))

    def testFancySubnetting(self):
        self.assertEqual(sorted(self.ipv4_network.subnets(prefixlen_diff=3)),
                         sorted(self.ipv4_network.subnets(new_prefix=27)))
        self.assertRaises(ValueError, list,
                          self.ipv4_network.subnets(new_prefix=23))
        self.assertRaises(ValueError, list,
                          self.ipv4_network.subnets(prefixlen_diff=3,
                                                   new_prefix=27))
        self.assertEqual(sorted(self.ipv6_network.subnets(prefixlen_diff=4)),
                         sorted(self.ipv6_network.subnets(new_prefix=68)))
        self.assertRaises(ValueError, list,
                          self.ipv6_network.subnets(new_prefix=63))
        self.assertRaises(ValueError, list,
                          self.ipv6_network.subnets(prefixlen_diff=4,
                                                   new_prefix=68))

    def testGetSubnets(self):
        self.assertEqual(list(self.ipv4_network.subnets())[0].prefixlen, 25)
        self.assertEqual(str(list(
                    self.ipv4_network.subnets())[0].network_address),
                         '1.2.3.0')
        self.assertEqual(str(list(
                    self.ipv4_network.subnets())[1].network_address),
                         '1.2.3.128')

        self.assertEqual(list(self.ipv6_network.subnets())[0].prefixlen, 65)

    def testGetSubnetForSingle32(self):
        ip = ipaddress.IPv4Network('1.2.3.4/32')
        subnets1 = [str(x) for x in ip.subnets()]
        subnets2 = [str(x) for x in ip.subnets(2)]
        self.assertEqual(subnets1, ['1.2.3.4/32'])
        self.assertEqual(subnets1, subnets2)

    def testGetSubnetForSingle128(self):
        ip = ipaddress.IPv6Network('::1/128')
        subnets1 = [str(x) for x in ip.subnets()]
        subnets2 = [str(x) for x in ip.subnets(2)]
        self.assertEqual(subnets1, ['::1/128'])
        self.assertEqual(subnets1, subnets2)

    def testSubnet2(self):
        ips = [str(x) for x in self.ipv4_network.subnets(2)]
        self.assertEqual(
            ips,
            ['1.2.3.0/26', '1.2.3.64/26', '1.2.3.128/26', '1.2.3.192/26'])

        ipsv6 = [str(x) for x in self.ipv6_network.subnets(2)]
        self.assertEqual(
            ipsv6,
            ['2001:658:22a:cafe::/66',
             '2001:658:22a:cafe:4000::/66',
             '2001:658:22a:cafe:8000::/66',
             '2001:658:22a:cafe:c000::/66'])

    def testSubnetFailsForLargeCidrDiff(self):
        self.assertRaises(ValueError, list,
                          self.ipv4_interface.network.subnets(9))
        self.assertRaises(ValueError, list,
                          self.ipv4_network.subnets(9))
        self.assertRaises(ValueError, list,
                          self.ipv6_interface.network.subnets(65))
        self.assertRaises(ValueError, list,
                          self.ipv6_network.subnets(65))

    def testSupernetFailsForLargeCidrDiff(self):
        self.assertRaises(ValueError,
                          self.ipv4_interface.network.supernet, 25)
        self.assertRaises(ValueError,
                          self.ipv6_interface.network.supernet, 65)

    def testSubnetFailsForNegativeCidrDiff(self):
        self.assertRaises(ValueError, list,
                          self.ipv4_interface.network.subnets(-1))
        self.assertRaises(ValueError, list,
                          self.ipv4_network.subnets(-1))
        self.assertRaises(ValueError, list,
                          self.ipv6_interface.network.subnets(-1))
        self.assertRaises(ValueError, list,
                          self.ipv6_network.subnets(-1))

    def testGetNum_Addresses(self):
        self.assertEqual(self.ipv4_network.num_addresses, 256)
        self.assertEqual(list(self.ipv4_network.subnets())[0].num_addresses, 128)
        self.assertEqual(self.ipv4_network.supernet().num_addresses, 512)

        self.assertEqual(self.ipv6_network.num_addresses, 18446744073709551616)
        self.assertEqual(list(self.ipv6_network.subnets())[0].num_addresses,
                         9223372036854775808)
        self.assertEqual(self.ipv6_network.supernet().num_addresses,
                         36893488147419103232)

    def testContains(self):
        self.assertTrue(ipaddress.IPv4Interface('1.2.3.128/25') in
                        self.ipv4_network)
        self.assertFalse(ipaddress.IPv4Interface('1.2.4.1/24') in
                         self.ipv4_network)
        # We can test addresses and string as well.
        addr1 = ipaddress.IPv4Address('1.2.3.37')
        self.assertTrue(addr1 in self.ipv4_network)
        # issue 61, bad network comparison on like-ip'd network objects
        # with identical broadcast addresses.
        self.assertFalse(ipaddress.IPv4Network('1.1.0.0/16').__contains__(
                ipaddress.IPv4Network('1.0.0.0/15')))

    def testBadAddress(self):
        self.assertRaises(ipaddress.AddressValueError, ipaddress.IPv4Interface,
                          'poop')
        self.assertRaises(ipaddress.AddressValueError,
                          ipaddress.IPv4Interface, '1.2.3.256')

        self.assertRaises(ipaddress.AddressValueError, ipaddress.IPv6Interface,
                          'poopv6')
        self.assertRaises(ipaddress.AddressValueError,
                          ipaddress.IPv4Interface, '1.2.3.4/32/24')
        self.assertRaises(ipaddress.AddressValueError,
                          ipaddress.IPv4Interface, '10/8')
        self.assertRaises(ipaddress.AddressValueError,
                          ipaddress.IPv6Interface, '10/8')


    def testBadNetMask(self):
        self.assertRaises(ipaddress.NetmaskValueError,
                          ipaddress.IPv4Interface, '1.2.3.4/')
        self.assertRaises(ipaddress.NetmaskValueError,
                          ipaddress.IPv4Interface, '1.2.3.4/33')
        self.assertRaises(ipaddress.NetmaskValueError,
                          ipaddress.IPv4Interface, '1.2.3.4/254.254.255.256')
        self.assertRaises(ipaddress.NetmaskValueError,
                          ipaddress.IPv4Interface, '1.1.1.1/240.255.0.0')
        self.assertRaises(ipaddress.NetmaskValueError,
                          ipaddress.IPv6Interface, '::1/')
        self.assertRaises(ipaddress.NetmaskValueError,
                          ipaddress.IPv6Interface, '::1/129')

    def testNth(self):
        self.assertEqual(str(self.ipv4_network[5]), '1.2.3.5')
        self.assertRaises(IndexError, self.ipv4_network.__getitem__, 256)

        self.assertEqual(str(self.ipv6_network[5]),
                         '2001:658:22a:cafe::5')

    def testGetitem(self):
        # http://code.google.com/p/ipaddr-py/issues/detail?id=15
        addr = ipaddress.IPv4Network('172.31.255.128/255.255.255.240')
        self.assertEqual(28, addr.prefixlen)
        addr_list = list(addr)
        self.assertEqual('172.31.255.128', str(addr_list[0]))
        self.assertEqual('172.31.255.128', str(addr[0]))
        self.assertEqual('172.31.255.143', str(addr_list[-1]))
        self.assertEqual('172.31.255.143', str(addr[-1]))
        self.assertEqual(addr_list[-1], addr[-1])

    def testEqual(self):
        self.assertTrue(self.ipv4_interface ==
                        ipaddress.IPv4Interface('1.2.3.4/24'))
        self.assertFalse(self.ipv4_interface ==
                         ipaddress.IPv4Interface('1.2.3.4/23'))
        self.assertFalse(self.ipv4_interface ==
                         ipaddress.IPv6Interface('::1.2.3.4/24'))
        self.assertFalse(self.ipv4_interface == '')
        self.assertFalse(self.ipv4_interface == [])
        self.assertFalse(self.ipv4_interface == 2)

        self.assertTrue(self.ipv6_interface ==
            ipaddress.IPv6Interface('2001:658:22a:cafe:200::1/64'))
        self.assertFalse(self.ipv6_interface ==
            ipaddress.IPv6Interface('2001:658:22a:cafe:200::1/63'))
        self.assertFalse(self.ipv6_interface ==
                         ipaddress.IPv4Interface('1.2.3.4/23'))
        self.assertFalse(self.ipv6_interface == '')
        self.assertFalse(self.ipv6_interface == [])
        self.assertFalse(self.ipv6_interface == 2)

    def testNotEqual(self):
        self.assertFalse(self.ipv4_interface !=
                         ipaddress.IPv4Interface('1.2.3.4/24'))
        self.assertTrue(self.ipv4_interface !=
                        ipaddress.IPv4Interface('1.2.3.4/23'))
        self.assertTrue(self.ipv4_interface !=
                        ipaddress.IPv6Interface('::1.2.3.4/24'))
        self.assertTrue(self.ipv4_interface != '')
        self.assertTrue(self.ipv4_interface != [])
        self.assertTrue(self.ipv4_interface != 2)

        self.assertTrue(self.ipv4_address !=
                         ipaddress.IPv4Address('1.2.3.5'))
        self.assertTrue(self.ipv4_address != '')
        self.assertTrue(self.ipv4_address != [])
        self.assertTrue(self.ipv4_address != 2)

        self.assertFalse(self.ipv6_interface !=
            ipaddress.IPv6Interface('2001:658:22a:cafe:200::1/64'))
        self.assertTrue(self.ipv6_interface !=
            ipaddress.IPv6Interface('2001:658:22a:cafe:200::1/63'))
        self.assertTrue(self.ipv6_interface !=
                        ipaddress.IPv4Interface('1.2.3.4/23'))
        self.assertTrue(self.ipv6_interface != '')
        self.assertTrue(self.ipv6_interface != [])
        self.assertTrue(self.ipv6_interface != 2)

        self.assertTrue(self.ipv6_address !=
                        ipaddress.IPv4Address('1.2.3.4'))
        self.assertTrue(self.ipv6_address != '')
        self.assertTrue(self.ipv6_address != [])
        self.assertTrue(self.ipv6_address != 2)

    def testSlash32Constructor(self):
        self.assertEqual(str(ipaddress.IPv4Interface(
                    '1.2.3.4/255.255.255.255')), '1.2.3.4/32')

    def testSlash128Constructor(self):
        self.assertEqual(str(ipaddress.IPv6Interface('::1/128')),
                                  '::1/128')

    def testSlash0Constructor(self):
        self.assertEqual(str(ipaddress.IPv4Interface('1.2.3.4/0.0.0.0')),
                          '1.2.3.4/0')

    def testCollapsing(self):
        # test only IP addresses including some duplicates
        ip1 = ipaddress.IPv4Address('1.1.1.0')
        ip2 = ipaddress.IPv4Address('1.1.1.1')
        ip3 = ipaddress.IPv4Address('1.1.1.2')
        ip4 = ipaddress.IPv4Address('1.1.1.3')
        ip5 = ipaddress.IPv4Address('1.1.1.4')
        ip6 = ipaddress.IPv4Address('1.1.1.0')
        # check that addreses are subsumed properly.
        collapsed = ipaddress.collapse_addresses(
            [ip1, ip2, ip3, ip4, ip5, ip6])
        self.assertEqual(list(collapsed), [ipaddress.IPv4Network('1.1.1.0/30'),
                                           ipaddress.IPv4Network('1.1.1.4/32')])

        # test a mix of IP addresses and networks including some duplicates
        ip1 = ipaddress.IPv4Address('1.1.1.0')
        ip2 = ipaddress.IPv4Address('1.1.1.1')
        ip3 = ipaddress.IPv4Address('1.1.1.2')
        ip4 = ipaddress.IPv4Address('1.1.1.3')
        #ip5 = ipaddress.IPv4Interface('1.1.1.4/30')
        #ip6 = ipaddress.IPv4Interface('1.1.1.4/30')
        # check that addreses are subsumed properly.
        collapsed = ipaddress.collapse_addresses([ip1, ip2, ip3, ip4])
        self.assertEqual(list(collapsed), [ipaddress.IPv4Network('1.1.1.0/30')])

        # test only IP networks
        ip1 = ipaddress.IPv4Network('1.1.0.0/24')
        ip2 = ipaddress.IPv4Network('1.1.1.0/24')
        ip3 = ipaddress.IPv4Network('1.1.2.0/24')
        ip4 = ipaddress.IPv4Network('1.1.3.0/24')
        ip5 = ipaddress.IPv4Network('1.1.4.0/24')
        # stored in no particular order b/c we want CollapseAddr to call [].sort
        ip6 = ipaddress.IPv4Network('1.1.0.0/22')
        # check that addreses are subsumed properly.
        collapsed = ipaddress.collapse_addresses([ip1, ip2, ip3, ip4, ip5,
                                                     ip6])
        self.assertEqual(list(collapsed), [ipaddress.IPv4Network('1.1.0.0/22'),
                                           ipaddress.IPv4Network('1.1.4.0/24')])

        # test that two addresses are supernet'ed properly
        collapsed = ipaddress.collapse_addresses([ip1, ip2])
        self.assertEqual(list(collapsed), [ipaddress.IPv4Network('1.1.0.0/23')])

        # test same IP networks
        ip_same1 = ip_same2 = ipaddress.IPv4Network('1.1.1.1/32')
        self.assertEqual(list(ipaddress.collapse_addresses(
                    [ip_same1, ip_same2])),
                         [ip_same1])

        # test same IP addresses
        ip_same1 = ip_same2 = ipaddress.IPv4Address('1.1.1.1')
        self.assertEqual(list(ipaddress.collapse_addresses(
                    [ip_same1, ip_same2])),
                         [ipaddress.ip_network('1.1.1.1/32')])
        ip1 = ipaddress.IPv6Network('2001::/100')
        ip2 = ipaddress.IPv6Network('2001::/120')
        ip3 = ipaddress.IPv6Network('2001::/96')
        # test that ipv6 addresses are subsumed properly.
        collapsed = ipaddress.collapse_addresses([ip1, ip2, ip3])
        self.assertEqual(list(collapsed), [ip3])

        # the toejam test
        addr_tuples = [
                (ipaddress.ip_address('1.1.1.1'),
                 ipaddress.ip_address('::1')),
                (ipaddress.IPv4Network('1.1.0.0/24'),
                 ipaddress.IPv6Network('2001::/120')),
                (ipaddress.IPv4Network('1.1.0.0/32'),
                 ipaddress.IPv6Network('2001::/128')),
        ]
        for ip1, ip2 in addr_tuples:
            self.assertRaises(TypeError, ipaddress.collapse_addresses,
                              [ip1, ip2])

    def testSummarizing(self):
        #ip = ipaddress.ip_address
        #ipnet = ipaddress.ip_network
        summarize = ipaddress.summarize_address_range
        ip1 = ipaddress.ip_address('1.1.1.0')
        ip2 = ipaddress.ip_address('1.1.1.255')
        # test a /24 is sumamrized properly
        self.assertEqual(list(summarize(ip1, ip2))[0],
                         ipaddress.ip_network('1.1.1.0/24'))
        # test an  IPv4 range that isn't on a network byte boundary
        ip2 = ipaddress.ip_address('1.1.1.8')
        self.assertEqual(list(summarize(ip1, ip2)),
                         [ipaddress.ip_network('1.1.1.0/29'),
                          ipaddress.ip_network('1.1.1.8')])

        ip1 = ipaddress.ip_address('1::')
        ip2 = ipaddress.ip_address('1:ffff:ffff:ffff:ffff:ffff:ffff:ffff')
        # test a IPv6 is sumamrized properly
        self.assertEqual(list(summarize(ip1, ip2))[0],
                         ipaddress.ip_network('1::/16'))
        # test an IPv6 range that isn't on a network byte boundary
        ip2 = ipaddress.ip_address('2::')
        self.assertEqual(list(summarize(ip1, ip2)),
                         [ipaddress.ip_network('1::/16'),
                          ipaddress.ip_network('2::/128')])

        # test exception raised when first is greater than last
        self.assertRaises(ValueError, list,
                          summarize(ipaddress.ip_address('1.1.1.0'),
                                    ipaddress.ip_address('1.1.0.0')))
        # test exception raised when first and last aren't IP addresses
        self.assertRaises(TypeError, list,
                          summarize(ipaddress.ip_network('1.1.1.0'),
                                    ipaddress.ip_network('1.1.0.0')))
        self.assertRaises(TypeError, list,
                          summarize(ipaddress.ip_network('1.1.1.0'),
                                    ipaddress.ip_network('1.1.0.0')))
        # test exception raised when first and last are not same version
        self.assertRaises(TypeError, list,
                          summarize(ipaddress.ip_address('::'),
                                    ipaddress.ip_network('1.1.0.0')))

    def testAddressComparison(self):
        self.assertTrue(ipaddress.ip_address('1.1.1.1') <=
                        ipaddress.ip_address('1.1.1.1'))
        self.assertTrue(ipaddress.ip_address('1.1.1.1') <=
                        ipaddress.ip_address('1.1.1.2'))
        self.assertTrue(ipaddress.ip_address('::1') <=
                        ipaddress.ip_address('::1'))
        self.assertTrue(ipaddress.ip_address('::1') <=
                        ipaddress.ip_address('::2'))

    def testNetworkComparison(self):
        # ip1 and ip2 have the same network address
        ip1 = ipaddress.IPv4Network('1.1.1.0/24')
        ip2 = ipaddress.IPv4Network('1.1.1.1/32')
        ip3 = ipaddress.IPv4Network('1.1.2.0/24')

        self.assertTrue(ip1 < ip3)
        self.assertTrue(ip3 > ip2)

        #self.assertEqual(ip1.compare_networks(ip2), 0)
        #self.assertTrue(ip1._get_networks_key() == ip2._get_networks_key())
        self.assertEqual(ip1.compare_networks(ip3), -1)
        self.assertTrue(ip1._get_networks_key() < ip3._get_networks_key())

        ip1 = ipaddress.IPv6Network('2001:2000::/96')
        ip2 = ipaddress.IPv6Network('2001:2001::/96')
        ip3 = ipaddress.IPv6Network('2001:ffff:2000::/96')

        self.assertTrue(ip1 < ip3)
        self.assertTrue(ip3 > ip2)
        self.assertEqual(ip1.compare_networks(ip3), -1)
        self.assertTrue(ip1._get_networks_key() < ip3._get_networks_key())

        # Test comparing different protocols.
        # Should always raise a TypeError.
        ipv6 = ipaddress.IPv6Interface('::/0')
        ipv4 = ipaddress.IPv4Interface('0.0.0.0/0')
        self.assertRaises(TypeError, ipv4.__lt__, ipv6)
        self.assertRaises(TypeError, ipv4.__gt__, ipv6)
        self.assertRaises(TypeError, ipv6.__lt__, ipv4)
        self.assertRaises(TypeError, ipv6.__gt__, ipv4)

        # Regression test for issue 19.
        ip1 = ipaddress.ip_network('10.1.2.128/25')
        self.assertFalse(ip1 < ip1)
        self.assertFalse(ip1 > ip1)
        ip2 = ipaddress.ip_network('10.1.3.0/24')
        self.assertTrue(ip1 < ip2)
        self.assertFalse(ip2 < ip1)
        self.assertFalse(ip1 > ip2)
        self.assertTrue(ip2 > ip1)
        ip3 = ipaddress.ip_network('10.1.3.0/25')
        self.assertTrue(ip2 < ip3)
        self.assertFalse(ip3 < ip2)
        self.assertFalse(ip2 > ip3)
        self.assertTrue(ip3 > ip2)

        # Regression test for issue 28.
        ip1 = ipaddress.ip_network('10.10.10.0/31')
        ip2 = ipaddress.ip_network('10.10.10.0')
        ip3 = ipaddress.ip_network('10.10.10.2/31')
        ip4 = ipaddress.ip_network('10.10.10.2')
        sorted = [ip1, ip2, ip3, ip4]
        unsorted = [ip2, ip4, ip1, ip3]
        unsorted.sort()
        self.assertEqual(sorted, unsorted)
        unsorted = [ip4, ip1, ip3, ip2]
        unsorted.sort()
        self.assertEqual(sorted, unsorted)
        self.assertRaises(TypeError, ip1.__lt__,
                          ipaddress.ip_address('10.10.10.0'))
        self.assertRaises(TypeError, ip2.__lt__,
                          ipaddress.ip_address('10.10.10.0'))

        # <=, >=
        self.assertTrue(ipaddress.ip_network('1.1.1.1') <=
                        ipaddress.ip_network('1.1.1.1'))
        self.assertTrue(ipaddress.ip_network('1.1.1.1') <=
                        ipaddress.ip_network('1.1.1.2'))
        self.assertFalse(ipaddress.ip_network('1.1.1.2') <=
                        ipaddress.ip_network('1.1.1.1'))
        self.assertTrue(ipaddress.ip_network('::1') <=
                        ipaddress.ip_network('::1'))
        self.assertTrue(ipaddress.ip_network('::1') <=
                        ipaddress.ip_network('::2'))
        self.assertFalse(ipaddress.ip_network('::2') <=
                         ipaddress.ip_network('::1'))

    def testStrictNetworks(self):
        self.assertRaises(ValueError, ipaddress.ip_network, '192.168.1.1/24')
        self.assertRaises(ValueError, ipaddress.ip_network, '::1/120')

    def testOverlaps(self):
        other = ipaddress.IPv4Network('1.2.3.0/30')
        other2 = ipaddress.IPv4Network('1.2.2.0/24')
        other3 = ipaddress.IPv4Network('1.2.2.64/26')
        self.assertTrue(self.ipv4_network.overlaps(other))
        self.assertFalse(self.ipv4_network.overlaps(other2))
        self.assertTrue(other2.overlaps(other3))

    def testEmbeddedIpv4(self):
        ipv4_string = '192.168.0.1'
        ipv4 = ipaddress.IPv4Interface(ipv4_string)
        v4compat_ipv6 = ipaddress.IPv6Interface('::%s' % ipv4_string)
        self.assertEqual(int(v4compat_ipv6.ip), int(ipv4.ip))
        v4mapped_ipv6 = ipaddress.IPv6Interface('::ffff:%s' % ipv4_string)
        self.assertNotEqual(v4mapped_ipv6.ip, ipv4.ip)
        self.assertRaises(ipaddress.AddressValueError, ipaddress.IPv6Interface,
                          '2001:1.1.1.1:1.1.1.1')

    # Issue 67: IPv6 with embedded IPv4 address not recognized.
    def testIPv6AddressTooLarge(self):
        # RFC4291 2.5.5.2
        self.assertEqual(ipaddress.ip_address('::FFFF:192.0.2.1'),
                          ipaddress.ip_address('::FFFF:c000:201'))
        # RFC4291 2.2 (part 3) x::d.d.d.d
        self.assertEqual(ipaddress.ip_address('FFFF::192.0.2.1'),
                          ipaddress.ip_address('FFFF::c000:201'))

    def testIPVersion(self):
        self.assertEqual(self.ipv4_address.version, 4)
        self.assertEqual(self.ipv6_address.version, 6)

    def testMaxPrefixLength(self):
        self.assertEqual(self.ipv4_interface.max_prefixlen, 32)
        self.assertEqual(self.ipv6_interface.max_prefixlen, 128)

    def testPacked(self):
        self.assertEqual(self.ipv4_address.packed,
                         _cb('\x01\x02\x03\x04'))
        self.assertEqual(ipaddress.IPv4Interface('255.254.253.252').packed,
                         _cb('\xff\xfe\xfd\xfc'))
        self.assertEqual(self.ipv6_address.packed,
                         _cb('\x20\x01\x06\x58\x02\x2a\xca\xfe'
                             '\x02\x00\x00\x00\x00\x00\x00\x01'))
        self.assertEqual(ipaddress.IPv6Interface('ffff:2:3:4:ffff::').packed,
                         _cb('\xff\xff\x00\x02\x00\x03\x00\x04\xff\xff'
                            + '\x00' * 6))
        self.assertEqual(ipaddress.IPv6Interface('::1:0:0:0:0').packed,
                         _cb('\x00' * 6 + '\x00\x01' + '\x00' * 8))

    def testIpStrFromPrefixlen(self):
        ipv4 = ipaddress.IPv4Interface('1.2.3.4/24')
        self.assertEqual(ipv4._ip_string_from_prefix(), '255.255.255.0')
        self.assertEqual(ipv4._ip_string_from_prefix(28), '255.255.255.240')

    def testIpType(self):
        ipv4net = ipaddress.ip_network('1.2.3.4')
        ipv4addr = ipaddress.ip_address('1.2.3.4')
        ipv6net = ipaddress.ip_network('::1.2.3.4')
        ipv6addr = ipaddress.ip_address('::1.2.3.4')
        self.assertEqual(ipaddress.IPv4Network, type(ipv4net))
        self.assertEqual(ipaddress.IPv4Address, type(ipv4addr))
        self.assertEqual(ipaddress.IPv6Network, type(ipv6net))
        self.assertEqual(ipaddress.IPv6Address, type(ipv6addr))

    def testReservedIpv4(self):
        # test networks
        self.assertEqual(True, ipaddress.ip_interface(
                '224.1.1.1/31').is_multicast)
        self.assertEqual(False, ipaddress.ip_network('240.0.0.0').is_multicast)

        self.assertEqual(True, ipaddress.ip_interface(
                '192.168.1.1/17').is_private)
        self.assertEqual(False, ipaddress.ip_network('192.169.0.0').is_private)
        self.assertEqual(True, ipaddress.ip_network(
                '10.255.255.255').is_private)
        self.assertEqual(False, ipaddress.ip_network('11.0.0.0').is_private)
        self.assertEqual(True, ipaddress.ip_network(
                '172.31.255.255').is_private)
        self.assertEqual(False, ipaddress.ip_network('172.32.0.0').is_private)

        self.assertEqual(True,
                          ipaddress.ip_interface(
                              '169.254.100.200/24').is_link_local)
        self.assertEqual(False,
                          ipaddress.ip_interface(
                              '169.255.100.200/24').is_link_local)

        self.assertEqual(True,
                          ipaddress.ip_network(
                              '127.100.200.254/32').is_loopback)
        self.assertEqual(True, ipaddress.ip_network(
                '127.42.0.0/16').is_loopback)
        self.assertEqual(False, ipaddress.ip_network('128.0.0.0').is_loopback)

        # test addresses
        self.assertEqual(True, ipaddress.ip_address('0.0.0.0').is_unspecified)
        self.assertEqual(True, ipaddress.ip_address('224.1.1.1').is_multicast)
        self.assertEqual(False, ipaddress.ip_address('240.0.0.0').is_multicast)

        self.assertEqual(True, ipaddress.ip_address('192.168.1.1').is_private)
        self.assertEqual(False, ipaddress.ip_address('192.169.0.0').is_private)
        self.assertEqual(True, ipaddress.ip_address(
                '10.255.255.255').is_private)
        self.assertEqual(False, ipaddress.ip_address('11.0.0.0').is_private)
        self.assertEqual(True, ipaddress.ip_address(
                '172.31.255.255').is_private)
        self.assertEqual(False, ipaddress.ip_address('172.32.0.0').is_private)

        self.assertEqual(True,
                          ipaddress.ip_address('169.254.100.200').is_link_local)
        self.assertEqual(False,
                          ipaddress.ip_address('169.255.100.200').is_link_local)

        self.assertEqual(True,
                          ipaddress.ip_address('127.100.200.254').is_loopback)
        self.assertEqual(True, ipaddress.ip_address('127.42.0.0').is_loopback)
        self.assertEqual(False, ipaddress.ip_address('128.0.0.0').is_loopback)
        self.assertEqual(True, ipaddress.ip_network('0.0.0.0').is_unspecified)

    def testReservedIpv6(self):

        self.assertEqual(True, ipaddress.ip_network('ffff::').is_multicast)
        self.assertEqual(True, ipaddress.ip_network(2**128-1).is_multicast)
        self.assertEqual(True, ipaddress.ip_network('ff00::').is_multicast)
        self.assertEqual(False, ipaddress.ip_network('fdff::').is_multicast)

        self.assertEqual(True, ipaddress.ip_network('fecf::').is_site_local)
        self.assertEqual(True, ipaddress.ip_network(
                'feff:ffff:ffff:ffff::').is_site_local)
        self.assertEqual(False, ipaddress.ip_network(
                'fbf:ffff::').is_site_local)
        self.assertEqual(False, ipaddress.ip_network('ff00::').is_site_local)

        self.assertEqual(True, ipaddress.ip_network('fc00::').is_private)
        self.assertEqual(True, ipaddress.ip_network(
                'fc00:ffff:ffff:ffff::').is_private)
        self.assertEqual(False, ipaddress.ip_network('fbff:ffff::').is_private)
        self.assertEqual(False, ipaddress.ip_network('fe00::').is_private)

        self.assertEqual(True, ipaddress.ip_network('fea0::').is_link_local)
        self.assertEqual(True, ipaddress.ip_network(
                'febf:ffff::').is_link_local)
        self.assertEqual(False, ipaddress.ip_network(
                'fe7f:ffff::').is_link_local)
        self.assertEqual(False, ipaddress.ip_network('fec0::').is_link_local)

        self.assertEqual(True, ipaddress.ip_interface('0:0::0:01').is_loopback)
        self.assertEqual(False, ipaddress.ip_interface('::1/127').is_loopback)
        self.assertEqual(False, ipaddress.ip_network('::').is_loopback)
        self.assertEqual(False, ipaddress.ip_network('::2').is_loopback)

        self.assertEqual(True, ipaddress.ip_network('0::0').is_unspecified)
        self.assertEqual(False, ipaddress.ip_network('::1').is_unspecified)
        self.assertEqual(False, ipaddress.ip_network('::/127').is_unspecified)

        # test addresses
        self.assertEqual(True, ipaddress.ip_address('ffff::').is_multicast)
        self.assertEqual(True, ipaddress.ip_address(2**128-1).is_multicast)
        self.assertEqual(True, ipaddress.ip_address('ff00::').is_multicast)
        self.assertEqual(False, ipaddress.ip_address('fdff::').is_multicast)

        self.assertEqual(True, ipaddress.ip_address('fecf::').is_site_local)
        self.assertEqual(True, ipaddress.ip_address(
                'feff:ffff:ffff:ffff::').is_site_local)
        self.assertEqual(False, ipaddress.ip_address(
                'fbf:ffff::').is_site_local)
        self.assertEqual(False, ipaddress.ip_address('ff00::').is_site_local)

        self.assertEqual(True, ipaddress.ip_address('fc00::').is_private)
        self.assertEqual(True, ipaddress.ip_address(
                'fc00:ffff:ffff:ffff::').is_private)
        self.assertEqual(False, ipaddress.ip_address('fbff:ffff::').is_private)
        self.assertEqual(False, ipaddress.ip_address('fe00::').is_private)

        self.assertEqual(True, ipaddress.ip_address('fea0::').is_link_local)
        self.assertEqual(True, ipaddress.ip_address(
                'febf:ffff::').is_link_local)
        self.assertEqual(False, ipaddress.ip_address(
                'fe7f:ffff::').is_link_local)
        self.assertEqual(False, ipaddress.ip_address('fec0::').is_link_local)

        self.assertEqual(True, ipaddress.ip_address('0:0::0:01').is_loopback)
        self.assertEqual(True, ipaddress.ip_address('::1').is_loopback)
        self.assertEqual(False, ipaddress.ip_address('::2').is_loopback)

        self.assertEqual(True, ipaddress.ip_address('0::0').is_unspecified)
        self.assertEqual(False, ipaddress.ip_address('::1').is_unspecified)

        # some generic IETF reserved addresses
        self.assertEqual(True, ipaddress.ip_address('100::').is_reserved)
        self.assertEqual(True, ipaddress.ip_network('4000::1/128').is_reserved)

    def testIpv4Mapped(self):
        self.assertEqual(ipaddress.ip_address('::ffff:192.168.1.1').ipv4_mapped,
                         ipaddress.ip_address('192.168.1.1'))
        self.assertEqual(ipaddress.ip_address('::c0a8:101').ipv4_mapped, None)
        self.assertEqual(ipaddress.ip_address('::ffff:c0a8:101').ipv4_mapped,
                         ipaddress.ip_address('192.168.1.1'))

    def testAddrExclude(self):
        addr1 = ipaddress.ip_network('10.1.1.0/24')
        addr2 = ipaddress.ip_network('10.1.1.0/26')
        addr3 = ipaddress.ip_network('10.2.1.0/24')
        addr4 = ipaddress.ip_address('10.1.1.0')
        self.assertEqual(sorted(list(addr1.address_exclude(addr2))),
                         [ipaddress.ip_network('10.1.1.64/26'),
                          ipaddress.ip_network('10.1.1.128/25')])
        self.assertRaises(ValueError, list, addr1.address_exclude(addr3))
        self.assertRaises(TypeError, list, addr1.address_exclude(addr4))
        self.assertEqual(list(addr1.address_exclude(addr1)), [])

    def testHash(self):
        self.assertEqual(hash(ipaddress.ip_network('10.1.1.0/24')),
                          hash(ipaddress.ip_network('10.1.1.0/24')))
        self.assertEqual(hash(ipaddress.ip_address('10.1.1.0')),
                          hash(ipaddress.ip_address('10.1.1.0')))
        # i70
        self.assertEqual(hash(ipaddress.ip_address('1.2.3.4')),
                          hash(ipaddress.ip_address(
                    int(ipaddress.ip_address('1.2.3.4')._ip))))
        ip1 = ipaddress.ip_address('10.1.1.0')
        ip2 = ipaddress.ip_address('1::')
        dummy = {}
        dummy[self.ipv4_address] = None
        dummy[self.ipv6_address] = None
        dummy[ip1] = None
        dummy[ip2] = None
        self.assertTrue(self.ipv4_address in dummy)
        self.assertTrue(ip2 in dummy)

    def testCopyConstructor(self):
        addr1 = ipaddress.ip_network('10.1.1.0/24')
        addr2 = ipaddress.ip_network(addr1)
        addr3 = ipaddress.ip_interface('2001:658:22a:cafe:200::1/64')
        addr4 = ipaddress.ip_interface(addr3)
        addr5 = ipaddress.IPv4Address('1.1.1.1')
        addr6 = ipaddress.IPv6Address('2001:658:22a:cafe:200::1')

        self.assertEqual(addr1, addr2)
        self.assertEqual(addr3, addr4)
        self.assertEqual(addr5, ipaddress.IPv4Address(addr5))
        self.assertEqual(addr6, ipaddress.IPv6Address(addr6))

    def testCompressIPv6Address(self):
        test_addresses = {
            '1:2:3:4:5:6:7:8': '1:2:3:4:5:6:7:8/128',
            '2001:0:0:4:0:0:0:8': '2001:0:0:4::8/128',
            '2001:0:0:4:5:6:7:8': '2001::4:5:6:7:8/128',
            '2001:0:3:4:5:6:7:8': '2001:0:3:4:5:6:7:8/128',
            '2001:0:3:4:5:6:7:8': '2001:0:3:4:5:6:7:8/128',
            '0:0:3:0:0:0:0:ffff': '0:0:3::ffff/128',
            '0:0:0:4:0:0:0:ffff': '::4:0:0:0:ffff/128',
            '0:0:0:0:5:0:0:ffff': '::5:0:0:ffff/128',
            '1:0:0:4:0:0:7:8': '1::4:0:0:7:8/128',
            '0:0:0:0:0:0:0:0': '::/128',
            '0:0:0:0:0:0:0:0/0': '::/0',
            '0:0:0:0:0:0:0:1': '::1/128',
            '2001:0658:022a:cafe:0000:0000:0000:0000/66':
            '2001:658:22a:cafe::/66',
            '::1.2.3.4': '::102:304/128',
            '1:2:3:4:5:ffff:1.2.3.4': '1:2:3:4:5:ffff:102:304/128',
            '::7:6:5:4:3:2:1': '0:7:6:5:4:3:2:1/128',
            '::7:6:5:4:3:2:0': '0:7:6:5:4:3:2:0/128',
            '7:6:5:4:3:2:1::': '7:6:5:4:3:2:1:0/128',
            '0:6:5:4:3:2:1::': '0:6:5:4:3:2:1:0/128',
            }
        for uncompressed, compressed in list(test_addresses.items()):
            self.assertEqual(compressed, str(ipaddress.IPv6Interface(
                uncompressed)))

    def testExplodeShortHandIpStr(self):
        addr1 = ipaddress.IPv6Interface('2001::1')
        addr2 = ipaddress.IPv6Address('2001:0:5ef5:79fd:0:59d:a0e5:ba1')
        addr3 = ipaddress.IPv6Network('2001::/96')
        self.assertEqual('2001:0000:0000:0000:0000:0000:0000:0001/128',
                         addr1.exploded)
        self.assertEqual('0000:0000:0000:0000:0000:0000:0000:0001/128',
                         ipaddress.IPv6Interface('::1/128').exploded)
        # issue 77
        self.assertEqual('2001:0000:5ef5:79fd:0000:059d:a0e5:0ba1',
                         addr2.exploded)
        self.assertEqual('2001:0000:0000:0000:0000:0000:0000:0000/96',
                         addr3.exploded)

    def testIntRepresentation(self):
        self.assertEqual(16909060, int(self.ipv4_address))
        self.assertEqual(42540616829182469433547762482097946625,
                         int(self.ipv6_address))

    def testHexRepresentation(self):
        self.assertEqual(hex(0x1020304),
                         hex(self.ipv4_address))

        self.assertEqual(hex(0x20010658022ACAFE0200000000000001),
                         hex(self.ipv6_address))

    def testForceVersion(self):
        self.assertEqual(ipaddress.ip_network(1).version, 4)
        self.assertEqual(ipaddress.IPv6Network(1).version, 6)

    def testWithStar(self):
        self.assertEqual(str(self.ipv4_interface.with_prefixlen), "1.2.3.4/24")
        self.assertEqual(str(self.ipv4_interface.with_netmask),
                         "1.2.3.4/255.255.255.0")
        self.assertEqual(str(self.ipv4_interface.with_hostmask),
                         "1.2.3.4/0.0.0.255")

        self.assertEqual(str(self.ipv6_interface.with_prefixlen),
                         '2001:658:22a:cafe:200::1/64')
        # rfc3513 sec 2.3 says that ipv6 only uses cidr notation for
        # subnets
        self.assertEqual(str(self.ipv6_interface.with_netmask),
                         '2001:658:22a:cafe:200::1/64')
        # this probably don't make much sense, but it's included for
        # compatibility with ipv4
        self.assertEqual(str(self.ipv6_interface.with_hostmask),
                         '2001:658:22a:cafe:200::1/::ffff:ffff:ffff:ffff')

    def testNetworkElementCaching(self):
        # V4 - make sure we're empty
        self.assertFalse('network_address' in self.ipv4_network._cache)
        self.assertFalse('broadcast_address' in self.ipv4_network._cache)
        self.assertFalse('hostmask' in self.ipv4_network._cache)

        # V4 - populate and test
        self.assertEqual(self.ipv4_network.network_address,
                         ipaddress.IPv4Address('1.2.3.0'))
        self.assertEqual(self.ipv4_network.broadcast_address,
                         ipaddress.IPv4Address('1.2.3.255'))
        self.assertEqual(self.ipv4_network.hostmask,
                         ipaddress.IPv4Address('0.0.0.255'))

        # V4 - check we're cached
        self.assertTrue('broadcast_address' in self.ipv4_network._cache)
        self.assertTrue('hostmask' in self.ipv4_network._cache)

        # V6 - make sure we're empty
        self.assertFalse('broadcast_address' in self.ipv6_network._cache)
        self.assertFalse('hostmask' in self.ipv6_network._cache)

        # V6 - populate and test
        self.assertEqual(self.ipv6_network.network_address,
                         ipaddress.IPv6Address('2001:658:22a:cafe::'))
        self.assertEqual(self.ipv6_interface.network.network_address,
                         ipaddress.IPv6Address('2001:658:22a:cafe::'))

        self.assertEqual(
            self.ipv6_network.broadcast_address,
            ipaddress.IPv6Address('2001:658:22a:cafe:ffff:ffff:ffff:ffff'))
        self.assertEqual(self.ipv6_network.hostmask,
                         ipaddress.IPv6Address('::ffff:ffff:ffff:ffff'))
        self.assertEqual(
            self.ipv6_interface.network.broadcast_address,
            ipaddress.IPv6Address('2001:658:22a:cafe:ffff:ffff:ffff:ffff'))
        self.assertEqual(self.ipv6_interface.network.hostmask,
                         ipaddress.IPv6Address('::ffff:ffff:ffff:ffff'))

        # V6 - check we're cached
        self.assertTrue('broadcast_address' in self.ipv6_network._cache)
        self.assertTrue('hostmask' in self.ipv6_network._cache)
        self.assertTrue('broadcast_address' in self.ipv6_interface.network._cache)
        self.assertTrue('hostmask' in self.ipv6_interface.network._cache)

    def testTeredo(self):
        # stolen from wikipedia
        server = ipaddress.IPv4Address('65.54.227.120')
        client = ipaddress.IPv4Address('192.0.2.45')
        teredo_addr = '2001:0000:4136:e378:8000:63bf:3fff:fdd2'
        self.assertEqual((server, client),
                         ipaddress.ip_address(teredo_addr).teredo)
        bad_addr = '2000::4136:e378:8000:63bf:3fff:fdd2'
        self.assertFalse(ipaddress.ip_address(bad_addr).teredo)
        bad_addr = '2001:0001:4136:e378:8000:63bf:3fff:fdd2'
        self.assertFalse(ipaddress.ip_address(bad_addr).teredo)

        # i77
        teredo_addr = ipaddress.IPv6Address('2001:0:5ef5:79fd:0:59d:a0e5:ba1')
        self.assertEqual((ipaddress.IPv4Address('94.245.121.253'),
                          ipaddress.IPv4Address('95.26.244.94')),
                         teredo_addr.teredo)


    def testsixtofour(self):
        sixtofouraddr = ipaddress.ip_address('2002:ac1d:2d64::1')
        bad_addr = ipaddress.ip_address('2000:ac1d:2d64::1')
        self.assertEqual(ipaddress.IPv4Address('172.29.45.100'),
                         sixtofouraddr.sixtofour)
        self.assertFalse(bad_addr.sixtofour)


if __name__ == '__main__':
    unittest.main()
