"""Common code between queues and channels."""


class ItemInterpreterDestroyed(Exception):
    """Raised when trying to get an item whose interpreter was destroyed."""


UNBOUND = object()
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
