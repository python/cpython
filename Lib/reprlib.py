"""Redo the builtin repr() (representation) but with limits on most sizes."""

__all__ = ["Repr", "repr", "recursive_repr"]

import builtins
from itertools import islice
from _thread import get_ident

# Note: Added sort_dicts parameter to Repr to allow preserving insertion order (gh-155812)
