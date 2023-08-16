:mod:`!xmlrpc` --- XMLRPC server and client modules
===================================================

XML-RPC is a Remote Procedure Call method that uses XML passed via HTTP as a
transport.  With it, a client can call methods with parameters on a remote
server (the server is named by a URI) and get back structured data.

``xmlrpc`` is a package that collects server and client modules implementing
XML-RPC.  The modules are:

* :mod:`xmlrpc.client`
* :mod:`xmlrpc.server`
