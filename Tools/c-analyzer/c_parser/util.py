import subprocess 


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
