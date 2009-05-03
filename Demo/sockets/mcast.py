#!/usr/bin/env python
#
# Send/receive UDP multicast packets.
# Requires that your OS kernel supports IP multicast.
#
# Usage:
#   mcast -s (sender, IPv4)
#   mcast -s -6 (sender, IPv6)
#   mcast    (receivers, IPv4)
#   mcast  -6  (receivers, IPv6)

MYPORT = 8123
MYGROUP_4 = '225.0.0.250'
MYGROUP_6 = 'ff15:7079:7468:6f6e:6465:6d6f:6d63:6173'
MYTTL = 1 # Increase to reach other networks

import ipaddr
import time
import struct
import socket
import sys

def main():
    group = MYGROUP_6 if "-6" in sys.argv[1:] else MYGROUP_4

    if "-s" in sys.argv[1:]:
        sender(group)
    else:
        receiver(group)

def _sockfam(ip):
    """Returns the family argument of socket.socket"""
    if ip.version == 4:
        return socket.AF_INET
    elif ip.version == 6:
        return socket.AF_INET6
    else:
        raise ValueError('IPv' + ip.version + ' is not supported')

def sender(group):
    group_ip = ipaddr.IP(group)

    s = socket.socket(_sockfam(group_ip), socket.SOCK_DGRAM)

    # Set Time-to-live (optional)
    ttl_bin = struct.pack('@i', MYTTL)
    if group_ip.version == 4:
        s.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, ttl_bin)
    else:
        s.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_MULTICAST_HOPS, ttl_bin)

    while True:
        data = repr(time.time()).encode('utf-8') + b'\0'
        s.sendto(data, (group_ip.ip_ext_full, MYPORT))
        time.sleep(1)


def receiver(group):
    group_ip = ipaddr.IP(group)

    # Create a socket
    s = socket.socket(_sockfam(group_ip), socket.SOCK_DGRAM)

    # Allow multiple copies of this program on one machine
    # (not strictly needed)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    # Bind it to the port
    s.bind(('', MYPORT))

    # Join group
    if group_ip.version == 4: # IPv4
        mreq = group_ip.packed + struct.pack('=I', socket.INADDR_ANY)
        s.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
    else:
        mreq = group_ip.packed + struct.pack('@I', 0)
        s.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_JOIN_GROUP, mreq)

    # Loop, printing any data we receive
    while True:
        data, sender = s.recvfrom(1500)
        while data[-1:] == '\0': data = data[:-1] # Strip trailing \0's
        print(str(sender) + '  ' + repr(data))


if __name__ == '__main__':
    main()
