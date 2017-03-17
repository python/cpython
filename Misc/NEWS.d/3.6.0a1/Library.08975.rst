Issue #26492: Exhausted iterator of array.array now conforms with the behavior
of iterators of other mutable sequences: it lefts exhausted even if iterated
array is extended.