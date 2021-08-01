:mod:`__main__` --- CLIs, import-time behavior, and ``__name__ == '__main__'``
==============================================================================

.. module:: __main__
   :synopsis: Command line interfaces, import-time behavior, and ``__name__ == '__main__'``

--------------

In Python, ``__main__`` is not a single mechanism in the language, but in fact
is part of two quite different constructs:

1. The ``__name__ == '__main__'`` statement
2. The ``__main__.py`` file in Python packages

Each of these mechanisms are related to Python modules; how
users interact with them and how they interact with each other. See
section :ref:`tut-modules`.


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
encapsulates the program's primary behavior, creating this pattern::

    # echo.py

    import shlex
    import sys

    def echo(phrase: str):
       # you can imagine that this dummy wrapper around print might be
       # different and truly worth re-using in a real program.
       print(phrase)

    def main():
        "Echo the sys.argv to standard output"
        echo(shlex.join(sys.argv))

    if __name__ == '__main__':
        main()

This has the added benefit of the *echo* function itself being isolated and
importable elsewhere::

    # elsewhere.py

    import sys

    from echo import echo

    def echo_platform():
        echo(sys.platform)


Packaging Considerations (``console_scripts``)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For detailed documentation on Python packaging, see the
`Python Packaging User Guide. <https://packaging.python.org/>`__

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
later package it as an console script entry-point in a pip-installable package.
We can revise the :file:`echo.py` example from earlier to follow this
convention::

    def main() -> int:  # now, main returns an integer
        "Echo the input to standard output"
        print(shlex.join(sys.argv))
        return 0


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


Idiomatic Usage
^^^^^^^^^^^^^^^

..
   should the first paragraph of this section be removed entirely? I see that
   this suggestion conflicts with setuptools's docs, where they do use
   if __name__ == '__main__' in __main__.py files

      (https://setuptools.readthedocs.io/en/latest/userguide/entry_point.html)

   However, I still think that the suggestion makes sense at face value. This
   is my reasoning:

      It seems to me that it is almost always redundant, except in the case of
      console scripts where __name__ would be package.__main__. Even then,
      wouldn't you **not** want your code to be under a __name__ ==
      '__main__' block in that case? If it were, the code you'd want to run
      wouldn't run when invoked as a console script. To me, this seems like
      another reason to tell users _not_ to guard code in __main__.py under
      an if __name__ == '__main__' block. __main__.py should always run
      from top-to-bottom; is that not the case?


Note that it may not be necessary to use the ``if __name__ == '__main__'``
statement in ``__main__.py`` itself. There is no reason for any other file to
import something from ``__main__.py``. ``__main__.py`` will normally always be
executed as the main program; therefore, ``__name__`` will always be
``'__main__'``. There are exceptions to this norm, though. For example, if you
have explicitly identified ``__main__`` as a console script entry point in
:file:`setup.py`. See section :ref:`entry-points`.

For an example of a package using ``__main__.py`` in our standard library, see
:mod:`venv`, and its invocation via ``python3 -m venv [directory]``.
