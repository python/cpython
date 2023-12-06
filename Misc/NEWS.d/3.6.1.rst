.. bpo: 29723
.. date: 9895
.. nonce: M5omgP
.. release date: 2017-03-21
.. section: Core and Builtins

The ``sys.path[0]`` initialization change for bpo-29139 caused a regression
by revealing an inconsistency in how sys.path is initialized when executing
``__main__`` from a zipfile, directory, or other import location. The
interpreter now consistently avoids ever adding the import location's parent
directory to ``sys.path``, and ensures no other ``sys.path`` entries are
inadvertently modified when inserting the import location named on the
command line.

..

.. bpo: 27593
.. date: 9894
.. nonce: nk7Etn
.. section: Build

fix format of git information used in sys.version

..

.. bpo: 0
.. date: 9893
.. nonce: usKKNQ
.. section: Build

Fix incompatible comment in python.h
