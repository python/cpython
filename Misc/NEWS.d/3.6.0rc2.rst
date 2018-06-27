.. bpo: 28147
.. date: 9795
.. nonce: CnK_xf
.. release date: 2016-12-16
.. section: Core and Builtins

Fix a memory leak in split-table dictionaries: setattr() must not convert
combined table into split table. Patch written by INADA Naoki.

..

.. bpo: 28990
.. date: 9794
.. nonce: m8xRMJ
.. section: Core and Builtins

Fix asyncio SSL hanging if connection is closed before handshake is
completed. (Patch by HoHo-Ho)

..

.. bpo: 28770
.. date: 9793
.. nonce: N9GQsz
.. section: Tools/Demos

Fix python-gdb.py for fastcalls.

..

.. bpo: 28896
.. date: 9792
.. nonce: ymAbmH
.. section: Windows

Deprecate WindowsRegistryFinder.

..

.. bpo: 28898
.. date: 9791
.. nonce: YGUd_i
.. section: Build

Prevent gdb build errors due to HAVE_LONG_LONG redefinition.
