"""Test module with __getattr__ for gh-144957."""

def __getattr__(name):
    """Provide dynamic attributes."""
    if name == "dynamic_attr":
        return "from_getattr"
    raise AttributeError(f"module has no attribute {name!r}")
