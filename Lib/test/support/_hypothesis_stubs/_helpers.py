# Stub out only the subset of the interface that we actually use in our tests.
class StubClass:
    def __init__(self, *args, **kwargs):
        self.__stub_args = args
        self.__stub_kwargs = kwargs

    def __repr__(self):
        argstr = ", ".join(self.__stub_args)
        kwargstr = ", ".join(
            f"{kw}={val}" for kw, val in self.__stub_kwargs.items()
        )

        in_parens = argstr
        if kwargstr:
            in_parens += ", " + kwargstr

        return f"{self.__qualname__}({in_parens})"


def stub_factory(klass, name, _seen={}):
    if (klass, name) not in _seen:

        class Stub(klass):
            def __init__(self, *args, **kwargs):
                super().__init__()
                self.__stub_args = args
                self.__stub_kwargs = kwargs

        Stub.__name__ = name
        Stub.__qualname__ = name
        _seen.setdefault((klass, name), Stub)

    return _seen[(klass, name)]
