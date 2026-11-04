:mod:`!__future__` --- Future statement definitions
===================================================

.. module:: __future__
   :synopsis: Future statement definitions

**Source code:** :source:`Lib/__future__.py`

--------------

Imports of the form ``from __future__ import feature`` are called
:ref:`future statements <future>`. These are special-cased by the Python compiler
to allow the use of new Python features in modules containing the future statement
before the release in which the feature becomes standard.

While these future statements are given additional special meaning by the
Python compiler, they are still executed like any other import statement and
the :mod:`!__future__` exists and is handled by the import system the same way
any other Python module would be. This design serves three purposes:

* To avoid confusing existing tools that analyze import statements and expect to
  find the modules they're importing.

* To document when incompatible changes were introduced, and when they will be
  --- or were --- made mandatory.  This is a form of executable documentation, and
  can be inspected programmatically via importing :mod:`!__future__` and examining
  its contents.

* To ensure that :ref:`future statements <future>` run under releases prior to
  Python 2.1 at least yield runtime exceptions (the import of :mod:`!__future__`
  will fail, because there was no module of that name prior to 2.1).

Module Contents
---------------

No feature description will ever be deleted from :mod:`!__future__`. Since its
introduction in Python 2.1 the following features have found their way into the
language using this mechanism:


.. list-table::
   :widths: auto
   :header-rows: 1

   * * feature
     * optional in
     * mandatory in
     * effect
   * * .. data:: nested_scopes
     * 2.1.0b1
     * 2.2
     * :pep:`227`: *Statically Nested Scopes*
   * * .. data:: generators
     * 2.2.0a1
     * 2.3
     * :pep:`255`: *Simple Generators*
   * * .. data:: division
     * 2.2.0a2
     * 3.0
     * :pep:`238`: *Changing the Division Operator*
   * * .. data:: absolute_import
     * 2.5.0a1
     * 3.0
     * :pep:`328`: *Imports: Multi-Line and Absolute/Relative*
   * * .. data:: with_statement
     * 2.5.0a1
     * 2.6
     * :pep:`343`: *The “with” Statement*
   * * .. data:: print_function
     * 2.6.0a2
     * 3.0
     * :pep:`3105`: *Make print a function*
   * * .. data:: unicode_literals
     * 2.6.0a2
     * 3.0
     * :pep:`3112`: *Bytes literals in Python 3000*
   * * .. data:: generator_stop
     * 3.5.0b1
     * 3.7
     * :pep:`479`: *StopIteration handling inside generators*
   * * .. data:: annotations
     * 3.7.0b1
     * Never [1]_
     * :pep:`563`: *Postponed evaluation of annotations*,
       :pep:`649`: *Deferred evaluation of annotations using descriptors*

.. XXX Adding a new entry?  Remember to update simple_stmts.rst, too.

.. _future-classes:

.. class:: _Feature

   Each statement in :file:`__future__.py` is of the form::

      FeatureName = _Feature(OptionalRelease, MandatoryRelease,
                             CompilerFlag)

   where, normally, *OptionalRelease* is less than *MandatoryRelease*, and both are
   5-tuples of the same form as :data:`sys.version_info`::

      (PY_MAJOR_VERSION, # the 2 in 2.1.0a3; an int
       PY_MINOR_VERSION, # the 1; an int
       PY_MICRO_VERSION, # the 0; an int
       PY_RELEASE_LEVEL, # "alpha", "beta", "candidate" or "final"; string
       PY_RELEASE_SERIAL # the 3; an int
      )

.. method:: _Feature.getOptionalRelease()

   *OptionalRelease* records the first release in which the feature was accepted.

.. method:: _Feature.getMandatoryRelease()

   In the case of a *MandatoryRelease* that has not yet occurred,
   *MandatoryRelease* predicts the release in which the feature will become part of
   the language.

   Else *MandatoryRelease* records when the feature became part of the language; in
   releases at or after that, modules no longer need a future statement to use the
   feature in question, but may continue to use such imports.

   *MandatoryRelease* may also be ``None``, meaning that a planned feature got
   dropped or that it is not yet decided.

.. attribute:: _Feature.compiler_flag

   *CompilerFlag* is the (bitfield) flag that should be passed in the fourth
   argument to the built-in function :func:`compile` to enable the feature in
   dynamically compiled code.  This flag is stored in the :attr:`_Feature.compiler_flag`
   attribute on :class:`_Feature` instances.

.. [1]
   ``from __future__ import annotations`` was previously scheduled to
   become mandatory in Python 3.10, but the change was delayed and ultimately
   canceled. This feature will eventually be deprecated and removed. See
   :pep:`649` and :pep:`749`.


.. seealso::

   :ref:`future`
      How the compiler treats future imports.

   :pep:`236` - Back to the __future__
      The original proposal for the __future__ mechanism.
