:mod:`runpy` --- Locating and executing Python modules
======================================================

.. module:: runpy
   :synopsis: Locate and run Python modules without importing them first.
.. moduleauthor:: Nick Coghlan <ncoghlan@gmail.com>

**Source code:** :source:`Lib/runpy.py`

--------------

The :mod:`runpy` module is used to locate and run Python modules without
importing them first. Its main use is to implement the :option:`-m` command
line switch that allows scripts to be located using the Python module
namespace rather than the filesystem.

The :mod:`runpy` module provides two functions:


.. function:: run_module(mod_name, init_globals=None, run_name=None, alter_sys=False)

   Execute the code of the specified module and return the resulting module
   globals dictionary. The module's code is first located using the standard
   import mechanism (refer to :pep:`302` for details) and then executed in a
   fresh module namespace.

   If the supplied module name refers to a package rather than a normal
   module, then that package is imported and the ``__main__`` submodule within
   that package is then executed and the resulting module globals dictionary
   returned.

   The optional dictionary argument *init_globals* may be used to pre-populate
   the module's globals dictionary before the code is executed. The supplied
   dictionary will not be modified. If any of the special global variables
   below are defined in the supplied dictionary, those definitions are
   overridden by :func:`run_module`.

   The special global variables ``__name__``, ``__file__``, ``__cached__``,
   ``__loader__``
   and ``__package__`` are set in the globals dictionary before the module
   code is executed (Note that this is a minimal set of variables - other
   variables may be set implicitly as an interpreter implementation detail).

   ``__name__`` is set to *run_name* if this optional argument is not
   :const:`None`, to ``mod_name + '.__main__'`` if the named module is a
   package and to the *mod_name* argument otherwise.

   ``__file__`` is set to the name provided by the module loader. If the
   loader does not make filename information available, this variable is set
   to :const:`None`.

    ``__cached__`` will be set to ``None``.

   ``__loader__`` is set to the :pep:`302` module loader used to retrieve the
   code for the module (This loader may be a wrapper around the standard
   import mechanism).

   ``__package__`` is set to *mod_name* if the named module is a package and
   to ``mod_name.rpartition('.')[0]`` otherwise.

   If the argument *alter_sys* is supplied and evaluates to :const:`True`,
   then ``sys.argv[0]`` is updated with the value of ``__file__`` and
   ``sys.modules[__name__]`` is updated with a temporary module object for the
   module being executed. Both ``sys.argv[0]`` and ``sys.modules[__name__]``
   are restored to their original values before the function returns.

   Note that this manipulation of :mod:`sys` is not thread-safe. Other threads
   may see the partially initialised module, as well as the altered list of
   arguments. It is recommended that the :mod:`sys` module be left alone when
   invoking this function from threaded code.


   .. versionchanged:: 3.1
      Added ability to execute packages by looking for a ``__main__`` submodule.

   .. versionchanged:: 3.2
      Added ``__cached__`` global variable (see :PEP:`3147`).


.. function:: run_path(file_path, init_globals=None, run_name=None)

   Execute the code at the named filesystem location and return the resulting
   module globals dictionary. As with a script name supplied to the CPython
   command line, the supplied path may refer to a Python source file, a
   compiled bytecode file or a valid sys.path entry containing a ``__main__``
   module (e.g. a zipfile containing a top-level ``__main__.py`` file).

   For a simple script, the specified code is simply executed in a fresh
   module namespace. For a valid sys.path entry (typically a zipfile or
   directory), the entry is first added to the beginning of ``sys.path``. The
   function then looks for and executes a :mod:`__main__` module using the
   updated path. Note that there is no special protection against invoking
   an existing :mod:`__main__` entry located elsewhere on ``sys.path`` if
   there is no such module at the specified location.

   The optional dictionary argument *init_globals* may be used to pre-populate
   the module's globals dictionary before the code is executed. The supplied
   dictionary will not be modified. If any of the special global variables
   below are defined in the supplied dictionary, those definitions are
   overridden by :func:`run_path`.

   The special global variables ``__name__``, ``__file__``, ``__loader__``
   and ``__package__`` are set in the globals dictionary before the module
   code is executed (Note that this is a minimal set of variables - other
   variables may be set implicitly as an interpreter implementation detail).

   ``__name__`` is set to *run_name* if this optional argument is not
   :const:`None` and to ``'<run_path>'`` otherwise.

   ``__file__`` is set to the name provided by the module loader. If the
   loader does not make filename information available, this variable is set
   to :const:`None`. For a simple script, this will be set to ``file_path``.

   ``__loader__`` is set to the :pep:`302` module loader used to retrieve the
   code for the module (This loader may be a wrapper around the standard
   import mechanism). For a simple script, this will be set to :const:`None`.

   ``__package__`` is set to ``__name__.rpartition('.')[0]``.

   A number of alterations are also made to the :mod:`sys` module. Firstly,
   ``sys.path`` may be altered as described above. ``sys.argv[0]`` is updated
   with the value of ``file_path`` and ``sys.modules[__name__]`` is updated
   with a temporary module object for the module being executed. All
   modifications to items in :mod:`sys` are reverted before the function
   returns.

   Note that, unlike :func:`run_module`, the alterations made to :mod:`sys`
   are not optional in this function as these adjustments are essential to
   allowing the execution of sys.path entries. As the thread-safety
   limitations still apply, use of this function in threaded code should be
   either serialised with the import lock or delegated to a separate process.

   .. versionadded:: 3.2

.. seealso::

   :pep:`338` - Executing modules as scripts
      PEP written and implemented by Nick Coghlan.

   :pep:`366` - Main module explicit relative imports
      PEP written and implemented by Nick Coghlan.

   :ref:`using-on-general` - CPython command line details
