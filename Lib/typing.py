def __getattr__(name):
    if name == "TYPE_CHECKING":
        return False
    import _typing

    return getattr(_typing, name)
