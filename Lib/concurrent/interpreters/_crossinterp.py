"""Common code between queues and channels."""


class ItemInterpreterDestroyed(Exception):
    """Raised when trying to get an item whose interpreter was destroyed."""


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


class UnboundBehavior:
    __slots__ = ('name', '_unboundop')

    def __init__(self, name, *, _unboundop):
        self.name = name
        self._unboundop = _unboundop

    def __repr__(self):
        return f'<{self.name}>'


UNBOUND = UnboundBehavior('UNBOUND', _unboundop=3)
UNBOUND_ERROR = UnboundBehavior('UNBOUND_ERROR', _unboundop=2)
UNBOUND_REMOVE = UnboundBehavior('UNBOUND_REMOVE', _unboundop=1)


def unbound_to_flag(unbounditems):
    if unbounditems is None:
        return -1
    try:
        return unbounditems._unboundop
    except AttributeError:
        raise NotImplementedError(
            f'unsupported unbound replacement object {unbounditems!r}')

def resolve_unbound(flag, exctype_destroyed):
    if flag == UNBOUND_REMOVE._unboundop:
        # "remove" not possible here
        raise NotImplementedError
    elif flag == UNBOUND_ERROR._unboundop:
        raise exctype_destroyed("item's original interpreter destroyed")
    elif flag == UNBOUND._unboundop:
        return UNBOUND
    else:
        raise NotImplementedError(
            f'unsupported unbound replacement op {flag!r}')
