.. _crypto:

**********************
Cryptographic Services
**********************

.. index:: single: cryptography

The modules described in this chapter implement various algorithms of a
cryptographic nature.  They are available at the discretion of the installation.
On Unix systems, the :mod:`crypt` module may also be available.
Here's an overview:


.. toctree::

   hashlib.rst
   hmac.rst

.. index::
   pair: AES; algorithm
   single: cryptography
   single: Kuchling, Andrew

Hardcore cypherpunks will probably find the cryptographic modules written by
A.M. Kuchling of further interest; the package contains modules for various
encryption algorithms, most notably AES.  These modules are not distributed with
Python but available separately.  See the URL http://www.pycrypto.org/ for more
information.
