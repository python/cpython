Issue #26802: Optimize function calls only using unpacking like
``func(*tuple)`` (no other positional argument, no keyword): avoid copying
the tuple. Patch written by Joe Jevnik.