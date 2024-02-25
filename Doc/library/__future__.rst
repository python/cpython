:mod:`__future__` --- Future statement definitions
==================================================

.. module:: __future__
   :synopsis: Future statement definitions

**Source code:** :source:`Lib/__future__.py`

--------------

To use a specific feature, import it from the :mod:`__future__` module using
the syntax ``from __future__ import <feature>``. It is important to note that
the ``__future__ import`` statement must be placed as the first statement in
the source file, except for a module docstring.

If a feature has been imported, the ``__future__ import`` remains valid even
after the *MandatoryRelease* version. However, it has no behavioural effect.
code will be compatible with multiple versions during the transition phase of
a feature. If code only supports versions greater than or equal to the
*MandatoryRelease* version, then no longer need the ``__future__ import`` for
that specific feature.

Once a feature has been imported, :ref:`future<future>` import remains valid
even after the *MandatoryRelease* version. However, it has no behavioral effect.
So this code will be compatible with multiple versions during the transition
phase of a feature. If code only supports versions greater than or equal to
the *MandatoryRelease* version, then no longer need to include the
:mod:`__future__` import for that specific feature.

.. _future-classes:

.. class:: _Feature

   Each statement in :file:`__future__.py` is of the form::

      FeatureName = _Feature(OptionalRelease, MandatoryRelease,
                             CompilerFlag)

   where, normally, *OptionalRelease* is less than *MandatoryRelease*, and both are
   5-tuples of the same form as :data:`sys.version_info`::

      (PY_MAJOR_VERSION,  # The 2 in 2.1.0a3; an int.
       PY_MINOR_VERSION,  # The 1; an int.
       PY_MICRO_VERSION,  # The 0; an int.
       PY_RELEASE_LEVEL,  # One of "alpha", "beta", "candidate" or "final"; string.
       PY_RELEASE_SERIAL  # The 3; an int.
      )

The release information for features is captured through *OptionalRelease* and
*MandatoryRelease*. The *OptionalRelease* records the first release in which the
feature was accepted, *MandatoryRelease* records when the feature became part
of the language. Once a feature is included in a *MandatoryRelease*, modules no
longer need a future statement to use the feature, but they may continue to use
such imports if needed.

.. method:: _Feature.getOptionalRelease()

   This method returns the release information for the OptionalRelease of a feature.

.. method:: _Feature.getMandatoryRelease()

   This method returns the release information for the MandatoryRelease of a feature.

.. attribute:: _Feature.compiler_flag

   The *CompilerFlag* is a bitfield flag that should be passed in the fourth
   argument to the built-in function :func:`compile` in order to enable the
   feature in dynamically compiled code. This flag is stored in the
   :attr:`_Feature.compiler_flag` attribute on :class:`_Feature` instances.

No feature description will ever be deleted from :mod:`__future__`. Since its
introduction in Python 2.1 the following features have found their way into the
language using this mechanism:

+------------------+-------------+--------------+---------------------------------------------+
| feature          | optional in | mandatory in | effect                                      |
+==================+=============+==============+=============================================+
| nested_scopes    | 2.1.0b1     | 2.2          | :pep:`227`:                                 |
|                  |             |              | *Statically Nested Scopes*                  |
+------------------+-------------+--------------+---------------------------------------------+
| generators       | 2.2.0a1     | 2.3          | :pep:`255`:                                 |
|                  |             |              | *Simple Generators*                         |
+------------------+-------------+--------------+---------------------------------------------+
| division         | 2.2.0a2     | 3.0          | :pep:`238`:                                 |
|                  |             |              | *Changing the Division Operator*            |
+------------------+-------------+--------------+---------------------------------------------+
| absolute_import  | 2.5.0a1     | 3.0          | :pep:`328`:                                 |
|                  |             |              | *Imports: Multi-Line and Absolute/Relative* |
+------------------+-------------+--------------+---------------------------------------------+
| with_statement   | 2.5.0a1     | 2.6          | :pep:`343`:                                 |
|                  |             |              | *The "with" Statement*                      |
+------------------+-------------+--------------+---------------------------------------------+
| print_function   | 2.6.0a2     | 3.0          | :pep:`3105`:                                |
|                  |             |              | *Make print a function*                     |
+------------------+-------------+--------------+---------------------------------------------+
| unicode_literals | 2.6.0a2     | 3.0          | :pep:`3112`:                                |
|                  |             |              | *Bytes literals in Python 3000*             |
+------------------+-------------+--------------+---------------------------------------------+
| generator_stop   | 3.5.0b1     | 3.7          | :pep:`479`:                                 |
|                  |             |              | *StopIteration handling inside generators*  |
+------------------+-------------+--------------+---------------------------------------------+
| annotations      | 3.7.0b1     | TBD [1]_     | :pep:`563`:                                 |
|                  |             |              | *Postponed evaluation of annotations*       |
+------------------+-------------+--------------+---------------------------------------------+

.. XXX Adding a new entry?  Remember to update simple_stmts.rst, too.

.. [1]
   ``from __future__ import annotations`` was previously scheduled to
   become mandatory in Python 3.10, but the Python Steering Council
   twice decided to delay the change
   (`announcement for Python 3.10 <https://mail.python.org/archives/list/python-dev@python.org/message/CLVXXPQ2T2LQ5MP2Y53VVQFCXYWQJHKZ/>`__;
   `announcement for Python 3.11 <https://mail.python.org/archives/list/python-dev@python.org/message/VIZEBX5EYMSYIJNDBF6DMUMZOCWHARSO/>`__).
   No final decision has been made yet. See also :pep:`563` and :pep:`649`.


.. seealso::

   :ref:`future`
      How the compiler treats future imports.
