# Test that generic `property` works with future import.

from __future__ import annotations

p: property[int, str] = property()
