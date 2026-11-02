import collections
import functools


# from jaraco.functools 4.0.2
def save_method_args(method):
    """
    Wrap a method such that when it is called, the args and kwargs are
    saved on the method.
    """
    args_and_kwargs = collections.namedtuple('args_and_kwargs', 'args kwargs')  # noqa: PYI024

    @functools.wraps(method)
    def wrapper(self, /, *args, **kwargs):
        attr_name = '_saved_' + method.__name__
        attr = args_and_kwargs(args, kwargs)
        setattr(self, attr_name, attr)
        return method(self, *args, **kwargs)

    return wrapper


# from jaraco.functools 4.3
def none_as(value, replacement=None):
    """
    >>> none_as(None, 'foo')
    'foo'
    >>> none_as('bar', 'foo')
    'bar'
    """
    return replacement if value is None else value
