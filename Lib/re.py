"""Minimal "re" compatibility wrapper.  See "sre" for documentation."""

engine = "sre" # Some apps might use this undocumented variable

from sre import *
from sre import __all__
