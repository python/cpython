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


class UnboundItem:
    """Represents a cross-interpreter item no longer bound to an interpreter.

    An item is unbound when the interpreter that added it to the
    cross-interpreter container is destroyed.
    """

    __slots__ = ()

    @classonly
    def singleton(cls, kind, module, name='UNBOUND'):
        doc = cls.__doc__.replace('cross-interpreter container', kind)
        doc = doc.replace('cross-interpreter', kind)
        subclass = type(
            f'Unbound{kind.capitalize()}Item',
            (cls,),
            dict(
                _MODULE=module,
                _NAME=name,
                __doc__=doc,
            ),
        )
        return object.__new__(subclass)

    _MODULE = __name__
    _NAME = 'UNBOUND'

    def __new__(cls):
        raise Exception(f'use {cls._MODULE}.{cls._NAME}')

    def __repr__(self):
        return f'{self._MODULE}.{self._NAME}'
#        return f'interpreters.queues.UNBOUND'


UNBOUND = object.__new__(UnboundItem)
UNBOUND_ERROR = object()
UNBOUND_REMOVE = object()

_UNBOUND_CONSTANT_TO_FLAG = {
    UNBOUND_REMOVE: 1,
    UNBOUND_ERROR: 2,
    UNBOUND: 3,
}
_UNBOUND_FLAG_TO_CONSTANT = {v: k
                             for k, v in _UNBOUND_CONSTANT_TO_FLAG.items()}


def serialize_unbound(unbound):
    op = unbound
    try:
        flag = _UNBOUND_CONSTANT_TO_FLAG[op]
    except KeyError:
        raise NotImplementedError(f'unsupported unbound replacement op {op!r}')
    return flag,


def resolve_unbound(flag, exctype_destroyed):
    try:
        op = _UNBOUND_FLAG_TO_CONSTANT[flag]
    except KeyError:
        raise NotImplementedError(f'unsupported unbound replacement op {flag!r}')
    if op is UNBOUND_REMOVE:
        # "remove" not possible here
        raise NotImplementedError
    elif op is UNBOUND_ERROR:
        raise exctype_destroyed("item's original interpreter destroyed")
    elif op is UNBOUND:
        return UNBOUND
    else:
        raise NotImplementedError(repr(op))
