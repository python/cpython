:mod:`ftplib` no longer trusts the IP address value returned from the server
in response to the PASV command by default.  This prevents a malicious FTP
server from using the response to probe IPv4 address and port combinations
on the client network.

Code that requires the former vulnerable behavior may set a
``trust_server_pasv_ipv4_address`` attribute on their
:class:`ftplib.FTP` instances to ``True`` to re-enable it.
