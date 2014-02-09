.. _ipc:

*****************************************
Interprocess Communication and Networking
*****************************************

The modules described in this chapter provide mechanisms for different processes
to communicate.

Some modules only work for two processes that are on the same machine, e.g.
:mod:`signal` and :mod:`mmap`.  Other modules support networking protocols
that two or more processes can use to communicate across machines.

The list of modules described in this chapter is:


.. toctree::

   socket.rst
   ssl.rst
   select.rst
   selectors.rst
   asyncio.rst
   asyncore.rst
   asynchat.rst
   signal.rst
   mmap.rst
