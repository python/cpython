"""
PEP 713 Callable Module - __call__ = func
"""

def func(value: int = 1, *, power: int = 1):
    return (42 * value) ** power

__call__ = func
