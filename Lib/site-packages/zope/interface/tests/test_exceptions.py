##############################################################################
#
# Copyright (c) 2010 Zope Foundation and Contributors.
# All Rights Reserved.
#
# This software is subject to the provisions of the Zope Public License,
# Version 2.1 (ZPL).  A copy of the ZPL should accompany this distribution.
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY AND ALL EXPRESS OR IMPLIED
# WARRANTIES ARE DISCLAIMED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF TITLE, MERCHANTABILITY, AGAINST INFRINGEMENT, AND FITNESS
# FOR A PARTICULAR PURPOSE.
#
##############################################################################
""" zope.interface.exceptions unit tests
"""
import unittest

def _makeIface():
    from zope.interface import Interface
    class IDummy(Interface):
        pass
    return IDummy

class DoesNotImplementTests(unittest.TestCase):

    def _getTargetClass(self):
        from zope.interface.exceptions import DoesNotImplement
        return DoesNotImplement

    def _makeOne(self, *args):
        iface = _makeIface()
        return self._getTargetClass()(iface, *args)

    def test___str__(self):
        dni = self._makeOne()
        self.assertEqual(
            str(dni),
            "An object has failed to implement interface "
            "<InterfaceClass zope.interface.tests.test_exceptions.IDummy>: "
            "Does not declaratively implement the interface."
        )

    def test___str__w_candidate(self):
        dni = self._makeOne('candidate')
        self.assertEqual(
            str(dni),
            "The object 'candidate' has failed to implement interface "
            "<InterfaceClass zope.interface.tests.test_exceptions.IDummy>: "
            "Does not declaratively implement the interface."
        )


class BrokenImplementationTests(unittest.TestCase):

    def _getTargetClass(self):
        from zope.interface.exceptions import BrokenImplementation
        return BrokenImplementation

    def _makeOne(self, *args):
        iface = _makeIface()
        return self._getTargetClass()(iface, 'missing', *args)

    def test___str__(self):
        dni = self._makeOne()
        self.assertEqual(
            str(dni),
            'An object has failed to implement interface '
            '<InterfaceClass zope.interface.tests.test_exceptions.IDummy>: '
            "The 'missing' attribute was not provided.")

    def test___str__w_candidate(self):
        dni = self._makeOne('candidate')
        self.assertEqual(
            str(dni),
            'The object \'candidate\' has failed to implement interface '
            '<InterfaceClass zope.interface.tests.test_exceptions.IDummy>: '
            "The 'missing' attribute was not provided.")


def broken_function():
    """
    This is a global function with a simple argument list.

    It exists to be able to report the same information when
    formatting signatures under Python 2 and Python 3.
    """


class BrokenMethodImplementationTests(unittest.TestCase):

    def _getTargetClass(self):
        from zope.interface.exceptions import BrokenMethodImplementation
        return BrokenMethodImplementation

    message = 'I said so'

    def _makeOne(self, *args):
        return self._getTargetClass()('aMethod', self.message, *args)

    def test___str__(self):
        dni = self._makeOne()
        self.assertEqual(
            str(dni),
            "An object has failed to implement interface <Unknown>: "
            "The contract of 'aMethod' is violated because I said so."
        )

    def test___str__w_candidate_no_implementation(self):
        dni = self._makeOne('some_function', '<IFoo>', 'candidate')
        self.assertEqual(
            str(dni),
            "The object 'candidate' has failed to implement interface <IFoo>: "
            "The contract of 'aMethod' is violated because I said so."
        )

    def test___str__w_candidate_w_implementation(self):
        self.message = 'implementation is wonky'
        dni = self._makeOne(broken_function, '<IFoo>', 'candidate')
        self.assertEqual(
            str(dni),
            "The object 'candidate' has failed to implement interface <IFoo>: "
            "The contract of 'aMethod' is violated because "
            "'broken_function()' is wonky."
        )

    def test___str__w_candidate_w_implementation_not_callable(self):
        self.message = 'implementation is not callable'
        dni = self._makeOne(42, '<IFoo>', 'candidate')
        self.assertEqual(
            str(dni),
            "The object 'candidate' has failed to implement interface <IFoo>: "
            "The contract of 'aMethod' is violated because "
            "'42' is not callable."
        )

    def test___repr__w_candidate(self):
        dni = self._makeOne(None, 'candidate')
        self.assertEqual(
            repr(dni),
            "BrokenMethodImplementation('aMethod', 'I said so', None, 'candidate')"
        )


class MultipleInvalidTests(unittest.TestCase):

    def _getTargetClass(self):
        from zope.interface.exceptions import MultipleInvalid
        return MultipleInvalid

    def _makeOne(self, excs):
        iface = _makeIface()
        return self._getTargetClass()(iface, 'target', excs)

    def test__str__(self):
        from zope.interface.exceptions import BrokenMethodImplementation
        excs = [
            BrokenMethodImplementation('aMethod', 'I said so'),
            Exception("Regular exception")
        ]
        dni = self._makeOne(excs)
        self.assertEqual(
            str(dni),
            "The object 'target' has failed to implement interface "
            "<InterfaceClass zope.interface.tests.test_exceptions.IDummy>:\n"
            "    The contract of 'aMethod' is violated because I said so\n"
            "    Regular exception"
        )

    def test__repr__(self):
        from zope.interface.exceptions import BrokenMethodImplementation
        excs = [
            BrokenMethodImplementation('aMethod', 'I said so'),
            # Use multiple arguments to normalize repr; versions of Python
            # prior to 3.7 add a trailing comma if there's just one.
            Exception("Regular", "exception")
        ]
        dni = self._makeOne(excs)
        self.assertEqual(
            repr(dni),
            "MultipleInvalid(<InterfaceClass zope.interface.tests.test_exceptions.IDummy>,"
            " 'target',"
            " (BrokenMethodImplementation('aMethod', 'I said so'),"
            " Exception('Regular', 'exception')))"
        )
