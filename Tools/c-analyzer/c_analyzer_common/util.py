import subprocess 


_NOT_SET = object()


def run_cmd(argv):
    proc = subprocess.run(
            argv,
            #capture_output=True,
            #stderr=subprocess.STDOUT,
            stdout=subprocess.PIPE,
            text=True,
            check=True,
            )
    return proc.stdout


class Slot:
    """A descriptor that provides a slot.

    This is useful for types that can't have slots via __slots__,
    e.g. tuple subclasses.
    """

    __slots__ = ('initial', 'default', 'readonly', 'instances', 'name')

    def __init__(self, initial=_NOT_SET, *,
                 default=_NOT_SET,
                 readonly=False,
                 ):
        self.initial = initial
        self.default = default
        self.readonly = readonly

        self.instances = {}
        self.name = None

    def __set_name__(self, cls, name):
        if self.name is not None:
            raise TypeError('already used')
        self.name = name

    def __get__(self, obj, cls):
        if obj is None:  # called on the class
            return self
        try:
            value = self.instances[id(obj)]
        except KeyError:
            if self.initial is _NOT_SET:
                value = self.default
            else:
                value = self.initial
            self.instances[id(obj)] = value
        if value is _NOT_SET:
            raise AttributeError(self.name)
        # XXX Optionally make a copy?
        return value

    def __set__(self, obj, value):
        if self.readonly:
            raise AttributeError(f'{self.name} is readonly')
        # XXX Optionally coerce?
        self.instances[id(obj)] = value

    def __delete__(self, obj):
        if self.readonly:
            raise AttributeError(f'{self.name} is readonly')
        self.instances[id(obj)] = self.default

    def set(self, obj, value):
        """Update the cached value for an object.

        This works even if the descriptor is read-only.  This is
        particularly useful when initializing the object (e.g. in
        its __new__ or __init__).
        """
        self.instances[id(obj)] = value


class classonly:
    """A non-data descriptor that makes a value only visible on the class.

    This is like the "classmethod" builtin, but does not show up on
    instances of the class.  It may be used as a decorator.
    """

    def __init__(self, value):
        self.value = value
        self.getter = classmethod(value).__get__
        self.name = None

    def __set_name__(self, cls, name):
        if self.name is not None:
            raise TypeError('already used')
        self.name = name

    def __get__(self, obj, cls):
        if obj is not None:
            raise AttributeError(self.name)
        # called on the class
        return self.getter(None, cls)


class _NTBase:

    __slots__ = ()

    @classonly
    def from_raw(cls, raw):
        if not raw:
            return None
        elif isinstance(raw, cls):
            return raw
        elif isinstance(raw, str):
            return cls.from_string(raw)
        else:
            if hasattr(raw, 'items'):
                return cls(**raw)
            try:
                args = tuple(raw)
            except TypeError:
                pass
            else:
                return cls(*args)
        raise NotImplementedError

    @classonly
    def from_string(cls, value):
        """Return a new instance based on the given string."""
        raise NotImplementedError

    @classmethod
    def _make(cls, iterable):  # The default _make() is not subclass-friendly.
        return cls.__new__(cls, *iterable)

    # XXX Always validate?
    #def __init__(self, *args, **kwargs):
    #    self.validate()

    # XXX The default __repr__() is not subclass-friendly (where the name changes).
    #def __repr__(self):
    #    _, _, sig = super().__repr__().partition('(')
    #    return f'{self.__class__.__name__}({sig}'

    # To make sorting work with None:
    def __lt__(self, other):
        try:
            return super().__lt__(other)
        except TypeError:
            if None in self:
                return True
            elif None in other:
                return False
            else:
                raise

    def validate(self):
        return

    # XXX Always validate?
    #def _replace(self, **kwargs):
    #    obj = super()._replace(**kwargs)
    #    obj.validate()
    #    return obj
