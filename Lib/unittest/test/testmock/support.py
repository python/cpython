import concurrent.futures
import time


target = {'foo': 'FOO'}


def is_instance(obj, klass):
    """Version of is_instance that doesn't access __class__"""
    return issubclass(type(obj), klass)


class SomeClass(object):
    class_attribute = None

    def wibble(self): pass


class X(object):
    pass


def call_after_delay(func, /, *args, **kwargs):
    time.sleep(kwargs.pop('delay'))
    func(*args, **kwargs)


def run_async(func, /, *args, executor=None, delay=0, **kwargs):
    if executor is None:
        executor = concurrent.futures.ThreadPoolExecutor(max_workers=5)

    executor.submit(call_after_delay, func, *args, **kwargs, delay=delay)
