
.. _ipc:

*****************************************
Interprocess Communication and Networking
*****************************************

The modules described in this chapter provide mechanisms for different processes
to communicate.

Some modules only work for two processes that are on the same machine, e.g.
:mod:`signal` and :mod:`subprocess`.  Other modules support networking protocols
that two or more processes can used to communicate across machines.

The list of modules described in this chapter is:


.. toctree::

   subprocess.rst
   socket.rst
   ssl.rst
   signal.rst
   popen2.rst
   asyncore.rst
   asynchat.rst
