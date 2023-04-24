"""
PEP 713 Callable Module - def __call__()
"""

def __call__(value: int = 1, *, power: int = 1):
    return (42 * value) ** power
