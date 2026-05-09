def __getattr__(name):
    if name == "dynamic_attr":
        return "from_getattr"
    raise AttributeError(name)
