"""Utilities for with-statement contexts.  See PEP 343."""

import sys

__all__ = ["contextmanager", "nested", "closing"]

class GeneratorContextManager(object):
    """Helper for @contextmanager decorator."""

    def __init__(self, gen):
        self.gen = gen

    def __context__(self):
        return self

    def __enter__(self):
        try:
            return self.gen.next()
        except StopIteration:
            raise RuntimeError("generator didn't yield")

    def __exit__(self, type, value, traceback):
        if type is None:
            try:
                self.gen.next()
            except StopIteration:
                return
            else:
                raise RuntimeError("generator didn't stop")
        else:
            try:
                self.gen.throw(type, value, traceback)
            except StopIteration:
                pass


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
    def helper(*args, **kwds):
        return GeneratorContextManager(func(*args, **kwds))
    try:
        helper.__name__ = func.__name__
        helper.__doc__ = func.__doc__
    except:
        pass
    return helper


@contextmanager
def nested(*contexts):
    """Support multiple context managers in a single with-statement.

    Code like this:

        with nested(A, B, C) as (X, Y, Z):
            <body>

    is equivalent to this:

        with A as X:
            with B as Y:
                with C as Z:
                    <body>

    """
    exits = []
    vars = []
    try:
        try:
            for context in contexts:
                mgr = context.__context__()
                exit = mgr.__exit__
                enter = mgr.__enter__
                vars.append(enter())
                exits.append(exit)
            yield vars
        except:
            exc = sys.exc_info()
        else:
            exc = (None, None, None)
    finally:
        while exits:
            exit = exits.pop()
            try:
                exit(*exc)
            except:
                exc = sys.exc_info()
            else:
                exc = (None, None, None)
        if exc != (None, None, None):
            raise


@contextmanager
def closing(thing):
    """Context manager to automatically close something at the end of a block.

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
    try:
        yield thing
    finally:
        thing.close()
