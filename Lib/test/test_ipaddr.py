# Copyright 2007 Google Inc.
#  Licensed to PSF under a Contributor Agreement.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# See also:  http://code.google.com/p/ipaddr-py/

"""Unittest for ipaddr module."""


import unittest

import ipaddr


class IpaddrUnitTest(unittest.TestCase):

    def setUp(self):
        self.ipv4 = ipaddr.IPv4('1.2.3.4/24')
        self.ipv4_hostmask = ipaddr.IPv4('10.0.0.1/0.255.255.255')
        self.ipv6 = ipaddr.IPv6('2001:658:22a:cafe:200:0:0:1/64')

    def test_repr(self):
        self.assertEqual("IPv4('1.2.3.4/32')", repr(ipaddr.IPv4('1.2.3.4')))
        self.assertEqual("IPv6('::1/128')", repr(ipaddr.IPv6('::1')))

    def test_invalid_strings(self):
        self.assertRaises(ValueError, ipaddr.IP, '')
        self.assertRaises(ValueError, ipaddr.IP, 'www.google.com')
        self.assertRaises(ValueError, ipaddr.IP, '1.2.3')
        self.assertRaises(ValueError, ipaddr.IP, '1.2.3.4.5')
        self.assertRaises(ValueError, ipaddr.IP, '301.2.2.2')
        self.assertRaises(ValueError, ipaddr.IP, '1:2:3:4:5:6:7')
        self.assertRaises(ValueError, ipaddr.IP, '1:2:3:4:5:6:7:')
        self.assertRaises(ValueError, ipaddr.IP, ':2:3:4:5:6:7:8')
        self.assertRaises(ValueError, ipaddr.IP, '1:2:3:4:5:6:7:8:9')
        self.assertRaises(ValueError, ipaddr.IP, '1:2:3:4:5:6:7:8:')
        self.assertRaises(ValueError, ipaddr.IP, '1::3:4:5:6::8')
        self.assertRaises(ValueError, ipaddr.IP, 'a:')
        self.assertRaises(ValueError, ipaddr.IP, ':')
        self.assertRaises(ValueError, ipaddr.IP, ':::')
        self.assertRaises(ValueError, ipaddr.IP, '::a:')
        self.assertRaises(ValueError, ipaddr.IP, '1ffff::')
        self.assertRaises(ValueError, ipaddr.IP, '0xa::')
        self.assertRaises(ValueError, ipaddr.IP, '1:2:3:4:5:6:1a.2.3.4')
        self.assertRaises(ValueError, ipaddr.IP, '1:2:3:4:5:1.2.3.4:8')
        self.assertRaises(ipaddr.IPv4IpValidationError, ipaddr.IPv4, '')
        self.assertRaises(ipaddr.IPv4IpValidationError, ipaddr.IPv4,
                          'google.com')
        self.assertRaises(ipaddr.IPv4IpValidationError, ipaddr.IPv4,
                          '::1.2.3.4')
        self.assertRaises(ipaddr.IPv6IpValidationError, ipaddr.IPv6, '')
        self.assertRaises(ipaddr.IPv6IpValidationError, ipaddr.IPv6,
                          'google.com')
        self.assertRaises(ipaddr.IPv6IpValidationError, ipaddr.IPv6,
                          '1.2.3.4')

    def test_get_network(self):
        self.assertEqual(self.ipv4.network, 16909056)
        self.assertEqual(self.ipv4.network_ext, '1.2.3.0')
        self.assertEqual(self.ipv4_hostmask.network_ext, '10.0.0.0')

        self.assertEqual(self.ipv6.network,
                         42540616829182469433403647294022090752)
        self.assertEqual(self.ipv6.network_ext,
                         '2001:658:22a:cafe::')
        self.assertEqual(self.ipv6.hostmask_ext,
                         '::ffff:ffff:ffff:ffff')

    def test_ip_from_int(self):
        self.assertEqual(self.ipv4.ip, ipaddr.IPv4(16909060).ip)
        self.assertRaises(ipaddr.IPv4IpValidationError,
                          ipaddr.IPv4, 2**32)
        self.assertRaises(ipaddr.IPv4IpValidationError,
                          ipaddr.IPv4, -1)

        self.assertEqual(self.ipv6.ip,
                         ipaddr.IPv6(42540616829182469433547762482097946625).ip)
        self.assertRaises(ipaddr.IPv6IpValidationError,
                          ipaddr.IPv6, 2**128)
        self.assertRaises(ipaddr.IPv6IpValidationError,
                          ipaddr.IPv6, -1)

        self.assertEqual(ipaddr.IP(self.ipv4.ip).version, 4)
        self.assertEqual(ipaddr.IP(self.ipv6.ip).version, 6)

    def test_ip_from_packed(self):
        ip = ipaddr.IP

        self.assertEqual(self.ipv4.ip,
                         ip(b'\x01\x02\x03\x04').ip)
        self.assertEqual(ip('255.254.253.252'),
                         ip(b'\xff\xfe\xfd\xfc'))
        self.assertRaises(ValueError, ipaddr.IP, b'\x00' * 3)
        self.assertRaises(ValueError, ipaddr.IP, b'\x00' * 5)
        self.assertEqual(self.ipv6.ip,
                         ip(b'\x20\x01\x06\x58\x02\x2a\xca\xfe'
                           b'\x02\x00\x00\x00\x00\x00\x00\x01').ip)
        self.assertEqual(ip('ffff:2:3:4:ffff::'),
                         ip(b'\xff\xff\x00\x02\x00\x03\x00\x04' +
                               b'\xff\xff' + b'\x00' * 6))
        self.assertEqual(ip('::'),
                         ip(b'\x00' * 16))
        self.assertRaises(ValueError, ip, b'\x00' * 15)
        self.assertRaises(ValueError, ip, b'\x00' * 17)

    def test_get_ip(self):
        self.assertEqual(self.ipv4.ip, 16909060)
        self.assertEqual(self.ipv4.ip_ext, '1.2.3.4')
        self.assertEqual(self.ipv4.ip_ext_full, '1.2.3.4')
        self.assertEqual(self.ipv4_hostmask.ip_ext, '10.0.0.1')

        self.assertEqual(self.ipv6.ip, 42540616829182469433547762482097946625)
        self.assertEqual(self.ipv6.ip_ext,
                         '2001:658:22a:cafe:200::1')
        self.assertEqual(self.ipv6.ip_ext_full,
                         '2001:0658:022a:cafe:0200:0000:0000:0001')

    def test_get_netmask(self):
        self.assertEqual(self.ipv4.netmask, 4294967040)
        self.assertEqual(self.ipv4.netmask_ext, '255.255.255.0')
        self.assertEqual(self.ipv4_hostmask.netmask_ext, '255.0.0.0')
        self.assertEqual(self.ipv6.netmask,
                         340282366920938463444927863358058659840)
        self.assertEqual(self.ipv6.netmask_ext, 64)

    def test_zero_netmask(self):
        ipv4_zero_netmask = ipaddr.IPv4('1.2.3.4/0')
        self.assertEqual(ipv4_zero_netmask.netmask, 0)
        self.assert_(ipv4_zero_netmask._is_valid_netmask(str(0)))

        ipv6_zero_netmask = ipaddr.IPv6('::1/0')
        self.assertEqual(ipv6_zero_netmask.netmask, 0)
        self.assert_(ipv6_zero_netmask._is_valid_netmask(str(0)))

    def test_get_broadcast(self):
        self.assertEqual(self.ipv4.broadcast, 16909311)
        self.assertEqual(self.ipv4.broadcast_ext, '1.2.3.255')

        self.assertEqual(self.ipv6.broadcast,
                         42540616829182469451850391367731642367)
        self.assertEqual(self.ipv6.broadcast_ext,
                         '2001:658:22a:cafe:ffff:ffff:ffff:ffff')

    def test_get_prefixlen(self):
        self.assertEqual(self.ipv4.prefixlen, 24)

        self.assertEqual(self.ipv6.prefixlen, 64)

    def test_get_supernet(self):
        self.assertEqual(self.ipv4.supernet().prefixlen, 23)
        self.assertEqual(self.ipv4.supernet().network_ext, '1.2.2.0')
        self.assertEqual(ipaddr.IPv4('0.0.0.0/0').supernet(),
                         ipaddr.IPv4('0.0.0.0/0'))

        self.assertEqual(self.ipv6.supernet().prefixlen, 63)
        self.assertEqual(self.ipv6.supernet().network_ext,
                         '2001:658:22a:cafe::')
        self.assertEqual(ipaddr.IPv6('::0/0').supernet(), ipaddr.IPv6('::0/0'))

    def test_get_supernet3(self):
        self.assertEqual(self.ipv4.supernet(3).prefixlen, 21)
        self.assertEqual(self.ipv4.supernet(3).network_ext, '1.2.0.0')

        self.assertEqual(self.ipv6.supernet(3).prefixlen, 61)
        self.assertEqual(self.ipv6.supernet(3).network_ext,
                         '2001:658:22a:caf8::')

    def test_get_subnet(self):
        self.assertEqual(self.ipv4.subnet()[0].prefixlen, 25)
        self.assertEqual(self.ipv4.subnet()[0].network_ext, '1.2.3.0')
        self.assertEqual(self.ipv4.subnet()[1].network_ext, '1.2.3.128')

        self.assertEqual(self.ipv6.subnet()[0].prefixlen, 65)

    def test_get_subnet_for_single32(self):
        ip = ipaddr.IPv4('1.2.3.4/32')
        subnets1 = [str(x) for x in ip.subnet()]
        subnets2 = [str(x) for x in ip.subnet(2)]
        self.assertEqual(subnets1, ['1.2.3.4/32'])
        self.assertEqual(subnets1, subnets2)

    def test_get_subnet_for_single128(self):
        ip = ipaddr.IPv6('::1/128')
        subnets1 = [str(x) for x in ip.subnet()]
        subnets2 = [str(x) for x in ip.subnet(2)]
        self.assertEqual(subnets1, ['::1/128'])
        self.assertEqual(subnets1, subnets2)

    def test_subnet2(self):
        ips = [str(x) for x in self.ipv4.subnet(2)]
        self.assertEqual(
            ips,
            ['1.2.3.0/26', '1.2.3.64/26', '1.2.3.128/26', '1.2.3.192/26'])

        ipsv6 = [str(x) for x in self.ipv6.subnet(2)]
        self.assertEqual(
            ipsv6,
            ['2001:658:22a:cafe::/66',
             '2001:658:22a:cafe:4000::/66',
             '2001:658:22a:cafe:8000::/66',
             '2001:658:22a:cafe:c000::/66'])

    def test_subnet_fails_for_large_cidr_diff(self):
        self.assertRaises(ipaddr.PrefixlenDiffInvalidError, self.ipv4.subnet, 9)
        self.assertRaises(ipaddr.PrefixlenDiffInvalidError, self.ipv6.subnet,
                          65)

    def test_supernet_fails_for_large_cidr_diff(self):
        self.assertRaises(ipaddr.PrefixlenDiffInvalidError, self.ipv4.supernet,
                          25)
        self.assertRaises(ipaddr.PrefixlenDiffInvalidError, self.ipv6.supernet,
                          65)

    def test_subnet_fails_for_negative_cidr_diff(self):
        self.assertRaises(ipaddr.PrefixlenDiffInvalidError, self.ipv4.subnet,
                          -1)
        self.assertRaises(ipaddr.PrefixlenDiffInvalidError, self.ipv6.subnet,
                          -1)

    def test_get_num_hosts(self):
        self.assertEqual(self.ipv4.numhosts, 256)
        self.assertEqual(self.ipv4.subnet()[0].numhosts, 128)
        self.assertEqual(self.ipv4.supernet().numhosts, 512)

        self.assertEqual(self.ipv6.numhosts, 18446744073709551616)
        self.assertEqual(self.ipv6.subnet()[0].numhosts, 9223372036854775808)
        self.assertEqual(self.ipv6.supernet().numhosts, 36893488147419103232)

    def test_contains(self):
        self.assertTrue(ipaddr.IPv4('1.2.3.128/25') in self.ipv4)
        self.assertFalse(ipaddr.IPv4('1.2.4.1/24') in self.ipv4)
        self.assertFalse(self.ipv4 in self.ipv6)
        self.assertFalse(self.ipv6 in self.ipv4)
        self.assertTrue(self.ipv4 in self.ipv4)
        self.assertTrue(self.ipv6 in self.ipv6)

    def test_bad_address(self):
        self.assertRaises(ipaddr.IPv4IpValidationError, ipaddr.IPv4, 'poop')
        self.assertRaises(ipaddr.IPv4IpValidationError,
                          ipaddr.IPv4, '1.2.3.256')

        self.assertRaises(ipaddr.IPv6IpValidationError, ipaddr.IPv6, 'poopv6')
        self.assertRaises(ipaddr.IPv4IpValidationError,
                          ipaddr.IPv4, '1.2.3.4/32/24')

    def test_bad_net_mask(self):
        self.assertRaises(ipaddr.IPv4NetmaskValidationError,
                          ipaddr.IPv4, '1.2.3.4/')
        self.assertRaises(ipaddr.IPv4NetmaskValidationError,
                          ipaddr.IPv4, '1.2.3.4/33')
        self.assertRaises(ipaddr.IPv4NetmaskValidationError,
                          ipaddr.IPv4, '1.2.3.4/254.254.255.256')

        self.assertRaises(ipaddr.IPv6NetmaskValidationError,
                          ipaddr.IPv6, '::1/')
        self.assertRaises(ipaddr.IPv6NetmaskValidationError,
                          ipaddr.IPv6, '::1/129')

    def test_nth(self):
        self.assertEqual(self.ipv4[5], '1.2.3.5')
        self.assertRaises(IndexError, self.ipv4.__getitem__, 256)

        self.assertEqual(self.ipv6[5],
                         '2001:658:22a:cafe::5')

    def test_getitem(self):
        # http://code.google.com/p/ipaddr-py/issues/detail?id=15
        addr = ipaddr.IPv4('172.31.255.128/255.255.255.240')
        self.assertEqual(28, addr.prefixlen)
        addr_list = list(addr)
        self.assertEqual('172.31.255.128', addr_list[0])
        self.assertEqual('172.31.255.128', addr[0])
        self.assertEqual('172.31.255.143', addr_list[-1])
        self.assertEqual('172.31.255.143', addr[-1])
        self.assertEqual(addr_list[-1], addr[-1])

    def test_equals(self):
        self.assertTrue(self.ipv4 == ipaddr.IPv4('1.2.3.4/24'))
        self.assertFalse(self.ipv4 == ipaddr.IPv4('1.2.3.4/23'))
        self.assertFalse(self.ipv4 == ipaddr.IPv4('1.2.3.5/24'))
        self.assertFalse(self.ipv4 == ipaddr.IPv6('::1.2.3.4/24'))
        self.assertFalse(self.ipv4 == '')
        self.assertFalse(self.ipv4 == [])
        self.assertFalse(self.ipv4 == 2)

        self.assertTrue(self.ipv6 ==
            ipaddr.IPv6('2001:658:22a:cafe:200::1/64'))
        self.assertFalse(self.ipv6 ==
            ipaddr.IPv6('2001:658:22a:cafe:200::1/63'))
        self.assertFalse(self.ipv6 ==
            ipaddr.IPv6('2001:658:22a:cafe:200::2/64'))
        self.assertFalse(self.ipv6 == ipaddr.IPv4('1.2.3.4/23'))
        self.assertFalse(self.ipv6 == '')
        self.assertFalse(self.ipv6 == [])
        self.assertFalse(self.ipv6 == 2)

    def test_not_equals(self):
        self.assertFalse(self.ipv4 != ipaddr.IPv4('1.2.3.4/24'))
        self.assertTrue(self.ipv4 != ipaddr.IPv4('1.2.3.4/23'))
        self.assertTrue(self.ipv4 != ipaddr.IPv4('1.2.3.5/24'))
        self.assertTrue(self.ipv4 != ipaddr.IPv6('::1.2.3.4/24'))
        self.assertTrue(self.ipv4 != '')
        self.assertTrue(self.ipv4 != [])
        self.assertTrue(self.ipv4 != 2)

        self.assertFalse(self.ipv6 !=
            ipaddr.IPv6('2001:658:22a:cafe:200::1/64'))
        self.assertTrue(self.ipv6 !=
            ipaddr.IPv6('2001:658:22a:cafe:200::1/63'))
        self.assertTrue(self.ipv6 !=
            ipaddr.IPv6('2001:658:22a:cafe:200::2/64'))
        self.assertTrue(self.ipv6 != ipaddr.IPv4('1.2.3.4/23'))
        self.assertTrue(self.ipv6 != '')
        self.assertTrue(self.ipv6 != [])
        self.assertTrue(self.ipv6 != 2)

    def test_slash32_constructor(self):
        self.assertEquals(str(ipaddr.IPv4('1.2.3.4/255.255.255.255')),
                          '1.2.3.4/32')

    def test_slash128_constructor(self):
        self.assertEquals(str(ipaddr.IPv6('::1/128')),
                                  '::1/128')

    def test_slash0_constructor(self):
        self.assertEquals(str(ipaddr.IPv4('1.2.3.4/0.0.0.0')), '1.2.3.4/0')

    def test_collapsing(self):
        ip1 = ipaddr.IPv4('1.1.0.0/24')
        ip2 = ipaddr.IPv4('1.1.1.0/24')
        ip3 = ipaddr.IPv4('1.1.2.0/24')
        ip4 = ipaddr.IPv4('1.1.3.0/24')
        ip5 = ipaddr.IPv4('1.1.4.0/24')
        # stored in no particular order b/c we want CollapseAddr to call [].sort
        ip6 = ipaddr.IPv4('1.1.0.0/22')
        # check that addreses are subsumed properlly.
        collapsed = ipaddr.collapse_address_list([ip1, ip2, ip3, ip4, ip5, ip6])
        self.assertEqual(collapsed, [ipaddr.IPv4('1.1.0.0/22'),
                                     ipaddr.IPv4('1.1.4.0/24')])
        # test that two addresses are supernet'ed properlly
        collapsed = ipaddr.collapse_address_list([ip1, ip2])
        self.assertEqual(collapsed, [ipaddr.IPv4('1.1.0.0/23')])

        ip_same1 = ip_same2 = ipaddr.IPv4('1.1.1.1/32')
        self.assertEqual(ipaddr.collapse_address_list([ip_same1, ip_same2]),
                         [ip_same1])
        ip1 = ipaddr.IPv6('::2001:1/100')
        ip2 = ipaddr.IPv6('::2002:1/120')
        ip3 = ipaddr.IPv6('::2001:1/96')
        # test that ipv6 addresses are subsumed properly.
        collapsed = ipaddr.collapse_address_list([ip1, ip2, ip3])
        self.assertEqual(collapsed, [ip3])

    def test_network_comparison(self):
        # ip1 and ip2 have the same network address
        ip1 = ipaddr.IPv4('1.1.1.0/24')
        ip2 = ipaddr.IPv4('1.1.1.1/24')
        ip3 = ipaddr.IPv4('1.1.2.0/24')

        self.assertTrue(ip1 < ip3)
        self.assertTrue(ip3 > ip2)

        self.assertEquals(ip1.compare_networks(ip2), 0)
        self.assertTrue(ip1._get_networks_key() == ip2._get_networks_key())
        self.assertEquals(ip1.compare_networks(ip3), -1)
        self.assertTrue(ip1._get_networks_key() < ip3._get_networks_key())

        ip1 = ipaddr.IPv6('2001::2000/96')
        ip2 = ipaddr.IPv6('2001::2001/96')
        ip3 = ipaddr.IPv6('2001:ffff::2000/96')

        self.assertTrue(ip1 < ip3)
        self.assertTrue(ip3 > ip2)
        self.assertEquals(ip1.compare_networks(ip2), 0)
        self.assertTrue(ip1._get_networks_key() == ip2._get_networks_key())
        self.assertEquals(ip1.compare_networks(ip3), -1)
        self.assertTrue(ip1._get_networks_key() < ip3._get_networks_key())

        # Test comparing different protocols
        ipv6 = ipaddr.IPv6('::/0')
        ipv4 = ipaddr.IPv4('0.0.0.0/0')
        self.assertTrue(ipv6 > ipv4)
        self.assertTrue(ipv4 < ipv6)

    def test_embedded_ipv4(self):
        ipv4_string = '192.168.0.1'
        ipv4 = ipaddr.IPv4(ipv4_string)
        v4compat_ipv6 = ipaddr.IPv6('::%s' % ipv4_string)
        self.assertEquals(v4compat_ipv6.ip, ipv4.ip)
        v4mapped_ipv6 = ipaddr.IPv6('::ffff:%s' % ipv4_string)
        self.assertNotEquals(v4mapped_ipv6.ip, ipv4.ip)
        self.assertRaises(ipaddr.IPv6IpValidationError, ipaddr.IPv6,
                          '2001:1.1.1.1:1.1.1.1')

    def test_ip_version(self):
        self.assertEqual(self.ipv4.version, 4)
        self.assertEqual(self.ipv6.version, 6)

    def test_packed(self):
        self.assertEqual(self.ipv4.packed,
                         b'\x01\x02\x03\x04')
        self.assertEqual(ipaddr.IPv4('255.254.253.252').packed,
                         b'\xff\xfe\xfd\xfc')
        self.assertEqual(self.ipv6.packed,
                         b'\x20\x01\x06\x58\x02\x2a\xca\xfe' +
                         b'\x02\x00\x00\x00\x00\x00\x00\x01')
        self.assertEqual(ipaddr.IPv6('ffff:2:3:4:ffff::').packed,
                         b'\xff\xff\x00\x02\x00\x03\x00\x04\xff\xff'
                            + b'\x00' * 6)
        self.assertEqual(ipaddr.IPv6('::1:0:0:0:0').packed,
                         b'\x00' * 6 + b'\x00\x01' + b'\x00' * 8)

    def test_ip_str_from_prefixlen(self):
        ipv4 = ipaddr.IPv4('1.2.3.4/24')
        self.assertEquals(ipv4._ip_string_from_prefix(), '255.255.255.0')
        self.assertEquals(ipv4._ip_string_from_prefix(28), '255.255.255.240')

    def test_ip_type(self):
        ipv4 = ipaddr.IP('1.2.3.4')
        ipv6 = ipaddr.IP('::1.2.3.4')
        self.assertEquals(ipaddr.IPv4, type(ipv4))
        self.assertEquals(ipaddr.IPv6, type(ipv6))

    def test_reserved_ipv4(self):
        self.assertEquals(True, ipaddr.IP('224.1.1.1/31').is_multicast)
        self.assertEquals(False, ipaddr.IP('240.0.0.0').is_multicast)

        self.assertEquals(True, ipaddr.IP('192.168.1.1/17').is_private)
        self.assertEquals(False, ipaddr.IP('192.169.0.0').is_private)
        self.assertEquals(True, ipaddr.IP('10.255.255.255').is_private)
        self.assertEquals(False, ipaddr.IP('11.0.0.0').is_private)
        self.assertEquals(True, ipaddr.IP('172.31.255.255').is_private)
        self.assertEquals(False, ipaddr.IP('172.32.0.0').is_private)

        self.assertEquals(True, ipaddr.IP('169.254.100.200/24').is_link_local)
        self.assertEquals(False, ipaddr.IP('169.255.100.200/24').is_link_local)

        self.assertEquals(True, ipaddr.IP('127.100.200.254/32').is_loopback)
        self.assertEquals(True, ipaddr.IP('127.42.0.0/16').is_loopback)
        self.assertEquals(False, ipaddr.IP('128.0.0.0').is_loopback)

    def test_reserved_ipv6(self):
        ip = ipaddr.IP

        self.assertEquals(True, ip('ffff::').is_multicast)
        self.assertEquals(True, ip(2**128-1).is_multicast)
        self.assertEquals(True, ip('ff00::').is_multicast)
        self.assertEquals(False, ip('fdff::').is_multicast)

        self.assertEquals(True, ip('fecf::').is_site_local)
        self.assertEquals(True, ip('feff:ffff:ffff:ffff::').is_site_local)
        self.assertEquals(False, ip('fbf:ffff::').is_site_local)
        self.assertEquals(False, ip('ff00::').is_site_local)

        self.assertEquals(True, ip('fc00::').is_private)
        self.assertEquals(True, ip('fc00:ffff:ffff:ffff::').is_private)
        self.assertEquals(False, ip('fbff:ffff::').is_private)
        self.assertEquals(False, ip('fe00::').is_private)

        self.assertEquals(True, ip('fea0::').is_link_local)
        self.assertEquals(True, ip('febf:ffff::').is_link_local)
        self.assertEquals(False, ip('fe7f:ffff::').is_link_local)
        self.assertEquals(False, ip('fec0::').is_link_local)

        self.assertEquals(True, ip('0:0::0:01').is_loopback)
        self.assertEquals(False, ip('::1/127').is_loopback)
        self.assertEquals(False, ip('::').is_loopback)
        self.assertEquals(False, ip('::2').is_loopback)

        self.assertEquals(True, ip('0::0').is_unspecified)
        self.assertEquals(False, ip('::1').is_unspecified)
        self.assertEquals(False, ip('::/127').is_unspecified)

    def test_addr_exclude(self):
        addr1 = ipaddr.IP('10.1.1.0/24')
        addr2 = ipaddr.IP('10.1.1.0/26')
        addr3 = ipaddr.IP('10.2.1.0/24')
        self.assertEqual(addr1.address_exclude(addr2),
                         [ipaddr.IP('10.1.1.64/26'),
                          ipaddr.IP('10.1.1.128/25')])
        self.assertRaises(ValueError, addr1.address_exclude, addr3)

    def test_hash(self):
        self.assertEquals(hash(ipaddr.IP('10.1.1.0/24')),
                          hash(ipaddr.IP('10.1.1.0/24')))
        dummy = {}
        dummy[self.ipv4] = None
        dummy[self.ipv6] = None
        self.assertTrue(self.ipv4 in dummy)

    def test_ipv4_prefix_from_int(self):
        addr1 = ipaddr.IP('10.1.1.0/24')
        addr2 = ipaddr.IPv4(addr1.ip)  # clone prefix
        addr2.prefixlen = addr1.prefixlen
        addr3 = ipaddr.IP(123456)

        self.assertEqual(123456, addr3.ip)
        self.assertRaises(ipaddr.IPv4NetmaskValidationError,
                          addr2._set_prefix, -1)
        self.assertEqual(addr1, addr2)
        self.assertEqual(str(addr1), str(addr2))

    def test_ipv6_prefix_from_int(self):
        addr1 = ipaddr.IP('2001:0658:022a:cafe:0200::1/64')
        addr2 = ipaddr.IPv6(addr1.ip)  # clone prefix
        addr2.prefixlen = addr1.prefixlen
        addr3 = ipaddr.IP(123456)

        self.assertEqual(123456, addr3.ip)
        self.assertRaises(ipaddr.IPv6NetmaskValidationError,
                          addr2._set_prefix, -1)
        self.assertEqual(addr1, addr2)
        self.assertEqual(str(addr1), str(addr2))

    def test_copy_constructor(self):
        addr1 = ipaddr.IP('10.1.1.0/24')
        addr2 = ipaddr.IP(addr1)
        addr3 = ipaddr.IP('2001:658:22a:cafe:200::1/64')
        addr4 = ipaddr.IP(addr3)

        self.assertEqual(addr1, addr2)
        self.assertEqual(addr3, addr4)

    def test_compress_ipv6_address(self):
        test_addresses = {
            '1:2:3:4:5:6:7:8': '1:2:3:4:5:6:7:8/128',
            '2001:0:0:4:0:0:0:8': '2001:0:0:4::8/128',
            '2001:0:0:4:5:6:7:8': '2001::4:5:6:7:8/128',
            '2001:0:3:4:5:6:7:8': '2001:0:3:4:5:6:7:8/128',
            '2001:0::3:4:5:6:7:8': '2001:0:3:4:5:6:7:8/128',
            '0:0:3:0:0:0:0:ffff': '0:0:3::ffff/128',
            '0:0:0:4:0:0:0:ffff': '::4:0:0:0:ffff/128',
            '0:0:0:0:5:0:0:ffff': '::5:0:0:ffff/128',
            '1:0:0:4:0:0:7:8': '1::4:0:0:7:8/128',
            '0:0:0:0:0:0:0:0': '::/128',
            '0:0:0:0:0:0:0:0/0': '::/0',
            '0:0:0:0:0:0:0:1': '::1/128',
            '2001:0658:022a:cafe:0000:0000:0000:0000/66':
            '2001:658:22a:cafe::/66',
            }
        for uncompressed, compressed in test_addresses.items():
            self.assertEquals(compressed, str(ipaddr.IPv6(uncompressed)))

    def test_explode_short_hand_ip_str(self):
        addr1 = ipaddr.IPv6('2001::1')
        self.assertEqual('2001:0000:0000:0000:0000:0000:0000:0001',
                         addr1._explode_shorthand_ip_string(addr1.ip_ext))

    def test_int_representation(self):
        self.assertEqual(16909060, int(self.ipv4))
        self.assertEqual(42540616829182469433547762482097946625, int(self.ipv6))

    def test_hex_representation(self):
        self.assertEqual(hex(0x1020304), hex(self.ipv4))

        self.assertEqual(hex(0x20010658022ACAFE0200000000000001),
                         hex(self.ipv6))


if __name__ == '__main__':
    unittest.main()
