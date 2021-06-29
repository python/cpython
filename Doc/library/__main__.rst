:mod:`__main__` --- CLIs, import-time behavior, and ``__name__ == '__main__'``
==============================================================================

.. module:: __main__
   :synopsis: CLIs, import-time behavior, and ``__name__ == '__main__'``

--------------

In Python, ``__main__`` is not a single mechanism in the language, but in fact
is part of two quite different constructs:

1. The ``__name__ == '__main__'`` statement
2. The ``__main__.py`` file in a Python packages

Each of these mechanisms are related to Python :ref:`tut-modules`; both how
users interact with them as well as how they interact with each other. See
:ref:`tut-modules` for details.


``__name__ == '__main__'``
---------------------------

``'__main__'`` is the name of the environment where top-level code is run.
"Top-level code" means when a Python module is initialized from an interactive
prompt, from standard input, from a file argument, from a :option:`-c` argument
or from a :option:`-m` argument, but **not** when it is initialized from an
import statement.  In any of these situations, the module's ``__name__`` is set
equal to ``'__main__'``.  The only other context in which Python code is run is
when it is imported through an import statement. In that case, ``__name__`` is
set equal to the module's name; usually the name of the file without the
``.py`` extension.

As a result, a module can discover whether or not it is running in the
top-level environment by checking its own ``__name__``, which allows a common
idiom for conditionally executing code when the module is not initialized from
an import statement::

    if __name__ == '__main__':
        # Execute when the module is not initialized from an import statement.
        ...

Design Patterns
^^^^^^^^^^^^^^^

Putting as few statements as possible in the block below ``if __name___ ==
'__main__'`` can improve the clarity of your code. Most often, a function named
``main`` encapuslates the program's "main" behavior, creating this pattern::

    # echo.py

    import sys

    def main(phrase: str):
       "Print the string to standard output"
       print(phrase)

    if __name__ == '__main__':
        main(' '.join(sys.argv))

This has the added benefit of the ``main`` function itself being importable
elsewhere::

    # elsewhere.py

    import sys

    from echo import main as echo_main

    def echo_platform():
       echo_main(sys.platform)


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
     ├── parent.py
     └── student.py

``__main__.py`` will be executed when the package itself is invoked
directly from the command line using the :option:`-m` flag. For example::

    python3 -m bandclass

This command will cause ``__main__.py`` to run. For more details about the
:option:`-m` flag, see :mod:`runpy`. How you utilize this mechanism will depend
on the nature of the package you are writing, but in this hypothetical case, it
might make sense to allow the teacher to search for students or parents using
:mod:`argparse`::

    # bandclass/__main__.py

    import argparse
    import sys

    from .parent import Parents
    from .student import Students

    parser = argparse.ArgumentParser()
    parser.add_argument('--student',
                        help="lookup a student and print their information")
    parser.add_argument('--parent',
                        help="lookup a parent and print their information")

    args = parser.parse_args()

    if args.student and student := Students.find(args.student):
        print(student)
        sys.exit('Student found')
    elif args.parent and parent := Parents.find(args.parent):
        print(parent)
        sys.exit('Parent found')
    else:
        print('Result not found')
        sys.exit(args.print_help())

Note that there is no reason to use the ``if __name__ == '__main__'`` statement
in ``__main__.py`` itself. There is no reason for any other file to import
something from ``__main__.py``, and therefore, ``__name__`` will always be
``'__main__'``; in most cases it would be a redundant statement. There are
exceptions to this norm, though. For example, if you have explicitly identified
``__main__`` as a console script entry point in :file:`setup.py`. See section
:ref:`entry-points`.

For a very popular example of a package using ``__main__.py`` in our standard
library, see :mod:`venv`, and its' invocation via ``python3 -m
venv [directory]``.
