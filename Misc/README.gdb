gdb Support for Python
======================

gdb 7 and later
---------------
In gdb 7, support for extending gdb with Python was added. When CPython is
built you will notice a python-gdb.py file in the root directory of your
checkout. Read the module docstring for details on how to use the file to
enhance gdb for easier debugging of a CPython process.


gdb 6 and earlier
-----------------
The file at ``Misc/gdbinit`` contains a gdb configuration file which provides
extra commands when working with a CPython process. To use the file, either
copy the commands to your personal gdb configuration file or symlink
``~/.gdbinit`` to ``Misc/gdbinit``.
