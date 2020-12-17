# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

import abc
import binascii
import inspect
import sys
import warnings


# We use a UserWarning subclass, instead of DeprecationWarning, because CPython
# decided deprecation warnings should be invisble by default.
class CryptographyDeprecationWarning(UserWarning):
    pass


# Several APIs were deprecated with no specific end-of-life date because of the
# ubiquity of their use. They should not be removed until we agree on when that
# cycle ends.
PersistentlyDeprecated2017 = CryptographyDeprecationWarning
PersistentlyDeprecated2019 = CryptographyDeprecationWarning


def _check_bytes(name, value):
    if not isinstance(value, bytes):
        raise TypeError("{} must be bytes".format(name))


def _check_byteslike(name, value):
    try:
        memoryview(value)
    except TypeError:
        raise TypeError("{} must be bytes-like".format(name))


def read_only_property(name):
    return property(lambda self: getattr(self, name))


def register_interface(iface):
    def register_decorator(klass):
        verify_interface(iface, klass)
        iface.register(klass)
        return klass

    return register_decorator


def register_interface_if(predicate, iface):
    def register_decorator(klass):
        if predicate:
            verify_interface(iface, klass)
            iface.register(klass)
        return klass

    return register_decorator


if hasattr(int, "from_bytes"):
    int_from_bytes = int.from_bytes
else:

    def int_from_bytes(data, byteorder, signed=False):
        assert byteorder == "big"
        assert not signed

        return int(binascii.hexlify(data), 16)


if hasattr(int, "to_bytes"):

    def int_to_bytes(integer, length=None):
        return integer.to_bytes(
            length or (integer.bit_length() + 7) // 8 or 1, "big"
        )


else:

    def int_to_bytes(integer, length=None):
        hex_string = "%x" % integer
        if length is None:
            n = len(hex_string)
        else:
            n = length * 2
        return binascii.unhexlify(hex_string.zfill(n + (n & 1)))


class InterfaceNotImplemented(Exception):
    pass


if hasattr(inspect, "signature"):
    signature = inspect.signature
else:
    signature = inspect.getargspec


def verify_interface(iface, klass):
    for method in iface.__abstractmethods__:
        if not hasattr(klass, method):
            raise InterfaceNotImplemented(
                "{} is missing a {!r} method".format(klass, method)
            )
        if isinstance(getattr(iface, method), abc.abstractproperty):
            # Can't properly verify these yet.
            continue
        sig = signature(getattr(iface, method))
        actual = signature(getattr(klass, method))
        if sig != actual:
            raise InterfaceNotImplemented(
                "{}.{}'s signature differs from the expected. Expected: "
                "{!r}. Received: {!r}".format(klass, method, sig, actual)
            )


class _DeprecatedValue(object):
    def __init__(self, value, message, warning_class):
        self.value = value
        self.message = message
        self.warning_class = warning_class


class _ModuleWithDeprecations(object):
    def __init__(self, module):
        self.__dict__["_module"] = module

    def __getattr__(self, attr):
        obj = getattr(self._module, attr)
        if isinstance(obj, _DeprecatedValue):
            warnings.warn(obj.message, obj.warning_class, stacklevel=2)
            obj = obj.value
        return obj

    def __setattr__(self, attr, value):
        setattr(self._module, attr, value)

    def __delattr__(self, attr):
        obj = getattr(self._module, attr)
        if isinstance(obj, _DeprecatedValue):
            warnings.warn(obj.message, obj.warning_class, stacklevel=2)

        delattr(self._module, attr)

    def __dir__(self):
        return ["_module"] + dir(self._module)


def deprecated(value, module_name, message, warning_class):
    module = sys.modules[module_name]
    if not isinstance(module, _ModuleWithDeprecations):
        sys.modules[module_name] = _ModuleWithDeprecations(module)
    return _DeprecatedValue(value, message, warning_class)


def cached_property(func):
    cached_name = "_cached_{}".format(func)
    sentinel = object()

    def inner(instance):
        cache = getattr(instance, cached_name, sentinel)
        if cache is not sentinel:
            return cache
        result = func(instance)
        setattr(instance, cached_name, result)
        return result

    return property(inner)
