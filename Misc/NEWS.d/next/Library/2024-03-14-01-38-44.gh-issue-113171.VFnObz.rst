Fixed various false positives and false negatives in

* :attr:`ipaddress.IPv4Address.is_private` (see these docs for details)
* :attr:`ipaddress.IPv4Address.is_global`
* :attr:`ipaddress.IPv6Address.is_private`
* :attr:`ipaddress.IPv6Address.is_global`

Also in the corresponding :class:`ipaddress.IPv4Network` and :class:`ipaddress.IPv6Network`
attributes.
