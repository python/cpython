"""Cilcular import involving a single-phase-init extension.

This module is imported from the _testsinglephase_circular module from
_testsinglephase, and imports that module again.
"""

import importlib
import _testsinglephase

name = '_testsinglephase_circular'
filename = _testsinglephase.__file__
loader = importlib.machinery.ExtensionFileLoader(name, filename)
spec = importlib.util.spec_from_file_location(name, filename, loader=loader)
mod = importlib._bootstrap._load(spec)
