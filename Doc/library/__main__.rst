:mod:`__main__` --- CLIs, import-time behavior, and ``__name__ == ‘__main__’``
=================================================================================

.. module:: __main__
   :synopsis: CLIs, import-time behavior, and ``__name__ == ‘__main__’``

--------------

The concept of ``__main__`` in Python can be confusing at first. It is not a
single mechanism in the language, but in fact is part of two quite different
constructs:

1. The ``__main__.py`` file in a Python module; see :ref:`___main__.py`.
2. The very common ``__name__ == '__main__'`` construct.

The term ``__main__`` in both of these use cases does indeed point towards a
common design philosophy, which hopefully will become clear.  Nonetheless, each
will be documented separately in the following two sections.


.. ___main__.py:

``__main__.py`` in Python Packages
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you are not familiar with Python packages, see section :ref:`tut-packages`.
Most commonly, the :file:`__main__.py` file in a Python package is used to
provide a command line interface (CLI) for that package. Consider the
following hypothetical package, "bandclass"::

    project_root
    └── bandclass
        ├── __init__.py
        ├── __main__.py
        ├── parent.py
        ├── section.py
        └── student.py

``__main__.py`` will be excecuted when the package itself is invoked directly
from the command line using the ``-m`` flag. For example::

    python3 -m bandclass

In this example, ``__main__.py`` might be used to provide a CLI for the
teacher to get information. In combination with :mod:`argparse`,
``__main__.py`` becomes a beautiful way to add command line functionality
to your packages.

For a very popular example of this usage pattern in our standard library,
see :mod:`http.server`, and its' invocation via
``python3 -m http.server [directory]``









.. REDRAFT PLAN

There have been many complaints about the shortcoming of the documentation
towards informing users about __main__. Both the popular __name__ == '__main__' construct, and the role of __main__.py in a python module.

bpo-17359
bpo-24632
bpo-38452

I propose a broad overhaul of Doc/library/__main__.rst to address these
shortcomings and to provide a single source of truth on __main__ (in
general!). This is an appropriate place to put this information.
Both the __name__ == '__main__' and fooModule/__main__.py
constructs reasonably fall under the category of “Python Runtime Services,”
because they both control the way that programs run depending on how they are
used (command-line versus import versus running directly).

The new Doc/library/__main__.rst should have a new synopsis of, “CLIs,
import-time behavior, and if __name__ == ‘__main__’”, reflecting its new and
broader focus.

Additionally, the new docs should have the following distinct sections:

    Differentiating between __name__ == ‘__main__’ and __main.__.py
    __main__.py and the -m flag (this is roughly what is there already, although
    it’s not as descriptive as it should be).
    __name__ and the if __name__ == '__main__' construct.

If there is interest, I would be happy to open uptake this work on as soon as there is
consensus around this plan. I’m looking forward to hearing what you think!







.. OLD DOCUMENTATION


``'__main__'`` is the name of the scope in which top-level code executes.
A module's __name__ is set equal to ``'__main__'`` when read from
standard input, a script, or from an interactive prompt.

A module can discover whether or not it is running in the main scope by
checking its own ``__name__``, which allows a common idiom for conditionally
executing code in a module when it is run as a script or with ``python
-m`` but not when it is imported::

   if __name__ == "__main__":
       # execute only if run as a script
       main()

For a package, the same effect can be achieved by including a
``__main__.py`` module, the contents of which will be executed when the
module is run with ``-m``.
