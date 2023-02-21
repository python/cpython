try:
    from func_timeout import func_set_timeout as set_timeout
except ImportError:  # pragma: no cover
    # provide a fallback that doesn't actually time out
    from ._context import TimedContext as set_timeout


__all__ = ['set_timeout']
