"""Utilities for with-statement contexts.  See PEP 343."""

import sys
from functools import wraps
from warnings import warn

__all__ = ["contextmanager", "closing", "ContextDecorator"]


class ContextDecorator(object):
    "A base class or mixin that enables context managers to work as decorators."
    def __call__(self, func):
        @wraps(func)
        def inner(*args, **kwds):
            with self:
                return func(*args, **kwds)
        return inner


class _GeneratorContextManager(ContextDecorator):
    """Helper for @contextmanager decorator."""

    def __init__(self, gen):
        self.gen = gen

    def __enter__(self):
        try:
            return next(self.gen)
        except StopIteration:
            raise RuntimeError("generator didn't yield")

    def __exit__(self, type, value, traceback):
        if type is None:
            try:
                next(self.gen)
            except StopIteration:
                return
            else:
                raise RuntimeError("generator didn't stop")
        else:
            if value is None:
                # Need to force instantiation so we can reliably
                # tell if we get the same exception back
                value = type()
            try:
                self.gen.throw(type, value, traceback)
                raise RuntimeError("generator didn't stop after throw()")
            except StopIteration as exc:
                # Suppress the exception *unless* it's the same exception that
                # was passed to throw().  This prevents a StopIteration
                # raised inside the "with" statement from being suppressed
                return exc is not value
            except:
                # only re-raise if it's *not* the exception that was
                # passed to throw(), because __exit__() must not raise
                # an exception unless __exit__() itself failed.  But throw()
                # has to raise the exception to signal propagation, so this
                # fixes the impedance mismatch between the throw() protocol
                # and the __exit__() protocol.
                #
                if sys.exc_info()[1] is not value:
                    raise


def contextmanager(func):
    """@contextmanager decorator.

    Typical usage:

        @contextmanager
        def some_generator(<arguments>):
            <setup>
            try:
                yield <value>
            finally:
                <cleanup>

    This makes this:

        with some_generator(<arguments>) as <variable>:
            <body>

    equivalent to this:

        <setup>
        try:
            <variable> = <value>
            <body>
        finally:
            <cleanup>

    """
    @wraps(func)
    def helper(*args, **kwds):
        return _GeneratorContextManager(func(*args, **kwds))
    return helper


class closing(object):
    """Context to automatically close something at the end of a block.

    Code like this:

        with closing(<module>.open(<arguments>)) as f:
            <block>

    is equivalent to this:

        f = <module>.open(<arguments>)
        try:
            <block>
        finally:
            f.close()

    """
    def __init__(self, thing):
        self.thing = thing
    def __enter__(self):
        return self.thing
    def __exit__(self, *exc_info):
        self.thing.close()
