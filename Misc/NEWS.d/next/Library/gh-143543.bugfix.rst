Fix a crash in itertools.groupby that could occur when a user-defined
__eq__ method re-enters the iterator during key comparison.