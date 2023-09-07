# Stub out only the subset of the interface that we actually use in our tests.
class StubClass:
    def __init__(self, *args, **kwargs):
        self.__stub_args = args
        self.__stub_kwargs = kwargs
        self.__repr = None

    def _with_repr(self, new_repr):
        new_obj = self.__class__(*self.__stub_args, **self.__stub_kwargs)
        new_obj.__repr = new_repr
        return new_obj

    def __repr__(self):
        if self.__repr is not None:
            return self.__repr

        argstr = ", ".join(self.__stub_args)
        kwargstr = ", ".join(f"{kw}={val}" for kw, val in self.__stub_kwargs.items())

        in_parens = argstr
        if kwargstr:
            in_parens += ", " + kwargstr

        return f"{self.__class__.__qualname__}({in_parens})"


def stub_factory(klass, name, *, with_repr=None, _seen={}):
    if (klass, name) not in _seen:

        class Stub(klass):
            def __init__(self, *args, **kwargs):
                super().__init__()
                self.__stub_args = args
                self.__stub_kwargs = kwargs

        Stub.__name__ = name
        Stub.__qualname__ = name
        if with_repr is not None:
            Stub._repr = None

        _seen.setdefault((klass, name, with_repr), Stub)

    return _seen[(klass, name, with_repr)]
