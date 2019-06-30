:mod:`__main__` --- Top-level code environment
==============================================

.. module:: __main__
   :synopsis: The environment where top-level code is run.

--------------

``'__main__'`` is the name of the environment where top-level code is run. A
module's ``__name__`` is set equal to ``'__main__'`` when the module is run
from the file system, from standard input or from the module namespace (with
the :option:`-m` command line switch), but not when it is imported.

A module can discover whether or not it is running in the main environment by
checking its own ``__name__``, which allows a common idiom for conditionally
executing code in a module when it is not imported::

   # Execute only if the module is not imported.
   if __name__ == "__main__":
       main()

For a package, the same effect can be achieved by including a __main__.py
module, the contents of which will be executed when the package is run from the
file system or from the module namespace, but not when it is imported.
