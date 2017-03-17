Issue #26167: Minimized overhead in copy.copy() and copy.deepcopy().
Optimized copying and deepcopying bytearrays, NotImplemented, slices,
short lists, tuples, dicts, sets.