
_NOT_SET = object()


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

        # The instance cache is not inherently tied to the normal
        # lifetime of the instances.  So must do something in order to
        # avoid keeping the instances alive by holding a reference here.
        # Ideally we would use weakref.WeakValueDictionary to do this.
        # However, most builtin types do not support weakrefs.  So
        # instead we monkey-patch __del__ on the attached class to clear
        # the instance.
        self.instances = {}
        self.name = None

    def __set_name__(self, cls, name):
        if self.name is not None:
            raise TypeError('already used')
        self.name = name
        try:
            slotnames = cls.__slot_names__
        except AttributeError:
            slotnames = cls.__slot_names__ = []
        slotnames.append(name)
        self._ensure___del__(cls, slotnames)

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
        self.instances[id(obj)] = self.default  # XXX refleak?

    def _ensure___del__(self, cls, slotnames):  # See the comment in __init__().
        try:
            old___del__ = cls.__del__
        except AttributeError:
            old___del__ = (lambda s: None)
        else:
            if getattr(old___del__, '_slotted', False):
                return

        def __del__(_self):
            for name in slotnames:
                delattr(_self, name)
            old___del__(_self)
        __del__._slotted = True
        cls.__del__ = __del__

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
