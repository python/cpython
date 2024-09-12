"""
Utilities for changing test behaviour while hunting
for refleaks
"""

_hunting_for_refleaks = False
def hunting_for_refleaks():
    return _hunting_for_refleaks
