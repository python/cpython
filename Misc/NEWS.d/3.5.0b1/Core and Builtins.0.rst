- Issue #24276: Fixed optimization of property descriptor getter.

- Issue #24268: PEP 489: Multi-phase extension module initialization.
  Patch by Petr Viktorin.

- Issue #23955: Add pyvenv.cfg option to suppress registry/environment
  lookup for generating sys.path on Windows.

- Issue #24257: Fixed system error in the comparison of faked
  types.SimpleNamespace.

- Issue #22939: Fixed integer overflow in iterator object.  Patch by
  Clement Rouault.

- Issue #23985: Fix a possible buffer overrun when deleting a slice from
  the front of a bytearray and then appending some other bytes data.

- Issue #24102: Fixed exception type checking in standard error handlers.

- Issue #15027: The UTF-32 encoder is now 3x to 7x faster.

- Issue #23290: Optimize set_merge() for cases where the target is empty.
  (Contributed by Serhiy Storchaka.)

- Issue #2292: PEP 448: Additional Unpacking Generalizations.

- Issue #24096: Make warnings.warn_explicit more robust against mutation of the
  warnings.filters list.

- Issue #23996: Avoid a crash when a delegated generator raises an
  unnormalized StopIteration exception.  Patch by Stefan Behnel.

- Issue #23910: Optimize property() getter calls.  Patch by Joe Jevnik.

- Issue #23911: Move path-based importlib bootstrap code to a separate
  frozen module.

- Issue #24192: Fix namespace package imports.

- Issue #24022: Fix tokenizer crash when processing undecodable source code.

- Issue #9951: Added a hex() method to bytes, bytearray, and memoryview.

- Issue #22906: PEP 479: Change StopIteration handling inside generators.

- Issue #24017: PEP 492: Coroutines with async and await syntax.

