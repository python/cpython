Issue #27137: the pure Python fallback implementation of ``functools.partial``
now matches the behaviour of its accelerated C counterpart for subclassing,
pickling and text representation purposes. Patch by Emanuel Barry and
Serhiy Storchaka.