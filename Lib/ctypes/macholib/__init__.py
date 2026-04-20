"""
Enough Mach-O to make your head spin.

See the relevant header files in /usr/include/mach-o

And also Apple's documentation.
"""

def __getattr__(name):
    if name == "__version__":
        from warnings import _deprecated

        _deprecated("__version__", remove=(3, 20))
        return "1.0"  # Do not change
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
