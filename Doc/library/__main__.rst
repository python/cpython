:mod:`__main__` --- Top-level code environment
==============================================

.. module:: __main__
   :synopsis: The environment where top-level code is run.

--------------

``'__main__'`` is the name of the environment where top-level code is run. A
module's ``__name__`` is set equal to ``'__main__'`` when the module is
initialized from an interactive prompt, from standard input, from a file
argument, from a :option:`-c` argument or from a :option:`-m` argument, but
not when it is initialized from an import statement.

A module can discover whether or not it is running in the main environment by
checking its own ``__name__``, which allows a common idiom for conditionally
executing code when the module is not initialized from an import statement::

    if __name__ == '__main__':
        # Execute when the module is not initialized from an import statement.
        main()

For a package, the same effect can be achieved by including a __main__.py
module, the contents of which will be executed when the package is initialized
from a file argument or from a :option:`-m` argument, but not when it is
initialized from an import statement.
