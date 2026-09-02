import functools
import types
from contextlib import AbstractContextManager

from ._itertools import always_iterable


def parameterize(names, value_groups):
    """
    Decorate a test method to run it as a set of subtests.

    Modeled after pytest.parametrize.

    Context Manager types are entered and exited.
    """

    def decorator(func):
        @functools.wraps(func)
        def wrapped(self):
            for values in value_groups:
                resolved = map(Invoked.eval, always_iterable(values))
                params = dict(zip(always_iterable(names), resolved))
                for value in params.values():
                    if isinstance(value, AbstractContextManager):
                        self.enterContext(value)
                with self.subTest(**params):
                    func(self, **params)

        return wrapped

    return decorator


class Invoked(types.SimpleNamespace):
    """
    Wrap a function to be invoked for each usage.
    """

    @classmethod
    def wrap(cls, func):
        return cls(func=func)

    @classmethod
    def eval(cls, cand):
        return cand.func() if isinstance(cand, cls) else cand
