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

The pure-Python reference implementation lives in ``_pydifflib``; this
module re-exports its public API.  Alternative Python implementations may
use ``_pydifflib`` directly as a self-contained reference.
"""

from _pydifflib import *  # noqa: F401, F403
from _pydifflib import __all__  # noqa: F401
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
