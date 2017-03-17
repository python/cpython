Issue #26494: Fixed crash on iterating exhausting iterators.
Affected classes are generic sequence iterators, iterators of str, bytes,
bytearray, list, tuple, set, frozenset, dict, OrderedDict, corresponding
views and os.scandir() iterator.