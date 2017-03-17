- Issue #23573: Increased performance of string search operations (str.find,
  str.index, str.count, the in operator, str.split, str.partition) with
  arguments of different kinds (UCS1, UCS2, UCS4).

- Issue #23753: Python doesn't support anymore platforms without stat() or
  fstat(), these functions are always required.

- Issue #23681: The -b option now affects comparisons of bytes with int.

- Issue #23632: Memoryviews now allow tuple indexing (including for
  multi-dimensional memoryviews).

- Issue #23192: Fixed generator lambdas.  Patch by Bruno Cauet.

- Issue #23629: Fix the default __sizeof__ implementation for variable-sized
  objects.

