def __getattr__(name):
    if name == "OptimizedUnicode":
        import warnings
        msg = ("""
            OptimizedUnicode is obsolete. You can safely remove it from your
            code, as it defaults to 'str' anyway.
        """)
        warnings.warn(msg, DeprecationWarning)
        return str
    raise AttributeError(f"module 'sqlite3' has no attribute '{name}'")
