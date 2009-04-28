:mod:`__future__` --- Future statement definitions
==================================================

.. module:: __future__
   :synopsis: Future statement definitions


:mod:`__future__` is a real module, and serves three purposes:

* To avoid confusing existing tools that analyze import statements and expect to
  find the modules they're importing.

* To ensure that future_statements run under releases prior to 2.1 at least
  yield runtime exceptions (the import of :mod:`__future__` will fail, because
  there was no module of that name prior to 2.1).

* To document when incompatible changes were introduced, and when they will be
  --- or were --- made mandatory.  This is a form of executable documentation, and
  can be inspected programmatically via importing :mod:`__future__` and examining
  its contents.

Each statement in :file:`__future__.py` is of the form::

   FeatureName = _Feature(OptionalRelease, MandatoryRelease,
                          CompilerFlag)


where, normally, *OptionalRelease* is less than *MandatoryRelease*, and both are
5-tuples of the same form as ``sys.version_info``::

   (PY_MAJOR_VERSION, # the 2 in 2.1.0a3; an int
    PY_MINOR_VERSION, # the 1; an int
    PY_MICRO_VERSION, # the 0; an int
    PY_RELEASE_LEVEL, # "alpha", "beta", "candidate" or "final"; string
    PY_RELEASE_SERIAL # the 3; an int
   )

*OptionalRelease* records the first release in which the feature was accepted.

In the case of a *MandatoryRelease* that has not yet occurred,
*MandatoryRelease* predicts the release in which the feature will become part of
the language.

Else *MandatoryRelease* records when the feature became part of the language; in
releases at or after that, modules no longer need a future statement to use the
feature in question, but may continue to use such imports.

*MandatoryRelease* may also be ``None``, meaning that a planned feature got
dropped.

Instances of class :class:`_Feature` have two corresponding methods,
:meth:`getOptionalRelease` and :meth:`getMandatoryRelease`.

*CompilerFlag* is the (bitfield) flag that should be passed in the fourth
argument to the builtin function :func:`compile` to enable the feature in
dynamically compiled code.  This flag is stored in the :attr:`compiler_flag`
attribute on :class:`_Feature` instances.

No feature description will ever be deleted from :mod:`__future__`.

.. seealso::

   :ref:`future`
      How the compiler treats future imports.
