"""
Module difflib -- helpers for computing deltas between objects.

Function get_close_matches(word, possibilities, n=3, cutoff=0.6):
    Use SequenceMatcher to return list of the best "good enough" matches.

Function context_diff(a, b):
    For two lists of strings, return a delta in context diff format.

Function ndiff(a, b):
    Return a delta: the difference between `a` and `b` (lists of strings).

Function restore(delta, which):
    Return one of the two sequences that generated an ndiff delta.

Function unified_diff(a, b):
    For two lists of strings, return a delta in unified diff format.

Class SequenceMatcher:
    A flexible class for comparing pairs of sequences of any type.

Class Differ:
    For producing human-readable deltas from sequences of lines of text.

Class HtmlDiff:
    For producing HTML side by side comparison with change highlights.

This module dispatches to a faster C-coded SequenceMatcher (the
``_difflib`` accelerator module) when available, falling back to the
pure-Python reference implementation in ``_pydifflib``.  The pure-Python
module is preserved so that alternative Python implementations have a
self-contained reference; CPython prefers the C version automatically.
"""

from _pydifflib import *  # noqa: F401, F403
from _pydifflib import __all__  # noqa: F401
from _pydifflib import SequenceMatcher as _PySequenceMatcher
# Private helpers referenced by the test suite and (potentially) by other
# stdlib callers; re-exported to keep ``difflib.X`` working transparently.
from _pydifflib import (  # noqa: F401
    _calculate_ratio,
    _format_range_context,
    _format_range_unified,
    _mdiff,
)
# can_colorize / get_theme were lazily-imported names on the original
# Lib/difflib.py module; re-export them here so they remain runtime
# attributes of ``difflib`` (test_pyclbr.test_easy verifies this) while
# keeping the lazy contract (test_difflib.LazyImportTest verifies that
# importing difflib does not import ``_colorize``).
lazy from _pydifflib import can_colorize, get_theme  # noqa: F401
from _pydifflib import SequenceMatcher as _PySequenceMatcher
from types import GenericAlias as _GenericAlias

# Use the C-accelerated SequenceMatcher when available.  The C type covers
# the hot methods (__init__, set_seqs/set_seq1/set_seq2, find_longest_match,
# get_matching_blocks, get_opcodes, ratio); the slow-path methods that the
# rest of the module needs (quick_ratio, real_quick_ratio,
# get_grouped_opcodes) are inherited from the pure-Python class.
try:
    # Imported under its own name (not aliased) so pyclbr's static analysis
    # sees the subclass's base as ``SequenceMatcher`` -- matching the
    # runtime ``__bases__[0].__name__`` from the C type.
    from _difflib import SequenceMatcher
except ImportError:
    pass
else:
    class SequenceMatcher(SequenceMatcher):  # noqa: F811
        __doc__ = _PySequenceMatcher.__doc__
        __class_getitem__ = classmethod(_GenericAlias)

        # Forward the pure-Python slow-path methods.  These are defined as
        # ``def``s (rather than direct attribute assignments) so the source
        # parser used by pyclbr sees them as methods of this class --
        # otherwise test_pyclbr.test_easy reports them as missing.
        def quick_ratio(self):
            return _PySequenceMatcher.quick_ratio(self)

        def real_quick_ratio(self):
            return _PySequenceMatcher.real_quick_ratio(self)

        def get_grouped_opcodes(self, n=3):
            return _PySequenceMatcher.get_grouped_opcodes(self, n)

    # Re-bind the name inside _pydifflib so the helper functions defined
    # there (unified_diff, context_diff, ndiff, get_close_matches, Differ,
    # HtmlDiff) -- which look up ``SequenceMatcher`` in their own module's
    # globals -- pick up the C-accelerated subclass instead of the
    # pure-Python class.  Without this rebind, ``difflib.unified_diff`` would
    # see no speedup from the accelerator even though ``difflib.SequenceMatcher``
    # itself is the C class.
    import _pydifflib as _pyd
    _pyd.SequenceMatcher = SequenceMatcher
    del _pyd
