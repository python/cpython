# B/W compat hack so code that says "import builtin" won't break after
# name change from builtin to __builtin__.
from __builtin__ import *
