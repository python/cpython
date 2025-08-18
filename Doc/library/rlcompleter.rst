:mod:`!rlcompleter` --- Completion function for GNU readline
============================================================

.. module:: rlcompleter
   :synopsis: Python identifier completion, suitable for the GNU readline library.

.. sectionauthor:: Moshe Zadka <moshez@zadka.site.co.il>

**Source code:** :source:`Lib/rlcompleter.py`

--------------

The :mod:`!rlcompleter` module defines a completion function suitable to be
passed to :func:`~readline.set_completer` in the :mod:`readline` module.

When this module is imported on a Unix platform with the :mod:`readline` module
available, an instance of the :class:`Completer` class is automatically created
and its :meth:`~Completer.complete` method is set as the
:ref:`readline completer <readline-completion>`. The method provides
completion of valid Python :ref:`identifiers and keywords <identifiers>`.

Example::

   >>> import rlcompleter
   >>> import readline
   >>> readline.parse_and_bind("tab: complete")
   >>> readline. <TAB PRESSED>
   readline.__doc__          readline.get_line_buffer(  readline.read_init_file(
   readline.__file__         readline.insert_text(      readline.set_completer(
   readline.__name__         readline.parse_and_bind(
   >>> readline.

The :mod:`!rlcompleter` module is designed for use with Python's
:ref:`interactive mode <tut-interactive>`.  Unless Python is run with the
:option:`-S` option, the module is automatically imported and configured
(see :ref:`rlcompleter-config`).

On platforms without :mod:`readline`, the :class:`Completer` class defined by
this module can still be used for custom purposes.


.. _completer-objects:

.. class:: Completer

   Completer objects have the following method:

   .. method:: Completer.complete(text, state)

      Return the next possible completion for *text*.

      When called by the :mod:`readline` module, this method is called
      successively with ``state == 0, 1, 2, ...`` until the method returns
      ``None``.

      If called for *text* that doesn't include a period character (``'.'``), it will
      complete from names currently defined in :mod:`__main__`, :mod:`builtins` and
      keywords (as defined by the :mod:`keyword` module).

      If called for a dotted name, it will try to evaluate anything without obvious
      side-effects (functions will not be evaluated, but it can generate calls to
      :meth:`~object.__getattr__`) up to the last part, and find matches for the
      rest via the :func:`dir` function.  Any exception raised during the
      evaluation of the expression is caught, silenced and :const:`None` is
      returned.
