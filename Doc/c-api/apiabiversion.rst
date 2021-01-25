.. highlight:: c

.. _apiabiversion:

***********************
API and ABI Versioning
***********************

``PY_VERSION_HEX`` is the Python version number encoded in a single integer.

For example if the ``PY_VERSION_HEX`` is set to ``0x030401a2``, the underlying
version information can be found by treating it as a 32 bit number in
the following manner:

   +-------+-------------------------+------------------------------------------------+
   | Bytes | Bits (big endian order) | Meaning                                        |
   +=======+=========================+================================================+
   | ``1`` |       ``1-8``           |  ``PY_MAJOR_VERSION`` (the ``3`` in            |
   |       |                         |  ``3.4.1a2``)                                  |
   +-------+-------------------------+------------------------------------------------+
   | ``2`` |       ``9-16``          |  ``PY_MINOR_VERSION`` (the ``4`` in            |
   |       |                         |  ``3.4.1a2``)                                  |
   +-------+-------------------------+------------------------------------------------+
   | ``3`` |       ``17-24``         |  ``PY_MICRO_VERSION`` (the ``1`` in            |
   |       |                         |  ``3.4.1a2``)                                  |
   +-------+-------------------------+------------------------------------------------+
   | ``4`` |       ``25-28``         |  ``PY_RELEASE_LEVEL`` (``0xA`` for alpha,      |
   |       |                         |  ``0xB`` for beta, ``0xC`` for release         |
   |       |                         |  candidate and ``0xF`` for final), in this     |
   |       |                         |  case it is alpha.                             |
   +-------+-------------------------+------------------------------------------------+
   |       |       ``29-32``         |  ``PY_RELEASE_SERIAL`` (the ``2`` in           |
   |       |                         |  ``3.4.1a2``, zero for final releases)         |
   +-------+-------------------------+------------------------------------------------+

Thus ``3.4.1a2`` is hexversion ``0x030401a2``.

All the given macros are defined in :source:`Include/patchlevel.h`.

