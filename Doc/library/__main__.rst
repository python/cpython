:mod:`__main__` --- CLIs, import-time behavior, and ``__name__ == ‘__main__’``
=================================================================================

.. module:: __main__
   :synopsis: CLIs, import-time behavior, and ``__name__ == ‘__main__’``

.. sectionauthor:: Jack DeVries <jdevries3133@gmail.com>

--------------

The concept of ``__main__`` in Python can be confusing at first. It is not a
single mechanism in the language, but in fact is part of two quite different
constructs:

1. The ``__main__.py`` file in a Python packages
2. The ``__name__ == '__main__'`` construct

Each of these mechanisms are related to Python modules: both how users interact
with them as well as how they interact with each other. See section
:ref:`tut-modules`.


.. _main.py:

``__main__.py`` in Python Packages
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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
directly from the command line using the ``-m`` flag. For example::

    python3 -m bandclass

This command will cause ``__main__.py`` to run. How you utilize this
mechanism will depend on the nature of the package you are writing, but
in this hypothetical case, it might make sense to allow the teacher to search
for students or parents using :mod:`argparse`::

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


For a very popular example of this usage pattern in our standard library, see
:mod:`http.server`, and its' invocation via ``python3 -m http.server
[directory]``.


``__name__ == '__main__'``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``__name__`` is a special identifier which is always defined in Python. If a
Python module has been imported, ``__name__`` will be the same as the module
name (which is typically the file name without the ``.py`` stem). However, consider a
Python program being called directly from the command line like this::

   python3 bandclass/parent.py

In this case, ``__name__`` would be set to ``'__main__'`` inside of
``parent.py``. ``parent.py`` may have several functions defined inside of it,
but the ``__name__ == '__main__'`` statement can be used to test whether the
module has been invoked by the user directly.  In response, Python modules can
behave differently when they are executed directly from the command line::

   # bandclass/parent.py

   def list_all():
       return InformationSystem.get_all('parents')
   ...
   if __name__ == '__main__':
       print(list_all())

The print function will run when this file is invoked directly, but not
if it is imported in ``student.py`` like so::

   # bandclass/student.py

   from .parent import list_all

In Python, imports cause the target module to be executed in its entirety; but
inside of ``parent.py``, ``__name__ == 'parent'`` when the Python module is
being executed in the context of an import.  Therefore, the print statement in
the ``if __name__ == '__main__'`` block will not run.

For more information, see section :ref:`tut-modulesasscripts`.
