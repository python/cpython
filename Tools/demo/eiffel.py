#!/usr/bin/env python3

"""
Support Eiffel-style preconditions and postconditions for functions.

An example for Python metaclasses.
"""

import unittest
from types import FunctionType as function

class EiffelBaseMetaClass(type):

    def __new__(meta, name, bases, dict):
        meta.convert_methods(dict)
        return super(EiffelBaseMetaClass, meta).__new__(
            meta, name, bases, dict)

    @classmethod
    def convert_methods(cls, dict):
        """Replace functions in dict with EiffelMethod wrappers.

        The dict is modified in place.

        If a method ends in _pre or _post, it is removed from the dict
        regardless of whether there is a corresponding method.
        """
        # find methods with pre or post conditions
        methods = []
        for k, v in dict.items():
            if k.endswith('_pre') or k.endswith('_post'):
                assert isinstance(v, function)
            elif isinstance(v, function):
                methods.append(k)
        for m in methods:
            pre = dict.get("%s_pre" % m)
            post = dict.get("%s_post" % m)
            if pre or post:
                dict[k] = cls.make_eiffel_method(dict[m], pre, post)


class EiffelMetaClass1(EiffelBaseMetaClass):
    # an implementation of the "eiffel" meta class that uses nested functions

    @staticmethod
    def make_eiffel_method(func, pre, post):
        def method(self, *args, **kwargs):
            if pre:
                pre(self, *args, **kwargs)
            rv = func(self, *args, **kwargs)
            if post:
                post(self, rv, *args, **kwargs)
            return rv

        if func.__doc__:
            method.__doc__ = func.__doc__

        return method


class EiffelMethodWrapper:

    def __init__(self, inst, descr):
        self._inst = inst
        self._descr = descr

    def __call__(self, *args, **kwargs):
        return self._descr.callmethod(self._inst, args, kwargs)


class EiffelDescriptor:

    def __init__(self, func, pre, post):
        self._func = func
        self._pre = pre
        self._post = post

        self.__name__ = func.__name__
        self.__doc__ = func.__doc__

    def __get__(self, obj, cls):
        return EiffelMethodWrapper(obj, self)

    def callmethod(self, inst, args, kwargs):
        if self._pre:
            self._pre(inst, *args, **kwargs)
        x = self._func(inst, *args, **kwargs)
        if self._post:
            self._post(inst, x, *args, **kwargs)
        return x


class EiffelMetaClass2(EiffelBaseMetaClass):
    # an implementation of the "eiffel" meta class that uses descriptors

    make_eiffel_method = EiffelDescriptor


class Tests(unittest.TestCase):

    def testEiffelMetaClass1(self):
        self._test(EiffelMetaClass1)

    def testEiffelMetaClass2(self):
        self._test(EiffelMetaClass2)

    def _test(self, metaclass):
        class Eiffel(metaclass=metaclass):
            pass

        class Test(Eiffel):
            def m(self, arg):
                """Make it a little larger"""
                return arg + 1

            def m2(self, arg):
                """Make it a little larger"""
                return arg + 1

            def m2_pre(self, arg):
                assert arg > 0

            def m2_post(self, result, arg):
                assert result > arg

        class Sub(Test):
            def m2(self, arg):
                return arg**2

            def m2_post(self, Result, arg):
                super(Sub, self).m2_post(Result, arg)
                assert Result < 100

        t = Test()
        self.assertEqual(t.m(1), 2)
        self.assertEqual(t.m2(1), 2)
        self.assertRaises(AssertionError, t.m2, 0)

        s = Sub()
        self.assertRaises(AssertionError, s.m2, 1)
        self.assertRaises(AssertionError, s.m2, 10)
        self.assertEqual(s.m2(5), 25)


if __name__ == "__main__":
    unittest.main()
