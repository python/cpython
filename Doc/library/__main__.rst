:mod:`__main__` --- CLIs, import-time behavior, and ``__name__ == '__main__'``
==============================================================================

.. module:: __main__
   :synopsis: Command line interfaces, import-time behavior, and ``__name__ == '__main__'``

--------------

In Python, ``__main__`` is not a single mechanism in the language, but in fact
is part of two quite different constructs:

1. The ``__name__ == '__main__'`` statement
2. The ``__main__.py`` file in Python packages

Each of these mechanisms are related to Python modules; how users interact with
them and how they interact with each other. See section :ref:`tut-modules`.


.. _name_is_main:

``__name__ == '__main__'``
---------------------------

``'__main__'`` is the name of the environment where top-level code is run.
"Top-level code" means when a Python module is initialized from an interactive
prompt, from standard input, from a file argument, from a :option:`-c`
argument, or from a :option:`-m` argument, but **not** when it is initialized
from an import statement.  In any of these situations, the module's
``__name__`` is set equal to ``'__main__'``.

The only other context in which Python code is run is when it is imported
through an import statement. In that case, ``__name__`` is set equal to the
module's name: usually the name of the file without the ``.py`` extension.

As a result, a module can discover whether or not it is running in the
top-level environment by checking its own ``__name__``, which allows a common
idiom for conditionally executing code when the module is not initialized from
an import statement::

    if __name__ == '__main__':
        # Execute when the module is not initialized from an import statement.
        ...


Idiomatic Usage
^^^^^^^^^^^^^^^

Putting as few statements as possible in the block below ``if __name___ ==
'__main__'`` can improve code clarity. Most often, a function named *main*
encapsulates the program's primary behavior::

    # echo.py

    import shlex
    import sys


    def echo(phrase: str):
       """A dummy wrapper around print."""
       # for demonstration purposes, you can imagine that there is some
       # valuable and reusable logic inside this function
       print(phrase)


    def main():
        """Echo the input arguments to standard output"""
        echo(shlex.join(sys.argv))


    if __name__ == '__main__':
        sys.exit(main())  # next section explains the use of sys.exit

This has the added benefit of the *echo* function itself being isolated and
importable elsewhere. None of the code in ``echo.py`` will execute at
import-time.


Packaging Considerations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For detailed documentation on Python packaging, see the
`Python Packaging User Guide. <https://packaging.python.org/>`_

*main* functions are often used to create command line tools by specifying them
as entry points for console scripts.  When this is done, pip inserts the
function call into a template script, where the return value of *main* is
passed into sys.exit. For example::

    sys.exit(main())

Since the call to *main* is wrapped in :func:`sys.exit`, the expectation is
that your function will return some value acceptable as an input to
:func:`sys.exit`; typically, an integer or ``None`` (which is implicitly
returned if your function does not have a return statement).

By proactively following this convention ourselves, our module will have the
same behavior when run directly (``python3 echo.py``) as it will have if we
later package it as a console script entry-point in a pip-installable package.
That is why the ``echo.py`` example from earlier used the ``sys.exit(main())``
convention.


``import __main__``
-------------------

All the values in the ``__main__`` namespace can be imported elsewhere in
Python packages. See section :ref:`name_is_main` for a list of where the
``__main__`` package is in different Python execution scenarios.

Here is an example package that consumes the ``__main__`` namespace::

    # namely.py

    import __main__

    def did_user_define_their_name():
        return 'my_name' in dir(__main__)

    def print_user_name():
        if did_user_define_their_name():
            print(__main__.my_name)
        else:
            print('Tell us your name by defining the variable `my_name`!')

The Python REPL is one example of a "top-level environment", so anything
defined in the REPL becomes part of the ``__main__`` package::

    >>> import namely
    >>> namely.did_user_define_their_name()
    False
    >>> namely.print_user_name()
    Tell us your name by defining the variable `my_name`!
    >>> my_name = 'David'
    >>> namely.did_user_define_their_name()
    True
    >>> namely.print_user_name()
    David

The ``__main__`` package is used in the implementation of :mod:`pdb` and
:mod:`rlcompleter`.


``__main__.py`` in Python Packages
----------------------------------

If you are not familiar with Python packages, see section :ref:`tut-packages`.
Most commonly, the ``__main__.py`` file is used to provide a command line
interface for a package. Consider the following hypothetical package,
"bandclass":

.. code-block:: text

   bandclass
     ├── __init__.py
     ├── __main__.py
     └── student.py

``__main__.py`` will be executed when the package itself is invoked
directly from the command line using the :option:`-m` flag. For example::

    python3 -m bandclass

This command will cause ``__main__.py`` to run. For more details about the
:option:`-m` flag, see :mod:`runpy`. How you utilize this mechanism will depend
on the nature of the package you are writing, but in this hypothetical case, it
might make sense to allow the teacher to search for students using
:mod:`argparse`::

    # bandclass/__main__.py

    import sys
    from .student import search_students

    student_name = sys.argv[2] if len(sys.argv) >= 2 else ''
    print('Found student: {search_students(student_name)}')

Note that ``from .student import search_students`` is an example of a relative
import.  This import style must be used when referencing modules within a
package.  For more details, see :ref:`tut-modules`; or, more specifically,
:ref:`intra-package-references`.

For an example of a package using ``__main__.py`` in our standard library, see
:mod:`venv`, and its invocation via ``python3 -m venv [directory]``.
