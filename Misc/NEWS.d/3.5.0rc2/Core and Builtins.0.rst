- Issue #24769: Interpreter now starts properly when dynamic loading
  is disabled.  Patch by Petr Viktorin.

- Issue #21167: NAN operations are now handled correctly when python is
  compiled with ICC even if -fp-model strict is not specified.

- Issue #24492: A "package" lacking a __name__ attribute when trying to perform
  a ``from .. import ...`` statement will trigger an ImportError instead of an
  AttributeError.

