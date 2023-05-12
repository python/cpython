try:
    import hypothesis
except ImportError:
    from . import _hypothesis_stubs as hypothesis
