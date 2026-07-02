def __getattr__(name):
    if name == "dynamic_attr":
        return "from_getattr"
    elif name == "raising_attr":
        raise ValueError("from_getattr")
    elif name == "import_error_attr":
        raise ImportError(name)
    elif name == "warning_attr":
        import warnings
        warnings.warn("from_getattr", category=UserWarning)
        return "from_warning_attr"
    raise AttributeError(name)
