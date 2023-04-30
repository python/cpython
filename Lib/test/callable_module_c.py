"""
PEP 713 Callable Module - __call__ = func
"""

def func(value: int = 42, *, power: int = 1):
    return value ** power

def __getattr__(name: str):
    if name == "__call__":
        return func
    raise AttributeError
