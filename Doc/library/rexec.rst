:mod:`rexec` --- Restricted execution framework
===============================================

.. module:: rexec
   :synopsis: Basic restricted execution framework.
   :deprecated:

.. deprecated:: 2.6
   The :mod:`rexec` module has been removed in Python 3.0.

.. versionchanged:: 2.3
   Disabled module.

.. warning::

   The documentation has been left in place to help in reading old code that uses
   the module.

This module contains the :class:`RExec` class, which supports :meth:`r_eval`,
:meth:`r_execfile`, :meth:`r_exec`, and :meth:`r_import` methods, which are
restricted versions of the standard Python functions :meth:`eval`,
:meth:`execfile` and the :keyword:`exec` and :keyword:`import` statements. Code
executed in this restricted environment will only have access to modules and
functions that are deemed safe; you can subclass :class:`RExec` to add or remove
capabilities as desired.

.. warning::

   While the :mod:`rexec` module is designed to perform as described below, it does
   have a few known vulnerabilities which could be exploited by carefully written
   code.  Thus it should not be relied upon in situations requiring "production
   ready" security.  In such situations, execution via sub-processes or very
   careful "cleansing" of both code and data to be processed may be necessary.
   Alternatively, help in patching known :mod:`rexec` vulnerabilities would be
   welcomed.

.. note::

   The :class:`RExec` class can prevent code from performing unsafe operations like
   reading or writing disk files, or using TCP/IP sockets.  However, it does not
   protect against code using extremely large amounts of memory or processor time.


.. class:: RExec([hooks[, verbose]])

   Returns an instance of the :class:`RExec` class.

   *hooks* is an instance of the :class:`RHooks` class or a subclass of it. If it
   is omitted or ``None``, the default :class:`RHooks` class is instantiated.
   Whenever the :mod:`rexec` module searches for a module (even a built-in one) or
   reads a module's code, it doesn't actually go out to the file system itself.
   Rather, it calls methods of an :class:`RHooks` instance that was passed to or
   created by its constructor.  (Actually, the :class:`RExec` object doesn't make
   these calls --- they are made by a module loader object that's part of the
   :class:`RExec` object.  This allows another level of flexibility, which can be
   useful when changing the mechanics of :keyword:`import` within the restricted
   environment.)

   By providing an alternate :class:`RHooks` object, we can control the file system
   accesses made to import a module, without changing the actual algorithm that
   controls the order in which those accesses are made.  For instance, we could
   substitute an :class:`RHooks` object that passes all filesystem requests to a
   file server elsewhere, via some RPC mechanism such as ILU.  Grail's applet
   loader uses this to support importing applets from a URL for a directory.

   If *verbose* is true, additional debugging output may be sent to standard
   output.

It is important to be aware that code running in a restricted environment can
still call the :func:`sys.exit` function.  To disallow restricted code from
exiting the interpreter, always protect calls that cause restricted code to run
with a :keyword:`try`/:keyword:`except` statement that catches the
:exc:`SystemExit` exception.  Removing the :func:`sys.exit` function from the
restricted environment is not sufficient --- the restricted code could still use
``raise SystemExit``.  Removing :exc:`SystemExit` is not a reasonable option;
some library code makes use of this and would break were it not available.


.. seealso::

   `Grail Home Page <http://grail.sourceforge.net/>`_
      Grail is a Web browser written entirely in Python.  It uses the :mod:`rexec`
      module as a foundation for supporting Python applets, and can be used as an
      example usage of this module.


.. _rexec-objects:

RExec Objects
-------------

:class:`RExec` instances support the following methods:


.. method:: RExec.r_eval(code)

   *code* must either be a string containing a Python expression, or a compiled
   code object, which will be evaluated in the restricted environment's
   :mod:`__main__` module.  The value of the expression or code object will be
   returned.


.. method:: RExec.r_exec(code)

   *code* must either be a string containing one or more lines of Python code, or a
   compiled code object, which will be executed in the restricted environment's
   :mod:`__main__` module.


.. method:: RExec.r_execfile(filename)

   Execute the Python code contained in the file *filename* in the restricted
   environment's :mod:`__main__` module.

Methods whose names begin with ``s_`` are similar to the functions beginning
with ``r_``, but the code will be granted access to restricted versions of the
standard I/O streams ``sys.stdin``, ``sys.stderr``, and ``sys.stdout``.


.. method:: RExec.s_eval(code)

   *code* must be a string containing a Python expression, which will be evaluated
   in the restricted environment.


.. method:: RExec.s_exec(code)

   *code* must be a string containing one or more lines of Python code, which will
   be executed in the restricted environment.


.. method:: RExec.s_execfile(code)

   Execute the Python code contained in the file *filename* in the restricted
   environment.

:class:`RExec` objects must also support various methods which will be
implicitly called by code executing in the restricted environment. Overriding
these methods in a subclass is used to change the policies enforced by a
restricted environment.


.. method:: RExec.r_import(modulename[, globals[, locals[, fromlist]]])

   Import the module *modulename*, raising an :exc:`ImportError` exception if the
   module is considered unsafe.


.. method:: RExec.r_open(filename[, mode[, bufsize]])

   Method called when :func:`open` is called in the restricted environment.  The
   arguments are identical to those of :func:`open`, and a file object (or a class
   instance compatible with file objects) should be returned.  :class:`RExec`'s
   default behaviour is allow opening any file for reading, but forbidding any
   attempt to write a file.  See the example below for an implementation of a less
   restrictive :meth:`r_open`.


.. method:: RExec.r_reload(module)

   Reload the module object *module*, re-parsing and re-initializing it.


.. method:: RExec.r_unload(module)

   Unload the module object *module* (remove it from the restricted environment's
   ``sys.modules`` dictionary).

And their equivalents with access to restricted standard I/O streams:


.. method:: RExec.s_import(modulename[, globals[, locals[, fromlist]]])

   Import the module *modulename*, raising an :exc:`ImportError` exception if the
   module is considered unsafe.


.. method:: RExec.s_reload(module)

   Reload the module object *module*, re-parsing and re-initializing it.


.. method:: RExec.s_unload(module)

   Unload the module object *module*.

   .. XXX what are the semantics of this?


.. _rexec-extension:

Defining restricted environments
--------------------------------

The :class:`RExec` class has the following class attributes, which are used by
the :meth:`__init__` method.  Changing them on an existing instance won't have
any effect; instead, create a subclass of :class:`RExec` and assign them new
values in the class definition. Instances of the new class will then use those
new values.  All these attributes are tuples of strings.


.. attribute:: RExec.nok_builtin_names

   Contains the names of built-in functions which will *not* be available to
   programs running in the restricted environment.  The value for :class:`RExec` is
   ``('open', 'reload', '__import__')``. (This gives the exceptions, because by far
   the majority of built-in functions are harmless.  A subclass that wants to
   override this variable should probably start with the value from the base class
   and concatenate additional forbidden functions --- when new dangerous built-in
   functions are added to Python, they will also be added to this module.)


.. attribute:: RExec.ok_builtin_modules

   Contains the names of built-in modules which can be safely imported. The value
   for :class:`RExec` is ``('audioop', 'array', 'binascii', 'cmath', 'errno',
   'imageop', 'marshal', 'math', 'md5', 'operator', 'parser', 'regex', 'select',
   'sha', '_sre', 'strop', 'struct', 'time')``.  A similar remark about overriding
   this variable applies --- use the value from the base class as a starting point.


.. attribute:: RExec.ok_path

   Contains the directories which will be searched when an :keyword:`import` is
   performed in the restricted environment.   The value for :class:`RExec` is the
   same as ``sys.path`` (at the time the module is loaded) for unrestricted code.


.. attribute:: RExec.ok_posix_names

   Contains the names of the functions in the :mod:`os` module which will be
   available to programs running in the restricted environment.  The value for
   :class:`RExec` is ``('error', 'fstat', 'listdir', 'lstat', 'readlink', 'stat',
   'times', 'uname', 'getpid', 'getppid', 'getcwd', 'getuid', 'getgid', 'geteuid',
   'getegid')``.

   .. Should this be called ok_os_names?


.. attribute:: RExec.ok_sys_names

   Contains the names of the functions and variables in the :mod:`sys` module which
   will be available to programs running in the restricted environment.  The value
   for :class:`RExec` is ``('ps1', 'ps2', 'copyright', 'version', 'platform',
   'exit', 'maxint')``.


.. attribute:: RExec.ok_file_types

   Contains the file types from which modules are allowed to be loaded. Each file
   type is an integer constant defined in the :mod:`imp` module. The meaningful
   values are :const:`PY_SOURCE`, :const:`PY_COMPILED`, and :const:`C_EXTENSION`.
   The value for :class:`RExec` is ``(C_EXTENSION, PY_SOURCE)``.  Adding
   :const:`PY_COMPILED` in subclasses is not recommended; an attacker could exit
   the restricted execution mode by putting a forged byte-compiled file
   (:file:`.pyc`) anywhere in your file system, for example by writing it to
   :file:`/tmp` or uploading it to the :file:`/incoming` directory of your public
   FTP server.


An example
----------

Let us say that we want a slightly more relaxed policy than the standard
:class:`RExec` class.  For example, if we're willing to allow files in
:file:`/tmp` to be written, we can subclass the :class:`RExec` class::

   class TmpWriterRExec(rexec.RExec):
       def r_open(self, file, mode='r', buf=-1):
           if mode in ('r', 'rb'):
               pass
           elif mode in ('w', 'wb', 'a', 'ab'):
               # check filename : must begin with /tmp/
               if file[:5]!='/tmp/':
                   raise IOError("can't write outside /tmp")
               elif (string.find(file, '/../') >= 0 or
                    file[:3] == '../' or file[-3:] == '/..'):
                   raise IOError("'..' in filename forbidden")
           else: raise IOError("Illegal open() mode")
           return open(file, mode, buf)

Notice that the above code will occasionally forbid a perfectly valid filename;
for example, code in the restricted environment won't be able to open a file
called :file:`/tmp/foo/../bar`.  To fix this, the :meth:`r_open` method would
have to simplify the filename to :file:`/tmp/bar`, which would require splitting
apart the filename and performing various operations on it.  In cases where
security is at stake, it may be preferable to write simple code which is
sometimes overly restrictive, instead of more general code that is also more
complex and may harbor a subtle security hole.
