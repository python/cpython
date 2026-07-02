target = {'foo': 'FOO'}


def is_instance(obj, klass):
    """Version of is_instance that doesn't access __class__"""
    return issubclass(type(obj), klass)


class SomeClass(object):
    class_attribute = None

    def wibble(self): pass


class X(object):
    pass

# A standin for weurkzeug.local.LocalProxy - issue 119600
def _inaccessible(*args, **kwargs):
    raise AttributeError


class OpaqueProxy:
    __getattribute__ = _inaccessible


g = OpaqueProxy()
