.. _filesys:

*************************
File and Directory Access
*************************

The modules described in this chapter deal with disk files and directories.  For
example, there are modules for reading the properties of files, manipulating
paths in a portable way, and creating temporary files.  The full list of modules
in this chapter is:


.. toctree::

   pathlib.rst
   os.path.rst
   stat.rst
   filecmp.rst
   tempfile.rst
   glob.rst
   fnmatch.rst
   linecache.rst
   shutil.rst


.. seealso::

   Module :mod:`os`
      Operating system interfaces, including functions to work with files at a
      lower level than Python :term:`file objects <file object>`.

   Module :mod:`io`
      Python's built-in I/O library, including both abstract classes and
      some concrete classes such as file I/O.

   Built-in function :func:`open`
      The standard way to open files for reading and writing with Python.
